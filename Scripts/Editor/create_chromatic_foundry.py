"""Author L_ChromaticFoundry using existing RGB gameplay actors and Unreal editor APIs.

Run with UnrealEditor-Cmd RGB.uproject -run=pythonscript -script=<this file>
This refuses to overwrite its map. Reference maps and shared assets are never saved.
"""
import unreal as u
import os
import json
import hashlib

ROOT = u.Paths.convert_relative_path_to_full(u.Paths.project_dir())
MAP = "/Game/RGB/Maps/L_ChromaticFoundry"
ASSETS = "/Game/RGB/Levels/ChromaticFoundry"
OUT = os.path.join(ROOT, "Saved", "LevelDesign")
os.makedirs(OUT, exist_ok=True)
if u.EditorAssetLibrary.does_asset_exist(MAP):
    raise RuntimeError("Map already exists. Refusing to overwrite an authored level: " + MAP)

levels = u.get_editor_subsystem(u.LevelEditorSubsystem)
actors = u.get_editor_subsystem(u.EditorActorSubsystem)
V = u.Vector
R = u.Rotator
refs = {}
for name in ("L_INTRO", "L_MapLayout"):
    disk = os.path.join(ROOT, "Content/RGB/Maps", name + ".umap")
    refs[name] = hashlib.sha256(open(disk, "rb").read()).hexdigest()

# Copy the intro's post process settings as a value, without modifying its volume.
levels.load_level("/Game/RGB/Maps/L_INTRO")
pp_settings = None
for actor in actors.get_all_level_actors():
    if isinstance(actor, u.PostProcessVolume):
        pp_settings = actor.get_editor_property("settings").copy()
        break
assert levels.new_level(MAP)
world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
world.get_world_settings().set_editor_property("default_game_mode", u.load_class(None, "/Game/RGB/Blueprints/Game/BP_RGBGameMode.BP_RGBGameMode_C"))
world.get_world_settings().set_editor_property("kill_z", -4000.0)

created_assets = []
def material(name, rgb, glow=0.0, metallic=0.3):
    path = ASSETS + "/Materials/" + name
    if u.EditorAssetLibrary.does_asset_exist(path):
        created_assets.append(path)
        return u.load_asset(path)
    mat = u.AssetToolsHelpers.get_asset_tools().create_asset(name, ASSETS + "/Materials", u.Material, u.MaterialFactoryNew())
    color = u.MaterialEditingLibrary.create_material_expression(mat, u.MaterialExpressionConstant3Vector, -350, 0)
    color.set_editor_property("constant", u.LinearColor(*rgb, 1.0))
    u.MaterialEditingLibrary.connect_material_property(color, "", u.MaterialProperty.MP_BASE_COLOR)
    rough = u.MaterialEditingLibrary.create_material_expression(mat, u.MaterialExpressionConstant, -350, 150)
    rough.set_editor_property("r", 0.65)
    u.MaterialEditingLibrary.connect_material_property(rough, "", u.MaterialProperty.MP_ROUGHNESS)
    metal = u.MaterialEditingLibrary.create_material_expression(mat, u.MaterialExpressionConstant, -350, 230)
    metal.set_editor_property("r", metallic)
    u.MaterialEditingLibrary.connect_material_property(metal, "", u.MaterialProperty.MP_METALLIC)
    if glow:
        emission = u.MaterialEditingLibrary.create_material_expression(mat, u.MaterialExpressionConstant3Vector, -350, -160)
        emission.set_editor_property("constant", u.LinearColor(*(c * glow for c in rgb), 1.0))
        u.MaterialEditingLibrary.connect_material_property(emission, "", u.MaterialProperty.MP_EMISSIVE_COLOR)
    u.MaterialEditingLibrary.recompile_material(mat)
    u.EditorAssetLibrary.save_loaded_asset(mat)
    created_assets.append(path)
    return mat

mats = {
    "body": material("M_Foundry_Charcoal", (0.045, 0.062, 0.08)),
    "deck": material("M_Foundry_Deck", (0.18, 0.22, 0.26)),
    "back": material("M_Foundry_Backwall", (0.018, 0.027, 0.042)),
    "white": material("M_Foundry_Ivory", (0.7, 0.8, 0.84), 0.25),
    "red": material("M_Foundry_Red", (0.9, 0.035, 0.025), 3),
    "green": material("M_Foundry_Green", (0.035, 0.8, 0.17), 3),
    "blue": material("M_Foundry_Blue", (0.035, 0.25, 1.0), 3),
    "amber": material("M_Foundry_Amber", (1.0, 0.38, 0.055), 2),
}
cube = u.load_asset("/Engine/BasicShapes/Cube")
cylinder = u.load_asset("/Engine/BasicShapes/Cylinder")
report = {"map": MAP, "references_before": refs, "arenas": [], "routes": [], "assets": created_assets}

def spawn(cls, label, xyz=(0, 0, 0), folder="00_Environment", rotation=None):
    if isinstance(cls, str):
        cls = u.load_class(None, cls)
    assert cls, label
    actor = actors.spawn_actor_from_class(cls, V(*xyz), rotation or R())
    assert actor, label
    actor.set_actor_label(label)
    actor.set_folder_path(folder)
    return actor

def component(actor, name):
    return next(c for c in actor.get_components_by_class(u.ActorComponent) if c.get_name() == name)

def block(label, xyz, size, mat="body", folder="00_Environment", collision=True, mesh=None):
    actor = spawn(u.StaticMeshActor, label, xyz, folder)
    sm = actor.static_mesh_component
    sm.set_static_mesh(mesh or cube)
    sm.set_material(0, mats[mat])
    sm.set_collision_profile_name("BlockAll" if collision else "NoCollision")
    actor.set_actor_scale3d(V(*(s / 100.0 for s in size)))
    return actor

def sign(label, text, xyz, size=75, color=(210, 230, 240), folder="00_Environment"):
    actor = spawn(u.TextRenderActor, label, xyz, folder, R(pitch=0, yaw=90, roll=0))
    comp = actor.text_render
    comp.set_text(text)
    comp.set_world_size(size)
    comp.set_horizontal_alignment(u.HorizTextAligment.EHTA_CENTER)
    comp.set_text_render_color(u.Color(*color, 255))
    return actor

def deck(label, x, z, width, folder, accent="amber"):
    block(label, (x, 0, z - 90), (width, 700, 180), "deck", folder)
    block(label + "_FrontBeam", (x, 365, z - 110), (width, 40, 140), "body", folder)
    block(label + "_EdgeLight", (x, 390, z - 25), (width - 30, 14, 18), accent, folder, False)

def camera(label, lo, hi, base, arena=True):
    actor = spawn("/Script/RGB.RGBArenaCameraZone", label, ((lo + hi) / 2, 0, base + 2500), "01_Cameras")
    component(actor, "ArenaBounds").set_box_extent(V((hi - lo) / 2, 430, 3200))
    cam = component(actor, "ArenaCamera")
    cam.set_editor_property("relative_location", V(0, 6400 if arena else 4000, -1600))
    cam.set_editor_property("relative_rotation", R(pitch=0, yaw=-90, roll=0))
    cam.set_editor_property("field_of_view", 65.0)
    actor.set_editor_property("camera_tracking_mode", u.RGBArenaCameraTrackingMode.FOLLOW_PLAYER_Z if arena else u.RGBArenaCameraTrackingMode.FOLLOW_PLAYER_XZ)
    actor.set_editor_property("vertical_follow_offset", 650.0 if arena else 350.0)
    actor.set_editor_property("blend_duration", 0.75)
    actor.set_editor_property("arena_view_mode", u.RGBArenaViewMode.NORMAL_PERSPECTIVE)
    return actor

def checkpoint(label, x, z, lo, hi, folder):
    actor = spawn("/Script/RGB.RGBCheckpoint", label, (x, 0, z), folder)
    component(actor, "RespawnPoint").set_editor_property("relative_location", V(0, 0, 110))
    trigger = component(actor, "RespawnTrigger")
    trigger.set_editor_property("relative_location", V((lo + hi) / 2 - x, 0, -2000 - z))
    trigger.set_box_extent(V((hi - lo) / 2, 1000, 150))
    block(label + "_Beacon", (x, -270, z + 100), (35, 35, 200), "white", folder, False)
    block(label + "_Halo", (x, 0, z + 4), (130, 500, 8), "amber", folder, False)
    return actor

def path(label, points, folder):
    actor = spawn("/Script/RGB.RGBPlatformPath", label, points[0], folder)
    spline = component(actor, "PathSpline")
    spline.clear_spline_points(False)
    for p in points:
        spline.add_spline_point(V(*p), u.SplineCoordinateSpace.WORLD, False)
    for i in range(len(points)):
        spline.set_spline_point_type(i, u.SplinePointType.LINEAR, False)
    spline.update_spline()
    return actor

def moving(label, points, size, folder, mat="amber", active=True, one_way=False, speed=450):
    route = path(label + "_Path", points, folder)
    actor = spawn("/Script/RGB.RGBMovingPlatform", label, points[0], folder)
    actor.set_editor_property("movement_path", route)
    sm = component(actor, "PlatformMesh")
    sm.set_static_mesh(cube)
    sm.set_material(0, mats[mat])
    sm.set_editor_property("relative_scale3d", V(*(s / 100 for s in size)))
    move = component(actor, "PathMovement")
    assert move.get_editor_property("movement_path") == route
    move.set_editor_property("start_active", active)
    move.set_editor_property("movement_speed", float(speed))
    move.set_editor_property("movement_mode", u.RGBPathMovementMode.ONE_WAY if one_way else u.RGBPathMovementMode.PING_PONG)
    component(actor, "RotationMovement").set_editor_property("start_active", False)
    return actor

def color_platform(label, x, z, width, initial, folder):
    actor = spawn("/Game/RGB/Blueprints/Platforms/BP_RGBPlatformLarge.BP_RGBPlatformLarge_C", label, (x, 0, z - 30), folder)
    actor.set_actor_scale3d(V(width / 400.0, 1.5, 1.2))
    actor.set_editor_property("interaction_mode", u.RGBPlatformInteractionMode.LANDING_ONLY)
    color = component(actor, "ColorComponent")
    color.set_editor_property("randomize_initial_color", False)
    color.set_editor_property("initial_color", initial)
    # Show the initial puzzle colours in the editor as well as at BeginPlay.
    material_name = {u.RGBColor.RED: "Red", u.RGBColor.GREEN: "Green", u.RGBColor.BLUE: "Blue"}[initial]
    component(actor, "PlatformMesh").set_material(actor.get_editor_property("color_material_index"), u.load_asset("/Game/RGB/Assets/Platforms/Materials/MI_PlatformLarge_" + material_name))
    return actor

def enemy(kind, label, x, surface_z, folder, initial=None):
    bp = {"ground": "BP_RGBEnemyHeavy", "teleporter": "BP_RGBEnemyTeleporter", "flying": "BP_RGBFlyingEnemy"}[kind]
    actor = spawn("/Game/RGB/Blueprints/Enemies/" + bp + "." + bp + "_C", label, (x, 0, surface_z + 200), folder)
    scale = 1.7 if kind == "ground" else 1.5 if kind == "teleporter" else 1.0
    actor.set_actor_scale3d(V(scale, scale, scale))
    half = actor.get_component_by_class(u.CapsuleComponent).get_scaled_capsule_half_height()
    actor.set_actor_location(V(x, 0, surface_z + half + 8), False, False)
    color = component(actor, "ColorComponent")
    color.set_editor_property("randomize_initial_color", initial is None)
    if initial is not None:
        color.set_editor_property("initial_color", initial)
    if kind == "teleporter":
        actor.set_editor_property("teleport_point_search_radius", 10000.0)
    return actor

def boost(label, x, z, speed, folder):
    actor = spawn("/Game/RGB/Blueprints/Platforms/BP_RGBJumpPad_Visual.BP_RGBJumpPad_Visual_C", label, (x, 0, z + 10), folder)
    actor.set_editor_property("launch_speed", float(speed))
    return actor

def arena(index, title, lo, hi, base, required, layouts, enemies):
    accent = ("red", "green", "blue")[index - 1]
    folder = "%02d_%s" % (index * 10, title)
    center = (lo + hi) / 2
    zone = camera("A%d_Camera" % index, lo, hi, base)
    # Neutral lower deck provides recovery space; puzzle pads remain distinct above it.
    deck("A%d_RecoveryDeck" % index, center, base - 550, hi - lo, folder, accent)
    deck("A%d_Entry" % index, lo + 250, base, 650, folder, accent)
    deck("A%d_Exit" % index, hi - 280, base, 800, folder, accent)
    platforms = []
    for n, (x, height, width, initial) in enumerate(layouts):
        platforms.append(color_platform("A%d_Puzzle_%02d" % (index, n + 1), x, base + height, width, initial, folder + "/Puzzle"))
        block("A%d_PadSupport_%02d" % (index, n + 1), (x, -200, base + height - 260), (width * 0.5, 180, 400), "body", folder, False)
        sign("A%d_PadNumber_%02d" % (index, n + 1), "%02d" % (n + 1), (x, -320, base + height + 155), 55, folder=folder)
    spawned_enemies = [enemy(kind, "A%d_%s_%d" % (index, kind, n + 1), x, base + z, folder + "/Enemies", initial) for n, (kind, x, z, initial) in enumerate(enemies)]
    targets = []
    if any(e[0] == "teleporter" for e in enemies):
        for n, (x, height, width, _) in enumerate(layouts):
            for side in (-1, 1):
                target = spawn(u.TargetPoint, "A%d_Teleport_%02d_%s" % (index, n + 1, side), (x + side * min(width / 3, 300), 0, base + height + 30), folder + "/TeleportPoints")
                targets.append(target)
    sign_x = center - 1600 if index == 3 else center
    control = spawn("/Game/RGB/Blueprints/Map_Arena/BP_RGBArenaControl.BP_RGBArenaControl_C", "A%d_Control_%s" % (index, accent.upper()), (sign_x, -410, base + 2000), folder + "/Rules")
    control.set_editor_property("required_color", required)
    control.set_editor_property("assigned_platforms", platforms)
    control.set_editor_property("assigned_enemies", spawned_enemies)
    control.set_editor_property("arena_camera_zone", zone)
    control.set_editor_property("use_arena_enemy_tuning", True)
    control.set_editor_property("teleport_target_points", targets)
    control.set_editor_property("scale_enemy_respawn_delay_with_arena_progress", True)
    control.set_editor_property("enemy_respawn_delay_at_no_progress", 12.0)
    control.set_editor_property("enemy_respawn_delay_near_completion", 7.0 if index == 1 else 5.0)
    tuning = control.get_editor_property("coordination_tuning")
    maximum = tuning.get_editor_property("maximum_attackers")
    maximum.set_editor_property("at_no_progress", 1.0)
    maximum.set_editor_property("near_completion", float(index + 1 if index < 3 else 3))
    tuning.set_editor_property("maximum_attackers", maximum)
    control.set_editor_property("coordination_tuning", tuning)
    # A full-height physical shutter prevents jumping past an unfinished puzzle.
    gate_x = hi - 70
    gate = moving("A%d_CompletionShutter" % index, [(gate_x, 0, base + 2400), (gate_x, 0, base + 9400)], (180, 950, 7000), folder + "/Exit", "body", False, True, 1800)
    activator = spawn("/Script/RGB.RGBArenaCompletionActivator", "A%d_OpenExitOnCompletion" % index, (hi - 400, -500, base), folder + "/Rules")
    activator.set_editor_property("arena_control", control)
    activator.set_editor_property("controlled_actors", [gate])
    # Architecture, lighting strips, and oversized room identities.
    block("A%d_Backwall" % index, (center, -850, base + 1700), (hi - lo + 100, 160, 5100), "back", folder)
    for x in range(int(lo + 300), int(hi), 1200):
        block("A%d_Rib_%d" % (index, x), (x, -730, base + 1700), (75, 150, 5000), "body", folder, False)
        block("A%d_RibLight_%d" % (index, x), (x + 65, -635, base + 1500), (18, 16, 1800), accent, folder, False)
    block("A%d_Header" % index, (center, -550, base + 3100), (hi - lo - 600, 50, 30), accent, folder, False)
    sign("A%d_Title" % index, "0%d / %s" % (index, title.upper().replace("_", " ")), (sign_x, -590, base + 2800), 150, folder=folder)
    sign("A%d_Goal" % index, "MATCH ALL %d PADS TO %s  /  EXIT UNLOCKS AT 100%%" % (len(platforms), accent.upper()), (sign_x, -590, base + 2540), 65, folder=folder)
    sign("A%d_ExitLabel" % index, "SEALED\nCOMPLETE ARENA", (hi - 220, 495, base + 500), 50, folder=folder)
    checkpoint("A%d_Checkpoint" % index, lo + 240, base, lo, hi, folder + "/Recovery")
    report["arenas"].append({"index": index, "control": control.get_actor_label(), "range": [lo, hi], "base_z": base, "platform_count": len(platforms), "enemy_count": len(spawned_enemies), "teleport_points": len(targets), "target": accent, "gate": gate.get_actor_label()})

# Lighting uses the intro's grading, with explicit practical fill for readable silhouettes.
sun = spawn(u.DirectionalLight, "Foundry_KeyLight", (0, 0, 5000), rotation=R(pitch=-48, yaw=-35, roll=0))
sun.light_component.set_mobility(u.ComponentMobility.MOVABLE)
sun.light_component.set_intensity(5.0)
sky = spawn(u.SkyLight, "Foundry_SkyFill", (0, 0, 3000))
sky.light_component.set_mobility(u.ComponentMobility.MOVABLE)
sky.light_component.set_editor_property("intensity", 3.0)
sky.light_component.set_editor_property("real_time_capture", True)
spawn(u.SkyAtmosphere, "Foundry_Atmosphere")
pp = spawn(u.PostProcessVolume, "Foundry_PostProcess")
if pp_settings:
    pp.set_editor_property("settings", pp_settings)
pp.set_editor_property("unbound", True)

# Arrival supply line uses the four existing ability pickups.
deck("Arrival_Deck", -100, 0, 3400, "02_Arrival")
block("Arrival_Backwall", (-100, -850, 1400), (3400, 150, 3200), "back", "02_Arrival")
spawn(u.PlayerStart, "PlayerStart_Foundry", (-1400, 0, 115), "02_Arrival")
camera("Arrival_Camera", -1900, 1490, 0, False)
checkpoint("Arrival_Checkpoint", -1350, 0, -1900, 1490, "02_Arrival")
for i, bp in enumerate(("BP_RGBDashPickup", "BP_RGBRedWeaponPickup", "BP_RGBGreenWeaponPickup", "BP_RGBBlueWeaponPickup")):
    pickup = spawn("/Game/RGB/Blueprints/Progression/" + bp + "." + bp + "_C", "Supply_" + bp, (-900 + i * 500, 0, 85), "02_Arrival")
    # Forgiving pickup triggers avoid requiring precise contact with the small meshes.
    pickup.get_component_by_class(u.SphereComponent).set_sphere_radius(300)
sign("Foundry_Title", "CHROMATIC\nFOUNDRY", (-200, -600, 1250), 180, folder="02_Arrival")
sign("Foundry_Brief", "THREE ROOMS. THREE COLOURS. ONE WAY THROUGH.", (-200, -590, 800), 55, folder="02_Arrival")
sign("Foundry_Hint", "LAND TO CYCLE: RED > GREEN > BLUE\nMATCH YOUR SHOTS TO THE ENEMY\nAMBER LIGHTS MARK THE ROUTE", (-200, -590, 540), 50, folder="02_Arrival")

red, green, blue = u.RGBColor.RED, u.RGBColor.GREEN, u.RGBColor.BLUE
arena(1, "Crimson_Press", 1500, 7600, 0, red,
      [(2700, 0, 1300, green), (4500, 0, 1300, blue), (6300, 0, 1300, green), (3500, 1000, 900, blue), (5500, 1000, 900, green)],
      [("ground", 2700, 0, red), ("ground", 6300, 0, blue)])

camera("Route01_Camera", 7610, 10990, 400, False)
for n, (x, z, width) in enumerate(((8100, 0, 900), (9100, 140, 700), (10100, 280, 700), (10800, 420, 700))):
    deck("Route01_Step%d" % n, x, z, width, "15_Transfer_Gantry")
checkpoint("Route01_Checkpoint", 8100, 0, 7610, 10990, "15_Transfer_Gantry")
sign("Route01_Sign", "TRANSFER GANTRY  >\nNEXT: VERDANT RELAY", (9300, -500, 1700), 80, folder="15_Transfer_Gantry")
report["routes"].append({"name": "Transfer Gantry", "range": [7610, 10990], "maximum_step_rise": 140, "maximum_edge_gap": 300, "launch_to_entry": True})

arena(2, "Verdant_Relay", 11000, 17000, 900, green,
      [(12100, 0, 1200, red), (13900, 0, 1200, blue), (15700, 0, 1200, red), (12900, 1100, 900, blue), (15000, 1100, 900, red), (14000, 2000, 800, blue)],
      [("teleporter", 12100, 0, None), ("teleporter", 15700, 0, None), ("flying", 14000, 1450, None)])

camera("Route02_Camera", 17010, 20490, 1300, False)
deck("Route02_Entry", 17450, 900, 900, "25_The_Induction_Gap")
moving("Route02_Shuttle", [(18150, 0, 1030), (19150, 0, 1580)], (750, 650, 100), "25_The_Induction_Gap", "deck", True, False, 400)
deck("Route02_RecoveryStep", 18500, 300, 650, "25_The_Induction_Gap")
deck("Route02_Exit", 19950, 1800, 1100, "25_The_Induction_Gap")
checkpoint("Route02_Checkpoint", 17450, 900, 17010, 20490, "25_The_Induction_Gap")
sign("Route02_Sign", "THE INDUCTION GAP\nRIDE THE SHUTTLE  >", (18600, -500, 2450), 80, folder="25_The_Induction_Gap")
report["routes"].append({"name": "Induction Gap", "range": [17010, 20490], "moving_shuttle": True})

arena(3, "Azure_Core", 20500, 27300, 1800, blue,
      [(21700, 0, 1600, red), (23900, 0, 1600, green), (26100, 0, 1600, red), (22000, 1100, 1000, green), (23900, 1700, 1000, red), (25800, 1100, 1000, green), (25200, 2800, 1100, red)],
      [("ground", 21700, 0, None), ("ground", 26100, 0, None), ("teleporter", 22000, 1100, None), ("teleporter", 25800, 1100, None), ("flying", 23900, 2200, None)])

# The actual player Blueprint jump is 1120 cm/s (about 213 cm high), so tall routes use authored launch pads.
for label, x, z, speed, folder in (
    ("A1_Boost_LeftUpper", 2400, 0, 2800, "10_Crimson_Press/Traversal"),
    ("A1_Boost_RightUpper", 4300, 0, 2800, "10_Crimson_Press/Traversal"),
    ("A1_RecoveryBoost_Left", 3600, -550, 2300, "10_Crimson_Press/Recovery"),
    ("A1_RecoveryBoost_Right", 5400, -550, 2300, "10_Crimson_Press/Recovery"),
    ("Route01_Boost", 10100, 280, 2200, "15_Transfer_Gantry"),
    ("A2_Boost_LeftUpper", 11800, 900, 3000, "20_Verdant_Relay/Traversal"),
    ("A2_Boost_RightUpper", 13700, 900, 3000, "20_Verdant_Relay/Traversal"),
    ("A2_Boost_ApexLeft", 12700, 2000, 2700, "20_Verdant_Relay/Traversal"),
    ("A2_Boost_ApexRight", 15200, 2000, 2700, "20_Verdant_Relay/Traversal"),
    ("A2_RecoveryBoost_Left", 13000, 350, 2300, "20_Verdant_Relay/Recovery"),
    ("A2_RecoveryBoost_Right", 14800, 350, 2300, "20_Verdant_Relay/Recovery"),
    ("Route02_RecoveryBoost", 17600, 900, 3500, "25_The_Induction_Gap"),
    ("A3_Boost_LeftUpper", 21050, 1800, 3500, "30_Azure_Core/Traversal"),
    ("A3_Boost_RightUpper", 26750, 1800, 3500, "30_Azure_Core/Traversal"),
    ("A3_Boost_CentreLeft", 22200, 2900, 3000, "30_Azure_Core/Traversal"),
    ("A3_Boost_CentreRight", 25600, 2900, 3000, "30_Azure_Core/Traversal"),
    ("A3_Boost_Apex", 23700, 3500, 3000, "30_Azure_Core/Traversal"),
    ("A3_RecoveryBoost_Left", 22800, 1250, 2300, "30_Azure_Core/Recovery"),
    ("A3_RecoveryBoost_Right", 25000, 1250, 2300, "30_Azure_Core/Recovery"),
):
    boost(label, x, z, speed, folder)

camera("Departure_Camera", 27310, 32000, 1800, False)
deck("Departure_Deck", 29200, 1800, 4000, "40_Departure", "blue")
checkpoint("Departure_Checkpoint", 27900, 1800, 27310, 32000, "40_Departure")
sign("Departure_End", "CHROMATIC FOUNDRY\nCIRCUIT COMPLETE", (29200, -500, 3000), 140, folder="40_Departure")
sign("Departure_Exit", "RETURN TO MAIN MENU  >", (29700, -500, 2400), 65, folder="40_Departure")
portal = spawn("/Script/RGB.RGBLevelTransitionArea", "Departure_ReturnToMenu", (30800, 0, 2100), "40_Departure")
portal.set_editor_property("destination_level", u.load_asset("/Game/RGB/Maps/L_MainMenu"))
portal.set_editor_property("fade_out_duration", 2.0)
component(portal, "TransitionTrigger").set_box_extent(V(150, 350, 450))
for x in (30550, 31050):
    block("Departure_PortalFrame_%d" % x, (x, -200, 2250), (90, 120, 900), "blue", "40_Departure", False)

# Distant industrial silhouette: visual-only so it cannot affect the play plane.
for i, x in enumerate(range(-1400, 32000, 1600)):
    height = 3000 + (i % 4) * 650
    block("Skyline_Tower_%02d" % i, (x, -1900, height / 2 - 500), (650, 500, height), "body", "00_Environment/Skyline", False)
    block("Skyline_Status_%02d" % i, (x, -1600, height - 700), (320, 30, 22), ("red", "green", "blue")[i % 3], "00_Environment/Skyline", False)

assert levels.save_current_level()
report["actor_count"] = len(actors.get_all_level_actors())
report["references_unchanged"] = all(refs[n] == hashlib.sha256(open(os.path.join(ROOT, "Content/RGB/Maps", n + ".umap"), "rb").read()).hexdigest() for n in refs)
assert report["references_unchanged"]
with open(os.path.join(OUT, "chromatic_foundry_build.json"), "w", encoding="utf-8") as f:
    json.dump(report, f, indent=2)
unreal_result = "RGB_FOUNDRY_CREATED: %s, %d actors, references unchanged" % (MAP, report["actor_count"])
u.log(unreal_result)
