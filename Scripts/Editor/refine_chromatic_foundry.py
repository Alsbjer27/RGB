"""Apply visual QA corrections to this new map only, through the editor."""
import unreal as u
levels = u.get_editor_subsystem(u.LevelEditorSubsystem)
assert not levels.is_in_play_in_editor(), "End PIE before applying the authoring correction"
levels.load_level("/Game/RGB/Maps/L_ChromaticFoundry")
actors = u.get_editor_subsystem(u.EditorActorSubsystem)
by_label = {a.get_actor_label(): a for a in actors.get_all_level_actors()}
for actor in actors.get_all_level_actors():
    if isinstance(actor, u.RGBArenaCameraZone):
        actor.get_component_by_class(u.CameraComponent).set_editor_property("relative_rotation", u.Rotator(pitch=0, yaw=-90, roll=0))
    if isinstance(actor, u.TextRenderActor):
        actor.set_actor_rotation(u.Rotator(pitch=0, yaw=90, roll=0), False)
    if isinstance(actor, (u.DirectionalLight, u.SkyLight)):
        actor.light_component.set_mobility(u.ComponentMobility.MOVABLE)
    if actor.get_actor_label() == "Foundry_KeyLight":
        actor.set_actor_rotation(u.Rotator(pitch=-48, yaw=-35, roll=0), False)
    if actor.get_actor_label() in ("A3_Title", "A3_Goal", "A3_Control_BLUE"):
        location = actor.get_actor_location()
        actor.set_actor_location(u.Vector(22300, location.y, location.z), False, False)

for n, target_z in ((1, 140), (2, 280), (3, 420)):
    prefix = "Route01_Step%d" % n
    delta_z = target_z - (by_label[prefix].get_actor_location().z + 90)
    for label, actor in by_label.items():
        if label.startswith(prefix):
            p = actor.get_actor_location()
            actor.set_actor_location(u.Vector(p.x, p.y, p.z + delta_z), False, False)

shuttle = by_label["Route02_Shuttle"]
shuttle.set_actor_location(u.Vector(18150, 0, 1030), False, False)
spline = by_label["Route02_Shuttle_Path"].get_component_by_class(u.SplineComponent)
spline.set_location_at_spline_point(0, u.Vector(18150, 0, 1030), u.SplineCoordinateSpace.WORLD, False)
spline.set_location_at_spline_point(1, u.Vector(19150, 0, 1580), u.SplineCoordinateSpace.WORLD, True)

for n in (1, 2, 3):
    platform = by_label["A3_Puzzle_%02d" % n]
    scale = platform.get_actor_scale3d()
    platform.set_actor_scale3d(u.Vector(4, scale.y, scale.z))
apex = by_label["A3_Puzzle_07"]
apex.set_actor_location(u.Vector(25200, 0, 4570), False, False)
apex.set_actor_scale3d(u.Vector(2.75, 1.5, 1.2))
for label in ("A3_PadSupport_07", "A3_PadNumber_07"):
    actor = by_label[label]
    p = actor.get_actor_location()
    actor.set_actor_location(u.Vector(25200, p.y, p.z), False, False)
for side in (-1, 1):
    actor = by_label["A3_Teleport_07_%s" % side]
    p = actor.get_actor_location()
    actor.set_actor_location(u.Vector(25200 + side * 300, p.y, p.z), False, False)

boosts = [
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
]
for label, x, z, speed, folder in boosts:
    actor = by_label.get(label)
    if not actor:
        actor = actors.spawn_actor_from_class(u.load_class(None, "/Game/RGB/Blueprints/Platforms/BP_RGBJumpPad_Visual.BP_RGBJumpPad_Visual_C"), u.Vector(x, 0, z + 10))
        actor.set_actor_label(label)
        actor.set_folder_path(folder)
    actor.set_editor_property("launch_speed", float(speed))
    actor.set_actor_location(u.Vector(x, 0, z + 10), False, False)
by_label["Foundry_Hint"].text_render.set_text("LAND TO CYCLE: RED > GREEN > BLUE\nMATCH YOUR SHOTS TO THE ENEMY\nLAND ON ROUND PADS TO BOOST")
by_label["Route01_Sign"].text_render.set_text("TRANSFER GANTRY  >\nUSE THE BOOST TO REACH VERDANT RELAY")
for actor in actors.get_all_level_actors():
    if isinstance(actor, u.RGBColorPlatform):
        colour = next(c for c in actor.get_components_by_class(u.ActorComponent) if c.get_name() == "ColorComponent").get_editor_property("initial_color")
        name = {u.RGBColor.RED: "Red", u.RGBColor.GREEN: "Green", u.RGBColor.BLUE: "Blue"}[colour]
        mesh = next(c for c in actor.get_components_by_class(u.StaticMeshComponent) if c.get_name() == "PlatformMesh")
        mesh.set_material(actor.get_editor_property("color_material_index"), u.load_asset("/Game/RGB/Assets/Platforms/Materials/MI_PlatformLarge_" + name))
assert levels.save_current_level()
u.EditorLevelLibrary.set_level_viewport_camera_info(u.Vector(4500, 7600, 1900), u.Rotator(pitch=-6, yaw=-90, roll=0))
u.log("RGB_FOUNDRY_VISUAL_REFINEMENT_SAVED")
