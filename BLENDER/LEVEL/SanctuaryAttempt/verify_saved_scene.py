"""Reopen both saved files and compare original geometry/transform/UV/material state."""
import bpy
import hashlib
import struct
import json
from pathlib import Path
from mathutils import Vector

OUT=Path(__file__).resolve().parent
ROOT=OUT.parent
def state(o):
    h=hashlib.sha256()
    h.update(o.type.encode())
    for row in o.matrix_world:h.update(struct.pack('4f',*row))
    if o.type=='MESH':
        for v in o.data.vertices:h.update(struct.pack('3f',*v.co))
        for e in o.data.edges:h.update(struct.pack('2I',*e.vertices))
        for p in o.data.polygons:
            h.update(struct.pack('I',len(p.vertices)))
            h.update(struct.pack(f'{len(p.vertices)}I',*p.vertices))
            h.update(struct.pack('I?',p.material_index,p.use_smooth))
        for layer in o.data.uv_layers:
            h.update(layer.name.encode())
            for uv in layer.data:h.update(struct.pack('2f',*uv.uv))
        h.update(str([m.name if m else None for m in o.data.materials]).encode())
    h.update((o.parent.name if o.parent else '').encode())
    return h.hexdigest()

build=json.loads((OUT/'verification_build.json').read_text(encoding='utf8'))
bpy.ops.wm.open_mainfile(filepath=str(ROOT/'LEVEL_INTRO.blend'),load_ui=False)
source={o.name:state(o) for o in bpy.context.scene.objects}
source_transforms={o.name:dict(location=list(o.location),rotation=list(o.rotation_euler),scale=list(o.scale),world=[list(r) for r in o.matrix_world]) for o in bpy.context.scene.objects}
source_modifiers={o.name:[(m.name,m.type) for m in o.modifiers] for o in bpy.context.scene.objects}
assert not bpy.context.scene.camera
bpy.ops.wm.open_mainfile(filepath=str(OUT/'LEVEL_INTRO_Sanctuary.blend'),load_ui=False)
scene=bpy.context.scene
# Globally hidden collections are not evaluated by Blender's dependency graph.
# Enable them IN MEMORY for comparison; otherwise cached matrix_world is identity.
for c in scene.collection.children:c.hide_viewport=False
bpy.context.view_layer.update()
missing=[n for n in source if n not in scene.objects]
changed=[n for n in source if n in scene.objects and source[n]!=state(scene.objects[n])]
mods_changed=[n for n in source if n in scene.objects and source_modifiers[n]!=[(m.name,m.type) for m in scene.objects[n].modifiers]]
hash_ok=hashlib.sha256((ROOT/'LEVEL_INTRO.blend').read_bytes()).hexdigest()==build['source_sha256']
fbx_ok=hashlib.sha256((ROOT/'Greybox.FBX').read_bytes()).hexdigest()==build['fbx_sha256']
images=[dict(name=i.name,packed=bool(i.packed_file)) for i in bpy.data.images if i.source=='FILE' and i.filepath]
layer_bounds={}
for c in scene.collection.children:
    if c.name.startswith(('10_','20_','30_','40_')):
        ps=[o.matrix_world@Vector(p) for o in c.objects if o.type=='MESH' for p in o.bound_box]
        layer_bounds[c.name]=dict(objects=len(c.objects),
                                 min=[min(v[i] for v in ps) for i in range(3)],
                                 max=[max(v[i] for v in ps) for i in range(3)])
polys=sum(len(o.data.polygons) for o in scene.objects if o.type=='MESH' and o.name.startswith('SAN_'))
report=dict(all_original_objects_checked=len(source),missing_objects=missing,changed_objects=changed,
            modifier_lists_changed=mods_changed,source_blend_byte_identical=hash_ok,
            supplied_fbx_byte_identical=fbx_ok,all_file_textures_packed=all(i['packed'] for i in images),
            images=images,active_camera=scene.camera.name,new_mesh_base_polygons=polys,
            background_bounds=layer_bounds,blender=bpy.app.version_string)
report['changed_transform_details']={n:dict(source=source_transforms[n],saved=dict(location=list(scene.objects[n].location),rotation=list(scene.objects[n].rotation_euler),scale=list(scene.objects[n].scale),world=[list(r) for r in scene.objects[n].matrix_world])) for n in changed}
report['passed']=not(missing or changed or mods_changed) and hash_ok and fbx_ok and report['all_file_textures_packed']
(OUT/'verification_saved.json').write_text(json.dumps(report,indent=2),encoding='utf8')
print(json.dumps(report,indent=2))
assert report['passed'],'Saved file did not preserve template.'
