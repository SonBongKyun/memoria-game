"""Phase1M: exact source grants, raw import/query and ordered toast enqueue observations."""
from pathlib import Path
import argparse,subprocess
import export_malet_potion_oracle as l
base=l.base;g=l.g
DEST=base.ROOT/'docs/unreal-migration/fixtures/malet_antidote'
def inputs():
    specs=[('canonical',{'potion':2},['potion'],'antidote',1,1),('absent',{},[],'antidote',1,1),('existing',{'potion':9,'antidote':7},['potion'],'antidote',1,1),('already_recent',{},['potion','antidote','firebomb'],'antidote',1,1),('five',{},['potion','hi_potion','firebomb','smoke_bomb','witness_ink'],'antidote',1,1),('normalize',{},['ANTIDOTE','potion','potion','bad','antidote','firebomb'],'antidote',1,1),('repeat',{},[],'antidote',1,2),('invalid',{},['bad','potion','potion','ANTIDOTE','antidote'],'bad',1,1),('case_sensitive',{'ANTIDOTE':7},[],'ANTIDOTE',1,1),('zero',{},[],'antidote',0,1),('negative',{'antidote':1},[],'antidote',-2,1),('ko',{'potion':2},['potion'],'antidote',1,1),('sequence',{},[],'antidote',1,1),('replacement_potion',{},[],'antidote',1,1),('replacement_antidote',{},[],'antidote',1,1),('presentation_absent',{},[],'antidote',1,1)]
    return [dict(id=n,items=i,recent=r,item=item,amount=a,repeats=t) for n,i,r,item,a,t in specs]
RUN=r'''extends Node
func _ready() -> void:
    call_deferred("run")
func changed(item: String) -> void:
    GameManager.observe("inventory_changed",{"item_id":item})
    if item == GameManager.replace_item:
        GameManager.player_data = {"items":{},"recent_items":[]}
        GameManager.observe("replacement",{})
func run() -> void:
    GameManager.inventory_changed.connect(changed)
    var args = OS.get_cmdline_user_args()
    var cases: Array = JSON.parse_string(FileAccess.get_file_as_string(args[0]))
    var out: Array = []
    for c in cases:
        GameManager.player_data = {"items":c.items.duplicate(true),"recent_items":c.recent.duplicate()}
        GameManager.current_locale = "ko" if c.id == "ko" else "en"
        GameManager.replace_item = "potion" if c.id == "replacement_potion" else ("antidote" if c.id == "replacement_antidote" else "")
        NotificationToast.available = c.id != "presentation_absent"
        NotificationToast._queue.clear()
        NotificationToast.enqueues.clear()
        NotificationToast.deliveries.clear()
        GameManager.observations.clear()
        var before = GameManager.item_snapshot()
        var before_query = GameManager.get_recent_items()
        var before_after_query = GameManager.item_snapshot()
        if c.id in ["sequence","replacement_potion","replacement_antidote"]: GameManager.add_item("potion",2)
        for i in c.repeats: GameManager.add_item(c.item,int(c.amount))
        var after = GameManager.item_snapshot()
        var saved = JSON.parse_string(JSON.stringify(GameManager.export_data()))
        GameManager.player_data = {"items":{},"recent_items":[]}
        GameManager.import_data(saved)
        var restored = GameManager.item_snapshot()
        var restored_query = GameManager.get_recent_items()
        out.append({"id":c.id,"before":before,"before_query":before_query,"before_after_query":before_after_query,"after":after,"steps":GameManager.observations.duplicate(true),"restored":restored,"restored_query":restored_query,"restored_after_query":GameManager.item_snapshot(),"enqueues":NotificationToast.enqueues.duplicate(true),"deliveries":NotificationToast.deliveries.duplicate(true),"item_record":GameManager.ITEMS.antidote})
    FileAccess.open(args[1],FileAccess.WRITE).store_string(JSON.stringify(out))
    print("MEMORIA_NARRATIVE_ORACLE_PASS cases=%d" % out.size())
    get_tree().quit(0)
'''
def setup(work):
    l.setup(work)
    with (work/'gm.gd').open('a',encoding='utf-8',newline='\n') as f:f.write('\nvar replace_item := ""\n')
    toast='extends Node\nenum ToastType { INFO, SUCCESS, WARNING }\nvar _queue: Array[Dictionary] = []\nvar _showing := false\nvar available := true\nvar enqueues: Array = []\nvar deliveries: Array = []\n'
    original=g.source_method('scripts/ui/notification_toast.gd','show_toast')
    toast+=g.instrument(original,{'_queue.append({"text": text, "type": type})':['enqueues.append(_queue.back().duplicate(true))','GameManager.observe("toast",_queue.back())']})
    toast+='func _process_queue() -> void:\n\tif available: deliveries.append(_queue.pop_front()) # inert visual sink, exact enqueue runs above\n'
    (work/'notification_stub.gd').write_bytes(toast.encode())
def run(a):
    base.BASE=DEST;base.inputs=inputs;base.ORACLE=RUN
    if not a.check:DEST.mkdir(parents=True,exist_ok=True)
    base.run(a,project_setup=setup)
    paths=['scripts/core/game_manager.gd','scripts/ui/notification_toast.gd','scripts/systems/save_manager.gd','scenes/maps/verdan_market.gd']
    methods={p:{n:g.source_method(p,n) for n in ns} for p,ns in [(paths[0],['add_item','get_recent_items','_record_recent_item','export_data','import_data','localized_runtime_text']),(paths[1],['show_toast']),(paths[3],['_on_reward_ended'])]}
    (a.evidence_dir/'source_attestation.json').write_bytes(base.canonical({'hashes':{p:base.sha((base.ROOT/p).read_bytes()) for p in paths},'lf_hashes':{p:base.sha((base.ROOT/p).read_bytes().replace(b'\r\n',b'\n')) for p in paths},'methods':methods,'revision':subprocess.check_output(['git','log','-1','--format=%H','--',*paths],cwd=base.ROOT,text=True).strip(),'observers':'Exact source functions with removable read-only hooks; actual queue append observed before inert visual sink. No firebomb or full callback invocation.'}))
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--godot',type=Path,required=True);p.add_argument('--evidence-dir',type=Path,required=True);p.add_argument('--check',action='store_true');run(p.parse_args())
