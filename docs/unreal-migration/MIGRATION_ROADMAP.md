# Bounded migration roadmap

Phase 0 is complete as an audit/design deliverable. No UE gameplay has been implemented. The next implementation task is P1 below; do not use this roadmap as authorization to mass-port all chapters now. Each subsequent task completes one reviewable slice and records actual evidence in SESSION_LOG.

## Source-control decision: A plus B

Recommend **an isolated `unreal/Memoria/` project inside this repository (A), developed on a dedicated `codex/unreal-5.7-foundation` branch (B)**. Create that branch in the next implementation task; no branch/commit/tag/push was performed by Phase 0. The original root Godot project remains runnable and its original asset paths stay in place.

| Option | Benefit | Cost / decision |
| --- | --- | --- |
| A: isolated directory | One source/content/fixture history; relative import roots; easy paired reference tests | Add Godot scan/export exclusions, UE generated-file ignores and separate CI. Chosen. |
| B: dedicated branch | Reviewable migration history, independent checkpoints, no accidental replacement of reference runtime | A branch alone does not isolate import scans/build outputs and can drift from source updates. Use with A. |
| C: separate repository | Independent permissions, binary storage/CI/release cadence | Duplicated history/assets or a new versioned external-source distribution contract; harder atomic source/fixture updates. Defer unless measured LFS/storage/team requirements justify it. |

Proposed layout, not files already created:

```text
Game/
  project.godot, scripts/, scenes/, assets/, data/  # retained Godot reference
  docs/unreal-migration/                          # this audit and future decisions
  unreal/
    .gdignore
    Memoria/
      Memoria.uproject                            # EngineAssociation 5.7
      Config/
      Source/MemoriaRuntime/
      Source/MemoriaEditor/
      Source/MemoriaTests/
      Content/Memoria/{Generated,UI,Maps}/
    Tools/                                       # import/compare/package adapters
```

Before the first UE project import, add `.gdignore` and explicit Godot export exclusions for `unreal/**`; add scoped Git ignores for UE Binaries, Intermediate, Saved, DerivedDataCache, IDE/local build outputs. Existing Godot export mode includes resources broadly, so `.gdignore` alone should not be treated as a verified export boundary. Check actual packaged contents in both engines.

Use Git LFS for **new** `unreal/**/*.uasset` and `unreal/**/*.umap` under a scoped `.gitattributes` policy, after verifying remote capacity. Do not rewrite existing Git history or move every PNG into LFS during this task. Retain original large images/music in their existing tracked locations; importer reads them there and produces only needed engine packages. Ignore conversion caches and raw duplicates. Avoid command-line blanket staging; inspect staged names and `git diff --cached --check` before any later authorized commit.

Record reference HEAD and working-tree diff hashes in each fixture/import manifest. A clean reference worktree at the recorded commit can support repeatable tests, but this checkout's pre-existing changes must be preserved separately. A checkpoint tag or promotion merge is a later explicit shipping action, not something Phase 0 performed.

Keep Godot CI intact. Add independent UE5.7 compile/Automation/cook jobs for migration paths and PRs using a host with the correct engine/license/toolchain. Current Godot branch triggers do not cover every branch push. Never treat a local PASS marker or editor launch as a release pipeline.

## Representative slice: Verdan and the price of remembering

Use the actual `ch2_market_arrival` → Verdan transition as the visible entrance. A development fixture imports a state produced by each Ch1 food/song/sword branch, preserving the resulting memory/flags/party. Walk, interact with Malet, make the existing transaction, inspect the archive, and exercise Malet's separate request knowledge and identity memory. Compare removal/restoration effects through existing authored dialogue/integration fixtures; do not add development dialogue to the shipped campaign.

For the combat leg, use a **Ch2-complete revisit fixture**, because Verdan's random Alley Rat/Market Thief encounters are gated to revisits; there are no authored market hunts. Use a recorded encounter draw and movement distance to enter that existing fight. Prove attack/guard/witness and one chosen memory burn with Elia/residue consequences, rewards, return to field, archive/world-rewrite feedback, then save/quit/load. Two fixtures—arrival and eligible revisit—prove real contracts without inventing a battle on first arrival.

The memory burned for each fixture must be selected from that fixture's actual intact/noncollateral memories; record its ID, rank, power and consequences in the expected trace. Never silently replenish burned memories to make a demonstration work. Malet identity deletion is a separate World Memory operation, not the combat burn cost.

Acceptance: P03–P10, P15–P19, P21–P24, P26, P28, P31 and P37–P39 for the bounded cases above; exact run/world state and ordered events; paired field/dialogue/archive/battle/CG captures; separate save/profile roots; clean packaged execution and no missing assets. Optional sites, all other battles and later chapters remain queued until this slice passes.

## Phases

### P0 — repository forensics and design (this task)

- **Objective:** identify executable scope, dependencies, data/save/assets and migration risks without production rewrite.
- **Files/systems:** the eight audit documents, evidence collector/reports, SESSION_LOG; source read-only.
- **Work/automation:** source/scene/data/asset indexing, boundary implementation review, existing guarded validation.
- **Dependencies:** accessible Godot checkout/4.6.2 reference.
- **Acceptance:** every first-party runtime source and every JSON/scene has a disposition; source vs planned verification clearly separated; route discrepancy and 5.7 toolchain gap recorded.
- **Failure modes:** documentation mistaken for behavior; passing subset mistaken for complete game; initiating bulk conversion. These are explicitly excluded.

### P1 — isolated 5.7 foundation and parity harness

- **Objective:** build a minimal Unreal-native shell and repeatable comparison harness, with no chapter port yet.
- **Files/systems:** new isolated project/modules/config/test map; ignore/export/LFS/CI boundaries; existing reference-fixture tooling remains available.
- **Implementation:** verify engine/compiler, create thin GameInstance/GameMode/PlayerController, constrained 2D test Pawn/camera, Enhanced Input contexts and one UMG modal host; introduce empty run/profile/save DTO contracts and injected test storage roots. Implement dependency/lifetime bootstrap and log/exit checks.
- **Automation:** source-manifest schema, reference-output format, one generated test asset import; commandlet runner and semantic snapshot comparison. Do not extract every chapter now.
- **Dependencies:** P0; verified UE5.7 toolchain. If login/UAC is required, finish reviewable local files and state the exact external action needed; do not substitute 5.8.
- **Acceptance:** 5.7 editor/runtime compile; Windows development package launches without source tree; test pawn moves on the declared plane; modal consumes Back once; save adapter writes only injected temp root; both Godot export and Git status exclude UE generated outputs; P01/P05/P46/P48 foundation subset.
- **Failure modes:** wrong engine, default 3D movement, world-owned persistent state, editor-only modules in package, collision units inverted, both UI/game consuming input.

### P2 — memory/run/world state vertical tests

- **Objective:** reproduce memory consequences independently of a large map/UI port.
- **Files/systems:** Run/PlayerMemory/WorldCognition/Progression C++ domains, actor/memory definition assets, events, pure rules tests and isolated Godot exporters.
- **Implementation:** original ordinals/IDs, burn/silent/residue/cascade/erosion/carry, loan/guard/extraction/preservation; GameManager state/flags; world mutation/conditions and schema; minimum economy operations used by slice. Explicit ordered effects and reset/import contracts.
- **Automation:** extract ordered definitions and original fixtures; produce command/state/event golden traces. Flat items/actors to typed tables, complex memories to assets.
- **Dependencies:** P1 lifetimes, content manifest and storage isolation.
- **Acceptance:** P09–P17/P32/P37 core fixture subset exact; no world removal invokes a player burn; no-op world command has no event; all raw grade/import boundaries pass.
- **Failure modes:** enum reorder, graph order drift, collateral treated as burned, event subscription ordering changes, false/missing fact collapse, resets contaminating a second run.

### P3 — Verdan exploration, field dialogue and transaction

- **Objective:** playable arrival half of the representative slice.
- **Files/systems:** Verdan map/grid/visuals/population, Pawn/NPC/companion components, field dialogue importer/interpreter, archive/CG/market UI and Malet world consequences.
- **Implementation:** actual 2D geometry, motion/interaction/camera, two dialogue dialect boundaries established but only required VN arrival/field groups imported; same choice/payment/flag/portrait timing; precise transaction consequences and reactions.
- **Automation:** one-map grid/population export, required texture/sprite/font conversion, field/arrival JSON import and choice-path fixtures.
- **Dependencies:** P2 domains, P1 UI/input/import skeleton.
- **Acceptance:** P03–P08/P16/P18–P19/P31 arrival subset; food/song/sword-derived inputs retain distinct results; screenshots with Korean text and clean default visuals; no invented market hunts.
- **Failure modes:** scene shell produces empty map, premature Ch2-complete flag, incorrect sprite foot pivot, missing embedded NPC text, changed failed-payment branch.

### P4 — existing revisit battle plus save/return loop

- **Objective:** finish the representative slice as a packaged playable loop.
- **Files/systems:** Verdan gated random encounters, BattleSession/rules/UI/VFX/audio, current memory commands, SaveGame adapter, return travel/reward UI.
- **Implementation:** one recorded Alley Rat/Market Thief fixture, attack/guard/witness/burn/residue, required enemy ability/status and Elia behavior, rewards/cleanup; manual+auto slot semantics and legacy source-state import. No claim that unported skills/bosses are done.
- **Automation:** deterministic draw tape, combat state/event fixtures, source PCM/art conversion for this fight, backup/failure/roundtrip fixtures, packaged smoke.
- **Dependencies:** P3; source revisit eligibility and expected battle traces captured first.
- **Acceptance:** representative slice gate above, including valid/invalid save, quit/reload, separate player/world memory and profile handling; P22–P24/P26/P28/P37–P39 bounded cases.
- **Failure modes:** identical-seed assumption, integer truncation drift, residue consumed, duplicate rewards on dismiss, autosave captures wrong destination, stale callback after map unload.

### P5 — complete current Chapter 1 and accept the entrance route

- **Objective:** run New Game through every Ch1 VN branch to the already proven Verdan arrival.
- **Files/systems:** title/reset/profile, all Ch1 VN and Ch2 arrival assets, SceneFlow interpreter, VN/CG/backlog/read-hash/font/audio presentation.
- **Implementation:** all existing step/choice/effect/continuation/tag behavior needed by Ch1; resume/save at branch/effect boundaries; new/unread skip and locale handling. Ch1 beast stays VN.
- **Automation:** import Ch1 without manual transcription, enumerate branch/goto graph, generate checkpoints and legacy hash mappings.
- **Dependencies:** P4 accepted state/save/presentation boundaries; full VN dialect tests P08.
- **Acceptance:** P01–P02/P08/P38/P43/P45 for every Ch1 branch, then representative field loop; no branch collapse or extra combat; approve paired timing/composition/audio evidence before bulk content.
- **Failure modes:** resume replays effects, skip bypasses unread text, filtered choice index wrong, new-game reset leaks old state, legacy forest route replaces current VN.

### P6 — current Ch2–Ch5 route, one chapter per task

- **Objective:** migrate current canon field continuation and the frozen Ch5 report boundary.
- **Files/systems:** remaining Verdan groups, Belt Waystation, Drift Shelter, Ch5 classifier entry/VN, relevant memory/population/quest/party/optional gates.
- **Implementation:** finish one chapter's script/visual/dialogue/exit/save matrix, then proceed to the next. Preserve `canon_ch6_seam_ready` return boundary.
- **Automation:** reuse map/dialogue importers, export chapter-specific trigger manifests, port Wave1/Wave2A/Malet/Sable fixtures into UE tests.
- **Dependencies:** P5 accepted entrance route and P15–P17 world contract.
- **Acceptance:** P16–P18/P33/P38/P42 chapter-specific variants and save/revisit cases; no automatic connection to legacy coast/Seam after classifier.
- **Failure modes:** report computed from current rather than historical evidence, gates broadened by chapter number, optional site bypasses canon boundary.

### P7 — remaining retained routes and combat breadth, one bounded cohort per task

- **Objective:** preserve legacy map/save destinations and explicit preview VN content without altering their entry policies.
- **Files/systems:** remaining core/optional maps, legacy BL07/epilogue, Ch11–24 preview, remaining enemies/bosses/skills/allies/modifiers/environments/endings.
- **Implementation:** migrate one map/encounter family or VN chapter at a time; complete combat ability, status, echo, chain, limit, directive, stance, ally, auto and rush coverage; preserve Ch18 demo gate and all ending precedence.
- **Automation:** full retained asset/scene alias resolution; graph-based route fixtures; formula/AI trace generation; ending truth-table cases.
- **Dependencies:** P6 current route stable, P4 combat/save architecture accepted.
- **Acceptance:** remaining P21–P30/P33/P41–P42/P46 cases; every retained route and old valid save destination accounted for; “authored but gated” stays distinguishable from absent.
- **Failure modes:** claiming all endings from a single resolver test, using legacy map count as canon progress, omitted ability/default branch, action-chain cleanup errors.

### P8 — secondary-system completion

- **Objective:** complete the systems only minimally exercised by earlier slices.
- **Files/systems:** complete archive/constellation/compass, puzzle, shop/loan/synthesis/upgrades/quickslots, all quests/resonances/curios/caches, diary/tutorials, atlas/minimap/journal/codex/achievements/backlog, statistics/galleries/NG+/rush menus and options.
- **Implementation:** finish every remaining source row in SYSTEM_MAP; keep derived views derived and profile/run boundaries explicit; characterize R16 source anomalies before changing behavior.
- **Automation:** import embedded tables/text/gallery manifests, screen/view-model fixtures and cross-slot/profile tests.
- **Dependencies:** target commands/content from P2–P7; earlier slices should already implement secondary rules required for their outcomes.
- **Acceptance:** all P12–P14/P18–P20/P30–P36/P40/P43–P45 cases, nested input and save/reopen scenarios included; no system silently replaced with a generic placeholder.
- **Failure modes:** treating constellation as progression tree, compass as navigation, missing profile records, puzzle duplicate reward, oath exclusivity invented, controller-only softlocks.

### P9 — parity acceptance, performance and release readiness

- **Objective:** certify the retained experience on packaged UE5.7, then prepare a separate concrete release decision.
- **Files/systems:** all imported content, presentation/audio/settings, packaging/CI, licensing/provenance and integration configuration.
- **Implementation:** fix measured parity defects, long-run leaks and load-time/memory problems; use source visual direction and accessibility defaults; configure external services only if required and credentials provided.
- **Automation:** all accepted fixture suites, full fatal-aware package smoke, route/ending traversal, repeated travel/load/menu soak, missing-asset/cook audit, performance captures.
- **Dependencies:** all required matrix rows accepted or explicitly dispositioned; exact toolchain and shipped-asset rights verified.
- **Acceptance:** every parity row has actual UE evidence; source/UE comparison artifacts reviewed; no unresolved critical behavior/save/data loss; performance budgets measured on declared hardware, release package independently tested. Publishing/merging remains a separately reviewable final action.
- **Failure modes:** optimizing away atmosphere/choices, treating a cook PASS as gameplay certification, missing dynamically loaded art/fonts, falsely reporting Steam integration, deleting the reference too early.

## Exact next implementation task

> Implement P1 only: verify a usable Unreal Engine 5.7 toolchain; create an isolated `unreal/Memoria` C++ project on `codex/unreal-5.7-foundation`, preserving all existing Godot changes and sources. Add explicit Godot/UE build and Git boundaries, thin lifetime owners, a constrained 2D test Pawn/camera, Enhanced Input plus one UMG modal, empty versioned run/profile DTOs with injected test storage, and a fatal-aware Automation/package harness. Compile and package with 5.7, demonstrate movement and single Back consumption, and prove no Godot production save path or UE generated directory enters reference exports. Stop at that reviewed foundation; do not port chapters or mass-convert assets.

If an exact 5.7 build is unavailable, the next agent should first inspect accessible engine installs and compiler configuration, prepare the bounded project/harness where feasible, and report the concrete install/login/UAC blocker. UE5.8.2 being present does not authorize changing the requested target. No human clarification is required to understand this architectural task.


## Phase 1A progress appendix — 2026-09-06

The Phase 1A user instruction narrowed/advanced the next delivery: use `Unreal/Memoria/` on `unreal-migration/foundation`, implement the first player-memory domain subset and offline parity fixtures, and exclude packaging/full UI/campaign conversion. The preceding roadmap is preserved as the original strategy; this appendix records actual implementation only.

Status: **PARTIAL, pending exact UE5.7 engine validation**. The isolated worktree is `C:\Users\jc\MemoriaMigration\foundation`. Runtime/editor-test modules, scoped Git/LFS/Godot boundaries, 2D component/input foundation, run-owned player-memory domain, typed run/save/narrative contracts and deterministic source parity fixtures now exist. Native MSVC memory parity passes 51/51 cases (95 commands, 109 event observations); Godot repository/VN/Korean checks and the official 15-case memory/world suite pass. Source-preservation static checks pass. These results do not complete original P1's real-editor modal/input demonstration or certify UE compilation.

Only UE5.8.2 was found in the inspected engine locations; no engine install, retarget or UE build was performed. The exact next Phase 1B task is to compile with 5.7 and pass all 54 authored Automation tests, then create the first 2D input/modal test map and validate plane, camera, foot pivots, collision and single Back consumption. No content bulk import or new Ch1 battle is authorized by this milestone.

See [PHASE_1A_REPORT](PHASE_1A_REPORT.md) for implemented rules, evidence and deviations, and [MIGRATION_STATE](MIGRATION_STATE.md) for commands and handoff.


## Phase 1B progress appendix — 2026-09-06

Status: **PARTIAL**. Phase 1A was checkpointed locally as `2b2a2607296faa1de2f4e8bf94f4eec847bfc8f9` after review, 47 static/preservation checks and 51 native parity cases passed. The previous roadmap and Phase 0/1A reports/evidence remain intact.

Actual Phase 1B work: exact-engine reinspection; corrected Automation report acceptance to require all 54 unique expected tests; fixture freshness before UBT; separate timestamped/explicit evidence destinations; checkout line-ending attributes preserving the original attested fixture bytes; 10 passing host-side validator regressions. Native memory parity remains 51/51. Godot repository, VN, Korean, editor import and official 15-case memory/world suite pass; exported catalog has zero export-log errors in this run.

UE5.7 was not found in the inspected Launcher/registry/environment/common locations; only Build.version-confirmed UE5.8.2 / CL 56702186 is present. Therefore UBT, UHT, compile/link, editor launch, all 54 Unreal Automation cases, SaveGame/GC proof, real test-map/input/modal creation, rendered plane/pivot checks and Back consumption remain unexecuted. No production C++, Godot gameplay, oracle expectation or campaign asset was changed. No substitute engine, installation or remote push was used.

The next action remains completion of Phase 1B with actual UE5.7. Phase 1C is gated: begin with deterministic starting-memory catalog IR/import and repeated-import equivalence only after the real foundation tests and map/input/modal proof pass. See [PHASE_1B_REPORT](PHASE_1B_REPORT.md) and [MIGRATION_STATE](MIGRATION_STATE.md) for exact evidence, checkpoints and commands.
