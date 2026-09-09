"""Independent Phase1J comparison against immutable I/H/G source goldens."""
import argparse,copy,hashlib,json,struct
from pathlib import Path
from narrative_ir import ROOT,canonical
from validate_unreal import current_test_paths,inspect_automation_report
from validate_malet_deal_evidence import read,normalize,source_state
FLAG='ch2_malet_done'
SUFFIX=['callback:reward:enter','flag:'+FLAG,'development:deferred:before:world_memory_seed']
def flag_delta(run):
    out=copy.deepcopy(run)
    for f in out['storyFlags']:
        if f['id']==FLAG:f['bValue']=True;break
    else:out['storyFlags'].append(dict(id=FLAG,bValue=True))
    return out
def validate(a):
    d=a.automation_dir.resolve();cap=d/'Phase1J';out=a.evidence_dir.resolve();out.mkdir(exist_ok=False,parents=True)
    checks=[];report=dict(status='RUNNING',checks=checks)
    def check(n,b):
        checks.append(dict(name=n,passed=bool(b)))
        if not b:raise ValueError(n)
    try:
        index=read(d/'automation_index.json');check('All retained97 plus new10 exact identities pass',inspect_automation_report(index,current_test_paths())['passed'])
        ig=read(ROOT/'docs/unreal-migration/fixtures/malet_reward/contract_expected.v1.json')
        hg=read(ROOT/'docs/unreal-migration/fixtures/malet_deal/contract_expected.v1.json')
        modes=['FirstEffectCanonical','FirstEffectPreexistingTrue','FirstEffectPreexistingFalse','CanonicalReward','CanonicalPayment','AlreadyBurnedPayment','AcceptPreEffectDeferred']
        for mode in modes:
            def state(label):return read(cap/(mode+'_'+label+'.json'))
            before=state('before_reward');last=state('reward_last');completed=state('reward_completion');after=state('after_flag');stop=state('reward_effects_deferred')
            expected=flag_delta(before['run']);hcase='already_burned' if mode=='AlreadyBurnedPayment' else 'accept_intact'
            for label in ('before_accept','after_accept','deal_first','deal_end'):
                x=state(label);gold=source_state(hg,hcase,label);start=max(i for i,e in enumerate(x['trace']) if e=='interact:Malet')
                check(mode+' H exact '+label,normalize(x['trace'][start:])==gold['events'])
            for label,source_label,done in [('before_reward','reward_first',False),('reward_middle','reward_row_3',False),('reward_last','reward_row_7',False),('reward_completion','completion_before_callback',True),('reward_effects_deferred','completion_before_callback',True)]:
                x=state(label);gold=source_state(ig,'reward_en',source_label)['events'].copy()
                if done:gold+=SUFFIX
                start=max(i for i,e in enumerate(x['trace']) if e=='interact:Malet');observed=normalize(x['trace'][start:])
                if hcase=='already_burned':gold=gold[gold.index('field:start:malet_reward'):];observed=observed[observed.index('field:start:malet_reward'):]
                check(mode+' exact I history and J suffix '+label,observed==gold)
                check(mode+' only authorized flag delta '+label,x['run']==(expected if done else before['run']))
                check(mode+' same full memory and run/domain '+label,x['memory']==before['memory'] and x['same_run_id'] and x['same_memory_domain'])
            for x in (completed,after,stop):
                check(mode+' true entry remains at STOP '+x['state'],x['ch2_malet_done_present'] and x['ch2_malet_done_value'] and x['run']==expected)
                counts=x['observed_downstream_effect_events'].copy();check(mode+' one flag event '+x['state'],counts.pop('flag:'+FLAG)==1)
                check(mode+' no world/inventory/shop/chapter/save/achievement events '+x['state'],not any(counts.values()))
                check(mode+' unimplemented owners explicitly null '+x['state'],x['world_memory_snapshot'] is None and x['shop_snapshot'] is None)
            for point in ('before_callback','before_flag','after_flag'):
                x=state('synchronous_'+point);pre=point!='after_flag'
                check(mode+' synchronous run snapshot '+point,x['run']==(before['run'] if pre else expected))
                check(mode+' synchronous memory snapshot '+point,x['memory']==before['memory'])
                check(mode+' synchronous prefix '+point,normalize(x['trace'])==normalize(stop['trace'])[:len(x['trace'])])
                check(mode+' no defer or seed at callback/flag snapshot '+point,not any(e.startswith(('development:deferred:','world:')) for e in x['trace']))
            if mode=='FirstEffectCanonical':
                check('canonical absent until callback',not before['ch2_malet_done_present'] and not last['ch2_malet_done_present'])
                (out/'canonical_full_trace.txt').write_text('\n'.join(stop['trace'])+'\n',encoding='utf-8')
                report['canonical']=stop
            check(mode+' final stop cannot move or continue',stop['trace']==completed['trace'] and stop['pawn_position']==before['pawn_position'])
            check(mode+' actual food/sword history',stop['memory']['burnedHistory']==['daily_market_food','identity_first_sword'])
            check(mode+' source timer contracts',299000<=stop['actual_delay_microseconds']<350000 and 499000<=stop['actual_reward_delay_microseconds']<550000)
            check(mode+' one completion and no pending callback/timer',stop['reward_completions']==1 and stop['reward_callback_intents']==1 and stop['field_invocations']==4 and not any(stop[k] for k in ('normal_delay_pending','reward_delay_pending','reward_callback_pending')))
        for point in ('BeforeCallback','BeforeFlag','AfterFlag'):
            for lifetime in ('OnRunReplace','OnWorldTeardown'):
                mode='FirstEffectCancel'+point+lifetime;b=read(cap/(mode+'_lifetime_before_cancel.json'))
                x=read(cap/(mode+('_cancelled_callback.json' if lifetime=='OnRunReplace' else '_world_callback_cancelled.json')))
                check(mode+' exact commit point',b['ch2_malet_done_present']==(point=='AfterFlag'))
                check(mode+' no stale callbacks or timers',not any(x[k] for k in ('normal_delay_pending','reward_delay_pending','reward_callback_pending')))
                if lifetime=='OnRunReplace':
                    check(mode+' replacement has no old flag',not x['ch2_malet_done_present'] and x['trace']==[] and x['run']['runId']!=b['run']['runId'])
                else:check(mode+' teardown preserves committed old run without rollback',x['run']==b['run'] and x['memory']==b['memory'] and x['trace']==b['trace'])
        for suffix in ('FieldOnRunReplace','BoundaryOnRunReplace','FieldOnWorldTeardown','BoundaryOnWorldTeardown'):
            x=read(cap/('CancelReward'+suffix+('_cancelled_callback.json' if suffix.endswith('OnRunReplace') else '_world_callback_cancelled.json')))
            check('Retained I lifetime '+suffix,not x['reward_callback_pending'] and x['ch2_malet_done_present']==(suffix=='BoundaryOnWorldTeardown'))
        gg=read(ROOT/'docs/unreal-migration/fixtures/malet_refusal/contract_expected.v1.json');gold=source_state(gg,'refusal_retry','retry')['events']
        events=normalize(read(cap/'CanonicalRefusalRetry_retry_boundary.json')['trace'])
        check('Exact Refuse cleanup/retry history',events[-len(gold):]==gold)
        for p in cap.glob('*.json'):read(p)
        report['snapshot_count']=len(list(cap.glob('*.json')));report['captures']=[]
        for name in ('Reward_LastLine','Reward_Completion','MaletDone_Set','WorldSeed_Deferred'):
            p=cap/('FirstEffectCanonical_'+name+'.png');data=p.read_bytes();size=struct.unpack('>II',data[16:24])
            check('Native readable capture '+name,data[:8]==b'\x89PNG\r\n\x1a\n' and size[0]>=1280 and size[1]>=720)
            report['captures'].append(dict(path=str(p.relative_to(ROOT)),sha256=hashlib.sha256(data).hexdigest(),dimensions=size))
        report.update(status='PASS',test_count=len(index['tests']),warnings=sum(t['warnings'] for t in index['tests']))
    except Exception as e:report.update(status='FAIL',error=str(e))
    (out/'acceptance.json').write_bytes(canonical(report));print('MEMORIA_FIRST_EFFECT_EVIDENCE_'+report['status']+' '+report.get('error',''))
    return 0 if report['status']=='PASS' else 1
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--automation-dir',type=Path,required=True);p.add_argument('--evidence-dir',type=Path,required=True);raise SystemExit(validate(p.parse_args()))
