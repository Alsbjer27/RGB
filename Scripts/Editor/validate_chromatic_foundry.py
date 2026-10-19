"""Read-only authoring checks. Run in an Unreal Python commandlet."""
import unreal as u
import os
import json

root = u.Paths.convert_relative_path_to_full(u.Paths.project_dir())
u.get_editor_subsystem(u.LevelEditorSubsystem).load_level("/Game/RGB/Maps/L_ChromaticFoundry")
actors = u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors()
by_label = {a.get_actor_label(): a for a in actors}
result = {"checks": [], "api_docs": {}}
player_defaults = u.get_default_object(u.load_class(None, "/Game/RGB/Blueprints/Player/BP_RGBPlayerCharacter.BP_RGBPlayerCharacter_C"))
movement = player_defaults.get_component_by_class(u.CharacterMovementComponent)
result["player_movement"] = {key: str(movement.get_editor_property(key)) for key in ("jump_z_velocity", "gravity_scale", "max_walk_speed", "max_acceleration", "air_control")}

def check(condition, text):
    result["checks"].append({"passed": bool(condition), "description": text})
    assert condition, text

controls = [a for a in actors if isinstance(a, u.RGBArenaControl)]
check(len(controls) == 3, "Three arena controls")
assigned = set()
for i, expected in ((1, 5), (2, 6), (3, 7)):
    control = by_label["A%d_Control_%s" % (i, ("RED", "GREEN", "BLUE")[i - 1])]
    pads = control.get_editor_property("assigned_platforms")
    enemies = control.get_editor_property("assigned_enemies")
    zone = control.get_editor_property("arena_camera_zone")
    check(len(pads) == expected, "Arena %d has %d puzzle pads" % (i, expected))
    check(zone is not None and control.get_editor_property("use_arena_enemy_tuning"), "Arena %d camera and AI profile connected" % i)
    for enemy in enemies:
        check(enemy not in assigned, enemy.get_actor_label() + " belongs to one arena")
        assigned.add(enemy)
        pos = enemy.get_actor_location()
        bounds = zone.get_component_by_class(u.BoxComponent)
        origin = bounds.get_world_location()
        extent = bounds.get_scaled_box_extent()
        check(abs(pos.x - origin.x) < extent.x and abs(pos.z - origin.z) < extent.z, enemy.get_actor_label() + " starts inside its camera zone")
    for point in control.get_editor_property("teleport_target_points"):
        check(point.get_actor_label().startswith("A%d_" % i), "Teleport point belongs to arena %d" % i)
    shutter = by_label["A%d_CompletionShutter" % i]
    move = next(c for c in shutter.get_components_by_class(u.ActorComponent) if c.get_name() == "PathMovement")
    check(move.get_editor_property("movement_path") is not None, "Arena %d shutter path saved" % i)
    check(move.get_editor_property("movement_mode") == u.RGBPathMovementMode.ONE_WAY and not move.get_editor_property("start_active"), "Arena %d shutter opens once, after completion" % i)
    opener = by_label["A%d_OpenExitOnCompletion" % i]
    check(opener.get_editor_property("arena_control") == control and shutter in opener.get_editor_property("controlled_actors"), "Arena %d completion opens its own exit" % i)
check(len(assigned) == 10, "All ten enemies assigned exactly once")
check(len([a for a in actors if isinstance(a, u.PlayerStart)]) == 1, "One player start")
check(len([a for a in actors if isinstance(a, u.RGBCheckpoint)]) == 7, "Seven checkpoints")
check(len([a for a in actors if isinstance(a, u.RGBAbilityPickup)]) == 4, "All four abilities supplied")
boosts = [a for a in actors if isinstance(a, u.RGBJumpPad)]
check(len(boosts) == 19, "Nineteen authored boost/recovery pads support the actual Blueprint jump")
check(all(a.get_editor_property("launch_speed") > 0 for a in boosts), "All boost pads have a positive launch speed")
check(by_label["Departure_ReturnToMenu"].get_editor_property("destination_level") is not None, "Final exit has a destination")
for actor in actors:
    if isinstance(actor, u.RGBArenaCameraZone):
        forward = actor.get_component_by_class(u.CameraComponent).get_forward_vector()
        check(forward.y < -0.99 and abs(forward.z) < 0.01, actor.get_actor_label() + " faces the X/Z play plane")
    if isinstance(actor, (u.DirectionalLight, u.SkyLight)):
        check(actor.light_component.get_editor_property("mobility") == u.ComponentMobility.MOVABLE, actor.get_actor_label() + " uses dynamic lighting without a bake")
for obj, method in ((u.AutomationLibrary, "take_high_res_screenshot"), (u.EditorLevelLibrary, "get_pie_worlds"), (u.LevelEditorSubsystem, "editor_request_begin_play"), (u.Actor, "set_actor_location")):
    result["api_docs"][method] = getattr(obj, method).__doc__
out = os.path.join(root, "Saved/LevelDesign/chromatic_foundry_validation.json")
with open(out, "w", encoding="utf8") as f:
    json.dump(result, f, indent=2)
u.log("RGB_FOUNDRY_STATIC_VALIDATION_PASSED " + str(len(result["checks"])))
