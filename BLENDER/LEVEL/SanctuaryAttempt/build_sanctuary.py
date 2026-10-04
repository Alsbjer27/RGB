"""Create an additive, editable art interpretation. Never transform source objects."""
import bpy
import math
import random
import json
import hashlib
import struct
from pathlib import Path
from mathutils import Vector

ROOT = Path(__file__).resolve().parent.parent
OUT = Path(__file__).resolve().parent
SOURCE = ROOT / 'LEVEL_INTRO.blend'
DEST = OUT / 'LEVEL_INTRO_Sanctuary.blend'
RNG = random.Random(240104)
bpy.ops.wm.open_mainfile(filepath=str(SOURCE), load_ui=False)
scene = bpy.context.scene
originals = list(scene.objects)

def fingerprint(o):
    h = hashlib.sha256()
    h.update(o.type.encode())
    for row in o.matrix_world:
        h.update(struct.pack('4f', *row))
    if o.type == 'MESH':
        for v in o.data.vertices:
            h.update(struct.pack('3f', *v.co))
        for e in o.data.edges:
            h.update(struct.pack('2I', *e.vertices))
        for p in o.data.polygons:
            h.update(struct.pack('I',len(p.vertices)))
            h.update(struct.pack(f'{len(p.vertices)}I', *p.vertices))
            h.update(struct.pack('I?',p.material_index,p.use_smooth))
        for layer in o.data.uv_layers:
            h.update(layer.name.encode())
            for uv in layer.data:
                h.update(struct.pack('2f', *uv.uv))
        h.update(str([m.name if m else None for m in o.data.materials]).encode())
    h.update((o.parent.name if o.parent else '').encode())
    return h.hexdigest()

before = {o.name: fingerprint(o) for o in originals}
source_hash = hashlib.sha256(SOURCE.read_bytes()).hexdigest()
fbx_hash = hashlib.sha256((ROOT/'Greybox.FBX').read_bytes()).hexdigest()

def collection(name):
    c = bpy.data.collections.new(name)
    scene.collection.children.link(c)
    return c

route = collection('00_PROTECTED_Unreal_Greybox')
machinery = collection('01_TEMPLATE_Mechanical_Details')
legacy = collection('02_TEMPLATE_Previous_Terrain [toggle to compare]')
facade = collection('05_VISUAL_Skins_and_Wall_Relief [no collision]')
near = collection('10_BG_NEAR | Y 14 to 65')
floor_collection = collection('15_VALLEY_FLOOR | continuous scenery behind route')
mid = collection('20_BG_MIDDLE | Y 65 to 190')
far = collection('30_BG_FAR | Y 190 to 650')
fore = collection('40_FOREGROUND | below route only')
lights = collection('80_LIGHTING_and_Atmosphere')
cameras = collection('90_REVIEW_Cameras [not gameplay cameras]')

legacy_names = {'Plane','Landscape.001','Landscape.002','Landscape_plane'}
legacy_names.update('Cube.%03d'%i for i in range(3,12))
for o in originals:
    target = route if o.name.startswith('Combined_') else legacy if o.name in legacy_names else machinery
    for c in list(o.users_collection):
        c.objects.unlink(o)
    target.objects.link(o)
    o['RGB_source_object'] = True
    o['RGB_original_geometry_transform_sha256'] = before[o.name]
    if target == route:
        o.hide_select = True
        o.lock_location=(True,True,True)
        o.lock_rotation=(True,True,True)
        o.lock_scale=(True,True,True)
legacy.hide_render=True
legacy.hide_viewport=True
for c in list(scene.collection.children):
    if c.name=='Collection' and not c.objects and not c.children:
        scene.collection.children.unlink(c)

def material(name, color, roughness=0.82, metallic=0, texture=False):
    m=bpy.data.materials.new('SAN_'+name)
    m.diffuse_color=(*color,1)
    m.use_nodes=True
    n=m.node_tree.nodes; l=m.node_tree.links
    bs=n.get('Principled BSDF')
    bs.inputs['Base Color'].default_value=(*color,1)
    bs.inputs['Roughness'].default_value=roughness
    bs.inputs['Metallic'].default_value=metallic
    if texture:
        tc=n.new('ShaderNodeTexCoord');tc.location=(-700,80)
        noise=n.new('ShaderNodeTexNoise');noise.location=(-480,80)
        noise.inputs['Scale'].default_value=1.8
        noise.inputs['Detail'].default_value=2.0
        l.new(tc.outputs['Object'],noise.inputs['Vector'])
        ramp=n.new('ShaderNodeValToRGB');ramp.location=(-260,160)
        ramp.color_ramp.elements[0].position=.12
        ramp.color_ramp.elements[0].color=(*(v*.82 for v in color),1)
        ramp.color_ramp.elements[1].position=.85
        ramp.color_ramp.elements[1].color=(*(min(v*1.12,1) for v in color),1)
        l.new(noise.outputs['Fac'],ramp.inputs['Fac'])
        l.new(ramp.outputs['Color'],bs.inputs['Base Color'])
        bump=n.new('ShaderNodeBump');bump.location=(-40,-80)
        bump.inputs['Strength'].default_value=.16
        bump.inputs['Distance'].default_value=.035
        l.new(noise.outputs['Fac'],bump.inputs['Height'])
        l.new(bump.outputs['Normal'],bs.inputs['Normal'])
    return m

stone=material('Warm_Limestone',(0.32,.226,.133),texture=True)
lightstone=material('Pale_Carved_Edges',(.48,.363,.235),texture=True)
darkstone=material('Weathered_Shadow_Stone',(.18,.14,.105),texture=True)
rockmat=material('Cliff_Sandstone',(.275,.169,.091),texture=True)
rocklight=material('Cliff_Facet_Light',(.36,.233,.133))
rockdark=material('Cliff_Facet_Shadow',(.205,.115,.073))
slate=material('Slate_Inset',(.045,.092,.102),.9)
teal=material('Oxidised_Bronze',(.078,.191,.17),.62,.4)
gold=material('Old_Brass',(.39,.257,.096),.48,.68)
cloth=material('Faded_Saffron_Banners',(.41,.113,.056),.97)
leaves=material('Dusty_Olive_Leaves',(.11,.159,.098),.92)
bark=material('Olive_Bark',(.13,.09,.063),texture=True)
midstone=material('Middle_Dusty_Stone',(.23,.29,.267),.92)
midlight=material('Middle_Light_Stone',(.33,.38,.32),.92)
midshadow=material('Middle_Shadow_Stone',(.12,.205,.216),.96)
farstone=material('Distant_Teal_Stone',(.19,.325,.347),.99)
farhigh=material('Distant_Teal_Facet',(.24,.39,.409),.99)
farest=material('Horizon_Haze',(.32,.468,.475),1)
foremat=material('Foreground_Dark_Shale',(.037,.054,.048),.95)
facing_mats=[material('Facing_Block_%d'%i,(.29+i*.023,.203+i*.017,.124+i*.011),texture=True) for i in range(5)]
for m,amount in [(farstone,.55),(farhigh,.65),(farest,.8)]:
    # Artistic aerial perspective: suppress distant lighting contrast, retain broad facets.
    n=m.node_tree.nodes;l=m.node_tree.links
    bs=n.get('Principled BSDF');out=n.get('Material Output')
    em=n.new('ShaderNodeEmission');em.inputs['Color'].default_value=m.diffuse_color
    em.inputs['Strength'].default_value=.72
    mix=n.new('ShaderNodeMixShader');mix.inputs[0].default_value=amount
    l.new(bs.outputs['BSDF'],mix.inputs[1]);l.new(em.outputs[0],mix.inputs[2])
    l.new(mix.outputs[0],out.inputs['Surface'])

def finish(o,name,c,m):
    o.name='SAN_'+name
    for old in list(o.users_collection):old.objects.unlink(o)
    c.objects.link(o)
    if m and o.type=='MESH':o.data.materials.append(m)
    o['RGB_environment_only']=True
    o['Unreal_collision']='NoCollision; cosmetic scenery'
    return o

def make_mesh(name,verts,faces,c,m):
    me=bpy.data.meshes.new('SAN_'+name+'_Mesh')
    me.from_pydata(verts,[],faces);me.update()
    o=bpy.data.objects.new('SAN_'+name,me);c.objects.link(o)
    if m:me.materials.append(m)
    o['RGB_environment_only']=True
    o['Unreal_collision']='NoCollision; cosmetic scenery'
    # Basic planar UVs provide editable coordinates, not a texture bake.
    uv=me.uv_layers.new(name='UVMap')
    for p in me.polygons:
        normal=p.normal
        ax=max(range(3),key=lambda i:abs(normal[i]))
        a,b=[i for i in range(3) if i!=ax]
        for li in p.loop_indices:
            v=me.vertices[me.loops[li].vertex_index].co
            uv.data[li].uv=(v[a]/8,v[b]/8)
    return o

def bevel(o,width=.12):
    mod=o.modifiers.new('Soft hand-cut edges','BEVEL');mod.width=width;mod.segments=2
    mod.limit_method='ANGLE'
    return o

def box(name,loc,size,c,m,edge=.1):
    x,y,z=(v/2 for v in size)
    vs=[(-x,-y,-z),(-x,-y,z),(-x,y,-z),(-x,y,z),(x,-y,-z),(x,-y,z),(x,y,-z),(x,y,z)]
    fs=[(0,4,6,2),(1,3,7,5),(0,1,5,4),(2,6,7,3),(0,2,3,1),(4,5,7,6)]
    o=make_mesh(name,vs,fs,c,m);o.location=loc
    if edge:bevel(o,min(edge,min(size)*.18))
    return o

def cylinder(name,loc,radius,depth,c,m,vertices=12,radius_top=None):
    rt=radius if radius_top is None else radius_top
    vs=[]
    for z,r in [(-depth/2,radius),(depth/2,rt)]:
        vs.extend((r*math.cos(2*math.pi*i/vertices),r*math.sin(2*math.pi*i/vertices),z) for i in range(vertices))
    fs=[tuple(reversed(range(vertices))),tuple(range(vertices,2*vertices))]
    fs.extend((i,(i+1)%vertices,(i+1)%vertices+vertices,i+vertices) for i in range(vertices))
    o=make_mesh(name,vs,fs,c,m);o.location=loc
    return o

def sphere(name,loc,size,c,m,subdiv=1):
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=subdiv,radius=1,location=loc)
    o=finish(bpy.context.object,name,c,m);o.scale=size
    return o

def beam(name,a,b,r,c,m):
    a,b=Vector(a),Vector(b)
    o=cylinder(name,(a+b)/2,r,(b-a).length,c,m,8)
    o.rotation_euler=(b-a).to_track_quat('Z','Y').to_euler()
    return o

def torus(name,loc,radius,tube,c,m):
    bpy.ops.mesh.primitive_torus_add(major_segments=64,minor_segments=8,
                                   location=loc,major_radius=radius,minor_radius=tube,
                                   rotation=(math.pi/2,0,0))
    return finish(bpy.context.object,name,c,m)

def column(name,x,y,z,h,r,c,m=stone,trim=lightstone,details=True):
    box(name+'_Foot',(x,y,z+.5),(r*3.5,r*3.5,1),c,trim,.18)
    box(name+'_Plinth',(x,y,z+1.4),(r*2.85,r*2.85,.9),c,m,.15)
    cylinder(name+'_Shaft',(x,y,z+h/2),r,h-3.8,c,m,12,r*.84)
    box(name+'_Capital',(x,y,z+h-1.15),(r*3.15,r*3.15,1.15),c,trim,.16)
    box(name+'_Abacus',(x,y,z+h-.3),(r*3.6,r*3.6,.6),c,m,.12)
    if details:
        for j,zz in enumerate([z+2.5,z+h*.72,z+h-2.35]):
            cylinder(name+'_BronzeBand_'+str(j),(x,y,zz),r*1.075,.3,c,teal,12)
        for k in range(6):
            t=k*math.tau/6
            beam(name+'_Flute_'+str(k),(x+r*.99*math.cos(t),y+r*.99*math.sin(t),z+3.2),
                 (x+r*.88*math.cos(t),y+r*.88*math.sin(t),z+h-3),.065,c,trim)

def arch(name,x,y,z,w,h,depth,c,m=stone,trim=lightstone,t=2.1,segments=17,ornate=False):
    r=w/2; spring=z+h-r
    leg_h=max(h-r,.5)
    for side in [-1,1]:
        column(name+('_L' if side<0 else '_R'),x+side*(r+t/2),y,z,leg_h,t*.55,c,m,trim,ornate)
    for i in range(segments):
        a=i*math.pi/segments+.009;b=(i+1)*math.pi/segments-.009
        vs=[(x+rr*math.cos(th),yy,spring+rr*math.sin(th))
            for yy in [y-depth/2,y+depth/2] for rr,th in [(r,a),(r,b),(r+t,b),(r+t,a)]]
        o=make_mesh(name+'_Voussoir_%02d'%i,vs,[(0,3,2,1),(4,5,6,7),(0,1,5,4),(1,2,6,5),(2,3,7,6),(3,0,4,7)],c,
                    trim if i==segments//2 else m)
        bevel(o,.06)
    if ornate:
        for i in range(segments):
            a=i*math.pi/segments+.005;b=(i+1)*math.pi/segments-.005
            rr=r+t*.76; rr2=rr+.28
            vs=[(x+rad*math.cos(th),y-depth/2-.08,spring+rad*math.sin(th))
                for rad,th in [(rr,a),(rr,b),(rr2,b),(rr2,a)]]
            make_mesh(name+'_InlaidArc_%02d'%i,vs,[(0,1,2,3)],c,gold)

def arched_panel(name,x,y,z,w,h,c,m):
    r=w/2;spring=z+h-r
    vs=[(x-r,y,z),(x+r,y,z)]
    vs.extend((x+r*math.cos(i*math.pi/24),y,spring+r*math.sin(i*math.pi/24)) for i in range(25))
    return make_mesh(name,vs,[tuple(range(len(vs)))],c,m)

def diamond(name,x,y,z,w,h,c,m):
    return make_mesh(name,[(x,y,z+h/2),(x+w/2,y,z),(x,y,z-h/2),(x-w/2,y,z)],[(0,1,2,3)],c,m)

def rock(name,x,y,z,sx,sy,h,c,mats):
    n=9; levels=[(0,.94),(.24,1),(.60,.86),(.88,.67),(1,.55)]
    vs=[]
    phase=RNG.random()*.8
    shift=RNG.uniform(-.3,.3)*sx
    for j,(zz,rad) in enumerate(levels):
        for i in range(n):
            a=i*math.tau/n+phase
            noise=RNG.uniform(.83,1.12)
            vs.append((sx*rad*math.cos(a)*noise+shift*zz,
                       sy*rad*math.sin(a)*noise, h*zz+RNG.uniform(-.04,.04)*h if j else 0))
    fs=[tuple(reversed(range(n)))]
    for j in range(len(levels)-1):
        for i in range(n):
            a=j*n+i;b=j*n+(i+1)%n;cc=(j+1)*n+(i+1)%n;d=(j+1)*n+i
            fs.extend([(a,b,d),(b,cc,d)])
    fs.append(tuple(range((len(levels)-1)*n,len(levels)*n)))
    o=make_mesh(name,vs,fs,c,mats[0]);o.location=(x,y,z)
    for mat in mats[1:]:o.data.materials.append(mat)
    for p in o.data.polygons:p.material_index=RNG.choices(range(len(mats)),[7]+[2]*(len(mats)-1))[0]
    return o

def stone_courses(src):
    """Add shallow blocks ONLY inside existing metal side faces, never across gaps."""
    polys=[]
    for p in src.data.polygons:
        mat=src.data.materials[p.material_index] if p.material_index<len(src.data.materials) else None
        if mat and mat.name.startswith('Metal') and p.normal.y<-.75:
            polys.append([src.matrix_world@src.data.vertices[i].co for i in p.vertices])
    if not polys:return
    verts=[];faces=[];indices=[]
    def inside(x,z,poly):
        hit=False
        for i,a in enumerate(poly):
            b=poly[(i+1)%len(poly)]
            if (a.z>z)!=(b.z>z) and x<(b.x-a.x)*(z-a.z)/(b.z-a.z)+a.x:hit=not hit
        return hit
    # Coplanar imported triangles are tested as one union to avoid visible diagonal seams.
    ys=sorted(set(round(sum(v.y for v in p)/len(p),3) for p in polys))
    for yy in ys:
        group=[p for p in polys if abs(sum(v.y for v in p)/len(p)-yy)<.01]
        xmin=min(v.x for p in group for v in p);xmax=max(v.x for p in group for v in p)
        zmin=min(v.z for p in group for v in p);zmax=max(v.z for p in group for v in p)
        cellw=7.5;cellh=2.9
        thin_wall=(max(v.y for p in group for v in p)>3)
        for j in range(math.ceil((zmax-zmin)/cellh)):
            zz=zmin+(j+.5)*cellh
            for k in range(math.ceil((xmax-xmin)/cellw)+1):
                xx=xmin+(k+.5)*cellw-(cellw/2 if j%2 else 0)
                w=cellw-.16;h=cellh-.13
                pts=[(xx-w/2,zz-h/2),(xx+w/2,zz-h/2),(xx-w/2,zz+h/2),(xx+w/2,zz+h/2)]
                if not all(any(inside(x,z,p) for p in group) for x,z in pts):continue
                depth=.025 if thin_wall else .13
                y=yy-.044 if thin_wall else yy-.07
                start=len(verts)
                verts.extend((xx+a*w/2,y+b*depth/2,zz+cc*h/2)
                             for a,b,cc in [(-1,-1,-1),(-1,-1,1),(-1,1,-1),(-1,1,1),
                                           (1,-1,-1),(1,-1,1),(1,1,-1),(1,1,1)])
                fs=[(0,4,6,2),(1,3,7,5),(0,1,5,4),(2,6,7,3),(0,2,3,1),(4,5,7,6)]
                faces.extend(tuple(start+i for i in f) for f in fs)
                indices.extend([RNG.randrange(len(facing_mats))]*6)
    if faces:
        o=make_mesh('Stone_Courses_'+src.name,verts,faces,facade,None)
        for m in facing_mats:o.data.materials.append(m)
        for p,i in zip(o.data.polygons,indices):p.material_index=i
        bevel(o,.035)
        o['RGB_note']='Thin visual dressing; all blocks are clipped to existing side faces.'

def ridge(name,y,width,height,c,mats,seed):
    rnd=random.Random(seed);n=65;rows=5
    xs=[-width/2+width*i/(n-1) for i in range(n)]
    heights=[]
    peaks=[(rnd.uniform(-width*.46,width*.46),rnd.uniform(.25,1)*height,rnd.uniform(35,90)) for _ in range(14)]
    for x in xs:
        v=25+max(h*math.exp(-((x-p)/s)**2) for p,h,s in peaks)
        heights.append(v*rnd.uniform(.85,1.06))
    vs=[]
    for j in range(rows):
        t=j/(rows-1)
        for i,x in enumerate(xs):
            z=-35+heights[i]*math.sin(math.pi*t)**.75
            vs.append((x+rnd.uniform(-3,3),y+(t-.5)*180,z+rnd.uniform(-2,2)))
    fs=[]
    for j in range(rows-1):
        for i in range(n-1):
            a=j*n+i;b=a+1;cc=a+n+1;d=a+n
            fs.extend([(a,b,d),(b,cc,d)])
    o=make_mesh(name,vs,fs,c,mats[0])
    for mat in mats[1:]:o.data.materials.append(mat)
    for p in o.data.polygons:p.material_index=rnd.choices(range(len(mats)),[6]+[2]*(len(mats)-1))[0]
    return o

def banner(name,x,y,z,w,h,c):
    nx,nz=5,9
    vs=[]
    for j in range(nz):
        f=j/(nz-1)
        for i in range(nx):
            t=i/(nx-1)
            dz=(abs(t-.5)*2)*w*.22*f**5
            vs.append((x+(t-.5)*w,y+math.sin(t*math.tau+f*3)*.65*f,z-f*h+dz))
    fs=[(j*nx+i,j*nx+i+1,(j+1)*nx+i+1,(j+1)*nx+i) for j in range(nz-1) for i in range(nx-1)]
    make_mesh(name,vs,fs,c,cloth)
    beam(name+'_Hanger',(x-w*.65,y,z+.1),(x+w*.65,y,z+.1),.16,c,gold)
    for j in range(5):diamond(name+'_StitchedDiamond'+str(j),x,y-.3,z-h*.3-j*h*.12,w*.45,h*.07,c,lightstone)

def olive(name,x,y,z,h,c):
    beam(name+'_Trunk',(x,y,z),(x-.9,y,z+h*.62),.38,c,bark)
    for j in range(4):
        a=j*math.tau/4+RNG.random()*.2
        end=(x+math.cos(a)*h*.3,y+math.sin(a)*h*.2,z+h*(.75+RNG.random()*.15))
        beam(name+'_Branch'+str(j),(x-.5,y,z+h*.4),end,.18,c,bark)
        sphere(name+'_Crown'+str(j),end,(h*.28,h*.19,h*.16),c,leaves,2)

# Subtle visual facades are added over metal faces, without changing source meshes/materials.
for src in originals:
    if not src.name.startswith('Combined_'):continue
    verts=[];fs=[]
    for p in src.data.polygons:
        mat=src.data.materials[p.material_index] if p.material_index<len(src.data.materials) else None
        if mat and mat.name.startswith('Metal') and p.normal.y<-.75:
            coords=[src.matrix_world@src.data.vertices[i].co for i in p.vertices]
            start=len(verts)
            verts.extend((v.x,v.y-.035,v.z) for v in coords)
            fs.append(tuple(range(start,len(verts))))
    if fs:
        skin=make_mesh('Cosmetic_Face_'+src.name,verts,fs,facade,stone)
        skin['RGB_original_underneath']=src.name
        skin['RGB_note']='Visual overlay only. Source tread/collision remains unchanged.'
    stone_courses(src)

# Frontal relief on the existing thin back-wall, safely behind gameplay-front faces.
for i,x in enumerate([82,119,156,193,224]):
    w=19 if i==4 else 24
    arched_panel('Sanctum_Niche_%02d'%i,x,3.50,8,w,77,facade,slate)
    arch('Sanctum_Niche_Frame_%02d'%i,x,3.32,8,w,77,.3,facade,darkstone,lightstone,1.25,19,False)
    for zz in [22,51,73]:
        diamond('Sanctum_Niche_Inlay_%02d_%02d'%(i,zz),x,3.19,zz,2.4,5,facade,gold)
    for xx in [-5,5]:box('Sanctum_Niche_Mullion',(x+xx,3.22,43),(.25,.25,57),facade,teal,.03)
for x in [65,101,138,175,212,235]:
    box('Wall_Pilaster',(x,3.18,48),(2,.6,94),facade,lightstone,.12)
    for z in [13,43,74,96]:box('Wall_Pilaster_Cap',(x,3.01,z),(3.6,.7,.9),facade,darkstone,.09)
for z in [9,48,91]:box('Wall_Entablature',(145,3.07,z),(180,.4,.7),facade,lightstone,.08)

# Entrance terrace: broken gate, low outcrops and a sheltered olive garden.
rock('Entrance_Bedrock',-254,30,-14,44,16,27,near,[rockmat,rocklight,rockdark])
rock('Gate_Left_Cliff',-291,42,-10,21,18,59,near,[rockmat,rocklight,rockdark])
rock('Gate_Right_Cliff',-191,45,-10,17,18,44,near,[rockmat,rocklight,rockdark])
arch('Gate_of_the_First_Light',-245,33,10,34,54,8,near,stone,lightstone,3.2,23,True)
for x in [-264,-226]:
    box('Gate_Terrace_Base',(x,35,7),(24,19,5),near,darkstone,.22)
    box('Gate_Terrace_Cornice',(x,34,10),(25,20,1.1),near,lightstone,.18)
banner('Entrance_Banner',-224,28,49,4.2,16,near)
column('Broken_Entrance_Pillar',-209,27,11,18,2.0,near)
for i in range(6):
    o=box('Fallen_Gate_Stone_%02d'%i,(-274+i*3.1,20+RNG.uniform(0,6),10),(2.5,3.2,1.9),near,
          lightstone if i%3==0 else stone,.18)
    o.rotation_euler=(RNG.uniform(-.15,.15),RNG.uniform(-.25,.25),RNG.uniform(-.4,.4))
olive('Entrance_Olive',-281,24,12,15,near)
olive('Terrace_Olive',-195,35,9,13,near)
for i,x in enumerate([-277,-239,-213,-188]):
    cylinder('Terrace_Urn_%02d'%i,(x,22,12),1.1,2.4,near,teal,12,.65)
    cylinder('Terrace_Urn_Lip_%02d'%i,(x,22,13.3),.85,.25,near,gold,12)

# Near rock ledges deliberately stop behind Y=14, leaving gaps in the measured route clear.
for i,(x,sz,h,zz) in enumerate([(-169,20,28,-17),(-144,15,25,-18),(-112,18,30,-17),
                                (-66,21,24,-14),(-34,18,34,-17),(15,28,47,-19),
                                (61,30,57,-24),(99,20,60,-30),(172,29,43,-26),(235,27,92,-12)]):
    rock('Near_Escarpment_%02d'%i,x,38,zz,sz,20,h,near,[rockmat,rocklight,rockdark])
for x,z in [(-164,12),(-116,15),(-54,13),(34,31),(76,48),(224,70)]:
    column('Waystone_%d'%x,x,22,z,12,1.1,near,stone,lightstone,False)
    diamond('Waystone_BrassMark_%d'%x,x,20.8,z+7.5,1.7,3.4,near,gold)

# Open middle layer: an aqueduct crossing the valley, with distinct silhouettes and real gaps.
for i,x in enumerate([-165,-133,-101,-69]):
    arch('Valley_Aqueduct_%02d'%i,x,84,-15,25,49,12,mid,midstone,midlight,2.5,17,False)
    if i!=2:
        box('Aqueduct_Coping_%02d'%i,(x,84,36),(34,16,3),mid,midlight,.18)
    else:
        # A broken BACKGROUND span opens the silhouette; the playable route stays intact.
        for j in [6,7,8,9,10]:
            ob=bpy.data.objects.get('SAN_Valley_Aqueduct_%02d_Voussoir_%02d'%(i,j))
            if ob:bpy.data.objects.remove(ob,do_unlink=True)
        for side in [-1,1]:
            box('Aqueduct_Broken_Deck',(x+side*12,84,36),(10,16,3),mid,midlight,.18)
        for j in range(3):
            block=box('Aqueduct_Fallen_Block_%02d'%j,(x-6+j*6,82,-12),(3.4,6,2.8),mid,midlight,.15)
            block.rotation_euler=(.17*j,.24,-.14*j)
    for xx in [-12,-4,4,12]:
        if i==2 and abs(xx)==4:continue
        box('Aqueduct_Parapet_%02d_%d'%(i,xx),(x+xx,81,39),(1.5,3,4),mid,midstone,.1)
rock('Aqueduct_West_Buttress',-190,114,-29,44,32,68,mid,[midshadow,midstone,midlight])
rock('Aqueduct_East_Buttress',-41,118,-25,33,24,72,mid,[midshadow,midstone,midlight])
box('Aqueduct_Valley_Foundation',(-117,84,-18),(182,18,7),mid,midshadow,.4)
for i,x in enumerate([-165,-133,-101,-69]):
    if i!=2:box('Aqueduct_Arch_Spandrel_%02d'%i,(x,84,35),(30,12,2.0),mid,midstone,.1)
column('Aqueduct_Broken_Marker',-185,78,34,16,1.5,mid,midstone,midlight,False)
banner('Aqueduct_Last_Banner',-54,76,43,5,20,mid)

# Vertical climb threshold, echoing the route without attaching to it.
arch('Climb_Threshold',22,42,13,39,76,10,near,stone,lightstone,3.0,25,True)
banner('Threshold_Banner_Left',-1,35,71,5,22,near)
banner('Threshold_Banner_Right',45,35,71,5,22,near)
box('Threshold_Crown',(22,42,94),(52,14,4),near,darkstone,.28)
for x in [-4,48]:
    cylinder('Threshold_Crown_Spire',(x,42,98),2,6,near,teal,8,.2)

# The large sanctuary behind the final climb: nested roofs, terraces, colonnades and solar oculus.
for i,(x,w,h,y) in enumerate([(78,39,106,122),(143,72,143,133),(222,34,123,129)]):
    box('Sanctuary_Tower_%02d'%i,(x,y,h/2-8),(w,32,h),mid,midstone,.32)
    for z in [h*.27,h*.59,h-9,h-3]:
        box('Sanctuary_Tower_Cornice_%02d'%i,(x,y-1,z),(w+5,35,1.7),mid,midlight,.2)
    for xx in [-w*.34,w*.34]:
        column('Sanctuary_Tower_Pilaster_%02d'%i,x+xx,y-18,-5,h-1,1.9,mid,midstone,midlight,False)
    arched_panel('Sanctuary_Tower_Window_%02d'%i,x,y-16.05,h*.35,w*.48,h*.45,mid,midshadow)
    arch('Sanctuary_Tower_Window_Frame_%02d'%i,x,y-16.3,h*.35,w*.48,h*.45,.8,mid,midstone,midlight,1.4,21,False)

def dome(name,x,y,z,r,h,c,m):
    n=48;rings=13;vs=[]
    for j in range(rings):
        a=(j/(rings-1))*math.pi/2
        rr=r*math.cos(a)*(1+.08*math.sin(a*math.pi))
        vs.extend((rr*math.cos(i*math.tau/n),rr*math.sin(i*math.tau/n),h*math.sin(a)) for i in range(n))
    fs=[(j*n+i,j*n+(i+1)%n,(j+1)*n+(i+1)%n,(j+1)*n+i) for j in range(rings-1) for i in range(n)]
    o=make_mesh(name,vs,fs,c,m);o.location=(x,y,z)
    for p in o.data.polygons:p.use_smooth=True
    cylinder(name+'_Drum',(x,y,z-2),r*1.02,4,c,midlight,48)
    for i in range(16):
        a=i*math.tau/16
        # Meridional bronze ribs define the dome without noisy textures.
        previous=None
        for j in range(10):
            t=j/10*math.pi/2;rr=r*math.cos(t)
            p=(x+rr*math.cos(a),y+rr*math.sin(a),z+h*math.sin(t))
            if previous:beam(name+'_Rib_%02d_%02d'%(i,j),previous,p,.12,c,gold)
            previous=p
    cylinder(name+'_Finial',(x,y,z+h+3),1.7,6,c,gold,12,.08)

dome('Crown_of_the_Sanctuary',143,133,136,35,28,mid,teal)
dome('West_Chapel',78,122,99,19,20,mid,midshadow)
dome('East_Chapel',222,129,116,16,18,mid,teal)
box('Sanctuary_Lower_Terrace',(145,96,46),(198,48,5),mid,midstone,.3)
box('Sanctuary_Upper_Terrace',(146,102,96),(186,41,3.5),mid,midlight,.3)
for i,x in enumerate([54,86,118,150,182,214,246]):
    arch('Sanctuary_Colonnade_%02d'%i,x,82,47,23,45,9,mid,midstone,midlight,2,17,False)
for x in [65,107,187,229]:banner('Sanctuary_Hanging_Banner_%d'%x,x,75,96,6,25,mid)
torus('Solar_Oculus_Outer',(143,94,123),22,.8,mid,gold)
torus('Solar_Oculus_Inner',(143,93.8,123),18,.4,mid,midlight)
for i in range(16):
    a=i*math.tau/16
    beam('Oculus_Spoke_%02d'%i,(143+18*math.cos(a),93.7,123+18*math.sin(a)),
         (143+21.8*math.cos(a),93.7,123+21.8*math.sin(a)),.21,mid,gold)
    diamond('Oculus_Inlay_%02d'%i,143+22*math.cos(a),93.3,123+22*math.sin(a),1.0,2.0,mid,teal)
torus('Oculus_Centre',(143,93.6,123),8,.35,mid,gold)
diamond('Oculus_Four_Pointed_Seal',143,93.0,123,9,15,mid,gold)

# Distant architectural skyline, on its own depth plane.
for i,(x,y,z,h,r) in enumerate([(-253,163,24,73,7),(-80,165,34,87,9),
                                (40,176,11,105,7),(283,170,34,108,9)]):
    cylinder('Distant_Watchtower_%02d'%i,(x,y,z+h/2),r,h,mid,midshadow,10,r*.8)
    for dz in [h*.25,h*.7,h-3]:cylinder('Watchtower_Ledge_%02d'%i,(x,y,z+dz),r*1.2,1.5,mid,midlight,10)
    cylinder('Watchtower_Point_%02d'%i,(x,y,z+h+8),r*1.2,16,mid,midshadow,10,.2)
    box('Watchtower_Window_%02d'%i,(x,y-r*.91,z+h*.78),(r*.45,.3,7),mid,slate,.06)
    rock('Watchtower_Foundation_%02d'%i,x,y,-45,r*2.9,r*1.8,z+48,mid,[midshadow,midstone,midlight])

ridge('Far_Mountain_Range',540,1800,252,far,[farest,farhigh],73)
ridge('Middle_Mountain_Range',380,1400,193,far,[farstone,farhigh,farest],123)
ridge('Valley_Mountain_Range',280,1100,132,far,[farstone,farhigh],311)
for i,x in enumerate([-430,-320,-145,17,180,350,470]):
    rock('Far_Mesa_%02d'%i,x,240,-48,35+RNG.random()*30,20,88+RNG.random()*75,far,[farstone,farhigh])

# Repetition in the distance is intentional; details disappear with depth.
for i in range(19):
    x=-440+i*47
    if i in [3,7,8,14]:continue
    h=RNG.uniform(22,43);base=-28
    box('Horizon_Citadel_%02d'%i,(x,405,base+h/2),(12,18,h),far,farest,.1)
    cylinder('Horizon_Citadel_Crown_%02d'%i,(x,405,base+h+4),8,8,far,farhigh,8,.01)

# A terraced canyon floor anchors distant structures. It remains entirely behind Y=14.
nx,ny=54,12
vs=[]
for j in range(ny):
    y=14+j/(ny-1)*210
    for i in range(nx):
        x=-345+i/(nx-1)*690
        z=-37+16*(j/(ny-1))+3*math.sin(x*.052)+1.8*math.sin(y*.08+x*.025)
        vs.append((x,y,z))
fs=[(j*nx+i,j*nx+i+1,(j+1)*nx+i+1,(j+1)*nx+i) for j in range(ny-1) for i in range(nx-1)]
make_mesh('Terraced_Valley_Floor',vs,fs,floor_collection,midshadow)

# Carved friezes live on cosmetic front faces, below/inside the existing silhouettes.
for x,z,w in [(-250,10,45),(-150,10,19),(-113,11,18),(-58,12,23),
              (38,43,22),(63,70,41),(135,85,73),(199,45,42),(178,16,45)]:
    y=-5.61
    box('Carved_Frieze_Panel_%d'%x,(x,y,z),(w,.15,2.0),facade,darkstone,.12)
    for k in range(int(w/5)):
        xx=x-w/2+2.5+k*5
        diamond('Frieze_Diamond_%d_%d'%(x,k),xx,y-.13,z,1.8,1.25,facade,lightstone)

# Broad shallow relief makes the solid climbing masses feel like authored architecture.
for i,(x,z,r) in enumerate([(43,66,5),(139,78,6),(196,35,4.5)]):
    torus('Carved_Route_Rosette_%02d'%i,(x,-5.72,z),r,.22,facade,darkstone)
    torus('Rosette_Inner_%02d'%i,(x,-5.79,z),r*.72,.1,facade,lightstone)
    for j in range(8):
        t=j*math.tau/8
        diamond('Rosette_Petal_%02d_%02d'%(i,j),x+r*.48*math.cos(t),-5.9,z+r*.48*math.sin(t),
                .5,1.9,facade,lightstone)

# Sparse shrubs and rubble behind the corridor; they never become new landing surfaces.
for i in range(38):
    x=RNG.uniform(-290,248);y=RNG.uniform(17,27)
    z=RNG.uniform(-5,3)
    rock('Loose_Stone_%02d'%i,x,y,z,RNG.uniform(.8,2.5),RNG.uniform(.7,2),RNG.uniform(1,3),near,[stone,darkstone])
for i,(x,z,h) in enumerate([(-161,6,9),(-99,3,8),(-34,8,9),(71,12,11),(211,8,10)]):
    olive('Courtyard_Olive_%02d'%i,x,28,z,h,near)

# Foreground lives below the route: dark, sparse framing without covering landing edges.
for i,(x,z,s,h) in enumerate([(-292,-14,18,17),(-179,-20,20,17),(-92,-18,17,15),
                             (-30,-22,20,18),(119,-28,24,19),(258,-18,23,17)]):
    rock('Foreground_Shale_%02d'%i,x,-24,z,s,7,h,fore,[foremat,slate])

# Background sky is a real separate surface, with a gentle horizon-to-zenith gradient.
sky=bpy.data.materials.new('SAN_Atmospheric_Sky');sky.use_nodes=True
n=sky.node_tree.nodes;l=sky.node_tree.links;n.clear()
tc=n.new('ShaderNodeTexCoord');tc.location=(-600,0)
sep=n.new('ShaderNodeSeparateXYZ');sep.location=(-400,0)
ramp=n.new('ShaderNodeValToRGB');ramp.location=(-200,0)
ramp.color_ramp.elements[0].position=.05;ramp.color_ramp.elements[0].color=(.49,.53,.44,1)
ramp.color_ramp.elements[1].position=.82;ramp.color_ramp.elements[1].color=(.10,.24,.31,1)
ramp.color_ramp.elements.new(.4).color=(.30,.44,.43,1)
emit=n.new('ShaderNodeEmission');emit.location=(30,0);emit.inputs['Strength'].default_value=.8
out=n.new('ShaderNodeOutputMaterial');out.location=(220,0)
l.new(tc.outputs['Generated'],sep.inputs[0]);l.new(sep.outputs['Z'],ramp.inputs[0])
l.new(ramp.outputs['Color'],emit.inputs['Color']);l.new(emit.outputs[0],out.inputs['Surface'])
box('Sky_Backdrop',(0,1050,300),(2600,2,1600),lights,sky,0)

world=bpy.data.worlds.new('SAN_Soft_Desert_Ambient');world.use_nodes=True
world.node_tree.nodes['Background'].inputs['Color'].default_value=(.40,.53,.58,1)
world.node_tree.nodes['Background'].inputs['Strength'].default_value=.55
scene.world=world
def sun(name,rot,color,energy,size):
    ld=bpy.data.lights.new('SAN_'+name,'SUN');ld.energy=energy;ld.color=color;ld.angle=size
    lo=bpy.data.objects.new('SAN_'+name,ld);lights.objects.link(lo);lo.rotation_euler=rot
sun('Warm_Raking_Sun',(math.radians(28),math.radians(-32),math.radians(-25)),(1,.80,.59),2.3,.08)
sun('Cool_Valley_Fill',(math.radians(45),math.radians(40),math.radians(155)),(.48,.71,1),.6,.25)

def camera(name,position,target,ortho=None,lens=38):
    ca=bpy.data.cameras.new('SAN_'+name);co=bpy.data.objects.new('SAN_'+name,ca)
    cameras.objects.link(co);co.location=position
    co.rotation_euler=(Vector(target)-co.location).to_track_quat('-Z','Y').to_euler()
    ca.lens=lens;ca.clip_end=3000;ca.clip_start=.1
    if ortho:ca.type='ORTHO';ca.ortho_scale=ortho
    co['RGB_note']='Added composition review camera; source contains no gameplay camera.'
    return co

overview=camera('CAM_00_Whole_Route',(-20,-900,149),(-20,50,71),ortho=660)
entry=camera('CAM_01_Entrance_Perspective',(-235,-153,31),(-235,0,31),lens=38)
valley=camera('CAM_02_Aqueduct_Perspective',(-118,-149,34),(-118,0,34),lens=36)
climb=camera('CAM_03_Threshold_Perspective',(20,-170,62),(20,0,62),lens=36)
sanctum=camera('CAM_04_Sanctuary_Perspective',(158,-205,85),(158,0,85),lens=38)
depthcam=camera('CAM_05_Depth_Layers',(-380,-460,300),(-15,165,60),ortho=760)
scene.camera=overview
scene.render.engine='CYCLES';scene.cycles.samples=32
scene.cycles.use_denoising=True
scene.render.resolution_x=2000;scene.render.resolution_y=900
scene.render.resolution_percentage=100
scene.render.image_settings.file_format='PNG'
scene.view_settings.view_transform='AgX'
scene.view_settings.exposure=.1
scene['RGB_environment_concept']='SANCTUARY OF THE FIRST LIGHT | player proceeds toward positive X'
scene['RGB_background_depth']='near Y14-65; middle Y65-190; far Y190-650. Valley floor in separate collection.'
scene['RGB_review_note']='Review cameras are illustrative; validate final framing with Unreal gameplay camera.'

# Resolve paths while the original .blend still owns the relative base, then pack textures.
missing=[]
for im in bpy.data.images:
    if im.source=='FILE' and im.filepath:
        p=Path(bpy.path.abspath(im.filepath))
        if p.is_file():
            im.filepath=str(p)
            try:im.pack()
            except RuntimeError:pass
        else:missing.append(dict(image=im.name,path=str(p)))

after={o.name:fingerprint(o) for o in originals}
changed=[n for n in before if before[n]!=after[n]]
assert not changed, f'Protected source changed: {changed}'
report=dict(source_file=str(SOURCE),source_sha256=source_hash,fbx_sha256=fbx_hash,
            all_original_objects=len(originals),protected_unreal_objects=len(route.objects),
            preserved_all_original_geometry_transforms_material_slots=True,
            changed_original_objects=changed,original_fingerprints=before,
            original_terrain_retained_hidden_collection=legacy.name,
            missing_images=missing,new_meshes=sum(o.type=='MESH' and o not in originals for o in scene.objects),
            new_cameras=[c.name for c in cameras.objects],
            blender=bpy.app.version_string,units=scene.unit_settings.system,
            source_contains_camera=False,
            depth_layers={near.name:[14,65],mid.name:[65,190],far.name:[190,650]})
(OUT/'verification_build.json').write_text(json.dumps(report,indent=2),encoding='utf8')

notes='''SANCTUARY OF THE FIRST LIGHT\n\nEnvironment interpretation for RGB. Player moves toward +X. Camera-side is -Y.\n\n00_PROTECTED_Unreal_Greybox contains every Combined_* mesh in its ORIGINAL saved transform.\nAll 44 original objects retain original geometry, transforms, UVs, parent links and material slots.\nRoute objects are locked against accidental selection/transforms. Toggle collection selectability to edit.\nThe supplied FBX is a merged earlier route with a different Z offset: it is for identification only,\nnot a replacement for the saved Blender meshes.\n\n02_TEMPLATE_Previous_Terrain retains the existing terrains/rocks unchanged, disabled by default.\nTurn the collection's viewport/render switches on to compare with your previous version.\nOriginal mechanical details remain visible in 01_TEMPLATE_Mechanical_Details.\n\n05_VISUAL_Skins_and_Wall_Relief contains optional cosmetic facades over existing metal side faces.\nDisable it to see original gameplay materials. No landings or tread geometry are edited.\nGameplay-color slots are not replaced. Facades are cosmetic, not collision meshes.\n\n10_BG_NEAR: gate, threshold, outcrops, trees and waystones; positive-Y scenery.\n20_BG_MIDDLE: aqueduct, sanctuary, domes, colonnades, watchtowers.\n30_BG_FAR: three mountain ranges and distant citadel silhouettes.\n40_FOREGROUND: low framing rocks below the route.\n80_LIGHTING: review lighting and atmospheric sky.\n90_REVIEW_Cameras: whole-route composition, four side-on perspective views, depth overview.\nThe source file contains no camera. These are review cameras, not an export of the gameplay camera.\nChoose a camera in Scene Properties > Camera, then press Numpad 0.\n\nMost stone pieces have editable bevel modifiers. All new meshes have initial UV coordinates.\nMaterials are procedural Blender previews; FBX does NOT transfer these node networks to Unreal.\nRebuild materials in Unreal or bake maps when committing to the art direction.\nExport only SAN_ geometry, layer by layer. Keep NoCollision on cosmetic background/foreground.\nDo not reimport the protected greybox as new collision. Exclude review cameras and lights.\nNo gameplay code, Unreal assets or map settings were changed by this attempt.\n'''
textblock=bpy.data.texts.new('READ_ME - Sanctuary of the First Light');textblock.write(notes)
(OUT/'README.txt').write_text(notes,encoding='utf8')

# Save a useful opening view and a clean selection.
bpy.ops.object.select_all(action='DESELECT')
bpy.context.view_layer.objects.active=overview
overview.select_set(True)
for screen in bpy.data.screens:
    for area in screen.areas:
        if area.type=='VIEW_3D':
            space=area.spaces.active
            space.region_3d.view_perspective='CAMERA'
            space.region_3d.view_camera_zoom=0
            space.clip_end=5000
            space.shading.type='MATERIAL'
            space.shading.use_scene_world=True
            space.shading.use_scene_lights=True
scene.render.filepath=str(OUT/'sanctuary_overview.png')
bpy.context.preferences.filepaths.save_version=0
bpy.ops.wm.save_as_mainfile(filepath=str(DEST),compress=True)
print('SAVED_RESULT',str(DEST))
print('ORIGINAL_OBJECTS_PRESERVED',len(originals))
print('NEW_MESHES',report['new_meshes'])
print('MISSING_IMAGES',missing)

# Render with explicit side views and the overview, then restore the saved opening view.
for cam,filename,w,h in [(overview,'sanctuary_overview.png',2000,900),
                         (entry,'01_entrance.png',1600,900),
                         (valley,'02_aqueduct.png',1600,900),
                         (climb,'03_threshold.png',1600,900),
                         (sanctum,'04_sanctuary.png',1600,900),
                         (depthcam,'05_depth_layers.png',1800,1100)]:
    scene.camera=cam
    scene.render.resolution_x=w;scene.render.resolution_y=h
    scene.render.filepath=str(OUT/filename)
    bpy.ops.render.render(write_still=True)
    print('RENDERED',filename,flush=True)
