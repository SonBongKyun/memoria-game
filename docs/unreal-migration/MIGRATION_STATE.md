# Migration handoff — Phase 1E complete on UE 5.8.2

Status: **Phase 1E COMPLETE**, bounded development slice. Engine **5.8.2 / CL56702186**
at `C:\Program Files\Epic Games\UE_5.8`. Historical Phase 0–1D evidence is preserved.

- Branch: `unreal-migration/ue58-foundation`.
- Worktree: `C:\Users\jc\MemoriaMigration\foundation`.
- Previous technical checkpoint: `ee61269daf5ee20724a02eb51d07d997f26a8474`.
- Clean starting HEAD: `a9dfae9e3c337782ba832c9577319faac5f30152`.
- New technical checkpoint: `e9f781cf3f73ba5949ab9a2af961193d02d3c620`; no push.
- [Phase 1E report](PHASE_1E_REPORT.md), [acceptance](evidence/phase1e/acceptance.json), [final Automation](evidence/phase1e/automation05/automation_index.json).
- Open `/Game/Tests/Campaign/L_Ch2VerdanSlice` and Play: imported VN originals 0–12 execute, terminal 12 sets `ch2_arrival_vn_seen` before requesting Verdan, native OpenLevel reaches `/Game/Tests/Campaign/L_VerdanHost`, Field invocation stays 0, exploration/movement activates.
- Separate `/Game/Tests/Campaign/L_VerdanUnseenFixture`: imported Field rows 0–4 execute once with VN-seen false, then exploration resumes.
- `UMemoriaNarrativeSubsystem` owns both separate interpreters/typed assets across travel, borrowing the RunSubsystem aggregate. Temporary `UMemoriaDevelopmentNarrativeWidget` only displays values and forwards original choice IDs. Existing Enhanced Input/modal/pawn/camera reused.
- Actual rendered replays select original choice 1 (food memory payment) and filtered visible second/original 2. Active-host SaveGame schema1 round-trip preserves current/pending/FIFO semantics; invalid restore is atomic; run replacement cancels stale UI.
- **Previous 68/68 + Phase 1E 4/4 = 72/72** exact-name Automation PASS; actual UBT/UHT/compile/link/Editor/PIE PASS. Four source route oracle cases and ten retained Phase1D oracle cases pass. Six narrative reimport/reload processes are unchanged with no save.
- Godot repository/VN/KO/import and official **15/15**, native **51/51**, host **38/38**, static **56/56** PASS. All **4,217 original files**, 13 accepted UE packages, schema1 and historical evidence are unchanged. Godot metadata1,056 restored.
- Failed compiler/oracle/duplicate-PIE observer attempts are preserved in full_logs.zip. Final source route, state, UI and travel checks pass; one pre-existing engine render-thread warning remains.
- Scope remains a development slice: no production New Game/Ch1, NPCs/trade/shop, battles, final UI/art/audio, cook/package or full-campaign parity.

Recommended **Phase 1F**: paid Verdan arrival → one placeholder Malet interaction →
the source three-row `malet_taste_burned` memory-loss reaction → exploration.
Preserve PerceptionFilter priority and `burn_reaction_heard_malet_taste_burned`
being set before the dialogue. Characterize intact/already-heard fallback dispatch
without silently redirecting it or expanding into the normal deal/shop/chapter chain.
This bounded first reaction is the next NPC dependency; it is not implemented here.
See the report for source dependency analysis and reproduction commands.
