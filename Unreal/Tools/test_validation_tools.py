"""Host-side validator regression tests. These do not execute Unreal tests."""
from copy import deepcopy
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import unittest

from validate_unreal import ROOT, expected_test_paths, inspect_automation_report, matches_required_version


class AutomationReportTests(unittest.TestCase):
    def setUp(self):
        self.expected = expected_test_paths(include_runtime=False)
        self.report = {'tests': [{'fullTestPath': path, 'state': 'Success'} for path in sorted(self.expected)], 'failed': 0}

    def inspect(self, report):
        return inspect_automation_report(report, self.expected)

    def test_exact_54_identities_succeed(self):
        result = self.inspect(self.report)
        self.assertTrue(result['passed'])
        self.assertEqual((result['discovered'], result['source_parity_discovered']), (54, 51))

    def test_all_expanded_tests_are_required(self):
        expected = expected_test_paths()
        self.assertEqual(len(expected_test_paths(include_catalog=False)), 57)
        self.assertEqual(len(expected), 60)
        self.assertTrue(self.expected < expected)
        report = {'tests': [{'fullTestPath': p, 'state': 'Success'} for p in sorted(expected)], 'failed': 0}
        self.assertTrue(inspect_automation_report(report, expected)['passed'])
        self.assertFalse(inspect_automation_report(self.report, expected)['passed'])
        report['tests'].append(report['tests'][0])
        self.assertFalse(inspect_automation_report(report, expected)['passed'])

    def test_exact_patch_version_is_required(self):
        self.assertTrue(matches_required_version(dict(MajorVersion=5, MinorVersion=8, PatchVersion=2)))
        for major, minor, patch in ((5, 8, 1), (5, 8, 3), (5, 7, 2), (5, 9, 2), (6, 8, 2)):
            self.assertFalse(matches_required_version(dict(MajorVersion=major, MinorVersion=minor, PatchVersion=patch)))
        self.assertFalse(matches_required_version({}))

    def test_duplicate_memory_case_cannot_replace_missing_case(self):
        changed = deepcopy(self.report)
        memory = [i for i, t in enumerate(changed['tests']) if '.SourceParity.' in t['fullTestPath']]
        changed['tests'][memory[1]] = deepcopy(changed['tests'][memory[0]])
        # Demonstrate the former count-only predicate accepted this bad report.
        self.assertEqual(sum('.SourceParity.' in t['fullTestPath'] for t in changed['tests']), 51)
        self.assertEqual(len(changed['tests']), 54)
        self.assertTrue(all(t['state'] == 'Success' for t in changed['tests']))
        self.assertFalse(self.inspect(changed)['passed'])

    def test_unknown_foundation_case_cannot_replace_required_case(self):
        self.report['tests'][0]['fullTestPath'] = 'Memoria.Foundation.Unrelated'
        self.assertFalse(self.inspect(self.report)['passed'])

    def test_missing_case_fails(self):
        self.report['tests'].pop()
        self.assertFalse(self.inspect(self.report)['passed'])

    def test_extra_case_fails(self):
        self.report['tests'].append({'fullTestPath': 'Memoria.Unexpected', 'state': 'Success'})
        self.assertFalse(self.inspect(self.report)['passed'])

    def test_non_success_states_fail(self):
        for state in ('Fail', 'NotRun', 'Skipped', 'InProcess', None):
            with self.subTest(state=state):
                changed = deepcopy(self.report)
                changed['tests'][0]['state'] = state
                self.assertFalse(self.inspect(changed)['passed'])

    def test_missing_and_malformed_reports_fail(self):
        for report in ({}, None, [], {'tests': None}, {'tests': []}, {'tests': [None]}, {'tests': [{'fullTestPath': 1, 'state': 'Success'}]}):
            with self.subTest(report=report):
                self.assertFalse(self.inspect(report)['passed'])

    def test_failed_summary_fails(self):
        self.report['failed'] = 1
        self.assertFalse(self.inspect(self.report)['passed'])


class EvidenceIntegrityTests(unittest.TestCase):
    def test_historical_evidence_destination_rejected(self):
        for phase in ('phase0', 'phase1a', 'phase1b', 'phase1b-ue58'):
            for tool in ('validate_unreal.py', 'validate_foundation.py', 'validate_native_memory.py', 'validate_godot_baseline.py'):
                with self.subTest(tool=tool, phase=phase):
                    command = [sys.executable, str(ROOT / 'Unreal/Tools' / tool), '--evidence-dir', str(ROOT / 'docs/unreal-migration/evidence' / phase)]
                    if tool == 'validate_godot_baseline.py':
                        command.extend(['--godot', 'unused.exe'])
                    result = subprocess.run(command, capture_output=True, text=True, encoding='utf-8', timeout=30)
                    self.assertEqual(result.returncode, 2)
                    self.assertIn('Historical evidence is immutable', result.stderr)


    def test_git_checkout_preserves_attested_fixture_bytes(self):
        provenance = json.loads((ROOT / 'docs/unreal-migration/fixtures/player_memory_provenance.json').read_text(encoding='utf-8'))
        for filename, key in [('player_memory_inputs.json', 'input_sha256'), ('player_memory_expected.json', 'expected_sha256')]:
            with self.subTest(filename=filename):
                path = 'docs/unreal-migration/fixtures/' + filename
                for autocrlf in ('true', 'false'):
                    checkout = subprocess.check_output(['git', '-C', str(ROOT), '-c', 'core.autocrlf=' + autocrlf,
                                                        'cat-file', '--filters', 'HEAD:' + path])
                    self.assertEqual(hashlib.sha256(checkout).hexdigest(), provenance[key])
                    self.assertEqual(checkout, (ROOT / path).read_bytes())


if __name__ == '__main__':
    unittest.main()
