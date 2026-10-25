"""Author L_THREE ("The Chroma Works"): Level 3, where the player obtains the blue weapon.

A robot-assembly factory built only from existing RGB gameplay actors (C++ classes and
existing Blueprints), existing meshes and existing materials. No C++, Blueprint class
defaults, project settings or other maps are changed. Reference maps are loaded read-only
and verified unchanged on disk afterwards.

Run with Unreal closed:
UnrealEditor-Cmd.exe <abs>/RGB.uproject -run=pythonscript -script=<abs>/Scripts/Editor/create_level_three.py
  -EnablePlugins=PythonScriptPlugin,EditorScriptingUtilities -unattended -nullrhi -nosound -nosplash

Rebuild rule: if L_THREE already exists, it is replaced ONLY when its file is byte-identical
to what this script last saved (nobody has edited it since). Otherwise the script refuses.
"""
import unreal as u
import os
import json
import math
import hashlib

ROOT = u.Paths.convert_relative_path_to_full(u.Paths.project_dir())
MAP = "/Game/RGB/Maps/L_THREE"
MAP_FILE = os.path.join(ROOT, "Content/RGB/Maps/L_THREE.umap")
OUT = os.path.join(ROOT, "Saved", "LevelDesign")
REPORT = os.path.join(OUT, "level_three_build.json")
REFERENCE_MAPS = ("L_INTRO", "L_TWO_2", "L_ChromaticFoundry", "L_MainMenu")
os.makedirs(OUT, exist_ok=True)


def sha(path):
    with open(path, "rb") as f:
        return hashlib.sha256(f.read()).hexdigest()


refs = {n: sha(os.path.join(ROOT, "Content/RGB/Maps", n + ".umap")) for n in REFERENCE_MAPS}

# ---------------------------------------------------------------- rebuild guard
if u.EditorAssetLibrary.does_asset_exist(MAP):
    previous = None
    if os.path.exists(REPORT):
        with open(REPORT, encoding="utf-8") as f:
            previous = json.load(f).get("saved_map_sha256")
    if not previous or not os.path.exists(MAP_FILE) or sha(MAP_FILE) != previous:
        raise RuntimeError("L_THREE exists and was edited after generation (or has no build record). "
                           "Refusing to overwrite hand-made changes: " + MAP)
    assert u.EditorAssetLibrary.delete_asset(MAP), "Could not remove the previous generated L_THREE"
    u.log("RGB_L3: previous generated L_THREE was unedited; rebuilding it")

levels = u.get_editor_subsystem(u.LevelEditorSubsystem)
actors = u.get_editor_subsystem(u.EditorActorSubsystem)
V = u.Vector
R = u.Rotator

# Copy Level 2's colour grading by value. L_TWO_2 is only read, never saved.
levels.load_level("/Game/RGB/Maps/L_TWO_2")
pp_settings = None
for a in actors.get_all_level_actors():
    if isinstance(a, u.PostProcessVolume):
        pp_settings = a.get_editor_property("settings").copy()
        break

assert levels.new_level(MAP)
world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
world.get_world_settings().set_editor_property("default_game_mode", u.load_class(None, "/Game/RGB/Blueprints/Game/BP_RGBGameMode.BP_RGBGameMode_C"))
world.get_world_settings().set_editor_property("kill_z", -4000.0)

# ---------------------------------------------------------------- existing assets only
def asset(path):
    obj = u.load_asset(path)
    assert obj, "Missing existing asset: " + path
    return obj


FM = "/Game/RGB/Levels/ChromaticFoundry/Materials/"
GM = "/Game/Grid_Materials/Materials/Instances/"
mats = {
    "body": asset(FM + "M_Foundry_Charcoal"),
    "deck": asset(FM + "M_Foundry_Deck"),
    "back": asset(FM + "M_Foundry_Backwall"),
    "white": asset(FM + "M_Foundry_Ivory"),
    "red": asset(FM + "M_Foundry_Red"),
    "green": asset(FM + "M_Foundry_Green"),
    "blue": asset(FM + "M_Foundry_Blue"),
    "amber": asset(FM + "M_Foundry_Amber"),
    "metal": asset(GM + "MI_BlockOut_Metal"),
    "asphalt": asset(GM + "MI_BlockOut_Asphalt"),
    "neon": asset(GM + "MI_BlockOut_Grass_Neon_2"),
    "lava": asset(GM + "MI_BlockOut_Lava"),
}
cube = asset("/Engine/BasicShapes/Cube")
tile_mesh = asset("/Game/RGB/Assets/Platforms/Meshes/PLATFORM_GROUND")   # Level 2 floor tile, 400 x 400 x 50, centred pivot
gear_mesh = asset("/Game/RGB/Assets/LEVEL_ASSETS/CustomMeshes/Gear/Gear_Sharp")
pole_mesh = asset("/Game/RGB/Assets/LEVEL_ASSETS/CustomMeshes/HazardPoles/Hazard_Pole")
robot_heavy = asset("/Game/RGB/Characters/HEAVY/Meshes/Robot_Rigged_BindPose")
robot_teleporter = asset("/Game/RGB/Characters/TELEPORTER/Meshes/TELEPORTER_RIGGED")
red, green, blue = u.RGBColor.RED, u.RGBColor.GREEN, u.RGBColor.BLUE

report = {"map": MAP, "references_before": refs, "sections": [], "arenas": [], "moving_platforms": 0,
          "rotating_platforms": 0, "jump_pads": 0, "robots_on_line": 0, "warnings": []}

# ---------------------------------------------------------------- spawning helpers
def spawn(cls, label, xyz=(0, 0, 0), folder="00_Environment", rotation=None):
    if isinstance(cls, str):
        cls = u.load_class(None, cls)
    assert cls, label
    a = actors.spawn_actor_from_class(cls, V(*xyz), rotation or R())
    assert a, label
    a.set_actor_label(label)
    a.set_folder_path(folder)
    return a


def component(actor, name):
    return next(c for c in actor.get_components_by_class(u.ActorComponent) if c.get_name() == name)


def block(label, xyz, size, mat="body", folder="00_Environment", collision=True, movable=False):
    a = spawn(u.StaticMeshActor, label, xyz, folder)
    sm = a.static_mesh_component
    if movable:
        sm.set_mobility(u.ComponentMobility.MOVABLE)
    sm.set_static_mesh(cube)
    sm.set_material(0, mats[mat])
    sm.set_collision_profile_name("BlockAll" if collision else "NoCollision")
    if not collision:
        sm.set_editor_property("cast_shadow", False)
    a.set_actor_scale3d(V(*(s / 100.0 for s in size)))
    return a


def attach(child, parent):
    child.attach_to_actor(parent, "", u.AttachmentRule.KEEP_WORLD, u.AttachmentRule.KEEP_WORLD, u.AttachmentRule.KEEP_WORLD, False)


def sign(label, text, xyz, size=75, color=(210, 230, 240), folder="00_Environment"):
    a = spawn(u.TextRenderActor, label, xyz, folder, R(pitch=0, yaw=90, roll=0))
    t = a.text_render
    t.set_text(text)
    t.set_world_size(size)
    t.set_horizontal_alignment(u.HorizTextAligment.EHTA_CENTER)
    t.set_text_render_color(u.Color(*color, 255))
    return a


def tiles(label, x0, x1, top, folder, edge="amber"):
    """A Level-2 style floor of PLATFORM_GROUND tiles covering x0..x1 with its top at z=top."""
    centers, c = [], x0 + 200
    while c + 200 < x1:
        centers.append(c)
        c += 390
    if not centers or centers[-1] + 200 < x1:
        centers.append(x1 - 200)
    made = []
    for i, cx in enumerate(centers):
        a = spawn(u.StaticMeshActor, "%s_Tile%02d" % (label, i), (cx, 0, top - 25), folder)
        a.static_mesh_component.set_static_mesh(tile_mesh)
        made.append(a)
    if edge:
        block(label + "_EdgeLight", ((x0 + x1) / 2, 212, top - 18), (x1 - x0 - 20, 14, 16), edge, folder, False)
        block(label + "_Girder", ((x0 + x1) / 2, -40, top - 115), (x1 - x0, 300, 130), "body", folder, False)
    return made


def path(label, points, folder, closed=False, curve=False):
    a = spawn("/Script/RGB.RGBPlatformPath", label, points[0], folder)
    spline = component(a, "PathSpline")
    spline.clear_spline_points(False)
    for p in points:
        spline.add_spline_point(V(*p), u.SplineCoordinateSpace.WORLD, False)
    for i in range(len(points)):
        spline.set_spline_point_type(i, u.SplinePointType.CURVE if curve else u.SplinePointType.LINEAR, False)
    spline.set_closed_loop(closed, False)
    spline.update_spline()
    return a


def mover(label, points, folder, mesh=None, size=None, mat=None, mode="ping_pong", speed=300.0, active=True,
          progress=0.0, collision=True, closed=False, curve=False):
    """Native RGBMovingPlatform following a native RGBPlatformPath."""
    route = path(label + "_Path", points, folder + "/Paths", closed, curve)
    a = spawn("/Script/RGB.RGBMovingPlatform", label, points[0], folder)
    a.set_editor_property("movement_path", route)
    if progress:
        a.set_editor_property("starting_path_progress", float(progress))
    sm = component(a, "PlatformMesh")
    sm.set_static_mesh(mesh or cube)
    if mat:
        sm.set_material(0, mats[mat])
    if size:
        sm.set_editor_property("relative_scale3d", V(*(s / 100.0 for s in size)))
    sm.set_collision_profile_name("BlockAll" if collision else "NoCollision")
    if not collision:
        sm.set_editor_property("cast_shadow", False)
    move = component(a, "PathMovement")
    assert move.get_editor_property("movement_path") == route, label + " path not applied"
    move.set_editor_property("start_active", active)
    move.set_editor_property("movement_speed", float(speed))
    move.set_editor_property("movement_mode", {"ping_pong": u.RGBPathMovementMode.PING_PONG,
                                               "one_way": u.RGBPathMovementMode.ONE_WAY,
                                               "loop": u.RGBPathMovementMode.LOOP}[mode])
    component(a, "RotationMovement").set_editor_property("start_active", False)
    report["moving_platforms"] += 1
    return a


def rotator_platform(label, xyz, folder, mesh=None, size=None, mat=None, axis=(0, 1, 0), speed=45.0, stepped=False,
                     step=180.0, delay=2.0, collision=True):
    """RGBMovingPlatform whose RGBRotationComponent spins it in place (its path is a 1 cm anchor)."""
    x, y, z = xyz
    a = mover(label, [(x, y, z), (x, y, z + 1)], folder, mesh, size, mat, "ping_pong", 1.0, False, 0.0, collision)
    report["moving_platforms"] -= 1
    rot = component(a, "RotationMovement")
    rot.set_editor_property("rotation_mode", u.RGBRotationMode.STEPPED if stepped else u.RGBRotationMode.CONTINUOUS)
    rot.set_editor_property("rotation_axis", V(*axis))
    rot.set_editor_property("rotation_speed", float(speed))
    rot.set_editor_property("step_angle", float(step))
    rot.set_editor_property("delay_between_steps", float(delay))
    rot.set_editor_property("start_active", True)
    report["rotating_platforms"] += 1
    return a


def flip_plate(label, x, top, folder, delay=2.6):
    """A floor tile that flips 180 degrees around the depth axis, then rests for `delay` seconds."""
    a = rotator_platform(label, (x, 0, top - 25), folder, tile_mesh, None, None, (0, 1, 0), 360.0, True, 180.0, delay)
    stripe = block(label + "_Warning", (x, 206, top - 18), (380, 10, 14), "amber", folder, False, True)
    attach(stripe, a)
    return a


def boost(label, x, z, speed, folder):
    a = spawn("/Game/RGB/Blueprints/Platforms/BP_RGBJumpPad_Visual.BP_RGBJumpPad_Visual_C", label, (x, 0, z + 10), folder)
    a.set_editor_property("launch_speed", float(speed))
    report["jump_pads"] += 1
    return a


def checkpoint(label, x, z, lo, hi, folder):
    a = spawn("/Script/RGB.RGBCheckpoint", label, (x, 0, z), folder)
    component(a, "RespawnPoint").set_editor_property("relative_location", V(0, 0, 110))
    trigger = component(a, "RespawnTrigger")
    trigger.set_editor_property("relative_location", V((lo + hi) / 2 - x, 0, -2000 - z))
    trigger.set_box_extent(V((hi - lo) / 2, 1000, 150))
    block(label + "_Beacon", (x, -270, z + 100), (35, 35, 200), "white", folder, False)
    block(label + "_Halo", (x, 0, z + 4), (130, 360, 8), "amber", folder, False)
    return a


def camera(label, lo, hi, base, arena=True):
    a = spawn("/Script/RGB.RGBArenaCameraZone", label, ((lo + hi) / 2, 0, base + 2500), "01_Cameras")
    component(a, "ArenaBounds").set_box_extent(V((hi - lo) / 2, 430, 3200))
    cam = component(a, "ArenaCamera")
    cam.set_editor_property("relative_location", V(0, 6400 if arena else 4000, -1600))
    cam.set_editor_property("relative_rotation", R(pitch=0, yaw=-90, roll=0))
    cam.set_editor_property("field_of_view", 65.0)
    a.set_editor_property("camera_tracking_mode", u.RGBArenaCameraTrackingMode.FOLLOW_PLAYER_Z if arena else u.RGBArenaCameraTrackingMode.FOLLOW_PLAYER_XZ)
    a.set_editor_property("vertical_follow_offset", 650.0 if arena else 350.0)
    a.set_editor_property("blend_duration", 0.75)
    a.set_editor_property("arena_view_mode", u.RGBArenaViewMode.NORMAL_PERSPECTIVE)
    return a


def color_platform(label, x, z, width, initial, folder):
    a = spawn("/Game/RGB/Blueprints/Platforms/BP_RGBPlatformLarge.BP_RGBPlatformLarge_C", label, (x, 0, z - 30), folder)
    a.set_actor_scale3d(V(width / 400.0, 1.5, 1.2))
    a.set_editor_property("interaction_mode", u.RGBPlatformInteractionMode.LANDING_ONLY)
    c = component(a, "ColorComponent")
    c.set_editor_property("randomize_initial_color", False)
    c.set_editor_property("initial_color", initial)
    name = {red: "Red", green: "Green", blue: "Blue"}[initial]
    component(a, "PlatformMesh").set_material(a.get_editor_property("color_material_index"), asset("/Game/RGB/Assets/Platforms/Materials/MI_PlatformLarge_" + name))
    return a


def enemy(kind, label, x, surface_z, folder, initial):
    bp = {"ground": "BP_RGBEnemyHeavy", "teleporter": "BP_RGBEnemyTeleporter", "flying": "BP_RGBFlyingEnemy"}[kind]
    a = spawn("/Game/RGB/Blueprints/Enemies/" + bp + "." + bp + "_C", label, (x, 0, surface_z + 200), folder)
    s = 1.7 if kind == "ground" else 1.5 if kind == "teleporter" else 1.0
    a.set_actor_scale3d(V(s, s, s))
    half = a.get_component_by_class(u.CapsuleComponent).get_scaled_capsule_half_height()
    a.set_actor_location(V(x, 0, surface_z + half + 8), False, False)
    c = component(a, "ColorComponent")
    c.set_editor_property("randomize_initial_color", False)
    c.set_editor_property("initial_color", initial)
    if kind == "teleporter":
        a.set_editor_property("teleport_point_search_radius", 10000.0)
    return a


def light(label, xyz, color, intensity=60.0, radius=3000.0, folder="00_Environment/Lights"):
    a = spawn(u.PointLight, label, xyz, folder)
    lc = a.light_component
    lc.set_mobility(u.ComponentMobility.MOVABLE)
    try:
        lc.set_editor_property("intensity_units", u.LightUnits.CANDELAS)
    except Exception:
        pass
    lc.set_intensity(float(intensity))
    lc.set_light_color(u.LinearColor(*color, 1.0))
    lc.set_attenuation_radius(float(radius))
    lc.set_cast_shadows(False)
    return a


def robot(label, mesh, height, xyz, folder):
    """A decorative, unanimated robot body (bind pose) standing on xyz, scaled to a target height."""
    a = spawn(u.SkeletalMeshActor, label, xyz, folder)
    sk = a.skeletal_mesh_component
    try:
        sk.set_skeletal_mesh_asset(mesh)
    except Exception:
        sk.set_editor_property("skeletal_mesh_asset", mesh)
    sk.set_collision_profile_name("NoCollision")
    sk.set_editor_property("cast_shadow", False)
    origin, extent = a.get_actor_bounds(False)
    if extent.z > 1:
        s = height / (extent.z * 2)
        a.set_actor_scale3d(V(s, s, s))
        origin, extent = a.get_actor_bounds(False)
    loc = a.get_actor_location()
    a.set_actor_location(V(loc.x + (xyz[0] - origin.x), loc.y + (xyz[1] - origin.y), loc.z + (xyz[2] - (origin.z - extent.z))), False, False)
    return a


# The gear mesh's thickness axis is found from its bounds, then turned to face the camera,
# so the RGBRotationComponent can spin it around world Y like a real flywheel.
_gb = gear_mesh.get_bounding_box()
_gc = V((_gb.min.x + _gb.max.x) / 2, (_gb.min.y + _gb.max.y) / 2, (_gb.min.z + _gb.max.z) / 2)
_gs = [_gb.max.x - _gb.min.x, _gb.max.y - _gb.min.y, _gb.max.z - _gb.min.z]
_thin = _gs.index(min(_gs))
_face = max(s for i, s in enumerate(_gs) if i != _thin)
_grot = {0: R(pitch=0, yaw=90, roll=0), 1: R(), 2: R(pitch=0, yaw=0, roll=90)}[_thin]
report["gear_mesh"] = {"size": _gs, "thickness_axis": "XYZ"[_thin]}


def gear(label, xyz, diameter, speed, folder, collision=False):
    a = rotator_platform(label, xyz, folder, gear_mesh, None, None, (0, 1, 0), abs(speed), False, collision=collision)
    if speed < 0:
        component(a, "RotationMovement").set_editor_property("rotation_axis", V(0, -1, 0))
    sm = component(a, "PlatformMesh")
    s = diameter / _face
    sm.set_editor_property("relative_rotation", _grot)
    sm.set_editor_property("relative_scale3d", V(s, s, s))
    cx, cy, cz = _gc.x * s, _gc.y * s, _gc.z * s
    # Same mapping as _grot: yaw 90 sends (x,y,z) to (-y,x,z); roll 90 sends it to (x,z,-y).
    centre = {0: (-cy, cx, cz), 1: (cx, cy, cz), 2: (cx, cz, -cy)}[_thin]
    sm.set_editor_property("relative_location", V(-centre[0], -centre[1], -centre[2]))
    return a


# ---------------------------------------------------------------- robot factory backdrop
ACCENT_RGB = {"red": (1.0, 0.08, 0.05), "green": (0.1, 1.0, 0.25), "blue": (0.1, 0.35, 1.0), "amber": (1.0, 0.45, 0.08)}


def backdrop(key, lo, hi, base, accent):
    f = "00_Environment/Factory/" + key
    w = hi - lo
    mid = (lo + hi) / 2
    # Hall shell: back wall, ribbed metal panels, accent band and observation windows.
    block(key + "_BackWall", (mid, -3700, base + 1500), (w + 40, 200, 10000), "back", f, False)
    for i, x in enumerate(range(int(lo) + 400, int(hi), 1600)):
        block("%s_Panel_%02d" % (key, i), (x, -3560, base + 1900), (1400, 40, 5200), "metal", f, False)
        block("%s_Rib_%02d" % (key, i), (x + 780, -3480, base + 1900), (120, 120, 6000), "body", f, False)
        block("%s_Window_%02d" % (key, i), (x, -3520, base + 3300), (900, 20, 160), "white", f, False)
        block("%s_Status_%02d" % (key, i), (x - 560, -3520, base + 900), (90, 20, 90), ("red", "green", "blue")[i % 3], f, False)
    block(key + "_AccentBand", (mid, -3500, base + 2700), (w, 20, 40), accent, f, False)
    # Ceiling trusses with hanging lamps.
    block(key + "_RoofBeam", (mid, -1300, base + 4400), (w, 160, 180), "body", f, False)
    for i, x in enumerate(range(int(lo) + 800, int(hi), 1600)):
        block("%s_Truss_%02d" % (key, i), (x, -2300, base + 4400), (140, 2600, 160), "body", f, False)
        block("%s_Lamp_%02d" % (key, i), (x, -1300, base + 4180), (260, 160, 30), "white", f, False)
    # Flywheels turning on the back wall.
    gear(key + "_Flywheel_A", (lo + w * 0.28, -3150, base + 2900), 2600, 9.0, f)
    gear(key + "_Flywheel_B", (lo + w * 0.74, -3150, base + 700), 1800, -14.0, f)
    # Lower catwalk with hazard railings.
    block(key + "_Catwalk", (mid, -2450, base - 400), (w, 500, 60), "asphalt", f, False)
    for i, x in enumerate(range(int(lo) + 700, int(hi) - 400, 1500)):
        p = spawn(u.StaticMeshActor, "%s_Railing_%02d" % (key, i), (x, -2240, base - 330), f)
        p.static_mesh_component.set_static_mesh(pole_mesh)
        p.static_mesh_component.set_collision_profile_name("NoCollision")
        p.set_actor_scale3d(V(0.45, 0.45, 0.25))
    # Gantry crane carrying a robot shell back and forth.
    block(key + "_GantryRail", (mid, -2200, base + 3500), (w, 150, 150), "body", f, False)
    block(key + "_GantryStripe", (mid, -2110, base + 3500), (w, 10, 24), accent, f, False)
    trolley = mover(key + "_GantryTrolley", [(lo + 700, -2200, base + 3330), (hi - 700, -2200, base + 3330)], f,
                    size=(420, 260, 160), mat="body", speed=240.0, collision=False)
    cable = block(key + "_GantryCable", (lo + 700, -2200, base + 2900), (18, 18, 700), "body", f, False, True)
    attach(cable, trolley)
    held = robot(key + "_GantryRobot", robot_heavy, 520, (lo + 700, -2200, base + 2020), f)
    attach(held, trolley)
    # Assembly conveyor: robot shells ride between two press housings that hide the loop seam.
    belt_z = base + 160
    block(key + "_Belt", (mid, -1500, belt_z), (w - 300, 380, 100), "deck", f, False)
    block(key + "_BeltEdge", (mid, -1300, belt_z + 40), (w - 300, 14, 18), "amber", f, False)
    for i, x in enumerate(range(int(lo) + 500, int(hi) - 300, 900)):
        block("%s_BeltLeg_%02d" % (key, i), (x, -1500, belt_z - 800), (80, 80, 1500), "body", f, False)
    for side, hx in (("In", lo + 450), ("Out", hi - 450)):
        block("%s_Housing%s" % (key, side), (hx, -1500, base + 600), (760, 640, 1000), "body", f, False)
        block("%s_HousingLight%s" % (key, side), (hx, -1175, base + 900), (500, 10, 40), accent, f, False)
    lane = [(lo + 450, -1500, belt_z + 70), (hi - 450, -1500, belt_z + 70)]
    count = max(2, int(w / 1500))
    for n in range(count):
        carrier = mover("%s_Carrier_%02d" % (key, n), lane, f, size=(320, 260, 40), mat="body", mode="loop",
                        speed=150.0, progress=n / float(count), collision=False)
        # Place the robot where the carrier will be at BeginPlay, then attach it.
        cx = lane[0][0] + (lane[1][0] - lane[0][0]) * n / float(count)
        carrier.set_actor_location(V(cx, -1500, belt_z + 70), False, False)
        body = robot("%s_LineRobot_%02d" % (key, n), robot_heavy if n % 2 == 0 else robot_teleporter,
                     430 if n % 2 == 0 else 300, (cx, -1500, belt_z + 90), f)
        attach(body, carrier)
        report["robots_on_line"] += 1
    # Stamping presses above the belt.
    for i, x in enumerate(range(int(lo) + 1500, int(hi) - 1200, 2200)):
        block("%s_PressColumn_%02d" % (key, i), (x, -1650, base + 2900), (200, 200, 2600), "body", f, False)
        head = mover("%s_PressHead_%02d" % (key, i), [(x, -1500, base + 1350), (x, -1500, base + 950)], f,
                     size=(340, 340, 220), mat="body", speed=160.0 + 30 * (i % 3), collision=False)
        glow = block("%s_PressGlow_%02d" % (key, i), (x, -1500, base + 1235), (300, 300, 12), accent, f, False, True)
        attach(glow, head)
    light(key + "_Light_A", (lo + w * 0.3, -1200, base + 1700), ACCENT_RGB[accent])
    light(key + "_Light_B", (lo + w * 0.7, -1200, base + 1700), ACCENT_RGB["amber"], 45.0)


def section(name, lo, hi, base, accent, arena=False):
    zone = camera(name + "_Camera", lo, hi, base, arena)
    backdrop(name, lo, hi, base, accent)
    report["sections"].append({"name": name, "range": [lo, hi], "base_z": base, "arena": arena})
    return zone


def catch_wall(label, x, bottom, top, folder):
    """Closes the end of a catch floor so a fall can never respawn the player past an obstacle."""
    block(label, (x, 0, (bottom + top) / 2), (40, 500, top - bottom), "body", folder)


# ---------------------------------------------------------------- arenas
def arena(index, title, lo, hi, base, required, layouts, enemies, boosts, entry, exit_, shuttle=None):
    accent = {red: "red", green: "green", blue: "blue"}[required]
    folder = "%02d_%s" % (index * 10, title)
    centre = (lo + hi) / 2
    zone = section("A%d" % index, lo, hi, base, accent, True)
    tiles("A%d_RecoveryDeck" % index, lo, hi, base - 550, folder + "/Recovery", accent)
    tiles("A%d_Entry" % index, entry[0], entry[1], base, folder, accent)
    tiles("A%d_Exit" % index, exit_[0], exit_[1], base, folder, accent)
    platforms = []
    for n, (x, height, width, initial) in enumerate(layouts):
        platforms.append(color_platform("A%d_Puzzle_%02d" % (index, n + 1), x, base + height, width, initial, folder + "/Puzzle"))
        block("A%d_PadSupport_%02d" % (index, n + 1), (x, -300, base + height - 260), (width * 0.5, 160, 400), "body", folder, False)
        sign("A%d_PadNumber_%02d" % (index, n + 1), "%02d" % (n + 1), (x, -320, base + height + 155), 55, folder=folder)
    spawned = [enemy(k, "A%d_%s_%d" % (index, k, n + 1), x, base + z, folder + "/Enemies", c) for n, (k, x, z, c) in enumerate(enemies)]
    targets = []
    if any(e[0] == "teleporter" for e in enemies):
        for n, (x, height, width, _) in enumerate(layouts):
            for side in (-1, 1):
                targets.append(spawn(u.TargetPoint, "A%d_Teleport_%02d_%s" % (index, n + 1, side),
                                     (x + side * min(width / 3, 300), 0, base + height + 30), folder + "/TeleportPoints"))
    control = spawn("/Game/RGB/Blueprints/Map_Arena/BP_RGBArenaControl.BP_RGBArenaControl_C", "A%d_Control_%s" % (index, accent.upper()),
                    (centre, -410, base + 2000), folder + "/Rules")
    control.set_editor_property("required_color", required)
    control.set_editor_property("assigned_platforms", platforms)
    control.set_editor_property("assigned_enemies", spawned)
    control.set_editor_property("arena_camera_zone", zone)
    control.set_editor_property("use_arena_enemy_tuning", True)
    control.set_editor_property("teleport_target_points", targets)
    control.set_editor_property("scale_enemy_respawn_delay_with_arena_progress", True)
    control.set_editor_property("enemy_respawn_delay_at_no_progress", 12.0)
    control.set_editor_property("enemy_respawn_delay_near_completion", 7.0 if index == 1 else 5.0)
    tuning = control.get_editor_property("coordination_tuning")
    maximum = tuning.get_editor_property("maximum_attackers")
    maximum.set_editor_property("at_no_progress", 1.0)
    maximum.set_editor_property("near_completion", float(min(index + 1, 3)))
    tuning.set_editor_property("maximum_attackers", maximum)
    control.set_editor_property("coordination_tuning", tuning)
    gate_x = hi - 70
    gate = mover("A%d_CompletionShutter" % index, [(gate_x, 0, base + 2400), (gate_x, 0, base + 9400)], folder + "/Exit",
                 size=(180, 950, 7000), mat="body", mode="one_way", speed=1800.0, active=False)
    attach(block("A%d_ShutterStripe" % index, (gate_x, 480, base + 300), (190, 10, 60), "amber", folder + "/Exit", False, True), gate)
    opener = spawn("/Script/RGB.RGBArenaCompletionActivator", "A%d_OpenExitOnCompletion" % index, (hi - 400, -500, base), folder + "/Rules")
    opener.set_editor_property("arena_control", control)
    opener.set_editor_property("controlled_actors", [gate])
    if shuttle:
        (sx0, sx1, sh) = shuttle
        mover("A%d_Shuttle" % index, [(sx0, 0, base + sh + 6 - 25), (sx1, 0, base + sh + 6 - 25)], folder + "/Traversal",
              tile_mesh, speed=220.0)
    for label, x, z, speed in boosts:
        boost("A%d_%s" % (index, label), x, base + z, speed, folder + "/Traversal")
    block("A%d_Header" % index, (centre, -550, base + 3100), (hi - lo - 600, 50, 30), accent, folder, False)
    sign("A%d_Title" % index, "%02d / %s" % (index, title.upper().replace("_", " ")), (centre, -590, base + 2800), 150, folder=folder)
    sign("A%d_Goal" % index, "MATCH ALL %d PADS TO %s  /  EXIT UNLOCKS AT 100%%" % (len(platforms), accent.upper()),
         (centre, -590, base + 2540), 65, folder=folder)
    sign("A%d_ExitLabel" % index, "SEALED\nCOMPLETE ARENA", (hi - 220, 495, base + 500), 50, folder=folder)
    checkpoint("A%d_Checkpoint" % index, entry[0] + 200, base, lo, hi, folder + "/Recovery")
    report["arenas"].append({"index": index, "control": control.get_actor_label(), "range": [lo, hi], "base_z": base,
                             "target": accent, "platforms": len(platforms), "enemies": [[e[0], str(e[3])] for e in enemies],
                             "teleport_points": len(targets), "gate": gate.get_actor_label()})
    return control


# ================================================================ LEVEL LAYOUT (x right, z up)
# Player: jump 1120 cm/s, gravity x3 => ~213 cm rise, ~760 cm running jump. Steps <= 140, gaps <= 300.
# Jump pad launch speed v rises ~v*v/5880 cm (2300 ~ 900, 2400 ~ 980, 2800 ~ 1330, 3000 ~ 1530, 3500 ~ 2080).

# Lighting: dim factory interior with coloured practicals.
sun = spawn(u.DirectionalLight, "Works_KeyLight", (0, 0, 6000), rotation=R(pitch=-52, yaw=-35, roll=0))
sun.light_component.set_mobility(u.ComponentMobility.MOVABLE)
sun.light_component.set_intensity(3.0)
sky = spawn(u.SkyLight, "Works_SkyFill", (0, 0, 3000))
sky.light_component.set_mobility(u.ComponentMobility.MOVABLE)
sky.light_component.set_editor_property("intensity", 2.0)
sky.light_component.set_editor_property("real_time_capture", True)
spawn(u.SkyAtmosphere, "Works_Atmosphere")
fog = spawn(u.ExponentialHeightFog, "Works_Haze", (0, 0, -1500))
fog.component.set_editor_property("fog_density", 0.012)
pp = spawn(u.PostProcessVolume, "Works_PostProcess")
if pp_settings:
    pp.set_editor_property("settings", pp_settings)
pp.set_editor_property("unbound", True)
# Molten floor far below the play space (visual only; falls are caught at z -2000).
block("Works_MoltenFloor", (19300, -1000, -2750), (46000, 7000, 100), "lava", "00_Environment", False)
for i, x in enumerate(range(0, 41000, 6000)):
    light("Works_MoltenGlow_%02d" % i, (x, -1000, -2200), (1.0, 0.35, 0.05), 250.0, 5000.0)
# Level boundaries.
block("Works_WallLeft", (-2450, 0, 1500), (100, 800, 6000), "body", "00_Environment")
block("Works_WallRight", (41050, 0, 4000), (100, 800, 6000), "body", "00_Environment")

# ---- S0 ARRIVAL: Intake Bay (-2400 .. 2000, z 0)
F = "02_Arrival"
section("Arrival", -2400, 2000, 0, "amber")
tiles("Arrival_Floor", -2400, 2000, 0, F)
spawn(u.PlayerStart, "PlayerStart_Works", (-2000, 0, 115), F)
checkpoint("Arrival_Checkpoint", -1900, 0, -2400, 2000, F)
# Backup unlocks: each pickup removes itself if the ability was already collected in this session
# (normal play arriving from Level 2). They only appear when Level 3 is started on its own.
for i, bp in enumerate(("BP_RGBDashPickup", "BP_RGBRedWeaponPickup", "BP_RGBGreenWeaponPickup")):
    p = spawn("/Game/RGB/Blueprints/Progression/" + bp + "." + bp + "_C", "Backup_" + bp, (-1400 + i * 400, 0, 85), F)
    p.get_component_by_class(u.SphereComponent).set_sphere_radius(300)
sign("Works_Title", "THE CHROMA WORKS", (-200, -600, 1350), 190, folder=F)
sign("Works_Brief", "ROBOT ASSEMBLY HALL 03  /  SOMEWHERE IN HERE THEY ARE MAKING BLUE", (-200, -590, 1000), 55, folder=F)
sign("Works_Hint", "AMBER STRIPES MARK MOVING MACHINERY\nPLATES FLIP - CROSS BETWEEN STAMPS", (900, -590, 620), 50, folder=F)

# ---- R1 STAMPING LINE (2000 .. 6800, z 0): flipping plates, a shuttle, then a launch up.
F = "05_Stamping_Line"
section("R1", 2000, 6800, 0, "amber")
checkpoint("R1_Checkpoint", 1700, 0, 2000, 6800, F)
for n, x in enumerate((2500, 3200)):
    flip_plate("R1_FlipPlate_%d" % (n + 1), x, 0, F)
tiles("R1_Island", 3700, 4100, 0, F)
mover("R1_Shuttle", [(4550, 0, -25), (5650, 0, -25)], F, tile_mesh, speed=350.0)
tiles("R1_ExitLedge", 6000, 6800, 0, F)
boost("R1_LaunchToAssembly", 6600, 0, 2400, F)
tiles("R1_CatchFloor", 2000, 6000, -600, F)
boost("R1_RecoveryBack", 2250, -600, 2400, F)
boost("R1_RecoveryForward", 5800, -600, 2400, F)
sign("R1_Sign", "STAMPING LINE\nWAIT FOR THE PLATES TO SETTLE", (4300, -590, 1500), 70, folder=F)

# ---- A1 ASSEMBLY LINE, target RED, no blue anywhere (6800 .. 13000, z 600)
arena(1, "Assembly_Line", 6800, 13000, 600, red,
      [(8300, 0, 1100, green), (9900, 0, 1100, blue), (11500, 0, 1100, green), (9100, 1000, 800, blue), (10700, 1000, 800, green)],
      [("ground", 8300, 0, red), ("ground", 11500, 0, green), ("flying", 9900, 1500, green)],
      [("Boost_LeftUpper", 8600, 0, 2800), ("Boost_RightUpper", 11200, 0, 2800),
       ("RecoveryBoost_Left", 9100, -550, 2300), ("RecoveryBoost_Right", 10700, -550, 2300)],
      (6800, 7700), (12100, 13000), shuttle=(9720, 10080, 1000))

# ---- R2 THE CLIMB (13000 .. 16800, z 600 -> 1800): lift, then flipping plates over a catch floor.
F = "15_The_Climb"
section("R2", 13000, 16800, 600, "amber")
tiles("R2_Start", 13000, 13800, 600, F)
checkpoint("R2_Checkpoint", 13300, 600, 13000, 16800, F)
mover("R2_Lift", [(14100, 0, 575), (14100, 0, 1775)], F, tile_mesh, speed=300.0)
block("R2_LiftShaft", (14100, -330, 1500), (440, 60, 2400), "body", F, False)
tiles("R2_UpperLedge", 14400, 15000, 1800, F)
for n, x in enumerate((15300, 15900, 16500)):
    flip_plate("R2_FlipPlate_%d" % (n + 1), x, 1800, F, 2.8)
tiles("R2_CatchFloor", 15000, 16800, 1200, F)
catch_wall("R2_CatchWall", 16790, 1150, 1750, F)
boost("R2_RecoveryBack", 15050, 1200, 2300, F)
boost("R2_RecoveryForward", 16750, 1200, 2300, F)
sign("R2_Sign", "SERVICE LIFT  /  CHROMA CORE AHEAD", (14700, -590, 2900), 70, folder=F)

# ---- CORE: the blue weapon (16800 .. 20000, z 1800)
F = "18_Chroma_Core"
section("Core", 16800, 20000, 1800, "blue")
tiles("Core_Floor", 16800, 20000, 1800, F, "blue")
checkpoint("Core_Checkpoint", 17100, 1800, 16800, 20000, F)
block("Core_Dais", (18400, 0, 1860), (900, 500, 120), "deck", F)
block("Core_DaisLight", (18400, 255, 1905), (880, 10, 20), "blue", F, False)
blue_pickup = spawn("/Game/RGB/Blueprints/Progression/BP_RGBBlueWeaponPickup.BP_RGBBlueWeaponPickup_C", "Core_BlueWeapon", (18400, 0, 2005), F)
blue_pickup.get_component_by_class(u.SphereComponent).set_sphere_radius(260)
for side in (-1, 1):
    block("Core_Containment_%d" % side, (18400 + side * 650, -150, 2700), (120, 120, 1800), "blue", F, False)
    block("Core_ContainmentCap_%d" % side, (18400 + side * 650, -150, 3640), (260, 260, 80), "body", F, False)
gear("Core_Ring_Outer", (18400, -700, 2900), 3200, 18.0, F)
gear("Core_Ring_Inner", (18400, -600, 2900), 1700, -30.0, F)
light("Core_BlueHeart", (18400, -300, 2600), ACCENT_RGB["blue"], 220.0, 3500.0, F)
sign("Core_Title", "CHROMA CORE", (18400, -590, 3450), 170, color=(120, 170, 255), folder=F)
sign("Core_Brief", "PROTOTYPE SPECTRUM: BLUE\nTAKE IT", (18400, -590, 3150), 60, color=(160, 200, 255), folder=F)
sign("Core_After", "BLUE ROBOTS IGNORE RED AND GREEN.\nSWITCH TO BLUE TO STOP THEM.", (19600, -590, 2700), 50, folder=F)

# ---- A2 CALIBRATION, target BLUE, every enemy is blue (20000 .. 26400, z 1800)
arena(2, "Calibration", 20000, 26400, 1800, blue,
      [(21500, 0, 1100, red), (23100, 0, 1100, green), (24700, 0, 1100, red), (22300, 1000, 800, green), (23900, 1000, 800, red),
       (23100, 1900, 700, green)],
      [("teleporter", 21500, 0, blue), ("teleporter", 24700, 0, blue), ("flying", 23100, 1500, blue)],
      [("Boost_LeftUpper", 21800, 0, 2800), ("Boost_RightUpper", 24400, 0, 2800),
       ("Boost_ApexLeft", 22550, 1000, 2700), ("Boost_ApexRight", 23650, 1000, 2700),
       ("RecoveryBoost_Left", 22300, -550, 2300), ("RecoveryBoost_Right", 23900, -550, 2300)],
      (20000, 20800), (25400, 26400), shuttle=(22920, 23280, 1000))

# ---- R3 THE PATERNOSTER (26400 .. 30500, z 1800 -> 2600): a ring of platforms carries you up.
F = "25_The_Paternoster"
section("R3", 26400, 30500, 1800, "amber")
tiles("R3_Start", 26400, 27400, 1800, F)
checkpoint("R3_Checkpoint", 26700, 1800, 26400, 29000, F)
WHEEL_X, WHEEL_Z, WHEEL_R = 28100, 2500, 700
# Clockwise as seen by the camera: up the left side, over the top, down to the exit ledge on the right.
ring = [(WHEEL_X + WHEEL_R * math.cos(math.radians(a)), 0, WHEEL_Z + WHEEL_R * math.sin(math.radians(a)) - 25) for a in range(360, 0, -45)]
for n in range(4):
    mover("R3_WheelCar_%d" % (n + 1), ring, F, tile_mesh, mode="loop", speed=300.0, progress=n * 0.25, closed=True, curve=True)
gear("R3_WheelHub", (WHEEL_X, -260, WHEEL_Z), 900, -25.0, F)
for side in (-1, 1):
    block("R3_WheelLeg_%d" % side, (WHEEL_X + side * 380, -330, 1500), (90, 90, 2200), "body", F, False)
block("R3_WheelRim", (WHEEL_X, -380, WHEEL_Z), (1600, 40, 1600), "back", F, False)
tiles("R3_CatchFloor", 27400, 29000, 1200, F)
catch_wall("R3_CatchWall", 28990, 1150, 2200, F)
boost("R3_RecoveryBack", 27550, 1200, 2300, F)
tiles("R3_Ledge", 29050, 30500, 2600, F)
checkpoint("R3_LedgeCheckpoint", 29400, 2600, 29000, 30500, F)
sign("R3_Sign", "PATERNOSTER\nRIDE UP, STEP OFF AT THE TOP RIGHT", (27300, -590, 3300), 60, folder=F)

# ---- A3 FINAL INSPECTION, target GREEN, enemies of all three colours (30500 .. 37300, z 2600)
# Platform/boost geometry is the PIE-verified Chromatic Foundry Azure Core, moved +10000 x and +800 z.
arena(3, "Final_Inspection", 30500, 37300, 2600, green,
      [(31700, 0, 1600, red), (33900, 0, 1600, blue), (36100, 0, 1600, red), (32000, 1100, 1000, blue), (33900, 1700, 1000, red),
       (35800, 1100, 1000, blue), (35200, 2770, 1100, red)],
      [("ground", 31700, 0, red), ("ground", 36100, 0, blue), ("teleporter", 32000, 1100, blue), ("teleporter", 35800, 1100, red),
       ("flying", 33900, 2200, green)],
      [("Boost_LeftUpper", 31050, 0, 3500), ("Boost_RightUpper", 36750, 0, 3500), ("Boost_CentreLeft", 32200, 1100, 3000),
       ("Boost_CentreRight", 35600, 1100, 3000), ("Boost_Apex", 33700, 1700, 3000),
       ("RecoveryBoost_Left", 32800, -550, 2300), ("RecoveryBoost_Right", 35000, -550, 2300)],
      (30500, 31100), (36600, 37300))

# ---- DEPARTURE (37300 .. 41000, z 2600)
F = "40_Departure"
section("Departure", 37300, 41000, 2600, "blue")
tiles("Departure_Floor", 37300, 41000, 2600, F, "blue")
checkpoint("Departure_Checkpoint", 37800, 2600, 37300, 41000, F)
sign("Departure_End", "SHIFT COMPLETE\nRED  GREEN  BLUE", (39000, -590, 3900), 140, folder=F)
sign("Departure_Exit", "RETURN TO MAIN MENU  >", (39800, -590, 3300), 65, folder=F)
portal = spawn("/Script/RGB.RGBLevelTransitionArea", "Departure_ReturnToMenu", (40400, 0, 2900), F)
portal.set_editor_property("destination_level", asset("/Game/RGB/Maps/L_MainMenu"))
portal.set_editor_property("fade_out_duration", 2.0)
component(portal, "TransitionTrigger").set_box_extent(V(150, 350, 450))
for x in (40150, 40650):
    block("Departure_PortalFrame_%d" % x, (x, -200, 3050), (90, 120, 900), "blue", F, False)

# ---------------------------------------------------------------- save and record
assert levels.save_current_level()
report["actor_count"] = len(actors.get_all_level_actors())
report["references_unchanged"] = all(refs[n] == sha(os.path.join(ROOT, "Content/RGB/Maps", n + ".umap")) for n in refs)
assert report["references_unchanged"], "A reference map changed on disk"
report["saved_map_sha256"] = sha(MAP_FILE)
with open(REPORT, "w", encoding="utf-8") as f:
    json.dump(report, f, indent=2)
u.log("RGB_L3_CREATED: %s, %d actors, references unchanged" % (MAP, report["actor_count"]))
