"""Stage Arcane Emporium painted meshes, Blender and GLB; no runtime generation.

Blender --background -noaudio --threads 2 --python tools/export_emporium.py --
  --output-root RUN/assets --report RUN/emporium-export.json
"""
import argparse
import json
import math
import sys
from pathlib import Path
import bpy
import numpy as np
from mathutils import Vector
sys.path.insert(0, str(Path(__file__).resolve().parent))
from city_architecture import House
from city_materials import extract_tiles, sample_mirrored, pack_regions, padded_patch, split_triangles
from city_mesh_export import export_collection

ROOT = Path(__file__).resolve().parent.parent
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output-root', type=Path, required=True)
parser.add_argument('--report', type=Path, required=True)
args = parser.parse_args(sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else [])
PACK = args.output_root / 'packs/sanctuary-emporium'
CITY = ROOT / 'assets/packs/sanctuary-city'
for folder in ('source', 'models'):
    (PACK / folder).mkdir(parents=True, exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)


def load(path, size=None):
    image = bpy.data.images.load(str(path), check_existing=False)
    if size:
        image.scale(*size)
    pixels = np.empty(len(image.pixels), dtype=np.float32)
    image.pixels.foreach_get(pixels)
    result = pixels.reshape(image.size[1], image.size[0], 4)[::-1].copy()
    bpy.data.images.remove(image)
    return result


def save(name, pixels):
    image = bpy.data.images.new(name, width=pixels.shape[1], height=pixels.shape[0], alpha=True)
    image.pixels.foreach_set(np.ascontiguousarray(pixels[::-1], dtype=np.float32).ravel())
    image.filepath_raw = str(PACK / 'source' / (name + '.png'))
    image.file_format = 'PNG'
    image.save()
    bpy.data.images.remove(image)


tiles = extract_tiles(load(CITY / 'source/material-master.png'))
# Joint-free wood for narrow rails, then stone-derived painted material colors.
grain = tiles[3]
strip = grain[:, int(.61 * grain.shape[1]):int(.68 * grain.shape[1])]
xx = np.arange(grain.shape[1])
sw = strip.shape[1]
xx = np.where((xx // sw) % 2, sw - 1 - xx % sw, xx % sw)
tiles.append(strip[:, xx].copy())  # 6 wood grain
for rgb in ((.18, .18, .20), (.16, .20, .42), (.90, .68, .23), (.87, .77, .53),
            (.62, .10, .09), (.08, .27, .61), (.10, .42, .24), (.65, .34, .09),
            (.36, .13, .50), (.60, .29, .12), (.19, .42, .38)):
    tile = tiles[1].copy()
    light = np.mean(tile[:, :, :3], axis=2)
    tile[:, :, :3] = light[:, :, None] * np.array(rgb) * 1.6
    tiles.append(tile)
IRON, INDIGO, GOLD, PAPER = 7, 8, 9, 10
RED, BLUE, GREEN, AMBER, VIOLET, COPPER, GLASS = range(11, 18)
EMBLEM = len(tiles)
tiles.append(load(ROOT / 'docs/mockups/magic-shop-v1/02-exterior-sign.png'))
save('floor', load(CITY / 'source/paving.png', (256, 256)) * [.85, .84, .82, 1])
save('wall', sample_mirrored(tiles[0], 256, 256, 256))
models = {}


def model(name):
    h = House(name, 0, 0, 0, 0)
    models[name] = h
    return h


def box(h, center, size, tile=3, shade=1):
    x, y, z = center
    w, high, deep = size
    h.box(x-w/2, x+w/2, y-high/2, y+high/2, z-deep/2, z+deep/2, tile, shade)


def lathe(h, x, y, z, levels, tile, sides=8):
    for (a, ra), (b, rb) in zip(levels, levels[1:]):
        for i in range(sides):
            u, v = math.tau*i/sides, math.tau*(i+1)/sides
            # Cylindrical/revolved surfaces use actual outward winding, including sloped shoulders.
            points = [(x+ra*math.cos(u), y+a, z+ra*math.sin(u)),
                      (x+ra*math.cos(v), y+a, z+ra*math.sin(v)),
                      (x+rb*math.cos(v), y+b, z+rb*math.sin(v)),
                      (x+rb*math.cos(u), y+b, z+rb*math.sin(u))]
            h.face(points, tile, (math.cos((u+v)/2), 0, math.sin((u+v)/2)),
                   shade=.85 + .12*(i % 3)/2)
    high, radius = levels[-1]
    h.face([(x+radius*math.cos(-math.tau*i/sides), y+high,
             z+radius*math.sin(-math.tau*i/sides)) for i in range(sides)], tile, (0, 1, 0))


def bottle(h, x, y, z, tile, scale=1):
    lathe(h, x, y, z, [(v*scale, r*scale) for v, r in
                       ((0, .021), (.015, .032), (.050, .032), (.068, .012),
                        (.085, .012), (.090, .017))], tile)
    lathe(h, x, y+.09*scale, z, [(0, .012*scale), (.013*scale, .012*scale)], 6)
    # Tiny pale face catches painted light without demanding transparency sorting.
    box(h, (x-.012*scale, y+.04*scale, z-.030*scale),
        (.007*scale, .023*scale, .004*scale), PAPER, .95)


def cabinet(h, width, high=.89):
    box(h, (0, high/2, .30), (width, high, .045), 6, .60)
    for x in (-width/2+.035, width/2-.035):
        box(h, (x, high/2, .16), (.07, high, .32), 6, .86)
    for y in (.055, .25, .45, .65, .865):
        box(h, (0, y, .16), (width, .035, .34), 6, .9)
    for i in range(round(width/.22)):
        x = -width/2+.12+i*.22
        box(h, (x, .15, -.008), (.20, .135, .035), 3, .85)
        box(h, (x, .16, -.03), (.025, .015, .012), GOLD)


for name in ('wall', 'exit-door', 'window-wall'):
    h = model(name)
    if name != 'window-wall':
        box(h, (0, .75, -.49), (1, 1.5, .04), 1)
    else:
        for c, s in [((0, .25, -.49), (1, .5, .04)), ((0, 1.20, -.49), (1, .6, .04)),
                     ((-.38, .7, -.49), (.24, .4, .04)), ((.38, .7, -.49), (.24, .4, .04))]:
            box(h, c, s, 1)
        box(h, (0, .7, -.475), (.52, .4, .025), GLASS, 1.5)
        for x in (-.29, .29):
            box(h, (x, .7, -.54), (.07, .48, .12), 5)
        for y in (.46, .94):
            box(h, (0, y, -.54), (.65, .07, .12), 5)
        for x in (-.16, 0, .16):
            box(h, (x, .7, -.555), (.025, .4, .025), 6)
        box(h, (0, .7, -.57), (.52, .025, .025), 6)
        for x in (-.39, .39):
            box(h, (x, .7, -.57), (.14, .40, .06), 3, .72)
    box(h, (0, .09, -.525), (1, .18, .05), 0, .88)
    for x in (-.46, .46):
        box(h, (x, .75, -.55), (.08, 1.5, .10), 6, .80)
    for y in (.20, 1.36, 1.47):
        box(h, (0, y, -.55), (1, .065, .10), 6, .85)
    if name == 'exit-door':
        box(h, (0, .34, -.58), (.43, .68, .07), 3, .72)
        for x in (-.25, .25):
            box(h, (x, .36, -.61), (.06, .72, .10), 5)
        box(h, (0, .73, -.61), (.56, .07, .10), 5)
        for y in (.16, .46):
            box(h, (0, y, -.625), (.40, .025, .025), IRON)
        box(h, (.14, .32, -.65), (.025, .05, .025), GOLD)

h = model('counter')
box(h, (0, .14, 0), (2.7, .28, .42), 3, .80)
box(h, (0, .28, 0), (2.9, .04, .52), 6)
box(h, (0, .145, -.218), (2.56, .205, .022), INDIGO)
for y in (.041, .249):
    box(h, (0, y, -.236), (2.58, .009, .012), GOLD)
for x in (-1.29, 1.29):
    box(h, (x, .145, -.236), (.009, .21, .012), GOLD)
# Supplied imagegen artwork, rather than a procedurally invented emblem.
h.face([(-.105, .04, -.247), (.105, .04, -.247), (.105, .25, -.247), (-.105, .25, -.247)],
       EMBLEM, (0, 0, -1))

h = model('potion-cabinet')
cabinet(h, 2.82)
for row, y in enumerate((.27, .47, .67)):
    for i in range(17):
        bottle(h, -1.25+i*.155, y, .09, (RED, BLUE, GREEN, AMBER, VIOLET)[(i//3+row) % 5],
               .80 + .08*(i % 3))

h = model('scroll-cabinet')
cabinet(h, 2.82)
for x in (-.69, 0, .69):
    box(h, (x, .55, .16), (.025, .60, .30), 6, .85)
# Rolled parchment lies horizontally into the cubby; radial faces reveal the roll.
for row, y in enumerate((.30, .50, .70)):
    for i in range(12):
        x = -1.25+i*.22
        # Reorient a short vertical lathe around X to make a horizontal paper roll.
        start = len(h.faces)
        lathe(h, 0, 0, 0, [(0, .025), (.10, .025)], PAPER)
        lathe(h, 0, .046, 0, [(0, .027), (.009, .027)], (RED, INDIGO)[row % 2])
        for points, _, _ in h.faces[start:]:
            for p in points:
                p.x, p.y, p.z = x+p.x, y+p.z, .14-p.y
        # Floor-facing origin end is visible from the aisle.
        h.face([(x+.025*math.cos(math.tau*k/8), y+.025*math.sin(math.tau*k/8), .039)
                for k in range(8)], PAPER, (0, 0, -1), .95)

h = model('rear-shelves')
cabinet(h, 2.82)
for i in range(21):
    x = -1.28+i*.127
    high = .13 + .02*(i % 3)
    box(h, (x, .685+high/2, .14), (.09, high, .18), (INDIGO, RED, PAPER)[i % 3], .86)
    for y in (.71, .79):
        box(h, (x, y, .043), (.075, .009, .008), GOLD)
for row, y in enumerate((.27, .47)):
    for i in range(14):
        bottle(h, -1.22+i*.185, y, .09, (BLUE, GREEN, VIOLET, AMBER)[(i+row) % 4], .88)

h = model('alchemy-bench')
box(h, (1, .14, .13), (2.8, .28, .57), 3, .82)
box(h, (1, .28, .10), (2.9, .04, .65), 6)
for x in (-.25, .25, .75, 1.25, 1.75, 2.25):
    box(h, (x, .17, -.17), (.40, .17, .035), 6, .82)
    box(h, (x, .20, -.195), (.032, .014, .012), GOLD)
# Small copper still and a bent solid spout, all in the right-hand workbench footprint.
lathe(h, .10, .30, .10, [(0, .065), (.025, .065), (.045, .042), (.095, .073),
                       (.135, .065), (.170, .025), (.195, .018)], COPPER, 12)
box(h, (.10, .327, .042), (.04, .028, .012), AMBER, 1.25)
for center, size in [((.10, .503, .10), (.035, .015, .035)),
                     ((.205, .505, .10), (.20, .018, .018)),
                     ((.305, .475, .10), (.018, .075, .018))]:
    box(h, center, size, COPPER)
bottle(h, .31, .30, .10, GLASS, 1.1)
# Mortar and pestle, jars, ingredient bundles and books.
lathe(h, .75, .30, .09, [(0, .045), (.035, .065), (.065, .065)], 0)
box(h, (.78, .38, .09), (.024, .12, .024), 0)
for i, x in enumerate((1.10, 1.40, 1.70, 2.02)):
    bottle(h, x, .30, .09, (AMBER, GREEN, VIOLET, GLASS)[i], 1.05)
herb_rack_shift = .65  # East-facing customer view: slide left onto plaster, clear of the window.
box(h, (1.4+herb_rack_shift, .84, .37), (.8, .035, .04), 6)
for i, x in enumerate((1.13, 1.38, 1.65)):
    x += herb_rack_shift
    box(h, (x, .74, .35), (.016, .19, .014), 6)
    for j in range(7):
        box(h, (x+(.018 if j % 2 else -.018), .64+j*.019, .34),
            (.055, .035, .018), (GREEN, VIOLET, AMBER)[i], .75)

# Metric face UVs, deduplicated patches and 16px gutters; artwork maps once in full.
regions, mapped = {}, {}
for name, h in models.items():
    mapped[name] = []
    for points, tile, shade in h.faces:
        n = (points[1]-points[0]).cross(points[2]-points[0]).normalized()
        axis = Vector((n.z, 0, -n.x)) if abs(n.y) < 1e-5 else (points[1]-points[0]).normalized()
        cross = Vector((0, -1, 0)) if abs(n.y) < 1e-5 else n.cross(axis)
        coords = [((p-points[0]).dot(axis)*256, (p-points[0]).dot(cross)*256) for p in points]
        lo = [min(c[k] for c in coords) for k in (0, 1)]
        hi = [max(c[k] for c in coords) for k in (0, 1)]
        w, high = [max(1, math.ceil(hi[k]-lo[k])) for k in (0, 1)]
        key = (w, high, tile, shade)
        regions[key] = None
        mapped[name].append((points, key, [(u-lo[0], v-lo[1]) for u, v in coords]))
size = 2048
positions = pack_regions([(k[0], k[1]) for k in regions], size, 16)
if positions is None:
    size = 4096
    positions = pack_regions([(k[0], k[1]) for k in regions], size, 16)
assert positions is not None, 'Emporium atlas capacity exceeded'
regions = dict(zip(regions, positions))
atlas = np.ones((size, size, 4), dtype=np.float32)
for (w, high, tile, shade), (x, y) in regions.items():
    source = tiles[tile]
    if tile == EMBLEM:
        patch = source[(np.arange(high)*source.shape[0]//high)[:, None],
                       (np.arange(w)*source.shape[1]//w)[None, :]].copy()
    else:
        patch = sample_mirrored(source, w, high, 256).copy()
    patch[:, :, :3] *= shade
    padded = padded_patch(patch, 16)
    atlas[y-16:y-16+padded.shape[0], x-16:x-16+padded.shape[1]] = padded
save('emporium-atlas', atlas)
del atlas, tiles
records, bindings = {}, {}
for name in models:
    vertices = []
    for points, key, coords in mapped[name]:
        n = (points[1]-points[0]).cross(points[2]-points[0]).normalized()
        x, y = regions[key]
        uv = [((x+u)/size, (y+v)/size) for u, v in coords]
        for i in range(1, len(points)-1):
            for k in (0, i, i+1):
                vertices.append([round(v, 6) for v in (*points[k], *n, *uv[k])])
    bindings[name] = []
    for i, part in enumerate(split_triangles(vertices)):
        partname = f'{name}-opaque-{i}'
        path = f'packs/sanctuary-emporium/models/{partname}.mesh.json'
        record = dict(name=partname, texture='packs/sanctuary-emporium/source/emporium-atlas.png',
                      cutout=False, vertices=part)
        assert all(abs(v[0]) <= 4 and abs(v[2]) <= 4 and 0 <= v[1] <= 4 for v in part), name
        (args.output_root / path).write_text(json.dumps(record, separators=(',', ':'))+'\n')
        records[path] = record
        bindings[name].append(path)
manifest = json.loads((ROOT / 'assets/packs/sanctuary-smithy/pack.json').read_text())
manifest['id'] = 'sanctuary-emporium'
for role in ('wall', 'floor'):
    manifest[role] = f'packs/sanctuary-emporium/source/{role}.png'
manifest['buildings'] = [dict(id=name, parts=parts) for name, parts in bindings.items()]
(PACK / 'pack.json').write_text(json.dumps(manifest, indent=2)+'\n')
export_collection(PACK, 'sanctuary-emporium', [(record, [(i % 4)*4, 0, (i//4)*4])
                                             for i, record in enumerate(records.values())])
bounds = {name: dict(min=[min(v[k] for p in parts for v in records[p]['vertices']) for k in range(3)],
                     max=[max(v[k] for p in parts for v in records[p]['vertices']) for k in range(3)])
          for name, parts in bindings.items()}
for name in ('counter', 'potion-cabinet', 'scroll-cabinet', 'rear-shelves'):
    b = bounds[name]
    assert -.5 <= b['min'][2] <= b['max'][2] <= .5, name
    assert -1.5 <= b['min'][0] <= b['max'][0] <= 1.5, name
b = bounds['alchemy-bench']
assert -.5 <= b['min'][0] <= b['max'][0] <= 2.5
assert -.5 <= b['min'][2] <= b['max'][2] <= .5
assert abs(bounds['counter']['max'][1]-.30) < .001
assert bounds['alchemy-bench']['max'][1] < .9
args.report.write_text(json.dumps(dict(atlas_size=size, texels_per_cell=256, bounds=bounds,
    models={name: sum(len(records[p]['vertices']) for p in parts) for name, parts in bindings.items()}),
    indent=2)+'\n')
print('Exported', len(models), 'emporium models', flush=True)
