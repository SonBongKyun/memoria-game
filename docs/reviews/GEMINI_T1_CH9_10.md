# T-1 원고·캐논 검수 보고서 — 9장·10장 (최종본 13~16화)

- **작성:** Gemini (Antigravity)
- **기준 커밋:** `4a8de43c` (`gemini/memoria-review`)
- **대조 원고:**
  - `C:/Users/jc/OneDrive/바탕 화면/메모리아/최종본/MEMORIA_KR_Ch013_final.md` (13화. 산수)
  - `C:/Users/jc/OneDrive/바탕 화면/메모리아/최종본/MEMORIA_KR_Ch014_final.md` (14화. 자장가가 돌아오다)
  - `C:/Users/jc/OneDrive/바탕 화면/메모리아/최종본/MEMORIA_KR_Ch015_final.md` (15화. 손이 기억하는 것)
  - `C:/Users/jc/OneDrive/바탕 화면/메모리아/최종본/MEMORIA_KR_Ch016_final.md` (16화. 출발)
  - (조개 캐논 교차 확인: `MEMORIA_KR_Ch010_final.md`)
- **검수 대상 파일:**
  - `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaManuscriptScenes.cpp` (`ms_clearing_*` 6개, `ms_return_*` 6개)
  - `Unreal/Tools/export_chapter_maps.py` (`haren_clearing`, `seam_return`, `manuscript_map`)
  - `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaManuscript.cpp` (S374/S375 메인 퀘스트)
  - `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaNavigationGoal.cpp` (S374/S375 목표)
  - `Unreal/Memoria/Source/Memoria/Private/Chapter/MemoriaChapterMap.cpp` (S374/S375 한국어 테이블)
  - `docs/unreal-migration/S374_REVIEW.md`, `docs/unreal-migration/S375_REVIEW.md`
  - 사용된 CG 7종 및 세이블 초상화 1종(`assets/cg/...`, `assets/portraits/sable_face_calm.png`) 직접 열람

---

## 요약

- **Must-fix (3건):**
  1. **토비아스 23화 이전 등장(캐논 위반):** `ms_clearing_haren` / `ms_clearing_rings`의 CG `ch08_ring_acceleration_v1.png`는 하렌이 아니라 구버전 8장의 **토비아스**(`data/chapter_expansion_gallery.json:49`, `tobias_ring_theory`)이며, `S374_REVIEW.md`에도 하렌으로 오기되어 있습니다.
  2. **화자 지시어 붕괴 + 세이블 맹인 캐논 위반:** `ms_return_road` 483~485행에서 원고의 `"다른 사람한테요."`와 `"내 사촌"`이 생략되어, 한국어에서는 하렌이 세이블을 `"그놈"`이라 부르는 문장이 되었고, 영어에서는 이를 `"She"`(세이블)로 번역해 맹인인 세이블이 `"she'd have to see the tree"`(나무를 봐야 한다)고 말하는 캐논 위반이 발생했습니다.
  3. **`S374_REVIEW.md`의 사실 오기:** `ch08_ring_acceleration_v1.png`가 하렌이며 토비아스가 없다고 기록된 부분을 CG 교체와 함께 정정해야 합니다.
- **Should-fix (8건):**
  - `ms_return_square` 579행 세이블 대사 `"전에 그런 사람들을 봤어."` 결정안 포함 (권고: **`"전에 그런 사람들을 겪었어."`**)
  - `ms_return_step` 503행 영문 오역(`leave that tree standing` — 이미 베어 쓰러진 나무, `wait to see` — 세이블 시각 동사)
  - `ms_clearing_channel` 447행 지시어 유실(`"그 사람"` 선행사 누락)
  - `ms_clearing_rings` 403~404행 핵심 비트 누락(`"말하기 전에는 몰랐어"` → `"알아"`)
  - `export_chapter_maps.py:149` + `MemoriaChapterMap.cpp` (`haren_clearing`의 `elia_repeat`가 아렐 대사이며 한국어 미등록)
  - CG 부적합 3건(`archive_ch1_camp_humming_v2.png`, `ch06_seam_living_streets_v1.png`, `ch16_eastward_road.png`)
- **Nit (14건):** 영문 뉘앙스, 경미한 압축 점프, `export_chapter_maps.py:246` 소스 주석 하드코딩(`6-7화`), 배경 분위기 불일치 CG 3건 등.
- **씬 순서 / 화자 캐논 전반:** 9장 6씬(13→14화)과 10장 6씬(15→16화)의 순서는 원고와 완전히 일치합니다. 갈색 코트의 사내는 무명·초상화 없음·서술 전용으로 캐논 준수. 조개(에코 셸/나선파) 언급은 14화 원고와 부합합니다. 세이블 초상화(`sable_face_calm.png`)와 `world_rewrite_sable_witness_v3.png`는 맹인 캐논을 충족합니다.

---

## 결정 사항: `ms_return_square` 579행 세이블의 `"전에 그런 사람들을 봤어."`

- **위치:** `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaManuscriptScenes.cpp:579`
  - 현재 한국어: `네가 그 말밖에 못 하는 동안에는. ...전에 그런 사람들을 봤어.`
  - 현재 영어: `Not while that's all you can say. ...I've known people like that before.`
  - 원고 (`15화:653, 657`): `“네가 그 말밖에 못 하는 동안에는.”` / `“전에 그런 사람들을 봤어.”`
- **충돌 지점:** `TEAM.md`는 `최종본 우선` 원칙과 `세이블 맹인 규칙(시각 동사 금지: hear, touch, know)`을 함께 두고 있습니다. 원고 15화:657의 `"봤어"`는 한국어 일상 관용 표현이지만, 게임 화면에서는 하얗게 흐린 눈의 세이블 초상화(`sable_calm`) 바로 옆에 `"사람들을 봤어"`가 출력되므로, 6화의 추상적 관용구(`"넷까지 볼 수 있겠지"`)보다 시각 동사 충돌이 훨씬 직접적으로 드러납니다. 실제로 영문판도 이를 피하기 위해 `"I've seen"` 대신 **`"I've known people like that before."`**로 이미 의역했습니다.
- **단일 권고안 (추천):**
  - **한국어:** `"네가 그 말밖에 못 하는 동안에는. ...전에 그런 사람들을 겪었어."`
  - **영어:** `"Not while that's all you can say. ...I've known people like that before."` (현행 유지)
  - **근거:** 원고의 문장 리듬(`전에 그런 사람들을 ___어.`)과 세이블 특유의 묵직한 어조를 유지하면서 시각 동사(`보다`)를 완전히 제거하고, 영문(`I've known...`)과도 의미·캐논이 일관되게 맞습니다. (참고: 작가가 원고 관용구 원문 보존을 최우선으로 결정할 경우에만 현행 `"봤어"` 유지.)

---

## 씬 및 파일별 상태 요약

| 대상 | 씬 순서 | 화자·캐논 | 텍스트/번역 | CG/아트 | 판정 |
|---|---|---|---|---|---|
| `ms_clearing_hand` (9장, 13화) | No issues | No issues | **No issues** | Nit 1건 (`chapter_splash_rim_forest.png`) | 텍스트 No issues / CG Nit |
| `ms_clearing_trees` (9장, 13화) | No issues | No issues | Nit 2건 | — | Nit only |
| `ms_clearing_haren` (9장, 13화) | No issues | 텍스트 No issues / CG **Must-fix** | **No issues** | **Must-fix** (`ch08_ring_acceleration_v1.png` = 토비아스) | 텍스트 No issues / CG Must-fix |
| `ms_clearing_rings` (9장, 13화) | No issues | 텍스트 No issues / CG **Must-fix** | Should-fix 1건 | **Must-fix** (`ch08_ring_acceleration_v1.png` = 토비아스) | Must-fix + Should-fix |
| `ms_clearing_rock` (9장, 13~14화) | No issues | No issues | Nit 1건 | — | Nit only |
| `ms_clearing_channel` (9장, 14화) | No issues | No issues | Should-fix 1건, Nit 4건 | Should-fix 1건 (`archive_ch1_camp_humming_v2.png`) | Should-fix |
| `ms_return_road` (10장, 15화) | No issues | **Must-fix** (세이블 `"그놈"` / `"see the tree"`) | **Must-fix** (483~485행) | — | Must-fix |
| `ms_return_bridge` (10장, 15화) | No issues | 텍스트 No issues / CG Should-fix | **No issues** | Should-fix 1건 (`ch06_seam_living_streets_v1.png`) | 텍스트 No issues / CG Should-fix |
| `ms_return_step` (10장, 15화) | No issues | 한국어 No issues / 영문 Should-fix (`wait to see`) | Should-fix 1건 (503행 영문) | Pass(맹인) + Nit 1건 (`world_rewrite_sable_witness_v3.png`) | 한국어 No issues / 영문 Should-fix |
| `ms_return_horn` (10장, 15화) | No issues | No issues | Nit 2건 | — | Nit only |
| `ms_return_square` (10장, 15화) | No issues | 결정 항목 (`봤어` vs `겪었어`) | Should-fix 1건, Nit 1건 | — | Should-fix (결정 권고) |
| `ms_return_crossroads` (10장, 16화) | No issues | 텍스트 No issues / CG Should-fix | Nit 3건 | Should-fix 1건 (`ch16_eastward_road.png`) | 한국어 No issues / CG Should-fix |
| `MemoriaManuscript.cpp` (S374/S375 메인 퀘스트) | — | No issues | **No issues** | — | **No issues** |
| `MemoriaNavigationGoal.cpp` (Ch9 목표) | No issues | No issues | **No issues** | — | **No issues** |
| `MemoriaNavigationGoal.cpp` (Ch10 목표) | No issues | No issues | Nit 1건 (56행) | — | Nit only |
| `export_chapter_maps.py` (`haren_clearing`, `seam_return`) | No issues | Should-fix (`elia_repeat` 화자 오류) | Should-fix 1건, Nit 2건 | Nit 2건 (스플래시) | Should-fix |
| `MemoriaChapterMap.cpp` (S374/S375 항목) | — | No issues | Should-fix 1건 (Ch9 `elia_repeat` 누락; 기존 항목 **No issues**) | — | Should-fix |
| `S374_REVIEW.md` / `S375_REVIEW.md` | — | **Must-fix** (S374 CG 오기) | Nit 2건 | — | Must-fix |

---

## 상세 지적 목록

형식: `file:line — game text — manuscript (화, quoted sentence) — proposed fix`

### Must-Fix

1. `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaManuscriptScenes.cpp:26,29,360,381` — `RingStump = TEXT("res://assets/cg/generated/chapter_expansion/ch08_ring_acceleration_v1.png")` (`ms_clearing_haren:360`, `ms_clearing_rings:381`, 주석: `the cut rings with Haren kneeling and the two watching`) — **13화 (133~143행, 209~215행, 267~271행)**: `"쓰러진 나무가 한쪽을 가로막고 있었고, 줄기 끝에서 사내가 도끼날에 붙은 부스러기를 털었다."` / `"더 심의 마지막 집 앞에서 길을 가리켜 주던 사람이었다. 도끼도 같았다."` / `"바깥쪽 껍질은 거뭇했지만 안쪽 목질은 희었다."` / `"이 나무는 내가 심었어. 너희가 걸음마하기도 전일 거야."` (및 `TEAM.md` 캐논: **토비아스는 23화부터만 등장**, 7화: 하렌은 긴 자루 도끼를 든 마흔쯤 된 나무꾼) — **수정안:** `ch08_ring_acceleration_v1.png`를 즉시 제거해야 합니다. 실제 이미지를 열람하고 `data/chapter_expansion_gallery.json:49`(`"desc": "Tobias measures how quickly the intervals are shrinking"`, `"dialogue_id": "tobias_ring_theory"`)를 확인한 결과, 그림 속 무릎 꿇은 인물은 하렌이 아니라 **토비아스**입니다(도끼 없음, 검은 장식 코트, 손등 문양, 황동 추·줄·펼친 연구서·빛나는 외곽 룬 문양, 거대한 밤중 그루터기). 대안으로 새 하렌/쓰러진 나무 단면 CG를 제작하기 전까지는 `ms_clearing_haren`과 `ms_clearing_rings`에서 `RingStump`를 빼고 `nullptr`(필드 배경 유지) 또는 숲 배경으로 대체하고, `MemoriaNarrativeArtwork.cpp:96-97` 주석도 정정하십시오.

2. `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaManuscriptScenes.cpp:483-485` — `Arrel: "세이블한테도 그렇게 말해 줄 수 있어요? 내가 없어도."` / `Haren: "말은 할 수 있어. 네가 본 것까지 들고 갈 수는 없어."` / `Haren: "그놈은 나를 알아. 내가 뭘 봐서 이런 소리를 하는지는 믿겠지. 나무를 봐야 알겠다는 말이 나오더라도."` (En: `Arrel: "Could you tell Sable that too? Even without me?"` / `Haren: "She knows me. She'll believe I saw something to talk like this. Even if what she says is she'd have to see the tree."`) — **15화 (113~147행)**: 아렐 `“세이블한테도 그렇게 말해 줄 수 있어요?”` / 하렌 `“네가 할 말을 대신?”` / 아렐 `“다른 사람한테요. 내가 없어도.”` … 하렌 `“내 사촌한테 나무 안에 글씨가 있었다고 하면, 베고 나서 누가 새겼겠지 할 거다.”` … `“그놈은 나를 알아. 내가 뭘 봐서 이런 소리를 하는지는 믿겠지. 나무를 봐야 알겠다는 말이 나오더라도.”` — **수정안:** `"다른 사람한테요."`와 `"내 사촌"`을 생략해 한국어에서는 하렌이 세이블을 `"그놈"`이라 지칭하게 되었고, 영어에서는 이를 `"She"`(세이블)로 번역해 **맹인인 세이블이 `"she'd have to see the tree"`라고 말하는 캐논 위반**까지 발생했습니다. 아래와 같이 복원하십시오:
   - 483행(아렐): Ko `세이블한테도, 다른 사람한테도 그렇게 말해 줄 수 있어요? 내가 없어도.` / En `Could you tell Sable—and other people too? Even without me?` (또는 원고대로 1줄 추가: 아렐 `"다른 사람한테요. 내가 없어도."`)
   - 485행(하렌): Ko `내 사촌한테 나무 안에 글씨가 있었다고 하면, 베고 나서 누가 새겼겠지 할 거다. 그래도 그놈은 나를 알아. 내가 뭘 봐서 이런 소리를 하는지는 믿겠지. 나무를 봐야 알겠다는 말이 나오더라도.` / En `If I tell my cousin there was writing inside a tree, he'll say someone carved it after I cut it. Still, he knows me. He'll believe I saw something to talk like this. Even if he says he'd have to see the tree.`

3. `docs/unreal-migration/S374_REVIEW.md:66,68` — `The cut rings with Haren kneeling and the two watching (ch08_ring_acceleration_v1, newly imported).` / `None shows Tobias or another companion.` — **13화 (135~143행)** 및 `TEAM.md` 캐논(`Tobias appears from 23화 only`), `data/chapter_expansion_gallery.json:49` (`tobias_ring_theory`) — **수정안:** `ch08_ring_acceleration_v1.png`가 토비아스 CG였음을 정정하고, CG 교체 후 리뷰 문서의 아트 항목을 실제 적용된 자산에 맞게 갱신하십시오.

---

### Should-Fix

4. `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaManuscriptScenes.cpp:579` — `Sable: "네가 그 말밖에 못 하는 동안에는. ...전에 그런 사람들을 봤어."` (En: `"Not while that's all you can say. ...I've known people like that before."`) — **15화 (653, 657행)**: `“네가 그 말밖에 못 하는 동안에는.”` / `“전에 그런 사람들을 봤어.”` (충돌 규칙: `TEAM.md:36` `Sable is a blind old woman. No seeing verbs for her (hear, touch, know).`) — **수정안:** 한국어를 **`"네가 그 말밖에 못 하는 동안에는. ...전에 그런 사람들을 겪었어."`**로 수정하십시오(영문 `"I've known people like that before."` 유지).

5. `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaManuscriptScenes.cpp:503` — `Sable (En): "Whoever wrote it will know where his record is. I'm not going to leave that tree standing and wait to see whether he comes this far for it."` (Ko: `"쓴 자는 자기 기록이 어디 있는지 알 거야. 그 나무를 그대로 두고 여기까지 찾아오는지 기다릴 생각은 없어."`) — **15화 (195, 213행)**: `“어제 벤 나무를 쪼개서 태워.”` / `“쓴 자는 자기 기록이 어디 있는지 알 거야. 그 나무를 그대로 두고 여기까지 찾아오는지 기다릴 생각은 없어.”` — **수정안:** 두 가지 영문 오류가 있습니다. (1) 나무는 어제 이미 베어져 쓰러진 상태이므로 `leave that tree standing`은 사실 오류입니다. (2) `wait to see`는 맹인 세이블에게 시각 동사 `see`를 사용했습니다. 영문을 **`"Whoever wrote it will know where his record is. I'm not going to leave that tree lying there and wait to find out whether he comes this far for it."`**로 수정하십시오.

6. `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaManuscriptScenes.cpp:447` — `Elia: "자기 전에 부르라고 했어. ...그 사람을 알기 전부터 불렀어."` (En: `"I was told to sing it before sleep. ...I sang it before I knew her."`) — **14화 (311~325행)**: 엘리아 `“자기 전에 부르라고 했어.”` / 아렐 `“누가.”` … 아렐 `“세이블한테 배웠어?”` / 엘리아 `“그 전이야.”` … `“그 사람을 알기 전부터 불렀어.”` — **수정안:** 아렐의 `"세이블한테 배웠어?"`가 생략되면서 `"그 사람"`(`her`)의 선행사가 사라져, 마치 "부르라고 시킨 사람을 알기 전부터 불렀다"는 모순된 문장처럼 읽힙니다. 447행 앞에 아렐 대사 `"세이블한테 배웠어?"` (`Did you learn it from Sable?`)를 복원하거나, 447행을 Ko **`"자기 전에 부르라고 했어. ...세이블을 알기 전부터 불렀어."`** / En **`"I was told to sing it before sleep. ...I sang it before I knew Sable."`**로 수정하십시오.

7. `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaManuscriptScenes.cpp:403-404` — `Elia: "내 거야. ...내 입에서 나왔어."` → `Arrel: "탑을 본 기억이 돌아온 거야?"` — **13화 (397~415행)**: 아렐 `“읽은 거야?”` / 엘리아 `“모르겠어.”` / 아렐 `“방금 말했잖아.”` / 엘리아 `“몰랐어. 말하기 전에는 몰랐어.”` / 아렐 `“그럼 지금은?”` / 엘리아 `“알아.”` / 아렐 `“탑을 본 기억이 돌아온 거야?”` — **수정안:** 엘리아가 **말하기 전까지는 자기도 몰랐다가 말한 뒤에 알게 되었다**는 13화 핵심 비트("기억이 아니라 산수"의 전제)가 생략되었습니다. 403~404행 사이에 한 쌍의 대사를 보충하십시오: 예) `Arrel: "방금 말했잖아."` (`You just said it.`) / `Elia: "말하기 전에는 몰랐어. 지금은 알아."` (`I didn't know before I said it. Now I do.`).

8. `Unreal/Tools/export_chapter_maps.py:149` & `Unreal/Memoria/Source/Memoria/Private/Chapter/MemoriaChapterMap.cpp:158-171` — `'elia_repeat': "Tell me if it happens again."` (한국어 테이블 미등록) — **13화 (59~61행)**: 아렐 `“다시 하면 말해.”` / 엘리아 `“말할게.”` (`MemoriaManuscriptScenes.cpp:342-343`에서도 화자는 `Arrel`) — **수정안:** `elia_repeat`에 **아렐의 대사**가 잘못 들어가 있으며, `seam_return`의 `"Still."`→`"지금도."`(`MemoriaChapterMap.cpp:167`)와 달리 한국어 번역도 누락되어 있습니다. `export_chapter_maps.py:149`를 13화의 엘리아 대사(예: `"I'm watching the same thing you are."` 또는 `"This time I put it in."` / `"I will."`)로 교체하고, 대응하는 한국어 항목을 `MemoriaChapterMap.cpp`에 추가하십시오.

9. `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaManuscriptScenes.cpp:27,30,443` — `Humming = TEXT("res://assets/cg/generated/archive_ch1_camp_humming_v2.png")` (`ms_clearing_channel:443`) — **14화 (71, 125~127, 263~277행)**: `"작은 불이 번진 뒤..."` / `"불빛이 닿는 곳에 검을 올려놓았다. 칼날은 꺼내지 않았다."` / `"그는 오른손으로 칼집을 붙들었다."` — **수정안:** `archive_ch1_camp_humming_v2.png`를 직접 확인한 결과, (1) 아렐이 **칼날을 뽑아 들고 있어** 14화:127(`칼날은 꺼내지 않았다`)과 정면 충돌하고, (2) 모닥불과 석회암 판이 전혀 없으며 폐허 숲에서 빛나는 금빛 실로 연결된 구버전 1장 아카이브 컷입니다(토비아스/추가 인물 없음은 통과). `MemoriaManuscriptScenes.cpp:27` 주석(`by the fire with Arrel's sword across his knees`)도 실제 그림과 다릅니다. 추후 마른 수로·작은 불·칼집에 든 검 전용 CG로 교체하거나, 당장 교체 자산이 없다면 최소한 주석 및 `S374_REVIEW.md:67`의 부정확한 묘사를 수정하십시오.

10. `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaManuscriptScenes.cpp:488` — `Streets = TEXT("res://assets/cg/generated/chapter_expansion/ch06_seam_living_streets_v1.png")` (`ms_return_bridge:488`) — **15화 (23, 157~179행)**: `"도끼날이 햇빛을 받았다."` / `"다리 너머에 이불들이 쌓여 있었다."` / `"담 위에서 말리던 천이 없어져 있었다."` (그리고 세이블은 다음 씬 `ms_return_step`에서야 집 앞 돌턱에 앉아 등장) — **수정안:** `ch06_seam_living_streets_v1.png`를 직접 확인한 결과, (1) 평온한 야간 마을에서 웃는 아이에게 국을 퍼 주고 담에 천이 그대로 걸려 있어 피난 준비·`"말리던 천이 없어져 있었다"`와 정반대이며, (2) 맹인인 세이블이 등불을 들고 문 앞에 서서 두 사람을 맞이하는 구도여서 `ms_return_bridge` 시점(하렌과 함께 다리를 건너는 아침, 아직 세이블에게 도착 전) 및 맹인 캐논과 어긋납니다. 10장 `ms_return_bridge`에서는 해당 CG를 빼고(`nullptr`) 낮 피난 분위기 필드 맵 자체를 보여 주거나 전용 CG로 교체하는 편이 낫습니다.

11. `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaManuscriptScenes.cpp:34,585` — `BeltRoad = TEXT("res://assets/cg/generated/ch16_eastward_road.png")` (`ms_return_crossroads:585`) — **16화 (3~87, 369행)**: `"낙엽 밟는 소리가 점점 커졌다."` / `"세이블의 손은 엘리아의 왼손목에 있었다."` / `"표지석은 길 왼쪽에 서 있었다. 허리 높이였다."` / `"다진 흙 위로 작은 자갈이 드러났다."` / `"비탈 아래에 교차로가 있었다."` (및 `data/vn_scenes/ch16_nera.json:7-26`, `scripts/ui/pause_menu.gd:1331-1334`: 구버전 16장 `"Three travelers cross drowned salt flats toward a horizon bruised violet"` — 아렐·엘리아·토비아스 3인) — **수정안:** 원경이라 4인 오류(`story_ch7_crossing_the_ridgeline`)는 피했으나, 실제 그림은 물에 잠긴 소금 평원 위 거대한 석조 둑길을 젊은 여행자 셋(구버전 아렐·엘리아·토비아스)이 보랏빛 폭풍을 향해 걷는 장면이어서 16화의 마른 자갈 능선길·손목을 잡고 걷는 맹인 노파 세이블과 지형·인물 모두 크게 어긋납니다. 전용 16화 능선/표지석 CG 제작 시 우선 교체 대상으로 지정하거나 당장 이질감이 크면 `nullptr` 처리하십시오.

---

### Nit

12. `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaManuscriptScenes.cpp:28,330` & `Unreal/Tools/export_chapter_maps.py:179` — `ClearingRoad = TEXT("res://assets/cg/generated/chapter_splash_rim_forest.png")` — **13화 (5, 73, 75행)**: `"나무 사이에 남은 햇빛이 길 한쪽만 비추고 있었다."` / `"햇빛이 숲 안쪽까지 들어왔다."` / `"나뭇잎 사이에 눌린 흙이 이어졌다."` — **수정안:** `chapter_splash_rim_forest.png`는 어두운 황혼/밤의 앙상한 나무, 길가의 빛나는 수정, 원경의 거대한 고딕 첨탑이 그려져 있어 13화의 햇빛 드는 잎사귀 숲길과 분위기가 다릅니다(인물 캐논 위반 없음). 전용 숲길/공터 스플래시가 생길 때 교체 권장.

13. `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaManuscriptScenes.cpp:352` — `Elia (En): "Where?"` (Ko: `"어디가."`) — **13화 (91~99행)**: 아렐 `“나무들.”` / 엘리아 `“어디가.”` / 아렐 `“서 있는 방식이.”` — **수정안:** 한국어 `"어디가."`는 위치가 아니라 "어느 부분이(어떻게) 진짜가 아니냐"는 뜻이므로 영문을 `"In what way?"` 또는 `"How?"`로 다듬으면 대답 `"The way they stand."`와 더 자연스럽게 호응합니다.

14. `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaManuscriptScenes.cpp:354-355` — `Elia: "나한테는 그냥 나무야."` → `Elia: "그럼 어디로 가려고."` — **13화 (103~113행)**: 엘리아 `“나한테는 그냥 나무야.”` / 엘리아 `“길이 바뀌었어?”` / 아렐 `“그건 모르겠어.”` / 엘리아 `“그럼 어디로 가려고.”` — **수정안:** 중간의 `"길이 바뀌었어?" / "그건 모르겠어."`가 빠지며 엘리아 대사가 연속되고 `"그럼"`(`Then`)의 연결이 약간 급합니다. 필요 시 두 줄을 복원하거나 355행의 `"그럼"`을 생략할 수 있습니다.

15. `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaManuscriptScenes.cpp:420` — `Elia: "마셔. ...그쪽 보면서 마시지 마."` (En: `"Drink. ...Not while you watch over there."`) — **14화 (11~17행)**: 엘리아 `“마셔.”` → (아렐이 물을 마시며 길을 보다 입가로 흘림) → 엘리아 `“그쪽 보면서 마시지 마.”` — **수정안:** 아렐이 마시는 행동 없이 한 줄로 합쳐져 타이밍이 약간 앞당겨졌습니다. 두 대사 사이에 짧은 서술(`아렐이 물을 마시다 다시 길을 보려는데 물이 입가로 흘렀다.`)을 넣거나 현행 압축 유지.

16. `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaManuscriptScenes.cpp:431` — `Elia (En): "We only knew after it moved. Back there too."` (Ko: `"움직인 뒤에 알았잖아. 아까도."`) — **14화 (99, 105행)**: `“움직인 뒤에 알았잖아. 아까도.”` / `“다른 걸 하고 있는데 내가 아직 모르는 거면.”` — **수정안:** 자기 손이 스스로 움직인 뒤에야 알았다는 엘리아의 불안이므로 `"I only knew after it moved. Back there too."`가 조금 더 가깝습니다.

17. `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaManuscriptScenes.cpp:440` — `Arrel: "여섯 음이 있어. ...그땐 셋이었어."` — **14화 (209~221행)**: 아렐 `“여섯 음이 있어.”` / `“앞부분은 알아. 전에 나한테 뭐냐고 물었잖아.”` / 엘리아 `“숲 내려왔을 때.”` / 아렐 `“그땐 셋이었어.”` — **수정안:** 중간 문답이 생략되어 `"그땐"`(`Back then`)의 지시 대상이 직전 서술(436행 `협곡을 내려온 뒤`)에만 기대고 있습니다. 이해는 가능하나 `"그땐"` 앞에 `"협곡을 내려왔을 땐"`을 넣거나 현행 유지.

18. `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaManuscriptScenes.cpp:458` — `Elia (En): "Why did Sable give us the shells? ...She never said we each had half."` (Ko: `"세이블은 조개를 왜 줬을까. ...우리한테 두 반쪽이 있다는 말은 안 했어."`) — **14화 (463~489, 503행)**: `“세이블은 조개를 왜 줬을까.”` … `“너한테도 줬잖아.”` … `“우리한테 두 반쪽이 있다는 말은 안 했어.”` (참고: 10화에서 아렐에게 에코 셸을 건넨 것은 엘리아이고, 세이블은 6화에서 아렐에게 작은 나선파를, 그 이전에 엘리아에게 에코 셸을 주었음) — **수정안:** 14화 원고 자체와 캐논 충돌은 없으나, 중간 대화가 생략된 상태에서 영문이 복수형 `give us the shells`로만 요약되면 10화에서 엘리아가 에코 셸을 넘긴 경위가 흐려집니다. 영문을 원고 463행에 맞춰 `"Why did Sable give the shell?"` / `"Why did Sable give us those shells?"` 정도로 조정하는 것을 고려할 수 있습니다.

19. `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaManuscriptScenes.cpp:465` — `Narration (Ko): "펼친 노트를 넘기지 않고 표지를 덮었다."` (En: `"Without turning the page, he closed his notebook."`) — **14화 (549행)**: `"펼친 노트가 무릎 위에 놓여 있었다. 그는 페이지를 넘기지 않고 표지를 덮었다."` — **수정안:** `"노트를 넘기지 않고"`는 한국어에서 다소 어색합니다. 원고에 맞춰 **`"펼친 노트의 페이지를 넘기지 않고 표지를 덮었다."`**로 다듬으십시오.

20. `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaManuscriptScenes.cpp:33,496` — `SableStep = TEXT("res://assets/cg/generated/illustration_expansion_v3/world_rewrite_sable_witness_v3.png")` — **15화 (181~183행)**: `"세이블은 그 집 앞 낮은 돌턱에 앉아 있었다. 무릎에는 숟가락도 나무 부스러기도 없었다... 두 손은 무릎을 짚고 있었다."` — **수정안:** 맹인 캐논(하얗게 흐린 반감은 눈)은 **완벽 통과**이며, 지팡이에 매달린 빛나는 줄 6가닥이 아렐의 여섯 음과 상징적으로 잘 어울립니다. 다만 배경이 밤중 불타는 폐허와 지팡이·종(chime) 소품이어서 낮 마을 돌턱과 차이는 있으므로, 향후 전용 낮 마을 돌턱 CG 제작 시 교체 고려 수준입니다.

21. `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaManuscriptScenes.cpp:549-551` & `565` — `ms_return_horn`의 소년 등장 서술 누락 및 `"연소자요?"` 생략 — **15화 (467, 571~573행)**: `"소년이 동쪽 길에서 뛰어 들어왔다."` / 세이블 `“너 같은 사람들에게 쓰려고 만든 거야.”` / 아렐 `“연소자요?”` — **수정안:** 550행(`"몇이냐."`)과 551행(소년 대사) 사이에 `"소년이 동쪽 길에서 뛰어 들어왔다."` 한 줄을 넣으면 등장 연결이 매끄럽고, 565행 앞에 `"연소자요?"`가 들어가면 몽둥이의 용도가 더욱 선명해집니다.

22. `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaManuscriptScenes.cpp:574` — `Sable (En): "We go in three. Three roads. Not the one we came in by."` (Ko: `"셋으로 나눠서 간다. 세 길로. 들어왔던 길은 쓰지 마."`) — **15화 (635행)**: `“셋으로 나눠서 간다. 세 길로. 들어왔던 길은 쓰지 마.”` — **수정안:** 마을 주민들에게 하는 말이므로 `"Not the one we came in by."`보다 `"Not the road in."` 또는 `"Not the way in."`이 상황에 더 자연스럽습니다.

23. `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaManuscriptScenes.cpp:583,596,611-612` — (a) 583행 En `"Haren did not return."` (Ko: `"하렌이 돌아오는 발소리는 없었다."`), (b) 596행 En `"Arrel began to look through what he had left."` (Ko: `"아렐은 남아 있는 기억들을 찾기 시작했다."`), (c) 611→612행 `"그럼 모르는 거다."` → `"아까는 다 보였어요."` — **16화 (11, 163~165, 325~345행)**: `"하렌이 돌아오는 발소리는 없었다."` / `"기억을 쓰면 달라질 수 있었다. 그는 남아 있는 것들을 찾기 시작했다."` / (밭과 돌무더기 너머 사각지대 대화 후 `"아까는 다 보였어요."`) — **수정안:** (a) 583행 영문을 `"No footsteps of Haren returning came behind them."`으로 완화(하렌의 생사·귀환 여부를 전지적으로 단정하지 않음), (b) 596행 영문을 한국어와 맞춰 `"the memories he had left"`로 명시, (c) 611~612행은 문맥상 이해 가능하므로 선택적 보강.

24. `Unreal/Tools/export_chapter_maps.py:187,215,246` — (a) 246행 `source='scenes/maps/%s.gd (ground) + 최종본 6-7화 (story)'`, (b) 187행 `'elia_repeat': "Still."`, (c) 215행 `chapter_splash_the_seam.png` — **15~16화** (`15화:419` 엘리아 `“지금도.”`는 `ms_return_horn` 시점 대사) — **수정안:** (a) 246행 하드코딩된 `최종본 6-7화`를 맵별 화수 매핑(예: `haren_clearing` → `13-14화`, `seam_return` → `15-16화`)으로 변경하여 `*.v1.json` 및 `MemoriaChapterMapSources.inl` 메타데이터를 바로잡으십시오. (b) `"Still."`은 엘리아 대사가 맞고 번역도 있으나 `ms_return_horn` 이전에도 노출될 수 있는 소소한 선행 참조입니다. (c) `chapter_splash_the_seam.png`는 6장 재사용 스플래시로 적합하나 아침 피난 분위기보다는 야간 온기가 강합니다.

25. `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaNavigationGoal.cpp:56` — `if (Run.GetFlag(TEXT("ch10_step"))) return Prefix + (bKo ? TEXT("동쪽 길 쪽으로") : TEXT("Toward the east road"));` — **15화 (397~457행)**: 구멍·공포 문답과 동쪽 뿔피리는 모두 세이블의 돌턱에서 이어져 발생 — **수정안:** 맵 탐험 리듬상 `ms_return_horn`을 동쪽 길 트리거로 분리한 설계 의도는 합리적이나, 씬 첫머리가 아직 돌턱 대화(`"구멍에는 들어갔니?"`)로 이어지고 555행에 `"아렐도 일어섰다"`가 남아 있습니다. 목표 텍스트를 `"세이블과 이야기를 이어 간다 (동쪽 길)"` 등으로 부드럽게 하거나 현행 유지.

---

## CG 및 초상화 시각 검수 결과 (직접 열람)

| 자산 경로 | 사용 위치 | 실제 이미지 내용 | 판정 |
|---|---|---|---|
| `assets/cg/generated/chapter_splash_rim_forest.png` | `haren_clearing` 스플래시, `ms_clearing_hand:330` | 어두운 황혼/밤, 앙상한 나무, 길가의 빛나는 수정, 원경의 고딕 첨탑. 인물 없음. | **Nit** (13화 햇빛 드는 잎사귀 숲길과 분위기 상이; 인물 캐논 위반 없음) |
| `assets/cg/generated/chapter_expansion/ch08_ring_acceleration_v1.png` | `ms_clearing_haren:360`, `ms_clearing_rings:381` | 밤중 숲의 거대한 그루터기 앞에 **토비아스**(갈색 머리, 검은 장식 코트, 손등 문양, 도끼 없음)가 무릎 꿇고 황동 추·실·펼친 책·빛나는 외곽 룬으로 나이테를 측정하며 아렐·엘리아가 지켜봄. (`data/chapter_expansion_gallery.json:49` 확인) | **Must-Fix** (**토비아스 23화 이전 등장 위반** + 하렌 인물/소품/주간 배경 전면 불일치) |
| `assets/cg/generated/archive_ch1_camp_humming_v2.png` | `ms_clearing_channel:443` | 밤중 폐허 숲에서 엘리아(작은 쪽지/부적 같은 종이를 쥠)와 아렐(**칼집에서 뽑은 맨 칼날**을 앞으로 듦)이 빛나는 금빛 실로 이어져 있음. 모닥불·석회암 판 없음. 2인만 등장. | **Should-Fix** (14화:127 `칼날은 꺼내지 않았다` 직접 충돌 + 모닥불/석회암 판 부재) |
| `assets/cg/generated/chapter_splash_the_seam.png` | `seam_return` 스플래시 | 협곡 속 따뜻한 등불과 흰 꽃이 핀 더 심 전경(6장 재사용). | **Pass / Nit** (마을 식별 스플래시로 적합, 아침 피난 톤과는 다소 차이) |
| `assets/cg/generated/chapter_expansion/ch06_seam_living_streets_v1.png` | `ms_return_bridge:488` | 야간 더 심 거리. 왼쪽에서 웃는 아이에게 국을 퍼 주고 담에 천이 걸려 있으며, 오른쪽 문 앞에서 세이블이 등불을 들고 아렐·엘리아를 맞이함. | **Should-Fix** (15화 아침 피난·`말리던 천이 없어져 있었다`와 반대 + `ms_return_step` 이전에 맹인 세이블이 등불을 들고 선행 등장) |
| `assets/cg/generated/illustration_expansion_v3/world_rewrite_sable_witness_v3.png` | `ms_return_step:496` | 하얗게 흐린 눈을 반쯤 감은 백발 노파 세이블이 앉아 지팡이에 매달린 빛나는 6가닥 줄에 손을 대고 듣는 모습. 배경은 밤중 불타는 폐허. | **Pass (맹인 캐논 충족) / Nit** (배경·소품은 낮 돌턱과 다르나 맹인 묘사 우수, 6가닥 줄이 6음과 상징 호응) |
| `assets/cg/generated/ch16_eastward_road.png` | `ms_return_crossroads:585` | 물에 잠긴 회색 소금 평원 위 넓은 석조 둑길을 3명의 작은 인물(구버전 아렐·엘리아·토비아스, `ch16_nera.json`)이 보랏빛 폭풍을 향해 걸어감. | **Should-Fix** (4인 오류는 피했으나 지형·방향·노파 세이블 동행 구도와 크게 불일치) |
| `assets/portraits/sable_face_calm.png` | `ms_return_step/horn/square/crossroads` 세이블 대사 | 하얗게 흐린 맹인 눈을 가진 백발 노파 초상화. | **Pass** (맹인 캐논 완전 충족) |
