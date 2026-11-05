"""SC03 representative tree: deterministic Blender source, GLB and runtime meshes.
Run after export_sanctuary_city.py, or alone to update only the tree prototype.
Generated masters remain untouched. Runtime coordinates are X/Y-up/Z.
"""
import json
import math
import struct
from pathlib import Path

import bpy
import numpy as np
from mathutils import Vector

ROOT = Path(__file__).resolve().parent.parent
PACK = ROOT / 'assets/packs/sanctuary-city'
bpy.ops.wm.read_factory_settings(use_empty=True)
scene = bpy.data.scenes.new('SC03Tree')
scene.render.threads_mode = 'FIXED'
scene.render.threads = 2


def texture(source, name, size, padding=0):
    image = bpy.data.images.load(str(PACK / 'source' / source), check_existing=False)
    image.scale(size - 2 * padding, size - 2 * padding)
    pixels = np.empty(len(image.pixels), dtype=np.float32)
    image.pixels.foreach_get(pixels)
    pixels = pixels.reshape(size - 2 * padding, size - 2 * padding, 4)
    if padding:
        padded = np.zeros((size, size, 4), dtype=np.float32)
        padded[padding:-padding, padding:-padding] = pixels
        pixels = padded
    result = bpy.data.images.new(name, width=size, height=size, alpha=True)
    result.pixels.foreach_set(np.ascontiguousarray(pixels).ravel())
    result.filepath_raw = str(PACK / 'source' / (name + '.png'))
    result.file_format = 'PNG'
    result.save()
    result.pack()
    return result


bark = texture('bark-master.png', 'tree-bark', 1024)
foliage = texture('foliage-master.png', 'tree-foliage', 512, 76)
parts = [[], []]


def face(part, points, coords):
    points = [Vector((p[0], max(0, p[1]), p[2])) for p in points]
    n = (points[1] - points[0]).cross(points[2] - points[0]).normalized()
    for i in range(1, len(points) - 1):
        for k in (0, i, i + 1):
            point = points[k].copy()
            point.y = max(0, point.y)
            parts[part].append([round(v, 6) for v in (*point, *n, *coords[k])])


def limb(points, radii, sides=9):
    # Shared rings give a continuous taper without coplanar joints/end caps.
    points = [Vector(p) for p in points]
    direction = (points[-1] - points[0]).normalized()
    u = direction.cross(Vector((0, 0, 1))).normalized()
    v = direction.cross(u).normalized()
    rings = [[p + radius * (math.cos(i * 2 * math.pi / sides) * u +
                           math.sin(i * 2 * math.pi / sides) * v)
              for i in range(sides)] for p, radius in zip(points, radii)]
    distance = 0
    for j in range(len(points) - 1):
        end = distance + (points[j + 1] - points[j]).length
        for i in range(sides):
            k = (i + 1) % sides
            # 1024 atlas at 256 texels/cell, grain follows each limb.
            width = 2 * math.pi * (radii[j] + radii[j + 1]) / 2 / sides / 4
            left = i * .0175
            face(0, [rings[j][i], rings[j][k], rings[j + 1][k], rings[j + 1][i]],
                 [(left, distance / 4), (left + width, distance / 4),
                  (left + width, end / 4), (left, end / 4)])
        distance = end
    face(0, list(reversed(rings[0])), [(0.1, 0.1)] * sides)
    face(0, rings[-1], [(0.2, 0.2)] * sides)


limb([(0, 0, 0), (.035, .28, -.025), (-.025, .72, .015),
      (.03, 1.1, 0), (.13, 1.48, .08), (.08, 1.9, .02)],
     [.19, .135, .105, .085, .05, .018])
for i in range(5):
    a = i * 2 * math.pi / 5 + .3
    limb([(0, .18, 0), (.27 * math.cos(a), .055, .27 * math.sin(a)),
          (.43 * math.cos(a), .014, .43 * math.sin(a))], [.085, .045, .008], 6)

clusters = []
for i in range(7):
    a = i * 2 * math.pi / 7 + .25
    radius = .64 + .07 * math.sin(i * 3.1)
    center = Vector((radius * math.cos(a), 1.8 + .10 * math.sin(i * 2.2), radius * math.sin(a)))
    limb([(.01, 1.04 + .065 * i, 0), (center.x * .5, center.y - .28, center.z * .5),
          (center.x, center.y, center.z)], [.065, .042, .012], 7)
    clusters.append((center, a))
for i in range(3):
    a = i * 2 * math.pi / 3
    clusters.append((Vector((.25 * math.cos(a), 1.93 + .035 * i, .25 * math.sin(a))), a + .4))

# 360 interior pixels / 256 = 1.40625 cell card. Artwork includes organic alpha gaps.
card_size = 360 / 256
uv = [(76 / 512, 436 / 512), (436 / 512, 436 / 512),
      (436 / 512, 76 / 512), (76 / 512, 76 / 512)]
for index, (center, angle) in enumerate(clusters):
    for cross in range(2):
        a = angle + cross * math.pi / 2
        u = Vector((math.cos(a), 0, math.sin(a))) * card_size / 2
        # Vary tilt to avoid obvious vertical cross patterns during turns.
        tilt = (-.22 if cross else .30) + .08 * math.sin(index)
        v = Vector((-math.sin(a) * tilt, math.sqrt(1 - tilt * tilt), math.cos(a) * tilt)) * card_size / 2
        points = [center - u - v, center + u - v, center + u + v, center - u + v]
        face(1, points, uv)
        face(1, list(reversed(points)), list(reversed(uv)))

assert all(0 <= v[6] <= 1 and 0 <= v[7] <= 1 for p in parts for v in p)
assert max(v[1] for part in parts for v in part) <= 4
assert min(v[1] for part in parts for v in part) >= 0
assert all(abs(v[0]) <= 4 and abs(v[2]) <= 4 for part in parts for v in part)
assert sum(map(len, parts)) <= 24000 and all(len(p) <= 6000 for p in parts)
names = ['courtyard-tree-bark', 'courtyard-tree-foliage']
images = [bark, foliage]
paths = []
for index, (name, vertices, image) in enumerate(zip(names, parts, images)):
    path = PACK / 'models' / (name + '.mesh.json')
    path.write_text(json.dumps(dict(name=name, texture='packs/sanctuary-city/source/' + image.name + '.png',
                                    cutout=bool(index), vertices=vertices), separators=(',', ':')) + '\n')
    paths.append(str(path.relative_to(ROOT / 'assets')))
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata([(v[0], -v[2], v[1]) for v in vertices], [],
                     [(i, i + 1, i + 2) for i in range(0, len(vertices), 3)])
    mesh.update()
    layer = mesh.uv_layers.new(name='PaintedUV')
    for loop in mesh.loops:
        vertex = vertices[loop.vertex_index]
        layer.data[loop.index].uv = (vertex[6], 1 - vertex[7])
    obj = bpy.data.objects.new(name, mesh)
    scene.collection.objects.link(obj)
    material = bpy.data.materials.new(name)
    material.use_nodes = True
    nodes = material.node_tree.nodes
    nodes.clear()
    tex = nodes.new('ShaderNodeTexImage')
    tex.image = image
    emission = nodes.new('ShaderNodeEmission')
    output = nodes.new('ShaderNodeOutputMaterial')
    material.node_tree.links.new(tex.outputs['Color'], emission.inputs['Color'])
    if index:
        transparent = nodes.new('ShaderNodeBsdfTransparent')
        mix = nodes.new('ShaderNodeMixShader')
        material.node_tree.links.new(tex.outputs['Alpha'], mix.inputs[0])
        material.node_tree.links.new(transparent.outputs[0], mix.inputs[1])
        material.node_tree.links.new(emission.outputs[0], mix.inputs[2])
        material.node_tree.links.new(mix.outputs[0], output.inputs['Surface'])
    else:
        material.node_tree.links.new(emission.outputs[0], output.inputs['Surface'])
    mesh.materials.append(material)

bpy.context.preferences.filepaths.save_version = 0
bpy.context.window.scene = scene
bpy.ops.wm.save_as_mainfile(filepath=str(PACK / 'models/courtyard-tree.blend'), compress=False)

# Deterministic unlit GLB, without optional Blender exporter dependencies.
binary, views, accessors, meshes = bytearray(), [], [], []


def append(data, target=None):
    while len(binary) % 4:
        binary.append(0)
    view = dict(buffer=0, byteOffset=len(binary), byteLength=len(data))
    if target:
        view['target'] = target
    views.append(view)
    binary.extend(data)
    return len(views) - 1


gimages, materials = [], []
for index, image in enumerate(images):
    gimages.append(dict(bufferView=append((PACK / 'source' / (image.name + '.png')).read_bytes()), mimeType='image/png'))
    material = dict(name=image.name, pbrMetallicRoughness=dict(baseColorTexture=dict(index=index),
                    metallicFactor=0, roughnessFactor=1), extensions={'KHR_materials_unlit': {}})
    if index:
        material.update(alphaMode='MASK', alphaCutoff=128 / 255)
    materials.append(material)
for index, vertices in enumerate(parts):
    attributes = {}
    for semantic, columns, kind in [('POSITION', (0, 1, 2), 'VEC3'), ('NORMAL', (3, 4, 5), 'VEC3'),
                                     ('TEXCOORD_0', (6, 7), 'VEC2')]:
        values = [[v[c] for c in columns] for v in vertices]
        data = struct.pack('<' + 'f' * (len(values) * len(columns)), *(v for row in values for v in row))
        accessor = dict(bufferView=append(data, 34962), componentType=5126, count=len(values), type=kind)
        if semantic == 'POSITION':
            accessor.update(min=[min(v[c] for v in values) for c in range(3)],
                            max=[max(v[c] for v in values) for c in range(3)])
        attributes[semantic] = len(accessors)
        accessors.append(accessor)
    meshes.append(dict(name=names[index], primitives=[dict(attributes=attributes, material=index, mode=4)]))
while len(binary) % 4:
    binary.append(0)
gltf = dict(asset=dict(version='2.0', generator='Blobber SC03 tree exporter'), scene=0,
            scenes=[dict(nodes=[0, 1])], nodes=[dict(mesh=i, name=names[i]) for i in range(2)],
            meshes=meshes, materials=materials, images=gimages, textures=[dict(source=i) for i in range(2)],
            buffers=[dict(byteLength=len(binary))], bufferViews=views, accessors=accessors,
            extensionsUsed=['KHR_materials_unlit'])
metadata = json.dumps(gltf, separators=(',', ':')).encode()
while len(metadata) % 4:
    metadata += b' '
glb = (struct.pack('<III', 0x46546c67, 2, 28 + len(metadata) + len(binary)) +
       struct.pack('<II', len(metadata), 0x4e4f534a) + metadata +
       struct.pack('<II', len(binary), 0x004e4942) + binary)
(PACK / 'models/courtyard-tree.glb').write_bytes(glb)
manifest = json.loads((PACK / 'pack.json').read_text())
manifest['buildings'] = [b for b in manifest['buildings'] if b['id'] != 'courtyard-tree']
manifest['buildings'].append(dict(id='courtyard-tree', parts=paths))
assert len(manifest['buildings']) <= 24
(PACK / 'pack.json').write_text(json.dumps(manifest, indent=2) + '\n')
report = dict(model='courtyard-tree', triangles=sum(map(len, parts)) // 3, cards=len(clusters) * 2,
              parts=2, textures=[1024, 512], texels_per_cell=256, cutout_threshold=128,
              bounds=[[min(v[c] for p in parts for v in p), max(v[c] for p in parts for v in p)]
                      for c in range(3)])
(PACK / 'models/tree-export-report.json').write_text(json.dumps(report, indent=2) + '\n')
print('SC03 tree export:', report, flush=True)
