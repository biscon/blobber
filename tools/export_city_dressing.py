"""SC03 deterministic planting, stalls, fountain, ivy and sign authoring.
Run in background Blender after architecture/tree, or alone for dressing updates.
Retains generated masters; exports packed source, unlit GLB and runtime meshes.
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
parser.add_argument('--output-root', type=Path, default=ROOT / 'assets',
                    help='Staged assets root containing a complete sanctuary-city pack')
args = parser.parse_args(sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else [])
ASSETS = args.output_root
PACK = ASSETS / 'packs/sanctuary-city'
bpy.ops.wm.read_factory_settings(use_empty=True)


def load(name, size=None):
    image = bpy.data.images.load(str(PACK / 'source' / (name + '.png')), check_existing=False)
    if size:
        image.scale(*size)
    data = np.empty(len(image.pixels), dtype=np.float32)
    image.pixels.foreach_get(data)
    data = data.reshape(image.size[1], image.size[0], 4)[::-1].copy()
    bpy.data.images.remove(image)
    return data


def save(name, data):
    image = bpy.data.images.new(name, width=data.shape[1], height=data.shape[0], alpha=True)
    image.pixels.foreach_set(np.ascontiguousarray(data[::-1]).ravel())
    image.filepath_raw = str(PACK / 'source' / (name + '.png'))
    image.file_format = 'PNG'
    image.save()
    bpy.data.images.remove(image)


tiles = extract_tiles(load('material-master'))
# Reuse established timber/stone; these are material recolors, not drawn emblems.
tiles.append(load('soil-master'))  # 6 exposed earth, not recolored paving
iron = tiles[5].copy()
iron[:, :, :3] = np.mean(iron[:, :, :3], axis=2)[:, :, None] * np.array([.23, .24, .25])
tiles.append(iron)  # 7 iron
for name in ('inn-sign', 'smithy-sign', 'provisioner-sign', 'market-cloth'):
    tiles.append(load(name))  # 8..11
cloth = tiles[11].copy()
cloth[:, :, :3] *= np.array([.72, .88, .92])
tiles.append(cloth)  # 12 faded green cloth
sack = tiles[11].copy()
sack[:, :, :3] = np.mean(sack[:, :, :3], axis=2)[:, :, None] * np.array([.80, .70, .49])
tiles.append(sack)  # 13 grain sacks
tiles.append(load('marble-master'))  # 14 pale fountain marble

# Shared cutout sheet: transparent gutters and metric patches for each size.
cutout = np.zeros((1024, 1024, 4), dtype=np.float32)
patches = {'bush': (16, 16, 144, 144), 'east': (192, 16, 288, 288), 'ivy': (512, 16, 224, 448)}
for name, (x, y, w, h) in patches.items():
    cutout[y:y+h, x:x+w] = load('ivy-master' if name == 'ivy' else 'foliage-master', (w, h))
save('dressing-cutout', cutout)
del cutout

# Fixed opaque water geometry. Offline pixel-stepped ripple phases, padded 4x2 atlas.
base = load('water-master', (480, 480))
water = np.ones((1024, 2048, 4), dtype=np.float32)
yy, xx = np.mgrid[:480, :480]
for phase in range(8):
    shift = (0, 2, 4, 2, 0, -2, -4, -2)[phase]
    frame = np.roll(base, shift, axis=1).copy()
    # Four restrained expanding splash rings meet the falling streams.
    for cx, cy in ((240-113, 240), (240+113, 240), (240, 240-113), (240, 240+113)):
        radius = np.sqrt(((xx//3)*3-cx)**2 + ((yy//3)*3-cy)**2)
        ring = (np.mod(radius - phase*3, 24) < 2) & (radius < 32) & (((xx+yy)//12) % 3 != 0)
        frame[ring, :3] = frame[ring, :3] * .68 + np.array([.49, .66, .61]) * .32
    # Reserved strip outside the basin UV disc: opaque stream with falling pixel glints.
    for row in range(480):
        glint = ((row - phase*6) % 48) < 12
        frame[row, 16:29, :3] = np.array([.24, .48, .45])
        frame[row, 19:23, :3] = np.array([.64, .77, .72]) if glint else np.array([.38, .61, .57])
    padded = np.pad(frame, ((16, 16), (16, 16), (0, 0)), mode='edge')
    water[(phase//4)*512:(phase//4+1)*512, (phase%4)*512:(phase%4+1)*512] = padded
save('fountain-water', water)
del water, base

models, special = {}, {}

def model(name):
    h = House(name, 0, 0, 0, 0)
    models[name] = h
    special[name] = []
    return h


def card(h, center, size, angle, patch='bush', tilt=.1, low=.012, high=4):
    x, y, w, hh = patches[patch]
    uv = [(x/1024, (y+hh)/1024), ((x+w)/1024, (y+hh)/1024),
          ((x+w)/1024, y/1024), (x/1024, y/1024)]
    center = Vector(center)
    u = Vector((math.cos(angle), 0, math.sin(angle))) * size[0]/2
    v = Vector((-math.sin(angle)*tilt, math.sqrt(1-tilt*tilt), math.cos(angle)*tilt)) * size[1]/2
    points = [center-u-v, center+u-v, center+u+v, center-u+v]
    # Clip along each sloped edge and preserve texture coordinates. Bush foliage
    # penetrates the soil; facade cards terminate below their actual eaves.
    for bottom, top in ((0, 3), (1, 2)):
        a, b = points[bottom].copy(), points[top].copy()
        ua, ub = Vector(uv[bottom]), Vector(uv[top])
        for index, height in ((bottom, max(low, a.y)), (top, min(high, b.y))):
            t = (height-a.y)/(b.y-a.y)
            points[index] = a.lerp(b, t)
            uv[index] = tuple(ua.lerp(ub, t))
    assert all(low-1e-6 <= p.y <= high+1e-6 for p in points)
    special[h.name].append(('cutout', points, uv))
    special[h.name].append(('cutout', list(reversed(points)), list(reversed(uv))))


def bush(h, x, z):
    h.box(x-.018,x+.018,.025,.15,z-.018,z+.018,3,.65)
    for i in range(3):
        a = i*math.pi/3
        card(h, (x+.07*math.cos(a*3), .14, z+.07*math.sin(a*3)), (144/256, .42), a, tilt=.25)


def bed(h, x0, x1, z0, z1):
    h.box(x0+.03, x1-.03, .005, .035, z0+.03, z1-.03, 6)
    for a,b in ((x0,x0+.07),(x1-.07,x1)):
        h.box(a,b,.01,.105,z0,z1,5,.83)
    for a,b in ((z0,z0+.07),(z1-.07,z1)):
        h.box(x0+.07,x1-.07,.01,.105,a,b,5,.83)


h = model('courtyard-garden')
bed(h, -1.5, 1.5, -1.5, .5)
for x,z in ((-1.02,-1.02),(.98,-1.05),(-1.08,.04),(1.07,.06)):
    bush(h,x,z)
h = model('lane-garden')
bed(h,-.5,1.5,-.5,.5)
for x in (-.08,.55,1.10):
    bush(h,x,0)
h = model('east-garden')
bed(h,-.5,1.5,-.5,1.5)
for x,z in ((-.11,1.03),(1.08,1.02),(1.06,-.10)):
    bush(h,x,z)

# Compact timber stalls: real posts, counter, planks, crates and peaked cloth roof.
for name, cloth_tile in (('market-produce',11),('market-grain',12)):
    h = model(name)
    for x in (-.42,.42):
        for z in (-.39,1.32):
            h.box(x-.035,x+.035,0,1.32,z-.035,z+.035,3,.78)
    h.box(-.46,.46,.43,.50,-.44,.04,3)
    h.box(-.43,.43,.08,.43,-.40,-.34,3,.75)
    for x in (-.42,.40):
        h.box(x,x+.035,.06,.43,-.35,1.30,3,.75)
    for z in (.48,1.22):
        h.box(-.42,.42,.08,.14,z,z+.06,3,.78)
    # Canopy sloped across its width, with a narrow hanging valance on the front.
    for a,b,ya,yb in ((-.49,0,1.29,1.48),(0,.49,1.48,1.29)):
        pts=[(a,ya,-.55),(b,yb,-.55),(b,yb,1.43),(a,ya,1.43)]
        h.face(pts,cloth_tile,(0,1,0))
        h.face(list(reversed(pts)),cloth_tile,(0,-1,0),.8)
        h.face([(a,ya,-.55),(b,yb,-.55),(b,yb-.13,-.55),(a,ya-.13,-.55)],cloth_tile,(0,0,-1),.88)
    for x in (-.27,.26):
        h.box(x-.16,x+.16,.05,.30,.68,1.02,3,.78)
        h.box(x-.17,x+.17,.29,.33,.67,1.03,3)
    # Small dimensional sacks/loaves reuse painted fabric/wood; no icon artwork.
    for x in (-.28,0,.28):
        h.box(x-.09,x+.09,.50,.60,-.30,-.09,13 if name=='market-grain' else 3,.95)

    # Author half-size geometry before metric UV packing. Center the old depth
    # span (-.55..1.43) in its new single-cell base; no runtime instance scaling.
    for points, _, _ in h.faces:
        for p in points:
            p.x *= .5
            p.y *= .5
            p.z = (p.z-.44)*.5

# Basin/profile rings: a low, broad fountain leaves the square sightlines open.
h = model('fountain')
def ring(radius, y, count=16):
    return [(0.5+radius*math.cos(i*2*math.pi/count),y,0.5+radius*math.sin(i*2*math.pi/count)) for i in range(count)]
def profile(rings, tile=5):
    for (r0,y0),(r1,y1) in zip(rings,rings[1:]):
        aa,bb=ring(r0,y0),ring(r1,y1)
        for i in range(16):
            j=(i+1)%16
            pts=[aa[i],bb[i],bb[j],aa[j]]
            # Profile traverses upward outer wall then inward/downward inner wall.
            h.face(pts,tile,shade=1.0 if y1>y0 else .92)
profile([(.86,.01),(.86,.08),(.78,.10),(.78,.24),(.83,.25),(.83,.30),(.66,.30),(.66,.26),(.11,.26)])
profile([(.17,.26),(.17,.33),(.11,.37),(.095,.67),(.30,.70),(.34,.79),(.25,.82),(.04,.82),(.04,.92),(.0,.96)])
# Top-down opaque water discs; atlas first frame authored with 16px gutters.
for radius,y in ((.655,.275),(.245,.825)):
    pts=list(reversed(ring(radius,y)))
    uv=[((256+(p[0]-.5)*256)/2048,(256+(p[2]-.5)*256)/1024) for p in pts]
    special[h.name].append(('water',pts,uv))
# Six-sided closed stream tubes retain width from all azimuths. Fixed geometry
# shares the existing water atlas, visibility/depth cache and eight-frame clock.
stream_sides = 6
for i in range(4):
    a=i*math.pi/2
    path=[(.27,.80),(.34,.74),(.41,.57),(.45,.28)]
    centers=[Vector((.5+r*math.cos(a),y,.5+r*math.sin(a))) for r,y in path]
    tangent=Vector((-math.sin(a),0,math.cos(a)))
    rings=[]
    distance=[0.0]
    for j,center in enumerate(centers):
        direction=(centers[min(j+1,len(centers)-1)]-centers[max(j-1,0)]).normalized()
        radial=direction.cross(tangent).normalized()
        radius=.021 if j<3 else .024
        rings.append([center+radius*(math.cos(k*2*math.pi/stream_sides)*tangent+
                                      math.sin(k*2*math.pi/stream_sides)*radial)
                      for k in range(stream_sides)])
        if j:
            distance.append(distance[-1]+(center-centers[j-1]).length)
    for j in range(len(rings)-1):
        for k in range(stream_sides):
            n=(k+1)%stream_sides
            pts=[rings[j][k],rings[j][n],rings[j+1][n],rings[j+1][k]]
            # Each face repeats the narrow glint strip; V follows physical length.
            uv=[(32/2048,(80+distance[j]*256)/1024), (45/2048,(80+distance[j]*256)/1024),
                (45/2048,(80+distance[j+1]*256)/1024), (32/2048,(80+distance[j+1]*256)/1024)]
            normal=(pts[1]-pts[0]).cross(pts[2]-pts[0])
            outward=(pts[0]+pts[1]+pts[2]+pts[3])/4-(centers[j]+centers[j+1])/2
            if normal.dot(outward)<0:
                pts.reverse();uv.reverse()
            special[h.name].append(('water',pts,uv))
    for end in (0,len(rings)-1):
        pts=rings[end].copy()
        outward=centers[end]-centers[1 if end==0 else end-1]
        if (pts[1]-pts[0]).cross(pts[2]-pts[0]).dot(outward)<0:
            pts.reverse()
        special[h.name].append(('water',pts,[(38/2048,(80+distance[end]*256)/1024)]*len(pts)))

# Signs project perpendicular from the front wall beside, above the door.
for name,front,tile in (('inn',-2.5,8),('smithy',-1.5,9),('provisioner',-1.5,10)):
    h=model(name+'-sign')
    # Inn upper window ends at x=1.08; mount on plaster before the corner timber.
    x=1.25 if name=='inn' else .70
    h.box(x-.022,x+.022,1.53,1.57,front-.68,front+.025,7)
    for z in (front-.57,front-.19):
        h.box(x-.012,x+.012,1.41,1.54,z-.012,z+.012,7)
    h.box(x-.026,x+.026,1.02,1.42,front-.64,front-.14,3,.85)
    # Two readable art faces; original generated square remapped to 128px square.
    for side in (-1,1):
        xx=x+side*.027
        h.face([(xx,1.02,front-.64),(xx,1.02,front-.14),(xx,1.42,front-.14),(xx,1.42,front-.64)],tile,(side,0,0))

for name,front,eaves in (('low-house-hip-ivy',-1.5,1.20),('tall-house-ivy',-2.5,2.15)):
    h=model(name)
    # Front corner away from door, plus sparse side clump away from windows.
    card(h,(-1.04,.91,front-.012),(.875,1.75),0,'ivy',0,high=eaves-.04)
    card(h,(-1.512,.91,front+.55),(.875,1.75),math.pi/2,'ivy',0,high=eaves-.04)

# Pack shared metric material patches; signs deliberately map to full artwork.
regions, mapped = {}, {}
for name,h in models.items():
    mapped[name]=[]
    for points,tile,shade in h.faces:
        n=(points[1]-points[0]).cross(points[2]-points[0]).normalized()
        if abs(n.y)<1e-5:
            axis=Vector((n.z,0,-n.x))
            cross=Vector((0,-1,0))
        else:
            axis=(points[1]-points[0]).normalized()
            cross=n.cross(axis)
        coords=[((p-points[0]).dot(axis)*256,(p-points[0]).dot(cross)*256) for p in points]
        lo=[min(p[k] for p in coords) for k in (0,1)]
        hi=[max(p[k] for p in coords) for k in (0,1)]
        w,hh=[max(1,math.ceil(hi[k]-lo[k])) for k in (0,1)]
        key=(w,hh,tile,shade)
        regions[key]=None
        mapped[name].append((points,key,[(u-lo[0],v-lo[1]) for u,v in coords]))
atlas_size=2048
positions=pack_regions([(k[0],k[1]) for k in regions],atlas_size,16)
if positions is None:
    atlas_size=4096
    positions=pack_regions([(k[0],k[1]) for k in regions],atlas_size,16)
assert positions is not None,'Dressing atlas capacity exceeded'
regions=dict(zip(regions,positions))
atlas=np.ones((atlas_size,atlas_size,4),dtype=np.float32)
for (w,hh,tile,shade),(x,y) in regions.items():
    source=tiles[tile]
    if tile in (8,9,10):
        # Full sign, correct upright orientation for vertical side faces.
        patch=source[(np.arange(hh)*source.shape[0]//hh)[:,None],(np.arange(w)*source.shape[1]//w)[None,:]].copy()
    else:
        patch=sample_mirrored(source,w,hh,256 if tile not in (11,12) else 512)
    patch[:,:,:3]*=shade
    pad=padded_patch(patch,16)
    atlas[y-16:y-16+pad.shape[0],x-16:x-16+pad.shape[1]]=pad
save('dressing-atlas',atlas)
del atlas,tiles

records,bindings = {},{}
def emit(vertices,points,coords):
    points=[Vector(p) for p in points]
    n=(points[1]-points[0]).cross(points[2]-points[0]).normalized()
    for i in range(1,len(points)-1):
        if (points[i]-points[0]).cross(points[i+1]-points[0]).length<1e-8:
            continue
        for k in (0,i,i+1):
            vertices.append([round(v,6) for v in (*points[k],*n,*coords[k])])
def write_part(name,vertices,kind):
    for i,part in enumerate(split_triangles(vertices)):
        partname='dressing-'+name+('-'+str(i) if i else '')
        record=dict(name=partname,texture='packs/sanctuary-city/source/'+{'opaque':'dressing-atlas','cutout':'dressing-cutout','water':'fountain-water','bark':'tree-bark'}[kind]+'.png',
                    cutout=kind=='cutout',water=kind=='water',vertices=part)
        assert all(abs(v[0])<=4 and abs(v[2])<=4 and 0<=v[1]<=4 for v in part),(name,'bounds')
        path='packs/sanctuary-city/models/'+partname+'.mesh.json'
        (ASSETS/path).write_text(json.dumps(record,separators=(',',':'))+'\n')
        records[path]=record
        yield path
for name in models:
    kinds={'opaque':[],'cutout':[],'water':[]}
    for points,key,coords in mapped[name]:
        x,y=regions[key]
        emit(kinds['opaque'],points,[((x+u)/atlas_size,(y+v)/atlas_size) for u,v in coords])
    for kind,points,uv in special[name]:
        emit(kinds[kind],points,uv)
    bindings[name]=[path for kind,verts in kinds.items() if verts for path in write_part(name+'-'+kind,verts,kind)]

# Smaller eastern tree derived from the approved tree. Rescale UVs with geometry.
for kind in ('bark','foliage'):
    original=json.loads((PACK/'models'/('courtyard-tree-'+kind+'.mesh.json')).read_text())
    verts=[]
    for source in original['vertices']:
        v=source.copy()
        v[0]=v[0]*.78+.27;v[2]=v[2]*.78+.38
        sy=.86 if kind=='bark' else .78
        v[1]=v[1]*sy if kind=='bark' else 1.02+(v[1]-1.00938)*sy
        n=Vector((v[3]/.78,v[4]/sy,v[5]/.78)).normalized()
        v[3:6]=list(n)
        if kind=='foliage':
            v[6]=(192+(v[6]*512-76)*.8)/1024
            v[7]=(16+(v[7]*512-76)*.8)/1024
        else:
            v[6]*=.78;v[7]*=.86
        verts.append(v)
    bindings['east-garden']+=list(write_part('east-tree-'+kind,verts,'bark' if kind=='bark' else 'cutout'))

manifest=json.loads((PACK/'pack.json').read_text())
manifest['daylightFog']=[4,12,.22]
manifest['viewDistance']=40
base={b['id']:[p for p in b['parts'] if '/dressing-' not in p] for b in manifest['buildings']}
base={name:parts for name,parts in base.items() if parts and name not in ('low-house-hip-ivy','tall-house-ivy','courtyard-garden')}
for name in ('inn','smithy','provisioner'):
    base[name]+=bindings[name+'-sign']
for name in ('low-house-hip','tall-house'):
    base[name+'-ivy']=base[name]+bindings[name+'-ivy']
for name in ('courtyard-garden','lane-garden','east-garden','market-produce','market-grain','fountain'):
    base[name]=bindings[name]
base['courtyard-garden']=base['courtyard-tree']+base['courtyard-garden']
manifest['buildings']=[dict(id=name,parts=parts) for name,parts in base.items()]
assert len(base)<=24 and all(len(parts)<=8 for parts in base.values())
(PACK/'pack.json').write_text(json.dumps(manifest,indent=2)+'\n')
# Source collection contains complete dressed models including reused architecture/tree.
collection=[]
report=[]
for i,name in enumerate(['courtyard-garden','lane-garden','east-garden','market-produce','market-grain','fountain','inn','smithy','provisioner','low-house-hip-ivy','tall-house-ivy']):
    total=0
    for path in base[name]:
        record=records.get(path) or json.loads((ASSETS/path).read_text())
        total+=len(record['vertices'])
        collection.append((record,[(i%4)*8,0,(i//4)*8]))
    assert total<=24000,(name,total)
    report.append(dict(model=name,parts=len(base[name]),triangles=total//3))
export_collection(PACK,'city-dressing',collection)
(PACK/'models/dressing-export-report.json').write_text(json.dumps(dict(models=report,opaque_atlas=atlas_size,cutout_atlas=1024,water_atlas=[2048,1024],water_frames=8,fps=8,texels_per_cell=256,gutter=16,stall_scale=.5,stream_sides=stream_sides,bush_top=.344,ivy_eaves_clearance=.04),indent=2)+'\n')
print('SC03 dressing:',report,flush=True)
