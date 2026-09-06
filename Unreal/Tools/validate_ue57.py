"""Probe exact UE 5.7; optionally build Editor and run real Automation tests.

No installation, version retargeting, packaging, or source profile access.
Exit 2 means the requested toolchain is absent. A native CMake build never
counts as UE compilation. Every invocation writes a fresh status report.
"""
from pathlib import Path
import argparse
import datetime
import json
import os
import re
import subprocess
import sys
import winreg

ROOT = Path(__file__).resolve().parents[2]
WORK = ROOT / 'Unreal/Memoria/Intermediate/UE57Validation'


def expected_test_paths():
    fixtures = json.loads((ROOT / 'docs/unreal-migration/fixtures/player_memory_inputs.json').read_text(encoding='utf-8'))['cases']
    paths = ['Memoria.Memory.SourceParity.' + case['id'] for case in fixtures]
    if len(paths) != 51 or len(set(paths)) != 51:
        raise ValueError('Phase 1B requires the 51 distinct attested memory fixtures')
    return set(paths) | {
        'Memoria.Foundation.MemoryAdapter',
        'Memoria.Foundation.SaveAndDialectBoundaries',
        'Memoria.Foundation.RunOwnership',
    }


def inspect_automation_report(result, expected):
    """Validate identities as well as counts; repeated/missing tests cannot pass."""
    errors = []
    if not isinstance(result, dict) or not isinstance(result.get('tests'), list):
        return {'passed': False, 'discovered': 0, 'source_parity_discovered': 0, 'errors': ['Missing tests array']}
    tests = result['tests']
    paths = []
    for test in tests:
        if not isinstance(test, dict) or not isinstance(test.get('fullTestPath'), str):
            errors.append('Malformed test record')
            continue
        paths.append(test['fullTestPath'])
        if test.get('state') != 'Success':
            errors.append('Unsuccessful test: ' + test['fullTestPath'])
    if len(paths) != len(set(paths)):
        errors.append('Duplicate test identity')
    missing = expected - set(paths)
    unexpected = set(paths) - expected
    if missing:
        errors.append('Missing tests: ' + ', '.join(sorted(missing)))
    if unexpected:
        errors.append('Unexpected tests: ' + ', '.join(sorted(unexpected)))
    if len(tests) != len(expected):
        errors.append('Incorrect test count')
    if result.get('failed', 0) != 0:
        errors.append('Report declares failed tests')
    return {'passed': not errors, 'discovered': len(tests),
            'source_parity_discovered': sum(p.startswith('Memoria.Memory.SourceParity.') for p in paths), 'errors': errors}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--engine-root', type=Path)
    parser.add_argument('--build-and-test', action='store_true')
    parser.add_argument('--evidence-dir', type=Path)
    args = parser.parse_args()
    evidence = args.evidence_dir.resolve() if args.evidence_dir else ROOT / 'Unreal/Memoria/Saved/Validation' / ('ue57-' + datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%S%f'))
    if evidence.is_relative_to(ROOT / 'docs/unreal-migration/evidence/phase1a'):
        parser.error('Phase 1A evidence is immutable; choose a new output directory')
    candidates = []
    parents_checked = []
    for name in ('UE57_ROOT', 'UE_ENGINE_ROOT'):
        if os.environ.get(name):
            candidates.append(Path(os.environ[name]))
    for directory in (Path('C:/Program Files/Epic Games'), Path('D:/Epic Games'), Path('G:/Epic Games')):
        parents_checked.append({'path': str(directory), 'exists': directory.is_dir()})
        if directory.is_dir():
            candidates.extend(directory.glob('UE_*'))
    launcher = Path(os.environ.get('PROGRAMDATA', 'C:/ProgramData')) / 'Epic/UnrealEngineLauncher/LauncherInstalled.dat'
    if launcher.is_file():
        for item in json.loads(launcher.read_text(encoding='utf-8-sig')).get('InstallationList', []):
            if item.get('AppName', '').startswith('UE_'):
                candidates.append(Path(item['InstallLocation']))
    registry_checked = []
    for hive, key_name in ((winreg.HKEY_CURRENT_USER, r'Software\Epic Games\Unreal Engine\Builds'),
                           (winreg.HKEY_LOCAL_MACHINE, r'SOFTWARE\EpicGames\Unreal Engine\5.7')):
        registry_checked.append(key_name)
        try:
            with winreg.OpenKey(hive, key_name) as key:
                for index in range(winreg.QueryInfoKey(key)[1]):
                    name, value, kind = winreg.EnumValue(key, index)
                    if kind == winreg.REG_SZ and (name == 'InstalledDirectory' or hive == winreg.HKEY_CURRENT_USER):
                        candidates.append(Path(value))
        except FileNotFoundError:
            pass
    if args.engine_root:
        candidates.insert(0, args.engine_root)
    detected = []
    for path in dict.fromkeys(str(p.resolve()) for p in candidates):
        version_file = Path(path) / 'Engine/Build/Build.version'
        record = {'path': path, 'build_version_present': version_file.is_file()}
        if version_file.is_file():
            record['version'] = json.loads(version_file.read_text(encoding='utf-8-sig'))
        detected.append(record)
    eligible = [d for d in detected if d.get('version', {}).get('MajorVersion') == 5 and d.get('version', {}).get('MinorVersion') == 7]
    if args.engine_root:
        eligible = [d for d in eligible if Path(d['path']) == args.engine_root.resolve()]
    report = {'utc': datetime.datetime.now(datetime.timezone.utc).isoformat(), 'requested_engine': '5.7',
              'detected': detected, 'registry_checked': registry_checked, 'launcher_checked': str(launcher),
              'installation_parents_checked': parents_checked,
              'engine_environment': {name: os.environ.get(name) for name in ('UE57_ROOT', 'UE_ENGINE_ROOT')},
              'scope': 'Launcher, known installation parents, registered builds, UE57_ROOT/UE_ENGINE_ROOT, explicit root; no whole-disk scan',
              'ue_compilation': 'NOT_RUN', 'editor_launch': 'NOT_RUN', 'ue_automation': 'NOT_RUN', 'commands': []}
    evidence.mkdir(parents=True, exist_ok=True)
    WORK.mkdir(parents=True, exist_ok=True)

    def save():
        (evidence / 'ue57_validation.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')

    if not eligible:
        report['status'] = 'BLOCKED_MISSING_UE_5_7'
        save()
        print('MEMORIA_UE57_BLOCKED: no UE 5.7 toolchain in inspected locations; no build/editor/Automation executed')
        return 2
    engine = Path(eligible[0]['path']) / 'Engine'
    build = engine / 'Build/BatchFiles/Build.bat'
    editor = engine / 'Binaries/Win64/UnrealEditor-Cmd.exe'
    report['selected_engine'] = str(engine.parent)
    if not build.is_file() or not editor.is_file():
        report['status'] = 'BLOCKED_INCOMPLETE_UE_5_7'
        save()
        print('MEMORIA_UE57_BLOCKED: missing Build.bat or UnrealEditor-Cmd.exe in exact 5.7 installation')
        return 2
    if not args.build_and_test:
        report['status'] = 'UE_5_7_AVAILABLE_NOT_EXECUTED'
        save()
        print('MEMORIA_UE57_FOUND; run with --build-and-test to validate')
        return 0

    def run(name, command, timeout):
        entry = {'name': name, 'command': command}
        report['commands'].append(entry)
        log_path = WORK / f'{name}.log'
        entry['log'] = str(log_path.relative_to(ROOT))
        with log_path.open('w', encoding='utf-8') as log:
            process = subprocess.Popen(command, cwd=ROOT, stdout=log, stderr=subprocess.STDOUT)
            try:
                entry['exit_code'] = process.wait(timeout=timeout)
            except subprocess.TimeoutExpired:
                subprocess.run(['taskkill', '/PID', str(process.pid), '/T', '/F'], capture_output=True)
                process.wait()
                entry['timed_out'] = True
                entry['exit_code'] = process.returncode
        output = log_path.read_text(encoding='utf-8', errors='replace')
        entry['fatal_diagnostics'] = re.findall(r'(?im)^.*(?:fatal error|Fatal error:|Assertion failed:|Unhandled Exception:).*$' , output)
        return entry['exit_code'] == 0 and not entry.get('timed_out') and not entry['fatal_diagnostics']

    project = str(ROOT / 'Unreal/Memoria/Memoria.uproject')
    # Reject altered or stale attested fixtures before launching the toolchain.
    if not run('fixture_check', [sys.executable, str(ROOT / 'Unreal/Tools/generate_memory_test_header.py'), '--check'], 60):
        report['status'] = 'FIXTURE_VALIDATION_FAILED'
        save()
        return 1
    report['ue_compilation'] = 'RUNNING'
    save()
    if not run('editor_build', [str(build), 'MemoriaEditor', 'Win64', 'Development', f'-Project={project}', '-WaitMutex', '-NoHotReloadFromIDE'], 3600):
        report.update(status='BUILD_FAILED', ue_compilation='FAIL')
        save()
        return 1
    report['ue_compilation'] = 'PASS'
    # Unique report path prevents a previous green report from masking zero tests.
    test_report = WORK / ('Automation-' + datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%S%f'))
    report['ue_automation'] = 'RUNNING'
    report['editor_launch'] = 'UNATTENDED_CMD'
    save()
    ran = run('automation', [str(editor), project, '-unattended', '-nop4', '-nosplash', '-NullRHI',
                            '-ExecCmds=Automation RunTests Memoria.', '-TestExit=Automation Test Queue Empty',
                            f'-ReportExportPath={test_report}', '-stdout', '-FullStdOutLogOutput'], 900)
    index = test_report / 'index.json'
    try:
        result = json.loads(index.read_text(encoding='utf-8-sig')) if index.is_file() else {}
        inspection = inspect_automation_report(result, expected_test_paths())
    except (ValueError, OSError) as error:
        inspection = {'passed': False, 'discovered': 0, 'source_parity_discovered': 0, 'errors': [str(error)]}
    report['automation_report'] = str(index.relative_to(ROOT))
    report['automation_discovered'] = inspection['discovered']
    report['source_parity_discovered'] = inspection['source_parity_discovered']
    report['automation_validation_errors'] = inspection['errors']
    passed = ran and inspection['passed']
    report.update(status='PASS' if passed else 'AUTOMATION_FAILED', ue_automation='PASS' if passed else 'FAIL')
    save()
    print(f"MEMORIA_UE57_{report['status']} discovered={inspection['discovered']} source_parity={inspection['source_parity_discovered']}")
    return 0 if passed else 1


if __name__ == '__main__':
    sys.exit(main())
