"""Independently verify rendered Phase 1H snapshots against executed Godot goldens."""
import argparse,copy,hashlib,json,struct
from pathlib import Path
from narrative_ir import ROOT,canonical
from validate_unreal import current_test_paths,inspect_automation_report

def read(path):return json.loads(path.read_text(encoding='utf-8-sig'))
def normalize(trace):
    return [e[6:] if e.startswith('field:') and not e.startswith(('field:start:','field:skip:')) else e for e in trace]
def source_state(cases,case,label):
    return next(s for c in cases if c['id']==case for s in c['states'] if s['label']==label)
def run(a):
    directory=a.automation_dir.resolve();captures=directory/'Phase1H';out=a.evidence_dir.resolve()
    if out.exists():raise ValueError('Use fresh independent evidence directory')
    out.mkdir(parents=True)
    checks=[]
    def check(name,ok):
        checks.append(dict(name=name,passed=bool(ok)))
        if not ok:raise ValueError(name)
    report={'status':'RUNNING','checks':checks}
    try:
        automation=read(directory/'automation_index.json')
        check('All exact 90 distinct Unreal identities successful',inspect_automation_report(automation,current_test_paths())['passed'])
        cases=read(ROOT/'docs/unreal-migration/fixtures/malet_deal/contract_expected.v1.json')
        for mode,case in [('CanonicalPayment','accept_intact'),('AlreadyBurnedPayment','already_burned'),('AcceptPreEffectDeferred','accept_intact')]:
            states={label:read(captures/(mode+'_'+label+'.json')) for label in ('normal_first','before_accept','after_accept','deal_first','deal_end','reward_boundary')}
            for label in ('before_accept','after_accept','deal_first','deal_end','reward_boundary'):
                actual=states[label];expected=source_state(cases,case,label)
                start=max(i for i,e in enumerate(actual['trace']) if e=='interact:Malet')
                check(mode+': exact source trace '+label,normalize(actual['trace'][start:])==expected['events'])
                owned=[dict(id=m['id'],burned=m['bBurned'],residue=m['bResidue'],faded=m['bFaded'],erosion=m['erosion']) for m in actual['memory']['owned']]
                check(mode+': all source memory states '+label,owned==expected['memory_state'] and actual['memory']['burnedHistory']==expected['burned'])
                check(mode+': live run/domain '+label,actual['same_run_id'] and actual['same_memory_domain'])
            before=states['before_accept'];after=states['after_accept'];final=states['reward_boundary']
            without_flags=copy.deepcopy(final['run'])
            without_flags['storyFlags']=[f for f in without_flags['storyFlags'] if f['id'] not in ('malet_deal_accepted','talked_Malet_malet_encounter')]
            check(mode+': entire unrelated run unchanged',without_flags==before['run'])
            check(mode+': no later memory or run effects',after['memory']==final['memory'] and after['run']==final['run'])
            # Whole memory DTO outside the actual sword/history delta must stay exact.
            reconstructed=copy.deepcopy(final['memory'])
            reconstructed['owned']=copy.deepcopy(before['memory']['owned'])
            reconstructed['burnedHistory']=copy.deepcopy(before['memory']['burnedHistory'])
            check(mode+': memory aggregate outside owned/history preserved',reconstructed==before['memory'])
            check(mode+': history order and single sword',final['memory']['burnedHistory']==['daily_market_food','identity_first_sword'])
            check(mode+': reward pre-execution defer',final['deferred_target']=='malet_reward' and final['field_invocations']==3 and final['reaction_invocations']==1)
            check(mode+': two separate measured timers',type(final['actual_delay_microseconds']) is int and 299000<=final['actual_delay_microseconds']<350000 and type(final['actual_reward_delay_microseconds']) is int and 499000<=final['actual_reward_delay_microseconds']<550000)
            requests=[e for e in normalize(final['trace'][max(i for i,e in enumerate(final['trace']) if e=='interact:Malet'):]) if e.startswith('request:')]
            check(mode+': no downstream requests',requests==['request:res://data/chapter2_dialogue.json::'+g for g in ('malet_encounter','malet_deal','malet_reward')])
            if mode=='CanonicalPayment':
                report['canonical']={k:final[k] for k in ('actual_delay_microseconds','actual_reward_delay_microseconds','deferred_target','same_run_id','same_memory_domain','field_invocations','reaction_invocations')}
                report['canonical'].update(run_id=final['run']['runId'],burned_history=final['memory']['burnedHistory'],hp=final['run']['player']['hp'],grains=final['run']['player']['grains'])
                (out/'accept_exact_trace.txt').write_text('\n'.join(normalize(final['trace'][start:]))+'\n',encoding='utf-8')
        refusal=read(captures/'CanonicalRefusalRetry_retry_boundary.json')
        gcases=read(ROOT/'docs/unreal-migration/fixtures/malet_refusal/contract_expected.v1.json')
        gexpected=source_state(gcases,'refusal_retry','retry')['events']
        normalized=normalize(refusal['trace'])
        first=next(i for i in range(len(normalized)) if normalized[i:i+len(gexpected)]==gexpected)
        check('Unchanged exact source Refuse cleanup/retry suffix',normalized[first:]==gexpected)
        # Every rendered snapshot must remain valid JSON, including cancellation paths.
        report['capture_json_count']=len(list(captures.glob('*.json')))
        for path in captures.glob('*.json'):read(path)
        check('All snapshot JSON parses',True)
        names=['NormalEncounter_Choices','Accept_Selected','Deal_FirstLine','Deal_MiddleLine','Deal_LastLine','RewardRequest_Deferred']
        report['captures']=[]
        for name in names:
            path=captures/('CanonicalPayment_'+name+'.png');data=path.read_bytes()
            size=struct.unpack('>II',data[16:24])
            check('Rendered PNG '+name,data[:8]==b'\x89PNG\r\n\x1a\n' and size[0]>=1280 and size[1]>=720)
            if report['captures']:check('Consistent native window capture '+name,list(size)==report['captures'][0]['dimensions'])
            report['captures'].append(dict(path=str(path.relative_to(ROOT)),sha256=hashlib.sha256(data).hexdigest(),dimensions=list(size)))
        report.update(status='PASS',test_count=len(automation['tests']),warnings=sum(t['warnings'] for t in automation['tests']),visual_inspection='Required separately; PNG presence is not visual approval.')
    except Exception as e:report.update(status='FAIL',error=str(e))
    (out/'acceptance.json').write_bytes(canonical(report))
    print('MEMORIA_MALET_DEAL_EVIDENCE_'+report['status']+' '+report.get('error',''))
    return 0 if report['status']=='PASS' else 1
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--automation-dir',type=Path,required=True);p.add_argument('--evidence-dir',type=Path,required=True)
    raise SystemExit(run(p.parse_args()))
