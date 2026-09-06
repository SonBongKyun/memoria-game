# MEMORIA migration audit — Phase 0

Audit date: 2026-09-06. Target: **Unreal Engine 5.7**, Windows, the existing 2D game. Reference checkout: `15df72809baa52eec281d8508f495a373e9f5884`, branch `overnight-gameplay-graphics`, repository `SonBongKyun/memoria-game`. The working tree already reported `project.godot` modified when this audit began; that state was preserved. No Unreal project, gameplay rewrite, asset conversion, commit, or deployment was performed.

Start with [SYSTEM_MAP](SYSTEM_MAP.md) to find implementation owners, [ARCHITECTURE](ARCHITECTURE.md) for target lifetimes, and [MIGRATION_ROADMAP](MIGRATION_ROADMAP.md) for the next task. [PARITY_MATRIX](PARITY_MATRIX.md), [DATA_MIGRATION](DATA_MIGRATION.md), [ASSET_INVENTORY](ASSET_INVENTORY.md), and [RISK_REGISTER](RISK_REGISTER.md) supply acceptance and conversion contracts.

## Evidence and limits

The audit inventoried all 4,196 tracked paths. It structurally scanned the complete text of all 186 first-party GDScript files, all 30 scene files, all 45 JSON files, and every file under `assets/` and `mugic/`. Manual implementation review concentrated on every runtime system's entry points, state mutation, serialization, integration, and presentation boundaries. Large procedural drawing functions were structurally indexed and sampled; the entire 1,792-file third-party tree was **not** line-reviewed. Plugin configuration, the Dialogic handler/UID, first-party references, and four actively referenced VFX Library shaders were inspected. This is an architecture audit, not a claim that every branch has been played or every illustration visually approved.

Read: root `AGENTS.md`, `CLAUDE.md`, `project.godot`, relevant `SESSION_LOG.md` entries, `BUILD_GUIDE.md`, script/scene instructions, canon stabilization notes, export configuration, Git ignore/CI configuration, and actual implementations. The advertised `../각종 문서/MEMORIA_GDD_v1.md` was absent; the historical document exists under `../각종 문서/구버전/`. Historical GDD/manuscript directives do not authorize changing executable behavior during an engine migration.

Reproducible inventory: `python docs/unreal-migration/collect_evidence.py`. It reads source and writes only this directory's `evidence/` reports. A local `.gdignore` prevents Godot from importing these audit CSVs as game translations; automatically generated audit-only import files were removed. [summary.json](evidence/summary.json) records the snapshot; [source_index.json](evidence/source_index.json) gives method line numbers and source hashes; [dependencies.csv](evidence/dependencies.csv) records lexical Autoload references; [signals.csv](evidence/signals.csv) and [event_sites.csv](evidence/event_sites.csv) locate declarations, emissions, and subscriptions. Lexical edges are candidates, including guarded/dead branches; they do not prove runtime reachability. Dynamically constructed references cannot be proved complete by this scanner.

## What is actually present

| Inventory | Verified count / interpretation |
| --- | --- |
| Runtime | 77 first-party runtime scripts, 53,784 lines; 109 other GDScript tool/test scripts; 65,905 total GDScript lines |
| Startup | 34 Autoload entries: 33 first-party and Dialogic |
| Scenes | 30 `.tscn`: 19 maps (10 core, 9 optional), 2 main, 1 battle, 1 player, 2 NPC, 1 story entry, 4 UI |
| Field dialogue | 11 JSON files, 128 dialogue groups, 992 top-level rows and 29 immediate choice entries; a row is not necessarily a spoken line |
| VN | 21 JSON files, 526 steps, including the Ch5 classifier and Part II/III preview content |
| Other JSON | 6 gallery/manifests, actor catalog, development state, 5 save fixtures; 45 JSON files overall |
| Assets | 1,816 files including import metadata, 1,409,950,207 bytes; 864 PNG, 13 JPG, 15 MP3, 2 TTF, 12 owned shaders |
| Persistent domains | Run slots, world cognition snapshot, player memories, VN cursor; separate profile/settings/read-history files |
| Rendering | Primarily 2D; existing `HybridDepthStage` also composites a decorative 3D SubViewport into 2D screens |

Counts replace old S54/S209 prose. They describe authored inventory, not the size of one reachable campaign route. See the asset report for metadata exclusions and the system map for every runtime source.

## Reachability and completion classification

**Implemented** below means executable implementation was inspected. **Baseline-covered** means the specific checks listed later ran successfully. Neither term means the entire game has passed an end-to-end playthrough.

| Area | Current classification and evidence |
| --- | --- |
| Current New Game | Implemented VN Ch1: `ch1_cold_open → ch1_prologue → ch1_forest_walk → ch1_void_beast → ch1_after_forest → ch2_market_arrival → verdan_market`. The beast sequence is VN, not a `BattleManager` fight. Forest VN has internal looping choices. |
| Current canon field route | Verdan → Belt Waystation → Drift Shelter → dedicated Ch5 classifier entry/VN → Drift Shelter with `canon_ch6_seam_ready`. Wave1/Wave2A baseline-covered. This is a deliberate pending Ch6 boundary, not evidence that all later legacy chapters are the current canon route. |
| Legacy maps and endings | Ten core maps and their older continuation/event/boss/ending scripts remain implemented. Fast-travel and story gates condition their availability. Preserve them and old save aliases; do not reconnect them into the current route by inference. |
| Aftermath preview | Explicit title action starts Ch11 VN preview; later scripts run through Ch24 and credits, with a `demo_build` gate after Ch18. This is a distinct entry route, not automatic proof of New Game continuity. |
| Memory / battle / economy | Substantial implementations: erosion, cascade, residue, loans/extraction, synthesis, carrying, passives, oaths; stances, witnesses, break/momentum, directives, allies, equipment, items, environments, NG+/rush. Not placeholder inventory/combat. |
| World Memory Engine | Implemented catalog/state/mutation/conditions and Malet/Sable/Kairos consequences, baseline-covered. Four actors; relationships/emotions/location/quest bags exist as snapshot fields, not a complete autonomous NPC simulation. |
| Constellation / compass | Implemented: constellation is a read-only relationship view; compass is an atmospheric density needle. Neither is a crafting skill tree or navigation/pathfinding system. |
| Controller support | Implemented field input, device hints, rumble; partial overall. Raw-key/mouse handlers remain in VN/tutorial/overlays, and sprint's displayed `LB` differs from configured button index 9. Need a measured controller coverage matrix. |
| Dialogic | Installed, enabled, Autoloaded (2.0 Alpha 20 WIP); no first-party story calls found. Actual story uses custom `DialogueManager` and `SceneFlow`. Do not migrate a presumed Dialogic timeline library. |
| ShaderV / VFX Library | ShaderV editor plugin enabled; VFX Library editor plugin not enabled, but four of its shader files are loaded by battle UI. Disabled editor plugin does not mean unused assets. |
| Procedural fallback | PixelSprite has real source-art paths plus legacy/fallback generation. Tile textures, walk frames, SFX, ambient layers, transitions, and much UI are generated by code. These are part of the actual pipeline. |
| Integration placeholders | Steam save/achievement hooks are integration stubs. External marketing/store actions need actual product configuration before release. Do not promise working Steam Cloud from comments. |
| Obsolete paths | `SceneFlow.goto_battle` explicitly warns and advances without combat; old NotificationToast branches after unconditional returns are unreachable. Retain these facts in conversion diagnostics instead of reviving or deleting gameplay silently. |

Rim's `_ready` leaves legacy random encounter/side-quest setup calls commented out. Verdan random encounters are enabled only on a **Ch2-complete revisit**, using Alley Rat/Market Thief and a 60–100 tile distance range. There are no authored Verdan world-population hunts. This matters for the proposed slice: its battle leg uses the existing revisit contract, not an invented first-arrival battle.

## Major behavioral contracts

### Player memory is a domain

[`memory_manager.gd`](../../scripts/systems/memory_manager.gd) stores irreversible burns, fading, erosion, residue, NPC links, guard use, loan collateral/extraction, and preservation progress. Raw grade ordinal 0..4 maps to `GRADE_5..GRADE_1`; changing enum order corrupts meaning in saves. Display rank is separate. Do not add a cumulative-loss gauge.

Normal burn rejects burned/faded/collateral entries as appropriate, sets burned, may emit residue first (Elia present and raw grade ≥2), appends burned history, cascades erosion, emits `memory_burned`, then carry/passive consequences. Silent burn omits residue. Debt extraction has separate extracted history and stronger cascade. Selling uses the burn pathway and therefore has narrative consequences. Residue reuse does not burn again or consume the residue. Synthesis removes two inputs and produces a new memory without treating the inputs as burned.

Connections combine same-NPC links and neighboring same-prefix entries in insertion order. Reordering definitions can change cascades. Carry is derived: weights `[1,2,3,4,6]`, capacity `min(34,14+2*(chapter-1))`; burned/collateral memories are excluded. Excess load multiplies erosion pressure. Loans mature at chapter+2 but extraction occurs only when chapter **exceeds** due chapter. Guards absorb one qualifying erosion/cascade. These distinctions need command-level fixtures.

### World cognition is a different domain

`ActorRegistry → WorldState → MemoryEngine` manages actor-addressed memories and facts. No `MemoryManager` burn is implicit in world removal/restoration. World memory removal retains a tombstone; restoration preserves history. No-op mutations do not increment revision or emit events. Events have deterministic sequence IDs, but EventBus is not an event store.

Malet's route-request knowledge is distinct from his memory of the requester's identity. Legacy route knowledge gains a canonical alias without deleting history or emitting a new mutation event. Ch5 records Kairos's report once from the then-current request and identity evidence. Restoring identity **after** the report must not rewrite that historic report. A future chapter hook is not a fully authored downstream chapter.

### Narrative has two execution dialects

`DialogueManager` runs field arrays with gates, text substitution, choices, optional structured world conditions, and jumps. `SceneFlow` runs VN steps, transitions, ending/checkpoint actions, and save cursors. Their gate/effect ordering and failed-cost behavior differ. VN step effects can execute before a requires-flag gate; field choices and VN choices do not handle payment failures identically. Both use zero-based authored jump destinations through an internal decrement/advance convention. A single generic conversation graph that normalizes these differences would change the game. See [DATA_MIGRATION](DATA_MIGRATION.md).

Ending resolution in `GameManager.evaluate_part3_ending` has priority order: name-burn/Zero → ≥75% burned/Hollow → eligible conversion/Weave → relationship or total-burn threshold/Ash → Tobias witness → Celah/hidden-history/keeper-key Seam → Preservation. There are seven distinct resolver outcomes, while old docs and other legacy/gallery paths use different counts. Inventory all pathways separately; a count is not a reachability certificate. Weave uses intact named anchor memories, fewer than four burns, and at least three secondary anchors. Preserve exact predicates and tie precedence.

### Hidden global coupling

The largest stores are mutable dictionaries, with callers bypassing signals through direct writes. Chapter changes trigger memory grants/erosion, loans, oath rewards, stats, dialogue gates, map travel, and saves. Burn listeners include GameManager, world rewrite, compass, diary, achievements, codex, audio, HUD, archive, and log/toast UI. Some listeners observe intermediate state: normal residue precedes burned-list append; battle start precedes complete encounter setup; battle-ended precedes rewards and cleanup. Listener order and awaited callbacks therefore matter.

PauseMenu pauses the scene tree while retaining the underlying game mode. Timers, always-processing overlays, global Autoload coroutines, and map node destruction have different lifetimes. `get_tree().current_scene`, groups (`player`, `npcs`), metadata, raw paths, node names, and canvas layers are implicit integration APIs. `WorldRewriteDirector` and `MemoryCompass` both call perception application on burn. `PerceptionFilter` hides/disables nodes and replaces dialogue; it does not symmetrically restore visibility/collision within the same existing scene. Port the observed cases; characterize questionable behavior before fixing it.

### Save boundaries

Current slot format is `0.4.0`: slots 1–3 plus autosave 0, JSON backup/recovery, five-minute exploration autosave, delayed map-entry checkpoint, boss/chapter checkpoints. Snapshot contains `game`, `memory`, `world_state`, `scene_flow`, diary, hints, player position, scene, and metadata. It does **not** serialize a live battle, particle/tween state, shuffled puzzle board, or compass/constellation view. Profile data is fragmented across separate files. Missing/current-version malformed fields are normalized; missing destination scene rejects load before run-state imports. Other imports can still mutate state before travel completion, and `load_completed` fires before the awaited transition finishes. [DATA_MIGRATION](DATA_MIGRATION.md) specifies compatibility and a staged UE load.

Potential source defects, not migration authorizations: diary import merges rather than clearing; reset ownership is distributed across title/NG+; synthesis API/UI guards differ; shop stock `sold` fields can be transient; BGM volume handling targets one of two music players in places. These need reproduction/explicit decisions, not silent cleanup during translation.

## Verification performed in this audit

| Command / check | Actual result |
| --- | --- |
| `python scripts/validation/repo_contract.py` | PASS; warning remains for legacy grade labels in user-facing text |
| `python scripts/tools/validate_vn_scenes.py` | PASS: 21 files, 526 steps, zero errors/warnings |
| `python scripts/tools/validate_korean_localization.py` | PASS: 32 files, 1,581 fields, 19 speakers, zero errors |
| `scripts/tools/run_memory_world_engine_smoke_suite.ps1 -GodotPath <4.6.2 console exe>` | PASS: 15 cases, fatal-log scan enabled, guarded test save paths, actor catalog present in exported pack |

The 15 cases cover runner contract, production/outside-temp save guards (expected nonzero exits), crash guards, world memory, five migration fixtures, actor catalog/registry, event schema/consumer, Malet domain/live integration, Sable memory gameplay, canon Wave1 and Wave2A. Exported validation pack: 1,187,832,036 bytes, no export-log errors. Logs are copied to [evidence/validation.txt](evidence/validation.txt). Existing test-root protection was used; this audit did not take a before/after hash inventory of every separate user profile file.

Not performed: full campaign playthrough, all 109 tool scripts, visual/audio capture approval, performance profiling, complete controller traversal, Unreal import/compile/package, or cross-engine comparison. Every Unreal acceptance case in the parity matrix is **planned**, not passed.

## Toolchain and repository readiness

The default Epic installation directory and launcher manifest show UE **5.8.2**, not requested 5.7. No 5.7 install or `.uproject` was discovered in the inspected locations; this was not an all-disk engine search. Godot 4.6.2 and its console binary were used for baseline verification. Target 5.7 remains fixed; compilation is pending a verified 5.7 toolchain. Nothing was installed or upgraded.

Git has about 1.39 GiB of packed objects and Git LFS 3.7.1 available, with no existing `.gitattributes` policy found. Godot CI responds to `main`, `stabilization/**`, PRs and manual runs; pushing the current branch alone is not a CI certificate. Use an isolated Unreal subtree **and** a dedicated migration branch next, as detailed in the roadmap. Preserve this checkout as the runnable reference.
