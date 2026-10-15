"""S358 immutable clothing/body refinement. Blender 4.5; no raster editing.
Reuse original paint/materials, 77-bone binds and clips. Retain S357 faces exactly.
"""
from pathlib import Path
import bpy,bmesh,math,json,hashlib,sys
from mathutils import Vector,Matrix
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'Unreal/ArtSource/HumanoidBodiesS358/R3'
NPCS=['Traveler','Bureauagent','Guard','Villagerf','Villagerm','Fisherman','Elder','Child','Scholar']
NAMES=NPCS+['Elia','Malet','AshWalker','BeltScavenger']
PARTS=[];RIG=None;OLD=None;K=1;FOE=False;NAME='';COMPONENTS=[]
PALETTE=[(.10,.085,.06),(.060,.033,.018),(.18,.15,.10),(.30,.24,.17),(.045,.038,.029),(.23,.18,.10),(.16,.11,.06),(.075,.085,.070)]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def bn(n):return RIG.data.bones[n].head_local.copy()
def fixed(n):return lambda p,t:{n:1.0}
def between(a,b,u):return {a:1-u,b:u} if 0<u<1 else {a if u<=0 else b:1.}
def torso(p,t=0):
 names=['pelvis','spine_01','spine_02','spine_03'];z=p.z
 for a,b in zip(names,names[1:]):
  za,zb=bn(a).z,bn(b).z
  if z<zb:return between(a,b,max(0,min(1,(z-za)/(zb-za))))
 return {'spine_03':1.}
def bounds(vs):return [[min(v[i] for v in vs),max(v[i] for v in vs)] for i in range(3)]
def weights(ob,v):return sorted((ob.vertex_groups[g.group].name,round(g.weight,6)) for g in v.groups if g.weight>1e-7)
def corner_digest(ob,limit=None,slot=None):
 rows=[];uv=ob.data.uv_layers.active;rgba=ob.data.color_attributes.active_color
 for p in ob.data.polygons:
  if slot is not None and p.material_index!=slot:continue
  if limit is not None and any(i>=limit for i in p.vertices):continue
  for li in p.loop_indices:
   v=ob.data.vertices[ob.data.loops[li].vertex_index]
   rows.append((p.material_index,tuple(round(x,5) for x in v.co),tuple(round(x,6) for x in uv.data[li].uv),tuple(round(x,6) for x in rgba.data[li].color) if rgba else None,weights(ob,v)))
 return sha_bytes(repr(sorted(rows)).encode()),len(rows)
def sha_bytes(b):return hashlib.sha256(b).hexdigest()
def normal_key(me,p,li):
 return (p.material_index,tuple(sorted(tuple(round(x,5) for x in me.vertices[v].co) for v in p.vertices)),tuple(round(x,5) for x in me.vertices[me.loops[li].vertex_index].co),tuple(round(x,6) for x in me.uv_layers.active.data[li].uv))
def mesh(label,vs,fs,uv,ws,tile=0,cat=.75):
 me=bpy.data.meshes.new(label);me.from_pydata(vs,[],fs);me.update();ob=bpy.data.objects.new(label,me);bpy.context.collection.objects.link(ob)
 slot=0 if FOE or NAME in NPCS else 1;me.materials.append(OLD.data.materials[slot])
 me.uv_layers.new(name='UVMap');me.color_attributes.new(name='Color',type='FLOAT_COLOR',domain='CORNER')
 # Refresh both references after CustomData allocation; S357 stale RNA bug must not return.
 uvl=me.uv_layers['UVMap'];col=me.color_attributes['Color']
 pigment=(.125,.118,.10) if FOE and NAME=='AshWalker' and cat==.25 else PALETTE[tile%8]
 for p in me.polygons:
  p.use_smooth=True
  for li in p.loop_indices:
   vi=me.loops[li].vertex_index;u,v=uv[vi]
   uvl.data[li].uv=(u,v) if FOE else ((tile%4+.04+u*.92)/4,1-(tile//4+.04+v*.92)/4)
   col.data[li].color=(*pigment,cat if FOE else 1.)
 for i,weight in enumerate(ws):
  total=sum(weight.values());assert total>.99
  for n,w in weight.items():
   if w>1e-7:(ob.vertex_groups.get(n) or ob.vertex_groups.new(name=n)).add([i],w/total,'REPLACE')
 PARTS.append(ob);COMPONENTS.append({'name':label,'vertices':len(vs),'tile':tile,'alpha':cat if FOE else 1.});return ob
def tube(label,points,radii,weight,tile=0,cat=.75,sides=16,flat=1,fold=.025):
 ps=list(map(Vector,points));vs=[];uv=[];ws=[];fs=[]
 for j,(p,r) in enumerate(zip(ps,radii)):
  axis=(ps[min(j+1,len(ps)-1)]-ps[max(0,j-1)]).normalized();ref=Vector((0,-1,0))
  if abs(axis.dot(ref))>.95:ref=Vector((1,0,0))
  a=(ref-axis*ref.dot(axis)).normalized();b=axis.cross(a)
  for i in range(sides+1):
   ang=math.tau*i/sides;rr=r*(1+fold*(math.sin(ang*3+j*.71)+.4*math.sin(ang*5-j*.4)))
   q=p+a*(math.cos(ang)*rr*flat)+b*(math.sin(ang)*rr);vs.append(q);uv.append((i/sides,j/(len(ps)-1)));ws.append(weight(q,j/(len(ps)-1)))
 for j in range(len(ps)-1):
  for i in range(sides):a=j*(sides+1)+i;fs.append((a,a+1,a+sides+2,a+sides+1))
 fs.extend([tuple(range(sides,-1,-1)),tuple(range((len(ps)-1)*(sides+1),len(ps)*(sides+1)))])
 return mesh(label,vs,fs,uv,ws,tile,cat)
def panel(label,rows,weight,tile=2,cat=.5):
 vs=[];uv=[];ws=[];fs=[]
 for j,row in enumerate(rows):
  for i,p in enumerate(row):p=Vector(p);vs.append(p);uv.append((i/(len(row)-1),j/(len(rows)-1)));ws.append(weight(p,j/(len(rows)-1)))
 cols=len(rows[0]);assert all(len(r)==cols for r in rows)
 for j in range(len(rows)-1):
  for i in range(cols-1):a=j*cols+i;fs.append((a,a+1,a+cols+1,a+cols))
 return mesh(label,vs,fs,uv,ws,tile,cat)
def seam(label,points,bone,tile=2,r=.38):return tube(label,points,[r*K]*len(points),fixed(bone),tile,1. if tile==5 else .5,sides=8,fold=0)
def build_foe_body():
 hip=bn('pelvis').z;cy=bn('spine_03').y;sh=bn('upperarm_l').z;neck=bn('neck_01').z;w=abs(bn('upperarm_l').x)
 profile=[(hip-8*K,.87,10.1),(hip,.88,10.7),(hip+8*K,.76,10.5),(hip+17*K,.78,11.1),(sh-18*K,.87,11.6),(sh-10*K,1.0,11.0),(sh-3*K,1.06,9.5),(sh+1*K,.93,8),(neck+1.7*K,.23,4.8)]
 vs=[];uv=[];ws=[];fs=[];sides=40
 for j,(z,wide,dep) in enumerate(profile):
  for i in range(sides+1):
   a=math.tau*i/sides;fold=.28*K*math.sin(a*5+j*.6);p=Vector((math.sin(a)*(w*wide+fold),cy+math.cos(a)*(dep*K+fold*.45),z))
   vs.append(p);uv.append((i/sides,j/(len(profile)-1)));ws.append(torso(p))
 for j in range(len(profile)-1):
  for i in range(sides):a=j*(sides+1)+i;fs.append((a,a+1,a+sides+2,a+sides+1))
 mesh('FittedWaistcoat',vs,fs,uv,ws,0,.5 if NAME=='BeltScavenger' else .75)
 for side in ['l','r']:
  shpt,el,wr=bn('upperarm_'+side),bn('lowerarm_'+side),bn('hand_'+side)
  points=[shpt.lerp(el,t) for t in [0,.12,.33,.58,.83,1]]+[el.lerp(wr,t) for t in [.2,.43,.67,.87,1]]
  def aw(p,t,s=side):return between('upperarm_'+s,'lowerarm_'+s,max(0,min(1,(t-.36)*5)))
  tube('ArticulatedSleeve_'+side,points,[r*K for r in [7.8,8,7.5,6.6,5.1,4.5,5.0,4.9,4.1,3.6,3.3]],aw,7,.75,sides=24,flat=.88)
  # Local taper and three layered folds replace spherical shoulder pads.
  d=(el-shpt).normalized()
  for j in range(3):
   t=.12+j*.12;p=shpt.lerp(el,t)
   tube('LayeredShoulder_'+side+str(j),[p-d*1.6*K,p+d*1.6*K],[(9.3-j*.35)*K,(9.1-j*.35)*K],fixed('upperarm_'+side),1,.5,sides=20,flat=.88,fold=.012)
  d=(wr-el).normalized()
  tube('TaperedBracer_'+side,[el.lerp(wr,.47),el.lerp(wr,.70),wr-d*2*K],[5.5*K,5.0*K,3.8*K],fixed('lowerarm_'+side),1,.5,sides=20,flat=.85)
  for t in [.54,.86]:
   p=el.lerp(wr,t);tube('BracerBinding_'+side+str(t),[p-d*.55*K,p+d*.55*K],[5.4*K if t<.6 else 4.4*K]*2,fixed('lowerarm_'+side),2,.5,sides=16,flat=.85,fold=0)
  thigh,knee,ank=bn('thigh_'+side),bn('calf_'+side),bn('foot_'+side)
  points=[thigh.lerp(knee,t) for t in [0,.15,.36,.63,.85,1]]+[knee.lerp(ank,t) for t in [.2,.43,.68,.88,1]]
  def lw(p,t,s=side):return between('thigh_'+s,'calf_'+s,max(0,min(1,(t-.36)*5)))
  tube('ShapedTrousers_'+side,points,[r*K for r in [8.4,8.8,8.1,7.4,6.2,5.3,5.7,5.1,4.3,4.1,4]],lw,7,.75,sides=24,flat=.88)
  points=[knee.lerp(ank,t) for t in [.42,.57,.79,.98]]
  tube('BootShaft_'+side,points,[5.6*K,5.5*K,4.9*K,4.4*K],fixed('calf_'+side),1,.5,sides=20,flat=.88,fold=.018)
  for t in [.50,.77]:
   p=knee.lerp(ank,t);d=(ank-knee).normalized();tube('BootStrap_'+side+str(t),[p-d*.7*K,p+d*.7*K],[5.65*K if t<.6 else 5.25*K]*2,fixed('calf_'+side),2,.5,sides=16,flat=.88,fold=0)
 # Asymmetric lapped skirt ends above the knees and follows the pelvis/thigh, with a real walking slit.
 for sign,side in [(1,'l'),(-1,'r')]:
  rows=[]
  for j in range(8):
   t=j/7;row=[]
   for i in range(13):
    a=sign*(.22+(math.pi-.44)*i/12);rr=w*(.88+.15*t);fold=.8*K*t*math.sin(a*5+.3)
    row.append((math.sin(a)*(rr+fold),cy+math.cos(a)*(11.4*K+fold),hip+1*K-26*K*t+math.sin(a*3)*t*1.5*K))
   rows.append(row)
  def sw(p,t,s=side):return between('pelvis','thigh_'+s,t*t*.22)
  panel('SplitOvercoat_'+side,rows,sw,0,.75)
 belt=[(math.sin(a)*w*.89,cy+math.cos(a)*11.9*K,hip+2*K) for a in [math.tau*i/48 for i in range(49)]]
 tube('StructuredWaistBelt',belt,[1.7*K]*49,fixed('pelvis'),1,.5,sides=8,flat=1.5,fold=0)
 front=cy-14*K
 for x in [-2.9,2.9]:seam('BuckleSide',[(x*K,front,hip-.8*K),(x*K,front,hip+5*K)],'pelvis',5,.5)
 for z in [hip-.8*K,hip+5*K]:seam('BuckleBar',[(-2.9*K,front,z),(2.9*K,front,z)],'pelvis',5,.5)
 for sign in [-1,1]:
  seam('ChestSeam',[(sign*w*.36,cy-11.6*K,hip+8*K),(sign*w*.48,cy-12.2*K,hip+23*K),(sign*w*.60,cy-11.8*K,sh-12*K)],'spine_02',2,.36)
def continuous_npc_neck():
 start=bn('neck_01')-Vector((0,0,.7*K));end=bn('head')+Vector((0,0,5*K))
 points=[start.lerp(end,j/8) for j in range(9)]
 def nw(p,t):return between('neck_01','head',max(0,min(1,t*1.4-.16)))
 tube('BlendedSolidNeck',points,[(4.9-.5*j/8)*K for j in range(9)],nw,3,0,sides=32,flat=.94,fold=0)
 tube('RaisedNeckCollar',[start-Vector((0,0,.4*K)),start+Vector((0,0,1.3*K))],[5.35*K,5.3*K],fixed('neck_01'),0,.75,sides=32,flat=.96,fold=0)

def tailor_npc():
 hip=bn('pelvis').z;cy=bn('spine_03').y;sh=bn('upperarm_l').z;w=abs(bn('upperarm_l').x)
 if NAME in ['Elia','Malet']:
  # Their painted bodice/coat stay fully exposed. Reinforce accessories at their actual attachment.
  for sign in [-1,1]:
   seam('ShoulderPiping',[(sign*w*.76,cy+7*K,sh+1*K),(sign*(w+4*K),cy+11*K,sh-8*K),(sign*w*.91,cy+19*K,sh-22*K)],'spine_03',2,.48)
  seam('LowerBackBinding',[(-w*.65,cy+13*K,hip+3*K),(0,cy+14*K,hip+2*K),(w*.65,cy+13*K,hip+3*K)],'pelvis',1,.65)
  return
 for side,sign in [('l',1),('r',-1)]:
  el,wr=bn('lowerarm_'+side),bn('hand_'+side);d=(wr-el).normalized()
  tube('BoundCuff_'+side,[wr-d*5*K,wr-d*2*K],[4.15*K,4.0*K],fixed('lowerarm_'+side),1,.5,sides=20,flat=.92,fold=.012)
  for t in [.26,.38]:
   p=el.lerp(wr,t);tube('SleeveGather_'+side+str(t),[p-d*.65*K,p+d*.65*K],[5.0*K,4.85*K],fixed('lowerarm_'+side),0,.75,sides=20,flat=.91)
  knee,ank=bn('calf_'+side),bn('foot_'+side);d=(ank-knee).normalized()
  tube('LeatherBootTop_'+side,[knee.lerp(ank,.58),ank-d*3*K],[5.1*K,4.2*K],fixed('calf_'+side),1,.5,sides=20,flat=.85,fold=.02)
  p=knee.lerp(ank,.65);tube('BootBinding_'+side,[p-d*.75*K,p+d*.75*K],[5.25*K]*2,fixed('calf_'+side),2,.5,sides=16,flat=.85,fold=0)
  # Follow existing lapel silhouette without covering its painted face.
  seam('RolledLapel_'+side,[(sign*4*K,cy-6*K,bn('head').z-2*K),(sign*w*.75,cy-10*K,sh-4*K),(sign*8*K,cy-14.2*K,hip+18*K)],'spine_03',2,.48)
  if NAME in ['Guard','Bureauagent','Traveler']:
   p=bn('upperarm_'+side);el=bn('lowerarm_'+side);d=(el-p).normalized()
   for j in range(2):
    q=p.lerp(el,.15+j*.13);tube('ShoulderBinding_'+side+str(j),[q-d*1.6*K,q+d*1.6*K],[(8.4-.5*j)*K,(8.1-.5*j)*K],fixed('upperarm_'+side),7 if NAME=='Guard' else 1,.5,sides=20,flat=.92)
 if NAME in ['Fisherman','Villagerm','Bureauagent','Scholar']:
  rows=[]
  for j in range(9):
   t=j/8;row=[]
   for i in range(13):
    u=i/12;x=(u-.5)*w*1.3;row.append((x,cy-14*K-(.4+1.1*t)*K*math.sin(u*math.tau*3),hip+1*K-24*K*t+.8*K*t*math.sin(u*8)))
   rows.append(row)
  panel('PleatedWorkTabard',rows,fixed('pelvis'),2 if NAME in ['Bureauagent','Scholar'] else 1,.5)
  for sign in [-1,1]:seam('TabardEdge',[(sign*w*.65,cy-14.5*K,hip),(sign*w*.65,cy-14.5*K,hip-12*K),(sign*w*.65,cy-14.5*K,hip-23*K)],'pelvis',6,.32)
 for z in [hip+10*K,hip+16*K,hip+22*K]:
  tube('CoatFastener',[(0,cy-13.5*K,z),(0,cy-14.4*K,z)],[.9*K]*2,torso,5,1.,sides=12,fold=0)
def build(name):
 global NAME,FOE,K,RIG,OLD,PARTS,COMPONENTS
 NAME=name;FOE=name in ['AshWalker','BeltScavenger'];PARTS=[];COMPONENTS=[]
 source=ROOT/('Unreal/ArtSource/HumanoidFacesS357/R4' if FOE else 'Unreal/ArtSource/CharacterUpgradeS356/Accepted' if name in ['Elia','Malet'] else 'Unreal/ArtSource/CharacterUpgrade')/name/(name+'.fbx')
 source_hash=sha(source);bpy.ops.wm.read_factory_settings(use_empty=True);bpy.ops.import_scene.fbx(filepath=str(source),colors_type='LINEAR' if FOE else 'SRGB')
 RIG=next(o for o in bpy.data.objects if o.type=='ARMATURE');OLD=next(o for o in bpy.data.objects if o.type=='MESH')
 OLD.parent=None;OLD.matrix_world=Matrix.Identity(4);RIG.scale=(1,1,1);RIG.location=(0,0,0);RIG.rotation_euler=(0,0,0)
 original_bounds=bounds([v.co for v in OLD.data.vertices]);H=original_bounds[2][1]-original_bounds[2][0];K=H/180
 bones=[{'name':b.name,'parent':b.parent.name if b.parent else None,'matrix':[list(row) for row in b.matrix_local]} for b in RIG.data.bones];assert len(bones)==77
 normals={normal_key(OLD.data,p,li):tuple(OLD.data.corner_normals[li].vector) for p in OLD.data.polygons for li in p.loop_indices}
 face_before=corner_digest(OLD,slot=2) if FOE else None
 removed=[]
 if FOE:
  # Replace body primitives, but retain face/gear, articulated hands and actual feet exactly.
  bm=bmesh.new();bm.from_mesh(OLD.data);bm.verts.ensure_lookup_table();dv=bm.verts.layers.deform.active;delete=[]
  for v in bm.verts:
   if any(f.material_index in [1,2] for f in v.link_faces):continue
   ns=[OLD.vertex_groups[i].name for i,w in v[dv].items() if w>.02]
   if any(n.startswith(('hand_','thumb_','index_','middle_','ring_','pinky_','foot_','ball_')) for n in ns):continue
   delete.append(v)
  removed=[{'vertices':len(delete),'policy':'slot0 except articulated hands/fingers and feet; slots1/2 immutable'}]
  bmesh.ops.delete(bm,geom=delete,context='VERTS');bm.to_mesh(OLD.data);bm.free()
 elif name in NPCS:
  # The S354 rigid neck and chest-weighted top collar stretched into visible strips in native views.
  # Replace only that connection; painted head/hair and all other garment geometry remain intact.
  bm=bmesh.new();bm.from_mesh(OLD.data);dv=bm.verts.layers.deform.active;delete=[];neck_count=collar_count=0
  neck=bn('neck_01');cy=bn('spine_03').y
  for v in bm.verts:
   ns=[OLD.vertex_groups[i].name for i,w in v[dv].items() if w>.02]
   if ns==['neck_01']:delete.append(v);neck_count+=1
   elif ns==['spine_03'] and v.co.z>neck.z+3.2*K and (Vector((v.co.x,v.co.y-cy,0))).length<7*K:delete.append(v);collar_count+=1
  assert neck_count==32 and 40<=collar_count<=52,(name,neck_count,collar_count)
  removed=[{'policy':'replace 32-vertex rigid neck and the top chest-weighted collar ring only','neck_vertices':neck_count,'collar_vertices':collar_count}]
  bmesh.ops.delete(bm,geom=delete,context='VERTS');bm.to_mesh(OLD.data);bm.free()
 retained_normals=[normals[normal_key(OLD.data,p,li)] for p in OLD.data.polygons for li in p.loop_indices]
 retained=corner_digest(OLD);limit=len(OLD.data.vertices);PARTS=[OLD]
 if FOE:build_foe_body()
 else:
  if name in NPCS:continuous_npc_neck()
  tailor_npc()
 bpy.ops.object.select_all(action='DESELECT')
 for ob in PARTS:ob.select_set(True)
 bpy.context.view_layer.objects.active=OLD;bpy.ops.object.join();body=OLD;body.name=name+'_Body'
 for mod in list(body.modifiers):body.modifiers.remove(mod)
 tri=body.modifiers.new('Triangles','TRIANGULATE');bpy.ops.object.modifier_apply(modifier=tri.name)
 auto=[tuple(n.vector) for n in body.data.corner_normals];assert len(auto)>=len(retained_normals)
 auto[:len(retained_normals)]=retained_normals;body.data.normals_split_custom_set(auto)
 assert corner_digest(body,limit)==retained,'Retained geometry, UV, color or weights changed'
 if FOE:assert corner_digest(body,slot=2)==face_before,'S357 face changed'
 assert len(body.data.materials)==(3 if FOE else 2 if name in ['Elia','Malet'] else 1)
 assert all(v.groups and abs(sum(g.weight for g in v.groups)-1)<.001 for v in body.data.vertices)
 assert bones==[{'name':b.name,'parent':b.parent.name if b.parent else None,'matrix':[list(row) for row in b.matrix_local]} for b in RIG.data.bones]
 final_bounds=bounds([v.co for v in body.data.vertices]);height=final_bounds[2][1]-final_bounds[2][0];assert abs(height-H)<.001,(name,height,H)
 body.data.calc_loop_triangles();triangles=len(body.data.loop_triangles);assert triangles<40000,(name,triangles)
 body.parent=RIG;body.matrix_parent_inverse=Matrix.Identity(4);body.matrix_basis=Matrix.Identity(4);skin=body.modifiers.new('Skin','ARMATURE');skin.object=RIG
 bpy.context.scene.unit_settings.system='METRIC';bpy.context.scene.unit_settings.scale_length=.01
 bpy.ops.object.select_all(action='DESELECT');body.select_set(True);RIG.select_set(True);bpy.context.view_layer.objects.active=RIG
 dest=OUT/name;dest.mkdir(parents=True,exist_ok=False);fbx=dest/(name+'.fbx')
 bpy.ops.export_scene.fbx(filepath=str(fbx),use_selection=True,object_types={'ARMATURE','MESH'},axis_forward='-Y',axis_up='Z',apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',add_leaf_bones=False,use_armature_deform_only=True,path_mode='RELATIVE',bake_anim=False,mesh_smooth_type='FACE',colors_type='LINEAR' if FOE else 'SRGB')
 assert sha(source)==source_hash
 receipt={'name':name,'revision':'R3','source':str(source.relative_to(ROOT)).replace('\\','/'),'source_sha256':source_hash,'output_sha256':sha(fbx),'generator_sha256':sha(Path(__file__)),'height_cm':height,'original_bounds_cm':original_bounds,'final_bounds_cm':final_bounds,'triangles':triangles,'bones':bones,'retained_vertices':limit,'retained_corner_digest':retained,'face_corner_digest':face_before,'removed_body_scope':removed,'components':COMPONENTS,'original_materials_reused':True,'raster_edits':False,'color_export':'LINEAR' if FOE else 'SRGB'}
 (dest/'geometry.json').write_text(json.dumps(receipt,indent=2)+'\n',encoding='utf-8');print('S358_GEOMETRY_PASS',name,height,triangles,flush=True);return receipt
if __name__=='__main__':
 names=sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else NAMES
 assert all(n in NAMES for n in names) and not any((OUT/n).exists() for n in names)
 results=[build(n) for n in names]
 (OUT/'source_inventory.json').write_text(json.dumps({'revision':'R3','generator_sha256':sha(Path(__file__)),'files':[{'path':str(p.relative_to(OUT)).replace('\\','/'),'sha256':sha(p)} for p in sorted(OUT.rglob('*')) if p.is_file()]},indent=2)+'\n',encoding='utf-8')
 print('S358_AUTHORING_COMPLETE',len(results),flush=True)
