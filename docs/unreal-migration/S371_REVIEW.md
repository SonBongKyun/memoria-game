# S371 — Chapter 6, The Seam, from the final manuscript (Claude, 2026-11-21)

Branch `claude/s366-upgrade`, on top of S370 (`4c42b46a`). The user asked to port Chapter 6, The Seam, starting from the manuscript.

## Source
- 최종본 6화 「더 심」 and 7화 「세이블」 (final-1, approved 2026-09-25), read in full.
- Canon kept:
  - Sable is blind (2026-07-03): she hears and touches, and never looks.
  - The seventeen who went looking, and her daughter Besa.
  - The spiralwave shell.
  - The bark map with its hole.
  - Haren leads them out.
  - "열일곱은 아니야."
- The Godot `the_seam.gd` tells the old story instead: Elia's reunion, Sable joining the party, BL-07's entrance and the epilogue. So only its **ground** is used: tiles, collision, atmosphere, encounter pool and its painted canvas. The story is the manuscript's.

## What changed
- **Map** (`export_chapter_maps.py`, new `MANUSCRIPT` table).
  - `the_seam` is exported with the Godot ground and a manuscript story:
    - a three-scene arrival chain (arrival and Elia's tears at the ferns, the name lost on the bridge, Feren);
    - two story places (the night in the inner room, the morning at Sable's table);
    - Haren's ridge road as the exit;
    - a chest and a canon clue after the chapter;
    - lanterns lit.
  - CLIFF tiles stand as walls. `chapter_memories` now carries Chapters 6 and 7. The level `L_TheSeam` comes from `-run=MemoriaChapterLevels`.
- **Scenes** (`MemoriaManuscriptSeam.cpp`): six scenes built at runtime, in both languages; no dialogue assets.
  - Sable's "몇 달" answer follows the burn mechanic: three months as written, two once Arrel has burned four or more.
  - The night scene gives `rel_echo_shell` ("획득: 나선파 조개").
  - The morning gives the bark map flag ("획득: 세이블의 나무껍질 지도").
- **Flow.**
  - After Chapter 5's classifier, Drift Shelter's east exit is now the road to the Seam (`MemoriaManuscript::RoadOnward`). The closing card says "동쪽 출구에서 6장으로 이어집니다".
  - The demo card's summary points there too, and the goal line reads "동쪽 출구 → 더 심".
  - The Seam's goal line walks through its steps.
  - Chapter 6's card names "능선길"; Chapter 7 is not ported yet.
- **Main quest:** three new steps (find Sable, ask the way, the ridge road with Haren) and the open last step, the hole in the bark.
- **Look.**
  - Ground cut from the Seam canvas (`seam_soil`, `seam_paved`, `-run=MemoriaChapterGroundAssets`).
  - Timber cottages with pitched roofs and a lit window on the HUT tiles.
  - Lantern posts on the LANTERN tiles.
  - Green growth on the garden tiles.
  - A violet-dusk, warm-lamp dressing.
  - Drift's dead-tree ring and camp no longer apply to other maps.
- **Art:**
  - The Seam splash, three canon CGs (first light, the living streets, the echo shell's voices) and Sable's blind portrait (`sable_calm`).
  - Not used: the briefing CG, which shows Tobias (not in the story until 23화).
- Korean names: 더 심, 있던 것과 올 것 사이, 능선길, 페렌, 하렌, 마을 여자; "보이드 홀" is shown as "공허 구멍".

## Verification
Native UE 5.8.2, D3D 1280x720, 2026-11-21.
- New `Memoria.S371.Seam`:
  - spawn, story places and exit on open ground;
  - every played group is one of the six manuscript scenes;
  - Chapter 6/7 memories;
  - Sable's three/two months;
  - the closing line;
  - rows indexed in both languages;
  - Drift's road on only after the classifier;
  - the Seam's goal lines;
  - main-quest steps.
- New `MemoriaVisual.S371Seam` on L_TheSeam (development run at Chapter 6):
  - the arrival chain, then night (shell acquired with its toast), then morning (bark map), then the ridge road;
  - closes with `ch6_complete` and `canon_ch7_ridge_ready`, and the card shows;
  - eleven captures inspected (`Saved/Validation/s371/focus2` for the scenes, `focus3` for the dressed village).
- Existing expectations updated:
  - the artwork count is 72;
  - Drift's goal after Chapter 5 is "동쪽 출구 → 더 심".
- Wide regression: every `MemoriaVisual.*` + `Memoria.S36*` + `Memoria.S37*` + `Memoria.Journal*` + `MemoriaSave.S362*` + input: `s371/wide1` **PASS 72/72**.

## Limits
- Chapter 7 onward (the ridge road, 듣는 숲 at manuscript 8화) is not ported. Chapter 6 ends on its card, and the map stays open for its chest, clue and encounters.
- The cottages are simple timber boxes with the shared pitched roof, not dedicated models.
- English lines are a translation of the manuscript written for this port; the user's English edition was not consulted.
