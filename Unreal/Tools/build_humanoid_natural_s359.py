"""S359 anatomy and contact-aware humanoid motion. Blender 4.5 background.

Original 77-bone bind matrices are immutable. New mesh and animation targets only.
Face topology helpers reuse S357's stitched orbital surfaces, never raster edits.
"""
from pathlib import Path
import bpy,bmesh,math,json,hashlib,sys,importlib.util
from mathutils import Vector,Matrix,Quaternion
from mathutils.bvhtree import BVHTree
from mathutils.geometry import barycentric_transform

ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'Unreal/ArtSource/HumanoidNaturalS359/R6'
NPCS=['Traveler','Bureauagent','Guard','Villagerf','Villagerm','Fisherman','Elder','Child','Scholar']
NAMES=NPCS+['Elia','Malet','AshWalker','BeltScavenger']
TAU=math.tau
PARTS=[];RIG=None;NAME='';H=180;K=1;MAT=None;COMPONENTS=[]
PALETTES={
 'Traveler':((.105,.09,.065),(.14,.125,.095),(.065,.055,.045),(.39,.275,.19),(.055,.043,.031)),
 'Bureauagent':((.055,.065,.07),(.13,.13,.115),(.040,.045,.047),(.44,.32,.235),(.035,.030,.026)),
 'Guard':((.065,.09,.105),(.115,.125,.125),(.050,.06,.068),(.40,.28,.20),(.045,.035,.027)),
 'Villagerf':((.17,.105,.14),(.23,.17,.19),(.07,.055,.063),(.49,.35,.25),(.14,.065,.035)),
 'Villagerm':((.115,.09,.065),(.19,.155,.115),(.06,.05,.040),(.42,.29,.21),(.050,.035,.025)),
 'Fisherman':((.055,.105,.135),(.15,.19,.19),(.045,.065,.075),(.39,.26,.18),(.075,.065,.050)),
 'Elder':((.12,.075,.14),(.19,.15,.18),(.055,.04,.065),(.44,.32,.235),(.33,.32,.30)),
 'Child':((.20,.235,.105),(.30,.25,.15),(.09,.07,.045),(.50,.365,.26),(.19,.105,.045)),
 'Scholar':((.045,.09,.06),(.14,.17,.13),(.035,.055,.040),(.46,.335,.24),(.06,.045,.030)),
 'Elia':((.14,.17,.20),(.25,.23,.19),(.045,.055,.075),(.49,.37,.29),(.46,.46,.43)),
 'Malet':((.12,.10,.07),(.18,.16,.13),(.055,.045,.035),(.43,.30,.22),(.070,.050,.035))}

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def clamp(x,a=0,b=1):return max(a,min(b,x))
def bn(n):return RIG.data.bones[n].head_local.copy()
def fixed(n):return lambda p,t:{n:1.}
def blend(a,b,t):t=clamp(t);return {a:1-t,b:t}
def box(vs):return [[min(v[i] for v in vs),max(v[i] for v in vs)] for i in range(3)]
def torso(p,t=0):
 ns=['pelvis','spine_01','spine_02','spine_03','neck_01']
 for a,b in zip(ns,ns[1:]):
  if p.z<bn(b).z:return blend(a,b,(p.z-bn(a).z)/(bn(b).z-bn(a).z))
 return {'neck_01':1.}

def mesh(label,vs,fs,weight,color,category=.75,uv=None):
 me=bpy.data.meshes.new(label);me.from_pydata(vs,[],fs);me.update()
 ob=bpy.data.objects.new(label,me);bpy.context.collection.objects.link(ob);me.materials.append(MAT)
 me.uv_layers.new(name='UVMap');me.color_attributes.new(name='Color',type='FLOAT_COLOR',domain='CORNER')
 for f in me.polygons:
  f.use_smooth=True
  for li in f.loop_indices:
   vi=me.loops[li].vertex_index;p=me.vertices[vi].co
   me.uv_layers[0].data[li].uv=uv[vi] if uv else (p.x/40,p.z/80)
   c=color(p) if callable(color) else color
   me.color_attributes['Color'].data[li].color=tuple(c[:3])+(category,)
 for v in me.vertices:
  ws=weight(v.co,v.index/max(1,len(vs)-1));total=sum(ws.values())
  for n,w in ws.items():
   if w>1e-7:(ob.vertex_groups.get(n) or ob.vertex_groups.new(name=n)).add([v.index],w/total,'REPLACE')
 bm=bmesh.new();bm.from_mesh(me);bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(me);bm.free()
 PARTS.append(ob);COMPONENTS.append({'name':label,'vertices':len(vs),'category':category});return ob

def tube(label,points,radii,weight,color,category=.75,sides=24,flat=1,fold=.018):
 ps=list(map(Vector,points));vs=[];fs=[];uv=[]
 for j,(p,r) in enumerate(zip(ps,radii)):
  d=(ps[min(j+1,len(ps)-1)]-ps[max(0,j-1)]).normalized();ref=Vector((0,-1,0))
  if abs(d.dot(ref))>.95:ref=Vector((1,0,0))
  a=(ref-d*ref.dot(d)).normalized();b=d.cross(a)
  for i in range(sides+1):
   ang=TAU*i/sides;rr=r*(1+fold*math.sin(ang*4+j*.7));vs.append(p+a*math.cos(ang)*rr*flat+b*math.sin(ang)*rr);uv.append((i/sides,j/(len(ps)-1)))
 for j in range(len(ps)-1):
  for i in range(sides):a=j*(sides+1)+i;fs.append((a,a+1,a+sides+2,a+sides+1))
 fs.extend([tuple(range(sides,-1,-1)),tuple(range((len(ps)-1)*(sides+1),len(ps)*(sides+1)))])
 return mesh(label,vs,fs,weight,color,category,uv)

def seam(label,points,bone,color,r=.25):return tube(label,points,[r*K]*len(points),fixed(bone),color,.5,8,1,0)

def body():
 coat,shirt,pants,skin,hair=PALETTES[NAME];cy=bn('spine_03').y;hip=bn('pelvis').z;sh=bn('upperarm_l').z;w=abs(bn('upperarm_l').x)
 width={'Guard':1.06,'Fisherman':1.02,'Villagerm':1.03,'Villagerf':.93,'Elder':.96,'Child':.97}.get(NAME,1.)
 # Rounded chest/clavicle profile; no long rigid collar triangles into the head.
 prof=[(hip-7*K,.84,10.2),(hip,.85,10.5),(hip+9*K,.78,10.0),(hip+20*K,.83,11.3),(sh-9*K,.94,11.7),(sh-2*K,1.0,10.0),(sh+3*K,.87,7.6),(sh+6*K,.56,5.9),(bn('neck_01').z+5*K,.22,4.5)]
 vs=[];fs=[];uv=[];n=48
 for j,(z,rx,ry) in enumerate(prof):
  for i in range(n+1):
   a=TAU*i/n;fold=.22*K*math.sin(a*5+j*.4);vs.append((math.sin(a)*(w*rx*width+fold),cy+math.cos(a)*(ry*K+fold*.5),z));uv.append((i/n,j/(len(prof)-1)))
 for j in range(len(prof)-1):
  for i in range(n):a=j*(n+1)+i;fs.append((a,a+1,a+n+2,a+n+1))
 fs.extend([tuple(range(n,-1,-1)),tuple(range((len(prof)-1)*(n+1),len(prof)*(n+1)))])
 mesh('RoundedTailoredTorso',vs,fs,torso,coat,.75,uv)
 for side,sign in [('l',1),('r',-1)]:
  shpt,el,wr=bn('upperarm_'+side),bn('lowerarm_'+side),bn('hand_'+side)
  points=[shpt.lerp(el,t) for t in [-.10,0,.12,.3,.55,.8,1]]+[el.lerp(wr,t) for t in [.2,.45,.7,.9,1]]
  def aw(p,t,s=side):
   a,b,c=bn('upperarm_'+s),bn('lowerarm_'+s),bn('hand_'+s);d=(p-a).dot(b-a)/(b-a).length_squared
   q=(p-b).dot(c-b)/(c-b).length_squared
   if d<.30:return blend('spine_03','upperarm_'+s,.18+.82*clamp((d+.18)/.48))
   return blend('upperarm_'+s,'lowerarm_'+s,clamp((d-.83)/.34) if q<.12 else 1.)
  tube('RoundedSleeve_'+side,points,[r*K for r in [6.4,7.0,7.0,6.8,6.0,5.2,4.5,4.6,4.2,3.7,3.2,3.05]],aw,coat,.75,28,.98,.024)
  d=(wr-el).normalized();tube('SoftCuff_'+side,[wr-d*3.5*K,wr-d*.4*K],[3.4*K,3.25*K],fixed('lowerarm_'+side),shirt,.75,24,.98,.012)
  thigh,knee,ank=bn('thigh_'+side),bn('calf_'+side),bn('foot_'+side)
  ps=[thigh.lerp(knee,t) for t in [0,.15,.35,.6,.82,1]]+[knee.lerp(ank,t) for t in [.20,.45,.70,1]]
  def lw(p,t,s=side):
   a,b,c=bn('thigh_'+s),bn('calf_'+s),bn('foot_'+s);u=(p-a).dot(b-a)/(b-a).length_squared
   if u<.14:return blend('pelvis','thigh_'+s,.70+.30*clamp(u/.14))
   return blend('thigh_'+s,'calf_'+s,(u-.82)/.35)
  tube('NaturalTrousers_'+side,ps,[r*K for r in [9.0,9.3,8.8,7.7,6.4,5.3,5.6,5.2,4.6,4.1]],lw,pants,.75,28,.96,.023)
  tube('FittedBoot_'+side,[knee.lerp(ank,.57),knee.lerp(ank,.77),ank],[5.4*K,5.0*K,4.3*K],fixed('calf_'+side),(.035,.027,.021),.5,24,.96,.012)
  p=knee.lerp(ank,.60);d=(ank-knee).normalized();tube('BootCuff_'+side,[p-d*.5*K,p+d*.5*K],[5.5*K]*2,fixed('calf_'+side),(.08,.06,.04),.5,24,.96,0)
  # Human shoe last: flat sole and heel, a long low toe box, and a tapered ankle.
  vs=[];fs=[];n=40
  for j,(z,rx,front,back) in enumerate([(0,4.6,-17,6),(1.8,4.7,-17,6),(3.2,4.55,-16.5,5.9),(7,4.15,-14.8,5.3),(10.8,3.95,-8.5,4.8),(14,3.85,-3.9,4.5)]):
   for i in range(n+1):
    a=TAU*i/n;c=math.cos(a);y=(front+back)/2-(back-front)/2*c
    vs.append((ank.x+math.sin(a)*rx*K,ank.y+y*K,z*K))
  for j in range(5):
   for i in range(n):a=j*(n+1)+i;fs.append((a,a+1,a+n+2,a+n+1))
  fs.append(tuple(range(n,-1,-1)));fs.append(tuple(range(5*(n+1),6*(n+1))))
  mesh('AnatomicalBootFoot_'+side,vs,fs,fixed('foot_'+side),lambda p:(.025,.021,.018) if p.z<2.5*K else (.048,.033,.023),.5)
 # Split coat panels with two-bone skinning continue the pelvis without a rigid apron block.
 long=NAME in ['Villagerf','Elder','Scholar'];length=(H*.27 if long else H*.15)
 for side,sign in [('l',1),('r',-1)]:
  vs=[];uv=[];fs=[];rows=12;cols=28
  for j in range(rows+1):
   t=j/rows
   for i in range(cols+1):
    a=sign*(.15+(math.pi-.3)*i/cols);rx=w*(.85+.10*t)*width;ry=(10.8+1.5*t)*K
    vs.append((math.sin(a)*rx,cy+math.cos(a)*ry,hip+1*K-length*t+.25*K*t*math.sin(a*5)));uv.append((i/cols,t))
  for j in range(rows):
   for i in range(cols):a=j*(cols+1)+i;fs.append((a,a+1,a+cols+2,a+cols+1))
  def skirt(p,t,s=side):return blend('pelvis','thigh_'+s,.22*clamp((hip-p.z)/length))
  mesh('SuppleSplitCoat_'+side,vs,fs,skirt,coat,.75,uv)
 belt=[(math.sin(a)*w*.87*width,cy+math.cos(a)*11*K,hip+2*K) for a in [TAU*i/64 for i in range(65)]]
 tube('WaistBelt',belt,[1.2*K]*65,fixed('pelvis'),(.06,.043,.026),.5,8,1.4,0)
 for sign in [-1,1]:
  # Narrow flat lapels lie against the curved chest, ending below the collar.
  vs=[];fs=[]
  for j in range(14):
   t=j/13;z=sh+3*K-(sh-hip-14*K)*t;x=sign*(5.2+4.0*t)*K
   low,high=next((a,b) for a,b in zip(prof,prof[1:]) if a[0]<=z<=b[0]);v=(z-low[0])/(high[0]-low[0]);rx=w*(low[1]+(high[1]-low[1])*v)*width;ry=(low[2]+(high[2]-low[2])*v)*K
   for xx in [x-sign*.8*K,x+sign*.8*K]:vs.append((xx,cy-ry*math.sqrt(max(0,1-(xx/rx)**2))-.45*K,z))
  for j in range(13):a=j*2;fs.append((a,a+1,a+3,a+2))
  mesh('SoftLapel_'+str(sign),vs,fs,torso,shirt,.75)
 neck=bn('neck_01');tube('SoftStandingCollar',[neck+Vector((0,0,3.8*K)),neck+Vector((0,0,6.0*K)),neck+Vector((0,0,8.5*K))],[5.25*K,5.15*K,5.0*K],fixed('neck_01'),coat,.75,40,.96,.008)
 for z in [hip+12*K,hip+18*K,hip+24*K]:tube('SmallCoatButton',[(0,cy-11.8*K,z),(0,cy-12.25*K,z)],[.55*K]*2,torso,(.22,.16,.08),1,12,1,0)
 if NAME in ['Fisherman','Villagerm']:
  # Leather working apron is curved and follows the thighs; no planar black rectangle.
  vs=[];fs=[]
  for j in range(12):
   t=j/11
   for i in range(19):
    a=(i/18-.5)*1.35;vs.append((math.sin(a)*w*.9,cy-math.cos(a)*(12.1+.6*t)*K,hip-1*K-23*K*t))
  for j in range(11):
   for i in range(18):a=j*19+i;fs.append((a,a+1,a+20,a+19))
  mesh('CurvedWorkApron',vs,fs,lambda p,t:blend('pelvis','thigh_l' if p.x>0 else 'thigh_r',.13*clamp((hip-p.z)/(23*K))),(.095,.068,.043),.5)

def faces():
 global PARTS
 spec=importlib.util.spec_from_file_location('s357_face',ROOT/'Unreal/Tools/build_humanoid_faces_s357.py');f=importlib.util.module_from_spec(spec);spec.loader.exec_module(f)
 f.IS_ASH=False;f.HZ=0.;f.CY=0.;f.TOP=21.;f.RIG=RIG;f.FACE_MAT=MAT
 f.PARTS=[PARTS[0]];f.COMPONENTS=[];f.FACE_BVH=None;f.EYE_BOUNDARIES={};f.CLEARANCE_REPORTS=[]
 f.PROFILE=[(-11,4.5,-4.5,4.5),(-6,4.15,-4.8,4.1),(-.4,3.6,-5.7,3.9),(2,5.4,-7.5,5.3),(5.5,6.7,-8.1,6.6),(9.5,7.4,-8.15,7.1),(12.5,7.5,-7.8,7.5),(16.5,6.7,-6.5,7.0),(19.5,4.3,-4.2,4.4),(20.88,.04,-.02,.02)]
 skin=PALETTES[NAME][3];hair=PALETTES[NAME][4]
 female=NAME in ['Villagerf','Elder','Elia'];old=NAME in ['Elder','Fisherman','Malet']
 base_sculpt=f.sculpt
 def face_color(x,z):
  blush=.035*(f.gauss(x,z,4.6,6.7,2.0,1.4)+f.gauss(x,z,-4.6,6.7,2.0,1.4))
  beard=.18*clamp((5.0-z)/3)*clamp(z/1.5) if NAME in ['Fisherman','Malet'] else 0
  return (skin[0]*(1-beard)+blush,skin[1]*(1-beard),skin[2]*(1-beard),0)
 f.face_color=face_color
 def sculpt(x,z):return base_sculpt(x,z)*(.78 if female else .85)
 f.sculpt=sculpt
 f.create_cranium();f.orbital_features(-1);f.orbital_features(1);f.refresh_face_bvh()
 eye_seal=f.validate_eye_seals();seam_weld=f.weld_orbital_seams();f.refresh_face_bvh()
 for sign in [-1,1]:
  f.surface_ribbon('SoftBrow'+str(sign),[(sign*1.5,10.6),(sign*2.7,10.95),(sign*4.0,10.8),(sign*4.7,10.45)],.12,tuple(v*.8 for v in hair)+(0,),.055)
  f.surface_ribbon('Nostril'+str(sign),[(sign*.5,5.65),(sign*.85,5.6),(sign*1.1,5.8)],.06,(.075,.038,.025,0),.02)
  if old:f.surface_ribbon('SmileFold'+str(sign),[(sign*1.7,5.35),(sign*2.22,4.7),(sign*2.3,4.1)],.025,tuple(v*.79 for v in skin)+(0,),.02)
 f.surface_ribbon('QuietMouth',[(-1.8,3.98),(-.75,3.95),(0,3.90),(.8,3.95),(1.8,3.98)],.045,(.12,.05,.034,0),.028)
 f.surface_ribbon('NaturalLowerLip',[(-1.45,3.6),(0,3.5),(1.45,3.6)],.105,(skin[0]*.80,skin[1]*.57,skin[2]*.55,0),.025)
 for sign in [-1,1]:
  vs=[];fs=[];cs=[];nr=12;n=40
  for j in range(nr+1):
   r=.001+j/nr
   for i in range(n):
    a=TAU*i/n;x=sign*(7.1+1.25*r*math.cos(a));z=8.1+2.6*r*math.sin(a);y=2.0+.6*r*math.cos(a)+.65*(1-r*r)-.45*math.exp(-((r-.8)/.15)**2)
    vs.append((x,y,z));cs.append(tuple(v*(.78+.22*r) for v in skin)+(0,))
  for j in range(nr):
   for i in range(n):a=j*n+i;b=j*n+(i+1)%n;fs.append((a,b,b+n,a+n))
  f.add_mesh('AnatomicalEar'+str(sign),vs,fs,colors=cs)
 # A fitted softly parted hair shell, with actual neck-length rear hair for Elia and female villagers.
 vs=[];fs=[];cs=[];ns=96;rows=28
 for j in range(rows+1):
  for i in range(ns):
   a=TAU*i/ns;front=max(0,math.cos(a));bottom=5.2+8.2*front**2+1.0*math.sin(a*2)**2
   bottom+=.55*front*math.sin(a*3+.5)
   z=bottom+(20.94-bottom)*(1-(1-j/rows)**1.55);rx,yf,yb=f.lerp_profile(z);r=.20*(1-j/rows)**.30+.035
   x=(rx+r)*math.sin(a);y=(yf+yb)*.5-((yb-yf)*.5+r)*math.cos(a)
   if math.cos(a)>0:y+=f.sculpt(x,z)*math.cos(a)**2.5
   vs.append((x,y,z));tone=.90+.07*math.sin(a*27+z*.7);cs.append(tuple(v*tone for v in hair)+(0,))
 for j in range(rows):
  for i in range(ns):a=j*ns+i;b=j*ns+(i+1)%ns;fs.append((a,b,b+ns,a+ns))
 apex=len(vs);vs.append((0,0,21));cs.append(hair+(0,))
 for i in range(ns):fs.append((rows*ns+i,rows*ns+(i+1)%ns,apex))
 f.add_mesh('FittedPartedHair',vs,fs,colors=cs)
 if female:
  vs=[];fs=[];cs=[];rows=14;cols=48
  for j in range(rows+1):
   t=j/rows
   for i in range(cols+1):
    a=math.pi*.52+math.pi*.96*i/cols;z=10-17*t;rad=7.4-.6*t+.15*math.sin(a*8+t*3)
    vs.append((math.sin(a)*rad,3.3+math.cos(a)*(-5.8-.2*t),z));cs.append(tuple(v*(.91+.06*math.sin(a*22)) for v in hair)+(0,))
  for j in range(rows):
   for i in range(cols):a=j*(cols+1)+i;fs.append((a,a+1,a+cols+2,a+cols+1))
  f.add_mesh('SoftRearHair',vs,fs,colors=cs)
 # Child has a proportionally larger but no longer oversized head. Adults retain role-specific widths.
 scale=.91 if NAME=='Child' else .99 if NAME in ['Villagerf','Elia'] else 1.
 sx=scale*({'Guard':1.04,'Fisherman':1.03,'Elder':.96,'Scholar':.96}.get(NAME,1.))
 origin=H-21*scale
 created=f.PARTS[1:]
 for ob in created:
  for v in ob.data.vertices:v.co=Vector((v.co.x*sx,v.co.y*scale+bn('head').y,v.co.z*scale+origin))
  # Rebuild the narrow neck blend in actual rig coordinates, independent of face-local helpers.
  for v in ob.data.vertices:
   z=v.co.z;t=clamp((z-(bn('neck_01').z+1*K))/(origin+2*scale-(bn('neck_01').z+1*K)))
   if ob.name=='ContinuousNeckCraniofacialSculpt':
    for g in list(v.groups):ob.vertex_groups[g.group].remove([v.index])
    for n,w in blend('neck_01','head',t).items():
     if w>1e-7:(ob.vertex_groups.get(n) or ob.vertex_groups.new(name=n)).add([v.index],w,'REPLACE')
 PARTS.extend(created);COMPONENTS.extend(f.COMPONENTS)
 return {'head_height_cm':21*scale,'head_width_cm':15*sx,'head_to_height_ratio':H/(21*scale),'orbital_seal':eye_seal,'stitched_eyelids':seam_weld,'reused_topology_helper_sha256':sha(ROOT/'Unreal/Tools/build_humanoid_faces_s357.py')}

def fuse_cloth_shoulders():
 global PARTS
 chosen=[o for o in PARTS if o.name.startswith(('RoundedTailoredTorso','RoundedSleeve_'))]
 assert len(chosen)==3
 bpy.ops.object.select_all(action='DESELECT')
 for o in chosen:o.select_set(True)
 ob=chosen[0];bpy.context.view_layer.objects.active=ob;bpy.ops.object.join();ob.data.calc_loop_triangles()
 positions=[v.co.copy() for v in ob.data.vertices];triangles=[tuple(t.vertices) for t in ob.data.loop_triangles]
 ws=[{ob.vertex_groups[g.group].name:g.weight for g in v.groups} for v in ob.data.vertices]
 tree=BVHTree.FromPolygons(positions,triangles,all_triangles=True)
 mod=ob.modifiers.new('ContinuousClothShoulders','REMESH');mod.mode='VOXEL';mod.voxel_size=.65*K;mod.use_smooth_shade=True;bpy.ops.object.modifier_apply(modifier=mod.name)
 mod=ob.modifiers.new('AnatomicalShoulderBlend','SMOOTH');mod.factor=.65;mod.iterations=10;bpy.ops.object.modifier_apply(modifier=mod.name)
 mod=ob.modifiers.new('FieldDensity','DECIMATE');mod.ratio=.16;mod.use_collapse_triangulate=True;bpy.ops.object.modifier_apply(modifier=mod.name)
 ob.vertex_groups.clear();weights=[]
 for v in ob.data.vertices:
  p,no,idx,d=tree.find_nearest(v.co);ids=triangles[idx]
  bc=barycentric_transform(p,*(positions[i] for i in ids),Vector((1,0,0)),Vector((0,1,0)),Vector((0,0,1)))
  row={}
  for i,f in zip(ids,bc):
   for n,w in ws[i].items():row[n]=row.get(n,0)+max(0,f)*w
  weights.append(row)
 # Smooth skin weights over the fused joint so a formerly independent sleeve cap cannot lift away.
 adjacent=[set() for v in ob.data.vertices]
 for e in ob.data.edges:a,b=e.vertices;adjacent[a].add(b);adjacent[b].add(a)
 for iteration in range(4):
  result=[]
  for i,row in enumerate(weights):
   out={n:w*.55 for n,w in row.items()}
   for j in adjacent[i]:
    for n,w in weights[j].items():out[n]=out.get(n,0)+w*.45/max(1,len(adjacent[i]))
   keep=sorted(out.items(),key=lambda x:-x[1])[:4];total=sum(w for n,w in keep);result.append({n:w/total for n,w in keep})
  weights=result
 for v,row in zip(ob.data.vertices,weights):
  for n,w in row.items():
   if w>1e-7:(ob.vertex_groups.get(n) or ob.vertex_groups.new(name=n)).add([v.index],w,'REPLACE')
 for layer in list(ob.data.uv_layers):ob.data.uv_layers.remove(layer)
 for layer in list(ob.data.color_attributes):ob.data.color_attributes.remove(layer)
 ob.data.uv_layers.new(name='UVMap');ob.data.color_attributes.new(name='Color',type='FLOAT_COLOR',domain='CORNER')
 for f in ob.data.polygons:
  f.use_smooth=True
  for li in f.loop_indices:
   p=ob.data.vertices[ob.data.loops[li].vertex_index].co;ob.data.uv_layers[0].data[li].uv=(p.x/40,p.z/80);ob.data.color_attributes['Color'].data[li].color=PALETTES[NAME][0]+(.75,)
 PARTS=[o for o in PARTS if o not in chosen]+[ob];COMPONENTS.append({'name':'FusedContinuousTorsoShouldersSleeves','vertices':len(ob.data.vertices),'policy':'voxel union and smooth joint; transferred nearest-triangle weights with four adjacency smoothing passes'})

def load(name):
 global NAME,RIG,H,K,PARTS,COMPONENTS,MAT
 NAME=name;PARTS=[];COMPONENTS=[];bpy.ops.wm.read_factory_settings(use_empty=True)
 src=ROOT/'Unreal/ArtSource/HumanoidBodiesS358/R3'/name/(name+'.fbx')
 bpy.ops.import_scene.fbx(filepath=str(src),colors_type='LINEAR' if name in ['AshWalker','BeltScavenger'] else 'SRGB')
 RIG=next(o for o in bpy.data.objects if o.type=='ARMATURE');old=next(o for o in bpy.data.objects if o.type=='MESH')
 old.parent=None;old.matrix_world=Matrix.Identity(4);RIG.scale=(1,1,1);RIG.rotation_euler=(0,0,0);RIG.location=(0,0,0);RIG.animation_data_clear()
 for p in RIG.pose.bones:p.matrix_basis=Matrix.Identity(4)
 H=max(v.co.z for v in old.data.vertices)-min(v.co.z for v in old.data.vertices);K=H/180
 bpy.context.scene.unit_settings.system='METRIC';bpy.context.scene.unit_settings.scale_length=.01
 return src,old

def export(path,animation=False):
 bpy.ops.export_scene.fbx(filepath=str(path),use_selection=True,object_types={'ARMATURE'} if animation else {'ARMATURE','MESH'},axis_forward='-Y',axis_up='Z',apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',add_leaf_bones=False,use_armature_deform_only=True,path_mode='RELATIVE',bake_anim=animation,bake_anim_use_all_actions=False,bake_anim_use_nla_strips=False,bake_anim_simplify_factor=0,bake_anim_step=1,mesh_smooth_type='FACE',colors_type='LINEAR')

def build_mesh(name):
 global MAT,PARTS
 source,old=load(name);original_bones=[{'name':b.name,'parent':b.parent.name if b.parent else None,'matrix':[list(r) for r in b.matrix_local]} for b in RIG.data.bones]
 MAT=bpy.data.materials.new('NaturalHumanRegions');MAT.use_nodes=True;nodes=MAT.node_tree.nodes
 vc=nodes.new('ShaderNodeVertexColor');vc.layer_name='Color';MAT.node_tree.links.new(vc.outputs['Color'],nodes['Principled BSDF'].inputs['Base Color']);nodes['Principled BSDF'].inputs['Roughness'].default_value=.76
 bm=bmesh.new();bm.from_mesh(old.data);dv=bm.verts.layers.deform.active;remove=[]
 for v in bm.verts:
  ns=[old.vertex_groups[i].name for i,w in v[dv].items() if w>.02]
  keep=any(n.startswith(('hand_','thumb_','index_','middle_','ring_','pinky_')) for n in ns) if name in NPCS else v.co.z<H-24.0
  if not keep:remove.append(v)
 deleted=len(remove);bmesh.ops.delete(bm,geom=remove,context='VERTS');bm.to_mesh(old.data);bm.free();retained_count=len(old.data.vertices);PARTS=[old]
 # One canonical FLOAT_COLOR layer on every joined part. Legacy FBX reads layer zero.
 # Inherited S358 files contain many unused white layers that must not precede authored pigment.
 retained_uv={u.name:[tuple(v.uv) for v in u.data] for u in old.data.uv_layers}
 while len(old.data.color_attributes):old.data.color_attributes.remove(old.data.color_attributes[0])
 old.data.color_attributes.new(name='Color',type='FLOAT_COLOR',domain='CORNER')
 for n,vs in retained_uv.items():
  for v,uv in zip(old.data.uv_layers[n].data,vs):v.uv=uv
 for f in old.data.polygons:
  for li in f.loop_indices:old.data.color_attributes['Color'].data[li].color=(1,1,1,0) if f.material_index==0 else PALETTES[name][0]+(1,)
 if name in NPCS:
  old.data.materials.clear();old.data.materials.append(MAT)
  for f in old.data.polygons:f.material_index=0
  for f in old.data.polygons:
   for li in f.loop_indices:
    v=old.data.vertices[old.data.loops[li].vertex_index];ns=[old.vertex_groups[g.group].name for g in v.groups]
    hand=any(n.startswith(('hand_','thumb_','index_','middle_','ring_','pinky_')) for n in ns)
    old.data.color_attributes['Color'].data[li].color=PALETTES[name][3]+(0,) if hand else (.035,.027,.021,.5)
  body();fuse_cloth_shoulders()
 face_report=faces()
 # BMesh can change layer storage during intermediate welds. Normalize every part
 # after topology edits, and discard blank inherited layers before Blender joins them.
 for ob in PARTS:
  me=ob.data;authored=[tuple(v.color) for v in me.color_attributes['Color'].data];authored_uv={u.name:[tuple(v.uv) for v in u.data] for u in me.uv_layers}
  while len(me.color_attributes):me.color_attributes.remove(me.color_attributes[0])
  me.color_attributes.new(name='Color',type='FLOAT_COLOR',domain='CORNER')
  assert len(me.color_attributes['Color'].data)==len(authored)
  for value,color in zip(me.color_attributes['Color'].data,authored):value.color=color
  for n,vs in authored_uv.items():
   for value,uv in zip(me.uv_layers[n].data,vs):value.uv=uv
  me.color_attributes.active_color_index=0;me.color_attributes.render_color_index=0
 bpy.ops.object.select_all(action='DESELECT')
 for ob in PARTS:ob.select_set(True)
 bpy.context.view_layer.objects.active=old;bpy.ops.object.join();old.name=name+'_NaturalBody'
 assert len(old.data.color_attributes)==1 and old.data.color_attributes[0].name=='Color'
 old.data.color_attributes.active_color_index=0;old.data.color_attributes.render_color_index=0
 for mod in list(old.modifiers):old.modifiers.remove(mod)
 tri=old.modifiers.new('Triangles','TRIANGULATE');bpy.ops.object.modifier_apply(modifier=tri.name)
 # Automatic normals from coherent curved anatomy, retaining painted named outfits' topology.
 old.data.normals_split_custom_set([(0,0,0)]*len(old.data.loops))
 old.parent=RIG;old.matrix_parent_inverse=Matrix.Identity(4);old.matrix_basis=Matrix.Identity(4);mod=old.modifiers.new('Skin','ARMATURE');mod.object=RIG
 assert all(v.groups and abs(sum(g.weight for g in v.groups)-1)<.0001 for v in old.data.vertices)
 height=max(v.co.z for v in old.data.vertices)-min(v.co.z for v in old.data.vertices);assert abs(height-H)<.02,(name,height,H)
 old.data.calc_loop_triangles();assert len(old.data.loop_triangles)<(48000 if name in NPCS else 65000),(name,len(old.data.loop_triangles))
 dest=OUT/name;dest.mkdir(parents=True,exist_ok=False);bpy.ops.object.select_all(action='DESELECT');old.select_set(True);RIG.select_set(True);bpy.context.view_layer.objects.active=RIG
 export(dest/(name+'.fbx'))
 report={'name':name,'source':str(source.relative_to(ROOT)),'source_sha256':sha(source),'output_sha256':sha(dest/(name+'.fbx')),'generator_sha256':sha(Path(__file__)),'height_cm':height,'triangles':len(old.data.loop_triangles),'bounds_cm':box([v.co for v in old.data.vertices]),'material_slots':[m.name for m in old.data.materials],'removed_vertices':deleted,'retained_vertices':retained_count,'face':face_report,'components':COMPONENTS,'bones':original_bones,'scope':'ambient anatomy replaced; original articulated hands retained and recolored; new anatomical boots' if name in NPCS else 'original painted garment geometry retained below head region; face/hair replaced'}
 (dest/'geometry.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
 # Verify the actual exported FBX layer-zero colors and original bind matrices before accepting source art.
 expected_mean=[sum(c.color[i] for c in old.data.color_attributes['Color'].data)/len(old.data.loops) for i in range(4)]
 expected_uv=sorted(tuple(v.uv) for v in old.data.uv_layers[0].data)
 bpy.ops.wm.read_factory_settings(use_empty=True);bpy.ops.import_scene.fbx(filepath=str(dest/(name+'.fbx')),colors_type='LINEAR')
 back=next(o for o in bpy.data.objects if o.type=='MESH');backrig=next(o for o in bpy.data.objects if o.type=='ARMATURE')
 assert len(back.data.color_attributes)==1 and back.data.color_attributes[0].name=='Color'
 actual_mean=[sum(c.color[i] for c in back.data.color_attributes[0].data)/len(back.data.loops) for i in range(4)]
 actual_uv=sorted(tuple(v.uv) for v in back.data.uv_layers[0].data);assert len(actual_uv)==len(expected_uv)
 uv_delta=max(abs(a-b) for av,bv in zip(actual_uv,expected_uv) for a,b in zip(av,bv));assert uv_delta<.000002
 assert max(abs(a-b) for a,b in zip(expected_mean,actual_mean))<.004
 assert [b.name for b in backrig.data.bones]==[b['name'] for b in original_bones]
 delta=max(abs(float(b.matrix_local[r][c])-original_bones[i]['matrix'][r][c]) for i,b in enumerate(backrig.data.bones) for r in range(4) for c in range(4));assert delta<.015
 report['fbx_roundtrip']={'color_layers':['Color'],'color_mean':actual_mean,'max_color_mean_delta':max(abs(a-b) for a,b in zip(expected_mean,actual_mean)),'max_uv_delta':uv_delta,'max_bind_matrix_delta':delta}
 (dest/'geometry.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
 print('S359_MESH',name,report['triangles'],face_report['head_to_height_ratio'],flush=True)

def reset_pose():
 for p in RIG.pose.bones:p.matrix_basis=Matrix.Identity(4);p.rotation_mode='QUATERNION';p.scale=(1,1,1)

def world_rotate(name,axis,angle):
 p=RIG.pose.bones[name];q=p.bone.matrix_local.to_quaternion();p.rotation_quaternion=p.rotation_quaternion@(q.inverted()@Quaternion(axis,angle)@q)

def world_move(name,delta):
 p=RIG.pose.bones[name];p.location=p.bone.matrix_local.to_quaternion().inverted()@Vector(delta)

def aim(name,child,target):
 bpy.context.view_layer.update();p=RIG.pose.bones[name];start=p.head.copy();rest=bn(child)-bn(name)
 q=rest.normalized().rotation_difference((Vector(target)-start).normalized())@p.bone.matrix_local.to_quaternion()
 p.matrix=Matrix.LocRotScale(start,q,Vector((1,1,1)));bpy.context.view_layer.update()

def ik(a,b,c,target,pole):
 bpy.context.view_layer.update();s=RIG.pose.bones[a].head.copy();target=Vector(target);pole=Vector(pole)
 l1=(bn(b)-bn(a)).length;l2=(bn(c)-bn(b)).length;d=target-s;dist=min(d.length,l1+l2-.01*K);axis=d.normalized()
 near=s+axis*dist;along=(l1*l1-l2*l2+dist*dist)/(2*dist);off=math.sqrt(max(.001,l1*l1-along*along));perp=pole-s-axis*(pole-s).dot(axis)
 if perp.length<.01:perp=Vector((0,-1,0))
 elbow=s+axis*along+perp.normalized()*off;aim(a,b,elbow);aim(b,c,near)

def contact_leg(side,phase,walk,pelvis_drop=0):
 sign=1 if side=='l' else -1;f=bn('foot_'+side);hip=bn('thigh_'+side);scale=H/180
 stance=clamp(abs(f.x)*.62,6*scale,12*scale);stride=(17 if NAME=='Child' else 23 if NAME=='AshWalker' else 26 if NAME=='BeltScavenger' else 30)*scale if walk else 0
 ph=phase%1
 if not walk:y=0;lift=0
 elif ph<.5:y=-stride/2+stride*ph*2;lift=0
 else:
  # Match the support leg's backward velocity at toe-off and heel contact.
  # Zero-velocity smoothstep endpoints caused a visible three-millimeter contact skid after retiming.
  t=(ph-.5)*2;y=stride*(.5+t-6*t*t+4*t*t*t);lift=(5 if NAME=='AshWalker' else 7)*scale*math.sin(math.pi*t)**1.25
 # Compensate swing extension so feet stay at ground height in the support half.
 target=Vector((sign*stance,f.y+y,f.z+lift))
 ik('thigh_'+side,'calf_'+side,'foot_'+side,target,(sign*stance,-90*scale,hip.z*.55))
 p=RIG.pose.bones['foot_'+side];p.matrix=Matrix.LocRotScale(p.head,p.bone.matrix_local.to_quaternion(),Vector((1,1,1)));bpy.context.view_layer.update()
 return {'foot':list(p.head),'support':ph<.5 if walk else True}

def arm(side,phase,walk,attack=0,wind=0,hit=0):
 sign=1 if side=='l' else -1;s=RIG.pose.bones['upperarm_'+side].head.copy();length=(bn('lowerarm_'+side)-bn('upperarm_'+side)).length+(bn('hand_'+side)-bn('lowerarm_'+side)).length
 swing=math.sin(TAU*phase)*(-1 if side=='l' else 1) if walk else .04*math.sin(TAU*phase)
 x=s.x+sign*2.5*K;y=s.y-2*K+swing*7*K;z=s.z-length*.965
 if NAME=='Elia' and side=='l':y-=4*K;z+=5*K
 if NAME=='Malet':y-=2*K;z+=2*K
 if NAME=='AshWalker':y-=5*K;z+=4*K
 active=NAME=='AshWalker' or side=='r'
 if active:
  y+=10*K*wind-35*K*attack;z+=27*K*attack;x+=sign*8*K*attack
 if hit:y+=8*K*hit;z+=5*K*hit
 ik('upperarm_'+side,'lowerarm_'+side,'hand_'+side,(x,y,z),(s.x+sign*8*K,s.y-20*K,s.z-length*.47))
 # Preserve the local hand/finger hierarchy, with a relaxed slight wrist extension.
 world_rotate('hand_'+side,(1,0,0),.035)
 for stem in ['index','middle','ring','pinky']:
  for j,a in [(1,.10),(2,.16),(3,.10)]:
   n=f'{stem}_0{j}_{side}'
   if n in RIG.pose.bones:world_rotate(n,(0,1,0),sign*a)

def animate(name):
 source,old=load(name);foe=name in ['AshWalker','BeltScavenger'];walker=name=='AshWalker'
 clips={('Zombie_Idle_Loop' if walker else 'Idle'):120,('Zombie_Walk_Fwd_Loop' if walker else 'Walk'):36}
 if foe:clips.update({('Zombie_Scratch' if walker else 'Sword_Regular_A'):48,'HitReact_Front_Lgt_01':30,'Death_Front_01':72})
 dest=OUT/name;dest.mkdir(parents=True,exist_ok=True);records=[];scene=bpy.context.scene;scene.render.fps=30
 for clip,end in clips.items():
  RIG.animation_data_clear();scene.frame_start=0;scene.frame_end=end;samples=[];first=None;last=None
  walk=clip in ['Walk','Zombie_Walk_Fwd_Loop'];idle=clip in ['Idle','Zombie_Idle_Loop'];death=clip=='Death_Front_01'
  for frame in range(end+1):
   scene.frame_set(frame);t=frame/end;reset_pose();phase=TAU*t;attack=wind=hit=0
   bob=.22*math.sin(phase) if idle else -.50+.42*math.cos(phase*2) if walk else 0
   world_move('pelvis',(0,0,bob*K-1.4*K))
   hunch=.08 if walker else .035 if name=='Elder' else .006
   world_rotate('spine_02',(1,0,0),hunch+.009*math.sin(phase))
   world_rotate('spine_03',(0,0,1),.025*math.sin(phase) if walk else .005*math.sin(phase))
   world_rotate('head',(1,0,0),-hunch*.6+.007*math.sin(phase));world_rotate('head',(0,0,1),.012*math.sin(phase))
   if not idle and not walk and not death:
    if clip=='HitReact_Front_Lgt_01':
     hit=math.sin(math.pi*t)**2;world_rotate('spine_01',(1,0,0),-.12*hit);world_rotate('spine_02',(1,0,0),-.20*hit);world_move('pelvis',(0,2*K*hit,-1.4*K-2*K*hit))
    else:
     attack=math.sin(math.pi*.5*min(1,t/.55))**3 if t<=.55 else math.cos(math.pi*.5*(t-.55)/.45)**2
     wind=math.sin(math.pi*min(1,t/.32))**2 if t<.32 else 0
     world_rotate('spine_01',(0,0,1),-.16*wind+.22*attack);world_rotate('spine_02',(1,0,0),-.08*wind+.15*attack);world_rotate('head',(1,0,0),-.08*attack)
     world_move('pelvis',(0,-2.5*K*attack,-1.4*K-1.0*K*attack))
   bpy.context.view_layer.update()
   feet=[contact_leg(s,(t+(0 if s=='l' else .5))%1,walk) for s in ['l','r']]
   arm('l',t,walk,attack,wind,hit);arm('r',t,walk,attack,wind,hit)
   if death:
    # Anatomical collapse: knees yield first, then a controlled pelvis-side fall and final settle.
    p=clamp(t/.78);p=p*p*(3-2*p);world_rotate('pelvis',(1,0,0),1.42*p);world_move('pelvis',(0,12*K*p,-1.4*K-72*K*p))
    world_rotate('thigh_l',(1,0,0),-.34*p);world_rotate('thigh_r',(1,0,0),.18*p);world_rotate('head',(1,0,0),-.13*p)
   bpy.context.view_layer.update()
   pose={p.name:[list(p.location),list(p.rotation_quaternion),list(p.scale)] for p in RIG.pose.bones}
   if frame==0:first=pose
   if frame==end:last=pose
   for p in RIG.pose.bones:
    assert all(abs(v-1)<.0001 for v in p.scale),(name,clip,frame,p.name,list(p.scale))
    p.keyframe_insert(data_path='location',frame=frame);p.keyframe_insert(data_path='rotation_quaternion',frame=frame);p.keyframe_insert(data_path='scale',frame=frame)
   if frame%3==0:samples.append({'frame':frame,'feet':feet,'head':list(RIG.pose.bones['head'].head),'hands':[list(RIG.pose.bones['hand_'+s].head) for s in ['l','r']]})
  # Loop endpoints are numerically equal, including the contact solver and secondary motion.
  if idle or walk:
   delta=max(abs(a-b) for n in first for aa,bb in zip(first[n],last[n]) for a,b in zip(aa,bb));assert delta<.0002,(name,clip,delta)
  else:delta=None
  bpy.ops.object.select_all(action='DESELECT');RIG.select_set(True);bpy.context.view_layer.objects.active=RIG
  export(dest/(clip+'.fbx'),True)
  records.append({'clip':clip,'source_fps':30,'source_frames':end,'sha256':sha(dest/(clip+'.fbx')),'loop_max_local_delta':delta,'samples':samples,'strike_apex_fraction':.55 if clip in ['Zombie_Scratch','Sword_Regular_A'] else None})
 (dest/'clips.json').write_text(json.dumps({'name':name,'source_sha256':sha(source),'generator_sha256':sha(Path(__file__)),'bones':77,'clips':records,'foot_policy':'two-bone contact solve, grounded stance, lifted swing; no root translation','timing_policy':'foe clips resampled to original runtime durations; new NPC gait uses existing distance phase'},indent=2)+'\n',encoding='utf-8')
 print('S359_MOTION',name,len(records),flush=True)

if __name__=='__main__':
 args=sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else []
 mode=args.pop(0) if args and args[0] in ['Mesh','Motion'] else 'All';names=args or NAMES
 assert all(n in NAMES for n in names)
 OUT.mkdir(parents=True,exist_ok=True)
 for name in names:
  if mode in ['Mesh','All'] and name not in ['AshWalker','BeltScavenger']:build_mesh(name)
  if mode in ['Motion','All']:animate(name)
 inv=[{'path':str(p.relative_to(OUT)),'bytes':p.stat().st_size,'sha256':sha(p)} for p in sorted(OUT.rglob('*')) if p.is_file() and p.name!='source_inventory.json']
 (OUT/'source_inventory.json').write_text(json.dumps({'revision':'R6','generator_sha256':sha(Path(__file__)),'files':inv},indent=2)+'\n',encoding='utf-8')
 print('S359_AUTHORING_COMPLETE',mode,names,flush=True)
