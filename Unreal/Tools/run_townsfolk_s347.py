"""Serial S347 authoring/QA runner; preserves command logs and stops at the first failure."""
from pathlib import Path
import argparse,json,subprocess,sys,time
from build_townsfolk_specs_s347 import HEIGHTS
p=argparse.ArgumentParser();p.add_argument('--models-root',required=True);p.add_argument('--evidence-dir',required=True);p.add_argument('ids',nargs='*');a=p.parse_args()
tools=Path(__file__).resolve().parent;out=Path(a.evidence_dir).resolve();out.mkdir(parents=True,exist_ok=True)
assert all(x in HEIGHTS for x in a.ids),a.ids
order=a.ids or ['npc_villager_f','npc_villager_m','npc_elder','npc_fisherman','npc_child','npc_scholar'];results=[]
for ident in order:
 for phase,script in [('build','build_townsfolk_s347.py'),('qa','qa_townsfolk_s347.py')]:
  log=out/f'{ident}_{phase}.log';cmd=[sys.executable,'-B',str(tools/script),ident,'--models-root',a.models_root]
  print(f'S347_START {ident} {phase}',flush=True);start=time.time()
  with log.open('w',encoding='utf-8') as f:r=subprocess.run(cmd,stdout=f,stderr=subprocess.STDOUT)
  text=log.read_text(encoding='utf-8',errors='replace');bad=any(x in text for x in ('Traceback (most recent call last)','Fatal error','EXCEPTION_ACCESS_VIOLATION'))
  record=dict(id=ident,phase=phase,exit_code=r.returncode,fatal=bad,seconds=round(time.time()-start,2),log=str(log));results.append(record)
  (out/'townsfolk_run.json').write_text(json.dumps(results,indent=2),encoding='utf-8')
  if r.returncode or bad:print(text[-2200:],flush=True);raise SystemExit(r.returncode or 1)
  assert 'S347_'+('MODEL_BUILD_PASS' if phase=='build' else 'FBX_QA_PASS') in text
  print(f'S347_PASS {ident} {phase} {record["seconds"]}s',flush=True)
print('S347_ALL_MODELS_QA_PASS',len(order),flush=True)
