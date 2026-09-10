# Phase 1L — First Malet reward potion contract

Status: **COMPLETE**. All bounded acceptance passed. Local checkpoint only; push prohibited. No Phase1M code.
Worktree `C:\Users\jc\MemoriaMigration\foundation`, branch `unreal-migration/ue58-foundation`.
Previous accepted checkpoint `6108aad55f80cc006272ac0e5513012b7313c8e3`.
UE5.8.2 / CL56702186, `C:\Program Files\Epic Games\UE_5.8`.
The new technical/documentation checkpoint is the enclosing local
`feat(unreal): grant first Malet reward item` commit. Resolve its full SHA with
`git log -1 --format=%H -- docs/unreal-migration/PHASE_1L_REPORT.md`.

## Exact authorized boundary

Same actual rendered PIE route: L_Ch2VerdanSlice, paid VN original1, real food burn,
native OpenLevel, physical walk/E, Malet reaction0..2, encounter/Accept0, real sword
burn, real0.3s, deal0..4, real0.5s, reward0..7, done flag and real source world seed.
Only then does the existing authoritative Run inventory receive potion2. No canonical
teleport, fixture replacement, run reconstruction, direct AddItem-only shortcut or
manual world/inventory injection. Supplementary source tests explicitly use fixtures.

```text
callback:reward:enter
flag:ch2_malet_done
worldseed:enter
world:knowledge:npc.malet:fact.bl07.route_request_received
world:revision:1
world:memory:npc.malet:memory.malet.bl07_request_source
world:revision:2
worldseed:end
item:add:begin:potion:2
inventory:potion:0->2
recent_items:potion
inventory_changed:potion
toast:+2 Potion:1
item:add:end:potion:2
development:deferred:before:item:antidote:1
STOP
```

The `item:add`, `inventory`, `recent_items` and development boundary records are
observational trace, not invented source signal payloads or a second inventory.
Synchronous source calls stay synchronous; no timer/frame gap was added. Canonical
before potion has items={} / recent=[] from the accepted development run, not the
unmigrated production New Game. After: items={potion:2}, recent=[potion].

## Executed Godot source contract

[Oracle](evidence/phase1l/oracle01/oracle.json) executes16 cases with exact extracted
GameManager.add_item, get_recent_items, _record_recent_item, export_data/import_data,
get_item_quick_slots and localized_runtime_text; exact ITEMS and localization constants.
Only removable observation lines are inserted. NotificationToast.show_toast executes
its real localization/queue call, with visual queue processing and profile disk writes
replaced by inert sinks. SaveManager's game export/import delegation and original
verdan_market reward handler were read and attested. Neither original checkout nor
migration Godot gameplay source is edited. Full attestation records exact methods/hashes.

1. `potion` is an exact case-sensitive String dictionary key in ITEMS. Display name
   `Potion` is content, not identity. Unknown `bad` or uppercase `POTION` returns before
   mutation, recent update, signal, toast, or log. Case-distinct inventory keys survive.
2. Source reads player_data.items.get(item_id,0), then writes current+count. An absent
   key becomes present even for amount0. Count1 becomes3;9 becomes11; repeated2 gives4.
   No positive-amount guard/clamp/key removal: source count1 plus -2 becomes -1 and
   toast text is `+-2 Potion`. Native int64 addition avoids C++ signed-overflow UB;
   broad overflow/legacy numeric-type parity is not certified by these cases.
3. get_recent_items reads the array, String-converts entries, filters IDs by source
   ITEMS membership, removes duplicates in first-seen order, stops after5. Recording
   removes the granted ID, pushes it to front, truncates to5, writes the array. No
   quantity is stored. This unique most-recent ordering is source behavior, not a fix.
   Native DTO accepts typed FString arrays; arbitrary Godot Variant coercion/import
   remains outside the native-save contract.
4. `inventory_changed(item_id: String)` emits once AFTER inventory and recent mutation.
   Payload is only `potion`, no amount/count field. Observers read fully committed state.
5. Then NotificationToast.show_toast receives `+2 Potion`, SUCCESS enum1. Source formats
   from count and ITEMS name, then calls localized_runtime_text. Both en and ko yield
   `+2 Potion`; no invented Korean translation. Source visual marker is `[+] `.
6. export_data shallow-copies player_data; JSON serialization captures nested inventory
   and recent array. import_data restores it, adds missing legacy defaults, normalizes
   quick slots but does not normalize recent items on import. Every source case records
   JSON export/import roundtrip. Source JSON numbers become integral floats on parse;
   comparisons preserve numeric value. No source inventory signal/toast is replayed
   merely by import.

Cases: absent, existing1, existing9 with unrelated antidote count7, already_recent,
unrelated, multiple previous items, malformed/duplicate recent IDs, repeated grant,
invalid, case_sensitive, zero, negative, save_restore, ko, replacement_signal,
presentation_absent. Display availability is an inert source sink; the native absence
contract additionally grants with NO presentation subscribers at all.

## Authority, presentation and lifetime

`UMemoriaRunSubsystem::AddRewardPotion` mutates existing FMemoriaRunSnapshot.Player.Items
and RecentItems. GetItemCount returns0 for absence. No shadow counter/completion bit,
new SaveGame section/schema, generic item use/catalog importer/equipment/shop API.
The bounded grant API rejects all IDs except potion; the16 pinned ITEMS identities
are used only to retain source recent-list membership. This is not general inventory
parity or authorization to grant the other15 items.

Typed OnInventoryChanged carries only source item_id. OnItemToastRequested carries
source text/type. OnPotionObserved carries value-only development snapshots, distinct
from gameplay notification. Narrative subscribes for the synchronous call, records
trace/read-only snapshots, and detaches immediately. Existing UMG reads committed or
explicitly recorded values and displays the request; it never changes inventory.
Final typography, animation, art, audio, general localization and a global notification
framework are not implemented. Phase-local raw-log ignore and fixture LF rules are
additive; originals are backed up under Saved/Validation/phase1l-config-backups.
Rollback removes those two rules; raw logs remain archived byte-for-byte.

Same Run ID, Player Memory object and World Cognition object persist across canonical
travel/payment/reward/potion. The entire Player Memory snapshot, definitions, transient
connections, effective power, carry (float64 observed as17-digit string), residue,
cascade erosion/faded/passives remain equal across potion. BurnedHistory remains
[daily_market_food,identity_first_sword]. World snapshot stays EXACT, revision2 and
sequence2: all4 actors, fact.veil.exists, route fact/memory, metadata, flags/quest bags.
Potion calls no LearnFact/AddMemory or player-memory mutation.

HP100,Grains0,currentChapter1,locale,other flags and unrelated item entries remain.
ch2_malet_done=true was committed before potion and stays true. Historical currentChapter1
metadata is deliberately not rewritten. New actual PIE lifetime cases replace before
potion, after inventory+recent commit/notification, and at the final stop; actual native
world teardown after completed grant preserves old committed inventory/world/run.
Presentation disappearance does not undo committed inventory. No asynchronous potion
cancellation point or compensation/rollback is invented.

SOURCE DIFFERENCE: Godot signal observer replacing player_data leaves the old function
running and still emits its toast against the replacement state. The executable case
records this. UE retains accepted run-generation cancellation: replacement ends the old
remaining contract, suppressing its stale presentation. New run inventory stays empty;
old committed evidence is retained. This is lifecycle isolation, not duplicate suppression.

## Binary SaveGame and source defects

The canonical actual final run is captured, serialized using SaveGameToMemory, reloaded
using LoadGameFromMemory and restored into an independent real Run subsystem. Original
canonical authority stays unchanged. Compare run, inventory/recent, story flags, full
player snapshot+derived observations, and world independently. Existing schema1 and
existing typed inventory fields suffice. All16 supplemental cases also binary-roundtrip,
including zero/negative count, ordering, repeats and malformed recent arrays. No disk
slot writer/autosave orchestration or Field cursor resume is added.

SOURCE DEFECT: full reward callback is non-idempotent; it can duplicate items and emit
the previously recorded duplicate shop signal error. EXPECTED SOURCE PARITY: repeated
potion API calls each add2 and emit signal/toast, with source recent ordering. No once
flag, duplicate suppression, hidden boolean, compensation or Godot fix. FUTURE FIX
DECISION: reward-wide idempotency requires separate explicit authorization and parity
versioning; it is not silently introduced here. Zero/negative grants are characterized
source behavior; no new validation policy is imposed.

## Validation and evidence

| Gate | Result / current evidence |
|---|---|
| UE5.8.2 UBT/UHT/compile/link/Editor/rendered PIE | PASS, [automation03](evidence/phase1l/automation03/unreal_validation.json) |
| Exact125 retained +22 new Automation | **147/147 PASS**, [index](evidence/phase1l/automation03/automation_index.json) |
| Independent full state/order/lifetime comparison | **728 checks PASS**,689 JSON snapshots, [acceptance02](evidence/phase1l/acceptance02/acceptance.json) |
| New source and prior source regressions | **L16/K13/J12/I8/H9/G7/F7/E4/D10/C7 PASS**, [suite](evidence/phase1l/regressions01/suite.json) |
| Native Player Memory / CTest | **51/51 +1/1 PASS**, [native01](evidence/phase1l/native01/native_memory_validation.json) |
| Host validators | **83/83 PASS**, [host03](evidence/phase1l/host03/host.json) |
| Official Godot / static / original protection | **Godot15/15, static60/60, protected4217/4217 PASS**, [Godot](evidence/phase1l/godot01/godot_baseline.json), [static03](evidence/phase1l/static03/foundation_static.json) |
| Prior UE packages / IR+fixtures | **21/21 +82/82 unchanged**, [protection](evidence/phase1l/content_protection.json) |
| Native PNGs |5 direct visual reviews, [visual_review](evidence/phase1l/visual_review.json) |

Final capture provenance contains1182 fresh files (689 JSON,493 PNG), filtered against
this run's recorded start time, with SHA256. No older run is substituted. Original
r.MotionVectorSimulation warning remains1. The earlier automation02 also recorded an Editor LogHttp connectivity
probe timeout (google generate_204,3s) in an existing teardown test; that test passed.
Final automation03 has ONLY the existing render warning1. All22 new tests have zero warnings/errors. Neither warning is hidden
or removed by settings changes. Full successful and aborted attempt logs are archived.


Fresh machine-readable evidence includes before_potion, after_inventory_mutation,
after_recent_items, after_inventory_changed, potion_complete, antidote_deferred,
canonical_full_trace and actual binary save_roundtrip. An independent validator compares
full H/I history, K seed snapshots, L source order/intermediate values, every retained
identity, no downstream events, owner lifetimes and complete preserved state. PNGs are
native original rendered captures; synchronous earlier states explicitly say
RECORDED SYNCHRONOUS SNAPSHOT / READ ONLY. Final stop reads live authority.

Existing exact125 identities remain; new22: source16, NativeInventoryContract,
CanonicalPotionGrant, ReplacementBeforePotion, ReplacementAfterCommit, ReplacementAtStop,
WorldTeardownAfterCommit. Logical recent/signal/toast/repeat/invalid/save/player/world/
nonexecution coverage is asserted within those cases, without padding the count.
Historical pre-effect test names remain semantic debt: their source-history prefix is
unchanged and current authorized suffix now ends before antidote. No test is dropped.

Attempt01 was explicitly aborted after read-only review found five newly added PIE
wrappers missing the existing AutomationOpenMap/EndPlayMap setup. The wrappers were
corrected and automation02 passed147. Final review then made the status text read
the actual flag instead of a literal true and strengthened supplemental full-player
preservation assertions; automation03 validates those final changes. Partial logs
are retained, never relabeled as success. Earlier standalone UBT build passed.

## Limits and exact Phase1M recommendation

Antidote and firebomb mutations/recent updates/signals/toasts never execute. No shop
state/stock/signal/UI/trade/close, ch2_complete, Chapter3, autosave, achievement,1.5s
transition,next map,world followup,extraNPC,battle/random encounter,Production New Game,
Chapter1,finalUI/art/audio,graphics modernization,Field save-resume,physicalUSB,
cook/package/Steam implementation. Zero new narrative IR/assets/packages; all existing
packages and prior IR/fixtures and original4217 files are protected. Full Inventory,
Malet,Verdan,Shop parity or Steam readiness is NOT claimed.

Recommend a separately authorized Phase1M restricted to source add_item("antidote",1)
after this exact completed potion contract, including that call's recent-list, signal
and toast behavior. Preserve147 identities, same run/player/world and world revision2,
then STOP before add_item("firebomb",1). Recharacterize exact antidote source payload
and repeated/lifetime/save semantics first. No Phase1M implementation exists here.

Static01/02 failures are retained: additive document writes first removed terminal
newlines, then Windows text-mode writing converted the protected LF prefix to CRLF.
Restored accepted HEAD bytes and wrote the appendix as bytes; no static check was
weakened. Historical document/evidence content stays unchanged. Source profiles and
Godot protected files passed preservation during both failed static attempts.

Final evidence: [full logs](evidence/phase1l/full_logs.zip), [log hashes](evidence/phase1l/full_logs_inventory.json), [normalized potion order](evidence/phase1l/normalized_potion_trace.json), [canonical binary save](evidence/phase1l/acceptance02/save_roundtrip.json), [final checks](evidence/phase1l/final_checks.json).
