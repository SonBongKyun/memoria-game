"""Independent Phase1I rendered snapshots vs unchanged H/G and executed I goldens."""
import argparse,copy,hashlib,json,struct
from pathlib import Path
from narrative_ir import ROOT,canonical
from validate_unreal import current_test_paths,inspect_automation_report
from validate_malet_deal_evidence import read,normalize,source_state

def run(a):
    directory=a.automation_dir.resolve();captures=directory/'Phase1I';out=a.evidence_dir.resolve()
    if out.exists():raise ValueError('Use fresh evidence path')
    out.mkdir(parents=True);checks=[];report={'status':'RUNNING','checks':checks}
    def check(name,ok):
        checks.append(dict(name=name,passed=bool(ok)))
        if not ok:raise ValueError(name)
    try:
        automation=read(directory/'automation_index.json')
        check('All exact prior90 plus new7 successful',inspect_automation_report(automation,current_test_paths())['passed'])
        hc=read(ROOT/'docs/unreal-migration/fixtures/malet_deal/contract_expected.v1.json')
        ic=read(ROOT/'docs/unreal-migration/fixtures/malet_reward/contract_expected.v1.json')
        for mode,hcase in [('CanonicalReward','accept_intact'),('CanonicalPayment','accept_intact'),('AlreadyBurnedPayment','already_burned'),('AcceptPreEffectDeferred','accept_intact')]:
            labels=('before_accept','after_accept','deal_first','deal_end','before_reward','reward_middle','reward_last','reward_completion','reward_effects_deferred')
            states={label:read(captures/(mode+'_'+label+'.json')) for label in labels}
            for label in labels[:4]:
                actual=states[label];expected=source_state(hc,hcase,label);start=max(i for i,e in enumerate(actual['trace']) if e=='interact:Malet')
                check(mode+': H exact trace '+label,normalize(actual['trace'][start:])==expected['events'])
                owned=[dict(id=m['id'],burned=m['bBurned'],residue=m['bResidue'],faded=m['bFaded'],erosion=m['erosion']) for m in actual['memory']['owned']]
                check(mode+': H exact memory '+label,owned==expected['memory_state'] and actual['memory']['burnedHistory']==expected['burned'])
            before=states['before_reward'];final=states['reward_effects_deferred']
            for label,source_label in [('before_reward','reward_first'),('reward_middle','reward_row_3'),('reward_last','reward_row_7'),('reward_completion','completion_before_callback'),('reward_effects_deferred','completion_before_callback')]:
                actual=states[label];expected=source_state(ic,'reward_en',source_label)
                start=max(i for i,e in enumerate(actual['trace']) if e=='interact:Malet');observed=normalize(actual['trace'][start:]);gold=expected['events'].copy()
                if label in ('reward_completion','reward_effects_deferred'):gold+=['callback:reward:enter','development:deferred:_on_reward_ended:before:ch2_malet_done']
                if hcase=='already_burned':
                    observed=observed[observed.index('field:start:malet_reward'):];gold=gold[gold.index('field:start:malet_reward'):]
                check(mode+': I exact source trace '+label,observed==gold)
                check(mode+': reward full memory/run preservation '+label,actual['memory']==before['memory'] and actual['run']==before['run'])
                check(mode+': same live run/domain '+label,actual['same_run_id'] and actual['same_memory_domain'])
                check(mode+': first flag absent '+label,not actual['ch2_malet_done_present'])
                check(mode+': no observed downstream effect events '+label,not any(actual['observed_downstream_effect_events'].values()))
                check(mode+': honest unmigrated owner state '+label,actual['world_memory_snapshot'] is None and actual['shop_snapshot'] is None)
            check(mode+': actual sword payment once',final['memory']['burnedHistory']==['daily_market_food','identity_first_sword'])
            check(mode+': one Field completion and callback intent',final['field_invocations']==4 and final['reward_field_invocations']==1 and final['reward_completions']==1 and final['reward_callback_intents']==1 and not final['reward_callback_pending'])
            check(mode+': no pending or invented completion timer',not final['normal_delay_pending'] and not final['reward_delay_pending'])
            check(mode+': source .3s and .5s world timers',type(final['actual_delay_microseconds']) is int and 299000<=final['actual_delay_microseconds']<350000 and type(final['actual_reward_delay_microseconds']) is int and 499000<=final['actual_reward_delay_microseconds']<550000)
            check(mode+': input cannot resume stopped effects',final['trace']==states['reward_completion']['trace'] and final['pawn_position']==before['pawn_position'])
            after=states['after_accept'];check(mode+': no effects since actual payment',after['run']==final['run'] and after['memory']==final['memory'])
            preserved=copy.deepcopy(final['run']);preserved['storyFlags']=[f for f in preserved['storyFlags'] if f['id'] not in ('malet_deal_accepted','talked_Malet_malet_encounter')]
            check(mode+': unrelated run preserved from before Accept',preserved==states['before_accept']['run'])
            if mode=='CanonicalReward':
                report['canonical']={k:final[k] for k in ('actual_delay_microseconds','actual_reward_delay_microseconds','deferred_target','same_run_id','same_memory_domain','field_invocations','reward_completions','reward_callback_intents','observed_downstream_effect_events')}
                report['canonical']['run_id']=final['run']['runId']
                (out/'reward_exact_trace.txt').write_text('\n'.join(normalize(final['trace'][start:]))+'\n',encoding='utf-8')
                (out/'canonical_full_trace.txt').write_text('\n'.join(final['trace'])+'\n',encoding='utf-8')
        for suffix in ('FieldOnRunReplace','BoundaryOnRunReplace','FieldOnWorldTeardown','BoundaryOnWorldTeardown'):
            label='cancelled_callback' if suffix.endswith('OnRunReplace') else 'world_callback_cancelled'
            x=read(captures/('CancelReward'+suffix+'_'+label+'.json'))
            check('Lifetime cancelled '+suffix,not x['reward_callback_pending'] and not x['normal_delay_pending'] and not x['reward_delay_pending'] and not x['ch2_malet_done_present'])
            if suffix.endswith('OnRunReplace'):check('Replacement cannot observe stale trace '+suffix,x['trace']==[] and x['reward_callback_intents']==0)
        gc=read(ROOT/'docs/unreal-migration/fixtures/malet_refusal/contract_expected.v1.json');g=source_state(gc,'refusal_retry','retry')['events']
        refusal=normalize(read(captures/'CanonicalRefusalRetry_retry_boundary.json')['trace']);start=next(i for i in range(len(refusal)) if refusal[i:i+len(g)]==g)
        check('Exact Refuse cleanup/retry regression',refusal[start:]==g)
        for p in captures.glob('*.json'):read(p)
        report['capture_json_count']=len(list(captures.glob('*.json')))
        report['captures']=[]
        for name in ('Reward_FirstLine','Reward_MiddleLine','Reward_LastLine','Reward_Completion','RewardEffects_Deferred'):
            p=captures/('CanonicalReward_'+name+'.png');data=p.read_bytes();size=struct.unpack('>II',data[16:24])
            check('Native readable PNG '+name,data[:8]==b'\x89PNG\r\n\x1a\n' and size[0]>=1280 and size[1]>=720)
            report['captures'].append(dict(path=str(p.relative_to(ROOT)),sha256=hashlib.sha256(data).hexdigest(),dimensions=list(size)))
        report.update(status='PASS',test_count=len(automation['tests']),warnings=sum(t['warnings'] for t in automation['tests']))
    except Exception as e:report.update(status='FAIL',error=str(e))
    (out/'acceptance.json').write_bytes(canonical(report));print('MEMORIA_MALET_REWARD_EVIDENCE_'+report['status']+' '+report.get('error',''));return 0 if report['status']=='PASS' else 1
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--automation-dir',type=Path,required=True);p.add_argument('--evidence-dir',type=Path,required=True);raise SystemExit(run(p.parse_args()))
