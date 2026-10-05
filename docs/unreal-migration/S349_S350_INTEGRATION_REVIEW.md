# S349/S350 integration review: Verdan movement and props

Claude **f3c1ed541ab205257f181e51b438559b5c65e4ed** and **1068d8af2770125f288d11de49ab400a2dd377c8** are adopted as **f2e43e932558a25d3f267398ccf85c552a55b056** and **95a2fb0af35c4f8c6b8910ba0f2e47bef20ac293** over **cf358235abb168912ffa1ab7105100bd8076f66e**. Actual checkout C:/Users/jc/orca/workspaces/Game/memoria-unreal-codex, branch codex/unreal-s342-integration-env-s343-20261003. Both cherry-picks were clean; prior Codex asset/scale tests remained. Peer documents90a0580e/29398d05 and top S350/S349 handoffs were inspected, with fresh Codex results recorded separately.

## Review and repairs

The five townsfolk roam within120cm of home at50cm/s and turn near Arrel170cm away. Their feet-driven gait and shadows follow the figures; fighting makes them watch the nearest foe. Pause stops world ticks; dialogue retains continuing source wander behavior.120cm and extending wander to all5figures are presentation tuning: source Verdan attaches32px wander to its first2figures, approximately96cm at chapter scale3. No story/navigation coordinates or original7collision surfaces changed.

Old quarter samples miss child path (280,245)->(331,277): endpoints and all quarter samples pass, while t=.85 reaches(323.35,272.2) inside the east stall exclusion. Old endpoint-only separation accepts woman(-511,205)->(-509,15) beside stationary elder(-549,92): endpoint86.77cm away but path approaches39.19cm.

**8ec8c16da2e2f6e95abebae3ac51b3bf8a49e623** checks whole segments against stall AABBs/story circles and reserves70cm clearance against other current/walking paths. UE safe segment-distance handles stationary figures. The geometric defects were proven analytically, without a claimed pre-fix native replay. New native regressions reject the corner, elder and crossing paths, retain clear/parallel paths, and sample actual movement/spacing/shadows every tick plus stopping/gait and rig aim. The passing replay records435.8cm travel and185.81cm minimum sampled pair separation.

All4props keep exact source flags: prop_barrel_160_128, prop_crate_352_288, prop_sign_256_64, prop_campfire_416_352. Barrel1-3Grains; crate40%potion/30%2-5Grains/30%empty; localized sign; fire+5HP. Each flag suppresses later use and persists in the existing run/save. English texts and sign's lack of a chime match source; Korean sign remains the peer's authored translation.

Two functional repairs in8ec8c16d:

- RestoreHp now safely saturates nonnegative addition before MaxHp. Accepted snapshots contain arbitrary signed HP/caps; the previous sum could overflow before Min. Save acceptance/schema remain unchanged.
- Crate uses source-compatible AddRewardPotion, retaining defined signed arithmetic, recent item, inventory event and +1 Potion notice before localized found notice. Scoped synchronous listener is removed; run replacement aborts follow-up handling.

Foundation.RuntimeLifetime now tests wounded37->62, zero/negative amounts, near-cap97->100, accepted signed extremes, and actual capture/reload of HP/cap plus all4spent flags. Visual replay deterministically exercises the real potion branch/event order, Korean/English notices, actual7HP damage then5HP fire healing and repeat attempts for all4props.

First compilation failed C4458: local Owner shadows AActor.Owner. **943b3141d31ec54bd5e4854377e2748ef57b9102** renames RewardRunId; build succeeds. Failed exit6/logs are retained. **6160f2612f0313a20099c64474fbf3165f5909fb** changes only the visual fixture: use a non-status foe, wait for staged hit effects to settle, then capture the fire. The original passing capture had a fall/status banner and is retained separately.

## Fresh execution

- Full **274/274** on943b3141; raw267success+7warning-bearing, failed/notRun/inProcess0; automation2058.313s.
- All **30/30 rendered visual** on6160f261; raw28success+2warning-bearing, failed/notRun/inProcess0; automation153.397s.
- Only943b3141->6160f261 file is MemoriaVerdanVisualTests.cpp, containing unselected MemoriaVisual implementations. Production and all274selected full test implementations are identical. No claim that274were rerun after the test-only capture update.
- Exact distinct IDs are compared to the unchanged registered sets; duplicate/missing/unexpected IDs fail. Every fixture/audio-source/Verdan-story/editor-build/automation command exits0/fatal0 in both pipelines.
- UE5.8.2 successful serial builds use Build.bat MemoriaEditor Win64 Development -Project=<own project> -WaitMutex -NoHotReloadFromIDE -MaxParallelActions=1. Standard validator up-to-date builds follow, so no older binary was used.
- Validator: python -B Unreal/Tools/validate_unreal.py --build-and-test --rendered --engine-root C:/Program Files/Epic Games/UE_5.8 --test-prefix Memoria. --automation-timeout3600 --evidence-dir <integration/full>. Visual uses MemoriaVisual., timeout1200 and <integration/capture_refresh/visual>.
- Unchanged MapInputAndModal passes: KEYBOARD_TRACE before(137.250,83.500), after(270.583,83.500), pressed1/ignored0. Prior peer S349273/274 and S350274/274 are historical evidence. Focus loss in S349 remains an inference; isolated retry is documented but no exact report pointer was found in the bounded audit.
- Existing startup Condition-failed lines: full15, visual15; all before first test: full=True, visual=True. Layout and memory-budget diagnostics remain in logs. Warning-bearing test names/states are retained in results; no warning-free-log claim.
-50fresh PlayFeel1/ChapterDressing captures all1280x720. Town-facing and barrel/crate/sign/fire inspected. Fire shows98/100HP after recovery; staged damage is test setup, not ordinary rest behavior. Preview title strip is retained.

Raw automation reports:
- Unreal\Memoria\Saved\Validation\unreal-run-20261005T105711445607\Automation-20261005T105714243808\index.json
- Unreal\Memoria\Saved\Validation\unreal-run-20261005T114201455122\Automation-20261005T114204727793\index.json

Generated Saved/Validation/s349-s350-integration/final_results.json retains exact commands/exit values, traces, report/artifact hashes, integrity, first failed compile and earlier passing artifacts. Versioned S349_S350_INTEGRATION_RESULTS.json retains all304exact passing IDs and summaries.

## Preservation and remaining polish

1014 unrelated own source/art/content/tool files and82 shared primary model files are hash-identical. RuntimeTests.cpp is the additional reviewed exception. No raw assets/packages imported or rewritten. Peer remains clean at29398d058e9852c992613d044bcbf8a2d274ea30; original Godot/foundation/shared models/Claude handoff remain read-only.

Spent barrel/crate still look fresh (source dims them); read sign keeps its marker. Primitive market props, shared NPC face/no new dialogue, existing pass-through stalls and canopy projection remain documented limitations.

No new push, deployment or packaging. Claude already owns both source features; consume only8ec8c16d/943b3141 plus6160f261's stronger visual fixture deliberately if continuing, then verify that lane. No automatic peer merge/message.
