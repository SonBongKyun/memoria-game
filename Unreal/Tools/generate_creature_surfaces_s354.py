from pathlib import Path
import numpy as np
from PIL import Image
import hashlib,json
out=Path(__file__).resolve().parents[1]/'ArtSource/CharacterUpgrade/Surface';out.mkdir(parents=True,exist_ok=True)
y,x=np.mgrid[0:512,0:512]/512
manifest={}
for name,seed in [('Fur',354),('Shell',355)]:
 rng=np.random.default_rng(seed)
 broad=sum(np.sin(2*np.pi*(rng.integers(1,8)*x+rng.integers(1,8)*y)+rng.uniform(0,6.28))/12 for i in range(12))
 if name=='Fur':
  phase=2*np.pi*(81*x+1.8*np.sin(2*np.pi*y)+.8*np.sin(8*np.pi*y))
  height=.65*np.sin(phase)*(.65+.35*np.sin(2*np.pi*(9*y+3*x)))+.20*np.sin(phase*2.13)+broad*.7
  value=np.clip(.67+broad*.38+.17*np.sin(phase),.30,.95);rough=np.clip(.85+broad*.09,.72,.95)
 else:
  cracks=(np.abs(np.sin(2*np.pi*(7*x+2*y)))<.10)|(np.abs(np.sin(2*np.pi*(2*x-9*y)+.9*np.sin(12*np.pi*x)))<.07)
  height=.16*np.sin(2*np.pi*43*x)*np.sin(2*np.pi*39*y)+broad*.8-cracks*.45
  value=np.clip(.65+broad*.52-cracks*.24,.20,.96);rough=np.clip(.69+broad*.19+cracks*.12,.5,.93)
 dx=(np.roll(height,-1,axis=1)-np.roll(height,1,axis=1))*.85
 dy=(np.roll(height,-1,axis=0)-np.roll(height,1,axis=0))*.85
 normal=np.stack([-dx,dy,np.ones_like(dx)],axis=-1);normal/=np.linalg.norm(normal,axis=-1,keepdims=True)
 arrays={'Normal':np.uint8(np.clip(normal*.5+.5,0,1)*255),'Surface':np.uint8(np.stack([value,rough,np.zeros_like(x)],axis=-1)*255)}
 for suffix,a in arrays.items():
  p=out/(name+'_'+suffix+'.png');Image.fromarray(a).save(p);manifest[p.name]=hashlib.sha256(p.read_bytes()).hexdigest()
(out/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n');print('S354_SURFACE_DATA',len(manifest))
