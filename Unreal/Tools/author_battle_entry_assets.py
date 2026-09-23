"""Import only the five new S291 textures, then reload and check them in a new process."""
from pathlib import Path
import argparse,subprocess,json,datetime,hashlib,zipfile
ROOT=Path(__file__).resolve().parents[2]
def main():
    parser=argparse.ArgumentParser();parser.add_argument('--evidence-dir',type=Path,required=True);parser.add_argument('--study-only',action='store_true');parser.add_argument('--alley-rat-study-only',action='store_true');parser.add_argument('--check-only',action='store_true');a=parser.parse_args()
    assert not (a.study_only and a.alley_rat_study_only)
    out=a.evidence_dir.resolve();out.mkdir(parents=True,exist_ok=False)
    editor=Path(r'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe')
    report={'utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'commands':[],'passed':False}
    for check in ([True] if a.check_only else [False,True]):
        name='check' if check else 'import'
        cmd=[str(editor),str(ROOT/'Unreal/Memoria/Memoria.uproject'),'-run=MemoriaBattleEntryAssets','-unattended','-nop4','-nosplash','-NullRHI','-stdout','-FullStdOutLogOutput']+(['-CheckOnly'] if check else ['-AlleyRatStudyOnly'] if a.alley_rat_study_only else ['-StudyOnly'] if a.study_only else [])
        log=out/(name+'.log')
        with log.open('wb') as f:code=subprocess.run(cmd,stdout=f,stderr=subprocess.STDOUT,timeout=600).returncode
        text=log.read_text(encoding='utf-8',errors='replace')
        count=text.count('BATTLE_ENTRY_ASSET_CHECKED' if check else 'BATTLE_ENTRY_ASSET_SAVED')
        report['commands'].append(dict(command=cmd,exit_code=code,log=str(log.relative_to(ROOT)),count=count))
        report['passed']=code==0 and count==(7 if check else 1 if (a.study_only or a.alley_rat_study_only) else 7) and not any(t in text for t in ['Fatal error:','Assertion failed:', 'Unhandled Exception:'])
        (out/'assets.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
        if not report['passed']:break
    manifest={}
    with zipfile.ZipFile(out/'source_and_assets.zip','w',zipfile.ZIP_DEFLATED) as z:
        for folder in ['Unreal/Memoria/Source','Unreal/Memoria/Content/Memoria/Presentation/BattleEntry']:
            for p in (ROOT/folder).rglob('*'):
                if p.is_file():
                    name=p.relative_to(ROOT).as_posix();z.write(p,name);manifest[name]=hashlib.sha256(p.read_bytes()).hexdigest()
    (out/'sha256.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(report));return not report['passed']
if __name__=='__main__':raise SystemExit(main())
