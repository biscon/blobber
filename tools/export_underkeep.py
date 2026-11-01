"""Run in Blender (MCP or --background --python); rebuild Underkeep source exports.
Only creates/replaces the dedicated Underkeep scene/data, never clears other scenes.
"""
import bpy, json, math
from pathlib import Path
from mathutils import Matrix, Vector
ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'assets/packs/underkeep'
scene=bpy.data.scenes.get('UnderkeepAssets')
if scene:
    for obj in list(scene.objects): bpy.data.objects.remove(obj,do_unlink=True)
else: scene=bpy.data.scenes.new('UnderkeepAssets')
scene.render.threads_mode='FIXED';scene.render.threads=2
master=bpy.data.images.load(str(OUT/'source/material-master.png'),check_existing=True)
master.scale(1024,1024)
# Terrain is 256 texels per cell at worldScale=1. Chests are half-size.
RUNTIME_DENSITY=256
ATLAS_SIZE={"chest":512,"cage":1024,"rack":1024}
DETAIL_SCALE=RUNTIME_DENSITY/80
models={}
master_pixels=list(master.pixels)
def model(name):
    verts=[];faces=[];tiles=[]
    def box(center,size,tile=0,transform=None):
        x,y,z=center;w,d,h=[v/2 for v in size]
        pts=[(x-w,y-d,z-h),(x+w,y-d,z-h),(x+w,y+d,z-h),(x-w,y+d,z-h),
             (x-w,y-d,z+h),(x+w,y-d,z+h),(x+w,y+d,z+h),(x-w,y+d,z+h)]
        if transform: pts=[tuple(transform@Vector(p)) for p in pts]
        start=len(verts);verts.extend(pts)
        for i,indices in enumerate([(3,2,1,0),(0,1,5,4),(1,2,6,5),(2,3,7,6),(3,0,4,7),(4,5,6,7)]):
            faces.append(tuple(start+j for j in indices))
            tiles.append(3 if tile==3 else (2 if i==0 else 1 if i==5 else tile))
    models[name]=(verts,faces,tiles,box)
    return box
for opened in (False,True):
    name='chest-open' if opened else 'chest-closed';box=model(name)
    box((0,0,.035),(.76,.58,.07),2)
    for x in (-.35,.35): box((x,0,.185),(.06,.58,.23))
    for y in (-.26,.26):box((0,y,.185),(.64,.06,.23))
    # Interior floor sits above the bottom, with prepainted darker wood.
    box((0,0,.075),(.64,.46,.025),2)
    for x in (-.26,.26):
        for y in (-.295,.295):box((x,y,.17),(.042,.014,.27),3)
    hinge=Matrix.Translation((0,.26,.30))@Matrix.Rotation(math.radians(-105 if opened else 0),4,'X')@Matrix.Translation((0,-.26,-.30))
    box((0,0,.328),(.78,.60,.056),0,hinge)
    for x in (-.26,.26):box((x,0,.366),(.045,.61,.020),3,hinge)
    box((0,-.302,.265),(.075,.02,.10),3)
    for x in (-.26,.26): box((x,.29,.29),(.06,.035,.035),3)
box=model('cage')
box((0,0,.035),(.72,.72,.07),2)
for x in (-.32,.32):
    for y in (-.32,.32):
        box((x,y,.43),(.075,.075,.84))
        for z in (.10,.74):box((x,y,z),(.082,.082,.04),3)
for z in (.12,.77):
    for y in (-.32,.32):box((0,y,z),(.565,.06,.065))
    for x in (-.32,.32):box((x,0,z),(.06,.565,.065))
for k in range(1,7):
    p=-.32+k*.64/7
    for y in (-.32,.32):box((p,y,.445),(.018,.022,.61),3)
    for x in (-.32,.32):box((x,p,.445),(.022,.018,.61),3)
    box((p,0,.80),(.018,.565,.018),3)
# Small barred door border and latch; gaps are real geometry.
for x in (-.17,.17):box((x,-.337,.43),(.024,.02,.53),3)
for z in (.18,.68):box((0,-.337,z),(.34,.025,.024),3)
box((.15,-.359,.44),(.10,.025,.035),3)
box=model('rack')
box((0,0,.29),(.68,1.7,.10))
for x in (-.26,.26):
    for y in (-.68,.68):box((x,y,.14),(.09,.10,.28))
    box((x,0,.10),(.05,1.45,.06),2)
# Octagonal rollers at both ends; a cuboid cradle and crank.
for y in (-.70,.70):
    # One watertight octagonal roller: no overlapping end caps.
    verts,faces,tiles,_=models['rack'];start=len(verts)
    for x in (-.37,.37):
        for k in range(8):
            a=math.tau*k/8
            verts.append((x,y+.065*math.cos(a),.405+.065*math.sin(a)))
    faces.extend([tuple(start+k for k in reversed(range(8))),tuple(start+8+k for k in range(8))]);tiles.extend([0,0])
    for k in range(8):
        j=(k+1)%8
        faces.append((start+k,start+j,start+8+j,start+8+k));tiles.append(0 if k<4 else 2)
    for x in (-.29,.29):box((x,y,.31),(.08,.09,.17),3)
    box((.40,y,.36),(.11,.035,.035),3)
    box((.45,y,.32),(.035,.035,.12),3)
for y in (-.35,.35):box((0,y,.351),(.55,.045,.022),3)
# Pack metric face rectangles, including a replicated gutter. Material detail is
# sampled from the high-resolution master, never stretched across a whole face.
prepared={};families={}
for name,(verts,faces,tiles,_) in models.items():
    mesh=bpy.data.meshes.new('Underkeep-'+name);mesh.from_pydata(verts,[],faces);mesh.update()
    atlas='chest' if name.startswith('chest') else name
    density=RUNTIME_DENSITY*(.5 if atlas=='chest' else 1)
    entries=families.setdefault(atlas,{})
    mappings=[]
    for poly,tile in zip(mesh.polygons,tiles):
        points=[mesh.vertices[i].co for i in poly.vertices]
        axis=(points[1]-points[0]).normalized();cross=poly.normal.cross(axis)
        coords=[((p-points[0]).dot(axis),(p-points[0]).dot(cross)) for p in points]
        lo=[min(c[i] for c in coords) for i in (0,1)];hi=[max(c[i] for c in coords) for i in (0,1)]
        w,h=[max(1,math.ceil((hi[i]-lo[i])*density)) for i in (0,1)]
        swap=h>w
        if swap:w,h=h,w
        key=(w,h,tile)
        entries[key]=None
        normalized=[((c[0]-lo[0])*density/(h if swap else w),
                     (c[1]-lo[1])*density/(w if swap else h)) for c in coords]
        mappings.append((key,[(v,u) if swap else (u,v) for u,v in normalized]))
    prepared[name]=(mesh,mappings,atlas)
images={}
for atlas,entries in families.items():
    size=ATLAS_SIZE[atlas]
    pixels=[0.0,0.0,0.0,1.0]*(size*size);x=y=row=0
    for w,h,tile in sorted(entries,key=lambda k:(-k[1],-k[0],k[2])):
        if x+w+8>size:x=0;y+=row;row=0
        assert y+h+8<=size,(atlas,'atlas full',y,h)
        entries[(w,h,tile)]=(x+4,y+4)
        for j in range(-4,h+4):
            for i in range(-4,w+4):
                u=max(0,min(w-1,i));v=max(0,min(h-1,j))
                # Bounded source field, away from the source master's broad seams.
                # Preserve the accepted physical grain scale, sampling real master
                # detail instead of enlarging the former 80-texel atlas pixels.
                sx=(tile%2)*512+40+int((u/DETAIL_SCALE%106)*4)
                sy=(1-tile//2)*512+40+int((v/DETAIL_SCALE%106)*4)
                src=(sy*1024+sx)*4;dst=((y+4+j)*size+x+4+i)*4
                edge=min(u,v,w-1-u,h-1-v)/DETAIL_SCALE
                shade=(.66 if edge<1 else .86 if edge<2 else 1.0)
                if tile==2:shade*=.68
                # Desaturate and quiet the original high-frequency grain.
                rgb=master_pixels[src:src+3];mean=sum(rgb)/3
                for c in range(3):pixels[dst+c]=(rgb[c]*.72+mean*.28)*shade
        x+=w+8;row=max(row,h+8)
    image=bpy.data.images.new('Underkeep-'+atlas+'-atlas',width=size,height=size)
    image.pixels=pixels;image.filepath_raw=str(OUT/'source'/f'{atlas}-atlas.png');image.file_format='PNG';image.save();image.pack();images[atlas]=image
for name,(mesh,mappings,atlas) in prepared.items():
    obj=bpy.data.objects.new('Underkeep-'+name,mesh);scene.collection.objects.link(obj)
    mat=bpy.data.materials.new('Underkeep-'+name);mat.use_nodes=True
    nodes=mat.node_tree.nodes;nodes.clear();tex=nodes.new('ShaderNodeTexImage');tex.image=images[atlas];tex.interpolation='Closest'
    shader=nodes.new('ShaderNodeEmission');output=nodes.new('ShaderNodeOutputMaterial')
    mat.node_tree.links.new(tex.outputs['Color'],shader.inputs['Color']);mat.node_tree.links.new(shader.outputs[0],output.inputs['Surface'])
    mesh.materials.append(mat);uv=mesh.uv_layers.new(name='PaintedUV')
    for poly,(key,coords) in zip(mesh.polygons,mappings):
        x,y=families[atlas][key];w,h,_=key
        for li,(u,v) in zip(poly.loop_indices,coords):uv.data[li].uv=((x+u*w)/ATLAS_SIZE[atlas],(y+v*h)/ATLAS_SIZE[atlas])
    mesh.calc_loop_triangles()
    exported=[]
    density=RUNTIME_DENSITY*(.5 if atlas=='chest' else 1)
    for tri in mesh.loop_triangles:
        points=[mesh.vertices[mesh.loops[i].vertex_index].co for i in tri.loops]
        coords=[uv.data[i].uv for i in tri.loops]
        area=(points[1]-points[0]).cross(points[2]-points[0]).length
        a,b=coords[1]-coords[0],coords[2]-coords[0]
        uv_area=abs(a.x*b.y-a.y*b.x)
        assert area>1e-10 and abs(ATLAS_SIZE[atlas]*math.sqrt(uv_area/area)-density)<.1, (name, 'UV density')
        for li in tri.loops:
            loop=mesh.loops[li];p=mesh.vertices[loop.vertex_index].co;n=tri.normal;t=uv.data[li].uv
            exported.append([round(v,6) for v in (p.x,p.z,-p.y,n.x,n.z,-n.y,t.x,1-t.y)])
    assert len(exported)<=6000
    data={'name':name,'texture':f'packs/underkeep/source/{atlas}-atlas.png','vertices':exported}
    (OUT/'models'/f'{name}.mesh.json').write_text(json.dumps(data,separators=(',',':'))+'\n')
    obj.location.x=list(models).index(name)*2.5
# One export avoids a Blender 5.2 crash when repeatedly exporting a temporary scene.
for obj in scene.objects: obj.select_set(True,view_layer=scene.view_layers[0])
with bpy.context.temp_override(scene=scene,view_layer=scene.view_layers[0]):
    bpy.ops.export_scene.gltf(filepath=str(OUT/'models/underkeep.glb'),use_selection=True,export_format='GLB',export_yup=True)
bpy.data.libraries.write(str(OUT/'models/underkeep.blend'),{scene},fake_user=True,compress=True)
print('Underkeep exports:',[(n,len(v[0].loop_triangles)) for n,v in prepared.items()])
