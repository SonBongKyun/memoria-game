# Data and save migration contract

Phase 0 design only. No content or player saves have been converted. Source JSON and GDScript definitions remain authoritative until paired behavior tests accept an imported revision. [data_catalog.json](evidence/data_catalog.json) inventories all 45 datasets, their keys/types/counts and hashes; the file-by-file disposition table below covers each one.

## Import architecture

Keep authoring sources in their existing locations. Generate a versioned intermediate representation (IR) plus manifest; an editor-only C++ importer/commandlet validates it and produces cooked Unreal assets. Runtime shipping content should normally load cooked assets, not arbitrary loose JSON. Keep JSON external for interchange, fixtures, development tooling, and read-only legacy save import. Supporting hot-loaded player mods is outside this migration.

Use PrimaryDataAssets for nested story sequences, memory definitions, map definitions, galleries and encounter definitions. Use DataTables for genuinely flat actor/item/equipment rows; curves only for curves already present in source, not invented smoothing. Unreal supports typed table rows and asset-driven data workflows; choosing the shapes here is a project design decision. [Epic data-driven gameplay documentation, 5.7](https://dev.epicgames.com/documentation/en-us/unreal-engine/data-driven-gameplay-elements-in-unreal-engine?application_version=5.7).

Pipeline stages:

1. **Collect:** exact source path, file-byte SHA-256, importer version, content schema, source revision, licensing/provenance reference. The Phase 0 source-index hash normalizes text on read; it is an audit fingerprint, not a binary-source transfer checksum.
2. **Extract:** JSON parse with duplicate-key detection; restricted GDScript definition extraction or an isolated Godot export harness for computed tables/maps/sprites/PCM. Never evaluate arbitrary source strings in Python. Do not invoke normal chapter advancement to obtain definitions: that burns/erodes memory and writes state. A future harness must isolate the entire app-data/profile root as well as SaveManager slots.
3. **Normalize:** stable namespaced IDs and typed values, retaining source ordering, original zero-based indices, localization pairs, legacy path aliases, condition dialect and explicit effect phases. Store original source JSON alongside normalized metadata in the build manifest for diagnosis, not a second runtime authority.
4. **Validate:** every opcode, field, enum, jump, target scene, memory/actor/fact/portrait/CG/audio ID; reject unknown/unhandled fields until reviewed. Allow explicit documented legacy no-ops only. Resolve generated/dynamic references from exported catalogs, not regex guesses. Reject duplicate IDs and missing localization/assets with source-file/index diagnostics.
5. **Import:** deterministic package names under `/Game/Memoria/Generated/`; update existing assets in place by stable ID; soft-reference resolver plus explicit Asset Manager cook rules. Re-running unchanged input must change no logical payload, create no duplicate packages and produce no dirty assets. Raw `.uasset` byte equality is not the determinism criterion.
6. **Compare and promote:** publish counts, semantic hashes, source-to-package mapping, redirects and validation report. Run Godot/UE command traces before advancing a content version. Never silently overwrite designer overrides; put overrides in separate child assets or fail on edited generated packages. Stale outputs are reported for review, not automatically recursively deleted.

## Two narrative dialects, preserved separately

| Contract | Field: `DialogueManager` | VN: `SceneFlow` / `vn_scene` |
| --- | --- | --- |
| Shape | Root chapter/title plus `dialogues` dictionary of ordered line arrays | Sequence metadata plus ordered steps, action and presentation fields |
| Text | Speaker, portrait, text/text_ko, burned text and other gates | Speaker/text localization, CG/portrait, typing/auto/read tracking and actions |
| Conditions | Memory/flag/party/weave logic plus opt-in recursive `all`, `any`, `not`, actor memory/knowledge condition schema | Step/choice flag and memory gates; does not consume the field structured-world-condition schema |
| Effect ordering | Line gate precedes effects; legacy `requires_memory` + burned-text path has special gating behavior | Step set_flag/set_chapter/chapter-complete/burn/ending/weave effects can precede requires_flag/not filtering; rewards occur after gate |
| Choices | Filter into `_current_choices`; selected index addresses filtered array; flags can occur before failed cost | Renderer retains original choice index; payment must succeed before flags/rewards; explicit burn has separate behavior |
| Jump | `jump_to` is a zero-based target via `target-1` followed by advance | `goto` same convention; `goto_scene.start_index` zero-based; continuation queue FIFO |
| Unused/legacy action | Preserve source branch semantics | `goto_battle` warns/advances; no authored use found; unknown actions warn/advance in source but importer must report them |
| Save | Field dialogue itself is not a resumable UI snapshot | Current/pending sequence IDs, indices, active state, resume queue and ledger snapshot |

Do not translate both to one Blueprint conversation node set before expressing these differences. Use `FFieldDialogueLine` and `FVNStoryStep` with tagged effect phases. Add stable step IDs as `<source-sequence>/<original-index>` initially; version the source-index-to-step-ID table so later authoring edits cannot silently relocate old saves. Stable text IDs must distinguish field/VN location and locale. Preserve text bytes/punctuation; escaping BBCode/presentation tags into RichText requires an explicit tag mapping and visual review.

`StoryLog` persists Godot's hash of `speaker + '|' + trimmed localized text`, not a source line ID. Export a legacy-hash → stable text-ID lookup using the actual Godot hash implementation and both locales; do not substitute C++/FName hash. Collisions or unknown old hashes must remain legacy entries until resolved. Backlog entries (300 maximum) are session-only; read keys (8,000 limit with oldest-half eviction) are profile state.

## Definitions that are not JSON

| Source | Extracted data / nonmechanical boundary |
| --- | --- |
| `memory_manager.gd` | Starting/chapter memory definitions, ranks/power/NPC links, passives, vigil and loan/guard tables. Preserve insertion order for generated connections; do not export only the currently intact subset. |
| `game_manager.gd` | Item/equipment/upgrade, difficulty/NG+, ending/gallery and boss-rush definitions; formulas remain C++ rules with tests. Retain dynamic player-data fields. |
| Map scripts + `world_population`, `world_atlas`, `optional_memory_site` | Grids, colliders, spawn/gateway/POI data, variants, chapter gates, authored NPC/hostile/cache/curio records. Runtime trigger orchestration remains code/components. |
| `battle_manager`, `encounter_modifiers`, `random_encounter` | Skills, enemy/ability/modifier/environment rows and encounter pools. AI and damage/turn order require C++ reengineering. Preserve display-name aliases used by Codex. |
| `memory_resonance`, `side_quest`, `journey_oath` | Locations/rewards/requirements/text/quest and oath metadata; consume/break/reward rules remain explicit commands. |
| UI scripts | Portrait/CG maps, achievements, diary/skills, tutorial definitions, shop lists, atlas titles, journal labels, story hints and visual layout constants. Widgets contain content: JSON-only extraction would lose it. |
| `pixel_sprite`, `tile_painter`, `audio_manager`, `scene_transition` | Generated images/frames/tiles and PCM/stings. Bake deterministic artifacts with fixed local generation RNG; log seeds/settings without changing gameplay RNG. |

## Flags and identity

Keep flags as case-sensitive string keys with typed values where verified. [story_flag_sites.csv](evidence/story_flag_sites.csv) contains 113 unique literal keys found through `get_flag/set_flag/has_flag` calls; this is a **lower bound**, excluding direct dictionary writes, JSON effects and dynamic prefixes. Importer validation must combine those sources, not whitelist only this CSV.

Important families: `chN_*`, `canon_*`, `p3_*`, `talked_*`, burn reaction/revisit/world-forgot flags, companion/relationship flags, visited maps, cache/hunt/curio/quest completion, oath chapter rewards and fracture flags. An absent flag and a false value may have different storage meaning; retain both when importing extension data. Never rename `player.arrel` to match a display spelling. Current actor IDs are `player.arrel`, `npc.malet`, `npc.sable`, `npc.kairos`.

Actor catalog schema 1 is immutable validated input; swap a newly imported catalog only after complete validation. World snapshot schema 1 carries `revision`, `event_sequence`, `actors`, `world_flags`, `quest_states`. Each actor has memories, knowledge, location, relationships, emotions, quest state and flags. Preserve active/removed memory records, revision history, source actor metadata, and missing-vs-false fact entries. Registry validation drops unknown actor entries in the current import contract; retain a diagnostic rather than silently extending the catalog.

## Save schema and ownership

| Godot persisted section / file | Content and UE destination |
| --- | --- |
| `saves/save_1.json` through `save_3.json`, `saves/autosave.json` | Manual slots 1–3 and autosave 0; version `0.4.0`, timestamp/unix time, scene, position, autosave flag, sections below → versioned `UMemoriaRunSaveGame` schema 1 plus read-only JSON adapter |
| `game` | player_data, story_flags, current_chapter, ng_plus_cycle, equipped, upgrade_levels, seen_endings, play_stats, current_locale → Run aggregate; seen-endings union also profile policy |
| `memory` | Memory instance fields, burned ID order, burn/anchor passives, vigil chapters, erosion guards/used slots, active loan, extraction IDs → PlayerMemoryDomain; connections/carry recomputed in stable order |
| `world_state` | Actor cognition and extension bags, revision/sequence → WorldCognitionDomain; not player burn history |
| `scene_flow` | `current_id`, `current_index`, `pending_scene_id`, `pending_start_index`, `resume_queue`, `is_active`, `ledger_burn_snapshot` → Narrative continuation DTO |
| `elia_diary`, `tutorial_hints` | Diary read entries/skill cooldown data and shown hint IDs → run-owned progression/guide state; widgets rebuilt |
| `player_pos`, `scene` | Source pixel coordinates and `res://` PackedScene path → versioned map alias/coordinate transform; restore after world readiness |
| `achievements.json` | Unlocks and achievement statistics → ProfileSaveGame; no Steam functionality implied |
| `codex.json` | Enemy encounters/defeats/scans and discovered memories; display-name keys/image aliases → ProfileSaveGame |
| `ng_plus.json`, `seen_endings.json`, `boss_rush.json` | Unlock/record state across runs → ProfileSaveGame with explicit merge/reset policy |
| `read_lines.json` | Legacy hashed localized text registry → profile read-history compatibility adapter |
| `settings.json` | Volumes, display, language and gameplay/accessibility/presentation settings → settings adapter; language also appears in run data, so preserve load precedence |
| `user://screenshots/` | Player screenshots, not save state → leave source files alone |

Never promise a mid-battle save: source saves the run and field return context, not the mutable enemy/turn/UI session. Puzzle board, flow pressure/cooldowns, random encounter distance, temporary companions/tweens/queues, current CG animation, constellation hover and compass hidden state are transient unless a source field explicitly says otherwise. Shop per-dictionary `sold` state is not a durable shop inventory catalog.

### Compatibility algorithm

1. Read a copy of source JSON; validate root/type/size, try existing backup rules if corrupted, and normalize missing sections as Godot does even when version says `0.4.0`.
2. Resolve `res://` scene through a complete table of 30 scene aliases, including legacy/preview/save destinations. Reject missing targets before mutating the active run. Do not substitute the new opening for an unknown legacy map.
3. Convert ordinals/number types explicitly. Preserve all known and extension keys. Import current-world defaults and legacy Malet fact alias exactly, with no extra revision/event. Import source ownership separately; do not cross-burn player memories when importing world tombstones.
4. Build and validate a candidate run/profile delta/continuation. Validate memory/actor IDs, references, loan state and cursors against the matching content revision. A VN save with invalid continuation currently falls back to `ch1_prologue`; encode/report that source compatibility behavior, not an unrelated chapter.
5. Commit the aggregate, load the map or VN host, restore position/continuation after readiness, then signal UE load completion. Emit no acquisition/burn/achievement rewards merely from reconstruction. Keep old source state available for rollback if world loading fails.
6. Write UE saves to a separate directory via temporary file, flush, backup and replace; never overwrite Godot JSON. Record adapter version/source hash/content version once. The atomic-write and completion timing changes are technical durability changes, documented here; they must not change story outcomes.

VN re-entry may rerun a current step's effects because the source saves a cursor rather than an effect journal. Characterize save/load immediately before/after every effectful step. Do not silently add once-only flags to authored content: choose a compatibility policy based on the observed trace, and version any approved bug fix separately. Likewise test diary import/reset, cross-slot contamination and NG+ carry behavior before deciding whether an apparent defect is part of a required compatibility path.

## Required import fixtures

Reuse all five `data/test_fixtures/save_migrations` fixtures, plus generated cases for absent/current-malformed world state, missing map, bad VN ID/index, both locales/read hashes, raw grades 0–4, faded/burned/residue combinations, active/overdue/extracted debt, full guard usage, synthesis IDs, oath flags, different cross-slot diary states, profile merge, and every ending precedence tie. Export normalized state and ordered event outputs for comparison. Fixture generators are future implementation work; Phase 0 only ran the existing five-fixture suite.

## Dataset disposition

The following table is generated from the audited file list. JSON remains original authoring/test source; “target” describes the future importer output.

| Source JSON | Future target / disposition |
| --- | --- |
| [data/chapter10_dialogue.json](../../data/chapter10_dialogue.json) | Field-dialogue PrimaryDataAsset; preserve ordered groups/line dialect and localization |
| [data/chapter1_dialogue.json](../../data/chapter1_dialogue.json) | Field-dialogue PrimaryDataAsset; preserve ordered groups/line dialect and localization |
| [data/chapter2_dialogue.json](../../data/chapter2_dialogue.json) | Field-dialogue PrimaryDataAsset; preserve ordered groups/line dialect and localization |
| [data/chapter3_dialogue.json](../../data/chapter3_dialogue.json) | Field-dialogue PrimaryDataAsset; preserve ordered groups/line dialect and localization |
| [data/chapter4_dialogue.json](../../data/chapter4_dialogue.json) | Field-dialogue PrimaryDataAsset; preserve ordered groups/line dialect and localization |
| [data/chapter5_dialogue.json](../../data/chapter5_dialogue.json) | Field-dialogue PrimaryDataAsset; preserve ordered groups/line dialect and localization |
| [data/chapter6_dialogue.json](../../data/chapter6_dialogue.json) | Field-dialogue PrimaryDataAsset; preserve ordered groups/line dialect and localization |
| [data/chapter7_dialogue.json](../../data/chapter7_dialogue.json) | Field-dialogue PrimaryDataAsset; preserve ordered groups/line dialect and localization |
| [data/chapter8_dialogue.json](../../data/chapter8_dialogue.json) | Field-dialogue PrimaryDataAsset; preserve ordered groups/line dialect and localization |
| [data/chapter9_dialogue.json](../../data/chapter9_dialogue.json) | Field-dialogue PrimaryDataAsset; preserve ordered groups/line dialect and localization |
| [data/chapter_expansion_gallery.json](../../data/chapter_expansion_gallery.json) | Gallery/visual manifest DataAsset and texture soft references; preserve metadata |
| [data/development/malet_memory_world_dialogue.json](../../data/development/malet_memory_world_dialogue.json) | Development field-dialogue fixture DA in test content only; do not add to campaign |
| [data/epilogue_dialogue.json](../../data/epilogue_dialogue.json) | Field-dialogue PrimaryDataAsset; preserve ordered groups/line dialect and localization |
| [data/illustration_expansion_gallery.json](../../data/illustration_expansion_gallery.json) | Gallery/visual manifest DataAsset and texture soft references; preserve metadata |
| [data/illustration_gapfill_gallery.json](../../data/illustration_gapfill_gallery.json) | Gallery/visual manifest DataAsset and texture soft references; preserve metadata |
| [data/interface_visual_gallery.json](../../data/interface_visual_gallery.json) | Gallery/visual manifest DataAsset and texture soft references; preserve metadata |
| [data/occult_boss_gallery.json](../../data/occult_boss_gallery.json) | Gallery/visual manifest DataAsset and texture soft references; preserve metadata |
| [data/test_fixtures/save_migrations/corrupt_world_state_0_4_0.json](../../data/test_fixtures/save_migrations/corrupt_world_state_0_4_0.json) | External test JSON only; feed legacy-save adapter tests, not shipping story |
| [data/test_fixtures/save_migrations/current_0_4_0.json](../../data/test_fixtures/save_migrations/current_0_4_0.json) | External test JSON only; feed legacy-save adapter tests, not shipping story |
| [data/test_fixtures/save_migrations/legacy_0_3_0.json](../../data/test_fixtures/save_migrations/legacy_0_3_0.json) | External test JSON only; feed legacy-save adapter tests, not shipping story |
| [data/test_fixtures/save_migrations/missing_world_state_0_4_0.json](../../data/test_fixtures/save_migrations/missing_world_state_0_4_0.json) | External test JSON only; feed legacy-save adapter tests, not shipping story |
| [data/test_fixtures/save_migrations/unsupported_world_schema_0_4_0.json](../../data/test_fixtures/save_migrations/unsupported_world_schema_0_4_0.json) | External test JSON only; feed legacy-save adapter tests, not shipping story |
| [data/vn_scenes/ch11_departure.json](../../data/vn_scenes/ch11_departure.json) | VN PrimaryDataAsset, typed step IR; original JSON retained |
| [data/vn_scenes/ch12_reader.json](../../data/vn_scenes/ch12_reader.json) | VN PrimaryDataAsset, typed step IR; original JSON retained |
| [data/vn_scenes/ch13_third_person.json](../../data/vn_scenes/ch13_third_person.json) | VN PrimaryDataAsset, typed step IR; original JSON retained |
| [data/vn_scenes/ch14_confessor_intervention.json](../../data/vn_scenes/ch14_confessor_intervention.json) | VN PrimaryDataAsset, typed step IR; original JSON retained |
| [data/vn_scenes/ch15_singer.json](../../data/vn_scenes/ch15_singer.json) | VN PrimaryDataAsset, typed step IR; original JSON retained |
| [data/vn_scenes/ch16_nera.json](../../data/vn_scenes/ch16_nera.json) | VN PrimaryDataAsset, typed step IR; original JSON retained |
| [data/vn_scenes/ch17_forgetting_storm.json](../../data/vn_scenes/ch17_forgetting_storm.json) | VN PrimaryDataAsset, typed step IR; original JSON retained |
| [data/vn_scenes/ch18_living_funeral.json](../../data/vn_scenes/ch18_living_funeral.json) | VN PrimaryDataAsset, typed step IR; original JSON retained |
| [data/vn_scenes/ch19_approach.json](../../data/vn_scenes/ch19_approach.json) | VN PrimaryDataAsset, typed step IR; original JSON retained |
| [data/vn_scenes/ch1_after_forest.json](../../data/vn_scenes/ch1_after_forest.json) | VN PrimaryDataAsset, typed step IR; original JSON retained |
| [data/vn_scenes/ch1_cold_open.json](../../data/vn_scenes/ch1_cold_open.json) | VN PrimaryDataAsset, typed step IR; original JSON retained |
| [data/vn_scenes/ch1_forest_walk.json](../../data/vn_scenes/ch1_forest_walk.json) | VN PrimaryDataAsset, typed step IR; original JSON retained |
| [data/vn_scenes/ch1_prologue.json](../../data/vn_scenes/ch1_prologue.json) | VN PrimaryDataAsset, typed step IR; original JSON retained |
| [data/vn_scenes/ch1_void_beast.json](../../data/vn_scenes/ch1_void_beast.json) | VN PrimaryDataAsset, typed step IR; original JSON retained |
| [data/vn_scenes/ch20_monolith.json](../../data/vn_scenes/ch20_monolith.json) | VN PrimaryDataAsset, typed step IR; original JSON retained |
| [data/vn_scenes/ch21_editors_turn.json](../../data/vn_scenes/ch21_editors_turn.json) | VN PrimaryDataAsset, typed step IR; original JSON retained |
| [data/vn_scenes/ch22_core.json](../../data/vn_scenes/ch22_core.json) | VN PrimaryDataAsset, typed step IR; original JSON retained |
| [data/vn_scenes/ch23_conversion.json](../../data/vn_scenes/ch23_conversion.json) | VN PrimaryDataAsset, typed step IR; original JSON retained |
| [data/vn_scenes/ch24_testimony.json](../../data/vn_scenes/ch24_testimony.json) | VN PrimaryDataAsset, typed step IR; original JSON retained |
| [data/vn_scenes/ch2_market_arrival.json](../../data/vn_scenes/ch2_market_arrival.json) | VN PrimaryDataAsset, typed step IR; original JSON retained |
| [data/vn_scenes/ch5_classifier.json](../../data/vn_scenes/ch5_classifier.json) | VN PrimaryDataAsset, typed step IR; original JSON retained |
| [data/world_population_visual_gallery.json](../../data/world_population_visual_gallery.json) | Gallery/visual manifest DataAsset and texture soft references; preserve metadata |
| [data/world_state/actors.json](../../data/world_state/actors.json) | Validated actor DataTable/catalog asset; schema 1 IDs retained |
