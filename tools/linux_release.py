#!/usr/bin/env python3
"""Build, package and verify Linux distributions without native CLion trees."""
import argparse
from contextlib import contextmanager
import fcntl
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
RECIPE = ROOT / 'packaging/linux'
TESTS = ['runtime_inventory', 'dist_staging', 'options', 'packaged_options', 'npc_metadata',
         'dialogue', 'audio', 'music', 'blueprints', 'library', 'campaign_save_smoke',
         'linux_distribution_tools', 'distribution_archive', 'distribution_smoke',
         'menu', 'gameplay_startup', 'commands', 'buttons', 'hosted_distribution_tools']
TARGETS = ['blobber_distribution_tests', 'blobber_options_tests', 'blobber_packaged_options_tests',
           'blobber_npc_metadata_tests', 'blobber_dialogue_tests', 'blobber_audio_tests',
           'blobber_music_tests', 'blobber_blueprint_tests', 'blobber_library_tests',
           'blobber_campaign_save_tests', 'blobber_distribution_smoke_tests', 'blobber_menu_tests',
           'blobber_gameplay_startup_tests', 'blobber_commands_tests', 'blobber_buttons_tests']


def sha(path):
    value = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            value.update(block)
    return value.hexdigest()


def source_identity():
    listed = subprocess.check_output(['git', '-C', str(ROOT), 'ls-files', '-z', '--cached',
                                      '--others', '--exclude-standard']).split(b'\0')
    files = {}
    for raw in sorted(set(listed) - {b''}):
        name = os.fsdecode(raw)
        path = ROOT / name
        if path.is_symlink():
            raise RuntimeError('Source links are not supported by release capture: ' + name)
        files[name] = sha(path) if path.exists() else None
    revision = capture(['git', '-C', ROOT, 'rev-parse', 'HEAD'])
    status = capture(['git', '-C', ROOT, 'status', '--porcelain', '--untracked-files=normal'])
    return {'revision': revision, 'dirty': bool(status), 'source_status': status.splitlines(),
            'source_files': files,
            'source_fingerprint': hashlib.sha256(json.dumps(files, sort_keys=True).encode()).hexdigest(),
            'source_epoch': int(capture(['git', '-C', ROOT, 'show', '-s', '--format=%ct', 'HEAD']))}



def recipe_identity():
    files = ['Dockerfile', 'lock.json', 'provision.py', 'bootstrap-ca.pem',
             'build-ubuntu22.debs.tsv', 'build-ubuntu22-lock.json']
    return hashlib.sha256(json.dumps({name: sha(RECIPE / name) for name in files},
                                    sort_keys=True).encode()).hexdigest()


def validate_recipe():
    lock = json.loads((RECIPE / 'lock.json').read_text())
    recipe = (RECIPE / 'Dockerfile').read_text()
    if 'FROM ' + lock['base_image'] not in recipe or lock['snapshot'] not in recipe:
        raise RuntimeError('Recipe/lock mismatch')
    if sha(RECIPE / 'bootstrap-ca.pem') != lock['bootstrap_ca']['sha256']:
        raise RuntimeError('Bootstrap CA checksum mismatch')
    validate_package_transport(RECIPE)
    # The original CMake FetchContent pins remain the dependency source of truth.
    cmake = (ROOT / 'CMakeLists.txt').read_text()
    for name in ('raylib', 'nlohmann_json'):
        if lock['sources'][name]['sha256'] not in cmake:
            raise RuntimeError('CMake/source-lock mismatch: ' + name)
    if 'f7320141120f720aecc4c32be25586e7da9eb978' not in cmake:
        raise RuntimeError('yaml-cpp commit mismatch')


def validate_package_transport(recipe):
    from urllib.parse import urlsplit
    transport = json.loads((recipe / 'build-ubuntu22-lock.json').read_text())
    lock = json.loads((recipe / 'lock.json').read_text())
    ledger = recipe / 'build-ubuntu22.debs.tsv'
    if (transport.get('version') != 1 or transport['base_image'] != lock['base_image'] or
        transport['ledger_sha256'] != sha(ledger) or transport['inventory_sha256'] != sha(recipe / 'packages.tsv')):
        raise RuntimeError('Baseline package transport lock differs')
    names = set()
    for line in ledger.read_text().splitlines():
        checksum, url, filename = line.split('\t')
        parsed = urlsplit(url)
        if (not re.fullmatch('[0-9a-f]{64}', checksum) or parsed.scheme not in ('http','https') or
            parsed.netloc not in ('archive.ubuntu.com','security.ubuntu.com') or not parsed.path.startswith('/ubuntu/pool/') or
            not re.fullmatch(r'[A-Za-z0-9+.%_~:-]+[.]deb', filename) or filename in names):
            raise RuntimeError('Unsafe/duplicate baseline package transport entry')
        names.add(filename)
    if len(names) != transport['packages']:
        raise RuntimeError('Baseline package transport count differs')


def run_command(args, log):
    print('+ ' + ' '.join(str(arg) for arg in args), flush=True)
    with log.open('w') as stream:
        subprocess.run([str(arg) for arg in args], check=True, stdout=stream, stderr=subprocess.STDOUT)


def capture(args):
    return subprocess.check_output([str(arg) for arg in args], text=True).strip()


@contextmanager
def build_lock(build):
    build.mkdir(parents=True, exist_ok=True)
    with (build / '.baseline.lock').open('a') as stream:
        try:
            fcntl.flock(stream, fcntl.LOCK_EX | fcntl.LOCK_NB)
        except BlockingIOError as error:
            raise RuntimeError('Baseline build already has a writer') from error
        yield


def check_build_identity(build, identity):
    marker = build / '.baseline-owner.json'
    if marker.is_file():
        if json.loads(marker.read_text()) != identity:
            raise RuntimeError('Baseline build identity changed; choose a new --build-dir, preserving the old cache')
    else:
        if any(path.name != '.baseline.lock' for path in build.iterdir()):
            raise RuntimeError('Refusing unowned nonempty baseline build directory')
        marker.write_text(json.dumps(identity, indent=2) + '\n')


def preparation_input(bundle):
    if bundle is None:
        return None
    from distribution import prepared
    bundle = Path(bundle).absolute()
    manifest = prepared.validate(bundle, ROOT)
    if not manifest['producer_environment'] or not manifest['identities']:
        raise RuntimeError('Missing prepared producer evidence')
    return {k: manifest[k] for k in ('producer_revision', 'input_digest', 'producer_environment', 'identities')}


def _build_release(run, build, audit_only=False, dist=False, allow_dirty=False, prepared_input=None):
    validate_recipe()
    recipe_id = recipe_identity()
    tag = 'blobber-baseline:' + recipe_id[:20]
    try:
        image = capture(['docker', 'image', 'inspect', '--format', '{{.Id}}', tag])
    except subprocess.CalledProcessError:
        run_command(['nice', '-n', '15', 'ionice', '-c', '3', 'docker', 'build', '--platform',
                     'linux/amd64', '-t', tag, RECIPE], run / 'image-build.log')
        image = capture(['docker', 'image', 'inspect', '--format', '{{.Id}}', tag])
    identity = {'version': 1, 'recipe_sha256': recipe_id, 'image_id': image,
                'source_root': str(ROOT), 'configuration': 'Release', 'baseline': 'ubuntu22.04-amd64',
                'configuration_sha256': sha(ROOT / 'cmake/LinuxBaseline.cmake')}
    publication = preparation_input(prepared_input)
    if publication:
        identity['prepared_input_digest'] = publication['input_digest']
        identity['prepared_input_root'] = str(Path(prepared_input).absolute())
    check_build_identity(build, identity)
    captured_source = source_identity()
    environment = {**identity, **captured_source, 'lock_sha256': sha(RECIPE / 'lock.json')}
    if publication:
        environment['prepared'] = publication
        (build / 'prepared-source-files.json').write_text(json.dumps(environment['source_files']) + '\n')
    if dist and environment['dirty'] and not allow_dirty:
        raise RuntimeError('Dirty source requires --allow-dirty')
    (build / 'baseline-environment.json').write_text(json.dumps(environment, indent=2) + '\n')
    base = ['docker', 'run', '--rm', '--init', '--network=none', '--cpus=2', '--memory=12g',
            '--pids-limit=256', '--user', str(os.getuid()) + ':' + str(os.getgid()),
            '--mount', 'type=bind,src=' + str(ROOT) + ',dst=/src,readonly',
            '--mount', 'type=bind,src=' + str(build) + ',dst=/build',
            '--mount', 'type=bind,src=' + str(run) + ',dst=/run-output',
            '--workdir', '/build']
    if publication:
        base += ['--mount', 'type=bind,src=' + str(Path(prepared_input).absolute()) + ',dst=/prepared-input,readonly']
    for key, value in {'HOME': '/run-output/home', 'XDG_CONFIG_HOME': '/run-output/home/config',
                       'XDG_DATA_HOME': '/run-output/home/data', 'TMPDIR': '/run-output/tmp',
                       'OMP_NUM_THREADS': '1', 'OPENBLAS_NUM_THREADS': '1', 'LP_NUM_THREADS': '2',
                       'CMAKE_BUILD_PARALLEL_LEVEL': '1', 'CTEST_PARALLEL_LEVEL': '1',
                       'BLOBBER_RELEASE_LOCK_HELD': '1',
                       'BLOBBER_DIST_ALLOW_DIRTY': '1' if allow_dirty else '0'}.items():
        base += ['--env', key + '=' + value]
    base += [image, 'nice', '-n', '15', 'ionice', '-c', '3']
    # Enforce the committed full package inventory before compilation.
    run_command(base + ['python3', '/src/tools/linux_release.py', 'environment'], run / 'environment.log')
    if audit_only:
        if not (build / 'CMakeCache.txt').is_file():
            raise RuntimeError('audit requires an existing configured baseline build')
        run_command(base + ['python3', '/src/tests/linux_distribution_tests.py'], run / 'checker-tests.log')
    else:
        import_options = ['-DBLOBBER_PREPARED_INPUT=/prepared-input',
                          '-DBLOBBER_PREPARED_SOURCE_LEDGER=/build/prepared-source-files.json'] if publication else []
        run_command(base + ['/opt/cmake/bin/cmake', '-S', '/src', '-B', '/build', '-G', 'Ninja',
                            '-DCMAKE_MAKE_PROGRAM=/usr/bin/ninja', '-DCMAKE_BUILD_TYPE=Release',
                            '-DBUILD_TESTING=ON', '-DBLOBBER_LINUX_BASELINE=ON'] + import_options, run / 'configure.log')
        targets = TARGETS + (['blobber_validate_prepared', 'blobber_distribution_compatibility'] if dist else [])
        run_command(base + ['/opt/cmake/bin/cmake', '--build', '/build', '--target'] + targets + ['-j', '2'],
                    run / 'focused-build.log')
        run_command(base + ['ctest', '--test-dir', '/build', '-R', '^(' + '|'.join(TESTS) + ')$',
                            '-j', '1', '--output-on-failure', '--no-tests=error'], run / 'focused-tests.log')
    if dist:
        run_command(base + ['/opt/cmake/bin/cmake', '--build', '/build', '--target',
                            'blobber_prepared_assets', '--parallel', '1'], run / 'prepared-build.log')
        run_command(base + ['/build/blobber_validate_prepared', '/src/packaging/runtime-content.json',
                            '/src/assets', '/build/assets/prepared'], run / 'prepared-validation.json')
        from distribution import prepared
        from distribution.archive import encoded
        environment = json.loads((build / 'baseline-environment.json').read_text())
        if publication is None:
            publication = {'producer_revision': environment['revision'], 'input_digest': prepared.input_digest(ROOT),
                           'producer_environment': {k: environment[k] for k in ('image_id','recipe_sha256','tools')},
                           'identities': json.loads((run / 'prepared-validation.json').read_text())}
        publication['publication_sha256'] = hashlib.sha256(encoded(prepared.files(build / 'assets/prepared'))).hexdigest()
        environment['prepared'] = publication
        (build / 'baseline-environment.json').write_bytes(encoded(environment))
    run_command(base + ['/opt/cmake/bin/cmake', '--build', '/build', '--target',
                        'dist' if dist else 'dist-audit', '-j', '1'], run / 'stage-audit.log')
    print('Baseline stage and audit:', build / 'dist-stage', flush=True)
    if dist:
        run_command(base + ['/build/blobber_distribution_compatibility', 'export',
                            '/run-output/compatibility-linux'], run / 'compatibility-linux.log')
        if prepared_input is not None:
            dry = capture(base + ['/opt/cmake/bin/cmake', '--build', '/build', '--target', 'blobber', '--', '-n'])
            if re.search(r'\bblobber_bake_(world|portraits|creator)\s', dry):
                raise RuntimeError('Imported Linux graph still schedules a production baker')
        if source_identity() != captured_source:
            raise RuntimeError('Source changed during baseline build')
        from distribution.archive import package_name, encoded
        version = re.search(r'project\(blobber VERSION ([0-9.]+)', (ROOT / 'CMakeLists.txt').read_text()).group(1)
        bundle = build / 'dist' / package_name(version, environment['revision'], environment['dirty'], allow_dirty)
        (run / 'distribution.json').write_bytes(encoded({'bundle': str(bundle), 'build': str(build)}))
        print('Package:', bundle, flush=True)


def build_release(run, build, audit_only=False, dist=False, allow_dirty=False, prepared_input=None):
    check_resources()
    (run / 'home').mkdir(exist_ok=True)
    (run / 'tmp').mkdir(exist_ok=True)
    with build_lock(build):
        (build / 'dist-stage/Release.audit.json').unlink(missing_ok=True)
        (run / 'distribution.json').unlink(missing_ok=True)
        _build_release(run, build, audit_only, dist, allow_dirty, prepared_input)


def check_resources():
    free = shutil.disk_usage(ROOT).free
    memory = Path('/proc/meminfo').read_text()
    available = next(int(line.split()[1]) for line in memory.splitlines() if line.startswith('MemAvailable:')) * 1024
    if free < 8 * 1024**3 or available < 6 * 1024**3:
        raise RuntimeError('Baseline requires at least 8 GiB free disk and 6 GiB available memory')
    result = {'free_scratch_bytes': free, 'available_memory_bytes': available}
    print(json.dumps(result), flush=True)
    return result


def environment_check():
    expected = (RECIPE / 'packages.tsv').read_text()
    actual = capture(['dpkg-query', '-W', '-f=${Package}\t${Version}\t${Architecture}\n']) + '\n'
    if actual != expected:
        raise RuntimeError('Image package inventory differs from committed packages.tsv')
    build = Path('/build')
    environment = json.loads((build / 'baseline-environment.json').read_text())
    environment['packages_sha256'] = sha(RECIPE / 'packages.tsv')
    environment['tools'] = {name: capture(args).splitlines()[0] for name, args in {
        'gcc': ['gcc-11', '--version'], 'g++': ['g++-11', '--version'], 'cmake': ['cmake', '--version'],
        'ninja': ['ninja', '--version'], 'linker': ['ld', '--version'], 'readelf': ['readelf', '--version'],
        'python': ['python3', '--version'], 'glibc': ['getconf', 'GNU_LIBC_VERSION']}.items()}
    if environment['tools']['glibc'] != 'glibc 2.35':
        raise RuntimeError('Unexpected image glibc')
    (build / 'baseline-environment.json').write_text(json.dumps(environment, indent=2) + '\n')
    subprocess.run(['python3', '/src/packaging/linux/notices.py', '--check',
                    '/src/packaging/notices'], check=True)
    print(json.dumps(environment['tools'], indent=2))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest='operation', required=True)
    for operation in ('build', 'audit', 'dist', 'verify-package'):
        command = commands.add_parser(operation)
        command.add_argument('--run-dir', type=Path, required=True)
        command.add_argument('--build-dir', type=Path, default=ROOT / 'build-linux-baseline')
        if operation == 'dist':
            command.add_argument('--allow-dirty', action='store_true')
            command.add_argument('--prepared-input', type=Path)
        if operation == 'verify-package':
            command.add_argument('--package-dir', type=Path, required=True)
    commands.add_parser('environment', help=argparse.SUPPRESS)
    args = parser.parse_args()
    try:
        if args.operation == 'environment':
            environment_check()
        else:
            run = args.run_dir.absolute()
            build = args.build_dir.absolute()
            if run.is_symlink() or run.parent != ROOT / 'output/agent-runs' or not (run / '.agent-run.json').is_file():
                raise RuntimeError('--run-dir must be an existing owned agent run')
            if build.is_symlink() or build.parent != ROOT or not build.name.startswith('build-linux-baseline'):
                raise RuntimeError('--build-dir must be a dedicated build-linux-baseline* directory in the checkout')
            (run / 'home').mkdir(exist_ok=True)
            (run / 'tmp').mkdir(exist_ok=True)
            if args.operation == 'verify-package':
                check_resources()
                from distribution.verify_package import verify_package
                verify_package(run, args.package_dir.absolute())
            else:
                build_release(run, build, args.operation == 'audit', args.operation == 'dist',
                              getattr(args, 'allow_dirty', False), getattr(args, 'prepared_input', None))
    except (RuntimeError, OSError, ValueError, subprocess.SubprocessError) as error:
        print('Baseline build failed:', error, file=sys.stderr)
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
