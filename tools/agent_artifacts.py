#!/usr/bin/env python3
"""Create disposable agent runs; preview or explicitly remove one owned run."""
import argparse
from datetime import datetime, timezone
import json
import os
from pathlib import Path
import shutil
import stat
import subprocess
import uuid

MARKER = '.agent-run.json'


def managed_root(project):
    project = project.resolve()
    root = project / 'output' / 'agent-runs'
    for path in (project / 'output', root):
        if path.is_symlink():
            raise ValueError(f'Symlink is not allowed: {path}')
        if path.exists() and not path.is_dir():
            raise ValueError(f'Not a directory: {path}')
    return root


def create(project):
    root = managed_root(project)
    root.mkdir(parents=True, exist_ok=True)
    name = datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%SZ-') + uuid.uuid4().hex
    run = root / name
    run.mkdir()
    (run / MARKER).write_text(json.dumps({'version': 1, 'run_id': name}) + '\n')
    return run


def clean(project, directory, apply=False):
    project = project.resolve()
    root = managed_root(project)
    directory = Path(directory)
    if '..' in directory.parts:
        raise ValueError('Parent traversal is not allowed')
    run = Path(os.path.abspath(directory))
    if run.parent != root:
        raise ValueError('Specify one direct child of output/agent-runs')
    if run.is_symlink():
        raise ValueError('Symlink is not allowed')
    # An already removed run is a harmless no-op, including on repeated cleanup.
    if not run.exists():
        return {'directory': str(run), 'files': 0, 'bytes': 0, 'state': 'absent'}
    if not run.is_dir():
        raise ValueError('Run must be a directory')
    marker = run / MARKER
    if marker.is_symlink() or not marker.is_file():
        raise ValueError('Run has no regular ownership marker')
    try:
        identity = json.loads(marker.read_text())
    except (OSError, ValueError) as error:
        raise ValueError('Invalid ownership marker') from error
    if identity != {'version': 1, 'run_id': run.name}:
        raise ValueError('Ownership marker does not match this run')
    tracked = subprocess.run(
        ['git', '-C', str(project), 'ls-files', '-z', '--', run.relative_to(project).as_posix()],
        check=True, capture_output=True).stdout
    if tracked:
        raise ValueError('Run contains tracked content; refusing cleanup')
    count = size = 0
    # Preflight the whole tree before removing anything. Never follow symlinks.
    def walk_error(error):
        raise error
    for parent, dirs, files in os.walk(run, followlinks=False, onerror=walk_error):
        for name in dirs + files:
            info = (Path(parent) / name).lstat()
            if getattr(info, 'st_file_attributes', 0) & getattr(stat, 'FILE_ATTRIBUTE_REPARSE_POINT', 0):
                raise ValueError('Run contains a Windows reparse point')
            if not (stat.S_ISREG(info.st_mode) or stat.S_ISDIR(info.st_mode)):
                raise ValueError('Run contains a symlink or special file')
            if stat.S_ISREG(info.st_mode):
                count += 1
                size += info.st_size
    if apply:
        if not shutil.rmtree.avoids_symlink_attacks and os.name != 'nt':
            raise ValueError('Safe directory removal is unavailable on this platform')
        shutil.rmtree(run)
    return {'directory': str(run), 'files': count, 'bytes': size,
            'state': 'removed' if apply else 'preview'}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest='command', required=True)
    commands.add_parser('create', help='Create a uniquely named disposable run')
    cleanup = commands.add_parser('clean', help='Preview one run; --apply deletes it')
    cleanup.add_argument('directory', type=Path)
    cleanup.add_argument('--apply', action='store_true')
    args = parser.parse_args()
    project = Path(__file__).resolve().parents[1]
    try:
        if args.command == 'create':
            print(create(project))
        else:
            print(json.dumps(clean(project, args.directory, args.apply), indent=2))
    except (ValueError, OSError, subprocess.CalledProcessError) as error:
        parser.exit(1, f'Artifact cleanup refused: {error}\n')


if __name__ == '__main__':
    main()
