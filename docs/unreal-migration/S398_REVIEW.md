# S398 — Chapter 18, the east road, from the final manuscript (Opus, 2026-11-28)

Branch `claude/s366-upgrade`, on top of `128dc475`. Team task **T-22** on `TEAM-BOARD.md`.

## Source
- 최종본 31화 「늙은 편집자」, 32화 「같은 다음 일」, 33화 「다시 하지 않기로 결정하다」, 34화 「마부가 돌아서다」 and 35화 「모르는 얼굴」, read in full.
- Read with Gemini's T-17 brief (30-32화) and T-20 brief (33-35화).
- Game Chapter 18 = 31-35화 on one road, as the T-20 brief's Option A recommends.
- No fight, so **no battles are placed**.
- **Ground:** a new hand-authored ground, `HANDMADE['east_road']` (map id `east_road`, level `L_EastRoad`, "동쪽 길").
  - It runs west to east:
    - out of Arcane's south gate past the **first mile stone**;
    - the **second mile**, where the cart waited;
    - **low field walls** to the third mile;
    - the **fourth-mile bend**, with a few trees in the pasture outside it;
    - the **seventh-mile post** on the road's right (south), set back a tile, its lamp hung unlit;
    - up into the **eighth-mile yard**: the doorless shed across the back, the low rail, the dry water trough, a bench and three other carts.
  - The pasture either side is open ground.
- **Cast:**
  - Arrel, Elia, Tobias (companion), Kairos, the messenger, and the driver (`Driver`, "마부"). The driver is nameless, has no portrait and only his six lines.
  - **Telos** (31화) never speaks.
  - **The man in the grey coat with the scar** (35화) has no name, no lines and no portrait. In the yard he stands by the goods cart near the shed's mouth, as an ambient figure (`traveler`).
  - Hanna stayed in Arcane: no Hanna here (`hanna_gone` is set in Chapter 17).

## What changed
- **Road:** Chapter 17's south gate (`arcane_leave`) now leads here.
- **Arrival** (sequence; two cutaways):
  - `ms_east_telos`: Telos's drawer in Lumea, shut twenty-five years. Seal, glass nib, the folded paper with green wax. The seal is not broken.
  - `ms_east_report`: Kairos's report as K-7.
    - "비무장 접촉." struck out; "검 사용 없음."
    - The head runs **0.3도** off true. "카이로스는 직각자를 집지 않았다."
    - The messenger: "함께 보낼 것은요?" "없네."
    - Records `ch18_kairos_tilted`.
- **Story places**, west to east:
  - **The first mile** (`ms_east_first_mile`): "돌 들어갔어?" "아니."
  - **The second mile** (`ms_east_cart`):
    - the hooded driver: "말 못 해.", "여덟째 마일 마당.", "두드려.";
    - "책은 북서쪽이야."; "마당까지.";
    - they board.
    - Records `ch18_cart_boarded`.
  - **The third mile** (`ms_east_third_mile`): nobody either way; "평범한지 어떻게 아는데."; the low wall ends.
  - **The fourth-mile bend** (`ms_east_rest`): under the tree; the bread cut at Arrel's finger; "먹을 때는 내가 볼게."
  - **Leaving the bend** (`ms_east_panel`, 33화), a cutaway:
    - Kairos at the obsidian panel;
    - the cart on the low band running east; "호흡 아래 침묵", five years ago in Lumea;
    - the panel's top edge **0.3도** off.
    - He moves no lever, rings no bell and picks up no chisel. "그는 손잡이를 잡지 않았다."
    - Then his room, and the Belt squad's redirect.
    - Records `ch18_panel_unmoved`.
  - **The seventh-mile post** (`ms_east_post`, 34화):
    - the lamp unlit; "아무도 나오지 않았다.";
    - the driver turns round: "둘째 마일에서 봤어. 아직 동쪽으로 가고 있어."; "마당까지."
    - Records `ch18_driver_turned`.
  - **The eighth-mile yard** (`ms_east_yard`, 35화):
    - "말랐어. 바닥까지.";
    - the scar, from the left eyebrow to the cheekbone;
    - Arrel's right hand goes to the hilt, but the sword is not drawn;
    - "기억에 없어." "모르겠어."; "당신 오른손을요."
    - Records `ch18_unknown_face`, with the toast "모르는 얼굴".
- **The exit** is east out of the yard (`ms_east_onward`):
  - "타."; "걸어 보려고 했어." "지금은 멀리 못 가."; "대신 멈추고 싶으면 말할 거야.";
  - the man stays by his cart; "수레는 동쪽 길로 올라섰다."
  - Records `ch18_complete` and `canon_ch19_east_onward_ready`.
  - The card reads "18장 완료 · 여덟째 마일 마당".
- After the chapter: a chest under the trees at the bend, and a clue at the dry trough.
- **Look:**
  - The dressing `East`: late afternoon going to evening, low warm light from the west, no lamps lit.
  - The set pieces (`BuildSetPieces`, `east_road`):
    - upright mile stones;
    - knee-high field walls;
    - trees with round crowns;
    - the post, low and set back so it does not hide the road from a camera looking north, its lamp a dark glass with no light;
    - the shed;
    - the rail;
    - the dry trough;
    - the bench;
    - three carts with disc wheels, one with its tarpaulin half drawn back.
  - Four figures stand in the yard from the post on: the one wiping a horse, the man on the bench, the woman with the bridle, and the man in the grey coat.
- **Art:**
  - **Every scene opens on black** (`Dark`); the map has no splash.
  - Kairos's own room in 31화, and his return to it at the end of 33화, play over `ch14_unsent_incident_report.png` (Gemini T-20: SAFE).
  - No new imports.
- **HUD:**
  - Goals: "동쪽으로: 첫째 마일 돌" … "마당 동쪽으로: 같은 수레".
  - Main quest:
    1. "동쪽으로 가는 마차까지 두 마일. 아직 정한 건 없다". This is Chapter 17's last line, now Chapter 18's first step.
    2. "후드 쓴 마부와 동쪽으로, 마당까지".
    3. "여덟째 마일 마당: 내릴지, 더 탈지".
    4. Then "같은 수레로 더 동쪽으로. 책은 여전히 북서쪽을 가리킨다".
- **Earlier tests:**
  - The S374-S396 main-quest loops skip `ch18_`.
  - S396 expects the south gate to lead here.
  - S371 allows the `ms_east_` dark rows.

## Riding (T-24)
- In 32-35화 the three ride the cart from the second mile on. Until Grok's T-24 cart lands, the field has no cart at the second mile, and the party walks the road between the scenes. The scenes themselves tell the ride.
- When T-24 lands:
  - the chapter will spawn the cart at the second mile;
  - `Board` at `ms_east_cart`'s end;
  - drive it through the third mile, the bend, the post and the yard as stops;
  - `GetDown` at the bend and at the yard;
  - turn the driver at the post (34화).

## Tests
- `Memoria.S398.EastRoad` (unit):
  - **Map:**
    - map, level, Korean name;
    - the eleven tile kinds;
    - four mile stones and three carts;
    - spawn west on the road; exit east of the yard;
    - every trigger on open ground, running west to east;
    - no fights, no lamps, no splash;
    - four yard figures shown from the post;
    - the two cutaways first;
    - 33화 right after the bend;
    - the roads in and out.
  - **Scenes:**
    - ten manuscript scenes, 255 rows, and their key lines;
    - only the three, Kairos, the messenger and the driver speak;
    - the driver has exactly six lines;
    - the faces: Tobias's and Kairos's; the driver and the messenger have none;
    - Kairos and the three never share a scene;
    - every scene opens with its picture set;
    - pictures are black, except Kairos's desk in his own scenes.
  - **Korean and HUD:** title, subtitle, toast, card place; the eight goals; the main quest through to the step after.
- `MemoriaVisual.S398EastRoad` (renderer):
  - plays the chapter from the gate road to the yard;
  - Tobias is in the party, and Hanna is not;
  - checks `ch18_kairos_tilted`, `ch18_cart_boarded`, `ch18_panel_unmoved`, `ch18_driver_turned`, `ch18_unknown_face` with its toast, `ch18_complete` and `canon_ch19_east_onward_ready`;
  - every one of the 255 rows is shown, then the card;
  - captures go to `Saved/Validation/S398East/`.
- Results: see the commit message.

## Limits
- No cart to ride until T-24 (see Riding).
- The man in the grey coat is a plain `traveler` figure; he has no field look of his own yet (no scar, no grey coat).
- English lines are this port's translation. Narration is trimmed more than in earlier chapters, since 31화 and 33화 are long single-viewpoint passages. Every spoken line is kept whole.
