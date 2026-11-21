# S372 — Chapter 7, the listening forest, from the final manuscript (Claude, 2026-11-21)

Branch `claude/s366-upgrade`, on top of S371 (`aa943bdc`). The user asked to port Chapter 7.

## Source
- 최종본 8화 「듣는 숲」, 9화 「빌려온 입」 and 10화 「대가」 (final-1), read in full. Game Chapter 7 = these three.
- Ground from the Godot `forgotten_forest.gd`: tiles, collision, encounter pool, its two clues and painted canvas. Its story (a memory-parasite forest heading to the Colorless Waste) is not used.

## Canon fix to S371
- In the manuscript, Sable gives the small **spiralwave** (나선파, 7화), and Elia gives the **echo shell** (에코 셸, 9-10화) in the valley.
- S371 had granted the echo shell's memory (`rel_echo_shell`, 「껍질 속의 목소리들」) at Sable's scene.
- Now Sable's night sets `has_spiralwave` ("획득: 나선파 조개"), and the memory comes with Elia's shell under the slab ("획득: 에코 셸"). The S371 tests follow.

## What changed
- **Road:** the Seam's ridge road now travels into the forest (next map `forgotten_forest`, Chapter 7). The Seam card reads "6장 완료 / 듣는 숲 / 7장으로 이어집니다".
- **Map** (`export_chapter_maps.py` MANUSCRIPT `forgotten_forest`): in on the north road, out to the south. The lights table is generalised; the Seam's lanterns are unchanged.
  - Arrival chain:
    - the ridge: Haren, Sable's warnings ("사슴길은 피해", "소리 쪽으로 고개를 돌리지 마"), and Arrel's question about burning less;
    - the forest's entrance: "듣는 숲이야", the mushroom rings, the counted breaths, the trees that move.
  - Story places down the road:
    - borrowed senses: the dripping water, the cold, the bread oven, Elia's song;
    - the mother's voice behind the wall of yellow flowers ("가지 마… 외우려고 하지 마"), with Kairos at the Seam: "관찰함. 개입하지 않음.";
    - the lost fifth hour and the voice that used Elia's way of calling, ending on "태워야겠어. …작은 걸로 해."
  - **The shape between the trees (9화) is a fight.** A Hollow Walker pack rises where the lost hour ends (`battles_gate` `ch7_lost_hour`); burning is how Arrel faced it in the manuscript.
  - The escape: the morning burned, but something else went. Then Sable's fold between two stone ridges.
  - The exit is the night under the slanted slab (9-10화):
    - forty-one went into the forest;
    - stones on the ridge;
    - "내가 배운 방식이었으니까";
    - the mark on her palm;
    - Elia's echo shell ("오늘 밤 목소리는 내 거니까");
    - patience and the ferns, "너 중에 너인 게 얼마나 되는데";
    - the child-shaped absence;
    - the shell: the counting is wrong from the seventeenth, his own name heard and unsayable, "다시 말해."
  - Chests and clues after the chapter; Chapter 8 memories on arrival (`export_chapter_memories` CHAPTERS through 8). Level `L_ForgottenForest`.
- **Scenes:** seven runtime scenes, Korean and English (`MemoriaManuscriptScenes.cpp`, renamed from `MemoriaManuscriptSeam.cpp`).
- **Goals and main quest.**
  - Goals: into the forest, then out of it, then down to the ridges.
  - Main quest: three steps (through the forest, away from the shape, the night under the slab) before the open "hole in the bark".
- **Look:**
  - ground cut from the forest canvas (`forest_soil`, `forest_paved`);
  - trees on the TRUNK tiles and along the canopy border;
  - pale mushroom rings round the trunks;
  - green growth on the soil;
  - a dim, still, green dressing with pale mushroom light.
- **Art:** the forest splash, the ridgeline, the forest entry and the mother in the forest.
  - Set aside: the pictures that show a third companion with the two (Tobias's look), including one imported then removed in this session (`story_ch8_forgotten_moss`).

## Verification
Native UE 5.8.2, D3D 1280x720, 2026-11-21.
- New `Memoria.S372.Forest`:
  - north entry, south exit;
  - the shape's battle and its gate;
  - seven manuscript scenes;
  - Chapter 8 memories;
  - the Seam's road leads into the forest;
  - the key lines (Sable's "덜 태우는 편이 낫겠지", "나무가 움직여", "가지 마", Kairos's "관찰함. 개입하지 않음.", "태워야겠어", "에코 셸.", "오늘 밤 목소리는 내 거니까");
  - no Sable line with a seeing verb;
  - the goals;
  - the main quest.
- New `MemoriaVisual.S372Forest` on L_ForgottenForest (development run at Chapter 7):
  - the ridge, the entry, then the echoes, the mother and Kairos, then the lost hour;
  - the shape rises (`battle_forgotten_forest_1`, live foes);
  - then the escape and the night under the slab;
  - closes with `ch7_complete`, `canon_ch8_coast_ready` and the echo shell memory, and the card shows;
  - thirteen captures inspected (`Saved/Validation/s372/focus1`). One picture with an extra companion was found there and removed.
- `Memoria.S371.Seam` and `MemoriaVisual.S371Seam` follow the canon fix (spiralwave at the Seam, no echo shell yet).
- The artwork count is 76.
- Wide regression: every `MemoriaVisual.*` + `Memoria.S36*` + `Memoria.S37*` + `Memoria.Journal*` + `MemoriaSave.S362*` + input: `s372/wide2` **PASS 74/74**.
  - `wide1` crashed at startup in the asset registry: the cache still listed the removed texture. The cache was moved aside, and it rebuilt on the next run.

## Limits
- Chapter 8 (11화: the Crumbling Coast and 공백 7호 itself) is not ported; Chapter 7 ends on "두 돌 능선 사이 · 8장으로 가는 길은 아직 준비 중".
- The shape is the existing Hollow Walker foe, not a dedicated hollow-in-the-tree model. Burning is not forced; the escape scene narrates the burn the manuscript has.
- The valley has no picture of its own; it plays over the forest splash.
- English lines are this port's translation.
