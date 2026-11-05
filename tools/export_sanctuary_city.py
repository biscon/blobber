"""Rebuild SC01.2 painted architecture in background Blender, bounded to two threads.
All coordinates below are runtime cells; Blender converts (x,y,z) to (x,-z,y).
Only this pack's outputs are written. Original generated master is never replaced.
"""
import bpy, json, math, struct, sys
import numpy as np
from pathlib import Path
from mathutils import Vector
sys.path.insert(0, str(Path(__file__).resolve().parent))
from city_materials import extract_tiles, sample_mirrored, padded_extent, padded_patch, pack_regions
ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / 'assets/packs/sanctuary-city'
DENSITY, GUTTER = 256, 16
scene = bpy.data.scenes.new('SanctuaryCityAssets')
scene.render.threads_mode = 'FIXED'; scene.render.threads = 2
master = bpy.data.images.load(str(OUT/'source/material-master.png'), check_existing=True)
w,h = master.size
pixels = np.empty(w*h*4, dtype=np.float32); master.pixels.foreach_get(pixels)
master_pixels = pixels.reshape(h,w,4)[::-1].copy()
# Top-down retained original; inset crops exclude generated panel dividers.
tiles = extract_tiles(master_pixels)
period = [1,2,1,1,1,.7,1]
# Iron derives its restrained granular finish from the stone master, recolored in the authoring bake.
iron = tiles[5].copy(); iron[:,:,:3] = np.mean(iron[:,:,:3],axis=2)[:,:,None]*np.array([.23,.24,.25])
tiles.append(iron)

def save_image(name, arr):
    hh,ww = arr.shape[:2]
    image=bpy.data.images.new(name,width=ww,height=hh)
    image.pixels.foreach_set(np.ascontiguousarray(arr[::-1]).ravel())
    image.filepath_raw=str(OUT/'source'/f'{name}.png'); image.file_format='PNG'; image.save(); image.pack()
    return image

def material_patch(tile, ww, hh, shade, offset=(0,0)):
    patch=sample_mirrored(tiles[tile],ww,hh,DENSITY*period[tile],offset)
    patch[:,:,:3]*=shade
    # Painted joint/recess shading has a fixed physical width, not a UV-sized blur.
    edge=np.minimum(np.minimum(np.arange(ww),ww-1-np.arange(ww))[None,:],
                    np.minimum(np.arange(hh),hh-1-np.arange(hh))[:,None])
    if tile not in (0,1,2,3,4):
        patch[:,:,:3]*=(.78+.22*np.minimum(edge/3,1))[:,:,None]
    return patch

# Native terrain repeats at one cell and keeps the normal pack baker path.
for name,tile in [('wall',0),('paving',4),('ceiling',1)]:
    save_image(name,sample_mirrored(tiles[tile],256,256,256))

class House:
    def __init__(self,name,front,back,eaves,ridge):
        self.name,self.front,self.back,self.eaves,self.ridge=name,front,back,eaves,ridge
        self.faces=[]
    def face(self,pts,tile=1,normal=None,shade=1):
        pts=[Vector(p) for p in pts]
        # Remove collapsed arch-cap corners before Blender triangulates the polygon.
        changed=True
        while changed and len(pts)>=3:
            changed=False
            for i in range(len(pts)):
                a,b,c=pts[i-1],pts[i],pts[(i+1)%len(pts)]
                if (b-a).cross(c-b).length<1e-9:
                    pts.pop(i);changed=True;break
        if len(pts)<3:return
        n=(pts[1]-pts[0]).cross(pts[2]-pts[0])
        if n.length<1e-8: return
        if normal and n.dot(Vector(normal))<0: pts.reverse(); n=-n
        n.normalize()
        if n.y<-.5: shade*=.60
        self.faces.append((pts,tile,round(shade,2)))
    def box(self,x0,x1,y0,y1,z0,z1,tile,shade=1):
        for pts,n in [([(x0,y0,z0),(x1,y0,z0),(x1,y1,z0),(x0,y1,z0)],(0,0,-1)),
                      ([(x0,y0,z1),(x1,y0,z1),(x1,y1,z1),(x0,y1,z1)],(0,0,1)),
                      ([(x0,y0,z0),(x0,y0,z1),(x0,y1,z1),(x0,y1,z0)],(-1,0,0)),
                      ([(x1,y0,z0),(x1,y0,z1),(x1,y1,z1),(x1,y1,z0)],(1,0,0)),
                      ([(x0,y1,z0),(x1,y1,z0),(x1,y1,z1),(x0,y1,z1)],(0,1,0)),
                      ([(x0,y0,z0),(x1,y0,z0),(x1,y0,z1),(x0,y0,z1)],(0,-1,0))]: self.face(pts,tile,n,shade)
    def facade(self,point,length,door=False,upper=False):
        # point(u,height,inset) maps a wall's local coordinates into the complete shell.
        normal=(Vector(point(0,0,0))-Vector(point(0,0,1))).normalized()
        def panel(x0,x1,y0,y1,tile=1,inset=0,shade=1):
            self.face([point(x0,y0,inset),point(x1,y0,inset),point(x1,y1,inset),point(x0,y1,inset)],tile,normal,shade)
        windows=[(-length*.32,.49,.44,.43),(length*.32,.49,.44,.43)]
        if not door: windows=[(-length*.26,.52,.46,.47),(length*.26,.52,.46,.47)]
        if upper: windows += [(-length*.28,1.43,.48,.52),(length*.28,1.43,.48,.52)]
        holes=[(x-ww/2,x+ww/2,y,y+hh) for x,y,ww,hh in windows]
        if door: holes.append((-.5,.5,0,1.12))
        xs=sorted(set([-length/2,length/2]+[v for r in holes for v in r[:2]]))
        ys=sorted(set([0,.23,self.eaves]+[v for r in holes for v in r[2:]]+([1.12] if upper else [])))
        for x0,x1 in zip(xs,xs[1:]):
            for y0,y1 in zip(ys,ys[1:]):
                mx,my=(x0+x1)/2,(y0+y1)/2
                if any(a<mx<b and c<my<d for a,b,c,d in holes):continue
                panel(x0,x1,y0,y1,0 if my<.23 or (upper and my<1.12) else 1)
        for x,y,ww,hh in windows:
            x0,x1=x-ww/2,x+ww/2; top=y+hh; depth=.085
            panel(x0,x1,y,top,3,depth,.66)
            for a,b in [((x0,y),(x0,top)),((x1,top),(x1,y)),((x0,top),(x1,top)),((x1,y),(x0,y))]:
                self.face([point(*a,0),point(*b,0),point(*b,depth),point(*a,depth)],5,shade=.69)
            # Raised stone lintel and sill; front/side faces have real thickness.
            for ya,yb in [(y-.045,y),(top,top+.055)]:
                panel(x0-.035,x1+.035,ya,yb,5,-.025)
                self.face([point(x0-.035,ya,-.025),point(x1+.035,ya,-.025),point(x1+.035,ya,0),point(x0-.035,ya,0)],5,shade=.7)
                self.face([point(x0-.035,yb,-.025),point(x1+.035,yb,-.025),point(x1+.035,yb,0),point(x0-.035,yb,0)],5)
            # Two shutter stiles and crossbars sit inside the reveal.
            for xa,xb in [(x0+.015,x0+.035),(x-.012,x+.012),(x1-.035,x1-.015)]: panel(xa,xb,y+.01,top-.01,3,depth-.008,.8)
            for yy in (y+.09,top-.09): panel(x0+.02,x1-.02,yy,yy+.025,6,depth-.012,.9)
        if not door:return
        # Rectangular opening above the outer arch closes with radial wedges.
        steps=12; spring=.62; ri=.36; ro=.50
        for k in range(steps):
            a=math.pi*k/steps; b=math.pi*(k+1)/steps
            xa,ya=ro*math.cos(a),spring+ro*math.sin(a)
            xb,yb=ro*math.cos(b),spring+ro*math.sin(b)
            self.face([point(xa,ya,0),point(xb,yb,0),point(xb,1.12,0),point(xa,1.12,0)],1,normal)
            # Separate wedge stones over a continuous dark mortar backing.
            for gap,inset,shade in [(0,.006,.48),(.006,-.035,1)]:
                aa,bb=a+gap,b-gap
                points=[(ri*math.cos(aa),spring+ri*math.sin(aa)),(ro*math.cos(aa),spring+ro*math.sin(aa)),
                        (ro*math.cos(bb),spring+ro*math.sin(bb)),(ri*math.cos(bb),spring+ri*math.sin(bb))]
                self.face([point(x,y,inset) for x,y in points],5,normal,shade)
            # Seal the outer lip back to the facade; side approaches cannot see behind the arch.
            self.face([point(xa,ya,-.035),point(xb,yb,-.035),point(xb,yb,0),point(xa,ya,0)],5,
                      Vector(point(math.cos((a+b)/2),spring+math.sin((a+b)/2),0))-Vector(point(0,spring,0)),.9)
            ia=(ri*math.cos(a),spring+ri*math.sin(a)); ib=(ri*math.cos(b),spring+ri*math.sin(b))
            self.face([point(*ia,-.035),point(*ib,-.035),point(*ib,.23),point(*ia,.23)],5,Vector(point(0,spring,0))-Vector(point(math.cos((a+b)/2),spring+math.sin((a+b)/2),0)),.57)
        for side in (-1,1):
            panel(min(side*ri,side*ro),max(side*ri,side*ro),0,spring,5,.006,.48)
            for j in range(4):
                panel(min(side*ri,side*ro),max(side*ri,side*ro),j*spring/4+.003,(j+1)*spring/4-.003,5,-.035,1-.025*(j%2))
            self.face([point(side*ri,0,-.035),point(side*ri,spring,-.035),point(side*ri,spring,.23),point(side*ri,0,.23)],5,Vector(point(0,0,0))-Vector(point(side,0,0)),.64)
            self.face([point(side*ro,0,-.035),point(side*ro,spring,-.035),point(side*ro,spring,0),point(side*ro,0,0)],5,
                      Vector(point(side,0,0))-Vector(point(0,0,0)),.9)
        # Closed arched timber leaf behind the actual tunnel, with iron straps.
        outline=[(-ri,0),(ri,0),(ri,spring)]+[(ri*math.cos(math.pi*k/steps),spring+ri*math.sin(math.pi*k/steps)) for k in range(1,steps+1)]
        self.face([point(x,y,.232) for x,y in outline],3,normal,.76)
        for y in (.17,.52):
            panel(-.335,.335,y,y+.04,6,.22)
            for x in (-.29,.29): panel(x-.012,x+.012,y+.01,y+.03,5,.216,.6)
        # Ring handle: a small octagonal solid torus, not flat icon artwork.
        center=(.19,.42)
        for k in range(12):
            a,b=math.tau*k/12,math.tau*(k+1)/12
            for j in range(4):
                c,d=math.tau*j/4,math.tau*(j+1)/4
                def ring(t,q):return point(center[0]+(.035+.007*math.cos(q))*math.cos(t),center[1]+(.035+.007*math.cos(q))*math.sin(t),.20+.007*math.sin(q))
                self.face([ring(a,c),ring(b,c),ring(b,d),ring(a,d)],6)
        self.face([point(-ro,0,0),point(ro,0,0),point(ro,0,.235),point(-ro,0,.235)],5,(0,1,0),.78)
    def build(self):
        f,b,e,r=self.front,self.back,self.eaves,self.ridge
        self.facade(lambda u,y,d:(u,y,f+d),3,True,e>1.5)
        self.facade(lambda u,y,d:(-u,y,b-d),3,False,e>1.5)
        center=(f+b)/2;length=b-f
        self.facade(lambda u,y,d:(-1.5+d,y,center-u),length,False,e>1.5)
        self.facade(lambda u,y,d:(1.5-d,y,center+u),length,False,e>1.5)
        # Gables close both ends, roof plane sits outside them with a sealed underside.
        for z,n in [(f,(0,0,-1)),(b,(0,0,1))]:
            self.face([(-1.5,e,z),(1.5,e,z),(0,r-.05,z)],1,n)
        # Thin roof prism, restrained overhangs and visible fascia.
        a,c=f-.16,b+.16
        for side in (-1,1):
            self.face([(0,r,a),(side*1.68,e+.01,a),(side*1.68,e+.01,c),(0,r,c)],2,(side,1,0))
            self.face([(0,r-.05,a),(side*1.68,e-.04,a),(side*1.68,e-.04,c),(0,r-.05,c)],3,(0,-1,0),.76)
            for z,n in [(a,(0,0,-1)),(c,(0,0,1))]:
                self.face([(0,r,z),(side*1.68,e+.01,z),(side*1.68,e-.04,z),(0,r-.05,z)],3,n,.85)
            self.face([(side*1.68,e-.04,a),(side*1.68,e+.01,a),(side*1.68,e+.01,c),(side*1.68,e-.04,c)],3,(side,0,0),.82)
        # Timber sill bands and small jointed eave brackets clear the walking path.
        if e>1.5:
            self.box(-1.51,1.51,1.09,1.15,f-.015,f+.015,3,.8)
            self.box(-1.51,1.51,1.09,1.15,b-.015,b+.015,3,.8)
            for x in (-1.5,1.5):self.box(x-.015,x+.015,1.09,1.15,f,b,3,.8)
        for x in (-1.51,1.51):
            for z in (f+.25,center,b-.25):
                self.box(x-.04,x+.04,e-.15,e-.04,z-.045,z+.045,3,.78)
        # Chimney emerges through the roof; hidden intersection stays below the opaque slate.
        x0,x1=.65,.94; z0,z1=b-.8,b-.48
        bottom=r-(r-e)*.65/1.68-.08; top=r+.26
        self.box(x0,x1,bottom,top,z0,z1,0,.9)
        self.box(x0-.035,x1+.035,top,top+.055,z0-.035,z1+.035,5)
        self.box(x0+.045,x1-.045,top+.055,top+.056,z0+.045,z1-.045,6,.45)
        return self

houses=[House('low-house',-1.5,1.5,1.23,1.85).build(),House('tall-house',-2.5,1.5,2.15,2.85).build()]
report=[]
gltf_models=[]
groups={}; raw_models={}
for house in houses:
    # Share continuous physical UV fields across facade fragments. Door grain stays
    # vertical even when the first polygon edge belongs to the curved arch.
    raw=[]
    for pts,tile,shade in house.faces:
        normal=(pts[1]-pts[0]).cross(pts[2]-pts[0]).normalized()
        if abs(normal.y)<1e-5:
            axis=Vector((normal.z,0,-normal.x));cross=Vector((0,-1,0));mode='vertical'
        elif tile==2:
            axis=Vector((0,0,1));cross=normal.cross(axis);mode='roof'
        else:
            axis=(pts[1]-pts[0]).normalized();cross=normal.cross(axis);mode='other'
        origin=Vector((0,0,(house.front+house.back)/2))
        coords=[((p-origin).dot(axis)*DENSITY,(p-origin).dot(cross)*DENSITY) for p in pts]
        group=(tile,shade,mode)
        groups.setdefault(group,[]).extend(coords);raw.append((group,coords))
    raw_models[house.name]=raw
entries={}; offsets={}
for group,coords in groups.items():
    lo=[math.floor(min(p[k] for p in coords)) for k in (0,1)]
    hi=[math.ceil(max(p[k] for p in coords)) for k in (0,1)]
    ww,hh=[max(1,hi[k]-lo[k]) for k in (0,1)]
    key=(ww,hh,group[0],group[1],group[2]);entries[key]=None;offsets[key]=lo
    groups[group]=(key,lo)
print('Atlas used texels:',sum(padded_extent(k[0],GUTTER)*padded_extent(k[1],GUTTER) for k in entries), 'regions:',len(entries),flush=True)
size=2048
while True:
    positions=pack_regions([(k[0],k[1]) for k in entries],size,GUTTER)
    if positions is not None:
        entries=dict(zip(entries,positions));break
    size*=2
    assert size<=4096,'Atlas capacity exceeded; split material parts without shrinking density'
atlas=np.zeros((size,size,4),dtype=np.float32);atlas[:,:,3]=1
for key,(x,y) in entries.items():
    ww,hh,tile,shade,_=key
    patch=material_patch(tile,ww,hh,shade,offsets[key])
    padded=padded_patch(patch,GUTTER)
    atlas[y-GUTTER:y-GUTTER+padded.shape[0],x-GUTTER:x-GUTTER+padded.shape[1]]=padded
img=save_image('architecture-atlas',atlas)
for house in houses:
    mappings=[]
    for group,coords in raw_models[house.name]:
        key,lo=groups[group];mappings.append((key,[(x-lo[0],y-lo[1]) for x,y in coords]))
    verts=[];faces=[]
    for pts,_,_ in house.faces:
        start=len(verts);verts.extend((p.x,-p.z,p.y) for p in pts);faces.append(tuple(range(start,len(verts))))
    mesh=bpy.data.meshes.new(house.name);mesh.from_pydata(verts,[],faces);mesh.update()
    obj=bpy.data.objects.new(house.name,mesh);scene.collection.objects.link(obj)
    mat=bpy.data.materials.new(house.name+'-painted');mat.use_nodes=True
    nodes=mat.node_tree.nodes;nodes.clear();tex=nodes.new('ShaderNodeTexImage');tex.image=img
    shader=nodes.new('ShaderNodeEmission');output=nodes.new('ShaderNodeOutputMaterial')
    mat.node_tree.links.new(tex.outputs['Color'],shader.inputs['Color']);mat.node_tree.links.new(shader.outputs[0],output.inputs['Surface']);mesh.materials.append(mat)
    uv=mesh.uv_layers.new(name='MetricPaintedUV')
    for poly,(key,coords) in zip(mesh.polygons,mappings):
        x,y=entries[key]
        for li,(u,v) in zip(poly.loop_indices,coords):uv.data[li].uv=((x+u)/size,1-(y+v)/size)
    mesh.calc_loop_triangles();exported=[]
    for tri in mesh.loop_triangles:
        points=[mesh.vertices[mesh.loops[i].vertex_index].co for i in tri.loops];coords=[uv.data[i].uv for i in tri.loops]
        area=(points[1]-points[0]).cross(points[2]-points[0]).length
        a,b=coords[1]-coords[0],coords[2]-coords[0];uvarea=abs(a.x*b.y-a.y*b.x)
        assert area>1e-9 and abs(size*math.sqrt(uvarea/area)-DENSITY)<.5,(house.name,'UV density',area)
        for li in tri.loops:
            p=mesh.vertices[mesh.loops[li].vertex_index].co;n=tri.normal;t=uv.data[li].uv
            exported.append([round(v,6) for v in (p.x,p.z,-p.y,n.x,n.z,-n.y,t.x,1-t.y)])
    assert len(exported)<=6000,(house.name,len(exported))
    data={'name':house.name,'texture':'packs/sanctuary-city/source/architecture-atlas.png','vertices':exported}
    (OUT/'models'/f'{house.name}.mesh.json').write_text(json.dumps(data,separators=(',',':'))+'\n')
    gltf_models.append((house.name, exported))
    obj.location.x=houses.index(house)*4
    report.append({'model':house.name,'triangles':len(exported)//3,'atlas':size,'texels_per_cell':DENSITY,'gutter':GUTTER,'panel_inset':min(w//3,h//2)//64})
# Keep the source scene independent of Blender's optional GLTF optimizer plugins.
bpy.context.preferences.filepaths.save_version = 0
bpy.context.window.scene = scene
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'models/sanctuary-city.blend'), compress=False)
binary=bytearray(); views=[]; accessors=[]; meshes=[]; nodes=[]; images=[]; materials=[]
def append(data,target=None):
    while len(binary)%4:binary.append(0)
    view={'buffer':0,'byteOffset':len(binary),'byteLength':len(data)}
    if target:view['target']=target
    views.append(view);binary.extend(data);return len(views)-1
for index,(name,vertices) in enumerate(gltf_models):
    attributes={}
    for semantic,columns,kind in [('POSITION',(0,1,2),'VEC3'),('NORMAL',(3,4,5),'VEC3'),('TEXCOORD_0',(6,7),'VEC2')]:
        values=[[v[c] for c in columns] for v in vertices]
        data=struct.pack('<'+'f'*(len(values)*len(columns)),*(x for v in values for x in v))
        accessor={'bufferView':append(data,34962),'componentType':5126,'count':len(values),'type':kind}
        if semantic=='POSITION':
            accessor['min']=[min(v[c] for v in values) for c in range(3)]
            accessor['max']=[max(v[c] for v in values) for c in range(3)]
        attributes[semantic]=len(accessors);accessors.append(accessor)
    if index==0:
        image_view=append((OUT/'source/architecture-atlas.png').read_bytes())
        images.append({'bufferView':image_view,'mimeType':'image/png'})
        materials.append({'name':'architecture-painted','pbrMetallicRoughness':{'baseColorTexture':{'index':0},'metallicFactor':0,'roughnessFactor':1},'extensions':{'KHR_materials_unlit':{}}})
    meshes.append({'name':name,'primitives':[{'attributes':attributes,'material':0,'mode':4}]})
    nodes.append({'mesh':index,'name':name,'translation':[index*4,0,0]})
while len(binary)%4:binary.append(0)
gltf={'asset':{'version':'2.0','generator':'Blobber deterministic sanctuary exporter'},'scene':0,'scenes':[{'nodes':list(range(len(nodes)))}],
      'nodes':nodes,'meshes':meshes,'materials':materials,'textures':[{'source':i} for i in range(len(images))],
      'images':images,'buffers':[{'byteLength':len(binary)}],'bufferViews':views,'accessors':accessors,'extensionsUsed':['KHR_materials_unlit']}
metadata=json.dumps(gltf,separators=(',',':')).encode()
while len(metadata)%4:metadata+=b' '
glb=struct.pack('<III',0x46546c67,2,12+8+len(metadata)+8+len(binary))+struct.pack('<II',len(metadata),0x4e4f534a)+metadata+struct.pack('<II',len(binary),0x004e4942)+binary
(OUT/'models/sanctuary-city.glb').write_bytes(glb)
pack=json.loads((ROOT/'assets/packs/dungeon/pack.json').read_text())
pack.update(id='sanctuary-city',wall='packs/sanctuary-city/source/wall.png',floor='packs/sanctuary-city/source/paving.png',ceiling='packs/sanctuary-city/source/ceiling.png',stairWall='packs/sanctuary-city/source/wall.png',stairFloor='packs/sanctuary-city/source/paving.png')
pack['buildings']=[{'id':h.name,'parts':[f'packs/sanctuary-city/models/{h.name}.mesh.json']} for h in houses]
(OUT/'pack.json').write_text(json.dumps(pack,indent=2)+'\n')
(OUT/'models/export-report.json').write_text(json.dumps(report,indent=2)+'\n')
print('Sanctuary exports:',report,flush=True)
