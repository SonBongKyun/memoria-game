# Bounded narrative IR v1

Reviewed production inputs are `ch2_market_arrival.vn.v1.json`,
`verdan_arrival.field.v1.json` and Phase1F `malet_taste_burned.field.v1.json`.
The default pipeline retains the first two; `--group malet_taste_burned`
selects only three Field rows at authored group position16. `contract_inputs.v1.json` and
`contract_expected.v1.json` are offline test/oracle artifacts, never assets.
Additional synthetic/negative IR lives under `../../fixtures/narrative/`.

The outer envelope has `schema_version=1`, `content_kind=narrative.vn` or
`narrative.field`, an explicit matching `dialect`, `extractor_version`,
`source_revision`, ordered `sources`, `definition`, and `semantic_sha256`.
The definition contains a source sequence/group ID, index mapping version 1,
typed metadata, and **steps for VN / rows for Field**. Each row/step retains
storage index, original index, stable dialect/sequence/index ID, provenance,
effect phase, optional text/presentation/gate/effect/action fields, and ordered
choices. A choice carries its original authored index, never a visible index.
Each record's provenance repeats file, normalized hash, revision, dialect,
group position, original row/step index, and original choice index (-1 for rows).

Text IDs are deterministically `record.id + '/' + text field`, including the
`_ko` suffix for Korean. Runtime `TextId()` uses that same rule. Text values are
literal strings: no punctuation, spelling, Unicode normalization or translation
changes. Empty-but-present speaker and other fields remain present. Typed
Unreal primitives have individual presence bits; absent differs from empty or
false. Absent jumps are explicitly null in IR; authored null jumps are rejected.
No runtime flags, filtered choices, active state or resume queue enter assets.

Canonical serialization reuses Phase 1C: UTF-8 without BOM, Unicode unescaped,
sorted ASCII keys, compact separators, one LF, ordered arrays, strict integer
and boolean types, duplicate-key/nonfinite rejection. Source text hashes
normalize CRLF to LF in memory; raw checkout hashes are evidence only. Fade
seconds convert to **exact integer milliseconds**, without rounding; invalid
types or submillisecond values are rejected. Original authored numeric text
remains retrievable by source revision/hash. No clock or absolute path affects
IR or semantic hashes. The semantic hash includes schema/kind/dialect and the
whole definition except repeated provenance; import metadata is compared
separately for no-op decisions. Source-index IDs are versioned locations, not
a promise that arbitrary future source insertion preserves save identity.

The schema is deliberately bounded to reviewed VN/Field shapes and explicit group/count pairs. Only
inspected gates, effects and transition requests are accepted. Unknown fields
or future opcodes require review and a schema/extractor decision. This is not
a general importer for every field/VN feature. In particular structured world
conditions, burned substitutions, chapter ledgers and other campaign actions
remain later cohorts. Source `record_ending` on a VN **choice** is a retained
legacy no-op (SceneFlow does not call it); a VN **step** or Field record has its
own effect behavior. No generic conversation graph replaces the two dialects.

`narrative_ir.py` checks Git revision, every dependency hash, source metadata and
the exact normalized authored records. The editor importer independently
validates canonical bytes, all types/indices/phases/IDs, source hashes and typed
payload against selected authored JSON before touching a package. The Python
wrapper also reruns the executable oracle. Run production imports through that
wrapper so Git revision attestation is enforced in addition to editor checks.
Modified/synthetic IR can load into transient test objects only with explicit
source verification disabled. The production commandlet always verifies source.

```powershell
python Unreal/Tools/narrative_ir.py --check --evidence-dir <fresh-evidence>/extract
python Unreal/Tools/export_narrative_oracle.py --godot <Godot-4.6.2-console.exe> --check --evidence-dir <fresh-evidence>/oracle
python Unreal/Tools/generate_narrative_types.py --check
python Unreal/Tools/narrative_test_fixtures.py --check
python Unreal/Tools/import_narrative.py --engine-root 'C:\Program Files\Epic Games\UE_5.8' --godot <Godot-4.6.2-console.exe> --build --evidence-dir <fresh-evidence>/import
```

Omit `--check` for a deliberately reviewed new extraction/oracle fixture update.
Rechecks never rewrite expected results. Generated reflection/codecs contain
schema fields only, no authored story values. Runtime consumes only typed
assets and the accepted memory domain, with borrowed run/definition references
that the caller must keep alive. The Phase1E/F development host owns UI/travel lifetime; broader campaign
orchestration remains deferred. SaveGame and continuation schema remain 1.

Phase1F NPC oracle and transient negatives live in ../../fixtures/malet/.
No normal transaction group is part of the reviewed import cohort.

S298 adds five plain-row Verdan exploration groups to the reviewed Field cohort:
`verdan_market_walk` (position 6), `verdan_old_burner` (7), `malet_backstory` (8),
`elia_sump_concern` (9) and `sump_atmosphere` (10). They carry text, speaker, portrait
and CG only, with no gates, effects or choices. Import each with `--group <id>`.
`elia_ch2_talk` (burned substitutions) and the Sump Ledger quest remain later cohorts.

S299 adds the Sump Ledger groups the source requests: `sq_sump_ledger_start` (position 11),
`sq_sump_ledger_found` (12) and `sq_sump_ledger_return` (13). `sq_sump_ledger_burn` is authored
but never requested by `verdan_market.gd`, so it is not imported.

S300 adds the Field-only legacy substitution keys `requires_memory`, `burned_text` and `burned_text_ko` (Text group) and `burned_portrait` (Presentation). They follow `dialogue_manager.gd`: they are never a gate, and the line swaps when the memory is in the burned list. A `FIELD_CASES` entry may name its source file as a fourth element. The C++ cohort table carries that file and its root chapter. `elia_ch2_talk` (chapter 2, position 5), `elia_song_burned` and `elia_sword_burned` (`chapter1_dialogue.json`, positions 14 and 15) are imported.

S302 adds the Chapter 1 VN cohort (`VN_CASES`: `ch1_cold_open`, `ch1_prologue`, `ch1_forest_walk`, `ch1_void_beast`, `ch1_after_forest`, alongside `ch2_market_arrival`). It also adds the VN-only keys listed under `VN_ONLY`, an optional `bgm`, and `goto_scene` across cohort scenes. Import each scene with `--group <id>`.
