#!/usr/bin/env python3
"""Reconcile retained dependency notices against the actual pinned build image."""
import argparse
import hashlib
from pathlib import Path
import re

DEPS = Path('/opt/blobber-deps')


def notices():
    result = {}
    for name, source in {'raylib.txt': 'raylib/LICENSE', 'glfw.txt': 'raylib/src/external/glfw/LICENSE.md',
                         'yaml-cpp.txt': 'yaml-cpp/LICENSE',
                         'nlohmann-json.txt': 'nlohmann_json/LICENSE.MIT'}.items():
        result[name] = (DEPS / source).read_text()
    external = DEPS / 'raylib/src/external'
    excerpts = ['Retained notices from hash-pinned raylib 6.2 embedded dependencies.\n'
                'Conservative inclusion also retains notices for optional formats.\n']
    covered = set()
    for source in sorted(external.iterdir()):
        if source.suffix not in ('.h', '.c'):
            continue
        text = source.read_text(errors='strict')
        blocks = re.findall(r'/\*.*?\*/|(?://[^\n]*\n)+', text, re.S)
        selected = []
        for block in blocks:
            lower = block.lower()
            if any(marker in lower for marker in ('permission is hereby granted',
                    'this software is provided', 'redistribution and use in source',
                    'alternative a - mit', 'alternative 1 - public domain',
                    'license, version 2', 'spdx-license-identifier')):
                selected.append(block.strip())
        if selected:
            covered.add(source.name)
            excerpts.append('\n===== ' + source.name + ' (SHA256 ' +
                            hashlib.sha256(source.read_bytes()).hexdigest() + ') =====\n' +
                            '\n\n'.join(selected) + '\n')
    required = {'qoi.h', 'miniaudio.h', 'dr_mp3.h', 'dr_wav.h', 'dr_flac.h', 'stb_vorbis.c',
                'stb_image.h', 'stb_image_write.h', 'stb_image_resize2.h', 'stb_truetype.h',
                'stb_rect_pack.h', 'sinfl.h', 'sdefl.h', 'rprand.h', 'glad.h',
                'jar_mod.h', 'jar_xm.h', 'cgltf.h', 'par_shapes.h'}
    if not required <= covered:
        raise RuntimeError('Missing embedded notices: ' + ', '.join(sorted(required - covered)))
    # qoi.h carries an MIT SPDX identifier; retain the MIT permission text too.
    excerpts.append('\n===== MIT permission text (QOI SPDX: MIT) =====\n' +
                    'Permission is hereby granted' +
                    result['nlohmann-json.txt'].split('Permission is hereby granted', 1)[1])
    result['raylib-components.txt'] = '\n'.join(excerpts)
    result['gcc-runtime-copyright.txt'] = Path('/usr/share/doc/gcc-11-base/copyright').read_text()
    result['gcc-runtime-GPL-3.txt'] = Path('/usr/share/common-licenses/GPL-3').read_text()
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path)
    parser.add_argument('--check', type=Path)
    args = parser.parse_args()
    if bool(args.output) == bool(args.check):
        parser.error('choose --output or --check')
    for name, content in notices().items():
        if args.check:
            if (args.check / name).read_text() != content:
                raise RuntimeError('Retained notice differs from pinned image: ' + name)
        else:
            args.output.mkdir(parents=True, exist_ok=True)
            (args.output / name).write_text(content)
    print('Pinned dependency notices reconciled')


if __name__ == '__main__':
    main()
