# Scout long-stride cannon run

Revision based on the user's reference C: extended forward reach, large front/back thigh separation, rear heel recovery, and reduced high-knee marching. Explicit thigh and knee pose timing replaces the previous foot-trajectory solver. The right forearm stays aimed forward. Previous versions are preserved.

Open `Scout_Run_Cannon_Right.blend` and press Space. Action: `Scout_Run_Cannon_Right_Long_Stride`. The in-place cycle is 24 frames at 36 fps; frame 25 duplicates frame 1 outside the playback range. `Scout_Run_Cannon_Right.fbx` contains mesh, rig, and baked animation. `Scout_Long_Stride_Side.gif` is the side-view preview; `Scout_Long_Stride.gif` is the three-quarter preview.

Verification: Blender 5.2.2 background execution of `BLENDER/animate_scout_long_stride.py` on the saved Scout rig. Exact evaluated-mesh loop closure, forward cannon aim, nonnegative floor clearance, and successful animated FBX reimport checked. Rendered 24 frames from each of two angles and inspected extended/passing side silhouettes. Numeric results are in `verification.json`. This is a stylized FK animation without contact locking or root motion; ground-speed tuning and Unreal verification remain pending. Deep joint bends retain the existing mesh's deformation limitations.
