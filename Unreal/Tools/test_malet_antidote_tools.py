from validate_unreal import battle_entry_test_paths, archive_test_paths, checkpoint_test_paths
import json,unittest
from pathlib import Path
import export_malet_antidote_oracle as m
from validate_unreal import battle_entry_test_paths, archive_test_paths, shop_transaction_test_paths,shop_test_paths,firebomb_test_paths,current_test_paths,antidote_test_paths
class AntidoteContracts(unittest.TestCase):
    def setUp(self):self.c={c['id']:c for c in json.loads((m.DEST/'contract_expected.v1.json').read_text(encoding='utf-8'))}
    def test_exact_147_retained(self):
        old=json.loads((m.base.ROOT/'docs/unreal-migration/evidence/phase1l/automation03/automation_index.json').read_text(encoding='utf-8-sig'))
        self.assertEqual({t['fullTestPath'] for t in old['tests']},(((current_test_paths()-battle_entry_test_paths()-archive_test_paths())-checkpoint_test_paths())-shop_transaction_test_paths()-shop_test_paths()-firebomb_test_paths())-antidote_test_paths())
    def test_source_record_and_localization(self):
        self.assertEqual(self.c['canonical']['item_record']['name'],'Antidote')
        for n in ('canonical','ko'):self.assertEqual(self.c[n]['enqueues'],[{'text':'+1 Antidote','type':1}])
    def test_sequential_real_enqueues(self):
        c=self.c['sequence'];self.assertEqual(c['enqueues'],[{'text':'+2 Potion','type':1},{'text':'+1 Antidote','type':1}]);self.assertEqual(c['deliveries'],c['enqueues'])
        self.assertEqual([s['point'] for s in c['steps']],['after_inventory_mutation','after_recent_items','inventory_changed','toast']*2)
        self.assertEqual(c['after'],{'items':{'potion':2,'antidote':1},'recent':['antidote','potion']})
    def test_raw_import_query(self):
        c=self.c['invalid'];self.assertEqual(c['before'],c['restored']);self.assertEqual(c['restored'],c['restored_after_query']);self.assertEqual(c['restored_query'],['potion','antidote']);self.assertNotEqual(c['restored']['recent'],c['restored_query'])
        self.assertEqual(self.c['five']['after']['recent'],['antidote','potion','hi_potion','firebomb','smoke_bomb'])
    def test_repeat_is_api_contract(self):
        self.assertEqual(self.c['repeat']['after']['items'],{'antidote':2});self.assertEqual(len(self.c['repeat']['enqueues']),2)
        self.assertEqual(self.c['negative']['after']['items'],{'antidote':-1});self.assertEqual(self.c['zero']['after']['items'],{'antidote':0})
    def test_absent_visual_sink_and_source_stale_difference(self):
        self.assertEqual(self.c['presentation_absent']['deliveries'],[]);self.assertEqual(len(self.c['presentation_absent']['enqueues']),1)
        self.assertEqual(self.c['replacement_potion']['after']['items'],{'antidote':1});self.assertEqual(len(self.c['replacement_antidote']['enqueues']),2)
    def test_one_shared_mutation_and_no_firebomb(self):
        r=m.base.ROOT/'Unreal/Memoria/Source/Memoria';api=(r/'Private/Run/MemoriaRunSubsystem.cpp').read_text(encoding='utf-8')
        for text in ('State.Player.Items.Add','State.Player.RecentItems=','OnInventoryChanged.Broadcast','OnItemToastRequested.Broadcast'):self.assertEqual(api.count(text),1)
        host=(r/'Private/Narrative/MemoriaNarrativeSubsystem.cpp').read_text(encoding='utf-8');body=host[host.index('void UMemoriaNarrativeSubsystem::CommitAntidoteAndDeferFirebomb'):host.index('void UMemoriaNarrativeSubsystem::PresentSeedObservation')]
        self.assertEqual(body.count('Run->AddRewardAntidote(TEXT("antidote"),1)'),1)
        self.assertIn('if(!Complete || !HasLiveRewardOwner())return;',body)
        self.assertIn('RewardToasts.Add(Text)',body)
        for text in ('SetTimer(', 'OpenLevel(', 'SaveGameToSlot(', 'AddRewardAntidote(TEXT("firebomb")'):self.assertNotIn(text,body)
if __name__=='__main__':unittest.main()
