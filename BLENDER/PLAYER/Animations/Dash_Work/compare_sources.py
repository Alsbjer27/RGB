import bpy, json, math
from pathlib import Path
from mathutils import Matrix
p=Path(__file__).resolve().parents[2]
def import_fbx(path):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.fbx(filepath=str(path),automatic_bone_orientation=False,ignore_leaf_bones=False)
    return next(o for o in bpy.data.objects if o.type=='ARMATURE')
data={}
for kind in ['Jump','Land']:
    r=import_fbx(p/'Animations'/f'PLAYER2_{kind}.fbx')
    a=r.animation_data.action
    f=a.frame_range[1 if kind=='Jump' else 0]
    bpy.context.scene.frame_set(math.floor(f),subframe=f%1)
    data[kind]={b.name:[list(x) for x in r.matrix_world@b.matrix] for b in r.pose.bones}
    data[kind]['ROOT']=[list(x) for x in r.matrix_world]
print('BOUNDARY_DIFF')
diff=[]
for n,mat in data['Jump'].items():
    a=Matrix(mat); b=Matrix(data['Land'][n]); d=(a.translation-b.translation).length
    angle=math.degrees(a.to_quaternion().rotation_difference(b.to_quaternion()).angle)
    diff.append((n,round(d,5),round(angle,3)))
print(sorted(diff,key=lambda x:x[1],reverse=True)[:12])
(Path(__file__).parent/'boundaries.json').write_text(json.dumps(data))
r=import_fbx(p/'source'/'newlyanimatedrobot4K,.fbx')
print('MESH_OBJECTS',[(o.name,o.parent.name if o.parent else None,o.parent_type,o.parent_bone,len(o.modifiers)) for o in bpy.data.objects if o.type=='MESH'])
print('ACTIONS',[(a.name,list(a.frame_range)) for a in bpy.data.actions if any(k in a.name.lower() for k in ['jump','land'])])
print('REST_BONES',[(b.name,list(b.head_local),list(b.tail_local)) for b in r.data.bones if b.name in ['Bone','Bone.001','Bone.002','upper leg.l','lowwer leg.l','foot.l','heel ik.l','upper arm.l','lowwer arm.l']])
