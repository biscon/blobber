#!/usr/bin/env python3
"""Image-only verified source/tool provisioning. Never invoked in normal frames."""
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile


def install(entry, destination):
    destination.mkdir(parents=True)
    with tempfile.TemporaryDirectory() as scratch:
        archive = Path(scratch) / 'source.tar'
        subprocess.run(['curl', '--fail', '--location', '--silent', '--show-error',
                        '--retry', '3', '--max-time', '300', entry['url'], '-o', str(archive)], check=True)
        if hashlib.sha256(archive.read_bytes()).hexdigest() != entry['sha256']:
            raise RuntimeError('Source checksum mismatch: ' + entry['url'])
        subprocess.run(['tar', '-xf', str(archive), '--strip-components=1',
                        '-C', str(destination)], check=True)


if __name__ == '__main__':
    lock = json.loads(Path('/opt/blobber-baseline/lock.json').read_text())
    install(lock['cmake'], Path('/opt/cmake'))
    for name, entry in lock['sources'].items():
        install(entry, Path('/opt/blobber-deps') / name)
