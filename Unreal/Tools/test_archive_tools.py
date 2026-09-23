"""Executed archive contract invariants, independent of native widget layout."""
import json,unittest
import export_archive_oracle as source

class ArchiveContract(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data=json.loads((source.DEST/'contract_expected.v1.json').read_text(encoding='utf-8'))
        cls.cases={c['id']:c for c in cls.data}

    def test_owned_order_filters_and_grade_direction(self):
        initial=self.cases['initial_en']
        self.assertEqual({r['grade'] for r in initial['rows']},set(range(5)))
        self.assertEqual(initial['filters'][1],'Grade 5, Sensory')
        self.assertEqual(initial['filters'][5],'Grade 1, Core')
        for case,grade in [('grade5',0),('grade3',2),('grade1',4)]:
            expected=[r['id'] for r in self.cases['states_en']['rows'] if r['grade']==grade]
            self.assertEqual([r['id'] for r in self.cases[case]['rows']],expected)
        self.assertEqual(self.cases['empty']['rows'],[])
        self.assertEqual(self.cases['empty']['counts'][:3],[0,0,0])

    def test_readonly_selection_and_source_state_priority(self):
        for case in self.data:
            self.assertTrue(case['memory_unchanged'])
            self.assertTrue(case['read_only'])
            self.assertTrue(case['mutation_controls_hidden'])
        states={r['id']:r for r in self.cases['states_en']['rows']}
        self.assertEqual(states['sense_forest_smell']['state_label'],'BURNED')
        self.assertEqual(states['rel_hand_reaching']['state_label'],'RESIDUE')
        self.assertEqual(states['daily_campfire_song']['state_label'],'FADED')
        self.assertTrue(states['identity_first_sword']['state_label'].startswith('ERODING '))
        # Retain the source discrepancy as evidence; native uses card state.
        self.assertEqual(states['daily_campfire_song']['detail_state'],'INTACT')
        self.assertEqual(self.cases['states_en']['counts'][2],2)

    def test_localization_and_counts_do_not_depend_on_filter(self):
        self.assertNotEqual(self.cases['initial_en']['title'],self.cases['initial_ko']['title'])
        self.assertNotEqual(self.cases['initial_en']['rows'][0]['title'],self.cases['initial_ko']['rows'][0]['title'])
        expected=self.cases['states_en']['counts']
        for case in ['states_ko','grade5','grade3','grade1']:
            self.assertEqual(self.cases[case]['counts'],expected)
        self.assertEqual(expected[0],len(self.cases['states_en']['rows']))

    def test_generated_native_labels_match_execution(self):
        text=source.TARGET.read_text(encoding='utf-8')
        start=text.index('// BEGIN EXECUTED ARCHIVE TEXT')+len('// BEGIN EXECUTED ARCHIVE TEXT')
        end=text.index('// END EXECUTED ARCHIVE TEXT')
        self.assertEqual(text[start:end],chr(10)+source.source_constants(self.data))
        # Static functions are extracted as exact methods, never duplicated constants.
        self.assertTrue(source.archive_method('_is_ko').startswith('static func _is_ko'))
        self.assertNotIn('const GRADE_COLORS',source.archive_method('grade_short'))

if __name__=='__main__':unittest.main()
