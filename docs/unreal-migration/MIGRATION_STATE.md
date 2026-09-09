# Migration handoff — Phase 1I complete on UE 5.8.2

Status: **Phase1I COMPLETE**, reward8 completed; `_on_reward_ended` deferred before
its first authoritative effect `ch2_malet_done`.
Engine **5.8.2 / CL56702186**, `C:\Program Files\Epic Games\UE_5.8`.
Worktree `C:\Users\jc\MemoriaMigration\foundation`; branch `unreal-migration/ue58-foundation`.
Previous technical checkpoint `5010a26ca934047d27160af3672a93a0caaf2e14`;
clean documentation start `bf75dad`. The new local checkpoint is the enclosing
`feat(unreal): add Malet reward dialogue boundary` commit; resolve its SHA with
`git log -1 --format=%H -- docs/unreal-migration/PHASE_1I_REPORT.md`. No push.

- [Report](PHASE_1I_REPORT.md), [97-test result](evidence/phase1i/automation01/automation_index.json),
  [193-check independent acceptance](evidence/phase1i/acceptance01/acceptance.json),
  [full canonical trace](evidence/phase1i/acceptance01/canonical_full_trace.txt).
- Paid VN original1 -> actual food burn -> native travel -> physical walk/E -> first
  reaction -> E/normal -> Accept0 -> same-domain sword payment -> real0.3s -> deal0..4
  -> real0.5s -> reward0..7 -> Field end -> source exploration event -> synchronous
  reward callback intent -> development pre-effect stop. No canonical fixture reset.
- Only new production narrative group/package is `malet_reward` / `DA_Field_MaletReward`.
  English/Korean, original indices, presence and provenance exact. Import CREATED,
  second and check-only UNCHANGED/no-save.20 existing UE packages preserved.
- Same live run/domain, food+sword history, accepted/heard flags, full memory,
  inventory/items/chapter retained. No `ch2_malet_done`, world seeding, reward items,
  shop, Chapter3 request, autosave or achievement execution. Unimplemented world/shop
  snapshots are honestly null; actual DTO/trace and static boundary proof accompany them.
- Source8 cases characterize full reward callback and both completion lifetime cuts.
  Source state-only replacement can retain callbacks; Unreal deliberately keeps its
  accepted OnRunReplaced cancellation. No new reward-completion timer. Deferred
  modal consumes input and remains stable; source exploration is only a traced
  synchronous transition, not an invented free-exploration frame.
- Previous90 exact identities + new7 =97/97 PASS. Historical AcceptPreEffectDeferred
  and payment test names retained; assertions now extend to reward pre-effect stop.
  Exact H payment/timers, G Refuse cleanup/retry and F reaction regressions preserved.
- UBT/UHT/compile/link/Editor/rendered PIE PASS. Existing MotionVectorSimulation
  warning1 remains unchanged. Source H9/G7/F7/E4/D10/C7; officialGodot15; native51+CTest1;
  host61; static59/original4217; original checkout HEAD/status and old20 packages PASS.
- Five native reward/completion/deferred PNGs directly inspected,133 valid snapshots.
  Failure logs retained. Full Malet parity, final visuals/KO typography and Field
  save-resume are not claimed. All absolute downstream scope bans remain in place.

**Phase1J has not started.** Recommend only the first authoritative reward effect:
set `ch2_malet_done` on this same run, then stop before world-memory seeding can mutate.
Require separate authorization, source characterization,97 retained identities and
fresh PIE/lifecycle evidence. Items/shop/Chapter3/autosave/achievements remain deferred.
