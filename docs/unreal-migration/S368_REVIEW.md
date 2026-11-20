# S368 The burned memory's last sight and its scorch (Claude, 2026-11-20)

Branch `claude/s366-upgrade`, on top of S367 (`1d022a7c`).

## What it does
- **The memory, seen one last time.** When a burn is confirmed, the memory's illustration rises above Arrel as a 168x94 card facing the camera, just over his head and below the HUD's burn banner. As the ring is released, the card burns away from its bottom edge behind a noise-broken seam of the grade's colour, shedding embers, and is gone 1 s later.
  - Illustrations come from the source's `MEMORY_BURN_CUTIN_PATHS` (six files): the exact id first, then the source's keyword fallback (`MemoriaBurnArtS368.h`).
  - A memory without one burns as a translucent smoke of its grade's colour.
- **The mark on the ground.** The release leaves dark soot (fading over 10 s) and a glow of the grade's colour (cooling over 5 s). At most four marks are kept. The glow's flicker stops under Reduce Motion.
- **Assets.** `-run=MemoriaAfterimageAssets -BurnArt` imports `T_MemoryBurn_<name>` (six) and authors `M_MemoryCard` (translucent, unlit, two-sided; `Card`, `HasArt`, `Burn`, `Alpha`, `Edge`). It refuses to overwrite existing packages.

## Verification (2026-11-20, native UE 5.8.2 / D3D 1280x720)
- `MemoriaVisual.S368BurnRelic` PASS (`Saved/Validation/s368/focus4`). Real R, arrow and Enter keys:
  - the first sword's card;
  - its burn progress and disappearance;
  - one mark per release;
  - a smoke card for an art-less memory;
  - marks gone within 10 s.
- Captures inspected. Two problems were found and fixed: the picture was horizontally mirrored, and the card came out square because the imported texture is padded to a square.
- Wide regression: every `MemoriaVisual.*` test + `Memoria.S36*` + `Memoria.Foundation.MapInputAndModal` -> `s368/wide2`: **PASS 57/57**, exit 0, fatal 0, inputs unchanged.
- Known harness quirk: the `BurnCardSeen` capture sometimes grabs the editor viewport (1014x550) instead of the game. It does not affect the test's checks.

## Limits
- Four of the six illustrations are abstract placeholders from the source.
- The card does not cast light.
- No package run.
