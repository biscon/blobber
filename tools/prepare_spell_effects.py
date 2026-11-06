#!/usr/bin/env python3
"""Pack authored SE01 frames into registered, padded runtime atlases (offline only)."""
import argparse
from pathlib import Path
from PIL import Image, ImageChops

def prepare(source: Path, output: Path):
    body = Image.open(source / 'body-clean.png').convert('RGBA')
    emission = Image.open(source / 'emission-master.png').convert('RGBA')
    atlases = [Image.new('RGBA', (1024, 1024)), Image.new('RGBA', (1024, 1024))]
    for row in range(8):
        for col in range(8):
            frames = []
            for master in (body, emission):
                w, h = master.size
                frame = master.crop((round(col*w/8), round(row*h/8), round((col+1)*w/8), round((row+1)*h/8)))
                # Remove low-alpha generation residue; no runtime processing.
                frame.putalpha(frame.getchannel('A').point(lambda a: 0 if a < 96 else a))
                frame = frame.resize((112, 112), Image.Resampling.NEAREST)
                frames.append(frame)
            # Emission never extends beyond the registered colour body's silhouette.
            mask = ImageChops.darker(frames[0].getchannel('A'), frames[1].getchannel('A'))
            frames[1].putalpha(mask)
            for atlas, frame in zip(atlases, frames):
                atlas.paste(frame, (col*128+8, row*128+8))
    output.mkdir(parents=True, exist_ok=True)
    for name, atlas in zip(('body', 'emission'), atlases):
        atlas.save(output / (name+'.png'))

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, default=Path('assets/effects/se01/source'))
    parser.add_argument('--output', type=Path, default=Path('assets/effects/se01'))
    args = parser.parse_args()
    prepare(args.source, args.output)
