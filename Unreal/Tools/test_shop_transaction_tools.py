"""Checks on the executed shop fixtures and retained historical test identities."""
import json,unittest
import export_shop_transactions_oracle as source
from validate_unreal import current_test_paths, shop_transaction_test_paths
class ShopTransactions(unittest.TestCase):
    def setUp(self):
        self.c={x['id']:x['states'] for x in json.loads((source.DEST/'contract_expected.v1.json').read_text(encoding='utf-8'))}
    def test_source_transaction_outcomes(self):
        self.assertEqual(set(self.c),{x['id'] for x in source.inputs()})
        for name,price in [('sense',5),('daily',15),('relation',30),('identity',60)]:
            s=self.c['sell_'+name][0];self.assertEqual(s['grains'],price);self.assertEqual(len(s['history']),1)
        self.assertTrue(next(m for m in self.c['sell_relation'][0]['memories'] if m['id']=='rel_hand_reaching')['residue'])
    def test_buy_affordability_and_stock(self):
        self.assertEqual(self.c['buy_poor'][0]['grains'],7);self.assertEqual(self.c['buy_poor'][0]['sold'],[])
        self.assertEqual(self.c['buy_exact'][0]['grains'],0)
        self.assertEqual(self.c['buy_both'][-1]['sold'],['sense_copper_taste','daily_malet_deal'])
        self.assertEqual(self.c['buy_sell_ko'][-1]['grains'],23)
    def test_oath_and_close_boundary(self):
        for locale in ['en','ko']:
            s=self.c['oath_'+locale][0];self.assertTrue(s['flags']['oath_ash_broken']);self.assertEqual(len(s['toasts']),2)
        first,last=self.c['close'];self.assertEqual(first['chapter'],3);self.assertTrue(first['flags']['ch2_complete'])
        self.assertIn('autosave:chapter_transition',first['events'])
        travel='chapter_transition:2:res://scenes/maps/belt_waystation.tscn'
        self.assertNotIn(travel,first['events']);self.assertIn(travel,last['events'])
    def test_shop_transaction_identities_retained(self):
        expected = {'Memoria.ShopTransactions.Source.' + case['id'] for case in source.inputs()}
        expected |= {'Memoria.ShopTransactions.Guards', 'Memoria.ShopTransactions.Canonical',
                     'Memoria.ShopTransactions.RequestCancellation'}
        self.assertEqual(shop_transaction_test_paths(), expected)
        current = current_test_paths()
        self.assertTrue(expected.issubset(current), expected - current)
