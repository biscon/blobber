#!/usr/bin/env python3
"""Checked portable publication export/import; never executes a foreign baker."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from archive import digest, encoded, promote, regular, require

ROOT = Path(__file__).resolve().parents[2]
MAX_BYTES = 2 * 1024**3


def source_files(root):
    names = subprocess.check_output(['git', '-C', str(root), 'ls-files', '-z', '--cached',
                                     '--others', '--exclude-standard']).decode('utf-8').split('\0')
    ledger = {}
    for name in sorted(set(names) - {''}):
        path = root / name
        require(not path.is_symlink() and not getattr(path, 'is_junction', lambda: False)(), 'Source link')
        ledger[name] = digest(regular(path)) if path.exists() else None
    return ledger


def inputs(root, source_ledger=None):
    # Conservative transitive source envelope: no independent artwork allowlist.
    # CMake/tool edits invalidate the envelope, including added/deleted bake inputs.
    if source_ledger is None:
        names = source_files(root)
    else:
        # The pinned baseline image has no Git. The orchestration host captures
        # names/hashes before mounting source read-only; still hash the inputs here.
        from archive import safe
        names = json.loads(regular(source_ledger).read_text())
        for name, expected in names.items():
            safe(name)
            path = root / name
            require((digest(regular(path)) if path.exists() else None) == expected,
                    'Source ledger differs: ' + name)
    selected = [n for n in names if (n.startswith(('assets/', 'src/', 'cmake/')) or
                n == 'CMakeLists.txt' or n == 'packaging/linux/lock.json' or
                n.startswith(('tools/bake', 'tools/prepare', 'tests/support/')) or n == 'tools/distribution_compatibility.cpp')]
    return {n: names[n] for n in selected}


def input_digest(root):
    import hashlib
    return hashlib.sha256(encoded(inputs(root))).hexdigest()


def files(root):
    require(root.is_dir() and not root.is_symlink(), 'Invalid publication root')
    ledger, total = {}, 0
    for path in sorted(root.rglob('*')):
        require(not path.is_symlink() and not getattr(path, 'is_junction', lambda: False)(), 'Publication link')
        require(path.is_dir() or path.is_file(), 'Publication special file')
        if path.is_file():
            name = path.relative_to(root).as_posix()
            total += path.stat().st_size
            require(total <= MAX_BYTES and len(ledger) < 4096, 'Publication capacity exceeded')
            ledger[name] = {'bytes': path.stat().st_size, 'sha256': digest(path)}
    require(all(n in ledger for n in ('creator.qoi', 'portraits-small.qoi', 'portraits-large.qoi',
                                      'world/metadata.json', 'world/sources.json', 'world/textures.bin')),
            'Incomplete prepared publication')
    return ledger


def validate(bundle, source, source_ledger=None):
    manifest = json.loads(regular(bundle / 'prepared.json').read_text())
    observed = inputs(source, source_ledger)
    require(manifest.get('version') == 1, 'Unsupported prepared input')
    require(manifest['inputs'] == observed and manifest['input_digest'] == __import__('hashlib').sha256(encoded(observed)).hexdigest(),
            'Prepared source inputs differ')
    require(manifest['files'] == files(bundle / 'prepared'), 'Prepared file ledger differs')
    return manifest


def export(prepared, output, source, environment=None, identities=None):
    require(not output.exists(), 'Export output already exists')
    ledger = files(prepared)
    # Fail before copying the large publication if host provenance is unavailable.
    revision = subprocess.check_output(['git', '-C', str(source), 'rev-parse', 'HEAD'], text=True).strip()
    observed = inputs(source)
    manifest = {'version': 1, 'producer_revision': revision,
                'input_digest': __import__('hashlib').sha256(encoded(observed)).hexdigest(),
                'inputs': observed, 'files': ledger,
                'producer_environment': environment or {}, 'identities': identities or {}}
    output.mkdir(parents=True)
    try:
        shutil.copytree(prepared, output / 'prepared')
        (output / 'prepared.json').write_bytes(encoded(manifest))
        validate(output, source)
    except BaseException:
        shutil.rmtree(output)
        raise


def import_prepared(bundle, output, source, source_ledger=None):
    manifest = validate(bundle, source, source_ledger)
    require(not output.is_symlink() and not getattr(output, 'is_junction', lambda: False)(), 'Unsafe prepared output')
    require(output.resolve() != (bundle / 'prepared').resolve(), 'Input/output overlap')
    marker = output.parent / '.prepared-import-owner.json'
    owner = {'version': 1, 'output': str(output.resolve())}
    if marker.exists():
        require(json.loads(regular(marker).read_text()) == owner, 'Prepared output ownership mismatch')
    else:
        require(not output.exists() or not any(output.iterdir()), 'Refusing unowned prepared output')
        output.parent.mkdir(parents=True, exist_ok=True)
        marker.write_bytes(encoded(owner))
    if output.is_dir() and files(output) == manifest['files']:
        return manifest
    candidate = output.with_name('.prepared-candidate')
    require(not candidate.exists(), 'Interrupted prepared import; inspect candidate')
    try:
        shutil.copytree(bundle / 'prepared', candidate)
        require(files(candidate) == manifest['files'], 'Prepared input changed during import')
        if output.exists() and not any(output.iterdir()):
            output.rmdir()
        promote(candidate, output)
    finally:
        if candidate.exists():
            shutil.rmtree(candidate)
    return manifest


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('operation', choices=['key', 'export', 'import', 'verify'])
    p.add_argument('--source', type=Path, default=ROOT)
    p.add_argument('--prepared', type=Path)
    p.add_argument('--bundle', type=Path)
    p.add_argument('--output', type=Path)
    p.add_argument('--environment', type=Path)
    p.add_argument('--validation', type=Path)
    p.add_argument('--source-ledger', type=Path)
    a = p.parse_args()
    if a.operation == 'key': print('wd01-prepared-v1-' + input_digest(a.source))
    elif a.operation == 'export': export(a.prepared, a.output, a.source,
        json.loads(a.environment.read_text()) if a.environment else None,
        json.loads(a.validation.read_text()) if a.validation else None)
    elif a.operation == 'import': import_prepared(a.bundle, a.output, a.source, a.source_ledger)
    else: validate(a.bundle, a.source, a.source_ledger)


if __name__ == '__main__': main()
