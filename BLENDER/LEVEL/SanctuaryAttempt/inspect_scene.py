import bpy
import json
import hashlib
from pathlib import Path
from mathutils import Vector

ROOT = Path(__file__).resolve().parent.parent
OUT = Path(__file__).resolve().parent

def mesh_digest(obj):
    if obj.type != 'MESH':
        return None
    h = hashlib.sha256()
    for v in obj.data.vertices:
        h.update(str(tuple(v.co)).encode())
    for p in obj.data.polygons:
        h.update(str(tuple(p.vertices)).encode())
    return h.hexdigest()

def describe(obj):
    bounds = [obj.matrix_world @ Vector(p) for p in obj.bound_box]
    r = dict(name=obj.name, type=obj.type, location=list(obj.location),
             rotation=list(obj.rotation_euler), scale=list(obj.scale),
             world_matrix=[list(row) for row in obj.matrix_world],
             dimensions=list(obj.dimensions), collections=[c.name for c in obj.users_collection],
             bounds_min=[min(p[i] for p in bounds) for i in range(3)],
             bounds_max=[max(p[i] for p in bounds) for i in range(3)],
             hide_render=obj.hide_render, hide_viewport=obj.hide_viewport,
             parent=obj.parent.name if obj.parent else None)
    if obj.type == 'MESH':
        r.update(vertices=len(obj.data.vertices), polygons=len(obj.data.polygons),
                 mesh_digest=mesh_digest(obj), materials=[m.name if m else None for m in obj.data.materials],
                 modifiers=[dict(name=m.name, type=m.type) for m in obj.modifiers])
    if obj.type == 'CAMERA':
        r.update(lens=obj.data.lens, sensor_width=obj.data.sensor_width, angle=obj.data.angle,
                 camera_type=obj.data.type, ortho_scale=obj.data.ortho_scale,
                 clip_start=obj.data.clip_start, clip_end=obj.data.clip_end)
    return r

bpy.ops.wm.open_mainfile(filepath=str(ROOT / 'LEVEL_INTRO.blend'), load_ui=False)
scene = bpy.context.scene
report = dict(blender=bpy.app.version_string, units=scene.unit_settings.system,
              unit_scale=scene.unit_settings.scale_length,
              camera=scene.camera.name if scene.camera else None,
              render_engine=scene.render.engine,
              resolution=[scene.render.resolution_x, scene.render.resolution_y],
              objects=[describe(o) for o in scene.objects],
              images=[dict(name=i.name, path=i.filepath, packed=bool(i.packed_file)) for i in bpy.data.images])
before = set(bpy.data.objects)
bpy.ops.import_scene.fbx(filepath=str(ROOT / 'Greybox.FBX'))
report['fbx_objects'] = [describe(o) for o in bpy.data.objects if o not in before]
(OUT / 'scene_inventory.json').write_text(json.dumps(report, indent=2), encoding='utf8')
print('INVENTORY_SAVED', OUT / 'scene_inventory.json')
for o in report['objects']:
    print(json.dumps(o))
print('FBX', json.dumps(report['fbx_objects']))
