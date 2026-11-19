"""Build the packed editable cave kit and review GLB from the metric mesh exports."""
from pathlib import Path
import json
import bpy

root = Path(__file__).resolve().parents[1] / 'assets'
pack = root / 'packs/damp-caves'
bpy.ops.wm.read_factory_settings(use_empty=True)
materials = {}
for index, path in enumerate(sorted((pack / 'models').glob('*.mesh.json'))):
    data = json.loads(path.read_text())
    texture = data['texture']
    if texture not in materials:
        mat = bpy.data.materials.new(Path(texture).stem)
        mat.use_nodes = True
        nodes = mat.node_tree.nodes
        emission = nodes.new('ShaderNodeEmission')
        image = nodes.new('ShaderNodeTexImage')
        image.image = bpy.data.images.load(str(root / texture), check_existing=True)
        mat.node_tree.links.new(image.outputs['Color'], emission.inputs['Color'])
        mat.node_tree.links.new(emission.outputs[0], nodes['Material Output'].inputs['Surface'])
        materials[texture] = mat
    vertices = data['vertices']
    positions, indices = [], {}
    corners = []
    for x, y, z, *_ in vertices:
        position = (x, -z, y)
        if position not in indices:
            indices[position] = len(positions)
            positions.append(position)
        corners.append(indices[position])
    mesh = bpy.data.meshes.new(data['name'])
    mesh.from_pydata(positions, [], [corners[i:i+3] for i in range(0,len(corners),3)])
    mesh.update()
    uv = mesh.uv_layers.new(name='Paint')
    for loop, vertex in zip(uv.data, vertices):
        loop.uv = (vertex[6], 1 - vertex[7])
    obj = bpy.data.objects.new(data['name'], mesh)
    bpy.context.collection.objects.link(obj)
    obj.data.materials.append(materials[texture])
    # Individual objects retain the cell-floor pivot in local coordinates.
    obj.location = (index % 4 * 5.5, index // 4 * 2.8, 0)
bpy.ops.file.pack_all()
bpy.context.preferences.filepaths.save_version = 0
bpy.ops.wm.save_as_mainfile(filepath=str(pack/'source/damp-caves.blend'))
bpy.ops.export_scene.gltf(filepath=str(pack/'models/damp-caves.glb'),export_format='GLB')
