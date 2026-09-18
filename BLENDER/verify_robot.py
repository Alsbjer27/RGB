import bpy,json
from pathlib import Path
from mathutils import Vector
out=Path(bpy.data.filepath).parent
rig=bpy.data.objects['Robot_Rig']; mesh=bpy.data.objects['Robot_Body']; scene=bpy.context.scene
assert len(rig.data.bones)==22
assert all(abs(sum(g.weight for g in v.groups)-1)<1e-6 for v in mesh.data.vertices)
def coords(frame):
    scene.frame_set(frame)
    ev=mesh.evaluated_get(bpy.context.evaluated_depsgraph_get()); data=ev.to_mesh()
    pts=[v.co.copy() for v in data.vertices];ev.to_mesh_clear();return pts
a=coords(1);b=coords(25)
error=max((x-y).length for x,y in zip(a,b));assert error<1e-5,error
floors=[min(p.z for p in coords(f)) for f in range(1,25)]
assert min(floors)>-1e-5,floors
scene.render.resolution_x=480;scene.render.resolution_y=480
for f in range(1,25):
    scene.frame_set(f);scene.render.filepath=str(out/f'preview_{f:02d}.png');bpy.ops.render.render(write_still=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=str(out/'Robot_Run_InPlace.fbx'))
import_rigs=[o for o in bpy.data.objects if o.type=='ARMATURE']
assert len(import_rigs)==1 and import_rigs[0].animation_data.action
report={'bones':22,'weighted_vertices':len(a),'loop_max_vertex_error':error,'minimum_floor_z':min(floors),'animated_fbx_reimport':'passed','mixamo_upload':'not tested','note':'Draft FK run. Connected mesh sections at joints may stretch; no IK controls or foot-lock system.'}
(out/'verification.json').write_text(json.dumps(report,indent=2))
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.wm.obj_import(filepath=str(out/'Robot_Mixamo_APose.obj'))
scene=bpy.context.scene
bpy.ops.object.camera_add(location=(3.2,-6,2.9));cam=bpy.context.object
cam.rotation_euler=(Vector((0,0,1))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.type='ORTHO';cam.data.ortho_scale=3.2;scene.camera=cam
scene.render.engine='BLENDER_WORKBENCH';scene.render.resolution_x=640;scene.render.resolution_y=640;scene.render.resolution_percentage=100
scene.render.filepath=str(out/'mixamo_apose.png');bpy.ops.render.render(write_still=True)
print('VERIFICATION',json.dumps(report))
