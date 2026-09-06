import copy, unittest
from narrative_ir import BASE, canonical, strict_load, load, extract, ir_path, fingerprint, validate, normalize
from narrative_test_fixtures import mutations, generated, FIXTURES
from validate_unreal import expected_test_paths, current_test_paths, narrative_test_paths, inspect_automation_report

class NarrativeTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls): cls.vn=load(ir_path('vn')); cls.field=load(ir_path('field'))
    def test_vn_source_extraction_identical(self): self.assertEqual(canonical(extract('vn')),ir_path('vn').read_bytes())
    def test_field_source_extraction_identical(self): self.assertEqual(canonical(extract('field')),ir_path('field').read_bytes())
    def test_strict_negative_semantics(self):
        for label,value in mutations(self.vn):
            with self.subTest(label=label):
                with self.assertRaises((ValueError,TypeError,KeyError)): validate(value,verify_sources=False)
    def test_wrong_hash_checked_against_source(self):
        v=copy.deepcopy(self.vn); v['sources'][1]['sha256_utf8_lf']='0'*64
        with self.assertRaises(ValueError): validate(v)
    def test_revision_is_git_attested(self):
        v=copy.deepcopy(self.vn); v['source_revision']='0'*40
        with self.assertRaises(Exception): validate(v)
    def test_changed_semantics_detected_and_not_production(self):
        v=copy.deepcopy(self.vn); v['definition']['steps'][0]['text']['narrate']+='!'; v['semantic_sha256']=fingerprint(v)
        self.assertNotEqual(v['semantic_sha256'],self.vn['semantic_sha256']); validate(v,verify_sources=False)
        with self.assertRaises(ValueError): validate(v)
    def test_empty_present_and_unicode_exact(self):
        r=self.field['definition']['rows'][0]; self.assertIn('speaker',r['text']); self.assertEqual(r['text']['speaker'],'')
        v=copy.deepcopy(self.field); v['definition']['rows'][0]['text'].pop('speaker'); self.assertNotEqual(fingerprint(v),fingerprint(self.field))
        self.assertEqual(strict_load(canonical(self.vn)),self.vn)
    def test_noncanonical_and_duplicate_keys_rejected(self):
        for b in (canonical(self.vn).replace(b'\n',b'\r\n'),b'{"a":1,"a":1}\n',b'{"a":NaN}\n',b'\xef\xbb\xbf'+canonical(self.vn)):
            with self.subTest(data=b[:20]), self.assertRaises(ValueError): strict_load(b)
    def test_unknown_authored_field_not_silently_dropped(self):
        with self.assertRaises(ValueError): normalize({'text':'a','unknown':True},'field')
        with self.assertRaises(ValueError): normalize({'fade':0.0001},'vn')
        for value in (None,True,'0'):
            with self.assertRaises(ValueError): normalize({'text':'choice','jump_to':value},'field',True)
    def test_original_choice_order(self):
        r=self.vn['definition']['steps'][10]; self.assertEqual([c['original_index'] for c in r['choices']],[0,1,2])
        self.assertEqual([c['effects']['set_flag'] for c in r['choices']],['guard_lied_to','guard_bribed','guard_pushed'])
    def test_isolated_fixture_generation(self):
        for n,v in generated().items(): self.assertEqual((FIXTURES/n).read_bytes(),canonical(v))
    def test_previous_sixty_identities_unchanged(self):
        previous=expected_test_paths(); self.assertEqual(len(previous),60); self.assertEqual(len(narrative_test_paths()),8)
        self.assertEqual(len(current_test_paths()),68); self.assertTrue(previous<current_test_paths())
        r={'tests':[{'fullTestPath':s,'state':'Success'} for s in sorted(current_test_paths())]}
        self.assertTrue(inspect_automation_report(r,current_test_paths())['passed']); r['tests'][-1]=r['tests'][0]
        self.assertFalse(inspect_automation_report(r,current_test_paths())['passed'])
if __name__=='__main__': unittest.main()
