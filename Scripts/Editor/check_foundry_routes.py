"""Real PIE physics probes for authored jumps and C++ jump-pad launches."""
import unreal as u
import time
import os
import json
import traceback

route_levels = u.get_editor_subsystem(u.LevelEditorSubsystem)
assert not route_levels.is_in_play_in_editor()
route_levels.load_level("/Game/RGB/Maps/L_ChromaticFoundry")
route_report = {"checks": [], "finished": False}
route_state = {"stage": "start", "next": time.monotonic() + 2, "index": 0}
route_probes = [
    ("Gantry jump 1", (8450, 0), (8900, 140), None),
    ("Gantry jump 2", (9350, 140), (9900, 280), None),
    ("Gantry boost to green entry", None, (11240, 900), "Route01_Boost"),
    ("A1 recovery", None, (4500, 0), "A1_RecoveryBoost_Left"),
    ("A1 left upper", None, (3500, 1000), "A1_Boost_LeftUpper"),
    ("A1 right upper", None, (5500, 1000), "A1_Boost_RightUpper"),
    ("A2 left upper", None, (12900, 2000), "A2_Boost_LeftUpper"),
    ("A2 right upper", None, (15000, 2000), "A2_Boost_RightUpper"),
    ("A2 apex", None, (14000, 2900), "A2_Boost_ApexLeft"),
    ("Induction recovery jump", (17800, 900), (18500, 300), None),
    ("Induction boost to blue entry", None, (19500, 1800), "Route02_RecoveryBoost"),
    ("A3 left upper", None, (22000, 2900), "A3_Boost_LeftUpper"),
    ("A3 right upper", None, (25800, 2900), "A3_Boost_RightUpper"),
    ("A3 central", None, (23900, 3500), "A3_Boost_CentreLeft"),
    ("A3 apex", None, (25200, 4600), "A3_Boost_Apex"),
]
route_out = os.path.join(u.Paths.project_dir(), "Saved/LevelDesign/chromatic_foundry_routes.json")
route_world = None
route_labels = {}

def route_write():
    with open(route_out, "w") as f:
        json.dump(route_report, f, indent=2)

def route_tick(dt):
    global route_world, route_labels
    try:
        stage = route_state["stage"]
        if stage == "flight":
            pawn = u.GameplayStatics.get_player_character(route_world, 0)
            movement = pawn.get_component_by_class(u.CharacterMovementComponent)
            title, _, (target_x, target_z), _ = route_probes[route_state["index"]]
            pos = pawn.get_actor_location()
            dx = target_x - pos.x
            if abs(dx) > 40:
                pawn.add_movement_input(u.Vector(1 if dx > 0 else -1, 0, 0), 1.0, True)
            elif not route_state.get("braked"):
                pawn.launch_character(u.Vector(), True, False)
                route_state["braked"] = True
            if movement.is_moving_on_ground() and time.monotonic() > route_state["launched"] + 0.2:
                passed = abs(dx) < 160 and abs(pos.z - (target_z + route_state["half"])) < 70
                route_report["checks"].append({"passed": passed, "description": title, "landed": [pos.x, pos.z], "target": [target_x, target_z]})
                route_write()
                assert passed, title + " landed on an unexpected surface"
                route_state.update(stage="set_probe", index=route_state["index"] + 1, next=time.monotonic() + 0.5)
                return
            if time.monotonic() > route_state["deadline"] or pos.z < -1500:
                route_report["checks"].append({"passed": False, "description": title, "position": [pos.x, pos.z]})
                raise RuntimeError(title + " did not reach its landing")
            return
        if time.monotonic() < route_state["next"]:
            return
        if stage == "start":
            route_levels.editor_request_begin_play()
            route_state.update(stage="wait", next=time.monotonic() + 4)
        elif stage == "wait":
            worlds = u.EditorLevelLibrary.get_pie_worlds(False)
            if not worlds:
                return
            route_world = worlds[0]
            route_labels = {a.get_actor_label(): a for a in u.GameplayStatics.get_all_actors_of_class(route_world, u.Actor)}
            for enemy in u.GameplayStatics.get_all_actors_of_class(route_world, u.RGBEnemyCharacter):
                enemy.set_actor_hidden_in_game(True)
                enemy.set_actor_enable_collision(False)
                enemy.set_editor_property("attack_damage", 0)
                if isinstance(enemy, u.RGBTeleporterEnemyCharacter):
                    enemy.set_editor_property("AoEDamage", 0)
            route_state.update(stage="set_probe", next=time.monotonic() + 1)
        elif stage == "set_probe":
            if route_state["index"] == len(route_probes):
                route_report["finished"] = True
                route_write()
                route_levels.editor_request_end_play()
                u.unregister_slate_post_tick_callback(route_handle)
                return
            _, start, _, boost = route_probes[route_state["index"]]
            pawn = u.GameplayStatics.get_player_character(route_world, 0)
            movement = pawn.get_component_by_class(u.CharacterMovementComponent)
            movement.stop_movement_immediately()
            half = pawn.get_component_by_class(u.CapsuleComponent).get_scaled_capsule_half_height()
            if boost:
                loc = route_labels[boost].get_actor_location()
                pawn.set_actor_location(u.Vector(loc.x, 0, loc.z + half + 180), False, True)
                movement.set_movement_mode(u.MovementMode.MOVE_FALLING)
                route_state.update(stage="wait_boost", next=time.monotonic(), deadline=time.monotonic() + 5)
            else:
                pawn.set_actor_location(u.Vector(start[0], 0, start[1] + half + 5), False, True)
                movement.set_movement_mode(u.MovementMode.MOVE_FALLING)
                route_state.update(stage="jump", next=time.monotonic() + 0.7)
            route_state["half"] = half
        elif stage in ("jump", "wait_boost"):
            pawn = u.GameplayStatics.get_player_character(route_world, 0)
            movement = pawn.get_component_by_class(u.CharacterMovementComponent)
            if stage == "wait_boost":
                if movement.velocity.z < 1500:
                    assert time.monotonic() < route_state["deadline"], "C++ jump pad did not launch the player"
                    return
            else:
                pawn.launch_character(u.Vector(0, 0, movement.get_editor_property("jump_z_velocity")), False, True)
            route_state.update(stage="flight", braked=False, launched=time.monotonic(), deadline=time.monotonic() + 8)
    except Exception:
        route_report["error"] = traceback.format_exc()
        route_write()
        route_levels.editor_request_end_play()
        u.unregister_slate_post_tick_callback(route_handle)

route_handle = u.register_slate_post_tick_callback(route_tick)
route_write()
