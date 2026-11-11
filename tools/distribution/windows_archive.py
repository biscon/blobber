#!/usr/bin/env python3
"""Streaming Windows ZIP with checked inventory, extraction and transactional promotion."""
import hashlib
import io
import json
from pathlib import Path
import re
import shutil
import stat
import time
import zipfile
from archive import digest, encoded, promote, regular, require, safe
from pe_audit import audit


def windows_path(name):
    path = safe(name)
    for part in path.parts:
        require(not any(c in part for c in ':<>"|?*') and all(ord(c) >= 32 for c in part), 'Invalid Windows name')
        require(part == part.rstrip(' .') and part.split('.')[0].upper() not in
                {'CON', 'PRN', 'AUX', 'NUL', *('COM'+str(i) for i in range(1,10)),
                 *('LPT'+str(i) for i in range(1,10))}, 'Reserved Windows name')
    return path


def write_zip(path, name, files, extra, epoch):
    require(not set(files) & set(extra), 'Metadata collides with payload')
    ledger = {n: {'bytes': regular(p).stat().st_size, 'sha256': digest(p)} for n,p in files.items()}
    ledger.update({n: {'bytes': len(b), 'sha256': hashlib.sha256(b).hexdigest()} for n,b in extra.items()})
    manifest = {'version': 1, 'self_excluded': 'MANIFEST.json', 'files': ledger}
    extra = {**extra, 'MANIFEST.json': encoded(manifest)}
    seen = set()
    date = time.gmtime(max(315532800, min(epoch, 4354819198)))[:6]
    with zipfile.ZipFile(path, 'w', compression=zipfile.ZIP_DEFLATED, compresslevel=6, allowZip64=True) as output:
        for n in sorted(set(files)|set(extra)):
            member = name+'/'+str(windows_path(n))
            require(member.casefold() not in seen, 'Case-colliding ZIP paths')
            seen.add(member.casefold())
            info = zipfile.ZipInfo(member, date_time=date)
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = (stat.S_IFREG|0o644)<<16
            info.create_system = 3
            info._compresslevel = 6
            with output.open(info, 'w', force_zip64=True) as dst:
                source = io.BytesIO(extra[n]) if n in extra else files[n].open('rb')
                with source:
                    shutil.copyfileobj(source, dst, 1024*1024)
            if n in files: require(digest(files[n]) == ledger[n]['sha256'], 'Input changed during ZIP')
    return manifest


def inspect_zip(path, output=None):
    target = Path(output) if output else None
    if target:
        require(not target.exists() and not any(p.is_symlink() or getattr(p,'is_junction',lambda:False)()
                for p in [target,*target.parents]), 'Unsafe extraction output')
        target.mkdir(parents=True)
    try:
        seen, ledger, root, total, manifest = set(), {}, None, 0, None
        with zipfile.ZipFile(path) as source:
            require(0 < len(source.infolist()) <= 65536, 'ZIP file count limit')
            for entry in source.infolist():
                p = windows_path(entry.filename)
                require(len(p.parts) > 1 and not entry.is_dir(), 'Invalid package member')
                root = root or p.parts[0]
                require(p.parts[0] == root, 'Multiple ZIP roots')
                require(entry.filename.casefold() not in seen, 'Duplicate/case-colliding ZIP entry')
                seen.add(entry.filename.casefold())
                mode = entry.external_attr >> 16
                require(stat.S_IFMT(mode) in (0, stat.S_IFREG) and not entry.flag_bits & 1, 'Special/encrypted ZIP member')
                require(entry.file_size <= 2*1024**3, 'ZIP member byte limit')
                total += entry.file_size
                require(total <= 8*1024**3, 'ZIP total byte limit')
                name = '/'.join(p.parts[1:])
                content = bytearray() if name == 'MANIFEST.json' else None
                checksum, size = hashlib.sha256(), 0
                dst = None
                if target:
                    destination = target.joinpath(*p.parts)
                    destination.parent.mkdir(parents=True, exist_ok=True)
                    dst = destination.open('xb')
                try:
                    with source.open(entry) as stream:
                        for block in iter(lambda: stream.read(1024*1024), b''):
                            checksum.update(block); size += len(block)
                            if content is not None:
                                require(size <= 4*1024**2, 'Manifest byte limit'); content.extend(block)
                            if dst: dst.write(block)
                finally:
                    if dst: dst.close()
                require(size == entry.file_size, 'Truncated ZIP')
                if content is not None: manifest = json.loads(content)
                else: ledger[name] = {'bytes': size, 'sha256': checksum.hexdigest()}
        require(manifest and manifest.get('version') == 1 and manifest.get('self_excluded') == 'MANIFEST.json'
                and manifest.get('files') == ledger, 'ZIP manifest/hash mismatch')
        require(all(n in ledger for n in ['blobber.exe','BUILD.json','README.txt']), 'Missing package metadata/game')
        return {'name': root, 'files': len(ledger)+1, 'unpacked_bytes': total, 'manifest': manifest}
    except BaseException:
        if target: shutil.rmtree(target)
        raise


def package(build, source, environment, version, allow_dirty=False, check_source=None):
    require(not environment['dirty'] or allow_dirty, 'Dirty source requires explicit allowance')
    require(re.fullmatch('[0-9a-f]{40}', environment['revision']), 'Invalid revision')
    require(re.fullmatch(r'\d+\.\d+\.\d+', version), 'Invalid project version')
    name = 'blobber-'+version+'-g'+environment['revision'][:12]+('-dirty' if environment['dirty'] else '')+'-windows-x86_64'
    stage = build/'dist-stage/Release'
    inventory = json.loads(regular(build/'dist-stage/Release.inventory.json').read_text())
    require(inventory['complete'] is True and inventory['configuration'] == 'Release', 'Incomplete stage')
    files = {n['path']: regular(stage/n['path']) for n in inventory['files']}
    require(set(files) == {p.relative_to(stage).as_posix() for p in stage.rglob('*') if p.is_file()}, 'Stage inventory mismatch')
    for n in inventory['files']: require(files[n['path']].stat().st_size == n['bytes'], 'Stage size differs')
    pes = audit(stage)
    info = {**environment, 'project_version': version, 'platform': 'windows-x86_64',
            'identities': inventory['identities'], 'pes': pes}
    root = build/'dist'
    owner = root/'.blobber-windows-dist-owner.json'
    require(not root.is_symlink(), 'Unsafe distribution root')
    if root.exists(): require(owner.is_file() and json.loads(owner.read_text()) == {'version':1}, 'Unowned distribution root')
    else: root.mkdir(); owner.write_bytes(encoded({'version':1}))
    candidate, final = root/('.candidate-'+name), root/name
    require(not candidate.exists() and not final.is_symlink(), 'Interrupted/unsafe package output')
    candidate.mkdir()
    try:
        archive = candidate/(name+'.zip')
        write_zip(archive,name,files,{'BUILD.json':encoded(info), 'README.txt':(source/'packaging/windows/player-readme.txt').read_bytes()}, environment['source_epoch'])
        summary = inspect_zip(archive)
        info['archive_sha256'], info['archive_bytes'] = digest(archive), archive.stat().st_size
        info['unpacked_bytes'], info['files'] = summary['unpacked_bytes'], summary['files']
        (candidate/(archive.name+'.sha256')).write_bytes((info['archive_sha256']+'  '+archive.name+'\n').encode())
        (candidate/'build.json').write_bytes(encoded(info))
        if check_source:check_source()
        promote(candidate,final)
        return final
    finally:
        if candidate.exists(): shutil.rmtree(candidate)
