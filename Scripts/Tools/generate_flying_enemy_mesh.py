"""Build the spiked flying enemy mesh in Blender and export it for Unreal.

Run from the repository root (Blender 5.x):
  "C:/Program Files/Blender Foundation/Blender 5.2/blender.exe" -b --factory-startup
      --python Scripts/Tools/generate_flying_enemy_mesh.py -- [--preview out.png] [--spike-layout faces]

Writes BLENDER/FLYING_ENEMY/FLYING_ENEMY.blend and SM_FlyingEnemy.fbx. The pivot is the
centre of the core. Material slot 0 is the base (core and collars) and slot 1 the emission
(spikes), matching ARGBFlyingEnemyCharacter::ApplyColorMaterial. Sizes are in metres
(1 m = 100 cm in Unreal); the overall diameter is about 1 m, close to the 96 cm capsule.
"""

import argparse
import math
import sys
from pathlib import Path

import bmesh
import bpy
from mathutils import Matrix, Vector

CORE_RADIUS = 0.30
PANEL_SEAM_WIDTH = 0.012
PANEL_SEAM_DEPTH = 0.006
COLLAR_RADIUS = 0.075
COLLAR_DEPTH = 0.08
SPIKE_RADIUS = 0.055
SPIKE_LENGTH = 0.16
SPIKE_SEGMENTS = 6

BASE_MATERIAL_INDEX = 0
EMISSION_MATERIAL_INDEX = 1


def parse_args():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    parser = argparse.ArgumentParser()
    parser.add_argument("--out-dir", default="BLENDER/FLYING_ENEMY")
    parser.add_argument("--preview", default="")
    # "vertices" gives 12 evenly spread spikes; "faces" puts one on each of the 20 icosahedron faces.
    parser.add_argument("--spike-layout", choices=["vertices", "faces"], default="vertices")
    return parser.parse_args(argv)


def spike_directions(layout):
    bm = bmesh.new()
    bmesh.ops.create_icosphere(bm, subdivisions=0, radius=1.0)
    if layout == "vertices":
        directions = [vert.co.normalized() for vert in bm.verts]
    else:
        directions = [face.calc_center_median().normalized() for face in bm.faces]
    bm.free()
    return directions


def orientation(direction):
    return direction.to_track_quat("Z", "Y").to_matrix().to_4x4()


def add_faces_with_material(bm, create, material_index):
    before = set(bm.faces)
    create()
    for face in set(bm.faces) - before:
        face.material_index = material_index


def add_spike(bm, direction, base_distance):
    # Built by hand so the tip is a single vertex instead of a collapsed cone ring.
    rotation = orientation(direction)
    base_center = direction * base_distance
    ring = []
    for index in range(SPIKE_SEGMENTS):
        angle = index / SPIKE_SEGMENTS * math.tau
        local = Vector((SPIKE_RADIUS * math.cos(angle), SPIKE_RADIUS * math.sin(angle), 0.0))
        ring.append(bm.verts.new(base_center + rotation.to_3x3() @ local))
    tip = bm.verts.new(direction * (base_distance + SPIKE_LENGTH))
    for index in range(SPIKE_SEGMENTS):
        bm.faces.new((ring[index], ring[(index + 1) % SPIKE_SEGMENTS], tip))
    bm.faces.new(list(reversed(ring)))


def build_mesh(layout):
    bm = bmesh.new()

    def create_core():
        bmesh.ops.create_icosphere(bm, subdivisions=1, radius=CORE_RADIUS)
        # Individual insets leave grooves between the faces, reading as bolted panels.
        bmesh.ops.inset_individual(bm, faces=list(bm.faces), thickness=PANEL_SEAM_WIDTH, depth=-PANEL_SEAM_DEPTH, use_even_offset=True)

    add_faces_with_material(bm, create_core, BASE_MATERIAL_INDEX)

    collar_center = CORE_RADIUS
    spike_base = collar_center + COLLAR_DEPTH / 2.0
    for direction in spike_directions(layout):
        collar_matrix = Matrix.Translation(direction * collar_center) @ orientation(direction)
        add_faces_with_material(
            bm,
            lambda: bmesh.ops.create_cone(bm, cap_ends=True, cap_tris=False, segments=SPIKE_SEGMENTS, radius1=COLLAR_RADIUS, radius2=COLLAR_RADIUS * 0.85, depth=COLLAR_DEPTH, matrix=collar_matrix),
            BASE_MATERIAL_INDEX,
        )
        add_faces_with_material(bm, lambda: add_spike(bm, direction, spike_base), EMISSION_MATERIAL_INDEX)

    bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
    mesh = bpy.data.meshes.new("SM_FlyingEnemy")
    bm.to_mesh(mesh)
    bm.free()
    for polygon in mesh.polygons:
        polygon.use_smooth = False
    return mesh


def make_material(name, base_color, metallic, roughness, emission=None):
    material = bpy.data.materials.new(name)
    material.diffuse_color = emission + (1.0,) if emission else base_color + (1.0,)
    if hasattr(material, "use_nodes"):
        try:
            material.use_nodes = True
        except (AttributeError, TypeError):
            pass
    tree = material.node_tree
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


def render_preview(path):
    scene = bpy.context.scene
    camera_data = bpy.data.cameras.new("PreviewCamera")
    camera = bpy.data.objects.new("PreviewCamera", camera_data)
    scene.collection.objects.link(camera)
    camera.location = (1.5, -1.9, 0.9)
    target = Vector((0.0, 0.0, 0.0))
    camera.rotation_euler = (target - camera.location).to_track_quat("-Z", "Y").to_euler()
    scene.camera = camera

    scene.render.engine = "BLENDER_WORKBENCH"
    scene.display.shading.light = "STUDIO"
    scene.display.shading.color_type = "MATERIAL"
    scene.display.shading.show_cavity = True
    scene.render.resolution_x = 640
    scene.render.resolution_y = 640
    scene.render.film_transparent = False
    scene.render.filepath = str(Path(path).resolve())
    bpy.ops.render.render(write_still=True)


def main():
    args = parse_args()
    out_dir = Path(args.out_dir).resolve()
    out_dir.mkdir(parents=True, exist_ok=True)

    for obj in list(bpy.data.objects):
        bpy.data.objects.remove(obj, do_unlink=True)

    mesh = build_mesh(args.spike_layout)
    mesh.materials.append(make_material("M_FlyingEnemy_Base", (0.16, 0.17, 0.2), 0.85, 0.35))
    mesh.materials.append(make_material("M_FlyingEnemy_Emission", (0.1, 0.9, 1.0), 0.0, 0.3, emission=(0.1, 0.9, 1.0)))

    obj = bpy.data.objects.new("SM_FlyingEnemy", mesh)
    bpy.context.scene.collection.objects.link(obj)
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)

    bpy.ops.wm.save_as_mainfile(filepath=str(out_dir / "FLYING_ENEMY.blend"))
    bpy.ops.export_scene.fbx(
        filepath=str(out_dir / "SM_FlyingEnemy.fbx"),
        use_selection=True,
        object_types={"MESH"},
        mesh_smooth_type="FACE",
        use_mesh_modifiers=True,
        add_leaf_bones=False,
        bake_anim=False,
    )
    print(f"Wrote {out_dir / 'FLYING_ENEMY.blend'} and {out_dir / 'SM_FlyingEnemy.fbx'}: "
          f"{len(mesh.vertices)} vertices, {len(mesh.polygons)} faces")

    if args.preview:
        render_preview(args.preview)


if __name__ == "__main__":
    main()
