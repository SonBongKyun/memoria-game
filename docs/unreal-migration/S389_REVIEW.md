# S389 — Chapter 15, the dry wells, from the final manuscript (Opus, 2026-11-24)

Branch `claude/s366-upgrade`, on top of `8c469ba6`. Team task **T-13** on `TEAM-BOARD.md`.

## Source
- 최종본 24화 「두 노트」, read in full.
- Game Chapter 15 = 24화 alone, as Gemini's T-11 brief recommends (`docs/reviews/GEMINI_T11_BRIEF_24_26.md`, Option A).
- The episode is told from Tobias's side. No fight, so **no battles are placed**.
- **Ground:** the unused Godot `seam_outskirts` map, through `GROUND` (map id `dry_wells`, level `L_DryWells`, "마른 우물").
  - **Tile renames:**
    - the rim (`WALL`) becomes `EDGE`: open pasture, not a wall;
    - the crack clusters (`CRACK`) become `TREE`: the topped poplars;
    - the ledges (`LEDGE`) become `RIDGE`: field stone;
    - the other void rocks (`VOID_ROCK`) become `RUBBLE`: old stones.
  - **The wells:** the two rock clusters either side of the path become a new solid tile, `WELL`. These are the two dry wells, five tiles apart ("서른 걸음쯤").
- **Cast:**
  - Arrel, Elia and Tobias. The first chapter where three walk together.
  - `party_tobias` is set by Chapter 14, so the T-12 companion follows.
  - A development run started straight into Chapter 15 or later now sets `party_tobias` too (`EnterChapterMap`).
  - **No Kairos.** He first appears in 26화.

## What changed
- **Road:** the farmhouse's exit (Chapter 14) now leads here (Chapter 15).
- **Map:** in from the farmhouse to the south; down through the pasture to the hollow; out north to the fork.
  - **Arrival** (sequence): **bread** (`ms_wells_bread`):
    - Tobias tears the bread;
    - "칼 있다며." "잘라. 쓰고 넣어.";
    - "책은." "아직 같습니다.";
    - "물부터 찾아야겠어."
  - **Story places:**
    - **The pasture** (`ms_wells_pasture`, the path):
      - Arrel, then Elia, then Tobias;
      - "아까 한 말은 책을 열어 달라는 뜻은 아니었습니다.";
      - the topped poplars;
      - three hours; Arrel raises his hand.
    - **The dry wells** (`ms_wells_dry`, between the wells):
      - pebbles, "물소리는 없었다.";
      - the books set three palms apart;
      - "표지가 들렸다." No wind; only the book opens.
      - "혼자 펴진 건 처음입니다."
      - The run records `ch15_books_opened`.
    - **Six lines** (`ms_wells_six`, Tobias's stone):
      - writing appears with no spreading ink and no pen;
      - "베르단 도로 꺾임에서 남쪽 사 마일, 셋째 목자 제단 갈라진 돌 밑.";
      - the vault under a long stone building;
      - the orchard "that was";
      - "여섯 곳입니다."
    - **Six angles** (`ms_wells_angles`, Elia in the shade by the first well):
      - angles from a centre, with no distance;
      - the towers, and the big one in the white city, are copies;
      - "여기서는 이 여섯을 원본이라고 불러.";
      - "잠들어 있어." "그런데 듣고 있어." "지금은 우리 쪽도. 조금."
      - The run records `ch15_six_originals`, with the toast "여섯 원본".
    - **The blood** (`ms_wells_blood`):
      - the nosebleed and the scarf;
      - "내 쪽에서 내놓으면 알아차릴 것 같아.";
      - "지금은 안 할 거야."
      - Both books are closed: "그는 덮었다."
  - **The exit is the fork** (`ms_wells_arcane`, north):
    - the warmth leans back over the ridge;
    - "아르케인입니다." "제가 살던 도시.";
    - "이 배지는 제 겁니다. 두 사람 몫은 아니에요.";
    - "갈림까지는 제가 앞에 가겠습니다.";
    - the last line: two sets of footsteps follow him up.
    - The run records `ch15_toward_arcane`, `ch15_complete` and `canon_ch16_arcane_ready`.
  - After the chapter: a chest (by the poplars), a clue (a well's rim), encounters (the colorless waste pool).
- **Look:**
  - The dressing `Wells`: a dry pasture past noon, plain warm daylight, no lamps.
  - The set pieces (`BuildSetPieces`, `dry_wells`):
    - two square rims of stacked stone, the corners uneven, dark inside, no bucket frame;
    - the broad flat stone between them;
    - topped poplars: a thick trunk cut at a height, thin branches grown again above the cut;
    - field stone on the ledges;
    - dead trees beyond the north and east rims.
- **Art:**
  - **No picture under the scenes: they play on black, as in Chapter 14.**
    - Gemini's T-11 audit found no SAFE picture for 24화: glowing books, a staff, an indoor library, a lantern, or the dusk road, which hangs a lit lantern.
  - No new imports.
- **HUD:**
  - Goals:
    1. "목초지 아래로: 물을 찾아";
    2. "움푹한 땅: 우물 둘";
    3. "토비아스의 돌: 새 글씨";
    4. "우물 옆 그늘: 엘리아의 각도";
    5. "엘리아 곁으로";
    6. "북쪽 갈림 → 능선 너머".
  - Main quest: "물부터: 움푹한 땅의 우물", "열린 두 노트: 묻힌 여섯 곳", "능선 너머 아르케인 쪽으로". The next step reads "아르케인: 성문, 배지는 하나".
  - The chapter card: "15장 완료 · 갈림길".
- **Earlier tests:**
  - The S374-S385 main-quest loops skip `ch15_`.
  - S385 expects the farmhouse's road to lead to the wells, and its next main step to read "물부터: 움푹한 땅의 우물".

## Tests
- `Memoria.S389.DryWells` (unit):
  - **Map:**
    - map, level, Korean name;
    - the seam_outskirts ground as a pasture with poplars;
    - two wells either side of the path;
    - spawn south; exit north at the fork;
    - every trigger on open ground; the books open between the wells;
    - no decorations, no ambient people, no splash;
    - the roads in and out.
  - **Scenes:**
    - seven manuscript scenes, 175 rows, and their key lines;
    - only Arrel, Elia and Tobias speak; Tobias wears his portrait;
    - no picture on any row;
    - no "빛" in any line: nothing glows;
    - no Kairos.
  - **Korean and HUD:** toast and title in Korean; the six goals; the main quest before, during and after.
- `MemoriaVisual.S389DryWells` (renderer):
  - plays the chapter from the farmhouse to the fork;
  - Tobias is in the party, and one companion actor walks with them;
  - checks `ch15_books_opened`, `ch15_six_originals` with its toast, `ch15_toward_arcane`, `ch15_complete` and `canon_ch16_arcane_ready`;
  - every one of the 175 rows is shown, then the card;
  - captures go to `Saved/Validation/S389Wells/`.
- Results: see the commit message.

## Limits
- Elia's field figure is still blond and carries a staff. Grok's T-15 fixes that (deferred to 2026-11-25).
- The hollow is not a dip in the ground; the wells stand on the level pasture.
- English lines are this port's translation; a few narration sentences are trimmed, and spoken lines are kept whole.
