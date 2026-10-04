"""Render all six delivered FBXs together at their real shared physical scale."""
from pathlib import Path
import argparse,json,math
import bpy
from mathutils import Vector,Quaternion,Matrix
from build_townsfolk_specs_s347 import HEIGHTS
from PIL import Image,ImageDraw,ImageFont
p=argparse.ArgumentParser();p.add_argument('--models-root',required=True);a=p.parse_args();root=Path(a.models_root).resolve();out=root/'_qa/s347';out.mkdir(parents=True,exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True);scene=bpy.context.scene;scene.render.engine='CYCLES';scene.cycles.samples=20;scene.cycles.device='CPU';scene.render.threads_mode='FIXED';scene.render.threads=4
scene.render.resolution_x=1800;scene.render.resolution_y=900;scene.render.resolution_percentage=100;scene.render.film_transparent=False;scene.view_settings.view_transform='Standard';scene.view_settings.exposure=-.3
order=['npc_villager_f','npc_villager_m','npc_fisherman','npc_elder','npc_child','npc_scholar']; positions=[]
for i,id in enumerate(order):
 before=set(scene.objects);bpy.ops.import_scene.fbx(filepath=str(root/id/f'{id}_rigged.fbx'),use_anim=False)
 new=set(scene.objects)-before;r=next(o for o in new if o.type=='ARMATURE');body=next(o for o in new if o.type=='MESH');r.matrix_world=Matrix.Translation(((i-2.5)*.86,0,0))@r.matrix_world
 for side,sign in [('l',1),('r',-1)]:
  n='upperarm_'+side;q=r.data.bones[n].matrix_local.to_quaternion();b=r.pose.bones[n];b.rotation_mode='QUATERNION';b.rotation_quaternion=q.inverted()@Quaternion(Vector((0,1,0)),math.radians(sign*18))@q
 mat=body.data.materials[0];pbr=mat.node_tree.nodes.get('Principled BSDF');pbr.inputs['Roughness'].default_value=.89;pbr.inputs['Specular IOR Level'].default_value=.19
 positions.append(dict(id=id,height_cm=HEIGHTS[id],world_x_m=(i-2.5)*.86))
scene.world=bpy.data.worlds.new('S347GalleryWorld');scene.world.use_nodes=True;scene.world.node_tree.nodes['Background'].inputs[0].default_value=(.075,.087,.108,1);scene.world.node_tree.nodes['Background'].inputs[1].default_value=.7
bpy.ops.mesh.primitive_plane_add(size=200,location=(0,0,-.002));floor=bpy.context.object;mat=bpy.data.materials.new('S347Ground');mat.diffuse_color=(.025,.03,.04,1);mat.use_nodes=True;pbr=mat.node_tree.nodes.get('Principled BSDF');pbr.inputs['Base Color'].default_value=(.025,.03,.04,1);pbr.inputs['Roughness'].default_value=.95;floor.data.materials.append(mat)
for name,loc,power,size in [('Key',(-3,-4,5),600,5),('Fill',(4,-2,3),350,4),('Rim',(0,3,4),700,5)]:
 ld=bpy.data.lights.new(name,'AREA');ob=bpy.data.objects.new(name,ld);scene.collection.objects.link(ob);ob.location=loc;ld.energy=power;ld.size=size;ob.rotation_euler=(Vector((0,0,.9))-ob.location).to_track_quat('-Z','Y').to_euler()
cd=bpy.data.cameras.new('S347GalleryCamera');cam=bpy.data.objects.new('S347GalleryCamera',cd);scene.collection.objects.link(cam);scene.camera=cam;cd.type='ORTHO';cd.ortho_scale=5.75;cam.location=(0,-10,3);cam.rotation_euler=(Vector((0,0,.9))-cam.location).to_track_quat('-Z','Y').to_euler();scene.render.filepath=str(out/'townsfolk_gallery_render.png');bpy.ops.render.render(write_still=True)
# Labels are the only added pixels; the characters and their relative heights come from this one real scene.
im=Image.open(out/'townsfolk_gallery_render.png').convert('RGB');sheet=Image.new('RGB',(1800,1040),(25,29,37));sheet.paste(im,(0,60));d=ImageDraw.Draw(sheet);font=ImageFont.truetype('C:/Windows/Fonts/arial.ttf',25);small=ImageFont.truetype('C:/Windows/Fonts/arial.ttf',20)
d.text((28,16),'MEMORIA / S347 townsfolk - delivered FBXs at one physical scale',font=font,fill=(223,211,187))
labels=['Villager F','Villager M','Fisherman','Elder','Child','Scholar']
for i,id in enumerate(order):
 x=round(1800*(.5+((i-2.5)*.86)/5.75));s=f'{labels[i]} / {HEIGHTS[id]} cm';box=d.textbbox((0,0),s,font=small);d.text((x-(box[2]-box[0])/2,975),s,font=small,fill=(223,211,187))
d.text((28,1014),'Blender studio preview; game placement and Epic animation retargeting remain the application step.',font=small,fill=(149,157,170));sheet.save(out/'townsfolk_gallery.png')
(out/'gallery_scene.json').write_text(json.dumps(dict(render_source='all six delivered FBXs freshly imported into one scene',positions=positions,view='orthographic front-oblique studio',resolution=[1800,900],labelled_resolution=[1800,1040],game_capture=False),indent=2),encoding='utf-8')
bpy.ops.wm.save_as_mainfile(filepath=str(root/'_raw/s347_townsfolk/townsfolk_gallery.blend'));print('S347_GALLERY_PASS',flush=True)
