"""One-time editor authoring of three explicit development maps; no replacement."""
from pathlib import Path
import argparse, json, subprocess, re
from narrative_ir import ROOT, canonical
from validate_unreal import matches_required_version

def main():
    p=argparse.ArgumentParser(); p.add_argument('--engine-root',type=Path,required=True); p.add_argument('--evidence-dir',type=Path,required=True)
    p.add_argument('--refresh',action='store_true',help='Update only the three owned development maps lighting/labels'); a=p.parse_args()
    if a.evidence_dir.exists(): p.error('Use a fresh evidence directory')
    a.evidence_dir.mkdir(parents=True)
    version=json.loads((a.engine_root/'Engine/Build/Build.version').read_text(encoding='utf-8-sig'))
    if not matches_required_version(version): p.error('Requires exact UE 5.8.2')
    command=list(map(str,[a.engine_root/'Engine/Binaries/Win64/UnrealEditor-Cmd.exe',ROOT/'Unreal/Memoria/Memoria.uproject',
                         '-run=MemoriaSliceAssets','-unattended','-nop4','-NullRHI','-stdout','-FullStdOutLogOutput']))
    if a.refresh: command.append('-Refresh')
    with (a.evidence_dir/'author.log').open('w',encoding='utf-8') as out:
        r=subprocess.run(command,stdout=out,stderr=subprocess.STDOUT,timeout=600,cwd=ROOT)
    log=(a.evidence_dir/'author.log').read_text(encoding='utf-8')
    errors=re.findall(r'(?im)^.*(?:fatal error|Assertion failed:|Unhandled Exception:|Log\w+: Error:).*$',log)
    names=re.findall(r'MEMORIA_SLICE_ASSET_SAVED (.+)',log)
    passed=r.returncode==0 and not errors and len(names)==3
    (a.evidence_dir/'author.json').write_bytes(canonical(dict(command=command,exit_code=r.returncode,errors=errors,maps=names,status='PASS' if passed else 'FAIL')))
    print('MEMORIA_SLICE_AUTHOR_'+('PASS' if passed else 'FAIL')); return 0 if passed else 1
if __name__=='__main__': raise SystemExit(main())
