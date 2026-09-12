# Phase 1N — Firebomb grant and stop before shop entry

Status: Phase 1N complete after user-approved original CRLF recovery on 2026-09-13 (KST). The local checkpoint is the commit containing this report; parent/start checkpoint `7845493cd7d4edb16b703e38bae510f02bff96aa`.
Worktree `C:\Users\jc\MemoriaMigration\foundation`; branch `unreal-migration/ue58-foundation`.
Engine: UE 5.8.2 / CL56702186, `C:\Program Files\Epic Games\UE_5.8`.
No push or Phase 1O implementation is authorized or performed.

## Bounded behavior and source authority

The same canonical VN / paid food memory / native travel / physical walk / reaction /
Accept original choice 0 / sword payment / real 0.3-second delay / deal / real 0.5-second
delay / reward / done flag / world seed / potion2 / antidote1 run now grants firebomb1.
The source callback next calls `_open_malet_shop()`. The new frontier is **before entry**,
including its local stock construction. That function would construct two authored
memory offers, call `MemoryShop.open_shop`, then connect `shop_closed`. The external
shop function's first mutation is `is_open = true` after the existing open guard.
None of that code is invoked by this implementation.

The oracle executes extracted original `add_item`, ITEMS, recent query/record,
export/import, runtime localization and `show_toast`, with observations immediately
after the actual queue append. Only profile output and visual delivery are inert.
No queue result, signal payload, or inventory result is manufactured. Shop methods
are attested by read-only extraction and are never invoked by the N oracle.

Executed Firebomb ITEMS record:

```json
{"name":"Firebomb","desc":"Deals 12 damage, then burns the enemy for 2 turns.","type":"burn","power":15,"impact":12,"price":18,"icon":"res://assets/ui/items/firebomb.png"}
```

Both actual en and ko source outputs request `+1 Firebomb`, SUCCESS enum value1.
The record's damage/use fields are data evidence only; no item-use effect is implemented.
[Per-file source attestation](evidence/phase1n/source02/source_attestation.json) records
raw/LF hashes and individual revisions. GameManager and Verdan currently last changed
at `84684aefa5721f97ee2318d2f8fa64d92dd077bc`; notification toast at
`7f405268a1a221577c301cd9278754088a794da9`; SaveManager at
`2c59212d3f96244802b82ab99b0f59fc50bf3e1c`; shop at
`4f772fafb1c7c6da68f56f44d60a7c7fed7f3949`. All overlapping M hashes match.
[Inherited dependencies](evidence/phase1n/inherited_dependency_attestation.json)
independently compare14 narrative/world/registry/memory dependencies to M attestations.
The inventory-set revision is not substituted for those files' individual revisions.

## Authority, order and historical assertions

`AddRewardFirebomb` is a thin strict-ID wrapper over the existing private
`GrantRewardItem`. Potion and Antidote wrappers stay strict. Only those three IDs
have Supported grant scope; the16-ID source membership remains a separate recent
filter. Unknown ID/case mismatch and known-but-deferred smoke_bomb remain distinguishable.
There is one inventory mutation/recent/signal/toast implementation, no shadow counts,
completion flag, once guard, duplicate suppression or general item catalog importer.

Canonical before firebomb: items={potion:2,antidote:1}, raw recent=[antidote,potion].
After: items={potion:2,antidote:1,firebomb:1}, raw recent=[firebomb,antidote,potion].
Same Run ID, Player Memory object and World Cognition object; world revision2/sequence2,
full world DTO, Player definitions/connections/effective power/carry/residue/erosion/
passives and burnedHistory=[daily_market_food,identity_first_sword] remain unchanged.
HP100, Grains0, chapter1, en locale, flags and quick slots are preserved. Chapter1 is
existing development metadata and is not corrected here. Supplemental existing-item
fixtures retain their own quantities and world revision19/sequence23.

The final source suffix is:

```text
item:add:end:antidote:1
item:add:begin:firebomb:1
inventory:firebomb:0->1
recent_items:firebomb,antidote,potion
inventory_changed:firebomb
toast:+1 Firebomb:1
item:add:end:firebomb:1
development:deferred:before:shop_open
```

Actual inventory listeners receive item_id only and read already committed inventory
and recent. Synchronous post-broadcast development observations are separate snapshots.
Normal reward grants emit three signals and requests in Potion/Antidote/Firebomb order.
The source appends all three actual queue records. UE broadcasts the three request
payloads and the development history displays all three. A separate telemetry subscriber
observes request generation while a removed presentation subscriber receives zero.
This does not implement or certify the source's animated visual toast queue.

[Assertion continuity](evidence/phase1n/assertion_continuity.json) lists the exact169
retained identities and core assertions. `potion_contract_complete` retains full L state,
source prefix, identity/domain and binary checks. `antidote_contract_complete` retains
M's two-item full state, source prefix, two signal/toast requests, owner and binary
checks. Only the final permitted frontier advances. Old test names may mention older
frontiers; their original logical assertions and immutable golden files remain.

## Lifetime, recent and save scope

Querying raw recent returns first unique valid source IDs, cap5, without mutation.
Grant commits its ID at the front of that normalized list. Invalid/duplicate typed
raw arrays remain raw through source export/import and actual binary restore; queries
are compared separately. Explicit repeated API calls accumulate normally. Existing
zero/negative/native signed-addition contracts are retained; no general Variant coercion,
legacy save importer or broad numerical parity claim is added.

Actual inventory_changed listeners replace the run during each of Potion, Antidote
and Firebomb. Old commit snapshots are retained; stale toast and later grants cannot
leak into the new run. Original callback ownership is checked after each grant. Source
state-only replacement continues stale toast/later calls; UE intentionally retains its
previously approved original-owner cancellation policy. No new ownership policy is introduced.
Full source reward callback duplication and duplicate shop-signal connection remain
separate characterized defects; explicit repeated AddItem is not treated as that defect.

Stop replacement isolates fresh inventory/cognition. Safe native world travel teardown
preserves the completed same-run state; no world is destroyed inside input Tick.
Presentation removal does not roll back inventory. E/Enter/Space/Back held and repeated
input remains at Deferred with no new grants or downstream continuation.

Actual `SaveGameToMemory → LoadGameFromMemory → independent GameInstance/Run RestoreSave`
compares full Run/inventory/raw recent/flags, Player Memory and derived observations,
and World Cognition. Original canonical authority remains unchanged; restored objects
are independent. Schema1 remains; byte size is measured from this run, not fixed at M's
13,009 bytes. Restore produces zero inventory signals, toast requests and grant observations.
Disk slots, autosave, Field cursor resume, packaging and Steam readiness are not certified.

## Validation and evidence — retained 2026-09-11 record

The following records the state before approved recovery. The original failed gate and runtime execution records remain unchanged; the recovery results are recorded separately below.

Final UE build/editor/rendered191/191 and independent1,413 checks PASS; four original native views reviewed. Canonical Run ID `3F8EADA247316AF8E8D9F58CC71E7DAE`; actual independent binary13,121 bytes/schema1, restore signal/toast/grant0. Final original-source static gate remains pending a preservation decision. Source N17 and unchanged M16/L16/K13/J12/
I8/H9/G7/F7/E4/D10/C7 all PASS. I passed its first attempt; no retry was used and the
historical M timing discrepancy is not claimed resolved. Official Godot repo/VN/KO/editor
and memory/world15 PASS. Native Player Memory51 and CTest1 PASS. Host96/96 PASS.

The initial full UE execution passed191/191 and the independent validator passed1,402
checks. Initial inline image review withheld acceptance because repeated text appeared missing
in two recorded previews; the live Shop_Deferred image was readable. Later read-only
raw PNG comparison found identical, complete header/Potion/Antidote glyph masks in all
eight original initial/final files. Thus no actual Unreal render defect is established;
the initial rendering diagnosis is corrected. No original image was edited or replaced. Those original images/results remain in
[automation01](evidence/phase1n/automation01/unreal_validation.json) and
[visual review01](evidence/phase1n/visual_review01.json). The N five-item fixture was also
refined to start with five *other* valid IDs and actually verify tail eviction. Only new N
fixture outputs changed, after the initial execution finished; source01 retains the
original executed inputs/outputs. Every prior phase golden remains untouched.

The capture helper now allows eight presentation-only frames after choosing a recorded
snapshot and explicitly refreshes/redraws the current native development widget. All
three gameplay grants still run synchronously before any of these frames. The first
capture-helper rebuild failed with C2039 because `SetIsVolatile` is not a UWidget API;
the installed UE header establishes `ForceVolatile`, which replaced that call.
[automation02](evidence/phase1n/automation02/unreal_validation.json) and its original
compiler log retain the failure. No warning, timeout or source tolerance was suppressed.
Final runtime acceptance is based on the separate automation03 execution, not these earlier attempts.

The final index has zero Memoria errors and one existing `r.MotionVectorSimulation`
render-thread warning, attached to `Memoria.Antidote.AntidoteSignalReplacement`. The raw
engine log contains15 pre-suite `Condition failed` self-test diagnostic lines; these are
separate from the Memoria index. Official Godot editor preserves three ShaderV duplicate
UID warnings and one ObjectDB exit warning. All actual occurrences and compiler failure
are recorded in [diagnostics](evidence/phase1n/diagnostics.json), with original logs archived.

The 60-check static run found one original file byte mismatch: `assets/fonts/theme.tres`.
Actual3,522 bytes equals the unchanged worktree/reference3,625 bytes with exactly103 CR
characters removed from CRLF. Normalized contents are identical; change author is not
established. Current original mtime is2026-09-11T12:30:04.857778Z, before the completed
entering-manifest record at12:33:01Z. This does not establish when or by whom it changed.
All other4,216 original files match; the worktree protection check passes8,405/8,405,
including86 prior IR/fixtures and21 packages, with no new packages. The original has not
been overwritten. [Exact comparison and both byte copies](evidence/phase1n/original_theme_difference.json)
make the proposed CRLF-only restoration reviewable. User approval was requested because
the task explicitly forbids original source changes; local commit is withheld until this
final preservation condition is resolved. No baseline or whitespace rule is relaxed.

Entering baseline computed from the actual starting commit: prior IR/fixtures86,
UE packages21, protected worktree files8,405. Original Godot4217 is checked independently
against the original manifest. No new narrative IR, asset or package is produced.
N fixture/evidence attributes are path-scoped; original source and historical data are
protected byte-for-byte. Config backups are in `Saved/Validation/phase1n-config-backups`.
Staged evidence blobs must equal raw executed/captured bytes before local commit.

## Approved original-only recovery and final closeout — 2026-09-13 KST

The user approved restoring only
`C:\Users\jc\OneDrive\바탕 화면\메모리아\Game\assets\fonts\theme.tres`.
This is the original Godot checkout (`overnight-gameplay-graphics`, HEAD
`15df72809baa52eec281d8508f495a373e9f5884`), distinct from the migration worktree.
The migration worktree's same relative file was already correct; its bytes and mtime
were not changed. The author of the original CRLF-to-LF change remains **unknown**.

[Pre-write conditions](evidence/phase1n/recovery-20260913/pre_write_conditions.json)
prove the current original still equaled the September 11 raw copy. The candidate came
from the retained `original_theme_reference_bytes.tres`, originally copied from the
unchanged migration worktree, and was independently authenticated by the existing
`C:\Users\jc\MemoriaMigration\phase1a-original-manifest.json`.
Both old byte copies and `original_theme_difference.json` remain byte-for-byte intact.
Fresh before/candidate copies, both repository status/working/staged diffs and index
records are under [recovery-20260913](evidence/phase1n/recovery-20260913/pre_write_conditions.json),
with raw Git output retained in `before_git_snapshots.zip`. No evidence file was added
to the original protected checkout.

| Bytes | Size | SHA-256 |
| --- | ---: | --- |
| Before, LF | 3,522 | `e296ce15ac815e6b6601bdc91b3400322fd52d3e647bf7d9f80b9c59208152b5` |
| Verified CRLF candidate and restored original | 3,625 | `a38ef594223ee8ca8bbf292f9058f82ef03b5a588214f82ef6dff396ae93e2fd` |
| Existing original manifest expectation | 3,625 from authenticated candidate | `a38ef594223ee8ca8bbf292f9058f82ef03b5a588214f82ef6dff396ae93e2fd` |

The full candidate bytes equal the original expectation, and replacing its exactly103
CRLF pairs with LF reproduces the pre-write file exactly. UTF-8 content, absent BOM,
all other whitespace and the final newline are unchanged. These combined byte/hash
conditions were verified before opening the target for its one binary write; normalized
text equality alone was not used as authorization. Immediate re-read confirmed all3,625
bytes and the expected SHA-256. [Restoration receipt](evidence/phase1n/recovery-20260913/restoration.json).

Fresh post-recovery checks passed:

- [Static60/60 and original4,217/4,217](evidence/phase1n/recovery-20260913/static01/foundation_static.json),
  with the actual new process output retained separately. The old `static01` FAIL remains intact.
- [Entering worktree8,405/8,405, packages21, prior IR/fixtures86](evidence/phase1n/recovery-20260913/post_recovery_protection_final.json).
  Historical reports14 and execution evidence8,280, plus all4,362 previously staged Phase1N
  evidence files, retain their original hashes. No narrative IR, asset or package was added.
- Every original visible file was compared before/after; only the approved theme changed.
  Original working/staged diffs and both pre-existing indexes were preserved. Neither
  repository staged `assets/fonts/theme.tres`; no source newline change enters this feature commit.
- Final working/cached diff checks, selective scope review and raw staged evidence blob
  equality are recorded in [final review](evidence/phase1n/recovery-20260913/final_review.json).
  Existing staged implementation/evidence were preserved; only current Phase1N documentation
  and the new recovery directory were added or restaged for this closeout.

The added recovery auditor initially misclassified1,803 inherited asset paths because
it used the migration-only protection subset as the full repository inventory. A second
extra comparison incorrectly applied original-checkout raw hashes to worktree files.
Both failed recovery-audit results are retained. The full entering Git tree proves all
flagged paths pre-existing and new content0; all912 cross-checkout byte differences match
exact expected worktree checkout bytes from that same commit under the existing rules.
[Baseline resolution](evidence/phase1n/recovery-20260913/worktree_asset_baseline_resolution.json)
records this read-only proof. No protection manifest, expected hash, source attribute,
whitespace rule or Git configuration was changed, and no additional source file was written.

The first recovery staged-diff check rejected blank context lines in the raw documentation
diff. Its failure is preserved in `staged_diff_attempt01.json`. The identical raw bytes
were archived in `recovery_documentation_diff.zip` and retained locally as an ignored log;
only that newly added text-index entry was removed. No whitespace rule was changed.
The final staged check below is the successful separate attempt.

**Runtime results are reused, not re-executed during recovery.** All77 files in the
September 11 `final_execution_inputs.json`, including runtime source, tests and validation
inputs, still have their recorded full SHA-256 values. Existing packages and prior inputs
also pass preservation. The reused results are automation03 UE191/191 and acceptance03
independent1,413; their original build/editor logs, native captures, warnings and failed
previous attempts remain intact. The new work executed byte restoration, preservation,
static and Git/staged-byte checks only. No new full UE, Godot or host runtime run is claimed.

All required closeout gates pass. The approved Phase1N implementation and recovery
records form the local checkpoint containing this report. No push or Phase1O implementation.

## Remaining limits and next dependency

The stop is a development boundary, not source exploration completion. Shop stock,
merchant preparation, MENU/shop widget, first_shop hint, shop-close delegate, ch2_complete,
chapter3, autosave, achievement/profile, 1.5-second timer and next-map requests stay absent.
Exact traces, full state snapshots and static call-scope checks jointly support the zero
observed downstream counters; no fake empty shop authority is created.

A separately authorized Phase1O should first establish the source shop-entry dependency
closure: authored two-offer stock and grade values, merchant state, open reentry guard,
MENU ownership, portrait/grains/list/UI/audio dependencies, and first_shop hint behavior.
Choose an explicit supported shop-entry boundary before implementing it. Acceptance should
preserve these191 identities and all three item-complete contracts, attest source records,
verify full Run/Player/World and input/modal lifetime, and stop before any purchase/sale,
close callback, chapter advancement or persistence side effects that are not separately
approved. No Phase1O code is included here. Toast animation and I oracle harness stabilization
remain separate work, with source defects and stale-owner differences explicitly tracked.
