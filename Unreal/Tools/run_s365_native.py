"""Run native S365 UE inspection and freeze current graphics evidence.

The process may exit zero when Automation tests failed. Success requires the
exact selected current source identities and a fresh, completed JSON report.
"""
from pathlib import Path
import argparse
import datetime
import importlib.util
import json
import hashlib
import os
import re
import shutil
import subprocess
import sys
import time

sys.dont_write_bytecode = True


def execution_inputs(root):
    """Hash actual saved inputs; no claim that a DLL was built from a source hash."""
    paths = {root/'Unreal/Memoria/Memoria.uproject'}
    for relative in ('Unreal/Memoria/Source', 'Unreal/Memoria/Config',
                     'Unreal/Memoria/Content', 'Unreal/ArtSource', 'Unreal/Tests', 'Unreal/Tools',
                     'docs/unreal-migration/fixtures'):
        folder = root/relative
        for parent, directories, files in os.walk(folder, followlinks=False):
            directories[:] = [name for name in directories if name != '__pycache__']
            for name in directories:
                path = Path(parent)/name
                if path.is_symlink() or getattr(path.stat(), 'st_file_attributes', 0) & 0x400:
                    raise ValueError('Execution input directory is a link/reparse point: '+str(path))
            for name in files:
                path = Path(parent)/name
                if path.is_symlink() or getattr(path.stat(), 'st_file_attributes', 0) & 0x400:
                    raise ValueError('Execution input is a link/reparse point: '+str(path))
                if relative == 'Unreal/Tools' and path.suffix not in ('.py', '.ps1'):
                    continue
                if path.is_file():
                    paths.add(path)
    binaries = root/'Unreal/Memoria/Binaries/Win64'
    paths.update(path for pattern in ('*.dll', '*.modules') for path in binaries.glob(pattern))
    for path in paths:
        if path.is_symlink() or getattr(path.stat(), 'st_file_attributes', 0) & 0x400:
            raise ValueError('Execution input is a link/reparse point: '+str(path))
    return {path.relative_to(root).as_posix(): hashlib.sha256(path.read_bytes()).hexdigest()
            for path in sorted(paths)}


def changed_inputs(before, after):
    return {'added': sorted(after.keys()-before.keys()),
            'removed': sorted(before.keys()-after.keys()),
            'changed': sorted(path for path in before.keys() & after.keys() if before[path] != after[path])}


def filter_matches(pattern, name):
    # UE Automation RunTests is a literal matcher, not a regular expression.
    # The command handler strips optional leading ^ / trailing $ anchors.
    pattern = pattern.strip()
    if not pattern or pattern.startswith(('Group:', 'StartsWith:')):
        raise ValueError('Use a nonempty literal test name/prefix and optional ^ / $ anchors')
    start, end = pattern.startswith('^'), pattern.endswith('$')
    needle = pattern[int(start):len(pattern)-int(end) if end else None].lower()
    value = name.lower()
    if not needle:
        raise ValueError('Empty anchored test filter')
    if start and end:
        return value.startswith(needle) and value.endswith(needle)
    if start:
        return value.startswith(needle)
    if end:
        return value.endswith(needle)
    return needle in value


def registry(root):
    spec = importlib.util.spec_from_file_location('memoria_validator', root/'Unreal/Tools/validate_unreal.py')
    module = importlib.util.module_from_spec(spec)
    sys.path.insert(0, str(root/'Unreal/Tools'))
    spec.loader.exec_module(module)
    paths = module.current_test_paths() | module.visual_test_paths() | module.checkpoint_process_paths()
    for source in (root/'Unreal/Memoria/Source').rglob('*.cpp'):
        text = source.read_text(encoding='utf-8-sig')
        paths.update(re.findall(r'IMPLEMENT_SIMPLE_AUTOMATION_TEST\s*\(\s*\w+\s*,\s*"(Memoria[^"\n]+)"', text))
    return paths


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument('--engine-root', type=Path, default=Path('C:/Program Files/Epic Games/UE_5.8'))
    parser.add_argument('--tests', default='Memoria', help='UE + separated literal filters with optional ^ / $ anchors')
    parser.add_argument('--evidence', type=Path, required=True)
    parser.add_argument('--timeout', type=int, default=7200)
    parser.add_argument('--extra', action='append', default=[])
    parser.add_argument('--list', action='store_true')
    args = parser.parse_args()
    args.root = args.root.resolve()
    args.evidence = args.evidence.resolve()
    expected = {p for p in registry(args.root) if any(filter_matches(f, p) for f in args.tests.split('+'))}
    if args.list:
        print(json.dumps(sorted(expected), indent=2)); return 0
    if not expected:
        raise ValueError('Selected zero source test identities')
    if args.evidence.exists():
        raise FileExistsError('Use a fresh evidence directory')
    args.evidence.mkdir(parents=True)
    before = execution_inputs(args.root)
    (args.evidence/'execution_inputs_before.json').write_text(json.dumps(before, indent=2), encoding='utf-8')
    report = args.evidence/'Automation'
    command = [str(args.engine_root/'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'),
        str(args.root/'Unreal/Memoria/Memoria.uproject'), '-unattended', '-nop4', '-nosplash',
        '-RenderOffscreen', '-MemoriaCapture', '-ResX=1280', '-ResY=720',
        '-ExecCmds=Automation RunTests '+args.tests, '-TestExit=Automation Test Queue Empty',
        '-ReportExportPath='+str(report), '-stdout', '-FullStdOutLogOutput', *args.extra]
    result = {'utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
        'command': command, 'expected': sorted(expected), 'status': 'RUNNING'}
    receipt = args.evidence/'result.json'
    receipt.write_text(json.dumps(result, indent=2), encoding='utf-8')
    started = time.time()
    with (args.evidence/'automation.log').open('w', encoding='utf-8') as output:
        try:
            run = subprocess.run(command, stdout=output, stderr=subprocess.STDOUT, timeout=args.timeout)
            result['exit_code'] = run.returncode
        except subprocess.TimeoutExpired:
            result['exit_code'] = None
            result['timed_out'] = True
    errors = []
    index = report/'index.json'
    try:
        data = json.loads(index.read_text(encoding='utf-8-sig'))
        tests = data['tests']
        actual = [t['fullTestPath'] for t in tests]
        if index.stat().st_mtime < started: errors.append('Stale report')
        if len(actual) != len(set(actual)): errors.append('Duplicate identity')
        if expected - set(actual): errors.append('Missing: '+', '.join(sorted(expected-set(actual))))
        if set(actual)-expected: errors.append('Unexpected: '+', '.join(sorted(set(actual)-expected)))
        result['tests'] = [{'path': t['fullTestPath'], 'state': t['state'],
            'errors': [e['event']['message'] for e in t.get('entries', []) if e.get('event', {}).get('type') == 'Error']}
            for t in tests]
        if any(t['state'] != 'Success' for t in tests): errors.append('Unsuccessful tests')
        for key in ('failed', 'notRun', 'inProcess'):
            if data.get(key, 0): errors.append('Report '+key+'='+str(data[key]))
    except (OSError, ValueError, KeyError, TypeError) as error:
        errors.append('Invalid report: '+str(error))
    log = (args.evidence/'automation.log').read_text(encoding='utf-8', errors='replace')
    fatal = re.findall(r'(?im)^.*(?:fatal error|Assertion failed:|Unhandled Exception:).*$', log)
    if fatal: errors.append('Fatal native diagnostics')
    if result.get('exit_code') != 0: errors.append('Nonzero process exit or timeout')
    # Test captures use the game's normal Saved/Validation paths. Freeze only
    # this run's actual new pixels before any later run can replace those names.
    captures = []
    saved = args.root/'Unreal/Memoria/Saved/Validation'
    for path in saved.rglob('*.png'):
        if path.is_relative_to(args.evidence) or path.stat().st_mtime < started:
            continue
        relative = path.relative_to(saved)
        destination = args.evidence/'Captures'/relative
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(path, destination)
        captures.append({'path': str(destination.relative_to(args.evidence)),
            'sha256': hashlib.sha256(destination.read_bytes()).hexdigest()})
    result['captures'] = captures
    # Preserve the same run's actual geometry and profiling evidence before
    # subsequent native tests can overwrite their established output names.
    artifacts = []
    for folder in ('HeroContactS362', 'S362Performance', 'S363Regional', 'S363Hud', 'S363Cloth', 'S364Combo', 'S364Defense', 'S364Focus', 'S365Swing', 'S365Feedback'):
        for path in (saved/folder).rglob('*'):
            if not path.is_file() or path.suffix.lower() not in ('.json', '.csv') or path.stat().st_mtime < started:
                continue
            destination = args.evidence/'Artifacts'/path.relative_to(saved)
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(path, destination)
            artifacts.append({'path': str(destination.relative_to(args.evidence)),
                'sha256': hashlib.sha256(destination.read_bytes()).hexdigest()})
    result['artifacts'] = artifacts
    after = execution_inputs(args.root)
    (args.evidence/'execution_inputs_after.json').write_text(json.dumps(after, indent=2), encoding='utf-8')
    differences = changed_inputs(before, after)
    result['execution_inputs'] = {'files': len(before), 'unchanged': not any(differences.values()),
                                  'differences': differences,
                                  'scope': 'Saved source/config/content/art source/test fixtures/Python and PowerShell tools and DLL/module inputs. Does not prove compilation provenance.'}
    if any(differences.values()):
        errors.append('Execution inputs changed during native run')
    result.update(status='PASS' if not errors else 'FAIL', validation_errors=errors,
        fatal_diagnostics=fatal, elapsed_seconds=round(time.time()-started, 3))
    receipt.write_text(json.dumps(result, indent=2), encoding='utf-8')
    print(json.dumps({k: result[k] for k in ('status', 'validation_errors', 'elapsed_seconds')}, ensure_ascii=False))
    print('Expected tests:', len(expected), 'Receipt:', receipt)
    return 0 if not errors else 1


if __name__ == '__main__':
    sys.exit(main())
