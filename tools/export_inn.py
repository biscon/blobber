"""Rebuild authored inn assets in bounded background Blender; no runtime Blender dependency.
Reuse accepted city materials and foliage. Retain prepared metric UVs and 16px gutters.
"""
import json, math, sys
from pathlib import Path
import bpy
import numpy as np
from mathutils import Vector
sys.path.insert(0, str(Path(__file__).resolve().parent))
from city_architecture import House
from city_materials import extract_tiles, sample_mirrored, pack_regions, padded_patch, split_triangles
from city_mesh_export import export_collection
ROOT=Path(__file__).resolve().parent.parent
PACK=ROOT/'assets/packs/sanctuary-inn'
CITY=ROOT/'assets/packs/sanctuary-city'
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
tiles=extract_tiles(load(CITY/'source/material-master.png'))
tiles.append(tiles[0].copy()) # Retain shared city tile numbering: iron=7, clay=8.
iron=tiles[5].copy();iron[:,:,:3]=np.mean(iron[:,:,:3],axis=2)[:,:,None]*[.23,.24,.25];tiles.append(iron)
clay=tiles[1].copy();clay[:,:,:3]*=[.68,.38,.23];tiles.append(clay)
# Repeating floor has painted plank joints, not an enlarged material swatch.
wood=sample_mirrored(tiles[3],256,256,256).copy()
for x in (0,64,128,192): wood[:,x:x+2,:3]*=.42
for x,y in ((0,94),(64,192),(128,64),(192,142)): wood[y:y+2,x:x+64,:3]*=.48
save('floor',wood)
save('wall',sample_mirrored(tiles[1],256,256,256));save('ceiling',sample_mirrored(tiles[3],256,256,256)*[.72,.72,.72,1])
save('foliage',load(CITY/'source/foliage-master.png',(256,256)))
save('flame',load(PACK/'source/flame-master.png',(128,192)))
models={};special={}
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
def chair(h,x,z,facing=1):
    box(h,(x,.24,z),(.33,.055,.32))
    for dx in (-.12,.12):
        for dz in (-.115,.115): box(h,(x+dx,.115,z+dz),(.045,.23,.045),shade=.78)
        box(h,(x+dx,.43,z+facing*.13),(.05,.43,.05))
    for yy in (.39,.52,.62): box(h,(x,yy,z+facing*.13),(.29,.08,.045))
for name in ('wall','exit-door'):
    h=model(name)
    # Local front faces -Z; anchor lies in the solid boundary cell.
    box(h,(0,1,-.49),(1,2,.04),1)
    box(h,(0,.18,-.525),(1,.36,.04),0)
    for x in (-.46,.46): box(h,(x,1,-.55),(.08,2,.10),3,.80)
    for y in (.37,1.82,1.96): box(h,(0,y,-.55),(1,.07,.10),3,.85)
    if name=='exit-door':
        box(h,(0,.56,-.58),(.66,1.12,.07),3,.72)
        for x in (-.38,.38): box(h,(x,.60,-.61),(.09,1.20,.12),5)
        box(h,(0,1.18,-.61),(.85,.10,.12),5)
        for y in (.22,.68): box(h,(0,y,-.625),(.61,.035,.025),7)
        box(h,(.22,.48,-.65),(.035,.075,.03),7)
h=model('table-group')
# Table + four chairs fit precisely in a two-by-two physical reservation.
box(h,(.5,.42,.5),(1.08,.075,.60))
for x in (.04,.96):
    for z in (.28,.72):box(h,(x,.20,z),(.075,.4,.075),shade=.8)
box(h,(.5,.12,.5),(.9,.07,.07),shade=.65)
for x in (.17,.83):
    chair(h,x,-.08,-1);chair(h,x,1.08,1)
# Tiny pottery is real geometry and remains noninteractive.
cylinder(h,.35,.46,.5,.035,.065,8)
cylinder(h,.68,.46,.48,.042,.08,8)
h=model('bar')
box(h,(0,.27,0),(2.75,.54,.55),3,.76)
for x in np.linspace(-1.3,1.3,14):box(h,(float(x),.27,-.286),(.018,.5,.025),3,.5)
box(h,(0,.58,0),(2.95,.10,.73))
for x in (-1.38,1.38):box(h,(x,.275,-.30),(.10,.55,.10),shade=.9)
box(h,(0,.12,-.42),(2.8,.045,.045),7)
for x in (-.8,0,.8):cylinder(h,x,.635,-.03,.045,.09,8)
h=model('shelves')
box(h,(0,.73,-.35),(2.9,1.45,.06),3,.58)
for x in (-1.42,0,1.42):box(h,(x,.75,-.12),(.075,1.5,.55))
for y in (.08,.55,1.02,1.47):box(h,(0,y,-.12),(2.95,.07,.58))
for yy in (.12,.59,1.06):
    for x in (-1.1,-.75,-.37,.28,.68,1.1):
        cylinder(h,x,yy,-.12,.07,.16,8)
        cylinder(h,x,yy+.16,-.12,.038,.045,8)
h=model('barrels')
barrel(h,-.23,-.12,r=.20,height=.48);barrel(h,.22,.16,r=.19,height=.45)
barrel(h,-.15,-.1,.48,r=.16,height=.36)
h=model('hearth')
# Width two cells, front open toward -Z. Rotated to face east in the authored map.
box(h,(.5,.055,0),(1.8,.11,.88),5,.9)
box(h,(.5,.48,.30),(1.65,.9,.18),0,.38)
for x in (-.21,1.21):
    for y in (.20,.43,.66):box(h,(x,y,0),(.30,.22,.70),5,.92)
box(h,(.5,.85,0),(1.78,.20,.76),5)
box(h,(.5,.98,0),(1.92,.09,.85),3)
box(h,(.5,1.45,.10),(1.30,.90,.52),0,.80)
# Baked warm stone at the back of the hearth and charred logs.
for x in (.20,.53,.86):
    box(h,(x,.15,-.12),(.14,.10,.38),3,.32)
for x in (.04,.34,.65,.96):box(h,(x,.21,-.34),(.025,.23,.025),7)
box(h,(.5,.18,-.35),(1.04,.025,.025),7)
h=model('stairs')
for i in range(12):
    z=2.38-i*.235;high=.12+i*.125
    box(h,(0,high/2,z),(.80,high,.235),3,.80)
    box(h,(0,high,z+.015),(.86,.035,.25))
    if i%3==0:
        for x in (-.40,.40):box(h,(x,high+.24,z),(.055,.48,.055))
# Sloped rails are small oriented prisms.
for x in (-.4,.4):
    a,b=Vector((x,.52,2.45)),Vector((x,1.92,-.25));u=Vector((.03,0,0));v=Vector((0,.035,.018))
    for pts in ([a-u-v,a+u-v,b+u-v,b-u-v],[a-u+v,b-u+v,b+u+v,a+u+v],
                [a-u-v,b-u-v,b-u+v,a-u+v],[a+u-v,a+u+v,b+u+v,b+u-v]):h.face(pts,3)
box(h,(0,1.63,-.30),(.86,.08,.4))
box(h,(0,1.83,-.46),(.75,.32,.06),3,.6)
h=model('plant')
cylinder(h,0,0,0,.17,.30,8)
cylinder(h,0,.28,0,.19,.045,8)
cylinder(h,0,.31,0,.16,.015,0)
for i in range(4):
    a=math.pi*i/4;u=Vector((math.cos(a)*.30,0,math.sin(a)*.30));c=Vector((0,.51,0));v=Vector((0,.30,0))
    pts=[c-u-v,c+u-v,c+u+v,c-u+v];uv=[(0,1),(1,1),(1,0),(0,0)]
    special['plant'].append((pts,uv));special['plant'].append((list(reversed(pts)),list(reversed(uv))))
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
assert positions is not None,'Inn atlas exhausted'
regions=dict(zip(regions,positions));atlas=np.ones((size,size,4),dtype=np.float32)
for (w,hh,tile,shade),(x,y) in regions.items():
    patch=sample_mirrored(tiles[tile],w,hh,256).copy();patch[:,:,:3]*=shade
    pad=padded_patch(patch,16);atlas[y-16:y-16+pad.shape[0],x-16:x-16+pad.shape[1]]=pad
save('inn-atlas',atlas);del atlas,tiles
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
            partname=f'{name}-{kind}-{i}';path=f'packs/sanctuary-inn/models/{partname}.mesh.json'
            record=dict(name=partname,texture=f'packs/sanctuary-inn/source/{"inn-atlas" if kind=="opaque" else "foliage"}.png',cutout=kind=='foliage',vertices=part)
            assert all(abs(v[0])<=4 and abs(v[2])<=4 and 0<=v[1]<=4 for v in part),name
            (ROOT/'assets'/path).write_text(json.dumps(record,separators=(',',':'))+'\n');records[path]=record;bindings[name].append(path)
base=json.loads((CITY/'pack.json').read_text());base['id']='sanctuary-inn'
for name in ('wall','floor','ceiling'):base[name]=f'packs/sanctuary-inn/source/{name}.png'
base['daylightFog']=[12,4,0];base['viewDistance']=16
base['buildings']=[dict(id=name,parts=parts) for name,parts in bindings.items()]
(PACK/'pack.json').write_text(json.dumps(base,indent=2)+'\n')
export_collection(PACK,'sanctuary-inn',[(record,[(i%4)*4,0,(i//4)*4]) for i,record in enumerate(records.values())])
(PACK/'models/export-report.json').write_text(json.dumps(dict(texels_per_cell=256,atlas_size=size,models={name:sum(len(records[p]['vertices']) for p in parts) for name,parts in bindings.items()}),indent=2)+'\n')
print('Exported',len(models),'inn models',flush=True)
