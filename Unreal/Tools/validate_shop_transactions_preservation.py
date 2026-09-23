"""Compare all pre-existing bytes, including prior uncommitted work, and original checkout."""
from pathlib import Path
import argparse,json,hashlib,concurrent.futures,subprocess
ROOT=Path(__file__).resolve().parents[2];BASE=ROOT/'docs/unreal-migration/evidence/shop_transactions1'
ALLOWED={'.gitattributes',
'SESSION_LOG.md','docs/unreal-migration/MIGRATION_STATE.md','docs/unreal-migration/PLAYABLE_SLICE.md',
'Unreal/Memoria/Source/Memoria/Private/Framework/MemoriaSliceHost.cpp',
'Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaNarrativeSubsystem.cpp',
'Unreal/Memoria/Source/Memoria/Public/Narrative/MemoriaNarrativeSubsystem.h',
'Unreal/Memoria/Source/Memoria/Private/Presentation/MemoriaDevelopmentNarrativeWidget.cpp',
'Unreal/Memoria/Source/Memoria/Private/Presentation/MemoriaShopWidget.cpp',
'Unreal/Memoria/Source/Memoria/Public/Presentation/MemoriaShopWidget.h',
'Unreal/Memoria/Source/Memoria/Private/Shop/MemoriaShopSubsystem.cpp',
'Unreal/Memoria/Source/Memoria/Public/Shop/MemoriaShopSubsystem.h',
'Unreal/Memoria/Source/Memoria/Public/Run/MemoriaRunSubsystem.h',
'Unreal/Memoria/Source/MemoriaTests/Private/MemoriaMaletTests.cpp',
'Unreal/Tools/validate_unreal.py',
'Unreal/Tools/test_malet_antidote_tools.py','Unreal/Tools/test_malet_firebomb_tools.py',
'Unreal/Tools/test_malet_potion_tools.py','Unreal/Tools/test_malet_world_seed_tools.py'}
def sha(p):
    if not p.is_file():return None
    h=hashlib.sha256()
    with p.open('rb') as f:
        for b in iter(lambda:f.read(1048576),b''):h.update(b)
    return h.hexdigest()
def mismatches(root,hashes):
    with concurrent.futures.ThreadPoolExecutor(max_workers=6) as ex:
        actual=dict(zip(hashes,ex.map(lambda p:sha(root/p),hashes)))
    return [p for p,h in hashes.items() if actual[p]!=h.lower()]
def main():
    ap=argparse.ArgumentParser();ap.add_argument('--output',type=Path,required=True);a=ap.parse_args()
    out=a.output.resolve();assert out.is_relative_to(BASE) and not out.exists()
    b=json.loads((BASE/'entering_baseline.json').read_text(encoding='utf-8'))
    changed=mismatches(ROOT,b['sha256']);bad=sorted(set(changed)-ALLOWED)
    mp=Path(b['original_manifest']);m=json.loads(mp.read_text(encoding='utf-8-sig'))
    original_bad=mismatches(Path(m['root']),m['hashes'])
    packages=[p for p in b['sha256'] if p.startswith('Unreal/Memoria/Content/') and Path(p).suffix in ['.uasset','.umap']]
    new=[p.relative_to(ROOT).as_posix() for p in (ROOT/'Unreal/Memoria/Content').rglob('*') if p.suffix in ['.uasset','.umap'] and p.relative_to(ROOT).as_posix() not in b['sha256']]
    checks={'original_manifest':sha(mp)==b['original_manifest_sha256'],'original_4217':len(m['hashes'])==4217 and not original_bad,
        'protected_worktree':not bad,'old_packages_95':len(packages)==95 and not set(packages)&set(changed),'no_new_packages':not new,
        'prior_evidence_fixtures':not any(p.startswith('docs/unreal-migration/') and p not in ALLOWED for p in changed),
        'head_unchanged':subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT).decode().strip()==b['head']}
    result={'status':'PASS' if all(checks.values()) else 'FAIL','checks':checks,'protected_count':len(set(b['sha256'])-ALLOWED),'authorized_modified':changed,'unexpected':bad,'original_mismatches':original_bad,'new_packages':new}
    out.write_text(json.dumps(result,indent=2),encoding='utf-8');print(json.dumps(result));return result['status']!='PASS'
if __name__=='__main__':raise SystemExit(main())

