# Phase 1K — first Malet world knowledge and route memory seed

Status: **COMPLETE**. All bounded Phase1K acceptance passed. Local checkpoint only. No push, no Phase1L.
Worktree `C:\Users\jc\MemoriaMigration\foundation`, branch `unreal-migration/ue58-foundation`.
Accepted previous/current starting HEAD: `00a58c86744a412bd9790ea632e731b745ba6841`.
Engine UE5.8.2 / CL56702186, `C:\Program Files\Epic Games\UE_5.8`.
New checkpoint is the enclosing `feat(unreal): seed first Malet world memory` commit;
resolve with `git log -1 --format=%H -- docs/unreal-migration/PHASE_1K_REPORT.md`.
Final response records its full SHA after all acceptance. No remote operation.

## Bounded authority and source re-characterization

Only the guarded seed's missing route knowledge and missing route world memory are
new gameplay effects. Existing reward0..7 -> callback -> ch2_malet_done=true remains.
After synchronous worldseed:end the development modal stops BEFORE the first potion2
inventory mutation. No partial grant/rollback, recent-item write, inventory_changed,
toast, antidote, firebomb, shop/trade/close, ch2_complete, Chapter3, autosave,
achievement, 1.5s transition, next map, followup, NPC/battle/NewGame/Ch1, final art,
graphics/cook/package/Steam implementation is included.

Source revision `4f772fafb1c7c6da68f56f44d60a7c7fed7f3949` is freshly attested, including
verdan_market.gd constants/seed/reward callback, GameManager flags/export/import,
WorldState/ActorRegistry/MemoryEngine/EventBus, actor catalog, SaveManager and downstream
boundaries. [Attestation](evidence/phase1k/oracle01/source_attestation.json) contains raw
and LF hashes plus original methods. Source gameplay files are unchanged.

The new seed-only harness reuses Phase1J's isolation and executes all four real engine
files verbatim. Only inherited super-calling lookup/store telemetry and removable map
observer lines are added. It calls the actual seed rather than the full reward handler;
no precomputed resolver result supplies its expected state. Source export/import is
executed for save_restore. [Inputs](fixtures/malet_world_seed/contract_inputs.v1.json),
[results](fixtures/malet_world_seed/contract_expected.v1.json),
[initial13](evidence/phase1k/oracle01/oracle.json) and a fresh deterministic --check.
[Executed-copy proof](evidence/phase1k/executed_source_attestation.json) verifies the four files actually used are byte-identical and records exact source save/flag methods.
The broader inherited oracle metadata mentions inert world adapters from older phases;
this K attestation explicitly identifies the real world engines used here.

Exact actor identity is `npc.malet`, looked up through catalog validity and persistent
actor state, never an Unreal display/Object/FName identity. Canonical source is
`player.arrel` (source spelling retained). Knowledge key is the identity-free
`fact.bl07.route_request_received`: record `{fact_id:<key>, value:true, updated_revision:1}`.
Route memory is `memory.malet.bl07_request_source`:

```json
{"id":"memory.malet.bl07_request_source","owner_actor_id":"npc.malet","status":"active","fact_ids":["fact.bl07.route_request_received"],"source_actor_id":"player.arrel","content":{"kind":"information_source","subject":"bl07_route_request"},"created_revision":2,"removed_revision":0,"last_removed_revision":0,"restored_revision":0}
```

Fresh source world schema1 has revision0/event_sequence0, four actors
(npc.kairos, npc.malet, npc.sable, player.arrel), empty world_flags/quest_states and
empty per-actor location/relationships/emotions/quest_state/flags/memories. Malet
already knows `fact.veil.exists=true` at updated_revision0. That baseline survives.
Knowledge creation advances revision/sequence to1, emits knowledge.learned with
payload `{value:true}` after committed state; memory creation advances both to2,
emits memory.added with status/fact_ids/source_actor_id after committed state.
The final state has only the source-required additional route fact and one memory;
all baseline actors/bags/fact.veil.exists remain exact.

Each world event has exactly schema_version1, event_id `world.%08d`, event_type,
event_sequence, revision, actor_id, target_id, payload. State mutates before sequence
allocation/event emission. Source has no persistent event log; observation records
are read-only evidence. No seed lookup/no-op allocates revision or sequence.

| Source fixture | Before -> after revision | New world events |
|---|---|---|
| fresh / already_true / repeat first call | 0 -> 2 | knowledge.learned, memory.added |
| knowledge_only | 1 -> 2 | memory.added |
| memory_only | 1 -> 2 | knowledge.learned |
| both | 2 -> 2 | none |
| removed | 2 -> 3 | knowledge.learned; tombstone unchanged |
| forgotten | 2 -> 3 | memory.added; false fact unchanged |
| actor_missing | 0 -> 0 | none; actual missing actor fixture |
| save_restore | 2 -> 2 | none; source normalized roundtrip |
| restored | 3 -> 4 | knowledge.learned; restored history unchanged |
| flag_false / case_sensitive uppercase flag only | 0 -> 0 | none |

All13 immediately repeat the seed with zero additional events/revisions/records.
The seed tests knowledge dictionary presence, so false is not re-learned. Any
existing memory record, including removed or restored, prevents add/restore. In
removed/restored fixtures content and historical revisions remain byte-equivalent
as normalized JSON. Missing actor or false prerequisite returns quietly; successful
world mutations emit only the two exact source event types. Seed-only oracle has
no accepted error allowance or diagnostic suppression.

## Minimal native architecture, save and lifetime

`UMemoriaRunSubsystem` owns a separate GC-tracked `UMemoriaWorldCognition` UObject.
Its native value DTOs are FMemoriaWorldSnapshot, FMemoriaActorCognition,
FMemoriaWorldKnowledge and FMemoriaWorldMemory. World revision/event sequence are
persistent int64 state, independent counters (also tested with starting revision7 /
sequence3). IDs use explicit case-sensitive FString comparisons; no domain FName.
The currently attested four-actor catalog is pinned in the minimal native identity
validator. Dynamic actor catalog import/simulation is deferred.

World cognition has only LearnFact, AddMemory and the source-specific seed command.
It does not include or call PlayerMemoryDomain, burn/cascade/residue/erosion, inventory,
shop, narrative, timers, scene actors or profile. Tombstone/forgotten/restored records
enter via validated supplemental native snapshots and are preserved; general remove,
restore and forget command implementations are not claimed.

A map handoff retains the same run ID, Player Memory domain and World Cognition
object. Run replacement constructs a new world candidate, validates it before any
player/run replacement, then atomically installs it and broadcasts the existing
OnRunReplaced cancellation. Old committed cognition is never rolled back; old
retained objects cannot write into the new owner. Actor/world teardown does not erase
persistent cognition. Existing native OpenLevel cancellation tests verify this after
seed as well as before flag, including no delayed stale effects. Source's state-only
reset can leave listeners alive; UE retains the already approved generation/owner
cancellation policy. Reentrant world mutation/restore during event dispatch is
rejected, matching the target's command observation discipline; observers are not a
new source simulation framework.

The existing UMemoriaRunSaveGame schema1 `WorldCognition` section already had
SchemaVersion1 and SourceJson. It now encodes typed cognition there. No schema bump,
new opaque save section or player-memory merge. Empty legacy UE section (A-J) restores
source defaults. Native strict decode validates the entire candidate; unsupported
world data fails before run/player/world replacement. This is NOT the future permissive
Godot legacy-save importer: its malformed-record normalization, legacy Malet alias
addition and arbitrary actor-catalog migration remain deferred. Native missing-actor snapshots preserve that absence exactly; source legacy import would reintroduce missing catalog actors. The missing-actor oracle deliberately erases the actor after reset, rather than misusing normalization. JSON dictionaries
for existing source extension bags/content roundtrip independently; runtime authority
is the typed state. Native JSON integers are validated within exact JSON numeric range.

Production Run CaptureSave/RestoreSave and the existing Narrative save preparation
both use that boundary. Tests perform actual UGameplayStatics::SaveGameToMemory ->
LoadGameFromMemory -> restore, comparing full run, full player and full world sections
independently, then prove reseeding is a no-op. Empty old section, malformed world
rejection and atomic state preservation are covered. No disk slot writer/autosave
or Field-cursor save-resume is introduced.

## Canonical rendered PIE, synchronous observations and hard stop

Same actual route: L_Ch2VerdanSlice -> paid VN1 -> food burn -> native OpenLevel ->
physical movement to Malet -> E/reaction3 -> normal encounter/Accept0 -> actual sword
burn -> source0.3s -> deal0..4 -> source0.5s -> reward0..7 -> done flag -> real run-owned
world seed -> before potion2. Canonical has no teleport, direct seed-only substitute,
manual world construction, reinitialization, fake revision or test-owned authority.

```text
callback:reward:enter
flag:ch2_malet_done
worldseed:enter
world:knowledge:npc.malet:fact.bl07.route_request_received
world:revision:1
world:memory:npc.malet:memory.malet.bl07_request_source
world:revision:2
worldseed:end
development:deferred:before:item:potion:2
STOP
```

Run snapshot differs from reward entry only by the previously authorized done flag.
Player full snapshot is identical before/after every world mutation, with ordered
BurnedHistory [daily_market_food, identity_first_sword], sword residue, campfire
cascade erosion4, all faded/connection/effective-power/passive data retained.
HP100, Grains0, items/recent items and currentChapter1 development metadata remain.
No World Memory record is inserted into player-card ownership or history.

Required machine boundaries are exact synchronous snapshots; after_knowledge and
after_memory are recorded after the authoritative event's state/sequence commit.
Rendered MaletDone_Set, WorldKnowledge_Seeded and WorldMemory_Seeded show recorded
read-only snapshots AFTER gameplay has already synchronously stopped. They explicitly
say RECORDED SYNCHRONOUS SNAPSHOT / READ ONLY. PotionReward_Deferred displays live
final authority. PresentSeedObservation selects only display data and increments UI
revision, never world revision or gameplay timing. No invented wait/frame boundary
splits source effects. Holding/retrying input cannot move the pawn or continue grants.

## Validation and protection

All acceptance PASS. Exact prior107 + new18 = **125/125** identities:13 WorldSeed.Source
cases, NativeContract, IdentityContract, SaveRoundTrip, RunReplacementIsolation and
WorldSeed.Canonical. Canonical tests also prove full player/inventory equality,
Run/domain identity, exact intermediate/final world state, order and pre-potion stop.
Historical UE test identities retain names while their completed reward frontier now
extends through the authorized seed. Existing I/J flag/callback, H payment, G refusal,
F reaction and previous source contracts remain required and immutable.

| Validation | Final result and evidence |
|---|---|
| UBT/UHT/compile/link, UE5.8.2 Editor/rendered PIE | **PASS;125/125**, [automation04](evidence/phase1k/automation04/unreal_validation.json), [exact test index](evidence/phase1k/automation04/automation_index.json) |
| Independent source/state/order/identity comparison | **437/437**,399 parsed JSON snapshots; [acceptance](evidence/phase1k/acceptance01/acceptance.json), [full trace](evidence/phase1k/acceptance01/canonical_full_trace.txt) |
| New18 focused native/actual PIE tests | **18/18**, [focused02](evidence/phase1k/focused02/validation.json) |
| Source K/J/I/H/G/F/E/D/C | **13/12/8/9/7/7/4/10/7**, [fresh suite](evidence/phase1k/regressions_final.json) |
| Official Godot repo/VN/KO/editor and memory/world | **PASS;15/15**, [report](evidence/phase1k/godot01/godot_baseline.json) |
| Native Player Memory and CTest | **51/51 +1/1**, [report](evidence/phase1k/native01/native_memory_validation.json) |
| Host tooling | **76/76**, [host02](evidence/phase1k/host02/host.json) |
| Static / original preservation | **60/60;4217/4217**, [static](evidence/phase1k/static02/foundation_static.json) |
| Existing content protection | **21/21 package hashes;80/80 prior IR/fixture files**, [proof](evidence/phase1k/content_protection.json); no new narrative IR/package |

The final run produced783 fresh files (399 JSON +384 PNG), filtered by its actual
start time and individually hashed in [provenance](evidence/phase1k/automation04/capture_provenance.json).
The existing r.MotionVectorSimulation warning remains once in
Memoria.Campaign.CanonicalPaidRoute; all new18 have zero warnings/errors in the
accepted full run. No suppression or engine setting change. All accepted processes
exit0; nonzero/failed/interrupted attempts remain clearly separate. [Full raw logs](evidence/phase1k/full_logs.zip)
are byte-exact verified against their [SHA inventory](evidence/phase1k/full_logs_inventory.json).
Original checkout HEAD remains15df72809baa52eec281d8508f495a373e9f5884, with its pre-existing
SESSION_LOG/project.godot/docs changes untouched. Official validation restored temporary import metadata.

Required machine evidence: [before_world_seed](evidence/phase1k/acceptance01/before_world_seed.json),
[after_knowledge](evidence/phase1k/acceptance01/after_knowledge.json),
[after_memory](evidence/phase1k/acceptance01/after_memory.json),
[world_seed_complete](evidence/phase1k/acceptance01/world_seed_complete.json),
[pre_potion_deferred](evidence/phase1k/acceptance01/pre_potion_deferred.json),
[world_save_roundtrip](evidence/phase1k/acceptance01/world_save_roundtrip.json).
Directly inspected original1286x760 rendered captures:
[MaletDone_Set](evidence/phase1k/automation04/Phase1K/WorldSeedCanonical_MaletDone_Set.png),
[WorldKnowledge_Seeded](evidence/phase1k/automation04/Phase1K/WorldSeedCanonical_WorldKnowledge_Seeded.png),
[WorldMemory_Seeded](evidence/phase1k/automation04/Phase1K/WorldSeedCanonical_WorldMemory_Seeded.png),
[PotionReward_Deferred](evidence/phase1k/automation04/Phase1K/WorldSeedCanonical_PotionReward_Deferred.png).
[Visual review](evidence/phase1k/visual_review.json). Native rendering was not edited.

## Attempts, source defects and remaining limits

- First UE build: native save JSON adapter lacked the runtime module's private Json
  link dependency. Added exactly that dependency after backing up Memoria.Build.cs;
  rollback removes that line. UHT/compilation had completed; failed full log retained.
- Initial I source regression during concurrent full compile hit its existing real
  0.3s timing assertion (272836us at start_frame18056us), then timeout. No source,
  golden fixture or tolerance is changed; rerun at lower load with fresh evidence.
- Source repeated FULL reward remains non-idempotent (double items and duplicate
  shop signal ERROR), freshly characterized by unchanged J12. Seed itself is a no-op
  on repeat. SOURCE DEFECT / FUTURE DECISION; no Godot defect is fixed here.
- Focused negative DTO test initially added an element from its own TArray; UE rejects this even for a deliberate duplicate. Fixed by copying to a separate value before Add. The production canonical route had passed before this test assertion.
- Explicit derived Player Memory observations were added because default UStruct JSON omits Transient connections. Effective burn power, definitions, connections and carry values are now independently compared; player domain code remains unchanged.
- First focused run exposed integer-only catalog canonicalization of fractional carry overload as INVALID_NUMBER. The evidence observer now records the actual float64 with 17 significant digits in a string. Runtime value is unchanged. The prematurely started full rerun was explicitly stopped; all failed logs/captures remain.
- Only phase1k raw-log ignore and world-seed fixture LF rule are appended, with
  config bytes backed up under Saved/Validation. Full raw logs retained in ZIP.
- Full World Memory/Malet/Verdan parity is not claimed. General cognition commands,
  actor importer, permissive Godot save import, Field resume, final presentation and
  Korean typography, physical USB certification and chapter metadata debt remain.

## Exact Phase1L recommendation (not implemented)

Separately authorize ONLY the next source `GameManager.add_item("potion", 2)` call,
including its validated inventory count, recent-items order, inventory_changed and
toast observation semantics through the smallest existing run-owned inventory API.
Re-characterize the exact source function and edge/save/lifetime cases first; retain
all125 identities and world seed idempotency/player-world separation. STOP before
`GameManager.add_item("antidote", 1)`. No antidote/firebomb/shop/chapter/autosave or
profile/achievement continuation is implied. No Phase1L implementation here.
