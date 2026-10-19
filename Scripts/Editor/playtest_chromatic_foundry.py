"""Editor-only smoke test using real PIE worlds, landing events, timers and respawn.

Launch UnrealEditor with -ExecCmds="py <this file>". Nothing is saved.
This is a scripted systems check, not a claim of a full human combat playthrough.
"""
import unreal as u
import time
import json
import os
import traceback

root = u.Paths.convert_relative_path_to_full(u.Paths.project_dir())
out = os.path.join(root, "Saved/LevelDesign/chromatic_foundry_pie.json")
levels = u.get_editor_subsystem(u.LevelEditorSubsystem)
levels.load_level("/Game/RGB/Maps/L_ChromaticFoundry")
results = {"checks": [], "observations": [], "finished": False}
state = {"stage": "start", "next": time.monotonic() + 4, "arena": 1, "pad": 0, "drops": 0}
world = None
labels = {}

def write():
    with open(out, "w", encoding="utf8") as f:
        json.dump(results, f, indent=2)

def check(value, label):
    results["checks"].append({"passed": bool(value), "description": label})
    write()
    if not value:
        raise RuntimeError(label)

def observe(text):
    results["observations"].append(text)
    u.log("RGB_FOUNDRY_PIE " + text)
    write()

def player():
    return u.GameplayStatics.get_player_character(world, 0)

def move_to(x, z):
    pawn = player()
    movement = pawn.get_component_by_class(u.CharacterMovementComponent)
    movement.stop_movement_immediately()
    pawn.set_actor_location(u.Vector(x, 0, z), False, True)
    movement.set_movement_mode(u.MovementMode.MOVE_FALLING)

def control(i):
    return labels["A%d_Control_%s" % (i, ("RED", "GREEN", "BLUE")[i - 1])]

def tick(dt):
    global world, labels
    if time.monotonic() < state["next"]:
        return
    try:
        stage = state["stage"]
        if stage == "start":
            levels.editor_request_begin_play()
            state.update(stage="wait_pie", next=time.monotonic() + 3)
            return
        if stage == "wait_pie":
            worlds = u.EditorLevelLibrary.get_pie_worlds(False)
            if not worlds or not u.GameplayStatics.get_player_character(worlds[0], 0):
                state["next"] = time.monotonic() + 1
                return
            world = worlds[0]
            labels = {a.get_actor_label(): a for a in u.GameplayStatics.get_all_actors_of_class(world, u.Actor)}
            # Temporarily suppress contact/melee damage during timed observation; real damage is tested at the end.
            for i in (1, 2, 3):
                for e in control(i).get_editor_property("assigned_enemies"):
                    e.set_editor_property("attack_damage", 0)
                    if isinstance(e, u.RGBTeleporterEnemyCharacter):
                        e.set_editor_property("AoEDamage", 0)
            check(player().get_actor_location().x < 0, "Player spawns on the arrival deck")
            check(all(e.get_action_state() == u.RGBEnemyActionState.IDLE for i in (1, 2, 3) for e in control(i).get_editor_property("assigned_enemies")), "All enemies idle before entering their rooms")
            observe("Arrival view ready")
            state.update(stage="supplies", supply=0, next=time.monotonic() + 8)
            return
        if stage == "supplies":
            if state["supply"] < 4:
                move_to(-900 + state["supply"] * 500, 110)
                state["supply"] += 1
                state["next"] = time.monotonic() + 1
                return
            observe("Crossed all four pickup triggers")
            state.update(stage="enter_arena", next=time.monotonic() + 1)
            return
        if stage == "enter_arena":
            i = state["arena"]
            x, base = ((1740, 0), (11240, 900), (20740, 1800))[i - 1]
            move_to(x, base + 110)
            state["enemy_positions"] = {e.get_actor_label(): e.get_actor_location() for e in control(i).get_editor_property("assigned_enemies")}
            observe("Entered arena %d" % i)
            state.update(stage="check_activation", next=time.monotonic() + 9)
            return
        if stage == "check_activation":
            i = state["arena"]
            enemies = control(i).get_editor_property("assigned_enemies")
            check(any(e.get_action_state() != u.RGBEnemyActionState.IDLE for e in enemies), "Arena %d enemies activate on entry" % i)
            if i > 1:
                teleporters = [e for e in enemies if isinstance(e, u.RGBTeleporterEnemyCharacter)]
                check(any((e.get_actor_location() - state["enemy_positions"][e.get_actor_label()]).length() > 150 for e in teleporters), "Arena %d teleporters change destination" % i)
            # Hide enemies only in PIE while checking deterministic landing/exit wiring.
            for e in enemies:
                e.set_actor_hidden_in_game(True)
                e.set_actor_enable_collision(False)
            check(not control(i).is_completed(), "Arena %d does not complete on entry" % i)
            state.update(stage="land", pad=0, drops=0, next=time.monotonic() + 1)
            return
        if stage == "land":
            i = state["arena"]
            pads = control(i).get_editor_property("assigned_platforms")
            target = control(i).get_editor_property("required_color")
            if state["pad"] == len(pads):
                state.update(stage="check_completion", next=time.monotonic() + 5)
                return
            pad = pads[state["pad"]]
            color = next(c for c in pad.get_components_by_class(u.ActorComponent) if c.get_name() == "ColorComponent")
            if color.get_editor_property("current_color") == target:
                state["pad"] += 1
                state["drops"] = 0
                return
            check(state["drops"] < 4, pad.get_actor_label() + " responds to player landings")
            origin, extent = pad.get_actor_bounds(False)
            move_to(pad.get_actor_location().x, origin.z + extent.z + 250)
            state["drops"] += 1
            state["next"] = time.monotonic() + 1.5
            return
        if stage == "check_completion":
            i = state["arena"]
            check(control(i).is_completed(), "Arena %d completes after all real landing colour changes" % i)
            gate = labels["A%d_CompletionShutter" % i]
            base = (0, 900, 1800)[i - 1]
            check(gate.get_actor_location().z > base + 9000, "Arena %d completion opens its exit shutter" % i)
            observe("Arena %d completed and exit opened" % i)
            if i < 3:
                state.update(arena=i + 1, stage="enter_arena", next=time.monotonic() + 1)
            else:
                move_to(20740, 1910)
                state.update(stage="respawn", next=time.monotonic() + 2)
            return
        if stage == "respawn":
            state["old_pawn"] = player().get_name()
            player().recieve_damage(999)
            state.update(stage="check_respawn", next=time.monotonic() + 3)
            return
        if stage == "check_respawn":
            check(player() is not None and player().get_name() != state["old_pawn"], "Player respawns after lethal damage")
            check(abs(player().get_actor_location().x - 20740) < 150, "Respawn uses the final arena checkpoint")
            check(all(not control(i).is_completed() for i in (1, 2, 3)), "Lethal damage follows the existing game-wide puzzle reset rule")
            results["finished"] = True
            observe("Scripted PIE checks passed; full combat and traversal feel still need a human playthrough")
            levels.editor_request_end_play()
            u.unregister_slate_post_tick_callback(handle)
            return
    except Exception:
        results["error"] = traceback.format_exc()
        write()
        u.log_error(results["error"])
        levels.editor_request_end_play()
        u.unregister_slate_post_tick_callback(handle)

handle = u.register_slate_post_tick_callback(tick)
write()
