# Scout robot rig

Source `../Scout_Robot.blend` is preserved. Created with Blender 5.2.2 LTS.

- **Scout_Rigged.blend**: clean 22-bone humanoid FK armature, weighted mesh, original materials, A-pose rest position. No animation or Rigify dependency.
- **Scout_Rigged.fbx**: mesh plus custom skeleton. Mixamo recognition of this skeleton has not been tested.
- **Scout_Mixamo_Upload.fbx**: mesh-only A-pose for uploading to Mixamo and using its auto-rigger. This is the recommended upload file.
- **Scout_Mixamo_Upload.obj / .mtl**: alternative mesh-only upload.
- **Scout_APose.png / Scout_Pose_Check.png**: rest-pose and temporary articulation checks. The test pose is not saved in the blend.

Upload Scout_Mixamo_Upload.fbx to Mixamo, place its requested anatomical markers, and select no fingers if that option is available. The robot has hand stubs. Mixamo will create its own skeleton and weights; those weights may bend armor differently from the custom Blender rig. Download the animated character with its skin for use as a complete asset. Bone names alone do not guarantee direct action compatibility between independently created rigs.

The new copy removes old armature modifiers, obsolete vertex groups, and Rigify widget objects. Existing metarig coordinates guided placement; ankle pivots were moved to the visible ankle joints. All 1,473 vertices have one full-weight assignment to preserve armor rigidity where possible. Connected geometry spanning joints can still deform. There are no IK controls.

Verification: ran `Blender 5.2/blender.exe --background BLENDER/Scout_Robot.blend --python BLENDER/rig_scout.py`. Checked every vertex's weights, rendered and inspected rest and elbow/knee/head articulation poses, reimported the rigged FBX (one mesh, one 22-bone skeleton, weighted vertices), and reimported the upload FBX (one mesh, no skeleton). The saved blend was reopened and checked. Mixamo upload/acceptance and Unreal integration have not been tested.
