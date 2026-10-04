"""Independent final rest-geometry, normals and A-pose audit; does not edit delivered assets."""
from pathlib import Path
import argparse,hashlib,json,math
import bpy,numpy as np
from build_townsfolk_specs_s347 import HEIGHTS
p=argparse.ArgumentParser();p.add_argument('--models-root',required=True);a=p.parse_args();root=Path(a.models_root).resolve();results=[]
def load(path):
 bpy.ops.wm.read_factory_settings(use_empty=True);bpy.ops.import_scene.fbx(filepath=str(path),use_anim=False)
 assert len(bpy.context.scene.objects)==2
 return next(o for o in bpy.context.scene.objects if o.type=='ARMATURE'),next(o for o in bpy.context.scene.objects if o.type=='MESH')
def direction(r,n):
 b=r.data.bones[n];return (r.matrix_world.to_3x3()@(b.tail_local-b.head_local)).normalized()
ref,_=load(root/'arrel/arrel_rigged.fbx');directions={n:direction(ref,n) for n in ('upperarm_l','upperarm_r','lowerarm_l','lowerarm_r')}
for id,h in HEIGHTS.items():
 r,body=load(root/id/f'{id}_rigged.fbx');me=body.data;me.calc_loop_triangles();v=np.array([body.matrix_world@x.co for x in me.vertices]);tri=np.array([t.vertices[:] for t in me.loop_triangles]);areas=np.linalg.norm(np.cross(v[tri[:,1]]-v[tri[:,0]],v[tri[:,2]]-v[tri[:,0]]),axis=1)*.5
 normals=np.array([x.normal[:] for x in me.vertices]);assert np.isfinite(normals).all() and np.isfinite(v).all();assert int((areas<=1e-12).sum())==0
 assert np.max(np.abs(np.linalg.norm(normals,axis=1)-1))<.0001
 assert body.matrix_world.determinant()>0 and r.matrix_world.determinant()>0
 angle={n:math.degrees(math.acos(max(-1,min(1,direction(r,n).dot(d))))) for n,d in directions.items()};assert max(angle.values())<15,angle
 origin=r.matrix_world.translation;assert origin.length<1e-5
 footmid=(r.matrix_world@r.data.bones['foot_l'].head_local+r.matrix_world@r.data.bones['foot_r'].head_local)*.5;assert abs(footmid.x)<.0001
 record=dict(id=id,triangles=len(tri),zero_area_triangles=0,minimum_triangle_area_m2=float(areas.min()),normals_finite_unit=True,positive_object_basis=True,arm_reference_direction_difference_degrees=angle,rig_object_origin_m=list(origin),foot_midpoint_m=list(footmid),height_cm=float(np.ptp(v[:,2]))*100,fbx_sha256=hashlib.sha256((root/id/f'{id}_rigged.fbx').read_bytes()).hexdigest());assert abs(record['height_cm']-h)<.1;results.append(record)
(root/'_qa/s347/geometry_audit.json').write_text(json.dumps(dict(status='PASS',models=results),indent=2),encoding='utf-8');print('S347_GEOMETRY_AUDIT_PASS',len(results),flush=True)
