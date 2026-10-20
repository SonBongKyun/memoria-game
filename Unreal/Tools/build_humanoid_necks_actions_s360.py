"""S360 R9: blended neck bridges on R8 binds, and complete compatible Elia actions."""
from pathlib import Path
import bpy,math,sys,json,importlib.util
from mathutils import Vector,Matrix
ROOT=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location('s360',ROOT/'Unreal/Tools/build_humanoid_anatomy_s360.py');b=importlib.util.module_from_spec(spec);spec.loader.exec_module(b)
m=b.m;OUT=ROOT/'Unreal/ArtSource/HumanoidAnatomyS360/R9'
def normalize(ob):
 me=ob.data;uv={u.name:[tuple(v.uv) for v in u.data] for u in me.uv_layers};cs=[tuple(v.color) for v in me.color_attributes['Color'].data]
 while len(me.color_attributes):me.color_attributes.remove(me.color_attributes[0])
 me.color_attributes.new(name='Color',type='FLOAT_COLOR',domain='CORNER')
 for a,c in zip(me.color_attributes[0].data,cs):a.color=c
 for n,values in uv.items():
  for a,u in zip(me.uv_layers[n].data,values):a.uv=u
def neck(name):
 bpy.ops.wm.read_factory_settings(use_empty=True);src=ROOT/'Unreal/ArtSource/HumanoidAnatomyS360/R8'/name/(name+'.fbx');bpy.ops.import_scene.fbx(filepath=str(src),colors_type='LINEAR');rig=next(o for o in bpy.data.objects if o.type=='ARMATURE');ob=next(o for o in bpy.data.objects if o.type=='MESH');ob.parent=None;ob.matrix_world=Matrix.Identity(4);rig.scale=(1,1,1);rig.rotation_euler=(0,0,0);rig.location=(0,0,0);rig.animation_data_clear()
 m.RIG=rig;m.NAME=name;m.H=json.loads((src.parent/'geometry.json').read_text())['height_cm'];m.K=m.H/180;m.PARTS=[];m.COMPONENTS=[];m.MAT=ob.data.materials[2];K=m.K
 for p in rig.pose.bones:p.matrix_basis=Matrix.Identity(4)
 face={i for f in ob.data.polygons if f.material_index==2 for i in f.vertices};body={i for f in ob.data.polygons if f.material_index!=2 for i in f.vertices}
 low=max(ob.data.vertices[i].co.z for i in body)-4*K;high=min(ob.data.vertices[i].co.z for i in face)+4*K
 cy=m.bn('neck_01').y;vs=[];fs=[];n=48;rows=12
 for row in range(rows+1):
  t=row/rows;z=low+(high-low)*t;r=(5.7-1.0*math.sin(math.pi*t))*K
  for i in range(n+1):
   a=math.tau*i/n;vs.append((math.sin(a)*r,cy+math.cos(a)*r*.91,z))
 for row in range(rows):
  for i in range(n):a=row*(n+1)+i;fs.append((a,a+1,a+n+2,a+n+1))
 def w(p,t):return m.blend('neck_01','head',b.clamp((p.z-low)/(high-low)))
 skin=(.095,.082,.074) if name=='AshWalker' else (.36,.23,.15)
 bridge=m.mesh('ContinuousMixedWeightNeck',vs,fs,w,skin,0)
 for part in [ob,bridge]:normalize(part)
 bpy.ops.object.select_all(action='DESELECT');ob.select_set(True);bridge.select_set(True);bpy.context.view_layer.objects.active=ob;bpy.ops.object.join()
 ob.data.normals_split_custom_set([(0,0,0)]*len(ob.data.loops));ob.parent=rig;ob.matrix_parent_inverse=Matrix.Identity(4);ob.matrix_basis=Matrix.Identity(4)
 for mod in list(ob.modifiers):ob.modifiers.remove(mod)
 mod=ob.modifiers.new('Skin','ARMATURE');mod.object=rig;bpy.context.scene.unit_settings.system='METRIC';bpy.context.scene.unit_settings.scale_length=.01
 dest=OUT/name;dest.mkdir(parents=True,exist_ok=False);bpy.ops.object.select_all(action='DESELECT');ob.select_set(True);rig.select_set(True);bpy.context.view_layer.objects.active=rig;m.export(dest/(name+'.fbx'))
 (dest/'geometry.json').write_text(json.dumps({'name':name,'source':str(src.relative_to(ROOT)),'source_sha256':b.sha(src),'output_sha256':b.sha(dest/(name+'.fbx')),'generator_sha256':b.sha(Path(__file__)),'height_cm':m.H,'neck_low_high_cm':[low,high],'neck_rings':rows+1,'bones':77,'policy':'R8 positions/UV/materials/binds retained; additional neck bridges with mixed neck/head weights'},indent=2)+'\n');print('S360_NECK_SOURCE',name,low,high,flush=True)
def actions():
 b.OUT=ROOT/'Unreal/ArtSource/HumanoidAnatomyS360/R7';src,ob=b.motionload('Elia');m.H=json.loads((src.parent/'geometry.json').read_text())['height_cm'];m.K=m.H/180;rig=m.RIG;dest=OUT/'Elia';dest.mkdir(exist_ok=False);records=[];scene=bpy.context.scene;scene.render.fps=30
 roles=['Attack_01','Attack_02','Attack_03','ChargedAttack','Dash','HitReact_Front_Lgt_01','Death_Front_01']
 for role in roles:
  rig.animation_data_clear();end=60;scene.frame_start=0;scene.frame_end=end
  for frame in range(end+1):
   scene.frame_set(frame);t=frame/end;m.reset_pose();m.world_move('pelvis',(0,0,-1.4*m.K));hit=0
   if role=='HitReact_Front_Lgt_01':
    hit=math.sin(math.pi*t)**2;attack=wind=0;m.world_rotate('spine_02',(1,0,0),-.18*hit)
   else:
    attack=math.sin(math.pi*.5*min(1,t/.55))**3 if t<=.55 else math.cos(math.pi*.5*(t-.55)/.45)**2;wind=math.sin(math.pi*min(1,t/.32))**2 if t<.32 else 0
    m.world_rotate('spine_01',(0,0,1),-.12*wind+.20*attack);m.world_rotate('spine_02',(1,0,0),-.05*wind+.10*attack)
   bpy.context.view_layer.update()
   for side in ['l','r']:m.contact_leg(side,0,False);m.arm(side,t,False,attack,wind,hit)
   if role=='Death_Front_01':
    q=b.clamp(t/.78);q=q*q*(3-2*q);m.world_rotate('pelvis',(1,0,0),1.42*q);m.world_move('pelvis',(0,12*m.K*q,-72*m.K*q))
   for p in rig.pose.bones:
    assert all(abs(v-1)<.0001 for v in p.scale)
    for prop in ['location','rotation_quaternion','scale']:p.keyframe_insert(data_path=prop,frame=frame)
  bpy.ops.object.select_all(action='DESELECT');rig.select_set(True);bpy.context.view_layer.objects.active=rig;m.export(dest/(role+'.fbx'),True);records.append({'clip':role,'sha256':b.sha(dest/(role+'.fbx')),'unit_scales':True})
 (dest/'clips.json').write_text(json.dumps({'name':'Elia','source_sha256':b.sha(src),'generator_sha256':b.sha(Path(__file__)),'clips':records,'timing':'import resamples to each original Elia combat deadline'},indent=2)+'\n')
if __name__=='__main__':
 OUT.mkdir(parents=True,exist_ok=True)
 for name in ['AshWalker','BeltScavenger']:neck(name)
 actions();(OUT/'source_inventory.json').write_text(json.dumps({'revision':'R9','generator_sha256':b.sha(Path(__file__)),'files':[{'path':str(p.relative_to(OUT)),'sha256':b.sha(p)} for p in sorted(OUT.rglob('*')) if p.is_file() and p.name!='source_inventory.json']},indent=2)+'\n');print('S360_NECK_ACTION_AUTHORING_COMPLETE',flush=True)
