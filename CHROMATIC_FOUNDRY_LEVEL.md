# Chromatic Foundry

Created 2026-10-17, with final verification on 2026-10-18, as a separate, playable level design prototype using the existing RGB game systems.

## Open and play

1. In Unreal's Content Browser, open `Content/RGB/Maps/L_ChromaticFoundry`.
2. Use Play from the normal Player Start, rather than Play From Here.
3. Walk right through the four supply pickups to obtain dash and all three weapons.
4. Solve the arenas in red, green, blue order. Match every numbered puzzle platform to the room's colour. Landings cycle colours; matching shots defeat enemies. Ground enemy deaths can transfer their colour to the platform below, according to the existing game rules.
5. Jump onto the round launch pads to reach high routes or recover from the lower deck. Steer left/right in the air; ordinary jumps still work for colour correction and short gaps.
6. At 100%, the room's shutter rises. Follow the amber path to the next room.
7. Beyond the blue room, the final portal fades to the existing main menu.

No C++ rebuild is needed for these new level/material assets. The project's current RGBEditor binaries and existing Blueprint asset configuration are used.

## Route and encounter design

| Area | Purpose | Content |
| --- | --- | --- |
| Arrival | Establish direction and equip the player | Supply pickups, movement/combat reminder, checkpoint |
| 01 Crimson Press | Introduce colour correction under ground pressure | Five pads at two heights, two ground enemies; make all pads red |
| Transfer Gantry | A quieter movement section between fights | Short 140 cm steps, a launch to the next entrance, amber edge lights, checkpoint |
| 02 Verdant Relay | Make the player revisit different elevations while teleporters reposition | Six pads at three heights, two teleporters, one flying enemy; make all pads green |
| Induction Gap | Change the rhythm with a moving route | A diagonal shuttle, an optional launch from the entry deck, a lower island, landing deck and checkpoint |
| 03 Azure Core | Combine the previous pressures in a taller room | Seven pads, two ground enemies, two teleporters, one flying enemy; make all pads blue |
| Departure | Resolve the level and offer a clear exit | Completion sign, checkpoint, return-to-menu portal |

The level has 18 puzzle platforms, ten enemies, seven checkpoints, 19 boost/recovery pads, three completion-controlled shutters, and separate cameras for rooms and connectors. Each arena has a neutral lower deck with recovery launch pads so a missed combat jump does not immediately mean death. The outer gaps use the game's existing checkpoint respawn behavior.

The player's actual Blueprint jump speed is 1120 cm/s with gravity scale 3, giving roughly 213 cm of unassisted rise. The level does not change this. Short stairs use 140 cm rises; higher arena routes use individually tuned C++ jump pads. Pads are offset from the centres of puzzle platforms so landing to change a colour does not require landing on a launch pad. The finale's highest platform is offset horizontally to give the launch a clear approach around its underside.

The arena layouts take their floating platforms, scale, layered combat and movement breaks from `L_INTRO` and `L_MapLayout`. They use the existing platform meshes, enemy Blueprints, pickup Blueprints, game mode and HUD. Eight new materials, dark industrial backplates, structural ribs, colour-coded lighting strips and distant towers give this separate level its visual identity. This is an authored gameplay prototype, with simple environment geometry rather than finished environment art.

## Gameplay connections

- Each arena control owns only its own puzzle pads and enemies, its own camera zone, and its own explicit teleport target points. Green has 12 destinations and blue has 14. These authored points do not require additional tags while arena AI tuning is enabled.
- Arena AI tuning is enabled on all three controls. The initial attack budget is one enemy, rising to two in red and three in green/blue as completion increases. Existing ground, teleporter and flying profiles provide the progress responses.
- Eliminated enemies respawn after 12 seconds at zero progress, decreasing to seven seconds in red and five seconds in green/blue near completion. Arena completion uses the existing enemy shutdown behavior.
- Exits use native `RGBMovingPlatform` actors following native spline paths in One Way mode. Native `RGBArenaCompletionActivator` actors enable them at completion. They are stationary at startup and open once; there is no gameplay logic in a Level Blueprint.
- The physical shutters are tall enough to prevent a normal jump over an unfinished room. Existing arena reset events return them to their starting positions.
- All gameplay remains in the existing C++ systems. New Blueprints and new gameplay classes were unnecessary.

## Recovery and reset behavior

This map preserves the current game's behavior. Falling into a checkpoint's recovery trigger respawns the player at that section. Lethal health damage calls the existing global mechanics reset: it respawns at the active checkpoint and resets arena colours, enemies and shutters throughout the map. Rooms behind the checkpoint can therefore reset too. This was checked in PIE; this level task did not change that rule. Reload the map to begin a completely fresh run from Arrival.

## Editing

The Outliner folders separate `00_Environment`, `01_Cameras`, `02_Arrival`, the three numbered arenas, the two connecting routes and `40_Departure`. Arena subfolders contain Puzzle, Enemies, TeleportPoints, Rules, Exit and Recovery actors.

Select an `A*_Control_*` actor to adjust its target colour, enemy list, teleport destinations or progress-based pressure. Select a numbered `A*_Puzzle_*` platform's ColorComponent to change its authored initial colour. The new environmental materials are under `Content/RGB/Levels/ChromaticFoundry/Materials`.

The creation script refuses to overwrite an existing map. Treat the saved `.umap` as the editable level; do not remove it to rerun the generator after hand-editing it. `refine_chromatic_foundry.py` is the authoring correction script used during creation and deliberately restores specific authored placements; do not run it over later hand edits. The separate validation and PIE check scripts do not save gameplay changes to the map.

## Verification record

- Unreal 5.7.1 Python commandlet created and saved the map: exit code 0, zero reported errors/warnings in the final creation run.
- Read-only saved-asset validation: 78 checks passed on the revised map. This covers arena lists, ownership, camera directions, checkpoint/pickup/boost-pad counts, teleport lists, shutter paths/activation, exit destination and dynamic light mobility. Command: `UnrealEditor-Cmd.exe RGB.uproject -run=pythonscript -script=Scripts/Editor/validate_chromatic_foundry.py -EnablePlugins=PythonScriptPlugin,EditorScriptingUtilities -unattended -nullrhi -nosound -nosplash` (run with absolute paths).
- Actual Play In Editor systems check: 47 checks passed. The player spawned, crossed the pickup triggers, enemies initially idled and activated on entry, both teleporter groups changed destinations, all 18 pads were solved through real CharacterMovement landing callbacks, all three arenas completed and their shutters opened, and lethal damage respawned at the last arena checkpoint with the existing global puzzle reset.
- During deterministic colour/exit checks, enemy damage was temporarily suppressed and enemies were hidden in the PIE copy after activation was observed. These test-only changes were never saved to the level. This is not a claim of a full combat playthrough.
- The initial black-screen camera rotation issue was corrected in the new map and verified visually in PIE. Dynamic lights avoid a lighting-bake requirement.
- Unreal's final map check reported zero errors and zero warnings. A clean editor preview of Crimson Press is saved as `Saved/LevelDesign/Foundry_CrimsonPress.png`.
- Actual PIE movement probes: 15 passed using the player's real 1120 cm/s normal jump, air steering, and the real C++ launch-pad landing behavior. These covered the short gantry jumps, the launch into each next arena, recovery from the first arena's lower deck, and the launch routes to each room's upper/apex platforms. Probe starts were positioned individually; this verifies reachability, not an uninterrupted human run. The second connector's launch begins on the entry deck to avoid striking the shuttle from below.
- The original `L_INTRO` and `L_MapLayout` map files retain their exact pre-task SHA-256 hashes. No existing map, shared Blueprint, shared material, startup map, input configuration or C++ gameplay file was edited for this level.
- Scripts and generated reports: authoring/check helpers are in `Scripts/Editor`; transient logs, JSON reports and preview captures are in `Saved/LevelDesign` and `Saved/Logs` and should not be committed.
- Remaining human assessment: a complete combat run, difficulty tuning, natural controller/keyboard traversal feel, shuttle riding, and the final menu transition/fade. This prototype is ready for that playtest, not marked as a production-ready stage.

## HUD progress follow-up (2026-10-18)

The prototype HUD originally selected a single arena control with GetActorOfClass
at construction. That was why the second and third rooms did not update its bar.
The shared WBP_RGBHUD_Prototype now derives from URGBHUDWidget. Its native arena
presenter uses the owning pawn's current camera-zone bounds and reads progress
from that room's control. It shows zero between rooms and switches immediately
to a room's stored progress on entry; progress changes within a room retain a
smooth fill animation. Existing health visuals and layout remain in Blueprint.

The fix changes the shared HUD asset, its native parent and a read-only arena
camera-zone getter. It does not alter this map or either reference map. The
migration uses RGBUpgradeHUDCommandlet and Unreal's Blueprint/asset APIs; a
pre-migration backup is in Saved/LevelDesign for local recovery only. The new
PIE regression helper is Scripts/Editor/check_foundry_hud.py. It temporarily
suppresses enemy damage and boost-pad collision only in PIE to isolate repeated
landings and HUD material checks. It saves no level changes and restores the
editor's background CPU throttle preference after checking.

HUD fix verification: RGBEditor Win64 Development built successfully using
UE 5.7.1 Build.bat with -WaitMutex -NoUBA -NoHotReloadFromIDE. The HUD asset
migration commandlet completed with zero errors and warnings. The final actual
PIE run completed all 217 assertions in check_foundry_hud.py: correct HUD arena
and liquid material fill during real platform landings in all three rooms,
full completion, zero on departure, completed-room reentry, one HUD after lethal
respawn, and correct progress after the global arena reset. Results are in
Saved/LevelDesign/foundry_hud_pie.json; log is Saved/Logs/FoundryHUDFinal.log.
The character creates a replacement HUD on respawn; its native parent now removes
older viewport HUDs for that same player to prevent stacked displays.
