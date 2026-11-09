"""Headless unlit front/rear/underside review of the BR02 packed source models.
blender --background --factory-startup -noaudio -t 2 --python THIS -- NEW_OUTPUT
"""
import sys
import argparse
from pathlib import Path
import bpy
from mathutils import Vector

root = Path(__file__).resolve().parent.parent
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('output', type=Path)
parser.add_argument('--kit', help='Review only one named kit after a localized edit')
args = parser.parse_args(sys.argv[sys.argv.index('--') + 1:])
out = args.output.resolve()
out.mkdir(parents=True, exist_ok=False)
bpy.ops.wm.open_mainfile(filepath=str(root / 'assets/packs/underkeep/models/blueprint-room-pair.blend'))
scene = bpy.data.scenes['blueprint-room-pair']
bpy.context.window.scene = scene
scene.render.engine = 'CYCLES'
scene.cycles.samples = 1
scene.cycles.use_denoising = False
scene.render.threads_mode = 'FIXED'
scene.render.threads = 2
scene.render.resolution_x = 640
scene.render.resolution_y = 480
scene.render.resolution_percentage = 100
scene.world = bpy.data.worlds.new('RoomReview')
scene.world.color = (.06, .06, .06)
scene.view_settings.view_transform = 'Standard'
cam = bpy.data.objects.new('ReviewCamera', bpy.data.cameras.new('ReviewCamera'))
scene.collection.objects.link(cam)
scene.camera = cam
cam.data.type = 'ORTHO'
objects = [o for o in scene.objects if o.type == 'MESH']
for obj in objects:
    if args.kit and obj.name != 'room-' + args.kit:
        continue
    for other in objects:
        other.hide_render = other != obj
    corners = [obj.matrix_world @ Vector(c) for c in obj.bound_box]
    center = sum(corners, Vector()) / 8
    cam.data.ortho_scale = 2.0 if obj.name.endswith(('table-and-benches', 'chandelier')) else 1.5
    for label, offset in [('front', (2, 3, 2.5)), ('rear', (-2, -3, 2)), ('underside', (1, -2, -3))]:
        cam.location = center + Vector(offset)
        cam.rotation_euler = (center - cam.location).to_track_quat('-Z', 'Y').to_euler()
        scene.render.filepath = str(out / f'{obj.name}-{label}.png')
        bpy.ops.render.render(write_still=True)
