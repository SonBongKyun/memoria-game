"""Rebuildable, reference-guided skeletal blockout; no manuscript/canon authority."""
from pathlib import Path
import argparse, math, json, hashlib
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'Unreal/ArtSource/Arrel3D/arrel_prototype.v1.json'
BONES=[]; V=[]; T=[]
def bone(name,parent,pos):
    idx=len(BONES); BONES.append({'name':name,'parent':parent,'position':pos});return idx
root=bone('root',-1,[0,0,0]);pelvis=bone('pelvis',0,[0,0,92]);spine=bone('spine',1,[0,0,114]);chest=bone('chest',2,[0,0,134]);neck=bone('neck',3,[0,0,149]);head=bone('head',4,[0,0,159])
for side,sgn in [('l',-1),('r',1)]:
    c=bone('clavicle_'+side,chest,[0,sgn*18,140]);a=bone('upperarm_'+side,c,[0,sgn*27,139]);f=bone('forearm_'+side,a,[0,sgn*30,114]);bone('hand_'+side,f,[0,sgn*32,90])
for side,sgn in [('l',-1),('r',1)]:
    a=bone('thigh_'+side,pelvis,[0,sgn*10,89]);f=bone('calf_'+side,a,[0,sgn*10,50]);bone('foot_'+side,f,[0,sgn*10,8])
c1=bone('cape_upper',chest,[-11,0,144]);c2=bone('cape_mid',c1,[-14,0,110]);c3=bone('cape_tip',c2,[-18,0,74])
idx={b['name']:i for i,b in enumerate(BONES)}
NAVY=[.018,.042,.093];BLUE=[.035,.10,.22];STEEL=[.32,.39,.48];EDGE=[.50,.56,.62];GOLD=[.32,.22,.10];DARK=[.025,.03,.04];SKIN=[.64,.44,.31];HAIR=[.63,.67,.72];WHITE=[.72,.77,.81];EYE=[.045,.29,.53]
def tri(points,b,color,mat=0):
    start=len(V)
    for p in points:
        weights=[[b,1.0]] if isinstance(b,int) else b(p)
        V.append([*[round(x,5) for x in p],color,weights])
    T.append([start,start+1,start+2,mat])
def quad(a,b,c,d,bone,color,mat=0):
    tri([a,b,c],bone,color,mat);tri([a,c,d],bone,color,mat)
def loft(rings,b,col,mat=0,n=12):
    rows=[[(x+rx*math.cos(i*math.tau/n),y+ry*math.sin(i*math.tau/n),z) for i in range(n)] for z,x,y,rx,ry in rings]
    for row,nxt in zip(rows,rows[1:]):
        for i in range(n):j=(i+1)%n;quad(row[i],row[j],nxt[j],nxt[i],b,col,mat)
    for row,reverse in [(rows[0],True),(rows[-1],False)]:
        center=tuple(sum(p[k] for p in row)/n for k in range(3))
        for i in range(n):
            a,c=row[i],row[(i+1)%n];tri([center,c,a] if reverse else [center,a,c],b,col,mat)
def ellipsoid(c,r,b,col,mat=0,n=12):
    rings=[]
    for k in range(1,8):
        a=-math.pi/2+k*math.pi/8;s=math.cos(a)
        rings.append((c[2]+r[2]*math.sin(a),c[0],c[1],r[0]*s,r[1]*s))
    loft(rings,b,col,mat,n)
def beam(a,b,r,bone,col,mat=0,n=8):
    # Stable ring frame for arbitrary sword/trim segments.
    axis=[b[i]-a[i] for i in range(3)];length=math.sqrt(sum(x*x for x in axis));axis=[x/length for x in axis]
    ref=[0,0,1] if abs(axis[2])<.9 else [1,0,0]
    def cross(u,v):return [u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0]]
    u=cross(axis,ref);s=math.sqrt(sum(x*x for x in u));u=[x/s for x in u];v=cross(axis,u)
    rows=[[[p[k]+r*(u[k]*math.cos(i*math.tau/n)+v[k]*math.sin(i*math.tau/n)) for k in range(3)] for i in range(n)] for p in [a,b]]
    for i in range(n):j=(i+1)%n;quad(rows[0][i],rows[0][j],rows[1][j],rows[1][i],bone,col,mat)
    for row,p,rev in [(rows[0],a,True),(rows[1],b,False)]:
        for i in range(n):tri([p,row[(i+1)%n],row[i]] if rev else [p,row[i],row[(i+1)%n]],bone,col,mat)
# Layered silhouette: fitted undercoat, breastplate, collar, belt and articulated armor.
loft([(87,0,0,11,15),(111,0,0,12,17),(135,0,0,12,21),(144,0,0,10,17)],spine,NAVY)
loft([(108,1,0,11.2,16.4),(121,1,0,13,20),(133,1,0,13.4,22),(139,1,0,11,19)],chest,STEEL,1)
loft([(137,0,0,11,18),(141,0,0,11,18)],chest,GOLD,1)
loft([(89,0,0,11.5,15.5),(96,0,0,11.5,15.5)],pelvis,DARK)
loft([(95,0,0,11.8,15.8),(97,0,0,11.8,15.8)],pelvis,GOLD,1)
ellipsoid((12.3,0,93),(2,4,4),pelvis,GOLD,1)
loft([(143,0,0,7.4,8),(153,0,0,6.8,7.4)],neck,NAVY)
loft([(152,0,0,7,7.6),(154,0,0,7,7.6)],neck,GOLD,1)
for side,s in [('l',-1),('r',1)]:
    upper,fore,hand=(idx[k+'_'+side] for k in ['upperarm','forearm','hand'])
    loft([(116,0,s*30,5.4,5.4),(137,0,s*27,6.5,6.5)],upper,NAVY)
    ellipsoid((0,s*27,138),(10,11,9),upper,STEEL,1)
    loft([(130,0,s*27,9.6,10.6),(133,0,s*27,10.2,11.2)],upper,GOLD,1)
    ellipsoid((0,s*30,114),(5.6,5.6,5.6),fore,DARK)
    loft([(93,0,s*32,4.8,4.8),(107,0,s*30.5,5.6,5.6),(115,0,s*30,5,5)],fore,STEEL,1)
    loft([(96,0,s*31.8,5,5),(99,0,s*31.8,5,5)],fore,EDGE,1)
    ellipsoid((1,s*32,86),(4.8,4,6.5),hand,DARK)
    thigh,calf,foot=(idx[k+'_'+side] for k in ['thigh','calf','foot'])
    loft([(52,0,s*10,6.4,6.8),(69,0,s*10,8,8),(91,0,s*10,9,9)],thigh,DARK)
    ellipsoid((4,s*10,69),(5.5,7.7,14),thigh,STEEL,1)
    ellipsoid((3,s*10,50),(6.5,7,6.5),calf,STEEL,1)
    loft([(10,0,s*10,5.2,5.4),(29,0,s*10,6.2,6.5),(46,0,s*10,6.8,6.8)],calf,STEEL,1)
    loft([(31,0,s*10,6.5,6.8),(34,0,s*10,6.5,6.8)],calf,GOLD,1)
    ellipsoid((4,s*10,6),(11,6.5,5.6),foot,DARK)
    ellipsoid((7,s*10,7.3),(8,6,3.6),foot,STEEL,1)
# Tapered navy cape, with skin weights interpolated through three editable bones.
def cape_weight(p):
    z=p[2]
    if z>=110:t=max(0,min(1,(z-110)/34));return [[c1,t],[c2,1-t]]
    t=max(0,min(1,(z-74)/36));return [[c2,t],[c3,1-t]]
rows=[]
for z,x,w in [(144,-12,18),(126,-16,21),(110,-18,23),(91,-20,25),(74,-22,26),(46,-25,27)]:
    rows.append([(x-(1.7 if j%2 else 0),(-1+j*.5)*w,z+(4 if j==2 else 0)) for j in range(5)])
for a,b in zip(rows,rows[1:]):
    for j in range(4):
        col=BLUE if j in [0,3] else NAVY
        quad(a[j],b[j],b[j+1],a[j+1],cape_weight,col)
        quad(a[j+1],b[j+1],b[j],a[j],cape_weight,col)
for j in [0,4]:
    for a,b in zip(rows,rows[1:]):beam(a[j],b[j],.7,cape_weight,GOLD,1)
# Face and ash-silver layered hair. No borrowed/generated facial texture.
ellipsoid((0,0,161),(9.2,8.1,12.5),head,SKIN,n=16)
ellipsoid((-.8,0,171.5),(10.6,9.7,7.5),head,HAIR,n=16)
for y in [-7,-4,0,3,6,8]:
    a=(5.5,y,176);b=(10.4,y-2,170);c=(10.8,y+1,160+abs(y)*.7);d=(6.7,y+3,173)
    tri([a,b,c],head,HAIR);tri([a,c,d],head,HAIR)
for y in [-9,9]:ellipsoid((-1,y,165),(7,2.3,7.5),head,HAIR)
for y in [-3.8,3.8]:
    ellipsoid((8.5,y,163.4),(1.3,2.1,1.3),head,DARK,n=8)
    ellipsoid((9.5,y,163.4),(.65,1.15,1.1),head,EYE,n=8)
tri([(9.7,-1,161),(12,0,158.4),(9.7,1,161)],head,SKIN)
beam((8.6,-2.5,154.7),(8.6,2.5,154.7),.35,head,[.24,.12,.10],n=6)
# Replaceable visual accessory. It does not grant, equip, or invoke a weapon.
beam((-4,-19,35),(0,-19,99),2.4,pelvis,NAVY)
beam((0,-25,99),(0,-13,99),1.3,pelvis,GOLD,1)
beam((0,-19,99),(0,-19,110),1.6,pelvis,DARK)
ellipsoid((0,-19,111),(2.6,2.6,2.6),pelvis,EYE,1,n=8)
# Small blue chest clasp echoing the concept, deliberately removable.
ellipsoid((14.5,-11,136),(2.1,3.2,3.2),chest,EYE,1,n=8)
def main():
    ap=argparse.ArgumentParser();ap.add_argument('--check',action='store_true');args=ap.parse_args()
    refs=['assets/game_image/reference/arrel_reference_turnaround.png','assets/portraits/character_shots/arrel_story_v2.png']
    data={'schema':1,'status':'editable_visual_prototype_not_canon','units':'centimeters','forward':'+X','bones':BONES,'vertices':V,'triangles':T,'materials':['Fabric','Steel'],'reference_sha256':{p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in refs}}
    raw=(json.dumps(data,separators=(',',':'))+'\n').encode()
    if args.check:assert OUT.read_bytes()==raw
    else:OUT.parent.mkdir(parents=True,exist_ok=True);OUT.write_bytes(raw)
    print(json.dumps({'status':'PASS','bones':len(BONES),'vertices':len(V),'triangles':len(T),'sha256':hashlib.sha256(raw).hexdigest()}))
if __name__=='__main__':main()
