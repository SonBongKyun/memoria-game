"""Exact Phase1I source/import/identity and pre-effect static boundaries."""
import json,unittest,re
from narrative_ir import ROOT,extract,validate,canonical,ir_path,FIELD_CASES,parse_source
from malet_reward_test_fixtures import fixtures
from export_malet_reward_oracle import bodies
from export_malet_refusal_oracle import MARK
from validate_unreal import current_test_paths,malet_reward_test_paths,malet_deal_test_paths,expected_test_paths,narrative_test_paths,slice_test_paths,malet_test_paths,malet_refusal_test_paths,inspect_automation_report
class RewardTools(unittest.TestCase):
    def test_exact_only_reward_added(self):
        self.assertEqual(set(FIELD_CASES),{'verdan_arrival','malet_taste_burned','malet_encounter','malet_refused','malet_deal','malet_reward'})
        v=extract('field',group='malet_reward');self.assertEqual(canonical(v),ir_path('field','malet_reward').read_bytes())
        original=parse_source((ROOT/'data/chapter2_dialogue.json').read_bytes())['dialogues']['malet_reward']
        self.assertEqual(len(original),8)
        for i,(r,o) in enumerate(zip(v['definition']['rows'],original)):
            self.assertEqual(r['original_index'],i);self.assertEqual(r['provenance']['group_position'],3)
            self.assertEqual(r['effects'],{});self.assertEqual(r['gate'],{});self.assertEqual(r['choices'],[]);self.assertFalse(r['choices_present'])
            for k in ('speaker','text','text_ko'):self.assertEqual(r['text'][k],o[k])
            self.assertEqual(r['presentation'],{k:o[k] for k in ('cg','portrait','side') if k in o})
        for g in ('malet_memory_world_followup','malet_shop','chapter3'):
            with self.assertRaises(ValueError):extract('field',group=g)
    def test_semantic_change_and_strict_provenance(self):
        for name,v in fixtures().items():
            if name.startswith('modified'):
                validate(v,verify_sources=False)
                with self.assertRaises(ValueError):validate(v)
            else:
                with self.assertRaises(ValueError):validate(v,verify_sources=False)
    def test_verbatim_callbacks_and_no_new_completion_timer(self):
        original,observed,_=bodies()
        stripped='\n'.join(x for x in observed.splitlines() if not x.endswith(MARK))+'\n'
        expected=''.join(original[n] for n in ('_on_any_dialogue_ended','_on_deal_ended','_on_refused_ended','_on_reward_ended','_seed_malet_memory_world_state_if_needed','_open_malet_shop','_on_shop_closed'))
        self.assertEqual(stripped,expected)
        reward=original['_on_reward_ended'];self.assertEqual(reward.splitlines()[1].strip(),'GameManager.set_flag("ch2_malet_done")');self.assertNotIn('await',reward)
        for effect in ('_seed_malet_memory_world_state_if_needed()','GameManager.add_item("potion", 2)','GameManager.add_item("antidote", 1)','GameManager.add_item("firebomb", 1)','_open_malet_shop()'):self.assertIn(effect,reward)
    def test_source_complete_effect_order_and_lifetime(self):
        cases=json.loads((ROOT/'docs/unreal-migration/fixtures/malet_reward/contract_expected.v1.json').read_text(encoding='utf-8'))
        by={c['id']:{s['label']:s for s in c['states']} for c in cases};self.assertEqual(len(by),8)
        for locale in ('reward_en','reward_ko'):
            before=by[locale]['completion_before_callback'];after=by[locale]['after_reward_callback']
            self.assertNotIn('ch2_malet_done',before['flags']);self.assertEqual(before['items'],{});self.assertFalse(before['active']);self.assertEqual(before['game_state'],0)
            tail=after['events'][len(before['events']):]
            self.assertEqual(tail,['callback:reward:enter','flag:ch2_malet_done','world:seed:enter','world:knowledge.learned:fact.bl07.route_request_received','world:memory.added:memory.malet.bl07_request_source','item:potion:2','item:antidote:1','item:firebomb:1','shop:request','shop:open','tutorial:first_shop','callback:shop:connect'])
            self.assertEqual(after['items'],{'potion':2,'antidote':1,'firebomb':1});self.assertTrue(after['shop_open']);self.assertFalse(after['reward_callback_connected'])
            self.assertEqual(before['memory_state'],after['memory_state']);self.assertEqual(after['chapter'],1)
        for name in ('replace_during_reward','replace_at_completion'):
            self.assertTrue(by[name]['after_reward_callback']['flags']['ch2_malet_done'])
        for name in ('teardown_during_reward','teardown_at_completion'):
            after=by[name]['after_reward_callback'];self.assertNotIn('ch2_malet_done',after['flags']);self.assertFalse(after['shop_open']);self.assertEqual(after['items'],{})
        self.assertEqual(by['preseeded_world']['reward_first']['world'],by['preseeded_world']['after_reward_callback']['world'])
        end=by['full_shop_continuation']['chapter_request'];self.assertEqual(end['chapter'],3)
        self.assertEqual(end['events'][-9:],['shop:close','callback:shop:enter','flag:ch2_complete','chapter:3','autosave:chapter_transition','achievement:chapter:2','achievement:unlock:merchant','delay:elapsed:1500','chapter_transition:2:res://scenes/maps/belt_waystation.tscn'])
    def test_exact_90_retained_plus_7(self):
        prior=expected_test_paths()|narrative_test_paths()|slice_test_paths()|malet_test_paths()|malet_refusal_test_paths()|malet_deal_test_paths()
        self.assertEqual(len(prior),90);self.assertEqual(len(malet_reward_test_paths()),7);self.assertFalse(prior&malet_reward_test_paths())
        self.assertEqual(current_test_paths(),prior|malet_reward_test_paths())
        report={'tests':[dict(fullTestPath=n,state='Success') for n in sorted(current_test_paths())]}
        self.assertTrue(inspect_automation_report(report,current_test_paths())['passed']);report['tests'].pop();self.assertFalse(inspect_automation_report(report,current_test_paths())['passed'])
    def test_pre_effect_runtime_has_no_downstream_calls(self):
        source=(ROOT/'Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaNarrativeSubsystem.cpp').read_text(encoding='utf-8')
        body=source[source.index('void UMemoriaNarrativeSubsystem::DeferRewardEffects()'):]
        for forbidden in ('SetStoryFlag(', 'BurnMemory(', 'Rewards(', 'OpenLevel(', 'SetTimer(', 'AddItem(', 'SaveGameToSlot(', 'LearnFact(', 'AddMemory('):self.assertNotIn(forbidden,body)
        self.assertIn('EMemoriaSliceState::Deferred',body)
        self.assertIn('RewardCallbackRunId != Run->GetRunSnapshot().RunId',body)
        self.assertNotIn('malet_memory_world_followup',source)
if __name__=='__main__':unittest.main()
