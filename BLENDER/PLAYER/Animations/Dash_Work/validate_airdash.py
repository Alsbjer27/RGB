import bpy,json,math
from pathlib import Path
from mathutils import Matrix
here=Path(__file__).resolve().parent
out=here.parent/'PLAYER2_AirDash'
expected=json.loads((here/'expected_export.json').read_text())
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.context.scene.render.fps=60
bpy.ops.import_scene.fbx(filepath=str(out/'PLAYER2_AirDash.fbx'),automatic_bone_orientation=False,ignore_leaf_bones=False)
r=next(o for o in bpy.data.objects if o.type=='ARMATURE')
# Blender auto-connects coincident joints on FBX import; that editor-only flag
# suppresses animated local translation. UE joints have no such restriction.
bpy.context.view_layer.objects.active=r;r.select_set(True)
bpy.ops.object.mode_set(mode='EDIT')
for b in r.data.edit_bones:b.use_connect=False
bpy.ops.object.mode_set(mode='OBJECT')
parents={b.name:b.parent.name if b.parent else None for b in r.data.bones}
assert parents==expected['parents'],'Hierarchy changed'
a=r.animation_data.action
duration=(a.frame_range[1]-a.frame_range[0])/bpy.context.scene.render.fps
assert abs(duration-0.3)<1e-5,duration
checks={}
for frame in range(1,20):
    bpy.context.scene.frame_set(int(a.frame_range[0])+frame-1)
    errors=[]
    for b in r.pose.bones:
        actual=r.matrix_world@b.matrix;want=Matrix(expected['frames'][str(frame)][b.name])
        pos=(actual.translation-want.translation).length
        angle=math.degrees(actual.to_quaternion().rotation_difference(want.to_quaternion()).angle)
        angle=min(angle,360-angle)
        errors.append((pos,angle,b.name))
    checks[frame]={'position':max(e[0] for e in errors),'degrees':max(e[1] for e in errors)}
    if checks[frame]['position']>=0.001:print('POSITION',frame,sorted(errors,reverse=True)[:4])
    if checks[frame]['degrees']>=0.15:print('ROTATION',frame,sorted(errors,key=lambda e:e[1],reverse=True)[:4])
    assert (r.matrix_world.translation-Matrix(expected['root']).translation).length<1e-5
result={'status':'PASS' if all(v['position']<0.001 and v['degrees']<0.15 for v in checks.values()) else 'FAIL','bone_count_excluding_root':len(parents),'root':r.name,'fps':bpy.context.scene.render.fps,
    'duration_seconds':duration,'frame_range':list(a.frame_range),'max_position_error_m':max(v['position'] for v in checks.values()),
    'max_rotation_error_degrees':max(v['degrees'] for v in checks.values()),'checks':checks,
    'unreal_import_and_playback':'Not run'}
(out/'validation.json').write_text(json.dumps(result,indent=2))
print('VALIDATION',json.dumps({k:v for k,v in result.items() if k!='checks'}))
assert result['status']=='PASS','FBX pose validation failed'
