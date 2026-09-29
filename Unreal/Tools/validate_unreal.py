"""Probe exact UE 5.8.2; optionally build Editor and run real Automation tests.

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
REQUIRED_VERSION = (5, 8, 2)
WORK = ROOT / 'Unreal/Memoria/Saved/Validation' / ('unreal-run-' + datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%S%f'))


def matches_required_version(version):
    return tuple(version.get(key) for key in ('MajorVersion', 'MinorVersion', 'PatchVersion')) == REQUIRED_VERSION


def expected_test_paths(include_runtime=True, include_catalog=True):
    fixtures = json.loads((ROOT / 'docs/unreal-migration/fixtures/player_memory_inputs.json').read_text(encoding='utf-8'))['cases']
    paths = ['Memoria.Memory.SourceParity.' + case['id'] for case in fixtures]
    if len(paths) != 51 or len(set(paths)) != 51:
        raise ValueError('Phase 1B requires the 51 distinct attested memory fixtures')
    legacy = set(paths) | {
        'Memoria.Foundation.MemoryAdapter',
        'Memoria.Foundation.SaveAndDialectBoundaries',
        'Memoria.Foundation.RunOwnership',
    }
    previous = legacy | ({'Memoria.Foundation.RuntimeLifetime', 'Memoria.Foundation.NondefaultSaveRoundTrip', 'Memoria.Foundation.MapInputAndModal'} if include_runtime else set())
    return previous | ({'Memoria.Content.StartingCatalogParity', 'Memoria.Content.StartingCatalogRuntime', 'Memoria.Content.StartingCatalogValidation'} if include_runtime and include_catalog else set())


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


def narrative_test_paths():
    return {'Memoria.Narrative.'+name for name in (
        'VNImportParity', 'FieldImportParity', 'DeterministicReimport',
        'FieldExecutionContract', 'VNExecutionContract', 'OriginalVisibleChoiceIndices',
        'ContinuationDTO', 'StrictValidationAndSemanticChange',
        'BurnUsesRunContext', 'StillHandsOath', 'AddItemSourceRules', 'ClassifierReport')}


def presentation_test_paths():
    return {'Memoria.Presentation.'+name for name in (
        'ArrelGaitBonesResolved', 'ArrelRigWithoutGaitBones', 'PlaceholderIdentification')}


def audio_test_paths():
    return {'Memoria.Audio.'+name for name in ('CatalogAssets', 'Routing', 'RunReplacement')}



def world_seed_test_paths():
    return {"Memoria.WorldSeed.Source."+n for n in ("fresh","knowledge_only","memory_only","both","removed","forgotten","actor_missing","repeat","already_true","save_restore","restored","flag_false","case_sensitive")} | {"Memoria.WorldSeed."+n for n in ("NativeContract","IdentityContract","SaveRoundTrip","RunReplacementIsolation","Canonical")}

def potion_test_paths():
    from export_malet_potion_oracle import inputs
    return {"Memoria.Potion.Source."+c['id'] for c in inputs()} | {"Memoria.Potion."+n for n in ("NativeInventoryContract","CanonicalPotionGrant","ReplacementBeforePotion","ReplacementAfterCommit","ReplacementAtStop","WorldTeardownAfterCommit")}

def antidote_test_paths():
    from export_malet_antidote_oracle import inputs
    return {"Memoria.Antidote.Source."+c["id"] for c in inputs()} | {"Memoria.Antidote."+n for n in ("NativeScopeAndPresentation","Canonical","PotionSignalReplacement","AntidoteSignalReplacement","ReplacementAtStop","WorldTeardownAtStop")}

def firebomb_test_paths():
    from export_malet_firebomb_oracle import inputs
    return {"Memoria.Firebomb.Source."+c["id"] for c in inputs()} | {"Memoria.Firebomb."+n for n in ("NativeScopeAndPresentation","Canonical","FirebombSignalReplacement","ReplacementAtStop","WorldTeardownAtStop")}

def shop_test_paths():
    from export_malet_shop_oracle import inputs
    return {"Memoria.Shop.Source."+c["id"] for c in inputs()} | {"Memoria.Shop.OwnerLifetime", "Memoria.Shop.Canonical"}


def shop_transaction_test_paths():
    from export_shop_transactions_oracle import inputs
    return {"Memoria.ShopTransactions.Source."+c["id"] for c in inputs()} | {"Memoria.ShopTransactions.Guards", "Memoria.ShopTransactions.Canonical", "Memoria.ShopTransactions.RequestCancellation"}


def checkpoint_test_paths():
    return {'Memoria.Checkpoint.'+name for name in ('DiskRoundTrip','BackupRecovery','RejectedSnapshots','WriteFailureRetry','SyntheticIsolation','Canonical')}


def checkpoint_process_paths():
    return {'MemoriaCheckpointProcess.Read','MemoriaCheckpointProcess.Startup.Continue','MemoriaCheckpointProcess.Startup.Missing'}


def archive_test_paths():
    return {'Memoria.Archive.'+n for n in ('RenderedInputFlow','WidgetReadOnly')} | {'Memoria.Archive.Source.'+n for n in ('initial_en','initial_ko','states_en','states_ko','grade5','grade3','grade1','empty')}


def battle_core_test_paths():
    from export_battle_core_oracle import inputs
    return {'Memoria.BattleCore.Source.'+case['id'] for case in inputs()} | {'Memoria.BattleCore.RenderedJourney','Memoria.BattleCore.OwnerLifetime','Memoria.BattleCore.StagePresentation'}


def battle_entry_test_paths():
    from export_battle_entry_oracle import inputs
    # Phase-step/Witness approach probes characterize source only; the playable
    # native revisit currently supports the original neutral ambient entry.
    paths = {'Memoria.BattleEntry.Source.'+case['id'] for case in inputs() if case['entry_mode']=='neutral'}
    return paths | {'Memoria.BattleEntry.'+name for name in ('EncounterDistance','OwnerLifetime','SavedStats','RenderedRevisitFlow')}


def chapter1_test_paths():
    from narrative_ir import VN_CASES
    from export_chapter1_oracle import inputs
    return {'Memoria.Chapter1.ImportParity.'+k for k in VN_CASES if k.startswith('ch1_')} | {'Memoria.Chapter1.RouteChain'} | {'Memoria.Chapter1.OracleRoute.'+c['id'] for c in inputs()} \
        | {'Memoria.Chapter1.NewGameHost.'+c['id'] for c in inputs()} | {'Memoria.Chapter1.AutosaveResume'}


def title_test_paths():
    return {'Memoria.Title.' + n for n in ('ContinueSource', 'Settings', 'Menu')}


def verdan_story_test_paths():
    return {'Memoria.VerdanStory.SourceTable', 'Memoria.VerdanStory.BurnedTextSubstitution'}


def visual_test_paths():
    return {"MemoriaVisual.ArtworkCoverage", "MemoriaVisual.DialogueInteraction", "MemoriaVisual.VerdanExploration",
            "MemoriaVisual.Chapter1Presentation", "MemoriaVisual.Chapter1Journey", "MemoriaVisual.FieldCharacters",
            "MemoriaVisual.TitleScreen", "MemoriaVisual.FieldCombat", "MemoriaVisual.FieldBurn", "MemoriaVisual.FieldFoes", "MemoriaVisual.FieldRewards", "MemoriaVisual.FieldFeel", "MemoriaVisual.PauseMenu", "MemoriaVisual.GameOver", "MemoriaVisual.EliaSkills", "MemoriaVisual.Chapter3", "MemoriaVisual.Chapter3Travel", "MemoriaVisual.Chapter4", "MemoriaVisual.Chapter5", "MemoriaVisual.TutorialHints"}


def current_test_paths():
    return expected_test_paths() | narrative_test_paths() | slice_test_paths() | malet_test_paths() | malet_refusal_test_paths() | malet_deal_test_paths() | malet_reward_test_paths() | malet_first_effect_test_paths() | world_seed_test_paths() | potion_test_paths() | antidote_test_paths() | firebomb_test_paths() | shop_test_paths() | shop_transaction_test_paths() | checkpoint_test_paths() | archive_test_paths() | battle_entry_test_paths() | battle_core_test_paths() | presentation_test_paths() | audio_test_paths() | verdan_story_test_paths() | chapter1_test_paths() | title_test_paths()


def malet_first_effect_test_paths():
    return {"Memoria.MaletFirstEffect."+n for n in ("AuthoritativeFlagContract","Canonical","PreexistingTrue","PreexistingFalse","CancelBeforeCallbackOnRunReplace","CancelBeforeFlagOnRunReplace","CancelAfterFlagOnRunReplace","CancelBeforeCallbackOnWorldTeardown","CancelBeforeFlagOnWorldTeardown","CancelAfterFlagOnWorldTeardown")}

def malet_reward_test_paths():
    return {"Memoria.MaletReward."+n for n in ("ImportContract","EnglishKoreanExecution","CanonicalReward","CancelRewardFieldOnRunReplace","CancelRewardFieldOnWorldTeardown","CancelRewardBoundaryOnRunReplace","CancelRewardBoundaryOnWorldTeardown")}


def malet_deal_test_paths():
    return {"Memoria.MaletDeal."+n for n in ("ImportContract","PaymentEdgeCases","CanonicalPayment","AlreadyBurnedPayment","CancelNormalOnRunReplace","CancelRewardOnRunReplace","CancelNormalOnWorldTeardown","CancelRewardOnWorldTeardown")}


def malet_refusal_test_paths():
    return {"Memoria.MaletRefusal."+n for n in ("ImportContract","ChoiceEffects","RepeatCache","CanonicalRefusalRetry","AcceptPreEffectDeferred","CallbackCancellation")}


def malet_test_paths():
    return {'Memoria.Malet.'+name for name in ('ImportContract', 'SourceDispatch', 'CanonicalInteraction', 'IntactInteraction')}


def slice_test_paths():
    return {'Memoria.Campaign.'+name for name in (
        'CanonicalPaidRoute', 'FilteredOriginalChoice', 'UnseenFieldRoute', 'HostContinuation')}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--engine-root', type=Path)
    parser.add_argument('--build-and-test', action='store_true')
    parser.add_argument('--build-only', action='store_true')
    parser.add_argument('--create-foundation-assets', action='store_true')
    parser.add_argument('--rendered', action='store_true')
    parser.add_argument('--test-prefix', default='Memoria.', choices=['Memoria.', 'Memoria.Chapter1.', 'Memoria.Title.', 'Memoria.VerdanStory.', 'Memoria.Shop.', 'Memoria.ShopTransactions.', 'Memoria.Checkpoint.', 'Memoria.Archive.', 'Memoria.BattleEntry.', 'Memoria.BattleCore.', 'MemoriaCheckpointProcess.', 'Memoria.Campaign.', 'Memoria.Malet.', 'Memoria.Foundation.', 'MemoriaVisual.', 'Memoria.Archive.+MemoriaVisual.+Memoria.Checkpoint.+Memoria.ShopTransactions.+Memoria.Shop.+Memoria.Campaign.', 'Memoria.BattleEntry.+Memoria.Archive.+MemoriaVisual.+Memoria.Checkpoint.+Memoria.ShopTransactions.+Memoria.Shop.+Memoria.Campaign.', 'Memoria.Narrative.+Memoria.Presentation.+Memoria.Campaign.+MemoriaVisual.', 'Memoria.Audio.+Memoria.Campaign.+MemoriaVisual.+Memoria.BattleEntry.', 'Memoria.Audio.+Memoria.Campaign.+MemoriaVisual.+Memoria.BattleEntry.+Memoria.ShopTransactions.'], help='Exact full registry or a bounded gameplay regression subset')
    parser.add_argument('--evidence-dir', type=Path)
    # The rendered full registry needs far longer than a bounded subset.
    parser.add_argument('--automation-timeout', type=int, default=900, help='Seconds before the automation process is killed')
    args = parser.parse_args()
    if args.automation_timeout <= 0:
        parser.error('--automation-timeout must be positive')
    evidence = args.evidence_dir.resolve() if args.evidence_dir else ROOT / 'Unreal/Memoria/Saved/Validation' / ('unreal-' + datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%S%f'))
    if any(evidence.is_relative_to(ROOT / 'docs/unreal-migration/evidence' / phase) for phase in ('phase0', 'phase1a', 'phase1b', 'phase1b-ue58')):
        parser.error('Historical evidence is immutable; choose a new output directory')
    candidates = []
    parents_checked = []
    for name in ('UE_ENGINE_ROOT',):
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
                           (winreg.HKEY_LOCAL_MACHINE, r'SOFTWARE\EpicGames\Unreal Engine\5.8')):
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
    eligible = [d for d in detected if matches_required_version(d.get('version', {}))]
    if args.engine_root:
        eligible = [d for d in eligible if Path(d['path']) == args.engine_root.resolve()]
    report = {'utc': datetime.datetime.now(datetime.timezone.utc).isoformat(), 'requested_engine': '5.8.2',
              'detected': detected, 'registry_checked': registry_checked, 'launcher_checked': str(launcher),
              'installation_parents_checked': parents_checked,
              'engine_environment': {name: os.environ.get(name) for name in ('UE_ENGINE_ROOT',)},
              'scope': 'Launcher, known installation parents, registered builds, UE_ENGINE_ROOT, explicit root; no whole-disk scan',
              'ue_compilation': 'NOT_RUN', 'editor_launch': 'NOT_RUN', 'ue_automation': 'NOT_RUN', 'commands': []}
    evidence.mkdir(parents=True, exist_ok=True)
    WORK.mkdir(parents=True, exist_ok=True)

    def save():
        (evidence / 'unreal_validation.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')

    if not eligible:
        report['status'] = 'BLOCKED_MISSING_UE_5_8_2'
        save()
        print('MEMORIA_UNREAL_BLOCKED: no UE 5.8.2 toolchain in inspected locations; no build/editor/Automation executed')
        return 2
    engine = Path(eligible[0]['path']) / 'Engine'
    build = engine / 'Build/BatchFiles/Build.bat'
    editor = engine / 'Binaries/Win64/UnrealEditor-Cmd.exe'
    report['selected_engine'] = str(engine.parent)
    if not build.is_file() or not editor.is_file():
        report['status'] = 'BLOCKED_INCOMPLETE_UE_5_8_2'
        save()
        print('MEMORIA_UNREAL_BLOCKED: missing Build.bat or UnrealEditor-Cmd.exe in exact 5.8.2 installation')
        return 2
    if not args.build_and_test and not args.build_only:
        report['status'] = 'UE_5_8_2_AVAILABLE_NOT_EXECUTED'
        save()
        print('MEMORIA_UNREAL_FOUND; run with --build-and-test to validate')
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
    # Rendered SFX/ambience sources must match the Godot-formula generator byte for byte.
    if not run('audio_source_check', [sys.executable, str(ROOT / 'Unreal/Tools/generate_audio_sources.py'), '--check'], 300):
        report['status'] = 'FIXTURE_VALIDATION_FAILED'
        save()
        return 1
    # Verdan story beats must still match the pinned Godot trigger table.
    if not run('verdan_story_check', [sys.executable, str(ROOT / 'Unreal/Tools/export_verdan_story_triggers.py'), '--check'], 60):
        report['status'] = 'FIXTURE_VALIDATION_FAILED'
        save()
        return 1
    report['ue_compilation'] = 'RUNNING'
    save()
    if not run('editor_build', [str(build), 'MemoriaEditor', 'Win64', 'Development', f'-Project={project}', '-WaitMutex', '-NoHotReloadFromIDE', '-MaxParallelActions=2'], 3600):
        report.update(status='BUILD_FAILED', ue_compilation='FAIL')
        save()
        return 1
    report['ue_compilation'] = 'PASS'
    if args.build_only:
        report['status'] = 'BUILD_PASS_AUTOMATION_NOT_RUN'
        save()
        print('MEMORIA_UNREAL_BUILD_PASS_AUTOMATION_NOT_RUN')
        return 0
    if args.create_foundation_assets:
        authored = run('foundation_authoring', [str(editor), project, '-run=MemoriaFoundationAssets', '-unattended', '-nop4', '-NullRHI', '-stdout', '-FullStdOutLogOutput'], 600)
        report['foundation_authoring'] = 'PASS' if authored else 'FAIL'
        if not authored:
            report['status'] = 'AUTHORING_FAILED'
            save()
            return 1
    # Unique report path prevents a previous green report from masking zero tests.
    test_report = WORK / ('Automation-' + datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%S%f'))
    report['ue_automation'] = 'RUNNING'
    report['editor_launch'] = 'UNATTENDED_CMD'
    save()
    ran = run('automation', [str(editor), project, '-unattended', '-nop4', '-nosplash', *(['-RenderOffscreen', '-MemoriaCapture', '-ResX=1280', '-ResY=720'] if args.rendered else ['-NullRHI']),
                            f'-ExecCmds=Automation RunTests {args.test_prefix}', '-TestExit=Automation Test Queue Empty',
                            f'-ReportExportPath={test_report}', '-stdout', '-FullStdOutLogOutput'], args.automation_timeout)
    index = test_report / 'index.json'
    try:
        result = json.loads(index.read_text(encoding='utf-8-sig')) if index.is_file() else {}
        inspection = inspect_automation_report(result, {p for p in current_test_paths() | visual_test_paths() | checkpoint_process_paths() if any(p.startswith(prefix) for prefix in args.test_prefix.split("+"))})
    except (ValueError, OSError) as error:
        inspection = {'passed': False, 'discovered': 0, 'source_parity_discovered': 0, 'errors': [str(error)]}
    report['automation_report'] = str(index.relative_to(ROOT))
    report['automation_discovered'] = inspection['discovered']
    report['source_parity_discovered'] = inspection['source_parity_discovered']
    report['legacy_required_total'] = len(expected_test_paths(include_runtime=False))
    report['phase1b_required_total'] = len(expected_test_paths(include_catalog=False))
    report['phase1c_required_total'] = len(expected_test_paths() - expected_test_paths(include_catalog=False))
    report['phase1d_required_total'] = len(narrative_test_paths())
    report['phase1e_required_total'] = len(slice_test_paths())
    report['phase1f_required_total'] = len(malet_test_paths())
    report['phase1h_required_total'] = len(malet_deal_test_paths())
    report['phase1i_required_total'] = len(malet_reward_test_paths())
    report['phase1g_required_total'] = len(malet_refusal_test_paths())
    report['current_required_total'] = len(current_test_paths())
    report['test_prefix'] = args.test_prefix
    report['selected_required_total'] = sum(any(p.startswith(prefix) for prefix in args.test_prefix.split("+")) for p in current_test_paths() | visual_test_paths() | checkpoint_process_paths())
    report['rendered'] = args.rendered
    report['automation_timeout_seconds'] = args.automation_timeout
    report['automation_validation_errors'] = inspection['errors']
    passed = ran and inspection['passed']
    report.update(status='PASS' if passed else 'AUTOMATION_FAILED', ue_automation='PASS' if passed else 'FAIL')
    save()
    print(f"MEMORIA_UNREAL_{report['status']} discovered={inspection['discovered']} source_parity={inspection['source_parity_discovered']}")
    return 0 if passed else 1


if __name__ == '__main__':
    sys.exit(main())
