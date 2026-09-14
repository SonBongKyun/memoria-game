import json,unittest
from pathlib import Path
import export_malet_firebomb_oracle as n
from validate_unreal import shop_test_paths,current_test_paths,firebomb_test_paths
class FirebombContracts(unittest.TestCase):
    def setUp(self):self.c={c['id']:c for c in json.loads((n.DEST/'contract_expected.v1.json').read_text(encoding='utf-8'))}
    def test_exact_169_retained(self):
        old=json.loads((n.base.ROOT/'docs/unreal-migration/evidence/phase1m/automation01/automation_index.json').read_text(encoding='utf-8-sig'))
        self.assertEqual({t['fullTestPath'] for t in old['tests']},current_test_paths()-shop_test_paths()-firebomb_test_paths())
    def test_source_record_and_localization(self):
        self.assertEqual(self.c['canonical']['item_record'],dict(name='Firebomb',desc='Deals 12 damage, then burns the enemy for 2 turns.',type='burn',power=15,impact=12,price=18,icon='res://assets/ui/items/firebomb.png'))
        for name in ('canonical','ko'):self.assertEqual(self.c[name]['enqueues'],[dict(text='+1 Firebomb',type=1)])
    def test_three_actual_queue_appends(self):
        c=self.c['sequence'];self.assertEqual(c['enqueues'],[dict(text='+2 Potion',type=1),dict(text='+1 Antidote',type=1),dict(text='+1 Firebomb',type=1)]);self.assertEqual(c['deliveries'],c['enqueues'])
        self.assertEqual([s['point'] for s in c['steps']],['after_inventory_mutation','after_recent_items','inventory_changed','toast']*3)
        self.assertEqual(c['steps'][-2]['payload'],dict(item_id='firebomb'))
    def test_raw_query_and_explicit_repeat(self):
        self.assertEqual(self.c['five']['after']['recent'],['firebomb','potion','antidote','hi_potion','smoke_bomb'])
        c=self.c['invalid'];self.assertEqual(c['before'],c['restored']);self.assertEqual(c['restored'],c['restored_after_query']);self.assertNotEqual(c['restored']['recent'],c['restored_query'])
        self.assertEqual(self.c['repeat']['after']['items'],dict(firebomb=2));self.assertEqual(self.c['negative']['after']['items'],dict(firebomb=-1));self.assertEqual(self.c['zero']['after']['items'],dict(firebomb=0))
    def test_stale_source_and_presentation(self):
        self.assertEqual(self.c['presentation_absent']['deliveries'],[]);self.assertEqual(len(self.c['presentation_absent']['enqueues']),1)
        for name in ('replacement_potion','replacement_antidote','replacement_firebomb'):self.assertEqual(len(self.c[name]['enqueues']),3)
    def test_shared_mutation_and_stop_before_shop(self):
        r=n.base.ROOT/'Unreal/Memoria/Source/Memoria';api=(r/'Private/Run/MemoriaRunSubsystem.cpp').read_text(encoding='utf-8')
        for text in ('State.Player.Items.Add','State.Player.RecentItems=','OnInventoryChanged.Broadcast','OnItemToastRequested.Broadcast'):self.assertEqual(api.count(text),1)
        host=(r/'Private/Narrative/MemoriaNarrativeSubsystem.cpp').read_text(encoding='utf-8');body=host[host.index('void UMemoriaNarrativeSubsystem::CommitFirebombAndDeferShop'):host.index('void UMemoriaNarrativeSubsystem::PresentSeedObservation')]
        self.assertEqual(body.count('Run->AddRewardFirebomb(TEXT("firebomb"),1)'),1);self.assertIn('if(!Complete || !HasLiveRewardOwner())return;',body);self.assertIn('before:shop_open',body)
        for text in ('SetTimer(', 'OpenLevel(', 'SaveGameToSlot(', 'open_shop(', '_open_malet_shop(', 'shop_closed', 'shop_items', 'first_shop', 'SetStoryFlag('):self.assertNotIn(text,body)
if __name__=='__main__':unittest.main()
