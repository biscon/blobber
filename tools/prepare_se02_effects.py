#!/usr/bin/env python3
"""Offline registration of authored SE02 sheets; no runtime image processing."""
import argparse
from pathlib import Path
from PIL import Image


def pack(source, rows):
    master = Image.open(source).convert('RGBA')
    result = Image.new('RGBA', (1024, rows * 128))
    for row in range(rows):
        for col in range(8):
            w, h = master.size
            frame = master.crop((round(col*w/8), round(row*h/rows),
                                 round((col+1)*w/8), round((row+1)*h/rows)))
            # Same explicit low-alpha residue policy as the accepted SE01 packer.
            frame.putalpha(frame.getchannel('A').point(lambda a: 0 if a < 96 else a))
            frame = frame.resize((112, 112), Image.Resampling.NEAREST)
            result.paste(frame, (col*128+8, row*128+8))
    return result


def prepare(source, output):
    output.mkdir(parents=True, exist_ok=True)
    for name in ('arcane', 'contact'):
        body = pack(source / (name+'-clean.png'), 8)
        if name == 'contact':
            pressure = pack(source / 'contact-pressure-master.png', 8)
            body.paste(pressure.crop((0, 4*128, 1024, 5*128)), (0, 4*128))
        body.save(output / (name+'-body.png'))
        # Registered emission is a luminance gate on authored cores, never a new silhouette.
        emission = body.copy()
        emission.putdata([(r,g,b,round(a*max(0, min(r,g,b)-128)/127))
                          for r,g,b,a in body.getdata()])
        emission.save(output / (name+'-emission.png'))
    pack(source / 'portrait-clean.png', 4).save(output / 'portraits.png')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, default=Path('assets/effects/se02/source'))
    parser.add_argument('--output', type=Path, default=Path('assets/effects/se02'))
    args = parser.parse_args()
    prepare(args.source, args.output)
