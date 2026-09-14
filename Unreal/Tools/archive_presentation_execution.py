"""Preserve fresh UE execution bytes in a ZIP without changing historical evidence."""
from pathlib import Path
import argparse,json,hashlib,zipfile
ROOT=Path(__file__).resolve().parents[2]
BASE=ROOT/'docs/unreal-migration/evidence/presentation1'
def main():
    ap=argparse.ArgumentParser();ap.add_argument('--execution',type=Path,required=True);ap.add_argument('--since',type=float,required=True);a=ap.parse_args()
    dest=a.execution.resolve();assert dest.is_relative_to(BASE)
    report=json.loads((dest/'unreal_validation.json').read_text(encoding='utf-8'))
    assert not list(dest.glob('raw_execution*.zip'));manifest={};files={}
    for command in report['commands']:
        if 'log' in command:files[command['name']+'.log']=ROOT/command['log']
    if 'automation_report' in report and (ROOT/report['automation_report']).is_file():files['automation_index.json']=ROOT/report['automation_report']
    for group in ['Phase1O','Phase1E']:
        for p in (ROOT/'Unreal/Memoria/Saved/Validation'/group).glob('*'):
            if p.is_file() and p.stat().st_mtime>=a.since:files[group+'/'+p.name]=p
    # Bound individual Git blobs while retaining every original log/snapshot/PNG.
    groups=[[]];size=0
    for name,p in files.items():
        if size+p.stat().st_size>48*1024*1024 and groups[-1]:groups.append([]);size=0
        groups[-1].append((name,p));size+=p.stat().st_size
    for index,group in enumerate(groups,1):
        archive='raw_execution.zip' if len(groups)==1 else f'raw_execution_{index:03d}.zip'
        with zipfile.ZipFile(dest/archive,'x',compression=zipfile.ZIP_DEFLATED,compresslevel=6) as z:
            for name,p in group:
                data=p.read_bytes();z.writestr(name,data);manifest[name]={'source':str(p),'sha256':hashlib.sha256(data).hexdigest(),'size':len(data),'archive':archive}
        with zipfile.ZipFile(dest/archive) as z:
            for name,p in group:assert hashlib.sha256(z.read(name)).hexdigest()==manifest[name]['sha256']
    (dest/'raw_manifest.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8');print('ARCHIVED',len(files),'exact raw files',len(groups),'archives')
if __name__=='__main__':main()
