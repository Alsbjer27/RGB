"""PIE regression: real platform landings and the HUD material across all Foundry rooms. Saves no assets."""
import unreal as u
import time, os, json, traceback
out=os.path.join(u.Paths.project_saved_dir(),'LevelDesign/foundry_hud_pie.json')
levels=u.get_editor_subsystem(u.LevelEditorSubsystem)
levels.load_level('/Game/RGB/Maps/L_ChromaticFoundry')
s={'phase':'start','next':time.monotonic()+3,'arena':1,'pad':0,'drops':0,'busy':False}
r={'checks':[],'finished':False}
def save():
 with open(out,'w') as f: json.dump(r,f,indent=2)
def check(ok,msg):
 r['checks'].append({'passed':bool(ok),'description':msg}); save()
 if not ok: raise RuntimeError(msg)
def pawn(): return u.GameplayStatics.get_player_character(s['world'],0)
def ctrl(i=None): return s['labels']['A%d_Control_%s'%((i or s['arena']),('RED','GREEN','BLUE')[(i or s['arena'])-1])]
def move(x,z):
 p=pawn(); m=p.get_component_by_class(u.CharacterMovementComponent); m.stop_movement_immediately(); p.set_actor_location(u.Vector(x,0,z),False,True); m.set_movement_mode(u.MovementMode.MOVE_FALLING)
def hudcheck(label,active=True):
 h=s['hud']; expected=ctrl().get_completion_fraction() if active else 0
 check(h.get_displayed_arena()==(ctrl() if active else None),label+' selects the correct arena')
 check(abs(h.get_arena_progress_target()-expected)<0.001,label+' target matches arena progress')
 image=h.get_editor_property('Image_ArenaLiquid'); mat=image.get_dynamic_material(); fill=mat.get_scalar_parameter_value('FillAmount')
 check(abs(fill-expected)<0.015,label+' visible material fill matches progress')
def tick(dt):
 if s['busy'] or time.monotonic()<s['next']: return
 s['busy']=True
 try:
  phase=s['phase']
  if phase=='start': levels.editor_request_begin_play(); s.update(phase='ready',next=time.monotonic()+5); return
  if phase=='ready':
   worlds=u.EditorLevelLibrary.get_pie_worlds(False)
   if not worlds: s['next']=time.monotonic()+1; return
   s['world']=worlds[0]; s['labels']={a.get_actor_label():a for a in u.GameplayStatics.get_all_actors_of_class(s['world'],u.Actor)}
   widgets=u.WidgetLibrary.get_all_widgets_of_class(s['world'],u.RGBHUDWidget,True)
   check(len(widgets)==1,'Existing HUD is created with the native arena presenter'); s['hud']=widgets[0]
   s['performance']=u.get_default_object(u.load_class(None,'/Script/UnrealEd.EditorPerformanceSettings')); s['old_throttle']=s['performance'].get_editor_property('bThrottleCPUWhenNotForeground'); s['performance'].set_editor_property('bThrottleCPUWhenNotForeground',False)
   # Isolate HUD colour changes from automatic boost-pad launches during repeated drops.
   for boost in u.GameplayStatics.get_all_actors_of_class(s['world'],u.RGBJumpPad): boost.set_actor_enable_collision(False)
   for i in (1,2,3):
    for e in ctrl(i).get_editor_property('assigned_enemies'):
     e.set_editor_property('attack_damage',0)
     if isinstance(e,u.RGBTeleporterEnemyCharacter): e.set_editor_property('AoEDamage',0)
     e.set_actor_enable_collision(False)
   hudcheck('Arrival outside arenas',False); s.update(phase='supplies',supply=0,next=time.monotonic()+.2); return
  if phase=='supplies':
   if s['supply']<4: move(-900+s['supply']*500,110); s['supply']+=1; s['next']=time.monotonic()+1; return
   s.update(phase='enter',next=time.monotonic()+.1); return
  if phase=='enter':
   i=s['arena']; move((1740,11240,20740)[i-1],(110,1010,1910)[i-1]); s.update(phase='entrycheck',next=time.monotonic()+2); return
  if phase=='entrycheck':
   hudcheck('Arena %d entry'%s['arena']); s.update(phase='land',pad=0,drops=0,next=time.monotonic()+.1); return
  if phase=='land':
   i=s['arena']; pads=ctrl().get_editor_property('assigned_platforms')
   hudcheck('Arena %d pad %d'%(i,s['pad']))
   if s['pad']==len(pads): s.update(phase='completed',next=time.monotonic()+2); return
   pad=pads[s['pad']]; c=pad.get_component_by_class(u.RGBColorComponent)
   if c.get_editor_property('current_color')==ctrl().get_editor_property('required_color'):
    s['pad']+=1; s['drops']=0; return
   check(s['drops']<4,'Platform responds to repeated real landings'); origin,extent=pad.get_actor_bounds(False); move(pad.get_actor_location().x,origin.z+extent.z+250); s['drops']+=1; s['next']=time.monotonic()+1.8; return
  if phase=='completed':
   check(ctrl().is_completed(),'Arena %d completes'%s['arena']); hudcheck('Arena %d completed'%s['arena'])
   move((8000,17500,28000)[s['arena']-1],(300,1200,1910)[s['arena']-1]); s.update(phase='outside',next=time.monotonic()+.6); return
  if phase=='outside':
   hudcheck('Arena %d departure'%s['arena'],False)
   if s['arena']<3: s['arena']+=1; s.update(phase='enter',next=time.monotonic()+.1)
   else: move(20740,1910); s.update(phase='reenter',next=time.monotonic()+2)
   return
  if phase=='reenter':
   hudcheck('Completed arena reentry'); pawn().recieve_damage(999); s.update(phase='reset',next=time.monotonic()+4); return
  if phase=='reset':
   widgets=u.WidgetLibrary.get_all_widgets_of_class(s['world'],u.RGBHUDWidget,True); check(len(widgets)==1,'Reset creates one HUD'); s['hud']=widgets[0]; move(20740,1910); s.update(phase='resetcheck',next=time.monotonic()+2); return
  if phase=='resetcheck':
   hudcheck('Arena reset'); check(not ctrl().is_completed(),'Reset clears arena completion'); s['performance'].set_editor_property('bThrottleCPUWhenNotForeground',s['old_throttle']); r['finished']=True; save(); u.log('RGB_HUD_PIE_ALL_CHECKS_PASSED'); levels.editor_request_end_play(); u.unregister_slate_post_tick_callback(handle)
 except Exception:
  if 'performance' in s and 'old_throttle' in s: s['performance'].set_editor_property('bThrottleCPUWhenNotForeground',s['old_throttle'])
  r['error']=traceback.format_exc(); save(); u.log_error(r['error']); levels.editor_request_end_play(); u.unregister_slate_post_tick_callback(handle)
 finally: s['busy']=False
handle=u.register_slate_post_tick_callback(tick)
save()






