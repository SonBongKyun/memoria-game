"""Read-only original/baseline audit, writing a new Phase 1O evidence file only."""
from pathlib import Path
import argparse,hashlib,json,subprocess
ROOT=Path(__file__).resolve().parents[2]
BASE=ROOT/'docs/unreal-migration/evidence/phase1o'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    ap=argparse.ArgumentParser();ap.add_argument('--output',type=Path,required=True);a=ap.parse_args()
    out=a.output.resolve();assert out.is_relative_to(BASE) and not out.exists()
    b=json.loads((BASE/'entering_baseline.json').read_text(encoding='utf-8'));manifest=Path(b['original_manifest']);original=json.loads(manifest.read_text(encoding='utf-8-sig'))
    allowed={'.gitattributes','.gitignore','SESSION_LOG.md','docs/unreal-migration/MIGRATION_STATE.md','docs/unreal-migration/MIGRATION_ROADMAP.md','docs/unreal-migration/PARITY_MATRIX.md',
    'Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaNarrativeSubsystem.cpp',
    'Unreal/Memoria/Source/Memoria/Public/Narrative/MemoriaNarrativeSubsystem.h',
    'Unreal/Memoria/Source/Memoria/Private/Presentation/MemoriaDevelopmentNarrativeWidget.cpp',
    'Unreal/Memoria/Source/Memoria/Public/Presentation/MemoriaDevelopmentNarrativeWidget.h',
    'Unreal/Memoria/Source/MemoriaTests/Private/MemoriaMaletTests.cpp',
    'Unreal/Memoria/Source/MemoriaTests/Private/MemoriaPotionEvidence.h',
    'Unreal/Memoria/Source/MemoriaTests/Private/MemoriaWorldCognitionTests.cpp',
    'Unreal/Tools/test_malet_antidote_tools.py','Unreal/Tools/test_malet_firebomb_tools.py','Unreal/Tools/test_malet_potion_tools.py','Unreal/Tools/test_malet_world_seed_tools.py',
    'Unreal/Tools/validate_malet_firebomb_evidence.py','Unreal/Tools/validate_unreal.py'}
    mismatches=[];changed=[];protected=0
    for rel,expected in b['tracked_sha256'].items():
        p=ROOT/rel;same=p.is_file() and sha(p)==expected
        if not same:changed.append(rel)
        if rel not in allowed:
            protected+=1
            if not same:mismatches.append(rel)
    original_bad=[r for r,h in original['hashes'].items() if not (Path(original['root'])/r).is_file() or sha(Path(original['root'])/r)!=h.lower()]
    old_packages=b['ue_packages'];new_packages=sorted(p.relative_to(ROOT).as_posix() for p in (ROOT/'Unreal/Memoria/Content').rglob('*') if p.suffix in ['.uasset','.umap'] and p.relative_to(ROOT).as_posix() not in old_packages)
    expected_new=['Unreal/Memoria/Content/Memoria/Presentation/Shop/T_MaletPortrait.uasset','Unreal/Memoria/Content/Memoria/Presentation/Shop/T_ShopBackdrop.uasset']
    checks={'manifest_unchanged':sha(manifest)==b['original_manifest_sha256'],'original_preserved':not original_bad,'worktree_preserved':not mismatches,'new_packages_exactly_two_presentation_textures':new_packages==expected_new,'previous_21_packages_preserved':all(sha(ROOT/r)==b['tracked_sha256'][r] for r in old_packages),'previous_88_ir_fixture_files_preserved':all(sha(ROOT/r)==b['tracked_sha256'][r] for r in b['prior_ir_fixtures'])}
    new_narrative=[p.relative_to(ROOT).as_posix() for parent in ['docs/unreal-migration/ir','Unreal/Memoria/Content/Memoria/Generated/Narrative'] for p in (ROOT/parent).rglob('*') if p.is_file() and p.relative_to(ROOT).as_posix() not in b['tracked_sha256']]
    checks['new_narrative_ir_packages_zero']=not new_narrative
    r={'status':'PASS' if all(checks.values()) else 'FAIL','checks':checks,'original_root':original['root'],'original_count':len(original['hashes']),'original_mismatches':original_bad,'baseline_tracked':len(b['tracked_sha256']),'protected_worktree_count':protected,'worktree_mismatches':mismatches,'authorized_modified_baseline_files':changed,'new_packages':new_packages,'new_narrative_files':new_narrative}
    out.parent.mkdir(parents=True,exist_ok=True);out.write_text(json.dumps(r,indent=2)+'\n',encoding='utf-8');print(json.dumps({k:r[k] for k in ['status','checks','protected_worktree_count','original_count']}));return int(r['status']!='PASS')
if __name__=='__main__':raise SystemExit(main())
