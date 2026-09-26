"""Execute pinned Godot battle rules, with excluded echo/party systems inert.
Record every random call and bound for native deterministic replay.
"""
from pathlib import Path
import argparse,json,re,zipfile
import export_battle_entry_oracle as entry
base=entry.base
DEST=base.ROOT/'docs/unreal-migration/fixtures/battle_core'
EXCLUDED={'_apply_memory_echo','_process_echoes_turn','_emit_companion_burn_reaction'}
KEEP=entry.KEEP|set('''player_attack player_burn player_defend player_use_item _enemy_turn _try_enemy_ability
_select_ability _check_player_defeated _check_enemy_defeated _end_player_turn _process_modifier_effects
has_modifier _get_player_attack _get_player_defense _get_combo_multiplier _check_combo_milestone
_reset_combo _get_element_multiplier _log_element_effect _register_break_pressure _apply_flat_break_pressure
_apply_break_damage_bonus _apply_momentum_damage_bonus _get_momentum_damage_mult _get_momentum_grains_bonus
_get_grains_reward _get_tactical_bonus _try_item_drop_return _finalize_tactical_objective
apply_status _process_statuses _get_weaken_multiplier has_status _get_status_name _soften_player_statuses
_try_last_stand_resonance _apply_burn_aftershock _advance_burn_aftershock get_burn_aftershock_preview
get_stance_atk_mult get_stance_def_mult has_echo consume_echo_charge _get_env_element_mult get_env_heal_mult
_check_env_evasion _check_env_enemy_miss dismiss_victory _calculate_battle_grade _advance_directive_streak'''.split())|EXCLUDED

def inputs():
    cases=[]
    def add(id,actions,**kw):
        c=dict(id=id,locale='en',enemy_index=0,chapter=3,hp=115,max_hp=130,enemy_hp=0,
               enemy_atk=0,abilities=[],objective='keep_memory',weakness='',resistance='',
               actions=actions,draws=[],statuses=[],enemy_statuses=[],erosion={},momentum=0,break_gauge=0,
               modifier_effect='',modifier_value=0,is_void=False,streak=0,flags={'ch2_complete':True},items={'potion':2,'antidote':2,'firebomb':2})
        c.update(kw);cases.append(c)
    attack={'action':'attack'};defend={'action':'defend'}
    burn=lambda id:dict(action='burn',id=id)
    item=lambda id:dict(action='item',id=id)
    add('attack_only',[attack]*6)
    add('ember_then_attack',[burn('sense_forest_smell')]+[attack]*5)
    add('grade3_chain',[burn('rel_hand_reaching'),burn('identity_first_sword')],enemy_hp=2000)
    add('grade2_dot',[burn('identity_first_sword'),defend],enemy_hp=2000)
    add('grade1',[burn('core_name_origin')],enemy_hp=2000)
    add('grade4_eroded',[burn('daily_market_food')],enemy_hp=400,erosion={'daily_market_food':40})
    add('guard_heal',[defend]);add('guard_limit',[defend],hp=130)
    add('guard_status',[defend],statuses=[{'effect':0,'turns':3,'power':7}])
    add('potion',[item('potion')],hp=42)
    add('antidote',[item('antidote')],hp=70,statuses=[{'effect':0,'turns':3,'power':7},{'effect':2,'turns':2,'power':4}])
    add('firebomb',[item('firebomb'),defend])
    add('poison_then_cure',[defend,item('antidote')],draws=[{'kind':'float','value':0.0},{'kind':'float','value':0.0},{'kind':'int','value':3}])
    add('weaken_attack',[defend,attack],enemy_index=1,draws=[{'kind':'float','value':0.0},{'kind':'float','value':0.0}])
    for ability in ['shield','reflect','charge','stun','drain']:
        add('ability_'+ability,[defend,attack,attack],enemy_hp=500,abilities=[ability],draws=[{'kind':'float','value':0.0},{'kind':'float','value':0.0}])
    add('break_two_turns',[attack]*4,enemy_hp=500,weakness='physical',break_gauge=58,objective='force_break')
    add('last_stand_defeat',[attack]*4,enemy_hp=1000,enemy_atk=200,hp=10)
    add('drop_yes',[attack],enemy_hp=1,draws=[{'kind':'int','value':5},{'kind':'float','value':0.0},{'kind':'int','value':5}])
    add('drop_no',[attack],enemy_hp=1)
    add('objective_failed',[burn('sense_forest_smell'),attack],enemy_hp=40)
    add('objective_swift',[attack],enemy_hp=1,objective='swift_finish')
    add('objective_combo',[attack]*4,enemy_hp=100,objective='combo_three')
    add('objective_momentum',[attack],enemy_hp=1,momentum=74,objective='kindle_momentum')
    add('resisted',[attack],enemy_hp=300,resistance='physical')
    add('strong_status_refresh',[defend,defend],statuses=[{'effect':0,'turns':1,'power':20}],draws=[{'kind':'float','value':0.0},{'kind':'float','value':0.0}])
    add('oath_still',[burn('daily_campfire_song')],flags={'ch2_complete':True,'oath_still_sworn':True})
    add('modifier_both_dot',[defend,attack],modifier_effect='dot_both',modifier_value=5)
    add('modifier_player_dot',[defend,attack],modifier_effect='player_dot',modifier_value=8)
    add('modifier_miss',[attack],modifier_effect='player_miss',modifier_value=100)
    add('modifier_double',[defend,defend],modifier_effect='enemy_double_turn',modifier_value=1)
    add('modifier_limit',[defend,defend],modifier_effect='turn_limit',modifier_value=2,streak=2)
    add('void_physical',[attack,attack],is_void=True)
    add('victory_streak',[attack],enemy_hp=1,streak=2)
    for c in cases:
        for locale in ['en','ko']:
            out=json.loads(json.dumps(c));out['id']+='_'+locale;out['locale']=locale;yield out
RNG='''extends Node
var tape=[]
var index=0
var calls=[]
func reset(values: Array) -> void:
    tape=values.duplicate(true);index=0;calls.clear()
func take(kind: String,lo: float,hi: float) -> float:
    var value=0.99 if kind=="float" else floor((lo+hi)/2.0)
    if index<tape.size():
        var entry=tape[index];index+=1
        assert(entry.kind==kind,"Random kind changed: "+kind)
        value=float(entry.value)
    assert(value>=lo and value<=hi,"Random value outside bounds")
    calls.append({"kind":kind,"min":lo,"max":hi,"value":value})
    return value
func randf() -> float:return take("float",0,1)
func randi_range(lo: int,hi: int) -> int:return int(take("int",lo,hi))
func randf_range(lo: float,hi: float) -> float:return take("range",lo,hi)
'''
RUN='''extends Node
var logs=[]
var events=[]
var reward={}
var ended=false
var player_ready=false
func _ready() -> void:call_deferred("run")
func statuses(values: Array) -> Array:
    var result=[]
    for v in values:result.append({"effect":v.effect,"turns":v.turns_left,"power":v.power})
    return result
func snapshot() -> Dictionary:
    var b=BattleManager
    var burned=[]
    for m in MemoryManager.burned_memories:burned.append(m.id)
    return {"hp":GameManager.player_data.hp,"max_hp":GameManager.player_data.max_hp,
        "enemy_hp":b.current_enemy.hp if b.current_enemy else 0,"state":int(b.state),
        "momentum":b.momentum,"rank":b.momentum_rank,"limit":b.limit_gauge,"break":b.enemy_break_gauge,
        "broken_turns":b.enemy_broken_turns,"combo":b.combo_count,"chain":b._burn_chain,
        "defending":b.player_defending,"shielded":b.enemy_shielded,"reflecting":b._enemy_reflecting,
        "charged":b._enemy_charged,"last_stand":b._last_stand_triggered_this_battle,
        "aftershock":b._burn_aftershock_turns,"turns":b._total_turns,"actions":b._player_actions_this_battle,
        "player_statuses":statuses(b.player_statuses),"enemy_statuses":statuses(b.enemy_statuses),
        "grains":GameManager.player_data.grains,"streak":GameManager.get_directive_streak(),"focus":GameManager.get_field_focus(),"items":GameManager.player_data.items.duplicate(true),
        "burned":burned,"flags":GameManager.story_flags.duplicate(true),"reward":reward.duplicate(true),
        "logs":logs.duplicate(),"events":events.duplicate(true),"rng":OracleRng.calls.duplicate(true),
        "objective_complete":b._objective_completed,"objective_failed":b._objective_failed}
func run() -> void:
    var args=OS.get_cmdline_user_args()
    GameManager.story_flags={"ch2_complete":true}
    var map=load("res://battle_map.gd").new()
    OracleRng.reset([]);map._setup_random_encounters()
    var pool=map._encounter_data.enemy_pool.duplicate(true)
    var scene=map._encounter_data.map_scene
    map.free()
    BattleManager.battle_log.connect(func(s):logs.append(s))
    BattleManager.damage_dealt.connect(func(target,amount,skill):events.append({"target":target,"amount":amount,"skill":skill}))
    BattleManager.victory_rewards_ready.connect(func(r):reward=r.duplicate(true))
    BattleManager.battle_ended.connect(func(_r):ended=true)
    BattleManager.player_turn_started.connect(func():player_ready=true)
    var catalog={"burn_skills":BattleManager.BURN_SKILLS,"items":GameManager.ITEMS,"messages":{"en":{},"ko":{}}}
    for locale in ["en","ko"]:
        GameManager.current_locale=locale
        for pair in JSON.parse_string(FileAccess.get_file_as_string("res://message_pairs.json")):
            catalog.messages[locale][pair[0]]=BattleManager._bl(pair[0],pair[1])
    FileAccess.open("res://core_catalog.json",FileAccess.WRITE).store_string(JSON.stringify(catalog))
    var results=[]
    for c in JSON.parse_string(FileAccess.get_file_as_string(args[0])):
        GameManager.current_locale=c.locale;GameManager.current_chapter=int(c.chapter)
        GameManager.story_flags=c.flags.duplicate(true)
        GameManager.player_data={"name":"Arrel","hp":int(c.hp),"max_hp":int(c.max_hp),"grains":17,
            "elia_with_party":true,"field_focus":0,"directive_streak":int(c.streak),"items":c.items.duplicate(true),
            "recent_items":[],"item_quick_slots":["potion","antidote","firebomb"]}
        GameManager.play_stats=GameManager.initial_play_stats.duplicate(true)
        MemoryManager.memories.clear();MemoryManager.burned_memories.clear();MemoryManager.burn_passives.clear()
        MemoryManager.anchor_passives.clear();MemoryManager._init_starting_memories()
        for m in MemoryManager.memories:
            if c.erosion.has(m.id):m.erosion=int(c.erosion[m.id])
        var p=pool[int(c.enemy_index)]
        var enemy=BattleManager.Enemy.new(p.name,int(p.hp),int(p.atk),c.is_void)
        enemy.is_ambient_encounter=true;enemy.abilities=p.abilities.duplicate()
        if c.enemy_hp>0:enemy.hp=int(c.enemy_hp);enemy.max_hp=int(c.enemy_hp)
        if c.enemy_atk>0:enemy.attack=int(c.enemy_atk)
        if not c.abilities.is_empty():enemy.abilities=c.abilities.duplicate()
        if c.weakness!="":enemy.weakness=c.weakness
        if c.resistance!="":enemy.resistance=c.resistance
        OracleRng.reset([]);BattleManager.start_battle(enemy,scene)
        if c.modifier_effect!="":BattleManager._encounter_modifier={"effect":c.modifier_effect,"value":int(c.modifier_value)}
        BattleManager.sable_in_party=false;BattleManager.tobias_in_party=false
        BattleManager.tactical_objective={}
        for objective in BattleManager._oracle_objective_pool:
            if objective.id==c.objective:BattleManager.tactical_objective=objective.duplicate(true)
        assert(not BattleManager.tactical_objective.is_empty(),"Missing source objective")
        BattleManager._objective_completed=false;BattleManager._objective_failed=false
        BattleManager.momentum=float(c.momentum);BattleManager.momentum_rank=BattleManager._get_momentum_rank(c.momentum)
        BattleManager._best_momentum_rank=BattleManager.momentum_rank;BattleManager.enemy_break_gauge=float(c.break_gauge)
        for st in c.statuses:BattleManager.apply_status("player",int(st.effect),int(st.turns),int(st.power))
        for st in c.enemy_statuses:BattleManager.apply_status("enemy",int(st.effect),int(st.turns),int(st.power))
        logs.clear();events.clear();reward={};ended=false;OracleRng.reset(c.draws)
        var states=[snapshot()]
        for action in c.actions:
            if ended:break
            events.clear();logs.clear();player_ready=false
            var old_actions=BattleManager._player_actions_this_battle
            match action.action:
                "attack":BattleManager.player_attack()
                "burn":
                    assert(MemoryManager._get_memory(action.id)!=null,"Unknown memory: "+action.id)
                    BattleManager.player_burn(action.id)
                "defend":BattleManager.player_defend()
                "item":BattleManager.player_use_item(action.id)
            assert(BattleManager._player_actions_this_battle>old_actions,"Action was rejected")
            while not ended and not player_ready:await get_tree().process_frame
            await get_tree().create_timer(0.06).timeout
            states.append(snapshot())
        assert(OracleRng.index==OracleRng.tape.size(),"Unused prescribed draws")
        results.append({"id":c.id,"states":states})
        if BattleManager.state==BattleManager.BattleState.VICTORY:
            BattleManager.dismiss_victory()
            while BattleManager.current_enemy!=null:await get_tree().process_frame
        elif not ended:
            BattleManager.player_flee()
            while BattleManager.current_enemy!=null:await get_tree().process_frame
        else:await get_tree().create_timer(0.03).timeout
    FileAccess.open(args[1],FileAccess.WRITE).store_string(JSON.stringify(results))
    print("MEMORIA_NARRATIVE_ORACLE_PASS cases=%d" % results.size());get_tree().quit(0)
'''
def replace_method(code,name,body):
    return re.sub(r'^func '+name+r'\([^\n]*\n(?:[ \t][^\n]*\n|\n)*',lambda _:body+'\n',code,flags=re.M)
def setup(work):
    old=entry.KEEP;entry.KEEP=KEEP
    try:entry.setup(work)
    finally:entry.KEEP=old
    code=(work/'battle.gd').read_text(encoding='utf-8')
    for name in EXCLUDED:
        code=replace_method(code,name,entry.body(entry.BATTLE,name).splitlines()[0]+'\n\tpass # excluded slice system\n')
    code=replace_method(code,'_build_battle_aftermath_line','func _build_battle_aftermath_line() -> String:\n\treturn "" # presentation-only aftermath\n')
    code=code.replace('get_tree().create_timer(', 'get_tree().create_timer(0.01 * ')
    (work/'battle.gd').write_text(code,encoding='utf-8')
    (work/'rng.gd').write_text(RNG.replace('    ','\t'),encoding='utf-8')
    gm=(work/'gm.gd').read_text(encoding='utf-8')
    for name in ['remove_item','_record_recent_item']:
        method=entry.body(entry.GM,name)
        if re.search(r'^func '+name+r'\(',gm,re.M):gm=replace_method(gm,name,method)
        else:gm+='\n'+method
    gm+='\nfunc get_equip_bonus(_key: String) -> int:return 0\nfunc has_equip_effect(_key: String) -> bool:return false\n'
    (work/'gm.gd').write_text(gm,encoding='utf-8')
    with (work/'audio_manager_stub.gd').open('a',encoding='utf-8') as f:
        f.write('\nfunc play_combat_sfx(_id: String) -> void:pass\nfunc update_low_hp_audio(_ratio: float) -> void:pass\nfunc update_battle_intensity(_ratio: float) -> void:pass\n')
    sink=(work/'unused.gd').read_text(encoding='utf-8')
    sink=replace_method(sink,'check_grains','func check_grains() -> void:pass\n')
    sink+='\nfunc record_item_used() -> void:pass\n'
    (work/'unused.gd').write_text(sink,encoding='utf-8')
    trans=(work/'transition.gd').read_text(encoding='utf-8')
    trans=replace_method(trans,'change_scene','func change_scene(path: String) -> void:\n\tGameManager.requested_map=path\n')
    (work/'transition.gd').write_text(trans,encoding='utf-8')
    with (work/'elia.gd').open('a',encoding='utf-8') as f:f.write('\nfunc tick_cooldowns() -> void:pass\n')
    (work/'tutorial.gd').write_text('extends Node\nfunc show_hint(_id: String) -> void:pass\n',encoding='utf-8')
    project=(work/'project.godot').read_text(encoding='utf-8').replace('[rendering]','TutorialHints="*res://tutorial.gd"\n[rendering]')
    (work/'project.godot').write_text(project,encoding='utf-8')
def run(a):
    base.BASE=DEST;base.inputs=lambda:list(inputs());base.ORACLE=RUN.replace('    ','\t')
    DEST.mkdir(parents=True,exist_ok=True)
    try:base.run(a,project_setup=setup)
    finally:
        report_path=a.evidence_dir/'oracle.json'
        if report_path.exists():
            report=json.loads(report_path.read_text(encoding='utf-8'));work=Path(report['commands'][-1]['command'][3])
            report['source_raw_hashes'][entry.BATTLE]=base.sha((base.ROOT/entry.BATTLE).read_bytes())
            report['bounded_adapters']='No equipment, echoes/party skills; presentation/profile sinks; timers x0.01; damage/RNG/status/objectives/rewards/burn/oath unchanged.'
            report_path.write_bytes(base.canonical(report))
            with zipfile.ZipFile(a.evidence_dir/'executed_harness.zip','w',zipfile.ZIP_DEFLATED) as z:
                for p in work.glob('*'):
                    if p.is_file():z.write(p,p.name)
    content=base.canonical(json.loads((work/'core_catalog.json').read_text(encoding='utf-8')))
    target=DEST/'source_catalog.v1.json'
    payload=entry.native_catalog_payload(content).replace('MemoriaBattleSourceJson','MemoriaBattleCoreSourceJson').replace('export_battle_entry_oracle','export_battle_core_oracle')
    native=base.ROOT/'Unreal/Memoria/Source/Memoria/Private/Battle/MemoriaBattleCoreSource.inl'
    if a.check:
        assert target.read_bytes()==content
        assert native.read_text(encoding='utf-8')==payload
    else:
        target.write_bytes(content);native.write_text(payload,encoding='utf-8')
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--godot',type=Path,required=True);p.add_argument('--evidence-dir',type=Path,required=True);p.add_argument('--check',action='store_true');run(p.parse_args())
