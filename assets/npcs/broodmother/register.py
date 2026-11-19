"""Register the vault's two painted masters and small DC03 spider to prepared appearances."""
import json
from pathlib import Path
from PIL import Image

root = Path(__file__).resolve().parent
views = ('front', 'back', 'left', 'right')


def tile(sheet, col, row, columns, rows):
    image = sheet.crop((col * sheet.width // columns, row * sheet.height // rows,
                        (col + 1) * sheet.width // columns, (row + 1) * sheet.height // rows))
    alpha = image.getchannel('A').point(lambda a: 0 if a < 16 else a)
    image.putalpha(alpha)
    return image.crop(alpha.getbbox())


def publish(directory, species, width, height, poses, sequences, size, hanging=False):
    directory.mkdir(exist_ok=True)
    frames = directory / 'frames'
    frames.mkdir(exist_ok=True)
    for name, image in poses.items():
        image.thumbnail((size[0] - 4, size[1] - 4), Image.Resampling.LANCZOS)
        frame = Image.new('RGBA', size)
        y = 2 if hanging else size[1] - image.height - 2
        frame.paste(image, ((size[0] - image.width) // 2, y))
        frame.save(frames / (name + '.png'))
    clips = {}
    for clip, keys, times in sequences:
        def ref(key, view):
            name = key if hanging else key + '-' + view
            return dict(image=f'frames/{name}.png', anchor=[.5, 1.0])
        clips[clip] = dict(durations_ms=times, views={v: [ref(k,v) for k in keys] for v in views})
    info = dict(id=species, width=width, height=height, alpha_cutoff=15, animations=clips)
    (directory / (directory.name + '.json')).write_text(json.dumps(info, indent=2) + '\n')


mother = Image.open(root / 'masters/directions.png')
poses = {pose + '-' + view: tile(mother, col, row, 4, 4)
         for row, pose in enumerate(('idle', 'stride', 'strike', 'corpse')) for col, view in enumerate(views)}
publish(root, 'broodmother', .5, .36, poses,
        [('idle',['idle'],[800]), ('walk',['stride','idle'],[300,300]),
         ('attack',['stride','idle','strike','idle'],[180,140,180,240]),
         ('hurt',['stride'],[220]), ('prone',['corpse'],[500]), ('death',['corpse'],[900])], (256,184))
clutch = root.parent / 'vault-clutch'
sheet = Image.open(clutch / 'masters/states.png')
publish(clutch, 'vault.clutch', .46, .72,
        {p: tile(sheet,i,0,3,1) for i,p in enumerate(('sealed','swollen','torn'))},
        [('idle',['sealed'],[800]), ('attack',['sealed','swollen'],[340,340]),
         ('prone',['torn'],[500]), ('death',['torn'],[1000])], (160,250), True)
young = root.parent / 'vault-young'
spider = root.parent / 'venomous-spider'
young.mkdir(exist_ok=True)
(young / 'frames').mkdir(exist_ok=True)
data = json.loads((spider / 'venomous-spider.json').read_text())
data.update(id='vault.young',width=.29,height=.19)
written = set()
for clip in data['animations'].values():
    if 'width' in clip:
        clip['width'] *= .58
    for refs in clip['views'].values():
        for ref in refs:
            dest = young / ref['image']
            if dest not in written:
                written.add(dest)
                im = Image.open(spider / ref['image'])
                im.resize((round(im.width*.58),round(im.height*.58)),Image.Resampling.LANCZOS).save(dest)
bristle_views = dict(data['animations']['idle']['views'])
bristle_views['front'] = data['animations']['attack']['views']['front'][1:3]
data['animations']['ranged_attack'] = dict(durations_ms=[240, 260], views=bristle_views)
(young / 'vault-young.json').write_text(json.dumps(data,indent=2)+'\n')
