"""Capture clean editor views; never changes a level asset."""
import unreal as u
import os
import time
preview_levels = u.get_editor_subsystem(u.LevelEditorSubsystem)
assert not preview_levels.is_in_play_in_editor()
preview_levels.load_level("/Game/RGB/Maps/L_ChromaticFoundry")
preview_levels.editor_set_game_view(True)
preview_views = [(4550, 4000, 1800, "CrimsonPress"), (14000, 4300, 2900, "VerdantRelay"), (23900, 4800, 4000, "AzureCore")]
preview_state = {"index": 0, "next": time.monotonic() + 2, "capture": False, "busy": False}
preview_output = os.path.join(u.Paths.project_dir(), "Saved/LevelDesign")

def preview_tick(dt):
    if preview_state["busy"] or time.monotonic() < preview_state["next"]:
        return
    if preview_state["index"] >= len(preview_views):
        u.EditorLevelLibrary.set_level_viewport_camera_info(u.Vector(4550, 4000, 1800), u.Rotator(pitch=-5, yaw=-90, roll=0))
        u.unregister_slate_post_tick_callback(preview_handle)
        return
    x, y, z, name = preview_views[preview_state["index"]]
    if not preview_state["capture"]:
        u.EditorLevelLibrary.set_level_viewport_camera_info(u.Vector(x, y, z), u.Rotator(pitch=-5, yaw=-90, roll=0))
        preview_state.update(capture=True, next=time.monotonic() + 2)
    else:
        preview_state["busy"] = True
        try:
            u.AutomationLibrary.take_high_res_screenshot(1920, 1080, os.path.join(preview_output, "Foundry_" + name + ".png"))
            preview_state.update(index=preview_state["index"] + 1, capture=False, next=time.monotonic() + 3)
        finally:
            preview_state["busy"] = False

preview_handle = u.register_slate_post_tick_callback(preview_tick)
