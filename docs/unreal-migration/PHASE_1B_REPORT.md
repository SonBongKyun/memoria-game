# Phase 1B — validation infrastructure and exact-engine blocker

Date: 2026-09-06. Status: **PARTIAL**. The exact UE 5.7 installation is still unavailable. No Unreal-dependent acceptance item is declared complete. The safely executable work—Phase 1A checkpoint, validator corrections, native/Godot regressions, immutable fixture/history checks and handoff—has been completed.

## Checkpoints and scope

- Worktree: `C:\Users\jc\MemoriaMigration\foundation`.
- Branch: `unreal-migration/foundation`.
- Project: `Unreal/Memoria/Memoria.uproject`, `EngineAssociation` remains **5.7**.
- Phase 1A checkpoint: **`2b2a2607296faa1de2f4e8bf94f4eec847bfc8f9`**, `chore(unreal): checkpoint Phase 1A foundation`.
- Phase 1B checkpoint: **pending final checkpoint recording**. Its message will describe the engine blocker and validator work, not imply a successful UE5.7 build.
- No push, engine installation, retargeting, campaign migration or package generation was performed.

Before checkpointing Phase 1A, all 67 inventoried file hashes matched the previous delivered state. The 85 untracked files were inside the intended migration scope; the three tracked modifications were `.gitignore`, `export_presets.cfg` and `SESSION_LOG.md`. All 88 staged paths were reviewed, with no generated Unreal binaries/caches included. Static/preservation checks passed 47/47 and the same production C++ memory model passed all 51 native cases. Phase 0 documents copied by Phase 1A were included in its checkpoint. The original dirty checkout was not staged or committed.

The initial preflight ran the original validators with only their report writes redirected to `C:\Users\jc\MemoriaMigration\phase1b-preflight`; their assertions and commands were unchanged. This avoided overwriting Phase 1A evidence before the checkpoint. Those records are copied under [Phase 1B evidence](evidence/phase1b/).

## Engine discovery and Unreal results

**Engine root actually used: none.** No verified UE5.7 root was found. `C:\Program Files\Epic Games\UE_5.8\Engine\Build\Build.version` was read as evidence, not used to compile. It reports major 5, minor 8, patch 2, changelist **56702186**, compatible changelist 55116800, branch `++UE5+Release-5.8`.

Exact checks:

| Location | Result |
| --- | --- |
| `C:\Program Files\Epic Games` | Present; UE_5.8 is the only engine candidate |
| `C:\Program Files\Epic Games\UE_5.7\Engine\Build\Build.version` | Absent |
| `C:\Program Files\Epic Games\4.0\Engine\Build\Build.version` | Absent; a stale HKLM registration points here |
| `C:\Epic Games`, `C:\UnrealEngine`, `C:\Unreal` | Absent |
| `D:\Epic Games`, `D:\UnrealEngine` | Absent |
| `G:\Epic Games`, `G:\UnrealEngine`, `G:\Program Files\Epic Games` | Absent |
| `C:\Users\jc\Documents\UnrealEngine` | Absent |
| `C:\ProgramData\Epic\UnrealEngineLauncher\LauncherInstalled.dat` | Inspected; no UE5.7 entry discovered |
| HKCU/HKLM `Software\Epic Games\Unreal Engine\Builds` | Absent |
| HKCU `SOFTWARE\EpicGames\Unreal Engine` | Absent |
| HKLM `SOFTWARE\EpicGames\Unreal Engine` | Root INSTALLDIR and stale 4.0 registration only |
| HKLM `SOFTWARE\EpicGames\Unreal Engine\5.7` | Absent |
| `UE57_ROOT`, `UE_ENGINE_ROOT`, `UE_ROOT` | Unset |

This was an inspection of explicit/common/registered locations, not an exhaustive whole-disk search. Exact parent and Build.version records are in [engine_locations.json](evidence/phase1b/engine_locations.json). The required validator was inspected, corrected, and invoked with `--build-and-test` and the Phase 1B evidence directory. Its actual Python process exited **2**, `BLOCKED_MISSING_UE_5_7`; no Unreal process was launched. See [ue57_invocation.json](evidence/phase1b/ue57_invocation.json) and [ue57_validation.json](evidence/phase1b/ue57_validation.json).

| Required Unreal proof | Actual result |
| --- | --- |
| UE5.7 version/changelist | **Not verified; engine unavailable** |
| Project file generation | NOT RUN |
| UnrealBuildTool | NOT RUN |
| UHT/reflection generation | NOT RUN |
| MemoriaEditor compilation | NOT RUN |
| Module linking | NOT RUN |
| Editor project load/launch | NOT RUN |
| All 54 Unreal Automation cases | **NOT RUN: 0/54 executed in Unreal** |
| RunSubsystem creation/GC/ownership/reconstruction | NOT VERIFIED inside Unreal |
| Catalog initialization/context/delegate lifecycle | NOT VERIFIED inside Unreal |
| SaveGame round trip | NOT RUN inside Unreal |

The authored three Unreal foundation tests remain unchanged. They already exercise selected adapter/ownership/serialization behavior, but do not yet explicitly establish every new Phase 1B requirement. In the actual 5.7 environment, extend them to force GC across a retained GameInstance lifetime and test nondefault chapter/player state, memory state, source scene/position, VN continuation and all reserved sections. A default-valued field is not adequate proof of a serialization round trip. No Godot 0.4.0 importer or live-battle serializer was introduced.

## Actual asset/input/geometry status

| Required asset or behavior | Result / path |
| --- | --- |
| `L_FoundationTest` or equivalent map | **NOT CREATED; no saved `.umap` path exists** |
| Enhanced Input movement/confirm/Back/menu assets | **NOT CREATED; no `.uasset` paths exist** |
| Minimal UMG modal | **NOT CREATED; no modal asset path exists** |
| XY movement and Z-drift assertions | NOT RUN |
| Orthographic camera axis | NOT VERIFIED in editor/runtime |
| Paper2D plane and foot/pivot anchoring | NOT VERIFIED in editor/runtime |
| Known source-coordinate restoration | NOT VERIFIED in editor/runtime |
| Keyboard/gamepad input path | NOT RUN; existing display/mapping discrepancy remains unresolved |
| Back press/hold/release single consumption | NOT RUN |
| Modal input blocking and focus restoration | NOT RUN |

The Phase 1A C++ framework still exists, but source inspection is not used as visual or gameplay approval. No text file was passed off as an Unreal asset. The engine-dependent portion stopped at the verified toolchain blocker, as requested.

## Defects reproduced and corrected

**V1 — count-only Automation acceptance could hide missing cases.** The prior validator accepted 51 memory records plus any three records if every state was Success. A duplicated memory case could replace another case without changing those counts. A host-side regression reproduces that former acceptance condition. The validator now compares all **54 exact unique names**, rejects duplicates/missing/unexpected or malformed records, requires every state to be Success and rejects a contradictory failed-test summary. Missing/invalid reports fail. The native-process exit, timeout and fatal-diagnostic checks remain required. It also checks attested fixture/header freshness before invoking UBT. These are validator tests, not fabricated Unreal execution results.

**V2 — fresh Windows checkout broke an attested fixture hash.** `git cat-file --filters HEAD:docs/unreal-migration/fixtures/player_memory_expected.json` reproduced conversion of Godot's LF output to CRLF. The recorded hash `d71446da0e23987c03ddda032c0014a6317f24d073871cb6bedefab33fe44d5e` would become `0e8b8b9ce6ae6b58adb7d84fafd92303f2608e7f46ce262fbdeb03568a460ac9`. Scoped attributes now preserve the output as LF and the already attested Python input as CRLF. Tests verify exact expected hashes with both `core.autocrlf=true` and `false`. **Neither fixture nor its provenance/expected values was edited or regenerated.** No existing Godot file was renormalized.

**V3 — repeat validation overwrote historical Phase 1A evidence.** Four validators previously wrote directly into `evidence/phase1a`. They now accept `--evidence-dir`, default to timestamped ignored `Saved/Validation` output, and reject destinations under the historical Phase 1A directory. This run records results only under `evidence/phase1b`. The static allowed-path check also recognizes the now-tracked `.gitattributes` migration policy.

No real Unreal compiler, linker, UHT, GC or gameplay defect has been diagnosed or fixed: those executions never occurred. Production C++ and its 54 Unreal test definitions remain unchanged.

## Executed validation

| Check | Result |
| --- | --- |
| Phase 1A preflight inventory | PASS, 67 hashes; 88 staged migration paths reviewed |
| Phase 1A preflight static/native | PASS, 47 static checks / 4,217 protected original files; 51 native cases; CTest 1/1 |
| Current native parity | **PASS, 51/51**, same production C++ and attested fixtures; not a UE build |
| Host validator regressions | **PASS, 10/10**; distinct from the 54 Unreal tests |
| Repository contract | PASS; existing legacy grade-label warning retained |
| VN validation | PASS, 21 files, 526 steps, 0 errors, 0 warnings |
| Korean localization | PASS, 32 files, 1,581 fields, 19 speakers, 0 errors |
| Godot editor import | PASS, exit 0, no script/parse/fatal diagnostics |
| Official memory/world suite | **PASS, 15/15**, fatal scanning and expected exit-1 path guards enabled |
| Exported actor catalog | PASS, 1,064,635,280-byte PCK; **export_log_errors=0 in this run** |
| Godot user data / editor metadata | Isolated subprocess app-data; 1,056 metadata files restored to pre-run bytes |
| Final source/history/static checks | **PASS, 47/47**; 4,217 protected original files; 42 immutable file hashes unchanged; 13 local handoff links valid |

The previous phase's cold-import/resource-error reports were not rewritten to match this run. Native passing evidence does not certify Unreal reflection or lifecycle. Host tests use synthetic report dictionaries solely to test the validator.

Commands executed from the migration worktree:

```powershell
python Unreal/Tools/validate_ue57.py --build-and-test --evidence-dir docs/unreal-migration/evidence/phase1b
python Unreal/Tools/validate_native_memory.py --evidence-dir docs/unreal-migration/evidence/phase1b
python -m unittest discover -s Unreal/Tools -p test_validation_tools.py -v
python Unreal/Tools/validate_foundation.py --original-manifest 'C:\Users\jc\MemoriaMigration\phase1a-original-manifest.json' --evidence-dir docs/unreal-migration/evidence/phase1b
python Unreal/Tools/validate_godot_baseline.py --godot 'C:\Users\jc\Downloads\Godot_v4.6.2-stable_win64.exe\Godot_v4.6.2-stable_win64_console.exe' --evidence-dir docs/unreal-migration/evidence/phase1b
```

Evidence: [native results](evidence/phase1b/native_memory_validation.json), [host unit tests](evidence/phase1b/validator_unit_tests.json), [Godot baseline](evidence/phase1b/godot_baseline.json), [preflight static](evidence/phase1b/preflight_foundation_static.json), [preflight native](evidence/phase1b/preflight_native_memory_validation.json).

## Files and next task

Modified: `.gitattributes`; `Unreal/Memoria/README.md`; four `Unreal/Tools/validate_*.py` files; `SESSION_LOG.md`; `MIGRATION_STATE.md`; actual-progress appendix in `MIGRATION_ROADMAP.md`.

Created: `Unreal/Tools/test_validation_tools.py`; this report; `docs/unreal-migration/evidence/phase1b/` records. No production source, fixture expectation, story, image, audio, map or input/modal asset was changed/created.

Remaining blocker: an actual UE **5.7** installation is required to perform the central Phase 1B work. No install permission was inferred. Uncompiled code may contain engine/API/lifecycle defects; those remain unknown, not cleared by static checks. Real hardware/device and rendered geometry/input tests also remain open.

Resume **Phase 1B first**: verify `Build.version`, run `validate_ue57.py --engine-root '<verified UE_5.7-root>' --build-and-test --evidence-dir docs/unreal-migration/evidence/phase1b`, resolve UBT/UHT/compile/link/test defects, complete GC/save coverage, then generate/save the isolated map, Enhanced Input assets and UMG modal through actual Unreal tooling. Prove plane/pivot/coordinate behavior and one Back action per physical press before calling Phase 1B complete.

Exact next **Phase 1C**, only after that gate: deterministically export the actual starting-memory catalog into versioned IR and import a typed Unreal catalog asset; verify a second unchanged import preserves IDs, owned order and semantic content, then execute the imported catalog with existing memory fixtures. This prepares the established Verdan arrival/trade vertical slice. It does not authorize Chapter 1 reconstruction, BattleManager or bulk narrative/art import while Phase 1B is incomplete.
