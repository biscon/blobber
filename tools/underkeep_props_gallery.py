"""Assemble comparison evidence from application-owned captures (Pillow)."""
import argparse
from pathlib import Path
from PIL import Image, ImageDraw
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--input', required=True, type=Path,
                    help='Capture root containing before/final game and Blender views')
parser.add_argument('--output', required=True, type=Path,
                    help='New disposable output directory (promote selected evidence manually)')
args = parser.parse_args()
source = args.input.resolve()
root = args.output.resolve()
if source == root or source in root.parents:
    parser.error('Output must be separate from the input capture tree')
root.mkdir(parents=True, exist_ok=False)
before='game-before-capture';after='game-final'
props=['chest-closed','chest-open','cage-closed','rack-closed']
crops=[(580,360,940,720),(580,360,940,720),(380,54,1140,814),(380,280,1140,850)]
for prop,crop in zip(props,crops):
    sheet=Image.new('RGB',(1200,650),'#171b1a');draw=ImageDraw.Draw(sheet)
    for i,folder in enumerate([before,after]):
        im=Image.open(source/folder/f'{prop}-close.png').crop(crop)
        im.thumbnail((590,590),Image.Resampling.NEAREST)
        sheet.paste(im,(i*600+(600-im.width)//2,50))
        draw.text((i*600+20,20),prop+' / '+('Before' if i==0 else 'After'),fill='white')
    sheet.save(root/f'{prop}-comparison.png')
    frames=[]
    for frame in range(5):
        pair=Image.new('RGB',(1200,360),'#171b1a');d=ImageDraw.Draw(pair)
        for i,folder in enumerate([before,after]):
            im=Image.open(source/folder/f'{prop}-motion-{frame}.png').crop((10,54,1510,900)).resize((600,338))
            pair.paste(im,(600*i,22));d.text((600*i+10,5),'Before' if i==0 else 'After',fill='white')
        frames.append(pair)
    frames[0].save(root/f'{prop}-motion.gif',save_all=True,append_images=frames[1:]+frames[-2:0:-1],duration=220,loop=0)
for folder in ['blender-final','game-final']:
    files=sorted((source/folder).glob('*.png'));w=320;h=230
    sheet=Image.new('RGB',(w*5,h*((len(files)+4)//5)), '#222222');d=ImageDraw.Draw(sheet)
    for i,p in enumerate(files):
        im=Image.open(p)
        if folder.startswith('game'):im=im.crop((10,54,1510,900))
        im.thumbnail((w,h-24));x=i%5*w;y=i//5*h;sheet.paste(im,(x,y+24));d.text((x+4,y+4),p.stem,fill='white')
    sheet.save(root/(folder+'-sheet.jpg'))
lines = ['# Underkeep prop comparison', '',
         'Application-owned captures. Comparison crops show before and revised artwork.',
         'Motion GIFs are slowed scripted turn samples, not real-time recordings.', '']
for prop in props:
    lines += [f'- [{prop} comparison]({prop}-comparison.png)',
              f'- [{prop} turn samples]({prop}-motion.gif)']
lines += ['', '- [Final game views](game-final-sheet.jpg)',
          '- [Final Blender views](blender-final-sheet.jpg)', '']
(root/'README.md').write_text('\n'.join(lines)+'\n')
