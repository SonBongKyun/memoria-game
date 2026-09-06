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
EVIDENCE = ROOT / 'docs/unreal-migration/evidence/phase1a'
WORK = ROOT / 'Unreal/Memoria/Intermediate/UE57Validation'


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--engine-root', type=Path)
    parser.add_argument('--build-and-test', action='store_true')
    args = parser.parse_args()
    candidates = []
    for name in ('UE57_ROOT', 'UE_ENGINE_ROOT'):
        if os.environ.get(name):
            candidates.append(Path(os.environ[name]))
    for directory in (Path('C:/Program Files/Epic Games'), Path('D:/Epic Games'), Path('G:/Epic Games')):
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
              'scope': 'Launcher, known installation parents, registered builds, UE57_ROOT/UE_ENGINE_ROOT, explicit root; no whole-disk scan',
              'ue_compilation': 'NOT_RUN', 'editor_launch': 'NOT_RUN', 'ue_automation': 'NOT_RUN', 'commands': []}
    EVIDENCE.mkdir(parents=True, exist_ok=True)
    WORK.mkdir(parents=True, exist_ok=True)

    def save():
        (EVIDENCE / 'ue57_validation.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')

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
    result = json.loads(index.read_text(encoding='utf-8-sig')) if index.is_file() else {}
    tests = result.get('tests', [])
    source_tests = [t for t in tests if t.get('fullTestPath', '').startswith('Memoria.Memory.SourceParity.')]
    expected = len(json.loads((ROOT / 'docs/unreal-migration/fixtures/player_memory_inputs.json').read_text(encoding='utf-8'))['cases'])
    report['automation_report'] = str(index.relative_to(ROOT))
    report['automation_discovered'] = len(tests)
    report['source_parity_discovered'] = len(source_tests)
    passed = ran and len(source_tests) == expected and len(tests) == expected + 3 and all(t.get('state') == 'Success' for t in tests)
    report.update(status='PASS' if passed else 'AUTOMATION_FAILED', ue_automation='PASS' if passed else 'FAIL')
    save()
    print(f"MEMORIA_UE57_{report['status']} discovered={len(tests)} source_parity={len(source_tests)}")
    return 0 if passed else 1


if __name__ == '__main__':
    sys.exit(main())
