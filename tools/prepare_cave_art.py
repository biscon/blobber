"""Prepare the painted limestone kit. Run from any directory with Pillow and NumPy."""
from pathlib import Path
import json
import math
import random
import numpy as np
from PIL import Image, ImageEnhance

ROOT = Path(__file__).resolve().parents[1] / 'assets'
PACK = ROOT / 'packs/damp-caves'
SOURCE = PACK / 'source'
MODELS = PACK / 'models'
WATER = 'packs/sanctuary-city/source/water-master.png'
RNG = random.Random(2402)
FACES = {}
PARTS = {}


def asset(path):
    return str(path.relative_to(ROOT))


def quarter(image, index):
    w, h = image.size
    x, y = index % 2 * (w // 2), index // 2 * (h // 2)
    return image.crop((x + 3, y + 3, x + w // 2 - 3, y + h // 2 - 3)).resize((256, 256), Image.Resampling.LANCZOS)


stone = Image.open(SOURCE / 'material-master.png').convert('RGB')
field = Image.open(SOURCE / 'field-master.png').convert('RGB')
for index, name in ((1, 'floor'), (2, 'ceiling')):
    quarter(stone, index).save(SOURCE / (name + '.png'))
wall = Image.open(SOURCE / 'wet-wall-master.png').convert('RGB')
bare_wall = wall.resize((256, 256), Image.Resampling.LANCZOS)
moss_rock = Image.open(SOURCE / 'moss-limestone-master.png').convert('RGB')
moss_rock = moss_rock.resize((256, 256), Image.Resampling.LANCZOS)
Image.blend(bare_wall, moss_rock, .85).save(SOURCE / 'wall.png')
silk = Image.open(SOURCE / 'silk-master.png').convert('RGBA')
silk = silk.resize((256, 256), Image.Resampling.LANCZOS)
bright = ImageEnhance.Brightness(silk.convert('RGB')).enhance(1.42)
bright.putalpha(silk.getchannel('A'))
bright.save(SOURCE / 'web.png')
paint = dict(rock=Image.blend(wall.resize((256, 256)), quarter(stone, 0), .14), linen=quarter(stone, 3),
             silk=quarter(field, 0), rope=quarter(field, 1), cloth=quarter(field, 2),
             leather=quarter(field, 3))


life = Image.open(SOURCE / 'wall-life-master.png').convert('RGBA')
def wall_moss(bounds, filename):
    # Isolate olive mineral seams at the same scale as the 256-texel wall.
    # Keep their rock-grain highlights and feather the boundary into the wall.
    patch = moss_rock.crop(bounds)
    rgb = np.asarray(patch, dtype=float)
    olive = np.minimum(rgb[:, :, 0], rgb[:, :, 1]) - rgb[:, :, 2]
    coverage = np.clip((olive - 9) / 11, 0, 1)
    y, x = np.ogrid[-1:1:complex(patch.height), -1:1:complex(patch.width)]
    edge = np.clip((1 - x*x - y*y) * 4, 0, 1)
    patch.putalpha(Image.fromarray(np.uint8(230 * coverage * edge)))
    patch.save(SOURCE / filename)


wall_moss((18, 76, 232, 198), 'moss.png')
wall_moss((16, 157, 231, 248), 'moss-bank.png')
sling = quarter(life, 1)
sling.putalpha(sling.getchannel('A').point(lambda a: max(0, min(255, (a-40)*1.3))))
sling.save(SOURCE / 'sling.png')
veil = Image.open(SOURCE / 'veil-master.png').convert('RGBA')
veil.thumbnail((256, 256), Image.Resampling.LANCZOS)
veil.save(SOURCE / 'veil.png')
timber = quarter(life, 2).convert('RGB')
door = Image.open(SOURCE / 'survey-door-master.png').convert('RGB')
door.thumbnail((384, 512), Image.Resampling.LANCZOS)
door.save(SOURCE / 'survey-door.png')
shelf = quarter(life, 3).convert('RGB')
paint.update(timber=timber, mineral=ImageEnhance.Brightness(quarter(stone, 2)).enhance(.82),
             mossstone=moss_rock, cap=shelf.crop((117, 117, 148, 134)).resize((256, 256)),
             iron=ImageEnhance.Color(quarter(stone, 2)).enhance(.16))


def face(name, material, points):
    points = np.array(points, dtype=float)
    u = points[1] - points[0]
    u /= np.linalg.norm(u)
    normal = np.cross(u, points[2] - points[0])
    normal /= np.linalg.norm(normal)
    v = np.cross(normal, u)
    uv = np.column_stack((points @ u, points @ v)) * 256
    uv -= uv.min(axis=0)
    FACES.setdefault(name, []).append((material, points, normal, uv))


def stack(name, material, cx, cz, levels, sides, phase=0, irregular=0):
    # Horizontal rings. Level coordinates are height, x/z radii and center offset.
    previous = None
    for h, rx, rz, dx, dz in levels:
        ring = []
        for k in range(sides):
            a = phase + k * 2 * math.pi / sides
            r = 1 - irregular * (.5 + .5 * math.sin(3*a + h))
            ring.append((cx + dx + rx*r*math.cos(a), h, cz + dz + rz*r*math.sin(a)))
        if previous is None:
            face(name, material, ring)
        else:
            for k in range(sides):
                n = (k + 1) % sides
                # Split nonplanar, leaning facets before calculating metric UVs.
                face(name, material, [previous[k], ring[k], ring[n]])
                face(name, material, [previous[k], ring[n], previous[n]])
        previous = ring
    face(name, material, list(reversed(previous)))


def tube(name, material, centers, radius, sides=8, flatten=1):
    previous = None
    for i, point in enumerate(centers):
        tangent = np.array(centers[min(i+1, len(centers)-1)]) - centers[max(i-1, 0)]
        tangent /= np.linalg.norm(tangent)
        width = np.cross(tangent, [0., 1., 0.])
        width /= np.linalg.norm(width)
        up = np.cross(tangent, width)
        ring = [point + radius * (math.cos(a)*width + flatten*math.sin(a)*up)
                for a in np.arange(sides)*2*math.pi/sides]
        if previous is None:
            face(name, material, list(reversed(ring)))
        else:
            for k in range(sides):
                n = (k+1) % sides
                face(name, material, [previous[k], previous[n], ring[n], ring[k]])
        previous = ring
    face(name, material, previous)


def rope(name, path):
    # A slight floor-hugging sag keeps the ochre cord's silhouette irregular.
    centers = [np.array([x, .026, z]) for x, z in path]
    tube(name, 'rope', centers, .023)
    for i in (len(path)//2,):
        c = centers[i]
        tube(name, 'linen', [c + np.array([-.028, .006, 0]), c + np.array([.028, .006, 0])], .029, flatten=.85)


def write_mesh(name, texture, vertices):
    out = MODELS / (name + '.mesh.json')
    out.write_text(json.dumps(dict(name=name, texture=texture, vertices=vertices), separators=(',', ':')) + '\n')
    PARTS[name] = [asset(out)]


def atlas(group, names):
    # Pack each planar face at 256 texels/cell with extruded four-texel gutters.
    patches = []
    for name in names:
        for material, points, normal, uv in FACES[name]:
            w, h = np.ceil(uv.max(axis=0)).astype(int) + 9
            patches.append((w, h, name, material, points, normal, uv))
    patches.sort(key=lambda p: -p[1])
    width = 2048 if group == 'limestone-formations' else 1024
    placements = []
    x = y = shelf = 0
    for w, h, *_ in patches:
        if x + w > width:
            y += shelf
            x = shelf = 0
        placements.append((x, y))
        x += w
        shelf = max(shelf, h)
    height = 16 * math.ceil((y + shelf) / 16)
    if height > 4096:
        raise ValueError(f'{group} atlas exceeds 4096 pixels')
    image = Image.new('RGB', (width, height))
    meshes = {name: [] for name in names}
    for (x, y), (w, h, name, material, points, normal, uv) in zip(placements, patches):
        master = np.asarray(paint[material])
        px = np.clip(np.arange(w)-4, 0, w-9)
        py = np.clip(np.arange(h)-4, 0, h-9)
        shade = np.clip(.85 + .22*normal[1] - .16*normal[0] - .12*normal[2], .55, 1.18)
        if material in ('rock', 'mineral', 'mossstone', 'timber', 'silk', 'cloth', 'linen'):
            # Sample one continuous painting on the form. An independent random
            # patch per triangle breaks long mineral veins and strands of silk.
            u = points[1] - points[0]
            u /= np.linalg.norm(u)
            v = np.cross(normal, u)
            xyz = (points[0] + (px[None, :, None]-uv[0,0])*u/256
                   + (py[:, None, None]-uv[0,1])*v/256)
            xw, yw, zw = xyz.transpose(2,0,1)
            if material == 'silk':
                # Two wraps around the largest sac. Wind the egg-silk upward
                # continuously; its physical width narrows toward the crown.
                col = np.arctan2(zw,xw)/math.pi + .12*yw
                row = -yw
            elif material in ('rock', 'mineral', 'mossstone', 'timber'):
                # A continuous stratum runs right around the rib, preserving
                # the natural bed direction on facets of different lengths.
                col = xw + .69*zw
                row = -yw + .13*zw
                if abs(normal[1]) > .88:
                    row = zw
            else:
                col, row = xw, zw + .7*yw
            patch = master[np.floor(row*256).astype(int)%256, np.floor(col*256).astype(int)%256]
        else:
            scale = 4 if material == 'rope' else 1
            u = (px*scale + RNG.randrange(256))%256
            v = (py*scale + RNG.randrange(256))%256
            patch = master[v[:, None], u]
        image.paste(Image.fromarray(np.uint8(np.clip(patch * shade, 0, 255))), (x, y))
        tex = (uv + [x+4.5, y+4.5]) / [width, height]
        for n in range(1, len(points)-1):
            for j in (0, n, n+1):
                meshes[name].append(np.concatenate((points[j], normal, tex[j])).round(6).tolist())
    filename = SOURCE / (group + '.png')
    image.save(filename)
    for name, vertices in meshes.items():
        write_mesh(name, asset(filename), vertices)


def spire(name, cx, cz, rx, rz, height, base=0, hanging=False, phase=.17, material='rock'):
    radii = (.95,.75,.49,.18,.035)
    levels = [(base + height*t, rx*r, rz*r, .035*t, -.025*t)
              for t,r in zip((0,.24,.60,.88,1), radii)]
    if hanging:
        levels = [(2*base + height - y, x, z, dx, dz) for y,x,z,dx,dz in reversed(levels)]
    stack(name, material, cx, cz, levels, 7, phase, .14)


# Closed formations cover the absent grid faces, with a broad opaque body at eye height.
for name, rx, height in [('limestone-rib',1.48,2.25), ('high-column',.49,2.25)]:
    levels = [(0, rx, .49,0,0),(.36,rx*.97,.47,0,0),(.78,rx*.96,.45,0,0),
              (1.18,rx*.89,.41,.01,.015),(height-.25,rx*.71,.32,.03,0),(height,rx*.83,.43,0,0)]
    stack(name,'rock',0,0,levels,12,0,.025)
    spire(name, -rx*.61,.22,.18,.20,.93)
    spire(name, rx*.63,-.24,.16,.16,1.35)
# Continuous terrain fills the dry corners. The pool tile extends beneath
# the wet rib so the waterline follows the lobes of the limestone itself.
atlas('limestone-formations', ['limestone-rib', 'high-column'])

# A tapering curtain and satellite straws. Vary their profile and aspect,
# keeping each group anchored exactly to its chamber's ceiling.
ceilings = [('camp-teeth', 1.25), ('arch-teeth', 1.5), ('lookout-teeth', 1.75),
            ('vault-teeth', 2.), ('gallery-teeth', 2.25)]
for i, (name, ceiling) in enumerate(ceilings):
    longest = min(.72, ceiling - 1.02)
    for k, (x, z, rx, rz, length) in enumerate([
        (-.26, -.12, .22, .13, .77), (-.06, -.08, .17, .14, 1.),
        (.16, -.055, .14, .09, .54), (.32, -.01, .095, .085, .70),
        (-.21, .18, .065, .075, .50)]):
        h = longest * length
        spire(name, x, z if i % 2 else -z, rx, rz, h, ceiling-h, True,
              .23*(i+k), 'mineral')
atlas('calcite', [name for name, _ in ceilings])

# Flowstone ribs and bracket fungi project into the outside third of a
# perimeter cell. Local negative Z points toward the supporting wall.
for name, fungi in [('flowstone', False), ('fungus-ledge', True)]:
    for j, (x, radius, h) in enumerate([(-.31,.19,.56),(-.09,.21,.90),(.16,.18,.72),(.34,.12,.41)]):
        levels = [(0,radius,.15,0,0),(.24*h,radius*1.12,.22,0,0),
                  (.60*h,radius*.78,.16,.015,.01),(h,radius*.34,.055,-.015,-.025)]
        stack(name, 'mossstone' if fungi or j%2 else 'rock', x,-.48,levels,7,.31*j,.13)
    if fungi:
        for x,y,z,r in [(-.22,.32,-.29,.15),(-.14,.51,-.34,.13),(.03,.63,-.38,.12),
                        (.21,.34,-.32,.14),(.22,.44,-.35,.09),(-.36,.23,-.31,.085)]:
            stack(name,'cap',x,z,[(y,r*.72,r*.59,0,0),(y+.023,r,r*.85,0,0),
                  (y+.06,r*.70,r*.53,0,-.01),(y+.071,r*.24,r*.22,0,-.02)],8,.21)
atlas('cave-ledges',['flowstone','fungus-ledge'])

# A long narrow quilt and a cylindrical head roll lying alongside the west wall.
stack('bedroll','linen',-.25,0,[(.006,.20,.52,0,0),(.027,.19,.51,0,0)],8,math.pi/8)
stack('bedroll','cloth',-.25,.045,[(.023,.19,.46,0,0),(.057,.184,.45,0,0),
      (.078,.16,.43,.006,0)],8,math.pi/8)
tube('bedroll','linen',[np.array([x,.084,-.38]) for x in (-.435,-.065)],.074,12,.95)
for x in (-.37,-.13):
    tube('bedroll','leather',[np.array([u,.084,-.38]) for u in (x-.012,x+.012)],.077,12,.95)

def box(name, material, x0, x1, y0, y1, z0, z1):
    bottom = [(x0,y0,z0),(x1,y0,z0),(x1,y0,z1),(x0,y0,z1)]
    top = [(x,y1,z) for x,_,z in bottom]
    for i in range(4):
        j = (i+1)%4
        face(name, material, [bottom[i],top[i],top[j],bottom[j]])
    face(name, material, list(reversed(top)))
    face(name, material, bottom)

for name,r,h in [('crate-small',.26,.22),('crate-large',.36,.30)]:
    box(name,'timber',-r,r,0,h,-.49,-.16)
    for x in (-r+.04,r-.04):
        box(name,'iron',x-.018,x+.018,.004,h+.005,-.492,-.151)
stack('brazier','iron',0,0,[(.20,.075,.075,0,0),(.31,.17,.17,0,0),
      (.36,.18,.18,0,0),(.371,.167,.167,0,0)],12)
for i in range(3):
    a = i*2*math.pi/3
    x,z = .145*math.cos(a), .145*math.sin(a)
    stack('brazier','iron',x,z,[(0,.028,.028,0,0),(.03,.031,.031,0,0),
          (.24,.017,.017,-.4*x,-.4*z)],6)
atlas('survey-stores',['crate-small','crate-large','brazier'])

# Coalesced oval egg bundles under an opaque matte silk envelope.
stack('sealed-clutch','silk',0,0,[(.003,.42,.37,0,0),(.13,.44,.39,0,0),
      (.33,.41,.32,.007,.01),(.54,.28,.23,.03,0),(.66,.13,.11,.04,-.02),(.69,.045,.04,.06,-.03)],12)
for cx,cz,size in [(-.24,.18,.13),(.18,.22,.14),(-.32,-.10,.097)]:
    stack('sealed-clutch','silk',cx,cz,[(.01,size,size,0,0),(.13,size,size,0,0),
          (.24,size*.80,size*.85,0,0),(.36,size*.24,size*.23,0,0)],8)
rope('trail-straight',[(-.5,0),(-.28,.025),(-.02,.043),(.20,-.017),(.5,0)])
rope('trail-bend',[(-.5,0),(-.23,.01),(-.065,.07),(-.012,.26),(0,.5)])
rope('buried-cord',[(0,-.5),(.01,-.40),(-.025,-.30),(-.04,-.22)])
atlas('survey-gear',['bedroll','sealed-clutch','trail-straight','trail-bend','buried-cord'])

for name, half in [('pool-long',2.5),('pool-end',.5),('pool-inner',1.5)]:
    corners=[[-half,.18,-.5,0,1,0,0,0],[-half,.18,.5,0,1,0,0,1],
             [half,.18,.5,0,1,0,1,1],[half,.18,-.5,0,1,0,1,0]]
    write_mesh(name,WATER,[corners[i] for i in (0,1,2,0,2,3)])
manifest = dict(id='damp-caves', worldScale=1., horizon=[.23,.26,.25],
                stairWall=asset(SOURCE/'wall.png'), stairFloor=asset(SOURCE/'floor.png'),
                door=asset(SOURCE/'survey-door.png'), lockedDoor=asset(SOURCE/'survey-door.png'),
                doorEdge=asset(SOURCE/'survey-door.png'), sky=asset(SOURCE/'ceiling.png'),
                crest=asset(SOURCE/'moss.png'), vine=asset(SOURCE/'veil.png'), currentMaster=WATER,
                decorations={'damp-a':asset(SOURCE/'moss-bank.png'), 'web-a':asset(SOURCE/'web.png'), 'web-b':asset(SOURCE/'sling.png')})
PARTS['limestone-rib'] += PARTS['pool-inner']
for name in ('wall','floor','ceiling'):
    manifest[name]=asset(SOURCE/(name+'.png'))
manifest['buildings']=[dict(id=n,parts=v) for n,v in PARTS.items() if not n.startswith('pool-') and n != 'buried-cord']
manifest['buildings'].append(dict(id='tethered-clutch',parts=PARTS['sealed-clutch'] + PARTS['buried-cord']))
for n in ('north','south','west','east'):
    manifest['buildings'].append(dict(id='pool-'+n,parts=PARTS['pool-long' if n in ('north','south') else 'pool-end']))
(PACK/'pack.json').write_text(json.dumps(manifest,indent=2)+'\n')
