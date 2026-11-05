"""Rebuild SC02 painted town architecture in background Blender, bounded to two threads.
All coordinates below are runtime cells; Blender converts (x,y,z) to (x,-z,y).
Only this pack's outputs are written. Original generated master is never replaced.
"""
import bpy, json, math, struct, sys
import numpy as np
from pathlib import Path
from mathutils import Vector
sys.path.insert(0, str(Path(__file__).resolve().parent))
from city_materials import extract_tiles, sample_mirrored, padded_extent, padded_patch, pack_pages, split_triangles
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

from city_architecture import make_kit

houses=make_kit()
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
            axis=Vector((1,0,0)) if abs(normal.z)>.001 else Vector((0,0,1))
            cross=normal.cross(axis).normalized();mode='roof'
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
size=4096
positions=pack_pages([(k[0],k[1]) for k in entries],size,GUTTER,2)
entries=dict(zip(entries,positions))
page_count=max(p[0] for p in positions)+1
atlas_images=[]
atlas_names=['architecture-atlas' if i==0 else f'architecture-atlas-{i}' for i in range(page_count)]
for page in range(page_count):
    atlas=np.zeros((size,size,4),dtype=np.float32);atlas[:,:,3]=1
    for key,(owner,x,y) in entries.items():
        if owner!=page:continue
        ww,hh,tile,shade,_=key
        patch=material_patch(tile,ww,hh,shade,offsets[key])
        padded=padded_patch(patch,GUTTER)
        atlas[y-GUTTER:y-GUTTER+padded.shape[0],x-GUTTER:x-GUTTER+padded.shape[1]]=padded
    atlas_images.append(save_image(atlas_names[page],atlas))
    del atlas
model_parts={}
for model_index,house in enumerate(houses):
    mappings=[]
    for group,coords in raw_models[house.name]:
        key,lo=groups[group];mappings.append((key,[(x-lo[0],y-lo[1]) for x,y in coords]))
    verts=[];faces=[]
    for pts,_,_ in house.faces:
        start=len(verts);verts.extend((p.x,-p.z,p.y) for p in pts);faces.append(tuple(range(start,len(verts))))
    mesh=bpy.data.meshes.new(house.name);mesh.from_pydata(verts,[],faces);mesh.update()
    obj=bpy.data.objects.new(house.name,mesh);scene.collection.objects.link(obj)
    for page,img in enumerate(atlas_images):
        mat=bpy.data.materials.new(house.name+f'-painted-{page}');mat.use_nodes=True
        nodes=mat.node_tree.nodes;nodes.clear();tex=nodes.new('ShaderNodeTexImage');tex.image=img
        shader=nodes.new('ShaderNodeEmission');output=nodes.new('ShaderNodeOutputMaterial')
        mat.node_tree.links.new(tex.outputs['Color'],shader.inputs['Color']);mat.node_tree.links.new(shader.outputs[0],output.inputs['Surface']);mesh.materials.append(mat)
    uv=mesh.uv_layers.new(name='MetricPaintedUV')
    for poly,(key,coords) in zip(mesh.polygons,mappings):
        page,x,y=entries[key];poly.material_index=page
        for li,(u,v) in zip(poly.loop_indices,coords):uv.data[li].uv=((x+u)/size,1-(y+v)/size)
    mesh.calc_loop_triangles();exported=[[] for _ in atlas_images]
    for tri in mesh.loop_triangles:
        points=[mesh.vertices[mesh.loops[i].vertex_index].co for i in tri.loops];coords=[uv.data[i].uv for i in tri.loops]
        area=(points[1]-points[0]).cross(points[2]-points[0]).length
        a,b=coords[1]-coords[0],coords[2]-coords[0];uvarea=abs(a.x*b.y-a.y*b.x)
        assert area>1e-9 and abs(size*math.sqrt(uvarea/area)-DENSITY)<.5,(house.name,'UV density',area)
        for li in tri.loops:
            p=mesh.vertices[mesh.loops[li].vertex_index].co;n=tri.normal;t=uv.data[li].uv
            assert abs(p.x)<=4 and abs(p.y)<=4 and 0<=p.z<=4,(house.name,'bounds',p[:])
            exported[tri.material_index].append([round(v,6) for v in (p.x,p.z,-p.y,n.x,n.z,-n.y,t.x,1-t.y)])
    total=sum(len(v) for v in exported)
    assert total<=24000,(house.name,'model vertex budget',total)
    parts=[]
    for page,vertices in enumerate(exported):
        for part in split_triangles(vertices):
            name=house.name if not parts else f'{house.name}-part-{len(parts)}'
            data={'name':name,'texture':f'packs/sanctuary-city/source/{atlas_names[page]}.png','vertices':part}
            (OUT/'models'/f'{name}.mesh.json').write_text(json.dumps(data,separators=(',',':'))+'\n')
            parts.append(f'packs/sanctuary-city/models/{name}.mesh.json')
            gltf_models.append((name,part,page,model_index))
    assert len(parts)<=8,(house.name,'part budget')
    model_parts[house.name]=parts
    obj.location.x=(model_index%4)*7;obj.location.y=-(model_index//4)*7
    report.append({'model':house.name,'triangles':total//3,'parts':len(parts),'atlases':page_count,'atlas_size':size,'texels_per_cell':DENSITY,'gutter':GUTTER,'panel_inset':min(w//3,h//2)//64})
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
for page,name in enumerate(atlas_names):
    image_view=append((OUT/'source'/f'{name}.png').read_bytes())
    images.append({'bufferView':image_view,'mimeType':'image/png'})
    materials.append({'name':name,'pbrMetallicRoughness':{'baseColorTexture':{'index':page},'metallicFactor':0,'roughnessFactor':1},'extensions':{'KHR_materials_unlit':{}}})
for index,(name,vertices,page,model_index) in enumerate(gltf_models):
    attributes={}
    for semantic,columns,kind in [('POSITION',(0,1,2),'VEC3'),('NORMAL',(3,4,5),'VEC3'),('TEXCOORD_0',(6,7),'VEC2')]:
        values=[[v[c] for c in columns] for v in vertices]
        data=struct.pack('<'+'f'*(len(values)*len(columns)),*(x for v in values for x in v))
        accessor={'bufferView':append(data,34962),'componentType':5126,'count':len(values),'type':kind}
        if semantic=='POSITION':
            accessor['min']=[min(v[c] for v in values) for c in range(3)]
            accessor['max']=[max(v[c] for v in values) for c in range(3)]
        attributes[semantic]=len(accessors);accessors.append(accessor)
    meshes.append({'name':name,'primitives':[{'attributes':attributes,'material':page,'mode':4}]})
    nodes.append({'mesh':index,'name':name,'translation':[(model_index%4)*7,0,(model_index//4)*7]})
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
pack['buildings']=[{'id':h.name,'parts':model_parts[h.name]} for h in houses]
(OUT/'pack.json').write_text(json.dumps(pack,indent=2)+'\n')
(OUT/'models/export-report.json').write_text(json.dumps(report,indent=2)+'\n')
print('Sanctuary exports:',report,flush=True)

# Preserve SC03 prototype when regenerating the complete city pack.
import runpy
runpy.run_path(str(ROOT / "tools/export_city_tree.py"), run_name="__main__")

runpy.run_path(str(ROOT / "tools/export_city_dressing.py"), run_name="__main__")
