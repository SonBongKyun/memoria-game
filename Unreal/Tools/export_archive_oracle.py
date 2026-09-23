"""Execute the source archive's cards, filters and readonly detail selection.
Artwork/rewrite and track summaries are excluded presentation endpoints.
No original files, gameplay mutations from archive, or profile saves are written.
"""
from pathlib import Path
import argparse,json,re,zipfile
import export_shop_transactions_oracle as prior
base=prior.base;g=prior.g
DEST=base.ROOT/'docs/unreal-migration/fixtures/archive'
UI='scripts/ui/memory_ui.gd'
TARGET=base.ROOT/'Unreal/Memoria/Source/Memoria/Private/Presentation/MemoriaArchiveView.cpp'

def inputs():
    return [dict(id=name,locale=locale,states=states,grade=grade,empty=empty)
            for name,locale,states,grade,empty in [
                ('initial_en','en',False,-1,False),('initial_ko','ko',False,-1,False),
                ('states_en','en',True,-1,False),('states_ko','ko',True,-1,False),
                ('grade5','en',True,0,False),('grade3','en',True,2,False),
                ('grade1','ko',True,4,False),('empty','en',False,-1,True)]]

RUN=r"""extends Node
func _ready() -> void:
    call_deferred("run")
func run() -> void:
    var args=OS.get_cmdline_user_args()
    var results=[]
    for c in JSON.parse_string(FileAccess.get_file_as_string(args[0])):
        GameManager.current_locale=c.locale
        GameManager.current_chapter=1
        GameManager.story_flags.clear()
        GameManager.player_data={"elia_with_party":true,"grains":0,"hp":100,"max_hp":100}
        MemoryManager.memories.clear();MemoryManager.burned_memories.clear();MemoryManager.active_loan={}
        MemoryManager._init_starting_memories()
        if c.states:
            MemoryManager.burn_memory("sense_forest_smell")
            MemoryManager.burn_memory("rel_hand_reaching")
            # Explicit synthetic UI states after real burns, never campaign facts.
            MemoryManager.find_memory("daily_campfire_song").is_faded=true
            MemoryManager.find_memory("identity_first_sword").erosion=3
        if c.empty:MemoryManager.memories.clear()
        var before=JSON.stringify(MemoryManager.export_data())
        var ui=load("res://archive.gd").new()
        add_child(ui)
        var title=ui._create_title_bar()
        var title_text=title.get_child(0).text
        title.free()
        var parent=HBoxContainer.new()
        ui.add_child(parent)
        ui._build_grade_tabs(parent)
        var filters=[]
        for child in ui.grade_tabs.get_children():
            if child is Button:filters.append(child.text)
        ui._on_grade_filter(int(c.grade))
        var rows=[]
        var row_index=0
        for memory in MemoryManager.memories:
            if int(c.grade)>=0 and memory.grade!=int(c.grade):continue
            var card=ui.card_list.get_child(row_index)
            card.pressed.emit()
            assert(ui.selected_memory==memory)
            var prefix="[%s] %s" % [ui.grade_short(memory.grade),MemoryManager.localized_memory_title(memory)]
            var suffix=card.text.substr(prefix.length())
            var state_label=ui.detail_status.text if suffix.is_empty() else suffix.trim_prefix(" [").trim_suffix("]")
            var color=ui.GRADE_COLORS[memory.grade]
            rows.append({"id":memory.id,"title":ui.detail_title.text,"description":ui.detail_desc.text,
                "effect":MemoryManager.localized_memory_effect(memory),"grade":memory.grade,
                "grade_label":ui.detail_grade.text,"state_label":state_label,
                "card_text":card.text,"detail_state":ui.detail_status.text,
                "burned":memory.is_burned,"residue":memory.is_residue,"faded":memory.is_faded,
                "erosion":memory.erosion,"erosion_ratio":MemoryManager.get_erosion_ratio(memory),
                "burn_power":memory.burn_power,"accent":[color.r,color.g,color.b,color.a]})
            row_index+=1
        ui._clear_detail()
        assert(before==JSON.stringify(MemoryManager.export_data()),"Archive reading mutated memory state")
        assert(not ui.synth_btn.visible and not ui.guard_btn.visible)
        results.append({"id":c.id,"title":title_text,"empty":ui.detail_title.text,"filters":filters,
            "counts":ui.summary.duplicate(),"count_text":ui.count_label.text,"rows":rows,
            "memory_unchanged":true,"read_only":ui._read_only,"mutation_controls_hidden":true})
        ui.free()
    FileAccess.open(args[1],FileAccess.WRITE).store_string(JSON.stringify(results))
    print("MEMORIA_NARRATIVE_ORACLE_PASS cases=%d" % results.size())
    get_tree().quit(0)
"""

def archive_method(name):
    source=(base.ROOT/UI).read_text(encoding='utf-8')
    match=re.search(r'^(?:static )?func '+re.escape(name)+r'\(',source,re.M)
    if not match:raise ValueError('Missing archive method '+name)
    lines=source[match.start():].splitlines(keepends=True)
    count=1
    while count<len(lines) and (not lines[count].strip() or lines[count][0].isspace()):
        count+=1
    return ''.join(lines[:count])

def setup(work):
    prior.setup(work)
    source=(base.ROOT/UI).read_text(encoding='utf-8')
    code='extends Node\nvar selected_grade_filter=-1\nvar selected_memory=null\nvar synthesis_mode=false\nvar synthesis_first=null\nvar _read_only=true\nvar grade_tabs: VBoxContainer\nvar summary: Array=[]\n'
    nodes={'card_list':'VBoxContainer','detail_title':'Label','detail_grade':'Label',
        'detail_desc':'RichTextLabel','detail_power':'Label','detail_npc':'Label',
        'detail_effect':'Label','detail_status':'Label','detail_rewrite':'Label',
        'count_label':'Label','synth_status_label':'Label','synth_btn':'Button',
        'guard_btn':'Button','guard_hint_label':'Label','detail_art':'TextureRect'}
    for name,typ in nodes.items():code+=f'var {name}={typ}.new()\n'
    for key in ['GRADE_NAMES','GRADE_NAMES_KO','GRADE_SHORT_KO']:
        code+=next(s for s in source.splitlines() if s.startswith('const '+key+' '))+'\n'
    start=source.index('const GRADE_COLORS');end=source.index('\n]',start)+2
    code+=source[start:end]+'\n'
    names=['_is_ko','grade_name','grade_short','_related_name','_create_title_bar',
           '_build_grade_tabs','_create_tab_button','_refresh_cards','_add_memory_card',
           '_show_detail','_clear_detail','_on_grade_filter','_refresh_guard_controls',
           '_count_same_grade','_exit_synthesis_mode']
    code+='\n'.join(archive_method(n) for n in names)
    code+='\nfunc _ready() -> void:\n'
    for name in nodes:code+=f'\tadd_child({name})\n'
    code+='func _update_carry_weight_label() -> void:\n\tpass # bounded list/filter/detail contract excludes carry summary\n'
    code+='func _update_archive_summary(total: int,intact: int,burned: int,faded: int,eroding: int) -> void:\n\tsummary=[total,intact,burned,faded,eroding]\n'
    code+='func _apply_memory_art_and_rewrite(_memory) -> void:\n\tpass # artwork and world-rewrite endpoint intentionally excluded\n'
    (work/'archive.gd').write_text(code,encoding='utf-8')
    (work/'executed_archive.gd').write_bytes((work/'archive.gd').read_bytes())
    gm=work/'game_manager_stub.gd'
    if not gm.exists():
        candidates=[x for x in work.glob('*.gd') if 'func change_state(' in x.read_text(encoding='utf-8')]
        assert len(candidates)==1,candidates
        gm=candidates[0]
    if 'func localized_speaker(' not in gm.read_text(encoding='utf-8'):
        with gm.open('a',encoding='utf-8') as f:
            f.write('\nfunc localized_speaker(npc: String) -> String:\n\treturn npc # speaker translations outside bounded archive fields\n')

def source_constants(data):
    def t(v):return 'TEXT('+json.dumps(v,ensure_ascii=True)+')'
    cases={c['id']:c for c in data}
    initial=[cases['initial_'+locale] for locale in ['en','ko']]
    states=[cases['states_'+locale] for locale in ['en','ko']]
    lines=['// Generated from executed source archive UI; do not hand-edit.']
    for key,name in [('title','SourceTitle'),('empty','SourceEmpty')]:
        lines.append('static const TCHAR* '+name+'[2] = {'+', '.join(t(c[key]) for c in initial)+'};')
    lines.append('static const TCHAR* SourceFilters[2][6] = {')
    for c in initial:lines.append('    {'+', '.join(t(x) for x in c['filters'])+'},')
    lines.append('};')
    all_states=[]
    eroding=[]
    for before,after in zip(initial,states):
        rows=after['rows']
        intact=before['rows'][0]['detail_state']
        burned=next(r['state_label'] for r in rows if r['burned'] and not r['residue'])
        residue=next(r['state_label'] for r in rows if r['burned'] and r['residue'])
        faded=next(r['state_label'] for r in rows if not r['burned'] and r['faded'])
        e=next(r for r in rows if not r['burned'] and not r['faded'] and r['erosion']>0)
        number=str(int(e['erosion_ratio']*100))
        assert number in e['state_label']
        eroding.append(e['state_label'].replace(number,'%d',1).replace('%','%%').replace('%%d','%d'))
        all_states.append([intact,burned,residue,faded])
    lines.append('static const TCHAR* SourceStates[2][4] = {')
    for row in all_states:lines.append('    {'+', '.join(t(x) for x in row)+'},')
    lines.append('};')
    lines.append('static const TCHAR* SourceEroding[2] = {'+', '.join(t(x) for x in eroding)+'};')
    accents={r['grade']:r['accent'] for r in initial[0]['rows']}
    lines.append('static const FLinearColor SourceAccents[5] = {')
    for grade in range(5):lines.append('    FLinearColor('+', '.join(format(v,'.8f')+'f' for v in accents[grade])+'),')
    lines.append('};')
    return '\n'.join(lines)+'\n'

def run(a):
    base.BASE=DEST;base.inputs=inputs;base.ORACLE=RUN
    if not a.check:DEST.mkdir(parents=True,exist_ok=True)
    base.run(a,project_setup=setup)
    report_path=a.evidence_dir/'oracle.json'
    report=json.loads(report_path.read_text(encoding='utf-8'))
    report['harness']='Exact original archive list/filter/card/detail/readonly guard methods and real MemoryManager; artwork/rewrite/carry/track summaries excluded. Fade and erosion examples are synthetic UI state fixtures after real burns. Legacy detail-vs-card state discrepancy retained.'
    report['source_raw_hashes'][UI]=base.sha((base.ROOT/UI).read_bytes())
    report_path.write_bytes(base.canonical(report))
    work=Path(report['commands'][-1]['command'][3])
    with zipfile.ZipFile(a.evidence_dir/'executed_harness.zip','w',zipfile.ZIP_DEFLATED) as z:
        for p in work.glob('*.gd'):z.write(p,p.name)
        for name in ['project.godot','inputs.json','outputs.json']:z.write(work/name,name)
    data=json.loads((DEST/'contract_expected.v1.json').read_text(encoding='utf-8'))
    before=TARGET.read_text(encoding='utf-8')
    start=before.index('// BEGIN EXECUTED ARCHIVE TEXT')+len('// BEGIN EXECUTED ARCHIVE TEXT')
    end=before.index('// END EXECUTED ARCHIVE TEXT')
    expected=before[:start]+'\n'+source_constants(data)+before[end:]
    if a.check:assert before==expected,'Executed archive constants changed'
    else:TARGET.write_text(expected,encoding='utf-8')

if __name__=='__main__':
    p=argparse.ArgumentParser()
    p.add_argument('--godot',type=Path,required=True)
    p.add_argument('--evidence-dir',type=Path,required=True)
    p.add_argument('--check',action='store_true')
    run(p.parse_args())
