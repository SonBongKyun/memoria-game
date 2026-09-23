"""Execute source battle entry and guaranteed ambient withdrawal in an isolated Godot app.
Original gameplay method bodies are retained. Only RNG built-ins are redirected to
an asserted typed tape; unavailable action methods are explicit fail-closed traps.
"""
from pathlib import Path
import argparse,json,re,zipfile,shutil
import export_malet_first_effect_oracle as prior
base=prior.base;g=prior.g
DEST=base.ROOT/'docs/unreal-migration/fixtures/battle_entry'
BATTLE='scripts/systems/battle_manager.gd'
MAP='scenes/maps/verdan_market.gd'
MOD='scripts/utils/encounter_modifiers.gd'
GM='scripts/core/game_manager.gd'
TARGET=base.ROOT/'Unreal/Memoria/Source/Memoria/Private/Battle/MemoriaBattleSource.inl'
KEEP=set('''_ready _bl start_battle player_flee _cleanup _coerce_enemy _enemy_from_dictionary
_normalize_enemy_key _get_enemy_preset_art resolve_enemy_image_by_name _uses_canonical_character_shot
_get_opening_tactical_hint _setup_tactical_objective select_tactical_objective _get_tactical_objective_progress
_emit_tactical_objective_update _check_tactical_objective _complete_tactical_objective _fail_tactical_objective
_ensure_tactical_objective_selected _reset_momentum _get_momentum_rank _get_momentum_label _add_momentum
_add_limit _get_difficulty_scale _get_witness_requirement _get_witness_line _get_witness_key
_record_witness_scan _apply_field_entry_bonuses _apply_opening_choice_battle_trait _detect_environment
prepare_field_entry get_field_entry_summary get_battle_speed paced pace_timer localize_objective_title
localize_objective_desc get_field_entry_bonus_text get_witness_state get_burn_aftershock_state get_next_turn_hint'''.split())

def inputs():
    cases=[]
    def add(name,**kw):
        c=dict(id=name,enemy_index=0,locale='en',burn_count=0,synthetic_burn_count=True,
               chapter=3,hp=100,max_hp=100,focus=0,grains=17,directive_streak=2,total_battles=0,
               flags={'ch2_complete':True},entry_mode='neutral',entry_power=0,elia=True,rng=[])
        c.update(kw);cases.append(c)
    for enemy in range(2):
        for burns in [0,2,4,7,10]:
            add(f'enemy{enemy}_burns{burns}',enemy_index=enemy,burn_count=burns,
                rng=[] if burns<=2 else [{'kind':'float','value':0.99}])
    add('locale_ko',enemy_index=1,locale='ko')
    add('tobias_joined',flags={'ch2_complete':True,'tobias_joined':True})
    add('focus100',focus=100)
    add('focus1',focus=1)
    add('high_max',hp=173,max_hp=200)
    add('injured',hp=1)
    add('elia_absent',elia=False)
    add('existing_momentum_stat',highest_momentum_rank=4,focus=1)
    add('opening_burn',flags={'ch2_complete':True,'burned_for_passage':True,'listened_to_humming':True})
    add('opening_refusal',flags={'ch2_complete':True,'refused_to_burn':True,'listened_to_humming':True})
    add('opening_spent',flags={'ch2_complete':True,'burned_for_passage':True,'refused_to_burn':True,
                              'listened_to_humming':True,'ch1_opening_trait_spent':True,'ch1_humming_focus_spent':True})
    for mode in ['ambush','guarded','witness']:
        add('entry_'+mode,entry_mode=mode,entry_power=100,focus=1)
    for count in [1,2,19,2147483646]:add('total_'+str(count),total_battles=count,enemy_index=count%2)
    for i in range(3):add('mid_modifier_'+str(i),burn_count=4,rng=[{'kind':'float','value':0.0},{'kind':'int','value':i}])
    for i in range(7):
        tape=[{'kind':'float','value':0.6},{'kind':'int','value':i}]
        if i==3:tape.append({'kind':'int','value':4})
        add('high_modifier_'+str(i),burn_count=7,rng=tape)
    for i in range(8):
        tape=[{'kind':'float','value':0.8},{'kind':'int','value':i}]
        if i==0:tape.append({'kind':'int','value':0})
        add('extreme_modifier_'+str(i),enemy_index=1,burn_count=10,rng=tape)
    return cases

RNG='''extends Node
var tape: Array=[]
var calls: Array=[]
var index=0
func reset(values: Array) -> void:
    tape=values.duplicate(true);calls.clear();index=0
func take(kind: String,lo: float,hi: float) -> float:
    assert(index<tape.size(),"Unexpected random call")
    var entry=tape[index];index+=1
    assert(entry.kind==kind,"Random kind changed")
    var value=float(entry.value)
    assert(value>=lo and value<=hi,"Random value outside source range")
    calls.append({"kind":kind,"min":lo,"max":hi,"value":value})
    return value
func randf() -> float:return take("float",0.0,1.0)
func randi_range(lo: int,hi: int) -> int:return int(take("int",lo,hi))
func randf_range(lo: float,hi: float) -> float:return take("range",lo,hi)
'''
RUN=r'''extends Node
var logs: Array=[]
var events: Array=[]
func _ready() -> void:call_deferred("run")
func enemy_snapshot() -> Variant:
    var e=BattleManager.current_enemy
    if e==null:return null
    return {"name":e.name,"hp":e.hp,"max_hp":e.max_hp,"attack":e.attack,"is_void":e.is_void_beast,
        "is_boss":e.is_boss,"is_ambient":e.is_ambient_encounter,"phase":e.phase,"phase_changed":e.phase_changed,
        "abilities":e.abilities.duplicate(),"weakness":e.weakness,"resistance":e.resistance}
func snapshot() -> Dictionary:
    var transient={}
    for property in BattleManager.get_property_list():
        if not (property.usage & PROPERTY_USAGE_SCRIPT_VARIABLE):continue
        var key=String(property.name)
        if key in ["current_enemy","_oracle_objective_pool"]:continue
        var value=BattleManager.get(key)
        if value is Object:continue
        transient[key]=value.duplicate(true) if value is Array or value is Dictionary else value
    return {"player":GameManager.player_data.duplicate(true),"flags":GameManager.story_flags.duplicate(true),
        "total_battles":GameManager.play_stats.total_battles,"play_stats":GameManager.play_stats.duplicate(true),
        "burn_count":MemoryManager.get_burn_count(),"enemy":enemy_snapshot(),"state":BattleManager.state,
        "game_state":GameManager.current_state,"transient":transient,
        "modifier":BattleManager._encounter_modifier.duplicate(true),"objective":BattleManager.tactical_objective.duplicate(true),
        "objective_options":BattleManager.tactical_objective_options.duplicate(true),
        "objective_view":{"title":BattleManager.localize_objective_title(BattleManager.tactical_objective.get("title","")),
            "description":BattleManager.localize_objective_desc(BattleManager.tactical_objective.get("desc","")),
            "progress":BattleManager._get_tactical_objective_progress()},
        "logs":logs.duplicate(),"events":events.duplicate(true),"external":GameManager.events.duplicate(),
        "elia_skills":EliaDiary.skills.duplicate(true),"art":{"background":BattleManager.battle_bg_image,"enemy":BattleManager.enemy_image},
        "entry_text":BattleManager.get_field_entry_bonus_text(),"requested_map":GameManager.requested_map}
func run() -> void:
    var args=OS.get_cmdline_user_args()
    var results=[]
    var map=load("res://battle_map.gd").new()
    GameManager.set_flag("ch2_complete")
    OracleRng.reset([{"kind":"range","value":60}])
    map._setup_random_encounters()
    var pool=map._encounter_data.enemy_pool.duplicate(true)
    var scene=map._encounter_data.map_scene
    var catalog={"enemy_pool":pool,"return_scene":scene,"min_steps":map._encounter_data.min_steps,
        "max_steps":map._encounter_data.max_steps,"modifiers":EncounterModifier.MODIFIERS,
        "extra_abilities":EncounterModifier.EXTRA_ABILITIES,"objectives":[],
        "objective_titles_ko":BattleManager.OBJECTIVE_TITLES_KO,"objective_descriptions_ko":BattleManager.OBJECTIVE_DESCS_KO,
        "momentum_labels":BattleManager.MOMENTUM_RANK_LABELS,"momentum_labels_ko":BattleManager.MOMENTUM_RANK_LABELS_KO,
        "environment":BattleManager.ENV_BONUSES["verdan_market"],"enemies":pool,
        "witness_lines_en":BattleManager.WITNESS_LINES_EN,"witness_lines_ko":BattleManager.WITNESS_LINES_KO,
        "messages":{"en":{},"ko":{}},"recurring_enemy_art":BattleManager.RECURRING_ENEMY_ART,
        "enemy_art":[],"background":map._encounter_data.bg_image}
    # Execute the original rich-presence formatter; never duplicate its authored
    # chapter names in the native model. Restore the collection probe input.
    catalog.exploration_presence={}
    var presence_probe_chapter=GameManager.current_chapter
    for chapter in range(1,11):
        GameManager.current_chapter=chapter
        catalog.exploration_presence[str(chapter)]=GameManager._get_exploration_presence()
    GameManager.current_chapter=presence_probe_chapter
    for enemy_data in pool:catalog.enemy_art.append(BattleManager.resolve_enemy_image_by_name(enemy_data.name))
    for locale in ["en","ko"]:
        GameManager.current_locale=locale
        for pair in JSON.parse_string(FileAccess.get_file_as_string("res://message_pairs.json")):
            catalog.messages[locale][pair[0]]=BattleManager._bl(pair[0],pair[1])
    map.free()
    # Catalogue-only synthetic probe enables every original pool condition. It
    # does not describe a playable Verdan enemy or a campaign state.
    GameManager.current_chapter=4
    GameManager.play_stats=GameManager.initial_play_stats.duplicate(true)
    BattleManager.sable_in_party=true
    var catalog_enemy=BattleManager.Enemy.new("catalog_probe",200,1,false)
    BattleManager._setup_tactical_objective(catalog_enemy)
    catalog.objectives=BattleManager._oracle_objective_pool.duplicate(true)
    # Preserve the source GameManager._ready connection ordering: total_battles is
    # incremented synchronously before BattleManager proceeds to objective ranking.
    BattleManager.battle_started.connect(func(_e):GameManager.add_stat("total_battles"))
    BattleManager.battle_started.connect(func(e):events.append({"event":"started","enemy":e.name}))
    BattleManager.battle_ended.connect(GameManager._on_battle_ended_stats)
    BattleManager.battle_ended.connect(func(value):events.append({"event":"ended","state":value}))
    BattleManager.player_turn_started.connect(func():events.append({"event":"player_turn"}))
    BattleManager.battle_log.connect(func(line):logs.append(line))
    BattleManager.tactical_objective_changed.connect(func(value):events.append({"event":"objective","value":value.duplicate(true)}))
    BattleManager.momentum_changed.connect(func(value,rank,label):events.append({"event":"momentum","value":value,"rank":rank,"label":label}))
    BattleManager.limit_changed.connect(func(value):events.append({"event":"limit","value":value}))
    BattleManager.break_changed.connect(func(value,cap):events.append({"event":"break","value":value,"max":cap}))
    GameManager.field_focus_changed.connect(func(value,cap):events.append({"event":"focus","value":value,"cap":cap}))
    for c in JSON.parse_string(FileAccess.get_file_as_string(args[0])):
        GameManager.story_flags=c.flags.duplicate(true)
        GameManager.current_locale=c.locale;GameManager.current_chapter=int(c.chapter)
        GameManager.current_state=GameManager.GameState.EXPLORATION
        GameManager.player_data={"name":"Arrel","elia_with_party":c.elia,"grains":int(c.grains),
            "hp":int(c.hp),"max_hp":int(c.max_hp),"field_focus":int(c.focus),"directive_streak":int(c.directive_streak),
            "items":{"potion":2},"recent_items":["potion"],"item_quick_slots":["potion","antidote","witness_ink"]}
        GameManager.play_stats=GameManager.initial_play_stats.duplicate(true)
        GameManager.play_stats.total_battles=int(c.total_battles)
        GameManager.play_stats.highest_momentum_rank=int(c.get("highest_momentum_rank",0))
        EliaDiary.skills={"remembered_strike":{"current_cooldown":3}} # explicit pre-existing cooldown fixture
        GameManager.requested_map=""
        MemoryManager.memories.clear();MemoryManager.burned_memories.clear();MemoryManager.burn_passives.clear()
        MemoryManager.anchor_passives.clear();MemoryManager._init_starting_memories()
        # Synthetic history-size fixtures test corruption thresholds, not a claimed
        # campaign sequence. No burn action or memory definition is invented.
        for i in range(int(c.burn_count)):MemoryManager.burned_memories.append(MemoryManager.memories[0])
        logs.clear();events.clear();GameManager.events.clear();OracleRng.reset(c.rng)
        var before={"player":GameManager.player_data.duplicate(true),"flags":GameManager.story_flags.duplicate(true),
            "total_battles":GameManager.play_stats.total_battles,"burn_count":MemoryManager.get_burn_count(),
            "play_stats":GameManager.play_stats.duplicate(true),"elia_skills":EliaDiary.skills.duplicate(true)}
        BattleManager.prepare_field_entry(c.entry_mode,int(c.entry_power))
        var p=pool[int(c.enemy_index)].duplicate(true)
        var enemy=BattleManager.Enemy.new(p.name,int(p.hp),int(p.atk),p.get("is_void",false))
        enemy.is_ambient_encounter=true;enemy.abilities=p.get("abilities",[]).duplicate()
        BattleManager.start_battle(enemy,scene)
        assert(BattleManager.state==BattleManager.BattleState.PLAYER_TURN)
        var initial=snapshot()
        if catalog.objectives.is_empty():catalog.objectives=BattleManager._oracle_objective_pool.duplicate(true)
        var rng_before_flee=OracleRng.calls.duplicate(true)
        BattleManager.player_flee()
        # Real source cleanup contains a 0.3s pacing timer. Wait until source
        # clears the enemy and requests the original field scene.
        var waited=0
        while BattleManager.current_enemy!=null and waited<120:
            await get_tree().process_frame
            waited+=1
        assert(BattleManager.current_enemy==null,"Ambient cleanup did not finish")
        assert(GameManager.current_state==GameManager.GameState.EXPLORATION)
        assert(OracleRng.index==OracleRng.tape.size(),"Unused random values")
        assert(rng_before_flee==OracleRng.calls,"Ambient flee consumed randomness")
        results.append({"id":c.id,"before":before,"initial":initial,"after_flee":snapshot(),"rng":OracleRng.calls.duplicate(true)})
    FileAccess.open(args[1],FileAccess.WRITE).store_string(JSON.stringify(results))
    FileAccess.open("res://catalog.json",FileAccess.WRITE).store_string(JSON.stringify(catalog))
    print("MEMORIA_NARRATIVE_ORACLE_PASS cases=%d" % results.size())
    get_tree().quit(0)
'''

def body(path,name):
    source=(base.ROOT/path).read_text(encoding='utf-8')
    match=re.search(r'^(?:static )?func '+re.escape(name)+r'\(',source,re.M)
    if not match:raise ValueError('Missing method '+name)
    lines=source[match.start():].splitlines(keepends=True)
    count=1
    while count<len(lines) and (not lines[count].strip() or lines[count][0].isspace()):count+=1
    return ''.join(lines[:count])
def constant(source,name):
    start=source.index('const '+name)
    lines=source[start:].splitlines(keepends=True);out=lines[0];i=1
    while i<len(lines) and (lines[i][:1].isspace() or lines[i].startswith(('}',']'))):out+=lines[i];i+=1
    return out

def battle_code():
    source=(base.ROOT/BATTLE).read_text(encoding='utf-8')
    # Preserve declarations/classes and exact selected methods; fail closed if any
    # action or victory-only helper is unexpectedly reached by this entry slice.
    pattern=re.compile(r'^func (\w+)\([^\n]*\n(?:[ \t][^\n]*\n|\n)*',re.M)
    def transform(m):
        name=m.group(1);original=m.group(0)
        if name in KEEP:
            if name=='_setup_tactical_objective':
                original=original.replace('\tvar seed_text :=','\t_oracle_objective_pool=pool.duplicate(true) # observation only\n\tvar seed_text :=')
            return original
        first=original.splitlines()[0];ret=first.split('->')[-1].strip().removesuffix(':') if '->' in first else 'void'
        value={'void':None,'bool':'false','String':'""','int':'0','float':'0.0','Dictionary':'{}','Array':'[]'}.get(ret,'null')
        return first+'\n\tassert(false,"Outside battle-entry oracle: '+name+'")\n'+('' if value is None else '\treturn '+value+'\n')+'\n'
    code=pattern.sub(transform,source)+'\nvar _oracle_objective_pool: Array=[]\n'
    return code

def setup(work):
    prior.setup(work)
    code=battle_code()
    # Only redirect random calls; preserve the original files separately for audit.
    for call in ['randf','randi_range','randf_range']:code=re.sub(r'(?<![.\w])'+call+r'\(', 'OracleRng.'+call+'(',code)
    (work/'battle.gd').write_text(code,encoding='utf-8')
    (work/'battle_original.gd.txt').write_bytes((base.ROOT/BATTLE).read_bytes())
    for src,dest in [(MOD,'encounter_modifiers.gd'),('scripts/utils/random_encounter.gd','random_encounter.gd')]:
        raw=(base.ROOT/src).read_text(encoding='utf-8');patched=raw
        for call in ['randf','randi_range','randf_range']:patched=re.sub(r'(?<![.\w])'+call+r'\(', 'OracleRng.'+call+'(',patched)
        (work/dest).write_text(patched,encoding='utf-8');(work/(dest+'.original.txt')).write_text(raw,encoding='utf-8')
    (work/'battle_map.gd').write_text('extends Node\nvar _encounter_data: RandomEncounter.EncounterData\n'+body(MAP,'_setup_random_encounters'),encoding='utf-8')
    (work/'rng.gd').write_text(RNG.replace('    ','\t'),encoding='utf-8')
    pairs=[]
    for name in sorted(KEEP):
        text=body(BATTLE,name)
        for match in re.finditer(r'_bl\(\s*("(?:[^"\\]|\\.)*")\s*,\s*("(?:[^"\\]|\\.)*")',text):
            pair=[json.loads(match.group(1)),json.loads(match.group(2))]
            if pair not in pairs:pairs.append(pair)
    (work/'message_pairs.json').write_bytes(base.canonical(pairs))
    gm=(work/'gm.gd').read_text(encoding='utf-8')
    source=(base.ROOT/GM).read_text(encoding='utf-8')
    for name in ['add_stat','max_stat','change_state']:
        original=body(GM,name)
        if re.search(r'^func '+name+r'\(',gm,re.M):gm=re.sub(r'(?ms)^func '+name+r'\(.*?(?=^func |\Z)',lambda _:original,gm,count=1)
        else:gm+='\n'+original
    stats=source[source.index('var play_stats:'):source.index('\n}',source.index('var play_stats:'))+2]
    gm+='\nsignal state_changed(new_state: GameState)\n'+constant(source,'RICH_PRESENCE_CHAPTERS')+'\n'+body(GM,'_get_exploration_presence')+'\nfunc update_rich_presence(status: String) -> void:\n\tevents.append("request:presence:"+status)\n'
    gm+='\n'+stats.replace('var play_stats:','var initial_play_stats:')+'\nvar boss_rush_mode: bool=false\nconst FIELD_FOCUS_MAX=3\nsignal field_focus_changed(value: int,cap: int)\n'
    for name in ['get_field_focus','consume_field_focus','add_field_focus','get_directive_streak','set_directive_streak','get_ng_scale','_on_battle_ended_stats']:
        gm+='\n'+body(GM,name)
    (work/'gm.gd').write_text(gm,encoding='utf-8')
    sinks={
        'options.gd':'extends Node\nvar settings={"difficulty":1,"battle_speed":0}\n',
        'input.gd':'extends Node\nfunc vibrate(id: String) -> void:\n\tGameManager.events.append("request:vibrate:"+id)\n',
        'elia.gd':'extends Node\nvar skills: Dictionary={}\n'+body('scripts/ui/elia_diary.gd','reset_cooldowns'),
        'codex.gd':'extends Node\nvar enemy_entries={}\nfunc _save_data() -> void:\n\tGameManager.events.append("request:codex:save")\n',
    }
    for name,content in sinks.items():(work/name).write_text(content,encoding='utf-8')
    with (work/'audio_manager_stub.gd').open('a',encoding='utf-8') as f:f.write('\nfunc dramatic_silence(seconds: float) -> void:\n\tGameManager.events.append("request:audio:silence:%s" % seconds)\n')
    with (work/'transition.gd').open('a',encoding='utf-8') as f:f.write('\nfunc change_scene(path: String) -> void:\n\tassert(false,"Defeat outside battle-entry oracle")\nfunc change_scene_battle(path: String) -> void:\n\tGameManager.requested_map=path\n')
    with (work/'unused.gd').open('a',encoding='utf-8') as f:f.write('\nfunc check_grains() -> void:\n\tassert(false,"Victory outside battle-entry oracle")\n')
    project=(work/'project.godot').read_text(encoding='utf-8')
    project=project.replace('[rendering]','\n'.join(name+'="*res://'+file+'"' for name,file in [
        ('OracleRng','rng.gd'),('OptionsMenu','options.gd'),('InputManager','input.gd'),('EliaDiary','elia.gd'),('Codex','codex.gd'),('BattleManager','battle.gd')])+'\n[rendering]')
    (work/'project.godot').write_text(project,encoding='utf-8')
    # This exact source resolver is reached by Alley Rat; copy only its requested
    # source bytes so ResourceLoader.exists retains the real branch outcome.
    art='assets/cg/generated/battle_stage_v2/enemy_ash_hound_stage_v1.png'
    target=work/art;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(base.ROOT/art,target)

def native_catalog_payload(content):
    """Bound each adjacent wide literal below MSVC's per-token string limit."""
    text=content.decode('utf-8').strip()
    chunks=[text[start:start+512] for start in range(0,len(text),512)]
    return ('// Generated by export_battle_entry_oracle.py from executed source; do not hand edit.\n'
            'static const TCHAR* MemoriaBattleSourceJson =\n'+
            '\n'.join('    TEXT('+json.dumps(chunk,ensure_ascii=True)+')' for chunk in chunks)+';\n')

def run(a):
    base.BASE=DEST;base.inputs=inputs;base.ORACLE=RUN
    if not a.check:DEST.mkdir(parents=True,exist_ok=True)
    try:base.run(a,project_setup=setup)
    finally:
        report_path=a.evidence_dir/'oracle.json'
        if report_path.exists():
            report=json.loads(report_path.read_text(encoding='utf-8'))
            commands=report.get('commands',[])
            if commands:
                work=Path(commands[-1]['command'][3])
                with zipfile.ZipFile(a.evidence_dir/'executed_harness.zip','w',zipfile.ZIP_DEFLATED) as z:
                    z.write(Path(__file__),'export_battle_entry_oracle.py')
                    for p in work.rglob('*'):
                        if p.is_file() and (p.suffix in ['.gd','.txt','.json','.godot','.tscn'] or p.name=='project.godot') and '.godot' not in p.relative_to(work).parts:
                            z.write(p,p.relative_to(work))
                report['harness']='Exact source battle start/ambient flee/cleanup bodies and reachable entry helpers; RNG built-ins redirected to an asserted typed tape. Battle actions/victory helpers are fail-closed. Audio/UI/profile/travel endpoints observed. Synthetic history sizes exercise modifier bands, not authored memory burn sequences.'
                report['source_raw_hashes'].update({name:base.sha((base.ROOT/name).read_bytes()) for name in [BATTLE,MOD,MAP,GM,'scripts/utils/random_encounter.gd','scripts/ui/elia_diary.gd','assets/cg/generated/battle_stage_v2/enemy_ash_hound_stage_v1.png']})
                report['kept_battle_methods']=sorted(KEEP)
                report_path.write_bytes(base.canonical(report))
    report=json.loads((a.evidence_dir/'oracle.json').read_text(encoding='utf-8'));work=Path(report['commands'][-1]['command'][3])
    content=base.canonical(json.loads((work/'catalog.json').read_text(encoding='utf-8')))
    target=DEST/'source_catalog.v1.json'
    if a.check:assert target.read_bytes()==content,'Source catalog changed'
    else:target.write_bytes(content)
    # Generated source-authored content only; native model owns consumption.
    payload=native_catalog_payload(content)
    if a.check:assert TARGET.read_text(encoding='utf-8')==payload,'Generated native catalog changed'
    else:TARGET.parent.mkdir(parents=True,exist_ok=True);TARGET.write_text(payload,encoding='utf-8')
    print('MEMORIA_BATTLE_ENTRY_ORACLE_PASS cases='+str(len(inputs())))

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--godot',type=Path,required=True);p.add_argument('--evidence-dir',type=Path,required=True);p.add_argument('--check',action='store_true');run(p.parse_args())
