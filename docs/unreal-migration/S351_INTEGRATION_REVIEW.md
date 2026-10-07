# S351 integration review — spent market props and alley smoke

Claude **37776bae2ccf7e120ce217139bae6253c7ae6ec4** adopted as **de2c249d618869dba26e5652ee5500950d79724f** over **c9e216a42b13d2ff6aceed5267056161da55c5af** in C:/Users/jc/orca/workspaces/Game/memoria-unreal-codex, branch codex/unreal-s342-integration-env-s343-20261003. Cherry-pick applied cleanly, including prior Codex S348 model/scale checks and S349/S350 repairs/tests. Peer documentation159ae961 and top S351 handoff are separate historical results.

## Behavior and source comparison

Barrel/crate each bind independent dynamic wood/iron materials; use sets the existing run flag and darkens actual Tint by .38/.5. The sign marker has its own nonshadow batch, hidden after reading. A newly entered market applies all four saved flags on its first Exploration tick, without another reward. Original collision/story coordinates, rewards, language handling and NPC paths remain unchanged.

Current Godot scripts/utils/map_effects.gd recolours containers and sets alpha .3; opaque 3D dimming is an adaptation. Its current sign implementation leaves the marker visible even after reading, so removal implements the requested port behavior rather than an exact source match. Attribution comments were corrected; no additional product logic repair was needed.

Godot scenes/maps/verdan_market.gd has three southern wisps rising60px at8px/s (7.5s) with alpha .08 fading to zero in4s. The port has no southern alleys: its actual north-building gaps are x−392/54/474, y600/600/590. Fifteen reusable soft translucent quads rise30–210cm and expand/drift, with a13.33s cycle and measured peak alpha .3240. These placement/timing/opacity choices are deliberate 3D tuning, not identical source settings. Components are allocated once; no collision/shadows or per-tick spawning.

## Added native verification

**4f86f001** adds bound wood/iron Tint checks, barrel independence from crate, continuous opacity/height sampling through a full smoke cycle, and actual isolated disk restoration followed by OpenLevel/reentry. **d956930c** corrects the new test setup to save at the fire's valid positive source position and visit fire→crate→sign→barrel before the north capture, below the minimum60-tile random-encounter distance.

All four flags are deliberately cleared after saving, restored by ContinueCheckpoint, then checked on exactly one new presentation in a different world. Loaded materials/hidden marker are verified and all four props retouched on native ticks: no extra Grains, potion or HP and zero uses. Existing actual potion inventory/recent-item/notices, wounded fire, locale, repeat, scale, path/shadow/stop checks remain.

Independent read-only reviewers found no product blocker. The initial serial build passed and Foundation6 passed on4f86f001; its supervisor was stopped before the related suite because review caught test setup coordinates/path issues. Those logs remain in Saved/Validation/s351-integration. Final verification reran after the correction; no initial partial run is claimed as final.

## Fresh execution on d956930c566733dc086cbf6c0cc211611f423aa9

- UE5.8.2 serial one-action build: exit0/fatal0,35.185s. Standard validator subsequently found the build up to date.
- Foundation **6/6**, raw5success+1warning-bearing; related **52/52**, raw50success+2warning-bearing. Related includes audio3+campaign4+shop transactions15+**all30 rendered visual**.58distinct exact IDs, failed/notRun/inProcess0. Full274 was not rerun; Claude's full result is historical.
- Each pipeline checks fixtures, audio source, Verdan story, editor build and automation; all commands exit0/fatal0. Exact selected IDs compared to the registered sets, not just PASS text. Foundation automation3.790s; related203.862s.
- MapInputAndModal: (137.250,83.500)→(307.250,83.500), pressed1/ignored0.
- Smoke:1complete reset,420fading samples, alpha .0004–.3240. Actual disk restore/new-world/materials pass, grants0.
- Prior townsfolk travel435.8cm, minimum sampled separation185.81cm, deterministic crate0Grains/+1potion all pass.
-52fresh PlayFeel1/ChapterDressing PNGs1280×720. Used props/marker and initial/reentry captures inspected; middle/right wisps visible, west partly obscured by the HUD. No claim that all3are simultaneously unobscured. Achievement toast/chapter3 label in reentry capture come from the staged closed-boundary fixture.
- Existing startup Conditionfailed15per pipeline precede the first test; layout1per pipeline and memory-budget messages are disclosed. Warning-bearing Foundation.MapInputAndModal and Campaign.CanonicalPaidRoute have r.MotionVectorSimulation thread-safety warnings; Chapter1Presentation has its existing synthetic missing-world warning. No warning-free claim.

Commands: Build.bat MemoriaEditor Win64 Development -Project=<own project> -WaitMutex -NoHotReloadFromIDE -MaxParallelActions=1, then python -B Unreal/Tools/validate_unreal.py --build-and-test --rendered --engine-root C:/Program Files/Epic Games/UE_5.8 with Memoria.Foundation. (timeout1200) and Memoria.Audio.+Memoria.Campaign.+MemoriaVisual.+Memoria.BattleEntry.+Memoria.ShopTransactions. (timeout1800). Exact commands and logs in the results file.

Raw reports:
- Unreal\Memoria\Saved\Validation\unreal-run-20261007T125303290921\Automation-20261007T125306506078\index.json
- Unreal\Memoria\Saved\Validation\unreal-run-20261007T125417007592\Automation-20261007T125422593061\index.json

## Preservation and handoff

1016unrelated own source/art/content/tool files and82shared primary model files remain hash-identical. No package/importer/model edits. Peer 159ae961f844e3492cd9857b6d472e8aa79b5d02 stays clean and unchanged; shared Claude handoff, original Godot and foundation were read-only.

Versioned S351_INTEGRATION_RESULTS.json contains all58exact identities, commands, exits, traces, diagnostic/report/capture hashes and integrity. Generated full evidence and52native captures: Unreal/Memoria/Saved/Validation/s351-integration. Final documentation commit changes records and one rounding-only attribution comment; executed production/test code remains the tested snapshot.

Build/GPU slot released after own engine/compiler exit; a separate StellaDrift editor is left untouched. No new push, packaging, deployment or peer merge/message. Claude already owns S351; if needed, consume the added validation commits deliberately, without duplicating the source adoption. Existing primitive prop geometry, common NPC faces, pass-through stalls and canopy overlap remain outside this task.
