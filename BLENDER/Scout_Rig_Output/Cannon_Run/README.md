# Scout cannon run

Open `Scout_Run_Cannon_Right.blend` and press Space to play. The action is `Scout_Run_Cannon_Right`: an in-place 24-frame cycle at 30 fps (0.8 seconds). Frame 25 duplicates frame 1 as the loop boundary, outside the playback range. The model faces Blender -Y. The robot's right arm points forward horizontally, with the left arm swinging. No cannon mesh has been added.

`Scout_Run_Cannon_Right.fbx` contains the skinned Scout and baked animation. No Mixamo processing is needed. `Scout_Cannon_Run.gif` previews the animation. Existing source and rig files were preserved.

The animation uses solved leg poses, a linear backward foot motion during contact, elevated foot recovery, slight hip bounce, and forward body lean. All transforms are baked; no runtime constraints or external plugins are required. This is a first-pass stylized animation; existing connected joint geometry can stretch at deep bends. Root motion is not included. Suggested starting ground speed for the in-place stride is approximately 1.2 Blender meters/second, to be tuned visually in the game.

Verification command: Blender 5.2.2 background mode opened `BLENDER/Scout_Rig_Output/Scout_Rigged.blend` and ran `BLENDER/animate_scout_cannon.py`. Verified matching evaluated vertices at frames 1 and 25, forward cannon direction throughout all baked frames, floor clearance within a 5 mm tolerance, and successful animated FBX reimport. Rendered all 24 frames and inspected three-quarter and side-view poses. Actual numeric results are in `verification.json`. Unreal import and PIE were not run; no gameplay code was changed.
