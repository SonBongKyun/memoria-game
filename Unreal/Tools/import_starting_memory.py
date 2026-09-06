"""Source-check, optionally build, then import/reimport/reload in separate UE processes."""
import argparse
import json
from pathlib import Path
import re
import subprocess
import sys
from starting_memory_ir import ROOT, IR, PACKAGE, load, canonical, sha
from validate_unreal import matches_required_version

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--engine-root',required=True,type=Path)
    parser.add_argument('--godot',required=True,type=Path)
    parser.add_argument('--build',action='store_true')
    parser.add_argument('--evidence-dir',required=True,type=Path)
    args=parser.parse_args()
    evidence=args.evidence_dir.resolve()
    if any(evidence.is_relative_to(ROOT/'docs/unreal-migration/evidence'/p) for p in ('phase0','phase1a','phase1b','phase1b-ue58')):
        parser.error('Historical evidence is immutable')
    if evidence.exists(): parser.error('Use a new evidence directory')
    evidence.mkdir(parents=True)
    report={'commands':[],'status':'RUNNING'}
    def run(name,command,timeout):
        path=evidence/(name+'.log')
        with path.open('w',encoding='utf-8') as out:
            process=subprocess.Popen([str(c) for c in command],cwd=ROOT,stdout=out,stderr=subprocess.STDOUT)
            try: code=process.wait(timeout=timeout)
            except subprocess.TimeoutExpired:
                subprocess.run(['taskkill','/PID',str(process.pid),'/T','/F'],capture_output=True); process.wait()
                raise ValueError(name+' timed out')
        text=path.read_text(encoding='utf-8',errors='replace')
        fatal=re.findall(r'(?im)^.*(?:fatal error|Assertion failed:|Unhandled Exception:|SCRIPT ERROR|Parse Error).*$',text)
        report['commands'].append({'name':name,'command':[str(c) for c in command],'exit_code':code,'fatal_diagnostics':fatal,'log':str(path.relative_to(ROOT))})
        if code or fatal: raise ValueError(name+' failed')
    try:
        version=json.loads((args.engine_root/'Engine/Build/Build.version').read_text(encoding='utf-8-sig'))
        if not matches_required_version(version): raise ValueError('Requires exact UE 5.8.2')
        report['engine_version']=version
        run('source_check',[sys.executable,ROOT/'Unreal/Tools/export_starting_memory.py','--godot',args.godot,'--check','--evidence-dir',evidence/'source'],240)
        ir=load()
        if args.build:
            run('build',[sys.executable,ROOT/'Unreal/Tools/validate_unreal.py','--engine-root',args.engine_root,'--build-only','--evidence-dir',evidence/'build'],3700)
        package=ROOT/'Unreal/Memoria/Content'/Path(PACKAGE.removeprefix('/Game/')+'.uasset')
        for name,check in [('first',False),('second',False),('reload_check',True)]:
            before=sha(package.read_bytes()) if package.exists() else None
            command=[args.engine_root/'Engine/Binaries/Win64/UnrealEditor-Cmd.exe',ROOT/'Unreal/Memoria/Memoria.uproject',
                     '-run=MemoriaStartingCatalog',f'-IR={IR}',f'-Report={evidence/(name+".json")}',
                     '-unattended','-nop4','-NullRHI','-stdout','-FullStdOutLogOutput']
            if check: command.append('-CheckOnly')
            run(name,command,600)
            result=json.loads((evidence/(name+'.json')).read_text(encoding='utf-8'))
            after=sha(package.read_bytes())
            if not result['passed'] or result['semantic_sha256']!=ir['semantic_sha256'] or result['ir_sha256']!=sha(IR.read_bytes()):
                raise ValueError(name+' semantic/provenance differs')
            if name!='first' and (result['result']!='UNCHANGED' or result['saved'] or before!=after):
                raise ValueError('Unchanged import must not save or change the package')
            report[name]=dict(result,package_sha256=after)
        report['status']='PASS'
    except Exception as error: report.update(status='FAIL',error=str(error))
    (evidence/'pipeline.json').write_bytes(canonical(report))
    print('MEMORIA_STARTING_PIPELINE_'+report['status']+' '+report.get('error',''))
    return 0 if report['status']=='PASS' else 1

if __name__=='__main__': raise SystemExit(main())
