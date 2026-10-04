import bpy, json, math
from pathlib import Path
from mathutils import Vector
ROOT = Path(__file__).resolve().parent.parent
OUT = Path(__file__).resolve().parent
bpy.ops.wm.open_mainfile(filepath=str(ROOT/'LEVEL_INTRO.blend'), load_ui=False)
scene=bpy.context.scene
scene.render.engine='CYCLES'
scene.cycles.samples=12
scene.cycles.use_denoising=True
scene.render.resolution_x=1800
scene.render.resolution_y=660
scene.render.resolution_percentage=100
scene.world=bpy.data.worlds.new('InspectionWorld')
scene.world.use_nodes=True
scene.world.node_tree.nodes['Background'].inputs['Color'].default_value=(0.3,0.4,0.5,1)
scene.world.node_tree.nodes['Background'].inputs['Strength'].default_value=0.7
ld=bpy.data.lights.new('InspectionSun','SUN'); ld.energy=2.0
lo=bpy.data.objects.new('InspectionSun',ld); scene.collection.objects.link(lo)
lo.rotation_euler=(math.radians(25),math.radians(-35),math.radians(-25))
cam=bpy.data.cameras.new('InspectionCamera'); co=bpy.data.objects.new('InspectionCamera',cam)
scene.collection.objects.link(co); scene.camera=co
cam.type='ORTHO'; cam.ortho_scale=575;cam.clip_end=3000
co.location=(-21,-650,120)
co.rotation_euler=(Vector((-21,0,48))-co.location).to_track_quat('-Z','Y').to_euler()
scene.render.filepath=str(OUT/'template_overview.png')
bpy.ops.render.render(write_still=True)
for o in scene.objects:
    if o.type=='MESH' and not o.name.startswith('Combined_'):
        o.hide_render=True
scene.render.filepath=str(OUT/'route_overview.png')
bpy.ops.render.render(write_still=True)
o=bpy.data.objects['Combined_04BF457E']
floors=[]
for p in o.data.polygons:
    if p.normal.z>0.7:
        coords=[o.matrix_world @ o.data.vertices[i].co for i in p.vertices]
        floors.append(dict(x0=min(v.x for v in coords),x1=max(v.x for v in coords),
                           z=sum(v.z for v in coords)/len(coords)))
(OUT/'floor_surfaces.json').write_text(json.dumps(floors,indent=2),encoding='utf8')
print('FLOORS',json.dumps(floors))
