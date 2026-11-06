#!/usr/bin/env python3
"""Two deterministic Underkeep maps plus a contact sheet of application-owned captures."""
import argparse
import json
from pathlib import Path
import subprocess
from PIL import Image, ImageDraw
from layout_gallery import panel, font, BACKGROUND, GOLD, TILE_W, TILE_H, CELL


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('generator', type=Path)
    parser.add_argument('captures', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    for name in ('maps.png', 'contact.png', 'captures.json'):
        if (args.output / name).exists():
            raise SystemExit('Choose an output without existing review artifacts')
    sheet = Image.new('RGB', (TILE_W * 2, TILE_H + 80), BACKGROUND)
    manifest = dict(seed=0, generator=10, catalog=7, schema=10, simulation=14,
                    visibility='world-visibility-v18', samples=[], captures=[])
    for col, depth in enumerate((1, 3)):
        report = json.loads(subprocess.check_output(
            [str(args.generator.resolve()), '--seed', '0', '--depth', str(depth), '--format', 'json'],
            text=True, timeout=10))
        if not report['ok'] or report['fallback']:
            raise RuntimeError('Review floor did not generate normally')
        name = f'seed-0-depth-{depth}.json'
        (args.output / name).write_text(json.dumps(report, indent=2) + '\n')
        panel(sheet, (TILE_W * col, 0), report, f'Underkeep · Depth {depth} · Seed 0')
        draw = ImageDraw.Draw(sheet)
        bx, by = TILE_W * col + 45, 100
        for prop in report['dressing']:
            if not prop['blocking']:
                continue
            x, y = prop['cell']
            cells = [(x, y)]
            for cx, cy in cells:
                draw.rectangle((bx + cx * CELL, by + cy * CELL, bx + (cx + 1) * CELL - 1,
                                by + (cy + 1) * CELL - 1), fill='#a16e45', outline='#efbc72')
            label = dict(chest='C', cage='X', rack='R')[prop['kind']]
            draw.text((bx + x * CELL, by + y * CELL - 3), label, font=font(11), fill='#fff1b5')
        manifest['samples'].append(dict(depth=depth, profile=report['profile'],
                                        fingerprint=report['fingerprint'], report=name))
    draw.text((20, TILE_H + 12), 'Brown cells block movement: X cage · R rack',
              font=font(16), fill=GOLD)
    draw.text((20, TILE_H + 40), 'E entrance · G guardians · O objective · B optional · yellow/orange doors',
              font=font(14), fill='#c8d6d3')
    sheet.save(args.output / 'maps.png')
    captures = sorted(args.captures.glob('*.png'))
    if not captures or len(captures) > 32:
        raise RuntimeError('Expected one bounded review set (1–32 captures)')
    columns, width, height = 3, 480, 296
    contact = Image.new('RGB', (columns * width, ((len(captures) + columns - 1) // columns) * height), BACKGROUND)
    ink = ImageDraw.Draw(contact)
    for i, path in enumerate(captures):
        x, y = (i % columns) * width, (i // columns) * height
        with Image.open(path) as image:
            contact.paste(image.convert('RGB').resize((480, 270), Image.Resampling.LANCZOS), (x, y + 26))
        ink.text((x + 8, y + 5), path.stem.replace('_', ' '), font=font(13), fill=GOLD)
        manifest['captures'].append(path.name)
    contact.save(args.output / 'contact.png')
    (args.output / 'captures.json').write_text(json.dumps(manifest, indent=2) + '\n')


if __name__ == '__main__':
    main()
