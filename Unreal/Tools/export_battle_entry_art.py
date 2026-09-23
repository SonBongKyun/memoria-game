"""Render the untouched source PixelSprite Market Thief fallback in isolated Godot."""
from pathlib import Path
import re,argparse,hashlib,json,subprocess,zipfile,datetime
ROOT=Path(__file__).resolve().parents[2]
SOURCE=ROOT/'scripts/utils/pixel_sprite.gd'
DEST=ROOT/'docs/unreal-migration/fixtures/battle_entry_art/market_thief.png'
GODOT=Path(r'C:\Users\jc\Downloads\Godot_v4.6.2-stable_win64.exe\Godot_v4.6.2-stable_win64_console.exe')
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def run(evidence):
    evidence.mkdir(parents=True,exist_ok=False)
    work=ROOT/'Unreal/Memoria/Saved/Validation'/('battle-art-'+datetime.datetime.now().strftime('%Y%m%d-%H%M%S'))
    work.mkdir(parents=True,exist_ok=False)
    (work/'project.godot').write_text('[application]\nconfig/name="Isolated source battle art"\n[rendering]\nrenderer/rendering_method="gl_compatibility"\n',encoding='utf-8')
    source=SOURCE.read_text(encoding='utf-8')
    def method(name):
        match=re.search(r'^static func '+re.escape(name)+r'\(',source,re.M);assert match,name
        lines=source[match.start():].splitlines(keepends=True);end=1
        while end<len(lines) and (not lines[end].strip() or lines[end][0].isspace()):end+=1
        return ''.join(lines[:end])
    names=['create_battle_enemy','_enemy_archetype','_add_outline_n','_bpx','_bfill','_bellipse','_darken','_lighten','_draw_battle_void_beast','_draw_battle_shadow_wisp','_draw_battle_memory_eater','_draw_battle_shade_sentinel','acc_color_sentinel','_draw_battle_void_stalker','_draw_battle_generic_enemy']
    code='extends RefCounted\n'+next(line for line in source.splitlines() if line.startswith('const BATTLE_SIZE'))+'\n'+'\n'.join(method(n) for n in names)
    (work/'pixel_sprite.gd').write_text(code,encoding='utf-8')
    (work/'main.gd').write_text('extends SceneTree\nfunc _initialize():\n\tvar texture=load("res://pixel_sprite.gd").create_battle_enemy("Market Thief")\n\tvar output=OS.get_cmdline_user_args()[0]\n\tassert(texture.get_image().save_png(output)==OK)\n\tprint("MEMORIA_BATTLE_ART_PASS")\n\tquit(0)\n',encoding='utf-8')
    hashes=[];commands=[]
    for index in range(2):
        output=work/('thief-'+str(index)+'.png')
        command=[str(GODOT),'--headless','--path',str(work),'--script','main.gd','--quit-after','3','--',str(output)]
        result=subprocess.run(command,capture_output=True,text=True,encoding='utf-8',errors='replace',timeout=60)
        log=result.stdout+result.stderr
        (evidence/('source-'+str(index)+'.log')).write_text(log,encoding='utf-8')
        commands.append(dict(command=command,exit_code=result.returncode))
        assert result.returncode==0 and 'MEMORIA_BATTLE_ART_PASS' in log and not any(x in log for x in ['SCRIPT ERROR','Parse Error','ERROR:']),log
        hashes.append(sha(output))
    assert hashes[0]==hashes[1]
    if DEST.exists():assert sha(DEST)==hashes[0]
    else:DEST.parent.mkdir(parents=True,exist_ok=True);DEST.write_bytes((work/'thief-0.png').read_bytes())
    with zipfile.ZipFile(evidence/'executed_harness.zip','w',zipfile.ZIP_DEFLATED) as archive:
        for file in work.iterdir():
            if file.is_file():archive.write(file,file.name)
    (evidence/'art.json').write_text(json.dumps(dict(source=str(SOURCE.relative_to(ROOT)),source_sha256=sha(SOURCE),output=str(DEST.relative_to(ROOT)),output_sha256=hashes[0],replay_equal=True,commands=commands,scope='Original create_battle_enemy Market Thief maps to void_stalker placeholder; not final character design.'),indent=2)+'\n',encoding='utf-8')
    print('BATTLE_ART_PASS '+str(DEST))
if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--evidence-dir',type=Path,required=True);run(parser.parse_args().evidence_dir)
