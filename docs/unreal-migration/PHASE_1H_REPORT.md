# Phase 1H — Malet deal payment and reward-request boundary

Status: **COMPLETE** — all bounded Phase1H acceptance passed.
Worktree: `C:\Users\jc\MemoriaMigration\foundation`.
Branch: `unreal-migration/ue58-foundation`.
Engine: **UE 5.8.2 / CL56702186**, `C:\Program Files\Epic Games\UE_5.8`.
Previous accepted technical checkpoint: `361b9039713446957f1da66470881990388762ba`.
Clean starting documentation HEAD: `5ea698c41687c54e14f9c4df382a15e111d6adcc`.
New technical checkpoint: `5010a26ca934047d27160af3672a93a0caaf2e14`. No push.

## Authorized scope

The paid VN, native Verdan travel, physical approach, first three-row Malet
reaction and ordinary second E interaction reuse the accepted runtime.
Original Accept0 now reaches the unchanged Field interpreter: actual flag and
actual sword payment in the same live run/domain, normal completion, source0.3s
continuation, typed `malet_deal` originals0..4, completion, separate source0.5s
continuation, then `malet_reward` request and **pre-execution defer**.

Only `malet_deal` is newly imported. No authored text is copied into C++ or
Blueprint. No runtime importer, second memory authority, simulated payment
Boolean, rollback, catalog mutation or canonical run replacement was added.

## Source, typed content and strict import

Source: `data/chapter2_dialogue.json`, `dialogues.malet_deal`, group position2,
five rows0..4. Relevant source revision:
`4f772fafb1c7c6da68f56f44d60a7c7fed7f3949`.

| Identity | SHA-256 |
|---|---|
| Raw source file | `76b75afef0d2cbcd9747e83884defee806046dce6947207f885aa4bf6913c691` |
| Source UTF-8/LF | `67ec5b7c5f455f41ba997fd6c84220f85f8f87f7f111ef9b3e28c8ead935ac21` |
| Exact IR bytes | `abc4d21e35bca399af3850c67ea54f2477f2936ca6007004629ca33a45d1b779` |
| Typed semantic fingerprint | `d6362c3c5f5c5e1ccc65c26a0f4d5008ec7d1e58730f42d2126598b4956b2d7f` |

IR: [malet_deal.field.v1.json](ir/narrative/malet_deal.field.v1.json).
Typed asset:
`/Game/Memoria/Generated/Narrative/DA_Field_MaletDeal.DA_Field_MaletDeal`.

English/Korean text, speaker, presentation metadata, original order/indices,
presence bits and provenance are retained. Rows0,1,3 carry original CG paths;
row4 carries `arrel_exhausted`. Metadata preservation does not certify final CG,
portrait presentation or Korean typography. Deal has no row effects or choices.
Both original normal choices remain visible with IDs0/1 and unchanged labels.

The existing source -> reviewed IR -> strict typed importer is extended by one
cohort. First import, second fresh-process import and check-only reload are
recorded in [pipeline](evidence/phase1h/import02/pipeline.json). Transient probes
check semantic text changes, source promotion rejection, bad position/count/index
and downstream group rejection. Existing packages are checked independently.

## Executable oracle and exact effect/callback order

[Source attestation](evidence/phase1h/oracle05/source_attestation.json) includes
NPC, perception, DialogueManager, MemoryManager, GameManager, map/script/scene
hashes. Remove the tagged observer lines to recover the exact source callback
bodies, including actual `create_timer(0.3)` and `create_timer(0.5)` awaits.
Inert presentation endpoints isolate execution; source production files are unchanged.

At normal row9, both original choices are visible. Actual Accept order is:

```text
select:field:0
choice:Accept the deal.
flag:malet_deal_accepted
burn:identity_first_sword:ok
visit:10
end
state:exploration
callback:normal:disconnect
delay:scheduled:300
callback:npc:first_talk
flag:talked_Malet_malet_encounter
delay:elapsed:300
callback:deal:connect
request:res://data/chapter2_dialogue.json::malet_deal
field:start:malet_deal
[exact authored visits/lines 0,1,2,3,4]
visit:5
end
state:exploration
callback:deal:enter
delay:scheduled:500
delay:elapsed:500
callback:reward:connect
request:res://data/chapter2_dialogue.json::malet_reward
development:deferred:malet_reward
```

The accepted flag is set immediately before the burn attempt by the actual
Field interpreter. Normal completion switches to exploration before emitting
the completion sequence. The map listener schedules0.3s before the NPC first-talk
listener sets its flag. Accepted is read again when that callback resumes.
Deal completion returns to exploration and schedules an independent0.5s timer.

The source reward-completion connection is observed as an event. Unreal does
not bind an executable reward-completion handler, load a reward asset, start its
Field rows or perform any reward effect. The final request/defer is the stop point.

## Memory and failure/lifetime characterization

[Oracle inputs](fixtures/malet_deal/contract_inputs.v1.json) and
[expected snapshots/events](fixtures/malet_deal/contract_expected.v1.json)
contain nine executed cases; deterministic checks agree.

| Case | Executed source result |
|---|---|
| Intact sword, English | Accepted true; one successful sword burn; deal -> reward request |
| Intact sword, Korean | Same effects and original IDs; exact Korean lines/choice log |
| Already burned sword | Burn returns failure; accepted stays true; no duplicate history; deal -> reward request |
| Unrelated state | HP73, Grains41, unrelated flag and forest-smell erosion2 preserved |
| Faded sword | Burn fails; history remains food only; accepted stays true; deal -> reward request |
| Replace state while0.3s, keep owner | Source accepted flag now absent; callback still requests refused |
| Replace state while0.5s, keep owner | Source callback still requests reward |
| Teardown owner while0.3s | No deal/refused/reward request after owner destruction |
| Teardown owner while0.5s | No reward request after owner destruction |

The last four are explicit isolated state-replacement/owner-lifetime probes,
not a production New Game implementation. **Source state replacement alone has
no generation cancellation guard.** Unreal deliberately retains the previously
accepted Phase1G run-replacement cancellation guarantee: its `OnRunReplaced`
reset cancels both world-owned timers. Actual world teardown also cancels them.
This is an explicit technical ownership difference, not a claim that the source
cancels a callback merely because flags/memories were reset.

Canonical before Accept: food burned, sword intact, history
`[daily_market_food]`. After Accept and through reward defer:
`[daily_market_food, identity_first_sword]`, sword occurs exactly once,
burned=true, residue=true, faded=false, erosion0. Source residue with Elia is
preserved; it is not converted into total disappearance. Campfire erosion4
from the earlier food payment remains4; all other owned state is source-identical.
Run ID, domain object, HP100, Grains0, items, prior flags, food burn and heard flag
are retained. Only accepted and first-talk flags are added to the pre-normal run.

## Tests and compatibility

New eight identities:

- `Memoria.MaletDeal.ImportContract`
- `Memoria.MaletDeal.PaymentEdgeCases`
- `Memoria.MaletDeal.CanonicalPayment`
- `Memoria.MaletDeal.AlreadyBurnedPayment`
- `Memoria.MaletDeal.CancelNormalOnRunReplace`
- `Memoria.MaletDeal.CancelRewardOnRunReplace`
- `Memoria.MaletDeal.CancelNormalOnWorldTeardown`
- `Memoria.MaletDeal.CancelRewardOnWorldTeardown`

All existing82 identities remain. The historical
`Memoria.MaletRefusal.AcceptPreEffectDeferred` name is retained for identity
compatibility; its assertion now checks the newly authorized Accept chain and
pre-effect **reward** boundary. It no longer claims Accept itself is deferred.
There is no test-only runtime authorization switch.

The canonical replay uses actual rendered PIE and Enhanced Input, paid original
VN1, native travel and key-driven physical movement to Malet. Original Accept0
is confirmed through input, not a direct interpreter call. No pawn teleport
or canonical fixture reinitialization is used. Separate supplemental tests use
explicit already-burned, faded/unrelated, new-run and actual OpenLevel-teardown
fixtures. Unit checks supplement rather than replace the canonical route.
The existing Refuse1 ->0.3s -> refused0..2 -> cleanup -> ordinary retry trace
and Phase1F reaction priority/movement checks remain.

## Validation and rendered evidence

| Verification | Final evidence |
|---|---|
| UE5.8.2 UBT/UHT/compile/link | [build01](evidence/phase1h/build01/unreal_validation.json); final build up to date |
| Editor / actual rendered PIE / Automation | **90/90**, previous82+new8; [index](evidence/phase1h/automation01/automation_index.json) |
| Independent source-to-rendered validation | **80 checks**, all65 capture JSON files parse; [acceptance02](evidence/phase1h/acceptance02/acceptance.json) |
| Canonical world timers | **300000us / 500000us**, separate callbacks; [final snapshot](evidence/phase1h/automation01/Phase1H/CanonicalPayment_reward_boundary.json) |
| Source exact trace | [Accept/deal/reward-request trace](evidence/phase1h/acceptance02/accept_exact_trace.txt) |
| Deal strict pipeline | [import02](evidence/phase1h/import02/pipeline.json): CREATED then UNCHANGED/no-save twice; identical package SHA |
| Phase1H executable oracle | **9/9**, original goldens unchanged; oracle05 and import02/oracle PASS |
| Source regressions G/F/E/D/C | refusal7, reaction7, route4, narrative10, seven-entry catalog PASS; [suite](evidence/phase1h/regressions/oracle_suite.json) |
| Official Godot baseline | repo/VN/Korean/editor import plus **15/15** memory/world suite PASS; [baseline](evidence/phase1h/godot01/godot_baseline.json) |
| Native production memory / CTest | **51/51 +1/1** PASS; [native](evidence/phase1h/native01/native_memory_validation.json) |
| Host tooling | **55/55** PASS after final observer changes; [host03](evidence/phase1h/host03/host.json) |
| Static / protected original | **59/59**, **4217/4217** files unchanged; [static02](evidence/phase1h/static02/foundation_static.json) |
| Accepted UE packages / historical source | **19/19** unchanged; exactly one new deal package; [protection](evidence/phase1h/protection.json) |

The only Automation warning remains the previously accepted
`r.MotionVectorSimulation` warning in `Memoria.Campaign.CanonicalPaidRoute`.
All new8 tests and retained Malet/refusal tests have zero warnings/errors.
All90 completed successfully; process exit0 and fatal-log checks passed.
The existing unmodified Godot legacy-grade diagnostic remains in baseline logs.

The six required native1286x760 PNGs were inspected directly, including window
chrome around the PIE view. No image modification was used.
[Visual inspection](evidence/phase1h/visual_review.json) records hashes and scope.

| Capture | Link |
|---|---|
| Both original choices | [NormalEncounter_Choices](evidence/phase1h/automation01/Phase1H/CanonicalPayment_NormalEncounter_Choices.png) |
| Accept highlight | [Accept_Selected](evidence/phase1h/automation01/Phase1H/CanonicalPayment_Accept_Selected.png) |
| Deal first original0 | [Deal_FirstLine](evidence/phase1h/automation01/Phase1H/CanonicalPayment_Deal_FirstLine.png) |
| Deal middle original2 | [Deal_MiddleLine](evidence/phase1h/automation01/Phase1H/CanonicalPayment_Deal_MiddleLine.png) |
| Deal last original4 | [Deal_LastLine](evidence/phase1h/automation01/Phase1H/CanonicalPayment_Deal_LastLine.png) |
| Reward request deferred | [RewardRequest_Deferred](evidence/phase1h/automation01/Phase1H/CanonicalPayment_RewardRequest_Deferred.png) |

All full logs, including failed attempts, are preserved byte-for-byte in
[full_logs.zip](evidence/phase1h/full_logs.zip) with the [SHA inventory](evidence/phase1h/full_logs_inventory.json).


Both timer measurements in machine-readable evidence are integer microseconds.
UE world-clock timer thresholds:0.299 <= normal <0.35 seconds;0.499 <= reward <0.55.
The accepted PIE harness uses a1/60-second simulation step; UE telemetry measures
actual world-clock callback elapsed time, not host wall-clock runtime.
Source harness waits preserve actual awaits. It records both wall-clock elapsed
microseconds and the starting frame delta, requiring their sum to be at least
nominal minus1000us (299000/499000). Godot processes frame signals/nodes before
subtracting the frame delta from SceneTreeTimer, including a timer created in
that frame; see the [4.6.2 engine source](https://github.com/godotengine/godot/blob/4.6.2-stable/scene/main/scene_tree.cpp#L630-L755)
and [measurement basis](evidence/phase1h/timer_measurement_basis.json).
Wall time is reported separately and is never relabeled as simulation time. Deterministic event/state output is separate
from nondeterministic wall-clock telemetry.

Required captures: `NormalEncounter_Choices`, `Accept_Selected`,
`Deal_FirstLine`, `Deal_MiddleLine`, `Deal_LastLine`,
`RewardRequest_Deferred`. Corresponding JSON retains full run/memory, trace,
same-run/domain evidence, invocation counts and both timer measurements.

## Defects and preserved attempts

- `host01`: new observer-equivalence test joined with literal backslash-n.
  Corrected the assertion; `host02` passes55/55. Production/source behavior
  was unaffected. Failure output remains preserved.
- `build01` initially waited for an active STELLA DRIFT Build.bat engine lock.
  The other project was left running; independent regressions proceeded.

- `import01`: the source timer observer measured273261us on a loaded frame,
  failed its fixed280000us lower bound and subsequently timed out; no package
  was written. Exact production awaits remain unchanged. The final observer
  records the current frame delta and uses the documented frame-aware bound.
- `oracle03`: observer generation emitted a literal backslash-n in Python;
  preserved pre-engine launch failure, corrected escaping.
- `oracle04`: inserted frame-delta line had incorrect indentation inside the
  normal callback, causing GDScript parse failure and timeout. The insertion
  now retains each source line's indent. `oracle05` passes9/9 and byte-compares
  the original goldens; stripped observers still recover exact callback bodies.

- `acceptance01`: all gameplay/snapshot checks passed, but the added PNG check
  incorrectly required1280x720 exact file dimensions. The existing full-window
  screenshot is1286x760. The checker now validates native readable dimensions
  and consistency, records actual dimensions and leaves every image untouched.
  `acceptance02` and direct visual review pass. No UE/runtime change or rerun
  was required for this evidence-checker correction.

## Remaining limits and exact Phase 1I recommendation

This is bounded payment/deal/reward-request acceptance, not full Malet or Verdan
parity. Reward8, world-memory seeding, item grants, shop/trade, Chapter3,
autosave, achievements, repeat-world group, extra NPCs, battle, production
New Game/Chapter1, final art/UI/audio, graphics modernization, cook/package
and Steam remain outside implementation. Field/timer save-resume and final
Korean typography/physical-device coverage are not certified.

Recommend **Phase1I only after separate authorization**: characterize and import
only `malet_reward` originals0..7 through the existing typed Field pipeline,
continue this same paid run and its0.5s request into those eight rows, then
observe completion and stop **before the first effect of `_on_reward_ended`**.
Keep `ch2_malet_done`, world-memory seeding, potion2/antidote1/firebomb1,
shop and its Chapter3 continuation deferred. Preserve all90 identities and
payment/refusal/reaction/lifetime contracts. No Phase1I code is implemented here.

## Reproduction

Use fresh evidence directories from the migration worktree.

```powershell
python Unreal/Tools/export_malet_deal_oracle.py --godot <Godot-4.6.2-console.exe> --check --evidence-dir <fresh>/oracle
python Unreal/Tools/malet_deal_test_fixtures.py --check
python Unreal/Tools/import_narrative.py --group malet_deal --engine-root 'C:\Program Files\Epic Games\UE_5.8' --godot <Godot-4.6.2-console.exe> --evidence-dir <fresh>/import
python Unreal/Tools/validate_unreal.py --engine-root 'C:\Program Files\Epic Games\UE_5.8' --build-and-test --rendered --evidence-dir <fresh>/automation
```
