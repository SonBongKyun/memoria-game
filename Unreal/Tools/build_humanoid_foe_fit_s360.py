"""S360 R8 mesh fitting repair; R7 NPC source and original sources stay immutable."""
from pathlib import Path
import importlib.util,bpy,bmesh,sys,json,hashlib
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location('base_s360',ROOT/'Unreal/Tools/build_humanoid_anatomy_s360.py');b=importlib.util.module_from_spec(spec);spec.loader.exec_module(b)
m=b.m;oldprepare=b.prepare;b.OUT=ROOT/'Unreal/ArtSource/HumanoidAnatomyS360/R8'
def prepare(name):
 result=oldprepare(name);src,old,human,uv,groups,j,orig,corrected,mouth=result;K=m.K;H=m.H
 excess=max(v.co.z for v in old.data.vertices)-H
 for v in old.data.vertices:
  w=sum(g.weight for g in v.groups if old.vertex_groups[g.group].name=='head');v.co.z-=excess*w
 # Replace egg-like retained feet with a flat sole, heel, instep and ankle-fitting boot.
 bm=bmesh.new();bm.from_mesh(old.data);bmesh.ops.delete(bm,geom=[v for v in bm.verts if v.co.z<13*K],context='VERTS');bm.to_mesh(old.data);bm.free()
 m.MAT=old.data.materials[1];m.PARTS=[]
 for side in ['l','r']:
  ank=j['foot_'+side];knee=j['calf_'+side]
  m.tube('FittedFoeBootUpper_'+side,[knee.lerp(ank,.58),ank+Vector((0,0,3*K)),ank],[5.5*K,4.5*K,4.3*K],m.fixed('calf_'+side),(.08,.046,.026),.5,28,.95,.01)
  vs=[];fs=[];n=40
  for z,rx,front,back in [(0,4.8,-17,5),(2,4.9,-17,5),(5,4.6,-15.8,4.8),(9,4.3,-11,4.4),(14,4.2,-3.8,4.4),(18,4.1,-3.8,4.4)]:
   for i in range(n+1):
    a=b.math.tau*i/n;vs.append((ank.x+b.math.sin(a)*rx*K,ank.y+(front+back)*K/2-b.math.cos(a)*(back-front)*K/2,z*K))
  for r in range(5):
   for i in range(n):a=r*(n+1)+i;fs.append((a,a+1,a+n+2,a+n+1))
  fs.append(tuple(range(n,-1,-1)));m.mesh('FoeBootLast_'+side,vs,fs,m.fixed('foot_'+side),(.06,.035,.020),.5)
 parts=[old]+m.PARTS
 for ob in parts:
  me=ob.data;uvs={u.name:[tuple(v.uv) for v in u.data] for u in me.uv_layers};layer=me.color_attributes.get('Color');cs=[tuple(v.color) for v in layer.data] if layer else [(1,1,1,0)]*len(me.loops)
  while len(me.color_attributes):me.color_attributes.remove(me.color_attributes[0])
  me.color_attributes.new(name='Color',type='FLOAT_COLOR',domain='CORNER')
  for v,c in zip(me.color_attributes[0].data,cs):v.color=c
  for n,values in uvs.items():
   for v,u in zip(me.uv_layers[n].data,values):v.uv=u
 bpy.ops.object.select_all(action='DESELECT')
 for o in parts:o.select_set(True)
 bpy.context.view_layer.objects.active=old;bpy.ops.object.join()
 return result
b.prepare=prepare
if __name__=='__main__':
 for name in ['AshWalker','BeltScavenger']:
  b.build(name);m.load=b.motionload;m.OUT=b.OUT;m.animate(name)
  p=b.OUT/name/'geometry.json';r=json.loads(p.read_text());r['fit_repair_generator_sha256']=b.sha(Path(__file__));r['reused_anatomy_generator_sha256']=b.sha(ROOT/'Unreal/Tools/build_humanoid_anatomy_s360.py');p.write_text(json.dumps(r,indent=2)+'\n')
 (b.OUT/'source_inventory.json').write_text(json.dumps({'revision':'R8','generator_sha256':b.sha(Path(__file__)),'files':[{'path':str(p.relative_to(b.OUT)),'sha256':b.sha(p)} for p in sorted(b.OUT.rglob('*')) if p.is_file() and p.name!='source_inventory.json']},indent=2)+'\n');print('S360_FOE_REPAIR_AUTHORING_COMPLETE',flush=True)
