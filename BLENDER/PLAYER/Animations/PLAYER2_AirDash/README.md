# PLAYER2 Air Dash

`PLAYER2_AirDash.fbx` is the animation-only deliverable. `PLAYER2_AirDash.blend`
contains the original robot mesh, its original weights, and the editable clip.
Source mesh, jump and landing FBXs were not changed.

## Motion

- 0.30 seconds, 60 fps, 19 samples including both endpoints.
- Frames 1–5: fast linear transition from the exported jump's exact last pose.
- Frames 5–13: forward lean with a leading arm, a trailing arm and trailing legs.
- Frames 13–19: linear return to the exported landing's exact first pose.
- Bone rotations use shortest-path quaternion interpolation, baked per frame;
  location and scale interpolation is linear. No easing curves or instant snap.
- Root stays fixed. CharacterMovement supplies the dash's world movement.

The jump's last pose and landing's first pose are different. This uses the
user-authorized landing-pose ending to join the landing clip. It is not a loop
and should not be played as a looping dash animation. Timing matches the current
C++ AirDashDuration default (0.30s); a Blueprint override may differ.

## Unreal import

Import the FBX into `Content/RGB/Characters/PLAYER2/Animations`, selecting the
same Skeleton asset already used by PLAYER2_Jump and PLAYER2_Land. Import the
animation only; no replacement mesh is included. Keep the import scale at 1,
retain the exported 60 fps (do not force 30 fps), and leave root motion off.
Check the resulting clip length is 0.30s and inspect the jump-to-dash and
dash-to-land joins on the current PLAYER2 mesh before wiring it into the AnimBP.

No Unreal asset, AnimBP or gameplay code was changed, and Unreal import/PIE
playback has not been run. Passing the FBX check is not an Unreal playback test.

## Editing and re-export

Open the blend in Blender 5.2. Select `Bone_007` and edit its `PLAYER2_AirDash`
action. The hidden source display rig follows it to show changes on the mesh.
The 119 child bones plus the Bone_007 root preserve the supplied UE hierarchy.
The separate source action is retained as an authoring reference; edit the
export action for changes to the delivered clip. Rigid attachment bone edits
are exportable, but the display meshes primarily follow their original parent
bones; inspect such changes after Unreal import.

Frame 0 is a rest reference outside the animation range; preserve it. Save the
blend, then run the accompanying `export_airdash.py` with Blender to refresh
the FBX. It reads the blend and does not overwrite it:

```powershell
& 'C:\Program Files\Blender Foundation\Blender 5.2\blender.exe' --background --python-exit-code 1 --python 'BLENDER/PLAYER/Animations/PLAYER2_AirDash/export_airdash.py'
```

## Verification

The FBX was reimported in Blender and all 19 sampled poses checked against the
intended world transforms. Bone names and parent relationships match the supplied
UE jump export; root is fixed; duration is 0.30s. Maximum measured positional
round-trip error was below 0.002cm; quaternion rotation differences were below
the check's 0.15-degree tolerance. See `validation.json` for actual results.
Blender's automatic connected-bone flags were disabled for validation because
they suppress translation channels; Unreal bones have no equivalent flag.

The build and validation scripts are in the neighboring `Dash_Work` folder.
The creator rebuilds from source and will replace local edits to this clip;
use `export_airdash.py` to export manual edits instead. Revalidate after edits.
