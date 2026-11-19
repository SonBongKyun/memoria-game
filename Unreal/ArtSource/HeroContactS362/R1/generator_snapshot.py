"""Immutable S362 sword/staff finish. Blender4.5 CPU, no image editing.

Keep the original hilt/UVs, prop origins, length and outer bounds. Replace only
the source's14triangle steel blade and20triangle crystal component. Staff gold
is given a separate native material while retaining its original paint/UVs.
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
import bmesh
from mathutils import Vector


def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def write_json(path,value):path.write_text(json.dumps(value,indent=2)+'\n',encoding='utf8')
def bounds(points):return [[min(p[i] for p in points),max(p[i] for p in points)] for i in range(3)]
def assert_bounds(original,current,tolerance=2e-6):
    error=max(abs(a-b) for aa,bb in zip(original,current) for a,b in zip(aa,bb))
    assert error<tolerance,('Prop origin/length/outer envelope changed',original,current,error)
    return {'maximum_world_bound_error_m':error,'tolerance_m':tolerance,'original':original,'current':current}


def signature(ob,materials=None):
    me=ob.data; result=Counter(); names=[u.name for u in me.uv_layers]
    for face in me.polygons:
        if materials is not None and face.material_index not in materials:continue
        for li in face.loop_indices:
            p=ob.matrix_world@me.vertices[me.loops[li].vertex_index].co
            result[(tuple(round(a,6) for a in p),tuple(tuple(round(a,6) for a in me.uv_layers[n].data[li].uv) for n in names))]+=1
    return names,result


def retained(before,after):
    assert before[0]==after[0],('Original UV channel order changed',before[0],after[0])
    total=sum(before[1].values()); kept=sum((before[1]&after[1]).values())
    assert total==kept,('Original painted/source corners lost',total,kept)
    return {'source_corners':total,'retained_corners':kept,'position_all_UVs_ratio':kept/max(1,total),'UV_channels':before[0]}


def create_part(body,name,world_points,faces,material_index,uv_rows=None,smooth=False):
    inv=body.matrix_world.inverted()
    me=bpy.data.meshes.new(name);me.from_pydata([inv@Vector(p) for p in world_points],[],faces);me.update()
    ob=bpy.data.objects.new(name,me);bpy.context.collection.objects.link(ob);ob.matrix_world=body.matrix_world.copy()
    for mat in body.data.materials:me.materials.append(mat)
    for channel in body.data.uv_layers:me.uv_layers.new(name=channel.name)
    for f in me.polygons:
        f.material_index=material_index;f.use_smooth=smooth
        for li in f.loop_indices:
            vi=me.loops[li].vertex_index
            uv=uv_rows[vi] if uv_rows is not None else (.5,.5)
            for channel in me.uv_layers:channel.data[li].uv=uv
    bm=bmesh.new();bm.from_mesh(me);bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(me);bm.free()
    return ob


def replace_steel(s,body):
    source=body.data
    slots=[i for i,m in enumerate(source.materials) if 'Blade' in m.name]
    assert slots==[1],('Named source steel slot is not unique',slots)
    polys={f.index for f in source.polygons if f.material_index==1}
    assert sum(len(source.polygons[i].vertices)-2 for i in polys)==14
    ids={vi for i in polys for vi in source.polygons[i].vertices}
    points=[body.matrix_world@source.vertices[i].co for i in ids]
    bb=bounds(points);base=bb[2][1];tip=bb[2][0];shoulder=tip+.09
    half_width=max(abs(bb[0][0]),abs(bb[0][1]));half_depth=max(abs(bb[1][0]),abs(bb[1][1]))
    assert .70<base-tip<.90 and abs(half_width-.022)<1e-5 and abs(half_depth-.0045)<1e-5
    hilt_before=signature(body,{0});s.remove_polygons(body,polys)
    # Every section is inside the old diamond cross-section. The original
    # root, tip and thickness extrema remain exact; a shallow central fuller
    # creates distinct ridges and bevels without a wider blade silhouette.
    profile=[(1.,0.),(.90,.08),(.40,.58),(.18,.82),(0.,.60),(-.18,.82),(-.40,.58),(-.90,.08),
             (-1.,0.),(-.90,-.08),(-.40,-.58),(-.18,-.82),(0.,-.60),(.18,-.82),(.40,-.58),(.90,-.08)]
    zs=[base,base-.018,base-(base-shoulder)*.48,shoulder+.018,shoulder]
    verts=[];uvs=[]
    for row,z in enumerate(zs):
        t=(base-z)/(base-shoulder);w=half_width*(1-t)+.018*t;depth=half_depth*(1-.20*t)
        for x,y in profile:
            if x==0 and row in (0,len(zs)-1):y=1. if y>0 else -1.
            verts.append(Vector((x*w,y*depth,z)))
            uvs.append((x*.5+.5,(base-z)/(base-tip)))
    tip_index=len(verts);verts.append(Vector((0,0,tip)));uvs.append((.5,1.))
    faces=[];n=len(profile)
    for row in range(len(zs)-1):
        for i in range(n):
            a,b=row*n+i,row*n+(i+1)%n;faces.append((a,b,b+n,a+n))
    for i in range(n):faces.append(((len(zs)-1)*n+i,(len(zs)-1)*n+(i+1)%n,tip_index))
    faces.append(tuple(range(n-1,-1,-1)))
    blade=create_part(body,'Arrel_BevelledFullerSteel',verts,faces,1,uvs)
    new_bb=bounds([blade.matrix_world@v.co for v in blade.data.vertices]);envelope=assert_bounds(bb,new_bb)
    retained(hilt_before,signature(body,{0}))
    return [body,blade],{'removed_only_named_steel_triangles':14,'old_blade_bounds_m':bb,'new_blade_envelope':envelope,
                         'hilt_painted_position_UVs':retained(hilt_before,signature(body,{0})),
                         'origin_world':list(body.matrix_world.translation),'fuller_depth_fraction':.40}


def staff_finish(s,body):
    me=body.data;W=body.matrix_world;regions=s.connected_regions(body,material=0)
    crystals=[];metal=[];gold_uv=None
    for region in regions:
        bb=bounds([W@me.vertices[i].co for i in region['vertices']]);size=[b-a for a,b in bb]
        if region['unique_vertices']==12 and region['triangles']==20 and .15<size[2]<.18 and .11<size[0]<.13:
            crystals.append(region)
        elif (region['triangles']==384 or (region['triangles']==44 and size[2]<.04)
              or (region['triangles']==20 and region['unique_vertices']==12)):
            metal.extend(region['polygons'])
            if gold_uv is None:
                face=me.polygons[region['polygons'][0]]
                gold_uv=tuple(me.uv_layers[0].data[face.loop_start].uv)
    assert len(crystals)==1 and gold_uv is not None
    assert sum(len(me.polygons[i].vertices)-2 for i in metal)==728
    crystal=crystals[0];crystal_ids=set(crystal['polygons'])
    original_crystal=[]
    for fi in crystal['polygons']:
        f=me.polygons[fi];assert len(f.vertices)==3
        original_crystal.append(([W@me.vertices[vi].co for vi in f.vertices],
                                 [tuple(me.uv_layers[0].data[li].uv) for li in f.loop_indices]))
    all_before=signature(body)
    material=bpy.data.materials.new('S362_StaffMetal');material.use_nodes=True;body.data.materials.append(material)
    material=bpy.data.materials.new('S362_StaffCrystal');material.use_nodes=True;body.data.materials.append(material)
    for fi in metal:me.polygons[fi].material_index=1
    s.remove_polygons(body,crystal_ids)
    parts=[body]
    # Each old triangular gem facet keeps all original outer vertices/UVs.
    # Its inset face is only0.8mm lower; original silhouette and source paint
    # coordinates remain anchored along every original outer edge.
    center=sum((p for points,uv in original_crystal for p in points),Vector())/(len(original_crystal)*3)
    for fi,(points,uv) in enumerate(original_crystal):
        centroid=sum(points,Vector())/3
        inward=(center-centroid).normalized()*.0008
        inner=[p*.84+centroid*.16+inward for p in points]
        inner_uv=[tuple(a*.84+sum(row[j] for row in uv)/3*.16 for j,a in enumerate(row)) for row in uv]
        verts=points+inner;uvs=uv+inner_uv
        faces=[(0,1,4,3),(1,2,5,4),(2,0,3,5),(3,4,5)]
        parts.append(create_part(body,'Elia_CrystalFacet_%02d'%fi,verts,faces,2,uvs))
    # Two small, rounded closed collars fitted to the retained wooden shaft.
    # They remain well inside the old whole-prop envelope and do not touch grip0.
    for band,z in enumerate((-.40,.22)):
        rows=[(z-.009,.0140),(z-.007,.0160),(z+.007,.0160),(z+.009,.0140)]
        verts=[];faces=[];n=48
        for height,radius in rows:
            for i in range(n):
                a=math.tau*i/n;verts.append(Vector((math.cos(a)*radius,math.sin(a)*radius,height)))
        for row in range(len(rows)-1):
            for i in range(n):a=row*n+i;b=row*n+(i+1)%n;faces.append((a,b,b+n,a+n))
        faces.append(tuple(range(n-1,-1,-1)));faces.append(tuple(range((len(rows)-1)*n,len(rows)*n)))
        parts.append(create_part(body,'Elia_FittedGoldCollar_%d'%band,verts,faces,1,[gold_uv]*len(verts),True))
    return parts,{'original_gold_triangles_material_finished':728,'replaced_only_crystal_triangles':20,
                  'crystal_facet_inset_m':.0008,'new_collars_z_m':[-.40,.22],'original_crystal_outer_points_kept':True,
                  'native_material_contract':'Old atlas BaseColor/UVs reused by copied gold/crystal graphs; wood/leather slot0 unchanged.'},all_before


def finish_export(s,parts,destination):
    body=parts[0]
    for ob in parts:s.normalize_colors(ob)
    bpy.ops.object.select_all(action='DESELECT')
    for ob in parts:ob.select_set(True)
    bpy.context.view_layer.objects.active=body;bpy.ops.object.join()
    if any(len(f.vertices)!=3 for f in body.data.polygons):
        modifier=body.modifiers.new('NewPropTriangles','TRIANGULATE');bpy.ops.object.modifier_apply(modifier=modifier.name)
    s.normalize_colors(body)
    bpy.ops.object.select_all(action='DESELECT');body.select_set(True);bpy.context.view_layer.objects.active=body
    bpy.ops.export_scene.fbx(filepath=str(destination),use_selection=True,object_types={'MESH'},axis_forward='-Y',axis_up='Z',
        apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',add_leaf_bones=False,path_mode='RELATIVE',
        bake_anim=False,mesh_smooth_type='FACE',colors_type='LINEAR',use_mesh_modifiers=False)
    return body


def main():
    parser=argparse.ArgumentParser();parser.add_argument('--project-root',type=Path,required=True);parser.add_argument('--revision',default='R1')
    args=parser.parse_args(sys.argv[sys.argv.index('--')+1:]);root=args.project_root.resolve()
    assert args.revision.startswith('R') and args.revision[1:].isdigit()
    out=root/'Unreal/ArtSource/HeroContactS362'/args.revision;assert not out.exists(),('Immutable prop revision exists',out)
    helper=root/'Unreal/ArtSource/HeroPolishS361/R4/generator_snapshot.py'
    spec=importlib.util.spec_from_file_location('accepted_s361',helper);s=importlib.util.module_from_spec(spec);spec.loader.exec_module(s)
    sources={'ArrelSword':Path(r'C:/Users/jc/Documents/Codex/MEMORIA-Unreal-Collaboration/models/_raw/s312_sword/arrel_prop_sword_drawn.fbx'),
             'EliaStaff':root/'Unreal/ArtSource/FieldCharacters/elia/elia_prop_staff.fbx'}
    frozen={str(p):sha(p) for p in [helper,*sources.values()]};out.mkdir(parents=True,exist_ok=False)
    snapshot=out/'generator_snapshot.py';snapshot.write_bytes(Path(__file__).read_bytes());reports=[]
    for name,path in sources.items():
        bpy.ops.wm.read_factory_settings(use_empty=True);bpy.ops.import_scene.fbx(filepath=str(path))
        meshes=[o for o in bpy.data.objects if o.type=='MESH'];assert len(meshes)==1;body=meshes[0]
        original_bounds=bounds([body.matrix_world@v.co for v in body.data.vertices]);original_origin=list(body.matrix_world.translation)
        original_materials=[m.name for m in body.data.materials]
        if name=='ArrelSword':
            before=signature(body,{0});parts,detail=replace_steel(s,body)
        else:parts,detail,before=staff_finish(s,body)
        folder=out/name;folder.mkdir();dest=folder/(name+'.fbx');body=finish_export(s,parts,dest)
        authored_retained=retained(before,signature(body,{0}) if name=='ArrelSword' else signature(body))
        authored_bounds=bounds([body.matrix_world@v.co for v in body.data.vertices]);assert_bounds(original_bounds,authored_bounds)
        authored_triangles=len(body.data.polygons);material_names=[m.name for m in body.data.materials]
        bpy.ops.wm.read_factory_settings(use_empty=True);bpy.ops.import_scene.fbx(filepath=str(dest))
        meshes=[o for o in bpy.data.objects if o.type=='MESH'];assert len(meshes)==1;body=meshes[0]
        assert len(body.data.polygons)==authored_triangles
        assert [m.name for m in body.data.materials]==material_names
        roundtrip_retained=retained(before,signature(body,{0}) if name=='ArrelSword' else signature(body))
        envelope=assert_bounds(original_bounds,bounds([body.matrix_world@v.co for v in body.data.vertices]))
        assert (Vector(original_origin)-body.matrix_world.translation).length<1e-7
        report={'schema':1,'session':'S362','revision':args.revision,'name':name,'source':str(path),'source_sha256':sha(path),
                'generator_sha256':sha(snapshot),'output_sha256':sha(dest),'source_materials':original_materials,'output_materials':material_names,
                'source_origin_m':original_origin,'triangles':authored_triangles,'detail':detail,'authored_retained':authored_retained,
                'roundtrip_retained':roundtrip_retained,'roundtrip_envelope':envelope,
                'contract':'Original prop origin, grip frame, length, outer bounds and all retained original paint UVs unchanged. No image, hero/clothes, rig, clip or gameplay edit.'}
        write_json(folder/'geometry.json',report);reports.append(report)
        print('S362_PROP_MESH_COMPLETE',name,authored_triangles,flush=True)
    assert frozen=={str(p):sha(p) for p in [helper,*sources.values()]},'Original input changed'
    write_json(out/'source_inventory.json',{'schema':1,'session':'S362','revision':args.revision,'source_frozen_sha256':frozen,
               'generator_sha256':sha(snapshot),'props':[{'name':r['name'],'triangles':r['triangles'],'output_sha256':r['output_sha256']} for r in reports]})
    print('S362_PROP_AUTHORING_COMPLETE',args.revision,'meshes=2 images=0 rigs=0 clips=0',flush=True)

if __name__=='__main__':main()
