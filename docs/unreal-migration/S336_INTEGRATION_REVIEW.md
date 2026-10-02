# S336 — S334 save review and S310–S335 integration

Date: 2026-10-02. Owner: Codex. **Review, integration and fresh validation complete.**

## Reviewed result

The two previously blocking S330 findings are resolved by S334 (`7d2ad69b`).

- `AMemoriaSliceGameMode::StartPlay` handles a refused `?Continue` for both active and absent runs before `EnterChapterMap`. A failed map load returns to the title and its localized footer shows the failure. The regression uses real travel through StartPlay rather than only the restore API.
- `ValidateMapSnapshot` rejects Diary/Hints schema and payload data, and the inactive flow's ledger count, before the run can be replaced. Hand-framed save tests retain a positive control and separately reject each unsupported field at validation, Continue selection and restoration.
- Prior requested position preservation is not satisfied: the old level has already been unloaded. Accept the documented title fallback while retaining the live run's game data. This is a deliberate acceptance adjustment, not a claim that the old actor position survives.
- No-active-run failure was additionally checked in a fresh rendered game process with a unique empty validation save leaf: its trace is exactly `autosave:map_resume_failed` then `title:enter`, with no chapter entry, fatal diagnostic or nonzero exit. This smoke verifies routing; it is not a serialized-state or UI assertion. Comprehensive review of every historical S313–S329 feature is not claimed; the dependency pass examined integration boundaries, state ownership, test profile isolation and current source references.

S335 (`7daf2ce9`) adopts all six delivered models with their fallback assets. The capture reset is limited to editor sessions launched with `-MemoriaCapture` and removes its delegate on module shutdown. No further blocking finding in the requested changes.

## Integration and preservation

- Original Codex base: `c0022579`, with 58 uncommitted S310/model/host-test files.
- Preserved original bytes in `Unreal/Memoria/Saved/Validation/s336-integration/before-integration.zip`, with SHA-256 manifest and binary patch.
- Local preservation commit: `a42f4526`, retained by `codex/s310-preserved-20261002`. Original branch remains at c0022579.
- Integrated on `codex/unreal-s335-integration-20261002`: S310 adoption then all 25 subsequent commits through S335, in order. The only conflict was the migration document's S310 adoption addendum, which was retained alongside the prior report.
- Runtime/art/tools match Claude's final Git tree. Of 849 tracked files checked directly, 721 are byte-identical; 128 differ only by CRLF/LF. SESSION_LOG's pre-existing Codex formatting is preserved.
- All 12 adopted model/texture source files match both the shared delivery and the recorded `final_delivery_audit.json` hashes.
- Required ignored template mannequin content was installed using the existing `install_mannequin.py` from the already installed UE engine. 48 existing UAL2 source packages were copied from Claude read-only, with individual hashes. No mutable Binaries, Intermediate, Saved or DDC folders were copied or shared.
- No source/runtime changes beyond the accepted commits. No peer/foundation/Godot changes, remote push, release or new art work.

## Fresh verification

- Six repaired host modules: **35/35 PASS**, in the actual integrated Codex lane.
- UE 5.8.2 Editor build: **PASS** (365.48 seconds). Full rendered `Memoria.`: **274/274 PASS** (267 without warnings, 7 with warnings; exit 0, no missing/extra/duplicate IDs). `MemoriaVisual.`: **26/26 PASS** (24 without warnings, 2 with warnings; exit 0, no missing/extra/duplicate IDs). Full automation runtime 1814.89 seconds; visual runtime 113.77 seconds, excluding process startup.
- Fresh no-active-run Continue smoke: **PASS**, exit 0, refusal then title, no chapter entry. Command and trace: `cold-continue.json`.
- Capture sizes depend on the existing test window path: the new map/title tests produce 1280x720; older full-registry replay captures include a 34px Slate PIE title bar and are 1280x754. No progressive shrinking has been observed. Do not generalize the handoff's 1280x720 claim to every diagnostic PNG. Final PNG audit: 125 at 1280x720 and 946 at 1280x754, including exported diagnostic copies. All 17 focused chapter/title PNGs are 1280x720.
- Full-registry test warnings: six transient narrative GameInstances have no world context; one `r.MotionVectorSimulation` render-thread warning. There are also 15 `LogAutomationTest: Error: Condition failed` lines during engine startup, before the requested tests, matching both Claude S335 logs. Their underlying cause remains unresolved; they are not relabeled as harmless. Fatal diagnostic patterns: 0.
- Read fresh captures: refused Continue title (localized failure and disabled Continue), Belt revisit (three NPCs and tank), Drift campfire and rubble. All six adopted models are visible across these views; the bright small rubble remains as described in the handoff.
- Fresh scan of all 542 packages: none reference the ten S333 deleted package paths (ASCII/UTF-16 name scan, not a dependency loader).
- `git diff --check a42f4526..HEAD`: PASS for integrated changes. The original preserved S310 commandlet has one pre-existing extra blank line at EOF; it remains unchanged and is recorded rather than relabeled as newly clean.

Evidence root: `Unreal/Memoria/Saved/Validation/s336-integration/`. The older Claude results are not counted as fresh Codex validation.

## Scope limits

The lost previous actor position on refused travel remains explicit. The fresh no-active-run process smoke verifies the travel route; it does not assert private DTO state or capture its UI. No packaged build, human play/listening/feel pass, Chapter 6+ expansion, Leads tab, optional rat, or tuning is claimed.

## Reproduction and final evidence

From the Codex lane:

```powershell
python -B Unreal/Tools/validate_unreal.py --build-and-test --rendered --test-prefix Memoria. --automation-timeout 5400 --evidence-dir Unreal/Memoria/Saved/Validation/s336-integration/full
python -B Unreal/Tools/validate_unreal.py --build-and-test --rendered --test-prefix MemoriaVisual. --automation-timeout 5400 --evidence-dir Unreal/Memoria/Saved/Validation/s336-integration/visual
python -B Unreal/Memoria/Saved/Validation/s336-integration/cold_continue.py
```

The six host modules are `test_checkpoint_tools`, `test_shop_transaction_tools`, `test_malet_antidote_tools`, `test_malet_firebomb_tools`, `test_malet_potion_tools`, and `test_malet_world_seed_tools` under `Unreal/Tools`; all 35 tests ran there with Python `-B`. Details are in `host-tests.log`.

- Full report: `Unreal/Memoria/Saved/Validation/unreal-run-20261002T040213180432/Automation-20261002T040820247576/index.json`.
- Visual report: `Unreal/Memoria/Saved/Validation/unreal-run-20261002T043944902776/Automation-20261002T043949780489/index.json`.
- Detailed warnings, raw diagnostic lines and PNG dimensions: `s336-integration/diagnostics.json`. Visual tests also retain the same 15 startup Condition failed diagnostics; fatal patterns are zero. Material usage-field deprecation warnings remain in the successful Editor build.
- Preservation archive was read back: ZIP CRC and all 58 SHA-256 values pass.
- Claude lane remains clean at `7daf2ce9`; the original Codex branch still names `c0022579`. The integrated gameplay/assets are unchanged from the validated S335 integration commit `c2afd34b`; final recording edits are documents only.

| Session | Claude commit | Codex integrated commit |
|---|---|---|
| S330 | bd78abc5 | ee7c4e5c |
| S331 | 73c1cb2c | 5a319a48 |
| S332 | 8d56502d | f32f1aec |
| S333 | eec6d6ae | 3c02dbd5 |
| S334 | 7d2ad69b | bb3ac844 |
| S335 | 7daf2ce9 | c2afd34b |

All 26 source/integrated commit pairs are retained in `s336-integration/commit-map.json`. No additional feature or art task is assigned by this review; tuning and the Chapter 6+/Leads direction remain user decisions.
