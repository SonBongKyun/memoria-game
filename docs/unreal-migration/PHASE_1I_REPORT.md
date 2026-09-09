# Phase 1I — Malet reward dialogue and pre-effect callback boundary

Status: **COMPLETE**. All bounded Phase1I acceptance passed on 2026-09-09.
Worktree: `C:\Users\jc\MemoriaMigration\foundation`.
Branch: `unreal-migration/ue58-foundation`.
Engine: **UE5.8.2 / CL56702186**, `C:\Program Files\Epic Games\UE_5.8`.
Previous accepted technical checkpoint: `5010a26ca934047d27160af3672a93a0caaf2e14`.
Clean starting documentation HEAD: `bf75dad` (full SHA in [preflight](evidence/phase1i/preflight.json)).
New checkpoint is the enclosing local `feat(unreal): add Malet reward dialogue boundary` commit;
resolve its exact SHA with `git log -1 --format=%H -- docs/unreal-migration/PHASE_1I_REPORT.md`.
The final task response records the SHA after committing. **No push. No Phase1J implementation.**

## Accepted scope and content

Only `malet_reward` joins the production narrative cohort. Original group position3,
eight rows, originals0..7, no row effects, gates or choices. All English/Korean,
speaker, presentation metadata, presence bits, order and provenance are mechanically
extracted from `data/chapter2_dialogue.json` at source revision
`4f772fafb1c7c6da68f56f44d60a7c7fed7f3949`.

| Identity | SHA-256 |
|---|---|
| Source raw file | `76b75afef0d2cbcd9747e83884defee806046dce6947207f885aa4bf6913c691` |
| Source UTF-8/LF | `67ec5b7c5f455f41ba997fd6c84220f85f8f87f7f111ef9b3e28c8ead935ac21` |
| Exact reviewed IR | `8c3290ec1665756e0e98ba3e5046739fe324e3b485cfc9c84d24eed98bb2fa55` |
| Typed semantic fingerprint | `e331828fa67026613b77b6590af9f8cfc8a6e60c26aa96e77aa97804be831b59` |
| Saved reward package | `715e708fe21c2ca030a5f04ecb86ab71475022154e2ce58a8de4afa8b8ea283f` |

IR: [malet_reward.field.v1.json](ir/narrative/malet_reward.field.v1.json).
Typed asset: `/Game/Memoria/Generated/Narrative/DA_Field_MaletReward.DA_Field_MaletReward`,
existing `UMemoriaFieldAsset`. [Per-row content/provenance inventory](evidence/phase1i/content_inventory.json).

The source -> reviewed/versioned IR -> strict validation -> existing editor importer
pipeline is unchanged in architecture. [Three fresh processes](evidence/phase1i/import01/pipeline.json):
**CREATED/saved=true**, **UNCHANGED/saved=false**, **check-only UNCHANGED/saved=false**,
identical package hashes. Temporary modified text changes semantic fingerprint and
fails production attestation; wrong group position/count/index and unreviewed
followup fail strict validation. Prior IR, fixtures and20 accepted UE packages remain exact.
No authored narrative text was copied into runtime C++ or Blueprint.

## Exact playable route and completion order

The existing rendered PIE/Enhanced Input replay starts `L_Ch2VerdanSlice`, chooses
paid VN original1, actually burns food, travels natively, walks to the Malet actor,
presses E for the three-row reaction, presses E again for normal encounter,
selects Accept0, burns the actual sword once, waits the real0.3s callback, executes
deal originals0..4, waits the separate real0.5s callback, then enters reward0..7.
No teleport, direct interpreter invocation, manual canonical memory mutation,
fixture reinitialization, run reconstruction or replacement occurs on this route.

Reward-entry/completion suffix:

```text
field:end                         # malet_deal
state:exploration
callback:deal:enter
delay:scheduled:500
delay:elapsed:500
callback:reward:connect
request:res://data/chapter2_dialogue.json::malet_reward
field:start:malet_reward
field:visit:0 / field:gate:0:pass / field:effects:0 / field:line:0:<source text>
[the same exact source phases for originals1,2,3,4,5,6,7]
field:visit:8
field:end
state:exploration
callback:reward:enter
development:deferred:_on_reward_ended:before:ch2_malet_done
STOP
```

`effects:N` is interpreter phase telemetry; the reward rows contain no effects.
[Full canonical trace](evidence/phase1i/acceptance01/canonical_full_trace.txt) and
[exact normalized normal/payment/deal/reward trace](evidence/phase1i/acceptance01/reward_exact_trace.txt)
include all text and visits. The independent comparer checks193 conditions and
all133 capture JSONs parse. The8 row sequence and both locales also match the
executed source oracle in the new language test.

Source `DialogueManager.end_dialogue` clears active/rows/index/choices, changes
GameManager to EXPLORATION, then synchronously emits `dialogue_ended`.
The reward listener was connected one-shot after the0.5s delay. There is **no
reward-completion timer or asynchronous exploration frame**. The oracle's earlier
observer sees inactive Field, index0, exploration and no reward effect; the
source reward listener then mutates state in the same call stack and is removed
after emission. The normal map listener was disconnected at the accepted normal
completion; its NPC first-talk listener had already been consumed.

Unreal records the source exploration transition, then immediately records the
reward callback intent and enters an explicitly development-only `Deferred` state.
The existing UMG/modal remains active, input cannot resume effects or move the pawn,
and Back cannot escape into additional content. It never calls the gameplay
reward handler and never applies an effect followed by rollback. The completed
boundary retains only weak world ownership for cleanup; no executable continuation
or invented timer is queued. This development stop is not claimed as source gameplay.

## Executable source characterization

[Oracle04 attestation](evidence/phase1i/oracle04/source_attestation.json) records exact
callback bodies and source hashes. The full original `_on_reward_ended` body is:

```gdscript
func _on_reward_ended() -> void:
    GameManager.set_flag("ch2_malet_done")
    _seed_malet_memory_world_state_if_needed()
    GameManager.add_item("potion", 2)
    GameManager.add_item("antidote", 1)
    GameManager.add_item("firebomb", 1)
    _open_malet_shop()
```

The attestation preserves original comments/indentation; the listing above omits
comments only. **First authoritative effect: `GameManager.set_flag("ch2_malet_done")`**,
immediately on callback entry, before any seed/item/shop request, with no timer.

The seed method first checks the flag and actual actor record. If the route
knowledge key is missing it calls real `MemoryEngine.learn_fact`; if the route
memory record is missing it calls real `MemoryEngine.add_memory`. For a fresh
WorldState this commits knowledge then memory, revisions1 and2. Existing removed
memory and explicit forgotten knowledge remain untouched on reseeding. Actual
WorldState/ActorRegistry/MemoryEngine/EventBus are copied verbatim into the isolated
oracle. Player MemoryManager remains separate.

Real `GameManager.add_item` validates the item, increments inventory, updates recent
items, emits inventory_changed and calls toast presentation, in potion2 -> antidote1
-> firebomb1 order. Original shop-stock construction and shop open/close state
functions execute in the oracle. `open_shop` sets is_open, merchant/stock/sell mode,
changes to MENU, calls presentation and `TutorialHints.show_hint("first_shop")`;
the map then connects `_on_shop_closed` one-shot. There is no Chapter3/autosave/
achievement call just from reward completion or opening the shop.

Only after shop close does the original `_on_shop_closed` execute:

```text
shop close -> exploration -> shop_closed.emit
_on_shop_closed entry
set_flag ch2_complete
current_chapter = 3
SaveManager.autosave_on_chapter_transition() -> autosave("chapter_transition")
AchievementManager.record_chapter_complete(2)
AchievementManager.unlock("merchant")
source create_timer(1.5)
SceneTransition.change_scene_chapter_complete(belt_waystation.tscn, 2)
```

The source achievement implementation routes record_chapter_complete to the matching
chapter unlock, and unlock performs profile/popup/Steam-placeholder/save calls when
not already unlocked. The oracle records chapter/merchant invocation boundaries;
profile writes, autosave storage, visual endpoints and actual scene travel are inert
sinks. It does not certify production profile, shop presentation or chapter-travel
orchestration. These downstream observations do **not** authorize Unreal execution.

Eight deterministic source cases: English, Korean, state replacement during row3,
state replacement from an earlier completion listener before reward mutation, owner
teardown at those two boundaries, preseeded removed/forgotten world state, and full
shop-close/1.5s transition request. [Inputs](fixtures/malet_reward/contract_inputs.v1.json),
[executed snapshots](fixtures/malet_reward/contract_expected.v1.json).
Oracle04 and fresh import01 oracle agree byte-for-byte. Prior source H9/G7/F7/E4/D10/C7
fixtures were checked in fresh processes without edits.

## Unchanged downstream and live memory evidence

[Before reward](evidence/phase1i/automation01/Phase1I/CanonicalReward_before_reward.json),
[completion](evidence/phase1i/automation01/Phase1I/CanonicalReward_reward_completion.json),
[stable pre-effect stop](evidence/phase1i/automation01/Phase1I/CanonicalReward_reward_effects_deferred.json).

- `ch2_malet_done` remains absent, rather than adding a false entry.
- Full run/player/items/recent-items/flags/chapter and full memory DTOs remain exact
  from reward entry through all8 rows, completion and repeated stop input.
- Food and sword remain burned; history is exactly `[daily_market_food, identity_first_sword]`.
  Sword occurs once, retains source Elia residue, and no memory reset/reconstruction occurs.
- Accepted and heard flags, first-talk flag, HP100/Grains0, all unrelated memory and
  earlier food cascade erosion remain. Same actual Run ID and domain object survive
  the complete canonical VN/native travel/physical approach/payment/reward path.
- No potion, antidote, firebomb or unrelated grant; no shop open/transaction; no Chapter3
  change/request; no autosave/achievement invocation. Reward Field invocation1,
  total Field invocations4 (reaction/normal/deal/reward), reaction1, completion1,
  callback intent1, no pending callback/timers at the final stop.
- World cognition, shop, autosave and achievement orchestration owners have not been
  implemented in Unreal. Evidence explicitly uses `null` for unavailable world/shop
  snapshots, not fabricated empty records. Observed downstream event counters are0;
  those counters alone are not proof of unimplemented systems. Exact run/inventory
  preservation, source-to-runtime trace cut and the static no-downstream-call check
  jointly establish nonexecution. No new world owner or shadow inventory was added.

## Timers, lifecycle and regression continuity

Actual canonical UE world-clock callbacks: **300000us and500000us** (integer telemetry).
The existing1/60 simulation-step harness, lower/upper timer bounds and source0.3/0.5
awaits remain unchanged. No setting was changed to suppress MotionVectorSimulation.

Source state-only replacement does not cancel the owner's one-shot reward listener;
its completion can mutate the replacement state. Owner destruction before emission,
or from the earlier completion observer, cancels it. Unreal intentionally retains
the accepted `OnRunReplaced` cancellation contract: pending Field/callback/timers
cannot affect the replacement run. Actual OpenLevel destroys the world owner and
also disposes the active Field or completed boundary. There is no externally
asynchronous UE gap between completion and callback intent; supplemental replacement
at the stopped boundary is after synchronous intent but before any gameplay effect.
It does not pretend a new source timer or production New Game exists.

All original **90 exact Automation identities** remain. New **7**:

- `Memoria.MaletReward.ImportContract`
- `Memoria.MaletReward.EnglishKoreanExecution`
- `Memoria.MaletReward.CanonicalReward`
- `Memoria.MaletReward.CancelRewardFieldOnRunReplace`
- `Memoria.MaletReward.CancelRewardFieldOnWorldTeardown`
- `Memoria.MaletReward.CancelRewardBoundaryOnRunReplace`
- `Memoria.MaletReward.CancelRewardBoundaryOnWorldTeardown`

Historical `AcceptPreEffectDeferred` keeps its name and now verifies the fully
authorized payment/deal/reward chain and **reward callback pre-effect** boundary.
`CanonicalPayment` and `AlreadyBurnedPayment` also extend to this boundary. Their
exact H source comparisons through deal completion, real payment/failure semantics
and both old timers remain, then reward assertions are added. The4 old normal/reward
delay lifetime tests, exact Refuse cleanup/retry and reaction-priority regressions
remain unchanged in intent. Host tests now allow exactly reward8 and still reject
followup groups. The old `reject_downstream` deal fixture remains malformed when
reinterpreted as reward (wrong row count/provenance), and still fails strict import.
These historical names are documented semantic debt; no broad rename occurred.

## Validation and captures

| Verification | Result / evidence |
|---|---|
| UE5.8.2 UBT/UHT/compile/link | PASS, [build01](evidence/phase1i/build01/unreal_validation.json), final automation01 build PASS |
| Editor / rendered PIE / exact Automation | **97/97**, [index](evidence/phase1i/automation01/automation_index.json) |
| Independent source-to-rendered comparison | **193 checks**,133 valid snapshot JSONs, [acceptance](evidence/phase1i/acceptance01/acceptance.json) |
| New reward source oracle | **8/8**, oracle04 and import01 deterministic check |
| Retained source H/G/F/E/D/C | **9/7/7/4/10/7**, [regression suite](evidence/phase1i/regressions01/suite.json) |
| Official Godot repository/VN/KO/editor/memory-world | PASS, **15/15** memory/world, [baseline](evidence/phase1i/godot01/godot_baseline.json) |
| Native memory / CTest | **51/51 +1/1**, [native](evidence/phase1i/native01/native_memory_validation.json) |
| Host validators | **61/61**, [host02](evidence/phase1i/host02/host.json) |
| Static / original files | **59/59;4217/4217**, [static](evidence/phase1i/static01/foundation_static.json) |
| Existing UE packages / production source | **20/20 unchanged; only reward package added**, [protection](evidence/phase1i/protection.json) |

All processes exited successfully and required fatal-log checks passed. Exactly one
known `r.MotionVectorSimulation` warning remains in `Memoria.Campaign.CanonicalPaidRoute`;
all new7 tests have zero warnings/errors. Godot's known legacy-grade/metadata noise
is retained; the official harness restores import metadata. Original Godot checkout
HEAD/status and all protected bytes remain unchanged. No remote operations occurred.

All five original1286x760 PNGs were directly inspected. No image transformation.
The image viewer itself hit the same sandbox ACL startup error as shell tools;
read-only original PNG byte transfer enabled inspection without altering evidence.
[Visual review](evidence/phase1i/visual_review.json).

| Required capture | Native evidence |
|---|---|
| Reward_FirstLine | [original0 /1of8](evidence/phase1i/automation01/Phase1I/CanonicalReward_Reward_FirstLine.png) |
| Reward_MiddleLine | [original3 /4of8](evidence/phase1i/automation01/Phase1I/CanonicalReward_Reward_MiddleLine.png) |
| Reward_LastLine | [original7 /8of8](evidence/phase1i/automation01/Phase1I/CanonicalReward_Reward_LastLine.png) |
| Reward_Completion | [completion stop](evidence/phase1i/automation01/Phase1I/CanonicalReward_Reward_Completion.png) |
| RewardEffects_Deferred | [stable pre-effect stop](evidence/phase1i/automation01/Phase1I/CanonicalReward_RewardEffects_Deferred.png) |

## Defects and preserved attempts

- Oracle01: appended transition observer mixed space/tab indentation. Corrected only
  the isolated observer generator. Failed import log retained.
- Oracle02: immediate heavy fixture initialization distorted the wall/frame timing
  probe. Added the same frame-settling awaits used by H; source timers/assertions
  remained unchanged. Failed execution/timeout retained.
- Oracle03: source shop open exposed the missing localized_speaker presentation
  helper in the isolated GameManager, plus a timing assertion during erroneous
  execution. Source helper/dictionary are now extracted, preserving authored bytes.
  Oracle04 and fresh deterministic import check pass8/8.
- Host01: the old Phase1F tool test still rejected newly authorized reward content.
  Kept its identity, asserted reward8 and rejected the unreviewed followup; host02
  passes61/61. UE assertions were extended without discarding their prior intent.
- A postvalidation metadata-report read used UTF-8 against UE's BOM-bearing index;
  reading with UTF-8-sig resolved it. It did not affect the actual validator,
  which already accepts the report encoding, or any gameplay/test result.
- Static validation was started while the official Godot harness was processing
  import metadata. It returned59/59; final source/package/status checks were run
  after the official harness completed and restored metadata. No cleanup/reset used.

- Final staged whitespace check found CRLF in three raw UE import logs. Extended
  the existing phase-scoped raw-log ignore rule; original bytes remain in the
  verified ZIP. Added the matching reward-fixture LF attribute for deterministic
  Windows checkouts. Prior .gitignore/.gitattributes bytes are backed up under
  Saved/Validation; rollback is removing only these two additive rules.

[Full logs](evidence/phase1i/full_logs.zip) and [SHA inventory](evidence/phase1i/full_logs_inventory.json)
retain successful and failed attempts, including UBT/UHT/link and Automation output.

## Limits and exact Phase1J recommendation

This completes **only reward originals0..7 through the pre-effect `_on_reward_ended`
boundary**, not full Malet/Verdan parity. No world seeding, item reward, Memory Shop,
trade, Chapter3, autosave orchestration, achievement, world followup dialogue, extra
NPC, battle/random encounter, production New Game/Chapter1, final UI/art/audio,
graphics modernization, cook/package or Steam work was implemented. Metadata and
Korean text preservation do not certify final CG/portrait rendering or Korean font
presentation. Field save-resume and physical USB-device coverage remain uncertified.

Recommend the next separately authorized **Phase1J** be bounded to the first source
reward effect: continue this same completed reward run, execute only
`GameManager.set_flag("ch2_malet_done")` through the existing authoritative run API,
then stop **before `_seed_malet_memory_world_state_if_needed` can make its first
world-memory mutation**. Recharacterize fresh/preexisting flags and owner lifetime,
retain97 identities and all payment/refusal/reaction contracts. World seeding,
items/shop/Chapter3/autosave/achievements remain deferred. This is a recommendation,
not authorization, and no Phase1J runtime code exists in this checkpoint.

## Reproduction

Run from the migration worktree with fresh evidence directories:

```powershell
python Unreal/Tools/export_malet_reward_oracle.py --godot <Godot-4.6.2-console.exe> --check --evidence-dir <fresh>/oracle
python Unreal/Tools/malet_reward_test_fixtures.py --check
python Unreal/Tools/import_narrative.py --group malet_reward --engine-root 'C:\Program Files\Epic Games\UE_5.8' --godot <Godot-4.6.2-console.exe> --evidence-dir <fresh>/import
python Unreal/Tools/validate_unreal.py --engine-root 'C:\Program Files\Epic Games\UE_5.8' --build-and-test --rendered --evidence-dir <fresh>/automation
python Unreal/Tools/validate_malet_reward_evidence.py --automation-dir <fresh>/automation --evidence-dir <fresh>/acceptance
```

For the independent comparer, copy that run's reported Automation index to
`automation_index.json` and its native `Saved/Validation/Phase1I` captures into the
fresh Automation evidence directory after process completion. Never substitute a
previous run's capture directory or index. The accepted automation01 copy and its
193-check comparison are retained here.
