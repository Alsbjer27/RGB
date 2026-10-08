"""Re-export the saved editable clip. Run with Blender --background --python.

Save edits to PLAYER2_AirDash.blend before running this script. It reopens that
file, exports only Bone_007, and does not rewrite the Blender source file.
"""
import bpy
from pathlib import Path

folder=Path(__file__).resolve().parent
bpy.ops.wm.open_mainfile(filepath=str(folder/'PLAYER2_AirDash.blend'))
scene=bpy.context.scene
rig=bpy.data.objects['Bone_007']
scene.frame_set(0)  # Rest reference for FBX node defaults; outside export range.
assert scene.frame_start==1 and scene.frame_end==19
bpy.ops.object.select_all(action='DESELECT')
rig.hide_set(False);rig.select_set(True);bpy.context.view_layer.objects.active=rig
bpy.ops.export_scene.fbx(filepath=str(folder/'PLAYER2_AirDash.fbx'),
    use_selection=True,object_types={'ARMATURE'},add_leaf_bones=False,
    use_armature_deform_only=False,bake_anim=True,bake_anim_use_all_bones=True,
    bake_anim_use_nla_strips=False,bake_anim_use_all_actions=False,
    bake_anim_force_startend_keying=True,bake_anim_step=1,bake_anim_simplify_factor=0,
    axis_forward='-Z',axis_up='Y')
