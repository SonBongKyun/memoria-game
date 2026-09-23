from validate_unreal import battle_entry_test_paths, archive_test_paths, checkpoint_test_paths
import unittest,json,re
from pathlib import Path
import export_malet_potion_oracle as l
from validate_unreal import battle_entry_test_paths, archive_test_paths, shop_transaction_test_paths,shop_test_paths,firebomb_test_paths,current_test_paths,potion_test_paths
class PotionContracts(unittest.TestCase):
    def setUp(self):self.c={x['id']:x for x in json.loads((l.DEST/'contract_expected.v1.json').read_text(encoding='utf-8'))}
    def test_exact_retained_identities(self):
        old=json.loads((l.base.ROOT/'docs/unreal-migration/evidence/phase1k/automation04/automation_index.json').read_text(encoding='utf-8-sig'))
        self.assertEqual({x['fullTestPath'] for x in old['tests']},(((current_test_paths()-battle_entry_test_paths()-archive_test_paths())-checkpoint_test_paths())-shop_transaction_test_paths()-shop_test_paths()-firebomb_test_paths())-potion_test_paths()-__import__("validate_unreal").antidote_test_paths());self.assertEqual(len(potion_test_paths()),22)
    def test_executed_case_set(self):self.assertEqual(set(self.c),{x['id'] for x in l.inputs()});self.assertEqual(len(self.c),16)
    def test_source_order_and_payload(self):
        steps=self.c['absent']['steps'];self.assertEqual([s['point'] for s in steps],['after_inventory_mutation','after_recent_items','inventory_changed','toast']);self.assertEqual(steps[2]['payload'],{'item_id':'potion'});self.assertEqual(steps[3]['payload'],{'text':'+2 Potion','type':1});self.assertEqual(steps[0]['state']['recent'],[]);self.assertEqual(steps[1]['state']['recent'],['potion'])
    def test_recent_and_signed_source_semantics(self):
        self.assertEqual(self.c['multiple']['after']['recent'],['potion','hi_potion','antidote','firebomb','smoke_bomb']);self.assertEqual(self.c['normalize']['after']['recent'],['potion','antidote','firebomb']);self.assertEqual(self.c['negative']['after']['items'],{'potion':-1});self.assertEqual(self.c['zero']['after']['items'],{'potion':0});self.assertEqual(self.c['repeat']['after']['items'],{'potion':4})
    def test_invalid_noop_and_localization(self):
        for n in ('invalid','case_sensitive'):self.assertEqual(self.c[n]['steps'],[]);self.assertEqual(self.c[n]['before'],self.c[n]['after'])
        self.assertEqual(self.c['ko']['steps'][-1]['payload'],self.c['absent']['steps'][-1]['payload'])
    def test_source_replacement_difference_explicit(self):
        self.assertEqual([s['point'] for s in self.c['replacement_signal']['steps']][-2:],['replacement','toast']);self.assertEqual(self.c['replacement_signal']['after'],{'items':{},'recent':[]})
    def test_production_frontier_and_domains(self):
        r=l.base.ROOT/'Unreal/Memoria/Source/Memoria';s=(r/'Private/Narrative/MemoriaNarrativeSubsystem.cpp').read_text(encoding='utf-8');body=s[s.index('void UMemoriaNarrativeSubsystem::CommitRewardFlagAndDeferSeed'):s.index('void UMemoriaNarrativeSubsystem::PresentSeedObservation')]
        self.assertEqual(body.count('Run->AddRewardPotion(TEXT("potion"),2)'),1);self.assertLess(body.index('World->SeedMaletRoute'),body.index('Run->AddRewardPotion'));self.assertLess(body.index('Run->AddRewardPotion'),body.index('before:shop_open'))
        for x in ('SetTimer(','OpenLevel(','SaveGameToSlot(','AddRewardPotion(TEXT("antidote")','AddRewardPotion(TEXT("firebomb")'):self.assertNotIn(x,body)
        api=(r/'Private/Run/MemoriaRunSubsystem.cpp').read_text(encoding='utf-8').split('bool UMemoriaRunSubsystem::AddRewardPotion',1)[1]
        for x in ('BurnMemory','LearnFact','AddMemory','reward_granted_once','potion_granted_once','SetTimer'):self.assertNotIn(x,api)
        for a,b in [('State.Player.Items.Add','after_inventory_mutation'),('State.Player.RecentItems=','OnInventoryChanged.Broadcast'),('OnInventoryChanged.Broadcast','OnItemToastRequested.Broadcast')]:self.assertLess(api.index(a),api.index(b))
if __name__=='__main__':unittest.main()
