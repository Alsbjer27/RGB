"""Capture editor views of L_THREE for review. Run INSIDE the open editor:
Tools > Execute Python Script... > Scripts/Editor/preview_level_three.py
Never saves anything. Writes Saved/LevelDesign/L3_<Section>.png (takes about 40 seconds)."""
import unreal as u
import os
import time

l3_levels = u.get_editor_subsystem(u.LevelEditorSubsystem)
assert not l3_levels.is_in_play_in_editor(), "Stop Play-in-Editor first"
l3_levels.load_level("/Game/RGB/Maps/L_THREE")
l3_levels.editor_set_game_view(True)
l3_views = [
    (-200, 4000, 700, "1_Arrival"),
    (4300, 4300, 500, "2_StampingLine"),
    (9900, 6400, 1800, "3_AssemblyLine"),
    (14900, 4600, 1700, "4_TheClimb"),
    (18400, 4300, 2600, "5_ChromaCore"),
    (23200, 6400, 3100, "6_Calibration"),
    (28300, 4600, 2600, "7_Paternoster"),
    (33900, 6800, 4300, "8_FinalInspection"),
    (39000, 4300, 3400, "9_Departure"),
    (19000, 26000, 2500, "0_Overview"),
]
l3_state = {"index": 0, "next": time.monotonic() + 2, "capture": False}
l3_out = os.path.join(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()), "Saved/LevelDesign")


def l3_tick(dt):
    if time.monotonic() < l3_state["next"]:
        return
    if l3_state["index"] >= len(l3_views):
        u.unregister_slate_post_tick_callback(l3_handle)
        u.log("RGB_L3_PREVIEW_DONE " + l3_out)
        return
    x, y, z, name = l3_views[l3_state["index"]]
    if not l3_state["capture"]:
        u.EditorLevelLibrary.set_level_viewport_camera_info(u.Vector(x, y, z), u.Rotator(pitch=-5, yaw=-90, roll=0))
        l3_state.update(capture=True, next=time.monotonic() + 2.5)
    else:
        u.AutomationLibrary.take_high_res_screenshot(1600, 900, os.path.join(l3_out, "L3_" + name + ".png"))
        l3_state.update(index=l3_state["index"] + 1, capture=False, next=time.monotonic() + 2.5)


l3_handle = u.register_slate_post_tick_callback(l3_tick)
