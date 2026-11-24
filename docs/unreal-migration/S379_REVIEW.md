# S379 — Chapter 12, the basin waystation, from the final manuscript (Opus, 2026-11-22)

Branch `claude/s366-upgrade`, on top of round 1's integration (`af939f8f`). Team task **T-6** on `TEAM-BOARD.md`.

## Source
- 최종본 19화 「미라」 and 20화 「회색 우리」 (final-1), read in full. Game Chapter 12 = these two.
  - 21화 「다시 베르단」 opens a new place and is left for the next chapter.
- 19화 is a cutaway: Mira at the eleventh desk of Lumea's second Registry. It plays as the chapter's first scene, before the road.
- 20화 has no fight. The adjuster's men do not appear, so **no wardens are placed**.
- Ground: the Godot `belt_waystation` yard, through `GROUND` (map id `basin_station`, level `L_BasinStation`, "분지의 역참").
  - The crossroads waystation layout (one walled house, ruins at the corners) matches "길 둘이 만나는 곳을 건물 넷이 둘러싸고".
  - The Belt's own props (rails, platform shelters, pylons) are drawn only for `belt_waystation`, so they do not appear here.
  - Its walled house stands as the low stone house with the ration window.

## What changed
- **Road:** the farmhouses' exit (Chapter 11) now travels on to the basin (Chapter 12).
- **Map:** in from the north, out by the east road.
  - Arrival:
    - **Mira** (19화):
      - the third error of the week;
      - nineteen years of aligning, names no longer read as sounds;
      - K-7's drift: 0.03 degrees, and this time 0.3, at the top only, back before the second paragraph;
      - her own notebook in the third drawer: "K-7. 수요일. 0.3도. 상단만.";
      - Belor: "오래 갖고 있었나." "칠 분." "뭔가 있었나." "없었습니다." "좋군.";
      - counting to seventy.
      - The run records `ch12_mira_kept`.
    - **The road:**
      - the dry trough, the fallen mile stone;
      - the name he does not say yet;
      - the fork and the post with no board: "남쪽으로 가자." "경계까지는 간다."
  - Story places:
    - **Below the fork:**
      - the signal tower on the east ridge, its flag at half-mast;
      - south-south-west, known without knowing who taught him; "not mourning";
      - the basin, four buildings round the crossroads, the window, about thirty-five people.
    - **The ration window** (south-west of the house, looking at the queue):
      - "배급 줄이 있어.";
      - the white pill swallowed dry;
      - "블랭커예요. …삼키고 가요.";
      - Verdan, his mother, the hand he wanted moved;
      - "너도 그 안으로 들어가겠다는 거니?"
      - The run records `ch12_blankers`, with the toast "배급 창구: 블랭커".
    - **Round the basin** (east):
      - sixty paces from the window;
      - Sable's dry palm on the back of his hand, not pressing, not pulling;
      - Elia's lips moving with no word;
      - "끈 좀 내려.";
      - they do not go into the waystation.
      - The run records `ch12_sable_hand`.
  - The exit is **the sheep pen over the dry creek**:
    - "먼저 볼게요." "그 뒤엔 내가 깨어 있으마.";
    - what breaking the pill would have done;
    - "내 손. …거기서 코트에 손 안 댔어." "책이 우리가 어디 있는지 알았을 테니까.";
    - the name still not said;
    - the morning: "일어나.";
    - "다음 창구 앞에서 무엇을 할지는 아직 고르지 않았다."
    - Ends with `ch12_complete` and `canon_ch13_verdan_ready`.
  - **The queue:** a clerk (`bureau_agent`) at the window and eleven townsfolk along the house's south wall, bending down the east side.
    - They are ambient figures, shown from the arrival on (gate `ch12_road`; an empty gate would keep them hidden).
  - After the chapter: a chest, a clue (the half-mast flag), encounters (the yard's own pool).
- **Scenes:** six runtime scenes in both languages.
  - Arrel, Elia and Sable speak; in the cutaway, Mira and Belor.
  - Mira and Belor have no portraits, and the stage dims for them (S374's fix).
  - Sable's lines avoid seeing verbs; "그를 돌아보지 않았다" is written as "그쪽으로 얼굴을 돌리지 않았다".
- **Goals and main quest:**
  - Goals: south road, the window from the west, round to the east, the east exit.
  - Main quest: two steps (the window, the sheep pen). The open step is now "벨트 도로 쪽으로: 다음 창구".
- **Look:**
  - grey daylight, the Belt yard's earth, low broken field walls round the rim, no lamps;
  - the signal tower beyond the east rim, its flag at half-mast;
  - the empty trough and the fallen mile stone by the north road;
  - the dark ration window with a stone sill in the house's south wall.
- **Art:**
  - Mira's registry: a dark archive desk with a candle, a brass compass and a closed book, nobody at it (`ui_pause_archive_backdrop`, newly imported; a placeholder until Lumea's registry has its own picture). (S383: corrected from "an open ledger" after Gemini's T-7 review.)
  - the map splash, the Belt road at dusk, for the pen.
  - The Lumea pictures show Arrel and Elia, who are not in Lumea in 19화, so they were not used. Pictures with Tobias were not used.

## Verification
Native UE 5.8.2, D3D 1280x720. Build slot taken by Opus on `TEAM-BOARD.md`.
- New `Memoria.S379.Basin`:
  - Chapter 12, its level and Korean name;
  - the Belt yard's tiles;
  - north entry, east exit, triggers on open ground;
  - the queue: a clerk and eleven, on open ground, shown from the arrival;
  - six scenes; the farmhouses' road leads here;
  - key lines; the cutaway is Mira's alone;
  - only the five speak; Sable's face is the blind one, and Mira and Belor have none;
  - Korean toast and title;
  - goals; the main quest at the window and then the next window.
- New `MemoriaVisual.S379Basin` on `L_BasinStation` (development run at Chapter 12):
  - plays from Mira's registry to the sheep pen;
  - the blankers toast;
  - Sable's hand;
  - the last line, the flags and the card.
- S374, S375 and S377 main-quest checks now ignore the later Chapter 12 steps. S377 also checks its road leads to the basin. The artwork count is 80.
- Captures inspected (`s379/focus1`, `focus4`). Two things fixed from them:
  - the queue never appeared (an empty ambient gate keeps figures hidden);
  - the window trigger stood too far west to see the queue; it is now south-west of the house.
- One focused run (`focus3`) quit before running any test, when the editor failed to load an engine plugin module (Interchange). The same run passed on retry (`focus4`), with no code change.
- Wide regression, every `MemoriaVisual.*` + `Memoria.S36*` + `Memoria.S37*` + `Memoria.Journal*` + `MemoriaSave.S362*` + input: `s379/wide2` **PASS 86/86**. The first wide run (`wide1`) was cut off when the session ended.

## Limits
- Chapter 13 (21화 「다시 베르단」) is not ported. Chapter 12 ends on "양 우리 · 13장으로 가는 길은 아직 준비 중".
- The sheep pen is a scene played at the east exit, not a walkable map.
- ~~The queue figures stroll a little around their places, and turn toward Arrel as he comes near (S349).~~
  - S383 (Grok T-8, `ec262ad6`): they now hold their places. The eleven look at the ration window and the clerk looks out at the line.
    Each glances at Arrel only inside 150 units, then turns back. In a fight they still watch the foe.
- The basin reuses the Belt yard's layout, so it can look like the Chapter 3 waystation without its props.
- English lines are this port's translation.

## S390: on black
- Gemini's T-14 backdrop hunt found no picture without people, a lantern or a glow.
- Chapter 12's splash is now empty, and the sheep pen night (`ms_basin_pen`) plays on black.
- Mira's registry desk is unchanged: it still shows under the Lumea cutaway.
