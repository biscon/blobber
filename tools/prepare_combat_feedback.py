#!/usr/bin/env python3
"""Register the three authored feedback backdrops; no runtime image processing."""
import argparse
from pathlib import Path
from PIL import Image


def prepare(source, output):
    with Image.open(source) as image:
        master = image.convert('RGBA')
    if master.getchannel('A').getextrema()[0] != 0:
        raise ValueError('Feedback artwork must have a transparent background')
    atlas = Image.new('RGBA', (768, 128))
    for column in range(3):
        cell = master.crop((round(column*master.width/3), 0,
                            round((column+1)*master.width/3), master.height))
        # Ignore near-invisible generated background residue before registering bounds.
        cell.putalpha(cell.getchannel('A').point(lambda alpha: 0 if alpha < 16 else alpha))
        bounds = cell.getchannel('A').getbbox()
        if not bounds:
            raise ValueError('Missing feedback backdrop')
        sprite = cell.crop(bounds)
        scale = min(224/sprite.width, 112/sprite.height)
        size = (max(1, round(sprite.width*scale)), max(1, round(sprite.height*scale)))
        # Premultiplied offline filtering avoids dark fringes on straight-alpha UI textures.
        sprite = sprite.convert('RGBa').resize(size, Image.Resampling.LANCZOS).convert('RGBA')
        atlas.paste(sprite, (column*256+(256-size[0])//2, (128-size[1])//2))
    output.parent.mkdir(parents=True, exist_ok=True)
    atlas.save(output)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path,
                        default=Path('assets/effects/combat-feedback/source/painted-splashes.png'))
    parser.add_argument('--output', type=Path, default=Path('assets/effects/combat-feedback/backdrops.png'))
    args = parser.parse_args()
    prepare(args.source, args.output)
