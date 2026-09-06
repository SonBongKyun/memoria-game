# Phase 1A — Unreal foundation and player-memory domain

Date: 2026-09-06. Status: **PARTIAL**. The requested source foundation and executable native parity checks are implemented. Unreal 5.7 compilation, UHT, module linking, editor launch and Unreal Automation execution remain unverified because the exact engine is unavailable. This is not an assertion that uncompiled Unreal code is error-free.

## Workspace and preservation

- Dedicated branch: `unreal-migration/foundation`.
- Worktree: `C:\Users\jc\MemoriaMigration\foundation`.
- Project: `C:\Users\jc\MemoriaMigration\foundation\Unreal\Memoria\Memoria.uproject`.
- Base HEAD: `15df72809baa52eec281d8508f495a373e9f5884`.
- Original checkout: `C:\Users\jc\OneDrive\바탕 화면\메모리아\Game`, on `overnight-gameplay-graphics`.
- Original preexisting `project.godot`, `SESSION_LOG.md` and Phase 0 untracked audit files were left in place. Audit files and the existing session log were copied into this worktree; the unrelated modified original `project.godot` was not copied over this worktree's clean base.
- An external pre-task manifest fingerprints **4,217 original files** at `C:\Users\jc\MemoriaMigration\phase1a-original-manifest.json`; the static evidence records the final comparison. Seven Phase 0 contract documents remain byte-identical; the roadmap retains its complete original contents followed by an actual-progress appendix.
- No GDScript, story JSON, artwork, audio, existing validators or runtime fixtures were edited. Godot editor import rewrote metadata/line endings in this isolated worktree; those task-generated changes were restored. The migration's final Godot changes are only the necessary Unreal export exclusions and session documentation.
- No local checkpoint commit, push, deployment, engine installation, LFS normalization or packaging was performed.

## Files and configuration

New project files are grouped below. [phase1a_files.json](evidence/phase1a/phase1a_files.json) lists the exact authored paths and SHA-256 hashes; generated caches/build outputs are excluded.

| Location | Responsibility |
| --- | --- |
| `Unreal/Memoria/Memoria.uproject`, `Source/*.Target.cs` | UE 5.7 association; V6 build settings and Unreal5_7 include order; game and editor targets |
| `Source/Memoria/Memoria.Build.cs`, `Private/Memoria.cpp` | Runtime module, C++20, Core/CoreUObject/Engine plus actual 2D/input dependencies |
| `Config/DefaultEngine.ini`, `DefaultGame.ini`, `DefaultInput.ini` | GameInstance/GameMode classes, conservative 2D renderer defaults, Enhanced Input classes, project metadata |
| `Source/Memoria/{Public,Private}/Domain/` | Typed definitions/state/snapshot/events, catalog DataAsset, run-owned UObject adapter and portable authoritative C++ rules |
| `Source/Memoria/{Public,Private}/Run/` | GameInstance subsystem, persistent run identity, chapter/locale, typed player state and case-sensitive boolean flags |
| `Source/Memoria/Public/Narrative/` | Separate field and VN contract types, effect-phase enums, runtime interfaces and VN continuation DTO |
| `Source/Memoria/{Public,Private}/Save/` | Versioned run SaveGame schema/header validation; separate world/diary/hint sections and source-coordinate return position |
| `Source/Memoria/{Public,Private}/Framework/` | Thin GameInstance/GameMode; plane-constrained Pawn, Paper2D sprite, orthographic camera, Enhanced Input controller, coordinate adapter |
| `Source/MemoriaTests/` | Editor-only Automation module: 51 shared Godot cases and 3 Unreal foundation tests |
| `Unreal/Tests/{Shared,Generated,Native}/` | Shared parity comparator, attested generated vectors, standalone CMake/MSVC test entry point |
| `Unreal/Tools/` | Offline Godot oracle, fixture conversion, exact engine probe/build/test, native validation, isolated Godot baseline and static preservation tools |
| `docs/unreal-migration/fixtures/` | 51 deterministic inputs, actual Godot expected results, source provenance and regeneration guide |
| `docs/unreal-migration/evidence/phase1a/` | Exact recorded command/results, initial cold-import failure, native pass, engine blocker, static checks and file inventory |

Modified tracked boundary files: `.gitignore`, `export_presets.cfg`, `SESSION_LOG.md`. New `.gitattributes` scopes LFS solely to `Unreal/**/*.uasset` and `Unreal/**/*.umap`. Generated Unreal directories, IDE files and Python caches are ignored only under Unreal. Existing PNG/audio tracking is unchanged. `Unreal/.gdignore` plus exclusions in both existing Godot export presets keep the migration subtree out of the reference import/export pipeline.

Only **Paper2D** and **Enhanced Input** are enabled. GAS, multiplayer and speculative plugins were not justified. No fabricated binary maps/assets were authored. No default campaign map, full input asset set, modal UI or production asset conversion exists in Phase 1A.

## Architecture implemented

`UMemoriaRunSubsystem` has GameInstance lifetime and owns a GC-tracked `UMemoriaPlayerMemoryDomain`. The domain owns a portable C++20 `Memoria::Memory::Model`; the same production `.cpp` is built by Unreal and the native parity target. This avoids maintaining two rule implementations. Definition data/provenance are separate from mutable owned state; connections and carry remain derived. IDs remain case-sensitive `FString`/UTF-8 strings, distinct from display names or Unreal package names.

The run subsystem starts only through an explicit catalog plus initial-ID list. It does not silently acquire all catalog rows, advance chapters or start the canonical campaign. It preserves `canon_*` in the flag collection rather than adding a competing canonical progression counter. Missing flags remain distinguishable from stored false. Run reconstruction validates typed entries and memory state before assigning the run snapshot; it emits no acquisition/burn rewards. Native commands and callbacks execute synchronously on the game thread; no networking/concurrent mutation guarantee is made.

The field Pawn establishes the contract's XY plane and -Z orthographic camera. Godot `(x,y)` maps to `(x,-y,0)` at one source pixel per Unreal unit. Paper2D's component rotation makes its local sprite plane face the camera. Controller input mapping assets, collision/foot pivot tuning, visual depth sorting and real-editor orientation checks are still Phase 1B work. Component defaults are not claimed as movement or visual parity.

NPC actor cognition has **no runtime implementation here**. Its reserved save section has a distinct type and no dependency on player burn commands. A player burn cannot remove an actor-addressed world memory through this foundation.

## Memory behavior and event contract

| Rule | Implemented source behavior |
| --- | --- |
| Grades | Raw ordinals `0=GRADE_5, 1=GRADE_4, 2=GRADE_3, 3=GRADE_2, 4=GRADE_1`; display rank remains separate (`raw + 1`) |
| Eligibility | Missing, burned, faded unless explicitly allowed, and active collateral reject burns; the domain does not invent a core-grade burn ban |
| Normal burn | Set burned; if Elia is present and raw grade ≥2 create residue; append history; cascade; notify burn/carry; evaluate passives |
| Silent burn | Does not create new residue, but retains a preexisting residue bit; still appends history, cascades, notifies burn/carry and evaluates passives |
| Residue | Burned + residue query; repeated reads do not burn again or consume it; battle reuse execution remains deferred |
| Connections | NPC groups first, then adjacent same-prefix IDs in owned insertion order; deduplicated links; refreshed after acquisition events |
| Cascade | Amounts `[2,4,7,11,0]`; skip burned/faded/core targets, consume guards; no collateral or Still Hands immunity in this path |
| Erosion | Round chapter argument × overload multiplier, at least 1; use run chapter for capacity; Elia and grade reductions truncate sequentially; zero increment still counts as processed |
| Erosion protections | Skip burned/faded/core, consume guard before Still Hands check; Still Hands shields exact `Elia` links in chapter erosion only |
| Effective power | `max(1, burn_power - erosion)` for an existing memory; missing query returns 0 |
| Carry | Weights `[1,2,3,4,6]`; burned and collateral excluded, faded included; capacity `min(34,14+2*max(0,chapter-1))`; overload query uncapped, erosion multiplier caps overload at 1 |
| Passives | Thresholds 5/10/20/30/50; voluntary count is `max(0, history.length - extracted.length)`; presence of even a false passive key prevents a second unlock |

Order is tested through snapshots inside each source signal:

| Typed event | Observable phase |
| --- | --- |
| Added | New definition and owned state exist; graph refresh has not happened |
| ResidueCreated | Burned/residue bits are set; current ID has not entered burned history |
| Faded | Target erosion and faded bit are committed; later targets have not been processed |
| Cascaded | Target processing completed; history already contains the burn; Burned event has not fired |
| Burned | Burn/history/cascade committed; new passive thresholds not yet evaluated |
| CarryChanged | After Added but before graph refresh, or after Burned but before passive evaluation |
| PassiveUnlocked | This passive key is set; later thresholds may still be pending |
| MemoriesEroded | All eligible chapter-erosion targets processed; count may include zero increments |

Native observers run before the Blueprint presentation delegate for each event. Observers may inspect these intermediate states; nested mutations return `Busy`. This is an explicit new ownership guard, not a claim that every GDScript listener's reentrant behavior has been ported. The runtime context is frozen for each command. A future listener that changes party/chapter/oath state during a signal needs a characterized adapter before integration. Gameplay mutations must not be added to presentation delegates casually.

The snapshot retains loan, extraction, erosion guards/used slots and preservation fields. Loan issuance/repayment, extraction commands, synthesis, guard purchase and preservation progression APIs are deferred. In particular, there is no generic RemoveMemory operation: future sale must use normal burn and its consequences; synthesis must remove inputs without adding burns. GameManager statistics, narrative reactions, milestone presentation, audio, UI and battle effects are not certified by this domain-only port.

## Narrative, save and data boundaries

`FFieldDialogueLine`/`IMemoriaFieldDialogueRuntime` are separate from `FVNStoryStep`/`IMemoriaVNRuntime`. They carry distinct choice indices and effect-phase enums. The interfaces do not execute a universal conversation graph. Future importers must retain field gate-before-effects, field filtered choice indices/flags-before-failed-cost behavior, VN effects-before-step-gates and payment-before-choice-rewards behavior. Source provenance, original step index, stable text/step IDs and mapping version are explicit. No authored narrative content has been duplicated into C++.

`FMemoriaVNContinuation` contains current/pending sequence and original indices, FIFO resume queue, active bit and ledger burn snapshot. Field dialogue/UI objects are not represented as resumable saves.

`UMemoriaRunSaveGame` is schema **1**, slots **0 autosave / 1–3 manual**, with content revision, run ID, timestamp, owned-order memory definitions/snapshot, separate reserved world/diary/hint sections and field-return scene/source-pixel position. Legacy Godot **0.4.0** is a separate provenance/version constant, not declared compatible. The reserved source JSON sections are opaque and unapplied. Header validation does not stand in for complete content/section validation. No disk writer, profile merge/reset policy, Godot importer, staged world restore, scene loader or live battle serialization has been implemented. Memory `Restore` is an internal UE snapshot/bootstrap contract, not Godot's permissive import/fallback algorithm.

## Validation evidence

| Validation | Actual result |
| --- | --- |
| Godot oracle | **PASS**, 51 cases / 95 commands / 109 event observations, actual 4.6.2 `71f334935`; source/input/output hashes recorded |
| Native production memory C++ | **PASS**, MSVC 19.44.35228.0, C++20, `/W4 /WX /permissive- /utf-8 /bigobj`; 51/51 cases, 0 failed |
| CTest | **PASS**, 1/1 test invoking all 51 cases |
| Repository contract | **PASS**, existing legacy grade-label warning retained |
| VN validation | **PASS**, 21 files, 526 steps, 0 errors, 0 warnings |
| Korean validation | **PASS**, 32 files, 1,581 fields, 19 speakers, 0 errors |
| Godot cold-cache editor import | **FAIL despite exit 0**, missing initial font caches and Dialogic dependency parse diagnostics; failed attempt preserved |
| Godot cached editor import | **PASS**, exit 0 with no script/parse fatal diagnostics, after generated caches became available; no source changes used |
| Official memory/world suite | **PASS**, 15/15, full fatal scans and expected exit-1 path guards; complete app-data isolation |
| Exported actor catalog | **PASS**, exported PCK runtime validation; pack 1,064,635,280 bytes; existing resource errors=422 surfaced by the unchanged official runner |
| Structural/file integrity | **PASS, 47 checks / 4,217 protected original files**. `foundation_static.json`: module/target/config presence, generated-include ordering, fixture freshness, source hashes, scoped Git/LFS/import/export boundaries and protected original files |
| UE compiler/UHT/linker | **NOT RUN — missing UE 5.7** |
| UE editor launch | **NOT RUN** |
| UE Automation | **NOT RUN**; 51 shared cases + 3 foundation tests authored |
| UE visual/input/map test / packaging | **NOT RUN / out of Phase 1A scope** |

The source oracle executes byte-identical `memory_manager.gd` and `journey_oath.gd` in a dedicated temporary Godot project with inert presentation adapters and an out-of-tree fixture domain. Expectations are not recomputed in Python. The C++ converter only translates recorded JSON to test values and verifies provenance. Unreal and standalone targets share the production rule file and comparator. Native passing evidence covers the portable domain; **it does not cover UHT/reflection/serialization, UObject lifecycle, engine linking or rendering**. Those concerns have three authored Unreal tests pending the engine.

The first standalone build hit MSVC C1128 from the generated vector object's section count. `/bigobj` was added to that test build, and compilation then passed with warnings treated as errors. The cached Godot import retry resolved first-import failures without modifying source or weakening the fatal scan. The 422 export resource errors are still recorded by the official runner; this report does not claim a warning-free export or a full-game playthrough.

Exact repeatable commands from the worktree:

```powershell
python Unreal/Tools/export_memory_oracle.py --godot 'C:\Users\jc\Downloads\Godot_v4.6.2-stable_win64.exe\Godot_v4.6.2-stable_win64_console.exe' --reference 'C:\Users\jc\OneDrive\바탕 화면\메모리아\Game'
python Unreal/Tools/generate_memory_test_header.py --check
python Unreal/Tools/validate_native_memory.py
python Unreal/Tools/validate_godot_baseline.py --godot 'C:\Users\jc\Downloads\Godot_v4.6.2-stable_win64.exe\Godot_v4.6.2-stable_win64_console.exe'
python Unreal/Tools/validate_foundation.py --original-manifest 'C:\Users\jc\MemoriaMigration\phase1a-original-manifest.json'
python Unreal/Tools/validate_ue57.py
# Once the required engine exists:
python Unreal/Tools/validate_ue57.py --engine-root '<UE_5.7-root>' --build-and-test
```

Recorded exact invocations, output, exit status and inspection scope are in [native_memory_validation.json](evidence/phase1a/native_memory_validation.json), [godot_baseline.json](evidence/phase1a/godot_baseline.json), [godot_cold_import_attempt.json](evidence/phase1a/godot_cold_import_attempt.json), [foundation_static.json](evidence/phase1a/foundation_static.json) and [ue57_validation.json](evidence/phase1a/ue57_validation.json). Full transient process logs live under `Unreal/Memoria/Intermediate/`; no generated engine/cache files are tracked.

## Phase 0 deviations and remaining work

The user's Phase 1A instruction overrides the original roadmap's directory/branch defaults with `Unreal/Memoria` and `unreal-migration/foundation`. It also explicitly excludes packaging and requests the first player-memory subset earlier than original P2. Consequently this phase does not claim original P1's full modal, input demonstration or package acceptance. The roadmap's strategy was retained with an appendix documenting this narrower delivery.

The runtime module is named `Memoria` to match the requested normal project layout, while the lifetime ownership remains `UMemoriaRunSubsystem` → `UMemoriaPlayerMemoryDomain` from ARCHITECTURE. An editor-only `MemoriaTests` module has an actual Automation responsibility; an empty general editor/importer module was not invented. The portable kernel is an implementation detail of that production domain, enabling executable parity while 5.7 is absent. The capacity formula documents the executable source's chapter lower clamp, which the abbreviated Phase 0 prose omitted.

Exact environment blocker: Launcher data, build registries, known installation parents and engine environment roots identified only **UE 5.8.2, changelist 56702186**, at `C:\Program Files\Epic Games\UE_5.8`. No verified **5.7** `Build.version`/toolchain was found. VS2022 Build Tools, MSVC and Windows SDK 10.0.26100.0 are available. No UE5.8 build was used to substitute for the requested engine, and no install was attempted. Missing 5.7 is the observed environment blocker; compiling with it may still expose code/API issues that static checks cannot rule out.

Exact next Phase 1B task: obtain an authorized usable **UE5.7** installation, run the provided Editor build and all **54 Automation cases**, resolve all UHT/compiler/test diagnostics, then create one isolated 2D test map with authored Enhanced Input assets and a single modal. Demonstrate movement, plane/sprite orientation, foot pivots, source-coordinate restore and one Back consumption per press in the real editor. Only after this foundation is verified should deterministic content import and the contract's Verdan integration slice proceed. Do not rewrite Chapter 1, add a Ch1 battle, or reconnect legacy routes into canon.

Reference APIs used for implementation review: [Epic subsystem ownership](https://dev.epicgames.com/documentation/en-us/unreal-engine/programming-subsystems-in-unreal-engine?application_version=5.7), [Epic C++ Automation tests](https://dev.epicgames.com/documentation/en-us/unreal-engine/write-cplusplus-tests-in-unreal-engine?application_version=5.7), [Epic Enhanced Input](https://dev.epicgames.com/documentation/en-us/unreal-engine/enhanced-input-in-unreal-engine?application_version=5.7). Header/API review is not an engine compilation result.
