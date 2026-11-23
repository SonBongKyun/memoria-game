# S382 — Chapter 13, Verdan again, from the final manuscript (Opus, 2026-11-23)

Branch `claude/s366-upgrade`, on top of `b8df8e81`. Team task **T-9** on `TEAM-BOARD.md`.

## Source
- 최종본 21화 「다시 베르단」 (final-1), read in full. Game Chapter 13 = this episode only.
  - It follows Gemini's T-7 porting brief (`docs/reviews/GEMINI_T7_BRIEF_21_23.md`): 21화 alone, then 22-23화 as Chapter 14.
- No fight in 21화, so **no wardens are placed**.
- Ground: the Godot `verdan_market` town, through `GROUND` (map id `verdan_return`, level `L_VerdanReturn`, "다시 베르단").
  - Same tiles as the Chapter 2 market. The DOOR tiles south of the alley ring stand as the Sump's door.
- Cast: Arrel, Elia, Sable (only until she turns back), and the oil vendor (`Vendor` / "상인", no portrait, no name).
  - **No Malet**: he is filed. **No Tobias**: he appears from 23화. **No wall** at Verdan (21화: "성벽은 없었다").

## What changed
- **Road:** the basin's exit (Chapter 12) now leads to Verdan (Chapter 13).
- **Map:** in from the north down the low slope, out by the east road.
  - Arrival (sequence):
    - **The road** (`ms_verdan_road`):
      - "네 시간.";
      - Elia gives Sable her left arm;
      - the second signal tower, its flag at the top this time;
      - the smell of oil comes first; no wall.
    - **The farewell** (`ms_verdan_farewell`):
      - "나는 여기까지다.";
      - Sable has the people who walked west to reach;
      - her two warnings, about not spending all his anger on one man at a window, and about catching what the girl drops before asking why;
      - Elia sets Sable's hand on the low stone wall: "갈림에서 길 잘 물어.";
      - the shoulders go behind the bend before the head.
      - The run records `ch13_sable_gone`, with the toast "작별: 세이블".
  - Story places:
    - **The market** (`ms_verdan_market`, the square):
      - stalls packing up, a cart going out with chair legs under its cover;
      - "전에는 이 시간에도 시끄러웠어.";
      - the public reservoir and the barn they slept in after the trade.
    - **The Sump** (`ms_verdan_sump`, below the DOOR tiles):
      - the stone stair between the tannery and the closed apothecary;
      - cord from the frame to the door, red wax where the cords cross: the Bureau seal;
      - no sound below. "기다려 보자."
    - **The oil stall** (`ms_verdan_vendor`, east corner):
      - "밑으로 갔어." "파일됐어.";
      - the man who opens other people's records and never shows his own;
      - "에디터." "에디터라고 한 적 없어. ...이름도 안 했어."
      - The run records `ch13_malet_filed`, with the toast "말레트: 파일됨".
    - **The alley** (`ms_verdan_alley`):
      - Malet's ledger, the brown-coat man's notebook (he stays unnamed);
      - her hand stops before the book: "여기서 확인하고 싶지 않아.";
      - who would hear them asking for Malet.
  - The exit is **the old furrows** (`ms_verdan_furrow`), east of town:
    - no food bought; bread left;
    - one fewer person to hand the water to first;
    - the book on her knee after dark, never opened: "따뜻해졌어." "기울고 있어.";
    - "아니. 북서쪽." "이 책 같은 게 있어.";
    - "밝아지면 가 보자."
    - The last line: her palm slips off the cover; the book stays shut; Arrel sits facing north-west.
    - Ends with `ch13_complete`, `ch13_book_northwest` and `canon_ch14_resonance_ready`.
  - **The townsfolk:** six ambient figures in the market, shown from the arrival on (gate `ch13_road`).
  - After the chapter: a chest (a cart by the west road), a clue (the seal), encounters (the colorless waste pool).
- **Look:**
  - The dressing `VerdanAgain`: cobbles and warm, thinning light.
  - The set pieces (`BuildSetPieces`, `verdan_return`):
    - STALL tiles get a counter and two posts; about half are packed bare, with no awning;
    - the oil stall at (24,10) keeps its awning and a lamp;
    - DOOR tiles get a dark recess, red wax and a cord across;
    - the reservoir's stone basin with water in the square;
    - the outgoing cart with chair legs.
- **Art:**
  - The market splash plays under the town scenes. Gemini rated it SAFE; it shows a busier night than 21화's thinning day, but no named people.
  - The night in the furrows plays over **the empty dusk road** (`chapter_splash_belt_waystation.png`, the farm chapter's splash). Gemini rated it RISKY: no people, but a paved station rather than grass between ruts. It is the closest picture in the game.
  - The first render had the market splash behind the furrows too. A crowded market under "그들은 들어온 길로 나가지 않았다" was wrong, so that scene was switched.
  - No CG with Malet, the Verdan gate, Tobias, an open or glowing book, or a lantern is used.
- **HUD:**
  - Goals: "시장으로: 공동 저수지" → "셈터로: 광장 남쪽의 돌계단" → "동쪽 모퉁이의 기름 노점" → "모퉁이 안쪽 골목으로" → "동쪽 출구 → 묵은 밭".
  - Main quest: "다시 베르단: 셈터", "말레트의 행방", "묵은 고랑의 밤". The next step reads "북서쪽: 이 책 같은 것".
  - The chapter card: "13장 완료 · 묵은 고랑".
- **Earlier tests:** S374/S375/S377/S379 main-quest loops skip `ch13_`. S379 now expects the basin exit to lead to Verdan (Chapter 13).

## Tests
- `Memoria.S382.VerdanReturn` (unit):
  - map, level, Korean name, the Verdan market ground, spawn north, exit east;
  - every trigger on open ground; the Sump's trigger right below a DOOR tile;
  - seven manuscript scenes; the key lines;
  - only Arrel, Elia, Sable and the vendor speak;
  - Sable speaks only in the road and farewell scenes and wears `sable_calm`; the vendor has no face;
  - toasts and title in Korean; the five goals; the main quest before and after.
- `MemoriaVisual.S382VerdanReturn` (renderer):
  - plays the chapter from the slope to the furrows;
  - checks both toasts, the three closing flags, the last furrow line and the card;
  - 17 captures in `Saved/Validation/S382Verdan/`.
- Results: see the commit message.
