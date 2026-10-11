"""S354 geometry authoring. Run Blender 4.5 --background --factory-startup --python this.py.
Reads only committed source FBX; no texture pixels are edited. Existing humanoid rigs/UV paint remain.
New creature anatomy, vertex palettes and five clips are authored locally and exported in cm.
"""
from pathlib import Path
import bpy,bmesh,math,json,hashlib,sys
from mathutils import Vector,Matrix,Quaternion
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'Unreal/ArtSource/CharacterUpgrade'
OUT.mkdir(parents=True,exist_ok=True)
TAU=math.tau
PARTS=[]; RIG=None; MAT=None; CREATURE=False

def mesh(name,verts,faces,uvs,weights,tile=0,color=(.2,.2,.2,1)):
 me=bpy.data.meshes.new(name);me.from_pydata(verts,[],faces);me.update()
 ob=bpy.data.objects.new(name,me);bpy.context.collection.objects.link(ob);me.materials.append(MAT)
 me.uv_layers.new(name='UVMap'); me.color_attributes.new(name='Color',type='BYTE_COLOR',domain='CORNER')
 uv=me.uv_layers['UVMap']; c=me.color_attributes['Color']
 for f in me.polygons:
  f.use_smooth=True
  for li in f.loop_indices:
   vi=me.loops[li].vertex_index;u,v=uvs[vi]
   uv.data[li].uv=((tile%4+.03+u*.94)/4,1-(tile//4+.03+v*.94)/4)
   c.data[li].color=color
 for i,ws in enumerate(weights):
  total=sum(ws.values())
  for n,w in ws.items():
   if w>1e-6:
    g=ob.vertex_groups.get(n) or ob.vertex_groups.new(name=n);g.add([i],w/total,'REPLACE')
 PARTS.append(ob);return ob

def fixed(n):return lambda t,p:{n:1}
def ell(name,c,r,tile,bone,color=(.2,.2,.2,1),nu=24,nv=16):
 vs=[];uv=[];fs=[];ws=[];c=Vector(c)
 for j in range(nv+1):
  b=.001+(math.pi-.002)*j/nv
  for i in range(nu+1):
   a=TAU*i/nu;p=c+Vector((r[0]*math.sin(b)*math.sin(a),r[1]*math.sin(b)*math.cos(a),r[2]*math.cos(b)))
   vs.append(p);uv.append((i/nu,j/nv));ws.append({bone:1})
 for j in range(nv):
  for i in range(nu):
   a=j*(nu+1)+i;fs.append((a,a+1,a+nu+2,a+nu+1))
 return mesh(name,vs,fs,uv,ws,tile,color)

def tube(name,points,radii,tile,weight,n=16,flat=1,fold=0,color=(.2,.2,.2,1),caps=True):
 ps=list(map(Vector,points));vs=[];uv=[];fs=[];ws=[]
 for j,(p,r) in enumerate(zip(ps,radii)):
  d=(ps[min(j+1,len(ps)-1)]-ps[max(j-1,0)]).normalized();ref=Vector((0,-1,0))
  if abs(ref.dot(d))>.96:ref=Vector((1,0,0))
  a=(ref-d*ref.dot(d)).normalized();b=d.cross(a)
  for i in range(n+1):
   ang=TAU*i/n;rr=r*(1+fold*math.cos(ang*7+j*.8));v=p+a*(math.cos(ang)*rr*flat)+b*(math.sin(ang)*rr)
   vs.append(v);uv.append((i/n,j/(len(ps)-1)));ws.append(weight(j/(len(ps)-1),v))
 for j in range(len(ps)-1):
  for i in range(n):
   a=j*(n+1)+i;fs.append((a,a+1,a+n+2,a+n+1))
 if caps:fs.extend([tuple(range(n,-1,-1)),tuple(range((len(ps)-1)*(n+1),len(ps)*(n+1)))])
 return mesh(name,vs,fs,uv,ws,tile,color)

def panel(name,points,tile,bone,thick=.4):
 # Ordered outline, subdivided curved quad strip authored by caller; material remains in its atlas tile.
 ob=mesh(name,points,[tuple(range(len(points)))],[(i%2,i//2/max(1,(len(points)//2-1))) for i in range(len(points))],[{bone:1} for p in points],tile)
 bpy.context.view_layer.objects.active=ob;ob.select_set(True)
 so=ob.modifiers.new('Thickness','SOLIDIFY');so.thickness=thick
 bpy.ops.object.modifier_apply(modifier=so.name);ob.select_set(False);return ob

def smooth_weights(z):
 names=['pelvis','spine_01','spine_02','spine_03'];nodes=[RIG.data.bones[n].head_local.z for n in names]
 if z<=nodes[0]:return {names[0]:1}
 for i in range(3):
  if z<nodes[i+1]:
   t=(z-nodes[i])/(nodes[i+1]-nodes[i]);return {names[i]:1-t,names[i+1]:t}
 return {names[-1]:1}

def join_export(ident,expected_height=None):
 bpy.ops.object.select_all(action='DESELECT')
 for ob in PARTS:ob.select_set(True)
 body=PARTS[0];bpy.context.view_layer.objects.active=body;bpy.ops.object.join();body.name=ident+'_Body'
 # Deterministic welded surface normals, retain intentionally layered garment thickness.
 bm=bmesh.new();bm.from_mesh(body.data);bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=.00001);bmesh.ops.dissolve_degenerate(bm,edges=list(bm.edges),dist=.000001);bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(body.data);bm.free()
 for mod in list(body.modifiers):body.modifiers.remove(mod)
 tri=body.modifiers.new('Triangles','TRIANGULATE');bpy.ops.object.modifier_apply(modifier=tri.name)
 body.data.materials.clear();body.data.materials.append(MAT)
 for f in body.data.polygons:f.material_index=0
 body.parent=RIG;body.matrix_parent_inverse=Matrix.Identity(4);body.matrix_basis=Matrix.Identity(4)
 skin=body.modifiers.new('Skin','ARMATURE');skin.object=RIG
 RIG.scale=(1,1,1);RIG.location=(0,0,0);RIG.rotation_euler=(0,0,0)
 bpy.context.scene.unit_settings.system='METRIC';bpy.context.scene.unit_settings.scale_length=.01
 zs=[v.co.z for v in body.data.vertices];height=max(zs)-min(zs)
 if expected_height:
  # Preserve the original skeletal bind pose and exact standing height, including footwear.
  assert abs(height-expected_height)<1,(ident,height,expected_height)
 assert all(v.groups and abs(sum(g.weight for g in v.groups)-1)<.001 for v in body.data.vertices)
 bpy.ops.object.select_all(action='DESELECT');body.select_set(True);RIG.select_set(True);bpy.context.view_layer.objects.active=RIG
 dest=OUT/ident;dest.mkdir(exist_ok=True)
 export(dest/(ident+'.fbx'),False)
 body.data.calc_loop_triangles()
 return body,{'name':ident,'height_cm':height,'triangles':len(body.data.loop_triangles),'bones':len(RIG.data.bones),'source_sha256':None}

def export(path,anim):
 bpy.ops.export_scene.fbx(filepath=str(path),use_selection=True,object_types={'ARMATURE','MESH'} if not anim else {'ARMATURE'},axis_forward='-Y',axis_up='Z',apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',add_leaf_bones=False,use_armature_deform_only=True,path_mode='RELATIVE',bake_anim=anim,bake_anim_use_all_actions=False,bake_anim_use_nla_strips=False,bake_anim_simplify_factor=0,bake_anim_step=1,mesh_smooth_type='FACE',colors_type='LINEAR' if CREATURE else 'SRGB')

NPCS={'Traveler':('npc_traveler',174),'Bureauagent':('npc_bureau_agent',176),'Guard':('npc_guard',178),'Villagerf':('npc_villager_f',165),'Villagerm':('npc_villager_m',175),'Fisherman':('npc_fisherman',172),'Elder':('npc_elder',165),'Child':('npc_child',120),'Scholar':('npc_scholar',172)}
def npc(name):
 global RIG,MAT,PARTS,CREATURE
 CREATURE=False;PARTS=[];bpy.ops.wm.read_factory_settings(use_empty=True)
 source_id,H=NPCS[name];source=ROOT/'Unreal/ArtSource/FieldCharacters'/source_id/(source_id+'_rigged.fbx')
 bpy.ops.import_scene.fbx(filepath=str(source));RIG=next(o for o in bpy.data.objects if o.type=='ARMATURE');old=next(o for o in bpy.data.objects if o.type=='MESH');MAT=old.data.materials[0]
 old.parent=None;old.matrix_world=Matrix.Identity(4);RIG.scale=(1,1,1)
 K=H/180;bone=lambda n:RIG.data.bones[n].head_local.copy()
 # Preserve only painted head/hair, articulated hands and actual boots. Replace all old cylindrical clothing.
 bm=bmesh.new();bm.from_mesh(old.data);dv=bm.verts.layers.deform.active
 keep=[]
 for v in bm.verts:
  names=[old.vertex_groups[i].name for i,w in v[dv].items() if w>.2]
  good=any(n=='head' or n.startswith(('hand_','thumb_','index_','middle_','ring_','pinky_','foot_','ball_')) for n in names)
  if not good:keep.append(v)
 bmesh.ops.delete(bm,geom=keep,context='VERTS');bm.to_mesh(old.data);bm.free();PARTS.append(old)
 # Subtle silhouette-only facial differences, with exact existing facial pixels retained.
 jaw={'Villagerf':.88,'Villagerm':1.07,'Fisherman':1.12,'Elder':.94,'Child':.96,'Scholar':.92,'Traveler':.99,'Bureauagent':1.02,'Guard':1.10}[name]
 hz=bone('head').z
 for v in old.data.vertices:
  if v.co.z>hz and abs(v.co.x)<14*K:
   fade=max(0,1-abs(v.co.z-(hz+7*K))/(14*K));v.co.x*=1+(jaw-1)*fade
 cy=bone('spine_03').y;hip=bone('pelvis').z;shoulder=bone('upperarm_l').z;neck=bone('neck_01').z
 width=abs(bone('upperarm_l').x);depth=11.7*K
 # Fitted tunic: wider shoulder/chest, indented waist, flared hips; neck ends inside the existing head.
 prof=[(hip-9*K,width*.84,depth*.91),(hip,width*.87,depth),(hip+10*K,width*.74,depth*.89),(hip+23*K,width*.78,depth*.92),(shoulder-14*K,width*.88,depth*1.02),(shoulder-5*K,width*.98,depth),(shoulder,width*1.06,depth*.79),(neck+3*K,5.2*K,4.8*K),(hz+5*K,4.7*K,4.4*K)]
 vs=[];uv=[];fs=[];ws=[];sides=48
 for j,(z,rx,ry) in enumerate(prof):
  for i in range(sides+1):
   a=TAU*i/sides;fold=(.45+.6*abs(math.sin(a)))*K*math.sin(a*10+.45*j)
   p=Vector(((rx+fold)*math.sin(a),cy+(ry+fold*.5)*math.cos(a),z));vs.append(p);uv.append((i/sides,j/(len(prof)-1)));ws.append(smooth_weights(z))
 for j in range(len(prof)-1):
  for i in range(sides):a=j*(sides+1)+i;fs.append((a,a+1,a+sides+2,a+sides+1))
 mesh('TailoredCoat',vs,fs,uv,ws,0)
 tube('ConnectedNeck',[bone('neck_01'),bone('head')+Vector((0,0,7*K))],[4.9*K,4.5*K],3,fixed('neck_01'))
 for side,sign in [('l',1),('r',-1)]:
  sh,el,wr=bone('upperarm_'+side),bone('lowerarm_'+side),bone('hand_'+side)
  ps=[sh.lerp(el,t) for t in (0,.12,.3,.52,.73,1)]+[el.lerp(wr,t) for t in (.2,.4,.65,.85,1)]
  radii=[8.2,8.0,7.4,6.8,5.7,4.6,4.8,4.4,4.0,3.5,3.1]
  def armweight(t,p,s=side):
   f=max(0,min(1,(t-.40)*5));return {'upperarm_'+s:1-f,'lowerarm_'+s:f}
  tube('SewnSleeve_'+side,ps,[r*K for r in radii],0,armweight,n=24,flat=.91,fold=.065)
  # Fold rolls and cuffs belong to the same arm so they never float at the shoulder joint.
  d=(wr-el).normalized();tube('TurnedCuff_'+side,[wr-d*4*K,wr-d*.4*K],[3.8*K,3.7*K],2,fixed('lowerarm_'+side),n=24,fold=.025)
  thigh,knee,ank=bone('thigh_'+side),bone('calf_'+side),bone('foot_'+side)
  ps=[thigh.lerp(knee,t) for t in (0,.18,.4,.7,1)]+[knee.lerp(ank,t) for t in (.2,.45,.7,1)]
  def legweight(t,p,s=side):
   f=max(0,min(1,(t-.38)*5));return {'thigh_'+s:1-f,'calf_'+s:f}
  tube('FoldedTrousers_'+side,ps,[r*K for r in (9.8,10.1,8.8,7.1,5.6,6.0,5.5,4.4,4)],7,legweight,n=24,flat=.82,fold=.075)
 # Sculpted coat/skirt panels with a walking slit; waist flows into lower garment, not a cylinder.
 long=name in ['Villagerf','Elder','Scholar'];length=(hip-(.22 if name=='Villagerf' else .16)*H) if long else H*.21
 for side,sgn in [('l',1),('r',-1)]:
  vs=[];uv=[];fs=[];ws=[];cols=24;rows=18
  for j in range(rows+1):
   t=j/rows;z=hip+4*K-length*t
   for i in range(cols+1):
    u=i/cols;a=(.10+(math.pi-.20)*u)*sgn;rx=width*(.86+.22*t);ry=depth*(1+.25*t);crease=K*(.35+1.6*t)*math.sin(a*12+t*.4)
    p=Vector(((rx+crease)*math.sin(a),cy+(ry+crease)*math.cos(a),z+K*t*t*math.sin(a*5)*1.2));vs.append(p);uv.append((u,t));f=t*t*.48
    ws.append({'pelvis':1-f,'thigh_'+side:f})
  for j in range(rows):
   for i in range(cols):a=j*(cols+1)+i;fs.append((a,a+1,a+cols+2,a+cols+1))
  mesh('DrapedSkirt_'+side,vs,fs,uv,ws,0)
  # Long lapels and broad turned collar emphasize recognizable clothing at field distance.
  p=[(sgn*4*K,cy-6*K,hz-1*K),(sgn*width*.78,cy-9*K,shoulder-3*K),(sgn*8*K,cy-depth-2*K,hip+18*K),(sgn*2*K,cy-depth-1*K,hip+22*K)]
  panel('FoldedLapel_'+side,p,2,'spine_03',.65*K)
 # Thick leather belt, bronze square buckle and asymmetrical pouch.
 ps=[(math.sin(a)*width*.86,cy+math.cos(a)*(depth+1.2*K),hip+3*K) for a in [TAU*i/64 for i in range(65)]]
 tube('WaistBelt',ps,[1.65*K]*65,1,fixed('pelvis'),n=8,flat=1.4)
 front=cy-depth-3*K
 for x in [-2.8,2.8]:tube('BuckleSide',[(x*K,front,hip),(x*K,front,hip+6*K)],[.52*K]*2,5,fixed('pelvis'),n=8)
 for z in [hip,hip+6*K]:tube('BuckleBar',[(-2.8*K,front,z),(2.8*K,front,z)],[.52*K]*2,5,fixed('pelvis'),n=8)
 ell('BeltPouch',(-width*.90,cy-1*K,hip-9*K),(6.4*K,4.2*K,8.6*K),1,'pelvis')
 panel('PouchFlap',[(-width-5*K,cy-5.3*K,hip-3*K),(-width+5*K,cy-5.3*K,hip-3*K),(-width+4*K,cy-5.4*K,hip-9*K),(-width-4*K,cy-5.4*K,hip-9*K)],6,'pelvis')
 # Distinct role silhouettes: bonnet, brimmed work hat, shawl, cap, shoulder armour or travelling mantle.
 if name in ['Fisherman','Villagerm','Child','Bureauagent','Guard']:
  center=Vector((0,cy,H-9*K));r=11*K if name!='Child' else 12*K
  ell('WorkCap',center,(r,9*K,5.8*K),1 if name in ['Guard','Bureauagent'] else 0,'head',nu=32)
  if name in ['Fisherman','Villagerm']:
   ps=[(math.sin(a)*15*K,cy+math.cos(a)*12*K,H-10*K) for a in [TAU*i/64 for i in range(65)]]
   tube('WornHatBrim',ps,[1.4*K]*65,0,fixed('head'),n=8,flat=.45)
 elif name=='Villagerf':
  ell('HairBun',(0,cy+10*K,H-14*K),(6.5*K,5*K,5.5*K),4,'head')
  ps=[(math.sin(a)*10*K,cy+math.cos(a)*8*K,H-12*K) for a in [math.pi*.55+math.pi*.9*i/32 for i in range(33)]]
  tube('BonnetTie',ps,[1.4*K]*33,2,fixed('head'),n=10)
 if name in ['Elder','Scholar','Traveler','Guard']:
  for side,sgn in [('l',1),('r',-1)]:
   points=[(sgn*5*K,cy-7*K,hz+1*K),(sgn*(width+7*K),cy,shoulder+1*K),(sgn*width,cy+14*K,shoulder-18*K),(sgn*5*K,cy+12*K,shoulder-25*K)]
   panel('ShoulderMantle_'+side,points,7 if name=='Guard' else 2,'spine_03',1.1*K)
 if name=='Scholar':
  for sign in [-1,1]:
   ps=[(sign*3.4*K+math.sin(a)*2.8*K,cy-10*K,hz+12*K+math.cos(a)*2.8*K) for a in [TAU*i/32 for i in range(33)]]
   tube('Spectacles',ps,[.23*K]*33,5,fixed('head'),n=6)
 body,report=join_export(name,H);report['source_sha256']=hashlib.sha256(source.read_bytes()).hexdigest();return report

# Creature bodies use explicit jointed rigs, independently authored clips and region vertex colours.
def creature(name):
 global RIG,MAT,PARTS,CREATURE
 CREATURE=True;PARTS=[];bpy.ops.wm.read_factory_settings(use_empty=True)
 MAT=bpy.data.materials.new('CreatureRegions');MAT.use_nodes=True
 vc=MAT.node_tree.nodes.new('ShaderNodeVertexColor');vc.layer_name='Color';MAT.node_tree.links.new(vc.outputs['Color'],MAT.node_tree.nodes['Principled BSDF'].inputs['Base Color'])
 MAT.node_tree.nodes['Principled BSDF'].inputs['Roughness'].default_value=.78
 data=bpy.data.armatures.new(name+'Rig');RIG=bpy.data.objects.new('Armature',data);bpy.context.collection.objects.link(RIG);bpy.context.view_layer.objects.active=RIG;RIG.select_set(True);bpy.ops.object.mode_set(mode='EDIT')
 def bone(n,at,parent):
  b=data.edit_bones.new(n);b.head=at;b.tail=Vector(at)+Vector((0,0,6));b.parent=data.edit_bones.get(parent) if parent else None;return b
 bone('root',(0,0,0),None);bone('body',(0,0,30),'root')
 rat=name=='RubbleRat';bone('head',(0,-32,33),'body');bone('jaw',(0,-40,24),'head')
 legs=[]
 for j,y in enumerate([-27,24] if rat else [-27,0,27]):
  for side,sgn in [('l',1),('r',-1)]:
   a=(sgn*(14 if rat else 20),y,30);b=(sgn*(21 if rat else 40),y+(7 if j%2 else -8),18 if rat else 29);c=(sgn*(23 if rat else 53),y-5,4)
   label=f'leg{j}_{side}';bone(label,a,'body');bone(label+'_tip',b,label);legs.append((label,a,b,c,j,sgn))
 if rat:
  for i in range(6):bone('tail'+str(i),(0,30+i*13,26-i*3.5),'body' if i==0 else 'tail'+str(i-1))
 bpy.ops.object.mode_set(mode='OBJECT')
 fur=(.095,.074,.060,1);light=(.20,.16,.12,1);skin=(.27,.12,.105,1);claw=(.42,.32,.21,1);black=(.009,.007,.006,1);eye=(.66,.24,.025,1)
 if rat:
  ell('Haunch',(0,13,29),(22,35,22),0,'body',fur,36,24);ell('Shoulders',(0,-16,32),(18,24,18),0,'body',fur,32,20)
  ell('Head',(0,-37,33),(12.5,18,13),0,'head',light,32,20)
  tube('LongMuzzle',[(0,-42,30),(0,-54,26),(0,-66,23)],[9.2,6,3.5],0,fixed('head'),n=28,flat=.76,color=light)
  ell('Nose',(0,-67,24),(4.1,2.9,2.4),0,'head',skin)
  ell('Jaw',(0,-53,20),(7,12,3.8),0,'jaw',skin)
  for s in [-1,1]:
   ell('Ear',(s*12,-28,48),(7.5,3,8.5),0,'head',fur);ell('InnerEar',(s*12,-31.1,48),(5.4,1.1,6.3),0,'head',skin)
   ell('EyeSocket',(s*12,-43,36),(2.5,2.8,2.6),0,'head',black);ell('AmberEye',(s*14,-44.5,37),(1.15,1.5,1.3),0,'head',eye)
   for j in range(3):tube('Whisker',[(s*5,-58,25+j),(s*(19+j*3),-57-j*3,27+j)],[.18,.045],0,fixed('head'),n=5,color=claw)
   tube('Incisor',[(s*2.4,-62,22),(s*2.4,-64,17)],[1.2,.5],0,fixed('head'),n=8,color=claw)
  for i in range(6):
   start=Vector((0,30+i*13,26-i*3.5));end=Vector((0,43+i*13,22.5-i*3.5));tube('TailSegment',[start,start.lerp(end,.5),end],[max(.7,4.8-i*.75),max(.5,4.4-i*.75),max(.25,4-i*.75)],0,fixed('tail'+str(i)),n=16,fold=.035,color=skin)
  # Voxel-fuse the core into one continuous anatomical surface, with a blended neck.
  core=[o for o in PARTS if o.name in ['Haunch','Shoulders','Head','LongMuzzle']]
  bpy.ops.object.select_all(action='DESELECT')
  for o in core:o.select_set(True);PARTS.remove(o)
  body=core[0];bpy.context.view_layer.objects.active=body;bpy.ops.object.join()
  rem=body.modifiers.new('ContinuousAnatomy','REMESH');rem.mode='VOXEL';rem.voxel_size=.9;rem.use_smooth_shade=True;bpy.ops.object.modifier_apply(modifier=rem.name)
  sm=body.modifiers.new('AnatomicalBlend','SMOOTH');sm.factor=.7;sm.iterations=5;bpy.ops.object.modifier_apply(modifier=sm.name)
  body.vertex_groups.clear()
  for layer in list(body.data.uv_layers):body.data.uv_layers.remove(layer)
  body.data.uv_layers.new(name='UVMap')
  for attr in list(body.data.color_attributes):body.data.color_attributes.remove(attr)
  attr=body.data.color_attributes.new(name='Color',type='BYTE_COLOR',domain='CORNER')
  bg=body.vertex_groups.new(name='body');hg=body.vertex_groups.new(name='head')
  for v in body.data.vertices:
   head=max(0,min(1,(-v.co.y-24)/21));head=head*head*(3-2*head)
   if head<1:bg.add([v.index],1-head,'REPLACE')
   if head>0:hg.add([v.index],head,'REPLACE')
  for f in body.data.polygons:
   f.use_smooth=True
   for li in f.loop_indices:
    v=body.data.vertices[body.data.loops[li].vertex_index].co
    u=(math.atan2(v.x,v.z-28)/TAU)%1;vv=max(0,min(1,(v.y+67)/116))
    body.data.uv_layers[0].data[li].uv=((.03+u*.94)/4,1-(.03+vv*.94)/4)
    head=max(0,min(1,(-v.y-30)/30));attr.data[li].color=tuple(fur[k]*(1-head)+light[k]*head for k in range(4))
  PARTS.append(body)

 else:
  shell=(.19,.13,.065,1);edge=(.32,.23,.105,1);dark=(.055,.041,.025,1)
  ell('Thorax',(0,-15,32),(27,27,19),0,'body',dark,32,20)
  for j in range(6):
   y=7+j*8;size=1-j*.095
   ell('OverlappingCarapace',(0,y,34),(29*size,12,22*size),0,'body',shell if j%2 else edge,32,18)
   for s in [-1,1]:tube('CarapaceSpine',[(s*23*size,y,38),(s*33*size,y+7,41),(s*36*size,y+11,38)],[3.2,1.8,.05],0,fixed('body'),n=9,color=shell)
  ell('HeadPlate',(0,-39,30),(19,17,12),0,'head',shell,32,18)
  for s in [-1,1]:
   for j in range(3):ell('Ocellus',(s*(7+j*4),-51+j*2,35),(2.3,2,2.2),0,'head',eye,16,10)
   tube('Mandible',[(s*12,-48,24),(s*19,-59,18),(s*13,-68,14),(s*5,-67,18)],[4.8,4,2,.08],0,fixed('jaw'),n=14,color=edge)
   tube('Antenna',[(s*10,-46,39),(s*20,-57,49),(s*27,-74,47),(s*31,-88,40)],[1.4,1,.55,.06],0,fixed('head'),n=9,color=dark)
 for label,a,b,c,j,sgn in legs:
  col=fur if rat else (.13,.087,.039,1)
  tube('Upper_'+label,[a,Vector(a).lerp(Vector(b),.4),b],[7 if rat else 4.5,6 if rat else 3.3,3],0,fixed(label),n=18,color=col)
  ell('Joint_'+label,b,(3.8,4,3.8),0,label+'_tip',light if rat else claw,20,12)
  tube('Lower_'+label,[b,Vector(b).lerp(Vector(c),.7),c],[3.1,2.3,1.2],0,fixed(label+'_tip'),n=14,color=skin if rat else col)
  if rat:
   for k in range(4):tube('Toe',[(c[0]+(k-1.5)*1.7,c[1],3),(c[0]+(k-1.5)*2.2,c[1]-8,2),(c[0]+(k-1.5)*2.2,c[1]-10,3)],[.9,.65,.07],0,fixed(label+'_tip'),n=7,color=skin if k%2 else claw)
  else:tube('TarsalClaw',[c,(c[0]+sgn*3,c[1]-6,1),(c[0]+sgn*4,c[1]-11,2)],[1.5,.8,.07],0,fixed(label+'_tip'),n=9,color=claw)
 body,report=join_export(name)
 scene=bpy.context.scene;scene.render.fps=30
 clips={'Idle':60,'Walk':30,'Bite':24,'HitReact_Front_Lgt_01':15,'Death_Front_01':40}
 for clip,end in clips.items():
  RIG.animation_data_clear();scene.frame_start=0;scene.frame_end=end
  for f in range(end+1):
   scene.frame_set(f);t=f/end
   for pb in RIG.pose.bones:pb.matrix_basis=Matrix.Identity(4);pb.rotation_mode='QUATERNION'
   def rotate(n,axis,angle):
    pb=RIG.pose.bones[n];q=pb.bone.matrix_local.to_quaternion();pb.rotation_quaternion=q.inverted()@Quaternion(axis,angle)@q
   def move(n,vec):
    pb=RIG.pose.bones[n];pb.location=pb.bone.matrix_local.to_quaternion().inverted()@Vector(vec)
   if clip in ['Idle','Walk']:
    phase=TAU*t;move('body',(0,0,math.sin(phase*(2 if clip=='Walk' else 1))*(1.8 if clip=='Walk' else .6)))
    rotate('head',(0,0,1),math.sin(phase)*.035)
    if clip=='Walk':
     for label,a,b,c,j,s in legs:
      p=phase+(0 if ((j+(s<0))%2==0) else math.pi)
      rotate(label,(1,0,0),math.sin(p)*(.52 if rat else .29));rotate(label+'_tip',(1,0,0),max(0,math.cos(p))*.56)
    if rat:
     for i in range(6):rotate('tail'+str(i),(0,0,1),math.sin(phase+i*.6)*.08)
   elif clip=='Bite':
    pulse=math.sin(math.pi*t)**2;move('body',(0,-12*pulse,-3*pulse));rotate('head',(1,0,0),-.22*pulse);rotate('jaw',(1,0,0),.60*math.sin(math.pi*t))
   elif clip=='HitReact_Front_Lgt_01':
    pulse=math.sin(math.pi*t);move('body',(0,8*pulse,-3*pulse));rotate('head',(0,0,1),.23*pulse)
   else:
    p=min(1,t*1.6);p=p*p*(3-2*p);move('body',(0,0,-20*p));rotate('body',(0,1,0),1.45*p)
    for label,a,b,c,j,s in legs:rotate(label,(0,1,0),s*.4*p);rotate(label+'_tip',(1,0,0),.7*p)
   for pb in RIG.pose.bones:
    pb.keyframe_insert(data_path='location',frame=f);pb.keyframe_insert(data_path='rotation_quaternion',frame=f);pb.keyframe_insert(data_path='scale',frame=f)
  export(OUT/name/(clip+'.fbx'),True)
 report['clips']=clips;report['role']='quadruped scavenging rat' if rat else 'six-legged plated dust crawler';return report

args=sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else []
selection=args or list(NPCS)+['RubbleRat','DustCrawler']
reports=[]
for name in selection:
 r=npc(name) if name in NPCS else creature(name);reports.append(r)
 (OUT/name/'geometry.json').write_text(json.dumps(r,indent=2),encoding='utf-8')
 print('S354_GEOMETRY',json.dumps(r),flush=True)
print('S354_AUTHORING_COMPLETE',len(reports),flush=True)
