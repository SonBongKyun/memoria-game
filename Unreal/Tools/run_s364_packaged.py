"""Run the real isolated Development executable and verify S364 graphics and gameplay evidence."""
from pathlib import Path
import argparse,datetime,hashlib,importlib.util,json,os,shutil,struct,subprocess,sys,time,uuid
sys.dont_write_bytecode=True
def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--root',type=Path,default=Path(__file__).resolve().parents[2])
    p.add_argument('--archive',required=True,type=Path);p.add_argument('--evidence',required=True,type=Path)
    p.add_argument('--timeout',type=int,default=600);a=p.parse_args()
    root=a.root.resolve();archive=a.archive.resolve();out=a.evidence.resolve()
    boundary=root/'Unreal/Memoria/Saved/Validation'
    if not (root/'Unreal/Memoria/Memoria.uproject').is_file() or not out.is_relative_to(boundary) or out==boundary or out.exists():p.error('Require actual project and fresh bounded evidence directory')
    exe=archive/'Windows/Memoria/Binaries/Win64/Memoria.exe'
    if not exe.is_file():p.error('Require cooked inner Development executable')
    out.mkdir(parents=True)
    spec=importlib.util.spec_from_file_location('s363_native_inputs',root/'Unreal/Tools/run_s362_native.py')
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
    before=module.execution_inputs(root)
    (out/'execution_inputs_before.json').write_text(json.dumps(before,indent=2),encoding='utf-8')
    user=boundary/'UpgradeSmokeRuntime'/('s364-'+uuid.uuid4().hex)
    assert not user.exists()
    cmd=[str(exe),'/Game/Tests/Campaign/L_Ch2VerdanSlice','-MemoriaUpgradeSmoke','-MemoriaGraphicsS363Smoke','-MemoriaS364Smoke','-UserDir='+str(user),'-unattended','-nop4','-nosplash','-RenderOffscreen','-ResX=1280','-ResY=720','-csvGpuStats','-csvCompression=0','-ExecCmds=csv.BlockOnCaptureEnd 1','-stdout','-FullStdOutLogOutput']
    result={'utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'command':cmd,'user_dir':str(user),'exe_sha256':digest(exe),'status':'RUNNING','provenance_limit':'Fresh cooked Development inspection; not Shipping, human input or a benchmark.'}
    receipt=out/'result.json'
    def save():receipt.write_text(json.dumps(result,indent=2),encoding='utf-8')
    save();started=time.monotonic()
    si=subprocess.STARTUPINFO();si.dwFlags|=subprocess.STARTF_USESHOWWINDOW;si.wShowWindow=0
    with (out/'runtime.log').open('w',encoding='utf-8') as stream:
        proc=subprocess.Popen(cmd,cwd=exe.parent,stdout=stream,stderr=subprocess.STDOUT,startupinfo=si,creationflags=subprocess.CREATE_NO_WINDOW)
        result['pid']=proc.pid;save()
        try:result['exit_code']=proc.wait(timeout=a.timeout)
        except subprocess.TimeoutExpired:proc.kill();proc.wait();result.update(exit_code=None,timeout=True)
    errors=[];reports=list(user.rglob('status.json')) if user.exists() else []
    if len(reports)!=1:errors.append('Require exactly one isolated runtime status')
    else:
        status=json.loads(reports[0].read_text(encoding='utf-8-sig'))
        result['runtime_status_path']=str(reports[0]);result['runtime_status']=status.get('status')
        shutil.copytree(reports[0].parent,out/'RuntimeArtifacts')
        if status.get('status')!='PASS':errors.append('Runtime did not report PASS')
        captures=status.get('captures',[])
        expected_names={'01_Verdan','02_Attack','03_BurnPicker','04_BurnWave','05_VerdanManualSaved','06_VerdanManualLoaded','07_Belt','08_Drift','09_DriftManualSaved','10_DriftManualLoaded','11_ExitQuestion','12_TitleVerdanLoaded','13_GuardHud','14_ComboChain','15_BlockOutcome','16_ParryOutcome','17_InteractionFocus'}
        names=[Path(c.get('path','')).stem for c in captures]
        if len(names)!=17 or len(set(names))!=17 or set(names)!=expected_names:errors.append('Expected exact17 unique actual native PNGs including combo/defense/focus')
        for cap in captures:
            path=Path(cap['path']).resolve()
            if not path.is_relative_to(user):errors.append('Capture escaped isolated directory');continue
            data=path.read_bytes()
            if data[:8]!=b'\x89PNG\r\n\x1a\n' or len(data)<24 or struct.unpack('>II',data[16:24])!=(1280,720):errors.append('Actual PNG header is not1280x720')
            if hashlib.sha1(data).hexdigest().upper()!=cap['sha1'].upper():errors.append('Capture SHA1 mismatch')
            if cap.get('width')!=1280 or cap.get('height')!=720:errors.append('Unexpected native PNG size')
        for key in ('s363_requested','s363_guard_clip_verified','s363_hud_non_overlap_verified','s363_picker_immediate_order_verified','title_verdan_manual_restore_verified','s364_requested','s364_early_combo_verified','s364_block_damage_feedback_verified','s364_parry_feedback_verified','s364_interaction_focus_verified'):
            if status.get(key) is not True:errors.append('Missing actual graphics verification: '+key)
        regions=status.get('regions',[])
        if {x.get('map') for x in regions}!={'verdan_market','belt_waystation','drift_shelter'}:errors.append('Expected three actual regions')
        for region in regions:
            if region.get('s363_solid_instances',0)<=0 or region.get('s363_soft_planes',0)<=0 or region.get('s363_invalid_collision_components',-1)!=0:errors.append('Missing/nonvisual S363 regional geometry: '+str(region.get('map')))
        result['engine_csv']=status.get('engine_csv',{})
        if result['engine_csv'].get('status')!='complete':errors.append('Owned engine CSV incomplete')
    log=(out/'runtime.log').read_text(encoding='utf-8',errors='replace')
    fatal=[line for line in log.splitlines() if any(x in line.lower() for x in ('fatal error','assertion failed:','unhandled exception:'))]
    if fatal:errors.append('Fatal native diagnostics')
    if result.get('exit_code')!=0:errors.append('Nonzero process exit or timeout')
    after=module.execution_inputs(root)
    (out/'execution_inputs_after.json').write_text(json.dumps(after,indent=2),encoding='utf-8')
    differences=module.changed_inputs(before,after)
    result['execution_inputs']={'files':len(before),'unchanged':before==after,'differences':differences}
    if before!=after:errors.append('Execution inputs changed during cooked inspection')
    result.update(status='PASS' if not errors else 'FAIL',validation_errors=errors,fatal_diagnostics=fatal,elapsed_seconds=round(time.monotonic()-started,3));save()
    print(json.dumps({k:result[k] for k in ('status','exit_code','validation_errors','elapsed_seconds')},ensure_ascii=False));return bool(errors)
if __name__=='__main__':raise SystemExit(main())
