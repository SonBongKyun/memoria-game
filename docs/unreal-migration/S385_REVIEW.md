# S385 — Chapter 14, the farmhouse under the ridge, from the final manuscript (Opus, 2026-11-23)

Branch `claude/s366-upgrade`, on top of `d98719d0`. Team task **T-10** on `TEAM-BOARD.md`.

## Source
- 최종본 22화 「공명」 and 23화 「세 번째 사람」, both read in full.
- Game Chapter 14 = these two episodes, as Gemini's T-7 brief recommends (`docs/reviews/GEMINI_T7_BRIEF_21_23.md`, Option A).
- No fight in 22-23화, so **no wardens and no battles are placed**.
- Ground: the Godot `drift_shelter` map, through `GROUND` (map id `ridge_farmhouse`, level `L_RidgeFarmhouse`, "능선 아래 농가").
  - The rim is renamed `EDGE`: open field, not a wall.
  - The slab ring (`CONCRETE`) is renamed `WALL`, so it stands as the farmhouse's high stone walls.
  - Tile edits:
    - the back wall's gap is closed: three high walls, and of the south side only the corners;
    - the road comes down from the ridge east of the house and passes the open side;
    - the old path behind the house is gone;
    - the shelter's east way out is closed.
- Cast:
  - Arrel and Elia throughout.
  - **Tobias only from 23화.** He is first named in the first line of `ms_ridge_slope`; 22화's scenes never name him. He speaks with his own portrait, `tobias_neutral`.
  - **Not present:**
    - Sable (gone since 21화);
    - Malet (filed);
    - the man hoeing and the person at the signal tower. They are in the narration only, as figures who never look up.

## What changed
- **Road:** Verdan's exit (Chapter 13) now leads here (Chapter 14).
- **Map:** in from the fields to the south; the night and first light inside the walls.
  - **Arrival** (sequence): **the furrows' morning** (`ms_ridge_morning`):
    - "그대로인지 볼게." / "북서쪽.";
    - "많이는 못 잤어." "그건 보이네.";
    - the cover not lifted once.
  - **Story places:**
    - **The fields** (`ms_ridge_fields`, on the field road):
      - the man hoeing who never looks up;
      - "더 잘 느껴져.";
      - "네가 길을 보고 있잖아."
    - **The signal tower** (`ms_ridge_tower`, west):
      - the flag halfway up the pole;
      - "저길 지나야 해?" "길로 가면.";
      - over the broken north wall, the bag strap slipping from his right hand.
    - **The stumps** (`ms_ridge_stumps`, north-west):
      - the wet boot;
      - he falls asleep on the stump;
      - "빵 먹고도 한참."
    - **The farmhouse** (`ms_ridge_house`, the open side):
      - three high walls and a corner;
      - "오늘은 여기서 자자.";
      - "열면 더 알 수 있을 것 같아.";
      - "그때도 네 손이었어.";
      - "안 열 거야." "지금은?" "지금은."
    - **The night** (`ms_ridge_night`, under the high wall):
      - "불은 피우지 않았다.";
      - "안 만지고 있었어.";
      - "그쪽이 오는 걸까.";
      - no light leaks from the coat;
      - the book goes into the bag;
      - "손이 멈추는 건 볼 수 있어.";
      - the light under the clouds in the north-west;
      - the sword drawn close to his left hand.
      - The run records `ch14_book_awake`.
    - **The slope** (`ms_ridge_slope`, the broken corner):
      - 23화 from its first line, "토비아스는 비탈 중간에서 멈췄다.";
      - his Arcane years;
      - thirty paces;
      - the left-hand draw, then "거기서." "서 있습니다.";
      - the seal-less book;
      - "앉아.";
      - the bag;
      - Elia's book on her knees, cover not lifted;
      - "십 년 가지고 있었습니다."
    - **Ten years** (`ms_ridge_ten_years`, the edge of the road):
      - Selen's desk;
      - "셀렌에게 딸 아미가 있었는데 셋에 하얘졌다.";
      - the confession booth;
      - "삭제를 먹습니다.";
      - "돌려준 적도 없습니다.";
      - "여기까지 오라고 한 적은 없어.";
      - "제 것도 그렇습니다."
  - **The exit is first light** (`ms_ridge_third`), inside the walls:
    - "이름." / "토비아스입니다. 동부국 삼급 등록 서기.";
    - "나는 아렐." "엘리아야.";
    - "같이 가고 싶습니다.";
    - "네 책은 네가 들어.";
    - he sits a few paces from Elia;
    - the last line: he reaches for the knife, looks at Arrel, and takes only the cloth.
    - The run records `party_tobias`, with the toast "합류: 토비아스", then `ch14_complete` and `canon_ch15_two_notes_ready`.
  - After the chapter: a chest (among the stumps), a clue (the warped shelf), encounters (the colorless waste pool).
- **For T-12 (Grok):** `party_tobias` is the flag that spawns his field companion.
- **Look:**
  - The dressing `Ridge`: dry fields, a grey sky, cool light, no lamps.
  - The set pieces (`BuildSetPieces`, `ridge_farmhouse`):
    - the warped shelf with two round marks on the west wall;
    - the broken bowl;
    - leaves under the north wall;
    - dry wood under the east wall, never burned;
    - the signal tower west beyond the rim, its flag halfway, with a low building and a cart;
    - rows of stumps north past the rim, the ridge's trees beyond them.
  - **No roof, no awning, no lantern:** the Drift shelter's roof, awning and lanterns (`BuildWalls`) are skipped on this map.
    - The first render showed a lantern post inside the walls. 23화 says "불빛은 없었다".
- **Art:**
  - **No picture under the scenes: they play on black.**
    - The empty dusk road (`chapter_splash_belt_waystation.png`, Gemini's RISKY pick) hangs a lit lantern and glowing blue sigils. It contradicts "불은 피우지 않았다".
    - The other candidates showed other people, a lantern, embers, floating ruins, or a blond Elia.
  - **Tobias's desk** (`world_rewrite_tobias_ink_v3.png`, Gemini: SAFE for the flashback only) plays under his Arcane years: rows 4-7 of `ms_ridge_slope` and rows 0-2 of `ms_ridge_ten_years`. The present goes back to black, and nobody speaks over the desk.
  - New import: `T_tobias_face_neutral` (`assets/portraits/tobias_face_neutral.png`).
  - No `ch13_*` CG, no open or glowing book, no lantern, no Sable.
  - Widget fix (`MemoriaDevelopmentNarrativeWidget`): a step with no picture drew a white image. It now draws black.
- **HUD:**
  - Goals:
    1. "밭 가장자리 길로";
    2. "서쪽: 무너진 돌담 너머";
    3. "북쪽 그루터기 사이로";
    4. "능선 아래 농가: 열린 쪽 앞의 길";
    5. "높은 벽 아래: 불 없는 밤";
    6. "무너진 모서리: 내려오는 길을 지킨다";
    7. "길 가장자리: 그가 앉은 돌";
    8. "벽 안쪽으로: 동이 튼다".
  - Main quest: "북서쪽: 신호탑을 돌아 그루터기 사이로", "능선 아래, 불 없는 밤", "능선에서 내려온 사람". The next step reads "셋이서 북서쪽으로".
  - The chapter card: "14장 완료 · 세 번째 사람".
- **Earlier tests:**
  - The S374/S375/S377/S379/S382 main-quest loops skip `ch14_`.
  - S382 now expects Verdan's main quest to go on to "북서쪽: 신호탑을 돌아 그루터기 사이로".

## Tests
- `Memoria.S385.RidgeFarmhouse` (unit):
  - **Map:**
    - map, level, Korean name;
    - the drift_shelter ground with its rim as `EDGE`;
    - three high walls and a south side of corners only;
    - spawn south; the exit inside the walls;
    - every trigger on open ground; the watch inside the broken corner;
    - no decorations, so no fire or lamp, and no ambient people;
    - Verdan's road leads here; the exit goes on to Chapter 15.
  - **Scenes:** nine manuscript scenes and their key lines.
  - **Cast:**
    - only Arrel, Elia and Tobias speak;
    - Tobias speaks only in 23화's three scenes, with his portrait;
    - 22화 never names him.
  - **Art:**
    - only black or his desk, and the desk only in 23화's past, with nobody speaking over it;
    - the portrait and the desk are imported.
  - **Korean and HUD:** the books stay shut; toasts and title in Korean; the eight goals; the main quest before and after.
- `MemoriaVisual.S385RidgeFarmhouse` (renderer):
  - plays the chapter from the fields to first light;
  - checks `ch14_book_awake`, `party_tobias` with its toast, `ch14_complete` and `canon_ch15_two_notes_ready`;
  - checks that **every one of the 256 rows** was shown, and the card;
  - captures go to `Saved/Validation/S385Ridge/`.
- Results: see the commit message.

## Limits
- **Elia's portrait** (`elia_neutral` = `character_shots/elia_story_v2.png`) is blond, with an anchor. 23화 gives her silver hair ("은색 머리카락이 코트 어깨에 걸려 있었다"). It is the same portrait every chapter has used since Chapter 1; not changed here.
- **Elia's field figure** carries a staff, as in every chapter.
- **Chapters 11-13** play their nights over the dusk road with its lit lantern too, for example Chapter 13's furrow night. Gemini's T-11 reviews that backdrop.
- The scenes trim a few narration sentences; spoken lines are kept whole.
- English lines are this port's translation.
