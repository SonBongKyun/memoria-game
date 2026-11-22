# S374 — Chapter 9, Haren's clearing, from the final manuscript (Claude, 2026-11-22)

Branch `claude/s366-upgrade`, on top of S373 (`648b907b`). The user asked to port Chapter 9.

## Source
- 최종본 13화 「산수」 and 14화 「자장가가 돌아오다」 (final-1), read in full. Game Chapter 9 = these two.
- Ground from the Godot `rim_forest.gd`: tiles, collision, painted canvas.
  - Its ring of path round a green hollow stands as Haren's clearing.
  - Its story (Chapter 1) is not used.
  - Its encounter pool is Chapter 1's. The listening forest's creatures stand in for it (`encounters_from`).
- The map has its own id, `haren_clearing` (level `L_HarenClearing`, "하렌의 공터"). `rim_forest` stays free for a later Chapter 1 port. `export_chapter_maps.py` gained `GROUND` (map id → Godot script) for this.
- The extractor now reads a tile detail given as a variable. `rim_forest`'s grass is `grass_detail := "grass_clean" if … else "grass"`. Before the fix, every tile definition after it was shifted by one. No other map's output changed.

## What changed
- **Road:** the coast's exit (the empty chair) now travels on to the clearing (Chapter 9).
- **Map:** in from the south (the right-hand road the man took), out to the north.
  - Arrival chain:
    - the hand: Elia's right hand keeps taking the blank book out ("또 해.", "다시 하면 말해." "말할게."), and the night by the low mound;
    - the trees: their spacing "is not real" ("저건 진짜가 아니야." "서 있는 방식이."), the fork with turned leaves, an axe.
  - Story places:
    - **Haren**, in the hollow:
      - "너희였군.", "나무 하러 나왔어.";
      - the water in the rock;
      - "밑에서 봐야 보이는 거야.";
      - "home": Arrel cannot name it.
    - **The rings**, at the felled tree's cut face:
      - writing inside the rings, cut an hour ago, in the brown coat's hand ("같은 글씨야.");
      - the blank book comes out again, with a new line, said aloud ("탑은 더 오래된 것의 복제품이야… 이건 기억이 아니야. 이건 산수야.");
      - she puts the book back herself;
      - "말해야 할지 생각 중이야.";
      - a new rough place on the hilt.
      - The run records `ch9_tower_copy`, with the toast "백 노트에 새 줄".
    - **The rock**, on the north path:
      - something white between two trunks;
      - clear water from the top, not the muddy pool;
      - the side with boot prints.
  - The exit is **the dry channel** (14화):
    - the broken bedrock, the limestone slab, a small fire;
    - "움직인 뒤에 알았잖아.";
    - four notches, the lowest clean ("내가 안 팠어.");
    - grip, beat, six notes;
    - Elia hums her seven: "다른데. …이어져.";
    - cradle and grip; "누군가 두 아이한테 나눠 준 거라면";
    - "네가 모르는 걸 전부 거기에 넣지는 마.";
    - his name gone, the six notes in its place;
    - wheels above;
    - "세이블은 조개를 왜 줬을까";
    - "적어도 그 사람 앞에서 물어야지.";
    - the brown-coat man, behind them, closes his notebook.
    - The run records `ch9_six_notes` and `ch9_lullaby_joined` ("돌아옴: 여섯 음"), then `ch9_complete` and `canon_ch10_seam_return_ready`.
  - After the chapter: a chest, a clue (the cut face), encounters. Saturation 0.72.
- **Scenes:** six runtime scenes, in both languages. Only Arrel, Elia and Haren speak. The brown-coat man stays unnamed and appears only in narration.
- **Goals and main quest:**
  - Goals: to the axe, then the cut face, then the rock's water north, then the north exit to the dry channel.
  - Main quest: three steps (the clearing, the rings, the dry channel). The open last step is now "아침에 더 심으로: 세이블에게, 그 앞에서 묻는다".
- **Look:**
  - Ground: `clearing_soil` and `clearing_paved`, cut from the rim forest canvas.
  - Dressing: daylight green, paler than the Seam.
  - Trees on the TREE tiles, with none on the south edge and lower ones near it, so the camera sees the path.
  - Shrubs on BUSH.
  - Haren's felled tree, its stump and pale cut face.
  - The low rock with water in its top.
  - Past the exit, the limestone slab and a small fire.
- **Art:**
  - The forest road toward far towers (`chapter_splash_rim_forest`), also the splash.
  - ~~The cut rings with Haren kneeling~~ **Corrected in S378:** the kneeling man in `ch08_ring_acceleration_v1` is **Tobias** (gallery entry `tobias_ring_theory`), who appears from 23화. It was removed and its import deleted. The clearing now plays over the map's splash.
  - ~~Elia humming beside Arrel's sword~~ **Corrected in S378:** `archive_ch1_camp_humming_v2` shows Arrel's blade drawn, where 14화 says it stays sheathed. It was removed from the scene.
  - (Found by Gemini's T-1 review, `docs/reviews/GEMINI_T1_CH9_10.md`.)
- **Fix (all chapters):** a speaker without a portrait (Haren, the brown-coat man) showed the last face on stage, here Arrel's. That side is now emptied and the stage dims. It also affected 6화's Haren and 12화's man in the brown coat.

## Verification
Native UE 5.8.2, D3D 1280x720.
- New `Memoria.S374.Clearing`:
  - Chapter 9, its level and Korean name;
  - the grass read as grass;
  - south entry, north exit, triggers on open ground;
  - six scenes; the coast's road leads here;
  - key lines;
  - only Arrel, Elia and Haren speak;
  - Korean toasts and title;
  - goals; the main quest at the clearing and then back to the Seam.
- New `MemoriaVisual.S374Clearing` on `L_HarenClearing` (development run at Chapter 9):
  - plays from the hand to the dry channel;
  - the new line, with its toast;
  - six notes joined, with their toast;
  - the last line (the brown coat), `ch9_complete` + `canon_ch10_seam_return_ready`, and the card.
- Captures inspected (`s374/focus1`, `focus2`). Three things fixed from them:
  - the borrowed portrait;
  - the south rim trees filling the opening frame;
  - the rings capture that never fired.
- The artwork count is 79.
- Wide regression, every `MemoriaVisual.*` + `Memoria.S36*` + `Memoria.S37*` + `Memoria.Journal*` + `MemoriaSave.S362*` + input: `s374/wide2` **PASS 78/78**, after the portrait fix. (`wide1` was cut off when the session ended and has no result.) The capture shows Haren's line with no borrowed face.

## Limits
- Chapter 10 (15화 onward: back to the Seam, asking Sable) is not ported. Chapter 9 ends on "마른 수로 · 10장으로 가는 길은 아직 준비 중".
- The dry channel is a scene played at the north exit, not a walkable map.
- The trees are the kit's bare tree model, tinted. The manuscript's trees are in leaf.
- English lines are this port's translation.
