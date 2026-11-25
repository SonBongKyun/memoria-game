# S393 — Chapter 16, Arcane, from the final manuscript (Opus, 2026-11-25)

Branch `claude/s366-upgrade`, on top of `81ab6528`. Team task **T-16** on `TEAM-BOARD.md`.

## Source
- 최종본 25화 「고해소」 and 26화 「카이로스 주저하다」, both read in full.
- Game Chapter 16 = these two episodes, as Gemini's T-11 brief recommends (`docs/reviews/GEMINI_T11_BRIEF_24_26.md`, Option A).
- No fight and no drawn sword, so **no battles**. Random encounters are off inside the city: `encounters_gate` is `ms_never`.
- **Ground: hand-authored, not read from a Godot map.**
  - Arcane is the story's ground from 25화 to at least 30화 (Gemini's T-14 brief), so it gets a ground of its own: `HANDMADE['arcane_city']` in `export_chapter_maps.py`.
  - `verdan_market.gd` only lends its tile size.
  - Map id `arcane_city`, level `L_ArcaneCity`, "아르케인".
  - Tiles: `ROAD`, `WALL`, `BOOTH`, `BENCH`, `WINDOW`, `CISTERN`, `DOOR`.
  - The route, as the manuscript has it:
    - the south gate;
    - the street north to the first square and its bread window;
    - the road curving west behind the five booths on the second square's south side;
    - the low bench beside the third booth;
    - the alley north-west to the cistern;
    - the low-arched way east to the door with the brass ring.
- **Cast:**
  - **25화:** Arrel, Elia and Tobias (the companion walks with them), and Arcane's nameless people: the gate clerk, the confessor, the booth clerk, the old man, the child, and a man in line.
  - **26화:** Kairos alone, with the messenger, the confessor, the clerks, the old man and the child. **Kairos never meets the three.**
  - The nameless have no portrait. Their Korean names are 성문 서기, 고해관, 서기, 아이, 노인, 줄 선 사람, 심부름꾼 and 접수 서기.

## What changed
- **Road:** the wells' exit (Chapter 15) now leads here (Chapter 16).
- **Exit chains:** a chapter exit may now list `then` groups.
  - They play one after another, a beat apart, after the exit's own group. The chapter closes when the last one ends.
  - Code: `FMemoriaChapterExit::Then`; `ExitThen` and `ExitThenAt` in `AMemoriaChapterPresentation`.
  - Here, 25화's room is followed by 26화's five Kairos cutaways.
- **Map:**
  - **Arrival** (sequence): **the gate** (`ms_arcane_gate`):
    - only Tobias called in;
    - "제 이름 밑에 적으세요. 같이 머물 겁니다.";
    - "흩어지면 당신한테 먼저 묻겠습니다.";
    - "뒤꿈치가 안 따라가.";
    - "다른 때는 바닥을 보는 게 낫습니다."
  - **Story places:**
    - **The squares** (`ms_arcane_squares`, the first square):
      - the bread window: "제가 혼자 사면 빵만 사면 됩니다.";
      - five booths with a bell on each door frame;
      - eleven at the third booth;
      - "고해소입니다." "나도 알아."
    - **The third booth** (`ms_arcane_booth`):
      - the old man at the door frame;
      - "누구 애냐.";
      - "셋째. 퇴실 보조.";
      - Tobias explains, and Arrel says "그 전에."
    - **The bench** (`ms_arcane_burn`):
      - Arrel burns the warmth of a hand that once held his head;
      - four steps; "걷습니다."; "세 걸음. 벤치까지.";
      - the child: "조금만 앉아요."
      - The run records `ch16_burned_hand`, with the toast "연소: 머리를 받쳐 주던 손".
    - **The alley** (`ms_arcane_alley`):
      - his knee gives;
      - "태웠지." "뭘 태웠는지 알아?";
      - "어릴 때 머리를 받쳐 주던 손."
    - **The cistern** (`ms_arcane_cistern`):
      - the tower: "여기서 태우면 탑에 남습니다. 보는 사람이 없어도요.";
      - "여기선 펴지 마."
  - **The exit is the door with the brass ring** (`ms_arcane_room`):
    - "성문에 제 이름이 있습니다."
    - "찾으면 저부터 묻겠죠."
    - "앉아. 먼저."
  - **Then 26화, Kairos alone:**
    - `ms_kairos_report`: the report at 12:22; K-7; "관찰함. 개입하지 않음."
    - `ms_kairos_booth`: "직접 본 것만 적게."; the old man and the child on the bench; "오늘 고해는 끝났어."
    - `ms_kairos_gate`: "토비아스." and "동행 둘"; his viewing mark.
    - `ms_kairos_relay`: the squad diverted, a day and a half away; he keeps the witness check to himself.
    - `ms_kairos_unsent`:
      - the burning ink stays stoppered;
      - "미제출로 남깁니까?" "그렇게 적게.";
      - "개입했나?";
      - he turns south, toward the cistern.
      - The run records `ch16_unsubmitted`, with the toast "카이로스: 미제출".
    - The chapter closes with `ch16_complete` and `canon_ch17_hanna_ready`, and the card "16장 완료 · 미제출".
  - **The townsfolk** (from the arrival on, gate `ch16_gate`):
    - five in the queue at the third booth, still and facing its door (T-8);
    - four at the bread window;
    - the gate clerk.
  - After the chapter: a chest (by the cistern) and a clue (the gate's register board).
- **Look:**
  - The dressing `Arcane`: grey stone streets at noon, cold even light.
  - The set pieces (`BuildSetPieces`, `arcane_city`):
    - **Booths and bench:**
      - five one-person wooden booths, doors facing the square, a small brass bell on each frame;
      - the third booth's metal arm at a seated temple's height;
      - the low bench.
    - **Window and cistern:**
      - the bread window, with its counter and loaves;
      - the cistern's stone wall with water low inside, and steps down.
    - **The door with the brass ring.**
    - **The gate:**
      - stone piers and a lintel;
      - the clerk's table and the register board.
    - **The small tower** over the roofs.
  - **No lamp by a door** in Arcane at noon.
    - The first render showed lit door lanterns, and an upper storey on every wall that hid the streets. Both were removed.
- **Art:**
  - **The city's splash:** `env_gray_belt_panorama.png` (Gemini: SAFE), imported.
  - **Kairos's room:** `ms_kairos_report` and `ms_kairos_unsent` show `ch14_unsent_incident_report.png` (Gemini: SAFE for 26화), imported.
  - The relay office plays on black.
  - **Kairos's face:**
    - `kairos_neutral` now shows `assets/portraits/kairos_face_neutral.png` (imported), the plain face without the sigil-lit story shot.
    - This also changes his face in the Godot-era Chapter 9 dialogue.
  - ArtworkCoverage now expects 81 sources.
  - **Not used:** `ch14_confessor_hall`, `ch14_arrel_burn_slash`, any drawn-sword or glowing picture.
- **HUD:**
  - Goals:
    1. "첫 광장: 빵 창구";
    2. "둘째 광장: 셋째 부스";
    3. "셋째 부스 옆 낮은 벤치";
    4. "북서쪽 골목으로";
    5. "저수조의 돌벽으로";
    6. "낮은 아치 골목 끝: 놋쇠 고리 문".
  - Main quest: "아르케인: 성문, 배지는 하나", "셋째 부스: 노인과 아이", "저수조 뒤, 토비아스의 방". The next step reads "토비아스의 방. 어딘가에서 카이로스는 회신하지 않았다".
- **Earlier tests:** the S374-S389 main-quest loops skip `ch16_`. S389 expects the wells' road to lead to Arcane.

## Tests
- `Memoria.S393.Arcane` (unit):
  - **Map:**
    - map, level, Korean name;
    - its own tiles, and five solid booths on the second square's south side;
    - in by the south gate; the exit before the door with the brass ring;
    - every trigger on open ground; the old man's scene before the third booth; the burn by the bench;
    - no fights, no lamps, the city splash;
    - five in the queue, facing the booth's door;
    - twelve scenes, and the exit chain (the room, then Kairos's five);
    - the roads in and out.
  - **Lines:** the key lines of all twelve scenes.
  - **Cast and art:**
    - only the three, Kairos and the nameless speak;
    - Tobias's and Kairos's faces; the nameless have none;
    - **Kairos and the three are never in one scene**;
    - pictures only in Kairos's room, and black in the relay office.
  - **Totals and Korean:**
    - 312 rows;
    - the art imported; toasts and title in Korean;
    - the six goals; the main quest before, during and after.
- `MemoriaVisual.S393Arcane` (renderer):
  - plays the chapter from the gate to the room, then the five cutaways;
  - Tobias walks with them;
  - checks `ch16_burned_hand` and `ch16_unsubmitted` with their toasts, `ch16_complete` and `canon_ch17_hanna_ready`;
  - **every one of the 312 rows** is shown, then the card;
  - captures go to `Saved/Validation/S393Arcane/`.
- **The wide regression filter now needs `+Memoria.S39`**, because the S393 tests do not match `Memoria.S38`.
- Results: see the commit message.

## Limits
- The booths are plain wooden boxes with a dark doorway; the old man, the child and the confessor are in the scenes only, not standing figures.
- Elia's hair tint and Tobias's coat are editor-only until Grok's T-18 lands.
- `kairos_face_neutral.png` is a flatter, cel-shaded style than the painted portraits.
- English lines are this port's translation; a few narration sentences are trimmed or merged, and spoken lines are kept whole.
