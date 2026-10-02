# S343 — S337–S342 integration and twelve environment models

Date: 2026-10-03. Owner: Codex. Status: COMPLETE. Local only; no remote push.

Codex branch `codex/unreal-s342-integration-env-s343-20261003`. Initial repairs and full-registry revision: **7bb330d0**. Isolated final rush-contact repair and final visual revision: **32bb631e0c703525ef59e307ddf83120b52e31d2**. The 274-test result is explicitly the earlier revision; it is not represented as a full run at final HEAD.

## Integration and preservation

Started at clean `7922b395`; Claude stayed read-only at clean `545fecaf`. Original Godot and foundation remained read-only.

| Peer commit | Codex commit | Accepted work |
|---|---|---|
| aff57ff7 | cf868aa0 | S338 painted ground, masonry, lamps and atmosphere |
| 9904c449 | 7642c0b9 | S339 source-art field HUD |
| 5b433d6e | f78b83cd | S340 impact feedback and ambient NPC movement |
| 6345b3f7 | 68784a23 | S341 field quick items |
| 55e30b37 | f5694cbd | S342 six encounter foes and caster/charger behavior |
| 545fecaf | a0cce5a7 | S341–S342 final peer validation records |
| 94998f4b | fccc682e | S338–S340 final peer validation records |

S338's new MemoriaChapterEnvironment owns ground/light. S337's six polished materials, beveled/stone packages, authoring tool and historical records remain; Surface retains the detailed material/tint behavior. Obsolete S337 tiled terrain members were removed. Graphics regressions require painted ground and focus-aware masonry in both maps, weathered concrete in Drift, and exact source blocker transforms in both.

`asset_integrity.json`: 149 scoped asset files preserve the S337 bytes, including all six polished material packages. Sixteen new peer packages match hydrated `545fecaf` LFS objects. Existing FBXs, textures, rigs and animation clips were not changed by integration.

## Review findings and repairs

1. Scorch was missing from victory cleanup and Anchor Pulse's all-status cure. Real Ash Walker hits, public-domain skill unlock/cure, final-foe victory and stable HP across the next status interval now verify both paths.
2. Charger movement retried unswept after pawn collision, allowing a long frame to cross a wall behind Arrel. The actual rush now ignores only pawns during a swept move, then restores their collision response.
3. Smoke escape kept the previous encounter's BurnChain. A distant surviving foe prevents victory cleanup from masking the defect; escape resets to 0 and the next first-sword burn starts at 1.
4. Final review found remaining rush contact used pre-move radial distance: a thin wall stopped the body but allowed damage through it, while a long open-lane frame could cross Arrel without hitting. Contact now uses the actual start/end segment, checks the closest point against RushHit, and rejects a visibility obstruction. The shared strike API skips final-position range only when the caller has already resolved contact; its default melee and existing orb behavior are unchanged. Guard/parry/dodge/ward/status handling stays shared.

The first three repairs and regressions are `7bb330d0`. The initial red visual run had 7 new assertion failures in FieldBurn/FoeVariety and 28 remaining passes. The final contact tests reproduced 4 expected failures in FoeVariety (blocked HP/strike count and missed overshoot HP/strike count), with 29 remaining passes. The final repair is `32bb631e`.

Two old host checks sliced GrantRewardItem through GrantFieldItem and unintentionally counted S341's inserted ConsumeItem inventory broadcast. They now stop at GrantRewardItem's own closing brace. Every mutation, source fixture and registry assertion remains; 35/35 pass after two reproduced failures.

## Fresh verification and exact revisions

| Check | Revision/scope | Result |
|---|---|---|
| Editor build + rendered Memoria. | 7bb330d0, integrated S337–S342 and initial repairs |274/274, 267 success + 7 existing warning-bearing tests |
| Final Editor build + MemoriaVisual. | 32bb631e, isolated rush contact repair |30/30, 28 success + 2 existing warning-bearing tests |
| Host | six unchanged-contract modules, repaired source-check scope |35/35 |
| Ground / HUD exports | accepted source art |5/5 and 9/9 |
| Independent model import/package | final delivery files |12 models, 27 files PASS |

Engine and validator exit0; exact expected identities, no missing/extra/duplicates, failed/not-run0, fatal patterns 0. Full summed test duration 1904.95 s; final visual 134.47 s. The late change stayed within rush geometry/obstruction and a renamed caller-resolved flag; run/save/narrative/inventory logic did not change. Read-only registry review confirmed executable monster/strike assertions are in MemoriaVisual; the full suites' live story worlds avoid combat. Therefore the relevant final 30 were rerun, and the unchanged 274 result is retained with its exact earlier revision.

Both processes retain 15 pre-existing engine-startup Condition failed lines, separately from requested test outcomes. Full warnings are six transient no-world-context warnings and one MotionVectorSimulation warning; final visual has the existing world-context/render-thread warnings. Primary engine UnifiedErrorTests English ToString literals match the locale symptom; no culture was forced and the engine is not certified error-free. Detailed messages and raw assertions are in results_summary.json.

The first final-contact build exposed a raw APawn pointer written as Target.Get(); it was corrected to Target before the successful rebuilt final 30. Failed build/reproduction logs are retained. No packaged release, manual feel/performance session, remote push or deployment is claimed.

## Environment kit — delivered, not applied

Twelve static models: signal post, platform shelter, crates/barrels, chain fence, lantern post, rail section, patched tarp, ruined wall, gramophone/records, dead tree, banner pole and dry grass. 12 FBXs/12 basecolors/3 emissive masks, requested dimensions/budgets, one mesh/material/UV each. Independent fresh import from copied delivery files verified units/origin, finite UV0..1, zero degenerate geometry, portable texture paths, grass alpha and all 27 hashes. Real CPU front/side/back/48-degree QA and separate read-only delivery review passed.

- Manifest: [S343_ENVIRONMENT_MANIFEST.md](C:/Users/jc/Documents/Codex/MEMORIA-Unreal-Collaboration/models/S343_ENVIRONMENT_MANIFEST.md).
- Gallery: shared `models/_qa/s343/environment_kit.png` (real Blender gallery at delivered sizes, not a game screenshot).
- Audit: `models/_qa/s343/final_delivery_audit.json`; bundle audit `models/_qa/s343/package_audit.json`.
- Bundle: `models/_handoff/s343_environment.zip`, 53,970,610 bytes, SHA256 `f6af3d1fad5c0214555464da52f7998eb1679d4fcc18adac3c23fcbb3fed073e`.
- Editable sources and reproduction: `models/_raw/s343_environment/belt/` and `drift/`.

Existing Belt/Drift canvas palettes and silhouettes were inspected, not changed. Geometry and new procedural atlases were locally authored with existing Blender/Python; CPU only, no inference/download/account/upload/paid tool. Signal was reported first; its provisional checkpoint hash is superseded by the final manifest.

Claude owns additive game import/placement. New 12 models are not in either game lane. Confirm the first signal's front/scale/light with S335 conversion/rotation; grass needs Masked/two-sided, emissive masks are linear and multiplied by warm colour. Tarp centre clearance 172.30 cm versus 150 cm Arrel. Preserve source blockers. Rails repeat at 192cm along localX; fence posts coincide at 160cm shared-post pitch (or use 192 cm full-footprint spacing). Complete notes and hashes are in the manifest.

No LODs, authored collision, wind/cloth animation or normal/roughness maps. Existing accepted gaps remain: guard-only ink, no-op items kept, fixed quick slots without gamepad bindings, six foes sharing two models, caster orbs through walls and untuned values. Six new foe models and gameplay tuning remain outside this round.

## Evidence locations

Codex `Unreal/Memoria/Saved/Validation/`:

- `s343-integration/full/unreal_validation.json` → `Unreal\Memoria\Saved\Validation\unreal-run-20261002T184256381584\Automation-20261002T184322572063\index.json`.
- `s343-rush-final-fixed/unreal_validation.json` → `Unreal\Memoria\Saved\Validation\unreal-run-20261002T192558335166\Automation-20261002T192650730208\index.json`.
- `s343-integration/visual/`: earlier30-pass checkpoint.
- `s343-regressions-before/`: initial 7 red assertions; `s343-rush-before/`: late 4 red assertions; `s343-rush-final/`: retained pointer compile failure.
- `s343-integration/results_summary.json`, `host-tests.log`, `asset_integrity.json` and ten copied 1280×720 integration game captures. Raw logs/captures remain local under repository evidence policy.

Finalization after the runtime commit changes only host source checks and records. The GPU/build slot is released after the successful final validator exits. Peer remains at `545fecaf`, clean; no remote push.
