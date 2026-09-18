import bpy, json
from mathutils import Vector
from pathlib import Path
out = Path(bpy.data.filepath).parent / 'Robot_Rig_Output'
out.mkdir(exist_ok=True)
o = next(o for o in bpy.context.scene.objects if o.type == 'MESH')
bpy.context.view_layer.objects.active=o
o.select_set(True)
bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
adj = {v.index: set() for v in o.data.vertices}
for e in o.data.edges:
    a,b=e.vertices; adj[a].add(b); adj[b].add(a)
remaining=set(adj); comps=[]
while remaining:
    todo=[min(remaining)]; ids=set(todo)
    while todo:
        for n in adj[todo.pop()]-ids: ids.add(n); todo.append(n)
    remaining-=ids
    pts=[o.matrix_world @ o.data.vertices[i].co for i in ids]
    lo=[min(p[k] for p in pts) for k in range(3)]
    hi=[max(p[k] for p in pts) for k in range(3)]
    comps.append(dict(id=len(comps),verts=sorted(ids),lo=lo,hi=hi,center=[(a+b)/2 for a,b in zip(lo,hi)]))
(out/'components.json').write_text(json.dumps(comps,indent=2))
print('COMPONENTS',json.dumps([{k:v for k,v in c.items() if k!='verts'} for c in comps]))
scene=bpy.context.scene
scene.render.engine='BLENDER_WORKBENCH'
scene.display.shading.light='STUDIO'
scene.display.shading.color_type='SINGLE'
scene.display.shading.single_color=(0.55,0.62,0.7)
scene.display.shading.show_shadows=True
scene.display.shading.show_cavity=True
scene.render.resolution_x=800;scene.render.resolution_y=800;scene.render.resolution_percentage=100
bpy.ops.object.camera_add(location=(9,-16,9))
cam=bpy.context.object;cam.rotation_euler=(Vector((0,0,3))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.type='ORTHO';cam.data.ortho_scale=8
scene.camera=cam;scene.render.filepath=str(out/'original.png')
bpy.ops.render.render(write_still=True)
