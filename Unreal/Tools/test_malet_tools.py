import unittest,copy
from narrative_ir import extract,validate,canonical,ir_path
from malet_test_fixtures import fixtures
from export_malet_oracle import npc_methods,npc_configuration
from validate_unreal import expected_test_paths,narrative_test_paths,slice_test_paths,malet_test_paths,current_test_paths,inspect_automation_report

class MaletToolTests(unittest.TestCase):
    def test_prior_ir_stays_exact(self):
        for dialect in ('vn','field'):
            self.assertEqual(canonical(extract(dialect)),ir_path(dialect).read_bytes())
    def test_only_selected_authored_group(self):
        v=extract('field',group='malet_taste_burned')
        self.assertEqual(v['definition']['id'],'malet_taste_burned')
        self.assertEqual([x['original_index'] for x in v['definition']['rows']],[0,1,2])
        self.assertTrue(all(x['provenance']['group_position']==16 for x in v['definition']['rows']))
        self.assertTrue(all('text_ko' in x['text'] for x in v['definition']['rows']))
    def test_negative_cohort_boundaries(self):
        for name,value in fixtures().items():
            if name.startswith('reject'):
                with self.subTest(name=name),self.assertRaises(ValueError): validate(value,verify_sources=False)
    def test_semantic_change_cannot_be_promoted(self):
        v=fixtures()['modified_semantic.field.v1.json']
        validate(v,verify_sources=False)
        with self.assertRaises(ValueError): validate(v)
        self.assertNotEqual(v['semantic_sha256'],extract('field',group='malet_taste_burned')['semantic_sha256'])
    def test_downstream_content_not_authorized(self):
        with self.assertRaises(ValueError): extract('field',group='malet_reward')
    def test_npc_observer_keeps_source_priority(self):
        body=npc_methods()
        self.assertLess(body.index('if DialogueManager.is_active'),body.index('PerceptionFilter.take_burn_reaction'))
        self.assertLess(body.index('PerceptionFilter.take_burn_reaction'),body.index('var talk_flag'))
        self.assertIn('return\n\n\tif dialogue_key',body)
        self.assertEqual(npc_configuration()['repeat_dialogue_key'],'malet_memory_world_followup')
    def test_previous_72_and_exact_phase1f_names(self):
        previous=expected_test_paths()|narrative_test_paths()|slice_test_paths()
        self.assertEqual(len(previous),72)
        self.assertEqual(len(malet_test_paths()),4)
        self.assertFalse(previous & malet_test_paths())
        self.assertTrue((previous|malet_test_paths()) <= current_test_paths())
        report={'tests':[dict(fullTestPath=x,state='Success') for x in previous]}
        self.assertFalse(inspect_automation_report(report,current_test_paths())['passed'])
if __name__=='__main__': unittest.main()
