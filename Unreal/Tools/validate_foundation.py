"""Structural/source preservation checks; explicitly NOT UHT or UE compilation."""
from pathlib import Path
import argparse
import ast
import datetime
import hashlib
import json
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser()
parser.add_argument('--original-manifest', type=Path)
parser.add_argument('--evidence-dir', type=Path)
args = parser.parse_args()
evidence = args.evidence_dir.resolve() if args.evidence_dir else ROOT / 'Unreal/Memoria/Saved/Validation' / ('foundation-' + datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%S%f'))
if any(evidence.is_relative_to(ROOT / 'docs/unreal-migration/evidence' / phase) for phase in ('phase0', 'phase1a', 'phase1b', 'phase1b-ue58')):
    parser.error('Historical evidence is immutable; choose a new output directory')
errors = []
checks = []


def check(name, valid):
    checks.append({'name': name, 'passed': bool(valid)})
    if not valid:
        errors.append(name)


def git(*args):
    return subprocess.check_output(['git', '-C', str(ROOT), *args], encoding='utf-8').strip()


project = ROOT / 'Unreal/Memoria'
config = json.loads((project / 'Memoria.uproject').read_text(encoding='utf-8'))
check('engine association 5.8', config['EngineAssociation'] == '5.8')
check('minimal plugins', {(p['Name'], p['Enabled']) for p in config['Plugins']} == {('Paper2D', True), ('EnhancedInput', True), ('AndroidFileServer', False)})
for module in config['Modules']:
    check(f"module rules: {module['Name']}", (project / 'Source' / module['Name'] / (module['Name'] + '.Build.cs')).is_file())
for target in ('Memoria', 'MemoriaEditor'):
    source = (project / 'Source' / (target + '.Target.cs')).read_text(encoding='utf-8')
    check(f'{target} pinned settings', 'BuildSettingsVersion.V7' in source and 'EngineIncludeOrderVersion.Unreal5_8' in source)
for file in (project / 'Source').rglob('*.h'):
    source = file.read_text(encoding='utf-8')
    if 'GENERATED_BODY()' in source:
        includes = re.findall(r'^#include "([^"]+)"', source, re.M)
        check(f'generated include last: {file.stem}', bool(includes) and includes[-1] == file.stem + '.generated.h')
for folder, prefix in (('Public', 'Framework'),):
    for name in ('MemoriaGameInstance', 'MemoriaGameMode', 'MemoriaPlayerController', 'MemoriaFieldPawn'):
        check(f'configured class declaration: {name}', (project / f'Source/Memoria/{folder}/{prefix}/{name}.h').is_file())
asset_root = project / 'Content/Tests/Foundation'
asset_names = [name + '.uasset' for name in ('IA_Move', 'IA_Confirm', 'IA_Back', 'IA_Menu', 'IMC_Foundation', 'IMC_Modal', 'T_FootPivot', 'SPR_FootPivot', 'WBP_FoundationModal')] + ['L_FoundationTest.umap']
check('all ten foundation packages have Unreal binary headers (runtime load tested separately)',
      all((asset_root / name).is_file() and (asset_root / name).read_bytes()[:4] == bytes.fromhex('c1832a9e') for name in asset_names))
check('Godot scanner boundary', (ROOT / 'Unreal/.gdignore').is_file())
check('Godot export boundary', 'Unreal/*, Unreal/**' in (ROOT / 'export_presets.cfg').read_text(encoding='utf-8'))
for relative in ('Unreal/Memoria/Binaries/probe.bin', 'Unreal/Memoria/Intermediate/probe.obj', 'Unreal/Memoria/Saved/probe.log',
                 'Unreal/Memoria/DerivedDataCache/probe.bin', 'Unreal/Memoria/.vs/probe.bin', 'Unreal/Memoria/Memoria.sln'):
    result = subprocess.run(['git', '-C', str(ROOT), 'check-ignore', '--no-index', relative], capture_output=True)
    check(f'ignored: {relative}', result.returncode == 0)
for relative in ('Unreal/Memoria/Content/probe.uasset', 'Unreal/Memoria/Content/probe.umap'):
    check(f'LFS: {relative}', git('check-attr', 'filter', '--', relative).endswith(': lfs'))
check('existing PNG assets not newly LFS', git('check-attr', 'filter', '--', 'assets/portraits/arrel_face_neutral.png').endswith(': unspecified'))
for file in (ROOT / 'Unreal/Tools').glob('*.py'):
    try:
        ast.parse(file.read_text(encoding='utf-8'))
    except SyntaxError:
        errors.append(f'Python syntax: {file.name}')
check('Python tools parse', not any(e.startswith('Python syntax:') for e in errors))
provenance = json.loads((ROOT / 'docs/unreal-migration/fixtures/player_memory_provenance.json').read_text(encoding='utf-8'))
for source, expected in provenance['source_sha256'].items():
    check(f'Godot oracle source hash: {source}', hashlib.sha256((ROOT / source).read_bytes()).hexdigest() == expected)
header = subprocess.run([sys.executable, str(ROOT / 'Unreal/Tools/generate_memory_test_header.py'), '--check'], capture_output=True, encoding='utf-8')
check('fixture header matches attested Godot outputs', header.returncode == 0)
changes = git('diff', '--name-only').splitlines()
allowed = {'.gitignore', '.gitattributes', 'export_presets.cfg', 'SESSION_LOG.md', 'docs/unreal-migration/MIGRATION_ROADMAP.md'}
check('tracked source changes confined to migration boundaries', all(p in allowed or p.startswith(('Unreal/', 'docs/unreal-migration/')) for p in changes))
check('project.godot unchanged from worktree base', subprocess.run(['git', '-C', str(ROOT), 'diff', '--quiet', 'HEAD', '--', 'project.godot']).returncode == 0)
protected_count = 0
if args.original_manifest:
    manifest = json.loads(args.original_manifest.read_text(encoding='utf-8-sig'))
    original = Path(manifest['root'])
    for relative, expected in manifest['hashes'].items():
        path = original / relative
        if not path.is_file() or hashlib.sha256(path.read_bytes()).hexdigest().lower() != expected.lower():
            errors.append(f'Original file changed: {relative}')
        protected_count += 1
    check(f'original checkout byte preservation: {protected_count} files', not any(e.startswith('Original file changed:') for e in errors))
    for relative in ('AUDIT.md', 'ARCHITECTURE.md', 'SYSTEM_MAP.md', 'PARITY_MATRIX.md', 'DATA_MIGRATION.md', 'ASSET_INVENTORY.md', 'RISK_REGISTER.md'):
        check(f'Phase 0 document preserved: {relative}', (ROOT / 'docs/unreal-migration' / relative).read_bytes() == (original / 'docs/unreal-migration' / relative).read_bytes())
    previous = (original / 'docs/unreal-migration/MIGRATION_ROADMAP.md').read_bytes()
    check('roadmap previous contents preserved as prefix', (ROOT / 'docs/unreal-migration/MIGRATION_ROADMAP.md').read_bytes().startswith(previous))
report = {'utc': datetime.datetime.now(datetime.timezone.utc).isoformat(), 'kind': 'static structure and file integrity; not UHT/build/editor validation',
          'checks': checks, 'protected_original_files': protected_count, 'errors': errors, 'status': 'PASS' if not errors else 'FAIL'}
evidence.mkdir(parents=True, exist_ok=True)
(evidence / 'foundation_static.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
print(f"MEMORIA_FOUNDATION_STATIC_{report['status']} checks={len(checks)} protected_files={protected_count}")
for error in errors:
    print('FAIL ' + error)
sys.exit(0 if not errors else 1)
