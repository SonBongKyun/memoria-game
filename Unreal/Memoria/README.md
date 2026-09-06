# MEMORIA Unreal 5.7 foundation

Read [MIGRATION_STATE](../../docs/unreal-migration/MIGRATION_STATE.md) before extending this project. The existing Godot project remains the executable reference.

`Memoria` is the runtime module; `MemoriaTests` is editor-only. Paper2D provides plane-facing sprite presentation, and Enhanced Input owns the input foundation. No GAS, multiplayer, CommonUI or speculative plugins are enabled. Build settings are pinned to V6 / Unreal5_7, C++20, with `EngineAssociation` 5.7.

`UMemoriaRunSubsystem` owns player/run state and `UMemoriaPlayerMemoryDomain`. The reflected domain adapter publishes typed synchronous events; the production `MemoriaMemoryModel.cpp` contains the authoritative rules. The same source is compiled by the standalone parity target. NPC cognition is a separate, deferred domain.

The field Pawn has XY movement constraints, a -Z orthographic camera, collision and a Paper2D sprite component oriented into the plane. Position schema 1 converts Godot `(x,y)` to Unreal `(x,-y,0)`, one unit per source pixel. Component sizes and movement tuning are unvalidated defaults, not certified movement parity. Phase 1B must create the test map and Enhanced Input assets, configure the controller class, then validate framing, pivots, collision and input. No fake `.umap` or `.uasset` files are provided. Launching this foundation does not start the campaign or populate every memory in a catalog.

From the worktree root on Windows:

```powershell
python Unreal/Tools/validate_foundation.py
python Unreal/Tools/validate_native_memory.py
python Unreal/Tools/validate_godot_baseline.py --godot '<Godot-4.6.2-console.exe>'
python Unreal/Tools/validate_ue57.py --engine-root '<UE_5.7-root>' --build-and-test
```

The UE runner requires a verified 5.7 `Build.version`, invokes `Build.bat MemoriaEditor Win64 Development`, and launches unattended `UnrealEditor-Cmd` with `Automation RunTests Memoria.` and a unique JSON report directory. It rejects failed builds, fatal diagnostics, timeouts, missing/zero tests, incomplete fixture discovery or any nonsuccess test state. Expected discovery is 51 source parity cases plus 3 foundation tests. Exit 2 means the required toolchain is unavailable; it is never a compilation pass. Editor/map visual validation remains separate from this NullRHI test run.

Generated files stay in this project's `Intermediate`, `Saved`, `Binaries` and cache directories. `Unreal/.gdignore` excludes the entire subtree from Godot import; the existing Godot export preset also excludes it. Git LFS applies only to `Unreal/**/*.uasset` and `Unreal/**/*.umap`; existing PNG/audio assets are untouched. No LFS history migration or mass normalization is performed.

The Godot baseline runner redirects app-data for every child and restores pre-run bytes of editor-rewritten `project.godot`/`.import` metadata. Run it in an isolated worktree with no concurrent edits to those files. A cold cache may require a second import after dependencies are generated; a failed attempt remains a failed attempt and must be recorded. The official suite owns expected exit-1 guard checks and exported-catalog validation.
