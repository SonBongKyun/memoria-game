# S353 — Character and monster surface detail

The user requested more graphic detail for existing characters and monsters. The live Unreal Codex checkout now gives14existing body materials surface relief and distinct material response, retaining the painted character designs. This is an independently owned Codex task; Claude is left alone.

Base: e50350229a033dfa77610c994e2c57e4ceb8745a. Source/assets commit: 2cfc182db5ba1a86435cbb246f75b8af6a1aa917. Final validation: PASS (58/58, including all30 rendered visual tests).

## Source authority

The approved Notion final hub and worldbuilding v10 were read live earlier in this chat on2026-10-11; links and approval metadata are in S352_REVIEW.md/S352_RESULTS.json. Character identity, painted costume motifs and the restrained dark-fantasy palette remain the authority. This task adds no story or new creature canon. ExistingFBX, basecolour and emissive-source paintings are preserved byte for byte.

## Changes

- Arrel, Elia and Malet share new periodic normal/packed data maps. Broad cloth relief repeats twice across their irregular packedUVs. Skin-colour attenuation and model-specific posed-local neck/head attenuation keep facial and hair relief very weak. Arrel's pale neutral armour has a restrained metallic response (maximum.30) and roughness.44; cloth stays matte. Head roughness returns to.74. This is procedural surface detail, not new hand-authored costume folds.
- Nine ambientNPC bodies (Traveler, Bureauagent, Guard, female/male villagers, Fisherman, Elder, Child, Scholar) share an atlas with cloth, leather and small brass details. Their lower head atlas stays almost flat and non-metallic. Existing height ratios, faces and movement are unchanged.
- Husk gains organic plate/bone relief; Thief gains cloth folds/leather wrinkles and metal response. Their atlas topology and luminous crack mask are retained. Six existing foe kinds receive muted reflected-surface tint/weathering profiles. Existing crack/rim/projectile colour, hit flash, all combat values and behaviour remain intact.
- Eight new deterministicPNGdata maps are generated from procedural fields; the generator never opens source albedo or model files. Linear samplers, normal compression, DirectXnormal convention, trilinear filtering, mipchains, atlas gutters and normal-edge fades keep these data separate from painted colour. The exact packed channels, seeds, hashes and dimensions are versioned in the manifest.
- MemoriaSurfaceDetail commandlet explicitly reimports only its eight data textures and rebuilds the fourteen named existing body materials in place. It requires rendering, fully loads packages before replacement/save, waits for texture/shader compilation, and refuses material save on shader errors. Existing meshes keep their live material bindings.
- Native tests inspect actual mesh-slot bindings, connected normal/roughness/AO graphs, packed metallic channels where applicable, native shader resources and runtime per-foeMID values. Hit flashes and glow are checked independently. A clean lineup waits for the real tutorial and achievement queues to expire; no product UI is hidden.

## Verification

Fresh UE5.8.2 single-action build, focused4/4native tests and final **58/58PASS** (Foundation6/6 + related52/52, including all30rendered visual tests). Exact registered identities match; failed/notRun/inProcess are zero. Every final pipeline command exits0 with no fatal diagnostics. The full274registry was not rerun.

47fresh before captures and48fresh focused after captures were preserved. The final regression produced77fresh selected character/foe/combat/chapterNPC/Elia captures, all1280x720 with SHA-256receipts. Actual imported material/texture resources and all14live mesh bindings pass. The generator --check regenerates all8PNGs and the manifest exactly without reading source images.

Existing startup diagnostics remain:15Condition-failed lines per engine process before the first test and memory-budget lines (Foundation6, related50). Foundation.MapInputAndModal and Campaign.CanonicalPaidRoute emit r.MotionVectorSimulation render-thread warnings; Chapter1Presentation emits a missing-world warning. This run also records a Chapter3 EOS telemetry-flush warning plus four listed analytics events. Raw summaries are Foundation5success+1warning-bearing and related49success+3warning-bearing. Exact messages/log paths are in S353_RESULTS.json; these passing warnings are not hidden.

Early attempts are retained: test-helper pointer/API compile errors; a rendering-disabled commandlet; a partly loaded reimport package; and one native asset check that inspected the temporary32px async texture placeholder. The fixes add correct UE pointer handling, render preflight, FullyLoad-before-overwrite/save and explicit compilation completion. Successful fresh runs follow these repairs. Only the exact own hung importer and its orphan shader workers were stopped; user programs were preserved.

Independent review removed two weak checks before acceptance: arbitraryMID overrides cannot prove the parent implements a crack parameter, and a loose texture node cannot prove connection to an output. Current checks verify the real parent and traverse the connected graph. A first passing material version was visually too subtle on heroes; the final version broadens cloth detail and adds protected armour response, followed by another4/4focus pass and final regression.

## Visual scope and limits

At close view Arrel's cloak and armour show the strongest change. Elia's pale robe stays subtle; Malet and the NPCs gain modest fabric response. Husk/Thief close-ups show leather/bone relief. The clean field lineup shows warmer dusty and grayer reflected surfaces, while the existing glow, height and shared silhouettes still dominate identification. No major geometry upgrade is claimed.

The head guard uses posed local height plus painted skin attenuation. It is conservative for current upright/walking poses, not a permanent bone-weight mask for arbitrary deep crouches or future animations. Fresh native CombatSwing2/CombatSwing3 close views and CombatDash at normal camera show no visible oily/metallic hair or face artifact in the current clips; this is a bounded visual check, not proof for arbitrary deeper future poses.

Nine ambientNPCs still share facial geometry; eight foe kinds still use the two existingHusk/Thief bodies. There are no new unique faces, quadruped rigs or models. The clean lineup expires tutorial/achievement overlays; foreground stalls still partially overlap an enemy and the normal HUD can cover labels. There is no Shipping package or GPU performance benchmark, and no full274registry rerun.

Preservation confirms1001unrelated tracked own source/art/content/tool files remain byte-identical; only the14named material packages and7tracked source/test files differ from the1022file baseline. All1159files in the broader validation freeze still match after the run. Claude remains clean/unchanged at159ae961f844e3492cd9857b6d472e8aa79b5d02; original/foundation/models/handoff were left untouched. No own editor/compiler remains. Scoped Git LF/-text rules preserve the new self-hashed generator, manifest and PNGs on Windows checkout; filtered checkout bytes were verified. This delivery-only rule follows validation without changing executed source/assets. Local commits only; no push, peer merge/message or packaging. Raw generated evidence: Unreal/Memoria/Saved/Validation/s353/.
