"""S347 authored townsfolk, using the project's exact S310/S334 rig and head template.
Run with the existing runtime311 Python (bpy4.5.3, NumPy and Pillow). No inference/network.
"""
from pathlib import Path
import argparse, ast, hashlib, json, math, random
import bpy, bmesh, numpy as np
from mathutils import Vector, Matrix
from PIL import Image, ImageDraw

SPECS = {
 'npc_villager_f': dict(height=165, width=.89, hair='long', coat=(115,89,107), shirt=(153,133,140), pants=(82,64,71), skin=(219,194,168), hair_color=(128,77,51), boots=(38,31,31)),
 'npc_villager_m': dict(height=175, width=1.02, hair='short', coat=(89,77,64), shirt=(115,102,89), pants=(56,51,46), skin=(204,173,148), hair_color=(71,56,46), boots=(31,26,20)),
 'npc_fisherman': dict(height=172, width=1.04, hair='short', coat=(64,89,102), shirt=(102,122,128), pants=(51,64,71), skin=(191,158,133), hair_color=(89,82,71), boots=(31,26,26)),
 'npc_elder': dict(height=165, width=.95, hair='long', coat=(89,71,97), shirt=(128,115,122), pants=(64,51,64), skin=(204,179,153), hair_color=(191,186,179), boots=(36,26,31)),
 'npc_child': dict(height=120, width=1.0, hair='short', coat=(128,140,89), shirt=(166,153,115), pants=(89,77,56), skin=(224,199,173), hair_color=(140,102,64), boots=(51,38,26)),
 'npc_scholar': dict(height=172, width=.94, hair='medium', coat=(46,71,56), shirt=(89,102,89), pants=(38,46,38), skin=(209,184,158), hair_color=(77,64,51), boots=(26,20,20)),
}
parser=argparse.ArgumentParser(); parser.add_argument('ident',choices=sorted(SPECS)); parser.add_argument('--models-root',required=True)
args=parser.parse_args(); ID=args.ident; cfg=SPECS[ID]; H=cfg['height']; K=H/180; HUSK=False
ROOT=Path(args.models_root).resolve(); OUT=ROOT/ID; WORK=ROOT/'_raw/s347_townsfolk'/ID
for p in (OUT,WORK): p.mkdir(parents=True,exist_ok=True)
random.seed(347+list(SPECS).index(ID)); rng=np.random.default_rng(347+list(SPECS).index(ID))
rig_source=ROOT/'_raw/s310_rigged/arrel_rigged.blend'
bpy.ops.wm.open_mainfile(filepath=str(rig_source))
source_body=bpy.data.objects['ArrelBody']; source_head=[]
for f in source_body.data.polygons:
 if all(source_body.data.vertices[i].co.z>151 and abs(source_body.data.vertices[i].co.x)<16 for i in f.vertices):
  mid=sum((source_body.data.vertices[i].co for i in f.vertices),Vector())/len(f.vertices)
  if mid.z<156 and mid.y>-4: continue
  source_head.append([(tuple(source_body.data.vertices[source_body.data.loops[i].vertex_index].co),tuple(source_body.data.uv_layers.active.data[i].uv)) for i in f.loop_indices])
assert source_head
rig=bpy.data.objects.get('Armature'); assert rig and len(rig.data.bones)==77
expected_parents={b.name:b.parent.name if b.parent else None for b in rig.data.bones}
for ob in list(bpy.data.objects):
 if ob!=rig: bpy.data.objects.remove(ob,do_unlink=True)
rig.name='Armature'; rig.animation_data_clear(); rig.hide_set(False); rig.hide_viewport=False; rig.hide_render=False
for b in rig.pose.bones: b.matrix_basis=Matrix.Identity(4)

def fit(co):
 p=Vector(co)
 if ID=='npc_child':
  z=float(np.interp(p.z,[0,100,150,180],[0,61,96,120])); w=.68 if p.z<148 else .80
  return Vector((p.x*w,p.y*w,z))
 return Vector((p.x*K*cfg['width'],p.y*K,p.z*K))

bpy.context.view_layer.objects.active=rig; rig.select_set(True); bpy.ops.object.mode_set(mode='EDIT')
for b in rig.data.edit_bones: b.head=fit(b.head); b.tail=fit(b.tail)
bpy.ops.object.mode_set(mode='OBJECT'); rig.select_set(False)
scene=bpy.context.scene; scene.unit_settings.system='METRIC'; scene.unit_settings.scale_length=.01
bones={b.name:dict(head=b.head_local.copy(),tail=b.tail_local.copy()) for b in rig.data.bones}
def head(n): return bones[n]['head'].copy()
def tail(n): return bones[n]['tail'].copy()
parts=[]; part_tags={}
colors=[cfg['coat'],cfg['boots'],cfg['shirt'],cfg['skin'],cfg['hair_color'],(151,128,83),(104,88,66),cfg['pants']]
atlas=np.zeros((2048,2048,3),dtype=np.uint8)
for i,color in enumerate(colors):
 yy,xx=np.mgrid[:512,:512]; n=rng.normal(0,.85,(512,512))
 grain=.8*np.sin(xx*2.0)+.7*np.sin(yy*2.1)
 if i==4: grain=3.5*np.sin(xx*.24+np.sin(yy*.006))+.6*np.sin(xx*2.5)
 broad=2*np.sin(xx*.03)*np.sin(yy*.025)
 if i in (0,2,7): broad+=6*np.sin(xx*.032+1.6*np.sin(yy*.012))*np.sin(yy*.008)
 tile=np.clip(np.array(color)[None,None,:]+(n+grain+broad)[:,:,None],0,255).astype(np.uint8)
 im=Image.fromarray(tile); d=ImageDraw.Draw(im)
 if i in (0,2,7):
  for y in range(15,499,11):
   for x in (12,498): d.line((x,y,x+2,y+4),fill=tuple(min(255,c+18) for c in color),width=1)
  for j in range(34):
   x=random.randrange(20,490); y=random.randrange(20,490)
   d.line((x,y,x+random.randrange(-8,9),min(503,y+random.randrange(5,25))),fill=tuple(max(0,c-6) for c in color),width=1)
 if i==1:
  for y in (410,450,470): d.line((10,y,502,y),fill=tuple(min(255,c+9) for c in color),width=2)
 if i==5:
  d.line((10,20,10,492),fill=(185,161,107),width=5)
 ty,tx=divmod(i,4); atlas[ty*512:(ty+1)*512,tx*512:(tx+1)*512]=np.asarray(im)
# The lower half holds the unchanged project's facial paint, fitted to each source skin/hair palette.
source_tex=Image.open(ROOT/'arrel/arrel_basecolor.png').convert('RGB').resize((2048,1024),Image.Resampling.LANCZOS)
ha=np.asarray(source_tex).astype(float); hm=Image.new('L',(2048,1024)); hd=ImageDraw.Draw(hm)
for face in source_head:
 cx,cy,cz=np.array([co for co,uv in face]).mean(0)
 if cz>161 or (abs(cx)>7.5 and cz>157) or (cy>7 and cz>157): hd.polygon([(uv[0]*2048,(1-uv[1])*1024) for co,uv in face],fill=255)
cool=(ha[:,:,2]>=ha[:,:,0]*.91)&(ha[:,:,2]>=ha[:,:,1]*.94)
hair_mask=(np.asarray(hm)>0)&cool; light=ha.mean(2)/255
ha[hair_mask]=np.clip(np.array(cfg['hair_color'])[None,None,:]*(.48+light[:,:,None]*.85),0,255)[hair_mask]
warm=(ha[:,:,0]>ha[:,:,1]*1.04)&(ha[:,:,1]>ha[:,:,2]*1.03)&(~hair_mask)
ratio=np.array(cfg['skin'])/np.array((219,194,168)); ha[warm]=np.clip(ha*ratio[None,None,:],0,255)[warm]
atlas[1024:]=ha.astype(np.uint8)
Image.fromarray(atlas).save(OUT/f'{ID}_basecolor.png')
mat=bpy.data.materials.new(ID+'_Surface'); mat.use_nodes=True; nt=mat.node_tree
pbr=nt.nodes.get('Principled BSDF'); pbr.inputs['Roughness'].default_value=.89; pbr.inputs['Specular IOR Level'].default_value=.19
tex=nt.nodes.new('ShaderNodeTexImage'); tex.image=bpy.data.images.load(str(OUT/f'{ID}_basecolor.png'),check_existing=False); nt.links.new(tex.outputs['Color'],pbr.inputs['Base Color'])
# GEOMETRY_FUNCTIONS: explicitly adapted from checked S334 helpers, appended at authoring time.


def mesh(name,verts,faces,uvs,weights,tile=0,tag="authored_cleanup"):
 me=bpy.data.meshes.new(name);me.from_pydata(verts,[],faces);me.update()
 ob=bpy.data.objects.new(name,me);scene.collection.objects.link(ob);me.materials.append(mat)
 uv=me.uv_layers.new(name="UVMap");ty,tx=divmod(tile,4)
 for face in me.polygons:
  face.use_smooth=True
  for li in face.loop_indices:
   v=me.loops[li].vertex_index;u,w=uvs[v]
   uv.data[li].uv=((tx*512+8+496*u)/2048,1-(ty*512+8+496*w)/2048)
 groups={}
 for vi,wd in enumerate(weights):
  sm=sum(wd.values())
  for bn,wt in wd.items():
   if wt<=.000001:continue
   if bn not in groups:groups[bn]=ob.vertex_groups.new(name=bn)
   groups[bn].add([vi],wt/sm,"REPLACE")
 parts.append(ob);part_tags[name]=tag
 return ob

def fixed(b):return lambda t,p:{b:1}

def chain_weight(names,t):
 # continuous equal arc parameter, blend across joints while retaining rigid limb centers
 seg=t*(len(names));i=min(len(names)-1,int(seg));f=seg-i
 if i>0 and f<.28:return {names[i-1]:(.28-f)/.56,names[i]:1-(.28-f)/.56}
 if i<len(names)-1 and f>.72:return {names[i]:(1.28-f)/.56,names[i+1]:1-(1.28-f)/.56}
 return {names[i]:1}

def tube(name,points,radii,tile,weight,n=16,flat=1,tip=True,creases=.035):
 ps=[Vector(p) for p in points];v=[];uv=[];ws=[];faces=[]
 for j,(p,r) in enumerate(zip(ps,radii)):
  d=(ps[min(j+1,len(ps)-1)]-ps[max(0,j-1)]).normalized()
  a=Vector((0,-1,0));a=(a-d*a.dot(d)).normalized();b=d.cross(a).normalized()
  for k in range(n+1):
   ang=k/n*math.tau;rr=r*(1+creases*math.cos(ang*6+j*.52))
   pos=p+a*math.cos(ang)*rr*flat+b*math.sin(ang)*rr
   v.append(tuple(pos));uv.append((k/n,j/(len(ps)-1)));ws.append(weight(j/(len(ps)-1),pos))
 for j in range(len(ps)-1):
  for k in range(n):a=j*(n+1)+k;faces.append((a,a+1,a+n+2,a+n+1))
 if tip:faces.extend([tuple(range(n,-1,-1)),tuple(range((len(ps)-1)*(n+1),len(ps)*(n+1)))])
 return mesh(name,v,faces,uv,ws,tile)

def ellipsoid(name,c,r,tile,bone,nu=20,nv=12):
 v=[];uv=[];faces=[];ws=[];c=Vector(c)
 for j in range(nv+1):
  theta=.001+(math.pi-.002)*j/nv
  for i in range(nu+1):
   a=math.tau*i/nu;p=c+Vector((r[0]*math.sin(theta)*math.sin(a),r[1]*math.sin(theta)*math.cos(a),r[2]*math.cos(theta)))
   v.append(tuple(p));uv.append((i/nu,j/nv));ws.append({bone:1})
 for j in range(nv):
  for i in range(nu):a=j*(nu+1)+i;faces.append((a,a+1,a+nu+2,a+nu+1))
 return mesh(name,v,faces,uv,ws,tile)

def panel(name,angle,length,width,tile,anchor="spine_03"):
 vs=[];us=[];weights=[];fs=[]
 for j in range(7):
  t=j/6
  for i in range(7):
   u=i/6;theta=angle+(u-.5)*width
   r=(17+8*t)*K;z=H*.79-length*t
   if j==6:z+=K*(.7*math.sin(i*2.6)+.35*(i%2))
   p=Vector((math.sin(theta)*r,6.5*K+math.cos(theta)*r*.69+2.3*K*math.sin(math.pi*t),z))
   vs.append(tuple(p));us.append((u,t));weights.append({anchor:1})
 for j in range(6):
  for i in range(6):a=j*7+i;fs.append((a,a+7,a+8,a+1))
 ob=mesh(name,vs,fs,us,weights,tile)
 # Thickness, so rear/side are visible in Unreal without a two-sided material.
 bpy.context.view_layer.objects.active=ob;ob.select_set(True)
 so=ob.modifiers.new("FabricThickness","SOLIDIFY");so.thickness=.38*K;so.offset=0
 bpy.ops.object.modifier_apply(modifier=so.name);ob.select_set(False)
 return ob

def torso_weight(t,p):
 nodes=sorted((head(n).z,n) for n in ('pelvis','spine_01','spine_02','spine_03'))
 if p.z<=nodes[0][0]:return {nodes[0][1]:1}
 if p.z>=nodes[-1][0]:return {nodes[-1][1]:1}
 for (za,a),(zb,b) in zip(nodes,nodes[1:]):
  if za<=p.z<=zb:
   f=(p.z-za)/max(zb-za,.001);return {a:1-f,b:f}
 return {'spine_02':1}

# Continuous garment rings and articulated limbs; sleeves avoid disconnected shoulder sockets.
vs=[]; fs=[]; us=[]; ws=[]; rings=11; sides=32
for j in range(rings):
 t=j/(rings-1); z=H*(.51+.29*t); width=(17.8+1.8*math.sin(math.pi*t))*K*cfg['width']
 depth=(10.5+1.2*math.sin(math.pi*t))*K
 if ID=='npc_villager_f': width*=.93+.08*t
 for i in range(sides+1):
  a=math.tau*i/sides; p=Vector((math.sin(a)*width,3*K+math.cos(a)*depth,z))
  vs.append(tuple(p));us.append((i/sides,t));ws.append(torso_weight(t,p))
for j in range(rings-1):
 for i in range(sides):a=j*(sides+1)+i;fs.append((a,a+1,a+sides+2,a+sides+1))
mesh('LinenTorso',vs,fs,us,ws,2)
if ID not in ('npc_villager_f','npc_elder','npc_scholar'):
 ellipsoid('ConnectedTrouserSeat',(0,4*K,H*.50),(21*K*cfg['width'],10*K,9*K),7,'pelvis',24,12)
tube('Neck',[head('neck_01'),tail('neck_01')+Vector((0,0,6*K))],[4.8*K,4.6*K],3,fixed('neck_01'),n=18)
for side,sign in (('l',1),('r',-1)):
 sh,el,wr=head('upperarm_'+side),head('lowerarm_'+side),head('hand_'+side)
 pts=[sh.lerp(el,t) for t in (0,.22,.5,.75,1)]+[el.lerp(wr,t) for t in (.25,.5,.75,1)]
 radii=[6.8,6.6,5.8,4.6,4.0,4.3,4.0,3.4,2.5]
 tube('CoatSleeve_'+side,pts,[r*K for r in radii],0,lambda t,p,s=side:chain_weight(['upperarm_'+s,'lowerarm_'+s],t),n=20,flat=.88)
 ellipsoid('SleeveShoulder_'+side,sh+Vector((-sign*.7*K,0,0)),(6.4*K,5.9*K,5.9*K),0,'upperarm_'+side,20,10)
 # The rolled cuff encloses the wrist and retains the lower-arm skin attachment.
 along=(wr-el).normalized();tube('Cuff_'+side,[wr-along*3*K,wr-along*1*K],[3.2*K,3.1*K],2,fixed('lowerarm_'+side),n=18)
 hip,knee,ank=head('thigh_'+side),head('calf_'+side),head('foot_'+side)
 if ID in ('npc_villager_f','npc_elder','npc_scholar'):
  # Hidden legs under closed skirts are omitted: they otherwise pierce the cloth in a stride.
  hem={'npc_villager_f':.23,'npc_elder':.15,'npc_scholar':.18}[ID]
  top=H*hem-2*K;f=min(1,max(0,(knee.z-top)/(knee.z-ank.z)))
  pts=[knee.lerp(ank,t) for t in np.linspace(f,1,6)]
  tube('ExposedTrouserCuff_'+side,pts,[r*K for r in (4.5,4.4,4.2,4.0,3.7,3.5)],7,fixed('calf_'+side),n=20,flat=1.03)
 else:
  pts=[hip.lerp(knee,t) for t in (0,.2,.45,.75,1)]+[knee.lerp(ank,t) for t in (.25,.5,.8,1)]
  tube('WorkTrousers_'+side,pts,[r*K for r in (8.1,8.3,7.2,5.7,4.7,5.4,4.8,3.7,3.5)],7,lambda t,p,s=side:chain_weight(['thigh_'+s,'calf_'+s],t),n=20,flat=1.03)
 ellipsoid('WornBoot_'+side,(ank.x,ank.y-4*K,8*K),(5.1*K,10.6*K,8*K),1,'foot_'+side,24,12)
 tube('BootShaft_'+side,[ank+Vector((0,0,-2*K)),ank+Vector((0,0,13*K))],[4.1*K,4.2*K],1,fixed('foot_'+side),n=16)
 handend=tail('hand_'+side);tube('Palm_'+side,[wr,wr.lerp(handend,.55),handend],[2.3*K,3.3*K,2.8*K],3,fixed('hand_'+side),n=16,flat=.4)
 for finger in ('thumb','index','middle','ring','pinky'):
  names=[f'{finger}_{i:02d}_{side}' for i in range(1,4)]; pts=[head(n) for n in names]+[tail(names[-1])]; ps=[]; radii=[]
  for j in range(3):
   for u in (0,.5): ps.append(pts[j].lerp(pts[j+1],u));radii.append(.70*K*(1-.15*(j+u)))
  ps.append(pts[-1]);radii.append(.24*K)
  tube(f'{finger}_{side}',ps,radii,3,lambda t,p,ns=names:chain_weight(ns,t),n=8,flat=.85,creases=0)
# Fitted project head. Skin/hair paint is preserved under the independent role silhouettes.
hv=[]; hf=[]; hu=[]
for face in source_head:
 ids=[]
 for co,uv in face:
  p=fit(co)
  if ID=='npc_villager_f':p.x*=.95
  elif ID=='npc_elder':p.x*=.96
  ids.append(len(hv));hv.append(tuple(p));hu.append((uv[0],uv[1]*.5))
 hf.append(tuple(ids))
hm=bpy.data.meshes.new('FittedProjectHead');hm.from_pydata(hv,[],hf);hm.update();ho=bpy.data.objects.new('FittedProjectHead',hm);scene.collection.objects.link(ho);hm.materials.append(mat)
uvlayer=hm.uv_layers.new(name='UVMap')
for f in hm.polygons:
 f.use_smooth=True
 for li in f.loop_indices:uvlayer.data[li].uv=hu[hm.loops[li].vertex_index]
g=ho.vertex_groups.new(name='head');g.add(list(range(len(hv))),1,'REPLACE');parts.append(ho);part_tags[ho.name]='Fitted project S310 head, source-role palette'
# Layered long/medium hair remains distinct from coat fabric, without cloth simulation.
ellipsoid('HairCrown',(0,4*K,H-8*K),(8.9*K,7*K,9.5*K),4,'head',24,12)
length=35 if cfg['hair']=='long' else 18 if cfg['hair']=='medium' else 8
for i,angle in enumerate(np.linspace(-1.43,1.43,9 if cfg['hair']=='long' else 7)):
 pts=[]
 for j in range(9):
  t=j/8; r=(9.2+1.5*math.sin(t*math.pi))*K
  pts.append(Vector((math.sin(angle)*r,4*K+math.cos(angle)*r*.78,H-(6+length*t)*K)))
 tube('HairLock'+str(i),pts,[(1.6*(1-.55*(j/8)**3))*K for j in range(9)],4,fixed('head'),n=8,flat=.48,creases=.015)
if cfg['hair'] in ('long','medium'):
 # A continuous back sheet closes the gaps between the sculpted locks.
 vs=[];fs=[];us=[];ws=[];nr=12;nc=32
 for j in range(nr+1):
  t=j/nr
  for i in range(nc+1):
   a=-1.62+3.24*i/nc;r=(8.8+.7*math.sin(t*math.pi))*K
   z=H-(7+length*t)*K + (.65*math.sin(a*7)*K*t*t)
   p=Vector((math.sin(a)*r,4*K+math.cos(a)*r*.72,z))
   vs.append(tuple(p));us.append((i/nc,t));ws.append({'head':1})
 for j in range(nr):
  for i in range(nc):a=j*(nc+1)+i;fs.append((a,a+1,a+nc+2,a+nc+1))
 ob=mesh('ContinuousHairBack',vs,fs,us,ws,4)
 bpy.context.view_layer.objects.active=ob;ob.select_set(True)
 so=ob.modifiers.new('HairBody','SOLIDIFY');so.thickness=.6*K;so.offset=0;bpy.ops.object.modifier_apply(modifier=so.name);ob.select_set(False)
if cfg['hair']=='long':
 for sign in (-1,1):
  pts=[Vector((sign*(8.6-.6*t)*K,(-2.6+2.8*t)*K,H-(12+24*t)*K)) for t in np.linspace(0,1,9)]
  tube('FaceFrame'+str(sign),pts,[1.15*K*(1-.5*t) for t in np.linspace(0,1,9)],4,fixed('head'),n=8,flat=.65)

def garment_panel(name,angle,top,bottom,tile,width=.54):
 v=[];u=[];w=[];f=[]; rows=11; cols=9
 for j in range(rows):
  t=j/(rows-1); z=top+(bottom-top)*t
  for i in range(cols):
   q=i/(cols-1); theta=angle+(q-.5)*width
   r=(18.5+4.2*t+1.05*math.cos(q*math.tau*2)*math.sin(t*math.pi))*K
   p=Vector((math.sin(theta)*r*cfg['width'],3*K+math.cos(theta)*r*.68,z+.45*K*math.sin(q*math.tau)))
   v.append(tuple(p));u.append((q,t))
   w.append(torso_weight(t,p))
 for j in range(rows-1):
  for i in range(cols-1):a=j*cols+i;f.append((a,a+cols,a+cols+1,a+1))
 ob=mesh(name,v,f,u,w,tile);bpy.context.view_layer.objects.active=ob;ob.select_set(True)
 so=ob.modifiers.new('SewnFabricThickness','SOLIDIFY');so.thickness=.36*K;so.offset=0;bpy.ops.object.modifier_apply(modifier=so.name);ob.select_set(False)
 return ob

# Open coat over linen, broad fold geometry at shoulders and waist; no armor on townsfolk.
for i,a in enumerate(np.linspace(-2.55,2.55,10)):
 garment_panel('CoatFold'+str(i),a,H*.795,H*(.48 if ID!='npc_child' else .51),0,width=.59)
if ID in ('npc_villager_f','npc_elder','npc_scholar'):
 hem={'npc_villager_f':.23,'npc_elder':.15,'npc_scholar':.18}[ID]
 tile=2 if ID=='npc_villager_f' else 0
 # Continuous wrap with a fixed waistband and weights approaching zero at center seams.
 # Discontinuous leg choice caused the first independent walking check to fail.
 vs=[];fs=[];us=[];ws=[];nr=18;nc=96
 for j in range(nr+1):
  t=j/nr;z=H*(.55+ (hem-.55)*t)
  for i in range(nc+1):
   a=math.tau*i/nc;r=(22.7+6.3*t+ .9*math.cos(a*12)*math.sin(math.pi*t))*K
   p=Vector((math.sin(a)*r*cfg['width'],3*K+math.cos(a)*r*.76,z+.35*K*math.sin(a*12)*t*t))
   vs.append(tuple(p));us.append((i/nc*4%1,t))
   side='l' if p.x>=0 else 'r';weight=.9*t*t*min(abs(p.x)/(13*K),1)**2
   ws.append({'pelvis':1-weight,'thigh_'+side:weight})
 for j in range(nr):
  for i in range(nc):a=j*(nc+1)+i;fs.append((a,a+1,a+nc+2,a+nc+1))
 ob=mesh('ContinuousPleatedDress',vs,fs,us,ws,tile)
 bpy.context.view_layer.objects.active=ob;ob.select_set(True)
 so=ob.modifiers.new('DressThickness','SOLIDIFY');so.thickness=.36*K;so.offset=0;bpy.ops.object.modifier_apply(modifier=so.name);ob.select_set(False)
# Soft collar, a worn leather belt and discrete fastening details.
pts=[Vector((math.sin(t)*6.3*K,3*K+math.cos(t)*5.6*K,H*.817)) for t in np.linspace(0,math.tau,41)]
tube('TurnedCollar',pts,[1.25*K]*len(pts),2,fixed('spine_03'),n=8,tip=False)
pts=[Vector((math.sin(t)*18.3*K*cfg['width'],3*K+math.cos(t)*11.6*K,H*.545)) for t in np.linspace(0,math.tau,65)]
tube('LeatherBelt',pts,[1.0*K]*len(pts),1,fixed('pelvis'),n=6,flat=1.5,tip=False)
if ID in ('npc_scholar','npc_elder'):
 ellipsoid('BrassBrooch',(0,-13.3*K,H*.735),(2.4*K,.8*K,2.8*K),5,'spine_03',20,10)
 ellipsoid('BroochInset',(0,-14.0*K,H*.735),(1.4*K,.25*K,1.7*K),6,'spine_03',16,8)
else:
 for i in range(3):ellipsoid('CoatButton'+str(i),(0,-12.4*K,H*(.69-.04*i)),(.5*K,.5*K,.5*K),5,'spine_02',10,6)
if ID=='npc_fisherman':
 ellipsoid('NetBag',(-24*K,5*K,H*.49),(8*K,5.5*K,11*K),6,'pelvis',20,12)
 for turn in range(4):
  pts=[Vector((-22*K+math.sin(a)*7.1*K, -2.5*K+turn*.9*K, H*.51+math.cos(a)*7.1*K)) for a in np.linspace(0,math.tau,49)]
  tube('CoiledLine'+str(turn),pts,[.48*K]*len(pts),6,fixed('pelvis'),n=6,tip=False,creases=0)
# Final single skinned material/mesh, recalculated normals and exactly two normalized influences.
bpy.ops.object.select_all(action='DESELECT')
for ob in parts:ob.select_set(True)
body=parts[0];bpy.context.view_layer.objects.active=body;bpy.ops.object.join();body.name=ID+'_Body';body.data.materials.clear();body.data.materials.append(mat)
for f in body.data.polygons:f.material_index=0
bm=bmesh.new();bm.from_mesh(body.data);bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=.0001);bmesh.ops.dissolve_degenerate(bm,edges=list(bm.edges),dist=.00001);bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(body.data);bm.free()
tr=body.modifiers.new('Triangles','TRIANGULATE');bpy.ops.object.modifier_apply(modifier=tr.name)
for v in body.data.vertices:
 groups=sorted(((g.group,g.weight) for g in v.groups if g.weight>1e-7),key=lambda g:g[1],reverse=True)[:2];assert groups
 for g in list(v.groups):body.vertex_groups[g.group].remove([v.index])
 total=sum(w for i,w in groups)
 for i,w in groups:body.vertex_groups[i].add([v.index],w/total,'REPLACE')
coords=np.array([v.co[:] for v in body.data.vertices]);minimum=float(coords[:,2].min());scale=H/float(np.ptp(coords[:,2]))
for v in body.data.vertices:v.co.z-=minimum;v.co*=scale
bpy.context.view_layer.objects.active=rig;bpy.ops.object.mode_set(mode='EDIT')
for b in rig.data.edit_bones:b.head*=scale;b.tail*=scale
bpy.ops.object.mode_set(mode='OBJECT');bpy.context.view_layer.objects.active=body
body.parent=rig;body.matrix_parent_inverse=rig.matrix_world.inverted();skin=body.modifiers.new('Skin','ARMATURE');skin.object=rig;skin.use_deform_preserve_volume=False
body.data.calc_loop_triangles();assert len(body.data.loop_triangles)<=25000
assert {b.name:b.parent.name if b.parent else None for b in rig.data.bones}==expected_parents
bpy.ops.object.select_all(action='DESELECT');rig.select_set(True);body.select_set(True);bpy.context.view_layer.objects.active=rig
bpy.ops.export_scene.fbx(filepath=str(OUT/f'{ID}_rigged.fbx'),use_selection=True,object_types={'MESH','ARMATURE'},axis_forward='-Y',axis_up='Z',apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',add_leaf_bones=False,use_armature_deform_only=True,path_mode='RELATIVE',embed_textures=False,bake_anim=False,mesh_smooth_type='FACE')
bpy.ops.wm.save_as_mainfile(filepath=str(WORK/f'{ID}_authored.blend'))
report=dict(id=ID,height_cm=H,triangles=len(body.data.loop_triangles),vertices=len(body.data.vertices),bone_count=len(rig.data.bones),max_influences=max(len(v.groups) for v in body.data.vertices),material_count=1,origin='between the reference feet at ground',hierarchy=expected_parents,parts=part_tags,authoring='locally authored geometry and deterministic atlas; fitted project S310 head and rig; no AI inference',rig_source_sha256=hashlib.sha256(rig_source.read_bytes()).hexdigest(),head_texture_source_sha256=hashlib.sha256((ROOT/'arrel/arrel_basecolor.png').read_bytes()).hexdigest(),source_palette=cfg)
(WORK/'build_report.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
print('S347_MODEL_BUILD_PASS',json.dumps({k:report[k] for k in ('id','height_cm','triangles','vertices','bone_count','max_influences')}),flush=True)
