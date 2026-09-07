import unittest
from narrative_ir import extract,validate,canonical,ir_path,FIELD_CASES
from malet_refusal_test_fixtures import fixtures
from export_malet_refusal_oracle import bodies,MARK
from validate_unreal import current_test_paths,expected_test_paths,narrative_test_paths,slice_test_paths,malet_test_paths,malet_refusal_test_paths,inspect_automation_report
class RefusalTools(unittest.TestCase):
    def test_two_exact_groups(self):
        for group,count,pos in [('malet_encounter',10,1),('malet_refused',3,4)]:
            v=extract('field',group=group);self.assertEqual(canonical(v),ir_path('field',group).read_bytes())
            self.assertEqual(len(v['definition']['rows']),count)
            self.assertTrue(all(r['provenance']['group_position']==pos for r in v['definition']['rows']))
        choices=extract('field',group='malet_encounter')['definition']['rows'][9]['choices']
        self.assertEqual([c['original_index'] for c in choices],[0,1])
        self.assertEqual(choices[0]['effects'],{'set_flag':'malet_deal_accepted','burn_memory':'identity_first_sword'})
        self.assertEqual(choices[1]['effects'],{'set_flag':'malet_deal_refused'})
    def test_transient_semantics_and_rejections(self):
        for name,v in fixtures().items():
            if '.modified.' in name:
                validate(v,verify_sources=False)
                with self.assertRaises(ValueError):validate(v)
            else:
                with self.assertRaises(ValueError):validate(v,verify_sources=False)
    def test_downstream_groups_rejected(self):
        self.assertEqual(set(FIELD_CASES),{'verdan_arrival','malet_taste_burned','malet_encounter','malet_refused'})
        for g in ('malet_deal','malet_reward','malet_memory_world_followup'):
            with self.assertRaises(ValueError):extract('field',group=g)
    def test_real_source_callback_observers(self):
        originals,normal,cleanup,npc=bodies()
        self.assertEqual('\n'.join(x for x in normal.splitlines() if not x.endswith(MARK))+'\n',originals['_on_any_dialogue_ended'])
        self.assertEqual('\n'.join(x for x in cleanup.splitlines() if not x.endswith(MARK))+'\n',originals['_on_refused_ended'])
        self.assertIn('await get_tree().create_timer(0.3).timeout',normal)
    def test_exact_prior_76_and_new_6(self):
        prior=expected_test_paths()|narrative_test_paths()|slice_test_paths()|malet_test_paths()
        self.assertEqual(len(prior),76);self.assertEqual(len(malet_refusal_test_paths()),6)
        self.assertEqual(current_test_paths(),prior|malet_refusal_test_paths())
        valid={'tests':[dict(fullTestPath=n,state='Success') for n in sorted(current_test_paths())]}
        self.assertTrue(inspect_automation_report(valid,current_test_paths())['passed'])
        valid['tests'][-1]['fullTestPath']=valid['tests'][0]['fullTestPath']
        self.assertFalse(inspect_automation_report(valid,current_test_paths())['passed'])
if __name__=='__main__':unittest.main()
