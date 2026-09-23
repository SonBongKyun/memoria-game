"""Retain each attempt's exact sources, logs, state outputs and native captures."""
from pathlib import Path
import argparse,json,zipfile,hashlib,datetime
ROOT=Path(__file__).resolve().parents[2]
def main():
    p=argparse.ArgumentParser();p.add_argument('--execution',type=Path,required=True);a=p.parse_args()
    d=a.execution.resolve();assert d.is_relative_to(ROOT/'docs/unreal-migration/evidence/checkpoint1')
    report=json.loads((d/'unreal_validation.json').read_text(encoding='utf-8'))
    since=datetime.datetime.fromisoformat(report['utc']).timestamp()
    until=(d/'unreal_validation.json').stat().st_mtime
    files={c['name']+'.log':ROOT/c['log'] for c in report.get('commands',[]) if 'log' in c and (ROOT/c['log']).is_file()}
    if 'automation_report' in report and (ROOT/report['automation_report']).is_file():files['automation_index.json']=ROOT/report['automation_report']
    for group in ['Phase1O','Phase1E','PlayFeel1','Checkpoint1']:
        for f in (ROOT/'Unreal/Memoria/Saved/Validation'/group).glob('*'):
            if f.is_file() and since<=f.stat().st_mtime<=until:files[group+'/'+f.name]=f
    # Actual isolated native save bytes, including backup and failed-write temp files.
    for f in (ROOT/'Unreal/Memoria/Saved/Validation/CheckpointTests').rglob('*'):
        if f.is_file() and since<=f.stat().st_mtime<=until:
            files['CheckpointTests/'+f.relative_to(ROOT/'Unreal/Memoria/Saved/Validation/CheckpointTests').as_posix()]=f
    for f in (ROOT/'Unreal/Memoria/Source').rglob('*'):
        if f.is_file():files[f.relative_to(ROOT).as_posix()]=f
    manifest={}
    with zipfile.ZipFile(d/'raw_execution.zip','x',zipfile.ZIP_DEFLATED) as z:
        for name,f in files.items():
            b=f.read_bytes();z.writestr(name,b);manifest[name]={'sha256':hashlib.sha256(b).hexdigest(),'bytes':len(b),'source':str(f)}
    (d/'raw_manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
    print('ARCHIVED',len(files))
if __name__=='__main__':main()

