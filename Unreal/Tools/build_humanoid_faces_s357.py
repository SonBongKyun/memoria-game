"""S357 immutable humanoid facial sculpt. Blender 4.5 background entry point.

No image pixels are authored here. Body coordinates, UV corners, weights and all
77 bind matrices are retained; only old head-weighted connected parts are replaced.
Outputs are new R4 FBX/JSON files, never overwrites. Native captures remain required.
"""
from pathlib import Path
from collections import Counter
import hashlib, json, math, sys
import bpy, bmesh
from mathutils import Vector, Matrix
from mathutils.bvhtree import BVHTree

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'Unreal/ArtSource/HumanoidFacesS357/R4'
NAMES = ('AshWalker', 'BeltScavenger')
TAU = math.tau
PARTS, COMPONENTS = [], []
RIG = FACE_MAT = None
IS_ASH = False
HZ = CY = TOP = 0.0
PROFILE = []
FACE_BVH = None
EYE_BOUNDARIES = {}
CLEARANCE_REPORTS = []

def clamp(x, lo=0.0, hi=1.0): return max(lo, min(hi, x))
def gauss(x, z, cx, cz, sx, sz): return math.exp(-((x-cx)/sx)**2-((z-cz)/sz)**2)
def digest(p): return hashlib.sha256(p.read_bytes()).hexdigest()
def bounds(vs): return [[float(min(v[k] for v in vs)), float(max(v[k] for v in vs))] for k in range(3)]
def weights(ob, v): return {ob.vertex_groups[g.group].name: float(g.weight) for g in v.groups if g.weight > 1e-7}

def connected(ob):
    adj = [set() for _ in ob.data.vertices]
    for e in ob.data.edges:
        a,b=e.vertices; adj[a].add(b); adj[b].add(a)
    seen=set(); result=[]
    for start in range(len(adj)):
        if start in seen: continue
        group={start}; stack=[start]; seen.add(start)
        while stack:
            for v in adj[stack.pop()]-seen: seen.add(v); group.add(v); stack.append(v)
        result.append(group)
    return result

def immutable_corners(ob, vertex_limit=None):
    rows=[]; uv=ob.data.uv_layers.active
    for f in ob.data.polygons:
        if vertex_limit is not None and any(i>=vertex_limit for i in f.vertices): continue
        for li in f.loop_indices:
            v=ob.data.vertices[ob.data.loops[li].vertex_index]
            rows.append((tuple(round(float(x),6) for x in v.co), tuple(round(float(x),7) for x in uv.data[li].uv), tuple(sorted((n,round(w,7)) for n,w in weights(ob,v).items()))))
    return hashlib.sha256(repr(sorted(rows)).encode()).hexdigest(),len(rows)

def lerp_profile(z):
    if z<=PROFILE[0][0]: return PROFILE[0][1:]
    if z>=PROFILE[-1][0]: return PROFILE[-1][1:]
    for i in range(len(PROFILE)-1):
        a,b=PROFILE[i:i+2]
        if a[0]<=z<=b[0]:
            t=(z-a[0])/(b[0]-a[0]); out=[]
            for k in range(1,4):
                # Nonuniform Hermite tangents retain a deliberate jaw/cranial profile.
                p=PROFILE[max(0,i-1)]; n=PROFILE[min(len(PROFILE)-1,i+2)]
                m0=(b[k]-p[k])/(b[0]-p[0]); m1=(n[k]-a[k])/(n[0]-a[0])
                value=(2*t**3-3*t*t+1)*a[k]+(t**3-2*t*t+t)*m0*(b[0]-a[0])+(-2*t**3+3*t*t)*b[k]+(t**3-t*t)*m1*(b[0]-a[0])
                out.append(max(.012,value) if k==1 else value)
            return out
    raise AssertionError(z)

def sculpt(x,z):
    q=z-HZ
    if IS_ASH:
        v=-.7*gauss(x,q,0,15,.9,4.3)-.55*gauss(x,q,0,8,2.1,3.8)
        for s in (-1,1):
            v+=1.25*gauss(x,q,s*3.55,14.1,2.0,1.45)
            v-=1.0*gauss(x,q,s*3.7,16.1,2.65,.75)
            v-=.8*gauss(x,q,s*5.5,10.5,1.8,2.3)
            v+=.65*gauss(x,q,s*3.7,7.8,1.35,2.0)
        # Quiet irregular erosion, with actual modeled jaw and orbital planes.
        v+=.12*math.sin(x*1.2+q*.68)*math.sin(q*.92)
        return v
    v=-.84*gauss(x,q,0,8.25,.67,2.4)-.96*gauss(x,q,0,6.45,1.10,1.05)
    for s in (-1,1):
        v+=.76*gauss(x,q,s*3.0,9.15,2.0,1.25)
        v-=.78*gauss(x,q,s*3.2,10.9,2.3,.65)
        v-=.67*gauss(x,q,s*4.7,7.1,1.8,1.55)
        v-=.77*gauss(x,q,s*1.0,6.0,.58,.51)
        v+=.43*gauss(x,q,s*.88,5.65,.3,.2)
        v+=.22*gauss(x,q,s*2.45,4.8,.22,1.1)
    v+=.16*gauss(x,q,0,4.83,.3,.52)
    v-=.48*gauss(x,q,0,4.20,1.93,.29)
    v-=.70*gauss(x,q,0,3.54,1.8,.39)
    v+=.28*gauss(x,q,0,3.89,2.05,.12)
    v-=.55*gauss(x,q,0,1.86,2.6,.88)
    # Glabella and forehead crease break the original smooth toy oval.
    v+=.12*gauss(x,q,.34,11.8,.15,.85)+.10*gauss(x,q,-.42,11.8,.16,.7)
    return v

def surface_y(x,z):
    rx,front,back=lerp_profile(z); t=clamp(abs(x)/rx)
    return (front+back)*.5-(back-front)*.5*math.sqrt(max(0,1-t*t))+sculpt(x,z)*(1-t*t)**1.25

def face_color(x,z):
    q=z-HZ
    if IS_ASH:
        weather=.82+.14*math.sin(x*.8+q*.72)*math.sin(q*.4)
        if q<1: return (.024,.029,.028,.75)
        return (.34*weather,.30*weather,.235*weather,.25)
    weather=.96+.035*math.sin(x*3.1+q*2.3)*math.sin(q*1.7)
    beard=clamp((5.1-q)/2.7)*clamp((q+.6)/1.7)
    redness=.018*(gauss(x,q,-4.5,6.8,2.0,1.8)+gauss(x,q,4.5,6.8,2.0,1.8))
    return (.30*weather*(1-beard*.48)+redness,.195*weather*(1-beard*.48),.135*weather*(1-beard*.46),0)

def head_weight(p):
    t=clamp((p[2]-(HZ-7.7))/9.7)
    return {'neck_01':1-t,'head':t}

def add_mesh(label,vs,fs,uvs=None,colors=None,weight=None,smooth=True,front_facing=False):
    me=bpy.data.meshes.new(label); me.from_pydata(vs,[],fs); me.update()
    ob=bpy.data.objects.new(label,me); bpy.context.collection.objects.link(ob); me.materials.append(FACE_MAT)
    # Adding CustomData layers can reallocate prior RNA storage. Re-fetch AFTER allocation.
    me.uv_layers.new(name='UVMap'); me.color_attributes.new(name='Color',type='FLOAT_COLOR',domain='CORNER')
    uv=me.uv_layers['UVMap']; col=me.color_attributes['Color']
    for f in me.polygons:
        f.use_smooth=smooth
        for li in f.loop_indices:
            vi=me.loops[li].vertex_index; p=me.vertices[vi].co
            uv.data[li].uv=uvs[vi] if uvs else (clamp(.5+p.x/22),clamp((p.z-HZ+10)/36))
            col.data[li].color=colors[vi] if colors else face_color(p.x,p.z)
    for v in me.vertices:
        ws=weight(v.co) if weight else {'head':1}
        for n,w in ws.items():
            if w>1e-7:
                g=ob.vertex_groups.get(n) or ob.vertex_groups.new(name=n); g.add([v.index],w,'REPLACE')
    # New sculpt normals only. Preserved body is never rewelded/recomputed.
    bm=bmesh.new(); bm.from_mesh(me); bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces))
    bm.normal_update()
    if front_facing and sum(f.normal.y*f.calc_area() for f in bm.faces)>0:
        bmesh.ops.reverse_faces(bm,faces=list(bm.faces))
    bm.to_mesh(me); bm.free()
    assert_color_contract(me)
    # Validate actual stored corner semantics after topology/normal processing.
    for f in me.polygons:
        for li in f.loop_indices:
            vi=me.loops[li].vertex_index; p=me.vertices[vi].co
            expected_uv=uvs[vi] if uvs else (clamp(.5+p.x/22),clamp((p.z-HZ+10)/36))
            expected_color=colors[vi] if colors else face_color(p.x,p.z)
            assert max(abs(a-b) for a,b in zip(me.uv_layers['UVMap'].data[li].uv,expected_uv))<.000002
            assert max(abs(a-b) for a,b in zip(me.color_attributes['Color'].data[li].color,expected_color))<.000002
    PARTS.append(ob); me.calc_loop_triangles()
    COMPONENTS.append({'name':label,'vertices':len(me.vertices),'triangles':len(me.loop_triangles),'bounds_cm':bounds([v.co for v in me.vertices]),'material_index':2})
    return ob

def eye_shape(x,z,sign,scale=1):
    cx=sign*(3.55 if IS_ASH else 3.0); cz=HZ+(14.1 if IS_ASH else 9.15)
    rx=1.78 if IS_ASH else 1.65; rz=.93 if IS_ASH else .55
    tilt=sign*(.13 if IS_ASH else .10)
    xx=(x-cx)/(rx*scale); zz=(z-cz-tilt*(x-cx))/(rz*scale)
    return xx*xx+zz*zz

def create_cranium():
    global FACE_BVH, EYE_BOUNDARIES
    vs=[]; fs=[]; uv=[]; colors=[]; nz=76 if IS_ASH else 72; ns=96; low=PROFILE[0][0]; high=PROFILE[-1][0]
    for j in range(nz+1):
        z=low+(high-low)*j/nz; rx,front,back=lerp_profile(z)
        for i in range(ns):
            a=TAU*i/ns; x=rx*math.sin(a); c=math.cos(a)
            y=(front+back)*.5-(back-front)*.5*c
            if c>0: y+=sculpt(x,z)*c**2.5
            if IS_ASH: y+=.06*math.sin(a*11+z*.73)*math.sin(a*7-z*.42)
            vs.append((x,y,z)); uv.append((i/ns,j/nz)); colors.append(face_color(x,z))
    for j in range(nz):
        for i in range(ns):
            a=j*ns+i; b=j*ns+(i+1)%ns; face=(a,b,b+ns,a+ns)
            points=[Vector(vs[k]) for k in face]; p=sum(points,Vector())/4
            # Actual opening; fitted annular lids conceal its deliberately small grid boundary.
            front=math.cos(TAU*(i+.5)/ns)>.35
            if front and any(eye_shape(v.x,v.z,s,1.02)<1 for v in points+[p] for s in (-1,1)): continue
            fs.append(face)
    fs.append(tuple(range(ns-1,-1,-1))); fs.append(tuple(nz*ns+i for i in range(ns)))
    base=add_mesh('ContinuousNeckCraniofacialSculpt',vs,fs,uv,colors,head_weight)
    # Freeze the actual exported triangulation before any feature is fitted to it.
    bpy.context.view_layer.objects.active=base
    tri=base.modifiers.new('ExactFacialTriangles','TRIANGULATE'); bpy.ops.object.modifier_apply(modifier=tri.name)
    me=base.data; me.calc_loop_triangles()
    FACE_BVH=BVHTree.FromPolygons([v.co.copy() for v in me.vertices],[tuple(t.vertices) for t in me.loop_triangles],all_triangles=True)
    edges=Counter(tuple(sorted((a,b))) for f in me.polygons for a,b in zip(f.vertices,tuple(f.vertices[1:])+tuple(f.vertices[:1])))
    adjacent={}
    for (a,b),count in edges.items():
        if count==1: adjacent.setdefault(a,set()).add(b); adjacent.setdefault(b,set()).add(a)
    assert adjacent and all(len(v)==2 for v in adjacent.values()),'Non-loop facial opening'
    seen=set(); EYE_BOUNDARIES={}
    for start in adjacent:
        if start in seen: continue
        loop=[]; previous=None; current=start
        while current not in seen:
            seen.add(current); loop.append(current)
            following=next(v for v in adjacent[current] if v!=previous)
            previous,current=current,following
        assert current==start and len(loop)>=8
        points=[me.vertices[i].co.copy() for i in loop]
        sign=-1 if sum(p.x for p in points)<0 else 1
        assert sign not in EYE_BOUNDARIES
        EYE_BOUNDARIES[sign]=points
    assert set(EYE_BOUNDARIES)=={-1,1}

def orbital_features(sign):
    cx=sign*(3.55 if IS_ASH else 3.0); cz=HZ+(14.1 if IS_ASH else 9.15)
    rx=1.78 if IS_ASH else 1.65; rz=.93 if IS_ASH else .55; tilt=sign*(.13 if IS_ASH else .1)
    # Stitch the actual cut edge, including its irregular grid corners, to the lid.
    # The outer ring duplicates exact exported skin boundary positions, so no guess
    # about an ellipse's outer radius can leave a triangle-shaped gap.
    boundary=EYE_BOUNDARIES[sign]; n=len(boundary); vs=[]; fs=[]; colors=[]
    stages=[0,.22,.48,.74,1]
    for j,t in enumerate(stages):
        for outer in boundary:
            a=math.atan2((outer.z-cz-tilt*(outer.x-cx))/rz,(outer.x-cx)/rx)
            dx=rx*.86*math.cos(a); ix=cx+dx; iz=cz+rz*.86*math.sin(a)+tilt*dx
            x=ix+(outer.x-ix)*t; z=iz+(outer.z-iz)*t
            base=surface_y(x,z)+(outer.y-surface_y(outer.x,outer.z))*t
            relief=([.03,.15,.20,.12,0][j])*(1.5 if IS_ASH else .65)
            p=outer.copy() if j==len(stages)-1 else Vector((x,base-relief,z))
            vs.append(p); c=face_color(x,z)
            shade=([.47,.68,.93,1.01,1] if IS_ASH else [.78,.88,.99,1.01,1])[j]
            colors.append(tuple(v*shade for v in c[:3])+(c[3],))
    for j in range(len(stages)-1):
        for i in range(n): a=j*n+i; b=j*n+(i+1)%n; fs.append((a,b,b+n,a+n))
    add_mesh(('HollowOrbitalRim' if IS_ASH else 'AnatomicalEyelid')+str(sign),vs,fs,colors=colors,front_facing=True)
    COMPONENTS[-1]['exact_stitched_skin_boundary_vertices']=n
    assert max((Vector(vs[(len(stages)-1)*n+i])-p).length for i,p in enumerate(boundary))==0
    # Recessed curved almond surface; no detached eyeball sphere.
    vs=[]; fs=[]; colors=[]; rings=8; n=48
    for j in range(rings+1):
        r=.001+.889*j/rings
        for i in range(n):
            a=TAU*i/n; dx=rx*r*math.cos(a); x=cx+dx; z=cz+rz*r*math.sin(a)+tilt*dx
            depth=(.82*(1-r*r)+.03) if IS_ASH else -.105*(1-r*r)+.035
            vs.append((x,surface_y(x,z)+depth,z))
            if IS_ASH: c=(.008+.006*r,.012+.006*r,.013+.006*r,.25)
            else:
                iris=math.sqrt((dx/.43)**2+((z-cz)/.45)**2)
                c=(.015,.020,.012,0) if iris<.42 else ((.065,.071,.036,0) if iris<1 else (.40,.36,.285,0))
            colors.append(c)
    for j in range(rings):
        for i in range(n): a=j*n+i; b=j*n+(i+1)%n; fs.append((a,b,b+n,a+n))
    # Close the innermost finite-radius ring with a real surface triangle fan.
    # Its center is sampled by the orbital seal check, so a pinhole cannot pass.
    center=len(vs); depth=.85 if IS_ASH else -.07
    vs.append((cx,surface_y(cx,cz)+depth,cz))
    colors.append((.008,.012,.013,.25) if IS_ASH else (.015,.020,.012,0))
    for i in range(n): fs.append((center,i,(i+1)%n))
    add_mesh(('RecessedEmptySocket' if IS_ASH else 'FittedEyeIrisPupil')+str(sign),vs,fs,colors=colors,front_facing=True)

def refresh_face_bvh():
    global FACE_BVH
    vs=[];fs=[]
    for ob in PARTS[1:]:
        start=len(vs); vs.extend(v.co.copy() for v in ob.data.vertices); ob.data.calc_loop_triangles()
        fs.extend(tuple(start+i for i in t.vertices) for t in ob.data.loop_triangles)
    FACE_BVH=BVHTree.FromPolygons(vs,fs,all_triangles=True)

def actual_face_y(x,z):
    p,normal,index,distance=FACE_BVH.ray_cast(Vector((x,-100,z)),Vector((0,1,0)))
    assert p is not None and p.y<CY+5,('Missing frontal facial surface',x,z,None if p is None else p.y)
    return p.y

def fit_clearance(label,vs,fs,minimum=.025):
    # Every fixed triangle is checked at vertices, edge quarters/midpoints and
    # centroid. A small bounded lift covers creases crossed between endpoint rays.
    samples=[(1,0,0),(0,1,0),(0,0,1),(1/3,1/3,1/3)]
    for a,b in ((0,1),(1,2),(2,0)):
        for t in (.25,.5,.75):
            w=[0,0,0];w[a]=1-t;w[b]=t;samples.append(tuple(w))
    original=[Vector(v) for v in vs]; vs=[v.copy() for v in original]
    for iteration in range(8):
        lifts=[0.0]*len(vs)
        for f in fs:
            low=min(actual_face_y(p.x,p.z)-p.y for w in samples for p in [sum((vs[i]*a for i,a in zip(f,w)),Vector())])
            if low<minimum:
                for i in f: lifts[i]=max(lifts[i],minimum-low+.003)
        if max(lifts)==0: break
        for v,lift in zip(vs,lifts): v.y-=lift
    min_clearance=min(actual_face_y(p.x,p.z)-p.y for f in fs for w in samples for p in [sum((vs[i]*a for i,a in zip(f,w)),Vector())])
    max_lift=max(a.y-b.y for a,b in zip(original,vs))
    assert min_clearance>=minimum-.00001,(label,min_clearance)
    assert max_lift<.38,(label,'Unexpected excessive lift',max_lift)
    CLEARANCE_REPORTS.append({'feature':label,'samples_per_triangle':len(samples),'triangles':len(fs),'min_surface_clearance_cm':min_clearance,'additional_max_lift_cm':max_lift})
    return vs

def surface_ribbon(label,points,width,color,raised=.035):
    # Fine feature ribbons follow the actual exported triangles, without tubes.
    ps=[]
    for a,b in zip(points,points[1:]):
        a,b=Vector(a),Vector(b)
        for i in range(8): ps.append(a.lerp(b,i/8))
    ps.append(Vector(points[-1])); vs=[]; fs=[]; cs=[]
    for i,p in enumerate(ps):
        d=(ps[min(i+1,len(ps)-1)]-ps[max(0,i-1)]).normalized(); side=Vector((-d.y,d.x))
        taper=.25+.75*math.sin(math.pi*i/(len(ps)-1))**.4
        for sign in (-1,1):
            x,z=p+side*width*sign*taper; vs.append(Vector((x,actual_face_y(x,z)-raised,z))); cs.append(color)
    for i in range(len(ps)-1): a=2*i; fs.extend([(a,a+1,a+3),(a,a+3,a+2)])
    vs=fit_clearance(label,vs,fs)
    add_mesh(label,vs,fs,colors=cs,front_facing=True)

def validate_eye_seals():
    misses=[];count=0
    for sign in (-1,1):
        cx=sign*(3.55 if IS_ASH else 3); cz=HZ+(14.1 if IS_ASH else 9.15)
        rx=1.78 if IS_ASH else 1.65; rz=.93 if IS_ASH else .55
        for i in range(101):
            for j in range(61):
                x=cx+rx*2*(i/50-1); z=cz+rz*2*(j/30-1)
                if eye_shape(x,z,sign,1.8)>1: continue
                p,n,idx,d=FACE_BVH.ray_cast(Vector((x,-100,z)),Vector((0,1,0))); count+=1
                if p is None or p.y-surface_y(x,z)>2: misses.append((x,z))
    assert not misses,('Unsealed orbital surface',len(misses),misses[:5])
    return {'front_ray_samples':count,'hole_samples':0,'stitched_boundary_vertices':{str(k):len(v) for k,v in EYE_BOUNDARIES.items()}}

def color_corner_digest(ob):
    me=ob.data; color=me.color_attributes['Color'];uv=me.uv_layers.active
    rows=[]
    for f in me.polygons:
        for li in f.loop_indices:
            v=me.vertices[me.loops[li].vertex_index]
            rows.append((tuple(round(float(x),6) for x in v.co),tuple(round(float(x),7) for x in uv.data[li].uv),tuple(round(float(x),7) for x in color.data[li].color)))
    return hashlib.sha256(repr(sorted(rows)).encode()).hexdigest()

def weld_orbital_seams():
    global PARTS
    base=next(o for o in PARTS if o.name=='ContinuousNeckCraniofacialSculpt')
    chosen=[base]+[o for o in PARTS if o.name.startswith(('HollowOrbitalRim','AnatomicalEyelid'))]
    assert len(chosen)==3
    remaining=[o for o in PARTS if o not in chosen]
    bpy.ops.object.select_all(action='DESELECT')
    for ob in chosen: ob.select_set(True)
    bpy.context.view_layer.objects.active=base; bpy.ops.object.join()
    geometry_before=immutable_corners(base); colors_before=color_corner_digest(base)
    bm=bmesh.new(); bm.from_mesh(base.data); bm.verts.ensure_lookup_table()
    before=len(bm.verts); lookup={}
    for v in bm.verts: lookup.setdefault(tuple(round(float(x),6) for x in v.co),[]).append(v)
    targetmap={}
    for p in [p for loop in EYE_BOUNDARIES.values() for p in loop]:
        pair=lookup[tuple(round(float(x),6) for x in p)]
        assert len(pair)==2,('Unexpected orbital seam multiplicity',len(pair),list(p))
        keep,drop=sorted(pair,key=lambda v:v.index)
        assert (keep.co-drop.co).length<.000001
        targetmap[drop]=keep
    bmesh.ops.weld_verts(bm,targetmap=targetmap)
    for face in bm.faces: face.smooth=True
    bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces)); bm.normal_update(); bm.to_mesh(base.data); bm.free()
    base.data.update()
    assert before-len(base.data.vertices)==len(targetmap)
    assert immutable_corners(base)==geometry_before,'Seam weld changed geometry, UV or weights'
    assert color_corner_digest(base)==colors_before,'Seam weld changed corner colors'
    assert_color_contract(base.data)
    # This intermediate face-only mesh has no inherited original body normal data.
    base.data.normals_split_custom_set([(0,0,0)]*len(base.data.loops))
    PARTS=[remaining[0],base]+remaining[1:]
    return {'shared_boundary_vertices':len(targetmap),'removed_duplicate_vertices':before-len(base.data.vertices),'geometry_uv_weight_corner_sha256':geometry_before[0],'color_corner_sha256':colors_before,'scope':'Only exact cranium/lid outer-boundary pairs; original body and other face parts excluded'}

def restore_body_and_smooth_face_normals(body,body_normals,body_vertices):
    me=body.data;count=len(body_normals)
    # Obtain actual automatic corner normals from the now-connected face mesh.
    # Original FBX split normals are restored explicitly for every retained body loop.
    me.normals_split_custom_set([(0,0,0)]*len(me.loops))
    automatic=[tuple(n.vector) for n in me.corner_normals]
    me.normals_split_custom_set(body_normals+automatic[count:]);me.update()
    actual=[Vector(n.vector) for n in me.corner_normals]
    body_delta=max((a-Vector(b)).length for a,b in zip(actual[:count],body_normals))
    assert body_delta<.0001,('Original body split normals changed',body_delta)
    lookup={}
    for v in me.vertices:
        if v.index>=body_vertices:lookup.setdefault(tuple(round(float(x),6) for x in v.co),[]).append(v.index)
    boundary_indices=[]
    for p in [p for loop in EYE_BOUNDARIES.values() for p in loop]:
        indices=lookup[tuple(round(float(x),6) for x in p)]
        assert len(indices)==1,('Unwelded face seam',len(indices),list(p))
        boundary_indices.append(indices[0])
    normals={i:[] for i in boundary_indices}
    for f in me.polygons:
        if f.material_index!=2:continue
        for li in f.loop_indices:
            vi=me.loops[li].vertex_index
            if vi in normals:normals[vi].append(actual[li])
    assert all(len(v)>=3 for v in normals.values())
    seam_delta=max((n-v[0]).length for v in normals.values() for n in v)
    assert seam_delta<.0001,('Discontinuous orbital corner normals',seam_delta)
    return {'original_body_loop_count':count,'original_body_max_normal_delta':body_delta,'shared_boundary_vertices':len(normals),'max_boundary_corner_normal_delta':seam_delta,'new_face_policy':'Automatic smooth normals from welded face topology; flat crown faces retained'}

def human_details():
    for sign in (-1,1):
        points=[(sign*1.45,HZ+10.35),(sign*2.5,HZ+10.9),(sign*3.8,HZ+10.8),(sign*4.85,HZ+10.4)]
        surface_ribbon('TaperedEyebrow'+str(sign),points,.20,(.035,.025,.017,0),.22)
        surface_ribbon('NostrilRecess'+str(sign),[(sign*.40,HZ+5.68),(sign*.8,HZ+5.56),(sign*1.13,HZ+5.82)],.105,(.049,.026,.018,0),.012)
        surface_ribbon('NasolabialCrease'+str(sign),[(sign*1.8,HZ+5.4),(sign*2.28,HZ+4.6),(sign*2.34,HZ+3.9)],.032,(.17,.095,.06,0),.018)
    surface_ribbon('CompressedLipSeam',[(-2.0,HZ+3.95),(-.8,HZ+3.94),(0,HZ+3.88),(.85,HZ+3.94),(2.0,HZ+3.92)],.075,(.074,.032,.020,0),.045)
    surface_ribbon('LowerLipVermilion',[(-1.6,HZ+3.56),(0,HZ+3.46),(1.6,HZ+3.56)],.15,(.26,.12,.09,0),.033)
    surface_ribbon('OldCheekScar',[(-4.45,HZ+9.9),(-4.75,HZ+8.6),(-4.53,HZ+7.15),(-4.15,HZ+6.25)],.07,(.38,.26,.18,0),.055)
    for j in range(3):
        z=HZ+8.9-j*.8
        surface_ribbon('ScarSuture'+str(j),[(-5.0,z),(-4.48,z+.16)],.03,(.19,.11,.07,0),.058)
    # Ear bowl, antihelix and lobe share one continuous parameterized surface.
    for sign in (-1,1):
        vs=[]; fs=[]; colors=[]; nr=10; n=40
        for j in range(nr+1):
            r=.001+j/nr
            for i in range(n):
                a=TAU*i/n; x=sign*(7.0+1.18*r*math.cos(a)); z=HZ+8.3+2.5*r*math.sin(a)
                y=5.0+.65*r*math.cos(a)+.85*(1-r*r)-.48*math.exp(-((r-.8)/.16)**2)
                vs.append((x,y,z)); colors.append((.26-.065*(1-r),.15-.04*(1-r),.10-.03*(1-r),0))
        for j in range(nr):
            for i in range(n): a=j*n+i; b=j*n+(i+1)%n; fs.append((a,b,b+n,a+n))
        add_mesh('SculptedEar'+str(sign),vs,fs,colors=colors,front_facing=True)
    # Close-cropped scalp with a receded, irregular hairline, not an added floating cap.
    vs=[]; fs=[]; colors=[]; ns=96; rows=24
    for j in range(rows+1):
        for i in range(ns):
            a=TAU*i/ns; front=max(0,math.cos(a)); bottom=HZ+5.8+8.2*front**2+1.4*math.sin(a*2)**2
            scalp_t=1-(1-j/rows)**1.6
            z=bottom+(TOP-.035-bottom)*scalp_t; rx,yf,yb=lerp_profile(z)
            # Dense fitted shell remains outside the sculpt at its chord midpoints.
            radius=.20*(1-j/rows)**.22+.035; x=(rx+radius)*math.sin(a)
            y=(yf+yb)*.5-((yb-yf)*.5+radius)*math.cos(a)
            if math.cos(a)>0: y+=sculpt(x,z)*math.cos(a)**2.5
            vs.append((x,y,z)); t=.85+.15*math.sin(a*23+z*5)
            fade=math.exp(-j*.72)
            colors.append(((.022+.057*fade)*t,(.016+.036*fade)*t,(.011+.025*fade)*t,0))
    for j in range(rows):
        for i in range(ns): a=j*ns+i; b=j*ns+(i+1)%ns; fs.append((a,b,b+ns,a+ns))
    # Fully closed scalp; the underlying skin pole is recessed 0.12 cm below it.
    apex=len(vs); vs.append((0,CY,TOP)); colors.append((.022,.016,.011,0))
    for i in range(ns): fs.append((rows*ns+i,rows*ns+(i+1)%ns,apex))
    add_mesh('FittedCroppedHair',vs,fs,colors=colors)

def ash_details():
    cracks=[[(0.9,22.2),(.4,20.4),(1.2,18.9),(.4,17.4),(.6,15.9)], [(-5.8,19.9),(-4.7,18.1),(-5.3,16.8),(-4.7,15.8)], [(4.8,12.8),(5.6,11.6),(4.4,10.8),(4.9,8.8),(3.5,7.7)], [(-4.1,11.5),(-3.3,9.7),(-3.6,7.8),(-2.2,6.0)], [(1.8,9.8),(1.0,8.1),(1.7,6.6),(.8,4.8)], [(-.8,3.1),(-1.4,4.7),(-.7,5.8)]]
    for j,points in enumerate(cracks):
        ps=[(x,HZ+z) for x,z in points]
        surface_ribbon('CalcifiedFissure'+str(j),ps,.07 if j%2 else .10,(.018,.021,.019,.25),.04)
        surface_ribbon('FissureWornEdge'+str(j),[(x+.10,z) for x,z in ps],.024,(.46,.40,.30,.25),.057)
    # Four rooted charred horn/crown prongs. Every root begins within the skull.
    for sign in (-1,1):
        for back in (False,True):
            x=sign*(6.35 if not back else 5.0); z=HZ+(18.0 if not back else 19.0)
            sy=surface_y(x,z)+.65 if not back else CY+4.3
            endz=TOP if not back else TOP-2.6
            points=[Vector((x,sy,z)),Vector((sign*8.5,sy+.9,z+3)),Vector((sign*9.0,CY+1.5,endz-1.8)),Vector((sign*7.7,CY+.9,endz))]
            radii=[1.55,1.05,.52,0]; vs=[]; fs=[]; colors=[]; n=12
            for j,(p,r) in enumerate(zip(points,radii)):
                d=(points[min(j+1,3)]-points[max(0,j-1)]).normalized(); a=d.cross(Vector((0,1,0))).normalized(); b=d.cross(a)
                for i in range(n):
                    angle=TAU*i/n; q=p+(a*math.cos(angle)+b*math.sin(angle))*r
                    q.z=min(TOP,q.z); vs.append(q); colors.append((.055+j*.006,.049+j*.005,.038+j*.004,.25))
            for j in range(3):
                for i in range(n): a=j*n+i; b=j*n+(i+1)%n; fs.append((a,b,b+n,a+n))
            fs.append(tuple(range(n-1,-1,-1)))
            add_mesh('EmbeddedCrown'+str(sign)+str(back),vs,fs,colors=colors,smooth=False)

def classify_body(old,name,groups):
    # Immutable UV tile semantics from S313 original 2048/4x4 atlas; geometric data retained.
    vertex_component={vi:g for g in groups for vi in g}
    for a in list(old.data.color_attributes): old.data.color_attributes.remove(a)
    old.data.color_attributes.new(name='Color',type='FLOAT_COLOR',domain='CORNER')
    color=old.data.color_attributes['Color']; uv=old.data.uv_layers.active; counts=Counter()
    for f in old.data.polygons:
        for li in f.loop_indices:
            vi=old.data.loops[li].vertex_index; v=old.data.vertices[vi]; g=vertex_component[vi]
            u,w=uv.data[li].uv; tile=int(math.floor(u*4))+4*int(math.floor((1-w)*4))
            groupweights=weights(old,v); dominant=max(groupweights,key=groupweights.get)
            if name=='AshWalker':
                category='cloth' if tile in (0,2,7) else 'leather'
                if tile==4: category='cloth'
                if tile==5 or tile==6 or (tile==1 and ((len(g)==24 and dominant.startswith('spine')) or (len(g)==144 and dominant.startswith('upperarm')))): category='bone'
                palette={'cloth':(.050,.058,.053,.75),'leather':(.085,.068,.051,.5),'bone':(.30,.275,.215,.25),'metal':(.11,.095,.07,1)}
            else:
                category='cloth' if tile==0 else 'leather'
                if tile in (3,4): category='skin'
                if tile==6 or (f.material_index==1 and tile==5): category='metal'
                palette={'cloth':(.095,.11,.083,.75),'leather':(.19,.11,.055,.5),'skin':(.28,.18,.12,0),'metal':(.21,.155,.083,1),'bone':(.31,.27,.20,.25)}
            base=palette[category]
            # Broad garment weathering follows existing vertices without touching UVs.
            modulation=.89+.11*math.sin(v.co.z*.13+abs(v.co.x)*.17)**2
            color.data[li].color=tuple(c*modulation for c in base[:3])+(base[3],); counts[category]+=1
    return dict(counts)

def assert_color_contract(me,tolerance=.00001):
    attr=me.color_attributes['Color']
    for value in attr.data:
        c=value.color
        assert all(math.isfinite(v) and -.00001<=v<=1.00001 for v in c),tuple(c)
        assert min(abs(c[3]-a) for a in (0,.25,.5,.75,1))<=tolerance,('invalid category',tuple(c))
        assert max(c[:3])<.61,('unexpected bright RGB',tuple(c))
    return {str(i):sum(abs(v.color[3]-i)<tolerance for v in attr.data) for i in (0,.25,.5,.75,1)}

def corner_channels(me):
    color=me.color_attributes['Color']; uv=me.uv_layers.active; result={}
    for slot in range(3):
        indices=[li for f in me.polygons if f.material_index==slot for li in f.loop_indices]
        result[str(slot)]={'corners':len(indices),'rgb_mean':[sum(color.data[i].color[k] for i in indices)/len(indices) for k in range(3)],'uvs':sorted(tuple(float(v) for v in uv.data[i].uv) for i in indices)}
    return result

def verify_roundtrip(path,expected,original_bones):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.fbx(filepath=str(path),colors_type='LINEAR')
    ob=next(o for o in bpy.data.objects if o.type=='MESH'); rig=next(o for o in bpy.data.objects if o.type=='ARMATURE')
    categories=assert_color_contract(ob.data,.003)
    actual=corner_channels(ob.data); max_uv_delta=0; max_rgb_mean_delta=0
    for slot in ('0','1','2'):
        a,b=actual[slot],expected[slot]; assert a['corners']==b['corners']
        delta=max(abs(x-y) for ca,cb in zip(a['uvs'],b['uvs']) for x,y in zip(ca,cb))
        assert delta<.000002,(path.name,slot,'UV',delta); max_uv_delta=max(max_uv_delta,delta)
        delta=max(abs(x-y) for x,y in zip(a['rgb_mean'],b['rgb_mean']))
        assert delta<.004,(path.name,slot,'RGB',delta); max_rgb_mean_delta=max(max_rgb_mean_delta,delta)
    assert [b.name for b in rig.data.bones]==[b['name'] for b in original_bones]
    bind_delta=max(abs(float(b.matrix_local[r][c])-original_bones[i]['matrix'][r][c]) for i,b in enumerate(rig.data.bones) for r in range(4) for c in range(4))
    assert bind_delta<.015,bind_delta
    result={'mode':'FBX reimport colors_type LINEAR','category_corner_counts':categories,'max_uv_delta':max_uv_delta,'max_rgb_mean_delta':max_rgb_mean_delta,'max_bind_matrix_delta':bind_delta,'material_slots':[m.name for m in ob.data.materials]}
    print('S357_LINEAR_ROUNDTRIP_PASS',path.stem,json.dumps(result)); return result

def build(name):
    global PARTS,COMPONENTS,RIG,FACE_MAT,IS_ASH,HZ,CY,TOP,PROFILE,FACE_BVH,EYE_BOUNDARIES,CLEARANCE_REPORTS
    PARTS=[]; COMPONENTS=[]; IS_ASH=name=='AshWalker'; FACE_BVH=None; EYE_BOUNDARIES={}; CLEARANCE_REPORTS=[]
    source=ROOT/'Unreal/ArtSource/CharacterUpgradeS356/HeadRefinement'/name/(name+'.fbx')
    source_hash=digest(source); bpy.ops.wm.read_factory_settings(use_empty=True); bpy.ops.import_scene.fbx(filepath=str(source))
    RIG=next(o for o in bpy.data.objects if o.type=='ARMATURE'); old=next(o for o in bpy.data.objects if o.type=='MESH')
    old.parent=None; old.matrix_world=Matrix.Identity(4); RIG.scale=(1,1,1)
    original_bounds=bounds([v.co for v in old.data.vertices]); TOP=original_bounds[2][1]
    original_bones=[{'name':b.name,'parent':b.parent.name if b.parent else None,'matrix':[list(row) for row in b.matrix_local]} for b in RIG.data.bones]
    assert len(original_bones)==77
    HZ=float(RIG.data.bones['head'].head_local.z); CY=float(RIG.data.bones['head'].head_local.y)
    groups=connected(old); removed=[]; removed_neck=[]; delete=set()
    for g in groups:
        if all(weights(old,old.data.vertices[i]).get('head',0)>.999 for i in g):
            removed.append({'vertices':len(g),'bounds_cm':bounds([old.data.vertices[i].co for i in g])}); delete.update(g)
        elif len(g)==32 and all(weights(old,old.data.vertices[i]).get('neck_01',0)>.999 for i in g):
            extent=bounds([old.data.vertices[i].co for i in g])
            if extent[0][1]-extent[0][0]<12 and abs(extent[2][1]-HZ)<.01:
                removed_neck.append({'vertices':32,'bounds_cm':extent,'reason':'Replace original overlapping rigid neck cylinder with the continuous blended neck; retain collar and all remaining body/gear'}); delete.update(g)
    assert any(r['vertices']==260 for r in removed),(name,removed)
    assert len(removed_neck)==1,(name,removed_neck)
    bm=bmesh.new(); bm.from_mesh(old.data); bm.verts.ensure_lookup_table(); bmesh.ops.delete(bm,geom=[bm.verts[i] for i in delete],context='VERTS'); bm.to_mesh(old.data); bm.free()
    retained_hash,retained_corners=immutable_corners(old); retained_vertices=len(old.data.vertices)
    retained_body_normals=[tuple(n.vector) for n in old.data.corner_normals]
    categories=classify_body(old,name,connected(old))
    original_sections=[f.material_index for f in old.data.polygons]
    old.data.materials.clear()
    for slot in ('Body','Gear'):
        mat=bpy.data.materials.new('M_'+name+slot); mat.use_nodes=True; old.data.materials.append(mat)
    for f,slot in zip(old.data.polygons,original_sections): f.material_index=slot
    assert immutable_corners(old)==(retained_hash,retained_corners)
    assert_color_contract(old.data)
    PARTS.append(old); FACE_MAT=bpy.data.materials.new('M_'+name+'Face'); FACE_MAT.use_nodes=True
    if IS_ASH:
        rows=[(-9.7,5.1,1.2,12.1),(-4.0,4.9,1.4,12.0),(0,5.4,.8,12.5),(2.3,4.2,.1,12.8),(4.5,5.7,-.7,13.1),(7.0,7.2,-1.1,13.5),(10.0,8.4,-1.4,14.2),(14,8.8,-1.1,14.6),(17,8.3,-.2,14.2),(19.5,7.1,1.0,12.6),(21.5,5.1,3.1,10.5),(23.01,.015,CY-.015,CY+.015)]
    else:
        rows=[(-9.2,4.65,1.55,10.9),(-4,4.5,1.7,10.8),(0,4.8,1.0,10.6),(1.2,4.7,-1.6,10.35),(2.5,5.9,-2.9,10.15),(4,6.7,-3.3,10.25),(6.5,7.1,-3.7,10.45),(9,7.3,-3.1,10.50),(11,7.1,-2.9,10.40),(13,7.2,-2.1,10.25),(15.5,7,-.8,9.8),(17.3,5.8,1.3,9.2),(18.9,3.4,3.6,8.15),(TOP-HZ,.015,CY-.015,CY+.015)]
    PROFILE=[(HZ+z,rx,f,b) for z,rx,f,b in rows]
    if not IS_ASH:
        _,rx,front,back=PROFILE[-1]
        PROFILE[-1]=(TOP-.12,rx,front,back)
    create_cranium(); orbital_features(-1); orbital_features(1)
    refresh_face_bvh(); ocular_seal=validate_eye_seals()
    if IS_ASH: ash_details()
    else: human_details()
    seam_weld=weld_orbital_seams()
    bpy.ops.object.select_all(action='DESELECT')
    for ob in PARTS: ob.select_set(True)
    bpy.context.view_layer.objects.active=old; bpy.ops.object.join(); body=old; body.name=name+'_Body'
    assert_color_contract(body.data)
    for mod in list(body.modifiers): body.modifiers.remove(mod)
    tri=body.modifiers.new('Triangles','TRIANGULATE'); bpy.ops.object.modifier_apply(modifier=tri.name)
    normal_continuity=restore_body_and_smooth_face_normals(body,retained_body_normals,retained_vertices)
    assert_color_contract(body.data)
    assert len(body.data.materials)==3,[m.name for m in body.data.materials]
    body.parent=RIG; body.matrix_parent_inverse=Matrix.Identity(4); body.matrix_basis=Matrix.Identity(4)
    skin=body.modifiers.new('Skin','ARMATURE'); skin.object=RIG
    RIG.scale=(1,1,1); RIG.location=(0,0,0); RIG.rotation_euler=(0,0,0)
    bpy.context.scene.unit_settings.system='METRIC'; bpy.context.scene.unit_settings.scale_length=.01
    assert all(v.groups and abs(sum(g.weight for g in v.groups)-1)<.0001 for v in body.data.vertices)
    assert all(math.isfinite(x) for v in body.data.vertices for x in v.co)
    final_bounds=bounds([v.co for v in body.data.vertices]); height=final_bounds[2][1]-final_bounds[2][0]
    assert abs(height-(original_bounds[2][1]-original_bounds[2][0]))<.035,(name,final_bounds,original_bounds)
    body.data.calc_loop_triangles(); sections=Counter(t.material_index for t in body.data.loop_triangles)
    assert len(body.data.loop_triangles)<35000,(name,len(body.data.loop_triangles))
    assert all(sections[i]>0 for i in range(3)),dict(sections)
    actual_bones=[{'name':b.name,'parent':b.parent.name if b.parent else None,'matrix':[list(row) for row in b.matrix_local]} for b in RIG.data.bones]
    assert actual_bones==original_bones
    # Retained vertices precede joined new parts and preserve all original source coordinates.
    assert len(body.data.vertices)>retained_vertices+5000
    assert immutable_corners(body,retained_vertices)==(retained_hash,retained_corners),'Body/gear coordinates, UVs or weights changed'
    channels=corner_channels(body.data)
    dest=OUT/name; dest.mkdir(parents=True,exist_ok=False); path=dest/(name+'.fbx')
    bpy.ops.object.select_all(action='DESELECT'); body.select_set(True); RIG.select_set(True); bpy.context.view_layer.objects.active=RIG
    bpy.ops.export_scene.fbx(filepath=str(path),use_selection=True,object_types={'ARMATURE','MESH'},axis_forward='-Y',axis_up='Z',apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',add_leaf_bones=False,use_armature_deform_only=True,path_mode='RELATIVE',bake_anim=False,bake_anim_use_all_actions=False,bake_anim_use_nla_strips=False,bake_anim_simplify_factor=0,bake_anim_step=1,mesh_smooth_type='FACE',colors_type='LINEAR')
    assert digest(source)==source_hash
    report={'name':name,'revision':'R4','source':str(source.relative_to(ROOT)).replace('\\','/'),'source_sha256':source_hash,'generator_sha256':digest(Path(__file__)),'output_sha256':digest(path),'height_cm':height,'runtime_height_unchanged':185 if IS_ASH else 165,'bounds_cm':final_bounds,'original_bounds_cm':original_bounds,'vertices':len(body.data.vertices),'triangles':len(body.data.loop_triangles),'retained_body_gear_vertices':retained_vertices,'retained_body_gear_corner_sha256':retained_hash,'retained_body_gear_corners':retained_corners,'removed_head_components':removed,'removed_neck_component_exception':removed_neck,'material_sections':{'Body':sections[0],'Gear':sections[1],'Face':sections[2]},'category_corner_counts':categories,'alpha_categories':{'skin':0,'bone':.25,'leather':.5,'cloth':.75,'metal':1},'new_components':COMPONENTS,'bones':original_bones,'head_pivot_cm':[0,CY,HZ],'facial_landmarks_cm':{'eye_left':[(-3.55 if IS_ASH else -3),surface_y(-3.55 if IS_ASH else -3,HZ+(14.1 if IS_ASH else 9.15)),HZ+(14.1 if IS_ASH else 9.15)],'eye_right':[(3.55 if IS_ASH else 3),surface_y(3.55 if IS_ASH else 3,HZ+(14.1 if IS_ASH else 9.15)),HZ+(14.1 if IS_ASH else 9.15)],'neck_base_z':PROFILE[0][0],'cranium_top_z':PROFILE[-1][0],'crown_top_z':TOP if IS_ASH else None},'head_weight_policy':'neck_01 to head blend from pivot-7.7 to pivot+2; all upper head/features head1','uv_policy':'retained body/gear UV0 immutable; new face normalized local UV0; category selects atlas swatch','normal_policy':'smooth new facial topology, faceted crown; original body normals retained'}
    report['seam_weld']=seam_weld
    report['normal_continuity']=normal_continuity
    report['surface_clearance']=CLEARANCE_REPORTS
    report['orbital_seal']=ocular_seal
    report['linear_roundtrip']=verify_roundtrip(path,channels,original_bones)
    (dest/'geometry.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print('S357_GEOMETRY_PASS',name,json.dumps({k:report[k] for k in ('height_cm','vertices','triangles','material_sections','retained_body_gear_vertices')}))
    return report

if __name__=='__main__':
    args=sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else []
    names=args or list(NAMES); assert all(n in NAMES for n in names)
    assert not any((OUT/n).exists() for n in names),'Refusing existing immutable S357 revision targets'
    OUT.mkdir(parents=True,exist_ok=True)
    executed_sha=digest(Path(__file__))
    reports=[build(n) for n in names]
    assert digest(Path(__file__))==executed_sha, 'Generator changed during execution'
    inventory=[{'path':str(p.relative_to(OUT)).replace('\\','/'),'sha256':digest(p),'bytes':p.stat().st_size} for p in sorted(OUT.rglob('*')) if p.is_file() and p.name!='source_inventory.json']
    (OUT/'source_inventory.json').write_text(json.dumps({'revision':'R4','generator_sha256':digest(Path(__file__)),'files':inventory},indent=2)+'\n',encoding='utf-8')
    print('S357_AUTHORING_COMPLETE',len(reports))
