# Phase 1D — bounded narrative content and execution contracts

Date: 2026-09-06. Status: **COMPLETE**. Technical checkpoint:
**`ee61269daf5ee20724a02eb51d07d997f26a8474`**, `feat(unreal): import bounded narrative contracts`.
The documentation follow-up records the technical SHA without attempting to
embed a commit's own hash. Local commits only; no push.

## Baseline, source scope and provenance

Branch: `unreal-migration/ue58-foundation`. Worktree:
`C:\Users\jc\MemoriaMigration\foundation`. Preflight `git status`, `git diff`,
`git diff --check`, and `git log --oneline -10` confirmed a clean worktree at
`4edb035360c19d6286fef0e17ca2fc4bda4b492f`. Accepted Phase 1C technical checkpoint:
`b6c632fc83fdbdbe21d26452b2135f4997704988`. The original Godot checkout was left
at its pre-existing HEAD/status. Engine Build.version was read directly:
**UE 5.8.2 / CL 56702186**, `C:\Program Files\Epic Games\UE_5.8`.

Only the complete `data/vn_scenes/ch2_market_arrival.json` and the
`verdan_arrival` group of `data/chapter2_dialogue.json` are imported. No other
dialogue group, VN file, map or campaign route was migrated. Source revision:
**`4f772fafb1c7c6da68f56f44d60a7c7fed7f3949`**, the latest commit touching the
audited dependency paths. Migration-only commits do not fabricate a new content
revision. Each dependency is checked against this Git revision and its hash.

| Source | SHA-256, UTF-8 text with LF |
| --- | --- |
| VN file | `970ae54c68e1d5b4337bd27267c047ad3c8a5113ee2cd7e2a23324333ebc980b` |
| Field file, only `verdan_arrival` extracted | `67ec5b7c5f455f41ba997fd6c84220f85f8f87f7f111ef9b3e28c8ead935ac21` |

Raw checkout SHA-256: VN
`33121d0e3aa9bb2870a82e2ff73dc279355045805b6a040dd08235d9f4469983`;
Field `76b75afef0d2cbcd9747e83884defee806046dce6947207f885aa4bf6913c691`.
The Field hash covers its entire source file to detect drift, while extraction
is restricted to the one selected group. Full dependency hashes are in
[extraction evidence](evidence/phase1d/export02/extraction.json).

Inspected executable sources: DialogueManager and SceneFlow, VN renderer choice
filtering/selection binding, VNHost pending/FIFO consumption, Verdan entry
callers, MemoryManager burn/intact rules, JourneyOath burn hook, and GameManager
flags/localization/rewards. The requested migration documents and session log
were read. The advertised GDD path is absent; its historical version is under
the original project's `각종 문서/구버전/`. Current source behavior controls parity.

## Extraction, IR and typed assets

Deterministic JSON extraction uses duplicate-key rejection and strict reviewed
field sets. No authored text is manually recopied into C++. English/Korean
text, title localization, empty speaker fields, source presentation references
and numeric semantics are retained. Fade seconds convert exactly to integer
milliseconds. No image, portrait, BGM or presentation asset is loaded.

| Dialect | IR, schema 1 / extractor `memoria-narrative/1` | Canonical IR SHA-256 |
| --- | --- | --- |
| VN | [ch2_market_arrival.vn.v1.json](ir/narrative/ch2_market_arrival.vn.v1.json), `narrative.vn` | `0416a72478b63e7b6178db54df11af0afb2489c644a24c92a0c307e3b0d07050` |
| Field | [verdan_arrival.field.v1.json](ir/narrative/verdan_arrival.field.v1.json), `narrative.field` | `7e7f8b22d01483f12a5c85cb687fdf8aafaa459c44617272f5333b891ceb877b` |

The [schema/workflow](ir/narrative/README.md) describes canonical UTF-8/LF,
Unicode, explicit null jumps, absent/empty/false presence, sorted keys, ordered
arrays, stable text IDs, provenance and exact type rejection. No timestamp or
machine path affects a semantic hash. Repeated extraction and check-only source
execution reproduce identical canonical bytes. Per-record provenance retains
file/hash/revision/dialect/group position/original row and choice indices.

`FMemoriaFieldDefinition` with `FMemoriaFieldRow/FMemoriaFieldChoice` and
`FMemoriaVNDefinition` with `FMemoriaVNStep/FMemoriaVNChoice` remain separate.
Only optional text/presentation/condition/effect/provenance primitives are
shared. `UMemoriaFieldAsset` and `UMemoriaVNAsset` contain reflected typed fields,
arrays and presence bits, not opaque IR strings. Stable narrative/text identity
comes from source sequence/group and original indices, never package names.

Assets:

- `/Game/Memoria/Generated/Narrative/DA_VN_Ch2MarketArrival`
- `/Game/Memoria/Generated/Narrative/DA_Field_VerdanArrival`

The new editor importer/commandlet reuses Phase 1C canonical JSON and SHA-256
primitives without adding a content switch to the memory importer. It validates
the full candidate before package mutation, rejects wrong-type existing
packages and edited generated semantic payloads, and compares reflected fields
for no-op import. It independently checks hashes and compares typed values to
selected authored JSON. The Python wrapper additionally verifies Git revision,
fresh extraction and the actual Godot oracle. Runtime depends only on typed
assets, existing run/continuation types and the accepted memory domain.

## Reimport and semantic change evidence

[Import pipeline](evidence/phase1d/import01/pipeline.json): six separate Unreal
processes, three per dialect. Each first import was **CREATED/saved=true**;
each second import and third check-only reload was **UNCHANGED/saved=false**.

| Dialect | First semantic fingerprint | Second / reload |
| --- | --- | --- |
| VN | `3ffc5fe3d8b88d72a6b2b78cd67d94cd85e9259f8c76e0c4284c4493fa740049` | Identical |
| Field | `18f3943353502cec8beb14e7271cf4f08c612d9b158a424847a62d8f2ae8edd8` | Identical |

Same paths, IDs, indices, choices, phases, gates, effects and action/continuation
data were reloaded. Exactly two narrative packages exist. Package bytes also
remained identical through both Automation runs; their hashes are recorded in
the import report. Binary equality is observed evidence, not a cross-engine
serialization requirement.

A single altered VN narration is copied to
`Unreal/Memoria/Saved/Validation/NarrativeTemporary/modified_ir.json`. Loading it
into a transient typed asset changes the computed semantic fingerprint. Source
attestation rejects it for production. No temporary Unreal package is saved.
Twenty-four recomputed negative IR variants cover schema/kind/dialect,
revision/provenance/hashes, ID and case changes, index/order corruption, numeric
and boolean types, null text, invalid jumps, phases, effects, gates, continuation
and mutable-state contamination. Unknown authored fields and null/bool/string
authored jumps fail loudly; malformed source is not repaired.

## Actual execution contracts and indices

Exact original indices: **VN `0,1,2,3,4,5,6,7,8,9,10,11,12`; Field
`0,1,2,3,4`**. Original group/array position is 0. Storage index is retained
separately even though it equals original index in these two source fixtures.
Index mapping version stays 1. VN choice step **10** preserves authored choices
**0,1,2**: `guard_lied_to`, `guard_bribed`, `guard_pushed` in that order. Field's
selected group has no authored choices; none were invented in production data.

| Contract | Source-authentic observation and Unreal result |
| --- | --- |
| Field traversal | All five English and Korean rows match, including empty speakers; end resets cursor to 0 |
| Field gate/effect | Failed row gate skips its flag effect; passed row effects run before exposure |
| Field visible index | Synthetic originals `[1,2]` become visible `[0,1]`; selecting visible 1 executes original 2 |
| Field failed cost | Choice text/flag occur before failed burn; 7 Grains still awarded and jump reaches original row 3 |
| VN gate/effect | Gated synthetic step sets `before_gate`, skips 100-Grain reward and display, then visits step 1 |
| VN original index | Burning market-food memory yields visible originals `[0,2]`; selecting original 2 sets `guard_pushed` and reaches step 11 |
| VN failed cost | Direct original-choice 1 call records the choice, fails payment, leaves flags/reward unchanged and remains at step 10 |
| VN explicit burn | Distinct from cost: flag occurs before failed explicit burn, then 9-Grain reward and jump still execute |
| Jumps | `jump_to`/`goto` preserve target-1 then advance; isolated tests reach 3 and return to 0 without renumbering |
| VN map request | Original step 12 sets `ch2_arrival_vn_seen`, becomes inactive, retains current sequence/index 12 and emits a Verdan map request only |
| Continuation | Current active cursor wins over pending; prepare makes it pending/inactive; consumption re-executes the original step; FIFO resumes 3 then 4 |

The existing `FMemoriaVNContinuation` and `UMemoriaRunSaveGame.SceneFlow` are used
directly. SaveGameToMemory/LoadGameFromMemory round-trip preserves the DTO;
**SaveGame schema 1 and continuation schema 1 are unchanged**. Effects can rerun
on resume because the source stores a cursor, not an effect journal. VN metadata
`chapter:2` is preserved metadata and does not itself advance the run chapter.

The Godot oracle copies DialogueManager, SceneFlow, MemoryManager and JourneyOath
byte-identically to a minimal project with isolated APPDATA/LOCALAPPDATA.
Observers call source methods through `super`. Only presentation, audio,
transition and unrelated world/profile adapters are inert. The complete actual
VN renderer filtering prefix executes verbatim, stopping before Button creation;
its hash is attested. Ten cases compare every ordered event prefix, visited
cursor, flags, grains, HP, burn history, map request and continuation snapshot.
The added filtered-original-selection case leaves all previous nine oracle
results unchanged: [extension evidence](evidence/phase1d/oracle_extension.json).
[Final oracle replay](evidence/phase1d/oracle05_check/oracle.json) checks existing
expectations without regeneration. Synthetic cases are explicitly marked and
cannot pass the production import path.

## Build, Automation, regressions and source protection

| Acceptance check | Actual result |
| --- | --- |
| UE 5.8.2 UBT / UHT / compile / link | PASS, real installed engine; all three incremental builds and final rendered Automation builds passed |
| Previous accepted suite | **60/60 PASS**, exact identities preserved |
| Phase 1D suite | **8/8 PASS**, zero test errors/warnings |
| Total Automation | **68/68 PASS**, process exit 0; exact test identity validation enabled |
| Godot source oracle | **10/10**, fresh-process recheck identical |
| Repository / VN / Korean | PASS; VN 21 files / 526 steps; Korean 32 files / 1,581 fields |
| Godot editor import | PASS on unchanged-source retry, exit 0, zero engine error lines |
| Official memory/world | **15/15 PASS**, fatal scan/save isolation; exported catalog 1,064,635,280 bytes, export_log_errors=0 |
| Native memory / CTest | **51/51**, **1/1 PASS** |
| Starting memory catalog | Source check exact Phase 1C IR hash; all three existing catalog Automation tests PASS |
| Host validators | **35/35 PASS**, including the previous 23 unchanged tests |
| Foundation static | **52/52 PASS** |
| Original source protection | **4,217 files byte-identical**, original HEAD/status preserved; Godot metadata 1,056 files restored |

Exact new test names (all `Success`):

- `Memoria.Narrative.VNImportParity`
- `Memoria.Narrative.FieldImportParity`
- `Memoria.Narrative.DeterministicReimport`
- `Memoria.Narrative.FieldExecutionContract`
- `Memoria.Narrative.VNExecutionContract`
- `Memoria.Narrative.OriginalVisibleChoiceIndices`
- `Memoria.Narrative.ContinuationDTO`
- `Memoria.Narrative.StrictValidationAndSemanticChange`

Evidence: [acceptance](evidence/phase1d/acceptance.json), [final exact Automation
report](evidence/phase1d/automation02/automation_index.json), [engine commands and
exit statuses](evidence/phase1d/automation02/unreal_validation.json),
[Godot](evidence/phase1d/regressions/godot02/godot_baseline.json),
[native](evidence/phase1d/regressions/native01/native_memory_validation.json),
[host](evidence/phase1d/regressions/host_tests.json),
[source protection](evidence/phase1d/regressions/static01/foundation_static.json),
[full-log archive inventory](evidence/phase1d/log_archive.json).
Raw logs retain their exact engine line endings in `full_logs.zip`; a scoped
ignore rule excludes loose copies from Git without editing evidence bytes.
The total suite retains the prior foundation map test's one engine render-thread
warning. No new narrative test emits a warning or error.

## Defects found, fixes and remaining risks

- First extraction caught an omitted Field root `title_ko`; strict validation
  rejected it and the extractor now preserves that authoritative field.
- The first oracle adapter had mixed indentation; the next lacked declarations
  for guarded, unused AchievementManager/SaveManager calls. These harness-only
  defects were fixed; the timeout and source engine diagnostics are preserved.
- First full Godot editor import exited with access violation `3221225477`.
  It is retained as failure evidence, not counted as a pass. No source/plugin
  repair was made; the separate retry passed import and the complete suite.
- Source re-verification found a next-phase routing trap: Verdan sees
  `ch2_arrival_vn_seen` and **skips `verdan_arrival`**, entering free exploration.
  Forcing VN followed by that Field group would change the canonical route.
- Text/grammar/localization oddities and dialect differences were preserved.
  No Godot gameplay/content defect was silently fixed. Null-jump validation and
  direct source-payload attestation prevent malformed/modified IR promotion.

Remaining limits: no cook/package certification, campaign boot, production UI,
portraits/CG/audio, Verdan map, BattleManager or full narrative opcode migration.
The runtime objects borrow run/domain/definition lifetimes; the next host must
own them correctly. Active-oath progression, structured world conditions,
chapter/ending orchestration, profile read-hash compatibility and broader
content resolution are outside these fixtures. Unknown continuation sequences
are explicitly rejected at this bounded boundary; a later resolver must model
the source's invalid-save fallback without inventing a Chapter 1 asset here.
Semantic equality is the reimport contract; future engine package bytes may
legitimately differ. No campaign/playable parity is claimed.

Created/modified files are listed in
[changed_files.json](evidence/phase1d/changed_files.json). Changes are confined to
new narrative types/runtime, editor importer/commandlet/tests, migration tools,
two typed assets, IR/oracle/test artifacts, evidence/docs, scoped LF policy,
session log and the additive exact-name Automation registry. Previous 60 tests,
memory kernel, input/movement/camera foundation, SaveGame schema, Godot source,
and historical evidence remain unchanged.

## Exact recommended next phase — not implemented

**Phase 1E: the smallest source-faithful `ch2_market_arrival` → Verdan arrival
development slice.** Use the imported VN asset, existing 2D movement/input/camera
foundation and only minimal temporary text/choice presentation. At the map
handoff preserve the actual `ch2_arrival_vn_seen` branch: enter the minimal
Verdan arrival host without automatically replaying `verdan_arrival`. Exercise
the imported Field asset through a separate arrival fixture whose VN-seen flag
is false. This proves both assets while preserving the current campaign route.
Keep New Game unchanged until that slice's explicit scope permits wiring it;
do not add Malet transactions, battle, Chapter 1, final art/audio or further
chapter migration to this first slice.
