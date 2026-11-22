# S373 — Chapter 8, the Crumbling Coast and Blank 7, from the final manuscript (Claude, 2026-11-22)

Branch `claude/s366-upgrade`, on top of S372 (`4f259bc3`). The user asked to port Chapter 8.

## Source
- 최종본 11화 「공포가 하던 일」 and 12화 「편집자」 (final-1), read in full. Game Chapter 8 = these two (the same day).
- Ground from the Godot `crumbling_coast.gd`: tiles, collision, encounter pool, painted canvas. Its story (the Classifier, the road to the Seam) is not used. Its pool of WATER tiles, walled in by cliffs, stands as **the hole**. The manuscript's coast has "no water to be seen", and blue-white light comes up from the hole.

## What changed
- **Road:** the forest's exit now travels to the coast (next map `crumbling_coast`, Chapter 8).
- **Map** (`export_chapter_maps.py` MANUSCRIPT `crumbling_coast`): in from the west (11화: "동쪽"), out to the north.
  - Arrival chain:
    - the morning under the slab: the shell silent, the cold left in his hand, the name heard but not sayable;
    - the shore: the grass cut along one line ("아홉 해가 지났는데"), no water, no wind, colours draining.
  - Story places:
    - the hole: "공백 7호?", "아홉 해 전에 네가 있던 곳?", "같이.";
    - **Blank 7**, at the hole's edge:
      - the still city, warm people, everything seen at once ("외로움이 없었어");
      - the fear that does not come;
      - one line of his mother's lullaby ("강물은 흐르지 않으리"), the broken window's star;
      - "네가 안 느껴져";
      - out again: "오 년 동안… 지금은 내 거야";
      - the blank book's first page bears the mark on her palm;
    - the young tree: "아직 너를 막지 않겠다."
  - The exit is **the empty chair** (12화):
    - the man in the brown coat, unnamed, his name answered with "앉아";
    - his list of Arrel's four losses;
    - the word that moves Elia's hands to the blank book;
    - "강 전까지 길은 두 번 더 갈라진다. 북쪽에서는 비가 그쳤다.";
    - the word left in the air over the seat;
    - the rough spot on the hilt.
  - Inside Blank 7 the run records `ch8_fear_gone` and `ch8_lullaby_line`, with the toasts "잃음: 공포" and "돌아옴: 자장가 한 줄".
  - Cliffs stand as broken ruin stone. A chest and a canon clue (the cut grass line) after the chapter. Encounters after the chapter. Saturation lowered. Level `L_CrumblingCoast`.
- **Scenes:** six runtime scenes in both languages (`MemoriaManuscriptScenes.cpp`). The speaker "Brown Coat" is shown as "갈색 코트의 사내", with no portrait: the manuscript does not confirm who he is.
- **Goals and main quest:**
  - Goals: to the hole, then into it together, then the north path, then the crossroads.
  - Main quest: three steps (the hole, Blank 7, the empty chair) before the open "hole in the bark".
- **Look:**
  - ground cut from the coast canvas (`coast_soil`, `coast_paved`);
  - a grey windless dressing;
  - the hole as a pale blue-white floor with low blue lights;
  - the young tree on the north path;
  - the chair and the flat stone past the exit.
- **Art:**
  - "No water could be seen": the two walking into grey flattened land (`ch09_equalized_waste_v1`) for the shore and the chair, also as the map's splash;
  - ~~the city in the rock for the hole~~ **removed in S380** (Gemini T-4): `ch05_city_in_bedrock_v1` has breaking sea at its right edge. The hole now plays over the grey splash;
  - the existing `sealed_city_ruins` for inside.
  - The Godot coast splash and the cliff-parting picture show breaking surf. Both were imported, then removed in this session, and the asset-registry cache was moved aside again.

## Verification
Native UE 5.8.2, D3D 1280x720.
- New `Memoria.S373.Coast`:
  - west entry, north exit;
  - the city is entered at the hole's edge;
  - six manuscript scenes;
  - the forest's road leads here;
  - key lines ("아홉 해가 지났는데", "공백 7호?", "외로움이 없었어", "네가 안 느껴져", "아직 너를 막지 않겠다", the man's road line);
  - no speaker named Kairos;
  - goals.
- New `MemoriaVisual.S373Coast` on L_CrumblingCoast (development run at Chapter 8):
  - plays from the morning to the empty chair;
  - fear lost and the lullaby line back, with their toasts;
  - closes with `ch8_complete` and `canon_ch9_clearing_ready`, and the card shows.
- Captures inspected (`s373/focus1`, `wide1`, `focus2`), and three things fixed from them:
  - pictures with sea replaced by the grey plain;
  - the hole, blown out to white by thirteen bright lights, now a pale blue floor (lights at 0.16).
- S372's main-quest expectation follows (after the forest comes the coast). The artwork count is 78.
- Wide regression, every `MemoriaVisual.*` + `Memoria.S36*` + `Memoria.S37*` + `Memoria.Journal*` + `MemoriaSave.S362*` + input: `s373/wide1` **PASS 76/76**.
  - That run came before the last hole-light change, which only changes the coast map's light data. After it, `Memoria.S373.Coast` + `MemoriaVisual.S373Coast` passed 2/2 (`s373/focus2`).

## Limits
- Chapter 9 (13화: Haren's clearing, the writing inside the tree rings) is not ported. Chapter 8 ends on "갈림길 · 9장으로 가는 길은 아직 준비 중".
- Blank 7 is a scene played at the hole's edge, not a walkable map of the still city.
- The hole's floor shows the tile seams of the pool.
- English lines are this port's translation.
