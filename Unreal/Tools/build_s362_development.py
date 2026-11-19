"""Build/cook/archive one Development candidate and retain its real provenance.

This records saved execution inputs. It does not establish that a pre-existing
binary was compiled from a particular source hash, or certify Shipping.
"""
from pathlib import Path
import argparse
import datetime
import hashlib
import importlib.util
import json
import os
import re
import subprocess
import sys
import time

sys.dont_write_bytecode = True
TIMEOUT_SECONDS = 3600
ARCHIVE_SUFFIXES = {'.exe', '.pak', '.utoc', '.ucas', '.dll'}


def sha256_file(path):
    digest = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            digest.update(block)
    return digest.hexdigest()


def write_json(path, value):
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2), encoding='utf-8')


def archive_hashes(archive):
    """Refuse links; never follow an archive junction into unrelated files."""
    hashes = {}
    if not archive.is_dir():
        return hashes
    for parent, directories, files in os.walk(archive, followlinks=False):
        for name in directories + files:
            path = Path(parent) / name
            if path.is_symlink() or getattr(path.stat(), 'st_file_attributes', 0) & 0x400:
                raise ValueError('Archive contains a link/reparse point: ' + str(path))
        for name in files:
            path = Path(parent) / name
            if path.suffix.lower() in ARCHIVE_SUFFIXES and path.is_file():
                hashes[path.relative_to(archive).as_posix()] = sha256_file(path)
    return dict(sorted(hashes.items()))


def load_input_recorder(root):
    path = root / 'Unreal/Tools/run_s362_native.py'
    spec = importlib.util.spec_from_file_location('memoria_s362_native_inputs', path)
    if spec is None or spec.loader is None:
        raise RuntimeError('Cannot load the actual native execution-input recorder')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument('--engine-root', type=Path, default=Path('C:/Program Files/Epic Games/UE_5.8'))
    parser.add_argument('--archive', type=Path, required=True)
    parser.add_argument('--evidence', type=Path, required=True)
    args = parser.parse_args()
    root, engine = args.root.resolve(), args.engine_root.resolve()
    archive, evidence = args.archive.resolve(), args.evidence.resolve()
    project = root / 'Unreal/Memoria/Memoria.uproject'
    uat = engine / 'Engine/Build/BatchFiles/RunUAT.bat'
    boundary = root / 'Unreal/Memoria/Saved/Validation'
    if not project.is_file() or not uat.is_file():
        parser.error('Require an actual Unreal project root and installed RunUAT.bat')
    if archive.exists() or evidence.exists():
        parser.error('Archive and evidence outputs must both be fresh')
    if not archive.is_relative_to(boundary) or not evidence.is_relative_to(boundary):
        parser.error('Archive and evidence must be inside this project Saved/Validation')
    if archive == boundary or evidence == boundary:
        parser.error('Use dedicated children of Saved/Validation')
    if archive.is_relative_to(evidence) or evidence.is_relative_to(archive):
        parser.error('Archive and evidence must be separate, non-nested directories')
    if os.name != 'nt':
        parser.error('The Win64 RunUAT batch build requires Windows')

    evidence.mkdir(parents=True)
    receipt = evidence / 'result.json'
    command = [str(uat), 'BuildCookRun', '-project=' + str(project), '-noP4',
               '-platform=Win64', '-clientconfig=Development', '-build', '-cook',
               '-stage', '-pak', '-archive', '-archivedirectory=' + str(archive),
               '-unattended', '-utf8output', '-NoXGE', '-UbtArgs=-MaxParallelActions=2']
    started = time.monotonic()
    result = {'utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
              'configuration': 'Development', 'command': command,
              'archive': str(archive), 'status': 'PREPARING', 'exit_code': None,
              'timeout_seconds': TIMEOUT_SECONDS,
              'tool_sha256': sha256_file(Path(__file__).resolve()),
              'provenance_limit': 'Saved source/config/content/art source/fixtures/tools '
                                  'and DLL/module hashes; no independent compilation-provenance '
                                  'or Shipping certification.'}
    write_json(receipt, result)
    errors = []
    recorder = None
    before = None
    try:
        recorder = load_input_recorder(root)
        before = recorder.execution_inputs(root)
        write_json(evidence / 'execution_inputs_before.json', before)
        result['execution_input_count_before'] = len(before)
        result['status'] = 'RUNNING'
        write_json(receipt, result)
        startup = subprocess.STARTUPINFO()
        startup.dwFlags |= subprocess.STARTF_USESHOWWINDOW
        startup.wShowWindow = 0
        process_started = time.monotonic()
        with (evidence / 'uat.log').open('wb') as output:
            process = subprocess.Popen(command, cwd=root, stdout=output,
                                       stderr=subprocess.STDOUT, startupinfo=startup,
                                       creationflags=subprocess.CREATE_NO_WINDOW)
            result['pid'] = process.pid
            write_json(receipt, result)
            try:
                result['exit_code'] = process.wait(timeout=TIMEOUT_SECONDS)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()
                result['timeout'] = True
                result['wrapper_exit_after_timeout'] = process.returncode
                result['timeout_cleanup_limit'] = (
                    'The owned wrapper was terminated. The coordinator must inspect '
                    'owned descendant engine/compiler processes before another build.')
                errors.append('UAT exceeded the 3600-second deadline')
        result['uat_elapsed_seconds'] = round(time.monotonic() - process_started, 3)
    except (OSError, RuntimeError, ValueError, ImportError) as error:
        errors.append('Build preparation/launch failed: ' + str(error))

    if recorder is not None and before is not None:
        try:
            after = recorder.execution_inputs(root)
            write_json(evidence / 'execution_inputs_after.json', after)
            differences = recorder.changed_inputs(before, after)
            result['execution_input_changes'] = differences
            blocked = {kind: [path for path in paths
                              if not path.startswith('Unreal/Memoria/Binaries/')]
                       for kind, paths in differences.items()}
            result['source_content_fixture_tool_changes'] = blocked
            result['source_content_fixture_tools_unchanged'] = not any(blocked.values())
            result['allowed_change_scope'] = 'Unreal/Memoria/Binaries/ only'
            if any(blocked.values()):
                errors.append('Source/content/fixtures/tools changed during build/cook')
        except (OSError, ValueError) as error:
            errors.append('Cannot verify post-build execution inputs: ' + str(error))

    log_path = evidence / 'uat.log'
    log = log_path.read_text(encoding='utf-8-sig', errors='replace') if log_path.is_file() else ''
    fatal = re.findall(r'(?im)^.*(?:fatal error|Assertion failed:|Unhandled Exception:|BUILD FAILED).*$', log)
    result['fatal_diagnostics'] = fatal
    if result['exit_code'] != 0:
        errors.append('Nonzero UAT exit or no completed UAT process')
    if fatal:
        errors.append('Fatal build/cook diagnostics')
    if 'BUILD SUCCESSFUL' not in log:
        errors.append('UAT success marker missing')
    executable = archive / 'Windows/Memoria/Binaries/Win64/Memoria.exe'
    result['inner_executable'] = str(executable)
    if not executable.is_file():
        errors.append('Actual inner packaged executable is missing')
    try:
        artifacts = archive_hashes(archive)
        write_json(evidence / 'archive_hashes.json', artifacts)
        result['archive_artifact_count'] = len(artifacts)
        if executable.is_file():
            result['inner_executable_sha256'] = artifacts[executable.relative_to(archive).as_posix()]
    except (OSError, ValueError, KeyError) as error:
        errors.append('Cannot verify archive artifacts: ' + str(error))
    result.update(status='PASS' if not errors else 'FAIL', validation_errors=errors,
                  elapsed_seconds=round(time.monotonic() - started, 3))
    write_json(receipt, result)
    print(json.dumps({key: result[key] for key in
                      ('status', 'exit_code', 'validation_errors', 'elapsed_seconds')}, ensure_ascii=False))
    return 0 if not errors else 1


if __name__ == '__main__':
    raise SystemExit(main())
