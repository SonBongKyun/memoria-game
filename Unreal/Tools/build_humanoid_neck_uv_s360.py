"""S360 R10: identical R9 neck geometry with explicit normalized bridge UVs."""
from pathlib import Path
import importlib.util,json,math
ROOT=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location('neck_source',ROOT/'Unreal/Tools/build_humanoid_necks_actions_s360.py');n=importlib.util.module_from_spec(spec);spec.loader.exec_module(n)
n.OUT=ROOT/'Unreal/ArtSource/HumanoidAnatomyS360/R10'
original_mesh=n.m.mesh
def mesh(name,*args,**kwargs):
 ob=original_mesh(name,*args,**kwargs)
 if name=='ContinuousMixedWeightNeck':
  for layer in ob.data.uv_layers:
   for loop in ob.data.loops:
    row,column=divmod(loop.vertex_index,49);layer.data[loop.index].uv=(column/48,row/12)
 return ob
n.m.mesh=mesh
original_export=n.m.export
def export(path,*args,**kwargs):
 import bpy
 for ob in bpy.context.selected_objects:
  if ob.type=='MESH':
   for layer in ob.data.uv_layers:
    assert all(math.isfinite(x) and -.001<=x<=1.001 for face in ob.data.polygons if face.material_index==2 for li in face.loop_indices for x in layer.data[li].uv),'Out-of-domain face UV before export'
 return original_export(path,*args,**kwargs)
n.m.export=export
if __name__=='__main__':
 n.OUT.mkdir(parents=True,exist_ok=False)
 for name in ['AshWalker','BeltScavenger']:
  n.neck(name);p=n.OUT/name/'geometry.json';r=json.loads(p.read_text(encoding='utf-8'))
  r['reused_neck_generator_sha256']=r['generator_sha256'];r['generator_sha256']=n.b.sha(Path(__file__));r['uv_policy']='All added bridge UV layers explicitly parameterized on [0,1]; original R8 face/body UVs retained.'
  p.write_text(json.dumps(r,indent=2)+'\n',encoding='utf-8')
 (n.OUT/'source_inventory.json').write_text(json.dumps({'revision':'R10','generator_sha256':n.b.sha(Path(__file__)),'files':[{'path':str(p.relative_to(n.OUT)),'sha256':n.b.sha(p)} for p in sorted(n.OUT.rglob('*')) if p.is_file()]},indent=2)+'\n',encoding='utf-8')
 print('S360_NECK_UV_AUTHORING_COMPLETE',flush=True)
