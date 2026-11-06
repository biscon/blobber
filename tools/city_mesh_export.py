"""Offline packed Blender/unlit GLB export for prepared SC03 mesh parts."""
import json
import struct
from pathlib import Path
import bpy


def export_collection(pack, name, records):
    scene = bpy.data.scenes.new(name)
    scene.render.threads_mode = 'FIXED'
    scene.render.threads = 2
    binary, views, accessors, meshes, nodes = bytearray(), [], [], [], []
    images, materials, image_keys = [], [], {}

    def append(data, target=None):
        while len(binary) % 4:
            binary.append(0)
        view = dict(buffer=0, byteOffset=len(binary), byteLength=len(data))
        if target:
            view['target'] = target
        views.append(view)
        binary.extend(data)
        return len(views) - 1

    for record, position in records:
        vertices = record['vertices']
        key = (record['texture'], record.get('cutout', False))
        if key not in image_keys:
            image_keys[key] = len(images)
            path = pack.parents[1] / record['texture']
            image = bpy.data.images.load(str(path), check_existing=True)
            image.pack()
            images.append(dict(bufferView=append(path.read_bytes()), mimeType='image/png'))
            material = dict(name=Path(path).stem, pbrMetallicRoughness=dict(
                baseColorTexture=dict(index=len(materials)), metallicFactor=0, roughnessFactor=1),
                extensions={'KHR_materials_unlit': {}})
            if key[1]:
                material.update(alphaMode='MASK', alphaCutoff=128 / 255)
            materials.append(material)
        material_index = image_keys[key]
        image = bpy.data.images.load(str(pack.parents[1] / record['texture']), check_existing=True)
        mesh = bpy.data.meshes.new(record['name'])
        mesh.from_pydata([(v[0], -v[2], v[1]) for v in vertices], [],
                         [(i, i + 1, i + 2) for i in range(0, len(vertices), 3)])
        mesh.update()
        uv = mesh.uv_layers.new(name='PaintedUV')
        for loop in mesh.loops:
            v = vertices[loop.vertex_index]
            uv.data[loop.index].uv = (v[6], 1 - v[7])
        obj = bpy.data.objects.new(record['name'], mesh)
        scene.collection.objects.link(obj)
        obj.location = (position[0], -position[2], position[1])
        mat = bpy.data.materials.new(record['name'])
        mat.use_nodes = True
        ns = mat.node_tree.nodes
        ns.clear()
        tex, emit, out = ns.new('ShaderNodeTexImage'), ns.new('ShaderNodeEmission'), ns.new('ShaderNodeOutputMaterial')
        tex.image = image
        mat.node_tree.links.new(tex.outputs['Color'], emit.inputs['Color'])
        if key[1]:
            transparent, mix = ns.new('ShaderNodeBsdfTransparent'), ns.new('ShaderNodeMixShader')
            mat.node_tree.links.new(tex.outputs['Alpha'], mix.inputs[0])
            mat.node_tree.links.new(transparent.outputs[0], mix.inputs[1])
            mat.node_tree.links.new(emit.outputs[0], mix.inputs[2])
            mat.node_tree.links.new(mix.outputs[0], out.inputs['Surface'])
        else:
            mat.node_tree.links.new(emit.outputs[0], out.inputs['Surface'])
        mesh.materials.append(mat)
        attributes = {}
        for semantic, columns, kind in [('POSITION', (0, 1, 2), 'VEC3'), ('NORMAL', (3, 4, 5), 'VEC3'), ('TEXCOORD_0', (6, 7), 'VEC2')]:
            values = [[v[c] for c in columns] for v in vertices]
            data = struct.pack('<' + 'f' * (len(values) * len(columns)), *(x for row in values for x in row))
            acc = dict(bufferView=append(data, 34962), componentType=5126, count=len(values), type=kind)
            if semantic == 'POSITION':
                acc.update(min=[min(v[c] for v in values) for c in range(3)], max=[max(v[c] for v in values) for c in range(3)])
            attributes[semantic] = len(accessors)
            accessors.append(acc)
        nodes.append(dict(mesh=len(meshes), name=record['name'], translation=position))
        meshes.append(dict(name=record['name'], primitives=[dict(attributes=attributes, material=material_index, mode=4)]))
    bpy.context.preferences.filepaths.save_version = 0
    bpy.context.window.scene = scene
    bpy.ops.wm.save_as_mainfile(filepath=str(pack / 'models' / (name + '.blend')), compress=False)
    while len(binary) % 4:
        binary.append(0)
    gltf = dict(asset=dict(version='2.0', generator='Blobber SC03 dressing exporter'), scene=0,
                scenes=[dict(nodes=list(range(len(nodes))))], nodes=nodes, meshes=meshes, materials=materials,
                images=images, textures=[dict(source=i) for i in range(len(images))],
                buffers=[dict(byteLength=len(binary))], bufferViews=views, accessors=accessors,
                extensionsUsed=['KHR_materials_unlit'])
    metadata = json.dumps(gltf, separators=(',', ':')).encode()
    while len(metadata) % 4:
        metadata += b' '
    (pack / 'models' / (name + '.glb')).write_bytes(
        struct.pack('<III', 0x46546c67, 2, 28 + len(metadata) + len(binary)) +
        struct.pack('<II', len(metadata), 0x4e4f534a) + metadata +
        struct.pack('<II', len(binary), 0x004e4942) + binary)
