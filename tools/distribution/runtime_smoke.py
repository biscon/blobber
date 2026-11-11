#!/usr/bin/env python3
"""Runs only inside an isolated runtime image with a read-only extracted package."""
import json
import os
from pathlib import Path
import subprocess
import time


def main():
    output = Path('/user-data')
    started = time.monotonic()
    for forbidden in ('/src', '/build', os.environ['BLOBBER_HOST_CHECKOUT']):
        if Path(forbidden).exists():
            raise RuntimeError('Original source/build is accessible: ' + forbidden)
    mounts = [line.split() for line in Path('/proc/self/mountinfo').read_text().splitlines()]
    if not any(row[4] == '/opt/blobber' and 'ro' in row[5].split(',') for row in mounts):
        raise RuntimeError('Package mount is not read-only')
    if Path('/usr/bin/g++').exists() or Path('/usr/bin/gcc').exists():
        raise RuntimeError('Runtime image unexpectedly contains a compiler')
    identity = {'os_release': Path('/etc/os-release').read_text(),
                'glibc': subprocess.check_output(['getconf', 'GNU_LIBC_VERSION'], text=True).strip(),
                'packages': Path('/opt/blobber-runtime/packages.tsv').read_text(),
                'cwd': os.getcwd(), 'mountinfo': Path('/proc/self/mountinfo').read_text(),
                'graphics': 'Xvfb with Mesa software GL; no host display/GPU',
                'audio': 'references checked, no device/playback/listening', 'phases': {}}
    (output / '.blobber-smoke-owner.json').write_text('{"version": 1}\n')
    for name in ('home', 'config', 'data', 'cache', 'tmp'):
        (output / name).mkdir()
    with (output / 'xvfb.log').open('w') as log:
        display = subprocess.Popen(['Xvfb', ':93', '-screen', '0', '1920x1080x24', '-nolisten', 'tcp',
                                    '-noreset', '-ac', '+extension', 'GLX'], stdout=log, stderr=subprocess.STDOUT)
        try:
            for _ in range(200):
                if display.poll() is not None:
                    raise RuntimeError('Xvfb exited before startup')
                if Path('/tmp/.X11-unix/X93').exists():
                    break
                time.sleep(.05)
            else:
                raise RuntimeError('Xvfb startup timed out')
            for phase in ('create', 'continue'):
                args = ['/opt/blobber/blobber', '--distribution-smoke', phase,
                        '--smoke-output', '/user-data/smoke']
                with (output / (phase + '.log')).open('w') as stream:
                    subprocess.run(args, check=True, stdout=stream, stderr=subprocess.STDOUT,
                                   timeout=max(1, 600 - (time.monotonic() - started)))
                record = json.loads((output / 'smoke' / (phase + '.json')).read_text())
                if record['status'] != 'passed' or record['assets'] != '/opt/blobber/assets' or \
                   record['prepared'] != '/opt/blobber/assets/prepared':
                    raise RuntimeError('Smoke did not use default packaged resources')
                for key in ('campaign', 'settings', 'library'):
                    if not Path(record[key]).is_relative_to(output) or not Path(record[key]).is_file():
                        raise RuntimeError('User data did not stay isolated')
                identity['phases'][phase] = record
            identity['status'] = 'passed'
        finally:
            display.terminate()
            try:
                display.wait(timeout=5)
            except subprocess.TimeoutExpired:
                display.kill()
                display.wait()
            identity['elapsed_seconds'] = round(time.monotonic() - started, 3)
            (output / 'runtime.json').write_text(json.dumps(identity, indent=2, sort_keys=True) + '\n')


if __name__ == '__main__':
    main()
