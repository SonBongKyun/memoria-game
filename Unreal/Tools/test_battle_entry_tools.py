"""Meaningful invariants of executed source battle entry and ambient withdrawal."""
import json,re,unittest
import export_battle_entry_oracle as source

class BattleEntryContract(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.inputs={c['id']:c for c in json.loads((source.DEST/'contract_inputs.v1.json').read_text(encoding='utf-8'))}
        cls.cases={c['id']:c for c in json.loads((source.DEST/'contract_expected.v1.json').read_text(encoding='utf-8'))}
        cls.catalog=json.loads((source.DEST/'source_catalog.v1.json').read_text(encoding='utf-8'))

    def test_source_methods_remain_executable_and_actions_fail_closed(self):
        code=source.battle_code()
        for name in ['start_battle','player_flee','_cleanup']:
            self.assertIn(source.body(source.BATTLE,name).rstrip(),code)
        for name in ['player_attack','player_burn','player_defend']:
            self.assertIn('Outside battle-entry oracle: '+name,code)
        self.assertNotIn('var play_stats',source.body(source.GM,'set_directive_streak'))

    def test_ambient_withdrawal_has_no_rewards_or_extra_rng(self):
        self.assertEqual(set(self.inputs),set(self.cases))
        for name,c in self.cases.items():
            initial=c['initial'];after=c['after_flee']
            self.assertEqual(initial['state'],1,name)
            self.assertEqual(after['state'],5,name)
            self.assertEqual(after['game_state'],0,name)
            self.assertIsNone(after['enemy'],name)
            self.assertEqual(after['requested_map'],self.catalog['return_scene'],name)
            presence=self.catalog['exploration_presence'][str(self.inputs[name]['chapter'])]
            self.assertEqual(after['external'][-2:],['request:presence:'+presence,'map:'+self.catalog['return_scene']],name)
            self.assertEqual(after['player'],initial['player'],name)
            self.assertEqual(after['flags'],initial['flags'],name)
            self.assertEqual(after['play_stats'],initial['play_stats'],name)
            self.assertEqual(after['burn_count'],c['before']['burn_count'],name)
            self.assertEqual(len(c['rng']),len(self.inputs[name]['rng']),name)
            self.assertEqual(after['events'][-1],{'event':'ended','state':5},name)

    def test_growth_focus_and_existing_stats_preserve_source_order(self):
        initial=self.cases['enemy0_burns0']['initial']
        self.assertEqual((initial['player']['hp'],initial['player']['max_hp']),(115,130))
        high=self.cases['high_max']['initial']['player']
        self.assertEqual((high['hp'],high['max_hp']),(173,200))
        self.assertEqual(self.cases['injured']['initial']['player']['hp'],16)
        focused=self.cases['focus100']['initial']
        self.assertEqual(focused['player']['field_focus'],2)
        self.assertEqual(focused['transient']['momentum'],25)
        self.assertEqual(focused['transient']['limit_gauge'],20)
        self.assertEqual(focused['objective']['reward_grains'],initial['objective']['reward_grains']+2)
        self.assertEqual(focused['play_stats']['highest_momentum_rank'],1)
        self.assertEqual(self.cases['existing_momentum_stat']['initial']['play_stats']['highest_momentum_rank'],4)
        self.assertEqual(initial['elia_skills']['remembered_strike']['current_cooldown'],0)
        self.assertEqual(self.cases['elia_absent']['initial']['elia_skills']['remembered_strike']['current_cooldown'],3)

    def test_all_modifier_effects_and_rng_boundaries_are_exercised(self):
        observed={c['initial']['modifier'].get('id') for c in self.cases.values()}-{None}
        expected={m['id'] for group in self.catalog['modifiers'].values() for m in group}
        self.assertEqual(observed,expected)
        for name in ['enemy0_burns0','enemy0_burns2','enemy1_burns0','enemy1_burns2']:
            self.assertEqual(self.cases[name]['rng'],[])
        atk=self.cases['mid_modifier_2']['initial']['enemy']
        self.assertEqual(atk['attack'],self.catalog['enemies'][0]['atk']+15)
        rat=self.cases['high_modifier_3']['initial']['enemy']
        self.assertEqual(rat['abilities'],['poison','shield'])
        converted=self.cases['extreme_modifier_4']['initial']
        self.assertTrue(converted['enemy']['is_void'])
        self.assertTrue(converted['enemy']['is_ambient'])
        self.assertEqual(converted['enemy']['name'],'Void Market Thief')
        self.assertEqual(converted['enemy']['weakness'],'void')
        self.assertEqual(converted['logs'][0],'A Market Thief appears!')
        self.assertEqual(converted['art']['enemy'],'')
        self.assertEqual(self.cases['high_modifier_0']['rng'][0]['value'],0.6)
        self.assertEqual(self.cases['extreme_modifier_0']['rng'][0]['value'],0.8)

    def test_source_objective_hash_uses_incremented_battle_count(self):
        for name,c in self.cases.items():
            self.assertEqual(c['initial']['total_battles'],self.inputs[name]['total_battles']+1,name)
            self.assertIn(c['initial']['objective']['id'],{o['id'] for o in self.catalog['objectives']})
        self.assertEqual(self.cases['enemy0_burns0']['initial']['objective']['id'],'witness_echo')
        self.assertEqual(self.cases['enemy1_burns0']['initial']['objective']['id'],'keep_memory')
        self.assertEqual(len(self.catalog['objectives']),11)
        self.assertIn('ally_coordination',{o['id'] for o in self.catalog['objectives']})
        self.assertTrue(self.cases['tobias_joined']['initial']['transient']['tobias_in_party'])

    def test_generated_content_art_and_locale_match_execution(self):
        data=source.base.canonical(self.catalog).decode('utf-8').strip()
        actual=source.TARGET.read_text(encoding='utf-8')
        expected=source.native_catalog_payload(source.base.canonical(self.catalog))
        self.assertEqual(actual,expected)
        # Each C++ token stays below the MSVC literal limit while concatenation
        # reconstructs the complete executed JSON without losing Unicode/escapes.
        chunks=[json.loads(value) for value in re.findall(r'TEXT\(("(?:[^"\\]|\\.)*")\)',actual)]
        self.assertGreater(len(chunks),1)
        self.assertLessEqual(max(map(len,chunks)),512)
        self.assertEqual(''.join(chunks),data)
        self.assertEqual(self.catalog['enemy_art'][0],self.cases['enemy0_burns0']['initial']['art']['enemy'])
        self.assertEqual(self.catalog['enemy_art'][1],'')
        self.assertEqual(self.catalog['background'],'')
        self.assertEqual(set(self.catalog['exploration_presence']),{str(chapter) for chapter in range(1,11)})
        self.assertTrue(all(self.catalog['exploration_presence'].values()))
        self.assertNotEqual(self.catalog['messages']['en']['A %s appears!'],self.catalog['messages']['ko']['A %s appears!'])
        self.assertTrue(self.cases['locale_ko']['initial']['objective_view']['title'])
        self.assertNotEqual(self.cases['locale_ko']['initial']['objective_view']['title'],self.cases['locale_ko']['initial']['objective']['title'])

if __name__=='__main__':unittest.main()
