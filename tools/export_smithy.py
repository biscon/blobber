"""Rebuild the authored smithy: bounded offline Blender, painted meshes, metric UVs.

Run with -- --output-root RUN/assets --report RUN/export.json, inspect, then
promote the resulting packs/sanctuary-smithy into assets/. No runtime Blender.
"""
import argparse
import json, math, sys
from pathlib import Path
import bpy
import numpy as np
from mathutils import Vector
sys.path.insert(0, str(Path(__file__).resolve().parent))
from city_architecture import House
from city_materials import extract_tiles, sample_mirrored, pack_regions, padded_patch, split_triangles
from city_mesh_export import export_collection
ROOT = Path(__file__).resolve().parent.parent
parser = argparse.ArgumentParser()
parser.add_argument('--output-root', type=Path, required=True)
parser.add_argument('--report', type=Path, required=True)
args = parser.parse_args(sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else [])
PACK = args.output_root / 'packs/sanctuary-smithy'
CITY = ROOT / 'assets/packs/sanctuary-city'
for folder in ('source', 'models'):
    (PACK / folder).mkdir(parents=True, exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
def load(path, size=None):
    im=bpy.data.images.load(str(path),check_existing=False)
    if size: im.scale(*size)
    v=np.empty(len(im.pixels),dtype=np.float32);im.pixels.foreach_get(v)
    a=v.reshape(im.size[1],im.size[0],4)[::-1].copy();bpy.data.images.remove(im);return a
def save(name,a):
    im=bpy.data.images.new(name,width=a.shape[1],height=a.shape[0],alpha=True)
    im.pixels.foreach_set(np.ascontiguousarray(a[::-1], dtype=np.float32).ravel());im.filepath_raw=str(PACK/'source'/f'{name}.png')
    im.file_format='PNG';im.save();bpy.data.images.remove(im)
tiles = extract_tiles(load(CITY/'source/material-master.png'))
# Preserve established city tile numbering; additional fields are painted material variants.
tiles.append(tiles[0].copy())
iron = tiles[5].copy()
iron[:, :, :3] = np.mean(iron[:, :, :3], axis=2)[:, :, None] * [.28, .30, .32]
tiles.append(iron)  # 7: dull iron
charcoal = tiles[0].copy(); charcoal[:, :, :3] *= [.12, .115, .11]
tiles.append(charcoal)  # 8
# Joint-free timber grain prevents seams cutting across narrow structural members.
grain = tiles[3]; strip = grain[:, int(.61*grain.shape[1]):int(.68*grain.shape[1])]
xx = np.arange(grain.shape[1]); sw = strip.shape[1]
xx = np.where((xx//sw)%2, sw-1-xx%sw, xx%sw)
tiles.append(strip[:, xx].copy())  # 9
coal = charcoal.copy(); coal[:, :, :3] = coal[:, :, :3] * .7 + [.32, .075, .012]
tiles.append(coal)  # 10: painted embers
water = iron.copy(); water[:, :, :3] *= [.55, .72, .90]
tiles.append(water)  # 11: static dark quench water
save('floor', load(CITY/'source/paving.png', (256,256)) * [.85,.84,.82,1])
save('wall', sample_mirrored(tiles[0],256,256,256))
models = {}; special = {}
def model(name):
    h=House(name,0,0,0,0);models[name]=h;special[name]=[];return h
def box(h,c,s,t=3,shade=1):
    x,y,z=c;w,hh,d=s;h.box(x-w/2,x+w/2,y-hh/2,y+hh/2,z-d/2,z+d/2,t,shade)
def cylinder(h,x,y,z,r,height,t=3,n=12):
    levels=[(y,r*.86),(y+height*.15,r),(y+height*.85,r),(y+height,r*.86)]
    for (a,ra),(b,rb) in zip(levels,levels[1:]):
        for i in range(n):
            u,v=math.tau*i/n,math.tau*(i+1)/n
            h.face([(x+ra*math.cos(u),a,z+ra*math.sin(u)),(x+ra*math.cos(v),a,z+ra*math.sin(v)),
                    (x+rb*math.cos(v),b,z+rb*math.sin(v)),(x+rb*math.cos(u),b,z+rb*math.sin(u))],t,
                   (math.cos((u+v)/2),0,math.sin((u+v)/2)),shade=.85+.1*(i%3)/2)
            # Prepared meshes use back-face culling: every side must face out.
            pts,_,_=h.faces[-1]
            normal=(pts[1]-pts[0]).cross(pts[2]-pts[0])
            assert normal.dot(Vector((math.cos((u+v)/2),0,math.sin((u+v)/2))))>0
    h.face([(x+levels[-1][1]*math.cos(-math.tau*i/n),y+height,z+levels[-1][1]*math.sin(-math.tau*i/n)) for i in range(n)],t,(0,1,0))
def barrel(h,x,z,y=0,r=.24,height=.55):
    cylinder(h,x,y,z,r,height)
    for yy in (y+.10,y+height-.12): cylinder(h,x,yy,z,r+.012,.035,7)
# Wall backs stay in the solid boundary; a thin modeled face owns beams and trim.
for name in ('wall','exit-door','window-wall'):
    h=model(name)
    if name != 'window-wall':
        box(h,(0,.75,-.49),(1,1.5,.04),1)
    else:
        # A recessed, closed window: split the wall face around the opening.
        for c,s in [((0,.25,-.49),(1,.5,.04)),((0,1.20,-.49),(1,.6,.04)),
                    ((-.38,.7,-.49),(.24,.4,.04)),((.38,.7,-.49),(.24,.4,.04))]:box(h,c,s,1)
        box(h,(0,.7,-.475),(.52,.4,.025),11,1.7)
        for x in (-.29,.29):box(h,(x,.7,-.54),(.07,.48,.12),5)
        for y in (.46,.94):box(h,(0,y,-.54),(.65,.07,.12),5)
        for x in (-.16,0,.16):box(h,(x,.7,-.555),(.025,.4,.025),9)
        box(h,(0,.7,-.57),(.52,.025,.025),9)
        for x in (-.39,.39):
            box(h,(x,.7,-.57),(.14,.40,.06),3,.72)
            for y in (.56,.83):box(h,(x,y,-.608),(.14,.025,.018),7)
    box(h,(0,.09,-.525),(1,.18,.05),0,.88)
    for x in (-.46,.46):box(h,(x,.75,-.55),(.08,1.5,.10),9,.80)
    for y in (.20,1.36,1.47):box(h,(0,y,-.55),(1,.065,.10),9,.85)
    if name=='exit-door':
        box(h,(0,.34,-.58),(.43,.68,.07),3,.72)
        for x in (-.25,.25):box(h,(x,.36,-.61),(.06,.72,.10),5)
        box(h,(0,.73,-.61),(.56,.07,.10),5)
        for y in (.16,.46):box(h,(0,y,-.625),(.40,.025,.025),7)
        box(h,(.14,.32,-.65),(.025,.05,.025),7)

h=model('counter')
box(h,(0,.14,0),(2.7,.28,.42),3,.78)
box(h,(0,.28,0),(2.9,.04,.52),9)
for x in (-1.30,1.30):box(h,(x,.14,-.23),(.07,.28,.06),9,.88)
for x in np.linspace(-1.2,1.2,17):box(h,(float(x),.14,-.218),(.012,.25,.016),9,.48)
for y in (.025,.245):box(h,(0,y,-.235),(2.6,.025,.025),9,.84)
# Small closed tool chest; leave the middle of the counter and NPC silhouette open.
box(h,(1.02,.345,.02),(.24,.09,.18),3,.72)
for x in (.94,1.10):box(h,(x,.35,-.079),(.018,.10,.018),7)

h=model('shelves')
box(h,(0,.44,-.32),(2.8,.88,.045),9,.63)
for x in (-1.35,1.35):box(h,(x,.46,-.22),(.07,.92,.28),9)
for y in (.08,.42,.77):box(h,(0,y,-.22),(2.8,.045,.31),9)
for x in (-1.02,-.74,-.46,.68,.97):
    box(h,(x,.12,-.22),(.22,.045,.13),7,.9)
    box(h,(x+.025,.165,-.22),(.18,.045,.11),7,1.1)
for x in (-.8,-.45,0,.4,.9):
    box(h,(x,.57,-.33),(.018,.20,.025),9,.85)
    box(h,(x,.65,-.30),(.10,.05,.05),7,1.1)
for x in (-.15,.10,.35):box(h,(x,.825,-.22),(.18,.06,.12),7)

h=model('forge')
# Local width two cells; east-facing instance maps it into cells (2,2) and (2,3).
box(h,(.5,.11,0),(1.76,.22,.86),0,.82)
box(h,(.5,.235,-.02),(1.84,.05,.90),5)
box(h,(.5,.47,.30),(1.70,.45,.16),0,.23)
for x in (-.22,1.22):box(h,(x,.47,.01),(.27,.42,.74),5,.83)
box(h,(.5,.69,.02),(1.78,.08,.80),5,.88)
# Tapered chimney hood with closed, outward-facing sides and dark underside.
lo=[(-.39,.73,-.40),(1.39,.73,-.40),(1.39,.73,.40),(-.39,.73,.40)]
hi=[(.06,1.08,-.15),(.94,1.08,-.15),(.94,1.08,.38),(.06,1.08,.38)]
for i in range(4):
    j=(i+1)%4
    h.face([lo[i],hi[i],hi[j],lo[j]],0,((0,0,-1),(1,0,0),(0,0,1),(-1,0,0))[i],.78)
box(h,(.5,1.29,.115),(.88,.42,.53),0,.72)
box(h,(.5,.265,-.12),(1.04,.025,.36),8,.6)
for i in range(11):
    x=.06+(i%6)*.17; z=-.26+(i//6)*.17
    box(h,(x,.282,z),(.11,.035,.10),10,.7+(i%3)*.10)
# A small tool rest at the front edge, fully inside the forge's reservation.
box(h,(.5,.35,-.405),(.85,.018,.025),7)
for x in (.15,.85):box(h,(x,.31,-.405),(.018,.08,.018),7)

h=model('anvil')
cylinder(h,0,0,0,.18,.16,9)
for y in (.03,.12):cylinder(h,0,y,0,.185,.02,7)
box(h,(0,.172,0),(.27,.025,.19),7)
box(h,(0,.205,0),(.12,.055,.12),7,.80)
box(h,(-.015,.252,0),(.30,.035,.15),7,1.2)
# Pointed solid horn and flat heel, no intersecting coplanar caps.
h.face([(.135,.27,-.065),(.135,.27,.065),(.32,.258,0)],7,(0,1,0),1.2)
h.face([(.135,.235,-.065),(.32,.258,0),(.135,.27,-.065)],7,(0,0,-1))
h.face([(.135,.27,.065),(.32,.258,0),(.135,.235,.065)],7,(0,0,1))
h.face([(.135,.235,-.065),(.135,.235,.065),(.32,.258,0)],7,(0,-1,0),.6)

h=model('quench-tub')
# Open top and visible inner wall rather than a solid lid.
n=16; ro=.22; ri=.19; high=.25
for i in range(n):
    a,b=math.tau*i/n,math.tau*(i+1)/n
    def p(t,r,y):return (r*math.cos(t),y,r*math.sin(t))
    outward=(math.cos((a+b)/2),0,math.sin((a+b)/2))
    h.face([p(a,ro,0),p(b,ro,0),p(b,ro,high),p(a,ro,high)],3,outward,.85)
    h.face([p(a,ri,.05),p(a,ri,high),p(b,ri,high),p(b,ri,.05)],3,tuple(-v for v in outward),.65)
    h.face([p(a,ro,high),p(b,ro,high),p(b,ri,high),p(a,ri,high)],9,(0,1,0))
for y in (.035,.19):
    # Thin solid hoops assembled from straight faces; preserve the open top.
    for i in range(n):
        a,b=math.tau*i/n,math.tau*(i+1)/n
        h.face([p(a,ro+.004,y),p(b,ro+.004,y),p(b,ro+.004,y+.018),p(a,ro+.004,y+.018)],7,
               (math.cos((a+b)/2),0,math.sin((a+b)/2)))
h.face([p(-math.tau*i/n,ri,.205) for i in range(n)],11,(0,1,0),.8)

h=model('charcoal')
box(h,(0,.07,0),(.43,.14,.35),9,.72)
box(h,(0,.145,0),(.39,.015,.31),8)
for i in range(12):
    x=-.14+(i%4)*.09; z=-.10+(i//4)*.09
    box(h,(x,.163+(i%2)*.015,z),(.075,.045,.07),8,.65+(i%3)*.12)
for x in (-.23,.23):box(h,(x,.11,0),(.04,.22,.42),3,.8)
for z in (-.19,.19):box(h,(0,.11,z),(.43,.22,.035),3,.8)

h=model('iron-stock')
for x in (-.22,.22):box(h,(x,.10,0),(.045,.20,.54),9,.8)
for y in (.04,.16):box(h,(0,y,0),(.48,.04,.54),9)
for i in range(5):
    box(h,(-.16+i*.08,.198+(i%2)*.025,0),(.05,.035,.65),7,.86+(i%2)*.16)

h=model('equipment')
# Three-cell wall display, rotated west. Each group remains inside its cell.
for x in (0,1,2):
    for dx in (-.34,.34):box(h,(x+dx,.28,.22),(.05,.56,.06),9,.8)
    for y in (.11,.42):box(h,(x,y,.20),(.75,.055,.07),9)
    for dx in (-.20,.14):
        sx=x+dx
        sword_start=len(h.faces)
        box(h,(sx,.37,.12),(.045,.42,.022),7,1.15)
        h.face([(sx-.022,.16,.108),(sx+.022,.16,.108),(sx,.09,.108)],7,(0,0,-1))
        box(h,(sx,.59,.12),(.15,.025,.045),7)
        box(h,(sx,.65,.12),(.025,.10,.035),9,.6)
        cylinder(h,sx,.70,.12,.027,.025,7,n=8)
        # Review correction: human-sized swords, guards resting on the upper rail.
        # Transform before UV packing to retain 256 texels per runtime cell.
        for pts,_,_ in h.faces[sword_start:]:
            for p in pts:
                p.x=sx+(p.x-sx)*.65
                p.y=.456+(p.y-.59)*.65
                p.z=.12+(p.z-.12)*.65
# One shield at the end: thick front/back and a painted iron rim, grounded human scale.
shield_start=len(h.faces)
outline=[(1.94,.53),(2.25,.53),(2.28,.36),(2.10,.18),(1.92,.36)]
for z,normal,shade in ((.07,(0,0,-1),.85),(.11,(0,0,1),.65)):
    h.face([(x,y,z) for x,y in outline],9,normal,shade)
for i in range(len(outline)):
    a,b=outline[i],outline[(i+1)%len(outline)]
    h.face([(a[0],a[1],.07),(b[0],b[1],.07),(b[0],b[1],.11),(a[0],a[1],.11)],7)
    # Narrow raised front border, inset toward the shield centre.
    def inset(p):return (2.10+(p[0]-2.10)*.87,.38+(p[1]-.38)*.87)
    ai,bi=inset(a),inset(b)
    h.face([(a[0],a[1],.065),(b[0],b[1],.065),(bi[0],bi[1],.065),(ai[0],ai[1],.065)],7,(0,0,-1))
box(h,(2.10,.38,.06),(.025,.22,.014),7)
for pts,_,_ in h.faces[shield_start:]:
    for p in pts:
        p.x=2.10+(p.x-2.10)*.75
        p.y=.38+(p.y-.38)*.75
        p.z=.085+(p.z-.085)*.75
# Shared atlas with metric 256 texels/cell UVs and bounded packing.
regions={};mapped={}
for name,h in models.items():
    mapped[name]=[]
    for pts,tile,shade in h.faces:
        n=(pts[1]-pts[0]).cross(pts[2]-pts[0]).normalized()
        axis=Vector((n.z,0,-n.x)) if abs(n.y)<1e-5 else (pts[1]-pts[0]).normalized()
        cross=Vector((0,-1,0)) if abs(n.y)<1e-5 else n.cross(axis)
        coords=[((p-pts[0]).dot(axis)*256,(p-pts[0]).dot(cross)*256) for p in pts]
        lo=[min(c[k] for c in coords) for k in (0,1)];hi=[max(c[k] for c in coords) for k in (0,1)]
        w,hh=[max(1,math.ceil(hi[k]-lo[k])) for k in (0,1)]
        key=(w,hh,tile,shade);regions[key]=None
        mapped[name].append((pts,key,[(u-lo[0],v-lo[1]) for u,v in coords]))
size=2048;positions=pack_regions([(k[0],k[1]) for k in regions],size,16)
if positions is None:
    size=4096;positions=pack_regions([(k[0],k[1]) for k in regions],size,16)
assert positions is not None,'Smithy atlas exhausted'
regions=dict(zip(regions,positions));atlas=np.ones((size,size,4),dtype=np.float32)
for (w,hh,tile,shade),(x,y) in regions.items():
    patch=sample_mirrored(tiles[tile],w,hh,256).copy();patch[:,:,:3]*=shade
    pad=padded_patch(patch,16);atlas[y-16:y-16+pad.shape[0],x-16:x-16+pad.shape[1]]=pad
save('smithy-atlas',atlas);del atlas,tiles
records={};bindings={}
def emit(out,pts,uv):
    n=(pts[1]-pts[0]).cross(pts[2]-pts[0]).normalized()
    for i in range(1,len(pts)-1):
        for k in (0,i,i+1):out.append([round(v,6) for v in (*pts[k],*n,*uv[k])])
for name in models:
    kinds={'opaque':[],'foliage':[]}
    for pts,key,coords in mapped[name]:
        x,y=regions[key];emit(kinds['opaque'],pts,[((x+u)/size,(y+v)/size) for u,v in coords])
    for pts,uv in special[name]:emit(kinds['foliage'],pts,uv)
    bindings[name]=[]
    for kind,vertices in kinds.items():
        if not vertices:continue
        for i,part in enumerate(split_triangles(vertices)):
            partname=f'{name}-{kind}-{i}';path=f'packs/sanctuary-smithy/models/{partname}.mesh.json'
            record=dict(name=partname,texture=f'packs/sanctuary-smithy/source/{"smithy-atlas"}.png',cutout=kind=='foliage',vertices=part)
            assert all(abs(v[0])<=4 and abs(v[2])<=4 and 0<=v[1]<=4 for v in part),name
            (args.output_root/path).write_text(json.dumps(record,separators=(',',':'))+'\n');records[path]=record;bindings[name].append(path)
base=json.loads((CITY/'pack.json').read_text());base['id']='sanctuary-smithy'
for name in ('wall','floor'):base[name]=f'packs/sanctuary-smithy/source/{name}.png'
base['ceiling']='packs/sanctuary-inn/source/ceiling.png'
base['daylightFog']=[12,4,0];base['viewDistance']=16
base['buildings']=[dict(id=name,parts=parts) for name,parts in bindings.items()]
(PACK/'pack.json').write_text(json.dumps(base,indent=2)+'\n')
export_collection(PACK,'sanctuary-smithy',[(record,[(i%4)*4,0,(i//4)*4]) for i,record in enumerate(records.values())])
bounds={name:dict(min=[min(v[k] for p in parts for v in records[p]['vertices']) for k in range(3)],
                  max=[max(v[k] for p in parts for v in records[p]['vertices']) for k in range(3)])
        for name,parts in bindings.items()}
# Bounds are a contract with the authored cell reservations, checked before promotion.
reservations = {'forge':(-.5,1.5,-.5,.5),'counter':(-1.5,1.5,-.5,.5),
                'shelves':(-1.5,1.5,-.5,.5),'equipment':(-.5,2.5,-.5,.5)}
for name in ('anvil','quench-tub','charcoal','iron-stock'):
    reservations[name]=(-.5,.5,-.5,.5)
for name,(x0,x1,z0,z1) in reservations.items():
    b=bounds[name]
    assert x0<=b['min'][0]<=b['max'][0]<=x1, name
    assert z0<=b['min'][2]<=b['max'][2]<=z1, name
assert bounds['counter']['max'][1]<.5
assert abs(bounds['anvil']['max'][1]-.27)<.001
args.report.write_text(json.dumps(dict(texels_per_cell=256,atlas_size=size,bounds=bounds,
    models={name:sum(len(records[p]['vertices']) for p in parts) for name,parts in bindings.items()}),indent=2)+'\n')
print('Exported',len(models),'smithy models',flush=True)
