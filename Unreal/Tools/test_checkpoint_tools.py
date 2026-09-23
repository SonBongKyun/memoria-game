"""Executed source save boundaries and retained test identities."""
import json,unittest,zipfile
import export_checkpoint_oracle as source
from validate_unreal import battle_entry_test_paths, archive_test_paths, checkpoint_test_paths,current_test_paths,shop_transaction_test_paths
class CheckpointContracts(unittest.TestCase):
    def setUp(self):
        self.c={x['id']:x for x in json.loads((source.DEST/'contract_expected.v1.json').read_text(encoding='utf-8'))}
    def test_capture_precedes_map_transition(self):
        self.assertEqual(set(self.c),{x['id'] for x in source.inputs()})
        c=self.c['close_save'];self.assertEqual(c['data']['scene'],'res://scenes/maps/verdan_market.tscn')
        self.assertEqual(c['data']['chapter'],3)
        self.assertTrue(c['data']['flags']['ch2_complete'])
        self.assertEqual(c['events'],['request:audio:ui_close','flag:ch2_complete','save_completed:0','autosave_completed','achievement:chapter:2','achievement:unlock:merchant'])
    def test_previous_backup_and_recovery(self):
        first=self.c['close_save']['data'];second=self.c['second_save']
        self.assertEqual(second['backup'],first);self.assertEqual(second['primary']['grains'],9)
        self.assertEqual(self.c['recovery']['data'],first);self.assertTrue(self.c['recovery']['primary_repaired'])
        self.assertTrue(self.c['load']['ok']);self.assertEqual(self.c['load']['memory'],first['memory'])
        self.assertEqual(self.c['load']['events'],['map:res://scenes/maps/verdan_market.tscn','load_completed:0'])
    def test_guard_before_mutation(self):
        self.assertEqual(self.c['missing_scene'],{'id':'missing_scene','ok':False,'grains':777,'events':[]})
        self.assertTrue(self.c['guards']['menu_unchanged']);self.assertTrue(self.c['guards']['synthetic_unchanged'])
        self.assertFalse(self.c['guards']['invalid_slot'])
    def test_previous_registry_retained(self):
        self.assertEqual(len(checkpoint_test_paths()),6)
        self.assertEqual(len((current_test_paths()-battle_entry_test_paths()-archive_test_paths())-checkpoint_test_paths()),217)
        self.assertTrue(shop_transaction_test_paths().issubset((current_test_paths()-battle_entry_test_paths()-archive_test_paths())))
