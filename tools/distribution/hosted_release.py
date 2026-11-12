#!/usr/bin/env python3
"""LD04 serial hosted orchestration; never tags, merges or publishes a Release."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tarfile
import zipfile
import zlib

sys.path.insert(0, str(Path(__file__).resolve().parent))
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from archive import digest, encoded, inspect_archive, regular, require
import hosted_evidence as evidence
import prepared

ROOT = Path(__file__).resolve().parents[2]


def linux(run):
    import linux_release
    from distribution.verify_package import verify_package
    evidence.check_source()
    cache = ROOT/'output/wd01-prepared'
    hit = os.environ.get('PREPARED_CACHE_HIT') == 'true'
    if hit:
        prepared.validate(cache, ROOT)
    else:
        require(not cache.exists(), 'Partial/nonexact cache restore is forbidden')
    build = ROOT/('build-linux-baseline-hosted-import' if hit else 'build-linux-baseline-hosted-native')
    linux_release.build_release(run, build, dist=True, prepared_input=cache if hit else None)
    bundle = Path(evidence.read_json(run/'distribution.json')['bundle'])
    info = evidence.read_json(bundle/'build.json')
    publication = info['prepared']
    if not hit:
        prepared.export(build/'assets/prepared', cache, ROOT,
                        publication['producer_environment'], publication['identities'])
        # Preserve WD01 standalone consumer compatibility; current run uses transport.
        shutil.copytree(run/'compatibility-linux', cache/'compatibility')
    prepared.validate(cache, ROOT)
    total = 0
    for path in cache.rglob('*'):
        if path.is_file():
            compressor = zlib.compressobj(3)
            with path.open('rb') as source:
                for block in iter(lambda: source.read(1024*1024), b''):
                    total += len(compressor.compress(block))
            total += len(compressor.flush())
    require(total <= 1024**3, 'Prepared compressed transfer estimate exceeds 1 GiB')
    # A new marked run gives extraction/user data a fresh lifetime, separate from build.
    verify_run = Path(subprocess.check_output([sys.executable, ROOT/'tools/agent_artifacts.py','create'], text=True).strip())
    with Path(os.environ['GITHUB_ENV']).open('a') as stream:
        stream.write('VERIFY_RUN_DIR='+str(verify_run)+'\n')
    linux_release.check_resources()
    verify_package(verify_run, bundle)
    transport = run/'transport'
    transport.mkdir()
    evidence.copy_file(build/'blobber_distribution_compatibility', transport/'verifier')
    for path in sorted((run/'compatibility-linux').glob('*.json')):
        evidence.copy_file(path, transport/'compatibility-linux'/path.name)
    evidence.seal(transport)
    evidence.platform_evidence('linux', bundle, run)
    evidence.atomic_json(run/'evidence/cache.json', {'hit':hit,'key':'wd01-prepared-v1-'+publication['input_digest'],
        'producer_revision':publication['producer_revision'],'deflate_estimate':total})
    for path in sorted((verify_run/'package-verification/ubuntu22/smoke').glob('*.png'))[:6]:
        evidence.copy_file(path, run/'evidence/captures'/path.name)
    evidence.seal(run/'evidence')
    evidence.write_outputs({'transport':transport})


def windows(run, transport):
    import windows_release
    evidence.check_source()
    evidence.validate_transfer(transport)
    windows_release.build_release(run, ROOT/'output/wd01-prepared', ROOT/'build-windows-release',
                                  False, compatibility_input=transport/'compatibility-linux')
    bundle = Path(evidence.read_json(run/'distribution.json')['bundle'])
    evidence.platform_evidence('windows', bundle, run)


def embedded_build(player, platform, root):
    if platform == 'linux':
        with tarfile.open(player, 'r:gz') as archive:
            stream = archive.extractfile(root+'/BUILD.json')
            return json.loads(stream.read(4*evidence.MIB+1))
    with zipfile.ZipFile(player) as archive:
        with archive.open(root+'/BUILD.json') as stream:
            return json.loads(stream.read(4*evidence.MIB+1))


def verify_player(player, checksum, report, directory):
    platform = report['platform']
    expected = report['player']
    require(expected['name'] == player.name and player.stat().st_size == expected['bytes'], 'Wrong player name/size')
    require(digest(player) == expected['sha256'], 'Wrong downloaded player hash')
    require(checksum.read_text() == expected['sha256']+'  '+player.name+'\n', 'Wrong downloaded checksum')
    if platform == 'linux':
        summary = inspect_archive(player)
    else:
        from windows_archive import inspect_zip
        summary = inspect_zip(player)
    info = embedded_build(player, platform, summary['name'])
    require(info['revision'] == evidence.context()['sha'] and info.get('dirty') is False, 'Wrong embedded source')
    suffix = '.tar.gz' if platform == 'linux' else '.zip'
    name = 'blobber-'+info['project_version']+'-'+platform+'-x86_64'
    require(summary['name'] == name and player.name == name+suffix, 'Wrong plain archive root/name')
    outside = evidence.read_json(directory/'build.json')
    # Windows's external build file adds archive totals after writing embedded BUILD.
    require(all(outside.get(k) == v for k,v in info.items()), 'Embedded/external build metadata differ')
    require(report['build_sha256'] == digest(directory/'build.json') and
            report['verification_sha256'] == digest(directory/'verification.json'), 'Wrong metadata hash')
    verification = evidence.read_json(directory/'verification.json')
    require(verification.get('status') == 'passed' and verification['archive_sha256'] == expected['sha256'],
            'Missing/stale required smoke')
    if platform == 'linux':
        from distribution.verify_package import verification_record
        verification_record(expected['sha256'], verification['runtimes'])
    else:
        require(verification.get('depth_copy') == 'passed' and set(verification.get('phases',{})) == {'create','continue'},
                'Incomplete Windows smoke/depth')
        require(all(p.get('status') == 'passed' for p in verification['phases'].values()), 'Incomplete Windows smoke phase')
    # Only manifest-approved runtime inventory plus packaging metadata may ship.
    from_payload = set(summary['manifest']['files'])
    inventory_names = {'BUILD.json','README.txt'}
    for name in from_payload - inventory_names:
        require(name == ('blobber' if platform == 'linux' else 'blobber.exe') or
                name.startswith(('assets/','notices/')), 'Non-runtime player payload: '+name)
    return info, verification


def verify(run):
    evidence.check_source()
    output = run/'combined.json'
    output.unlink(missing_ok=True)
    require(shutil.disk_usage(run).free >= 6*1024**3, 'Insufficient final verification disk')
    reports, builds, checks, artifacts = {}, {}, {}, {}
    for platform in ('linux','windows'):
        ids = {kind:os.environ[platform.upper()+'_'+kind.upper()+'_ID'] for kind in ('player','checksum','evidence')}
        evidence_zip = run/(platform+'-evidence.zip')
        meta = evidence.download_artifact(ids['evidence'], evidence_zip, evidence.LIMIT)
        directory = evidence.extract_evidence(evidence_zip, run/(platform+'-evidence'))
        report = evidence.read_json(directory/'platform.json')
        require(report['context'] == evidence.context() and report['platform'] == platform and report['status'] == 'passed',
                'Wrong platform report')
        require('/' not in report['player']['name'] and '\\' not in report['player']['name'], 'Unsafe player name')
        player = run/report['player']['name']
        player_meta = evidence.download_artifact(ids['player'], player)
        require(player_meta['name'] == player.name, 'Artifact player name differs')
        checksum = run/(player.name+'.sha256')
        checksum_meta = evidence.download_artifact(ids['checksum'], checksum, evidence.MIB)
        require(checksum_meta['name'] == checksum.name, 'Artifact checksum name differs')
        builds[platform], checks[platform] = verify_player(player, checksum, report, directory)
        reports[platform] = report
        artifacts[platform] = {kind:{'id':int(ids[kind]), 'name':m['name'], 'bytes':m['size_in_bytes'],
                                      'sha256':m['digest'].removeprefix('sha256:')} for kind,m in
                               [('player',player_meta),('checksum',checksum_meta),('evidence',meta)]}
    evidence.validate_pair(reports['linux'], reports['windows'])
    evidence.validate_builds(builds['linux'], builds['windows'])
    transport_zip = run/'linux-transport.zip'
    evidence.download_artifact(os.environ['LINUX_TRANSPORT_ID'], transport_zip, evidence.LIMIT)
    transport = evidence.extract_evidence(transport_zip, run/'transport')
    verifier = regular(transport/'verifier')
    verifier.chmod(0o755)
    with (run/'reverse-formats.txt').open('w') as log:
        subprocess.run([str(verifier),'import',str(run/'windows-evidence/compatibility-windows'),
                        str(run/'compatibility-roundtrip')],stdout=log,stderr=subprocess.STDOUT,check=True,timeout=60)
    result = {'version':1,'status':'passed','context':evidence.context(), 'verification_runner':evidence.runner_identity(), 'artifacts':artifacts,
              'platforms':reports, 'prepared':builds['linux']['prepared'], 'identities':builds['linux']['identities'],
              'formats':{'linux_to_windows':'passed','windows_to_linux':'passed'},
              'limits':['Ubuntu 22.04 and Debian 13 share the hosted kernel', 'Software OpenGL only',
                        'Windows Server runner does not establish a Windows 11 hardware matrix',
                        'Audio references only; no playback/listening', 'Existing notice gaps; no publication clearance']}
    evidence.atomic_json(output, result)
    summary = ['## Verified desktop packages', '', 'Source: `'+evidence.context()['sha']+'`', '']
    base = 'https://github.com/'+evidence.context()['repository']+'/actions/runs/'+evidence.context()['run_id']+'/artifacts/'
    for platform in ('linux','windows'):
        artifact = artifacts[platform]['player']
        summary.append('- ['+artifact['name']+']('+base+str(artifact['id'])+') — SHA-256 `'+artifact['sha256']+'`')
    summary += ['', 'Both extracted packages, downloaded bytes and bidirectional format checks passed.',
                'Software OpenGL; physical GPU and playback/listening coverage remain separate. No public Release was published.']
    with Path(os.environ['GITHUB_STEP_SUMMARY']).open('a') as stream:
        stream.write('\n'.join(summary)+'\n')
    evidence.write_outputs({'combined':output})


def cleanup():
    for key in ('RUN_DIR','VERIFY_RUN_DIR'):
        if os.environ.get(key):
            for apply in ([],['--apply']):
                subprocess.run([sys.executable,ROOT/'tools/agent_artifacts.py','clean',os.environ[key],*apply],check=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('operation',choices=('linux','windows','verify','cleanup','diagnostics'))
    parser.add_argument('--run-dir',type=Path)
    parser.add_argument('--transport',type=Path)
    args = parser.parse_args()
    if args.operation == 'diagnostics':
        for key in ('RUN_DIR','VERIFY_RUN_DIR'):
            if os.environ.get(key):
                for path in Path(os.environ[key]).rglob('*.log'):
                    if path.is_file():
                        with path.open('rb') as stream:
                            stream.seek(max(0,path.stat().st_size-6000))
                            print(str(path)+':\n'+stream.read().decode('utf-8',errors='replace'))
        return
    if args.operation == 'cleanup':
        cleanup()
        return
    run = args.run_dir.absolute()
    require(run.parent == ROOT/'output/agent-runs' and not run.is_symlink() and
            (run/'.agent-run.json').is_file(), 'Marked owned run required')
    if args.operation == 'linux':linux(run)
    elif args.operation == 'windows':windows(run,args.transport)
    else:verify(run)


if __name__ == '__main__':
    main()
