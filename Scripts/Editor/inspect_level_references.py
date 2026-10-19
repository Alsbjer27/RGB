"""Read reference maps without saving them; export authoring data for the new level."""
import unreal
import json
import os
import hashlib

ROOT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
OUT = os.path.join(ROOT, "Saved", "LevelDesign")
os.makedirs(OUT, exist_ok=True)
report = {"maps": {}, "assets": {}}
actors_api = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)

def val(v):
    if isinstance(v, unreal.Object):
        return v.get_path_name()
    return str(v)

def props(obj, keys):
    result = {}
    for key in keys:
        try:
            result[key] = val(obj.get_editor_property(key))
        except Exception:
            pass
    return result

for name in ("L_INTRO", "L_MapLayout"):
    disk = os.path.join(ROOT, "Content", "RGB", "Maps", name + ".umap")
    before = hashlib.sha256(open(disk, "rb").read()).hexdigest()
    levels.load_level("/Game/RGB/Maps/" + name)
    rows = []
    for actor in actors_api.get_all_level_actors():
        row = {"label": actor.get_actor_label(), "class": actor.get_class().get_path_name(), "location": str(actor.get_actor_location()), "scale": str(actor.get_actor_scale3d()), "bounds": str(actor.get_actor_bounds(False))}
        row["properties"] = props(actor, ("required_color", "assigned_platforms", "assigned_enemies", "arena_camera_zone", "granted_ability", "teleport_target_tag", "teleport_target_points", "default_game_mode", "interaction_mode", "movement_path", "red_material", "green_material", "blue_material"))
        row["components"] = []
        for comp in actor.get_components_by_class(unreal.ActorComponent):
            cp = props(comp, ("static_mesh", "skeletal_mesh_asset", "relative_location", "relative_rotation", "relative_scale3d", "box_extent", "initial_color", "randomize_initial_color", "intensity", "auto_exposure_bias", "field_of_view", "movement_speed", "movement_mode", "start_active"))
            if cp:
                row["components"].append({"name": comp.get_name(), "class": comp.get_class().get_name(), "properties": cp})
        rows.append(row)
    report["maps"][name] = {"hash": before, "unchanged": before == hashlib.sha256(open(disk, "rb").read()).hexdigest(), "actors": rows}

registry = unreal.AssetRegistryHelpers.get_asset_registry()
for folder in ("/Game/RGB/Blueprints", "/Game/RGB/Assets", "/Game/Grid_Materials/Materials/Instances"):
    report["assets"][folder] = [{"path": str(a.package_name), "class": str(a.asset_class_path.asset_name)} for a in registry.get_assets_by_path(folder, recursive=True)]
with open(os.path.join(OUT, "reference_audit.json"), "w", encoding="utf-8") as f:
    json.dump(report, f, indent=2)
unreal.log("RGB_LEVEL_AUDIT_COMPLETE " + OUT)
