"""Build PLAYER2 dash from supplied UE boundary poses, preserving both rigs."""
import bpy, math, json
from pathlib import Path
from mathutils import Matrix, Vector, Quaternion

HERE=Path(__file__).resolve().parent
BASE=HERE.parents[1]
OUT=BASE/'Animations'/'PLAYER2_AirDash'
OUT.mkdir(exist_ok=True)
boundary=json.loads((HERE/'boundaries.json').read_text())
def ue_name(n): return n.replace(' ','-').replace('.','_')
def set_frame(f): bpy.context.scene.frame_set(int(f), subframe=f%1)
def update(): bpy.context.view_layer.update()
def disconnect_bones(r):
    bpy.ops.object.select_all(action='DESELECT');r.select_set(True);bpy.context.view_layer.objects.active=r
    bpy.ops.object.mode_set(mode='EDIT')
    for b in r.data.edit_bones:b.use_connect=False
    bpy.ops.object.mode_set(mode='OBJECT')
def world(r,b): return r.matrix_world@r.pose.bones[b].matrix
def set_world(r,b,m):
    r.pose.bones[b].matrix=r.matrix_world.inverted()@m
    update()
def pose_snapshot(r):
    return {b.name:(b.location.copy(), b.rotation_quaternion.copy(), b.scale.copy()) for b in r.pose.bones}
def set_pose(r,pose):
    for n,(l,q,s) in pose.items():
        b=r.pose.bones[n];b.location=l;b.rotation_quaternion=q;b.scale=s
    update()
def apply_boundary(r,kind):
    for b in r.pose.bones:
        key='ROOT' if b.name=='Bone.007' else ue_name(b.name)
        set_world(r,b.name,Matrix(boundary[kind][key]))
def aim(r,n,direction):
    m=world(r,n); q=m.to_quaternion()
    swing=(q@Vector((0,1,0))).rotation_difference(Vector(direction).normalized())
    loc,_,scale=m.decompose()
    set_world(r,n,Matrix.LocRotScale(loc,swing@q,scale))
def rotate(r,n,axis,degrees):
    m=world(r,n);l,q,s=m.decompose()
    set_world(r,n,Matrix.LocRotScale(l,Quaternion(Vector(axis),math.radians(degrees))@q,s))

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=str(BASE/'source'/'newlyanimatedrobot4K,.fbx'),automatic_bone_orientation=False,ignore_leaf_bones=False)
rig=next(o for o in bpy.data.objects if o.type=='ARMATURE')
disconnect_bones(rig)
meshes=[o for o in bpy.data.objects if o.type=='MESH']
for o in list(bpy.data.objects): o.animation_data_clear()
for a in list(bpy.data.actions): bpy.data.actions.remove(a)
for b in rig.pose.bones: b.rotation_mode='QUATERNION'
apply_boundary(rig,'Jump'); start=pose_snapshot(rig)
geometry_start={ue_name(o.name):o.matrix_world.copy() for o in meshes}
apply_boundary(rig,'Land'); finish=pose_snapshot(rig)
set_pose(rig,start)

# Pitch from the pelvis, then pose the limbs in world space. Source faces -X.
rotate(rig,'Bone',(0,1,0),-62)
rotate(rig,'Bone.002',(0,1,0),18)
rotate(rig,'Bone.003',(0,1,0),30)
# Leading right arm reaches forward/down, with an elbow bend; left sweeps back.
aim(rig,'upper arm.r',(-0.92,0.12,-0.38))
aim(rig,'lowwer arm.r',(-0.96,-0.08,0.27))
aim(rig,'upper arm.l',(0.88,-0.13,-0.46))
aim(rig,'lowwer arm.l',(0.72,-0.08,0.69))
# Trailing legs, staggered at knee to give the dash an airborne silhouette.
aim(rig,'upper leg.l',(0.77,-0.08,-0.63))
aim(rig,'lowwer leg.l',(0.95,0.04,-0.31))
aim(rig,'upper leg.r',(0.87,0.09,-0.48))
aim(rig,'lowwer leg.r',(0.66,-0.04,-0.75))
# Feet are separate IK-root branches in the supplied skeleton: relocate each
# ankle from the calf endpoint rather than leaving the feet at the jump pose.
for side in ['l','r']:
    calf=rig.pose.bones['lowwer leg.'+side]
    ankle=rig.matrix_world@calf.tail
    foot=world(rig,'foot.'+side);fl,fq,fs=foot.decompose()
    fl=ankle
    set_world(rig,'foot.'+side,Matrix.LocRotScale(fl,fq,fs))
    aim(rig,'foot.'+side,(0.63,0,-0.77))
dash=pose_snapshot(rig)

scene=bpy.context.scene
scene.render.fps=60;scene.render.fps_base=1
scene.frame_start=1;scene.frame_end=19
rig.animation_data_create(); action=bpy.data.actions.new('PLAYER2_AirDash_Editable')
rig.animation_data.action=action
keys=[(1,start),(5,dash),(13,dash),(19,finish)]
samples={}
last_q={}
for frame in range(1,20):
    for (fa,pa),(fb,pb) in zip(keys,keys[1:]):
        if fa<=frame<=fb: break
    t=(frame-fa)/(fb-fa)
    pose={}
    for n in pa:
        l1,q1,s1=pa[n];l2,q2,s2=pb[n]
        q=q1.slerp(q2,t)
        if n in last_q and q.dot(last_q[n])<0: q.negate()
        last_q[n]=q.copy()
        pose[n]=(l1.lerp(l2,t),q,s1.lerp(s2,t))
    set_frame(frame);set_pose(rig,pose)
    for b in rig.pose.bones:
        for prop in ['location','rotation_quaternion','scale']:
            b.keyframe_insert(prop,frame=frame,group=b.name)
    samples[frame]={'bones':{ue_name(b.name):world(rig,b.name).copy() for b in rig.pose.bones},
                    'geometry':{ue_name(o.name):o.matrix_world.copy() for o in meshes}}
for layer in action.layers:
    for strip in layer.strips:
        for bag in strip.channelbags:
            for fc in bag.fcurves:
                for k in fc.keyframe_points:k.interpolation='LINEAR'
rig.name='PLAYER2_EditableRig'

# The UE animation export carries the exact target hierarchy, including rigid
# mesh attachment bones and the Bone_007 root object. Use it as the export rig.
before=set(bpy.data.objects)
bpy.ops.import_scene.fbx(filepath=str(BASE/'Animations'/'PLAYER2_Jump.fbx'),automatic_bone_orientation=False,ignore_leaf_bones=False)
export=next(o for o in bpy.data.objects if o not in before and o.type=='ARMATURE')
disconnect_bones(export)
export.animation_data_clear()
for b in export.pose.bones:b.rotation_mode='QUATERNION'
scene.render.fps=60;scene.frame_end=19
export.matrix_world=Matrix(boundary['Jump']['ROOT'])
export.animation_data_create();ea=bpy.data.actions.new('PLAYER2_AirDash');export.animation_data.action=ea
offsets={}
for b in export.pose.bones:
    if b.name not in samples[1]['bones']:
        assert b.name in geometry_start,b.name
        offsets[b.name]=geometry_start[b.name].inverted()@Matrix(boundary['Jump'][b.name])
last_q={}
target_frames={}
for frame in range(1,20):
    set_frame(frame)
    target_frames[frame]={}
    for b in export.pose.bones:
        if frame in [1,19]:
            target=Matrix(boundary['Jump' if frame==1 else 'Land'][b.name])
        elif b.name in samples[frame]['bones']:target=samples[frame]['bones'][b.name]
        else:target=samples[frame]['geometry'][b.name]@offsets[b.name]
        set_world(export,b.name,target)
        if b.name in last_q and b.rotation_quaternion.dot(last_q[b.name])<0:
            b.rotation_quaternion.negate()
        last_q[b.name]=b.rotation_quaternion.copy()
        for prop in ['location','rotation_quaternion','scale']:b.keyframe_insert(prop,frame=frame,group=b.name)
        target_frames[frame][b.name]=[list(row) for row in target]
for layer in ea.layers:
    for strip in layer.strips:
        for bag in strip.channelbags:
            for fc in bag.fcurves:
                for k in fc.keyframe_points:k.interpolation='LINEAR'
# Animation-only FBX has no skin bind pose. Supply rest transforms at the
# current frame so default FBX node scales are not taken from the jump pose.
# The exported animation range still starts at frame 1.
set_frame(0)
for b in export.pose.bones:
    b.matrix_basis=Matrix.Identity(4)
    for prop in ['location','rotation_quaternion','scale']:b.keyframe_insert(prop,frame=0,group=b.name)
update()
bpy.ops.object.select_all(action='DESELECT');export.select_set(True);bpy.context.view_layer.objects.active=export
bpy.ops.export_scene.fbx(filepath=str(OUT/'PLAYER2_AirDash.fbx'),use_selection=True,object_types={'ARMATURE'},
    add_leaf_bones=False,use_armature_deform_only=False,bake_anim=True,bake_anim_use_all_bones=True,
    bake_anim_use_nla_strips=False,bake_anim_use_all_actions=False,bake_anim_force_startend_keying=True,
    bake_anim_step=1,bake_anim_simplify_factor=0,axis_forward='-Z',axis_up='Y')
(HERE/'expected_export.json').write_text(json.dumps({'root':[list(x) for x in export.matrix_world],
    'parents':{b.name:b.parent.name if b.parent else None for b in export.data.bones},'frames':target_frames}))
export.hide_set(True);export.hide_render=True
# Make the supplied mesh follow the final export rig, so edits to its baked
# action are visible immediately in the Blender file. Keep source weights.
for b in rig.pose.bones:
    c=b.constraints.new('COPY_TRANSFORMS');c.name='Follow export animation'
    c.target=export
    if b.name!='Bone.007':c.subtarget=ue_name(b.name)
    c.target_space='WORLD';c.owner_space='WORLD'
rig.hide_set(True);export.hide_set(False);export.show_in_front=True
for name,frame in [('Jump end',1),('Dash pose',5),('Return begins',13),('Landing start',19)]:scene.timeline_markers.new(name,frame=frame)
scene['animation_notes']='0.30s in-place dash, 60fps. Linear entry 1-5, hold 5-13, linear return 13-19. Exact exported Jump end / Land start poses. Edit Bone_007 and action PLAYER2_AirDash; the source display rig follows it. Frame 0 is a rest reference outside the clip. Re-export with export_airdash.py.'
set_frame(8)
# Studio camera is only for internal geometry/pose inspection.
cam_data=bpy.data.cameras.new('InspectionCamera');cam=bpy.data.objects.new('InspectionCamera',cam_data);scene.collection.objects.link(cam)
cam.location=(0,25,-1);target=Vector((0,0,-3.5));cam.rotation_euler=(target-cam.location).to_track_quat('-Z','Y').to_euler()
cam_data.type='ORTHO';cam_data.ortho_scale=13;scene.camera=cam
scene.render.engine='BLENDER_WORKBENCH';scene.render.resolution_x=900;scene.render.resolution_y=760;scene.render.resolution_percentage=100
scene.display.shading.light='STUDIO';scene.display.shading.color_type='SINGLE';scene.display.shading.single_color=(0.4,0.48,0.55)
scene.display.shading.show_shadows=True;scene.display.shading.show_cavity=True
scene.display.shading.background_type='WORLD'
if scene.world is None:scene.world=bpy.data.worlds.new('StudioWorld')
scene.world.color=(0.12,0.12,0.12)
bpy.ops.object.select_all(action='DESELECT');export.select_set(True);bpy.context.view_layer.objects.active=export
for area in bpy.context.screen.areas:
    if area.type=='VIEW_3D':
        area.spaces.active.region_3d.view_distance=16
        area.spaces.active.region_3d.view_location=(0,0,-3)
        area.spaces.active.region_3d.view_rotation=cam.rotation_euler.to_quaternion()
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'PLAYER2_AirDash.blend'))
scene.render.filepath=str(HERE/'qa_dash.png');bpy.ops.render.render(write_still=True)
print('AIR_DASH_OUTPUT',str(OUT))
