# Robot rig and run draft

Source: `../PLAYER_CHARACTER(5.0.0).blend`, preserved unchanged.
Generated and reopened using Blender 5.2.2 LTS.

- `Robot_Rigged_Run.blend`: 22-bone humanoid FK skeleton and `Robot_Run_InPlace` action. Open and press Space to play frames 1–24 at 30 fps. Frame 25 is the duplicate loop boundary.
- `Robot_Run_InPlace.fbx`: skinned mesh and baked running animation.
- `Robot_Rigged_BindPose.fbx`: skinned mesh and skeleton without animation. Mixamo bone naming is used, but acceptance of this custom skeleton is unverified.
- `Robot_Mixamo_APose.obj` and `.mtl`: unrigged A-pose for Mixamo's auto-rigger. Upload the OBJ, place the chin/wrist/elbow/knee/groin markers, and use the no-fingers option if offered (the robot has pointed hands). Download the resulting animated FBX. Mixamo generates a new skeleton; its downloaded animation is not guaranteed to apply directly to the custom rig.
- `Robot_Run_Preview.gif`: looping preview.
- `verification.json`: actual automated check results.

The model was normalized to approximately 2 meters tall. All 970 vertices have a single full-weight bone assignment. Original materials are retained; preview renders use a neutral studio material. The FK action uses a compact alternating stride, arm swing, knee flexion, restrained torso twist, and floor-height correction. It is an in-place first pass, without IK controls, ground-speed calibration, or foot locking. Connected geometry spanning joints can stretch and needs mesh segmentation/joint refinement for perfectly rigid mechanical articulation.

Verification: ran `inspect_robot.py` and `rig_robot.py` against the source using Blender background mode; ran `verify_robot.py` against the output blend. Weight sums passed, frames 1 and 25 matched exactly, minimum evaluated vertex height across all 24 frames was nonnegative, and animated FBX reimport recovered the armature and action. Rendered 24 preview frames and visually inspected representative run poses and the reimported OBJ A-pose. Mixamo upload and Unreal import/PIE were not performed. This is an art asset draft, with no gameplay changes.
