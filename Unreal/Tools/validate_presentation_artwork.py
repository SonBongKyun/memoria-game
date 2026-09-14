"""Source/protected-byte audit for the first illustrated Unreal presentation pass."""
from pathlib import Path
import argparse,hashlib,json,subprocess
ROOT=Path(__file__).resolve().parents[2]
BASE=ROOT/'docs/unreal-migration/evidence/presentation1'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    ap=argparse.ArgumentParser();ap.add_argument('--output',type=Path,required=True);args=ap.parse_args()
    out=args.output.resolve();assert out.is_relative_to(BASE) and not out.exists()
    b=json.loads((BASE/'entering_baseline.json').read_text(encoding='utf-8'));art=json.loads((BASE/'artwork_sources.json').read_text(encoding='utf-8'))
    allowed={'.gitattributes','SESSION_LOG.md','docs/unreal-migration/MIGRATION_STATE.md','docs/unreal-migration/PLAYABLE_SLICE.md',
    'Unreal/Memoria/Source/Memoria/Public/Narrative/MemoriaNarrativeSubsystem.h',
    'Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaNarrativeSubsystem.cpp',
    'Unreal/Memoria/Source/Memoria/Private/Framework/MemoriaSliceHost.cpp',
    'Unreal/Memoria/Source/Memoria/Public/Presentation/MemoriaDevelopmentNarrativeWidget.h',
    'Unreal/Memoria/Source/Memoria/Private/Presentation/MemoriaDevelopmentNarrativeWidget.cpp',
    'Unreal/Memoria/Source/MemoriaTests/Private/MemoriaSliceRuntimeTests.cpp',
    'Unreal/Memoria/Source/MemoriaTests/Private/MemoriaMaletTests.cpp','Unreal/Tools/validate_unreal.py'}
    changes=[r for r,h in b['tracked_sha256'].items() if not (ROOT/r).is_file() or sha(ROOT/r)!=h]
    bad=[r for r in changes if r not in allowed]
    manifest=Path(b['original_manifest']);original=json.loads(manifest.read_text(encoding='utf-8-sig'))
    original_bad=[r for r,h in original['hashes'].items() if sha(Path(original['root'])/r)!=h.lower()]
    old_packages=[r for r in b['tracked_sha256'] if r.startswith('Unreal/Memoria/Content/') and Path(r).suffix in ['.uasset','.umap']]
    packages={p.relative_to(ROOT).as_posix():sha(p) for p in (ROOT/'Unreal/Memoria/Content').rglob('*') if p.suffix in ['.uasset','.umap']}
    new=sorted(set(packages)-set(old_packages));expected=sorted('Unreal/Memoria/Content/'+r['package'].removeprefix('/Game/')+'.uasset' for r in art['assets'] if not r['reuse'])
    checks={'original_manifest_unchanged':sha(manifest)==b['original_manifest_sha256'],'original_4217_preserved':not original_bad,
    'protected_worktree_bytes_preserved':not bad,'previous_23_packages_preserved':len(old_packages)==23 and all(packages.get(r)==b['tracked_sha256'][r] for r in old_packages),
    'new_packages_exactly_18_presentation_textures':new==expected,
    'all_19_source_images_unchanged':all(sha(ROOT/r['source'].removeprefix('res://'))==r['sha256'] for r in art['assets']),
    'source_reference_files_unchanged':all(sha(ROOT/r)==h for r,h in art['source_files'].items()),
    'new_narrative_ir_zero':not any(p.is_file() and p.relative_to(ROOT).as_posix() not in b['tracked_sha256'] for p in (ROOT/'docs/unreal-migration/ir').rglob('*'))}
    result={'status':'PASS' if all(checks.values()) else 'FAIL','checks':checks,'original_count':len(original['hashes']),'original_mismatches':original_bad,'protected_worktree_count':len(b['tracked_sha256'])-len(allowed),'worktree_mismatches':bad,'authorized_modified':changes,'new_packages':{r:packages[r] for r in new},'previous_package_count':len(old_packages),'prior_ir_fixture_count':sum(r.startswith(('docs/unreal-migration/ir/','docs/unreal-migration/fixtures/')) for r in b['tracked_sha256'])}
    out.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8');print(json.dumps({k:result[k] for k in ['status','checks','protected_worktree_count','prior_ir_fixture_count']}));return result['status']!='PASS'
if __name__=='__main__':raise SystemExit(main())
