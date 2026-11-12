"""Small, hash-bound CI transfers and platform evidence; no game build logic."""
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess

from archive import digest, encoded, regular, require, safe

MIB = 1024**2
LIMIT = 32 * MIB
PLAYER_LIMIT = 1024**3
ROOT = Path(__file__).resolve().parents[2]


def read_json(path):
    path = regular(path)
    require(path.stat().st_size <= 4*MIB, 'Oversized metadata')
    return json.loads(path.read_text(encoding='utf-8'))


def context():
    keys = ('GITHUB_SHA', 'GITHUB_RUN_ID', 'GITHUB_RUN_ATTEMPT', 'GITHUB_WORKFLOW_REF', 'GITHUB_WORKFLOW_SHA')
    result = {k.removeprefix('GITHUB_').lower(): os.environ[k] for k in keys}
    require(re.fullmatch('[0-9a-f]{40}', result['sha']), 'Invalid source SHA')
    result['repository'] = os.environ['GITHUB_REPOSITORY']
    return result


def runner_identity():
    return {name:os.environ.get(name) for name in ('RUNNER_OS','RUNNER_ARCH','ImageOS','ImageVersion')}


def check_source():
    expected = context()
    actual = subprocess.check_output(['git', '-C', str(ROOT), 'rev-parse', 'HEAD'], text=True).strip()
    status = subprocess.check_output(['git', '-C', str(ROOT), 'status', '--porcelain'], text=True).strip()
    require(actual == expected['sha'] and not status, 'Expected clean exact dispatched source')
    return expected


def atomic_json(path, value):
    path = Path(path)
    temporary = path.with_name('.' + path.name + '.tmp')
    temporary.write_bytes(encoded(value))
    temporary.replace(path)


def ledger(directory):
    require(directory.is_dir() and not directory.is_symlink(), 'Unsafe evidence directory')
    result, total = {}, 0
    for path in sorted(directory.rglob('*')):
        require(not path.is_symlink() and not getattr(path, 'is_junction', lambda: False)(), 'Evidence link')
        require(path.is_file() or path.is_dir(), 'Evidence special file')
        if path.is_file():
            name = path.relative_to(directory).as_posix()
            if name == 'transfer.json':
                continue
            total += path.stat().st_size
            require(total <= LIMIT and len(result) < 128, 'Evidence capacity exceeded')
            result[name] = {'bytes': path.stat().st_size, 'sha256': digest(path)}
    return result


def seal(directory):
    atomic_json(directory/'transfer.json', {'version':1, 'context':context(), 'files':ledger(directory)})
    require(sum(p.stat().st_size for p in directory.rglob('*') if p.is_file()) <= LIMIT, 'Evidence capacity exceeded')


def validate_transfer(directory):
    record = read_json(directory/'transfer.json')
    require(record.get('version') == 1 and record.get('context') == context(), 'Stale/mixed CI transfer')
    require(record['files'] == ledger(directory), 'Transfer file/hash mismatch')
    return record


def copy_file(source, target):
    source = regular(source)
    require(source.stat().st_size <= LIMIT, 'Oversized evidence file')
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(source, target)


def write_outputs(values):
    with Path(os.environ['GITHUB_OUTPUT']).open('a', encoding='utf-8') as stream:
        for key, value in values.items():
            require('\n' not in str(value) and '\r' not in str(value), 'Invalid step output')
            stream.write(str(key) + '=' + str(value) + '\n')


def platform_evidence(platform, bundle, run):
    info, verification = read_json(bundle/'build.json'), read_json(bundle/'verification.json')
    require(info['revision'] == context()['sha'] and info.get('dirty') is False, 'Wrong/dirty platform build')
    require(verification.get('status') == 'passed', 'Required extracted smoke did not pass')
    if platform == 'windows':
        require(verification.get('depth_copy') == 'passed', 'Required Windows depth test did not pass')
    else:
        from verify_package import verification_record
        verification_record(verification['archive_sha256'], verification['runtimes'])
    suffix = '.zip' if platform == 'windows' else '.tar.gz'
    name = 'blobber-' + info['project_version'] + '-' + platform + '-x86_64' + suffix
    player = regular(bundle/name)
    checksum = regular(bundle/(name+'.sha256'))
    require(player.stat().st_size <= PLAYER_LIMIT, 'Player exceeds 1 GiB upload bound')
    sha = digest(player)
    require(checksum.read_text() == sha+'  '+name+'\n', 'Player checksum differs')
    require(verification['archive_sha256'] == sha, 'Smoke hash differs')
    evidence = run/'evidence'
    evidence.mkdir()
    for n in ('build.json', 'verification.json'):
        copy_file(bundle/n, evidence/n)
    focused = bundle/'focused-tests.txt' if platform == 'windows' else run/'focused-tests.log'
    copy_file(focused, evidence/'focused-tests.txt')
    if platform == 'windows':
        copy_file(bundle/'depth-copy.txt', evidence/'depth-copy.txt')
        for p in sorted((bundle/'compatibility-windows').glob('*.json')):
            copy_file(p, evidence/'compatibility-windows'/p.name)
        for p in sorted((bundle/'captures').glob('*.png'))[:6]:
            copy_file(p, evidence/'captures'/p.name)
    else:
        workspace = run/'package-verification'
        for p in sorted(workspace.glob('*/capture-*.png'))[:6]:
            copy_file(p, evidence/'captures'/(p.parent.name+'-'+p.name))
    record = {'version':1, 'context':context(), 'runner':runner_identity(), 'platform':platform, 'status':'passed',
              'player':{'name':name,'bytes':player.stat().st_size,'sha256':sha},
              'build_sha256':digest(bundle/'build.json'), 'verification_sha256':digest(bundle/'verification.json')}
    atomic_json(evidence/'platform.json', record)
    seal(evidence)
    write_outputs({'player':player, 'checksum':checksum, 'evidence':evidence})
    return record


def download_artifact(artifact_id, target, limit=PLAYER_LIMIT):
    require(str(artifact_id).isdigit(), 'Invalid artifact ID')
    repo = context()['repository']
    endpoint = 'repos/'+repo+'/actions/artifacts/'+str(artifact_id)
    metadata = json.loads(subprocess.check_output(['gh','api',endpoint], text=True))
    run = metadata.get('workflow_run', {})
    require(str(run.get('id')) == context()['run_id'] and run.get('head_sha') == context()['sha'],
            'Artifact belongs to another run/source')
    require(not metadata['expired'] and metadata['size_in_bytes'] <= limit, 'Expired/oversized artifact')
    name = str(safe(metadata['name']))
    require('/' not in name, 'Invalid artifact name')
    with target.open('xb') as output:
        subprocess.run(['gh','api','--allow-escape-sequences',endpoint+'/zip'], stdout=output, check=True)
    expected = metadata.get('digest','')
    require(expected == 'sha256:'+digest(target), 'Downloaded transport digest differs')
    require(target.stat().st_size == metadata['size_in_bytes'], 'Downloaded byte count differs')
    return metadata


def extract_evidence(archive, destination):
    import zipfile
    destination.mkdir()
    total, seen = 0, set()
    with zipfile.ZipFile(archive) as source:
        require(len(source.infolist()) <= 256, 'Too many evidence members')
        for entry in source.infolist():
            name = str(safe(entry.filename.rstrip('/')))
            require(name not in seen, 'Duplicate evidence entry')
            seen.add(name)
            total += entry.file_size
            require(total <= LIMIT and not entry.flag_bits & 1, 'Oversized/encrypted evidence')
            require((entry.external_attr >> 16) & 0o170000 in (0,0o100000,0o040000), 'Special evidence entry')
            target = destination/name
            if entry.is_dir():
                target.mkdir(parents=True, exist_ok=True)
            else:
                target.parent.mkdir(parents=True, exist_ok=True)
                with source.open(entry) as inp, target.open('xb') as out:
                    shutil.copyfileobj(inp, out, 1024*1024)
    validate_transfer(destination)
    return destination


def validate_pair(linux, windows):
    require(linux['context'] == context() and windows['context'] == context(), 'Mixed source/run/attempt')
    require(linux['status'] == windows['status'] == 'passed', 'Incomplete platform result')
    require(linux['platform'] == 'linux' and windows['platform'] == 'windows', 'Wrong platform result')


def validate_builds(linux, windows):
    require(linux['project_version'] == windows['project_version'], 'Mixed versions')
    require(linux['identities'] == windows['identities'], 'Mixed runtime content')
    require(linux['prepared'] == windows['prepared'], 'Mixed prepared publication/provenance')
