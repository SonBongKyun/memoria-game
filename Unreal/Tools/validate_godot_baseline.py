"""Run unchanged Godot validators with ALL subprocess app-data isolated."""
from pathlib import Path
import argparse
import datetime
import json
import os
import re
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser()
parser.add_argument('--godot', required=True)
parser.add_argument('--evidence-dir', type=Path)
args = parser.parse_args()
work = ROOT / 'Unreal/Memoria/Intermediate/GodotBaseline'
evidence = args.evidence_dir.resolve() if args.evidence_dir else ROOT / 'Unreal/Memoria/Saved/Validation' / ('godot-' + datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%S%f'))
if any(evidence.is_relative_to(ROOT / 'docs/unreal-migration/evidence' / phase) for phase in ('phase0', 'phase1a', 'phase1b', 'phase1b-ue58')):
    parser.error('Historical evidence is immutable; choose a new output directory')
work.mkdir(parents=True, exist_ok=True)
evidence.mkdir(parents=True, exist_ok=True)
env = os.environ.copy()
env['APPDATA'] = str(work / 'Roaming')
env['LOCALAPPDATA'] = str(work / 'Local')
env['PYTHONIOENCODING'] = 'utf-8'
Path(env['APPDATA']).mkdir(exist_ok=True)
Path(env['LOCALAPPDATA']).mkdir(exist_ok=True)
godot = str(Path(args.godot).resolve())
version = subprocess.check_output([godot, '--version'], text=True).strip()
if not version.startswith('4.6.2.'):
    raise SystemExit(f'Baseline requires Godot 4.6.2; detected {version}')
report = {'utc': datetime.datetime.now(datetime.timezone.utc).isoformat(), 'engine_version': version,
          'appdata': env['APPDATA'], 'localappdata': env['LOCALAPPDATA'], 'checks': []}
commands = [
    ('repo_contract', [sys.executable, 'scripts/validation/repo_contract.py'], 60, 'MEMORIA repository contracts passed'),
    ('vn', [sys.executable, 'scripts/tools/validate_vn_scenes.py'], 60, '0 errors'),
    ('korean', [sys.executable, 'scripts/tools/validate_korean_localization.py'], 60, '0 errors'),
    ('editor_import', [godot, '--headless', '--editor', '--path', str(ROOT), '--import', '--quit'], 900, ''),
    ('memory_world_suite', [shutil.which('pwsh') or 'pwsh', '-NoProfile', '-File', 'scripts/tools/run_memory_world_engine_smoke_suite.ps1', '-GodotPath', godot], 1200,
     'MEMORY_WORLD_ENGINE_SUITE_PASS cases=15 fatal_scan=enabled save_isolation=guarded export_catalog=verified'),
]
# Godot editor rewrites tracked .import metadata and line endings. Preserve
# exact pre-run bytes; use this runner in an isolated worktree without concurrent
# edits to these files. No Godot source code or user profile data is restored.
tracked = subprocess.check_output(['git', '-C', str(ROOT), 'ls-files', '-z']).split(b"\0")
protected = [ROOT / p.decode('utf-8') for p in tracked if p and (p.endswith(b'.import') or p == b'project.godot')]
backups = {path: path.read_bytes() for path in protected}
try:
    for name, command, timeout, marker in commands:
        result = subprocess.run(command, cwd=ROOT, env=env, capture_output=True, text=True, encoding='utf-8', errors='replace', timeout=timeout)
        output = result.stdout + result.stderr
        (work / f'{name}.log').write_text(output, encoding='utf-8')
        # Official suite already checks expected exit-1 guards and every runtime's
        # complete fatal output. Editor plugin resource warnings remain visible.
        fatal = re.findall(r'(?im)^.*(?:SCRIPT ERROR|Parse Error|Assertion failed|Unhandled Exception|\[SMOKE\]\[(?:FAIL|SUITE_FAIL)\]).*$', output)
        passed = result.returncode == 0 and marker in output and not fatal
        report['checks'].append({'name': name, 'command': command, 'exit_code': result.returncode, 'passed': passed,
                                 'fatal_diagnostics': fatal, 'log': str((work / f'{name}.log').relative_to(ROOT)),
                                 'output': output if name != 'editor_import' else '\n'.join(output.splitlines()[-40:]),
                                 'engine_error_lines': len(re.findall(r'(?m)^ERROR:', output))})
        report['status'] = 'PASS' if all(c['passed'] for c in report['checks']) and len(report['checks']) == len(commands) else 'INCOMPLETE'
        (evidence / 'godot_baseline.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
        print(f"{name}: {'PASS' if passed else 'FAIL'} exit={result.returncode}", flush=True)
        if not passed:
            print(output[-10000:])
            raise SystemExit(1)
    print('MEMORIA_GODOT_BASELINE_PASS checks=5 user_profile=isolated')
finally:
    restored = []
    for path, before in backups.items():
        if path.read_bytes() != before:
            path.write_bytes(before)
            restored.append(str(path.relative_to(ROOT)))
    report['editor_metadata_restored'] = restored
    (evidence / 'godot_baseline.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
