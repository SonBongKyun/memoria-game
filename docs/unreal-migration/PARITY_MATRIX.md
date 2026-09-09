# Observable gameplay parity

All UE tests below are **planned / not run**. Source evidence is implementation inspection unless the baseline column explicitly cites a test run. “Baseline suite” means the 15-case Godot suite in [AUDIT](AUDIT.md); it does not cover all variations in this table.

Use immutable fixtures, recorded input/choice/random-draw traces, normalized state snapshots and ordered event traces. Exact equality applies to IDs, flags, ranks, integer HP/damage/currency, choices, event order/count and route destinations. Timing/position comparison initially uses the same fixed simulation step; calibrate a documented visual/movement tolerance from reference captures before accepting differences. A tolerance cannot waive a missed collision, hidden choice, changed consequence or unreadable text.

| ID | System / observable setup and action | Required observation | Baseline / future automation |
| --- | --- | --- | --- |
| P01 | Fresh run, load profile, enter title, start New Game twice | Initial state/flags/party/memories reset consistently; profile unlocks retained by source policy; one VN host, no duplicate listeners | Static; UE lifecycle functional + reset snapshots |
| P02 | Play cold open through Ch1, choose each food/song/sword branch | Exact text/choices/burns/flags/CG order; looping forest exits correctly; arrive at Verdan through Ch2 VN; no runtime battle inserted into Ch1 | VN validation; paired route trace |
| P03 | Walk straight/diagonal/half-stick, reverse, stop, sprint, collide at corners | Same path and stopping envelope, analog magnitude, facing and animation; no diagonal boost or wall penetration; foot sorting/camera framing match | Static; input replay + trajectory/capture |
| P04 | Approach NPC from each side; interact, hold confirm, leave range, return | Nearest eligible interaction, one dialogue advance, persistent talked/repeat/reaction behavior; follower distance/warp not blocking doors | Static; functional map/input |
| P05 | Open archive/pause/options/backlog/CG and press shared confirm/back keys | Only top eligible layer consumes command, focus returns, field is paused where source pauses; no same-event clickthrough or stuck pause | Static; keyboard/mouse/controller matrix |
| P06 | Device switch and replug during exploration/battle/VN; all mapped/raw keys | Mappings match source inventory; glyph discrepancies and unsupported actions reported; rumble presets and deadzone switch behave predictably | Partial controller source; functional hardware review |
| P07 | Field dialogue with missing/burned/faded memory, false flags, nested world conditions | Exact line visibility/burned substitution and choice indices; effects in source order; failed-cost outcome matches its dialect | Malet/live baseline subset; pure interpreter fixtures |
| P08 | VN gated effect step, filtered choices, goto 0/end, nested continuation | Preserve before-gate effects, original choice index, zero-based jumps/FIFO queue, cost handling and legacy no-op diagnostics | VN validation; interpreter golden traces |
| P09 | Acquire, normal burn, silent burn, sell and extract same-rank memories in separate fixtures | Exact burned/residue/extracted state and ordered cascade/burn/carry/passive events; reject disallowed commands without accidental rewards | Static; memory command fixtures |
| P10 | Repeat P09 for raw grades 0..4, Elia absent/present, faded/collateral | Preserve raw grades, residue eligibility, power and guardrails; Zero/core restrictions not inferred from display names | Repo contract; parameterized rules |
| P11 | Same-NPC/prefix graph; change source ordering only in negative fixture | Reference connection/cascade targets match; importer detects unintended order drift; guard consumed once, no recursive burn | Static; graph/cascade snapshots |
| P12 | Chapter erosion at capacity/overcapacity; protected/high-grade/Elia/oath memory | Exact integer erosion, fade threshold and narrative events; core immunity and soft carry pressure; no cumulative-loss gauge | Static; boundary math + UI capture |
| P13 | Loan issuance, early repayment, due chapter, chapter after due, extraction | Correct principal, repayment, collateral exclusion, maturity boundary, tombstone/extraction history and cascade | Static; rules/save roundtrip |
| P14 | Guard purchase/use, synthesis, vigil and passive unlock progression | Exact currency/rank/power/ID/input removal and unlocks; guards do not duplicate; API/UI discrepancy is characterized before alteration | Static; domain plus UI commands |
| P15 | Add/remove/restore actor memory; learn/forget/learn false fact; repeat no-op | World snapshot revision/sequence and event fields match; no player burn; tombstone/history retained; unknown actor rejected | World/catalog/event baseline; UE Automation |
| P16 | Malet request then identity removal/restoration before/after Ch5 report | Request and identity remain distinct; report freezes at entry; restoration after report changes present cognition, not historic Kairos report | Malet + canon Wave2A baseline; paired story fixture |
| P17 | Sable memory cases and field condition consumers | Exact consequences/choices with active/removed/missing evidence; catalog remains source of actor validity | Sable baseline; UE domain + field functional |
| P18 | Burn/fade memory in field, talk again, revisit map | World rewrite flags, tint/hiding/collision, authored first reaction/revisit once flags, compass and absence afterglow match; duplicate listeners do not multiply rewards | Static; functional visibility/collision/captures |
| P19 | Open archive filters/detail, constellation and compass before/after loss | Status, connection lines, degree/rank labels and density change match; constellation does not mutate memory; compass does not invent route finding | Static; view-model tests + visual review |
| P20 | Flow travel → sprint/pulse/dash → warning encounter/visible threat | Preserve flow gain/cost 42/duration .22/cooldown .54, focus rewards, threat entry/bypass and oath effects; pressure and proximity cues visible under clean mode policy | Static; deterministic field replay |
| P21 | Random encounter below/above threshold, phase suppression, escape and re-entry | Correct map/chapter gate, pool, distance budget and escapability; Verdan only after Ch2 complete; transient counter reset matches | Static; controlled random/distance functional |
| P22 | Battle start with field entry/focus, difficulty, NG+ and chapter growth | Exact initial HP/ATK scaling, status resets, modifiers, momentum/limit and ordered start notification; no duplicate boss autosave | Static; battle-start snapshots |
| P23 | Attack/guard/stance across weak/resist/void/environment/combo/crit cases | Integer damage/crit/limit/break values match at each stage; ordinary void damage is reduced (not universally immune) | Static; pure combat trace |
| P24 | Burn each skill, residue reuse, chain burn, limit break | Costs, animation cue order, shield/DOT/echo/passives, power and turn consumption match; residue reusable without second burn event | Static; rules + battle visual functional |
| P25 | Player status, enemy DOT, stun/charge/reflect, boss phase, Elia/Sable/Tobias turns | Exact turn order/cooldowns/skip/ability decisions; ally command and probabilities use recorded draw stream; no stale callbacks after victory | Static; battle scenarios/teardown |
| P26 | Witness normal/void/boss with/without scan/hum; lethal alternative | Required progress, release vs boss break, preservation bonus/flags/statistics match; no fabricated zero-kill semantics | Static; command traces + text/reward view |
| P27 | Directives, momentum/break, field focus and successive encounter chain | Tactical condition/reward grade and carried transient descriptor consumed once; UI forecasts reflect actual result | Static; chain fixture |
| P28 | Victory, flee, defeat, reward-dismiss timeout, retry/title during effects | `battle_ended → rewards-ready → cleanup` observables preserved; heal/drop/grains once; return/retry map correct; no softlock | Static; asynchronous functional |
| P29 | Toggle auto battle; low/high enemy HP with valuable memories | Same selected actions, including possible high-grade burns; no unrequested “safe auto” redesign | Static; deterministic auto decision |
| P30 | Boss rush start/advance/fail/complete; NG+/NG++ reset | Boss sequence waits for cleanup, records/unlocks/scaling/retained items match; unrelated old transient state does not silently enter new run | Static; run lifecycle/profile fixture |
| P31 | Shop buy/sell/items/equipment/upgrades/quickslots; insufficient funds | Exact prices/counts/stats/sale-burn/oath effect; characterize transient stock and failed-command cases; no duplicated purchase reward | Static; economy commands + modal functional |
| P32 | Swear Ash/Witness/Still, sell, bypass, voluntary Elia burn, forced VN burn | Each oath can be sworn per current code; exact break/reward/chapter-once state; forced step burn excludes Still violation | Static/S263 source; boundary fixtures |
| P33 | Quest, resonance, cache, curio, gateway, optional site; revisit/load | Eligibility, texts, enemy/reward tables, once flags, map return and authored art match; no broadened current-canon gates | Static; per-map manifests + fixtures |
| P34 | Puzzle 3–8 pairs: match/mismatch, attempt count, close/reopen | Same card pool/fillers, delay and grains formula; no claim that a board is saved; reward paid once | Static; seeded puzzle + UI functional |
| P35 | Diary skills/cooldowns, tutorials first events, journal/atlas/minimap | Entries, read/shown state, POIs and quests match; derived journal needs no invented independent save; show-once survives source save policy | Static; view models/save functional |
| P36 | Enemy encounter/scan/defeat and memory acquisition across runs | Codex keys/assets/counts, achievement unlocks/stats and profile separation match; no real Steam success claimed | Static; profile fixture |
| P37 | Save/load all five existing fixtures plus malformed current version | Normalized world/player state matches; source JSON unchanged; raw ordinals stable; bad destination rejected before mutation | Save migration + guarded suite baseline; adapter Automation |
| P38 | Save active/pending VN, both branches around effect steps, invalid ID | Exact resume route/cursor and effect outcome; no silent duplication policy; field/player position restored only after destination ready | Static; paired save/reload trace |
| P39 | Autosave timer/map checkpoint/chapter/boss, corrupt main+valid backup | Slot 0 vs manual slots preserved, correct retry point, backup recovered, error useful; all tests confined to injected sandbox roots | Save guards baseline subset; I/O fault injection |
| P40 | Save A/load B/new run/NG+ with different diary/loan/oath/profile states | Explicit state replacement/merge policy matches reference or separately approved defect decision; no accidental cross-slot migration leakage | Static; lifecycle characterization |
| P41 | Trigger every ending and tied predicates via controlled fixtures | Seven Part3 resolver IDs and precedence; legacy BL07/epilogue paths checked separately; gallery unlock/credits and saved endings match | Static; ending truth table + route functional |
| P42 | Current field route Ch2→Ch5; legacy travel saves; preview Ch11→24 | Each cohort retains its gates and destination; Ch6 pending boundary and Ch18 demo gate preserved; no silent route splice | Canon baseline subset; graph + playthrough |
| P43 | CG/portrait change, blocking CG, typewriter/auto/fast-forward/new line | Exact layer/portrait/emotion/tag behavior; new unread text stops skipping, backlog/read profile behavior matches both locales | VN validation; timed narrative + capture |
| P44 | Both music players, dialogue duck, battle layers, low HP, burn silence, options | Track/event order, crossfade/gains/muting and synthesized SFX match; reproduce source mix anomalies before deliberate fix | Static; audio envelope/event capture + listening |
| P45 | Paired title/field/battle/VN/archive at 1280×720 and scaled windows | Composition, textures, Korean serif/sans weights, outlines/alpha and readable choices match; clean/low-motion cues retained; no 3D redesign | Static; approved screenshot baselines pending |
| P46 | Cook/package and run without source tree or editor | All referenced story/maps/art/audio/fonts load, no dynamic-path omissions; no editor/Dialogic/Godot dependencies; logs clean and exit successful | Godot actor-pack export baseline only; UE packaged smoke |
| P47 | Reimport same source twice, reorder input files, introduce bad ID/opcode | Stable semantic hashes/packages/order; no duplicate binaries; explicit errors for missing data; original sources unchanged | Phase0 inventory only; importer automation |
| P48 | Long repeated travel/battle/menu/load loop and clean quit | No listener multiplication, leaked actors/audio/UI, stuck async job or fatal diagnostics; profile writes isolated in test mode | Crash guards baseline subset; soak + fatal scan |

## Release parity gate

Accept one bounded slice at a time. Publish its source revision, fixture/input trace, expected and actual normalized snapshots/events, test process exit codes/full-log fatal scan, and paired screenshots/audio notes. All unresolved differences need an ID in the risk register and a disposition; “looks close” cannot waive memory, story, economy or save differences. Preserve a runnable Godot reference until all retained routes and secondary systems have accepted UE results.

No deletion of old content is implied by an unreachable current route. Route availability, implementation completeness, and tests passed are three separate properties. The importer and final checklist must account for all three.


## Phase 1K scoped evidence appendix — 2026-09-09

The entire Phase0 matrix above remains byte-identical as a prefix. Its broad planned
rows are not retroactively marked complete. This appendix records only the bounded
Malet seed implementation; final125/125 and independent437 checks PASS (static60/original4217 preserved). See [Phase1K report](PHASE_1K_REPORT.md).

| Original row / bounded observation | Phase1K contract and evidence |
|---|---|
| P15/P16 Malet knowledge and source memory | Typed separate run-owned world cognition; source13 exact snapshots/events/revisions. Fresh route fact then memory0 ->1 ->2; no player-card mutation. General remove/restore/forget commands and full report/followup are not migrated. |
| P15 no-op/tombstone/forgotten | Presence guard preserves removed/restored content/history and false fact; repeat/missing actor/false flag quiet no-op. Source13 and new native contract tests. |
| P37 bounded native SaveGame | Existing schema1 world JSON section; real binary SaveGameToMemory/load/restore compares run/player/world independently. Prior absent world defaults and malformed-native rejection tested. Full legacy import is deferred. |
| P40/P48 lifetime | Same Run/player/world owner across real native travel; replacement creates clean world owner and cancels old narrative. World actor teardown retains persistent cognition. Source state-only replacement behavior differs as previously documented. |
| P04/P07/P08 existing canonical path | Real paid VN/food/native travel/physical walk/Malet reaction/Accept/sword/deal/reward through done flag and seed. All107 prior Automation identities retained;18 added. |
| P31 first inventory boundary | Deferred BEFORE potion2, inventory/recent items unchanged; zero item/signal/toast/shop/chapter/profile calls. No inventory feature expansion. |
| P45 development evidence only | Four native PNGs; three explicitly recorded read-only synchronous snapshots and final live pre-potion stop. Final art/typography parity is not claimed. |
| P47 no content change | Existing21 package hashes and80 prior IR/fixture files unchanged. No narrative group, asset, package or dialogue import added. |

Source-derived behavioral expectations remain authoritative. General World Memory,
Malet trade, Verdan progression and production New Game remain outside this appendix.
