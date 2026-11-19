from pathlib import Path
import argparse,datetime,hashlib,json,os,shutil,subprocess,time,uuid

def main():
    p=argparse.ArgumentParser();p.add_argument('--root',type=Path,default=Path(__file__).resolve().parents[2]);p.add_argument('--archive',type=Path,required=True);p.add_argument('--evidence',type=Path,required=True);p.add_argument('--timeout',type=int,default=600)
    a=p.parse_args();a.archive=a.archive.resolve();a.evidence=a.evidence.resolve()
    root=a.root.resolve()
    if not (root/'Unreal/Memoria/Memoria.uproject').is_file():raise RuntimeError('Require actual Unreal project root')
    exe=a.archive/'Windows/Memoria/Binaries/Win64/Memoria.exe'
    if not exe.is_file() or a.evidence.exists():raise RuntimeError('Require actual inner game executable and fresh evidence folder')
    a.evidence.mkdir(parents=True)
    user=root/'Unreal/Memoria/Saved/Validation/UpgradeSmokeRuntime'/('s362-'+uuid.uuid4().hex)
    assert not user.exists() and user.parent.name=='UpgradeSmokeRuntime'
    command=[str(exe),'/Game/Tests/Campaign/L_Ch2VerdanSlice','-MemoriaUpgradeSmoke','-UserDir='+str(user),
             '-unattended','-nop4','-nosplash','-RenderOffscreen','-ResX=1280','-ResY=720',
             '-csvGpuStats','-csvCompression=0','-ExecCmds=csv.BlockOnCaptureEnd 1','-stdout','-FullStdOutLogOutput']
    result={'utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'command':command,'user_dir':str(user),
            'exe_sha256':hashlib.sha256(exe.read_bytes()).hexdigest(),'status':'RUNNING'}
    receipt=a.evidence/'result.json'
    receipt.write_text(json.dumps(result,indent=2),encoding='utf-8')
    startup=subprocess.STARTUPINFO();startup.dwFlags|=subprocess.STARTF_USESHOWWINDOW;startup.wShowWindow=0
    started=time.time()
    with (a.evidence/'runtime.log').open('w',encoding='utf-8') as output:
        process=subprocess.Popen(command,cwd=exe.parent,stdout=output,stderr=subprocess.STDOUT,startupinfo=startup,creationflags=subprocess.CREATE_NO_WINDOW)
        result['pid']=process.pid;receipt.write_text(json.dumps(result,indent=2),encoding='utf-8')
        try:result['exit_code']=process.wait(timeout=a.timeout)
        except subprocess.TimeoutExpired:
            process.kill();process.wait();result['exit_code']=None;result['timeout']=True
    errors=[]
    reports=list(user.rglob('status.json')) if user.exists() else []
    if len(reports)!=1:errors.append('Expected exactly one isolated runtime status.json')
    else:
        status=json.loads(reports[0].read_text(encoding='utf-8-sig'))
        result['runtime_status_path']=str(reports[0]);result['runtime_status']=status.get('status')
        shutil.copytree(reports[0].parent,a.evidence/'RuntimeArtifacts')
        if status.get('status')!='PASS':errors.append('Runtime harness did not report PASS')
        if len(status.get('captures',[]))!=12:errors.append('Expected twelve verified native PNGs')
        if status.get('title_verdan_manual_restore_verified') is not True:errors.append('Actual cooked Title-to-Verdan restore was not verified')
        for capture in status.get('captures',[]):
            path=Path(capture['path']);payload=path.read_bytes()
            if not path.is_relative_to(user) or hashlib.sha1(payload).hexdigest().upper()!=capture['sha1'].upper():errors.append('Capture path/hash invalid')
        engine_csv=status.get('engine_csv',{})
        result['engine_csv']=engine_csv
        if engine_csv.get('status')!='complete':errors.append('Owned engine CSV incomplete')
    log=(a.evidence/'runtime.log').read_text(encoding='utf-8',errors='replace')
    fatal=[line for line in log.splitlines() if any(s in line.lower() for s in ('fatal error','assertion failed:','unhandled exception:'))]
    if fatal:errors.append('Fatal native diagnostics')
    if result.get('exit_code')!=0:errors.append('Nonzero exit/timeout')
    result.update(status='PASS' if not errors else 'FAIL',validation_errors=errors,fatal_diagnostics=fatal,elapsed_seconds=round(time.time()-started,3))
    receipt.write_text(json.dumps(result,indent=2),encoding='utf-8')
    print(json.dumps({k:result[k] for k in ('status','exit_code','validation_errors','elapsed_seconds')},ensure_ascii=False));return bool(errors)

if __name__=='__main__':raise SystemExit(main())
