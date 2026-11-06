#!/usr/bin/env python3
"""Register authored SE03 frames offline; derive emission from their painted cores."""
import argparse
from pathlib import Path
from PIL import Image
from prepare_se02_effects import pack


def prepare(source, output):
    output.mkdir(parents=True, exist_ok=True)
    sheets = {}
    for name in ('support', 'ground'):
        body = pack(source / (name + '-master.png'), 8)
        body.save(output / (name + '-body.png'))
        emission = body.copy()
        emission.putdata([(r, g, b, round(a * max(0, min(r, g, b) - 128) / 127))
                          for r, g, b, a in body.getdata()])
        emission.save(output / (name + '-emission.png'))
        sheets[name] = body
    # Portrait rows: stabilization, cleansing, boon, curse, faith, armor, sanctuary, sleep.
    portrait = Image.new('RGBA', (1024, 1024))
    for dest, (sheet, row) in enumerate((('ground', 6), ('ground', 7), ('support', 0),
            ('support', 1), ('support', 5), ('support', 6), ('support', 7), ('support', 2))):
        portrait.paste(sheets[sheet].crop((0, row*128, 1024, (row+1)*128)), (0, dest*128))
    portrait.save(output / 'portraits.png')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, default=Path('assets/effects/se03/source'))
    parser.add_argument('--output', type=Path, default=Path('assets/effects/se03'))
    args = parser.parse_args()
    prepare(args.source, args.output)
