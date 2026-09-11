import copy,json,unittest
from pathlib import Path
import export_malet_first_effect_oracle as j
class FirstEffectContract(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.cases={c['id']:c for c in json.loads((j.DEST/'contract_expected.v1.json').read_text(encoding='utf-8'))}
    def test_exact_case_set(self):self.assertEqual(set(self.cases),set(j.CASES))
    def test_first_effect_cut_is_only_flag(self):
        for name,c in self.cases.items():
            if name=='seed_guard_false':continue
            before=c['states'][0];cut=c['boundary_snapshots'][0]
            self.assertEqual(cut['events'],['callback:reward:enter','flag:ch2_malet_done'])
            flags=copy.deepcopy(before['flags']);flags['ch2_malet_done']=True
            self.assertEqual(cut['flags'],flags);self.assertEqual(cut['world'],before['world'])
    def test_world_revision_event_matrix(self):
        expected={'fresh':['knowledge.learned','memory.added'],'knowledge_present':['memory.added'],'memory_present':['knowledge.learned'],'memory_removed':['knowledge.learned'],'memory_restored':['knowledge.learned'],'knowledge_forgotten':['memory.added'],'actor_missing':[],'already_true':['knowledge.learned','memory.added'],'repeated_reward':['knowledge.learned','memory.added'],'false_flag':['knowledge.learned','memory.added'],'case_sensitive':['knowledge.learned','memory.added'],'seed_guard_false':[]}
        for name,events in expected.items():
            b,a=self.cases[name]['states'][:2];actual=a['world_events']
            self.assertEqual([e['event_type'] for e in actual],events,name)
            self.assertEqual([e['revision'] for e in actual],list(range(b['world']['revision']+1,a['world']['revision']+1)),name)
            self.assertEqual(a['world']['revision']-b['world']['revision'],len(events),name)
    def test_tombstones_and_forgotten_are_preserved(self):
        for n,key,entry in [('memory_removed','memories','memory.malet.bl07_request_source'),('memory_restored','memories','memory.malet.bl07_request_source'),('knowledge_forgotten','knowledge','fact.bl07.route_request_received')]:
            b,a=self.cases[n]['states'][:2]
            self.assertEqual(b['world']['actors']['npc.malet'][key][entry],a['world']['actors']['npc.malet'][key][entry])
        self.assertFalse(self.cases['knowledge_forgotten']['states'][1]['world']['actors']['npc.malet']['knowledge']['fact.bl07.route_request_received']['value'])
    def test_repeated_callback_seed_only_is_idempotent(self):
        a,b=self.cases['repeated_reward']['states'][1:]
        self.assertEqual(a['world'],b['world'])
        self.assertEqual(b['items'],{'potion':4,'antidote':2,'firebomb':2})
        self.assertEqual(b['events'].count('flag:ch2_malet_done'),2)
        self.assertEqual(b['events'].count('world:seed:enter'),2)
    def test_save_flag_identity(self):
        for name,c in self.cases.items():
            for s in c['states']:self.assertEqual(s['flags'],s['save']['story_flags'])
        flags=self.cases['case_sensitive']['states'][1]['flags']
        self.assertEqual(flags,{'CH2_MALET_DONE':True,'ch2_malet_done':True})
        self.assertFalse(self.cases['false_flag']['states'][0]['flags']['ch2_malet_done'])
    def test_source_store_order(self):
        events=self.cases['fresh']['states'][1]['events']
        self.assertEqual(events[:6],['callback:reward:enter','flag:ch2_malet_done','world:seed:enter','lookup_or_store:get_actor_state','lookup_or_store:_store_knowledge_value','world:knowledge.learned:fact.bl07.route_request_received'])
        self.assertLess(events.index('lookup_or_store:_store_knowledge_value'),events.index('lookup_or_store:_store_memory_record'))
    def test_runtime_has_no_world_seed_implementation(self):
        p=j.base.ROOT/'Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaNarrativeSubsystem.cpp'
        s=p.read_text(encoding='utf-8');body=s[s.index('void UMemoriaNarrativeSubsystem::CommitRewardFlagAndDeferSeed()'):]
        self.assertEqual(body.count('Run->SetStoryFlag(TEXT("ch2_malet_done"), true)'),1)
        self.assertLess(body.index('callback:reward:enter'),body.index('Run->SetStoryFlag'))
        self.assertLess(body.index('Run->SetStoryFlag'),body.index('before:item:firebomb:1'))
        for token in ('learn_fact(', 'add_memory(', '_seed_malet_memory_world_state_if_needed(', 'SetTimer(', 'RemoveStoryFlag('):self.assertNotIn(token,body)
if __name__=='__main__':unittest.main()
