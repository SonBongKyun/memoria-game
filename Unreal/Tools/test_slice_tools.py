import unittest
from validate_unreal import current_test_paths, expected_test_paths, narrative_test_paths, slice_test_paths, inspect_automation_report
from export_slice_oracle import route_source

class SliceToolTests(unittest.TestCase):
    def test_previous_68_and_exact_addition(self):
        old=expected_test_paths() | narrative_test_paths()
        self.assertEqual(len(old),68)
        self.assertEqual(len(slice_test_paths()),4)
        self.assertFalse(old & slice_test_paths())
        self.assertEqual(current_test_paths(),old | slice_test_paths())

    def test_68_green_cannot_mask_missing_slice(self):
        old=expected_test_paths() | narrative_test_paths()
        report={'tests':[dict(fullTestPath=x,state='Success') for x in old]}
        self.assertFalse(inspect_automation_report(report,current_test_paths())['passed'])

    def test_source_route_extraction_keeps_both_guards_and_methods(self):
        guard, methods=route_source()
        self.assertIn('if not GameManager.get_flag("ch2_arrived"):',guard)
        self.assertIn('if GameManager.get_flag("ch2_arrival_vn_seen"):',guard)
        self.assertIn('_start_ch2_free_exploration_after_vn()',guard)
        self.assertIn('_start_ch2_sequence()',guard)
        self.assertIn('DialogueManager.load_and_start(DIALOGUE_FILE, "verdan_arrival")',methods)
        self.assertEqual(methods.count('func '),3)

if __name__=='__main__': unittest.main()
