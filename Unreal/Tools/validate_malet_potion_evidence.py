"""Independent complete-state/source trace comparison for Phase1L."""
import argparse,copy,json,struct,hashlib
from pathlib import Path
from narrative_ir import ROOT,canonical
from validate_unreal import current_test_paths,inspect_automation_report
from validate_malet_deal_evidence import read,normalize,source_state
from validate_malet_first_effect_evidence import flag_delta
SUFFIX=['callback:reward:enter','flag:ch2_malet_done','worldseed:enter','world:knowledge:npc.malet:fact.bl07.route_request_received','world:revision:1','world:memory:npc.malet:memory.malet.bl07_request_source','world:revision:2','worldseed:end','item:add:begin:potion:2','inventory:potion:0->2','recent_items:potion','inventory_changed:potion','toast:+2 Potion:1','item:add:end:potion:2','development:deferred:before:item:antidote:1']
def potion_delta(run):
    result=copy.deepcopy(run);result['player']['items']=[{'id':'potion','count':2}];result['player']['recentItems']=['potion'];return result

def inventory(run):
    return {'items':{x['id']:x['count'] for x in run['player']['items']},'recent':run['player']['recentItems']}

def validate(a):
    d=a.automation_dir.resolve();cap=d/'Phase1L';out=a.evidence_dir.resolve();out.mkdir(parents=True,exist_ok=False)
    checks=[];report={'status':'RUNNING','checks':checks}
    def check(n,b):
        checks.append({'name':n,'passed':bool(b)})
        if not b:raise ValueError(n)
    try:
        index=read(d/'automation_index.json');check('Exact old125 plus new22 identities PASS',inspect_automation_report(index,current_test_paths())['passed'])
        prior=read(ROOT/'docs/unreal-migration/evidence/phase1k/automation04/automation_index.json');old_ids={t['fullTestPath'] for t in prior['tests']};new_ids={t['fullTestPath'] for t in index['tests']};check('Exact accepted Phase1K125 identities retained',len(old_ids)==125 and old_ids.issubset(new_ids) and len(new_ids-old_ids)==22)
        gold=read(ROOT/'docs/unreal-migration/fixtures/malet_world_seed/contract_expected.v1.json');fresh=gold[0]
        ig=read(ROOT/'docs/unreal-migration/fixtures/malet_reward/contract_expected.v1.json');hg=read(ROOT/'docs/unreal-migration/fixtures/malet_deal/contract_expected.v1.json')
        modes=['PotionCanonical','WorldSeedCanonical','FirstEffectCanonical','FirstEffectPreexistingTrue','FirstEffectPreexistingFalse','CanonicalReward','CanonicalPayment','AlreadyBurnedPayment','AcceptPreEffectDeferred']
        for mode in modes:
            def state(label):return read(cap/(mode+'_'+label+'.json'))
            before=state('before_reward');stop=state('antidote_deferred');expected=flag_delta(before['run']);hcase='already_burned' if mode=='AlreadyBurnedPayment' else 'accept_intact'
            for label in ('before_accept','after_accept','deal_first','deal_end'):
                x=state(label);g=source_state(hg,hcase,label);start=max(i for i,e in enumerate(x['trace']) if e=='interact:Malet');check(mode+' exact H '+label,normalize(x['trace'][start:])==g['events'])
            for label,source_label,done in [('before_reward','reward_first',False),('reward_middle','reward_row_3',False),('reward_last','reward_row_7',False),('reward_completion','completion_before_callback',True),('antidote_deferred','completion_before_callback',True)]:
                x=state(label);g=source_state(ig,'reward_en',source_label)['events'].copy()
                if done:g+=SUFFIX
                start=max(i for i,e in enumerate(x['trace']) if e=='interact:Malet');obs=normalize(x['trace'][start:])
                if hcase=='already_burned':g=g[g.index('field:start:malet_reward'):];obs=obs[obs.index('field:start:malet_reward'):]
                check(mode+' exact I/J/K trace '+label,obs==g)
                check(mode+' only flag in run delta '+label,x['run']==(potion_delta(expected) if done else before['run']))
                check(mode+' complete Player Memory unchanged '+label,x['memory']==before['memory'] and x['player_observables']==before['player_observables'])
                check(mode+' Run/player/world identities '+label,all(x[k] for k in ('same_run_id','same_memory_domain','same_world_domain')))
                check(mode+' complete world state '+label,x['world_memory_snapshot']==fresh['after' if done else 'before'])
            for label,world in [('before_world_seed',fresh['before']),('after_knowledge',fresh['steps'][0]['world']),('after_memory',fresh['steps'][1]['world']),('world_seed_complete',fresh['after'])]:
                x=state(label);check(mode+' synchronous world '+label,x['world_memory_snapshot']==world);check(mode+' synchronous memory/run '+label,x['memory']==before['memory'] and x['player_observables']==before['player_observables'] and x['run']==expected)
                check(mode+' ordered prefix '+label,x['trace']==stop['trace'][:len(x['trace'])])
            check(mode+' only potion/recent run delta',stop['run']==potion_delta(expected))
            check(mode+' zero banned downstream calls',all(stop['observed_downstream_effect_events'][p]==0 for p in ('shop:','chapter:','autosave:','achievement:')))
            check(mode+' source timer durations',299000<=stop['actual_delay_microseconds']<350000 and 499000<=stop['actual_reward_delay_microseconds']<550000)
            check(mode+' one completion/no stale timers',stop['reward_completions']==stop['reward_callback_intents']==1 and stop['field_invocations']==4 and not any(stop[k] for k in ('normal_delay_pending','reward_delay_pending','reward_callback_pending')))
            check(mode+' prepotion stop stable',stop['deferred_target']=='before:item:antidote:1' and stop['trace']==state('reward_completion')['trace'] and stop['pawn_position']==before['pawn_position'])
            check(mode+' paid food sword history',stop['memory']['burnedHistory']==['daily_market_food','identity_first_sword'])
            if mode=='PotionCanonical':
                report['canonical']=stop;(out/'canonical_full_trace.txt').write_text('\n'.join(stop['trace'])+'\n',encoding='utf-8')
                for label in ('before_world_seed','after_knowledge','after_memory','world_seed_complete','before_potion','after_inventory_mutation','after_recent_items','after_inventory_changed','potion_complete','antidote_deferred'):(out/(label+'.json')).write_bytes(canonical(state(label)))
        potion_gold=read(ROOT/'docs/unreal-migration/fixtures/malet_potion/contract_expected.v1.json')
        for mode in modes:
            def state(label):return read(cap/(mode+'_'+label+'.json'))
            before=state('before_potion');check(mode+' real Phase1K state before potion',before['world_memory_snapshot']==fresh['after'] and inventory(before['run'])==potion_gold[0]['before'])
            for label,step in zip(('after_inventory_mutation','after_recent_items','after_inventory_changed','potion_complete'),potion_gold[0]['steps']):
                x=state(label);check(mode+' exact source potion snapshot '+label,inventory(x['run'])==step['state']);check(mode+' all memory unchanged '+label,all(x[k]==before[k] for k in ('memory','player_observables','world_memory_snapshot')))
                check(mode+' snapshot is synchronous trace prefix '+label,state('antidote_deferred')['trace'][:len(x['trace'])]==x['trace'])
            stop=state('antidote_deferred');trace=stop['trace'];check(mode+' one signal and toast',trace.count('inventory_changed:potion')==trace.count('toast:+2 Potion:1')==1)
            check(mode+' no antidote/firebomb/shop/transition',not any(any(e.startswith(p) for p in ('inventory:antidote','inventory:firebomb','inventory_changed:antidote','inventory_changed:firebomb','shop:','chapter:','autosave:','achievement:','chapter_transition:')) for e in trace))
        for c in potion_gold:
            x=read(cap/('potion_source_'+c['id']+'.json'));steps=c['steps'][:-1] if c['id']=='replacement_signal' else c['steps']
            check('potion source events '+c['id'],x['steps']==steps);check('potion source state '+c['id'],x['before']['inventory']==c['before'] and x['after']['inventory']==c['after'])
            for section in ('player','player_observables','world'):check('potion source preserved '+c['id']+section,x['before'][section]==x['after'][section])
            sv=read(cap/('potion_save_'+c['id']+'.json'));check('potion binary save '+c['id'],sv['binary_bytes']>0 and sv['schema']==1 and sv['before']==sv['after'])
        save_potion=read(cap/'save_roundtrip.json');check('Canonical actual binary save all independent sections',save_potion['binary_bytes']>0 and save_potion['schema']==1 and save_potion['before']==save_potion['after'])
        for section in ('run','inventory','player','player_observables','world'):check('canonical independent binary section '+section,save_potion['before'][section]==save_potion['after'][section])
        (out/'save_roundtrip.json').write_bytes(canonical(save_potion))
        for mode,granted in [('PotionCancelBeforePotionOnRunReplace',False),('PotionCancelAfterCommitOnRunReplace',True)]:
            b=read(cap/(mode+'_lifetime_before_cancel.json'));x=read(cap/(mode+'_cancelled_callback.json'))
            check(mode+' exact commit seam',inventory(b['run'])=={'items':{'potion':2} if granted else {},'recent':['potion'] if granted else []})
            check(mode+' no new run leakage',x['run']['runId']!=b['run']['runId'] and inventory(x['run'])=={'items':{},'recent':[]} and x['trace']==[] and x['world_memory_snapshot']==fresh['before'])
        x=read(cap/'PotionAfterStopOnRunReplace_cancelled_callback.json');check('Replacement at stop clean',inventory(x['run'])=={'items':{},'recent':[]} and x['trace']==[])
        b=read(cap/'PotionAfterStopOnWorldTeardown_reward_completion.json');x=read(cap/'PotionAfterStopOnWorldTeardown_world_callback_cancelled.json');check('World teardown retains committed potion/world/player/run',all(x[k]==b[k] for k in ('run','memory','player_observables','world_memory_snapshot','trace')))
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
        for name in ('WorldSeed_Complete','Potion_Before','Potion_Granted','Potion_Toast','Antidote_Deferred'):
            p=cap/('PotionCanonical_'+name+'.png');raw=p.read_bytes();size=struct.unpack('>II',raw[16:24]);check('Native capture '+name,raw[:8]==b'\x89PNG\r\n\x1a\n' and size[0]>=1280 and size[1]>=720);report['captures'].append(dict(path=str(p.relative_to(ROOT)),sha256=hashlib.sha256(raw).hexdigest(),dimensions=size))
        report.update(status='PASS',test_count=len(index['tests']),warnings=sum(t['warnings'] for t in index['tests']))
    except Exception as e:report.update(status='FAIL',error=str(e))
    (out/'acceptance.json').write_bytes(canonical(report));print('MEMORIA_POTION_EVIDENCE_'+report['status']+' '+report.get('error',''));return int(report['status']!='PASS')
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--automation-dir',type=Path,required=True);p.add_argument('--evidence-dir',type=Path,required=True);raise SystemExit(validate(p.parse_args()))
