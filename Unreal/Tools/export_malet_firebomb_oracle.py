"""Phase1N: exact source grants, raw import/query and ordered toast enqueue observations."""
from pathlib import Path
import argparse,subprocess
import export_malet_antidote_oracle as m
l=m.l
base=l.base;g=l.g
DEST=base.ROOT/'docs/unreal-migration/fixtures/malet_firebomb'
def inputs():
    result=m.inputs()
    for c in result:
        c['item']=c['item'].replace('antidote','firebomb').replace('ANTIDOTE','FIREBOMB')
        c['items']={k.replace('antidote','firebomb').replace('ANTIDOTE','FIREBOMB'):v for k,v in c['items'].items()}
        c['recent']=[v.replace('antidote','firebomb').replace('ANTIDOTE','FIREBOMB') for v in c['recent']]
        if c['id'] in ('canonical','ko'):c.update(items={'potion':2,'antidote':1},recent=['antidote','potion'])
        if c['id']=='existing':c['items']['antidote']=6
        if c['id']=='five':c['recent']=['potion','antidote','hi_potion','smoke_bomb','witness_ink']
    result.append(dict(id='replacement_firebomb',items={},recent=[],item='firebomb',amount=1,repeats=1))
    return result
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
        GameManager.replace_item = "potion" if c.id == "replacement_potion" else ("antidote" if c.id == "replacement_antidote" else ("firebomb" if c.id == "replacement_firebomb" else ""))
        NotificationToast.available = c.id != "presentation_absent"
        NotificationToast._queue.clear()
        NotificationToast.enqueues.clear()
        NotificationToast.deliveries.clear()
        GameManager.observations.clear()
        var before = GameManager.item_snapshot()
        var before_query = GameManager.get_recent_items()
        var before_after_query = GameManager.item_snapshot()
        if c.id in ["sequence","replacement_potion","replacement_antidote","replacement_firebomb"]:
            GameManager.add_item("potion",2)
            GameManager.add_item("antidote",1)
        for i in c.repeats: GameManager.add_item(c.item,int(c.amount))
        var after = GameManager.item_snapshot()
        var saved = JSON.parse_string(JSON.stringify(GameManager.export_data()))
        GameManager.player_data = {"items":{},"recent_items":[]}
        GameManager.import_data(saved)
        var restored = GameManager.item_snapshot()
        var restored_query = GameManager.get_recent_items()
        out.append({"id":c.id,"before":before,"before_query":before_query,"before_after_query":before_after_query,"after":after,"steps":GameManager.observations.duplicate(true),"restored":restored,"restored_query":restored_query,"restored_after_query":GameManager.item_snapshot(),"enqueues":NotificationToast.enqueues.duplicate(true),"deliveries":NotificationToast.deliveries.duplicate(true),"item_record":GameManager.ITEMS.firebomb})
    FileAccess.open(args[1],FileAccess.WRITE).store_string(JSON.stringify(out))
    print("MEMORIA_NARRATIVE_ORACLE_PASS cases=%d" % out.size())
    get_tree().quit(0)
'''
def setup(work):
    m.setup(work)
    source=(base.ROOT/"scripts/ui/notification_toast.gd").read_text(encoding="utf-8")
    enum=next(line for line in source.splitlines() if line.startswith("enum ToastType "))
    p=work/"notification_stub.gd";p.write_bytes(p.read_text(encoding="utf-8").replace("enum ToastType { INFO, SUCCESS, WARNING }",enum).encode())

def run(a):
    base.BASE=DEST;base.inputs=inputs;base.ORACLE=RUN
    if not a.check:DEST.mkdir(parents=True,exist_ok=True)
    base.run(a,project_setup=setup)
    paths=['scripts/core/game_manager.gd','scripts/ui/notification_toast.gd','scripts/systems/save_manager.gd','scenes/maps/verdan_market.gd','scripts/ui/memory_shop.gd']
    methods={p:{n:g.source_method(p,n) for n in ns} for p,ns in [(paths[0],['add_item','get_recent_items','_record_recent_item','export_data','import_data','localized_runtime_text']),(paths[1],['show_toast']),(paths[3],['_on_reward_ended','_open_malet_shop']),(paths[4],['open_shop'])]}
    import json
    previous=json.loads((base.ROOT/'docs/unreal-migration/evidence/phase1m/source01/source_attestation.json').read_text(encoding='utf-8'))
    files={p:dict(revision=subprocess.check_output(['git','log','-1','--format=%H','--',p],cwd=base.ROOT,text=True).strip(),raw_sha256=base.sha((base.ROOT/p).read_bytes()),lf_sha256=base.sha((base.ROOT/p).read_bytes().replace(b'\r\n',b'\n')),phase1m_raw_sha256=previous['hashes'].get(p)) for p in paths}
    for v in files.values():v['matches_phase1m']=None if v['phase1m_raw_sha256'] is None else v['raw_sha256']==v['phase1m_raw_sha256']
    (a.evidence_dir/'source_attestation.json').write_bytes(base.canonical(dict(files=files,methods=methods,observers='Exact source grants and actual queue append; inert visual sink. Shop functions read and attested only, never invoked. First local prohibited operation is stock construction; first external shop mutation is is_open = true after its guard.')))
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--godot',type=Path,required=True);p.add_argument('--evidence-dir',type=Path,required=True);p.add_argument('--check',action='store_true');run(p.parse_args())
