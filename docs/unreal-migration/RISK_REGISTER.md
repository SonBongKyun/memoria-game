# Migration risk register

All risks are open unless marked as a Phase 0 finding resolved by inspection. H = high consequence or broad coupling; M = bounded local consequence. Owners are proposed work areas, not assigned people. Existing suspected defects require characterization; none is silently authorized for correction by this audit.

| ID / level | Evidence and failure mode | Mitigation / owner | Exit evidence |
| --- | --- | --- | --- |
| R01 H | Raw MemoryGrade order, normal/silent/extraction/sale/residue and cascade ordering differ; generic inventory port destroys identity | First-class C++ memory domain, original ordinals/order and command fixtures / Domain | P09–P14 exact state/event traces |
| R02 H | World MemoryEngine and player MemoryManager are separate; Malet report history depends on when identity was removed | Separate domains and frozen report predicate; explicit mutation adapters / Domain + Narrative | P15–P17 before/after report and save fixtures |
| R03 H | Current canon, legacy maps and preview routes coexist; S54/GDD descriptions are stale | Three route cohorts, map aliases, gate inventory; no automatic story reconstruction / Narrative | P02, P42 route graphs and playthroughs |
| R04 H | DialogueManager and SceneFlow differ in gate/effect/payment/index handling | Two typed interpreters, preserve effect phases and original indices / Narrative | P07–P08 golden traces including rejected payments |
| R05 H | VN cursor saves can replay current-step effects; profile data fragmented; imports not fully transactional | Characterize resume boundaries; separate run/profile DTOs; staged load, versioned compatibility decisions / Persistence | P37–P40 fault/reset/resume cases |
| R06 H | Signals observe partial state; global awaits survive scene changes; battle end/reward/cleanup are separate | Explicit event phases, command queue, generation tokens, lifecycle ownership / Runtime | P22, P25, P28, P48 no duplicate reward/stale callback |
| R07 H | 2D collisions/camera/foot sorting and procedural walk frames are engine-specific | One coordinate adapter, custom constrained movement, recorded paths and source-art bake / World + Art | P03–P04, P45 measured match |
| R08 H | Maps and UI are mostly generated in scripts; scene-only or JSON-only conversion loses content | Export grids/population/constants/embedded texts to IR; source coverage manifest / Importer | P33, P46–P47 all references accounted for |
| R09 H | Global random calls mix combat, field and visual synthesis; identical seed is not portable | Capture draw/result traces; deterministic test injection; separate future streams only with behavior validation / Domain | P21–P30 comparable outcomes |
| R10 H | 1.41 GB asset tree, PNG-heavy CG, existing large Git history and future UE binaries | Preserve sources in place, stable imports, scoped LFS for UE binaries, capacity/cook budget / Build | No duplicate originals, clean repeat import, packaged dependency audit |
| R11 H | UE 5.7 not found in inspected install locations; 5.8.2 present | Verify exact 5.7 engine + supported compiler before P1; do not silently retarget / Build | Editor/commandlet + packaged foundation compile on 5.7 |
| R12 H | Shader code, alpha, font variations and decorative 3D SubViewport cannot transfer literally | Material/UI-layer prototype, stable art/camera, weight-instance fonts and screenshot baselines / Art + UI | P43, P45 with Korean and clean/low-motion modes |
| R13 M | InputManager hints are not bindings; raw handlers and nested pause/CG/backlog consume shared keys | Import project mappings and raw-handler inventory; single modal/focus controller; device matrix / UI | P05–P06 no double activation or stuck input |
| R14 M | Generated SFX/stings/ambience absent from asset directories; BGM A/B settings paths differ | Export PCM with provenance; centralized mix adapter; characterize both players before fix / Audio | P44 event/envelope comparison and listening |
| R15 M | Dialogic installed but not used by first-party story; disabled VFX plugin still supplies shaders | Preserve four shader effects; exclude unused runtime framework from UE; keep source addon tree / Importer | Packaged game has required effects and no presumed timelines |
| R16 M | Diary import merges, reset routines dispersed, stock sold flags transient, synthesis API/UI validation differs | Reproduce with isolated paired fixtures; record preserve/fix decision per defect / Domain + Persistence | P14, P31, P40 explicit expected outcome |
| R17 M | PerceptionFilter is one-way per existing scene; compass and rewrite duplicate application | Characterize hide/show/revisit collision semantics; avoid new reactive behavior by assumption / World | P18 scene-local and re-entry capture/collision tests |
| R18 M | Read-history hashes depend on Godot/localized strings; Codex uses display-name keys | Legacy hash/name alias tables, stable new IDs, retain unresolved entries / Importer + Profile | P36, P38, P43 cross-locale roundtrip |
| R19 M | Existing low-motion/clean options suppress some decoration, not every semantic cue | Import actual option effects; test field danger/battle cues under each preset / UI + Art | P20, P45 accessible cue review |
| R20 H before release | Asset generation/source files and font notes do not constitute a complete commercial rights ledger | Preserve all provenance; create license/source register; confirm only unresolved rights before distribution / Production | Identified rights holder/license for shipped art/music/fonts/addon-derived effects |
| R21 M before release | Steam hooks/store actions are placeholders; Godot CI branch coverage is narrow | No claim of working service; configure only if product scope demands; independent UE CI / Build | Actual product configuration and packaged integration evidence |
| R22 H | Passing tests can be overstated: 15 source cases are not a full game or UE test suite | Report baseline/static/planned separately; require successful exit + full fatal scan, no PASS-only acceptance / QA | Every phase publishes bounded evidence and open differences |

## Decisions resolved by Phase 0

- Existing executable behavior is the specification; no gameplay/story simplification or 3D redesign.
- Target remains 5.7. Architecture does not depend on installing a different engine.
- Custom dialogue interpreters own story; importing a Dialogic project is not the migration path.
- Keep source Godot project intact. Recommend isolated `unreal/` directory plus dedicated migration branch; defer historical LFS rewrites/separate repository.
- First integrated slice is Verdan arrival/trade/world cognition plus the existing Ch2-complete revisit encounter contract. Chapter 1 remains VN.

## Human input that may become necessary

No unanswered design preference blocks this completed Phase 0 audit. Before a verified UE build, an unavailable 5.7 installation may require the user's Epic login/license/UAC action; first check existing accessible installations. Before distribution, only genuinely unresolved source-asset rights, service credentials/product IDs, or remote LFS/storage limits need owner input. Do not request these merely to continue local source analysis or importer tests.

Unreal-required technical changes (lifetime/serialization/coordinate/material implementation) are specified here and in the architecture. A proposed correction to an existing gameplay anomaly or route change needs a concrete reproduced case and a separate reviewable decision; elapsed time or an old design document is not approval.
