"""Attest both sources and executable oracle, import/reimport/reload in fresh UE processes."""
import argparse, json, re, subprocess, sys
from pathlib import Path
from narrative_ir import ROOT, CASES, FIELD_CASES, ir_path, package, load, canonical, sha
from validate_unreal import matches_required_version

def main():
    p=argparse.ArgumentParser(); p.add_argument('--engine-root',type=Path,required=True); p.add_argument('--godot',type=Path,required=True)
    p.add_argument('--group',choices=tuple(FIELD_CASES)); p.add_argument('--build',action='store_true'); p.add_argument('--evidence-dir',type=Path,required=True); a=p.parse_args()
    ev=a.evidence_dir.resolve()
    if ev.exists() or any(ev.is_relative_to(ROOT/'docs/unreal-migration/evidence'/phase) for phase in ('phase0','phase1a','phase1b','phase1b-ue58','phase1c')): p.error('Use a fresh, nonhistorical evidence directory')
    ev.mkdir(parents=True); report={'status':'RUNNING','commands':[],'cases':{}}
    def run(name,cmd,timeout):
        cmd=list(map(str,cmd)); log=ev/(name+'.log')
        with log.open('w',encoding='utf-8') as out:
            process=subprocess.Popen(cmd,cwd=ROOT,stdout=out,stderr=subprocess.STDOUT)
            try: code=process.wait(timeout)
            except subprocess.TimeoutExpired:
                subprocess.run(['taskkill','/PID',str(process.pid),'/T','/F'],capture_output=True); process.wait(); raise ValueError(name+' timeout')
        text=log.read_text(encoding='utf-8',errors='replace')
        fatal=re.findall(r'(?im)^.*(?:fatal error|Assertion failed:|Unhandled Exception:|SCRIPT ERROR|Parse Error).*$',text)
        report['commands'].append(dict(name=name,command=cmd,exit_code=code,fatal_diagnostics=fatal,log=str(log.relative_to(ROOT))))
        if code or fatal: raise ValueError(name+' failed; inspect '+str(log))
    try:
        v=json.loads((a.engine_root/'Engine/Build/Build.version').read_text(encoding='utf-8-sig'))
        if not matches_required_version(v): raise ValueError('Exact UE 5.8.2 required')
        report['engine_version']=v
        run('source_check',[sys.executable,ROOT/'Unreal/Tools/narrative_ir.py','--check',*(['--group',a.group] if a.group else []),'--evidence-dir',ev/'source'],120)
        run('oracle_check',[sys.executable,ROOT/'Unreal/Tools'/('export_malet_refusal_oracle.py' if a.group in ('malet_encounter','malet_refused') else 'export_malet_oracle.py' if a.group=='malet_taste_burned' else 'export_narrative_oracle.py'),'--godot',a.godot,'--check','--evidence-dir',ev/'oracle'],240)
        run('fixture_check',[sys.executable,ROOT/'Unreal/Tools/narrative_test_fixtures.py','--check'],120)
        if a.build: run('build',[sys.executable,ROOT/'Unreal/Tools/validate_unreal.py','--engine-root',a.engine_root,'--build-only','--evidence-dir',ev/'build'],3700)
        # Each stage is a different UE process per dialect (six processes total).
        for d in (('field',) if a.group else CASES):
            report['cases'][d]={}; ir=load(ir_path(d,a.group)); path=ROOT/'Unreal/Memoria/Content'/(package(d,a.group).removeprefix('/Game/')+'.uasset')
            for stage,check in [('first',False),('second',False),('reload_check',True)]:
                name=d+'_'+stage; before=sha(path.read_bytes()) if path.exists() else None
                command=[a.engine_root/'Engine/Binaries/Win64/UnrealEditor-Cmd.exe',ROOT/'Unreal/Memoria/Memoria.uproject','-run=MemoriaNarrative',f'-IR={ir_path(d,a.group)}',f'-Dialect={d}',f'-Report={ev/(name+".json")}','-unattended','-nop4','-NullRHI','-stdout','-FullStdOutLogOutput']
                if check: command.append('-CheckOnly')
                run(name,command,600); result=json.loads((ev/(name+'.json')).read_text(encoding='utf-8'))
                after=sha(path.read_bytes())
                if not result['passed'] or result['semantic_sha256']!=ir['semantic_sha256'] or result['ir_sha256']!=sha(ir_path(d,a.group).read_bytes()): raise ValueError('Fingerprint differs: '+name)
                if stage!='first' and (result['result']!='UNCHANGED' or result['saved'] or before!=after): raise ValueError('Reimport saved/changed: '+name)
                report['cases'][d][stage]=dict(result,package_sha256=after)
        files=sorted(x.name for x in (ROOT/'Unreal/Memoria/Content/Memoria/Generated/Narrative').glob('*.uasset'))
        previous={x[2]+'.uasset' for x in CASES.values()}
        allowed=previous|{v[2]+'.uasset' for v in FIELD_CASES.values()}
        required=previous|({FIELD_CASES[a.group][2]+'.uasset'} if a.group else set())
        if not required <= set(files) <= allowed: raise ValueError('Unexpected/missing bounded narrative packages')
        report['selected_group']=a.group
        report.update(status='PASS',packages=files)
    except Exception as e: report.update(status='FAIL',error=str(e))
    (ev/'pipeline.json').write_bytes(canonical(report)); print('MEMORIA_NARRATIVE_PIPELINE_'+report['status']+' '+report.get('error',''))
    return 0 if report['status']=='PASS' else 1
if __name__=='__main__': raise SystemExit(main())
