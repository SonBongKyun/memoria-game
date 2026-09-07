# Migration handoff — Phase 1F complete on UE 5.8.2

Status: **Phase 1F COMPLETE**, bounded first NPC reaction.
Engine **5.8.2 / CL56702186** at `C:\Program Files\Epic Games\UE_5.8`.

- Branch: `unreal-migration/ue58-foundation`; worktree `C:\Users\jc\MemoriaMigration\foundation`.
- Previous technical checkpoint: `e9f781cf3f73ba5949ab9a2af961193d02d3c620`.
- Clean starting HEAD: `d53aaa84efa1a42c0e2078821e10520a998f58cd`.
- New technical checkpoint: `PENDING_LOCAL_COMMIT`; no push.
- [Phase1F report](PHASE_1F_REPORT.md), [acceptance](evidence/phase1f/acceptance.json), [final Automation](evidence/phase1f/automation05/automation_index.json).
- Open `/Game/Tests/Campaign/L_Ch2VerdanSlice`, choose original VN choice1 to burn `daily_market_food`. Real travel skips Field arrival and reaches exploration. Walk within80 units of Malet at(240,-160,0), press E/A.
- Actual BurnedHistory and source reaction priority drive `malet_taste_burned`. Real heard flag is set before Field.Start. Only originals0–2 execute once, then modal/context/focus/movement/Z/camera restore.
- `IMemoriaInteractable` and `UMemoriaInteractionComponent` resolve nearby eligible unoccluded actors; NPC delegates to narrative subsystem. Existing input/domain/Field/temporary UMG reused. Explicit physical release prevents held Interact from skipping row0.
- Second ordinary press and intact press record/defer `malet_encounter`. Persisted normal-talk completion requests `malet_memory_world_followup`. No normal target/chain imported or executed.
- New typed asset: deterministic source attestation, unchanged/no-save reimport, check-only reload, semantic-change rejection PASS. Old two IR/assets unchanged; six-process narrative pipeline and10 source cases PASS.
- **Previous72/72 + Phase1F4/4 =76/76** exact-name Automation PASS. Real UBT/UHT/compile/link/Editor/rendered PIE PASS. One retained engine render-thread warning; all new tests clean.
- NPC source oracle7/7, route4/4, Godot repo/VN/KO/import and official15/15, native51/51+CTest1/1, catalog, host45/45, static59/59 PASS. Original4217 files and15 other UE packages preserved; Verdan map and one new asset changed. Godot metadata1056 restored.
- Actual before/prompt/first/later/after/moved captures and trace under `evidence/phase1f/automation05/Phase1F`. Failed compile, held-input tests and transient DLL lock attempts retained with full logs.
- Development slice only: no normal deal/refusal/reward/shop, repeat world-memory group, Chapter3/autosave/achievements, extra NPC/battle/New Game/Ch1/final art/UI/audio/cook/package. No full Malet/Verdan parity.

Recommended **Phase1G**, not implemented: normal `malet_encounter` → original
refusal choice1 → `malet_refused` → source refusal cleanup → exploration/retry.
Preserve both authored choices; characterize Accept in the oracle and defer the
whole selection before partial effects in development. Keep deal/reward/shop/
Chapter3 and repeat world-memory group deferred. See report for exact dependencies.
