"""Phase1O: execute source shop entry and sell-list projection in an isolated Godot project.
UI endpoints collect the source's real rows; achievement/profile/tutorial handlers are deferred request sinks.
No original checkout writes and no purchase/sale/close callback execution.
"""
from pathlib import Path
import argparse,json,subprocess,re
import export_malet_reward_oracle as parent
base=parent.base;g=parent.g
DEST=base.ROOT/'docs/unreal-migration/fixtures/malet_shop'
SHOP='scripts/ui/memory_shop.gd'
def inputs():
    cases=[]
    for id in ['canonical','ko','empty','core_only','faded','collateral','all_burned','grains37','already_open','no_burns']:
        cases.append(dict(id=id,locale='ko' if id=='ko' else 'en',grains=37 if id=='grains37' else 0,burn=[] if id in ['no_burns','core_only','empty','all_burned'] else ['daily_market_food','identity_first_sword']))
    return cases
RUN=r'''extends Node
func _ready() -> void:
    call_deferred("run")
func snapshot() -> Dictionary:
    var rows: Array = []
    for row in MemoryShop.rows: rows.append(row.duplicate(true))
    return {"open":MemoryShop.is_open,"merchant":MemoryShop._merchant_name,"mode":MemoryShop._current_mode,"stock":MemoryShop._shop_inventory.duplicate(true),"rows":rows,"title":MemoryShop.shop_title.text,"grains_text":MemoryShop.grains_label.text,"caption":MemoryShop._resolve_merchant_caption(MemoryShop._merchant_name),"portrait":MemoryShop._resolve_merchant_portrait(MemoryShop._merchant_name),"detail_title":MemoryShop.detail_title.text,"selected":MemoryShop._selected_item.duplicate(true),"action_visible":MemoryShop.action_btn.visible,"ui_visible":MemoryShop.main_panel.visible,"game_state":GameManager.current_state,"events":GameManager.events.duplicate(),"close_connections":MemoryShop.shop_closed.get_connections().size()}
func run() -> void:
    var args=OS.get_cmdline_user_args()
    var out: Array=[]
    for c in JSON.parse_string(FileAccess.get_file_as_string(args[0])):
        GameManager.current_locale=c.locale
        GameManager.current_chapter=1
        GameManager.current_state=GameManager.GameState.DIALOGUE
        GameManager.player_data.grains=int(c.grains)
        MemoryManager.memories.clear();MemoryManager.burned_memories.clear();MemoryManager.active_loan={}
        MemoryManager._init_starting_memories()
        for id in c.burn:MemoryManager.burn_memory(id)
        if c.id=="empty":MemoryManager.memories.clear()
        if c.id=="core_only":
            var retained: Array[MemoryManager.Memory]=[]
            for m in MemoryManager.memories:
                if m.grade==MemoryManager.MemoryGrade.GRADE_1:retained.append(m)
            MemoryManager.memories=retained
        if c.id=="faded":MemoryManager.find_memory("sense_forest_smell").is_faded=true
        if c.id=="collateral":MemoryManager.active_loan={"memory_id":"sense_forest_smell"}
        if c.id=="all_burned":
            for m in MemoryManager.memories:m.is_burned=true
        MemoryShop.is_open=false;MemoryShop.rows.clear();MemoryShop._shop_inventory=[];MemoryShop.main_panel.visible=false
        GameManager.events.clear()
        var owner=load("res://shop_route.gd").new()
        owner._open_malet_shop()
        var first=snapshot()
        if c.id=="already_open":MemoryShop.open_shop("Other",[])
        out.append({"id":c.id,"first":first,"after":snapshot(),"sell_prices":MemoryShop.SELL_PRICES,"grade_names":MemoryShop.GRADE_NAMES})
        owner.free()
    FileAccess.open(args[1],FileAccess.WRITE).store_string(JSON.stringify(out))
    print("MEMORIA_NARRATIVE_ORACLE_PASS cases=%d" % out.size());get_tree().quit(0)
'''
def setup(work):
    parent.setup(work)
    source=(base.ROOT/SHOP).read_text(encoding='utf-8')
    names=['open_shop','_loc','_resolve_merchant_caption','_resolve_merchant_portrait','_update_grains','_populate_sell_list','_clear_detail','_show_ui']
    shop='extends Node\nsignal shop_closed()\nvar is_open=false\nvar _merchant_name=""\nvar _current_mode=""\nvar _shop_inventory: Array[Dictionary]=[]\nvar _selected_item={}\nvar rows: Array=[]\n'
    nodes={'shop_title':'Label','grains_label':'Label','detail_title':'Label','detail_grade':'Label','detail_desc':'Label','detail_price':'Label','detail_effect':'Label','action_btn':'Button','main_panel':'Control','backdrop':'Control','overlay':'Control','item_list':'VBoxContainer'}
    for name,typ in nodes.items():shop+=f'var {name}={typ}.new()\n'
    for key in ['SELL_PRICES','GRADE_NAMES']:shop+=next(s for s in source.splitlines() if s.startswith('const '+key))+'\n'
    start=source.index('const GRADE_COLORS');end=source.index('\n]',start)+2;shop+=source[start:end]+'\n'
    shop+='\n'.join(g.source_method(SHOP,n) for n in names)
    shop+='\nfunc _refresh_items() -> void:\n\trows.clear()\n\t_clear_detail()\n\t_populate_sell_list()\n'
    shop+='func _update_merchant_portrait(_name: String) -> void:\n\tpass # visual texture endpoint; source resolution is executed separately\n'
    shop+='func _add_item_button(title: String,_color: Color,price: int,item: Dictionary) -> void:\n\tvar m=item.memory\n\trows.append({"id":m.id,"title":title,"description":MemoryManager.localized_memory_description(m),"grade":m.grade,"price":price,"story_effect":MemoryManager.localized_memory_effect(m)})\n'
    shop+='func _exit_tree() -> void:\n'
    for n in nodes:shop+=f'\t{n}.free()\n'
    (work/'shop.gd').write_bytes(shop.encode())
    (work/'shop_route.gd').write_bytes(('extends Node\n'+g.source_method('scenes/maps/verdan_market.gd','_open_malet_shop')+'func _on_shop_closed() -> void:\n\tassert(false,"Shop close is outside this phase")\n').encode())
    # Preserve these outbound calls as observed requests; do not write profile or hint saves.
    (work/'unused.gd').write_bytes(b'extends Node\nfunc check_grains() -> void:\n\tGameManager.events.append("request:achievement:check_grains")\n')
    (work/'hints.gd').write_bytes(b'extends Node\nfunc show_hint(id: String) -> void:\n\tGameManager.events.append("request:tutorial:"+id)\n')
    (work/'audio_manager_stub.gd').write_bytes(b'extends Node\nfunc play_sfx(id: String) -> void:\n\tGameManager.events.append("request:audio:"+id)\nfunc play_bgm(_id: String) -> void:\n\tpass\n')
    (work/'ui_theme.gd').write_bytes(b'class_name UITheme\nconst TEXT_DIM=Color.GRAY\n')
    for n in ['shop.gd','shop_route.gd']: (work/('executed_'+n)).write_bytes((work/n).read_bytes())
def run(a):
    base.BASE=DEST;base.inputs=inputs;base.ORACLE=RUN
    if not a.check:DEST.mkdir(parents=True,exist_ok=True)
    base.run(a,project_setup=setup)
    report_path=a.evidence_dir/'oracle.json'
    report=json.loads(report_path.read_text(encoding='utf-8'))
    report['harness']='Exact source shop entry/stock, default sell-list rules and localization executed; UI collection endpoints and audio/achievement/tutorial request sinks. No transaction or close handler execution.'
    report_path.write_bytes(base.canonical(report))
    paths=[SHOP,'scenes/maps/verdan_market.gd','scripts/systems/memory_manager.gd','scripts/ui/tutorial_hints.gd','scripts/ui/achievement_manager.gd','assets/portraits/malet_face_neutral.png','assets/cg/generated/ui_memory_shop_backdrop_v2.png']
    files={p:{'sha256':base.sha((base.ROOT/p).read_bytes()),'revision':subprocess.check_output(['git','log','-1','--format=%H','--',p],cwd=base.ROOT,text=True).strip()} for p in paths}
    (a.evidence_dir/'source_attestation.json').write_bytes(base.canonical({'files':files,'scope':'Exact stock constructor, open_shop, default sell-list filtering/prices/localization, clear-detail and visible-state methods executed. Texture/tab widgets are presentation sinks. Achievement check, audio and first_shop calls are observed requests, not certified handler execution. Purchase/sale/close never invoked.'}))
    # Runtime content is mechanically generated from executed source output; never authored by hand in C++.
    data=json.loads((DEST/'contract_expected.v1.json').read_text(encoding='utf-8'))[0]
    def t(s):return 'TEXT('+json.dumps(s,ensure_ascii=True)+')'
    lines=['// Generated from executed Godot shop contract; regenerate with export_malet_shop_oracle.py.','static TArray<FMemoriaShopOffer> SourceMaletOffers()','{','    return {']
    for v in data['first']['stock']:lines.append('        {'+', '.join([t(v['id']),t(v['title']),t(v['description']),str(v['grade']),str(v['burn_power']),str(v['price']),t(v.get('story_effect','')),t(v.get('related_npc',''))])+'},')
    lines+=['    };','}','static constexpr int64 SourceSellPrices[] = {'+', '.join(str(data['sell_prices'][str(i)]) for i in range(5))+'};','static const TCHAR* SourceGradeNames[] = {'+', '.join(t(v) for v in data['grade_names'])+'};','']
    all_cases=json.loads((DEST/'contract_expected.v1.json').read_text(encoding='utf-8'))
    for id,suffix in [('canonical','En'),('ko','Ko')]:
        view=next(c['first'] for c in all_cases if c['id']==id)
        for key,name in [('title','Title'),('caption','Caption'),('detail_title','EmptyDetail'),('portrait','Portrait')]:lines.append('static const TCHAR* Source'+name+suffix+' = '+t(view[key])+';')
    target=base.ROOT/'Unreal/Memoria/Source/Memoria/Private/Shop/MemoriaShopSource.inl';content=('\n'.join(lines)+'\n').encode()
    if a.check:assert target.read_bytes()==content
    else:target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(content)
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--godot',type=Path,required=True);p.add_argument('--evidence-dir',type=Path,required=True);p.add_argument('--check',action='store_true');run(p.parse_args())
