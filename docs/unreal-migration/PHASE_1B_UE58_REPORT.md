# Phase 1B — UE 5.8.2 foundation

Date: 2026-09-06. Status: **COMPLETE — bounded engine foundation accepted**. Phase 1C has not started.

## Target and checkpoints

The user intentionally changed the target **UE 5.7 -> UE 5.8.2** on 2026-09-06. The earlier version was an internal baseline, not a product constraint; 5.8.2 was already installed, and no engine-bound migration assets existed at the change. Phase 0/1A/1B reports and their evidence retain their historical 5.7 references.

- Worktree: `C:\Users\jc\MemoriaMigration\foundation`.
- Branch: `unreal-migration/ue58-foundation`, created from verified clean HEAD `a58b1e15fe2fb6c86568b8df45d279c4f0844ca1`.
- Phase 1A checkpoint: `2b2a2607296faa1de2f4e8bf94f4eec847bfc8f9`.
- Previous Phase 1B checkpoint: `d77c566a141441ac8f935871a7756467eb9373de`.
- New technical checkpoint: **`44d08dde96b352c3f86b770ed11948bcc18f6bb6`** — `feat(unreal): validate foundation on UE 5.8.2`. This SHA is recorded by a documentation-only follow-up so the technical commit need not contain its own hash. No push.
- Engine root actually used: **`C:\Program Files\Epic Games\UE_5.8`**.
- Directly read `Engine/Build/Build.version`: **5.8.2, CL 56702186**, compatible CL 55116800, promoted build, branch `++UE5+Release-5.8`.

Project association uses the installed launcher's `5.8` identifier. The generalized `validate_unreal.py` additionally requires all three version components **5/8/2** and records the actual changelist. It replaces the former validator rather than duplicating it. Tests reject other 5.8 hotfixes. Build targets use **V7 / Unreal5_8**, with C++20.

## Real engine work

Installed headers and implementation were inspected for build rules, reflection/ownership, subsystem/GC lifetime, delegates, SaveGame, Automation, Pawn/controller, Enhanced Input, Paper2D, camera, world/package factories and UMG. See [API review evidence](evidence/phase1b-ue58/api_review.json).

UBT, UHT, compilation and both module DLL links have genuinely executed successfully. The Editor loads the project and runs PIE through `UnrealEditor-Cmd`. No installation, engine substitution, packaging or source expectation regeneration occurred.

Every build/authoring/test attempt has its own `evidence/phase1b-ue58/attempt*` record. Failed attempts are retained. The validator checks process exit, timeouts, fatal diagnostics and exact test identities. It never counts native or host tests as Unreal tests. The legacy required suite is **54** (51 Godot oracle cases plus three original foundation tests); the expanded suite is **57**, adding:

- `Memoria.Foundation.RuntimeLifetime`
- `Memoria.Foundation.NondefaultSaveRoundTrip`
- `Memoria.Foundation.MapInputAndModal`

The earlier rendered attempt passed 57/57 but visual inspection exposed a rotated screen basis; it was not accepted as final visual proof. The subsequent projection and initialization checks make that gap explicit. Final **attempt12-rendered** passed all 57 tests with real graphics, process exit 0 and no fatal diagnostics. Its captures were visually inspected and show the upright sprite and working UMG modal.

## Final acceptance

| Check | Actual result |
| --- | --- |
| Installed engine | 5.8.2 / CL 56702186, read from Build.version |
| UBT / UHT / C++ / module linking | PASS; real builds and generated reflection code, both module DLLs linked |
| Editor / PIE | PASS, unattended Editor command process with offscreen graphics |
| Legacy exact-name Automation suite | **54/54 PASS**, including all 51 unchanged source oracle cases |
| Total exact-name Automation suite | **57/57 PASS**, zero test errors |
| Subsystem / UObject lifetime / full GC | PASS, retained ownership and eventual release |
| Nondefault SaveGame / schema 1 / slots 0-3 | PASS, every reflected SaveGame field round-tripped |
| Map / Enhanced Input / UMG packages | PASS, ten genuine saved packages loaded at runtime |
| Movement / camera / sprite / foot / coordinates | PASS, numeric assertions and inspected rendered captures |
| Back / confirm / focus | PASS, one Back dispatch per simulated key press through Enhanced Input |
| Native parity / CTest | **51/51**, **1/1 PASS** |
| Godot repository / VN / Korean / import / official suite | PASS; official memory/world **15/15**, export-log errors **0** |
| Host validator regressions | **12/12 PASS** |
| Static / original-file protection | **49/49 PASS**, **4,217 original files unchanged** |

Evidence: [acceptance and asset hashes](evidence/phase1b-ue58/acceptance.json), [final engine commands and result](evidence/phase1b-ue58/attempt12-rendered/unreal_validation.json), [all 57 test identities and observations](evidence/phase1b-ue58/attempt12-rendered/automation_index.json), [archived full command logs](evidence/phase1b-ue58/log_archive.json), [Godot retry](evidence/phase1b-ue58/regressions-retry/godot_baseline.json), [native](evidence/phase1b-ue58/regressions/native_memory_validation.json), [static](evidence/phase1b-ue58/regressions/foundation_static.json), [host tests](evidence/phase1b-ue58/regressions/validator_unit_tests.json).

Final captures inspected: [upright sprite](evidence/phase1b-ue58/attempt12-rendered/FoundationSprite.png) and [visible modal with confirm response](evidence/phase1b-ue58/attempt12-rendered/FoundationModal.png). These are actual 1280x720 Editor/PIE captures, not mockups.

One nonblocking engine warning remains in the final map test: `r.MotionVectorSimulation` is used on the render thread without `ECVF_RenderThreadSafe`. The warning and all logs are retained; no engine code was patched to suppress it. No central acceptance blocker remains. Packaged builds, physical USB device testing and campaign parity are outside this phase.

## Actual assets

All packages are under `/Game/Tests/Foundation` (`Unreal/Memoria/Content/Tests/Foundation` on disk), created by installed Unreal tooling:

| Purpose | Asset |
| --- | --- |
| Isolated map | `L_FoundationTest.umap` |
| Movement | `IA_Move.uasset` |
| Interact/Confirm | `IA_Confirm.uasset` |
| Back/Cancel | `IA_Back.uasset` |
| Menu | `IA_Menu.uasset` |
| Exploration mapping | `IMC_Foundation.uasset` |
| Modal mapping | `IMC_Modal.uasset` |
| Controlled texture and sprite | `T_FootPivot.uasset`, `SPR_FootPivot.uasset` |
| Real UMG Widget Blueprint | `WBP_FoundationModal.uasset` |

The editor-only commandlet uses `UWorldFactory`, `UPackage::SavePackage`, Enhanced Input UObject assets and actual Widget Blueprint compilation. It refuses to replace existing packages. Partial authoring output from failed attempts was retained in ignored `Saved/Validation` before a fresh complete authoring run succeeded. No text was disguised as a binary asset; runtime tests reload the packages.

The sprite is deliberately asymmetric validation geometry: warm head, violet body, red right-hand marker, cyan feet. It is 32x48 source pixels with bottom-center pivot `(16,48)` and 1 pixel per Unreal unit. No production character art was imported.

## Runtime coverage

The expanded lifetime test uses a retained GameInstance and forces full GC, checks subsystem/domain ownership and eventual release, initializes an explicit in-memory test catalog, reconstructs a nondefault run, observes ordered command events and rejects reentrant flag mutation. This catalog is test data, not a starting-memory content import.

Save tests call real `UGameplayStatics::SaveGameToMemory` / `LoadGameFromMemory` for autosave slot 0 and manual slots 1-3. Every reflected SaveGame field is compared after round trip. Nondefault data includes run ID/revision, chapter 6, Korean locale, player HP/max HP/Grains/party/focus/streak/items/quick slots/history, independent case-sensitive flags, memory definitions/provenance/owned order/burn/residue/fade/erosion/passives/loan/guards/extractions, source scene/map/coordinates, current/pending/FIFO VN continuation and separate nonempty world/diary/hint sections. No disk UX, live battle serializer or Godot 0.4.0 compatibility is claimed.

Input replay goes through actual Enhanced Input using simulated keyboard/gamepad key events. It tests move/stop, stable Z, modal movement blocking, confirm press/hold, Escape opening, release then Back closing, no reopen while held, gamepad Start/A/B, and focus/movement return. These are engine key-path tests; USB device behavior and the whole campaign's controller coverage are not certified.

Coordinate schema 1 remains `(x,y) -> (x,-y,0)`. The test restores `(137.25,-83.5)` to `(137.25,83.5,0)`. The -Z camera uses a +Y screen-up basis; a scoped `UMemoriaLocalPlayer` view-X reflection keeps +X screen-right and keeps projection/deprojection and culling consistent. Saved/world coordinates are not altered for rendering. Tests check plane, camera normal, sprite normal/up, foot bounds and screen axes, plus rendered captures.

The restore tolerance is 0.001 world units. Final keyboard movement takes `(137.25,83.5,0)` to `(327.25,83.5,0)`; analog movement and modal return also pass with stable Z. Projected +X moves screen X from 562.831 to 576.431; +Y moves screen Y from 236.513 to 222.913. UE deprojection deliberately truncates input to integer pixels: its separate assertion checks reprojection to that truncated pixel within 0.01 pixel and world error within one pixel of the measured scale. This does not relax saved-coordinate restoration. The completed replay records two Back dispatches (one keyboard, one gamepad), four modal transitions and two confirms across 240 frames.

The Godot sprint hint says LB while its configured button index is 9. That historical discrepancy is retained separately; sprint and full controller UI migration are outside this phase.

## Defects and fixes

1. **V6 shared build settings conflict:** real UBT rejected backward defaults that differed from UE5.8 Editor products. Adopted installed V7 defaults; no override/unique-engine workaround.
2. **Incomplete portable model in generated UObject code:** UHT-generated vtable construction instantiated the unique-pointer deleter with an incomplete type. Included the complete model definition; reflection and ownership remain intact.
3. **UE controller return type:** `GetPawn()` returns `TObjectPtr<APawn>`; changed invalid `auto*` deduction to explicit `APawn*`.
4. **MSVC C4883 in a large attested fixture builder:** disabled optimization only around immutable generated fixture construction using current UE macros. Production rules and all expected values remain unchanged.
5. **Commandlet discovery and world authoring:** editor module loading moved to Default so the commandlet is discoverable. Replaced interactive mode-tool map creation with commandlet-compatible `UWorldFactory`; authoring now refuses existing packages instead of attempting partial overwrites.
6. **Case-sensitive run IDs:** the default FString set rejected flags/items that differed only in case. Added matching case-sensitive equality/hash for validation, preserving exact-duplicate rejection. New nondefault save/lifetime tests reproduced this divergence.
7. **Sprite foot direction and screen basis:** real runtime bounds found the sprite below the foot; actual rendering then found a sideways camera basis. Corrected sprite rotation and scoped view basis, and added screen projection/deprojection assertions.
8. **Test harness corrections:** aligned the new connected-burn trace with unchanged Godot behavior when Elia is absent; copied an array element before adding a duplicate test entry; synchronized input replay with actual frames and stable PIE initialization; accounted for documented-in-source integer-pixel deprojection in the screen-ray assertion. The source-coordinate tolerance remains 0.001 units. These changes do not alter the original 51 oracle expectations.
9. **Unrelated Android server config:** disabled the default AndroidFileServer plugin to prevent it writing an unrelated generated server token/config into this Windows-only foundation. No MEMORIA system or test was disabled.

## Regression and source preservation

Native parity passes **51/51**, CTest **1/1**. Static checks pass **49/49**, including **4,217 original protected files** and the ten package signatures. Host validator tests cover exact identities, patch matching, historical evidence guards and both Git line-ending modes.

The first Godot regression attempt passed repository/VN/Korean checks but its import process exited with Windows access violation code 3221225477. That failed result and log are preserved. A separate serial retry passed all five checks, including import exit 0 and the official 15-case suite; 1,056 metadata files were restored. The exported actor catalog is 1,064,635,280 bytes with zero export-log errors. No source behavior was changed to address the failure; its cause was not established.

Final scoped Git comparison against starting HEAD confirms unchanged Godot scripts/scenes/assets/data/addons/project configuration, portable memory model, attested fixtures and historical Phase 1A/1B reports/evidence. The original Godot checkout still has its original HEAD and pre-existing working changes; no commit was made there.

## Commands and next scope

Run from the migration worktree:

```powershell
python Unreal/Tools/validate_unreal.py --engine-root 'C:\Program Files\Epic Games\UE_5.8' --build-and-test --rendered --evidence-dir docs/unreal-migration/evidence/phase1b-ue58/new-run
python Unreal/Tools/validate_native_memory.py --evidence-dir docs/unreal-migration/evidence/phase1b-ue58/regressions
python Unreal/Tools/validate_foundation.py --original-manifest 'C:\Users\jc\MemoriaMigration\phase1a-original-manifest.json'
python Unreal/Tools/validate_godot_baseline.py --godot 'C:\Users\jc\Downloads\Godot_v4.6.2-stable_win64.exe\Godot_v4.6.2-stable_win64_console.exe'
python -m unittest discover -s Unreal/Tools -p test_validation_tools.py -v
```

Phase 1B acceptance is complete. The exact next Phase 1C task is deterministic export of the actual starting-memory catalog into versioned IR and import into a typed Unreal catalog asset; verify repeated import preserves IDs, owned order and semantic content against source hashes. This prepares the established Verdan arrival/trade slice. No starting catalog asset, Chapter 1 reconstruction, campaign dialogue, BattleManager, Verdan or bulk art/audio was migrated here. Phase 1C remains unstarted.
