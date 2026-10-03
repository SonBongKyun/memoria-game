# S344 integration review — Codex (2026-10-03)

Status: **COMPLETE — accepted and freshly verified.** No additional S344 code repair was required. Both registries ran on `07120526d6462d797154ce766b4ef53c87b33532`; the subsequent completion commit changes documentation only.

## Adopted revision and scope

- User-requested source: Claude `277b92e492681c8795cbd35a1c962875054837ab` (S344); peer inspected clean at `d63c47a8`.
- Codex branch: `codex/unreal-s342-integration-env-s343-20261003`, clean base `360ffff23fade6899b06f575a1bf4149f09aa95f`.
- Adopted local code revision: `07120526d6462d797154ce766b4ef53c87b33532`.
- The only cherry-pick conflict was `MIGRATION_STATE.md`. Both the incoming Claude S344 application record and the existing Codex S343 integration record are retained, followed by the previous history.
- S344 adds 27 delivered FBX/PNG files, 20 hydrated Unreal packages, the environment import commandlet and map placement/test changes. It does not change combat, run/save/narrative/inventory logic.

## Review and acceptance

The twelve delivered models now replace the main box stand-ins across Belt Waystation and Drift Shelter. The Belt places signal, platform shelter, crates, fence, lanterns, rails, banner, ruined walls and grass; Drift places tarp canopies, ruined walls, gramophone, dead trees, lanterns, crates and grass. Material atlases are deduplicated; colour is sRGB, glow masks are linear, grass uses two-sided masked alpha. Existing map light/focus-cut integration remains.

The commandlet follows the existing S335 import convention: scene/unit conversion, forced front X and a -90-degree turn, imported normals/tangents, one combined mesh and no generated collision. All delivered dimensions are checked within 1 cm and the models must stand on z=0. Existing packages can be inspected without `-Rebuild`; a rebuild is not needed for adoption.

The map's original navigation blockers remain one per source solid tile with unchanged transforms. Dressing instances have no collision. The existing `CheckChapterVisualPolish` assertions for the exact blockers, polished materials, beveled mesh and stone surface remain intact; the new tests add map kit-kind/placement and lamp counts rather than relaxing those assertions.

Canopies are kept over the rear solid slab row or outside the east border. Their focus opening tracks Arrel only: placing a complete roof across the walkable shelter would hide NPC/foe heads. The two rear awnings use 60% depth to keep the floor open. This is an accepted placement limitation, not a claim of NPC roof-occlusion support.

## Integrity evidence

`Unreal/Memoria/Saved/Validation/s344-integration/asset_integrity.json` records:

- 149 previously checked asset files unchanged from the S343 baseline, including the S337 six polished materials.
- Nine preserved source/host files unchanged, including scorch cleanup, smoke escape chain, swept rush/contact regressions and detailed chapter material hooks.
- 27 new source files match both the shared S343 delivery and the peer checkout byte for byte.
- 20 Unreal packages match the source commit's LFS SHA-256 and size and the peer bytes. All are hydrated; no pointer stubs.

The preservation scope is these 158 baseline files, not a claim of whole-checkout byte identity.

## Fresh verification

- Fresh UE 5.8.2 build: PASS (exit 0; 244.37 seconds including new commandlet/UHT work).
- Fresh rendered `MemoriaVisual.`: **30/30 PASS**, 28 success plus 2 warning-bearing successes; failed/not-run/in-process 0, exact test identities, fatal diagnostics 0. Build and engine processes both exit 0.
- Bounds commandlet: **12/12 PASS**, each imported size within 1 cm of delivery and all triangle counts identical; existing 20 package hashes unchanged, saved packages 0.
- Captures: all 16 chapter-dressing PNGs are fresh and 1280x720. Contact sheet inspected in full, with native-size inspection of BeltPlatform/BeltSignal and DriftTarp/DriftShelter/DriftCamp. The rail line, platform roof, signal mast, freight/fence, ruined wall, grass, gramophone and exterior camp appear as intended; Arrel and the walkable shelter floor remain visible in these views.
- Fresh full rendered `Memoria.`: **274/274 PASS** on the same adopted revision, including all 51 source-parity identities; 266 success plus 8 warning-bearing successes, failed/not-run/in-process 0. Exact identities and all command exits 0; fatal diagnostics 0. Test duration 2118.11 seconds. Historical peer results are not substituted for this run.
- Both successful engine processes retain 15 existing startup `Condition failed` lines each. The full run has six transient no-world-context warnings, one `r.MotionVectorSimulation` warning and one additional `LogHttp` timeout (3 seconds, `https://www.google.com/generate_204`). This is the stock Unreal editor home-screen connectivity probe: `Engine/Source/Editor/MainFrame/Private/HomeScreen/SHomeScreen.cpp:956` sets that exact URL. No occurrence of the endpoint is present in project Source/Tools/Config. The HTTP warning was observed in `Memoria.MaletFirstEffect.PreexistingFalse`; that test succeeded.
- The successful visual engine process retains 15 existing startup `Condition failed` lines. Two warning-bearing tests are Achievements (`r.MotionVectorSimulation` render-thread access) and Chapter1Presentation (a transient object with no world context). These are disclosed separately from successful project tests; an error-free engine startup is not claimed.
- Render material/package diagnostics: authored shader/fallback/package-load failures 0. Five optional engine profiler/audio load messages are retained separately in `render_diagnostics.json`. The first generic-load scan included those engine messages; classification was narrowed to actual material and project-package failures, without changing any test or engine setting.
- One evidence-reading helper initially used Windows' default encoding for the UTF-8-BOM automation JSON. It was corrected to explicit UTF-8; the engine/bounds command had already exited 0 and no game code or assets changed.

Evidence: `Unreal/Memoria/Saved/Validation/s344-integration/visual/unreal_validation.json` points to the unique automation report. Bounds, integrity and capture reports are in the same evidence directory; the raw visual run is `unreal-run-20261003T032133781015`. Full run: `unreal-run-20261003T033225171789`.

## Known limits and ownership

- Dressing has no authored collision. Crates and awning poles can overhang a source solid tile by about 30 cm; Arrel can clip that decorative edge. Source passability is preserved.
- Fence spans adjust length by up to 12%. Repeated end-post geometry is retained from the supplied placement.
- The kit has no LODs, wind/cloth animation, normal maps or separate roughness maps. Distant relay pylons, slag ridges, border walls, embankment and relay-house structure still use primitives where there is no matching model.
- Existing S341/S342 limits remain: guard-only ink, fixed keyboard item slots, six foes using two models, caster orbs through walls and untuned numbers.
- Original Godot, foundation and Claude checkout are read-only for this task. Shared model files and Claude's handoff are unchanged. No model generation, packaging, push or deployment is performed.

Post-run hashes rechecked PASS for all 158 preserved files, 27 delivered originals and 20 adopted packages. `final_results.json` contains the exact report paths and warning events. Peer remains clean at `d63c47a8502ebed9e0b7aa5c26886392042428e8`. No Unreal/editor/compiler processes remained at slot release. Requested shared `codex-review.md`, `BOARD.md` and session records contain the accepted revision and verification result; their previous history is preserved.
