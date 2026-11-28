# S396 — Chapter 17, leaving Arcane, from the final manuscript (Opus, 2026-11-27)

Branch `claude/s366-upgrade`, on top of `8714a82f`. Team task **T-19** on `TEAM-BOARD.md`.

## Source
- 최종본 27화 「조용함의 무게」, 28화 「노래하는 자」, 29화 「지하의 노래들」 and 30화 「어머니의 자장가」, read in full, with Gemini's T-14 and T-17 briefs.
- Game Chapter 17 = 27-30화. 30화 continues 29화's room directly and ends through the south gate.
- No fight, so **no battles are placed**.
- **Ground:** Arcane's hand-authored ground again (`HANDMADE['arcane_leave']` is `HANDMADE['arcane_city']`), map id `arcane_leave`, level `L_ArcaneLeave`, "아르케인".
- **Cast:**
  - Arrel, Elia, Tobias (companion) and the gate clerk.
  - **Hanna never speaks.** What she writes on her board stands as narration, word for word ("책은 닫아 둬.", "네가 정해."). She has no portrait; the old "Han" art is not used.
  - **No Kairos.**

## What changed
- **Road:** Arcane's exit (Chapter 16) now leads here (`next_map: arcane_leave`). Kairos's five cutaways still play first, then the card, then the road.
- **Map:** out of the door with the brass ring (spawn on the tile in front of it, where Chapter 16 went in); west past the cistern; down through the squares and the street to the south gate.
  - **Arrival** (sequence, ten scenes, all in Tobias's room, on black):
    1. `ms_leave_quiet`: the short candle, water; "읽는 것도 피 나."; Arrel sleeps against the wall.
    2. `ms_leave_relay`: the book's new lines, "중계는 손으로 열리지 않는다." to "끝까지 열면 죽는다."; the nosebleed. Records `ch17_relay_read`.
    3. `ms_leave_cost`: Tobias asks; "읽는 것하고 여는 건 달라."; "내가 먼저 알아야 해."; humming at the door; "아는 사람입니다."
    4. `ms_leave_hanna` (28화): the grey coat, the hatband's darker patch, the palm greeting; "한나 씨입니다."; the board: "책은 닫아 둬.", "오늘 안에 나가.", "셋째 종까지 남문.", "마차는 둘째 마일."
    5. `ms_leave_east`: "걸어서 간다.", "동쪽.", "여기서는 못 쓴다.", "네가 정해."; "마차를 보는 데까지야."
    6. `ms_leave_wait`: "머물러. 두 시간.", "먼저 다녀올 곳이 있다."; two notes, Elia's hand to her throat; "십 년입니다."
    7. `ms_leave_printer` (29화): ten years ago in Lumea; "앉으시오.", "이레 뒤에 사라졌소.", "우리가 다다르는 것들을."; the song through the stone wall; "가운데가 닮았어."
    8. `ms_leave_songs`: "한나 씨도 누구에게서 받아요."; Hanna comes back humming; "다섯 음을 지났는데, 한나는 멈추지 않았다."
    9. `ms_leave_lullaby` (30화): the cart ("여덟째 마일 마당.", "약속받지 못했다."); three black loaves; "자장가야."; "작은 아이야. 천천히. 그다음에 강."; "오늘은 안 불러요."; the warmth he burned does not come. Records `ch17_lullaby_words`, toast "떠오름: 작은 아이야, 천천히, 강".
    10. `ms_leave_door`: "나는 남문까지."; "셋이 보고 정한다고 했다."; out, the brass ring caught by hand. Sets **`party_hanna`**.
  - **Story places:**
    - **The cistern** (`ms_leave_cistern`, the road above it): Hanna quickens; Arrel's boot drags; she comes back and walks beside them.
    - **The low stone inside the south gate** (`ms_leave_gate`): 'second mile' once more; Hanna turns into another alley. Sets **`hanna_gone`**.
  - **The exit is the south gate** (`ms_leave_out`): "동행은요?" "둘입니다."; Tobias on Arrel's left, Elia keeping to his shortened stride. Records `ch17_complete` and `canon_ch18_east_road_ready`; the card reads "17장 완료 · 남문".
  - After the chapter: a clue at the low stone. No chest.
- **Look:**
  - The same `Arcane` dressing and set pieces (booths, bench, bread window, cistern, the door, the gate), no lamps by day.
  - Ambient: four at the bread window and the clerk at the gate. The booths' queue is gone.
- **Art:**
  - The room's scenes play on black. Each of the ten opens on `Dark`; without it the map's splash (the city from above) showed through the room, as the first captures showed.
  - The two street scenes open over `env_gray_belt_panorama.png` (Gemini T-11/T-14: SAFE). No new imports.
- **HUD:**
  - Goals: "한나를 따라 저수조 쪽으로", "거리를 지나 남문 안쪽의 낮은 돌로", "남문 아래로: 성문 서기".
  - Main quest: "토비아스의 방: 책, 그리고 노래하는 사람", "한나와 함께, 셋째 종 전에 남문으로", "남문 밖으로: 동행 둘". The next step reads "동쪽으로 가는 마차까지 두 마일. 아직 정한 건 없다".
- **Earlier tests:**
  - The S374-S393 main-quest loops skip `ch17_`.
  - S393 expects Arcane's road to lead here and its next main step to be Chapter 17's room.

## For T-21 (Grok, Hanna's companion)
- `party_hanna` is set when `ms_leave_door` ends (the last arrival scene), so she should appear in front of Tobias's door and follow.
- `hanna_gone` is set when `ms_leave_gate` starts, at the low stone inside the south gate (tiles 11-13, row 14).
- **The point she walks to:** another alley, away from the gate street. S397 gives it as `MemoriaManuscript::HannaLeavingPoint`, tile (20, 10): the east end of the road past the squares. That keeps her clear of the queue at the bread window, row 12.

## Tests
- `Memoria.S396.ArcaneLeave` (unit):
  - **Map:**
    - map, level, Korean name;
    - the same ground as Chapter 16, tile for tile;
    - spawn in front of the door, where Chapter 16 went in;
    - the exit at the south gate;
    - two triggers on open ground, the cistern before the stone;
    - no fights, no lamps, the city splash;
    - no queue at the booths;
    - one clue;
    - the roads in and out.
  - **Scenes:**
    - thirteen manuscript scenes (ten in the arrival), 414 rows, and their key lines;
    - only Arrel, Elia, Tobias and the gate clerk speak; Hanna never does;
    - Tobias wears his portrait, Elia hers, the clerk none;
    - pictures only in the two street scenes, and only the panorama;
    - no Kairos.
  - **Korean and HUD:** title, subtitle, toast and card place in Korean; the three goals; the main quest through to the step after.
- `MemoriaVisual.S396ArcaneLeave` (renderer):
  - plays the chapter from the door to the gate;
  - Tobias walks with them;
  - checks `ch17_relay_read`, `ch17_lullaby_words` with its toast, `party_hanna` at the door, `hanna_gone` at the stone, `ch17_complete` and `canon_ch18_east_road_ready`;
  - every one of the 414 rows is shown, then the card;
  - captures go to `Saved/Validation/S396Leave/`.
- Results: see the commit message.

## Limits
- ~~Hanna has no field figure until T-21 lands.~~ S397 integrates T-21: see below.
- The room is not built: its scenes play on black, as Tobias's room did in Chapter 16.
- English lines are this port's translation. A few narration sentences are trimmed. Spoken lines and every board line are kept whole.

## S397: T-21 integrated (Hanna in the field)
- Grok's `AMemoriaHannaCompanion` and `M_HannaLook` (`ab2eb27f`, cherry-picked) are in. Her look:
  - a grey coat and short hair with white strands at the back;
  - a hat whose band has a darker patch;
  - a bag with a buckle;
  - no weapon, no lantern, no glow.
- **Joining:** `party_hanna` is set when `ms_leave_door` ends. The chapter then calls `AMemoriaHannaCompanion::Join` at once, so she comes out behind Tobias without a reload. The load-time spawn blocks still cover reloads.
- **Leaving:** when `ms_leave_gate` starts, the chapter calls `SetLeavingPoint(ToWorld(HannaLeavingPoint))`, then sets `hanna_gone`. She walks off and is removed. A reload with `hanna_gone` set does not bring her back.
- **The replay checks:**
  - no Hanna before the door;
  - one Hanna from the door, walking behind Tobias (`S396HannaFollows.png`, Arrel walked four seconds west from the door);
  - still one after the cistern;
  - at the stone she walks off and is removed before the gate (`S396HannaLeaving.png`);
  - none under the gate.
- **Limit:** the companions spawn south of Arrel, inside the wall block below the door, and come out once he walks. This is the same spawn offset Tobias has always used. They pass through walls while catching up.
