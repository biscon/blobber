"""Headless material review: emission materials, no lighting; all four sides and underside."""
import bpy, sys, math
from pathlib import Path
from mathutils import Vector
root=Path(__file__).resolve().parent.parent
out=Path(sys.argv[sys.argv.index('--')+1]);out.mkdir(parents=True,exist_ok=True)
bpy.ops.wm.open_mainfile(filepath=str(root/'assets/packs/underkeep/models/underkeep.blend'))
scene=bpy.data.scenes['UnderkeepAssets'];bpy.context.window.scene=scene
scene.render.engine='CYCLES';scene.cycles.samples=1
scene.render.threads_mode='FIXED';scene.render.threads=2
scene.render.resolution_x=420;scene.render.resolution_y=420;scene.render.resolution_percentage=100
scene.world=bpy.data.worlds.new('Review');scene.world.color=(.06,.06,.06)
scene.view_settings.view_transform='Standard'
cam=bpy.data.objects.new('ReviewCamera',bpy.data.cameras.new('ReviewCamera'));scene.collection.objects.link(cam);scene.camera=cam
cam.data.type='ORTHO'
objects=[o for o in scene.objects if o.type=='MESH']
for obj in objects:
 for other in objects:other.hide_render=other!=obj
 center=obj.location+Vector((0,0,.35));cam.data.ortho_scale=2.15 if 'rack' in obj.name else 1.25
 for i,offset in enumerate([(1,-2,1.5),(2,1,1.5),(-1,2,1.5),(-2,-1,1.5),(0,-1,-2)]):
  cam.location=center+Vector(offset);cam.rotation_euler=(center-cam.location).to_track_quat('-Z','Y').to_euler()
  scene.render.filepath=str(out/f'{obj.name}-{i}.png');bpy.ops.render.render(write_still=True)
