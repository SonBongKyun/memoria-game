# Migration handoff — Phase 1A

Status: **PARTIAL — source foundation and native memory parity implemented; exact UE 5.7 validation pending.** No UE build/editor/Automation success is claimed.

- Branch: `unreal-migration/foundation`
- Worktree: `C:\Users\jc\MemoriaMigration\foundation`
- Project: `Unreal/Memoria/Memoria.uproject`
- Base HEAD: `15df72809baa52eec281d8508f495a373e9f5884`
- Original dirty checkout remains at `C:\Users\jc\OneDrive\바탕 화면\메모리아\Game`; continue migration in this worktree.
- No local checkpoint commit or remote push created for Phase 1A.

Implemented: runtime/editor-test modules, exact engine/version and export/Git boundaries; XY/orthographic/Paper2D/Enhanced Input component foundation; GameInstance-owned run aggregate and player-memory UObject; case-sensitive IDs and typed flags/player state; separate field/VN contract types; UE save schema 1 and slots 0–3; separate reserved world/diary/hint sections. No runtime narrative interpreter, legacy save importer, world-cognition service, battle serialization or campaign content import exists yet.

Memory: raw grades 0..4, normal/silent burns, eligibility, history/residue/faded state, chapter erosion, cascade graph, guards, effective power, carry/overload and five passive thresholds. Loan/extraction/preservation fields survive DTO reconstruction; their creation, repayment, extraction, synthesis and preservation commands are deferred. Shop sale must later invoke the burn pathway, while synthesis must not.

Evidence: 51 actual-Godot vectors, 95 commands, 109 event observations; standalone MSVC parity and CTest pass. Unreal Automation has 51 shared cases + 3 reflection/save/ownership tests ready to execute, currently **not run**. See [PHASE_1A_REPORT](PHASE_1A_REPORT.md) and [phase1a evidence](evidence/phase1a/) for exact Godot baseline and source-preservation results.

Blocker: rechecked Launcher, registered builds, known installation parents and engine environment roots; only UE **5.8.2** detected (`C:\Program Files\Epic Games\UE_5.8`). Target remains **5.7**. No install or retarget is authorized. Native C++ testing does not validate UHT, UObject linking, editor or visuals.

Exact Phase 1B task: **with a real UE 5.7 installation, compile MemoriaEditor and pass all 54 Automation cases, then create one isolated 2D foundation test map with authored Enhanced Input assets and one modal demonstrating single Back consumption.** Validate source-pixel coordinate round trips, Paper2D orientation/foot pivots, movement/collision and memory event observers in the real editor. Resolve UHT/build/test issues before importing campaign content. Do not add Ch1 battle content; the first later integration slice remains Verdan arrival/trade and the existing Ch2-complete revisit battle.

Commands from this worktree:

```powershell
python Unreal/Tools/validate_foundation.py --original-manifest 'C:\Users\jc\MemoriaMigration\phase1a-original-manifest.json'
python Unreal/Tools/validate_native_memory.py
python Unreal/Tools/validate_godot_baseline.py --godot 'C:\Users\jc\Downloads\Godot_v4.6.2-stable_win64.exe\Godot_v4.6.2-stable_win64_console.exe'
python Unreal/Tools/validate_ue57.py --engine-root '<UE_5.7-root>' --build-and-test
```

Nonnegotiable rules: preserve Godot and Phase 0 documents; raw grade ordinals never reverse; memory event-time state matters; player memory and actor cognition never share inventory operations; field choices use filtered indices while VN uses original indices and different effect/payment ordering; source Godot 0.4.0 is not UE schema 1; no live battle save; no generic remove command for burn/sell/synthesis; no percentage loss gauge. Treat the frozen model context and reentrant-mutation rejection as explicit foundation boundaries, not proof that every future listener has been ported.
