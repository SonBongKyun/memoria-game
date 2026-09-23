"""Execute original memory buy/sell/close handlers in an isolated source harness."""
from pathlib import Path
import argparse,json,zipfile
import export_malet_shop_oracle as prior
base=prior.base;g=prior.g
DEST=base.ROOT/'docs/unreal-migration/fixtures/shop_transactions'
def inputs():
    cases=[]
    for name,locale,grains,actions,oath in [
        ('sell_sense','en',0,[['sell','sense_forest_smell']],False),
        ('sell_daily','en',0,[['sell','daily_campfire_song']],False),
        ('sell_relation','en',0,[['sell','rel_hand_reaching']],False),
        ('sell_identity','en',0,[['sell','identity_first_sword']],False),
        ('oath_en','en',0,[['sell','sense_forest_smell']],True),
        ('oath_ko','ko',0,[['sell','sense_forest_smell']],True),
        ('buy_poor','en',7,[['buy','sense_copper_taste']],False),
        ('buy_exact','en',8,[['buy','sense_copper_taste']],False),
        ('buy_both','en',28,[['buy','sense_copper_taste'],['buy','daily_malet_deal']],False),
        ('buy_sell_ko','ko',28,[['buy','daily_malet_deal'],['sell','daily_malet_deal']],False),
        ('exchange','en',3,[['sell','sense_forest_smell'],['buy','sense_copper_taste']],False),
        ('close','en',0,[['close','']],False)]:
        cases.append(dict(id=name,locale=locale,grains=grains,actions=actions,oath=oath))
    return cases
RUN=r"""extends Node
func _ready() -> void:
    call_deferred("run")
func snapshot() -> Dictionary:
    var memories=[]
    for m in MemoryManager.memories:
        memories.append({"id":m.id,"burned":m.is_burned,"residue":m.is_residue,"faded":m.is_faded,"erosion":m.erosion,"connections":m.connections.duplicate()})
    var sold=[]
    for s in MemoryShop._shop_inventory:
        if s.get("sold",false):sold.append(s.id)
    var history=[]
    for m in MemoryManager.burned_memories:history.append(m.id)
    return {"grains":GameManager.player_data.grains,"chapter":GameManager.current_chapter,"flags":GameManager.story_flags.duplicate(),"open":MemoryShop.is_open,"sold":sold,"memories":memories,"history":history,"toasts":NotificationToast.messages.duplicate(),"events":GameManager.events.duplicate()}
func run() -> void:
    var args=OS.get_cmdline_user_args()
    var results=[]
    for c in JSON.parse_string(FileAccess.get_file_as_string(args[0])):
        GameManager.story_flags.clear()
        GameManager.current_locale=c.locale
        GameManager.current_chapter=1
        GameManager.player_data={"elia_with_party":true,"grains":int(c.grains),"items":{},"recent_items":[]}
        MemoryManager.memories.clear();MemoryManager.burned_memories.clear();MemoryManager.active_loan={}
        MemoryManager._init_starting_memories()
        if c.oath:GameManager.set_flag("oath_ash_sworn")
        MemoryShop.is_open=false
        var owner=load("res://shop_route.gd").new()
        add_child(owner)
        owner._open_malet_shop()
        NotificationToast.messages.clear();GameManager.events.clear()
        var states=[]
        for action in c.actions:
            if action[0]=="close":
                MemoryShop.close_shop()
            else:
                MemoryShop._current_mode=action[0]
                MemoryShop._refresh_items()
                for row in MemoryShop.items.duplicate():
                    var id=row.memory.id if row.type=="sell" else row.data.id
                    if id==action[1]:
                        MemoryShop._selected_item=row
                        if action[0]=="sell":MemoryShop._execute_sell()
                        else:MemoryShop._execute_buy()
                        break
            states.append(snapshot())
        if c.id=="close":
            await get_tree().create_timer(1.65).timeout
            states.append(snapshot())
        results.append({"id":c.id,"states":states})
        owner.free()
    FileAccess.open(args[1],FileAccess.WRITE).store_string(JSON.stringify(results))
    print("MEMORIA_NARRATIVE_ORACLE_PASS cases=%d" % results.size())
    get_tree().quit(0)
"""
def setup(work):
    prior.setup(work)
    s=(work/'shop.gd').read_text(encoding='utf-8')
    start=s.index('func _refresh_items()');end=s.index('func _update_merchant_portrait',start)
    s=s[:start]+'''func _refresh_items() -> void:
    items.clear()
    _selected_item={}
    if _current_mode=="sell":_populate_sell_list()
    else:_populate_buy_list()
'''+s[end:]
    start=s.index('func _add_item_button');end=s.index('func _exit_tree',start)
    s=s[:start]+'''func _add_item_button(_title: String,_color: Color,_price: int,item: Dictionary) -> void:
    items.append(item)
'''+s[end:]
    s=s.replace('signal shop_closed()','signal shop_closed()\nsignal grains_changed(value: int)\nvar items: Array=[]')
    s+='\n'.join(g.source_method(prior.SHOP,n) for n in ['_populate_buy_list','_execute_sell','_execute_buy','close_shop','_hide_ui'])
    (work/'shop.gd').write_text(s,encoding='utf-8')
    (work/'shop_route.gd').write_text('extends Node\n'+g.source_method('scenes/maps/verdan_market.gd','_open_malet_shop')+g.source_method('scenes/maps/verdan_market.gd','_on_shop_closed'),encoding='utf-8')
    (work/'notification_stub.gd').write_text('extends Node\nenum ToastType { INFO,SUCCESS,WARNING }\nvar messages: Array=[]\nfunc show_toast(text: String,type: int=0) -> void:\n    messages.append({"text":text,"type":type})\n',encoding='utf-8')
    with (work/'unused.gd').open('a',encoding='utf-8') as f:
        f.write(g.source_method('scripts/systems/save_manager.gd','autosave_on_chapter_transition')+'\nfunc autosave(reason: String) -> void:\n    GameManager.events.append("autosave:"+reason)\nfunc record_chapter_complete(c: int) -> void:\n    GameManager.events.append("achievement:chapter:%d" % c)\nfunc unlock(id: String) -> void:\n    GameManager.events.append("achievement:unlock:"+id)\n')
    for name in ['shop.gd','unused.gd']:
        q=work/name;q.write_text(q.read_text(encoding='utf-8').replace('    ','\t'),encoding='utf-8')
    (work/'executed_shop.gd').write_bytes((work/'shop.gd').read_bytes())
    (work/'executed_shop_route.gd').write_bytes((work/'shop_route.gd').read_bytes())
def run(a):
    base.BASE=DEST;base.inputs=inputs;base.ORACLE=RUN
    if not a.check:DEST.mkdir(parents=True,exist_ok=True)
    base.run(a,project_setup=setup)
    report=json.loads((a.evidence_dir/'oracle.json').read_text(encoding='utf-8'))
    report['harness']='Exact original memory shop handlers and Malet close callback; UI endpoints collect rows. Real MemoryManager/JourneyOath; audio, toast, save, achievement and map endpoints are observed sinks, no real save/travel.'
    report['source_raw_hashes'].update({name:base.sha((base.ROOT/name).read_bytes()) for name in [prior.SHOP,'scenes/maps/verdan_market.gd','scripts/systems/save_manager.gd']})
    (a.evidence_dir/'oracle.json').write_bytes(base.canonical(report))
    work=Path(report['commands'][-1]['command'][3])
    with zipfile.ZipFile(a.evidence_dir/'executed_harness.zip','w',zipfile.ZIP_DEFLATED) as z:
        for p in work.glob('*.gd'):z.write(p,p.name)
        for name in ['project.godot','inputs.json','outputs.json']:z.write(work/name,name)
    data=json.loads((DEST/'contract_expected.v1.json').read_text(encoding='utf-8'))
    def row(id):return next(x for x in data if x['id']==id)['states'][0]
    def t(s):return 'TEXT('+json.dumps(s,ensure_ascii=True)+')'
    lines=['// Generated from actual Godot transaction notifications, not manuscript additions.']
    for case,suffix in [('oath_en','En'),('oath_ko','Ko')]:
        toasts=row(case)['toasts']
        lines.append('static const TCHAR* SourceAshBroken'+suffix+' = '+t(next(x['text'] for x in toasts if x['type']==2 and ('Oath' in x['text'] or '맹세' in x['text'])))+';')
        sold=toasts[-1]['text'];before,rest=sold.split(': ',1);name=rest.rsplit(' (+',1)[0]
        lines.append('static const TCHAR* SourceSold'+suffix+' = '+t(sold.replace(name,'%s').replace('+5 G','+%lld G'))+';')
    buy=row('buy_exact')['toasts'][-1]['text']
    lines.append('static const TCHAR* SourceBought = '+t(buy.replace('The Taste of Copper','%s').replace('-8 G','-%lld G'))+';')
    target=base.ROOT/'Unreal/Memoria/Source/Memoria/Private/Shop/MemoriaShopTransactionText.inl'
    content=('\n'.join(lines)+'\n').encode()
    if a.check:assert target.read_bytes()==content
    else:target.write_bytes(content)
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--godot',type=Path,required=True);p.add_argument('--evidence-dir',type=Path,required=True);p.add_argument('--check',action='store_true');run(p.parse_args())

