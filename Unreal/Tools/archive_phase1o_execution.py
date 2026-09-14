"""Copy a completed execution's raw logs, exact index and fresh capture bytes."""
from pathlib import Path
import argparse,json,shutil,hashlib
ROOT=Path(__file__).resolve().parents[2];BASE=ROOT/'docs/unreal-migration/evidence/phase1o'
def main():
    p=argparse.ArgumentParser();p.add_argument('--execution',type=Path,required=True);p.add_argument('--since',type=float,default=0);a=p.parse_args()
    dest=a.execution.resolve();assert dest.is_relative_to(BASE)
    r=json.loads((dest/'unreal_validation.json').read_text(encoding='utf-8'))
    assert r.get('ue_automation') in ['PASS','FAIL']
    manifest={}
    def copy(src,out):
        assert not out.exists();out.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(src,out);raw=out.read_bytes();assert raw==src.read_bytes();manifest[out.relative_to(dest).as_posix()]={'sha256':hashlib.sha256(raw).hexdigest(),'size':len(raw),'source':str(src)}
    for c in r['commands']:
        if 'log' in c:copy(ROOT/c['log'],dest/(c['name']+'.log'))
    copy(ROOT/r['automation_report'],dest/'automation_index.json')
    for name in ['Phase1O','Phase1E']:
        for src in (ROOT/'Unreal/Memoria/Saved/Validation'/name).glob('*'):
            if src.is_file() and src.stat().st_mtime>=a.since:copy(src,dest/name/src.name)
    (dest/'raw_files.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8');print('ARCHIVED',len(manifest),'exact raw files')
if __name__=='__main__':main()
