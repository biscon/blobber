"""Register the two painted RGBA sheets to the newt's common ground point."""
import json
from pathlib import Path

from PIL import Image

root = Path(__file__).resolve().parent
frames = root / 'frames'
frames.mkdir(exist_ok=True)
views = ('front', 'back', 'left', 'right')
canvas = (320, 240)
foot = (160, 232)
anchor = [foot[0] / canvas[0], foot[1] / canvas[1]]


def register(sheet, poses, columns, scale):
    """Use a fixed scale per view, so movement of the head survives registration."""
    for row, pose in enumerate(poses):
        for col, view in enumerate(views):
            tile = sheet.crop((columns[col], row * sheet.height // len(poses),
                               columns[col + 1], (row + 1) * sheet.height // len(poses)))
            # Remove the generator's nearly transparent background noise. Keep contour coverage.
            alpha = tile.getchannel('A').point(lambda a: 0 if a < 12 else a)
            tile.putalpha(alpha)
            tile = tile.crop(alpha.getbbox())
            factor = scale[col]
            tile = tile.resize((round(tile.width * factor), round(tile.height * factor)),
                               Image.Resampling.LANCZOS)
            if tile.width > canvas[0] or tile.height > foot[1]:
                raise ValueError(f'{pose}-{view} exceeds its footprint')
            out = Image.new('RGBA', canvas)
            out.paste(tile, (foot[0] - tile.width // 2, foot[1] - tile.height))
            out.save(frames / f'{pose}-{view}.png')


directions = Image.open(root / 'masters/directions.png')
register(directions, ('stance', 'stride', 'brace', 'inhale', 'snap', 'prone'),
         (0, 256, 512, 768, 1024), (1.04, 1.04, 1.20, 1.20))
reactions = Image.open(root / 'masters/reactions.png')
register(reactions, ('recoil', 'corpse', 'stride-b'),
         (0, 360, 665, 1064, 1448), (.84, .79, .79, .79))


def ref(pose, view):
    return dict(image=f'frames/{pose}-{view}.png', anchor=anchor)


clips = {}
for name, poses, durations in (
    ('idle', ('stance',), (800,)),
    ('walk', ('stride', 'stride-b'), (300, 300)),
    ('attack', ('brace', 'inhale', 'snap', 'stance'), (240, 180, 140, 260)),
    ('hurt', ('recoil',), (280,)),
    ('prone', ('prone',), (600,)),
    ('death', ('recoil', 'prone', 'corpse'), (160, 180, 620)),
):
    clips[name] = dict(durations_ms=durations,
                       views={v: [ref(p, v) for p in poses] for v in views})
(root / 'blind-cave-newt.json').write_text(json.dumps(dict(id='blind.cave.newt',
    width=.5, height=.375, alpha_cutoff=15, animations=clips), indent=2) + '\n')
portrait = Image.open(frames / 'inhale-front.png').crop((74, 72, 246, 244))
portrait.resize((160, 160), Image.Resampling.LANCZOS).save(root.parents[1] / 'ui/blind-cave-newt.png')
