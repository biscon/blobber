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
master.scale(256,256)
models={}
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
    for x in (-.35,.35): box((x,0,.165),(.06,.58,.26))
    for y in (-.26,.26):box((0,y,.165),(.64,.06,.26))
    # Interior floor sits above the bottom, with prepainted darker wood.
    box((0,0,.075),(.64,.46,.025),2)
    for x in (-.26,.26):
        for y in (-.295,.295):box((x,y,.17),(.042,.014,.27),3)
    hinge=Matrix.Translation((0,.26,.30))@Matrix.Rotation(math.radians(-105 if opened else 0),4,'X')@Matrix.Translation((0,-.26,-.30))
    box((0,0,.325),(.78,.60,.055),0,hinge)
    for x in (-.26,.26):box((x,0,.36),(.045,.60,.015),3,hinge)
    box((0,-.302,.265),(.075,.02,.10),3)
    for x in (-.26,.26): box((x,.29,.29),(.06,.035,.035),3)
box=model('cage')
box((0,0,.035),(.72,.72,.07),2)
for x in (-.32,.32):
    for y in (-.32,.32):
        box((x,y,.43),(.075,.075,.84))
        for z in (.10,.74):box((x,y,z),(.082,.082,.04),3)
for z in (.12,.77):
    for y in (-.32,.32):box((0,y,z),(.64,.06,.065))
    for x in (-.32,.32):box((x,0,z),(.06,.64,.065))
for k in range(1,7):
    p=-.32+k*.64/7
    for y in (-.32,.32):box((p,y,.445),(.018,.022,.61),3)
    for x in (-.32,.32):box((x,p,.445),(.022,.018,.61),3)
    box((p,0,.785),(.018,.64,.018),3)
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
    for k in range(8):
        a=math.tau*k/8
        t=Matrix.Translation((0,y,.36))@Matrix.Rotation(a,4,'X')
        box((0,0,0),(.74,.046,.10),0,t)
    for x in (-.29,.29):box((x,y,.31),(.08,.09,.17),3)
    box((.40,y,.36),(.11,.035,.035),3)
    box((.45,y,.32),(.035,.035,.12),3)
for y in (-.35,.35):box((0,y,.35),(.55,.045,.012),2)
for name,(verts,faces,tiles,_) in models.items():
    mesh=bpy.data.meshes.new('Underkeep-'+name);mesh.from_pydata(verts,[],faces);mesh.update()
    obj=bpy.data.objects.new('Underkeep-'+name,mesh);scene.collection.objects.link(obj)
    image=bpy.data.images.load(str(OUT/'source/material-master.png'),check_existing=False);image.scale(256,256);image.name='Underkeep-'+name+'-atlas'
    atlas='chest' if name.startswith('chest') else name
    image.filepath_raw=str(OUT/'source'/f'{atlas}-atlas.png');image.file_format='PNG';image.save();image.pack()
    mat=bpy.data.materials.new('Underkeep-'+name);mat.use_nodes=True
    nodes=mat.node_tree.nodes;nodes.clear();tex=nodes.new('ShaderNodeTexImage');tex.image=image;tex.interpolation='Closest'
    shader=nodes.new('ShaderNodeEmission');output=nodes.new('ShaderNodeOutputMaterial')
    mat.node_tree.links.new(tex.outputs['Color'],shader.inputs['Color']);mat.node_tree.links.new(shader.outputs[0],output.inputs['Surface'])
    mesh.materials.append(mat)
    uv=mesh.uv_layers.new(name='PaintedUV')
    for poly,tile in zip(mesh.polygons,tiles):
        ox=.5*(tile%2);oy=.5*(1-tile//2)
        for li,(u,v) in zip(poly.loop_indices,[(0,0),(1,0),(1,1),(0,1)]):uv.data[li].uv=(ox+.018+u*.464,oy+.018+v*.464)
    mesh.calc_loop_triangles()
    exported=[]
    for tri in mesh.loop_triangles:
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
print('Underkeep exports:',[(n,len(v[1])*2) for n,v in models.items()])
