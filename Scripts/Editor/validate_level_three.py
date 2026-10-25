"""Read-only checks of the saved L_THREE map. Never saves. Run in an Unreal Python commandlet.
Writes Saved/LevelDesign/level_three_validation.json and keeps going after a failed check,
so one run reports every problem."""
import unreal as u
import os
import json

root = u.Paths.convert_relative_path_to_full(u.Paths.project_dir())
u.get_editor_subsystem(u.LevelEditorSubsystem).load_level("/Game/RGB/Maps/L_THREE")
all_actors = u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors()
by_label = {a.get_actor_label(): a for a in all_actors}
result = {"checks": [], "failed": 0}


def check(condition, text):
    result["checks"].append({"passed": bool(condition), "description": text})
    if not condition:
        result["failed"] += 1


def comp(actor, name):
    return next((c for c in actor.get_components_by_class(u.ActorComponent) if c.get_name() == name), None)


def colour(actor):
    return comp(actor, "ColorComponent").get_editor_property("initial_color")


red, green, blue = u.RGBColor.RED, u.RGBColor.GREEN, u.RGBColor.BLUE
controls = {a.get_actor_label(): a for a in all_actors if isinstance(a, u.RGBArenaControl)}
check(sorted(controls) == ["A1_Control_RED", "A2_Control_BLUE", "A3_Control_GREEN"], "Three arena controls: red, blue, green")
assigned = set()
expect = {1: ("A1_Control_RED", red, 5, 3), 2: ("A2_Control_BLUE", blue, 6, 3), 3: ("A3_Control_GREEN", green, 7, 5)}
for i, (label, target, pads_n, enemies_n) in expect.items():
    control = controls.get(label)
    if not control:
        continue
    pads = list(control.get_editor_property("assigned_platforms"))
    enemies = list(control.get_editor_property("assigned_enemies"))
    zone = control.get_editor_property("arena_camera_zone")
    check(control.get_editor_property("required_color") == target, "Arena %d target colour" % i)
    check(len(pads) == pads_n and all(p is not None for p in pads), "Arena %d has %d puzzle pads" % (i, pads_n))
    check(len(enemies) == enemies_n and all(e is not None for e in enemies), "Arena %d has %d enemies" % (i, enemies_n))
    check(zone is not None and control.get_editor_property("use_arena_enemy_tuning"), "Arena %d camera and AI tuning connected" % i)
    check(not all(colour(p) == target for p in pads), "Arena %d does not start already solved" % i)
    if zone:
        box = zone.get_component_by_class(u.BoxComponent)
        o, e = box.get_world_location(), box.get_scaled_box_extent()
        for en in enemies:
            p = en.get_actor_location()
            check(abs(p.x - o.x) < e.x and abs(p.z - o.z) < e.z, en.get_actor_label() + " starts inside its arena camera zone")
        for pad in pads:
            p = pad.get_actor_location()
            check(abs(p.x - o.x) < e.x, pad.get_actor_label() + " lies inside its arena")
    for en in enemies:
        check(en not in assigned, en.get_actor_label() + " belongs to exactly one arena")
        assigned.add(en)
    teleporters = [en for en in enemies if isinstance(en, u.RGBTeleporterEnemyCharacter)]
    points = list(control.get_editor_property("teleport_target_points"))
    check(not teleporters or len(points) >= 4, "Arena %d teleporters have destinations" % i)
    check(all(pt.get_actor_label().startswith("A%d_" % i) for pt in points), "Arena %d teleport points are its own" % i)
    shutter = by_label.get("A%d_CompletionShutter" % i)
    opener = by_label.get("A%d_OpenExitOnCompletion" % i)
    check(shutter is not None and opener is not None, "Arena %d has a shutter and completion opener" % i)
    if shutter and opener:
        move = comp(shutter, "PathMovement")
        check(move.get_editor_property("movement_path") is not None and not move.get_editor_property("start_active")
              and move.get_editor_property("movement_mode") == u.RGBPathMovementMode.ONE_WAY, "Arena %d shutter opens once, only on completion" % i)
        check(opener.get_editor_property("arena_control") == control and shutter in list(opener.get_editor_property("controlled_actors")),
              "Arena %d completion opens its own shutter" % i)

# The blue story beat.
a1 = [colour(e) for e in controls["A1_Control_RED"].get_editor_property("assigned_enemies")] if "A1_Control_RED" in controls else []
a2 = [colour(e) for e in controls["A2_Control_BLUE"].get_editor_property("assigned_enemies")] if "A2_Control_BLUE" in controls else []
a3 = [colour(e) for e in controls["A3_Control_GREEN"].get_editor_property("assigned_enemies")] if "A3_Control_GREEN" in controls else []
check(a1 and blue not in a1, "Arena 1 (before blue) has no blue enemies")
check(a2 and all(c == blue for c in a2), "Arena 2 (after blue) has only blue enemies")
check(a3 and {red, green, blue} <= set(a3), "Arena 3 uses enemies of all three colours")
pickups = [a for a in all_actors if isinstance(a, u.RGBAbilityPickup)]
abilities = sorted(str(p.get_editor_property("granted_ability")) for p in pickups)
check(len(pickups) == 4, "Four pickups: backup dash/red/green and the blue weapon (" + ", ".join(abilities) + ")")
blue_pickup = by_label.get("Core_BlueWeapon")
check(blue_pickup is not None and blue_pickup.get_editor_property("granted_ability") == u.RGBPlayerAbility.BLUE_WEAPON, "Blue weapon pickup in the Chroma Core")
if blue_pickup and "A1_Control_RED" in controls and "A2_Control_BLUE" in controls:
    bx = blue_pickup.get_actor_location().x
    check(by_label["A1_CompletionShutter"].get_actor_location().x < bx < by_label["A2_Puzzle_01"].get_actor_location().x,
          "Blue is collected after arena 1 and before arena 2")

# Progression furniture.
check(len([a for a in all_actors if isinstance(a, u.PlayerStart)]) == 1, "One player start")
checkpoints = [a for a in all_actors if isinstance(a, u.RGBCheckpoint)]
check(len(checkpoints) >= 9, "%d checkpoints" % len(checkpoints))
pads = [a for a in all_actors if isinstance(a, u.RGBJumpPad)]
check(len(pads) >= 20 and all(p.get_editor_property("launch_speed") >= 2200 for p in pads), "%d jump pads with real launch speeds" % len(pads))
portal = by_label.get("Departure_ReturnToMenu")
check(portal is not None and portal.get_editor_property("destination_level") is not None, "Final exit has a destination")
gm = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world().get_world_settings().get_editor_property("default_game_mode")
check(gm is not None and "BP_RGBGameMode" in gm.get_path_name(), "World uses BP_RGBGameMode")

# Moving and rotating machinery.
movers = [a for a in all_actors if isinstance(a, u.RGBMovingPlatform)]
no_path = [m.get_actor_label() for m in movers if comp(m, "PathMovement").get_editor_property("movement_path") is None]
check(not no_path, "Every moving platform has a path " + str(no_path[:5]))
flips = [m for m in movers if "FlipPlate" in m.get_actor_label()]
check(len(flips) == 5 and all(comp(m, "RotationMovement").get_editor_property("start_active")
                              and comp(m, "RotationMovement").get_editor_property("rotation_mode") == u.RGBRotationMode.STEPPED for m in flips),
      "Five stepped flip plates are active")
cars = [m for m in movers if "WheelCar" in m.get_actor_label()]
check(len(cars) == 4 and all(comp(m, "PathMovement").get_editor_property("movement_mode") == u.RGBPathMovementMode.LOOP for m in cars),
      "Paternoster has four looping cars")
check(len({round(m.get_editor_property("starting_path_progress"), 2) for m in cars}) == 4, "Paternoster cars are evenly spaced")
for label in ("R1_Shuttle", "R2_Lift", "A1_Shuttle", "A2_Shuttle"):
    m = by_label.get(label)
    check(m is not None and comp(m, "PathMovement").get_editor_property("start_active"), label + " moves from the start")
robots = [a for a in all_actors if isinstance(a, u.SkeletalMeshActor)]
check(robots and all(r.get_attach_parent_actor() is not None for r in robots), "%d decorative robots ride the machinery" % len(robots))
for a in all_actors:
    if isinstance(a, u.RGBArenaCameraZone):
        f = a.get_component_by_class(u.CameraComponent).get_forward_vector()
        check(f.y < -0.99 and abs(f.z) < 0.01, a.get_actor_label() + " faces the X/Z play plane")
    if isinstance(a, (u.DirectionalLight, u.SkyLight, u.PointLight)):
        check(a.light_component.get_editor_property("mobility") == u.ComponentMobility.MOVABLE, a.get_actor_label() + " is dynamic (no light bake needed)")

result["actor_count"] = len(all_actors)
out = os.path.join(root, "Saved/LevelDesign/level_three_validation.json")
with open(out, "w", encoding="utf8") as f:
    json.dump(result, f, indent=2)
u.log("RGB_L3_VALIDATION %s: %d checks, %d failed" % ("PASSED" if result["failed"] == 0 else "FAILED", len(result["checks"]), result["failed"]))
