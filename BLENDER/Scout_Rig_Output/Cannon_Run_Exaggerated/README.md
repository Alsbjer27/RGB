# Exaggerated Scout cannon run

Open `Scout_Run_Cannon_Right.blend` and press Space. Action: `Scout_Run_Cannon_Right_Exaggerated`. Animated export: `Scout_Run_Cannon_Right.fbx`. Previous animation files remain unchanged.

The in-place cycle now plays 24 frames at 36 fps (two-thirds of a second). Changes include a wider stride, higher recovery knee, shorter ground-contact phase with flight between steps, deeper body compression and bounce, stronger forward lean, torso twist and side tilt, and a larger free-arm swing with varying elbow bend. The right upper arm responds to the bounce while its forearm and hand remain aimed forward. No mesh changes or root motion.

Preview: `Scout_Exaggerated_Run.gif`; side view: `Scout_Exaggerated_Side.gif`. Frame 25 duplicates frame 1 and is outside playback range.

Verification: ran Blender 5.2.2 in background on `Scout_Rigged.blend` with `BLENDER/animate_scout_cannon_exaggerated.py`. Exact evaluated mesh match across the loop boundary; forward cannon aim error below 0.000001; minimum mesh height approximately 0.001 m; animated FBX reimport passed. Rendered all frames from two angles and inspected representative poses. Results are recorded in `verification.json`. Connected armor geometry can still stretch at deep joint bends. Unreal import/PIE not tested.
