"""Build a non-destructive robot rig and run-cycle draft using Blender's API."""
import bpy, math, json
from pathlib import Path
from mathutils import Vector, Quaternion

out=Path(bpy.data.filepath).parent/'Robot_Rig_Output'
out.mkdir(exist_ok=True)
mesh=next(o for o in bpy.context.scene.objects if o.type=='MESH')
bpy.ops.object.select_all(action='DESELECT'); mesh.select_set(True)
bpy.context.view_layer.objects.active=mesh
bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
mesh.name='Robot_Body'
cs=json.loads((out/'components.json').read_text())
floor=min(v.co.z for v in mesh.data.vertices)
scale=2/(max(v.co.z for v in mesh.data.vertices)-floor)
def point(v): return Vector((v[0]*scale,v[1]*scale,(v[2]-floor)*scale))
original={v.index:v.co.copy() for v in mesh.data.vertices}
for v in mesh.data.vertices: v.co=point(v.co)
arm=bpy.data.armatures.new('Robot_Skeleton')
rig=bpy.data.objects.new('Robot_Rig',arm);bpy.context.collection.objects.link(rig)
bpy.context.view_layer.objects.active=rig;mesh.select_set(False);rig.select_set(True)
rig.show_in_front=True
bpy.ops.object.mode_set(mode='EDIT')
def bone(name,head,tail,parent=None):
    b=arm.edit_bones.new('mixamorig:'+name);b.head=point(head);b.tail=point(tail)
    if parent: b.parent=arm.edit_bones['mixamorig:'+parent]
    return b
bone('Hips',(0,-.1,1.48),(0,-.1,1.9))
bone('Spine',(0,-.1,1.9),(0,-.1,2.8),'Hips')
bone('Spine1',(0,-.1,2.8),(0,-.1,3.5),'Spine')
bone('Spine2',(0,-.1,3.5),(0,-.18,4.25),'Spine1')
bone('Neck',(0,-.18,4.25),(0,-.3,4.65),'Spine2')
bone('Head',(0,-.3,4.65),(0,-.4,5.65),'Neck')
for side,s in [('Left',1),('Right',-1)]:
    bone(side+'Shoulder',(.25*s,-.1,3.95),(1.12*s,-.04,3.95),'Spine2')
    bone(side+'Arm',(1.12*s,-.04,3.95),(1.55*s,0,2.48),side+'Shoulder')
    bone(side+'ForeArm',(1.55*s,0,2.48),(1.6*s,.04,1.06),side+'Arm')
    bone(side+'Hand',(1.6*s,.04,1.06),(1.6*s,.05,.5),side+'ForeArm')
    bone(side+'UpLeg',(.65*s,-.1,1.47),(.67*s,-.1,.86),'Hips')
    bone(side+'Leg',(.67*s,-.1,.86),(.67*s,-.06,.34),side+'UpLeg')
    bone(side+'Foot',(.67*s,-.06,.34),(.67*s,-.5,.1),side+'Leg')
    bone(side+'ToeBase',(.67*s,-.5,.1),(.67*s,-.64,.1),side+'Foot')
bpy.ops.object.mode_set(mode='OBJECT')
groups={b.name:mesh.vertex_groups.new(name=b.name) for b in arm.bones}
for c in cs:
    for i in c['verts']:
        p=original[i]; side='Left' if p.x>0 else 'Right'; k=c['id']
        if k==0: name='Spine2'
        elif k in [1,12]: name='Hips'
        elif k in [2,3,4,5]: name='Spine2'
        elif k in [6,7]: name=side+('Arm' if p.z>2.48 else 'ForeArm')
        elif k in [8,9]: name=side+('UpLeg' if p.z>.89 else 'Leg' if p.z>.43 else 'Foot')
        elif k==10: name='Head'
        elif k==11: name='Neck'
        elif k in [13,14]: name=side+'Hand'
        groups['mixamorig:'+name].add([i],1,'REPLACE')
mod=mesh.modifiers.new('Robot skeletal deformation','ARMATURE');mod.object=rig
mesh.parent=rig
scene=bpy.context.scene;scene.unit_settings.system='METRIC'
scene.render.fps=30;scene.frame_start=1;scene.frame_end=24
def rotate(name,axis,angle):
    pb=rig.pose.bones['mixamorig:'+name];pb.rotation_mode='QUATERNION'
    local_axis=pb.bone.matrix_local.to_3x3().inverted()@Vector(axis)
    pb.rotation_quaternion=Quaternion(local_axis,math.radians(angle))
def select_export():
    bpy.ops.object.select_all(action='DESELECT');mesh.select_set(True);rig.select_set(True)
    bpy.context.view_layer.objects.active=rig

# Export a bind-pose skeleton before animation is assigned.
select_export()
bpy.ops.export_scene.fbx(filepath=str(out/'Robot_Rigged_BindPose.fbx'),use_selection=True,object_types={'ARMATURE','MESH'},add_leaf_bones=False,bake_anim=False,axis_forward='-Z',axis_up='Y')
# A separate evaluated mesh export lets Mixamo generate its own skeleton.
rotate('LeftArm',(0,1,0),-55);rotate('RightArm',(0,1,0),55)
bpy.context.view_layer.update()
deps=bpy.context.evaluated_depsgraph_get()
upload=bpy.data.objects.new('Robot_Mixamo_APose',bpy.data.meshes.new_from_object(mesh.evaluated_get(deps)))
bpy.context.collection.objects.link(upload)
bpy.ops.object.select_all(action='DESELECT');upload.select_set(True);bpy.context.view_layer.objects.active=upload
bpy.ops.wm.obj_export(filepath=str(out/'Robot_Mixamo_APose.obj'),export_selected_objects=True,forward_axis='NEGATIVE_Z',up_axis='Y',export_materials=True)
bpy.data.objects.remove(upload,do_unlink=True)
for pb in rig.pose.bones: pb.rotation_quaternion=Quaternion()

# One 0.8-second cycle; frame 25 duplicates frame 1 and is excluded from playback.
for frame in range(1,26):
    phase=2*math.pi*(frame-1)/24
    for pb in rig.pose.bones: pb.rotation_quaternion=Quaternion();pb.location=(0,0,0)
    rotate('Hips',(1,0,0),3)
    rotate('Spine',(1,0,0),5)
    rotate('Spine2',(0,0,1),3*math.sin(phase))
    rotate('Head',(1,0,0),-5)
    for side,offset in [('Left',0),('Right',math.pi)]:
        p=phase+offset; stride=math.cos(p)
        rotate(side+'UpLeg',(1,0,0),-32*stride-4)
        knee=20+48*max(0,math.sin(p))
        rotate(side+'Leg',(1,0,0),knee)
        rotate(side+'Foot',(1,0,0),12*stride-knee*.4)
        rotate(side+'Arm',(1,0,0),22*stride)
        rotate(side+'ForeArm',(1,0,0),-38-12*max(0,-stride))
    bpy.context.view_layer.update()
    evaluated=mesh.evaluated_get(bpy.context.evaluated_depsgraph_get())
    evmesh=evaluated.to_mesh()
    minz=min((evaluated.matrix_world@v.co).z for v in evmesh.vertices)
    evaluated.to_mesh_clear()
    # Keep feet above the floor with a small twice-per-cycle suspension bounce.
    lift=-minz+.018*abs(math.sin(phase))
    hips=rig.pose.bones['mixamorig:Hips']
    hips.location=hips.bone.matrix_local.to_3x3().inverted()@Vector((0,0,lift))
    for pb in rig.pose.bones:
        pb.keyframe_insert(data_path='rotation_quaternion',frame=frame,group=pb.name)
    hips.keyframe_insert(data_path='location',frame=frame,group=hips.name)
rig.animation_data.action.name='Robot_Run_InPlace'
rig.animation_data.action.use_fake_user=True
for layer in rig.animation_data.action.layers:
    for strip in layer.strips:
        for bag in strip.channelbags:
            for fc in bag.fcurves:
                for kp in fc.keyframe_points: kp.interpolation='LINEAR'
select_export()
bpy.ops.export_scene.fbx(filepath=str(out/'Robot_Run_InPlace.fbx'),use_selection=True,object_types={'ARMATURE','MESH'},add_leaf_bones=False,bake_anim=True,bake_anim_use_all_actions=False,bake_anim_use_nla_strips=False,bake_anim_simplify_factor=0,axis_forward='-Z',axis_up='Y')

# Preview camera is kept in a separate collection in the working blend.
bpy.ops.object.camera_add(location=(3.2,-6,2.9));cam=bpy.context.object
cam.name='Preview_Camera';cam.rotation_euler=(Vector((0,0,1))-cam.location).to_track_quat('-Z','Y').to_euler()
cam.data.type='ORTHO';cam.data.ortho_scale=2.7;scene.camera=cam
scene.render.engine='BLENDER_WORKBENCH';scene.display.shading.light='STUDIO'
scene.display.shading.color_type='SINGLE';scene.display.shading.single_color=(.55,.62,.7)
scene.display.shading.show_shadows=True;scene.display.shading.show_cavity=True
scene.render.resolution_x=800;scene.render.resolution_y=800;scene.render.resolution_percentage=100
select_export();scene.frame_set(1)
for screen in bpy.data.screens:
    for area in screen.areas:
        if area.type=='VIEW_3D':
            area.spaces.active.region_3d.view_distance=3.7
            area.spaces.active.region_3d.view_location=(0,0,1)
bpy.context.preferences.filepaths.save_version=0
bpy.ops.wm.save_as_mainfile(filepath=str(out/'Robot_Rigged_Run.blend'))
for frame in [1,7,13,19]:
    scene.frame_set(frame);scene.render.filepath=str(out/f'run_{frame:02d}.png');bpy.ops.render.render(write_still=True)
print('RIG_COMPLETE',len(arm.bones),'bones',len(mesh.data.vertices),'weighted vertices')
