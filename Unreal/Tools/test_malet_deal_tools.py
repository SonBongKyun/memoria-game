import json,unittest
from narrative_ir import ROOT,extract,validate,canonical,ir_path,FIELD_CASES
from malet_deal_test_fixtures import fixtures
from export_malet_deal_oracle import bodies
from export_malet_refusal_oracle import MARK
from validate_unreal import current_test_paths,malet_deal_test_paths,expected_test_paths,narrative_test_paths,slice_test_paths,malet_test_paths,malet_refusal_test_paths,inspect_automation_report
class DealTools(unittest.TestCase):
    def test_single_group_and_retained_contracts(self):
        self.assertEqual(set(FIELD_CASES),{'verdan_arrival','malet_taste_burned','malet_encounter','malet_refused','malet_deal','malet_reward','verdan_market_walk','verdan_old_burner','malet_backstory','elia_sump_concern','sump_atmosphere'})
        for g in FIELD_CASES:self.assertEqual(canonical(extract('field',group=g)),ir_path('field',g).read_bytes())
        v=extract('field',group='malet_deal');self.assertEqual(len(v['definition']['rows']),5)
        for i,r in enumerate(v['definition']['rows']):
            self.assertEqual(r['original_index'],i);self.assertEqual(r['provenance']['group_position'],2)
            self.assertIn('text_ko',r['text'])
        for g in ('malet_memory_world_followup',):
            with self.assertRaises(ValueError):extract('field',group=g)
    def test_semantic_probes(self):
        for name,v in fixtures().items():
            if name.startswith('modified'):
                validate(v,verify_sources=False)
                with self.assertRaises(ValueError):validate(v)
            else:
                with self.assertRaises(ValueError):validate(v,verify_sources=False)
    def test_actual_source_timers(self):
        original,normal,deal,_,_=bodies()
        for body,key,delay in [(normal,'_on_any_dialogue_ended','0.3'),(deal,'_on_deal_ended','0.5')]:
            self.assertEqual('\n'.join(x for x in body.splitlines() if not x.endswith(MARK))+'\n',original[key])
            self.assertIn('await get_tree().create_timer('+delay+').timeout',body)
    def test_source_failure_and_lifetime_characterization(self):
        cases=json.loads((ROOT/'docs/unreal-migration/fixtures/malet_deal/contract_expected.v1.json').read_text(encoding='utf-8'))
        last={c['id']:c['states'][-1] for c in cases};self.assertEqual(len(last),9)
        for name in ('already_burned','faded_sword'):
            self.assertIn('burn:identity_first_sword:fail',last[name]['events'])
            self.assertEqual(last[name]['requested'],['malet_encounter','malet_deal','malet_reward'])
        self.assertEqual(last['already_burned']['burned'].count('identity_first_sword'),1)
        self.assertEqual(last['replace300_keep_world']['requested'][-1],'malet_refused')
        self.assertEqual(last['replace500_keep_world']['requested'][-1],'malet_reward')
        self.assertEqual(last['teardown300']['requested'],['malet_encounter'])
        self.assertEqual(last['teardown500']['requested'],['malet_encounter','malet_deal'])
    def test_exact_82_plus_8(self):
        prior=expected_test_paths()|narrative_test_paths()|slice_test_paths()|malet_test_paths()|malet_refusal_test_paths()
        self.assertEqual(len(prior),82);self.assertEqual(len(malet_deal_test_paths()),8);self.assertFalse(prior&malet_deal_test_paths())
        self.assertTrue((prior|malet_deal_test_paths()) <= current_test_paths())
        report={'tests':[dict(fullTestPath=n,state='Success') for n in sorted(current_test_paths())]}
        self.assertTrue(inspect_automation_report(report,current_test_paths())['passed'])
        report['tests'].pop();self.assertFalse(inspect_automation_report(report,current_test_paths())['passed'])
if __name__=='__main__':unittest.main()
