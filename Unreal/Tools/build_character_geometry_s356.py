"""S356 immutable character geometry; Blender background factory-startup entry point.
S354's mesh/skin helpers are read verbatim; this revision writes only CharacterUpgradeS356.
Original painted images, all original rigs/clips and S354 generated packages stay immutable.
"""
from pathlib import Path
import json, hashlib, sys, math
import bpy, bmesh
from mathutils import Vector, Matrix, Quaternion
from mathutils.bvhtree import BVHTree
ROOT=Path(__file__).resolve().parents[2]
previous=(ROOT/'Unreal/Tools/build_character_geometry_s354.py').read_text(encoding='utf-8')
helpers=previous.split("\nargs=sys.argv[")[0]
helpers=helpers.replace("'Traveler':.99,'Bureauagent':1.02,'Guard':1.10}[name]", "'Traveler':.99,'Bureauagent':1.02,'Guard':1.10}.get(name,1.0)")
helpers=helpers.replace("long=name in ['Villagerf','Elder','Scholar']", "long=name in ['Villagerf','Elder','Scholar','Elia','Malet','AshWalker']")
helpers=helpers.replace("body,report=join_export(name,H)", "s356_garments(name,H);body,report=join_export(name,H)")
helpers=helpers.replace("bm.free();PARTS.append(old)","bm.free();tag_preserved(old);PARTS.append(old)")
helpers=helpers.replace("colors_type='LINEAR' if CREATURE else 'SRGB'","colors_type='LINEAR'")
# Avoid even the prior output-directory initializer; its authored FBX and pigment assets are read-only.
helpers=helpers.replace("OUT=ROOT/'Unreal/ArtSource/CharacterUpgrade'", "OUT=ROOT/'Unreal/ArtSource/CharacterUpgradeS356/Accepted'")
exec(compile(helpers,'S354_readonly_helpers','exec'),globals())
OUT=ROOT/'Unreal/ArtSource/CharacterUpgradeS356/Accepted'
SOURCES={'Elia':'elia','Malet':'malet','AshWalker':'void_husk','BeltScavenger':'market_thief'}
CURRENT_CHARACTER='Elia'
HEAD_REFINEMENT=False
FACE_BVH=None
FACE_CENTER=None
FACE_RADII=None
PALETTES={
 'Elia':[(.48,.40,.29),(.065,.035,.018),(.23,.18,.105),(.34,.25,.17),(.07,.055,.035),(.32,.19,.065),(.27,.20,.12),(.12,.10,.07)],
 'Malet':[(.10,.13,.105),(.06,.033,.018),(.28,.23,.15),(.34,.25,.17),(.055,.035,.018),(.32,.19,.065),(.23,.14,.065),(.17,.13,.075)],
 'AshWalker':[(.055,.047,.038),(.035,.025,.018),(.09,.07,.045),(.20,.15,.10),(.023,.020,.017),(.16,.10,.04),(.34,.28,.18),(.027,.023,.019)],
 'BeltScavenger':[(.075,.065,.044),(.045,.027,.014),(.15,.12,.07),(.25,.18,.12),(.025,.023,.018),(.18,.14,.075),(.13,.085,.039),(.04,.045,.035)]}
_original_mesh=mesh
def mesh(name,verts,faces,uvs,weights,tile=0,color=(.2,.2,.2,1)):
    if not CREATURE:color=tuple(PALETTES[CURRENT_CHARACTER][tile%8])+(1,)
    return _original_mesh(name,verts,faces,uvs,weights,tile,color)
def tag_preserved(ob):
    # Original irregular head/hand/boot UVs survive exactly; alpha selects them in the new material.
    for attr in list(ob.data.color_attributes):ob.data.color_attributes.remove(attr)
    layer=ob.data.color_attributes.new(name='Color',type='BYTE_COLOR',domain='CORNER')
    for item in layer.data:item.color=(1,1,1,0)


def ribbon(name,points,width,tile,bone,color=(.2,.2,.2,1)):
    ps=list(map(Vector,points));vs=[];uv=[];ws=[];fs=[]
    for j,p in enumerate(ps):
        t=j/max(1,len(ps)-1);side=Vector((1,0,0));w=width*(1-.6*t)
        for s in [-1,1]:vs.append(p+side*w*s);uv.append(((s+1)/2,t));ws.append({bone:1})
    for j in range(len(ps)-1):a=j*2;fs.append((a,a+1,a+3,a+2))
    return mesh(name,vs,fs,uv,ws,tile,color)

def face_point(x,z,offset=.1):
    location,normal,index,distance=FACE_BVH.ray_cast(Vector((x,-1000,z)),Vector((0,1,0)))
    assert location is not None,('face surface missed',x,z)
    return Vector((x,location.y-offset,z))

def s356_garments(name,H):
    """Accessories surround, rather than replace, the original painted outfit and face."""
    K=H/180;bn=lambda n:RIG.data.bones[n].head_local.copy()
    hip=bn('pelvis').z;sh=bn('upperarm_l').z;hz=bn('head').z;cy=bn('spine_03').y;w=abs(bn('upperarm_l').x)
    def piping(label,ps,tile=2,bone='spine_03',r=.8):return tube(label,ps,[r*K]*len(ps),tile,fixed(bone),n=8)
    if name=='Elia':
        # Open front, short split cape: embroidered bodice and existing shoulder tailoring stay visible.
        for sign in [-1,1]:
            ps=[(sign*5*K,cy+10*K,sh+1*K),(sign*(w+6*K),cy+6*K,sh-2*K),(sign*(w+3*K),cy+21*K,sh-26*K),(sign*4*K,cy+24*K,sh-32*K)]
            panel('OpenBackCape',ps,2,'spine_03',.7*K);piping('CapeEdge',ps[1:])
        ell('TravelSatchel',(-w-8*K,cy+3*K,hip-11*K),(8*K,5*K,10*K),1,'pelvis',nu=24,nv=16)
        panel('SatchelFlap',[(-w-15*K,cy-2*K,hip-4*K),(-w-1*K,cy-2*K,hip-4*K),(-w-2*K,cy-3*K,hip-12*K),(-w-14*K,cy-3*K,hip-12*K)],2,'pelvis',.6*K)
        piping('SideSatchelStrap',[(-w*.88,cy+5*K,sh-3*K),(-w-5*K,cy+4*K,hip+3*K)],1,'spine_02',1.15)
        ell('SideCapeClasp',(w*.9,cy-6*K,sh-6*K),(2*K,1*K,2*K),5,'spine_03',nu=16,nv=10)
    elif name=='Malet':
        # No hat, collar, scarf or apron over the detailed original coat, waistcoat and glasses.
        for x in [-w-5*K,w+5*K]:
            ell('TradePouch',(x,cy+4*K,hip-9*K),(6.3*K,4.4*K,9*K),1,'pelvis',nu=24,nv=16)
            ell('PouchClasp',(x,cy-.5*K,hip-5*K),(1*K,.6*K,1.4*K),5,'pelvis',nu=12,nv=8)
        ps=[(x*K,cy+15*K,sh-12*K) for x in [-13,-7,0,7,13]]
        tube('RolledMerchantBlanket',ps,[4*K]*5,7,fixed('spine_02'),n=20,fold=.07)
        for x in [-8,8]:piping('BlanketBinding',[(x*K,cy+10*K,sh-8*K),(x*K,cy+18*K,sh-8*K)],1,'spine_02',.8)
        piping('WatchChain',[(w*.67,cy-6*K,hip+25*K),(w*.45,cy-10*K,hip+15*K),(w*.12,cy-8*K,hip+24*K)],5,'spine_02',.4)
    elif name=='BeltScavenger':
        ell('RationPack',(0,cy+21*K,sh-20*K),(14*K,9*K,21*K),1,'spine_02',nu=24,nv=16)
        for sign in [-1,1]:
            piping('PackSideStrap',[(sign*w*.85,cy+16*K,hip+14*K),(sign*w*.9,cy+7*K,sh+1*K),(sign*w*.85,cy-4*K,hip+12*K)],1,'spine_02',1.3)
        for j in range(3):
            panel('SalvagedShoulderPlate',[(w-4*K,cy-4*K,sh-j*3*K),(w+10*K+j*K,cy-2*K,sh-j*4*K),(w+9*K+j*K,cy+7*K,sh-6*K-j*4*K),(w-3*K,cy+9*K,sh-3*K-j*3*K)],5,'upperarm_l',.8*K)
        for side in ['l','r']:
            p=bn('calf_'+side).lerp(bn('foot_'+side),.45)
            tube('BoundGreave',[p+Vector((0,0,9*K)),p,p-Vector((0,0,9*K))],[4.8*K,5.1*K,4*K],1,fixed('calf_'+side),n=16,flat=.85,fold=.04)
        # Small leather backing reaches the original crown height; the full face remains in front.
        if not HEAD_REFINEMENT:
            ell('BackHeadBinding',(0,cy+8*K,H-4*K),(7*K,4*K,4*K),1,'head',nu=24,nv=12)
        else:
            # Every facial path follows a ray intersection with the actual original face triangles.
            for sign in [-1,1]:
                ps=[face_point(sign*x,z) for x,z in [(2,158.12),(2.85,158.12),(4.7,158.12)]]
                tube('ConformingEyeSlit',ps,[.32,.42,.18],4,fixed('head'),n=10)
                ps=[face_point(sign*x,z) for x,z in [(1.5,160.15),(3,160.15),(5,159.65)]]
                tube('ConformingBrow',ps,[.22,.40,.18],1,fixed('head'),n=10)
            # Compact faceted nose projects from the face, with all six base vertices embedded.
            base=[face_point(x,z,0) for x,z in [(-.85,157.7),(.85,157.7),(1.25,153.8),(-1.25,153.8)]]
            tip=face_point(0,155.5,1.5)
            mesh('NoseBridge',base+[tip],[(0,1,4),(1,2,4),(2,3,4),(3,0,4)],[(0,0),(1,0),(1,1),(0,1),(.5,.5)],[{'head':1}]*5,3)
            ps=[face_point(x,z) for x,z in [(-2.4,152),(0,151.8),(2.4,152)]]
            tube('ConformingMouth',ps,[.15,.25,.15],4,fixed('head'),n=8)
            ps=[face_point(x,z) for x,z in [(-5,155.3),(-4.5,153.8),(-3.9,153.1)]]
            tube('OldCheekScar',ps,[.12,.20,.08],1,fixed('head'),n=8)
    else:
        for sign in [-1,1]:
            points=[(sign*4*K,cy+11*K,sh+1*K),(sign*(w+8*K),cy+8*K,sh-2*K),(sign*(w+12*K),cy+17*K,hip+13*K),(sign*(w*.9),cy+23*K,hip-37*K),(sign*4*K,cy+24*K,hip-50*K)]
            panel('CharredBackMantle',points,7,'spine_02',.8*K)
            for j in range(3):
                z=hip+17*K+j*8*K
                tube('CalcifiedRib',[(sign*5*K,cy-10*K,z+1*K),(sign*12*K,cy-11*K,z),(sign*(w+1*K),cy-3*K,z+5*K)],[1.2*K,1.5*K,.25*K],6,fixed('spine_02'),n=10)
            if HEAD_REFINEMENT:
                root=face_point(sign*6.4,173,-.35)
                tube('RootedBurntCrown',[root,(sign*11,FACE_CENTER.y,180),(sign*8,FACE_CENTER.y,H)],[2.2*K,1.3*K,.05*K],7,fixed('head'),n=10)
            else:
                tube('BurntCrown',[(sign*8*K,cy+4*K,hz+16*K),(sign*11*K,cy+6*K,hz+23*K),(sign*8*K,cy+8*K,H)],[2.2*K,1.3*K,.05*K],7,fixed('head'),n=10)
        # Small relief is embedded into the original mask surface, rather than a detached plane.
        z=hz+11*K;front=cy-8*K
        for sign in [-1,1]:
            if HEAD_REFINEMENT:
                ps=[face_point(sign*x,zz) for x,zz in [(1.5,171.5),(5.5,172.5),(7,170.5)]]
                tube('ConformingAshBrow',ps,[.50,.75,.2],6,fixed('head'),n=10)
                ps=[face_point(sign*x,zz) for x,zz in [(6.5,167),(5,163),(2,161)]]
                tube('ConformingAshCheek',ps,[.50,.65,.2],6,fixed('head'),n=10)
            else:
                tube('AshBrow',[(sign*1.5*K,front,z+4*K),(sign*5.5*K,front-.5*K,z+5*K),(sign*8*K,front+1*K,z+3*K)],[.7*K,1*K,.2*K],6,fixed('head'),n=8)
                tube('AshCheek',[(sign*7*K,front+1*K,z),(sign*5*K,front,z-4*K),(sign*2*K,front+1*K,z-6*K)],[.65*K,.9*K,.2*K],6,fixed('head'),n=8)

def head_components(ob):
    adj={v.index:set() for v in ob.data.vertices}
    for e in ob.data.edges:a,b=e.vertices;adj[a].add(b);adj[b].add(a)
    done=set();groups=[]
    for vi in adj:
        if vi in done:continue
        group={vi};todo=[vi];done.add(vi)
        while todo:
            for other in adj[todo.pop()]-done:done.add(other);group.add(other);todo.append(other)
        if all(any(ob.vertex_groups[g.group].name=='head' and g.weight>.99 for g in ob.data.vertices[i].groups) for i in group):groups.append(group)
    return groups

def humanoid(name):
    global CURRENT_CHARACTER,RIG,MAT,PARTS,CREATURE,FACE_BVH,FACE_CENTER,FACE_RADII
    CURRENT_CHARACTER=name;CREATURE=False;PARTS=[]
    source_id=SOURCES[name];source=ROOT/'Unreal/ArtSource/FieldCharacters'/source_id/(source_id+'_rigged.fbx')
    bpy.ops.wm.read_factory_settings(use_empty=True);bpy.ops.import_scene.fbx(filepath=str(source))
    RIG=next(o for o in bpy.data.objects if o.type=='ARMATURE');old=next(o for o in bpy.data.objects if o.type=='MESH')
    H=max(v.co.z for v in old.data.vertices)-min(v.co.z for v in old.data.vertices);original_count=len(old.data.vertices)
    old.parent=None;old.matrix_world=Matrix.Identity(4);RIG.scale=(1,1,1)
    removed=[];moved=[]
    if name in ['AshWalker','BeltScavenger']:
        # S313 source has separately connected HoodShell(360), HoodRim(328), Scarf(264).
        # Its faceless husk is deliberately recessed; expose that original UV surface at the head front.
        groups=head_components(old);delete=set()
        for group in groups:
            if len(group) in [360,328] or (name=='BeltScavenger' and (len(group)==264 or (HEAD_REFINEMENT and len(group)==84))):delete.update(group);removed.append(len(group))
            elif len(group)==260:
                points=[old.data.vertices[i].co.copy() for i in group]
                lo=Vector([min(v[k] for v in points) for k in range(3)]);hi=Vector([max(v[k] for v in points) for k in range(3)])
                FACE_CENTER=(lo+hi)*.5;FACE_RADII=(hi-lo)*.5
                polygons=[tuple(f.vertices) for f in old.data.polygons if all(i in group for i in f.vertices)]
                FACE_BVH=BVHTree.FromPolygons([v.co.copy() for v in old.data.vertices],polygons,all_triangles=True)
            if name=='AshWalker' and len(group)==260 and not HEAD_REFINEMENT:
                for i in group:old.data.vertices[i].co.y-=13*H/185
                moved.append({'component_vertices':260,'offset_cm':[0,-13*H/185,0]})
        assert sorted(removed)==([328,360] if name=='AshWalker' else ([84,84,264,328,360] if HEAD_REFINEMENT else [264,328,360])),(name,removed)
        bm=bmesh.new();bm.from_mesh(old.data);bm.verts.ensure_lookup_table();bmesh.ops.delete(bm,geom=[bm.verts[i] for i in delete],context='VERTS');bm.to_mesh(old.data);bm.free()
    tag_preserved(old);PARTS.append(old);preserved_count=len(old.data.vertices)
    # The active original material is slot 0. New opaque accessories use independent slot 1.
    original_material=old.data.materials[0];original_material.name='OriginalBody'
    MAT=bpy.data.materials.new(name+'Accessories');MAT.use_nodes=True
    vc=MAT.node_tree.nodes.new('ShaderNodeVertexColor');vc.layer_name='Color';MAT.node_tree.links.new(vc.outputs['Color'],MAT.node_tree.nodes['Principled BSDF'].inputs['Base Color'])
    s356_garments(name,H);accessory_count=sum(len(o.data.vertices) for o in PARTS[1:])
    bpy.ops.object.select_all(action='DESELECT')
    for ob in PARTS:ob.select_set(True)
    body=old;bpy.context.view_layer.objects.active=body;bpy.ops.object.join();body.name=name+'_Body'
    assert len(body.data.materials)==2,(name,len(body.data.materials))
    for mod in list(body.modifiers):body.modifiers.remove(mod)
    tri=body.modifiers.new('Triangles','TRIANGULATE');bpy.ops.object.modifier_apply(modifier=tri.name)
    body.parent=RIG;body.matrix_parent_inverse=Matrix.Identity(4);body.matrix_basis=Matrix.Identity(4)
    skin=body.modifiers.new('Skin','ARMATURE');skin.object=RIG
    RIG.scale=(1,1,1);RIG.location=(0,0,0);RIG.rotation_euler=(0,0,0)
    bpy.context.scene.unit_settings.system='METRIC';bpy.context.scene.unit_settings.scale_length=.01
    assert all(v.groups and abs(sum(g.weight for g in v.groups)-1)<.001 for v in body.data.vertices)
    height=max(v.co.z for v in body.data.vertices)-min(v.co.z for v in body.data.vertices)
    assert abs(height-H)<.1 or (HEAD_REFINEMENT and name=='BeltScavenger' and abs(height-164.969)<.1),(name,height,H)
    bpy.ops.object.select_all(action='DESELECT');body.select_set(True);RIG.select_set(True);bpy.context.view_layer.objects.active=RIG
    dest=OUT/name;dest.mkdir(parents=True,exist_ok=True);export(dest/(name+'.fbx'),False);body.data.calc_loop_triangles()
    return {'name':name,'height_cm':height,'triangles':len(body.data.loop_triangles),'bones':len(RIG.data.bones),'source_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'source_character':source_id,'original_source_vertices':original_count,'retained_original_vertices':preserved_count,'new_accessory_vertices':accessory_count,'preserved_paint_vertex_alpha':0,'garment_vertex_alpha':1,'material_slots':['immutable original body material','new opaque accessory material'],'full_original_body_preserved':name in ['Elia','Malet'],'removed_occluding_head_components':removed,'head_surface_offsets':moved,'head_refinement':HEAD_REFINEMENT,'runtime_world_height':185 if name=='AshWalker' else 165 if name=='BeltScavenger' else None,'face_surface_fitting':'actual source triangle ray intersections with .1cm center protrusion' if HEAD_REFINEMENT else None,'role':{'Elia':'original embroidered dress plus open back cape and side satchel','Malet':'original merchant coat and clear glasses face plus trading pouches and rolled blanket','AshWalker':'original charred body with exposed original void mask, calcified relief and torn back mantle','BeltScavenger':'original leather body and exposed face with salvage pack, shoulder plates and bound greaves'}[name]}

def humanoid_clips(name):
    """Author local transforms directly on the original rig; never retarget mannequin animation."""
    global RIG
    bpy.ops.wm.read_factory_settings(use_empty=True)
    source_id=SOURCES[name];source=ROOT/'Unreal/ArtSource/FieldCharacters'/source_id/(source_id+'_rigged.fbx')
    bpy.ops.import_scene.fbx(filepath=str(source));RIG=next(o for o in bpy.data.objects if o.type=='ARMATURE')
    for ob in list(bpy.data.objects):
        if ob!=RIG:bpy.data.objects.remove(ob,do_unlink=True)
    RIG.scale=(1,1,1);RIG.location=(0,0,0);RIG.rotation_euler=(0,0,0)
    bpy.context.scene.unit_settings.system='METRIC';bpy.context.scene.unit_settings.scale_length=.01
    bpy.ops.object.select_all(action='DESELECT');RIG.select_set(True);bpy.context.view_layer.objects.active=RIG
    dest=OUT/name;dest.mkdir(parents=True,exist_ok=True)
    walker=name=='AshWalker';clips={'Zombie_Idle_Loop' if walker else 'Idle':60,'Zombie_Walk_Fwd_Loop' if walker else 'Walk':30,'Zombie_Scratch' if walker else 'Sword_Regular_A':40,'HitReact_Front_Lgt_01':24,'Death_Front_01':60}
    scene=bpy.context.scene;scene.render.fps=30;records=[]
    for clip,end in clips.items():
        RIG.animation_data_clear();scene.frame_start=0;scene.frame_end=end;zmin=1e9;zmax=-1e9
        for f in range(end+1):
            scene.frame_set(f);t=f/end;phase=TAU*t
            for pb in RIG.pose.bones:pb.matrix_basis=Matrix.Identity(4);pb.rotation_mode='QUATERNION';pb.scale=(1,1,1)
            def rotate(n,axis,angle):
                pb=RIG.pose.bones[n];q=pb.bone.matrix_local.to_quaternion();pb.rotation_quaternion=pb.rotation_quaternion@(q.inverted()@Quaternion(axis,angle)@q)
            def move(n,vec):
                pb=RIG.pose.bones[n];pb.location=pb.bone.matrix_local.to_quaternion().inverted()@Vector(vec)
            # Lower the delivered T-pose into a stable, slightly open stance. The head stays on its neck.
            for side,sign in [('l',1),('r',-1)]:
                rotate('upperarm_'+side,(0,1,0),sign*1.25)
                rotate('lowerarm_'+side,(0,1,0),sign*.12)
            if clip in ['Idle','Zombie_Idle_Loop']:
                rotate('spine_02',(1,0,0),(.075 if walker else .025)+math.sin(phase)*.012)
                rotate('head',(1,0,0),-.03)
                move('pelvis',(0,0,math.sin(phase)*.55))
            elif clip in ['Walk','Zombie_Walk_Fwd_Loop']:
                for side,sign in [('l',1),('r',-1)]:
                    swing=math.sin(phase)*sign
                    rotate('thigh_'+side,(1,0,0),swing*.33)
                    rotate('calf_'+side,(1,0,0),-.42*max(0,-swing))
                    rotate('foot_'+side,(1,0,0),-swing*.13)
                    rotate('upperarm_'+side,(1,0,0),-swing*.23)
                move('pelvis',(0,0,abs(math.sin(phase))*1.2))
                rotate('spine_02',(1,0,0),.07 if walker else .025)
            elif clip in ['Zombie_Scratch','Sword_Regular_A']:
                # Attack reaches its authored apex at the unchanged .55 StrikeAt fraction.
                p=math.sin(math.pi*.5*min(1,t/.55))**2 if t<=.55 else math.cos(math.pi*.5*(t-.55)/.45)**2
                wind=math.sin(math.pi*min(1,t/.35)) if t<.35 else 0
                rotate('spine_02',(1,0,0),.20*p);rotate('spine_03',(0,0,1),(.14 if walker else -.22)*p)
                for side,sign in [('l',1),('r',-1)]:
                    if walker or side=='r':
                        rotate('upperarm_'+side,(1,0,0),-.95*p+.20*wind)
                        rotate('upperarm_'+side,(0,1,0),-sign*.60*p)
                        rotate('lowerarm_'+side,(1,0,0),-.35*p)
                rotate('head',(1,0,0),-.10*p)
                move('pelvis',(0,-3*p,-2*p))
            elif clip=='HitReact_Front_Lgt_01':
                p=math.sin(math.pi*t);move('pelvis',(0,5*p,-3*p))
                rotate('spine_01',(1,0,0),-.10*p);rotate('spine_02',(1,0,0),-.16*p);rotate('head',(1,0,0),.10*p)
                for side in ['l','r']:rotate('upperarm_'+side,(1,0,0),.20*p)
            else:
                p=min(1,t/0.70);p=p*p*(3-2*p)
                # Fall around the pelvis onto the ground; all descendants retain unit local scale.
                rotate('pelvis',(1,0,0),1.30*p);move('pelvis',(0,12*p,-60*p))
                rotate('thigh_l',(1,0,0),-.35*p);rotate('thigh_r',(1,0,0),.20*p)
                rotate('head',(1,0,0),-.18*p)
            bpy.context.view_layer.update();head=RIG.pose.bones['head'].head
            zmin=min(zmin,head.z);zmax=max(zmax,head.z)
            for pb in RIG.pose.bones:
                assert all(abs(x-1)<1e-7 for x in pb.scale),(name,clip,f,pb.name,pb.scale[:])
                pb.keyframe_insert(data_path='location',frame=f);pb.keyframe_insert(data_path='rotation_quaternion',frame=f);pb.keyframe_insert(data_path='scale',frame=f)
        export(dest/(clip+'.fbx'),True)
        records.append({'clip':clip,'source_frames':end,'source_fps':30,'all_77_local_scales':[1,1,1],'head_source_z_range':[zmin,zmax],'duration_policy':'importer resamples normalized keys to immutable original frame rate and frame count','strike_apex':.55 if clip in ['Zombie_Scratch','Sword_Regular_A'] else None})
    (dest/'clips.json').write_text(json.dumps({'name':name,'bones':len(RIG.data.bones),'original_rig_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'clips':records},indent=2),encoding='utf-8')
    print('S356_HUMANOID_CLIPS',name,json.dumps(records),flush=True)

def leech_center(j):
    # Compact arched body: a readable worm at the existing 150-unit height normalization.
    return Vector(([0,8,14,12,6,-3,0][j],33-j*11,[24,44,67,85,73,47,28][j]))

def supernatural(name):
    global RIG,MAT,PARTS,CREATURE
    CREATURE=True;PARTS=[];bpy.ops.wm.read_factory_settings(use_empty=True)
    MAT=bpy.data.materials.new(name+'Regions');MAT.use_nodes=True
    vc=MAT.node_tree.nodes.new('ShaderNodeVertexColor');vc.layer_name='Color';MAT.node_tree.links.new(vc.outputs['Color'],MAT.node_tree.nodes['Principled BSDF'].inputs['Base Color'])
    data=bpy.data.armatures.new(name+'Rig');RIG=bpy.data.objects.new('Armature',data);bpy.context.collection.objects.link(RIG);RIG.select_set(True);bpy.context.view_layer.objects.active=RIG;bpy.ops.object.mode_set(mode='EDIT')
    def bone(n,p,parent):
        b=data.edit_bones.new(n);b.head=p;b.tail=Vector(p)+Vector((0,0,6));b.parent=data.edit_bones.get(parent) if parent else None
    wisp=name=='VoidWisp';bone('root',(0,0,0),None);bone('body',(0,0,70 if wisp else 25),'root')
    if wisp:
        for j in range(6):
            a=TAU*j/6;bone('shard'+str(j),(math.sin(a)*16,math.cos(a)*16,75),'body')
            bone('trail'+str(j),(math.sin(a)*13,math.cos(a)*13,58),'body');bone('trail'+str(j)+'_tip',(math.sin(a)*22,math.cos(a)*22,27),'trail'+str(j))
    else:
        for j in range(7):bone('segment'+str(j),leech_center(j),'body' if j==0 else 'segment'+str(j-1))
        bone('jaw',(0,-44,28),'segment6')
        for s in ['l','r']:bone('palp_'+s,(16 if s=='l' else -16,-36,26),'jaw')
    bpy.ops.object.mode_set(mode='OBJECT')
    if wisp:
        dark=(.025,.045,.07,1);blue=(.10,.31,.44,1);core=(.32,.72,.92,1);edge=(.055,.16,.23,1)
        ell('LivingNucleus',(0,0,77),(14,14,22),0,'body',core,32,20)
        ell('DarkSocket',(0,-13,79),(9,4,11),0,'body',dark,24,16)
        ell('MemoryEye',(0,-16.5,80),(4,1.8,6),0,'body',core,20,12)
        for j in range(6):
            a=TAU*j/6;s,c=math.sin(a),math.cos(a);bn='shard'+str(j)
            # Fractured tapered shell pieces, open gaps expose the luminous nucleus.
            tube('OrbitingShard',[(s*21,c*21,55),(s*27,c*27,73),(s*19,c*19,97),(s*14,c*14,112)],[4,8,5,.15],0,fixed(bn),n=10,flat=.55,color=edge)
            tube('CrystalEdge',[(s*27,c*27,73),(s*24,c*24,91),(s*14,c*14,111)],[1.1,.7,.08],0,fixed(bn),n=7,color=blue)
            ps=[(s*15,c*15,59),(s*22,c*22,40),(s*24+math.cos(a)*9,c*24,22),(s*31,c*29,7)]
            def tw(t,p,j=j):
                q=max(0,min(1,(t-.25)*1.8));return {'trail'+str(j):1-q,'trail'+str(j)+'_tip':q}
            tube('TrailingTendril',ps,[3.4,2.5,1.5,.05],0,tw,n=12,flat=.65,color=blue)
        for s in [-1,1]:tube('CrownProng',[(s*8,0,96),(s*15,1,112),(s*9,3,123)],[3,2,.1],0,fixed('body'),n=12,color=dark)
    else:
        skin=(.10,.035,.075,1);ridge=(.21,.075,.13,1);black=(.012,.006,.015,1);tooth=(.42,.30,.28,1);inner=(.38,.075,.16,1)
        for j in range(7):
            x,y,z=leech_center(j);sz=1-.055*abs(j-3)
            ell('SegmentedBody',(x,y,z),(23*sz,18,18*sz),0,'segment'+str(j),skin,28,18)
            # Repeating fleshy annuli give the elongated body a real contour.
            ps=[(x+math.sin(a)*23.5*sz,y,z+math.cos(a)*18.5*sz) for a in [TAU*i/40 for i in range(41)]]
            tube('SegmentAnnulus',ps,[.65]*41,0,fixed('segment'+str(j)),n=8,color=ridge)
            for s in [-1,1]:tube('GrippingCilium',[(x+s*18*sz,y,z-10),(x+s*27*sz,y-4,max(9,z-22)),(x+s*30*sz,y-9,max(5,z-33))],[2.7,1.8,.1],0,fixed('segment'+str(j)),n=9,color=ridge)
        # Fuse overlapping annelid flesh, so the silhouette is continuous rather than a stack of toys.
        core=[o for o in PARTS if o.name.startswith('SegmentedBody')]
        bpy.ops.object.select_all(action='DESELECT')
        for ob in core:ob.select_set(True);PARTS.remove(ob)
        flesh=core[0];bpy.context.view_layer.objects.active=flesh;bpy.ops.object.join()
        rem=flesh.modifiers.new('ContinuousAnnelid','REMESH');rem.mode='VOXEL';rem.voxel_size=.85;rem.use_smooth_shade=True;bpy.ops.object.modifier_apply(modifier=rem.name)
        sm=flesh.modifiers.new('SoftOrganicBlend','SMOOTH');sm.factor=.65;sm.iterations=4;bpy.ops.object.modifier_apply(modifier=sm.name)
        # Collapse only the fused flesh before assigning UVs/weights; appendages retain their authored detail.
        dec=flesh.modifiers.new('FieldDensity','DECIMATE');dec.ratio=.12;dec.use_collapse_triangulate=True;bpy.ops.object.modifier_apply(modifier=dec.name)
        flesh.vertex_groups.clear()
        for layer in list(flesh.data.uv_layers):flesh.data.uv_layers.remove(layer)
        flesh.data.uv_layers.new(name='UVMap')
        for layer in list(flesh.data.color_attributes):flesh.data.color_attributes.remove(layer)
        region=flesh.data.color_attributes.new(name='Color',type='BYTE_COLOR',domain='CORNER')
        for vertex in flesh.data.vertices:
            ds=sorted([(max(.5,(vertex.co-leech_center(j)).length),j) for j in range(7)])[:2]
            weights=[1/(d*d*d) for d,j in ds];total=sum(weights)
            for (d,j),v in zip(ds,weights):(flesh.vertex_groups.get('segment'+str(j)) or flesh.vertex_groups.new(name='segment'+str(j))).add([vertex.index],v/total,'REPLACE')
        for face in flesh.data.polygons:
            face.use_smooth=True
            for li in face.loop_indices:
                v=flesh.data.vertices[flesh.data.loops[li].vertex_index].co
                u=(math.atan2(v.x,v.z-40)/TAU)%1;vv=(v.y+50)/110
                flesh.data.uv_layers[0].data[li].uv=((.03+u*.94)/4,1-(.03+vv*.94)/4)
                variation=.92+.08*math.sin(v.y*.3+v.z*.2)
                region.data[li].color=tuple(c*variation for c in skin[:3])+(1,)
        PARTS.append(flesh)
        ell('SuckerFace',(0,-48,27),(22,7,21),0,'jaw',black,32,20)
        ps=[(math.sin(a)*21,-54,27+math.cos(a)*20) for a in [TAU*i/48 for i in range(49)]]
        tube('SuckerRim',ps,[3.5]*49,0,fixed('jaw'),n=12,color=inner)
        for j in range(15):
            a=TAU*j/15;s,c=math.sin(a),math.cos(a)
            tube('HookedSuckerTooth',[(s*17,-55,27+c*16),(s*12,-59,27+c*12),(s*9,-57,27+c*8)],[1.5,1,.08],0,fixed('jaw'),n=8,color=tooth)
        for s,side in [(1,'l'),(-1,'r')]:
            tube('SensoryPalp',[(s*15,-39,31),(s*28,-50,37),(s*31,-65,34),(s*25,-76,26)],[3.1,2.3,1.2,.1],0,fixed('palp_'+side),n=14,color=ridge)
            for j in range(2):ell('MemorySac',leech_center(2+j*2)+Vector((s*20,0,2)),(5,8,7),0,'segment'+str(2+j*2),inner,20,12)
    body,report=join_export(name)
    dimensions=[max(v.co[i] for v in body.data.vertices)-min(v.co[i] for v in body.data.vertices) for i in range(3)]
    world_height=135 if wisp else 150
    normalized=[v*world_height/dimensions[2] for v in dimensions]
    report['bounds_cm']=dict(zip(['width','length','height'],dimensions))
    report['normalized_at_existing_world_height']=dict(zip(['width','length','height'],normalized))
    if not wisp:
        assert normalized[1]<=225 and normalized[0]<=130,(name,normalized)
        assert report['triangles']<=25000,(name,report['triangles'])
    scene=bpy.context.scene;scene.render.fps=30
    clips={'Idle':60,'Walk':30,'Cast' if wisp else 'Bite':24,'HitReact_Front_Lgt_01':15,'Death_Front_01':40}
    for clip,end in clips.items():
        RIG.animation_data_clear();scene.frame_start=0;scene.frame_end=end
        for f in range(end+1):
            scene.frame_set(f);t=f/end
            for pb in RIG.pose.bones:pb.matrix_basis=Matrix.Identity(4);pb.rotation_mode='QUATERNION'
            def rotate(n,axis,angle):
                pb=RIG.pose.bones[n];q=pb.bone.matrix_local.to_quaternion();pb.rotation_quaternion=q.inverted()@Quaternion(axis,angle)@q
            def move(n,vec):
                pb=RIG.pose.bones[n];pb.location=pb.bone.matrix_local.to_quaternion().inverted()@Vector(vec)
            phase=TAU*t
            if clip in ['Idle','Walk']:
                move('body',(0,0,math.sin(phase)*(3.5 if wisp else .8)))
                if wisp:
                    for j in range(6):
                        rotate('shard'+str(j),(0,0,1),math.sin(phase+j)*.08)
                        rotate('trail'+str(j),(1,0,0),math.sin(phase+j)*.20);rotate('trail'+str(j)+'_tip',(0,1,0),math.sin(phase+j+.7)*.25)
                else:
                    for j in range(7):rotate('segment'+str(j),(0,0,1),math.sin(phase-j*.7)*(.12 if clip=='Walk' else .025))
            elif clip in ['Cast','Bite']:
                p=math.sin(math.pi*t)**2;move('body',(0,-10*p,5*p if wisp else -2*p))
                if wisp:
                    for j in range(6):rotate('shard'+str(j),(0,0,1),.40*p);rotate('trail'+str(j),(1,0,0),-.5*p)
                else:rotate('jaw',(1,0,0),-.30*p);rotate('palp_l',(0,0,1),-.35*p);rotate('palp_r',(0,0,1),.35*p)
            elif clip=='HitReact_Front_Lgt_01':
                p=math.sin(math.pi*t);move('body',(0,8*p,-4*p));rotate('body',(1,0,0),.13*p)
            else:
                p=min(1,t*1.6);p=p*p*(3-2*p);move('body',(0,0,-50*p if wisp else -10*p));rotate('body',(0,1,0),.7*p)
                if wisp:
                    for j in range(6):rotate('trail'+str(j),(1,0,0),.45*p)
                else:
                    for j in range(7):rotate('segment'+str(j),(0,0,1),.18*p)
            for pb in RIG.pose.bones:
                pb.keyframe_insert(data_path='location',frame=f);pb.keyframe_insert(data_path='rotation_quaternion',frame=f);pb.keyframe_insert(data_path='scale',frame=f)
        export(OUT/name/(clip+'.fbx'),True)
    report['clips']=clips;report['role']='floating fractured nucleus with six articulated trails' if wisp else 'segmented annelid with sucker jaw, hooked teeth and sensory palps'
    return report

args=sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else []
reports=[]
if args==['Heads']:
    HEAD_REFINEMENT=True;OUT=ROOT/'Unreal/ArtSource/CharacterUpgradeS356/HeadRefinement'
    assert not any((OUT/name/(name+'.fbx')).exists() for name in ['AshWalker','BeltScavenger']),'HeadRefinement sources already exist; use fresh revision'
    OUT.mkdir(parents=True,exist_ok=True);args=['AshWalker','BeltScavenger']
if args==['Clips']:
    for name in ['AshWalker','BeltScavenger']:humanoid_clips(name)
    print('S356_HUMANOID_CLIPS_COMPLETE 10',flush=True)
    sys.exit(0)
assert HEAD_REFINEMENT or not any(name in ['AshWalker','BeltScavenger'] for name in args),'Use -- Heads for final encounter meshes'
for name in args or ['Elia','Malet','VoidWisp','MemoryLeech']:
    report=humanoid(name) if name in SOURCES else supernatural(name)
    report['authoring_helper_sha256']=hashlib.sha256((ROOT/'Unreal/Tools/build_character_geometry_s354.py').read_bytes()).hexdigest()
    (OUT/name/'geometry.json').write_text(json.dumps(report,indent=2),encoding='utf-8');reports.append(report)
    print('S356_CHARACTER',json.dumps(report),flush=True)
if not args:
    for name in ['AshWalker','BeltScavenger']:humanoid_clips(name)
print('S356_CHARACTER_AUTHORING_COMPLETE',len(reports),flush=True)