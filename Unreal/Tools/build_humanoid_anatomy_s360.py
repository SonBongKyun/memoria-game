"""S360 continuous anatomical humans. Blender 4.5; CC0 hm08 base, no MH code.

77 names/parents retained; anatomically placed NEW bind rigs and compatible clips.
Original sources/packages immutable. Named clothing UVs/materials retained while fitted.
"""
from pathlib import Path
import bpy,bmesh,math,json,sys,hashlib,importlib.util
from mathutils import Vector,Matrix,Quaternion
from mathutils.bvhtree import BVHTree
ROOT=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location('motion',ROOT/'Unreal/Tools/build_humanoid_natural_s359.py')
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
OUT=ROOT/'Unreal/ArtSource/HumanoidAnatomyS360/R7'
VENDOR=ROOT/'Unreal/ArtSource/HumanoidAnatomyS360/Vendor/base.obj'
NAMES=m.NAMES
original_load=m.load
original_ik=m.ik
def anatomical_ik(a,b,c,target,pole):
 target=Vector(target)
 if a.startswith('upperarm_'):target.x+=(1 if a.endswith('_l') else -1)*3.0*m.K
 original_ik(a,b,c,target,pole)
m.ik=anatomical_ik
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def clamp(x,a=0,b=1):return max(a,min(b,x))
def obj():
 vs=[];uv=[];faces=[];groups={};g=''
 for l in VENDOR.read_text().splitlines():
  s=l.split()
  if not s:continue
  if s[0]=='v':vs.append(Vector(tuple(map(float,s[1:4]))))
  elif s[0]=='vt':uv.append(tuple(map(float,s[1:3])))
  elif s[0]=='g':g=s[1];groups[g]=[]
  elif s[0]=='f':groups[g].append([tuple(int(v)-1 for v in token.split('/')[:2]) for token in s[1:]])
 female=m.NAME in ['Elia','Villagerf','Elder'];sex='female' if female else 'male';age='old' if m.NAME in ['Elder','Malet','Fisherman'] else 'child' if m.NAME=='Child' else 'young'
 ethnic=f'caucasian-{sex}-{age}.target';body=f'universal-{sex}-'+('old' if female and age=='old' else 'child' if age=='child' else 'young')+'-averagemuscle-averageweight.target'
 for n,amount in [(ethnic,1.),(body,1.)]:
  path=VENDOR.parent/n
  if path.exists():
   for line in path.read_text().splitlines():
    s=line.split()
    if len(s)==4 and not s[0].startswith('#'):vs[int(s[0])]+=Vector(tuple(map(float,s[1:])))*amount
 return vs,uv,groups
def weights(p,j):
 # Candidates follow anatomical regions; inverse-distance and smooth joint blends.
 x,y,z=p;side='l' if x>0 else 'r';ax=abs(x)
 if z>j['neck_01'].z+3*m.K:return {'head':1.}
 if z>j['spine_03'].z and ax<6*m.K:
  return m.blend('neck_01','head',(z-j['neck_01'].z)/(j['head'].z-j['neck_01'].z)) if z>=j['neck_01'].z else m.blend('spine_03','neck_01',(z-j['spine_03'].z)/(j['neck_01'].z-j['spine_03'].z))
 sh=j['upperarm_'+side];el=j['lowerarm_'+side];wr=j['hand_'+side]
 # Arm surface has separation from the rib cage outside the axilla.
 if (ax>abs(sh.x)*.85 and z>j['pelvis'].z+9*m.K) or ax>abs(sh.x)*1.65:
  segments=[('upperarm_'+side,sh,el),('lowerarm_'+side,el,wr)]
  if (p-wr).length<19*m.K:
   segments.append(('hand_'+side,wr,j['middle_01_'+side]))
   for stem in ['thumb','index','middle','ring','pinky']:
    for i in range(1,4):
     a=f'{stem}_0{i}_{side}';b=f'{stem}_0{i+1}_{side}'
     end=j[b] if i<3 else j[a]+(j[a]-j[f'{stem}_02_{side}'])*.7
     segments.append((a,j[a],end))
  # Scapula and clavicle blend only over the actual shoulder cap, not the sleeve.
  if z>sh.z-5*m.K and ax<abs(sh.x)+4*m.K:segments += [('spine_03',j['spine_03'],j['neck_01']),('clavicle_'+side,j['clavicle_'+side],sh)]
 elif z<j['pelvis'].z-1*m.K:
  segments=[('thigh_'+side,j['thigh_'+side],j['calf_'+side]),('calf_'+side,j['calf_'+side],j['foot_'+side]),('foot_'+side,j['foot_'+side],j['ball_'+side])]
  if z>j['pelvis'].z-13*m.K:segments.append(('pelvis',j['pelvis']-Vector((0,0,7*m.K)),j['pelvis']+Vector((0,0,8*m.K))))
 else:
  return m.torso(p)
 ds=[]
 for n,a,b in segments:
  t=clamp((p-a).dot(b-a)/max(.001,(b-a).length_squared));d=(p-a-(b-a)*t).length
  ds.append((n,1/(d+1.3*m.K)**5))
 ds=sorted(ds,key=lambda v:-v[1])[:4];total=sum(w for n,w in ds)
 return {n:w/total for n,w in ds}

def prepare(name):
 source,old=original_load(name);H=m.H;K=m.K
 vs,uv,groups=obj();bodyids={v for f in groups['body'] for v,t in f};low=min(vs[i].y for i in bodyids);top=max(vs[i].y for i in bodyids);scale=H/(top-low)
 def convert(p):return Vector((p.x*scale,-p.z*scale+6*K,(p.y-low)*scale))
 human=[convert(p) for p in vs]
 def joint(s):
  ids={v for f in groups['joint-'+s] for v,t in f};return sum((human[i] for i in ids),Vector())/len(ids)
 j={'root':Vector(), 'pelvis':joint('pelvis'),'spine_01':joint('spine-3'),'spine_02':joint('spine-2'),'spine_03':joint('spine-1'),'neck_01':joint('neck'),'head':joint('head')}
 mouth=joint('mouth')
 for s in ['l','r']:
  for n,mh in [('clavicle','clavicle'),('upperarm','shoulder'),('lowerarm','elbow'),('hand','hand'),('thigh','upper-leg'),('calf','knee'),('foot','ankle'),('ball','foot-2')]:j[n+'_'+s]=joint(s+'-'+mh)
  for stem,finger in [('thumb',1),('index',2),('middle',3),('ring',4),('pinky',5)]:
   for i in range(1,4):j[f'{stem}_0{i}_{s}']=joint(f'{s}-finger-{finger}-{i}')
 # Role shape variation is expressed consistently across mesh and rig.
 width={'Guard':1.10,'Fisherman':1.04,'Villagerm':1.05,'Malet':1.12,'Villagerf':.94,'Elia':.94,'Elder':.96,'Child':.94,'AshWalker':.94}.get(name,1.)
 def shape(p):
  p=p.copy();p.x*=width
  # Adult head is about 7.3 heads tall; child about 5.7. No cylindrical giraffe neck.
  if name=='Child' and p.z>H*.78:
   t=clamp((p.z-H*.78)/(H*.08));p.x*=1+.18*t;p.y=6*K+(p.y-6*K)*(1+.12*t)
  return p
 human=[shape(p) for p in human];j={n:shape(p) for n,p in j.items()}
 rig=m.RIG;original={b.name:b.matrix_local.copy() for b in rig.data.bones};originaltail={b.name:b.tail_local.copy() for b in rig.data.bones}
 # Cloth helper bones follow proportion changes, never discarded.
 for b in rig.data.bones:
  if b.name not in j:
   q=b.head_local.copy();q.x*=width*.75;q.z*=.96;j[b.name]=q
 bpy.context.view_layer.objects.active=rig;bpy.ops.object.mode_set(mode='EDIT')
 for b in rig.data.edit_bones:
  n=b.name;oldhead=original[n].translation
  children=[c for c in b.children if c.name in j and not c.name.startswith('cloth_')]
  child=next((c for c in children if not c.name.startswith(('clavicle_','thigh_'))),children[0] if children else None)
  a=j[n];d=j[child.name]-a if child else (originaltail[n]-oldhead)*.80
  if d.length<.05:d=Vector((0,0,1*K))
  b.use_connect=False;b.head=a;b.tail=a+d;b.align_roll(original[n].to_3x3()@Vector((0,0,1)))
 bpy.ops.object.mode_set(mode='OBJECT');bpy.context.view_layer.update()
 corrected={b.name:b.matrix_local.copy() for b in rig.data.bones}
 # Fit retained painted garments and foe equipment to the new anatomical joint frame.
 transforms={n:corrected[n]@original[n].inverted() for n in original}
 if name in ['Elia','Malet']:
  bm=bmesh.new();bm.from_mesh(old.data);bmesh.ops.delete(bm,geom=[v for v in bm.verts if v.co.z>H-24],context='VERTS');bm.to_mesh(old.data);bm.free()
 for v in old.data.vertices:
  p=Vector();total=0
  for g in v.groups:
   n=old.vertex_groups[g.group].name
   if n in transforms:p+=(transforms[n]@v.co)*g.weight;total+=g.weight
  if total:v.co=p/total
 if name in ['Elia','Malet','AshWalker','BeltScavenger']:
  ground=min(v.co.z for v in old.data.vertices)
  if ground<0:
   for v in old.data.vertices:
    t=clamp(v.co.z/(20*K));v.co.z-=ground*(1-t*t*(3-2*t))
 return source,old,human,uv,groups,j,original,corrected,mouth

def add_eyes(j):
 m.EYE_CENTERS={}
 skin=m.PALETTES[m.NAME][3]
 vs0,uv0,g=obj();H=m.H;K=m.K
 # Joint eye centers come from the same topology as the actual eyelid openings.
 ids={i for f in g['body'] for i,t in f};low=min(vs0[i].y for i in ids);top=max(vs0[i].y for i in ids);sc=H/(top-low)
 for side in ['l','r']:
  ids={i for f in g['joint-'+side+'-eye'] for i,t in f};p=sum((vs0[i] for i in ids),Vector())/len(ids);p=Vector((p.x*sc,-p.z*sc+6*K,(p.y-low)*sc))
  width={'Guard':1.10,'Fisherman':1.04,'Villagerm':1.05,'Malet':1.12,'Villagerf':.94,'Elia':.94,'Elder':.96,'Child':.94,'AshWalker':.94}.get(m.NAME,1.)
  p.x*=width
  if m.NAME=='Child':p.x*=1.18;p.y=6*K+(p.y-6*K)*1.12
  m.EYE_CENTERS[side]=p.copy()
  r=1.12*K
  bpy.ops.mesh.primitive_uv_sphere_add(segments=32,ring_count=20,radius=r,location=p);ob=bpy.context.object;bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
  me=ob.data;vv=[v.co.copy() for v in me.vertices];ff=[tuple(f.vertices) for f in me.polygons];bpy.data.objects.remove(ob,do_unlink=True)
  def color(v):
   dx=(v.x-p.x);dz=(v.z-p.z);rr=math.sqrt(dx*dx+dz*dz)/r
   if v.y<p.y-.4*r and rr<.49:return (.012,.018,.014) if rr<.22 else (.075,.105,.080) if rr<.43 else (.025,.035,.028)
   return (.65,.61,.52)
  m.mesh('HumanEye_'+side,vv,ff,m.fixed('head'),color,0)

def hair(human,groups):
 H=m.H;K=m.K;color=m.PALETTES[m.NAME][4];vs=[];fs=[]
 tree=BVHTree.FromPolygons(human,[tuple(i for i,t in f) for f in groups['body']])
 rows=28;ns=96;cy=m.bn('head').y
 for row in range(rows+1):
  t=row/rows
  for i in range(ns):
   a=math.tau*i/ns;front=max(0,math.cos(a));back=max(0,-math.cos(a));bottom=H-(12+5*back-5.0*front**2)*K+.5*K*math.sin(a*3+.4)*front
   z=bottom+(H-.1*K-bottom)*t;d=Vector((math.sin(a),-math.cos(a),0));center=Vector((0,cy,z));hit,no,idx,dist=tree.ray_cast(center+d*35*K,-d,70*K)
   if hit is None:hit=center+d*.2*K
   p=hit+d*(.42+.14*math.sin(a*23+z*.30))*K+Vector((0,0,.25*K));vs.append(p)
 for row in range(rows):
  for i in range(ns):a=row*ns+i;b=row*ns+(i+1)%ns;fs.append((a,b,b+ns,a+ns))
 tip=len(vs);vs.append((0,cy,H+.3*K))
 for i in range(ns):fs.append((rows*ns+i,rows*ns+(i+1)%ns,tip))
 m.mesh('FittedStrandHair',vs,fs,m.fixed('head'),lambda p:tuple(v*(.92+.07*math.sin(p.x*3+p.z*.4)) for v in color),0)
 if m.NAME in ['Elia','Villagerf','Elder']:
  vs=[];fs=[];rows=18;cols=48
  for row in range(rows+1):
   t=row/rows
   for i in range(cols+1):
    a=math.pi*.49+math.pi*1.02*i/cols;z=H-12*K-20*K*t;rx=(8.2+.4*t)*K;ry=(8.2+.2*t)*K
    vs.append((math.sin(a)*rx,cy-math.cos(a)*ry,z))
  for row in range(rows):
   for i in range(cols):a=row*(cols+1)+i;fs.append((a,a+1,a+cols+2,a+cols+1))
  m.mesh('LayeredRearHair',vs,fs,lambda p,t:m.blend('neck_01','head',clamp((p.z-(H-32*K))/(15*K))),color,0)
 # Brows sit on the same anatomical facial surface; ray cast gives the actual ridge depth.
 for sign,side in [(-1,'r'),(1,'l')]:
  points=[]
  for t in [0,.25,.5,.75,1]:
   eye=m.EYE_CENTERS[side];x=sign*(abs(eye.x)-1.05*K+2.65*K*t);z=eye.z+(1.65+.40*math.sin(math.pi*t))*K
   hit,no,idx,dist=tree.ray_cast(Vector((x,-40*K,z)),Vector((0,1,0)),70*K)
   if hit is not None:points.append(hit+Vector((0,-.07*K,0)))
  if len(points)>2:m.tube('FittedEyebrow_'+str(sign),points,[.11*K]*len(points),m.fixed('head'),tuple(v*.65 for v in color),0,8,.55,0)

def build(name):
 source,old,human,uv,groups,j,original,corrected,mouth=prepare(name);H=m.H;K=m.K;m.PARTS=[];m.COMPONENTS=[]
 m.MAT=bpy.data.materials.new('AnatomicalHumanRegions');m.MAT.use_nodes=True;nodes=m.MAT.node_tree.nodes;vc=nodes.new('ShaderNodeVertexColor');vc.layer_name='Color';m.MAT.node_tree.links.new(vc.outputs['Color'],nodes['Principled BSDF'].inputs['Base Color'])
 named=name in ['Elia','Malet'];foe=name in ['AshWalker','BeltScavenger']
 if foe:
  # Preserve monster-specific sculpted face, gear and authored pigment; fit its torso to new rig.
  m.PARTS=[old]
 else:
  if named:
   m.PARTS=[old]
  else:bpy.data.objects.remove(old,do_unlink=True)
  if name not in m.PALETTES:m.PALETTES[name]=m.PALETTES['Guard']
  coat,shirt,pants,skin,haircolor=m.PALETTES[name]
  ids=sorted({i for f in groups['body'] for i,t in f if (human[i].z>H*.815 if named else human[i].z>17*K)});idsset=set(ids);idx={v:i for i,v in enumerate(ids)}
  fs=[];uvcorners=[]
  for face in groups['body']:
   if all(i in idsset for i,t in face):fs.append(tuple(idx[i] for i,t in face));uvcorners.append([uv[t] for i,t in face])
  positions=[];rows=[];colors=[]
  for i in ids:
   p=human[i].copy();ws=weights(p,j);rows.append(ws)
   hand=sum(w for n,w in ws.items() if n.startswith(('hand','thumb','index','middle','ring','pinky')))>.60
   head=p.z>j['neck_01'].z+2*K
   if head or hand:
    c=skin;cat=0
    # lips, facial warmth, nostril creases and brows use anatomical location, not painted-flat features.
    if head:
     z=p.z/H;x=abs(p.x)/(K or 1);front=p.y<0
     lip=math.exp(-((p.z-mouth.z)/(.42*K))**2)*math.exp(-(p.x/(2*K))**6) if front else 0
     c=tuple(a*(1-lip)+b*lip for a,b in zip(skin,(skin[0]*.77,skin[1]*.61,skin[2]*.61)))
   else:
    cat=.75;c=coat if p.z>H*.48 else pants
    if p.z<H*.15:c=(.045,.030,.020);cat=.5
    # Ease over anatomy to create clothed rather than painted-on naked geometry.
    if H*.49<p.z<H*.80 and abs(p.x)<15.8*K:
     p.x*=1.035
     # A loose textile envelope removes breast/navel contours and joins chest to abdomen.
     chest=clamp((p.z-H*.58)/(H*.16));chest=chest*chest*(3-2*chest)
     cy=4*K;rx=(20.8+1.2*chest)*K;ry=(14.8+1.4*chest)*K
     surf=math.sqrt(max(0,1-(p.x/rx)**2))*ry
     edge=clamp((H*.80-p.z)/(4*K))*clamp((p.z-H*.49)/(4*K))*clamp((17*K-abs(p.x))/(3*K))
     desired=cy-surf if p.y<cy else cy+surf*.83
     p.y=p.y*(1-edge)+desired*edge
    elif not hand:p.x+=math.copysign(.55*K,p.x)
   positions.append(p);colors.append(tuple(c)+(cat,))
  ob=m.mesh('ContinuousHumanTopology',positions,fs,lambda p,t:rows[round(t*(len(rows)-1))],skin,0)
  for f,uvs in zip(ob.data.polygons,uvcorners):
   for li,u in zip(f.loop_indices,uvs):
    ob.data.uv_layers[0].data[li].uv=u;ob.data.color_attributes['Color'].data[li].color=colors[ob.data.loops[li].vertex_index]
  # Subdivision preserves eyelid/lip/finger topology and rounds the continuous axilla.
  bpy.context.view_layer.objects.active=ob;mod=ob.modifiers.new('AnatomicalSurface','SUBSURF');mod.levels=1;bpy.ops.object.modifier_apply(modifier=mod.name)
  mod=ob.modifiers.new('FieldBudget','DECIMATE');mod.ratio=.44;bpy.ops.object.modifier_apply(modifier=mod.name)
  add_eyes(j);hair(human,groups)
  if not named:
   n=j['neck_01'];tree=BVHTree.FromPolygons(human,[tuple(i for i,t in f) for f in groups['body']]);vs=[];fs=[];ns=64
   for row in range(5):
    z=n.z+(-.5+row*.95)*K
    for i in range(ns+1):
     a=math.tau*i/ns;d=Vector((math.sin(a),math.cos(a),0));center=Vector((0,n.y,z));hit,no,idx,dist=tree.ray_cast(center+d*30*K,-d,60*K)
     vs.append((hit if hit is not None else center+d*6*K)+d*.35*K)
   for row in range(4):
    for i in range(ns):a=row*(ns+1)+i;fs.append((a,a+1,a+ns+2,a+ns+1))
   m.mesh('FittedSewnCollar',vs,fs,m.fixed('neck_01'),coat,.75)
   cy=j['pelvis'].y;hip=j['pelvis'].z
   # Loose tunic hides the trouser crotch and adds a plausible clothing silhouette.
   points=[];radii=[]
   for t in [0,.2,.5,.8,1]:points.append((0,cy,hip+7*K-22*K*t));radii.append((21.5+1.2*t)*K)
   m.tube('ClothTunicHem',points,radii,lambda p,t:m.blend('pelvis','spine_01',clamp((p.z-hip)/(14*K))*.3),coat,.75,64,.72,.012)
   belt=[(math.sin(a)*21.7*K,cy+math.cos(a)*15.8*K,hip+5.2*K) for a in [math.tau*i/64 for i in range(65)]]
   m.tube('LeatherWaistBelt',belt,[1.0*K]*65,m.fixed('pelvis'),(.07,.046,.025),.5,8,1.4,0)
   # A narrow garment placket and small fasteners give the shirt a readable construction.
   ps=[]
   for i in range(18):
    z=hip+8*K+(j['spine_03'].z-hip-8*K)*i/17;chest=clamp((z-H*.58)/(H*.16));chest=chest*chest*(3-2*chest);ps.append((0,4*K-(14.8+1.4*chest)*K-.3*K,z))
   m.tube('ShirtPlacket',ps,[.28*K]*len(ps),m.torso,shirt,.75,8,.7,0)
   for s in ['l','r']:
    ank=j['foot_'+s];knee=j['calf_'+s]
    m.tube('BootUpper_'+s,[knee.lerp(ank,.55),ank+Vector((0,0,3*K)),ank],[5.0*K,4.0*K,3.9*K],m.fixed('calf_'+s),(.046,.030,.020),.5,28,.95,.012)
    vs=[];fs=[];n=40
    for z,rx,front,back in [(0,4.6,-16.5,5),(2,4.7,-16.5,5),(5,4.4,-15.4,4.8),(9,4.0,-11,4.4),(14,3.85,-3.8,4.4),(18,3.8,-3.8,4.4)]:
     for i in range(n+1):
      a=math.tau*i/n;vs.append((ank.x+math.sin(a)*rx*K,ank.y+(front+back)*K/2-math.cos(a)*(back-front)*K/2,z*K))
    for r in range(5):
     for i in range(n):a=r*(n+1)+i;fs.append((a,a+1,a+n+2,a+n+1))
    fs.append(tuple(range(n,-1,-1)));m.mesh('BootLast_'+s,vs,fs,m.fixed('foot_'+s),(.038,.026,.019),.5)
 # Normalize layers and retain all UV coordinates after CustomData allocation.
 for ob in m.PARTS:
  me=ob.data;oldUV={u.name:[tuple(v.uv) for v in u.data] for u in me.uv_layers};cs=[tuple(v.color) for v in (me.color_attributes.get('Color') or me.color_attributes[0]).data] if len(me.color_attributes) else [(1,1,1,0)]*len(me.loops)
  while len(me.color_attributes):me.color_attributes.remove(me.color_attributes[0])
  me.color_attributes.new(name='Color',type='FLOAT_COLOR',domain='CORNER')
  for v,c in zip(me.color_attributes[0].data,cs):v.color=c
  for n,values in oldUV.items():
   for v,u in zip(me.uv_layers[n].data,values):v.uv=u
 bpy.ops.object.select_all(action='DESELECT')
 for ob in m.PARTS:ob.select_set(True)
 ob=m.PARTS[0];bpy.context.view_layer.objects.active=ob;bpy.ops.object.join();ob.name=name+'_Anatomy'
 for mod in list(ob.modifiers):ob.modifiers.remove(mod)
 tri=ob.modifiers.new('Triangles','TRIANGULATE');bpy.ops.object.modifier_apply(modifier=tri.name)
 ob.data.normals_split_custom_set([(0,0,0)]*len(ob.data.loops));ob.parent=m.RIG;ob.matrix_parent_inverse=Matrix.Identity(4);ob.matrix_basis=Matrix.Identity(4);mod=ob.modifiers.new('Skin','ARMATURE');mod.object=m.RIG
 dest=OUT/name;dest.mkdir(parents=True,exist_ok=False);bpy.ops.object.select_all(action='DESELECT');ob.select_set(True);m.RIG.select_set(True);bpy.context.view_layer.objects.active=m.RIG;m.export(dest/(name+'.fbx'))
 actual_height=max(v.co.z for v in ob.data.vertices)-min(v.co.z for v in ob.data.vertices)
 report={'name':name,'source_sha256':sha(source),'generator_sha256':sha(Path(__file__)),'base_sha256':sha(VENDOR),'height_cm':actual_height,'authored_original_height_cm':H,'triangles':len(ob.data.polygons),'components':m.COMPONENTS,'output_sha256':sha(dest/(name+'.fbx')),'joints_cm':{n:list(v) for n,v in j.items()},'bones':[{'name':b.name,'parent':b.parent.name if b.parent else None,'matrix':[list(r) for r in b.matrix_local]} for b in m.RIG.data.bones],'policy':'new anatomical rig; original name/hierarchy; named paint UVs retained, clothing coordinates fitted' }
 (dest/'geometry.json').write_text(json.dumps(report,indent=2)+'\n')
 print('S360_MESH',name,len(ob.data.polygons),flush=True)

def motionload(name):
 bpy.ops.wm.read_factory_settings(use_empty=True);src=OUT/name/(name+'.fbx');bpy.ops.import_scene.fbx(filepath=str(src),colors_type='LINEAR');m.RIG=next(o for o in bpy.data.objects if o.type=='ARMATURE');old=next(o for o in bpy.data.objects if o.type=='MESH')
 old.parent=None;old.matrix_world=Matrix.Identity(4);m.RIG.scale=(1,1,1);m.RIG.rotation_euler=(0,0,0);m.RIG.location=(0,0,0);m.RIG.animation_data_clear()
 for p in m.RIG.pose.bones:p.matrix_basis=Matrix.Identity(4)
 m.NAME=name;m.H=json.loads((OUT/name/'geometry.json').read_text())['height_cm'];m.K=m.H/180;bpy.context.scene.unit_settings.system='METRIC';bpy.context.scene.unit_settings.scale_length=.01
 return src,old
if __name__=='__main__':
 args=sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else [];mode=args.pop(0) if args and args[0] in ['Mesh','Motion','All'] else 'All';names=args or NAMES
 for n in names:
  if mode in ['Mesh','All']:build(n)
  if mode in ['Motion','All']:
   m.load=motionload;m.OUT=OUT;m.animate(n)
 inv=[{'path':str(p.relative_to(OUT)),'sha256':sha(p)} for p in sorted(OUT.rglob('*')) if p.is_file() and p.name!='source_inventory.json'];(OUT/'source_inventory.json').write_text(json.dumps({'revision':OUT.name,'files':inv},indent=2)+'\n');print('S360_AUTHORING_COMPLETE',mode,names,flush=True)
