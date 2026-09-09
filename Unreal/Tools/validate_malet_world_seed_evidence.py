"""Independent complete-state/source trace comparison for Phase1K."""
import argparse,copy,json,struct,hashlib
from pathlib import Path
from narrative_ir import ROOT,canonical
from validate_unreal import current_test_paths,inspect_automation_report
from validate_malet_deal_evidence import read,normalize,source_state
from validate_malet_first_effect_evidence import flag_delta
SUFFIX=['callback:reward:enter','flag:ch2_malet_done','worldseed:enter','world:knowledge:npc.malet:fact.bl07.route_request_received','world:revision:1','world:memory:npc.malet:memory.malet.bl07_request_source','world:revision:2','worldseed:end','development:deferred:before:item:potion:2']
def validate(a):
    d=a.automation_dir.resolve();cap=d/'Phase1K';out=a.evidence_dir.resolve();out.mkdir(parents=True,exist_ok=False)
    checks=[];report={'status':'RUNNING','checks':checks}
    def check(n,b):
        checks.append({'name':n,'passed':bool(b)})
        if not b:raise ValueError(n)
    try:
        index=read(d/'automation_index.json');check('Exact old107 plus new18 identities PASS',inspect_automation_report(index,current_test_paths())['passed'])
        prior=read(ROOT/'docs/unreal-migration/evidence/phase1j/automation03/automation_index.json');old_ids={t['fullTestPath'] for t in prior['tests']};new_ids={t['fullTestPath'] for t in index['tests']};check('Exact accepted Phase1J107 identities retained',len(old_ids)==107 and old_ids.issubset(new_ids) and len(new_ids-old_ids)==18)
        gold=read(ROOT/'docs/unreal-migration/fixtures/malet_world_seed/contract_expected.v1.json');fresh=gold[0]
        ig=read(ROOT/'docs/unreal-migration/fixtures/malet_reward/contract_expected.v1.json');hg=read(ROOT/'docs/unreal-migration/fixtures/malet_deal/contract_expected.v1.json')
        modes=['WorldSeedCanonical','FirstEffectCanonical','FirstEffectPreexistingTrue','FirstEffectPreexistingFalse','CanonicalReward','CanonicalPayment','AlreadyBurnedPayment','AcceptPreEffectDeferred']
        for mode in modes:
            def state(label):return read(cap/(mode+'_'+label+'.json'))
            before=state('before_reward');stop=state('pre_potion_deferred');expected=flag_delta(before['run']);hcase='already_burned' if mode=='AlreadyBurnedPayment' else 'accept_intact'
            for label in ('before_accept','after_accept','deal_first','deal_end'):
                x=state(label);g=source_state(hg,hcase,label);start=max(i for i,e in enumerate(x['trace']) if e=='interact:Malet');check(mode+' exact H '+label,normalize(x['trace'][start:])==g['events'])
            for label,source_label,done in [('before_reward','reward_first',False),('reward_middle','reward_row_3',False),('reward_last','reward_row_7',False),('reward_completion','completion_before_callback',True),('pre_potion_deferred','completion_before_callback',True)]:
                x=state(label);g=source_state(ig,'reward_en',source_label)['events'].copy()
                if done:g+=SUFFIX
                start=max(i for i,e in enumerate(x['trace']) if e=='interact:Malet');obs=normalize(x['trace'][start:])
                if hcase=='already_burned':g=g[g.index('field:start:malet_reward'):];obs=obs[obs.index('field:start:malet_reward'):]
                check(mode+' exact I/J/K trace '+label,obs==g)
                check(mode+' only flag in run delta '+label,x['run']==(expected if done else before['run']))
                check(mode+' complete Player Memory unchanged '+label,x['memory']==before['memory'] and x['player_observables']==before['player_observables'])
                check(mode+' Run/player/world identities '+label,all(x[k] for k in ('same_run_id','same_memory_domain','same_world_domain')))
                check(mode+' complete world state '+label,x['world_memory_snapshot']==fresh['after' if done else 'before'])
            for label,world in [('before_world_seed',fresh['before']),('after_knowledge',fresh['steps'][0]['world']),('after_memory',fresh['steps'][1]['world']),('world_seed_complete',fresh['after'])]:
                x=state(label);check(mode+' synchronous world '+label,x['world_memory_snapshot']==world);check(mode+' synchronous memory/run '+label,x['memory']==before['memory'] and x['player_observables']==before['player_observables'] and x['run']==expected)
                check(mode+' ordered prefix '+label,x['trace']==stop['trace'][:len(x['trace'])])
            check(mode+' inventory/recent items/chapter exact',stop['run']['player']==before['run']['player'] and stop['run']['currentChapter']==before['run']['currentChapter'])
            check(mode+' zero banned downstream calls',all(stop['observed_downstream_effect_events'][p]==0 for p in ('item:','shop:','chapter:','autosave:','achievement:')))
            check(mode+' source timer durations',299000<=stop['actual_delay_microseconds']<350000 and 499000<=stop['actual_reward_delay_microseconds']<550000)
            check(mode+' one completion/no stale timers',stop['reward_completions']==stop['reward_callback_intents']==1 and stop['field_invocations']==4 and not any(stop[k] for k in ('normal_delay_pending','reward_delay_pending','reward_callback_pending')))
            check(mode+' prepotion stop stable',stop['deferred_target']=='before:item:potion:2' and stop['trace']==state('reward_completion')['trace'] and stop['pawn_position']==before['pawn_position'])
            check(mode+' paid food sword history',stop['memory']['burnedHistory']==['daily_market_food','identity_first_sword'])
            if mode=='WorldSeedCanonical':
                report['canonical']=stop;(out/'canonical_full_trace.txt').write_text('\n'.join(stop['trace'])+'\n',encoding='utf-8')
                for label in ('before_world_seed','after_knowledge','after_memory','world_seed_complete','pre_potion_deferred'):(out/(label+'.json')).write_bytes(canonical(state(label)))
        for c in gold:
            x=read(cap/('source_'+c['id']+'.json'));check('source case exact '+c['id'],all(x[k]==c[k] for k in ('before','after','events','steps')));check('source case separation '+c['id'],x['player_unchanged'] and x['run_unchanged'] and x['player_observables_unchanged'])
        save=read(cap/'world_save_roundtrip.json');check('Real binary SaveGame roundtrip',save['binary_bytes']>0 and save['save_schema']==save['world_schema']==1)
        for section in ('world','run','player','player_observables'):check(section+' independent save roundtrip',save['before_'+section]==save['after_'+section])
        check('Post-restore no-op',save['post_restore_events']==0);(out/'world_save_roundtrip.json').write_bytes(canonical(save))
        for point in ('BeforeCallback','BeforeFlag','AfterFlag'):
            for lifetime in ('OnRunReplace','OnWorldTeardown'):
                mode='FirstEffectCancel'+point+lifetime;b=read(cap/(mode+'_lifetime_before_cancel.json'));x=read(cap/(mode+('_cancelled_callback.json' if lifetime=='OnRunReplace' else '_world_callback_cancelled.json')))
                check(mode+' exact flag seam',b['ch2_malet_done_present']==(point=='AfterFlag'))
                check(mode+' no stale callback/timer',not any(x[k] for k in ('normal_delay_pending','reward_delay_pending','reward_callback_pending')))
                if lifetime=='OnRunReplace':check(mode+' clean new run/world',not x['ch2_malet_done_present'] and x['trace']==[] and x['run']['runId']!=b['run']['runId'] and x['world_memory_snapshot']==fresh['before'])
                else:check(mode+' preserve old committed state across native travel',all(x[k]==b[k] for k in ('run','memory','trace','world_memory_snapshot')))
        for suffix in ('FieldOnRunReplace','BoundaryOnRunReplace','FieldOnWorldTeardown','BoundaryOnWorldTeardown'):
            x=read(cap/('CancelReward'+suffix+('_cancelled_callback.json' if suffix.endswith('OnRunReplace') else '_world_callback_cancelled.json')))
            check('Retained I lifetime '+suffix,not x['reward_callback_pending'] and x['world_memory_snapshot']==fresh['after' if suffix=='BoundaryOnWorldTeardown' else 'before'])
        gg=read(ROOT/'docs/unreal-migration/fixtures/malet_refusal/contract_expected.v1.json');g=source_state(gg,'refusal_retry','retry')['events'];events=normalize(read(cap/'CanonicalRefusalRetry_retry_boundary.json')['trace']);check('Refuse cleanup/retry unchanged',events[-len(g):]==g)
        for p in cap.glob('*.json'):read(p)
        report['snapshot_count']=len(list(cap.glob('*.json')));report['captures']=[]
        for name in ('MaletDone_Set','WorldKnowledge_Seeded','WorldMemory_Seeded','PotionReward_Deferred'):
            p=cap/('WorldSeedCanonical_'+name+'.png');raw=p.read_bytes();size=struct.unpack('>II',raw[16:24]);check('Native capture '+name,raw[:8]==b'\x89PNG\r\n\x1a\n' and size[0]>=1280 and size[1]>=720);report['captures'].append(dict(path=str(p.relative_to(ROOT)),sha256=hashlib.sha256(raw).hexdigest(),dimensions=size))
        report.update(status='PASS',test_count=len(index['tests']),warnings=sum(t['warnings'] for t in index['tests']))
    except Exception as e:report.update(status='FAIL',error=str(e))
    (out/'acceptance.json').write_bytes(canonical(report));print('MEMORIA_WORLD_SEED_EVIDENCE_'+report['status']+' '+report.get('error',''));return int(report['status']!='PASS')
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--automation-dir',type=Path,required=True);p.add_argument('--evidence-dir',type=Path,required=True);raise SystemExit(validate(p.parse_args()))
