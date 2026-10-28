"""Build the dash pickup emblem (concept C2 "Wingfoot Emblem") in Blender and export it for Unreal.

Run from the repository root (Blender 4.x / 5.x):
  "C:/Program Files/Blender Foundation/Blender 5.2/blender.exe" -b --factory-startup
      --python Scripts/Tools/generate_dash_pickup_mesh.py -- --preview BLENDER/DASH_PICKUP/preview

Writes BLENDER/DASH_PICKUP/DASH_PICKUP.blend and SM_DashPickup.fbx (+ preview_front.png and
preview_angle.png when --preview is given).

Layout: the emblem stands in the X/Z plane (X right, Z up) with its thickness along Y, so it
spins around the vertical Z axis like a coin. Every part pokes out of BOTH faces, so the back
looks complete too. The pivot is the centre of the gear. Units are metres (1 m = 100 cm in
Unreal); the gear is 1 m across.

Material slots:
  0  M_DashPickup_Metal  gear, bolts
  1  M_DashPickup_Glow   wing feathers, inner ring, sole strip, dash ticks, ankle core (emissive)
  2  M_DashPickup_Dark   boot plates, ankle joint
"""

import argparse
import math
import sys
from pathlib import Path

import bmesh
import bpy
from mathutils import Vector

METAL, GLOW, DARK = 0, 1, 2

# ---- sizes (metres) -------------------------------------------------------------------------
PX = 0.5 / 330.0                 # concept-sheet pixels to metres (gear tip radius 330 px = 0.5 m)
GEAR_TIP = 330 * PX
GEAR_ROOT = 290 * PX
GEAR_HOLE = 245 * PX
GEAR_TEETH = 10
GEAR_THICKNESS = 0.14
BOOT_THICKNESS = 0.22            # thicker than the gear, so the boot stands out on both sides
WING_THICKNESS = 0.035
GLOW_THICKNESS = 0.18
SEGMENTS_PER_TOOTH_ARC = 3


def parse_args():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    parser = argparse.ArgumentParser()
    parser.add_argument("--out-dir", default="BLENDER/DASH_PICKUP")
    parser.add_argument("--preview", default="", help="Path prefix for preview renders, e.g. BLENDER/DASH_PICKUP/preview")
    return parser.parse_args(argv)


def concept(x, y):
    """Concept-sheet foot coordinates (pixels, y down, origin 20 px right / 40 px below the gear centre) to X/Z metres."""
    return ((20 + x) * PX, -(40 + y) * PX)


# ---- geometry helpers -----------------------------------------------------------------------
def tag(faces, material):
    for face in faces:
        face.material_index = material


def prism(bm, outline, thickness, material, y_offset=0.0):
    """Extrudes a simple (possibly concave) X/Z outline along Y, centred on y_offset."""
    y0, y1 = y_offset - thickness / 2.0, y_offset + thickness / 2.0
    front = [bm.verts.new((x, y1, z)) for x, z in outline]
    back = [bm.verts.new((x, y0, z)) for x, z in outline]
    faces = [bm.faces.new(front), bm.faces.new(list(reversed(back)))]
    count = len(outline)
    for i in range(count):
        j = (i + 1) % count
        faces.append(bm.faces.new((back[i], back[j], front[j], front[i])))
    tag(faces, material)


def annulus(bm, outer, inner, thickness, material):
    """Ring between two closed X/Z loops of equal length (outer may be a gear profile)."""
    y0, y1 = -thickness / 2.0, thickness / 2.0
    of = [bm.verts.new((x, y1, z)) for x, z in outer]
    ob = [bm.verts.new((x, y0, z)) for x, z in outer]
    inf = [bm.verts.new((x, y1, z)) for x, z in inner]
    inb = [bm.verts.new((x, y0, z)) for x, z in inner]
    faces = []
    count = len(outer)
    for i in range(count):
        j = (i + 1) % count
        faces.append(bm.faces.new((of[i], of[j], inf[j], inf[i])))     # front face
        faces.append(bm.faces.new((ob[j], ob[i], inb[i], inb[j])))     # back face
        faces.append(bm.faces.new((ob[i], ob[j], of[j], of[i])))       # outer wall
        faces.append(bm.faces.new((inb[j], inb[i], inf[i], inf[j])))   # inner wall
    tag(faces, material)


def circle(radius, count, cx=0.0, cz=0.0, start=0.0):
    return [(cx + radius * math.cos(start + math.tau * i / count), cz + radius * math.sin(start + math.tau * i / count)) for i in range(count)]


def cylinder_y(bm, cx, cz, radius, length, material, segments=12):
    prism(bm, circle(radius, segments, cx, cz), length, material)


def gear_profile():
    """Outer gear outline, plus the hole loop sampled at the same angles (one inner point per outer point)."""
    step = math.tau / GEAR_TEETH
    # Fractions of one tooth period: flank up, flat tip, flank down, then root arc points up to the next tooth.
    keys = [(0.30, GEAR_ROOT), (0.38, GEAR_TIP), (0.62, GEAR_TIP), (0.70, GEAR_ROOT)]
    keys += [(0.70 + 0.60 * k / (SEGMENTS_PER_TOOTH_ARC + 1), GEAR_ROOT) for k in range(1, SEGMENTS_PER_TOOTH_ARC + 1)]
    outer, inner = [], []
    for tooth in range(GEAR_TEETH):
        base = tooth * step + math.pi / 2.0 - 0.5 * step   # first tooth centred at the top
        for frac, radius in keys:
            angle = base + frac * step
            outer.append((radius * math.cos(angle), radius * math.sin(angle)))
            inner.append((GEAR_HOLE * math.cos(angle), GEAR_HOLE * math.sin(angle)))
    return outer, inner


# ---- parts ----------------------------------------------------------------------------------
def build_gear(bm):
    outer, inner = gear_profile()
    annulus(bm, outer, inner, GEAR_THICKNESS, METAL)
    # Glowing ring hugging the inside of the hole, standing slightly proud of both faces.
    ring_outer = circle(GEAR_HOLE + 2 * PX, 64)
    ring_inner = circle(GEAR_HOLE - 12 * PX, 64)
    annulus(bm, ring_outer, ring_inner, GLOW_THICKNESS, GLOW)
    # Bolts between the teeth, going straight through so they show on both faces.
    step = math.tau / GEAR_TEETH
    for tooth in range(GEAR_TEETH):
        angle = tooth * step + math.pi / 2.0 + 0.5 * step
        radius = (GEAR_HOLE + GEAR_ROOT) / 2.0
        cylinder_y(bm, radius * math.cos(angle), radius * math.sin(angle), 7 * PX, GEAR_THICKNESS + 0.03, METAL, 10)


def build_boot(bm):
    ankle = [(-95, -150), (-5, -150), (-5, -78), (-115, -78), (-115, -120)]
    body = [(-115, -72), (-5, -72), (-5, -30), (12, -8), (12, 52), (-135, 52), (-135, 0), (-115, -20)]
    toe = [(18, -5), (95, -5), (150, 30), (150, 52), (18, 52)]
    for outline in (ankle, body, toe):
        # Concept outlines are clockwise on screen (y down); flipping y makes them counter-clockwise in X/Z.
        prism(bm, [concept(x, y) for x, y in outline], BOOT_THICKNESS, DARK)
    jx, jz = concept(-75, -112)
    cylinder_y(bm, jx, jz, 18 * PX, BOOT_THICKNESS + 0.04, DARK, 16)
    cylinder_y(bm, jx, jz, 7 * PX, BOOT_THICKNESS + 0.07, GLOW, 12)
    # Glowing sole strip under the boot.
    sole = [(-130, 60), (150, 60), (150, 74), (-130, 74)]
    prism(bm, [concept(x, y) for x, y in sole], GLOW_THICKNESS, GLOW)


def build_wing(bm):
    for index in range(4):
        rx, ry = -100, -138 + index * 30
        length, rise = 250 - index * 42, 95 - index * 26
        tx, ty = rx - length, ry - rise

        def along(t, offset):
            return (rx + (tx - rx) * t, ry + (ty - ry) * t + offset)

        blade = [(rx, ry - 10), along(0.5, -20), (tx, ty), along(0.62, 12), (rx, ry + 22)]
        # Stagger the feathers slightly in depth so they layer instead of z-fighting.
        prism(bm, [concept(x, y) for x, y in blade], WING_THICKNESS, GLOW, y_offset=(index - 1.5) * 0.012)


def build_dash_ticks(bm):
    for x0, length, y in ((-250, 70, 0), (-285, 55, 24), (-240, 40, 48)):
        tick = [(x0, y), (x0 + length, y), (x0 + length, y + 10), (x0, y + 10)]
        prism(bm, [concept(px, py) for px, py in tick], 0.06, GLOW)


def build_mesh():
    bm = bmesh.new()
    build_gear(bm)
    build_boot(bm)
    build_wing(bm)
    build_dash_ticks(bm)
    bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
    bmesh.ops.triangulate(bm, faces=list(bm.faces), quad_method="BEAUTY", ngon_method="BEAUTY")
    # Simple planar UVs (front projection) so Unreal has a UV channel to work with.
    uv_layer = bm.loops.layers.uv.new("UVMap")
    for face in bm.faces:
        for loop in face.loops:
            loop[uv_layer].uv = (loop.vert.co.x + 0.5, loop.vert.co.z + 0.5)
    mesh = bpy.data.meshes.new("SM_DashPickup")
    bm.to_mesh(mesh)
    bm.free()
    for polygon in mesh.polygons:
        polygon.use_smooth = False
    return mesh


# ---- materials, preview, export --------------------------------------------------------------
def make_material(name, base_color, metallic, roughness, emission=None):
    material = bpy.data.materials.new(name)
    material.diffuse_color = (emission if emission else base_color) + (1.0,)
    try:
        material.use_nodes = True
    except (AttributeError, TypeError):
        pass
    tree = getattr(material, "node_tree", None)
    shader = tree.nodes.get("Principled BSDF") if tree else None
    if shader:
        values = {"Base Color": base_color + (1.0,), "Metallic": metallic, "Roughness": roughness}
        if emission:
            values["Emission Color"] = emission + (1.0,)
            values["Emission Strength"] = 6.0
        for socket, value in values.items():
            if socket in shader.inputs:
                shader.inputs[socket].default_value = value
    return material


def render(path, location):
    scene = bpy.context.scene
    camera = scene.camera
    camera.location = location
    camera.rotation_euler = (Vector((0.0, 0.0, 0.0)) - Vector(location)).to_track_quat("-Z", "Y").to_euler()
    scene.render.filepath = str(Path(path).resolve())
    bpy.ops.render.render(write_still=True)


def render_previews(prefix):
    scene = bpy.context.scene
    camera_data = bpy.data.cameras.new("PreviewCamera")
    camera_data.lens = 50
    camera = bpy.data.objects.new("PreviewCamera", camera_data)
    scene.collection.objects.link(camera)
    scene.camera = camera
    scene.render.engine = "BLENDER_WORKBENCH"
    scene.display.shading.light = "STUDIO"
    scene.display.shading.color_type = "MATERIAL"
    scene.display.shading.show_cavity = True
    scene.render.resolution_x = 800
    scene.render.resolution_y = 800
    scene.render.film_transparent = False
    render(prefix + "_front.png", (0.0, -3.4, 0.0))
    render(prefix + "_angle.png", (2.0, -2.6, 0.9))
    render(prefix + "_side.png", (3.4, 0.0, 0.0))


def main():
    args = parse_args()
    out_dir = Path(args.out_dir).resolve()
    out_dir.mkdir(parents=True, exist_ok=True)

    for obj in list(bpy.data.objects):
        bpy.data.objects.remove(obj, do_unlink=True)

    mesh = build_mesh()
    mesh.materials.append(make_material("M_DashPickup_Metal", (0.72, 0.75, 0.82), 0.9, 0.3))
    mesh.materials.append(make_material("M_DashPickup_Glow", (0.43, 0.84, 1.0), 0.0, 0.3, emission=(0.43, 0.84, 1.0)))
    mesh.materials.append(make_material("M_DashPickup_Dark", (0.11, 0.13, 0.17), 0.7, 0.45))

    obj = bpy.data.objects.new("SM_DashPickup", mesh)
    bpy.context.scene.collection.objects.link(obj)
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)

    bpy.ops.wm.save_as_mainfile(filepath=str(out_dir / "DASH_PICKUP.blend"))
    bpy.ops.export_scene.fbx(
        filepath=str(out_dir / "SM_DashPickup.fbx"),
        use_selection=True,
        object_types={"MESH"},
        mesh_smooth_type="FACE",
        use_mesh_modifiers=True,
        add_leaf_bones=False,
        bake_anim=False,
    )
    print(f"Wrote {out_dir / 'DASH_PICKUP.blend'} and {out_dir / 'SM_DashPickup.fbx'}: "
          f"{len(mesh.vertices)} vertices, {len(mesh.polygons)} triangles")

    if args.preview:
        render_previews(args.preview)


if __name__ == "__main__":
    main()
