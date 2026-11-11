"""Local/CI entry point for isolated shipped-game verification; never publishes."""
import json
import os
from pathlib import Path
import subprocess

from .archive import digest, encoded, inspect_archive, regular, require, validate_bundle, writer

ROOT = Path(__file__).resolve().parents[2]
RECIPE = ROOT / 'packaging/linux'


def run(args, log):
    print('+ ' + ' '.join(str(a) for a in args), flush=True)
    with log.open('w') as output:
        subprocess.run([str(a) for a in args], check=True, stdout=output, stderr=subprocess.STDOUT)


def capture(args):
    return subprocess.check_output([str(a) for a in args], text=True).strip()


def runtime_image(runtime, logs):
    recipe = regular(RECIPE / runtime['dockerfile'])
    identity = digest(recipe) + digest(RECIPE / 'bootstrap-ca.pem')
    identity += ''.join(digest(regular(RECIPE / name)) for name in runtime.get('inputs', []))
    import hashlib
    identity = hashlib.sha256(identity.encode()).hexdigest()
    tag = 'blobber-runtime-' + runtime['name'] + ':' + identity[:20]
    try:
        image = capture(['docker', 'image', 'inspect', '--format', '{{.Id}}', tag])
    except subprocess.CalledProcessError:
        run(['nice', '-n', '15', 'ionice', '-c', '3', 'docker', 'build', '--force-rm', '--cpu-period=100000', '--cpu-quota=200000',
             '--memory=4g', '--platform',
             'linux/amd64', '-f', recipe, '-t', tag, RECIPE], logs / (runtime['name'] + '-image.log'))
        image = capture(['docker', 'image', 'inspect', '--format', '{{.Id}}', tag])
    return image, identity


def run_runtime(runtime, workspace, package):
    image, recipe_hash = runtime_image(runtime, workspace)
    data = workspace / runtime['name']
    data.mkdir()
    args = ['nice', '-n', '15', 'ionice', '-c', '3', 'docker', 'run', '--rm', '--init',
            '--network=none', '--read-only', '--cpus=2', '--memory=10g', '--pids-limit=256',
            '--cap-drop=ALL', '--security-opt=no-new-privileges', '--shm-size=256m',
            '--tmpfs', '/tmp:rw,nosuid,nodev,size=128m,mode=1777',
            '--user', str(os.getuid()) + ':' + str(os.getgid()),
            '--mount', 'type=bind,src=' + str(package) + ',dst=/opt/blobber,readonly',
            '--mount', 'type=bind,src=' + str(data) + ',dst=/user-data',
            '--mount', 'type=bind,src=' + str(ROOT / 'tools/distribution/runtime_smoke.py') +
            ',dst=/verification.py,readonly', '--workdir', '/work']
    for key, value in {'HOME': '/user-data/home', 'XDG_CONFIG_HOME': '/user-data/config',
                       'XDG_DATA_HOME': '/user-data/data', 'XDG_CACHE_HOME': '/user-data/cache',
                       'TMPDIR': '/user-data/tmp', 'DISPLAY': ':93', 'LIBGL_ALWAYS_SOFTWARE': '1',
                       'GALLIUM_DRIVER': 'llvmpipe', 'MESA_DEBUG': 'context', 'LP_NUM_THREADS': '2', 'OMP_NUM_THREADS': '1',
                       'OPENBLAS_NUM_THREADS': '1', 'BLOBBER_HOST_CHECKOUT': str(ROOT)}.items():
        args += ['--env', key + '=' + value]
    args += [image, 'nice', '-n', '15', 'ionice', '-c', '3', 'python3', '/verification.py']
    run(args, workspace / (runtime['name'] + '-driver.log'))
    result = json.loads((data / 'runtime.json').read_text())
    result['image_id'], result['recipe_sha256'] = image, recipe_hash
    # Detailed mount topology is local diagnostic evidence; keep compact durable results.
    result.pop('mountinfo', None)
    return result


def verification_record(checksum, runtimes):
    require(set(runtimes) == {'ubuntu22', 'debian13'}, 'Both required runtime results must be present')
    for name, result in runtimes.items():
        require(result.get('status') == 'passed' and set(result.get('phases', {})) == {'create', 'continue'},
                'Incomplete runtime: ' + name)
        for phase in ('create', 'continue'):
            require(result['phases'][phase].get('status') == 'passed', 'Failed smoke phase')
    return {'version': 1, 'status': 'passed', 'archive_sha256': checksum, 'runtimes': runtimes,
            'limits': ['Software OpenGL only', 'No physical GPU or human visual review',
                       'Audio references only; no device playback or listening',
                       'Containers share the host kernel', 'Notice gaps remain; no publication clearance']}


def check_bundle_location(bundle):
    # Validate ownership and all output ancestors before opening a lock or removing
    # an earlier result. In particular, a bundle symlink must never redirect unlink.
    require(bundle.parent.name == 'dist' and bundle.parent.parent.parent == ROOT and
            bundle.parent.parent.name.startswith('build-linux-baseline'), 'Expected baseline dist bundle')
    require(bundle.is_dir() and not bundle.is_symlink(), 'Unsafe/missing package directory')
    owner = json.loads(regular(bundle.parent / '.blobber-dist-owner.json').read_text())
    require(owner == {'version': 1, 'build': '/build'}, 'Distribution owner mismatch')
    baseline = json.loads(regular(bundle.parent.parent / '.baseline-owner.json').read_text())
    require(baseline.get('baseline') == 'ubuntu22.04-amd64', 'Managed baseline build required')


def verify_package(run_dir, bundle):
    bundle = Path(bundle)
    check_bundle_location(bundle)
    with writer(bundle.parent.parent):
        return _verify_package(run_dir, bundle)


def _verify_package(run_dir, bundle):
    run_dir, bundle = Path(run_dir), Path(bundle)
    verification = bundle / 'verification.json'
    verification.unlink(missing_ok=True)
    archive, checksum, summary = validate_bundle(bundle)
    output = regular(bundle / 'build.json')
    require(json.loads(output.read_text())['baseline']['baseline'] == 'ubuntu22.04-amd64', 'Nonbaseline package')
    workspace = run_dir / 'package-verification'
    require(not workspace.exists(), 'Verification workspace already exists; use a fresh owned run')
    workspace.mkdir()
    extracted = workspace / 'extracted'
    inspect_archive(archive, extracted)
    package = extracted / summary['name']
    runtimes = {}
    specification = json.loads((RECIPE / 'runtime-images.json').read_text())
    for runtime in specification['runtimes']:
        runtimes[runtime['name']] = run_runtime(runtime, workspace, package)
    # Validate immutable extracted payload as well as the archive itself after both runs.
    for name, record in summary['manifest']['files'].items():
        path = regular(package / name)
        require(path.stat().st_size == record['bytes'] and digest(path) == record['sha256'],
                'Installed content changed: ' + name)
    require(digest(archive) == checksum, 'Archive changed during verification')
    record = verification_record(checksum, runtimes)
    record.update({'verification_driver_sha256': digest(ROOT / 'tools/distribution/runtime_smoke.py'),
                   'archive_bytes': archive.stat().st_size, 'unpacked_bytes': summary['unpacked_bytes'],
                   'files': summary['files']})
    temporary = bundle / '.verification.json.tmp'
    temporary.write_bytes(encoded(record))
    temporary.replace(verification)
    print('Relocated package verification passed:', verification, flush=True)
