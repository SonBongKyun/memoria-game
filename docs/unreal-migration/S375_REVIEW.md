# S375 — Chapter 10, the Seam, leaving, from the final manuscript (Claude, 2026-11-22)

Branch `claude/s366-upgrade`, on top of S374 (`b3cf51a3`). The user asked to port Chapter 10.

## Source
- 최종본 15화 「손이 기억하는 것」 and 16화 「출발」 (final-1), read in full. Game Chapter 10 = these two.
- 15화 happens in the Seam itself, so the chapter stands on the Seam's own ground again: Godot `the_seam.gd` through the `GROUND` table.
  - The map has its own id, `seam_return` (level `L_SeamReturn`, "떠나는 더 심").
  - Chapter 6's map is untouched.
- 16화 (the road west and the crossroads below the ridge) is the exit scene.

## What changed
- **Road:** the dry channel's exit (Chapter 9) now travels back to the Seam (Chapter 10).
- **Map:** in from the north-east path (Haren brings them down), out by the west.
  - Arrival chain:
    - the road down: the coals buried, Haren met on the way, what he told Sable and what he cannot carry ("네가 본 것까지 들고 갈 수는 없어"), Elia's nosebleed;
    - the bridge: bedding tied in bundles, the cloths gone from the walls; "다른 게 오고 있어."
  - Story places:
    - **Sable's step**, before the house where Elia drank:
      - Haren sent to split and burn the felled tree and bury the ash ("그건 기록이야." "네가 쓴 것은 아니지.");
      - "그래. 물어.";
      - Arrel sings his six; "요람가야.";
      - the name not said here;
      - Elia's seven; "두 반쪽이 맞아.", "두 아이에게 나눠 넣었어";
      - "끔찍한 자선이지." "자선이라고 하지 마."
      - The run records `ch10_cradle_halves`, with the toast "한 요람가의 두 반쪽".
    - **The east road:**
      - "아렐이 안 느껴져. …지금도."; the fear gone since the city;
      - the horn; the boy: eight and the adjuster, short clubs held low;
      - Arrel's hand on the fourth notch; "얼마나 태울 건데.";
      - what the clubs do and who they were made for ("그래. 너에게.");
      - "같이 가자." "서쪽."
    - **The square's west end:**
      - three groups, three roads;
      - "오늘은 싸우지 않아";
      - "네가 방금 얻은 분노";
      - Haren not back, no smoke.
      - The run records `ch10_anger`, with the toast "얻음: 분노".
  - The exit is **the crossroads below the ridge** (16화):
    - Sable on Elia's wrist; the paling leaves; the marker stone with nothing to read;
    - eight in dark coats and the adjuster in grey; the old man's arm touched and gone still;
    - "여긴 아니야." twice;
    - Elia's hand on the book, not taking it out;
    - the bag carried;
    - "벨트 도로까지 저런 곳이 셋 더 있다." "남은 건 가지고 가.";
    - Arrel goes over the ridge first.
    - Ends with `ch10_complete`, `ch10_not_here` and `canon_ch11_belt_road_ready`.
  - After the chapter: a chest, a clue (the bowl left at a door), encounters (the Seam's own pool).
- **Scenes:** six runtime scenes in both languages. Speakers are Arrel, Elia, Haren, Sable (the blind portrait `sable_calm`) and a boy ("소년", no portrait). The grey adjuster does not speak.
- **Goals and main quest:**
  - Goals: Sable's step, then the east road, then the square's west end, then the west exit.
  - Main quest: three steps (Sable, the horn, the crossroads). The open last step is now "벨트 도로로: 저런 곳이 셋 더".
- **Look:**
  - the Seam's own ground textures;
  - a morning dressing: cooler key, paler sky, lamps lower, fewer motes;
  - the cottages and lantern posts as in Chapter 6;
  - bundles of bedding past the bridge and at the doors;
  - Sable's low stone step.
- **Art:**
  - Sable seated and listening with her eyes half shut (`world_rewrite_sable_witness_v3`, blind canon), newly imported;
  - ~~the Seam's living streets for the bridge~~ **removed in S378**: a calm night, cloths still on the walls, and Sable at a door with a lantern;
  - ~~three small figures on a road (`ch16_eastward_road`)~~ **removed in S378**: drowned salt flats, made for the old storyline's trio. Its import was deleted.
  - (Found by Gemini's T-1 review.) The S375 line "전에 그런 사람들을 봤어" now reads "겪었어", and Haren's cousin is restored in `ms_return_road`.
  - A first choice for the ridge, `story_ch7_crossing_the_ridgeline`, shows four people. It was dropped after the capture review.

## Canon notes
- ~~One Sable line kept the manuscript's "봤어"~~. **Updated in S378** to "전에 그런 사람들을 겪었어", per Gemini's T-1 recommendation, removing the seeing verb.

## Verification
Native UE 5.8.2, D3D 1280x720.
- New `Memoria.S375.Return`:
  - Chapter 10, its level and Korean name;
  - the same tiles as the Seam;
  - north-east entry, west exit, triggers on open ground;
  - six scenes; the dry channel leads here;
  - key lines;
  - only the five speak, and Sable's face is the blind one;
  - Korean toasts and title;
  - goals; the main quest at Sable's step and then the Belt road.
- New `MemoriaVisual.S375Return` on `L_SeamReturn` (development run at Chapter 10):
  - plays from the road down to the crossroads;
  - the two halves and anger, with their toasts;
  - the last line, the flags and the card.
- S374's main-quest check now ignores the later Chapter 10 steps. Its road check now also expects the clearing to lead to `seam_return`. The artwork count is 81.
- Captures inspected (`s375/focus1`):
  - Sable's step shows the listening picture and her blind portrait;
  - the boy speaks without a borrowed face;
  - the village has bundles and lit lantern posts.
- Wide regression, every `MemoriaVisual.*` + `Memoria.S36*` + `Memoria.S37*` + `Memoria.Journal*` + `MemoriaSave.S362*` + input: `s375/wide1` **PASS 80/80**. It ran after the road-image change, and its capture shows the road west at the crossroads.

## Limits
- Chapter 11 (17화 「읽는 자」 onward) is not ported. Chapter 10 ends on "능선 아래 교차로 · 11장으로 가는 길은 아직 준비 중".
- The crossroads is a scene played at the west exit, not a walkable map. The adjuster's clubs are not yet an enemy mechanic.
- English lines are this port's translation.
