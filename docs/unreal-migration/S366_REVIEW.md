# S366 Afterimage — perfect dodge, Echo Counter and dash ghosts (Claude, 2026-11-19)

Branch `claude/s366-upgrade` in the Claude lane. Base: `16483d58`, a byte-for-byte snapshot of Codex's working tree (HEAD `6fd6fce0` + uncommitted S362–S365) taken on 2026-11-19. Codex's worktree was only read.

## What it does
- **Afterimage (gameplay).** A dash (Shift / pad) begun no more than 0.18 s before a foe's blow lands leaves Arrel's afterimage where he stood, and the blow falls on it:
  - no harm and no curse;
  - the foe loses him and reels for 0.9 s;
  - the world slows to 0.30 for 0.55 real seconds;
  - the next swing within 1.6 world seconds is an **Echo Counter** (x1.6, a big hit with a violet popup). It applies to every foe that one swing reaches.
  The blow has to be aimed at where he stood or where he is; a swing at empty air is no dodge. It counts once per dash. Melee, orbs and charger rushes all go through the same `StrikePlayer` path.
- **Ghosts (graphics).** Every dash leaves a faint ghost of Arrel's own 3D pose (a `UPoseableMeshComponent` copying his skeletal pose, with the drawn blade). A second, fainter ghost appears mid-dash, but not under Reduce Motion. On an Afterimage, the dash's first ghost flares violet (alpha .62) and lingers 1.15 world seconds.
  The new material `/Game/Memoria/Presentation/Combat/M_Afterimage` is translucent and unlit, with a Fresnel-lit rim and `Tint`/`Alpha` parameters, usable on skeletal meshes. It is authored by `-run=MemoriaAfterimageAssets` and refuses to overwrite an existing package.
- **Slow-motion look.** An unbound post-process on the pawn (priority 10) drops saturation to .35, adds a cool gain, vignette .75 and a light fringe. It blends in over the first 15% of the slow and eases out. Reduce Flash halves it.
- Time dilation is coordinated: a hit stop that ends inside the slow returns to the slow, not to 1.0. The burn picker still owns time while open. `ClearRecoveryState` (revive / restore) clears ghosts, slow and counter.

## Files
- `Public/Combat/MemoriaFieldCombatTypes.h`: S366 tuning constants.
- `Public/Combat/MemoriaFieldCombatSubsystem.h`: accessors, the `FMemoriaAfterimageGhost` struct, private state.
- `Private/Combat/MemoriaFieldCombatSubsystem.cpp`: hooks in `RequestDash`, `StartStep`, `ResolveSwing`, `StrikePlayer`, `TickFeel` and `ClearRecoveryState`, plus includes.
- `Private/Combat/MemoriaAfterimageS366.cpp` (new): dodge, ghosts, post-process, clear.
- `Public/Combat/MemoriaFieldMonster.h`: one read-only accessor, `GetStateTime()`.
- `MemoriaTests`: `MemoriaAfterimageAssetsCommandlet.{h,cpp}` and `MemoriaAfterimageS366Tests.cpp` (`MemoriaVisual.S366Afterimage`).
- `Content/Memoria/Presentation/Combat/M_Afterimage.uasset` (LFS).

## Verification (2026-11-19, native UE 5.8.2 / D3D, 1280x720, fresh editor build)
- Editor build after the final source edit: `Result: Succeeded`, with no warnings from the S366 files.
- `-run=MemoriaAfterimageAssets`: `AFTERIMAGE_ASSET ... SAVED`, 0 errors. It wrote only `M_Afterimage.uasset`.
- `run_s365_native.py --tests MemoriaVisual.S366Afterimage` -> `Saved/Validation/s366/focus2`: **PASS 1/1**. Real Shift and J presses against the market thief: a perfect dodge with no harm and no curse, the thief staggers, dilation .3, drain weight > .5, ghost alpha > .4. The counter hits for exactly 12 x 1.6. Time and colour return to normal. An early dash is not an Afterimage, and its ghosts fade. Captures: `AfterimageDodge.png`, `AfterimageCounter.png` (inspected).
- Regression subset `MemoriaVisual.Field*`, `MemoriaVisual.S36*`, `FoeVariety`, `EliaSkills`, `Memoria.S36*` and `Memoria.Foundation.MapInputAndModal` -> `Saved/Validation/s366/related1`: **PASS 32/32**, exit 0, fatal 0, execution inputs unchanged.
- Two earlier attempts failed and are kept for the record. `focus1`: the test timed Shift by frame count and missed the window. `probe1`: the test attacked mid-dash and checked the colour drain one frame early. Both were fixed in the test (it now times Shift by the thief's windup clock and waits for the dash to end); no game threshold was loosened.
- Not run: the full registry and the packaged/cooked smoke.

## Limits
- No physical gamepad or human play, and no Shipping/package run.
- The ghosts are plain translucent copies: no motion blur, and no dissolve beyond the alpha fade.
- The thresholds (0.18 s window, x1.6, 0.9 s stun) are first-pass numbers, tuned by feel. They are not taken from the Godot source, which has no dash.
