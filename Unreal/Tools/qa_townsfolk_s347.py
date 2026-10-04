"""S347 independent relocated FBX roundtrip, units, rig, skin and visible deformation QA."""
from pathlib import Path
import argparse, hashlib, importlib, json, math, shutil, sys, types
import bpy, numpy as np
from mathutils import Matrix, Vector, Quaternion
from PIL import Image, ImageDraw, ImageFont
from build_townsfolk_specs_s347 import HEIGHTS
p=argparse.ArgumentParser();p.add_argument('ident',choices=HEIGHTS);p.add_argument('--models-root',required=True);args=p.parse_args()
ID=args.ident; H=HEIGHTS[ID]; ROOT=Path(args.models_root).resolve(); Q=ROOT/'_qa/s347'/ID; Q.mkdir(parents=True,exist_ok=True)
OUT=ROOT/ID; relocated=ROOT/'_qa/s347/relocated'/ID; relocated.mkdir(parents=True,exist_ok=True)
for suffix in ('_rigged.fbx','_basecolor.png'):shutil.copy2(OUT/(ID+suffix),relocated/(ID+suffix))
# Pure Blender FBX parser checks the file's encoded global axis/unit and relative texture link.
pkg=types.ModuleType('s347_fbx');pkg.__path__=[str(ROOT/'_tools/runtime311/Lib/site-packages/bpy/4.5/scripts/addons_core/io_scene_fbx')];sys.modules['s347_fbx']=pkg
parse=importlib.import_module('s347_fbx.parse_fbx').parse
fbxroot,version=parse(str(relocated/f'{ID}_rigged.fbx'))
def child(n,ident):return next((c for c in n.elems if c.id==ident),None)
settings={c.props[0].decode():[x.decode() if isinstance(x,bytes) else x for x in c.props[4:]] for c in child(child(fbxroot,b'GlobalSettings'),b'Properties70').elems}
assert abs(settings['UnitScaleFactor'][0]-1)<1e-6,settings
assert settings['UpAxis']==[2] and settings['UpAxisSign']==[1],settings
assert settings['FrontAxis']==[1] and settings['FrontAxisSign']==[1],settings # FBX stores parity; exporter requested -Y/Z.
objects=child(fbxroot,b'Objects');textures=[n for n in objects.elems if n.id in (b'Texture',b'Video')]
relative=[]
for t in textures:
 n=child(t,b'RelativeFilename')
 if n:relative.append(n.props[0].decode().replace('\\','/'))
assert relative and all((relocated/x).is_file() for x in relative),relative

def empty():bpy.ops.wm.read_factory_settings(use_empty=True)
def imported(path):
 bpy.ops.import_scene.fbx(filepath=str(path),use_anim=False)
 rigs=[o for o in bpy.context.scene.objects if o.type=='ARMATURE'];meshes=[o for o in bpy.context.scene.objects if o.type=='MESH']
 assert len(rigs)==1 and len(meshes)==1
 return rigs[0],meshes[0]
def hierarchy(r):return {b.name:b.parent.name if b.parent else None for b in r.data.bones}
def coords(ob):
 deps=bpy.context.evaluated_depsgraph_get();ob=bpy.data.objects[ob.name].evaluated_get(deps);me=ob.to_mesh();v=np.array([ob.matrix_world@x.co for x in me.vertices]);ob.to_mesh_clear();return v
empty();ref,refbody=imported(ROOT/'arrel/arrel_rigged.fbx');expected=hierarchy(ref)
empty();rig,body=imported(relocated/f'{ID}_rigged.fbx');actual=hierarchy(rig);assert actual==expected and len(actual)==77
base=coords(body);assert np.isfinite(base).all();height=float(np.ptp(base[:,2]));assert abs(height-H/100)<.001
assert abs(float(base[:,2].min()))<.0001
body.data.calc_loop_triangles();tri=len(body.data.loop_triangles);assert tri<=25000
sums=np.array([sum(g.weight for g in v.groups) for v in body.data.vertices]);influences=[sum(g.weight>1e-6 for g in v.groups) for v in body.data.vertices]
assert sums.min()>.999 and sums.max()<1.001 and max(influences)<=2
uv=np.array([x.uv[:] for x in body.data.uv_layers.active.data]);assert np.isfinite(uv).all() and uv.min()>=0 and uv.max()<=1
assert len(body.data.materials)==1 and Image.open(relocated/f'{ID}_basecolor.png').size==(2048,2048)
assert Image.open(relocated/f'{ID}_basecolor.png').mode=='RGB'
images=[n.image for n in body.data.materials[0].node_tree.nodes if n.type=='TEX_IMAGE' and n.image]
assert images and all(Path(bpy.path.abspath(im.filepath)).resolve().is_relative_to(relocated.resolve()) for im in images),[im.filepath for im in images]
# Visible render uses the established rough cloth preset; no geometry repair after the roundtrip.
scene=bpy.context.scene; scene.render.engine='CYCLES';scene.cycles.device='CPU';scene.cycles.samples=12;scene.render.threads_mode='FIXED';scene.render.threads=4
scene.render.resolution_x=480;scene.render.resolution_y=640;scene.render.resolution_percentage=100;scene.render.film_transparent=True;scene.render.image_settings.color_mode='RGBA'
scene.view_settings.view_transform='Standard';scene.view_settings.exposure=-.3
mat=body.data.materials[0];mat.use_nodes=True;pbr=mat.node_tree.nodes.get('Principled BSDF');pbr.inputs['Roughness'].default_value=.89;pbr.inputs['Specular IOR Level'].default_value=.19
scene.world=bpy.data.worlds.new('S347Studio');scene.world.use_nodes=True;scene.world.node_tree.nodes['Background'].inputs[0].default_value=(.34,.36,.42,1);scene.world.node_tree.nodes['Background'].inputs[1].default_value=.8
for name,loc,power,size in [('Key',(-2.2,-2.9,3.1),230,2.7),('Fill',(2.1,-.7,2.2),110,2.1),('Rim',(.4,2.4,2.9),220,1.7)]:
 ld=bpy.data.lights.new(name,'AREA');lo=bpy.data.objects.new(name,ld);scene.collection.objects.link(lo);lo.location=loc;ld.energy=power;ld.size=size;lo.rotation_euler=(Vector((0,0,H/200))-lo.location).to_track_quat('-Z','Y').to_euler()
cd=bpy.data.cameras.new('S347QA');cam=bpy.data.objects.new('S347QA',cd);scene.collection.objects.link(cam);scene.camera=cam;cd.type='ORTHO';cd.ortho_scale=H/100*1.3;cd.clip_end=100

def render(label,loc):
 cam.location=loc;cam.rotation_euler=(Vector((0,0,H/200))-cam.location).to_track_quat('-Z','Y').to_euler();scene.render.filepath=str(Q/f'{label}.png');bpy.ops.render.render(write_still=True)
rest={b.name:b.matrix_local.copy() for b in rig.data.bones}
def reset():
 for b in rig.pose.bones:b.matrix_basis=Matrix.Identity(4);b.rotation_mode='QUATERNION'
 bpy.context.view_layer.update()
def rotate(name,axis,angle):
 q=rest[name].to_quaternion();rig.pose.bones[name].rotation_quaternion=q.inverted()@Quaternion(Vector(axis),math.radians(angle))@q

def pose(label):
 reset()
 for side,sg in [('l',1),('r',-1)]:rotate('upperarm_'+side,(0,1,0),sg*18)
 rotate('spine_01',(1,0,0),2);rotate('neck_01',(1,0,0),-2)
 if label.startswith('walk'):
  phase=1 if label=='walk_left' else -1
  for side,sg in [('l',1),('r',-1)]:
   rotate('thigh_'+side,(1,0,0),sg*phase*22);rotate('calf_'+side,(1,0,0),-14 if sg*phase>0 else -32)
 elif label=='reach':
  rotate('spine_02',(1,0,0),12);rotate('lowerarm_l',(1,0,0),-35);rotate('lowerarm_r',(1,0,0),-25)
 bpy.context.view_layer.update()
edges=np.array([e.vertices[:] for e in body.data.edges]);lengths=np.linalg.norm(base[edges[:,0]]-base[edges[:,1]],axis=1);keep=lengths>.001
report=dict(id=ID,triangles=tri,vertices=len(base),bones=len(actual),hierarchy_matches_arrel=True,height_cm=height*100,sole_z_cm=float(base[:,2].min())*100,max_influences=max(influences),weight_sum_min=float(sums.min()),weight_sum_max=float(sums.max()),material_count=1,texture_mode='RGB',texture_size=[2048,2048],uv_bounds=[float(uv.min()),float(uv.max())],fbx_version=version,global_settings=settings,relative_texture_links=relative,relocated_texture_resolves=True,poses={},bone_parents=actual,fbx_sha256=hashlib.sha256((OUT/f'{ID}_rigged.fbx').read_bytes()).hexdigest(),texture_sha256=hashlib.sha256((OUT/f'{ID}_basecolor.png').read_bytes()).hexdigest())
for label,loc in [('front',(0,-5,H/200)),('side',(-5,0,H/200)),('back',(0,5,H/200))]:reset();render(label,loc)
for label in ('idle','walk_left','walk_right','reach'):
 pose(label);v=coords(body);ratio=np.linalg.norm(v[edges[:,0]]-v[edges[:,1]],axis=1)[keep]/lengths[keep]
 metrics=dict(finite=bool(np.isfinite(v).all()),edge_stretch_p99=float(np.percentile(ratio,99)),edge_stretch_max=float(ratio.max()),bounds_m=[v.min(0).tolist(),v.max(0).tolist()]);assert metrics['finite'] and metrics['edge_stretch_p99']<1.8 and metrics['edge_stretch_max']<4,metrics
 report['poses'][label]=metrics;render(label,(3.2,-4,H/200+1.3))
pose('idle');scene.render.resolution_x=1280;scene.render.resolution_y=720;cd.ortho_scale=H/100*5.4;render('game_scale_48deg',(3.2,-4,H/200+4.0))
bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'_raw/s347_townsfolk'/ID/f'{ID}_roundtrip.blend'))
(Q/'fbx_roundtrip.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
font=ImageFont.truetype('C:/Windows/Fonts/arial.ttf',18);sheet=Image.new('RGB',(1440,1400),(30,34,42));d=ImageDraw.Draw(sheet)
for i,label in enumerate(('front','side','back','idle','walk_left','walk_right')):
 im=Image.open(Q/f'{label}.png').convert('RGBA');x=i%3*480;y=i//3*700;sheet.paste(im,(x,y+35),im);d.text((x+12,y+8),ID+' / '+label,font=font,fill=(229,220,204))
sheet.save(Q/'review_sheet.png');print('S347_FBX_QA_PASS',json.dumps({k:report[k] for k in ('id','height_cm','triangles','bones','max_influences','poses')}),flush=True)
