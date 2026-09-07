# Migration handoff — Phase 1G complete on UE 5.8.2

Status: **Phase 1G COMPLETE**, bounded normal Malet encounter -> refusal -> cleanup/retry.
Engine **5.8.2 / CL56702186**, `C:\Program Files\Epic Games\UE_5.8`.

- Worktree `C:\Users\jc\MemoriaMigration\foundation`; branch `unreal-migration/ue58-foundation`.
- Previous technical checkpoint `fea1cdd3c7fc1947e2fa8b55d86f39a21f7a1d24`; clean starting documentation HEAD `0f37e58318a749a5d8057a5303c4f18e022f864c`.
- New technical checkpoint: `361b9039713446957f1da66470881990388762ba`. This documentation follow-up records that accepted feature commit. No push.
- [Report](PHASE_1G_REPORT.md), [acceptance](evidence/phase1g/acceptance.json), [final82-test report](evidence/phase1g/automation02/automation_index.json), [exact retry trace](evidence/phase1g/retry_exact_trace.txt).
- Open `/Game/Tests/Campaign/L_Ch2VerdanSlice`. Select original paid VN choice1, travel, walk to Malet, E and finish the accepted3-row reaction. E again starts `malet_encounter` originals0..9. Select original refusal1, wait the source0.3s callback, advance `malet_refused` originals0..2. Cleanup restores exploration and movement; ordinary E retries original0. Acceptance stops there.
- Both original choices and English/Korean source data are intact. Original Accept0 is visible but host-deferred before interpreter effects: accepted flag absent, sword intact, full run/memory unchanged, no rollback or downstream request.
- Two new typed Field assets passed first import, fresh-process unchanged reimport, check-only reload, semantic/source attestation and transient rejection probes. All17 accepted packages unchanged.
- The normal/refusal callback order and source cache/flag erasure match the executable oracle. Final real timer measured300000us. Pending timer cannot mutate a new run; no save-schema/controller/interpreter redesign.
- Previous76 exact IDs + new6 = **82/82** Automation, real UBT/UHT/compile/link/Editor/rendered PIE PASS. The existing `r.MotionVectorSimulation` warning remains1; new tests have0 warnings/errors.
- Phase1G oracle7; Phase1F oracle7/import; Phase1E route4; Phase1D oracle10/six-process import; Phase1C catalog; officialGodot15; native51+CTest1; host50; static59 PASS. Original4217 files and historical reports/evidence unchanged.
- Required7 final PNGs plus Accept view were directly inspected; final capture JSON parses. Failed oracle/build/first evidence-format attempts and full logs remain archived. See report for defects/limits.
- No deal/reward/repeat-world content, sword payment in the playable route, world seeding/shop/trade/Chapter3/autosave/achievements, extra NPC/battle/NewGame/Ch1/finalart/audio/graphics/cook/package/Steam implementation. No full Malet parity claim.

Recommend **Phase1H only if separately authorized**: original Accept0 flag plus one real sword burn -> source0.3s callback -> import/execute only `malet_deal` originals0..4 -> real0.5s callback -> record `malet_reward` request and stop before reward execution. Characterize source first; preserve refusal/retry and82 IDs. Keep reward8/world-memory/items/shop/Chapter3/autosave/achievements/repeat group deferred. **Phase1H has not started.**
