"""Independent complete-state/source trace comparison for Phase1N."""
import argparse,copy,json,struct,hashlib
from pathlib import Path
from narrative_ir import ROOT,canonical
from validate_unreal import current_test_paths,inspect_automation_report,shop_test_paths
from validate_malet_deal_evidence import read,normalize,source_state
from validate_malet_first_effect_evidence import flag_delta
SUFFIX=['callback:reward:enter','flag:ch2_malet_done','worldseed:enter','world:knowledge:npc.malet:fact.bl07.route_request_received','world:revision:1','world:memory:npc.malet:memory.malet.bl07_request_source','world:revision:2','worldseed:end','item:add:begin:potion:2','inventory:potion:0->2','recent_items:potion','inventory_changed:potion','toast:+2 Potion:1','item:add:end:potion:2','item:add:begin:antidote:1','inventory:antidote:0->1','recent_items:antidote,potion','inventory_changed:antidote','toast:+1 Antidote:1','item:add:end:antidote:1','item:add:begin:firebomb:1','inventory:firebomb:0->1','recent_items:firebomb,antidote,potion','inventory_changed:firebomb','toast:+1 Firebomb:1','item:add:end:firebomb:1','development:deferred:before:shop_open']
def potion_delta(run):
    result=copy.deepcopy(run);result['player']['items']=[{'id':'potion','count':2},{'id':'antidote','count':1},{'id':'firebomb','count':1}];result['player']['recentItems']=['firebomb','antidote','potion'];return result

def inventory(run):
    return {'items':{x['id']:x['count'] for x in run['player']['items']},'recent':run['player']['recentItems']}

SHOP_TAIL=["shop:open:Malet:sell","request:audio:ui_open","request:achievement:check_grains","request:tutorial:first_shop","development:deferred:before:shop_actions"]

def validate(a,shop_frontier=False):
    d=a.automation_dir.resolve();cap=d/('Phase1O' if shop_frontier else 'Phase1N');out=a.evidence_dir.resolve();out.mkdir(parents=True,exist_ok=False)
    checks=[];report={'status':'RUNNING','checks':checks,'scope':'Phase1O shop frontier with all retained N checks' if shop_frontier else 'Phase1N original pre-shop frontier'}
    def check(n,b):
        checks.append({'name':n,'passed':bool(b)})
        if not b:raise ValueError(n)
    try:
        index=read(d/'automation_index.json');check('Exact current identities PASS',inspect_automation_report(index,current_test_paths() if shop_frontier else current_test_paths()-shop_test_paths())['passed'])
        prior=read(ROOT/'docs/unreal-migration/evidence/phase1m/automation01/automation_index.json');old_ids={t['fullTestPath'] for t in prior['tests']};new_ids={t['fullTestPath'] for t in index['tests']};check('Exact accepted Phase1M169 identities retained',len(old_ids)==169 and old_ids.issubset(new_ids) and len(new_ids-old_ids)==(34 if shop_frontier else 22))
        gold=read(ROOT/'docs/unreal-migration/fixtures/malet_world_seed/contract_expected.v1.json');fresh=gold[0]
        ig=read(ROOT/'docs/unreal-migration/fixtures/malet_reward/contract_expected.v1.json');hg=read(ROOT/'docs/unreal-migration/fixtures/malet_deal/contract_expected.v1.json')
        modes=['FirebombCanonical','AntidoteCanonical','PotionCanonical','WorldSeedCanonical','FirstEffectCanonical','FirstEffectPreexistingTrue','FirstEffectPreexistingFalse','CanonicalReward','CanonicalPayment','AlreadyBurnedPayment','AcceptPreEffectDeferred']
        for mode in modes:
            def state(label):return read(cap/(mode+'_'+label+'.json'))
            before=state('before_reward');stop=state('shop_deferred');expected=flag_delta(before['run']);hcase='already_burned' if mode=='AlreadyBurnedPayment' else 'accept_intact'
            for label in ('before_accept','after_accept','deal_first','deal_end'):
                x=state(label);g=source_state(hg,hcase,label);start=max(i for i,e in enumerate(x['trace']) if e=='interact:Malet');check(mode+' exact H '+label,normalize(x['trace'][start:])==g['events'])
            for label,source_label,done in [('before_reward','reward_first',False),('reward_middle','reward_row_3',False),('reward_last','reward_row_7',False),('reward_completion','completion_before_callback',True),('shop_deferred','completion_before_callback',True)]:
                x=state(label);g=source_state(ig,'reward_en',source_label)['events'].copy()
                if done:g+=SUFFIX+(SHOP_TAIL if shop_frontier else [])
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
            check(mode+' exact shop entry and zero banned downstream calls',stop['observed_downstream_effect_events']['shop:']==int(shop_frontier) and all(stop['observed_downstream_effect_events'][p]==0 for p in ('chapter:','autosave:','achievement:')))
            check(mode+' source timer durations',299000<=stop['actual_delay_microseconds']<350000 and 499000<=stop['actual_reward_delay_microseconds']<550000)
            check(mode+' one completion/no stale timers',stop['reward_completions']==stop['reward_callback_intents']==1 and stop['field_invocations']==4 and not any(stop[k] for k in ('normal_delay_pending','reward_delay_pending','reward_callback_pending')))
            check(mode+' prepotion stop stable',stop['deferred_target']==('before:shop_actions' if shop_frontier else 'before:shop_open') and stop['trace']==state('reward_completion')['trace'] and stop['pawn_position']==before['pawn_position'])
            check(mode+' paid food sword history',stop['memory']['burnedHistory']==['daily_market_food','identity_first_sword'])
            if mode=='FirebombCanonical':
                report['canonical']=stop;(out/'canonical_full_trace.txt').write_text('\n'.join(stop['trace'])+'\n',encoding='utf-8')
                for label in ('before_world_seed','after_knowledge','after_memory','world_seed_complete','before_potion','after_inventory_mutation','after_recent_items','after_inventory_changed','potion_complete','synchronous_antidote_contract_complete','synchronous_before_firebomb','actual_signal_firebomb','synchronous_firebomb_firebomb_complete','synchronous_firebomb_contract_complete','shop_deferred'):(out/(label+'.json')).write_bytes(canonical(state(label)))
        potion_gold=read(ROOT/'docs/unreal-migration/fixtures/malet_potion/contract_expected.v1.json')
        for mode in modes:
            def state(label):return read(cap/(mode+'_'+label+'.json'))
            before=state('before_potion');check(mode+' real Phase1K state before potion',before['world_memory_snapshot']==fresh['after'] and inventory(before['run'])==potion_gold[0]['before'])
            for label,step in zip(('after_inventory_mutation','after_recent_items','after_inventory_changed','potion_complete'),potion_gold[0]['steps']):
                x=state(label);check(mode+' exact source potion snapshot '+label,inventory(x['run'])==step['state']);check(mode+' all memory unchanged '+label,all(x[k]==before[k] for k in ('memory','player_observables','world_memory_snapshot')))
                check(mode+' snapshot is synchronous trace prefix '+label,state('shop_deferred')['trace'][:len(x['trace'])]==x['trace'])
            stop=state('shop_deferred');trace=stop['trace'];check(mode+' one signal and toast',trace.count('inventory_changed:potion')==trace.count('toast:+2 Potion:1')==1)
            check(mode+' exact shop count and no transition',sum(e.startswith('shop:') for e in trace)==int(shop_frontier) and not any(any(e.startswith(p) for p in ('chapter:','autosave:','achievement:','chapter_transition:')) for e in trace))
        antidote_gold=read(ROOT/'docs/unreal-migration/fixtures/malet_antidote/contract_expected.v1.json')
        canon=next(c for c in antidote_gold if c['id']=='canonical')
        for mode in modes:
            def state(label):return read(cap/(mode+'_'+label+'.json'))
            b=state('synchronous_potion_contract_complete');stop=state('synchronous_antidote_contract_complete')
            check(mode+' preserved potion complete source prefix',b['trace']==stop['trace'][:len(b['trace'])] and b['trace'][-1]=='item:add:end:potion:2' and inventory(b['run'])==canon['before'])
            check(mode+' original L source prefix independently retained',normalize(b['trace'][max(i for i,e in enumerate(b['trace']) if e=='interact:Malet'):])==normalize(stop['trace'][max(i for i,e in enumerate(stop['trace']) if e=='interact:Malet'):-6]))
            for label,step in zip(('synchronous_antidote_after_inventory_mutation','synchronous_antidote_after_recent_items','actual_signal_antidote','synchronous_antidote_antidote_complete'),canon['steps']):
                x=state(label);check(mode+' source antidote state '+label,inventory(x['run'])==step['state'])
                check(mode+' antidote preserves player world identity '+label,all(x[k]==b[k] for k in ('memory','player_observables','world_memory_snapshot')) and all(x[k] for k in ('same_run_id','same_memory_domain','same_world_domain')))
            check(mode+' exactly two ordered toast requests',[e for e in stop['trace'] if e.startswith('toast:')]==['toast:'+q['text']+':'+str(q['type']) for q in next(c for c in antidote_gold if c['id']=='sequence')['enqueues']])
            check(mode+' exactly two item-only signals',[e for e in stop['trace'] if e.startswith('inventory_changed:')]==['inventory_changed:potion','inventory_changed:antidote'])
        firebomb_gold=read(ROOT/'docs/unreal-migration/fixtures/malet_firebomb/contract_expected.v1.json')
        firecanon=next(c for c in firebomb_gold if c['id']=='canonical')
        sequence=next(c for c in firebomb_gold if c['id']=='sequence')
        for mode in modes:
            def state(label):return read(cap/(mode+'_'+label+'.json'))
            b=state('synchronous_antidote_contract_complete');stop=state('shop_deferred')
            check(mode+' exact M full source state preserved',inventory(b['run'])==firecanon['before'])
            check(mode+' M exact prefix ends after antidote',b['trace']==stop['trace'][:len(b['trace'])] and b['trace'][-1]=='item:add:end:antidote:1')
            before_firebomb=state('synchronous_before_firebomb')
            check(mode+' firebomb begins at same completed M authority',all(before_firebomb[k]==b[k] for k in ('run','memory','player_observables','world_memory_snapshot','trace')))
            for label,step in zip(('synchronous_firebomb_after_inventory_mutation','synchronous_firebomb_after_recent_items','actual_signal_firebomb','synchronous_firebomb_firebomb_complete'),firecanon['steps']):
                x=state(label);check(mode+' exact source firebomb state '+label,inventory(x['run'])==step['state'])
                check(mode+' firebomb preserves entire domains and identities '+label,all(x[k]==b[k] for k in ('memory','player_observables','world_memory_snapshot')) and all(x[k] for k in ('same_run_id','same_memory_domain','same_world_domain')))
            check(mode+' three ordered source toast requests',[e for e in stop['trace'] if e.startswith('toast:')]==['toast:'+q['text']+':'+str(q['type']) for q in sequence['enqueues']])
            check(mode+' three item-only signals',[e for e in stop['trace'] if e.startswith('inventory_changed:')]==['inventory_changed:potion','inventory_changed:antidote','inventory_changed:firebomb'])
            check(mode+' actual signal payload has only source item ID',state('actual_signal_firebomb')['actual_signal_payload']=={'item_id':'firebomb'})
            check(mode+' actual signal precedes its toast','toast:+1 Firebomb:1' not in state('actual_signal_firebomb')['trace'])
        for c in firebomb_gold:
            x=read(cap/('firebomb_source_'+c['id']+'.json'));steps=c['steps'];replaced=c['id'].startswith('replacement_')
            if replaced:steps=steps[:next(i for i,v in enumerate(steps) if v['point']=='replacement')+1]
            check('firebomb source events '+c['id'],x['steps']==steps)
            check('firebomb source full inventory delta '+c['id'],x['before']['inventory']==c['before'] and x['after']['inventory']==(steps[-1]['state'] if replaced else c['after']))
            for section in ('player','player_observables','world'):check('firebomb entire domain preserved '+c['id']+section,x['before'][section]==x['after'][section])
            sv=read(cap/('firebomb_save_'+c['id']+'.json'));check('firebomb independent binary '+c['id'],sv['before']==sv['after'] and sv['schema']==1 and sv['binary_bytes']>0 and sv['restore_signals']==sv['restore_toasts']==sv['restore_grants']==0)
            if not replaced:check('firebomb source raw/query import and restore '+c['id'],sv['restored_recent_query']==c['restored_query'] and c['restored']==c['restored_after_query']==c['after'])
        presentation=read(cap/'firebomb_presentation.json');check('Request generation separate from removed presentation delivery',presentation['generated_requests']==['+2 Potion','+1 Antidote','+1 Firebomb','+1 Firebomb'] and presentation['presentation_deliveries']==0)
        b=read(cap/'FirebombSignalFirebombOnRunReplace_lifetime_before_cancel.json');x=read(cap/'FirebombSignalFirebombOnRunReplace_cancelled_callback.json')
        check('Firebomb actual listener preserves old commit',inventory(b['run'])==firecanon['after'] and 'toast:+1 Firebomb:1' not in b['trace'])
        check('Firebomb actual listener replacement isolates new run',inventory(x['run'])=={'items':{},'recent':[]} and x['trace']==[] and x['run']['runId']!=b['run']['runId'])
        b=read(cap/'FirebombAfterStopOnWorldTeardown_reward_completion.json');x=read(cap/'FirebombAfterStopOnWorldTeardown_world_callback_cancelled.json')
        check('Firebomb completed world teardown preserves all state',all(x[k]==b[k] for k in ('run','memory','player_observables','world_memory_snapshot','trace')))
        x=read(cap/'FirebombAfterStopOnRunReplace_cancelled_callback.json');check('Firebomb stop replacement isolates new state',inventory(x['run'])=={'items':{},'recent':[]} and x['trace']==[] and x['world_memory_snapshot']==fresh['before'])
        ant_save=read(cap/'antidote_canonical_save.json');check('Original M binary at exact antidote contract seam',ant_save['before']==ant_save['after'] and ant_save['before']['inventory']==firecanon['before'] and ant_save['restore_signals']==ant_save['restore_toasts']==ant_save['restore_grants']==0)
        for c in antidote_gold:
            x=read(cap/('antidote_source_'+c['id']+'.json'));steps=c['steps'];replaced=c['id'].startswith('replacement_')
            if replaced:steps=steps[:next(i for i,v in enumerate(steps) if v['point']=='replacement')+1]
            check('antidote source events '+c['id'],x['steps']==steps)
            check('antidote source state '+c['id'],x['before']['inventory']==c['before'] and x['after']['inventory']==(steps[-1]['state'] if replaced else c['after']))
            check('source raw query immutable '+c['id'],c['before']==c['before_after_query'] and c['restored']==c['restored_after_query']==c['after'])
            for section in ('player','player_observables','world'):check('antidote preserve '+c['id']+section,x['before'][section]==x['after'][section])
            sv=read(cap/('antidote_save_'+c['id']+'.json'));check('antidote binary source '+c['id'],sv['before']==sv['after'] and sv['schema']==1 and sv['binary_bytes']>0 and sv['restore_signals']==sv['restore_toasts']==sv['restore_grants']==0)
            if not replaced:check('source restored query '+c['id'],sv['restored_recent_query']==c['restored_query'])
        for item in ('Potion','Antidote'):
            mode='AntidoteSignal'+item+'OnRunReplace';b=read(cap/(mode+'_lifetime_before_cancel.json'));x=read(cap/(mode+'_cancelled_callback.json'))
            check(mode+' actual signal committed old state',inventory(b['run'])==({'items':{'potion':2},'recent':['potion']} if item=='Potion' else canon['after']))
            check(mode+' old toast absent at signal',('toast:+1 Antidote:1' not in b['trace']) and (item!='Potion' or 'toast:+2 Potion:1' not in b['trace']))
            check(mode+' new run isolated',inventory(x['run'])=={'items':{},'recent':[]} and x['trace']==[] and x['run']['runId']!=b['run']['runId'])
        b=read(cap/'AntidoteAfterStopOnWorldTeardown_reward_completion.json');x=read(cap/'AntidoteAfterStopOnWorldTeardown_world_callback_cancelled.json')
        check('Antidote teardown preserves all committed domains',all(x[k]==b[k] for k in ('run','memory','player_observables','world_memory_snapshot','trace')))
        x=read(cap/'AntidoteAfterStopOnRunReplace_cancelled_callback.json');check('Antidote stop replacement no leakage',inventory(x['run'])=={'items':{},'recent':[]} and x['trace']==[])
        ps=read(cap/'potion_canonical_save.json');check('Original potion-complete binary contract retained',ps['before']==ps['after'] and ps['before']['inventory']==canon['before'])
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
        if shop_frontier:
            old=read(ROOT/'docs/unreal-migration/evidence/phase1n/automation03/automation_index.json')
            check('All 191 prior test identities retained plus exactly 12 shop tests',len(old['tests'])==191 and {t['fullTestPath'] for t in old['tests']}.issubset(new_ids) and len(new_ids)==203)
            for c in read(ROOT/'docs/unreal-migration/fixtures/malet_shop/contract_expected.v1.json'):
                x=read(cap/('shop_source_'+c['id']+'.json'))
                check('Shop source exact projection '+c['id'],set(x['first'])=={'open','merchant','mode','title','caption','grains_text','detail_title','portrait','rows','stock','events'} and all(v==c['first'][k] for k,v in x['first'].items()))
                check('Shop full state and repeat-open preservation '+c['id'],x['before_state']==x['after_state'] and x['first']==x['after'])
                sv=read(cap/('shop_save_'+c['id']+'.json'))
                check('Shop binary preservation '+c['id'],sv['before']==sv['after'] and sv['schema']==1 and sv['restore_signals']==sv['restore_toasts']==sv['restore_grants']==0)
            expected_shop=read(ROOT/'docs/unreal-migration/fixtures/malet_shop/contract_expected.v1.json')[0]['first']
            for mode in modes+['ShopCanonical']:
                stop=read(cap/(mode+'_shop_deferred.json'));seam=read(cap/(mode+'_synchronous_firebomb_contract_complete.json'))
                check(mode+' Phase1N synchronous state unchanged through shop',all(seam[k]==stop[k] for k in ('run','memory','player_observables','world_memory_snapshot')))
                check(mode+' N full ordered prefix then exact O suffix',stop['trace']==seam['trace']+['development:deferred:before:shop_open']+SHOP_TAIL)
                check(mode+' no shop before N item-complete seam',seam['shop_snapshot'] is None)
                check(mode+' entire canonical shop projection',set(stop['shop_snapshot'])=={'open','merchant','mode','title','caption','grains_text','detail_title','portrait','rows','stock','events'} and all(v==expected_shop[k] for k,v in stop['shop_snapshot'].items()))
            for owner in ['Firebomb','Antidote','Potion']:
                before=read(cap/(owner+'AfterStopOnWorldTeardown_reward_completion.json'));after=read(cap/(owner+'AfterStopOnWorldTeardown_world_callback_cancelled.json'))
                check(owner+' real map travel discards shop without source close',before['shop_snapshot']['open'] and after['shop_snapshot'] is None and all(before[k]==after[k] for k in ['run','memory','player_observables','world_memory_snapshot','trace']))
                after=read(cap/(owner+'AfterStopOnRunReplace_cancelled_callback.json'))
                check(owner+' replacement has no leaked shop',after['shop_snapshot'] is None)
            sv=read(cap/'shop_canonical_save.json');check('Shop canonical binary independent exact preservation',sv['before']==sv['after'] and sv['restore_signals']==sv['restore_toasts']==sv['restore_grants']==0)
            check('Shop owner lifecycle evidence',read(cap/'shop_lifetime.json')['cleanup_no_close_effects'])
        report['snapshot_count']=len(list(cap.glob('*.json')));report['captures']=[]
        for name in ('WorldSeed_Complete','Potion_Before','Potion_Granted','Potion_Toast','Antidote_Before','Antidote_Granted','Antidote_Signal','Antidote_Toast','Firebomb_Before','Firebomb_Granted','Reward_Toasts','Shop_Deferred'):
            p=cap/('FirebombCanonical_'+name+'.png');raw=p.read_bytes();size=struct.unpack('>II',raw[16:24]);check('Native capture '+name,raw[:8]==b'\x89PNG\r\n\x1a\n' and size[0]>=1280 and size[1]>=720);report['captures'].append(dict(path=str(p.relative_to(ROOT)),sha256=hashlib.sha256(raw).hexdigest(),dimensions=size))
        report.update(status='PASS',test_count=len(index['tests']),warnings=sum(t['warnings'] for t in index['tests']))
    except Exception as e:report.update(status='FAIL',error=str(e))
    (out/'acceptance.json').write_bytes(canonical(report));print(('MEMORIA_SHOP_EVIDENCE_' if shop_frontier else 'MEMORIA_FIREBOMB_EVIDENCE_')+report['status']+' '+report.get('error',''));return int(report['status']!='PASS')
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--automation-dir',type=Path,required=True);p.add_argument('--evidence-dir',type=Path,required=True);raise SystemExit(validate(p.parse_args()))
