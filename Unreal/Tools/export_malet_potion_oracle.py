"""Phase1L: execute exact source add_item/recent/save methods with inert toast display."""
from pathlib import Path
import argparse,json,re,subprocess
import export_malet_first_effect_oracle as j
base=j.base;g=j.g
DEST=base.ROOT/'docs/unreal-migration/fixtures/malet_potion'
def inputs():
    specs=[('absent',{},[],'potion',2,1),('existing1',{'potion':1},[],'potion',2,1),('existing9',{'potion':9,'antidote':7},[],'potion',2,1),('already_recent',{},['antidote','potion','firebomb'],'potion',2,1),('unrelated',{},['antidote'],'potion',2,1),('multiple',{},['hi_potion','antidote','firebomb','smoke_bomb','witness_ink','root_balm'],'potion',2,1),('normalize',{},['POTION','antidote','antidote','potion','bad','firebomb'],'potion',2,1),('repeat',{},[],'potion',2,2),('invalid',{},['antidote'],'bad',2,1),('case_sensitive',{'POTION':7},[],'POTION',2,1),('zero',{},[],'potion',0,1),('negative',{'potion':1},[],'potion',-2,1),('save_restore',{},['antidote'],'potion',2,1),('ko',{},[],'potion',2,1),('replacement_signal',{},[],'potion',2,1),('presentation_absent',{},[],'potion',2,1)]
    return [dict(id=n,items=i,recent=r,item=item,amount=a,repeats=t) for n,i,r,item,a,t in specs]
RUN=r'''extends Node
func _ready() -> void:
    call_deferred("run")
func changed(item: String) -> void:
    GameManager.observe("inventory_changed",{"item_id":item})
    if GameManager.replace_on_signal:
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
        GameManager.replace_on_signal = c.id == "replacement_signal"
        NotificationToast.available = c.id != "presentation_absent"
        GameManager.observations.clear()
        var before = GameManager.item_snapshot()
        for i in c.repeats: GameManager.add_item(c.item,int(c.amount))
        var after = GameManager.item_snapshot()
        var saved = JSON.parse_string(JSON.stringify(GameManager.export_data()))
        GameManager.player_data = {"items":{},"recent_items":[]}
        GameManager.import_data(saved)
        out.append({"id":c.id,"before":before,"after":after,"steps":GameManager.observations.duplicate(true),"restored":GameManager.item_snapshot()})
    FileAccess.open(args[1],FileAccess.WRITE).store_string(JSON.stringify(out))
    print("MEMORIA_NARRATIVE_ORACLE_PASS cases=%d" % out.size())
    get_tree().quit(0)
'''
def setup(work):
    j.setup(work)
    p=work/'gm.gd';s=p.read_text(encoding='utf-8')
    original=g.source_method('scripts/core/game_manager.gd','add_item')
    observed=g.instrument(original,{'player_data.items[item_id] = current + count':['observe("after_inventory_mutation",{})'],'_record_recent_item(item_id)':['observe("after_recent_items",{})']})
    s=re.sub(r'(?ms)^func add_item\(.*?(?=^func |\Z)',lambda _:observed,s,count=1)
    s+='\nconst ITEM_QUICK_SLOT_COUNT: int = 3\nvar replace_on_signal := false\nvar observations: Array = []\n'
    s+=g.source_method('scripts/core/game_manager.gd','get_item_quick_slots')+g.source_method('scripts/core/game_manager.gd','import_data')
    src=(base.ROOT/'scripts/core/game_manager.gd').read_text(encoding='utf-8')
    const=src[src.index('const RUNTIME_TEXT_KO:'):src.index('\n}',src.index('const RUNTIME_TEXT_KO:'))+2]
    s+='\n'+const+'\n'+g.source_method('scripts/core/game_manager.gd','localized_runtime_text')
    s+='\nfunc _save_seen_endings() -> void:\n\tpass\nfunc item_snapshot() -> Dictionary:\n\treturn {"items":player_data.items.duplicate(true),"recent":player_data.recent_items.duplicate()}\nfunc observe(point: String,payload: Dictionary) -> void:\n\tobservations.append({"point":point,"payload":payload.duplicate(true),"state":item_snapshot()})\n'
    p.write_text(s,encoding='utf-8')
    toast='extends Node\nenum ToastType { INFO, SUCCESS, WARNING }\nvar _queue: Array[Dictionary] = []\nvar _showing := false\nvar available := true\n'
    toast+=g.instrument(g.source_method('scripts/ui/notification_toast.gd','show_toast'),{'text = GameManager.localized_runtime_text(text)':['GameManager.observe("toast",{"text":text,"type":type})']})
    toast+='func _process_queue() -> void:\n\t_queue.clear() # inert display sink, availability never owns inventory\n'
    (work/'notification_stub.gd').write_text(toast,encoding='utf-8')
    (work/'executed_add_item.gd').write_text(observed,encoding='utf-8')
def run(a):
    base.BASE=DEST;base.inputs=inputs;base.ORACLE=RUN
    if not a.check:DEST.mkdir(parents=True,exist_ok=True)
    base.run(a,project_setup=setup)
    paths=['scripts/core/game_manager.gd','scripts/ui/notification_toast.gd','scripts/systems/save_manager.gd','scenes/maps/verdan_market.gd']
    methods={p:{n:g.source_method(p,n) for n in ns} for p,ns in [(paths[0],['add_item','get_recent_items','_record_recent_item','export_data','import_data','localized_runtime_text']),(paths[1],['show_toast']),(paths[3],['_on_reward_ended'])]}
    (a.evidence_dir/'source_attestation.json').write_bytes(base.canonical({'hashes':{p:base.sha((base.ROOT/p).read_bytes()) for p in paths},'methods':methods,'revision':subprocess.check_output(['git','log','-1','--format=%H','--',*paths],cwd=base.ROOT,text=True).strip(),'observers':'Only removable read-only observer lines; exact add/recent/import/export/localization and toast enqueue. Profile disk and toast display inert.'}))
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--godot',type=Path,required=True);p.add_argument('--evidence-dir',type=Path,required=True);p.add_argument('--check',action='store_true');run(p.parse_args())
