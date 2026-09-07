# Phase 1F — first Malet memory reaction

Status: **COMPLETE — bounded development interaction**, UE **5.8.2 / CL56702186**.
Validated 2026-09-07. This does not establish full Malet or Verdan parity.

- Branch: `unreal-migration/ue58-foundation`.
- Worktree: `C:\Users\jc\MemoriaMigration\foundation`.
- Previous technical checkpoint: `e9f781cf3f73ba5949ab9a2af961193d02d3c620`.
- Clean starting checkpoint: `d53aaa84efa1a42c0e2078821e10520a998f58cd`.
- New technical checkpoint: `PENDING_LOCAL_COMMIT`; recorded by the following documentation commit. No push.
- [Acceptance](evidence/phase1f/acceptance.json), [final exact-name Automation](evidence/phase1f/automation05/automation_index.json), [build](evidence/phase1f/automation05/unreal_validation.json).

## Source and deterministic content

Only `malet_taste_burned` from `data/chapter2_dialogue.json` was added.
It is authored group position **16**, exactly **three rows, originals 0–2**.
English, Korean, speaker spelling, order and indices are extracted mechanically.
No authored dialogue was typed into runtime C++ or Blueprint.

| Artifact | SHA-256 |
|---|---|
| Source JSON, UTF-8 normalized LF | `67ec5b7c5f455f41ba997fd6c84220f85f8f87f7f111ef9b3e28c8ead935ac21` |
| Source JSON, raw worktree bytes | `76b75afef0d2cbcd9747e83884defee806046dce6947207f885aa4bf6913c691` |
| [Versioned Field IR](ir/narrative/malet_taste_burned.field.v1.json) | `368e04339e3ff4e59c4cce4d26358683f9819f114afa76c6902b570b533d8645` |
| Semantic fingerprint | `200d1e6c65b72abd8f86f6ce27ddc5797c9968041f5edf4e2e04d5c44142a418` |

Attested source revision: `4f772fafb1c7c6da68f56f44d60a7c7fed7f3949`.
The IR records every dependency; [NPC oracle attestation](evidence/phase1f/oracle01/source_attestation.json)
also records NPC method bytes, full PerceptionFilter source, source scene configuration,
and the exact Verdan entry guard. The oracle report also attests the memory manager.

Typed asset: `/Game/Memoria/Generated/Narrative/DA_Field_MaletTasteBurned`
(`UMemoriaFieldAsset`), stored at
`Unreal/Memoria/Content/Memoria/Generated/Narrative/DA_Field_MaletTasteBurned.uasset`.

The additive reviewed Field cohort reuses schema/extractor v1 and the existing
strict codec/import commandlet. Default extraction/import retains the old two
Phase1D groups; explicit `--group malet_taste_burned` selects only this group.
[Import pipeline](evidence/phase1f/import01/pipeline.json): first **CREATED**,
second fresh process **UNCHANGED / saved=false**, third check-only fresh process
**UNCHANGED / saved=false**. IR, semantic and package hashes agree.
A transient semantic text change has a different fingerprint and is rejected by
production source attestation. Wrong group position, count and unknown group
are rejected without saving packages. Temporary fixtures remain offline tests.

Only three narrative packages exist: accepted VN arrival, accepted Field arrival,
and this reaction. No normal Malet group is extracted/imported. Old IR/assets
remain exact; the [six-process old pipeline](evidence/phase1f/regressions/narrative01/pipeline.json)
returned UNCHANGED/no save in every process.

## Source-authentic dispatch

Inspected `npc.gd::interact`, its first-talk callback and runtime name/line helpers,
complete `PerceptionFilter.take_burn_reaction`, `MemoryManager.is_memory_burned`,
Malet's scene exports and reaction metadata, all three selected authored rows,
and Verdan's dialogue-ended routing.

Source priority: active-dialogue guard → memory reaction resolver → normal
first/repeat talk routing. The resolver verifies the actual reaction group exists
before setting `burn_reaction_heard_malet_taste_burned`, then returns the request.
It never enters the normal encounter first.

Unreal checks the existing domain's **BurnedHistory** for `daily_market_food`,
case-sensitively. Absence from intact memories is insufficient: a separate
faded-but-unburned case proves it does not trigger this reaction. Canonical
PIE obtains the burn exclusively from original VN choice **1**, using the accepted
imported starting-memory catalog. No Boolean substitute, second authority or
interaction-time burn call exists.

The resolver sets the real run flag before returning the reaction; the host
records its live value immediately before existing Field interpreter `Start`.
Tests compare event order and the source oracle's active first-row snapshot,
rather than only checking the final flag.

| Source case | Resolved request | Execution here |
|---|---|---|
| Burned, unheard, fresh talk | `malet_taste_burned` | Three rows once |
| Burned, unheard, already talked | `malet_taste_burned` | Reaction still wins |
| Burned, already heard, fresh talk | `malet_encounter` | Record and defer |
| Intact, unheard, fresh talk | `malet_encounter` | Record and defer |
| Burned, heard, persisted talked flag | `malet_memory_world_followup` | Record and defer |
| Burned, unheard, missing reaction group | `malet_encounter` | No heard consumption; defer |
| Paid source VN → Verdan → interaction | `malet_taste_burned` | Full bounded source trace |

The seven-case [Godot oracle](evidence/phase1f/oracle01/oracle.json) executes the real
PerceptionFilter and verbatim NPC interaction methods in the existing isolated
harness. Actual DialogueManager, MemoryManager and SceneFlow govern execution.
Facing/pixel presentation is inert; the observer records dialogue requests
and deliberately defers normal target execution. The resolver is not mocked.
Source files are copied into ignored isolated fixtures, never edited in place.

The repeat target comes from `repeat_dialogue_key` in the real scene.
The source in-memory talk cache is not a second Unreal runtime authority:
this host never executes normal talk; persisted `talked_Malet_malet_encounter`
is characterized explicitly.

## Runtime NPC, input and canonical trace

One native `AMemoriaMaletActor`, actor name `Malet`, lives in
`/Game/Tests/Campaign/L_VerdanHost.L_VerdanHost:PersistentLevel.Malet`
at **(240, -160, 0)**, with **80-unit** interaction range.
A labeled cube placeholder uses a Pawn/Visibility-blocking box and a visual
mesh without duplicate collision. This presentation actor can later be replaced.

`IMemoriaInteractable` + `UMemoriaInteractionComponent` form the reusable boundary.
The component overlaps nearby WorldDynamic actors, checks eligibility/range/
Visibility occlusion, chooses nearest distance with deterministic name tie-breaking,
stores a weak target and revalidates at Interact. The controller uses accepted
**IA_Confirm Enhanced Input** (E / Space / Enter / gamepad A); it does not identify
Malet or execute dialogue rules directly. The narrative subsystem owns resolution
and the existing Field interpreter. Existing temporary UMG presents the imported
asset; no NPC-specific dialogue widget was added.

Actual canonical replay opens `L_Ch2VerdanSlice`, advances source VN 0–12,
selects original choice1, performs native map travel, walks with physical key
events into range (no teleport), resolves the overlap/interface target and presses E.

Meaningful trace, in source order:

```text
vn:start:ch2_market_arrival
vn:step:0 ... vn:step:10
select:vn:1
vn:burn:daily_market_food:ok
vn:flag:guard_bribed
vn:step:11
vn:flag:ch2_arrival_vn_seen
vn:map:res://scenes/maps/verdan_market.tscn
travel:verdan
verdan:enter
flag:ch2_arrived
field:skip:verdan_arrival
exploration:ready
interact:Malet
resolver:begin:burned=true:heard=false
flag:burn_reaction_heard_malet_taste_burned
resolver:reaction:malet_taste_burned
request:res://data/chapter2_dialogue.json::malet_taste_burned
field:start:malet_taste_burned:heard=true
field:line:0
field:line:1
field:line:2
field:end
exploration:ready
```

The [complete runtime trace and snapshots](evidence/phase1f/automation05/Phase1F/CanonicalInteraction_canonical_complete.json)
retain visits, gate/effect observations and exact authored line text.
Only deterministic interpreter-internal bookkeeping is omitted when comparing
the meaningful event sequence to the executable Godot oracle.

At canonical stop, deferred target is empty, reaction invocations=1,
Field invocations=1, heard=true. Full memory snapshot is unchanged under canonical
serialization from before interaction. Run ID and the actual domain instance
survive native map handoff and interaction.

Field modal and its mapping context are removed, exploration context remains,
viewport owns focus, movement is enabled, Z stays zero and the accepted camera
follows the player. Back does not escape this Field group. Held opening Interact
cannot skip row0; crossing a narrative boundary waits for explicit key release,
independent of processed key state flushed by input-mode/context changes.

After canonical capture, a separately labeled supplemental second-press probe
records `malet_encounter` as deferred, starts no Field and stays in exploration.
This is a development boundary, not production-complete Malet.
The [second-press snapshot](evidence/phase1f/automation05/Phase1F/CanonicalInteraction_second_press_boundary.json)
and [intact-route snapshot](evidence/phase1f/automation05/Phase1F/IntactInteraction_intact_fallback.json)
prove the requested target. No deal/refusal flags, rewards, world seeding,
shop, achievement, autosave or Chapter3 request occurs.

## Validation and rendered evidence

**Previous 72/72 + Phase 1F 4/4 = total 76/76 PASS.**
The validator requires every exact test identity. One pre-existing warning in
the retained campaign test concerns `r.MotionVectorSimulation` render-thread
access: 75 success plus1 success-with-warning, zero failed/missing.
All four Malet tests have no warnings.

| New test | Observable contracts |
|---|---|
| `Memoria.Malet.ImportContract` | Typed source parity, strict validation, no-op/reload, semantic and negative fixtures |
| `Memoria.Malet.SourceDispatch` | Burn authority, priority, pre-Start flag, three rows, heard/intact/talked/missing cases, no re-burn |
| `Memoria.Malet.CanonicalInteraction` | Actual VN/travel/walk/Interact, held-input guard, imported Field, exact source trace, state/focus/context/movement/camera, bounded second press |
| `Memoria.Malet.IntactInteraction` | Actual intact route/Interact, exact fallback, no Field/heard consumption, exploration/memory preservation |

Real **UBT/UHT/compile/link, Editor load, rendered PIE and Automation PASS**
on UE5.8.2. Fresh [automation05](evidence/phase1f/automation05/unreal_validation.json)
has process exit0 and no fatal diagnostics.

| Regression | Result |
|---|---|
| Godot repository, VN, Korean, editor import | PASS; VN21 files/526 steps, Korean32 files/1581 fields |
| Official memory/world | 15/15 PASS; exit0 and full fatal scan |
| Native production memory model | 51/51 PASS; CTest1/1 |
| Phase1C catalog/source | PASS; accepted IR and ordered seven memories unchanged |
| Phase1D narrative pipeline/oracle | Six unchanged UE processes; 10/10 source cases |
| Phase1E route oracle/tests | 4/4 source cases; four old Unreal tests pass |
| Phase1F source oracle | 7/7 |
| Host validator tests | 45/45 |
| New offline fixture check | PASS |
| Final static/source integrity | 59/59; 4,217 protected files |

[Evidence index](evidence/phase1f/acceptance.json).
Godot validation restored 1,056 import metadata files.
Original checkout remains at `15df72809baa52eec281d8508f495a373e9f5884` with its
pre-existing SESSION_LOG/project.godot/docs status unchanged.
All fifteen other accepted UE packages are byte-identical; only the owned Verdan
map changes, plus one new reaction asset. Historical Phase0–1E evidence and
existing source content remain unchanged. [Protection](evidence/phase1f/source_protection.json).

Actual PIE captures, visually inspected without modification:

- [Before interaction: player and placeholder](evidence/phase1f/automation05/Phase1F/CanonicalInteraction_BeforeInteraction.png)
- [Range prompt and Malet label](evidence/phase1f/automation05/Phase1F/CanonicalInteraction_Prompt.png)
- [First real reaction row, 1/3](evidence/phase1f/automation05/Phase1F/CanonicalInteraction_FirstLine.png)
- [Last real reaction row, 3/3](evidence/phase1f/automation05/Phase1F/CanonicalInteraction_LaterLine.png)
- [Exploration after reaction, canonical stop](evidence/phase1f/automation05/Phase1F/CanonicalInteraction_AfterReaction.png)
- [Movement after supplemental fallback probe](evidence/phase1f/automation05/Phase1F/CanonicalInteraction_Moved.png)

Full raw logs from all attempts are preserved in `evidence/phase1f/full_logs.zip`.
[Archive inventory](evidence/phase1f/log_archive.json) records lengths and hashes.
[Changed paths](evidence/phase1f/changed_files.json) enumerate task-owned changes.

## Defects fixed and limits

- Explicit UWorld pointer corrected test compilation with UE5.8 TObjectPtr;
  `automation01` preserves the failed build.
- Held Interact was reinterpreted across modal/context changes and skipped row0.
  IsInputKeyDown polling was insufficient because processed state was flushed.
  Physical press/release tracking at InputKey fixes it. `automation02` and
  `automation03` preserve75/76 failures; the same held-repeat assertion now passes.
- Inherited PIE window could shrink vertically across sequential replays.
  The new test resizes the actual PIE window and verifies height before capture.
  Final captures are1286x760 including window chrome.
- Mirrored placeholder TextRender label was corrected for the top-down camera;
  final before/prompt captures verify it.
- `automation04` failed linking a temporarily locked editor DLL (LNK1104).
  Subsequent inspection found no remaining UnrealEditor process; the same-source
  retry passed. No process was killed or engine configuration changed.

No normal transaction, repeat world-memory group, deal/refusal/reward/shop,
economy/world seeding/Chapter3/autosave/achievement, extra NPC/battle/New Game/Ch1,
final art/UI/audio, graphics modernization or cook/package was implemented.
The oracle reads the larger JSON only to observe source resolver requests.
No full campaign or full Malet/Verdan parity is claimed.

The development80-unit proximity boundary does not claim parity for Godot's
directional32-pixel ray or facing animation. Remapping, external focus loss/device
reconnect, final Korean typography and physical USB controller testing are not
certified. Keyboard/gamepad events run through real Enhanced Input. No new save
schema or production save-resume orchestration is added; previous save tests pass.

## Exact recommended Phase 1G — not implemented

Add only **normal first Malet encounter → explicit refusal → refusal response
→ exploration/retry**. Import `malet_encounter` (10 rows, two original choices at9)
and `malet_refused` (3 rows). Preserve reaction priority, original choice IDs,
first-talk completion, source0.3-second callback ordering and refusal cleanup:
clear `malet_deal_refused`, `talked_Malet_malet_encounter` and source talk cache,
then allow another ordinary encounter. Prove it with source oracle and real PIE.

Keep authored Accept choice visible/intact in data. Source Accept sets
`malet_deal_accepted` and burns `identity_first_sword`. Characterize that complete
selection/subsequent request in the oracle; in development, defer an attempted
Accept **before any partial choice effects** until its chain is authorized.
Do not relabel it as refusal or claim both branches playable. Canonical next-phase
acceptance selects original refusal choice1. Deal5/reward8, world-memory seeding,
items/shop close/Chapter3/autosave/achievements and `malet_memory_world_followup`
remain deferred. This is only a recommendation; no Phase1G implementation exists.

## Reproduction

From migration worktree, use fresh evidence directories:

```powershell
python Unreal/Tools/narrative_ir.py --group malet_taste_burned --check --evidence-dir <fresh>/extract
python Unreal/Tools/export_malet_oracle.py --godot <Godot-4.6.2-console.exe> --check --evidence-dir <fresh>/oracle
python Unreal/Tools/malet_test_fixtures.py --check
python Unreal/Tools/import_narrative.py --group malet_taste_burned --engine-root 'C:\Program Files\Epic Games\UE_5.8' --godot <Godot-4.6.2-console.exe> --evidence-dir <fresh>/import
python Unreal/Tools/validate_unreal.py --engine-root 'C:\Program Files\Epic Games\UE_5.8' --build-and-test --rendered --evidence-dir <fresh>/automation
```

Open `/Game/Tests/Campaign/L_Ch2VerdanSlice` and Play, select the original food
memory payment, walk to Malet, press E and advance three rows.
The committed map already contains Malet. Editor-only
`author_slice_assets.py --add-malet` verifies owned map/GameMode and refuses
duplicate/misplaced actors; runtime never invokes authoring.
