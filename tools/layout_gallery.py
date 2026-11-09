#!/usr/bin/env python3
"""Bounded review gallery: twelve ordinary floors and the two checked stone fallbacks."""
import argparse
import json
from pathlib import Path
import statistics
import subprocess
from PIL import Image, ImageDraw, ImageFont

BACKGROUND = '#101818'
GOLD = '#e6bd64'
TILE_W, TILE_H, CELL = 480, 460, 10


def font(size):
    return ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf', size)


def panel(sheet, origin, report, title):
    x, y = origin
    draw = ImageDraw.Draw(sheet)
    draw.text((x + 20, y + 12), title, font=font(19), fill=GOLD)
    metrics = report['metrics']
    draw.text((x + 20, y + 40), report['profile'].replace('_', ' '), font=font(15), fill='#c8d6d3')
    draw.text((x + 20, y + 64), f"{metrics['walkable_cells']} cells · {metrics['progression_steps']} progression steps", font=font(13), fill='#adbeb8')
    bx, by = x + 45, y + 100
    rooms = {tuple(c): r['id'] for r in report['rooms'] for c in r['cells']}
    for cy, row in enumerate(report['map'].splitlines()):
        for cx, cell in enumerate(row):
            if cell == '#':
                continue
            shade = '#52645f' if (cx, cy) in rooms else '#45544f'
            draw.rectangle((bx + cx * CELL, by + cy * CELL, bx + (cx + 1) * CELL - 1, by + (cy + 1) * CELL - 1), fill=shade, outline='#293b35')
    for f in report['fixtures']:
        cx, cy = f['position']
        if f['kind'] in (0, 5):
            draw.rectangle((bx + cx * CELL + 2, by + cy * CELL + 2, bx + cx * CELL + 7, by + cy * CELL + 7), fill='#d2bc73' if not f['key'] else '#e8934a')
    for r in report['rooms']:
        marker = dict(entrance='E', approach='G', objective='O', optional='B').get(r['role'])
        if marker:
            cx, cy = r['anchor']
            draw.text((bx + cx * CELL, by + cy * CELL - 3), marker, font=font(12), fill='#fff1b5')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('generator', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    binary = str(args.generator.resolve())

    def generate(seed, depth, fallback=False):
        command = [binary, '--seed', str(seed), '--depth', str(depth), '--format', 'json']
        if fallback:
            command.append('--force-fallback')
        report = json.loads(subprocess.check_output(command, text=True, timeout=10))
        if not report['ok'] or report['fallback'] != fallback:
            raise RuntimeError('Review sample did not produce its requested generation mode')
        name = f"{'fallback' if fallback else 'seed'}-{seed}-depth-{depth}.json"
        (args.output / name).write_text(json.dumps(report, indent=2) + '\n')
        return report

    sheet = Image.new('RGB', (TILE_W * 4, TILE_H * 3), BACKGROUND)
    summary = []
    for depth in (1, 2, 3):
        reports = [generate(seed, depth) for seed in range(4)]
        for seed, report in enumerate(reports):
            panel(sheet, (TILE_W * seed, TILE_H * (depth - 1)), report, f'Depth {depth} · Seed {seed}')
        depth_sheet = sheet.crop((0, TILE_H * (depth - 1), TILE_W * 4, TILE_H * depth))
        depth_sheet.save(args.output / f'depth-{depth}.png')
        summary.append(dict(depth=depth, median_progression_steps=statistics.median(r['metrics']['progression_steps'] for r in reports),
                            min_walkable_cells=min(r['metrics']['walkable_cells'] for r in reports),
                            max_walkable_cells=max(r['metrics']['walkable_cells'] for r in reports)))
    sheet.save(args.output / 'all-floors.png')
    fallbacks = Image.new('RGB', (TILE_W * 2, TILE_H), BACKGROUND)
    for column, depth in enumerate((1, 3)):
        report = generate(5, depth, True)
        panel(fallbacks, (TILE_W * column, 0), report, f'Authored fallback · Depth {depth}')
    fallbacks.save(args.output / 'fallbacks.png')
    (args.output / 'metrics.json').write_text(json.dumps(summary, indent=2) + '\n')
    print(json.dumps(summary))


if __name__ == '__main__':
    main()
