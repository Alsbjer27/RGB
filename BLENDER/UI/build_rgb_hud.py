import bpy
import math
import os
from mathutils import Vector


BLEND_PATH = r"C:\Users\emilr\Desktop\SpelProjekt\RGB\BLENDER\UI\RGB_HUD_UI.blend"
OUTPUT_DIR = r"C:\Users\emilr\Desktop\SpelProjekt\RGB\BLENDER\UI\Renders"


def clear_scene():
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)
    for datablocks in (bpy.data.meshes, bpy.data.curves, bpy.data.materials,
                       bpy.data.cameras, bpy.data.lights):
        for datablock in list(datablocks):
            if datablock.users == 0:
                datablocks.remove(datablock)
    for collection in list(bpy.data.collections):
        bpy.data.collections.remove(collection)


def make_collection(name):
    collection = bpy.data.collections.new(name)
    bpy.context.scene.collection.children.link(collection)
    return collection


def link_object(obj, collection):
    for current in list(obj.users_collection):
        current.objects.unlink(obj)
    collection.objects.link(obj)


def make_material(name, base_color, metallic=0.0, roughness=0.4,
                  emission_color=None, emission_strength=0.0, alpha=1.0):
    material = bpy.data.materials.new(name)
    material.use_nodes = True
    material.diffuse_color = (*base_color[:3], alpha)
    material.surface_render_method = "DITHERED" if alpha < 1.0 else "DITHERED"
    bsdf = material.node_tree.nodes.get("Principled BSDF")
    bsdf.inputs["Base Color"].default_value = (*base_color[:3], 1.0)
    bsdf.inputs["Metallic"].default_value = metallic
    bsdf.inputs["Roughness"].default_value = roughness
    if "Alpha" in bsdf.inputs:
        bsdf.inputs["Alpha"].default_value = alpha
    if emission_color is not None:
        emission_input = bsdf.inputs.get("Emission Color") or bsdf.inputs.get("Emission")
        if emission_input:
            emission_input.default_value = (*emission_color[:3], 1.0)
        strength_input = bsdf.inputs.get("Emission Strength")
        if strength_input:
            strength_input.default_value = emission_strength
    return material


def chamfer_points(width, height, chamfer):
    half_width = width * 0.5
    half_height = height * 0.5
    chamfer = min(chamfer, half_width * 0.8, half_height * 0.8)
    return [
        (-half_width + chamfer, half_height),
        (half_width - chamfer, half_height),
        (half_width, half_height - chamfer),
        (half_width, -half_height + chamfer),
        (half_width - chamfer, -half_height),
        (-half_width + chamfer, -half_height),
        (-half_width, -half_height + chamfer),
        (-half_width, half_height - chamfer),
    ]


def create_chamfer_ring(name, width, height, inset, chamfer, depth, location,
                        material, collection, bevel_width=0.0, bevel_segments=2):
    outer = chamfer_points(width, height, chamfer)
    inner = chamfer_points(width - inset * 2.0, height - inset * 2.0,
                           max(chamfer - inset * 0.45, 0.015))
    z_bottom = -depth * 0.5
    z_top = depth * 0.5
    vertices = []
    for z in (z_top, z_bottom):
        vertices.extend([(x, y, z) for x, y in outer])
        vertices.extend([(x, y, z) for x, y in inner])

    outer_top = 0
    inner_top = 8
    outer_bottom = 16
    inner_bottom = 24
    faces = []
    for index in range(8):
        next_index = (index + 1) % 8
        faces.append((outer_top + index, outer_top + next_index,
                      inner_top + next_index, inner_top + index))
        faces.append((outer_bottom + index, inner_bottom + index,
                      inner_bottom + next_index, outer_bottom + next_index))
        faces.append((outer_bottom + index, outer_bottom + next_index,
                      outer_top + next_index, outer_top + index))
        faces.append((inner_bottom + index, inner_top + index,
                      inner_top + next_index, inner_bottom + next_index))

    mesh = bpy.data.meshes.new(f"{name}_Mesh")
    mesh.from_pydata(vertices, [], faces)
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    collection.objects.link(obj)
    obj.location = location
    obj.data.materials.append(material)
    obj["quad_topology"] = all(len(face.vertices) == 4 for face in mesh.polygons)
    obj["ui_role"] = "frame"
    if bevel_width > 0.0:
        bevel = obj.modifiers.new("Small manufactured bevel", "BEVEL")
        bevel.width = bevel_width
        bevel.segments = bevel_segments
        bevel.limit_method = "ANGLE"
    return obj


def create_chamfer_panel(name, width, height, chamfer, depth, location,
                         material, collection, bevel_width=0.0):
    points = chamfer_points(width, height, chamfer)
    z_bottom = -depth * 0.5
    z_top = depth * 0.5
    vertices = [(x, y, z_top) for x, y in points] + [(x, y, z_bottom) for x, y in points]
    faces = [
        (0, 1, 2, 7),
        (7, 2, 3, 6),
        (6, 3, 4, 5),
        (8, 15, 10, 9),
        (15, 14, 11, 10),
        (14, 13, 12, 11),
    ]
    for index in range(8):
        next_index = (index + 1) % 8
        faces.append((8 + index, 8 + next_index, next_index, index))
    mesh = bpy.data.meshes.new(f"{name}_Mesh")
    mesh.from_pydata(vertices, [], faces)
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    collection.objects.link(obj)
    obj.location = location
    obj.data.materials.append(material)
    obj["quad_topology"] = all(len(face.vertices) == 4 for face in mesh.polygons)
    if bevel_width > 0.0:
        bevel = obj.modifiers.new("Edge bevel", "BEVEL")
        bevel.width = bevel_width
        bevel.segments = 2
        bevel.limit_method = "ANGLE"
    return obj


def create_shield(name, center_x, material_outer, material_inner, collection, depleted=False):
    rows = [
        (-0.152, 0.335, 0.152),
        (-0.171, 0.145, 0.171),
        (-0.121, -0.090, 0.121),
        (-0.020, -0.335, 0.020),
    ]

    def shield_prism(object_name, row_data, depth, z, material, bevel_width):
        top_vertices = []
        bottom_vertices = []
        for left_x, y, right_x in row_data:
            top_vertices.extend([(left_x, y, depth * 0.5), (right_x, y, depth * 0.5)])
            bottom_vertices.extend([(left_x, y, -depth * 0.5), (right_x, y, -depth * 0.5)])
        vertices = top_vertices + bottom_vertices
        faces = []
        for row in range(3):
            a = row * 2
            b = a + 1
            c = (row + 1) * 2 + 1
            d = (row + 1) * 2
            faces.append((a, b, c, d))
            faces.append((8 + d, 8 + c, 8 + b, 8 + a))
        perimeter = [0, 1, 3, 5, 7, 6, 4, 2]
        for index in range(len(perimeter)):
            current = perimeter[index]
            following = perimeter[(index + 1) % len(perimeter)]
            faces.append((8 + current, 8 + following, following, current))
        mesh = bpy.data.meshes.new(f"{object_name}_Mesh")
        mesh.from_pydata(vertices, [], faces)
        mesh.update()
        obj = bpy.data.objects.new(object_name, mesh)
        collection.objects.link(obj)
        obj.location = (center_x, 0.0, z)
        obj.data.materials.append(material)
        obj["quad_topology"] = all(len(face.vertices) == 4 for face in mesh.polygons)
        obj["ui_role"] = "health_pip"
        bevel = obj.modifiers.new("Shield bevel", "BEVEL")
        bevel.width = bevel_width
        bevel.segments = 2
        bevel.limit_method = "ANGLE"
        return obj

    outer = shield_prism(f"{name}_Outer", rows, 0.075, 0.135, material_outer, 0.018)
    inner_rows = [(left * 0.76, y * 0.76, right * 0.76) for left, y, right in rows]
    inner = shield_prism(f"{name}_Glow", inner_rows, 0.035, 0.188, material_inner, 0.010)
    outer["depleted_preview"] = depleted
    inner["depleted_preview"] = depleted
    return outer, inner


def create_camera(name, location, ortho_scale, collection):
    data = bpy.data.cameras.new(name)
    data.type = "ORTHO"
    data.ortho_scale = ortho_scale
    obj = bpy.data.objects.new(name, data)
    collection.objects.link(obj)
    obj.location = location
    obj.rotation_euler = (0.0, 0.0, 0.0)
    return obj


def create_area_light(name, location, energy, color, size, collection):
    data = bpy.data.lights.new(name, "AREA")
    data.energy = energy
    data.color = color
    data.shape = "RECTANGLE"
    data.size = size
    data.size_y = size
    obj = bpy.data.objects.new(name, data)
    collection.objects.link(obj)
    obj.location = location
    direction = Vector((0.0, 0.0, 0.0)) - obj.location
    obj.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()
    return obj


def set_render_visibility(visible_names):
    visible_names = set(visible_names)
    for obj in bpy.context.scene.objects:
        if obj.type == "MESH":
            obj.hide_render = obj.name not in visible_names


def render_asset(scene, camera, resolution, filepath, visible_names):
    scene.camera = camera
    scene.render.resolution_x, scene.render.resolution_y = resolution
    scene.render.resolution_percentage = 100
    scene.render.filepath = filepath
    set_render_visibility(visible_names)
    bpy.ops.render.render(write_still=True)


clear_scene()
scene = bpy.context.scene
try:
    scene.render.engine = "BLENDER_EEVEE_NEXT"
except Exception:
    try:
        scene.render.engine = "BLENDER_EEVEE"
    except Exception:
        pass
scene.render.film_transparent = True
scene.render.image_settings.file_format = "PNG"
scene.render.image_settings.color_mode = "RGBA"
scene.render.resolution_percentage = 100
scene.render.image_settings.color_depth = "8"
scene.render.filepath = ""
scene.world.color = (0.004, 0.006, 0.010)
try:
    scene.view_settings.look = "AgX - Medium High Contrast"
except Exception:
    pass

arena_collection = make_collection("HUD_Arena")
health_collection = make_collection("HUD_Health")
preview_collection = make_collection("HUD_Preview")
camera_collection = make_collection("HUD_Cameras")
light_collection = make_collection("HUD_Lights")

gunmetal = make_material("M_HUD_Gunmetal", (0.035, 0.045, 0.060), metallic=0.88, roughness=0.23)
silver = make_material("M_HUD_SilverHighlight", (0.32, 0.42, 0.55), metallic=0.96, roughness=0.16)
ceramic = make_material("M_HUD_BlackCeramic", (0.004, 0.008, 0.014), metallic=0.28, roughness=0.27)
blue_glass = make_material("M_HUD_LiquidPreview", (0.005, 0.12, 0.55), metallic=0.05, roughness=0.18,
                           emission_color=(0.01, 0.22, 1.0), emission_strength=3.2, alpha=0.92)
red_metal = make_material("M_HUD_ShieldMetal", (0.12, 0.006, 0.012), metallic=0.72, roughness=0.23)
red_glow = make_material("M_HUD_ShieldGlow", (0.34, 0.003, 0.009), metallic=0.05, roughness=0.16,
                         emission_color=(1.0, 0.006, 0.012), emission_strength=1.8)
red_glow_dim = make_material("M_HUD_ShieldGlow_Depleted", (0.10, 0.008, 0.012), metallic=0.08, roughness=0.35,
                             emission_color=(0.18, 0.004, 0.008), emission_strength=0.25)
white_mask = make_material("M_HUD_WhiteMask", (1.0, 1.0, 1.0), metallic=0.0, roughness=1.0,
                           emission_color=(1.0, 1.0, 1.0), emission_strength=1.0)

# 600x80 UMG proportions expressed as 7.5 x 1 Blender units.
arena_center_x = -0.8125
health_center_x = 3.0125

arena_main = create_chamfer_ring("Arena_Frame_Main", 5.875, 1.0, 0.082, 0.145, 0.105,
                                 (arena_center_x, 0.0, 0.075), gunmetal, arena_collection, 0.022, 3)
arena_highlight = create_chamfer_ring("Arena_Frame_Highlight", 5.79, 0.915, 0.018, 0.112, 0.018,
                                      (arena_center_x, 0.0, 0.142), silver, arena_collection, 0.006, 2)
arena_back = create_chamfer_panel("Arena_Backplate", 5.70, 0.82, 0.095, 0.045,
                                  (arena_center_x, 0.0, 0.015), ceramic, preview_collection, 0.012)
arena_liquid = create_chamfer_panel("Arena_LiquidPreview", 3.09, 0.73, 0.080, 0.026,
                                    (arena_center_x - 1.26, 0.0, 0.075), blue_glass, preview_collection, 0.010)
arena_mask = create_chamfer_panel("Arena_LiquidMask", 5.61, 0.73, 0.080, 0.010,
                                  (arena_center_x, 0.0, 0.02), white_mask, preview_collection, 0.0)
arena_mask.hide_render = True
arena_mask.hide_viewport = True
arena_mask["ui_role"] = "liquid_mask_source"

health_main = create_chamfer_ring("Health_Frame_Main", 1.475, 1.0, 0.082, 0.145, 0.105,
                                  (health_center_x, 0.0, 0.075), gunmetal, health_collection, 0.022, 3)
health_highlight = create_chamfer_ring("Health_Frame_Highlight", 1.39, 0.915, 0.018, 0.112, 0.018,
                                       (health_center_x, 0.0, 0.142), silver, health_collection, 0.006, 2)
health_back = create_chamfer_panel("Health_Backplate", 1.30, 0.82, 0.085, 0.045,
                                   (health_center_x, 0.0, 0.015), ceramic, preview_collection, 0.012)

pip_centers = (2.62, 3.0125, 3.405)
pip_1 = create_shield("Health_Pip_1", pip_centers[0], red_metal, red_glow, health_collection)
pip_2 = create_shield("Health_Pip_2", pip_centers[1], red_metal, red_glow, health_collection)
pip_3 = create_shield("Health_Pip_3", pip_centers[2], red_metal, red_glow_dim, health_collection, depleted=True)

camera_full = create_camera("CAM_HUD_Full", (0.0, 0.0, 8.0), 7.62, camera_collection)
camera_arena = create_camera("CAM_ArenaFrame", (arena_center_x, 0.0, 8.0), 5.94, camera_collection)
camera_health = create_camera("CAM_HealthFrame", (health_center_x, 0.0, 8.0), 1.50, camera_collection)
camera_shield = create_camera("CAM_HealthShield", (pip_centers[0], 0.0, 8.0), 0.82, camera_collection)

create_area_light("Key_Light", (-4.0, -3.0, 6.0), 720.0, (0.58, 0.72, 1.0), 4.0, light_collection)
create_area_light("Rim_Light", (4.5, 2.5, 4.5), 480.0, (0.20, 0.42, 1.0), 3.0, light_collection)
create_area_light("Soft_Fill", (0.0, 4.0, 5.0), 280.0, (1.0, 0.20, 0.16), 5.0, light_collection)

for obj in (arena_main, arena_highlight, health_main, health_highlight,
            *pip_1, *pip_2, *pip_3):
    obj["export_ready"] = True

os.makedirs(OUTPUT_DIR, exist_ok=True)

arena_frame_names = {"Arena_Frame_Main", "Arena_Frame_Highlight"}
health_frame_names = {"Health_Frame_Main", "Health_Frame_Highlight"}
shield_names = {"Health_Pip_1_Outer", "Health_Pip_1_Glow"}
mask_names = {"Arena_LiquidMask"}
preview_names = {
    "Arena_Frame_Main", "Arena_Frame_Highlight", "Arena_Backplate", "Arena_LiquidPreview",
    "Health_Frame_Main", "Health_Frame_Highlight", "Health_Backplate",
    "Health_Pip_1_Outer", "Health_Pip_1_Glow",
    "Health_Pip_2_Outer", "Health_Pip_2_Glow",
    "Health_Pip_3_Outer", "Health_Pip_3_Glow",
}

# Transparent production renders.
render_asset(scene, camera_arena, (1880, 320), os.path.join(OUTPUT_DIR, "T_UI_ArenaFrame.png"), arena_frame_names)
render_asset(scene, camera_health, (472, 320), os.path.join(OUTPUT_DIR, "T_UI_HealthFrame.png"), health_frame_names)
render_asset(scene, camera_shield, (256, 512), os.path.join(OUTPUT_DIR, "T_UI_HealthShield.png"), shield_names)

# Flat white mask used to clip the Unreal liquid material to the inner reservoir silhouette.
original_engine = scene.render.engine
try:
    scene.render.engine = "BLENDER_WORKBENCH"
    scene.display.shading.light = "FLAT"
    scene.display.shading.color_type = "MATERIAL"
    scene.display.shading.show_shadows = False
    scene.display.shading.show_cavity = False
    arena_mask.hide_render = False
    arena_mask.hide_viewport = False
    render_asset(scene, camera_arena, (1880, 320), os.path.join(OUTPUT_DIR, "T_UI_ArenaLiquidMask.png"), mask_names)
finally:
    scene.render.engine = original_engine

# Full presentation preview.
render_asset(scene, camera_full, (2400, 320), os.path.join(OUTPUT_DIR, "RGB_HUD_Preview.png"), preview_names)

# Restore authoring state and make the full camera active when the file opens.
for obj in scene.objects:
    obj.hide_render = False
arena_mask.hide_render = True
arena_mask.hide_viewport = True
scene.camera = camera_full
scene.render.resolution_x = 2400
scene.render.resolution_y = 320
scene.render.resolution_percentage = 100
scene.render.filepath = os.path.join(OUTPUT_DIR, "RGB_HUD_Preview.png")

for obj in scene.objects:
    obj.select_set(False)
arena_main.select_set(True)
bpy.context.view_layer.objects.active = arena_main

readme = bpy.data.texts.get("README_HUD_ASSET") or bpy.data.texts.new("README_HUD_ASSET")
readme.clear()
readme.write(
    "RGB HUD Blender source\n"
    "======================\n"
    "All frame and shield source meshes use quad topology.\n"
    "HUD_Arena contains the arena frame. HUD_Health contains the health frame and pips.\n"
    "HUD_Preview contains non-export backplates, a liquid preview, and the hidden white mask source.\n"
    "The third shield is intentionally shown depleted in the full preview.\n"
    "Transparent production renders are stored in BLENDER/UI/Renders.\n"
    "The animated liquid itself remains an Unreal UI material.\n"
)

scene["hud_umg_size"] = "600x80"
scene["arena_umg_size"] = "470x80"
scene["health_umg_size"] = "118x80"
scene["asset_notes"] = "Editable quad-based source for RGB HUD option 3"

bpy.ops.wm.save_as_mainfile(filepath=BLEND_PATH)

quad_failures = [
    obj.name for obj in scene.objects
    if obj.type == "MESH" and any(len(poly.vertices) != 4 for poly in obj.data.polygons)
]
print("RGB_HUD_BUILD_COMPLETE")
print("BLEND", BLEND_PATH)
print("OUTPUT", OUTPUT_DIR)
print("QUAD_FAILURES", quad_failures)
print("OBJECTS", len(scene.objects))
