# Phase 1M — Malet antidote grant and pre-firebomb stop

Status: **COMPLETE**. All bounded acceptance gates passed. Local commit only; no push.
Worktree `C:\Users\jc\MemoriaMigration\foundation`; branch `unreal-migration/ue58-foundation`.
Starting HEAD `e8bebd2ef1b6ef08dde51d3f007545428614c720`, clean before work.
Actual installed UE5.8.2 / CL56702186, `C:\Program Files\Epic Games\UE_5.8`.
No push. Phase1N not implemented. Final checkpoint will be the enclosing local
`feat(unreal): grant Malet antidote reward` commit; its exact full SHA is resolved by
`git log -1 --format=%H -- docs/unreal-migration/PHASE_1M_REPORT.md`.

## Scope and source authority

The same real PIE paid VN1 / food burn / native travel / physical walk and E /
reaction0..2 / encounter Accept0 / sword burn / real0.3s / deal0..4 / real0.5s /
reward0..7 / done / world seed / potion2 path continues synchronously into antidote1.
No canonical teleport, manually assigned inventory, replacement fixture or rebuilt run.
Supplemental tests explicitly use independent fixtures, including inventory potion9 +
antidote7 and nondefault World revision19/event_sequence23; grant preserves those values.

`GameManager.add_item("antidote",1)` is the only newly authorized effect. Shared private
`GrantRewardItem` performs the existing mutation/recent/signal/toast implementation;
`AddRewardPotion(const FString&,int64)` remains strict potion-only, and the new
`AddRewardAntidote` is strict antidote-only. The16-ID source membership filter is separate
from `RewardItemScope` (InvalidSourceId / DeferredByPhase / Supported). No other grant,
framework, dynamic importer, shadow inventory, reward-once flag or schema bump.

Fresh exact-method source execution and hashes: [source01](evidence/phase1m/source01/oracle.json),
[source attestation](evidence/phase1m/source01/source_attestation.json).
The source revision is `84684aefa5721f97ee2318d2f8fa64d92dd077bc`; raw and LF hashes are recorded there. The actual ITEMS record is:

```json
{"name":"Antidote","desc":"Cures poison and burn, then restores 12 HP.","type":"cure","power":0,"recovery":12,"price":10,"icon":"res://assets/ui/items/antidote.png"}
```

No cure/use effect is executed. Both en and ko resolve the request to `+1 Antidote`,
ToastType.SUCCESS=1. Exact source signal carries only item_id, never quantity. Mutation
and recent commit precede that signal; toast follows it. Supplemental source cases
execute actual add/recent/export/import/localization and actual toast `_queue.append`;
read-only hooks observe the queue payload after append. Only visual display/profile
sinks are inert; they do not manufacture gameplay results. The absent visual sink has
zero deliveries while the actual enqueue and inventory commit still occur.

## Canonical contract and presentation

Before antidote: items={potion:2}, raw recent=[potion], done=true, world revision2/sequence2,
burned history=[daily_market_food,identity_first_sword]. After: items={potion:2,antidote:1},
raw recent=[antidote,potion]. Same Run ID and same Player/World objects. Full World,
Player snapshot+definitions/connections/effective powers/carry observations, HP, Grains,
chapter, locale and other flags stay identical. No firebomb entry exists.

```text
item:add:end:potion:2
item:add:begin:antidote:1
inventory:antidote:0->1
recent_items:antidote,potion
inventory_changed:antidote
toast:+1 Antidote:1
item:add:end:antidote:1
development:deferred:before:item:firebomb:1
STOP
```

Full trace retains all earlier source events. Exactly one signal and one toast per
item, totals2 and2. Toast requests remain ordered [`+2 Potion`:1, `+1 Antidote`:1].
Development presentation keeps both requests; a second request cannot overwrite the
first. It displays both lines together. The source visual animation queue is not
implemented in UE; this is ordered request delivery and a minimal history display.
Recorded synchronous snapshots say RECORDED / READ ONLY. Final stop reads live Run.
No capture timer, wait, frame gap or asynchronous grant cancellation point is invented.
E/Enter/Space/Back and held input are tested after stop without another effect.
The Antidote_Signal PNG replays the post-broadcast, pre-toast development snapshot;
`AntidoteCanonical_actual_signal_antidote.json` separately records the real signal
listener seam. Neither is described as an asynchronous cancellation interval.

## Recent, repeats, ownership and binary save

`GetRecentItems` filters valid source IDs, keeps first unique entries, caps at5 without
mutating raw stored recent. Grant then moves its ID to front, trims and commits. Raw
malformed typed FString arrays survive source export/import and UE binary restore;
query results are compared separately. Arbitrary Godot Variant conversion and legacy
import are outside scope. Zero and negative counts remain permitted; native signed
addition retains defined wrapping without C++ signed-overflow UB.

Two explicit API calls accumulating twice are expected API behavior. The separate
source defect is an unintended repeated full reward callback, including duplicate
whole rewards/shop-signal error. Its prior oracle/evidence remain unchanged. No once
guard, duplicate suppression, rollback or source bug fix is introduced.

New PIE probes replace the run inside actual inventory_changed callbacks, separately
at potion and antidote. Old inventory/recent commits are captured before replacement;
old toast and subsequent grants cannot enter the new run. The original callback owner
is rechecked after return. Existing Phase1L development-observer cancellation probes
remain separate supplemental seams, not mislabeled actual signal listeners. Source
state replacement continues old toast and later calls; UE intentionally keeps its
established stale-owner cancellation. This difference is explicitly represented in
source/native comparison. Completed world teardown retains persistent committed state;
absent or removed presentation subscribers cannot undo inventory.

Actual SaveGameToMemory -> LoadGameFromMemory -> independent Run RestoreSave compares
Run/inventory/raw recent/flags, Player+derived, and World independently. Original
canonical authority stays unchanged. Restore observers count zero inventory signals,
zero toast requests and zero grant observations. Canonical run `D30A70C643231D76EDE2FA806BC31219` serialized to13,009 bytes. Schema1 stays; actual byte count is
recorded, never compared with the previous12,897-byte constant. Disk save, autosave and
Field cursor resume are not implemented.

## Assertion continuity and preservation

[Exact identity and boundary ledger](evidence/phase1m/assertion_continuity.json) maps
all147 prior identities. Original source tests and core assertions remain. The complete
potion-only Run delta, exact source prefix and full Player/World/derived/identity checks
now run explicitly at `potion_contract_complete`, the same logical source boundary
before antidote. The original PotionCanonical save assertions run there too. The final
development-stop assertions alone extend to antidote-complete/pre-firebomb; earlier
payment/refusal/reaction/timer/lifetime boundaries stay intact. Historical frontier
names are retained and documented as naming debt.

Protection covers all actual prior files:4217 original Godot files,21 accepted packages,
84 current prior IR/fixtures. The user-quoted82 is Phase1L's entering baseline; its two
potion fixture JSONs bring Phase1M's entering baseline to84. Prior reports/evidence and
oracle expected files are byte-preserved. New narrative IR/assets/packages=0. Current
M fixtures and evidence are new files in fresh paths only.

## Execution results

UE build-only and final UBT/UHT/compile/link/Editor/rendered PIE passed. Final
[automation01](evidence/phase1m/automation01/unreal_validation.json) contains169 exact
identities, all Success:147 retained+22 added. New tests are16 source cases plus
NativeScopeAndPresentation, Canonical, PotionSignalReplacement,
AntidoteSignalReplacement, ReplacementAtStop, WorldTeardownAtStop.
[Independent acceptance](evidence/phase1m/acceptance01/acceptance.json):1,049 checks PASS.
Fresh capture provenance contains1,656 files, filtered by this run start UTC and hashed.
The files are1,038 JSON snapshots and618 PNGs. Six native original PNGs directly inspected; [visual review](evidence/phase1m/visual_review.json)
records run ID, hashes and actual vs recorded presentation distinction.

The sole Automation warning is the existing `r.MotionVectorSimulation` render-thread
warning, now attached to `Memoria.Antidote.AntidoteSignalReplacement` (first rendered
case). This is not a new warning-free suite claim. No setting suppresses it; no failed
test or new compile warning was observed. Engine startup also logs its pre-suite
`LogAutomationTest: Error: Condition failed` self-test diagnostics, as in prior runs;
full raw logs are retained and are not represented as empty of errors. All169 actual
Memoria test records have zero errors. UE and M source acceptance passed on their first attempts. The retained I regression
failed its existing timer instrumentation: elapsed271745us+start_frame18056us=289801us
was below299000us, then the interrupted coroutine caused the90s runner timeout.
[Failed attempt](evidence/phase1m/regressions01/I/oracle.json) and full raw log remain.
This happened after UE ended; no overlap with compilation was scheduled. The cause
of the scheduling discrepancy is not claimed proven. The exact unchanged test and
expected fixture passed alone on the fresh retry (I8); neither timeout nor tolerance was relaxed.

Source M16/L16/K13/J12/I8/H9/G7/F7/E4/D10/C7 now all PASS;
[aggregate](evidence/phase1m/source_regressions.json) retains both the failed I attempt
and successful unchanged retry. Host90/90 PASS. Official Godot repo/VN/Korean/editor and memory/world15/15 PASS; Native Player
Memory51/51 and CTest1/1 PASS. Official editor's three existing ShaderV duplicate UID
warnings and ObjectDB exit warning are retained. Original profiles stay isolated;
tracked worktree import metadata was restored by the official runner.

| Gate | Fresh result |
|---|---|
| UE UBT/UHT/compile/link/Editor/rendered PIE |169/169 exact identities PASS|
| Independent source/state/order/lifetime evidence |1,049 checks PASS|
| Source oracle and regressions |M16/L16/K13/J12/I8/H9/G7/F7/E4/D10/C7 PASS; first I attempt retained|
| Native Player Memory / CTest |51/51 and1/1 PASS|
| Official Godot |repo/VN/Korean/editor and memory/world15/15 PASS|
| Host tools |90/90 PASS|
| Static / original / packages / prior data |60/60 checks,4217/4217 original,21/21 packages,84/84 prior IR/fixtures PASS|

Evidence: [UE](evidence/phase1m/automation01/unreal_validation.json),
[index](evidence/phase1m/automation01/automation_index.json),
[canonical full trace](evidence/phase1m/acceptance01/canonical_full_trace.txt),
[binary save](evidence/phase1m/acceptance01/save_roundtrip.json),
[Godot](evidence/phase1m/godot01/godot_baseline.json),
[native](evidence/phase1m/native01/native_memory_validation.json),
[host](evidence/phase1m/host01.json), [raw logs](evidence/phase1m/full_logs.zip),
[log hashes](evidence/phase1m/full_logs_inventory.json).

All final state is local. Starting original Godot HEAD remains
`15df72809baa52eec281d8508f495a373e9f5884`; its preexisting SESSION_LOG.md/project.godot
modifications and untracked migration docs are retained. No source reset, clean,
stash, publication or push. .gitignore adds only phase1m raw-log exclusion (logs are
archived); .gitattributes adds LF for the new antidote fixtures and -text only for Phase1M
evidence. Stage review found CRLF source harness files gm.gd/executed_add_item.gd
normalized by Git; the narrow evidence rule preserves their actually executed bytes
and hashes. All evidence index bytes are verified after restaging. Exact config backups
are retained under Saved/Validation/phase1m-config-backups.

## Limits and Phase1N recommendation

Firebomb add_item is never entered: no firebomb inventory/recent/signal/toast. Shop,
ch2_complete, chapter progression, autosave, achievement and next-map requests are
unexecuted. Whole snapshot, exact trace and static call-scope checks supplement zero
counters. This is a development stop, not the source game's final exploration flow.
Full Inventory/Malet/Verdan/Shop parity or Steam readiness is not claimed.

Recommend a separately authorized Phase1N limited to exact source
`GameManager.add_item("firebomb",1)` and its recent/signal/toast contract after this
completed antidote, with source-derived en/ko payload, same authority and preserved
169 identities, then STOP before the next source shop-opening operation. Do not start
shop orchestration. Phase1N is not implemented here. Final art, font/KO typography,
audio, toast animation, disk saves, physical-input certification and packaging remain
outside this phase.

Final [static](evidence/phase1m/static03/foundation_static.json) passed60 checks and
all4217 original hashes. [Protection](evidence/phase1m/content_protection.json) also
preserves21 packages,84 prior IR/fixtures and all6,567 historical evidence/report files
(6,672 protected worktree files total). No new narrative package or IR. Full log archive
contains53 current logs, including the failed I attempt and unchanged successful retry.
Final local status/diff checks and no-push policy are recorded in
[final checks](evidence/phase1m/final_checks.json). Post-commit SHA/clean verification
is reported to the user; an enclosing commit cannot embed its own SHA in its content.

Evidence staging verification additionally caught nine cached normalized blobs; a
Phase1M-only `git add --renormalize` restaged the actual raw bytes. Their native CRLF
then appeared as trailing whitespace to git diff --check. The same narrow evidence
attribute now recognizes CR-at-EOL while retaining blank-at-EOL/EOF and space-before-tab
checks. No code/source test or global whitespace setting was relaxed. The failed
cached check is retained in the log archive. Final byte comparison and cached diff
check must both pass before commit. Historical files were not renormalized.
