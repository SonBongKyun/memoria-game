# Phase 1G — Malet normal encounter, refusal and retry

Status: **COMPLETE — bounded refusal/retry acceptance PASS.** Final UE5.8.2 build, link, Editor/rendered PIE and exact **82/82 Automation** passed. [Acceptance](evidence/phase1g/acceptance.json), [final Automation](evidence/phase1g/automation02/automation_index.json).

## Checkpoints and scope

- Worktree: `C:\Users\jc\MemoriaMigration\foundation`
- Branch: `unreal-migration/ue58-foundation`
- Previous technical checkpoint: `fea1cdd3c7fc1947e2fa8b55d86f39a21f7a1d24`.
- Clean starting documentation HEAD: `0f37e58318a749a5d8057a5303c4f18e022f864c`.
- New technical checkpoint: `361b9039713446957f1da66470881990388762ba`; its tree contains the completed implementation and acceptance evidence.
- Engine: actual UE **5.8.2 / CL56702186**, `C:\Program Files\Epic Games\UE_5.8`.
- No push. Phase 1H is a recommendation only.

Only `malet_encounter` and `malet_refused` join the production narrative cohort.
All authored English/Korean text, empty/present fields, presentation metadata,
original row indices, original choice IDs, effects and provenance derive from
source-attested IR. No authored dialogue is copied into runtime C++/Blueprint.
The existing Field interpreter, interaction interface/component/NPC, Enhanced
Input controller and temporary development UMG remain the execution path.

## Source and deterministic content

Both groups come from `data/chapter2_dialogue.json` at source revision
`4f772fafb1c7c6da68f56f44d60a7c7fed7f3949`.

- Whole source raw SHA-256: `76b75afef0d2cbcd9747e83884defee806046dce6947207f885aa4bf6913c691`.
- Whole source UTF-8/LF SHA-256: `67ec5b7c5f455f41ba997fd6c84220f85f8f87f7f111ef9b3e28c8ead935ac21`.
- Full source/IR/package inventory: [content_inventory.json](evidence/phase1g/content_inventory.json).

| Group | Source group position | Rows | IR SHA-256 | Semantic SHA-256 |
|---|---:|---:|---|---|
| `malet_encounter` | 1 | 10 | `02e349caca0d1e7e9b4408be601720237d6e571eb272336c28f646dae9b2808d` | `d8447432bd6a587445000460693446975658b3c5a1180f6a15b240eedd53dcaa` |
| `malet_refused` | 4 | 3 | `63a992d15ff900317f9402d72cfa9015fbd49b6f198776d9bd6832fe96d3004a` | `ac515bff0f2276e6b792d6b1b5e4e02423300ee0427b5fea2b01cedb9ced0c8b` |

IR files: [malet_encounter.field.v1.json](ir/narrative/malet_encounter.field.v1.json)
and [malet_refused.field.v1.json](ir/narrative/malet_refused.field.v1.json).

Typed `UMemoriaFieldAsset` objects:

- `/Game/Memoria/Generated/Narrative/DA_Field_MaletEncounter.DA_Field_MaletEncounter`.
- `/Game/Memoria/Generated/Narrative/DA_Field_MaletRefused.DA_Field_MaletRefused`.

Each passed first import **CREATED/saved**, second fresh-process import
**UNCHANGED/not saved**, and third fresh-process **check-only reload** with the
same IR, semantic and package hashes. See [encounter pipeline](evidence/phase1g/import_encounter01/pipeline.json)
and [refused pipeline](evidence/phase1g/import_refused01/pipeline.json).
Temporary changed-text objects change the typed fingerprint, remain transient,
and cannot pass production source attestation. Wrong provenance/index/downstream
group probes are rejected. Runtime has no importer, Python, Godot or loose-JSON
dependency.

## Executable source oracle

[Source attestation](evidence/phase1g/oracle04/source_attestation.json) contains
the exact extracted `npc.gd` first-talk methods and Verdan entry/end/refusal
callbacks. Removing tagged observer lines reproduces original callback bodies.
The real `await get_tree().create_timer(0.3).timeout` remains executable.

Seven cases: heard normal first talk; English refusal/cleanup/retry; Korean
refusal/cleanup/retry; original Accept effects and next request; persisted repeat;
transient-cache repeat; unheard reaction priority even with both talked states.
Fresh checks reproduce the same [golden states and events](fixtures/malet_refusal/contract_expected.v1.json).
Wall-clock timing is measured separately from deterministic golden output.
The first successful run measured 299.940ms, 299.932ms and 299.662ms for its three
actual delayed callbacks; [fresh timing measurements](evidence/phase1g/oracle04/timings.json)
remain attached.

The existing source-derived memory domain is used. The Godot harness creates
isolated starting-memory runs; the canonical Unreal replay obtains its burned
food from the real paid VN choice and native travel.

## Exact ordinary interaction and callback order

The complete, per-row trace is in [the final retry record](evidence/phase1g/automation02/Phase1G/CanonicalRefusalRetry_retry_boundary.json) and [normalized exact trace](evidence/phase1g/retry_exact_trace.txt).
Its normal-interaction suffix is compared byte-for-byte after removing only
existing `field:` event namespaces against the source oracle.

```text
actual paid VN original1 -> native Verdan travel -> walk -> E
burned + unheard -> malet_taste_burned originals0..2 -> exploration
E -> interact:Malet
resolver:begin:burned=true:heard=true -> resolver:none
cache:set:malet_encounter -> callback:npc:connect
request:res://data/chapter2_dialogue.json::malet_encounter
field:start:malet_encounter
for originals0..8: visit -> gate:pass -> effects -> exact localized line
original9: visit -> gate:pass -> effects -> choices:0,1
select:field:1 -> choice:Refuse. -> flag:malet_deal_refused
visit:10 -> end -> state:exploration
callback:normal:disconnect -> delay:scheduled:300
callback:npc:first_talk -> flag:talked_Malet_malet_encounter
[real 0.3-second timer]
delay:elapsed:300 -> callback:refused:connect
request:res://data/chapter2_dialogue.json::malet_refused
field:start:malet_refused
originals0,1,2 in order -> visit:3 -> end -> state:exploration
callback:refused:enter
erase:flag:malet_deal_refused
erase:flag:talked_Malet_malet_encounter
erase:cache:malet_encounter
callback:normal:connect -> exploration:ready
actual movement and camera follow -> ordinary E
normal resolver/cache/callback/request -> malet_encounter original0 -> STOP
```

Choice mapping at source row9:

| Original index / stable ID | Original English label | Authored effects |
|---|---|---|
| 0 / `field/malet_encounter/9/choice/0` | Accept the deal. | `set_flag:malet_deal_accepted` then `burn_memory:identity_first_sword` |
| 1 / `field/malet_encounter/9/choice/1` | Refuse. | `set_flag:malet_deal_refused` |

The refused flag is set by the unchanged Field interpreter before normal end.
The map listener disconnects and schedules its timer before the NPC completion
listener writes the talked flag. The source interval is exploration, not an
instant modal-to-modal transition. Final PIE measured **300,000 microseconds (0.300000 seconds)**. The real world timer is cancelled on run
replacement, subsystem teardown or cleanup of its owning world.

Refused originals: row0 Malet dismisses Arrel; row1 narrates his absolute
dismissal; row2 Elia says “Arrel...”. All three source lines and both localizations
remain verbatim in typed data. At completion, flags are removed as entries rather
than assigned false, then the transient NPC cache is cleared and the ordinary
callback is reconnected. Run ID, memory domain, HP, Grains, items, burned history,
heard-reaction flag and other flags survive. The run returns exactly to its
pre-normal snapshot. Actual Enhanced Input movement, focus, mapping contexts,
Z and camera checks pass before the ordinary retry begins at original0.
No teleport replaces the walk.

## Accept boundary and downstream nonexecution

The Godot source case `accept_source` records:

```text
select:field:0 -> choice:Accept the deal.
flag:malet_deal_accepted -> burn:identity_first_sword:ok
normal end -> exploration -> callback disconnect -> timer schedule
NPC first-talk callback -> talked flag -> actual 0.3-second timeout
callback:deal:connect -> request:...::malet_deal
development:deferred:malet_deal
```

The oracle records that requested group and does not execute it. An isolated
Unreal interpreter contract test independently checks original choice effects;
the playable development host is separately gated before interpreter entry.

Actual PIE original0 input produces only
`development:deferred:malet_encounter:choice:0`. It does not produce an
interpreter selection, choice log, flag, burn, timer or subsequent request.
The entire live run and memory snapshot compare equal before/after the attempt;
the accepted flag is absent, the sword is intact, and both unchanged labels stay
visible at source row9. There is no rollback.
See final `AcceptPreEffectDeferred_accept_pre_effect_deferred.json` and PNG.

Canonical/refusal and Accept-boundary traces contain no deal/reward/repeat
Field, world-memory seeding, item reward, shop/trade, Chapter3 request,
autosave or achievement chain. Production package inventory includes only the
two newly authorized groups. No additional NPC/framework/save schema or
authoritative state store was introduced.

## Tests and evidence

The exact previous **76 Automation identities** remain required. The old
Phase1F canonical reaction is still compared against its untouched oracle before
any ordinary follow-up. Its supplementary next press and the intact-food test
now expect normal original0, since that request is newly authorized here.
This advances only their former development-defer boundary; it does not weaken
the reaction, memory, input, focus, run or movement assertions. There is no
test-only runtime authorization switch.

Six new `Memoria.MaletRefusal.*` tests:

| Identity suffix | Evidence |
|---|---|
| ImportContract | Both complete typed assets, English/Korean/provenance, no-op reload, transient semantic and negative probes |
| ChoiceEffects | Exact original0/1 interpreter event order and source burn history, English/Korean |
| RepeatCache | Unheard priority, transient and persisted repeat, actual flag erasure |
| CanonicalRefusalRetry | Actual paid VN/travel/walk/E, 10-row normal, refusal, real timer, 3-row response, cleanup, move, retry |
| AcceptPreEffectDeferred | Actual original0 input leaves full run/memory unchanged before effects |
| CallbackCancellation | Actual pending refusal callback cannot affect a replacement run |

Final acceptance passed **82/82** exact identities; duplicate/missing-name checks also pass.
All capture JSON is parsed and checked for successful file save inside the replay.
Final timing telemetry uses integer microseconds compatible with the existing
strict canonical serializer.

Required captures under final `automation02/Phase1G`:
`NormalEncounter_FirstLine`, `NormalEncounter_Choices`,
`NormalEncounter_RefusalSelected`, `Refused_FirstLine`, `Refused_LastLine`,
`Exploration_AfterRefusal` and `RetryBoundary`, each prefixed
`CanonicalRefusalRetry_`. The extra Accept deferred capture is retained.
The first attempt's eight required/Accept views were directly inspected:
readable original labels/text, selected refusal, restored exploration and normal
retry. All seven final views plus Accept were directly inspected after rerun; all capture JSON parses and saves. Final canonical images are 1286x760 pixels.

Regression evidence under `evidence/phase1g/regressions`:

- Phase1F oracle7 and its three-process import pipeline.
- Phase1E route oracle4 and four original Unreal campaign tests.
- Phase1D oracle10, six-process narrative import and eight original Unreal tests.
- Phase1C executable ordered catalog/source check and three original Unreal tests.
- Official Godot repo/VN/KO/import and memory-world15, isolated user profiles.
- Native memory51 plus CTest1.
- Host tooling **50/50**; static structure/original-file checks **59/59**.
- Original **4217/4217** files and all **17/17** accepted UE packages byte-preserved; exactly two new packages. All historical reports/evidence and Godot source remain unchanged.
- Existing `r.MotionVectorSimulation` render-thread warning remains visible.

## Defects found and preserved attempts

- `oracle01`: the wall-clock timer observer's lower-bound assertion failed
  during startup; a timeout followed. Initial frame timing was the suspected cause. The source timer remained unchanged.
  Stabilizing harness frames resolved it. The timeout handler also received
  string captured output and needed UTF-8 encoding before writing bytes.
  The original engine log and failure report are retained.
- `oracle02`: observer insertion emitted literal backslash-n and caused a
  GDScript parse error. Fixed insertion; subsequent deterministic source checks pass.
- `build01`: the new localization test referenced nonexistent Context.Locale.
  It now uses the accepted run snapshot CurrentLocale; build02 passed.
- `automation01`: all82 tests passed, but post-timer evidence contained
  INVALID_NUMBER because the existing canonical JSON format permits integers
  only. Store measured integer microseconds and assert JSON parse/save in every
  replay; preserve original invalid records and all captures/logs.

Full raw logs (86 files) and their SHA-256 inventory are archived in [full_logs.zip](evidence/phase1g/full_logs.zip) and [inventory](evidence/phase1g/full_logs_inventory.json). Failures
are retained, not overwritten or recategorized as successful acceptance.

## Limits and exact Phase 1H recommendation

This is bounded development refusal/retry acceptance, not full Malet/Verdan or
campaign parity. Accept's playable chain, deal/reward/repeat-world groups,
world-memory seeding, shop/trades, Chapter3, autosave, achievements, extra NPCs,
battle, production New Game/Chapter1, final art/UI/audio, graphics modernization,
cook/package and Steam integration remain outside implementation.
Final Korean typography, physical USB gamepad testing, remapping/device reconnect
and unrestricted save/resume during Field/timer states are not certified.

Recommend **Phase1H only if separately authorized**:
after the same canonical paid-VN/reaction/normal route, execute original Accept0
with its real accepted flag then one sword burn; preserve the source0.3s callback;
import and execute only `malet_deal` originals0..4; observe its real0.5s completion
callback and record the next `malet_reward` request, then stop before that group.
Characterize the whole bounded chain in the source oracle first. Preserve current
refusal/retry and all82 identities; add exact payment/run/input/timer/next-request
tests and fresh rendered evidence. Do not import/execute reward8, world-memory
seeding, items/shop/Chapter3/autosave/achievements or the repeat-world group.
No Phase1H code or asset is included here.

## Reproduction

Use fresh evidence directories from the migration worktree:

```powershell
python Unreal/Tools/export_malet_refusal_oracle.py --godot <Godot-4.6.2-console.exe> --check --evidence-dir <fresh>/oracle
python Unreal/Tools/malet_refusal_test_fixtures.py --check
python Unreal/Tools/import_narrative.py --group malet_encounter --engine-root 'C:\Program Files\Epic Games\UE_5.8' --godot <Godot-4.6.2-console.exe> --evidence-dir <fresh>/encounter
python Unreal/Tools/import_narrative.py --group malet_refused --engine-root 'C:\Program Files\Epic Games\UE_5.8' --godot <Godot-4.6.2-console.exe> --evidence-dir <fresh>/refused
python Unreal/Tools/validate_unreal.py --engine-root 'C:\Program Files\Epic Games\UE_5.8' --build-and-test --rendered --evidence-dir <fresh>/automation
```
