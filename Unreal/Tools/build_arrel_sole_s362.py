"""Immutable Arrel-only R3 sole revision; run in Blender CPU with -t2.
Copies R2 FBX object data exactly, changes20sole weights and4z scalars only.
No scene export/rebuild, images, normals, UVs, material, bind or action changes.
"""
from pathlib import Path
import argparse,array,copy,hashlib,json,math,struct,sys
import bpy
from io_scene_fbx import parse_fbx,encode_bin,data_types

SOLE={'l':[329,4492,4504,4964,4981,4993,5331,5667,5671,5680],
      'r':[6573,10722,10734,11185,11202,11214,11549,11882,11886,11895]}
FACET={'l':[4964,4981,5671],'r':[11185,11202,11886]}
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def child(e,id):return next((c for c in e.elems if c.id==id),None)
def raw_name(e):return e.props[1].split(b'\x00\x01')[0].decode()
def bits(v):return struct.pack('<f',v).hex()
def semantic(e,skip=frozenset(),path=()):
 path=path+(e.id.decode(errors='replace')+(':'+str(e.props[0]) if e.id in (b'Geometry',b'Deformer',b'Model') and e.props else ''),)
 if path in skip:return b'EXPLICIT_AUTHORIZED_ARRAY'
 h=hashlib.sha256();h.update(e.id);h.update(bytes(e.props_type))
 for t,v in zip(e.props_type,e.props):
  if isinstance(v,array.array):h.update(v.tobytes())
  elif isinstance(v,bytes):h.update(struct.pack('<Q',len(v)));h.update(v)
  elif isinstance(v,float):h.update(struct.pack('<d',v))
  else:h.update(str(v).encode())
 for c in e.elems:h.update(semantic(c,skip,path))
 return h.digest()
def encoded(e):
 n=encode_bin.FBXElem(e.id)
 methods={data_types.BOOL:'add_bool',data_types.CHAR:'add_char',data_types.INT8:'add_int8',data_types.INT16:'add_int16',data_types.INT32:'add_int32',data_types.INT64:'add_int64',data_types.FLOAT32:'add_float32',data_types.FLOAT64:'add_float64',data_types.BYTES:'add_bytes',data_types.STRING:'add_string',data_types.INT32_ARRAY:'add_int32_array',data_types.INT64_ARRAY:'add_int64_array',data_types.FLOAT32_ARRAY:'add_float32_array',data_types.FLOAT64_ARRAY:'add_float64_array',data_types.BOOL_ARRAY:'add_bool_array',data_types.BYTE_ARRAY:'add_byte_array'}
 for t,v in zip(e.props_type,e.props):getattr(n,methods[t])(v)
 n.elems=[encoded(c) for c in e.elems];return n

def scene_snapshot(path):
 bpy.ops.wm.read_factory_settings(use_empty=True);bpy.ops.import_scene.fbx(filepath=str(path),colors_type='LINEAR',use_anim=False)
 meshes=[o for o in bpy.data.objects if o.type=='MESH'];rigs=[o for o in bpy.data.objects if o.type=='ARMATURE'];assert len(meshes)==len(rigs)==1
 ob,rig=meshes[0],rigs[0];me=ob.data;assert me.shape_keys is None;assert len(rig.data.bones)==77
 positions=[tuple(v.co)for v in me.vertices]
 weights=[tuple(sorted((ob.vertex_groups[g.group].name,g.weight)for g in v.groups if g.weight>0))for v in me.vertices]
 normals=[tuple(n.vector)for n in me.corner_normals]
 uv=[(u.name,[tuple(v.uv)for v in u.data])for u in me.uv_layers]
 color=[(c.name,c.domain,c.data_type,[tuple(v.color)for v in c.data])for c in me.color_attributes]
 bones=[(n.name,n.parent.name if n.parent else None,tuple(tuple(r)for r in n.matrix_local))for n in rig.data.bones]
 return dict(positions=positions,weights=weights,normals=normals,UV=uv,RGBA=color,bones=bones,materials=[m.name for m in me.materials],polygons=[(tuple(f.vertices),f.material_index)for f in me.polygons],mesh_world=tuple(tuple(r)for r in ob.matrix_world),rig_world=tuple(tuple(r)for r in rig.matrix_world),triangle_count=len(me.polygons))

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--project-root',type=Path,required=True);ap.add_argument('--output-root',type=Path,required=True);ap.add_argument('--verify-existing',action='store_true');args=ap.parse_args(sys.argv[sys.argv.index('--')+1:])
 project=args.project_root.resolve();out=args.output_root.resolve();assert not out.exists() or (args.verify_existing and (out/'Arrel/Arrel.fbx').is_file() and not (out/'Arrel/geometry.json').exists()),('Immutable output exists',out)
 source=project/'Unreal/ArtSource/HeroFinishS362/R2/Arrel/Arrel.fbx';source_hash=sha(source);source_tree,version=parse_fbx.parse(str(source));tree=copy.deepcopy(source_tree)
 objects=child(tree,b'Objects');original_objects=child(source_tree,b'Objects');geometries=[e for e in objects.elems if e.id==b'Geometry' and e.props[2]==b'Mesh'];assert len(geometries)==1
 geometry=geometries[0];vertices=child(geometry,b'Vertices').props[0];assert len(vertices)//3==17005
 connections=child(tree,b'Connections');bone_for_cluster={}
 models={e.props[0]:raw_name(e)for e in objects.elems if e.id==b'Model'}
 clusters={e.props[0]:e for e in objects.elems if e.id==b'Deformer' and e.props[2]==b'Cluster'}
 for c in connections.elems:
  if c.id==b'C' and len(c.props)>=3 and c.props[0]==b'OO' and c.props[1]in models and c.props[2]in clusters:bone_for_cluster[c.props[2]]=models[c.props[1]]
 bybone={bone_for_cluster[k]:v for k,v in clusters.items()};assert all(n in bybone for s in SOLE for n in ('calf_'+s,'foot_'+s,'ball_'+s))
 original_snapshot=scene_snapshot(source);allowed={i for ids in SOLE.values()for i in ids};changes=[];position_changes=[];skip=set()
 for side,ids in SOLE.items():
  clusters_for_side={n:bybone[n+'_'+side]for n in ('calf','foot','ball')};rows={n:dict(zip(child(c,b'Indexes').props[0],child(c,b'Weights').props[0]))for n,c in clusters_for_side.items()}
  minimum=min(vertices[i*3+2]for i in ids)
  for i in ids:
   p=list(vertices[i*3:i*3+3]);assert max(abs(a-b)for a,b in zip(p,original_snapshot['positions'][i]))<1e-5,('FBXcontrolpoint/BpyID mismatch',i,p,original_snapshot['positions'][i])
   old={n+'_'+side:rows[n][i]for n in rows};assert .35<old['calf_'+side]<.43
   denominator=rows['foot'][i]+rows['ball'][i];assert denominator>.5
   expected={'foot_'+side:rows['foot'][i]/denominator,'ball_'+side:rows['ball'][i]/denominator}
   rows['calf'].pop(i);rows['foot'][i]=expected['foot_'+side];rows['ball'][i]=expected['ball_'+side]
   changes.append({'source_ID':i,'side':side,'source_position_cm':p,'old_weights':old,'expected_weights':expected})
   if i in FACET[side] and vertices[i*3+2]!=minimum:
    delta=minimum-vertices[i*3+2];assert -.18028<=delta<0;vertices[i*3+2]=minimum
    position_changes.append({'source_ID':i,'old_source_position_cm':p,'new_source_position_cm':list(vertices[i*3:i*3+3]),'z_delta_source_cm':delta})
  for n,c in clusters_for_side.items():
   old_indexes=list(child(c,b'Indexes').props[0]);keys=[i for i in old_indexes if i in rows[n]]
   child(c,b'Indexes').props[0]=array.array(child(c,b'Indexes').props[0].typecode,keys)
   child(c,b'Weights').props[0]=array.array(child(c,b'Weights').props[0].typecode,[rows[n][i]for i in keys])
   for field in ('Indexes','Weights'):skip.add(('Objects','Deformer:'+str(c.props[0]),field))
 skip.add(('Objects','Geometry:'+str(geometry.props[0]),'Vertices'))
 assert semantic(original_objects,skip)==semantic(objects,skip), 'Unapproved FBX object property changed'
 old_vertices=child(next(e for e in original_objects.elems if e.id==b'Geometry'),b'Vertices').props[0]
 changed_scalars=[i for i,(a,b)in enumerate(zip(old_vertices,vertices))if a!=b];assert len(changed_scalars)==4 and all(i%3==2 and i//3 in allowed for i in changed_scalars)
 assert len(changes)==20 and len(position_changes)==4
 out.mkdir(parents=True,exist_ok=args.verify_existing);folder=out/'Arrel';folder.mkdir(exist_ok=args.verify_existing);destination=folder/'Arrel.fbx'
 if not args.verify_existing:encode_bin.write(str(destination),encoded(tree),version)
 backtree,backversion=parse_fbx.parse(str(destination));assert backversion==version
 assert semantic(child(backtree,b'Objects'))==semantic(objects),'Encoded FBX roundtrip object data changed'
 after=scene_snapshot(destination)
 for field in ('UV','RGBA','bones','materials','polygons','mesh_world','rig_world','triangle_count'):assert original_snapshot[field]==after[field],('Exact roundtrip preservation failed',field)
 decoded_normal_changes=[i for i,(a,b)in enumerate(zip(original_snapshot['normals'],after['normals']))if a!=b]
 loop_vertices=[v for f,mat in original_snapshot['polygons']for v in f]
 assert all(original_snapshot['positions'][loop_vertices[i]][2]<1. for i in decoded_normal_changes),'Blender decoded-normal change outside exact sole-neighbor cache'
 maximum_decoded_normal_error=max((math.dist(original_snapshot['normals'][i],after['normals'][i])for i in decoded_normal_changes),default=0)
 # Host custom-normal packing changes at moved faces; encoded FBX normal
 # arrays are bit-exact by whole Objects semantic proof above. Native output
 # reuses accepted R2 corner attributes verbatim, so no normal guard weakens.
 actual_p=[i for i,(a,b)in enumerate(zip(original_snapshot['positions'],after['positions']))if a!=b];assert set(actual_p)=={r['source_ID']for r in position_changes}
 actual_w=[i for i,(a,b)in enumerate(zip(original_snapshot['weights'],after['weights']))if a!=b];assert set(actual_w)==allowed
 for row in changes:
  actual=dict(after['weights'][row['source_ID']]);assert set(actual)==set(row['expected_weights']);assert max(abs(actual[k]-v)for k,v in row['expected_weights'].items())<6e-8
 assert source_hash==sha(source),'Accepted R2 source was modified'
 receipt={'schema':'s362-arrel-sole-r3-v1','status':'STAGED_NOT_NATIVE_ACCEPTED','source':str(source),'source_sha256':source_hash,'output_sha256':sha(destination),'fbx_version':version,'triangles':after['triangle_count'],'vertices':len(after['positions']),'bones':77,'materials':after['materials'],'weight_exceptions':changes,'position_exceptions':position_changes,'all_other_attributes_exact':['positions outside4','weights outside20','all encoded FBX loop normal arrays (exact)' ,'all UV channels','all RGBA layers','all polygons/material slots','all77bind matrices/names/parents','mesh and rig object matrices'],'unchanged_FBX_object_semantics_except_explicit_arrays_sha256':semantic(objects,skip).hex(),'roundtrip_FBX_object_semantics_sha256':semantic(objects).hex(),'blender_roundtrip_other_attributes_exact':True,'raw_FBX_normals_exact':True,'blender_decoded_normal_cache_diagnostic':{'changed_loops':len(decoded_normal_changes),'maximum_vector_delta':maximum_decoded_normal_error,'all_changed_reference_z_below_cm':1,'native_corners_must_be_copied_bit_exact_from_R2':True},'native_import_required':True,'no_Elia_skeleton_clip_material_texture_authored':True}
 (folder/'geometry.json').write_text(json.dumps(receipt,indent=2)+'\n');(out/'generator_snapshot.py').write_bytes(Path(__file__).read_bytes());(out/'source_inventory.json').write_text(json.dumps({'revision':'R3','source_sha256':source_hash,'body':'Arrel only','body_sha256':sha(destination),'generator_sha256':sha(Path(__file__)),'staged_not_activated':True},indent=2)+'\n')
 print('S362_ARREL_SOLE_R3_SOURCE_PASS',json.dumps({'triangles':after['triangle_count'],'vertices':len(after['positions']),'weights_changed':len(actual_w),'positions_changed':len(actual_p),'all_other_attributes_exact':True,'FBX_sha256':sha(destination)}),flush=True)
if __name__=='__main__':main()