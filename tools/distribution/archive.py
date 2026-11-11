#!/usr/bin/env python3
"""LD03 bounded, deterministic package construction and checked extraction."""
import argparse
from contextlib import contextmanager
import fcntl
import gzip
import hashlib
import io
import json
import os
from pathlib import Path, PurePosixPath
import re
import shutil
import subprocess
import tarfile


def digest(path):
    value = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            value.update(block)
    return value.hexdigest()


def encoded(value):
    return (json.dumps(value, indent=2, sort_keys=True) + '\n').encode()


def require(condition, message):
    if not condition:
        raise ValueError(message)


def safe(path):
    require(isinstance(path, str) and path and '\\' not in path and '\0' not in path, 'Unsafe package path')
    value = PurePosixPath(path)
    require(not value.is_absolute() and all(p not in ('', '.', '..') for p in path.split('/')),
            'Unsafe package path: ' + path)
    return value


def regular(path):
    path = Path(path)
    for ancestor in [path, *path.parents]:
        require(not ancestor.is_symlink(), 'Symlink input: ' + str(ancestor))
    require(path.is_file(), 'Missing regular file: ' + str(path))
    return path


def package_name(version, revision, dirty, allow_dirty):
    require(re.fullmatch(r'[0-9]+\.[0-9]+\.[0-9]+(?:[-+][A-Za-z0-9.-]+)?', version), 'Invalid version')
    require(re.fullmatch(r'[0-9a-f]{40}', revision), 'Invalid Git revision')
    require(not dirty or allow_dirty, 'Dirty source requires --allow-dirty')
    return f'blobber-{version}-g{revision[:12]}' + ('-dirty' if dirty else '') + '-linux-x86_64'


def stage_inputs(stage, inventory, audit):
    require(inventory.get('complete') is True and inventory.get('configuration') == 'Release',
            'Incomplete Release stage')
    require(audit.get('status') == 'passed' and audit.get('stage_id') == inventory.get('stage_id'),
            'Stale or failed audit')
    files = {}
    for item in inventory['files']:
        name = str(safe(item['path']))
        require(name not in files, 'Duplicate inventory path')
        path = regular(stage / name)
        require(path.stat().st_size == item['bytes'], 'Stage size differs: ' + name)
        files[name] = path
    actual = set()
    for path in stage.rglob('*'):
        require(not path.is_symlink() and (path.is_file() or path.is_dir()), 'Unsafe stage entry')
        if path.is_file():
            actual.add(path.relative_to(stage).as_posix())
    require(actual == set(files), 'Unexpected or missing stage files')
    require('blobber' in files and files['blobber'].stat().st_mode & 0o111, 'Missing executable permission')
    for item in audit['elfs']:
        require(item['path'] in files and digest(files[item['path']]) == item['sha256'], 'Stale ELF audit')
    return files


def write_archive(path, name, files, extra, epoch):
    require(isinstance(epoch, int) and 0 <= epoch <= 0xffffffff, 'Invalid archive epoch')
    require(not set(files) & set(extra), 'Package metadata collides with stage')
    entries = {}
    for relative, source in sorted(files.items()):
        regular(source)
        entries[relative] = {'bytes': source.stat().st_size, 'sha256': digest(source),
                             'mode': 0o755 if source.stat().st_mode & 0o111 else 0o644}
    for relative, data in extra.items():
        entries[relative] = {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest(), 'mode': 0o644}
    manifest = {'version': 1, 'self_excluded': 'MANIFEST.json', 'files': entries}
    extra = {**extra, 'MANIFEST.json': encoded(manifest)}
    paths = sorted(set(files) | set(extra))
    directories = {name}
    for relative in paths:
        safe(relative)
        directories.update(str(PurePosixPath(name, relative).parents[i])
                           for i in range(len(PurePosixPath(relative).parents))
                           if str(PurePosixPath(name, relative).parents[i]) != '.')
    with Path(path).open('wb') as raw, gzip.GzipFile(filename='', mode='wb', fileobj=raw,
                                                    mtime=epoch, compresslevel=6) as compressed:
        with tarfile.open(fileobj=compressed, mode='w|', format=tarfile.USTAR_FORMAT) as archive:
            for directory in sorted(directories):
                info = tarfile.TarInfo(directory)
                info.type, info.mode, info.mtime = tarfile.DIRTYPE, 0o755, epoch
                archive.addfile(info)
            for relative in paths:
                info = tarfile.TarInfo(name + '/' + relative)
                info.mtime = epoch
                if relative in extra:
                    info.size, info.mode = len(extra[relative]), 0o644
                    archive.addfile(info, io.BytesIO(extra[relative]))
                else:
                    record = entries[relative]
                    info.size, info.mode = record['bytes'], record['mode']
                    with files[relative].open('rb') as source:
                        archive.addfile(info, source)
                    require(digest(files[relative]) == record['sha256'], 'Input changed while archiving')
    return manifest


def inspect_archive(path, destination=None):
    """Never use extractall: validate members, then stream only regular files."""
    name, seen, entries, manifest, total = None, set(), {}, None, 0
    target = Path(destination) if destination else None
    if target:
        require(not target.exists(), 'Extraction destination must not exist')
        require(not any(p.is_symlink() for p in [target, *target.parents]), 'Unsafe extraction destination')
        target.mkdir()
    try:
        with tarfile.open(path, mode='r|gz') as archive:
            for member in archive:
                p = safe(member.name)
                require(member.name not in seen, 'Duplicate archive member')
                seen.add(member.name)
                require(len(seen) <= 65536, 'Archive entry capacity exceeded')
                name = name or p.parts[0]
                require(p.parts[0] == name, 'Multiple package roots')
                require(member.type in (tarfile.REGTYPE, tarfile.AREGTYPE, tarfile.DIRTYPE),
                        'Links/special archive members forbidden')
                require(member.mode == (0o755 if member.isdir() else (0o755 if len(p.parts) == 2 and p.name == 'blobber' else 0o644)),
                        'Unexpected archive permission')
                require(member.uid == 0 and member.gid == 0, 'Unexpected ownership')
                if member.isdir():
                    if target:
                        (target / str(p)).mkdir(parents=True, exist_ok=True)
                    continue
                require(len(p.parts) > 1, 'File at package root')
                relative = PurePosixPath(*p.parts[1:]).as_posix()
                require(member.size <= 2 * 1024**3, 'Oversized archive file')
                total += member.size
                require(total <= 8 * 1024**3, 'Archive size capacity exceeded')
                stream = archive.extractfile(member)
                checksum, count = hashlib.sha256(), 0
                content = bytearray() if relative == 'MANIFEST.json' else None
                output = None
                if target:
                    dest = target / str(p)
                    dest.parent.mkdir(parents=True, exist_ok=True)
                    output = dest.open('xb')
                try:
                    for block in iter(lambda: stream.read(1024 * 1024), b''):
                        count += len(block)
                        checksum.update(block)
                        if content is not None:
                            require(count <= 4 * 1024**2, 'Oversized manifest')
                            content.extend(block)
                        if output:
                            output.write(block)
                finally:
                    if output:
                        output.close()
                        dest.chmod(member.mode)
                require(count == member.size, 'Truncated archive file')
                if content is not None:
                    manifest = json.loads(content)
                else:
                    entries[relative] = {'bytes': count, 'sha256': checksum.hexdigest(), 'mode': member.mode}
        require(manifest and manifest.get('version') == 1 and manifest.get('self_excluded') == 'MANIFEST.json',
                'Missing/invalid manifest')
        require(manifest.get('files') == entries, 'Archive manifest/hash mismatch')
        require(all(k in entries for k in ('blobber', 'README.txt', 'BUILD.json')), 'Missing package metadata/game')
        return {'name': name, 'files': len(entries) + 1, 'unpacked_bytes': total,
                'manifest': manifest}
    except BaseException:
        if target:
            shutil.rmtree(target)
        raise


def validate_bundle(bundle):
    bundle = Path(bundle)
    name = bundle.name
    archive = regular(bundle / (name + '.tar.gz'))
    actual = digest(archive)
    require(regular(bundle / (name + '.tar.gz.sha256')).read_text() == actual + '  ' + archive.name + '\n',
            'Archive checksum mismatch')
    summary = inspect_archive(archive)
    require(summary['name'] == name, 'Archive root/name mismatch')
    return archive, actual, summary


def promote(candidate, final):
    previous = final.with_name('.previous-' + final.name)
    require(not previous.exists(), 'Interrupted promotion; inspect previous bundle')
    moved = False
    try:
        if final.exists():
            final.rename(previous)
            moved = True
        candidate.rename(final)
    except BaseException:
        if moved:
            previous.rename(final)
        raise
    if moved:
        shutil.rmtree(previous)


def check_source(source, environment):
    for relative, expected in environment['source_files'].items():
        path = source / relative
        actual = digest(regular(path)) if path.exists() else None
        require(actual == expected, 'Source changed during build: ' + relative)


@contextmanager
def writer(build):
    if os.environ.get('BLOBBER_RELEASE_LOCK_HELD') == '1':
        yield
        return
    with (build / '.baseline.lock').open('a') as stream:
        fcntl.flock(stream, fcntl.LOCK_EX | fcntl.LOCK_NB)
        yield


def distribute(build, source, version, allow_dirty):
    build, source = Path(build), Path(source)
    regular(build / 'CMakeCache.txt')
    owner = json.loads(regular(build / '.baseline-owner.json').read_text())
    require(owner.get('baseline') == 'ubuntu22.04-amd64' and owner.get('configuration') == 'Release',
            'Managed controlled baseline build required')
    with writer(build):
        env = json.loads((build / 'baseline-environment.json').read_text())
        require(env['baseline'] == 'ubuntu22.04-amd64', 'Controlled baseline required')
        name = package_name(version, env['revision'], env['dirty'], allow_dirty)
        check_source(source, env)
        cache = dict(line.split('=', 1) for line in (build / 'CMakeCache.txt').read_text().splitlines()
                     if '=' in line and not line.startswith(('#', '//')))
        cmake = cache['CMAKE_COMMAND:INTERNAL']
        regular(cache.get('CMAKE_MAKE_PROGRAM:UNINITIALIZED', cache.get('CMAKE_MAKE_PROGRAM:FILEPATH', '')))
        subprocess.run([cmake, '--build', str(build), '--target', 'dist-audit', '--parallel', '1'], check=True)
        stage = build / 'dist-stage/Release'
        inventory_path = stage.with_suffix('.inventory.json')
        audit_path = stage.with_suffix('.audit.json')
        inventory, audit = json.loads(inventory_path.read_text()), json.loads(audit_path.read_text())
        require(audit['inventory_sha256'] == digest(inventory_path) and audit['environment'] == env,
                'Stale inventory/environment audit')
        require(audit['policy_sha256'] == digest(source / 'packaging/linux/runtime-policy.json'), 'Stale policy')
        files = stage_inputs(stage, inventory, audit)
        root = build / 'dist'
        require(not root.is_symlink(), 'Unsafe distribution root')
        root.mkdir(exist_ok=True)
        marker = root / '.blobber-dist-owner.json'
        owner = {'version': 1, 'build': str(build)}
        if marker.exists():
            require(json.loads(regular(marker).read_text()) == owner, 'Distribution owner mismatch')
        else:
            require(not any(root.iterdir()), 'Refusing unowned distribution root')
            marker.write_bytes(encoded(owner))
        candidate, final = root / ('.candidate-' + name), root / name
        require(not final.is_symlink() and not candidate.is_symlink(), 'Unsafe bundle path')
        if candidate.exists():
            shutil.rmtree(candidate)
        candidate.mkdir()
        try:
            build_info = {'version': 1, 'project_version': version, 'revision': env['revision'],
                          'dirty': env['dirty'], 'source_status': env['source_status'],
                          'source_fingerprint': env['source_fingerprint'], 'identities': inventory['identities'],
                          'baseline': {k: env[k] for k in ('baseline', 'image_id', 'recipe_sha256', 'lock_sha256',
                                                          'configuration_sha256', 'packages_sha256', 'tools')},
                          'policy_sha256': audit['policy_sha256'], 'elfs': audit['elfs'],
                          'required_glibc': audit['required_glibc']}
            archive = candidate / (name + '.tar.gz')
            write_archive(archive, name, files, {'BUILD.json': encoded(build_info),
                          'README.txt': regular(source / 'packaging/linux/player-readme.txt').read_bytes()},
                          env['source_epoch'])
            inspect_archive(archive)
            check_source(source, env)
            checksum = digest(archive)
            (candidate / (archive.name + '.sha256')).write_text(checksum + '  ' + archive.name + '\n')
            (candidate / 'build.json').write_bytes(encoded(build_info))
            promote(candidate, final)
            print('Package complete:', final / archive.name, flush=True)
        finally:
            if candidate.exists():
                shutil.rmtree(candidate)
        return final


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build', type=Path, required=True)
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--version', required=True)
    args = parser.parse_args()
    distribute(args.build, args.source, args.version, os.environ.get('BLOBBER_DIST_ALLOW_DIRTY') == '1')


if __name__ == '__main__':
    main()
