# Phase 1C — deterministic starting-memory catalog

Date: 2026-09-06. Status: **COMPLETE — all Phase 1C acceptance checks passed**.

## Baseline and scope

Branch: `unreal-migration/ue58-foundation`. Worktree: `C:\Users\jc\MemoriaMigration\foundation`. Starting clean HEAD: `b06cfbad7488cb5c8b9ea030512e4f7a6c309c16`; accepted Phase 1B technical checkpoint: `44d08dde96b352c3f86b770ed11948bcc18f6bb6`. New technical checkpoint: **`b6c632fc83fdbdbe21d26452b2135f4997704988`** — `feat(unreal): import deterministic starting memory catalog`. A documentation-only follow-up records this SHA so the technical commit need not contain its own hash. No push.

Build.version was directly re-read: UE **5.8.2 / CL 56702186**, compatible CL 55116800, at `C:\Program Files\Epic Games\UE_5.8`. Phase 1B's 57 tests, coordinate/input/sprite/map assets, player-memory kernel, SaveGame schema 1 and ownership contracts are preserved. This task adds offline content tooling and explicit memory initialization only.

## Source audit and extraction

Source repository revision: **`4f772fafb1c7c6da68f56f44d60a7c7fed7f3949`**, the latest commit touching the audited source paths; current migration commits do not fabricate a new content revision. Each source is checked against both this Git revision and its recorded text hash. The original Godot checkout's HEAD remains `15df72809baa52eec281d8508f495a373e9f5884`.

| Source | SHA-256 of UTF-8/LF text |
| --- | --- |
| `scripts/systems/memory_manager.gd` | `5551c4fe5541e5201541a4dcee835ae4ec8fc7404167a4fc81e547e3b0520938` |
| `scripts/utils/journey_oath.gd` | `39610117fa7a7b357960a931b024a28b1a27068f5da5d3602d67829b7a3d2635` |
| `scenes/main/main.gd` | `139392fec566ea84a488934e86f8f42da9fe98e536771376a00086d6c7512e68` |
| `scripts/core/game_manager.gd` | `302846dfed4304f009b39a728812ac91484cfe6b858e235e661e84fa7fb9e6d1` |

`memory_manager.gd` declares the Memory constructor, `_init_starting_memories`, English definitions, `MEMORY_TEXT_KO`, insertion and connection rules. `scenes/main/main.gd` confirms normal New Game clears the two memory arrays and invokes that initializer; `game_manager.gd` contains the analogous NG+ call. `journey_oath.gd` is an unchanged rule dependency for the source command oracle. The latter three files are provenance/review dependencies, not extra memory catalogs.

The exporter copies byte-identical memory manager and oath source into a separate minimal Godot 4.6.2 project under ignored Saved/Validation. It reuses the accepted Phase 1A oracle and inert GameManager/audio/log/toast adapters. An instance outside the SceneTree calls the real initializer and exposes the definitions. No production map, UI, audio, battle, network, profile or campaign boot is required; app-data roots are isolated. The harness rejects unknown Memory script properties, unknown localization fields, empty output, missing Korean text or nonzero authored mutable defaults. It never parses/copies authored values by hand.

Actual entry count: **7**. Exact insertion order:

1. `sense_forest_smell`
2. `sense_warm_light`
3. `daily_market_food`
4. `daily_campfire_song`
5. `rel_hand_reaching`
6. `identity_first_sword`
7. `core_name_origin`

Despite a comment labeling it late-game, `core_name_origin` is constructed at initialization and is therefore included. No chapter-added or Phase 1B test memory is included. Raw grades are 0=GRADE_5 through 4=GRADE_1. `Unknown` and empty related-NPC values are literal source strings, not missing lookup failures.

Connections are derived at runtime: same-NPC groups first, then adjacent memories of each ID prefix, in insertion order with duplicate edges removed. They are recorded only in the separate source oracle, never as immutable definition content. Burned/residue/faded/erosion, history, loans, guards, preservation and passive state are likewise excluded from the catalog. Korean title/description/effect rows are typed catalog localization; missing effect versus present empty effect is retained with an explicit presence bit. No localization UI was added.

## IR and validator

IR: [starting_memory_catalog.v1.json](ir/starting_memory_catalog.v1.json). Schema **1**, kind `player_memory.starting_catalog`, extractor `memoria-starting-memory/1`.

Canonical IR SHA-256: **`fddc133a6bc3b69b62a709691d4f3864570c1b19262e2951feba6a16759f54d2`**.

Semantic fingerprint: **`c3bd646d2667ad98b8633b52de02a4245fc7f506688f2c1ea39f8e5f4b8e845c`**.

Canonical JSON uses UTF-8 without BOM, unescaped Unicode, sorted ASCII field keys, compact separators, one LF terminator, exact integer fields and ordered entry arrays. No timestamp or absolute machine path enters the IR. `.gitattributes` pins only new IR JSON to LF; original source/fixtures are not renormalized. Source text hashes normalize CRLF to LF in memory; raw checkout SHA-256 is separately recorded in extraction evidence. Semantic fingerprint hashes canonical `{schema_version, content_kind, entries}`, including localization and field presence, but excluding source revision/importer/package metadata. Asset import metadata is still compared separately for exact no-op detection.

Both Python and C++ reject unsupported schema/kind/extractor, missing/unknown fields, absent provenance, incorrect source hashes, empty/exact-duplicate IDs, invalid raw grade, nonpositive/nonintegral/out-of-range burn power, invalid text/NPC representation and mutable-state contamination. JSON duplicate keys and noncanonical bytes fail. Case-only ID differences remain distinct and are never normalized. Numeric strings are rejected explicitly instead of relying on Unreal's permissive JSON conversions. Check-only source export executes the source again into a temporary output and compares bytes; it never regenerates the recorded IR or oracle.

## Typed import and runtime

Existing runtime class: **`UMemoriaMemoryCatalog`**, with `FMemoriaMemoryDefinition` unchanged. Metadata and typed Korean text rows extend the catalog rather than introducing another authority. Runtime consumes the saved asset only; it does not depend on Python, Godot, source files, editor import code or loose JSON.

Asset: **`/Game/Memoria/Generated/Memory/DA_StartingMemoryCatalog`**, following DATA_MIGRATION's generated package namespace. Disk: `Unreal/Memoria/Content/Memoria/Generated/Memory/DA_StartingMemoryCatalog.uasset`, under the existing scoped LFS rule. Domain IDs remain FString values separate from the package name.

Editor-only importer/commandlet lives in the existing `MemoriaTests` editor module. OpenSSL is used there for SHA-256; the runtime module gains no crypto/import dependency. Installed UE's generic GetSHA256Signature implementation is not a usable platform implementation here, so it is not called.

Candidate IR is validated completely before loading or modifying the destination. A previously edited generated semantic payload fails for review. Existing identical typed fields and provenance cause `UNCHANGED`, no dirtying and no save. Changed validated input updates the same package/object identity. Missing or wrong-type existing packages are not silently overwritten. Stale outputs are not deleted.

`UMemoriaRunSubsystem::BeginStartingMemoryRun` explicitly loads this catalog and passes definitions in their array order to the accepted BeginRun/Restore path. It creates fresh mutable memory state, reconstructs connections, and does not start narrative or travel. This proves New Game memory bootstrap, not complete New Game inventory/profile reset parity.

## Actual import, runtime and regression results

Actual results, retained in [acceptance.json](evidence/phase1c/acceptance.json):

| Check | Result |
| --- | --- |
| Real UE 5.8.2 UBT / UHT / compile / both module links | PASS; first failed compiler attempt retained, corrected build passed |
| First import, separate commandlet process | CREATED, saved=true, 7 entries |
| Second import, new process | UNCHANGED, saved=false |
| Third process check-only reload | UNCHANGED, saved=false |
| First semantic fingerprint | `c3bd646d2667ad98b8633b52de02a4245fc7f506688f2c1ea39f8e5f4b8e845c` |
| Second semantic fingerprint | `c3bd646d2667ad98b8633b52de02a4245fc7f506688f2c1ea39f8e5f4b8e845c` |
| Package SHA-256 after all three processes | `cfd2e8b97bad9c124352589c5318b695e42283c950ccb2d35e3c5b5368cfdfd5` (10,631 bytes) |
| Runtime memory initialization and source command/event parity | PASS |
| Temporary IR semantic change detection | PASS; transient object only, no temporary package saved |
| Previous accepted Unreal suite | **57/57 PASS**, exact identities retained |
| Phase 1C suite | **3/3 PASS**, zero test errors or warnings |
| Total rendered Editor/PIE Automation | **60/60 PASS**, process exit 0, zero errors |
| Repository / VN / Korean / Godot editor import | PASS; VN 21 files/526 steps, Korean 32 files/1,581 fields |
| Official Godot memory/world | **15/15 PASS**, fatal scan/save isolation enabled; exported catalog 1,064,635,280 bytes, export_log_errors=0 |
| Native memory / CTest | **51/51**, **1/1 PASS** |
| Host validator tests | **23/23 PASS** (12 existing + 11 catalog tests, including both Git checkout modes) |
| Foundation static | **50/50 PASS** |
| Protected original source | **4,217 files unchanged**; original HEAD and pre-existing status preserved |

The first export and two later source checks produced identical full IR bytes. Second import is an actual semantic no-op and additionally leaves package bytes unchanged. Binary equality is observed here; the contract remains semantic identity because Unreal metadata may legitimately vary across engine versions. No duplicate package was created.

Evidence: [first source export](evidence/phase1c/export01/source_export.json), [build/import/reimport/reload](evidence/phase1c/import02/pipeline.json), [exact 60-name report](evidence/phase1c/automation01/automation_index.json), [engine commands](evidence/phase1c/automation01/unreal_validation.json), [Godot](evidence/phase1c/regressions/godot_baseline.json), [native](evidence/phase1c/regressions/native_memory_validation.json), [host](evidence/phase1c/regressions/host_tests.json), [source protection](evidence/phase1c/regressions/foundation_static.json), [archived build logs](evidence/phase1c/log_archive.json). Godot metadata restoration completed for 1,056 files.

The total suite retains the Phase 1B map test's one nonblocking engine `r.MotionVectorSimulation` render-thread warning. All three new catalog tests have zero warnings; no engine code or accepted test was changed to hide it.

The three new executed Automation tests are `Memoria.Content.StartingCatalogParity`, `Memoria.Content.StartingCatalogRuntime`, and `Memoria.Content.StartingCatalogValidation`. They retain the previous 57 exact names; total executed count is 60. Source-derived expectations cover definitions/localization/provenance, initial order/connections/default mutable state, normal/silent burns, erosion, residue, carry, first passive threshold, full ordered events and event-time snapshots. Temporary modified IR loads only into a transient catalog, proves fingerprint changes, and is not saved as a production package.

## Defects and changes

- The first real compile exposed UE5.8 JSON keys stored as `FSharedString`; the importer now uses `ToView()` and `TryGetField`, without enabling a legacy compatibility macro. The same compile found the correct reflection flag is `EFieldIterationFlags::None`, not `ExcludeSuper`. The failed build and subsequent success are preserved.
- Installed API inspection found permissive number/string JSON conversions and case-insensitive default string equality. The new importer explicitly checks JSON types and uses case-sensitive comparison for identifiers, schema keys, metadata and canonical bytes. Rejection tests cover invalid representations and case-only distinct IDs. [API evidence](evidence/phase1c/api_review.json).
- New IR checkout policy pins LF without changing the accepted oracle or source bytes. Source Git revision plus normalized hashes and repeated executable extraction prevent silently accepting stale content. Existing validators now also protect historical `phase1b-ue58` evidence.
- There was no player-memory kernel defect and no need to change the 57 accepted Unreal tests, SaveGame schema 1, input/coordinate behavior, source values or foundation packages. The source comment describing the core name as late-game was treated as a comment, not permission to omit an actually initialized memory.

Changed/created files are enumerated in [changed_files.json](evidence/phase1c/changed_files.json). Runtime changes are limited to catalog metadata/typed localization and an explicit memory bootstrap entry point. Editor changes are the import helper, commandlet, SHA-256 dependency and three tests. Tooling changes are source export, strict IR validation, import/reimport orchestration, host tests, exact 60-name validation, historical evidence guards and scoped IR line endings. Documentation, IR/oracle, evidence and one production catalog asset complete the task.

## Reproducible commands

Run from the migration worktree. Use fresh evidence paths; historical Phase 0/1A/1B/UE58 evidence is protected.

```powershell
python Unreal/Tools/export_starting_memory.py --godot '<Godot-4.6.2-console.exe>' --evidence-dir docs/unreal-migration/evidence/phase1c/new-export
python Unreal/Tools/export_starting_memory.py --godot '<Godot-4.6.2-console.exe>' --check --evidence-dir docs/unreal-migration/evidence/phase1c/new-check
python Unreal/Tools/import_starting_memory.py --engine-root 'C:\Program Files\Epic Games\UE_5.8' --godot '<Godot-4.6.2-console.exe>' --build --evidence-dir docs/unreal-migration/evidence/phase1c/new-import
python Unreal/Tools/validate_unreal.py --engine-root 'C:\Program Files\Epic Games\UE_5.8' --build-and-test --rendered --evidence-dir docs/unreal-migration/evidence/phase1c/new-automation
python -m unittest discover -s Unreal/Tools -p 'test_*tools.py' -v
```

The import wrapper source-checks first, builds if requested, launches first import, repeated import and check-only reload in three separate UE processes, and compares fingerprints plus actual package bytes. The direct commandlet is `UnrealEditor-Cmd.exe Memoria.uproject -run=MemoriaStartingCatalog -IR=<canonical-IR> -Report=<new-report> -unattended -nop4 -NullRHI`; add `-CheckOnly` for a read-only comparison. Report paths must be fresh.

## Risks and exact next task

No Phase 1C acceptance blocker remains. Phase 1D has not started.

No packaging/cook, full New Game profile/inventory behavior, UI localization rendering, campaign, Chapter 1, Verdan, BattleManager, map/art/CG/audio import or Phase 1D implementation is included. A future source schema change must be reviewed and the extractor/schema version advanced as appropriate; unknown fields do not silently disappear. Import production data through the wrapper so source execution is checked before the editor importer.

Recommended Phase 1D: versioned shared narrative provenance/text/step IR with **separate field and VN typed contracts**, using `data/vn_scenes/ch2_market_arrival.json` and only the `verdan_arrival` group of `data/chapter2_dialogue.json` as bounded source-derived fixtures to verify original indices, choice order, effect/gate phases, continuation and deterministic reimport. Preserve field gate-before-effects versus VN effects-before-step-gates and original/filtered choice-index differences. Stop before campaign presentation; then plan the smallest canonical `ch2_market_arrival` -> Verdan arrival slice from the roadmap.
