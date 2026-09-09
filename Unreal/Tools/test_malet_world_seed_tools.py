import json,unittest
from pathlib import Path
import export_malet_world_seed_oracle as k
from validate_unreal import current_test_paths,world_seed_test_paths
class WorldSeedTools(unittest.TestCase):
    @classmethod
    def setUpClass(cls):cls.c={x['id']:x for x in json.loads((k.DEST/'contract_expected.v1.json').read_text(encoding='utf-8'))}
    def test_exact_cases_and_retained_identities(self):
        self.assertEqual(set(self.c),set(k.CASES));self.assertEqual(len(current_test_paths()-world_seed_test_paths()),107);self.assertEqual(len(world_seed_test_paths()),18)
    def test_fresh_revisions_and_payload(self):
        c=self.c['fresh'];self.assertEqual(c['before']['revision'],0);self.assertEqual([s['world']['revision'] for s in c['steps']],[1,2]);self.assertEqual([e['event_type'] for e in c['events']],['knowledge.learned','memory.added'])
        a=c['after']['actors']['npc.malet'];self.assertEqual(a['knowledge']['fact.bl07.route_request_received'],dict(fact_id='fact.bl07.route_request_received',value=True,updated_revision=1));m=a['memories']['memory.malet.bl07_request_source'];self.assertEqual(m['content'],dict(kind='information_source',subject='bl07_route_request'));self.assertEqual(m['source_actor_id'],'player.arrel');self.assertEqual(m['created_revision'],2)
    def test_all_cases_repeat_and_mutation_accounting(self):
        for name,c in self.c.items():
            self.assertEqual(c['after'],c['after_repeat'],name);self.assertEqual(c['repeat_events'],[],name)
            self.assertEqual(c['after']['revision']-c['before']['revision'],len(c['events']),name);self.assertEqual(c['after']['event_sequence']-c['before']['event_sequence'],len(c['events']),name)
    def test_removed_restored_forgotten(self):
        for name,section,key in [('removed','memories','memory.malet.bl07_request_source'),('restored','memories','memory.malet.bl07_request_source'),('forgotten','knowledge','fact.bl07.route_request_received')]:
            c=self.c[name];self.assertEqual(c['before']['actors']['npc.malet'][section][key],c['after']['actors']['npc.malet'][section][key])
    def test_guard_and_missing_actor(self):
        for name in ('flag_false','case_sensitive','actor_missing','both','save_restore'):
            self.assertEqual(self.c[name]['before'],self.c[name]['after']);self.assertEqual(self.c[name]['events'],[])
    def test_production_domain_and_item_boundary(self):
        root=k.base.ROOT/'Unreal/Memoria/Source/Memoria';world=(root/'Private/World/MemoriaWorldCognition.cpp').read_text();host=(root/'Private/Narrative/MemoriaNarrativeSubsystem.cpp').read_text();body=host[host.index('void UMemoriaNarrativeSubsystem::CommitRewardFlagAndDeferSeed()'):]
        for token in ('PlayerMemoryDomain','BurnMemory','FName','SetTimer','AddItem','MemoryShop'):self.assertNotIn(token,world)
        self.assertLess(body.index('Run->SetStoryFlag'),body.index('World->SeedMaletRoute'));self.assertLess(body.index('World->SeedMaletRoute'),body.index('worldseed:end'));self.assertLess(body.index('worldseed:end'),body.index('before:item:potion:2'))
        for token in ('AddItem(', 'SaveGameToSlot(', 'SetTimer(', 'BurnMemory('):self.assertNotIn(token,body)
    def test_save_schema_is_reserved_one(self):
        root=k.base.ROOT/'Unreal/Memoria/Source/Memoria';header=(root/'Public/Save/MemoriaRunSaveGame.h').read_text();runtime=(root/'Private/Run/MemoriaRunSubsystem.cpp').read_text();self.assertIn('CurrentSchemaVersion = 1',header);self.assertIn('Save->WorldCognition.SourceJson=WorldCognition->ExportJson()',runtime);self.assertIn('!UMemoriaWorldCognition::Decode',runtime)
if __name__=='__main__':unittest.main()
