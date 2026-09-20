# Verdan exploration presentation pass 2

Status: verified; the local checkpoint is the commit containing this report.
Entry checkpoint: `78d2ef7794cacda221bf83fc7ead7bfdeb6defb6`.
UE 5.8.2 / CL56702186. Source gameplay frontier stays `before:shop_actions`.

## Visible result

- The connected Verdan field now displays the existing Arrel and Malet field art,
  stone paving, market facades, lanterns, contact shadows and restrained amber light.
  The exploration prompt is smaller and still reports Malet's actual interaction range.
- Arrel faces all four movement directions and plays the original four-frame gait.
  Feet use a fixed source pivot, sorting follows foot position, and animation stops
  when the pawn is stationary, including held input against a wall.
- The top-down market canvas is used as an atlas of stone/facade regions. Its whole
  central courtyard image is not placed over incompatible collision geometry.
  Perimeter facades and the two lantern plinths align with the existing boundaries
  and obstacle footprints. This remains a small test courtyard, not the full Godot map.
- The original seven physical surfaces, pawn collision, camera, movement code,
  Malet position/range, physical interaction and narrative travel are preserved.
  The runtime-only presentation actor has no collision or overlap events.
- Eleven new textures, twenty-nine sprite regions and one soft-light material:
  41 new presentation packages, 82 total. The initial floor region is retained as
  an unused import candidate; the final lower-contrast floor is an additive sprite.
  No previous package was replaced during the visual refinement.

![Verdan field](evidence/presentation2/verdan_final.png)
![Reachable Malet interaction](evidence/presentation2/malet_approach_final.png)

## Source provenance and boundary

[Artwork sources](evidence/presentation2/artwork_sources.json) records seven original
PNGs and four derived gait atlases, SHA-256, byte sizes, source rectangles and pivots.
The exact `_detect_leg_top`, `_build_walk_cycle`, `_make_walk_frame` and
`get_texture_source` functions and five WALK constants were extracted from the
protected `scripts/utils/pixel_sprite.gd` and executed with Godot 4.6.2 in an isolated
Saved project. Four 512 x 160 atlases hold the original four-frame, 9 fps cycles.
Only presentation playback rate follows actual movement speed. Original art and
source scripts were never saved. Runtime reads UE packages, not Godot/PNG source.

The [source execution](evidence/presentation2/source_walk_export.json), raw harnesses
and logs are retained in `source_walk_raw.zip`. The first harness failed because its
extraction boundary included the following cache declaration twice. The corrected
boundary stops at `static var`; it does not rewrite the source function. Both attempts
are retained. The new UE test's first build failed on `auto*` deduction from
`TObjectPtr<UWorld>`; the explicit `UWorld*` declaration fixed the test compilation.

No shop transaction, shop-close handler, Chapter 3, battle, sound, save schema,
narrative IR, interpreter effect or authored story content was added. Original Godot
checkout remained read only. No push. The foundation map and previous map packages
were not re-authored; the new actor is spawned only for the two existing Verdan field maps.

## Fresh validation and historical reuse

- `visual03`: rendered 3/3 on the final floor/HUD. The real PIE movement replay saw
  all sixteen directional walk sprites; all seven physical surfaces stayed collidable,
  wall sweep stopped at X=881.888, feet stayed attached, and Malet's prompt remained
  reachable. Run, memory and narrative trace were unchanged by the visual replay.
- `campaign01`: rendered 4/4; actual input, arrival travel, choice identity, save/restore,
  modal movement gating, camera follow and boundary collision still pass.
- `shop01`: rendered 12/12; the connected Malet approach, dialogue/reward and first
  shop screen retain the existing contracts. These three executions total 19 tests;
  they are not a new full 203-test execution.
- Host tools 96/96. Final static gate 66 checks. New Python helpers pass syntax checks.
- [Final preservation](evidence/presentation2/preservation01.json): original 4,217 files,
  21,993 protected tracked worktree files, previous 41 UE packages, previous 90 IR/fixture
  files and historical evidence/reports preserved; exactly 41 new presentation packages,
  zero new narrative IR. Original protection manifest unchanged. SESSION_LOG prior bytes
  remain an exact prefix.
- Working and staged diff checks and raw evidence/LFS verification are recorded in
  [staged verification](evidence/presentation2/staged_verification.json).
- Previous full UE203/203 and independent 1,500 checks / 1,475 snapshots remain the
  historical `presentation1/automation02` and `presentation1/independent01` results.
  Neither the full suite nor the full independent comparison was rerun here. The
  unchanged native domain/source contract fixtures also reuse their historical evidence.
  The Godot gait export above is a fresh graphics execution, not a rerun of gameplay oracles.

Every fresh UE attempt retains raw logs, its exact automation report and produced JSON
observations in a ZIP with per-member hashes. Final and initial visual captures are
retained. Repeated story screenshots from the campaign/shop replays are deliberately
omitted from the ZIP, with their filenames recorded; this avoids duplicating the previous
large image archive. Final displayed PNGs are copied byte-for-byte from the new UE run.

## Next bounded development task

The foundation now supports an illustrated playable Chapter 2 slice. The next useful
playability task is to characterize the source shop purchase/sale/close behavior, then
implement that bounded contract so the current shop screen can complete. Reuse the art
already integrated. Do not treat the current checkpoint as a full campaign or packaged release.
