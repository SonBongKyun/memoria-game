"""Second editable Arrel mesh; the first generator/source remain reproducible."""
import argparse, hashlib, json, math
import generate_arrel_prototype as base
from generate_arrel_prototype import (ROOT, BONES, V, T, idx, pelvis, spine, chest, neck, head,
    NAVY, BLUE, STEEL, EDGE, GOLD, DARK, SKIN, HAIR, EYE, tri, quad, loft, ellipsoid, beam, cape_weight)
OUT = ROOT / 'Unreal/ArtSource/Arrel3D/arrel_refined.v2.json'
# Reuse only the established rig and geometric primitives, then build a fresh surface.
V.clear(); T.clear()
PARTS = {}
def part(name, build):
    start = len(T); build(); PARTS[name] = [start, len(T)]

def torso():
    # Both nested layers follow chest. A visible clearance replaces almost coplanar shells.
    loft([(91,0,0,8.5,13),(112,0,0,9,14),(134,0,0,9.5,16),(142,0,0,8,14)],chest,NAVY,n=16)
    loft([(108,1,0,12,16),(118,1,0,14,19),(133,1,0,14.5,21),(140,1,0,11.5,18)],chest,STEEL,1,n=16)
    # Raised central ridge and a narrow collar rim stay on the same rigid armor segment.
    for s in [-1,1]:
        q=[(15.8,0,116),(16.4,0,132),(14,s*8,136),(14,s*7,117)]
        quad(*(q if s<0 else q[::-1]),chest,EDGE,1)
        beam((13,s*13,135),(10,s*16,139),.65,chest,GOLD,1)
    loft([(140,0,0,8,9),(147,0,0,7.4,8.2)],chest,NAVY,n=16)
    loft([(143,0,0,7,7.4),(153,0,0,6.2,6.8)],neck,NAVY,n=16)
    loft([(151,0,0,6.8,7.3),(153,0,0,6.8,7.3)],neck,GOLD,1,n=16)
    # Articulated waist plates have a dark gap; they do not overlap the moving breastplate.
    for lo,hi,rx,ry in [(99,104,11.6,15.6),(104.8,108.2,12,16)]:
        loft([(lo,0,0,rx,ry),(hi,0,0,rx,ry)],spine,STEEL,1,n=16)
    loft([(89,0,0,11.3,15.2),(96,0,0,11.3,15.2)],pelvis,DARK,n=16)
    loft([(94,0,0,11.6,15.5),(95.5,0,0,11.6,15.5)],pelvis,GOLD,1,n=16)
    ellipsoid((12,0,92),(1.5,3.6,3),pelvis,GOLD,1)
    ellipsoid((15.7,-10,135),(1.7,2.5,2.5),chest,EYE,1)
part('nested_torso_and_armor',torso)

def limbs():
    for side,s in [('l',-1),('r',1)]:
        upper,fore,hand=(idx[k+'_'+side] for k in ['upperarm','forearm','hand'])
        loft([(114,0,s*30,4.5,4.7),(130,0,s*28,5.7,5.7),(139,0,s*27,5.7,6)],upper,NAVY)
        # Stacked, tapering shoulder plates instead of spherical caps.
        loft([(131,0,s*28,7.3,8.1),(136,0,s*27,8.6,9.1),(142,0,s*26,7.7,8),(145,0,s*25,4.2,5)],upper,STEEL,1)
        loft([(131,0,s*28,7.5,8.3),(132.5,0,s*28,7.8,8.6)],upper,GOLD,1)
        ellipsoid((0,s*30,114),(4.7,4.7,5.8),fore,DARK)
        loft([(92,0,s*32,4.1,4.3),(104,0,s*30.7,5,5.1),(113,0,s*30,4.8,4.8)],fore,STEEL,1)
        loft([(95,0,s*31.8,4.6,4.8),(97,0,s*31.8,4.6,4.8)],fore,EDGE,1)
        ellipsoid((1,s*32,86),(4.1,3.4,6),hand,DARK)
        ellipsoid((3,s*32,87),(2.2,3.2,3.5),hand,STEEL,1)
        thigh,calf,foot=(idx[k+'_'+side] for k in ['thigh','calf','foot'])
        loft([(50,0,s*10,5.4,5.8),(69,0,s*10,7.3,7.4),(90,0,s*10,8.3,8.2)],thigh,DARK)
        loft([(57,2,s*10,5.5,6),(72,2,s*10,7,7.1),(83,1,s*10,7.6,7.8)],thigh,STEEL,1)
        ellipsoid((3,s*10,50),(6,6.3,5.6),calf,EDGE,1)
        loft([(10,0,s*10,4.8,5),(29,0,s*10,5.5,5.8),(44,0,s*10,6.1,6.2)],calf,STEEL,1)
        loft([(31,0,s*10,5.8,6.1),(32.5,0,s*10,5.8,6.1)],calf,GOLD,1)
        # Flat soles keep contact measurable at source Z=0, independent of ankle roll.
        loft([(0,3,s*10,10,5.9),(2,3,s*10,10,5.9),(5,3,s*10,9.6,5.6),(10,0,s*10,5,5)],foot,DARK)
        loft([(3,5,s*10,7.5,5.2),(6,5,s*10,7.7,5.3),(8,4,s*10,6,4.8)],foot,STEEL,1)
part('articulated_limbs_and_flat_soles',limbs)

def cape():
    rows=[]
    for z,x,w in [(143,-13,18),(127,-19,20),(110,-24,22),(90,-28,24),(72,-32,25),(48,-35,24)]:
        rows.append([(x-(2.0 if j%2 else 0),(-1+j*.25)*w,z+(3 if j in [1,7] else 0)) for j in range(9)])
    for a,b in zip(rows,rows[1:]):
        for j in range(8):
            color=BLUE if j in [0,7] else NAVY
            quad(a[j],b[j],b[j+1],a[j+1],cape_weight,color)
            quad(a[j+1],b[j+1],b[j],a[j],cape_weight,color)
    for j in [0,8]:
        for a,b in zip(rows,rows[1:]):beam(a[j],b[j],.5,cape_weight,GOLD,1)
part('folded_cape',cape)

def face():
    loft([(149,0,0,4,4.5),(153,1,0,6.1,5.5),(157,.7,0,7.5,6.7),(164,0,0,8.6,7.5),(170,-.6,0,8,7.5),(174,-1,0,6,6)],head,SKIN,n=20)
    for s in [-1,1]:
        ellipsoid((0,s*7.6,160),(2,1.5,3.4),head,SKIN)
        ellipsoid((7.9,s*3.35,163.2),(.5,2,1.0),head,[.13,.09,.085],n=12)
        ellipsoid((8.2,s*3.35,163.25),(.38,1.6,.68),head,[.58,.60,.58],n=12)
        ellipsoid((8.55,s*3.1,163.25),(.25,.65,.67),head,EYE,n=12)
        ellipsoid((8.75,s*3.1,163.3),(.18,.25,.4),head,DARK,n=8)
        beam((8.05,s*1.6,165.2),(7.3,s*5.1,165.8),.38,head,[.23,.25,.29],n=6)
    points=[(8.4,-.95,163),(8.4,.95,163),(11.0,0,159.2),(8.2,-1.3,158.5),(8.2,1.3,158.5)]
    for a,b,c in [(0,2,1),(0,3,2),(1,2,4),(3,4,2)]:tri([points[a],points[b],points[c]],head,SKIN)
    beam((8,-2,155.9),(8,2,155.9),.22,head,[.29,.16,.13],n=6)
    # Closed tapered locks give the side silhouette volume as well as frontal bangs.
    ellipsoid((-1.8,0,172),(9.6,8.7,6.5),head,HAIR,n=20)
    loft([(155.5,-6.8,0,1.6,3.5),(162,-7.6,0,3.8,6.5),(171,-6.5,0,4,6.3)],head,HAIR,n=16)
    for i,y in enumerate([-7,-4.5,-2,1,3.8,6.5]):
        end_z=165+abs(y)*.25
        loft([(end_z,9.4,y-1.2,.18,.22),(170,8.4,y,2.4,2.2),(176,3.7,y+1.1,3,2.6)],head,[v*(.86+i*.02) for v in HAIR],n=6)
    for s in [-1,1]:
        for x,z in [(-4,156.5),(-1,157.5),(2,160)]:
            loft([(z,x,s*7.8,.25,.3),(165,x-1,s*8.6,2.2,1.9),(173,x-2,s*6.8,3,2.6)],head,HAIR,n=6)
part('jaw_eyes_and_volume_hair',face)

def accessory():
    beam((-4,-19,35),(0,-19,99),2.2,pelvis,NAVY)
    beam((0,-25,99),(0,-13,99),1,pelvis,GOLD,1)
    beam((0,-19,99),(0,-19,110),1.4,pelvis,DARK)
    ellipsoid((0,-19,111),(2.2,2.2,2.2),pelvis,EYE,1)
part('removable_visual_scabbard',accessory)

def main():
    ap=argparse.ArgumentParser();ap.add_argument('--check',action='store_true');args=ap.parse_args()
    refs=['assets/game_image/reference/arrel_reference_turnaround.png','assets/portraits/character_shots/arrel_story_v2.png']
    data={'schema':1,'revision':2,'status':'editable_visual_prototype_not_canon','units':'centimeters','forward':'+X','bones':BONES,'vertices':V,'triangles':T,'materials':['Fabric','Steel'],'parts':PARTS,'reference_sha256':{p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in refs}}
    raw=(json.dumps(data,separators=(',',':'))+'\n').encode()
    if args.check:assert OUT.read_bytes()==raw
    else:
        assert not OUT.exists(),'Keep prior source bytes; explicitly archive before any revised source write.'
        OUT.parent.mkdir(parents=True,exist_ok=True);OUT.write_bytes(raw)
    print(json.dumps({'status':'PASS','bones':len(BONES),'vertices':len(V),'triangles':len(T),'sha256':hashlib.sha256(raw).hexdigest()}))
if __name__=='__main__':main()
