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


## Phase 1B UE 5.8.2 progress appendix — 2026-09-06

Status: **COMPLETE** for the bounded engine foundation. The user intentionally changed **UE 5.7 -> UE 5.8.2** on 2026-09-06 because 5.7 was an internal baseline rather than a product constraint, 5.8.2 was installed, and no engine-bound migration assets had yet been produced. Earlier roadmap text and reports above remain historical; this appendix and MIGRATION_STATE define the current target.

The dedicated branch is `unreal-migration/ue58-foundation`, from clean `a58b1e15fe2fb6c86568b8df45d279c4f0844ca1`. Build.version was re-read at `C:\Program Files\Epic Games\UE_5.8`: **5.8.2 / CL 56702186**. Real UBT/UHT/C++/module links, Editor load and rendered PIE pass. The exact legacy Automation suite passes **54/54** and the expanded suite **57/57**, including retained UObject/GC lifetime, nondefault SaveGame fields, real map/Enhanced Input/UMG behavior, XY/Z/camera/foot-pivot checks, source-coordinate restoration and one Back dispatch per simulated physical-key press.

Ten genuine packages now exist under `/Game/Tests/Foundation`, including `L_FoundationTest` and `WBP_FoundationModal`. Final captures were visually inspected. Godot official **15/15**, native **51/51**, host validator **12/12**, static **49/49** pass; **4,217 original files** remain preserved. Failed intermediate attempts and one nonblocking engine render-thread warning are retained. No USB hardware or packaged/campaign certification is claimed.

The generalized exact-patch validator replaces validate_ue57.py and preserves exact-name, provenance, historical evidence and line-ending guards. The new report records compiler/runtime defects and fixes without changing oracle expectations. No 5.7 installation, original-checkout changes or push occurred. The technical checkpoint SHA is recorded in the current report/state by a documentation follow-up.

**Phase 1C has not started.** Its exact next task is deterministic starting-memory catalog export into versioned IR, typed Unreal catalog import and repeated-import equivalence for IDs, owned order and semantic content against source hashes. No Chapter 1, Verdan, campaign dialogue, BattleManager or bulk art/audio was migrated. See [PHASE_1B_UE58_REPORT](PHASE_1B_UE58_REPORT.md) and [MIGRATION_STATE](MIGRATION_STATE.md).


## Phase 1C progress appendix — 2026-09-06

Status: **COMPLETE**. On the existing `unreal-migration/ue58-foundation` branch, the actual Godot starting-memory initializer now feeds a deterministic UTF-8/LF versioned IR, strict Python/C++ validation, editor commandlet and the existing typed `UMemoriaMemoryCatalog`. Seven source-ordered definitions include the actually initialized core name memory. Korean authored text is retained as typed localization; derived connections and mutable run state remain separate.

The production asset `/Game/Memoria/Generated/Memory/DA_StartingMemoryCatalog` was saved through UE5.8.2 tooling. First import CREATED it; second import and third-process check-only reload were UNCHANGED without saving, with identical semantic fingerprints and package bytes. Temporary modified IR proved semantic change detection without saving a mutated package. Explicit RunSubsystem bootstrap loads the asset and reproduces source order/connections and representative command/event behavior. No hand-copied content or new domain authority was introduced.

Actual UBT/UHT/compile/link and rendered Editor/PIE pass. Existing **57/57** plus new **3/3** = **60/60** Automation pass. Godot **15/15**, native **51/51**, host **23/23**, static **50/50** pass; all **4,217 original protected files** remain unchanged. Failed first compiler evidence and one nonblocking engine warning are retained. Original Godot, accepted foundation behavior/assets and historical reports/evidence are preserved. Local technical checkpoint and documentation follow-up only; no push. See [PHASE_1C_REPORT](PHASE_1C_REPORT.md) and [MIGRATION_STATE](MIGRATION_STATE.md) for checkpoint SHA, source/IR/asset hashes and exact commands.

**Phase 1D has not started.** Recommended next bounded task: shared narrative provenance/text/step IR with separate field/VN import contracts, using `data/vn_scenes/ch2_market_arrival.json` and only `verdan_arrival` in `data/chapter2_dialogue.json` as initial fixtures. Preserve original indices, choice order, distinct gate/effect phases and VN continuation; verify unchanged reimport. Then plan the canonical Verdan arrival playable slice. No campaign presentation, Chapter 1 rebuild, maps, battle or bulk graphics/audio was implemented here.

## Phase 1D actual progress — 2026-09-06

Phase 1D is COMPLETE on UE5.8.2 / CL56702186. See [PHASE_1D_REPORT](PHASE_1D_REPORT.md) and [acceptance](evidence/phase1d/acceptance.json). Starting clean `4edb035`, this task imported only `ch2_market_arrival` (VN 13 steps, original 0–12) and `verdan_arrival` (Field five rows, original 0–4). Versioned canonical IR, provenance, strict source attestation, separate typed assets/interpreters, original/visible choice indices, dialect-specific gate/effect/cost behavior and existing continuation DTO schema1 are validated. No campaign/UI/map/audio boot was added.

Both assets were CREATED then UNCHANGED without saves in separate reimport/reload processes. Semantic fingerprints and observed package bytes match. Modified/synthetic test IR cannot pass production source attestation. Ten source-authentic oracle cases and eight new Unreal tests pass; previous60/60 + new8/8 = total68/68, exact identities enforced. Real UBT/UHT/compile/link, Godot repo/VN/KO/import and official15/15, native51/51, starting-memory catalog, host35/35, static52/52 and original4217-file preservation pass. Initial oracle adapter failures and a Godot editor access violation are retained; the unchanged-source Godot retry passed. Local technical checkpoint is recorded in PHASE_1D_REPORT/MIGRATION_STATE by a documentation follow-up; no push.

**Next recommended task: Phase 1E, minimum source-faithful ch2_market_arrival → Verdan arrival development slice.** Use the imported VN asset and existing 2D movement/input/camera with minimal temporary presentation. Actual Verdan code skips `verdan_arrival` when `ch2_arrival_vn_seen` is true, so preserve that branch and enter free exploration after VN. Exercise the imported Field asset via a separate arrival fixture with VN-seen false. Do not force VN then Field onto the canonical campaign path; do not expand into Chapter1, Malet/battle, final art/audio, or bulk migration. Phase 1E was not implemented in Phase 1D.

## Phase 1E actual progress — 2026-09-07

Phase 1E COMPLETE: explicit ch2_market_arrival development VN entry now travels
to a minimal Verdan host. Terminal original 12 sets ch2_arrival_vn_seen before
travel; the actual arrival guard skips Field (0 invocations), then enables
exploration. A separate VN-unseen fixture displays all five imported Field rows
once and returns to exploration. Typed production assets, the existing memory
catalog/run owner and Enhanced Input/camera are reused. Temporary native UMG
only displays values/forwards intent; schema1 continuation round-trips in PIE.

Actual UBT/UHT/compile/link/Editor/rendered PIE PASS. Previous68 + new4 = 72/72
exact Automation identities pass; source route4 and prior narrative10 oracle
cases, six unchanged narrative imports, Godot official15, native51, host38,
static56 and original4217-file/13-package protection pass. Failed attempts and
actual rendered captures are retained. Report/checkpoint: [PHASE_1E_REPORT](PHASE_1E_REPORT.md).
Local checkpoint only, no push. No production New Game/Ch1, NPC/trade/battle,
final UI/art/audio or package/cook was added.

Next Phase 1F recommendation after source dependency review: the paid arrival's
first Malet interaction through `malet_taste_burned` (three authored rows) back
to exploration, including the source reaction priority/one-time flag. Normal
Malet trade pulls in world seeding, rewards, shop and chapter/autosave hooks;
New Game pulls in reset/profile/inventory and the preceding Ch1 VN chain.
The smaller first memory-reaction dependency is recommended, with fallback
dispatch characterized and no silent route change. Phase 1F remains unimplemented.


## Phase 1F actual progress — 2026-09-07

Completed paid arrival's first memory consequence: actual VN payment, native
Verdan handoff, walk/Interact with one Malet placeholder, source reaction priority,
heard flag before Field.Start, imported `malet_taste_burned` three rows, then
restored exploration. Reused typed IR/import, memory domain, Field, input and UMG.
Added only a small interface/component and one NPC presentation boundary.

Seven-case real NPC/PerceptionFilter oracle PASS. Intact/already-heard normal
targets are recorded/deferred; no transaction group or subsequent chain imported.
Previous72+new4=76/76 UE tests PASS; UE5.8.2 build/rendered PIE, Godot official15,
native51, old narrative six-process no-op/10cases, route4, catalog, host45,
static59 and4217-file protection PASS. Failed attempts/real captures preserved.
[Report](PHASE_1F_REPORT.md); local checkpoint in MIGRATION_STATE. No push/Phase1G.

Next bounded recommendation: normal first Malet encounter, original refusal,
source response/cleanup, exploration/retry. Preserve both authored choices;
characterize Accept in oracle and defer whole selection before partial effects.
Deal/reward/shop/world seeding/Chapter3/autosave/achievements remain later work.
Full Malet/Verdan parity is not established.


## Phase 1G checkpoint — 2026-09-07

COMPLETE: source-attested malet_encounter10 + malet_refused3; canonical original
refusal1 through actual0.3s callback, source cleanup, exploration/movement and
ordinary retry at original0. Accept0 stays visible and is deferred before any
playable choice effects. Previous76 + new6 =82/82 rendered UE5.8.2 Automation.
Godot15, native51, host50, static59; original4217 files and17 accepted packages
preserved. See [Phase1G report](PHASE_1G_REPORT.md) and [current state](MIGRATION_STATE.md).
No push; failed attempts and raw logs retained.

Next recommendation, not authorized/implemented: Phase1H Accept payment -> only
malet_deal originals0..4 -> real0.5s completion -> malet_reward request boundary.
Do not execute reward/world/shop/Chapter3/autosave/achievements/repeat chain.

## Phase 1H accepted — 2026-09-08

The actual paid-VN/reaction/normal Accept0 now sets accepted then burns the sword
once in the same run/domain. Real0.3s callback starts the only new typed group,
`malet_deal` originals0..4; its separate0.5s callback requests `malet_reward`
and stops before execution. UE5.8.2 build/rendered PIE90/90, source/regression
oracles, Godot15, native51+CTest1, host55, static59/original4217 and existing19
packages pass. Six required captures and integer-microsecond/full-snapshot
evidence verified; failures retained. See [Phase1H report](PHASE_1H_REPORT.md).

Next recommendation only: Phase1I may import/execute reward originals0..7 and
stop before `_on_reward_ended` first effect. World seeding, item grants, shop,
Chapter3, autosave and achievements remain deferred. No Phase1I implementation.

## Phase 1I accepted — 2026-09-09

COMPLETE: canonical paid VN/native travel/physical walk/reaction/Accept/payment/
real0.3s/deal/real0.5s now enters the only new typed group `malet_reward` originals0..7.
Field completion and synchronous source exploration/callback order are recorded;
the development modal stops before `_on_reward_ended` first effect `ch2_malet_done`.
No world-memory seeding/items/shop/Chapter3/autosave/achievement executes.

UE5.8.2 build/rendered PIE previous90+new7=97/97; independent193 checks; reward source8
and H9/G7/F7/E4/D10/C7; Godot15; native51+CTest1; host61; static59/original4217 and old20
packages PASS. New package import CREATED then two fresh UNCHANGED/no-save passes.
Full run/domain/memory/flags/inventory preserved. Existing warning and test-name
semantic debt documented. [Report](PHASE_1I_REPORT.md). Local checkpoint only; no push.

Next recommendation only: separately authorize Phase1J to apply just the first
`ch2_malet_done` effect and stop before world seeding's first mutation. Preserve97
identities and lifetime/Refuse/payment/reaction evidence. No Phase1J implementation.


## Phase 1J accepted boundary — 2026-09-09

Phase1J is COMPLETE on UE5.8.2: same canonical reward run commits only
ch2_malet_done=true through the authoritative run API, then stops before world seed.
No new narrative package;21 package bytes and76 prior IR/fixtures preserved.
Old97+new10=107/107 rendered Automation, independent358 checks, sourceJ12 and all
previous oracles, Godot15/native51+CTest1/host69/static59/original4217 PASS.
Exact source setter/seed cases, pending-world and run replacement ownership, historical
test-name debt and currentChapter1 fixture debt are documented in PHASE_1J_REPORT.md.
Phase1K is recommendation only: bounded source seed, stop before potion2. No implementation
or remote push is included. The local checkpoint is the enclosing first-effect commit.


## Phase 1K bounded seed — 2026-09-09

COMPLETE: all bounded acceptance PASS. Same canonical route commits
only guarded source Malet knowledge then route memory after done=true, then stops
before potion2. Separate typed run-owned World Cognition survives native travel and
roundtrips in existing schema1 WorldCognition.SourceJson. Defaults retain four actors
and fact.veil.exists; source canonical revision/event_sequence0 ->1 ->2. Removed,
restored and forgotten records persist; repeat/missing actor/false flag no-op.

Source13 + previous J12/I8/H9/G7/F7/E4/D10/C7, Godot15, native51+CTest1 and host76 PASS;
new18 focused PASS, full prior107+18=125/125 PASS; independent437 checks/399 JSONs; static60/original4217 PASS.21 package bytes and80 prior
IR/fixture files unchanged; no new narrative content. [Phase1K report](PHASE_1K_REPORT.md).
Only local checkpoint after acceptance, no push. Full world/Malet parity not claimed.

Phase1L recommendation only: exact first potion2 inventory call (including its own
recent-items/signal/toast behavior), STOP before antidote1. No further reward/shop/
chapter/autosave/achievement effects implied; no Phase1L implementation.


## Phase 1L first potion boundary — 2026-09-10

Current bounded implementation: same canonical paid run now executes source potion2
inventory/recent/signal/toast after the accepted Malet world seed, then defers BEFORE
antidote1. No new narrative package or downstream effect. Existing125 identities plus
22 new =147 rendered Automation PASS; independent728 and sourceL16 plus all prior
oracles PASS. Existing21 packages and82 prior IR/fixtures unchanged. See
[Phase1L report](PHASE_1L_REPORT.md) for final gates, lifetime/source differences and
full logs. Local checkpoint only after all acceptance, no push.

Phase1M recommendation only: exact antidote1 call, then STOP before firebomb1.
No Phase1M implementation, shop/trade/chapter/profile or full inventory parity claim.


## Phase 1M — Antidote grant / pre-firebomb boundary (2026-09-11)

Same real paid VN/food/native travel/physical walk/reaction/Accept/sword/timers/deal/
reward/done/world seed/potion route now executes only source add_item("antidote",1).
Run items={potion:2,antidote:1}, recent=[antidote,potion]; exact item-only signal then
+1 Antidote/SUCCESS1 toast, with the earlier +2 Potion request retained in order.
Shared internal grant code preserves source zero/negative/repeated/recent behavior;
16-ID source membership remains distinct from two authorized entry points. Source
raw recent import and normalized read-only query are separately tested.

Full Player/derived and World revision2/sequence2 plus identities stay unchanged.
Actual signal observer replacement, independent schema1 binary save (13,009 bytes,
zero restore events), teardown and absent/removed presentation are covered. Original
potion-complete full-state/source-prefix assertions remain at their exact logical
boundary; only final development stop advances. Existing147+new22=169 Automation
PASS; independent1,049 evidence checks PASS. Final remaining gates and exact local
checkpoint are recorded in [PHASE_1M_REPORT.md](PHASE_1M_REPORT.md).

Firebomb, shop and all downstream chapter/autosave/achievement/map effects remain
unexecuted. Zero new narrative IR/asset/package;21 prior packages and84 entering
IR/fixtures protected (Phase1L quoted82 plus its two potion fixture JSONs). Minimal
UI retains both toast requests; source visual animation queue is not implemented.
Recommend separately authorized Phase1N only firebomb1/recent/signal/toast, then stop
before shop opening. No Phase1N implementation, push or full Inventory/Shop parity.


## Phase 1N — firebomb grant verified; final source byte gate pending (2026-09-11)

The same canonical run now adds source firebomb1 after potion2/antidote1 and stops
before `_open_malet_shop()` entry, including stock construction. Items/recent, three
ordered actual item-only signals and SUCCESS1 requests, full Player+derived/World
preservation, each actual signal replacement and schema1 independent binary13,121 bytes
are verified. Potion/Antidote complete logical assertions and exact169 prior identities
remain; final UE191/191, independent1,413, host96, sourceN17 plus all historical source
checks, official Godot15 and native51+CTest1 PASS. I first attempt PASS; no retry.

No new narrative IR/asset/package. Prior86 IR/fixtures,21 packages and8,405 protected
worktree files match. Static original-byte preservation fails only on original
`assets/fonts/theme.tres`:103 CRLF endings became LF, identical normalized content.
User approval for narrow restoration was requested; no original write or new local
commit/push occurred. Completion is withheld until the byte gate is resolved.
[Current report](PHASE_1N_REPORT.md) includes exact source hashes, raw original captures,
first attempts, one corrected capture-helper compilation failure, warnings and the
preview/pixel reassessment. Source animation queue, item use and all shop/downstream
chapter/persistence behavior remain outside scope. Phase1O is not implemented.


### Phase 1N closeout update — 2026-09-13 KST

The above September11 pending state is retained as history. User-approved binary
restoration of only the original Godot `assets/fonts/theme.tres` recovered103 CRLF
pairs (3,522→3,625 bytes) and the exact old manifest SHA-256. The worktree theme was
not rewritten. Fresh static60/original4217 and worktree8405/package21/prior IR-fixture86
preservation pass; all historical evidence remains intact and new IR/asset/package0.
Existing UE191/191 and independent1413 results are reused from September11 after exact
runtime-input/package/fixture checks; no new full UE execution is claimed. Both recovery
auditor baseline-scope mistakes and their read-only resolutions are retained.
Final diff/staged raw-byte checks pass; Phase1N is complete in the local checkpoint
containing this update. Original change author remains unknown. No push or Phase1O.
[Recovery and final review](evidence/phase1n/recovery-20260913/final_review.json).


## Phase 1O checkpoint — 2026-09-14

The user authorized the first shop screen and gradual source-art integration.
The canonical route now reaches Malet's default sell view, with read-only memory
inspection and two original-art texture packages. Fresh UE203/203 and independent1500
checks pass. Transactions, close callback and chapter3 remain deferred. See
[Phase1O report](PHASE_1O_REPORT.md) and [play instructions](PLAYABLE_SLICE.md).
Next work should combine one reviewed shop transaction with visible screen progress;
expand original portrait/backdrop presentation as connected gameplay is verified.
