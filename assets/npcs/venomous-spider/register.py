"""Rebuild registered game frames and the initiative portrait from the two painted masters."""
from pathlib import Path
import json
from PIL import Image

root = Path(__file__).resolve().parent
frames = root / 'frames'
frames.mkdir(exist_ok=True)
canvas = (320, 210)
anchor = [0.5, 200 / canvas[1]]
views = ('front', 'back', 'left', 'right')
directions = Image.open(root / 'masters/directions.png')
bite = Image.open(root / 'masters/bite.png')
row_edges = (0, 230, 425, 620, 835, 1025, 1254)
poses = ('stance', 'step-a', 'step-b', 'recoil', 'prone', 'corpse')


def register(image, name, scale=1):
    bounds = image.getchannel('A').point(lambda a: 255 if a > 15 else 0).getbbox()
    sprite = image.crop(bounds)
    if scale != 1:
        sprite = sprite.resize(tuple(round(n * scale) for n in sprite.size), Image.Resampling.LANCZOS)
    out = Image.new('RGBA', canvas)
    out.paste(sprite, ((canvas[0] - sprite.width) // 2, 200 - sprite.height))
    out.save(frames / (name + '.png'))


for col, view in enumerate(views):
    for row, pose in enumerate(poses):
        register(directions.crop((round(col * directions.width / 4), row_edges[row],
                                  round((col + 1) * directions.width / 4), row_edges[row + 1])),
                 f'{pose}-{view}')
    frame = bite.crop((col * bite.width // 4, 0, (col + 1) * bite.width // 4, bite.height))
    register(frame, f'bite-{col}', 0.52)

clips = {}
for name, sequence, durations in (
        ('idle', ('stance', 'step-a'), (600, 600)),
        ('walk', ('step-a', 'step-b'), (300, 300)),
        ('hurt', ('recoil', 'prone'), (150, 150)),
        ('prone', ('prone',), (600,)),
        ('death', ('recoil', 'step-b', 'prone', 'corpse'), (200, 200, 200, 600))):
    clips[name] = dict(durations_ms=durations, views={
        side: [dict(image=f'frames/{pose}-{side}.png', anchor=anchor) for pose in sequence]
        for side in views})
clips['attack'] = dict(durations_ms=[300, 150, 150, 300], views={
    'front': [dict(image=f'frames/bite-{i}.png', anchor=anchor) for i in range(4)]})
metadata = dict(id='venomous.spider', width=0.5, height=0.5 * canvas[1] / canvas[0],
                alpha_cutoff=15, animations=clips)
(root / 'venomous-spider.json').write_text(json.dumps(metadata, indent=2) + '\n')
bite.crop((1190, 225, 1510, 545)).resize((160, 160), Image.Resampling.LANCZOS).save(
    root.parents[1] / 'ui/venomous-spider.png')
