# S377 — Chapter 11, the roofless farmhouses, from the final manuscript (Claude/Opus, 2026-11-22)

Branch `claude/s366-upgrade`, on top of the team setup (`4a8de43c`). Team task **T-3** on `TEAM-BOARD.md`. (S376 is Grok's T-2, the wardens.)

## Source
- 최종본 17화 「읽는 자」 and 18화 「펠의 손」 (final-1), read in full. Game Chapter 11 = these two.
- Ground: the Godot `colorless_waste` plain, through `GROUND` (map id `roofless_farms`, level `L_RooflessFarms`, "지붕 없는 농가").
  - Its winding path stays the road.
  - The `PILLAR` clusters stand as the farmhouse walls.
  - The `RIDGE` clusters are field-boundary stones; the `MARKER` tiles are standing stones.
  - The rim tile is renamed `EDGE`, so the open fields get no masonry border; trees stand beyond it.
- Ground textures are reused: the Belt's dry earth for soil, the clearing's trodden path for paving. No new ground assets.

## What changed
- **Road:** the Seam's west exit (Chapter 10) now travels on to the fields (Chapter 11).
- **Map:** in from the south (over the ridge), out to the north.
  - Arrival: the road.
    - The book heavier and warm through the coat.
    - "돌이 있니?" "아니. 옷이 걸렸어."
    - "가장자리 밟지 마. 부서져."
    - The farmhouse behind the mound, no roof, the west wall broken off at the waist.
  - Story places:
    - **The farmhouse** (17화): "오늘은 여기까지 가자."
      - The book opened knowingly; the third line.
      - The relay lines: "중계를 연다… 중계를 닫지 마라. 닫는 것은 네 것이 아니다. 여는 자가 곧 중계다."
      - The nosebleed; "중계는 몸이 아니다. 몸은 중계의 비용이다."
      - Sable's "아이." "책이야." "아니야."
      - A reason not to tell him yet.
      - The run records `ch11_relay`, with the toast "백 노트: 중계".
    - **The night:**
      - "네가 먼저 자." "그건 보여."
      - The four notches turned toward her.
      - "코." "아무것도 아니야."
      - The fourth line beginning; her palm on the warm book.
    - **The morning** (18화):
      - The fourth line joined: "펠.", and a hand held long; the hand that left the writing, no face.
      - "밝니?" "길은 보일 거야."; Arrel's right hand stopping on the belt.
      - The run records `ch11_fel_name`, with the toast "이름 하나: 펠".
    - **The glove:**
      - A grey leather glove, cuff open, empty.
      - The cleaners' smell from the Belt.
      - Off the road onto the southern stones; he does not take it.
  - The exit is **the dry channel and the second farmhouse**:
    - "책이 움직여?" "내가 대고 있어." "나한테 할 말 없어?" "지금은 안 할래." "알았어.";
    - two walls leaning on each other, no fire;
    - "추우면 옮길게.";
    - "펠." in her sleep;
    - "누구냐고 묻기 전에, 자기가 들었다는 말부터 해야 했다."
    - Ends with `ch11_complete`, `ch11_fel_heard` and `canon_ch12_belt_road_ready`.
  - After the chapter: a chest, a clue (the glove still in the ditch), encounters (the waste's own pool).
- **Scenes:** six runtime scenes in both languages. Only Arrel, Elia and Sable speak. Fel is a name, not a speaker. Haren does not come back.
- **Goals and main quest:**
  - Goals: the farmhouse, the night, the threshold, the road off the ridge, the north exit.
  - Main quest: three steps (the farmhouse, the glove, the second farmhouse). The open step is now "벨트 도로로: 청소하는 자들이 다니는 길을 피해".
- **Look:**
  - late-light fields: straw and dust, no lamps;
  - pale field-stone walls on the `PILLAR` tiles, waist-high where broken and full height at the corner, with roof-tile shards inside;
  - boundary stones and standing stones;
  - trees beyond the rim (none along the south, where the camera looks over);
  - past the north exit, two walls leaning on each other.
- **Art:**
  - ~~the three small figures on the road (`ch16_eastward_road`)~~ **replaced in S378**: the splash is now the Belt road at dusk with nobody on it (`chapter_splash_belt_waystation`), and the road and second-farmhouse lines play over it;
  - ~~the warm book in two hands, no faces (`story_ch3_blank_book_warmth`)~~ **removed from the farmhouse in S380** (Gemini T-4): the hands are Arrel's, in gauntlets, in a library.
  - No new imports. Pictures with Tobias reading relay books were rejected (he appears from 23화).

## Verification
Native UE 5.8.2, D3D 1280x720. Build slot taken by Opus on TEAM-BOARD (Grok had not started a build yet).
- New `Memoria.S377.Farms`:
  - Chapter 11, its level and Korean name;
  - the `EDGE` rename;
  - south entry, north exit, triggers on open ground;
  - the first trigger inside a standing corner;
  - six scenes; the Seam's road leads here;
  - key lines; only the three speak, and Sable's face is the blind one;
  - Korean toasts and title;
  - goals; the main quest at the farmhouse and then the Belt road.
- New `MemoriaVisual.S377Farms` on `L_RooflessFarms` (development run at Chapter 11):
  - plays from the road to the second farmhouse;
  - the relay and the name, with their toasts;
  - the last line, the flags and the card.
- S374's and S375's main-quest checks now ignore the later Chapter 11 steps. S375 also checks that its road leads to the farmhouses.
- Captures inspected (`s377/focus1`, `focus2`). Two things fixed from them:
  - the walls rendered as black cubes; they now use a pale field stone (`FieldStone`, a new stone kind in `Focus`);
  - the south-side trees filled the opening frame.
- Wide regression, every `MemoriaVisual.*` + `Memoria.S36*` + `Memoria.S37*` + `Memoria.Journal*` + `MemoriaSave.S362*` + input: `s377/wide1` **PASS 82/82**, after the wall and tree fixes.

## Limits
- Chapter 12 (19화 onward) is not ported. Chapter 11 ends on "두 번째 농가 · 12장으로 가는 길은 아직 준비 중".
- The dry channel and the second farmhouse are a scene played at the north exit, not a walkable map.
- The walls are whole-tile blocks over the collision tiles, not thin walls with a doorway.
- English lines are this port's translation.
