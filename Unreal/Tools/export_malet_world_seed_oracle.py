"""Phase1K seed-only oracle: real isolated source engines, no reward items."""
from pathlib import Path
import argparse,json,subprocess
import export_malet_first_effect_oracle as j
base=j.base;g=j.g
DEST=base.ROOT/'docs/unreal-migration/fixtures/malet_world_seed'
CASES=('fresh','knowledge_only','memory_only','both','removed','forgotten','actor_missing','repeat','already_true','save_restore','restored','flag_false','case_sensitive')
def inputs():return [dict(id=n) for n in CASES]
RUN=r'''extends Node
var events: Array = []
var steps: Array = []
const A = "npc.malet"
const F = "fact.bl07.route_request_received"
const M = "memory.malet.bl07_request_source"
func _ready() -> void:
    call_deferred("run")
func on_world(e: Dictionary) -> void:
    events.append(e.duplicate(true))
    steps.append({"event":e.duplicate(true),"world":WorldState.export_data()})
func run() -> void:
    EventBus.world_event_committed.connect(on_world)
    var args = OS.get_cmdline_user_args()
    var cases: Array = JSON.parse_string(FileAccess.get_file_as_string(args[0]))
    var results: Array = []
    for c in cases:
        GameManager.story_flags.clear()
        GameManager.set_flag("ch2_malet_done",c.id not in ["flag_false","case_sensitive"])
        if c.id == "case_sensitive": GameManager.set_flag("CH2_MALET_DONE")
        WorldState.reset_to_defaults()
        var owner = load("res://malet_route.gd").new()
        add_child(owner)
        if c.id in ["knowledge_only","both","forgotten"]: MemoryEngine.learn_fact(A,F)
        if c.id in ["memory_only","both","removed","restored"]:
            MemoryEngine.add_memory(A,M,{"fact_ids":[F],"source_actor_id":"player.arrel","content":{"retained":"source fixture"}})
        if c.id in ["removed","restored"]: MemoryEngine.remove_memory(A,M)
        if c.id == "restored": MemoryEngine.restore_memory(A,M)
        if c.id == "forgotten": MemoryEngine.forget_fact(A,F)
        if c.id == "actor_missing": WorldState._state.actors.erase(A)
        if c.id == "save_restore":
            owner._seed_malet_memory_world_state_if_needed()
            var saved = WorldState.export_data()
            WorldState.reset_to_defaults()
            assert(WorldState.import_data(JSON.parse_string(JSON.stringify(saved))))
        events.clear()
        steps.clear()
        var before = WorldState.export_data()
        owner._seed_malet_memory_world_state_if_needed()
        var after = WorldState.export_data()
        var first_events = events.duplicate(true)
        var first_steps = steps.duplicate(true)
        events.clear()
        steps.clear()
        owner._seed_malet_memory_world_state_if_needed()
        results.append({"id":c.id,"done":GameManager.get_flag("ch2_malet_done"),"before":before,"after":after,"events":first_events,"steps":first_steps,"after_repeat":WorldState.export_data(),"repeat_events":events.duplicate(true)})
        owner.free()
    FileAccess.open(args[1],FileAccess.WRITE).store_string(JSON.stringify(results))
    print("MEMORIA_NARRATIVE_ORACLE_PASS cases=%d" % results.size())
    get_tree().quit(0)
'''
def run(a):
    base.BASE=DEST;base.inputs=inputs;base.ORACLE=RUN
    if not a.check:DEST.mkdir(parents=True,exist_ok=True)
    base.run(a,project_setup=j.setup)
    paths=[j.prior.MAP,'scripts/core/game_manager.gd',*j.prior.EXTRA]
    revision=subprocess.check_output(['git','log','-1','--format=%H','--',*paths],cwd=base.ROOT,text=True).strip()
    hashes={}
    for p in paths:
        raw=(base.ROOT/p).read_bytes()
        assert base.source_bytes(raw)==base.source_bytes(subprocess.check_output(['git','show',revision+':'+p],cwd=base.ROOT))
        hashes[p]={'raw_sha256':base.sha(raw),'lf_sha256':base.sha(base.source_bytes(raw))}
    originals={n:g.source_method(j.prior.MAP,n) for n in ('_on_reward_ended','_seed_malet_memory_world_state_if_needed')}
    (a.evidence_dir/'source_attestation.json').write_bytes(base.canonical(dict(revision=revision,hashes=hashes,original_methods=originals,observation='Seed-only execution; four verbatim engines via prior isolated harness; WorldState super observers only. No item/shop callback invoked. Actual export/import JSON roundtrip.')))
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--godot',type=Path,required=True);p.add_argument('--evidence-dir',type=Path,required=True);p.add_argument('--check',action='store_true');run(p.parse_args())
