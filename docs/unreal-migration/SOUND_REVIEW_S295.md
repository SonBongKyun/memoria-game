# S295 - Claude sound pass review and integration (2026-09-26)

## Scope and history

Reviewed `792034ff..9c3b2033` (five Claude commits) against the previous Codex snapshot `fd76c0fe`. Outside historical evidence, the pre-sound source trees match; the remaining baseline differences are ignore rules and documentation. The clean Codex worktree now uses `codex/unreal-sound-review-20260926` based on Claude `9c3b2033`. The original Codex branch and snapshot ref still retain `fd76c0fe`; no rebase, hard reset, peer write, foundation merge or remote push was used. There are zero tracked files under `docs/unreal-migration/evidence` in the integrated tree.

Accepted source assets: 25 generated WAV cues/loops and 27 Unreal sound packages (23 SFX, two music, two ambient). The pasted transcript's intermediate count of 29 packages was inaccurate; the final handoff and actual tree both contain 27. Existing binary assets and authored Godot content were not edited.

## Findings and changes

1. **P1 - synchronous shop request callbacks could continue into a replaced run.** `OnRequestRecorded` was added inside open/sell/buy/close without the revision guards already used for other observers. A callback at `ui_close` could replace the run before the old close wrote chapter 3 and the completion flag. The payload also referenced `Requests.Last()`, which a replacement clears. Each request now owns a stable string and returns whether the owner/revision survived; callers stop immediately on cancellation. Opening also rejects reentrant shop operations.
2. **P2 - a burn drama survived new game or save restore.** The persistent audio subsystem retained its timed rising-tone/ignition sequence. It now listens to `OnRunReplaced` (including same-ID restore), cancels the old sequence, restores loop levels and resets transient encounter/debounce observations.
3. **Audio lifecycle and verification.** The context loop retries if a named track has no playing component; fading/stopped components created without auto-destroy are explicitly destroyed. A dialogue duck change during burn restoration is reconciled when the drama ends. Rendered campaign and revisit tests now inspect actual music/ambience component playback as well as requested track/cue identities. These checks do not constitute human listening or speaker-output verification.

## Validation

- Red: new `Memoria.Audio.RunReplacement` and `Memoria.ShopTransactions.RequestCancellation` both failed on the unmodified runtime. The latter probes all 13 request positions across open/sell/buy/close. Unreal exited 0 while the report contained two failed tests; the report is the authoritative result. Local evidence: `Unreal/Memoria/Saved/Validation/s295-red/index.json`.
- The first test-only build failed because the new test treated `RestoreSave` (bool) like `BeginStartingMemoryRun` (enum). The test was corrected, and `s295-red-build02` compiled successfully. Both build records are retained locally.
- Sound generator and validation host tests: `python -m unittest test_audio_sources test_validation_tools`, 17/17 passed before runtime changes.
- Green: `validate_unreal.py --build-and-test --rendered --test-prefix Memoria.Audio.+Memoria.Campaign.+MemoriaVisual.+Memoria.BattleEntry.+Memoria.ShopTransactions. --automation-timeout 2400 --evidence-dir Unreal/Memoria/Saved/Validation/s295-rendered` passed **72/72**, exact expected identities, no validation errors, engine exit 0. Includes both formerly failing regressions. Build passed; audio warnings/errors 0; fatal diagnostics 0. Audio devices initialized and the new component playback assertions passed.
- Post-change host sound/validator tests: **17/17** passed (`Unreal/Memoria/Saved/Validation/s295-host.log`). Generated audio source check passed for all 25 cues. `git diff --check` passed.
- Full Unreal registry is now 290; the full 290 was not rerun and the old Claude 288/288 result is not presented as current validation. The selected 72 cover Audio3 + Campaign4 + Visual3 + BattleEntry47 + ShopTransactions15.
- Final machine-readable validation: `Unreal/Memoria/Saved/Validation/s295-rendered/unreal_validation.json`; exact test report: `Unreal/Memoria/Saved/Validation/unreal-run-20260926T001032044988/Automation-20260926T001258325129/index.json`. Evidence remains ignored and local-only.

## Existing host-suite debt

A broader `python -m unittest discover -s Unreal/Tools -p 'test_*.py'` run before the runtime/validator patch executed 119 tests: 104 passed, 10 failed, five errored. This is not a green full host suite. Four errors require historical evidence removed from Git (phase1k/phase1l/phase1m automation indexes and depth1 raw_execution.zip). Other failures inspect outdated registry counts or pre-refactor item mutation strings. The affected files are `test_checkpoint_tools`, `test_malet_antidote_tools`, `test_malet_deal_tools`, `test_malet_firebomb_tools`, `test_malet_potion_tools`, `test_malet_refusal_tools`, `test_malet_reward_tools`, `test_malet_tools`, `test_malet_world_seed_tools`, `test_narrative_tools`, `test_shop_transaction_tools`, and `test_slice_tools`. They were left unchanged; no skips or weaker assertions were introduced to make this review green. Their maintenance is a separate task.

## Remaining work

`BATTLE_CORE_SPEC.md` is accepted as a proposed next-task specification, not implemented combat or approved changes to canon. Its chapter-3 revisit premise and attack/last-stand source locations were spot-checked. Begin any implementation with executable Godot oracle coverage; do not treat the proposed feel targets as measured balance. Attacks, combat burns, enemy turns, win/loss/rewards and witness remain unimplemented. No packaged audio build or human listening pass was performed. The pre-existing depth-material deprecation warnings remain. The GDD could not be hydrated from OneDrive (OS error 389); the local source, migration reports and handoff were used for this bounded review.
