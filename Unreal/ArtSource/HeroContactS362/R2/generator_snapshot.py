"""S362 R2 additive sources for native MeshDescription preservation.

Export new blade/collars only. StaffRegions is an unchanged original-geometry
classification reference, never a replacement for the native painted staff.
CPU Blender only; no images, armatures, animation or accepted asset edits.
"""
from collections import Counter
from pathlib import Path
import argparse
import hashlib
import importlib.util
import json
import math
import sys
import bpy
from mathutils import Vector


def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def save_json(path,value):path.write_text(json.dumps(value,indent=2)+'\n',encoding='utf8')
def load_module(name,path):
    spec=importlib.util.spec_from_file_location(name,path)
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);return module
def bounds(points):return [[min(p[i] for p in points),max(p[i] for p in points)] for i in range(3)]
def signature(s,ob):
    me=ob.data;names=[u.name for u in me.uv_layers];pigment=s.colors(me);rows=Counter()
    for f in me.polygons:
        for li in f.loop_indices:
            p=ob.matrix_world@me.vertices[me.loops[li].vertex_index].co
            rows[(tuple(round(float(v),6) for v in p),
                  tuple(tuple(round(float(v),6) for v in me.uv_layers[n].data[li].uv) for n in names),
                  tuple(round(float(v),6) for v in pigment[li]))]+=1
    return names,rows
def require_same(before,after):
    assert before[0]==after[0],('UV channels changed',before[0],after[0])
    retained=sum((before[1]&after[1]).values());total=sum(before[1].values())
    assert retained==total,('Position/allUV/RGBA corners changed',retained,total)
    return {'retained_corners':retained,'source_corners':total,'all_UVs_RGBA_position_ratio':retained/total,'UV_channels':before[0]}
def import_single(path):
    bpy.ops.wm.read_factory_settings(use_empty=True);bpy.ops.import_scene.fbx(filepath=str(path))
    meshes=[ob for ob in bpy.data.objects if ob.type=='MESH'];assert len(meshes)==1
    return meshes[0]
def sole_material(ob,material):
    ob.data.materials.clear();ob.data.materials.append(material)
    for face in ob.data.polygons:face.material_index=0
def material_report(ob):
    result=[]
    for slot,material in enumerate(ob.data.materials):
        faces=[f for f in ob.data.polygons if f.material_index==slot]
        points=[ob.matrix_world@ob.data.vertices[v].co for f in faces for v in f.vertices]
        result.append({'slot':slot,'name':material.name,'triangles':sum(len(f.vertices)-2 for f in faces),
                       'bounds_m':bounds(points) if points else None})
    return result
def export_roundtrip(s,r1,parts,folder,name,expected,source_signature=None):
    folder.mkdir();destination=folder/(name+'.fbx')
    body=r1.finish_export(s,parts,destination)
    assert len(body.data.polygons)==expected,(name,len(body.data.polygons),expected)
    before=signature(s,body);materials=material_report(body)
    origin=list(body.matrix_world.translation)
    envelope=bounds([body.matrix_world@v.co for v in body.data.vertices])
    authored_source=require_same(source_signature,before) if source_signature is not None else None
    body=import_single(destination)
    assert len(body.data.polygons)==expected
    assert [m['name'] for m in material_report(body)]==[m['name'] for m in materials]
    roundtrip=require_same(before,signature(s,body))
    assert (Vector(origin)-body.matrix_world.translation).length<1e-7
    envelope_error=r1.assert_bounds(envelope,bounds([body.matrix_world@v.co for v in body.data.vertices]))
    report={'schema':2,'session':'S362','revision':'R2','name':name,'triangles':expected,
            'materials':materials,'origin_world_m':origin,'bounds_m':envelope,
            'authored_original_geometry':authored_source,'roundtrip_position_UVs_RGBA':roundtrip,
            'roundtrip_envelope':envelope_error,'output_sha256':sha(destination),
            'purpose':'Classification reference only; do not replace native painted geometry.' if source_signature is not None else 'Append new geometry only to a copy of original native MeshDescription.'}
    save_json(folder/'geometry.json',report);print('S362_NATIVE_ADDITIVE_COMPLETE',name,expected,flush=True)
    return report


def main():
    parser=argparse.ArgumentParser();parser.add_argument('--project-root',type=Path,required=True)
    parser.add_argument('--output-root',type=Path);parser.add_argument('--revision',default='R2')
    args=parser.parse_args(sys.argv[sys.argv.index('--')+1:]);assert args.revision=='R2'
    root=args.project_root.resolve();out=args.output_root or root/'Unreal/ArtSource/HeroContactS362/R2'
    assert not out.exists(),('Immutable R2 source directory exists',out)
    helper=root/'Unreal/ArtSource/HeroPolishS361/R4/generator_snapshot.py'
    r1folder=root/'Unreal/ArtSource/HeroContactS362/R1';r1path=r1folder/'generator_snapshot.py'
    sword=Path(r'C:/Users/jc/Documents/Codex/MEMORIA-Unreal-Collaboration/models/_raw/s312_sword/arrel_prop_sword_drawn.fbx')
    staff=root/'Unreal/ArtSource/FieldCharacters/elia/elia_prop_staff.fbx'
    protected=[helper,sword,staff,*sorted(p for p in r1folder.rglob('*') if p.is_file())]
    frozen={str(p):sha(p) for p in protected}
    s=load_module('accepted_s361',helper);r1=load_module('frozen_prop_r1',r1path)
    out.mkdir(parents=True,exist_ok=False);snapshot=out/'generator_snapshot.py';snapshot.write_bytes(Path(__file__).read_bytes())
    reports=[]
    body=import_single(sword);parts,detail=r1.replace_steel(s,body);blade=parts[1]
    blade_material=blade.data.materials[1];sole_material(blade,blade_material)
    reports.append(export_roundtrip(s,r1,[blade],out/'ArrelBlade','ArrelBlade',158))

    body=import_single(staff);before=signature(s,body);me=body.data;regions=s.connected_regions(body,material=0)
    gold=[];crystal=[];gold_uv=None
    for region in regions:
        bb=bounds([body.matrix_world@me.vertices[i].co for i in region['vertices']]);size=[b-a for a,b in bb]
        if region['unique_vertices']==12 and region['triangles']==20 and .15<size[2]<.18 and .11<size[0]<.13:
            crystal.extend(region['polygons'])
        elif (region['triangles']==384 or (region['triangles']==44 and size[2]<.04)
              or (region['triangles']==20 and region['unique_vertices']==12)):
            gold.extend(region['polygons'])
            if gold_uv is None:gold_uv=tuple(me.uv_layers[0].data[me.polygons[region['polygons'][0]].loop_start].uv)
    assert sum(len(me.polygons[i].vertices)-2 for i in gold)==728 and len(crystal)==20 and gold_uv is not None
    for name in ('S362_StaffMetal','S362_StaffCrystal'):
        material=bpy.data.materials.new(name);material.use_nodes=True;me.materials.append(material)
    for i in gold:me.polygons[i].material_index=1
    for i in crystal:me.polygons[i].material_index=2
    reports.append(export_roundtrip(s,r1,[body],out/'EliaRegions','EliaRegions',836,before))

    # Reimport the untouched source to avoid inheriting a reference-export basis.
    body=import_single(staff);material=bpy.data.materials.new('S362_StaffMetal');material.use_nodes=True
    body.data.materials.append(material);parts=[]
    for band,z in enumerate((-.40,.22)):
        rows=[(z-.009,.0140),(z-.007,.0160),(z+.007,.0160),(z+.009,.0140)]
        vertices=[];faces=[];n=48
        for height,radius in rows:
            for i in range(n):
                a=math.tau*i/n;vertices.append(Vector((math.cos(a)*radius,math.sin(a)*radius,height)))
        for row in range(len(rows)-1):
            for i in range(n):a=row*n+i;b=row*n+(i+1)%n;faces.append((a,b,b+n,a+n))
        faces.append(tuple(range(n-1,-1,-1)));faces.append(tuple(range((len(rows)-1)*n,len(rows)*n)))
        part=r1.create_part(body,'Elia_FittedGoldCollar_%d'%band,vertices,faces,1,[gold_uv]*len(vertices),True)
        sole_material(part,material);parts.append(part)
    reports.append(export_roundtrip(s,r1,parts,out/'EliaCollars','EliaCollars',760))
    assert frozen=={str(p):sha(p) for p in protected},'Protected original or R1 file changed'
    inventory={'schema':2,'session':'S362','revision':'R2','generator_sha256':sha(snapshot),
               'protected_files_sha256':frozen,'assets':[{'name':r['name'],'triangles':r['triangles'],'output_sha256':r['output_sha256']} for r in reports],
               'native_contract':'Duplicate original native MeshDescription/build settings. Keep all native painted hilt/staff attributes. Replace only steel. Append only blade/collars. StaffRegions only classifies material groups; original native crystal geometry retained.'}
    save_json(out/'source_inventory.json',inventory)
    print('S362_NATIVE_ADDITIVE_AUTHORING_COMPLETE R2 meshes=3 replacement_hilts=0 replacement_staffs=0 images=0 rigs=0 clips=0',flush=True)


if __name__=='__main__':main()
