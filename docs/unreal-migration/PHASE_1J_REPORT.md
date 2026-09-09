# Phase 1J — First authoritative Malet reward effect

Status: **COMPLETE** — all bounded acceptance passed on 2026-09-09.
Worktree: `C:\Users\jc\MemoriaMigration\foundation`.
Branch: `unreal-migration/ue58-foundation`.
Engine: **UE5.8.2 / CL56702186** at `C:\Program Files\Epic Games\UE_5.8`.
Accepted technical checkpoint and actual starting documentation HEAD:
`b3308a72d745e144704f7675fd35b78f555b5417`.
New checkpoint will be the enclosing local `feat(unreal): commit first Malet reward effect` commit;
resolve with `git log -1 --format=%H -- docs/unreal-migration/PHASE_1J_REPORT.md`.
No remote push. No Phase1K implementation.

## Scope and no-new-content proof

Only new gameplay mutation is `ch2_malet_done=true`. Existing run API
`UMemoriaRunSubsystem::SetStoryFlag(FString, bool)` owns it; no shadow boolean,
world owner, alternate inventory or test-only authority was added. The callback
uses the same run/domain from paid VN through native travel/payment/reward.

No narrative group, IR, importer rule, asset or map changed. Existing reaction,
encounter, refused, deal, reward assets are reused. All **21 existing UE packages**
and all **76 prior IR/fixture files** retain their bytes. The new source callback
fixtures are test observations, not production narrative content.
[Preflight](evidence/phase1j/preflight.json), [content protection](evidence/phase1j/content_protection.json).

## Executable source flag contract

Fresh source revision `4f772fafb1c7c6da68f56f44d60a7c7fed7f3949`.
[Exact methods and raw/LF hashes](evidence/phase1j/oracle03/source_attestation.json).
`GameManager.set_flag` assigns `story_flags[flag_name]=value`, then prints a flag-set
log. Absent, existing false, and existing true all execute this assignment/log.
There is no equality short-circuit, signal, EventBus emission, autosave or achievement.
String dictionary keys are case-sensitive. `get_flag` returns false for an absent
key, while dictionary membership distinguishes absent from explicit false.

`export_data` copies the story_flags dictionary into the save's game record.
The source oracle executes that exact export and serializes its booleans/keys.
Source import assigns the saved dictionary; UE's existing typed array representation
preserves the same exact identities and bool values. The new UE contract test
round-trips the actual run DTO through SaveGameToMemory/LoadGameFromMemory.
Run API's bool success return is adapter status; Godot's setter returns void.

Original `_on_reward_ended` order is:

```text
set_flag ch2_malet_done
_seed_malet_memory_world_state_if_needed
add_item potion 2
add_item antidote 1
add_item firebomb 1
_open_malet_shop
```

Setter return -> seed call is synchronous. No completion timer exists. A preexisting
true flag does not skip the setter or seed. Diagnostic UE `flag:` trace records the
call even when the stored value was already true; no extra gameplay bookkeeping.

## Exact canonical route and stop

Actual rendered PIE and Enhanced Input:
L_Ch2VerdanSlice -> paid VN original1 -> actual daily_market_food burn -> native
Verdan travel -> physical walk -> E reaction0..2 -> E normal encounter0..9 ->
Accept0 -> actual identity_first_sword payment -> source0.3s -> deal0..4 -> separate
source0.5s -> reward0..7 -> completion. No canonical teleport, direct interpreter
entry, manual flag/memory setup, run reconstruction, or fixture reset substitutes
for any of these steps. Only supplementary preexisting-flag tests use explicit
SetStoryFlag setup, after the ordinary payment/deal route has run.

```text
field:visit:8
field:end                         # completed malet_reward
state:exploration
callback:reward:enter
flag:ch2_malet_done
development:deferred:before:world_memory_seed
STOP
```

Source end_dialogue clears its active rows/index/choices, enters exploration, then
emits dialogue_ended synchronously. Existing one-shot reward callback was connected
after0.5s. UE preserves the source event order and immediately enters its development
modal after committing the flag. It does not call any seed method, queue a reward
continuation/timer, apply effects then roll them back, or expose an extra exploration
frame. Existing input/UMG owns presentation only; E/Back/movement cannot escape the stop.
The read-only status displays actual run flag plus zero seed invocations.

## Source world-seed characterization — NOT Unreal authorization

Source guard order: get_flag(ch2_malet_done) -> WorldState.get_actor_state(npc.malet)
-> return if empty -> knowledge key membership -> optional learn_fact -> memory
record lookup -> optional add_memory. get_actor_state requires registry membership
and a stored world actor, returns a deep copy, and does not create an actor. A missing
actor returns early without repairing the registry/state. The missing-actor fixture
erases only its isolated world actor record; source import normalization would add
registry actors back, so it is explicitly a malformed-state test fixture.

Fresh first actual write: `MemoryEngine.learn_fact("npc.malet",
"fact.bl07.route_request_received")` -> `WorldState._store_knowledge_value` stores
that knowledge entry (true, updated_revision1), advances revision1, allocates event
sequence1 and emits `world.00000001` / `knowledge.learned` through EventBus.
Then add_memory creates `memory.malet.bl07_request_source` (owner npc.malet,
source_actor_id player.arrel, fact_ids route fact, information_source/bl07_route_request
content), advances revision2/sequence2, emits `world.00000002` / `memory.added`.
EventBus validates and emits a deep copy; it owns no persistent state. ActorRegistry
is a lookup-only participant in this seed. IDs, full event payloads and lookup/store
traces are retained in the golden observations.

| Source case | Revision | Events during callback/seed |
|---|---|---|
| fresh | 0 -> 2 | knowledge.learned, memory.added |
| knowledge_present | 1 -> 2 | memory.added |
| memory_present | 1 -> 2 | knowledge.learned |
| memory_removed | 2 -> 3 | knowledge.learned |
| knowledge_forgotten | 2 -> 3 | memory.added |
| actor_missing | 0 -> 0 | none |
| already_true | 0 -> 2 | knowledge.learned, memory.added |
| repeated_reward | 0 -> 2 | knowledge.learned, memory.added |
| false_flag | 0 -> 2 | knowledge.learned, memory.added |
| case_sensitive | 0 -> 2 | knowledge.learned, memory.added |
| seed_guard_false | 0 -> 0 | none |
| memory_restored | 3 -> 4 | knowledge.learned |

The existing-record cases deliberately separate knowledge and memory: an existing
memory alone does not imply the knowledge exists. Explicit forgotten knowledge is
created by actual learn_fact then forget_fact; forgetting a missing fact is a no-op.
The seed tests key membership, not current knowledge truth, so false is preserved.
Removed/restored records remain byte-identical, including historical revisions.
Knowledge-only seeds just memory; memory-only seeds just missing knowledge.
No-op seed creates no revision/event. Direct seed with false flag returns immediately.

Repeated full reward callback calls/logs the setter and seed again, but seed itself
is idempotent. The full handler is NOT idempotent: potion becomes4, antidote2,
firebomb2, and the second shop_closed connection logs a source duplicate-connection
ERROR. This intentional source diagnostic is retained and accepted only once by
exact full-line/count matching in this new oracle; every other fatal diagnostic
still fails. It is not fixed or recreated in Unreal. Source shop presentation,
profile/storage and actual chapter travel endpoints remain inert oracle sinks.

## Authoritative delta and lifecycle evidence

At canonical reward entry/last line/before callback/before flag, ch2_malet_done is
absent. Immediately after SetStoryFlag, at completed UI and stable stop it exists
with true. Full run comparison permits only that one flag delta. Full memory and
Run ID/domain identity remain unchanged; burn history remains
[daily_market_food, identity_first_sword], sword once. Payment state, residue/erosion,
HP100, Grains0, inventory/recent items, other flags remain as before reward.
Development fixture current_chapter remains1; Chapter2 metadata bootstrap debt is
not silently corrected to2 or3.

World/shop owners are not implemented; snapshots explicitly contain null for those
owners. Zero observed world/item/shop/chapter/autosave/achievement event counts are
combined with exact run/inventory equality and static no-call proof. They are not
presented as measured state from a fabricated world owner. No ch2_complete, reward
grant, world revision, knowledge/memory creation/restoration, shop or next-map request.

Read-only Automation observers record the synchronous boundaries before callback,
before flag (the historical I frontier), and after flag but before development defer.
Normal executable route has no authority-changing observer. Lifecycle fixtures use
real BeginStartingMemoryRun or OpenLevel at those seams. Run replacement before
commit cancels the stale callback; after commit preserves the old committed snapshot
and gives the new run its own clean flags. No rollback runs. World requests are
observed synchronously via UE WorldContext.TravelURL; native teardown/travel then
occurs at the engine's safe point and preserves the old run, including any committed
flag. Both invalid run identity and outgoing/destroyed world invalidate continuation.

This is not an invented source timer/gap. Godot state replacement alone retains
its source listener; I's executable replacement/owner destruction fixtures remain
unchanged and are rerun. UE deliberately retains its accepted OnRunReplaced
cancellation policy and cancels a pending outgoing-world continuation. Direct actor
Tick teardown is not used as an approximation of native travel.

## Tests and validation

Existing exact97 Automation identities retained; new10 = **107/107 PASS**:

- Memoria.MaletFirstEffect.AuthoritativeFlagContract
- Memoria.MaletFirstEffect.Canonical
- Memoria.MaletFirstEffect.PreexistingTrue
- Memoria.MaletFirstEffect.PreexistingFalse
- Memoria.MaletFirstEffect.CancelBeforeCallbackOnRunReplace
- Memoria.MaletFirstEffect.CancelBeforeFlagOnRunReplace
- Memoria.MaletFirstEffect.CancelAfterFlagOnRunReplace
- Memoria.MaletFirstEffect.CancelBeforeCallbackOnWorldTeardown
- Memoria.MaletFirstEffect.CancelBeforeFlagOnWorldTeardown
- Memoria.MaletFirstEffect.CancelAfterFlagOnWorldTeardown

Historical AcceptPreEffectDeferred/CanonicalPayment/AlreadyBurnedPayment/reward
boundary names remain; they now extend to the authorized first flag and stop before
seed. The I trace through completion is compared unchanged, with only J suffix.
The old two-boundary lifetime coverage remains and the new exact synchronous seams
are additional coverage. Historical host test names also remain; their prior97 set
must be a subset of the new exact107 and downstream prohibition now permits one flag.
Refuse cleanup/retry, failed/already-burned payment, reaction priorities, native travel,
Field/VN and catalog contracts remain required.

| Verification | Result / evidence |
|---|---|
| UE5.8.2 UBT/UHT/compile/link, Editor/rendered PIE | PASS, [automation03](evidence/phase1j/automation03/unreal_validation.json) |
| Exact old97 + new10 Automation identities | **107/107**, [index](evidence/phase1j/automation03/automation_index.json) |
| Independent I/H/G history plus J trace/state comparison | **358 checks PASS**,287 parsed snapshot JSONs, [acceptance](evidence/phase1j/acceptance01/acceptance.json) |
| New executable source oracle | **12/12**, oracle03 plus fresh deterministic --check |
| Retained I/H/G/F/E/D/C source regressions | **8/9/7/7/4/10/7**, [suite](evidence/phase1j/regressions01/suite.json) |
| Official Godot repo/VN/KO/editor/memory-world | PASS, memory-world **15/15**, [Godot](evidence/phase1j/godot01/godot_baseline.json) |
| Native memory / CTest | **51/51 +1/1**, [native](evidence/phase1j/native01/native_memory_validation.json) |
| Host validators | **69/69**, [host02](evidence/phase1j/host02/host.json) |
| Static / protected original | **59/59;4217/4217**, [static](evidence/phase1j/static01/foundation_static.json) |
| Existing UE package set and bytes / prior IR+fixtures | **21/21;76/76**, no new package, [protection](evidence/phase1j/content_protection.json) |

All accepted processes exited0. The original r.MotionVectorSimulation warning remains
once in Memoria.Campaign.CanonicalPaidRoute; all new10 tests have zero warnings/errors.
No engine/test settings were changed to hide it. Known official Godot legacy output
and the exact expected repeated-callback source diagnostic remain in full logs.
The official Godot harness restored its temporary import metadata. Original checkout
HEAD/status and protected data remain unchanged. [Full logs](evidence/phase1j/full_logs.zip)
and [inventory](evidence/phase1j/full_logs_inventory.json) preserve failed and successful attempts.

All four original1286x760 PNGs were visually inspected without image editing:
[Reward_LastLine](evidence/phase1j/automation03/Phase1J/FirstEffectCanonical_Reward_LastLine.png),
[Reward_Completion](evidence/phase1j/automation03/Phase1J/FirstEffectCanonical_Reward_Completion.png),
[MaletDone_Set](evidence/phase1j/automation03/Phase1J/FirstEffectCanonical_MaletDone_Set.png),
[WorldSeed_Deferred](evidence/phase1j/automation03/Phase1J/FirstEffectCanonical_WorldSeed_Deferred.png).
Final evidence contains637 files generated after this accepted run began, with no
stale files included. [Capture provenance](evidence/phase1j/automation03/capture_provenance.json),
[visual review](evidence/phase1j/visual_review.json),
[complete canonical trace](evidence/phase1j/acceptance01/canonical_full_trace.txt).
Synchronous snapshots, before_reward, reward_last, after_flag, reward_effects_deferred
carry complete run/memory/flags/inventory/chapter plus callback/timer/trace evidence.

## Defects, attempts and limits

- Preflight JSON writer had a Python parenthesis syntax error before any write; corrected.
- Oracle01 exposed the expected full-handler repeated shop signal connection ERROR.
  Exact one-line/count allowance was added only for this characterization; logs remain.
- Oracle02 forgotten fixture called forget on missing knowledge (source no-op). Corrected
  fixture to actual learn then forget; oracle03 and independent fresh check pass12/12.
- Automation01: new unit test used InitializeStandalone on a transient GameInstance
  without Engine outer. Changed to the existing strong-owned GameInstance/Init test
  pattern. The genuine canonical payment/reward/flag route had passed before that crash.
- Automation02: direct BeginTearingDown from the input callback violated UE Tick/UMG
  teardown ordering. Replaced with real queued OpenLevel plus outgoing-world guard;
  no test-only cleanup broadcast, engine suppression or source timer change.
- Supplementary preexisting flag setup required its own entry expectation, retaining
  canonical absent assertions. Old host method/count assertions were extended without
  renaming or dropping prior identity/source-history coverage.

This is only first reward flag parity, not full Malet/World Memory parity. World
owner/seed, items/shop/trade, ch2_complete/chapter3/autosave/achievement/1.5s transition,
world followup/extraNPC/battle/NewGame/Ch1/finalUI/art/audio/graphics/cook/package/Steam
are not implemented. Current chapter fixture debt, final Korean typography/assets,
Field save-resume and physical USB input certification remain outside this slice.

- The first base64 PNG inspection transfer truncated three images at its output
  budget. Increasing the read-only transfer budget allowed direct inspection of all
  original PNGs; no rendering or evidence file was changed.
- Added only phase1j raw-log ignore and first-effect fixture LF rules, matching prior
  phases. Original .gitignore/.gitattributes bytes are backed up under Saved/Validation;
  rollback is removing those two additive rules. Raw logs remain byte-exact in ZIP.

## Exact Phase1K recommendation (not implemented)

Separately authorize only the source seed function through existing persistent-world
architecture: preserve the committed ch2_malet_done, perform guarded missing route
knowledge then missing route memory creation through authoritative world command/state
owners with exact revisions/events, retain missing-actor and removed/forgotten/restored
semantics, then STOP before GameManager.add_item("potion",2). Require the characterized
source cases and all107 retained tests. No item/shop/chapter/profile effect is implied.
