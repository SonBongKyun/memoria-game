# MEMORIA Unreal 5.8.2 foundation

Read [MIGRATION_STATE](../../docs/unreal-migration/MIGRATION_STATE.md) before extending this project. Godot remains the behavioral reference. The user changed the target from 5.7 to installed **5.8.2** on 2026-09-06, before any engine-bound assets existed. Historical reports retain their original target.

`Memoria` owns runtime code; editor-only `MemoriaTests` owns Automation and the explicit foundation-asset commandlet. Build settings use **V7 / Unreal5_8 / C++20**. The project association is `5.8`; the validator additionally requires the exact **5.8.2** Build.version and records the actual changelist.

`UMemoriaRunSubsystem` retains `UMemoriaPlayerMemoryDomain` through UPROPERTY. The portable memory model remains shared with native tests. Flags and item identities compare case-sensitively, including during snapshot validation. NPC cognition remains an independent deferred domain.

`/Game/Tests/Foundation/L_FoundationTest` is isolated test content. It uses the existing Pawn/controller, four Enhanced Input actions, exploration/modal contexts, a controlled asymmetric sprite with bottom-center foot pivot, and one UMG modal. No campaign data or production art is imported. Coordinate schema 1 maps Godot `(x,y)` to `(x,-y,0)`, one unit per pixel. The -Z camera uses world +Y at the top; `UMemoriaLocalPlayer` reflects view X so world +X appears right and rendering, projection/deprojection and culling share the same basis. UE screen-ray deprojection intentionally truncates to integer pixels; this does not change the source/world coordinate contract.

Exploration and modal input have one semantic owner. Started events drive confirm/menu/Back; Triggered drives movement. The higher-priority modal context claims Escape, B and Start; mapping changes ignore already-held keys until release. Opening stops residual movement, closing restores viewport focus. Test replay enters actual Enhanced Input through simulated keyboard/gamepad key events; it does not certify USB hardware or the full campaign UI. The historical Godot sprint glyph says LB while its configured button is index 9; this foundation does not resolve that discrepancy or implement sprint.

```powershell
python Unreal/Tools/validate_unreal.py --engine-root 'C:\Program Files\Epic Games\UE_5.8' --build-and-test --evidence-dir docs/unreal-migration/evidence/phase1b-ue58/new-run
python Unreal/Tools/validate_unreal.py --engine-root 'C:\Program Files\Epic Games\UE_5.8' --build-and-test --rendered
python Unreal/Tools/validate_native_memory.py
python Unreal/Tools/validate_foundation.py --original-manifest 'C:\Users\jc\MemoriaMigration\phase1a-original-manifest.json'
python Unreal/Tools/validate_godot_baseline.py --godot '<Godot-4.6.2-console.exe>'
python -m unittest discover -s Unreal/Tools -p test_validation_tools.py -v
```

On a fresh checkout without foundation assets, add `--create-foundation-assets` to the first UE invocation. The commandlet saves real packages through installed Unreal tooling and refuses to replace existing packages. Normal validation loads the committed assets; it does not regenerate them. `--rendered` enables the real graphics backend, offscreen Editor/PIE and test screenshots. NullRHI results alone are not visual approval.

The UE runner retains the **57 accepted Phase 1B tests** and adds three starting-catalog tests: **60 total**. The original 54-test subset and 51 Godot oracle cases remain explicit. Missing, duplicated, unexpected, malformed, skipped or failed results fail validation. Fixtures are checked before UBT. Compiler errors, fatal diagnostics, timeouts and nonzero subprocess exits fail. Large immutable fixture builders alone are unoptimized to avoid MSVC C4883; production memory rules retain normal optimization and unchanged oracle expectations.

Without an evidence directory, validators write timestamped ignored `Saved/Validation` reports. Historical `evidence/phase0`, `phase1a`, `phase1b` and `phase1b-ue58` destinations are rejected. Input/output fixture line endings remain pinned to their attested bytes. Godot regression isolates subprocess app-data and restores editor-rewritten metadata. Unreal binaries/caches remain ignored; Git LFS is scoped to Unreal packages. No push or legacy Godot save import is implied.

AndroidFileServer is explicitly disabled: its default editor module otherwise writes an unrelated generated server token into the Windows foundation config. Paper2D, Enhanced Input and all MEMORIA systems remain enabled.

Phase 1C's [source-derived starting catalog](../../docs/unreal-migration/ir/README.md) lives at `/Game/Memoria/Generated/Memory/DA_StartingMemoryCatalog`. `UMemoriaRunSubsystem::BeginStartingMemoryRun()` explicitly loads it and initializes owned memories in source order without travel or narrative. The runtime uses typed definitions and separate localization rows; importer/provenance checking and SHA-256 tooling stay in the editor-only module. SaveGame schema 1 and the original memory rules are unchanged.

```powershell
python Unreal/Tools/export_starting_memory.py --godot '<Godot-4.6.2-console.exe>' --check
python Unreal/Tools/import_starting_memory.py --engine-root 'C:\Program Files\Epic Games\UE_5.8' --godot '<Godot-4.6.2-console.exe>' --build --evidence-dir docs/unreal-migration/evidence/phase1c/new-import
python -m unittest discover -s Unreal/Tools -p 'test_*tools.py' -v
```

Omit `--check` from the exporter only when deliberately generating a reviewed source revision. The import wrapper always checks source execution first, then performs import, unchanged reimport and check-only reload in separate engine processes. See [PHASE_1C_REPORT](../../docs/unreal-migration/PHASE_1C_REPORT.md).
