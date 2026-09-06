"""Build the production memory kernel with MSVC and run Godot parity vectors.

This provides native C++ evidence only; it cannot validate Unreal headers,
reflection, module linking, UObject ownership, editor behavior or rendering.
"""
from pathlib import Path
import argparse
import datetime
import json
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
work = ROOT / 'Unreal/Memoria/Intermediate/NativeParity'
work.mkdir(parents=True, exist_ok=True)
parser = argparse.ArgumentParser()
parser.add_argument('--evidence-dir', type=Path)
args = parser.parse_args()
evidence = args.evidence_dir.resolve() if args.evidence_dir else ROOT / 'Unreal/Memoria/Saved/Validation' / ('native-' + datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%S%f'))
if any(evidence.is_relative_to(ROOT / 'docs/unreal-migration/evidence' / phase) for phase in ('phase0', 'phase1a', 'phase1b')):
    parser.error('Historical evidence is immutable; choose a new output directory')
evidence.mkdir(parents=True, exist_ok=True)
report = {'utc': datetime.datetime.now(datetime.timezone.utc).isoformat(), 'scope': 'same production C++ memory model; no UE compilation', 'commands': []}
commands = [
    ('fixture_check', [sys.executable, 'Unreal/Tools/generate_memory_test_header.py', '--check']),
    ('configure', ['cmake', '-S', 'Unreal/Tests/Native', '-B', str(work), '-G', 'Visual Studio 17 2022', '-A', 'x64']),
    ('build', ['cmake', '--build', str(work), '--config', 'Debug']),
    ('parity', [str(work / 'Debug/MemoriaMemoryParity.exe')]),
    ('ctest', ['ctest', '--test-dir', str(work), '-C', 'Debug', '--output-on-failure']),
]
for name, command in commands:
    result = subprocess.run(command, cwd=ROOT, capture_output=True, encoding='utf-8', errors='replace', timeout=900)
    output = result.stdout + result.stderr
    (work / (name + '.log')).write_text(output, encoding='utf-8')
    passed = result.returncode == 0
    if name == 'parity':
        expected = len(json.loads((ROOT / 'docs/unreal-migration/fixtures/player_memory_inputs.json').read_text(encoding='utf-8'))['cases'])
        report['cases'] = expected
        passed = passed and f'MEMORIA_NATIVE_MEMORY_PARITY PASS cases={expected} failed=0' in output and len(re.findall(r'(?m)^PASS ', output)) == expected
    report['commands'].append({'name': name, 'command': command, 'exit_code': result.returncode, 'passed': passed, 'output': output})
    report['status'] = 'PASS' if passed and name == 'ctest' else 'INCOMPLETE' if passed else 'FAIL'
    (evidence / 'native_memory_validation.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(f"{name}: {'PASS' if passed else 'FAIL'}", flush=True)
    if not passed:
        print(output[-10000:])
        raise SystemExit(1)
print('MEMORIA_NATIVE_MEMORY_VALIDATION_PASS (not Unreal compilation)')
