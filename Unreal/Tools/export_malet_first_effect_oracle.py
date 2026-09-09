"""Phase1J: exact source flag/callback/seed characterization; no UE world authority."""
from pathlib import Path
import argparse,json,re,subprocess
import export_malet_reward_oracle as prior
base=prior.base;g=prior.g
DEST=base.ROOT/'docs/unreal-migration/fixtures/malet_first_effect'
CASES=('fresh','knowledge_present','memory_present','memory_removed','knowledge_forgotten','actor_missing','already_true','repeated_reward','false_flag','case_sensitive','seed_guard_false','memory_restored')
def inputs():return [dict(id=n) for n in CASES]
RUN=r"""extends Node
var world_events: Array = []
const A = "npc.malet"
const F = "fact.bl07.route_request_received"
const M = "memory.malet.bl07_request_source"
func _ready() -> void:
    call_deferred("run")
func on_world(e: Dictionary) -> void:
    world_events.append(e.duplicate(true))
    GameManager.events.append("world:"+e.event_type+":"+e.target_id)
func snapshot(label: String) -> Dictionary:
    return {"label":label,"flags":GameManager.story_flags.duplicate(true),"save":GameManager.export_data(),"world":WorldState.export_data(),"events":GameManager.events.duplicate(),"world_events":world_events.duplicate(true),"items":GameManager.player_data.items.duplicate(true),"shop_open":MemoryShop.is_open}
func run() -> void:
    EventBus.world_event_committed.connect(on_world)
    var args = OS.get_cmdline_user_args()
    var cases: Array = JSON.parse_string(FileAccess.get_file_as_string(args[0]))
    var results: Array = []
    for c in cases:
        GameManager.story_flags.clear()
        GameManager.player_data = {"elia_with_party":true,"grains":0,"hp":100,"max_hp":100,"items":{},"recent_items":[]}
        GameManager.current_chapter = 1
        WorldState.reset_to_defaults()
        MemoryShop.is_open = false
        var owner = load("res://malet_route.gd").new()
        add_child(owner)
        if c.id == "knowledge_present": MemoryEngine.learn_fact(A,F)
        if c.id in ["memory_present","memory_removed","memory_restored"]:
            MemoryEngine.add_memory(A,M,{"fact_ids":[F],"source_actor_id":"player.arrel"})
        if c.id in ["memory_removed","memory_restored"]: MemoryEngine.remove_memory(A,M)
        if c.id == "memory_restored": MemoryEngine.restore_memory(A,M)
        if c.id == "knowledge_forgotten":
            MemoryEngine.learn_fact(A,F)
            MemoryEngine.forget_fact(A,F)
        # Deliberate corrupted-state fixture; import_data normalizes missing registry actors back in.
        if c.id == "actor_missing": WorldState._state.actors.erase(A)
        if c.id == "already_true": GameManager.set_flag("ch2_malet_done")
        if c.id in ["false_flag","seed_guard_false"]: GameManager.set_flag("ch2_malet_done",false)
        if c.id == "case_sensitive": GameManager.set_flag("CH2_MALET_DONE")
        GameManager.events.clear()
        world_events.clear()
        var states: Array = [snapshot("before")]
        if c.id == "seed_guard_false": owner._seed_malet_memory_world_state_if_needed()
        else: owner._on_reward_ended()
        states.append(snapshot("after"))
        if c.id == "repeated_reward":
            owner._on_reward_ended()
            states.append(snapshot("after_repeat"))
        results.append({"id":c.id,"states":states,"boundary_snapshots":owner.boundary_snapshots.duplicate(true)})
        owner.free()
    FileAccess.open(args[1],FileAccess.WRITE).store_string(JSON.stringify(results))
    print("MEMORIA_NARRATIVE_ORACLE_PASS cases=%d" % results.size())
    get_tree().quit(0)
"""
def setup(work):
    prior.setup(work)
    gm=(work/'gm.gd').read_text(encoding='utf-8')
    for name in ('set_flag','get_flag'):
        body=g.source_method('scripts/core/game_manager.gd',name)
        if name=='set_flag':body=g.instrument(body,{'story_flags[flag_name] = value':['events.append("flag:"+flag_name)']})
        gm=re.sub(r'(?ms)^func '+name+r'\(.*?(?=^func |\Z)',lambda _:body,gm,count=1)
    gm+='\nvar ng_plus_cycle: int = 0\nvar equipped: Dictionary = {}\nvar upgrade_levels: Dictionary = {}\nvar seen_endings: Array = []\nvar play_stats: Dictionary = {}\n'+g.source_method('scripts/core/game_manager.gd','export_data')
    (work/'gm.gd').write_text(gm,encoding='utf-8')
    mp=work/'malet_route.gd';s=mp.read_text(encoding='utf-8')
    reward=g.instrument(g.source_method(prior.MAP,'_on_reward_ended'),{'GameManager.set_flag("ch2_malet_done")':['boundary_snapshots.append({"point":"after_flag_before_seed","flags":GameManager.story_flags.duplicate(true),"world":WorldState.export_data(),"events":GameManager.events.duplicate()})']},'callback:reward:enter')
    seed=g.instrument(g.source_method(prior.MAP,'_seed_malet_memory_world_state_if_needed'),entry='world:seed:enter')
    for name,body in [('_on_reward_ended',reward),('_seed_malet_memory_world_state_if_needed',seed)]:
        s=re.sub(r'(?ms)^func '+name+r'\(.*?(?=^func |\Z)',lambda _:body,s,count=1)
    s=s.replace('var malet_npc: Node','var boundary_snapshots: Array = []\nvar malet_npc: Node')
    mp.write_text(s,encoding='utf-8')
    # Super-calling observers retain byte-identical WorldState implementation.
    observed='extends "res://scripts/systems/world_state.gd"\n'
    for name,params,args,ret in [('get_actor_state','actor_id: String','actor_id','Dictionary'),('get_memory_record','actor_id: String, memory_id: String','actor_id,memory_id','Dictionary'),('_store_knowledge_value','actor_id: String, fact_id: String, value: bool','actor_id,fact_id,value','int'),('_store_memory_record','actor_id: String, memory_id: String, record: Dictionary','actor_id,memory_id,record','int')]:
        observed+=f'func {name}({params}) -> {ret}:\n\tGameManager.events.append("lookup_or_store:{name}")\n\treturn super.{name}({args})\n'
    (work/'world_observer.gd').write_text(observed,encoding='utf-8')
    pr=work/'project.godot';pr.write_text(pr.read_text(encoding='utf-8').replace('WorldState="*res://scripts/systems/world_state.gd"','WorldState="*res://world_observer.gd"'),encoding='utf-8')
def run(a):
    base.BASE=DEST;base.inputs=inputs;base.ORACLE=RUN
    if not a.check:DEST.mkdir(parents=True,exist_ok=True)
    base.run(a,project_setup=setup,expected_execute_errors=(("ERROR: Signal 'shop_closed' is already connected to given callable 'Node(malet_route.gd)::_on_shop_closed' in that object.",1),))
    paths=[prior.MAP,'scripts/core/game_manager.gd',*prior.EXTRA]
    originals={n:g.source_method(prior.MAP,n) for n in ('_on_reward_ended','_seed_malet_memory_world_state_if_needed','_on_deal_ended')}
    originals.update({n:g.source_method('scripts/core/game_manager.gd',n) for n in ('set_flag','get_flag','export_data','import_data')})
    revision=subprocess.check_output(['git','-C',str(base.ROOT),'log','-1','--format=%H','--',*paths],text=True).strip()
    hashes={}
    for p in paths:
        raw=(base.ROOT/p).read_bytes();normalized=base.source_bytes(raw)
        assert normalized==base.source_bytes(subprocess.check_output(['git','-C',str(base.ROOT),'show',revision+':'+p]))
        hashes[p]={'raw_sha256':base.sha(raw),'lf_sha256':base.sha(normalized)}
    (a.evidence_dir/'source_attestation.json').write_bytes(base.canonical(dict(revision=revision,hashes=hashes,original_methods=originals,observation='Exact source flag/callback/seed methods with removable observer lines; byte-identical world engine via super-calling lookup/store observers. Presentation/save/profile endpoints inert.')))
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--godot',type=Path,required=True);p.add_argument('--evidence-dir',type=Path,required=True);p.add_argument('--check',action='store_true');run(p.parse_args())
