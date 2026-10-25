"""Read-only audit of the Level 3 reference maps (L_TWO_2, L_ChromaticFoundry, L_INTRO).

Loads each map, records actors and their gameplay settings, and verifies on disk
that no map file changed. Nothing is saved. Output:
Saved/LevelDesign/level3_reference_audit.json

Run with Unreal closed:
UnrealEditor-Cmd.exe <abs>/RGB.uproject -run=pythonscript -script=<abs>/Scripts/Editor/inspect_level3_references.py
  -EnablePlugins=PythonScriptPlugin,EditorScriptingUtilities -unattended -nullrhi -nosound -nosplash
"""
import unreal
import json
import os
import hashlib

ROOT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
OUT = os.path.join(ROOT, "Saved", "LevelDesign")
os.makedirs(OUT, exist_ok=True)
MAPS = ("L_TWO_2", "L_ChromaticFoundry", "L_INTRO")

actors_api = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)

ACTOR_KEYS = (
    "required_color", "assigned_platforms", "assigned_enemies", "arena_camera_zone",
    "teleport_target_points", "teleport_target_tag", "granted_ability", "destination_level",
    "fade_out_duration", "black_hold_duration", "launch_speed", "movement_path",
    "enable_arena_ai_tuning", "arena_ai_tuning", "watched_arena", "target_actors",
    "red_material", "green_material", "blue_material", "camera_mode", "initial_color",
)
COMP_KEYS = (
    "static_mesh", "skeletal_mesh_asset", "relative_location", "relative_rotation",
    "relative_scale3d", "box_extent", "sphere_radius", "initial_color", "randomize_initial_color",
    "intensity", "light_color", "field_of_view", "movement_speed", "movement_mode",
    "start_active", "text", "world_size", "camera_mode",
)


def val(v):
    if isinstance(v, unreal.Object):
        return v.get_path_name()
    if isinstance(v, (list, tuple, unreal.Array)):
        return [val(x) for x in v]
    return str(v)


def props(obj, keys):
    result = {}
    for key in keys:
        try:
            result[key] = val(obj.get_editor_property(key))
        except Exception:
            pass
    return result


def vec(v):
    return [round(v.x, 1), round(v.y, 1), round(v.z, 1)]


def file_hash(path):
    with open(path, "rb") as f:
        return hashlib.sha256(f.read()).hexdigest()


report = {"maps": {}}
for name in MAPS:
    disk = os.path.join(ROOT, "Content", "RGB", "Maps", name + ".umap")
    if not os.path.exists(disk):
        report["maps"][name] = {"missing": True}
        continue
    before = file_hash(disk)
    levels.load_level("/Game/RGB/Maps/" + name)
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    ws = world.get_world_settings()
    info = {"hash": before, "world_settings": props(ws, ("default_game_mode", "kill_z")), "actors": []}
    for actor in actors_api.get_all_level_actors():
        origin, extent = actor.get_actor_bounds(False)
        row = {
            "label": actor.get_actor_label(),
            "name": actor.get_name(),
            "class": actor.get_class().get_path_name(),
            "folder": str(actor.get_folder_path()),
            "location": vec(actor.get_actor_location()),
            "rotation": str(actor.get_actor_rotation()),
            "scale": vec(actor.get_actor_scale3d()),
            "bounds_origin": vec(origin),
            "bounds_extent": vec(extent),
            "tags": [str(t) for t in actor.tags],
            "properties": props(actor, ACTOR_KEYS),
            "components": [],
        }
        for comp in actor.get_components_by_class(unreal.ActorComponent):
            cp = props(comp, COMP_KEYS)
            if isinstance(comp, unreal.MeshComponent):
                try:
                    cp["materials"] = [val(m) for m in comp.get_materials()]
                except Exception:
                    pass
            if cp:
                row["components"].append({"name": comp.get_name(), "class": comp.get_class().get_name(), "properties": cp})
        info["actors"].append(row)
    info["unchanged"] = before == file_hash(disk)
    report["maps"][name] = info

with open(os.path.join(OUT, "level3_reference_audit.json"), "w", encoding="utf-8") as f:
    json.dump(report, f, indent=1)
unreal.log("RGB_LEVEL3_AUDIT_COMPLETE " + OUT)
