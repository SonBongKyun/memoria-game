# T-4 원고·캐논 검수 보고서 — 11장(17~18화) · 6~8장(6~12화) · S378 검증

- **작성:** Gemini (Antigravity)
- **기준 커밋:** `af939f8f` (`claude/s366-upgrade` 병합 후 `gemini/memoria-review` HEAD `433cc4c2`)
- **대조 원고:**
  - 11장: `MEMORIA_KR_Ch017_final.md` (17화. 읽는 자), `MEMORIA_KR_Ch018_final.md` (18화. 펠의 손)
  - 6장: `MEMORIA_KR_Ch006_final.md` (6화. 더 심), `MEMORIA_KR_Ch007_final.md` (7화. 세이블)
  - 7장: `MEMORIA_KR_Ch008_final.md` (8화. 듣는 숲), `MEMORIA_KR_Ch009_final.md` (9화. 빌려온 입), `MEMORIA_KR_Ch010_final.md` (10화. 대가)
  - 8장: `MEMORIA_KR_Ch011_final.md` (11화. 공포가 하던 일), `MEMORIA_KR_Ch012_final.md` (12화. 편집자)
- **검수 대상 파일:**
  - `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaManuscriptScenes.cpp` (`ms_farm_*` 6개, `ms_seam_*` 6개, `ms_forest_*` 7개, `ms_coast_*` 6개 — 총 25개 씬)
  - `Unreal/Tools/export_chapter_maps.py` (`roofless_farms`, `the_seam`, `forgotten_forest`, `crumbling_coast`)
  - `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaManuscript.cpp` (S371~S373, S377 메인 퀘스트)
  - `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaNavigationGoal.cpp` (6~8장, 11장 목표)
  - `Unreal/Memoria/Source/Memoria/Private/Chapter/MemoriaChapterMap.cpp` (S371~S373, S377 한국어 테이블)
  - `docs/unreal-migration/S377_REVIEW.md`, `S371_REVIEW.md`, `S372_REVIEW.md`, `S373_REVIEW.md`, `S375_REVIEW.md`
  - S378 커밋 `3b96dcbd` 전체 diff
  - CG 11종 직접 열람 + `data/chapter_expansion_gallery.json`, `scripts/ui/pause_menu.gd`, `data/vn_scenes/*.json` 메타데이터 교차 확인

---

## 요약

- **행 인덱스 안정성:** 아래 제안하는 모든 대사·서술 수정은 **기존 행을 제자리(in-place)에서 수정**하도록 설계했습니다(`field:<group>:<row>` 인덱스 변동 없음). 행 추가가 필요한 항목은 없습니다.
- **Must-fix (5건 — 모두 6~7장):**
  1. **6장 `ms_seam_arrival` CG 토비아스 등장 (`story_ch5_seam_first_light.png`):** 구버전 4인(아렐·엘리아·세이블 + 우측 **토비아스**) 도착 컷으로 23화 이전 토비아스 등장 위반 + 아직 만나지 않은 세이블 선행 등장.
  2. **7장 `ms_forest_ridge` CG 토비아스+4인 파티 (`story_ch7_crossing_the_ridgeline.png`):** S375에서 4인 파티 이유로 제외했던 동일 CG가 7장에 그대로 남아 있음(좌측 **토비아스** + 4번째 백발 여검사 포함).
  3. **7장 `ms_forest_valley:226-227` 화자 오류 + 영문 주객 전도 오역:** `"마지막을 조금 올리잖아. 대답 안 해도 한 번 기다리고."`는 원고(9화:357)에서 **아렐**의 대사인데 엘리아로 잘못 귀속되었고, 엘리아의 `"나한테서 배운 게 아니야."`(숲이 나한테서 배운 게 아니다)를 영문으로 `"I didn't learn it from you."`(내가 너한테서 배운 게 아니다)로 정반대 번역함.
  4. **`S371_REVIEW.md:43` 사실 오기:** `story_ch5_seam_first_light.png`가 토비아스 포함 구버전 CG임에도 캐논 CG로 기술됨.
  5. **`S372_REVIEW.md:46-47` 사실 오기:** `story_ch7_crossing_the_ridgeline.png`가 4인 파티(토비아스 포함) CG임에도 추가 동료 그림을 배제했다고 기술됨.
- **Should-fix (9건):**
  - 11장: `story_ch3_blank_book_warmth.png` (중갑옷 건틀릿을 낀 아렐의 손 + 서재 배경 — 17화 엘리아의 맨손/지붕 없는 농가와 불일치) 및 `S377_REVIEW.md:63` 묘사 정정.
  - 6장: `ch06_seam_living_streets_v1.png` (야간 + 등불 든 세이블이 만나기 전인 `ms_seam_name`, `ms_seam_feren` 및 아침 `ms_seam_morning`에 노출), `ch07_echo_shell_voices_v1.png` (7화:83 `"등불을 들고 있지 않았다"` 위반 + 나선파가 아니라 구버전 에코 셸 구체 CG), `ms_seam_night:89,92` 세이블 대사의 `보다` 관용구(`거기까지 본 거야`, `넷까지 볼 수 있겠지`), `ms_seam_night:101` `"그 사람들은"` 선행사 누락.
  - 7장: `ms_forest_valley:234` `"에코 셸."` → `"인내."` 급격한 압축 점프.
  - 8장: `ch05_city_in_bedrock_v1.png` 우측에 **부서지는 파도(바다)**가 남아 11화:57 `"붕괴의 해안에는 물이 보이지 않았다"` 및 `S373_REVIEW.md:48`과 충돌.
  - 6~8장 공통: `export_chapter_maps.py`의 6·7·8장 `elia_repeat` 3종이 `MemoriaChapterMap.cpp` 한국어 테이블에 전부 미등록(6장 `elia_repeat`는 구버전 잔존 문구).
- **Nit (11건):** 영문 미세 뉘앙스, 경미한 압축/순서 차이, `S375_REVIEW.md:67`의 S378 잔여 문구, 배경 분위기 불일치 CG 등.
- **S378(`3b96dcbd`) 검증:** T-1 Must-fix 3건·Should-fix 8건 **전부 반영 완료**, Nit 14건 중 10건 반영·4건 합리적 유지. 유일한 잔여 1건은 `S375_REVIEW.md:67`(`## Canon notes`)의 구 문구 미갱신(Nit).

---

## 씬 및 파일별 상태 요약

### Part 1 — 11장 (`roofless_farms`, 17~18화)

| 대상 | 씬 순서 | 화자·캐논 | 텍스트/번역 | CG/아트 | 판정 |
|---|---|---|---|---|---|
| `ms_farm_road` (17화) | No issues | No issues | **No issues** | — | **No issues** |
| `ms_farm_house` (17화) | No issues | No issues | **No issues** | Should-fix (`story_ch3_blank_book_warmth.png`) | 텍스트 No issues / CG Should-fix |
| `ms_farm_night` (17화) | No issues | No issues | Nit 1건 (661행 영문) | — | 한국어 No issues / En Nit |
| `ms_farm_fel` (18화) | No issues | No issues | Nit 1건 (680행 영문) | — | 한국어 No issues / En Nit |
| `ms_farm_glove` (18화) | No issues | No issues | **No issues** | — | **No issues** |
| `ms_farm_second` (18화) | No issues | No issues | **No issues** | — | **No issues** |
| `export_chapter_maps.py` (`roofless_farms`) | No issues | No issues | **No issues** | Nit 1건 (`chapter_splash_belt_waystation.png`) | 텍스트 No issues / 스플래시 Nit |
| `MemoriaManuscript.cpp` (S377) | — | No issues | **No issues** | — | **No issues** |
| `MemoriaNavigationGoal.cpp` (Ch11) | No issues | No issues | **No issues** | — | **No issues** |
| `MemoriaChapterMap.cpp` (S377) | — | No issues | **No issues** | — | **No issues** |
| `S377_REVIEW.md` | — | No issues | Should-fix 1건 (63행 CG 묘사) | — | Should-fix |

*(참고: `ms_farm_house:632` 세이블의 `"다른 데도 보이니?"`는 17화:35 원문 그대로이며 아렐의 시야를 묻는 표현이고 영문도 `"Anything else out there?"`로 시각 동사를 쓰지 않아 맹인 캐논 위반 아님.)*

### Part 2 — 6~8장 (`the_seam`, `forgotten_forest`, `crumbling_coast`, 6~12화)

| 대상 | 씬 순서 | 화자·캐논 | 텍스트/번역 | CG/아트 | 판정 |
|---|---|---|---|---|---|
| `ms_seam_arrival` (6장, 6화) | No issues | 텍스트 No issues / CG **Must-fix** | **No issues** | **Must-fix** (`story_ch5_seam_first_light.png` = 토비아스+세이블) | 텍스트 No issues / CG Must-fix |
| `ms_seam_name` (6장, 6화) | No issues | 텍스트 No issues / CG Should-fix | **No issues** | Should-fix (`ch06_seam_living_streets_v1.png`) | 텍스트 No issues / CG Should-fix |
| `ms_seam_feren` (6장, 6화) | No issues | 텍스트 No issues / CG Should-fix | **No issues** | Should-fix (`ch06_seam_living_streets_v1.png`) | 텍스트 No issues / CG Should-fix |
| `ms_seam_night` (6장, 6~7화) | No issues | Should-fix (`89,92`행 `보다` 관용구) | Should-fix 1건 (`101`행 선행사) | Should-fix (`ch07_echo_shell_voices_v1.png`) | Should-fix |
| `ms_seam_morning` (6장, 7화) | Nit 1건 (`118-120` 순서) | 텍스트 No issues / CG Should-fix | **No issues** | Should-fix (`ch06_seam_living_streets_v1.png`) | CG Should-fix / Nit |
| `ms_seam_departure` (6장, 7화) | No issues | No issues | **No issues** | Pass (`chapter_splash_the_seam.png`) | **No issues** |
| `ms_forest_ridge` (7장, 8화) | No issues | 텍스트 No issues / CG **Must-fix** | **No issues** | **Must-fix** (`story_ch7_crossing_the_ridgeline.png` = 토비아스+4인) | 텍스트 No issues / CG Must-fix |
| `ms_forest_enter` (7장, 8화) | No issues | No issues | **No issues** | Pass (`ch08_memory_forest_entry_v1.png`) | **No issues** |
| `ms_forest_echoes` (7장, 8화) | No issues | No issues | **No issues** | — | **No issues** |
| `ms_forest_mother` (7장, 8화) | No issues | No issues (8화 카이로스 실명 캐논 준수) | **No issues** | Pass + Nit (`story_ch8_ghost_mother.png`) | 텍스트 No issues / CG Nit |
| `ms_forest_lost_hour` (7장, 8~9화) | No issues | No issues | **No issues** | Pass (`chapter_splash_forgotten_forest.png`) | **No issues** |
| `ms_forest_escape` (7장, 9화) | No issues | No issues | **No issues** | Pass (`chapter_splash_forgotten_forest.png`) | **No issues** |
| `ms_forest_valley` (7장, 9~10화) | No issues | **Must-fix** (`226-227` 화자+오역) | **Must-fix** + Should-fix (`234`) | Nit (`chapter_splash_forgotten_forest.png`) | Must-fix + Should-fix |
| `ms_coast_morning` (8장, 11화) | No issues | No issues | **No issues** | Nit (`ch09_equalized_waste_v1.png`) | 텍스트 No issues / CG Nit |
| `ms_coast_shore` (8장, 11화) | No issues | No issues | Nit 1건 (`257`행 영문) | — | 한국어 No issues / En Nit |
| `ms_coast_hole` (8장, 11화) | No issues | 텍스트 No issues / CG Should-fix | **No issues** | Should-fix (`ch05_city_in_bedrock_v1.png` = 파도) | 텍스트 No issues / CG Should-fix |
| `ms_coast_city` (8장, 11화) | No issues | No issues | Nit 1건 (`292`행 한국어) | Pass + Nit (`sealed_city_ruins.png`, `ch09_equalized_waste_v1.png`) | Nit only |
| `ms_coast_tree` (8장, 11화) | No issues | No issues | **No issues** | — | **No issues** |
| `ms_coast_chair` (8장, 12화) | No issues | No issues (갈색 코트 사내 무명·무초상화 준수) | **No issues** | Nit (`ch09_equalized_waste_v1.png`) | 텍스트 No issues / CG Nit |
| `export_chapter_maps.py` (Ch6~8) & `MemoriaChapterMap.cpp` | No issues | No issues | Should-fix 1건 (`elia_repeat` 3건 미번역+Ch6 구버전 문구), Nit 1건 (Ch7 구버전 단서) | — | Should-fix |
| `MemoriaManuscript.cpp` (S371~373) | — | No issues | **No issues** | — | **No issues** |
| `MemoriaNavigationGoal.cpp` (Ch6~8) | No issues | No issues | **No issues** | — | **No issues** |
| `S371_REVIEW.md` / `S372_REVIEW.md` / `S373_REVIEW.md` | — | **Must-fix** (S371·S372 CG 오기), Should-fix (S373 파도) | — | — | Must-fix |

---

## 상세 지적 목록

형식: `file:line — game text — manuscript (화, quoted sentence) — proposed fix`

### Must-Fix

1. `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaManuscriptScenes.cpp:12,40` — `FirstLight = TEXT("res://assets/cg/generated/story_ch5_seam_first_light.png")` (`ms_seam_arrival:40`) — **6화 (3~105행)** `"아렐은 초록을 보기 전에 냄새를 맡았다... 엘리아는 여섯 걸음쯤 앞에서 기다리고 있었다."` 및 `TEAM.md` 캐논 (**토비아스는 23화부터만 등장**, 세이블은 7화 밤에 첫 대면) — **수정안:** 이미지를 직접 열람하고 `scripts/ui/pause_menu.gd:829-830`(`"The party meets Sable where impossible color survives inside The Seam"`)를 확인한 결과, 화면에 아렐·엘리아·세이블 외에 우측 전경에 **토비아스**가 함께 서 있습니다. 23화 이전 토비아스 등장 캐논 위반이자 아직 세이블을 만나기 전 시점이므로 `ms_seam_arrival:40`의 `FirstLight`를 제거(`nullptr` 또는 `Splash` 대체)하고 `MemoriaNarrativeArtwork.cpp:85` 등록 및 `.uasset`을 정리하십시오. *(행 인덱스 변동 없음)*

2. `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaManuscriptScenes.cpp:17,132` — `Ridgeline = TEXT("res://assets/cg/generated/story_ch7_crossing_the_ridgeline.png")` (`ms_forest_ridge:132`, 주석 16행: `Canon art only: no picture with Tobias or a fourth companion inside the forest.`) — **8화 (3~69행)** `"하렌은 마지막 집 앞에서 멈췄다... 세이블이 올라오고 있었다."` 및 `TEAM.md` 캐논 (**토비아스는 23화부터만 등장**, 추가 동료 그림 거부) — **수정안:** `story_ch7_crossing_the_ridgeline.png`는 좌측 안경·가방의 **토비아스**, 엘리아, 아렐, 그리고 선두의 **4번째 백발 여검사** 총 4명이 그려진 구버전 파티 CG(`scripts/ui/pause_menu.gd:1021-1022`)입니다. S375에서 이미 4인 이유로 제외했던 동일 자산이 7장 `ms_forest_ridge:132`에 남아 있으므로 즉시 제거(`nullptr` 또는 `ForestPath` 대체)하고 `MemoriaNarrativeArtwork.cpp:91` 등록 및 `.uasset`을 삭제하십시오. *(행 인덱스 변동 없음)*

3. `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaManuscriptScenes.cpp:226-227` — `Arrel (226): "숲에서 내 이름을 부른 거. 그냥 목소리만 같은 게 아니었어. 네가 나를 갑자기 돌아보게 하기 싫을 때. 그때처럼 불렀어."` / `Elia (227): "마지막을 조금 올리잖아. 대답 안 해도 한 번 기다리고. ...나한테서 배운 게 아니야. 내가 배운 방식이었으니까."` (En: `Elia (227): "You lift the end a little. And you wait once, even if no one answers. ...I didn't learn it from you. It's the way I learned."`) — **9화 (349~371행)**: 아렐 `“그냥 목소리만 같은 게 아니었어.”` / `“네가 나를 갑자기 돌아보게 하기 싫을 때. 그때처럼 불렀어.”` / (엘리아가 두 손을 뒤집음) / **아렐** `“마지막을 조금 올리잖아. 대답 안 해도 한 번 기다리고.”` / (그녀는 손을 그대로 둠) / **아렐** `“그것도 네 기억에서 가져간 거야?”` / **엘리아** `“나한테서 배운 게 아니야.”` / `“내가 배운 방식이었으니까.”` — **수정안:** 두 가지 중대 오류가 겹쳐 있습니다. (1) `"마지막을 조금 올리잖아. 대답 안 해도 한 번 기다리고."`는 아렐이 엘리아의 억양을 설명하는 **아렐의 대사**인데 엘리아에게 잘못 배정되었습니다. (2) `"나한테서 배운 게 아니야."`는 "숲(그것)이 내 기억에서 가져간 게 아니다"라는 뜻인데 영문이 `"I didn't learn it from you."`(내가 너한테서 배운 게 아니다)로 주객이 완전히 뒤집혔습니다. 행 인덱스를 유지한 채 아래와 같이 제자리 수정하십시오:
   - **226행 (Arrel):** Ko `"숲에서 내 이름을 부른 거. 그냥 목소리만 같은 게 아니었어. 네가 나를 갑자기 돌아보게 하기 싫을 때, 마지막을 조금 올리고 대답 안 해도 한 번 기다리는 것처럼 불렀어."` / En `"In the forest, when my name was called. It wasn't just your voice. It was the way you call me when you don't want me to turn too fast—lifting the end a little, waiting once even if no one answers."`
   - **227행 (Elia):** Ko `"나한테서 배운 게 아니야. 내가 배운 방식이었으니까."` / En `"It didn't learn it from me. It's the way I learned."`

4. `docs/unreal-migration/S371_REVIEW.md:43` — `The Seam splash, three canon CGs (first light, the living streets, the echo shell's voices) and Sable's blind portrait (sable_calm).` — `scripts/ui/pause_menu.gd:829-830` (`story_ch5_seam_first_light.png` 토비아스 포함 4인), `data/chapter_expansion_gallery.json:45` (`ch07_echo_shell_voices_v1.png` 에코 셸) — **수정안:** `story_ch5_seam_first_light.png`에 토비아스가 포함되어 있음을 정정하고, CG 교체/제거 내역을 리뷰 문서에 기록하십시오.

5. `docs/unreal-migration/S372_REVIEW.md:46-47` — `Art: the forest splash, the ridgeline, the forest entry and the mother in the forest. Set aside: the pictures that show a third companion with the two (Tobias's look)...` — `scripts/ui/pause_menu.gd:1021-1022`, `docs/unreal-migration/S375_REVIEW.md:64` (`story_ch7_crossing_the_ridgeline` shows four people) — **수정안:** `story_ch7_crossing_the_ridgeline.png` 제거와 함께 리뷰 문서에서 `the ridgeline`을 정정하십시오.

---

### Should-Fix

6. `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaManuscriptScenes.cpp:32,34,637` & `docs/unreal-migration/S377_REVIEW.md:63` — `BookWarmth = TEXT("res://assets/cg/generated/story_ch3_blank_book_warmth.png")` (`ms_farm_house:637`, 주석·리뷰: `the warm book in two hands, no faces`) — **17화 (63~97, 179행)**: `"가죽 모서리는 두 손 사이에 들어왔다... 왼손을 페이지 위에 폈다... 작은 표식이 있었고, 손등에는 오래된 굽은 흉터가 있었다."` (및 `scripts/ui/pause_menu.gd:1843-1844`: `"The Blank Book warms in Arrel's hands"`) — **수정안:** 실제 그림은 어두운 도서관 서가 앞에서 **강철 판금 건틀릿을 낀 아렐의 두 손**이 마법 빛줄기가 떠오르는 빈 책을 들고 있고, 양옆에 다른 두 인물의 뒷모습이 서 있는 구버전 3장 컷입니다. 지붕 없는 농가에서 맨손으로 백 노트를 여는 엘리아(17화)와 인물·소품·장소가 모두 어긋나므로 `ms_farm_house:637`에서 `BookWarmth`를 빼고(`nullptr`) `S377_REVIEW.md:63` 묘사를 정정하십시오. *(행 인덱스 변동 없음)*

7. `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaManuscriptScenes.cpp:13,54,70,103` — `Streets = TEXT("res://assets/cg/generated/chapter_expansion/ch06_seam_living_streets_v1.png")` (`ms_seam_name:54`, `ms_seam_feren:70`, `ms_seam_morning:103`) — **6화 (109~292행), 7화 (83, 169행)**: 낮의 첫 집과 개울 다리(`ms_seam_name`), 물 마시는 실내 긴 탁자(`ms_seam_feren`), 아침에 숟가락 깎는 세이블(`ms_seam_morning`); 세이블은 7화 밤에 처음 만나며 `"여자는 등불을 들고 있지 않았다"`(7화:83) — **수정안:** S378에서 10장 `ms_return_bridge`에서는 맹인 세이블이 등불을 들고 서 있다는 이유로 이 CG를 제거했으나, 6장 세 씬에는 그대로 남아 있습니다. 특히 `ms_seam_name`·`ms_seam_feren`은 아렐이 세이블을 **아직 만나기 전**이고, `ms_seam_morning`은 아침 탁자 씬이므로 6장 세 씬에서도 `Streets`를 제거(`nullptr` 또는 `Splash` 대체)하는 편이 일관됩니다. *(행 인덱스 변동 없음)*

8. `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaManuscriptScenes.cpp:14,96` — `ShellVoices = TEXT("res://assets/cg/generated/chapter_expansion/ch07_echo_shell_voices_v1.png")` (`ms_seam_night:96`) — **7화 (83, 99~107행)**: `"여자는 등불을 들고 있지 않았다."` / `"손바닥보다 작았고, 둥글게 감긴 표면에는 가는 줄이 나 있었다... 나선파라고 불러."` (및 `data/chapter_expansion_gallery.json:45`: `"Voices in the Echo Shell"`, `"Ordinary fragments of erased people circle an opened shell."`) — **수정안:** 세이블의 흰 맹인 눈은 캐논에 맞지만, (1) 세이블이 **등불을 들고 있어** 7화:83(`등불을 들고 있지 않았다`)과 직접 충돌하고, (2) 아렐이 탁자 위의 큰 기계식 구형 상자(구버전 **에코 셸**)를 열어 기억 파편이 뿜어져 나오는 장면이어서, S372에서 바로잡은 조개 캐논(세이블 = 작은 **나선파**, 엘리아 = 10화 **에코 셸**)과 충돌합니다. `ms_seam_night:96`에서 `ShellVoices`를 빼고(`nullptr`) 전용 작은 나선파 컷이 생길 때까지 실내 필드/스플래시로 유지하십시오. *(행 인덱스 변동 없음)*

9. `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaManuscriptScenes.cpp:89,92` — `Sable (89): "그럼 오늘은 거기까지 본 거야. ...네가 물으러 온 걸 물어."` / `Sable (92): "몇 달. 셋. 지금 정도로 버티면 넷까지 볼 수 있겠지. 더 크게 태우면 그보다 짧고."` — **6화 (439행)** `“그럼 오늘은 거기까지 본 거야.”`, **7화 (13행)** `“셋. 지금 정도로 버티면 넷까지 볼 수 있겠지.”` (대조 규칙: `TEAM.md:36` & `TEAM-BOARD.md:33` `Sable (blind: no seeing verbs, in Korean or English)`) — **수정안:** 원고의 관용구이고 영문은 이미 `"that's as far as today went"`, `"Four, if he holds as he is"`로 시각 동사를 피했습니다. S378에서 10장의 `"봤어"`를 `"겪었어"`로 정리한 기준과 통일한다면 한국어도 제자리에서 `89행: "그럼 오늘은 거기까지 간 거야. ...네가 물으러 온 걸 물어."`, `92행: "몇 달. 셋. 지금 정도로 버티면 넷까지 갈 수 있겠지. 더 크게 태우면 그보다 짧고."`로 수정하십시오(또는 관용구 예외로 유지할지 명시). *(행 인덱스 변동 없음)*

10. `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaManuscriptScenes.cpp:101` — `Sable: "그 사람들은 네가 지고 있는 걸 지고 있지 않았으니까. ...준 거야. 될 수 있으면 자."` (En: `"The others weren't carrying what you carry. ...It's given. Sleep, if you can."`) — **7화 (131~145행)**: 세이블 `“이걸 잠깐 쥐어 보겠다는 사람들은 있었어. 그러고는 한동안 말이 없어졌지.”` → 아렐 `“왜 나는 가져가도 돼.”` → 세이블 `“그 사람들은 네가 지고 있는 걸 지고 있지 않았으니까.”` — **수정안:** 7화:131이 생략되어 `"그 사람들은"`(`The others`)의 선행사가 사라졌습니다(열일곱 명 이야기도 아직 나오기 전). 행을 늘리지 않고 101행 제자리에서 Ko **`"전에 이걸 쥐어 보려던 사람들은 네가 지고 있는 걸 지고 있지 않았으니까. ...준 거야. 될 수 있으면 자."`**, En **`"The ones who tried holding it before weren't carrying what you carry. ...It's given. Sleep, if you can."`**으로 보완하십시오. *(행 인덱스 변동 없음)*

11. `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaManuscriptScenes.cpp:234` — `Elia (233): "에코 셸."` → `Elia (234): "인내. 네가 단어를 찾을 때 기다리는 거. 다음 말을 알 것 같아도 먼저 말하지 않는 거. ...그리고 고사리. 내가 만지고 있는데, 내 방식이 아니었어. 나는 그게 안 돼."` — **10화 (85~153행)**: 아렐 `“목소리 말고는. 다른 것도 빌려 온 거라면, 어떤 걸까.”` → 엘리아 `“인내. ...그리고 고사리.”` / `“네가 글씨를 따라 써 봐도 네가 쓴 건 알잖아. 나는 그게 안 돼.”` — **수정안:** 233행(`"에코 셸."`) 직후 아렐의 질문 없이 엘리아가 갑자기 `"인내."`를 말하고, 한국어 끝문장 `"나는 그게 안 돼."`의 선행사(`글씨를 따라 써 봐도 네가 쓴 건 알잖아`)도 빠졌습니다(영문은 `"I can't tell which is mine."`으로 풀어 씀). 행을 추가하지 않으려면 234행을 제자리에서 Ko **`"목소리 말고도 그래. 인내. 네가 단어를 찾을 때 기다리는 거. 다음 말을 알 것 같아도 먼저 말하지 않는 거. ...그리고 고사리. 내가 만지고 있는데, 내 방식이 아니었어. 어느 게 내 건지 모르겠어."`**, En **`"It's not just the voice. Patience. Waiting when you look for a word. Not saying it first even when I know it. ...And the ferns. I was touching them someone else's way. I can't tell which is mine."`**로 보강하십시오. *(행 인덱스 변동 없음)*

12. `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaManuscriptScenes.cpp:24,260` & `docs/unreal-migration/S373_REVIEW.md:46-48` — `CityInRock = TEXT("res://assets/cg/generated/chapter_expansion/ch05_city_in_bedrock_v1.png")` (`ms_coast_hole:260`) — **11화 (57, 83~97행)**: `"붕괴의 해안에는 물이 보이지 않았다... 반짝이는 수면은 나타나지 않았다."` / `"자갈밭 한가운데가 둥글게 끊겨 있었다. 폭은 다섯 걸음쯤 되어 보였다."` (및 `data/chapter_expansion_gallery.json:29`: 구버전 5장 해안 절벽 컷) — **수정안:** `S373_REVIEW.md:48`에서 파도 있는 해안 CG를 제외했다고 적었으나, `ch05_city_in_bedrock_v1.png`를 직접 열어 보면 **우측 하단 전체가 흰 파도가 부서지는 바다**이며 수평선에 빛나는 균열이 있고 아렐은 검을 뽑아 들고 있습니다(토비아스 없음은 통과). `"물이 보이지 않았다"` 캐논과 충돌하므로 `ms_coast_hole:260`에서 `CityInRock`을 제거(`nullptr` 또는 `GreyShore` 유지)하고 `S373_REVIEW.md:46-48`을 정정하십시오. *(행 인덱스 변동 없음)*

13. `Unreal/Tools/export_chapter_maps.py:42,78,117` & `Unreal/Memoria/Source/Memoria/Private/Chapter/MemoriaChapterMap.cpp:147-157` — `the_seam` `'elia_repeat': "I'm here. I'm not going anywhere."`, `forgotten_forest` `'elia_repeat': "Don't turn toward the sounds."`, `crumbling_coast` `'elia_repeat': 'Nine years, and still.'` — **6화, 8화 (39행), 11화 (49행)**: 6~8장의 `elia_repeat` 3종이 `MemoriaChapterMap.cpp` 한국어 테이블에 모두 빠져 있어 한국어 로케일에서 영문이 그대로 노출됩니다. 또한 6장 `"I'm here. I'm not going anywhere."`는 6~7화 원고에 없는 구버전 문구입니다. — **수정안:** 6장 `elia_repeat`를 6~7화 엘리아 대사(예: `"I wanted to stay a little longer."` / `"난 조금 더 있고 싶었어."` 또는 `"I'm fine. Give me a moment."`)로 바꾸고, 6·7·8장의 `elia_repeat` 3종을 모두 `MemoriaChapterMap.cpp` 한국어 테이블에 추가하십시오(`"Don't turn toward the sounds."` → `"소리 쪽으로 고개를 돌리지 마."`, `"Nine years, and still."` → `"아홉 해가 지났는데."`).

---

### Nit

14. `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaManuscriptScenes.cpp:661` — `Narration (En): "He sat along the same wall, a sleeper's length away."` (Ko: `"그는 같은 벽을 따라 두 사람이 누울 만한 거리를 두고 앉았다."`) — **17화 (257행)** `"두 사람이 누울 만한 거리를 두고 앉았다."` — **수정안:** `a sleeper's length away`는 1명 분량 길이이므로 `"with room enough for two to lie down between them"` 또는 `"two sleepers' width away"`가 원문에 더 가깝습니다. *(제자리 수정)*

15. `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaManuscriptScenes.cpp:680` — `Elia (En): "We'll see the road."` (Ko: `"길은 보일 거야."`) — **18화 (63~65행)** 세이블 `“움직일 만큼?”` / 엘리아 `“길은 보일 거야.”` — **수정안:** 영문 `"We'll see"`가 맹인 세이블까지 포함하는 어감이 살짝 있으므로 `"Enough to see the road."` 또는 `"The road will show."`가 더 자연스럽습니다. *(제자리 수정)*

16. `Unreal/Tools/export_chapter_maps.py:257` — `'splash': 'res://assets/cg/generated/chapter_splash_belt_waystation.png'` (`roofless_farms`) — **17~18화** 지붕 없는 농가·마른 밭 — **수정안:** 인물 없음(토비아스 배제)은 통과이나 3장 벨트 역참(시계탑 아치·푸른 등불·바닥 마법진) 스플래시 재사용이므로 향후 11장 전용 농가/밭 스플래시 제작 시 교체 권장.

17. `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaManuscriptScenes.cpp:118-120` — `ms_seam_morning` 끝에 엘리아의 `"난 조금 더 있고 싶었어." / "알아." / "알아 달라는 게 아니야."` 배치 — **7화 (367~371행)** — **수정안:** 원고에서는 하렌 등장(383행)·나무껍질 지도 제작(407행) 이전에 나오는 대화이나, 씬 마무리 정서상 자연스러우므로 현행 유지 가능(순서 차이만 기록).

18. `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaManuscriptScenes.cpp:168` — `GhostMother = TEXT("res://assets/cg/generated/story_ch8_ghost_mother.png")` (`ms_forest_mother:168`) — **8화 (211, 243행)** `"길가에 낮은 돌담이 보였다. 돌 사이에서 노란 꽃이 피어 있었고..."` / `"여자의 얼굴은 담 너머에 가려 있었다."` — **수정안:** 2인(아렐·엘리아) + 어머니 환영만 등장해 토비아스 캐논은 통과이나, 돌담·노란 꽃 대신 숲속 반투명 여성 얼굴과 구버전 갑옷 아렐·나무 가면이 그려져 있습니다. 전용 컷 제작 시 교체 권장.

19. `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaManuscriptScenes.cpp:220` — `ForestPath = TEXT("res://assets/cg/generated/chapter_splash_forgotten_forest.png")` (`ms_forest_valley:220`) — **9화 (243~249행)** 두 돌 능선 사이, 기울어진 돌판 아래 — **수정안:** 이미 `S372_REVIEW.md:75`에 한계로 기록됨(전용 돌판 골짜기 CG 제작 시 교체).

20. `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaManuscriptScenes.cpp:244,306` — `GreyShore = TEXT("res://assets/cg/generated/chapter_expansion/ch09_equalized_waste_v1.png")` (`ms_coast_morning:244`, `ms_coast_chair:306`) — **11화 (3~29행)** 돌판 아래 아침, **12화 (3~415행)** 나무 사이 갈림길과 빈 의자 — **수정안:** 2인(아렐·엘리아)만 등장하고 바다가 없어 해안(`crumbling_coast` 스플래시, `ms_coast_city:288`)에는 잘 맞으나, `ms_coast_morning`(돌 능선 사이)과 `ms_coast_chair`(숲 갈림길 의자)에서는 배경이 회색 폐허 평원이고 두 사람 사이에 구버전 9장 밧줄(`tether`)이 묶여 있습니다. 추후 전용 컷 제작 시 교체 권장.

21. `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaManuscriptScenes.cpp:257` — `Arrel (En): "Your eyes. ...It's going out of here too."` (Ko: `"눈. ...여기도 빠지고 있어."`) — **11화 (71~77행)** `"눈."` / `"여기도 빠지고 있어."` (자기 소매를 들며 색이 빠진다고 말함) — **수정안:** 영문 `"It's"`의 선행사(colour)가 명시되지 않아 `"Your eyes. ...The colour's draining here too."`가 더 명료합니다. *(제자리 수정)*

22. `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaManuscriptScenes.cpp:273` — `CityStill = TEXT("res://assets/cg/game_image/sealed_city_ruins.png")` (`ms_coast_city:273`) — **11화 (161~183행)** 우유 빛 돌집, 멈춰 선 따뜻한 사람들 — **수정안:** 인물 없음(토비아스 배제)은 통과이나 얼어붙은 빈 고딕 폐허·푸른 수정 그림이어서 따뜻한 멈춘 도시 분위기와 차이가 있습니다. 추후 전용 컷 제작 시 교체 권장.

23. `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaManuscriptScenes.cpp:292` — `Elia (Ko): "모르겠어. ...조용해. 너무."` (En: `"I don't know what letter it is. ...It's quiet. Too quiet."`) — **11화 (433~469행)** 아렐 `“같아.”` → 아렐 `“그게 무슨 글자야.”` → 엘리아 `“모르겠어.”` — **수정안:** 아렐의 질문이 생략되어 한국어가 `"같아."` 직후 `"모르겠어."`로 이어집니다. 영문처럼 제자리에서 Ko **`"무슨 글자인지는 모르겠어. ...조용해. 너무."`**로 다듬으면 자연스럽습니다. *(행 인덱스 변동 없음)*

24. `Unreal/Tools/export_chapter_maps.py:97-100` — `forgotten_forest` clues (`"The roots pulse faintly. Not with sap: with residue. These trees are digesting someone's childhood."` 등) — **8~10화** 듣는 숲 — **수정안:** 구버전 Godot `forgotten_forest.gd`의 단서 2개를 그대로 유지한 것(`S372_REVIEW.md:7` 명시)으로 한국어 번역도 완비되어 있으나, 최종본 8화의 흰 버섯 줄·움직이는 나무 톤과는 약간 결이 다릅니다(선택적 원고화).

---

## CG 시각 + 메타데이터 전수 검수표 (6~8장, 11장)

| 자산 경로 | 사용 씬 | 원본 용도 (`gallery` / `pause_menu` / `vn_scenes`) | 실제 이미지 육안 확인 | 판정 |
|---|---|---|---|---|
| `assets/cg/generated/story_ch5_seam_first_light.png` | `ms_seam_arrival:40` | `pause_menu.gd:829-830` `"The party meets Sable where impossible color survives inside The Seam"` | 아렐·엘리아·세이블 + 우측에 **토비아스** 포함 4인 구도 | **Must-Fix** (토비아스 23화 이전 + 세이블 선행 등장) |
| `assets/cg/generated/chapter_expansion/ch06_seam_living_streets_v1.png` | `ms_seam_name:54`, `ms_seam_feren:70`, `ms_seam_morning:103` | `chapter_expansion_gallery.json:35` `"Living Streets of the Seam"` | 야간 거리, 국 배식, 우측 문 앞에서 **세이블이 등불을 들고** 두 사람을 맞이함 | **Should-Fix** (만나기 전 세이블 선행 + 등불 위반 + 아침 씬 불일치) |
| `assets/cg/generated/chapter_expansion/ch07_echo_shell_voices_v1.png` | `ms_seam_night:96` | `chapter_expansion_gallery.json:45` `"Voices in the Echo Shell"` | 맹인 세이블이 **등불을 들고** 서 있고, 아렐이 구형 **에코 셸** 상자를 여는 장면 | **Should-Fix** (7화:83 `등불을 들고 있지 않았다` + 나선파≠에코 셸 충돌) |
| `assets/cg/generated/chapter_splash_the_seam.png` | `the_seam` 스플래시, `ms_seam_night:81`, `ms_seam_departure:122` | `pause_menu.gd:43` Ch6 스플래시 | 협곡 속 등불과 흰 꽃의 더 심 전경(인물 없음) | **Pass** |
| `assets/cg/generated/story_ch7_crossing_the_ridgeline.png` | `ms_forest_ridge:132` | `pause_menu.gd:1021-1022` `"The Seam's last colors fall behind as the party enters the dead country"` | 좌측 **토비아스**(안경·가방) + 엘리아 + 아렐 + **4번째 백발 여검사** 총 4인 | **Must-Fix** (토비아스 23화 이전 + 4인 파티; S375에서 이미 폐기한 동일 파일) |
| `assets/cg/generated/chapter_expansion/ch08_memory_forest_entry_v1.png` | `ms_forest_enter:145` | `chapter_expansion_gallery.json:47` `"Entering the Memory Forest"` | 아렐·엘리아 **2인만** 어두운 나무 터널 앞에 서 있음 | **Pass** |
| `assets/cg/generated/story_ch8_ghost_mother.png` | `ms_forest_mother:168` | `pause_menu.gd:1063-1064` `"A translucent mother cradles the shape of someone the forest consumed"` | 아렐·엘리아 **2인** + 반투명 어머니 환영(토비아스 없음; 단 돌담·노란 꽃 부재, 구버전 갑옷/가면 나무) | **Pass / Nit** |
| `assets/cg/generated/chapter_splash_forgotten_forest.png` | `forgotten_forest` 스플래시, `ms_forest_lost_hour`, `escape`, `valley` | `pause_menu.gd:45,343` Ch8 스플래시 | 어두운 숲길과 뿌리 주변 작은 흰 버섯(인물 없음; 나무 가면 존재) | **Pass / Nit** |
| `assets/cg/generated/chapter_expansion/ch09_equalized_waste_v1.png` | `crumbling_coast` 스플래시, `ms_coast_morning`, `city:288`, `chair:306` | `chapter_expansion_gallery.json:53` `"The Equalized Waste"` (`tether keeps two people together`) | 아렐·엘리아 **2인만** 물 없는 회색 폐허로 걸어감(손 사이 짧은 끈 존재) | **Pass / Nit** |
| `assets/cg/generated/chapter_expansion/ch05_city_in_bedrock_v1.png` | `ms_coast_hole:260` | `chapter_expansion_gallery.json:29` `"City in the Bedrock"` | 아렐(검 뽑음)·엘리아 2인 + 절벽 도시 + **우측 하단 부서지는 바다 파도** | **Should-Fix** (11화:57 `물이 보이지 않았다` 및 S373 파도 배제 원칙 위반) |
| `assets/cg/game_image/sealed_city_ruins.png` | `ms_coast_city:273` | `pause_menu.gd:397`, `data/vn_scenes/ch1_after_forest.json:72` | 인물 없는 차가운 청백색 고딕 폐허·푸른 수정 | **Pass / Nit** |
| `assets/cg/generated/story_ch3_blank_book_warmth.png` | `ms_farm_house:637` | `pause_menu.gd:1843-1844` `"The Blank Book warms in Arrel's hands"`, `chapter_expansion_gallery.json:19` | 어두운 서재에서 **강철 건틀릿을 낀 아렐의 손**이 빛나는 빈 책을 들고 있고 양옆에 2명 서 있음 | **Should-Fix** (17화 엘리아 맨손·지붕 없는 농가와 불일치) |
| `assets/cg/generated/chapter_splash_belt_waystation.png` | `roofless_farms` 스플래시 | `chapter_expansion_gallery.json:17`, `pause_menu.gd:40,409` Ch3 스플래시 | 황혼의 빈 석조 벨트 역참·시계탑 아치·푸른 등불(인물 없음) | **Pass / Nit** |

---

## Part 3 — S378 (`3b96dcbd`) T-1 수정 반영 검증 체크리스트

커밋 `3b96dcbd` (`fix(story): apply Gemini's T-1 canon review to Chapters 9-11 (S378)`)를 `docs/reviews/GEMINI_T1_CH9_10.md` 25건과 항목별로 대조했습니다.

| T-1 항목 | 등급 | S378 반영 여부 | 확인 내용 |
|---|---|---|---|
| #1 `ch08_ring_acceleration_v1` (토비아스) 제거 | Must-fix | **반영 완료** | `ms_clearing_haren`/`rings`에서 `nullptr` 처리, `MemoriaNarrativeArtwork.cpp` 제거, `T_ch08_ring_acceleration_v1.uasset` 삭제, 아트 수 81→79 갱신 |
| #2 `ms_return_road:483-485` `"다른 사람한테요"` + `"내 사촌"` + 영문 `he` 복원 | Must-fix | **반영 완료** | 제자리 수정으로 `"다른 사람한테요"`와 `"내 사촌"` 복원, 영문 맹인 세이블 `"see the tree"` 완전 해소 |
| #3 `S374_REVIEW.md:66,68` 토비아스 오기 정정 | Must-fix | **반영 완료** | 취소선 + S378 정정 문구 반영 |
| #4 `ms_return_square:579` 세이블 `"봤어"` → `"겪었어"` | Should-fix | **반영 완료 (문서 잔여 Nit 1건)** | 코드(`MemoriaManuscriptScenes.cpp`)와 `S375_REVIEW.md:63` 반영 완료. 단, **`docs/unreal-migration/S375_REVIEW.md:67`** (`## Canon notes`)에 `"전에 그런 사람들을 봤어 ... the newer manuscript's wording was kept"` 구 문구가 그대로 남아 63행과 모순됨 → 1줄 갱신 필요 (**Nit**). |
| #5 `ms_return_step:503` 영문 `lying there and wait to find out` | Should-fix | **반영 완료** | 제안 문구 그대로 반영 |
| #6 `ms_clearing_channel:447` `"세이블을 알기 전부터"` / `before I knew Sable` | Should-fix | **반영 완료** | 제자리 수정 반영 |
| #7 `ms_clearing_rings:403` `"말하기 전에는 몰랐어. 지금은 알아."` 보충 | Should-fix | **반영 완료** | 403행 제자리 통합으로 행 인덱스 유지하며 반영 |
| #8 `export_chapter_maps.py` Ch9 `elia_repeat` 화자 수정 + `MemoriaChapterMap.cpp` 등록 | Should-fix | **반영 완료** | `"I'm watching the same thing you are."` / `"네가 보는 걸 나도 보고 있어."` 등록 및 IR/INL 재생성 완료 |
| #9 `archive_ch1_camp_humming_v2` 제거 + `S374_REVIEW.md` 정정 | Should-fix | **반영 완료** | `ms_clearing_channel`에서 제거, 주석·리뷰 갱신 |
| #10 `ch06_seam_living_streets_v1` (`ms_return_bridge`) 제거 + `S375_REVIEW.md` 정정 | Should-fix | **반영 완료** | `ms_return_bridge`에서 제거, 주석·리뷰 갱신 |
| #11 `ch16_eastward_road` 제거 + `.uasset` 삭제 (11장까지 선제 교체) | Should-fix | **반영 완료** | 10장·11장 모두 제거, `T_ch16_eastward_road.uasset` 삭제, `S375`·`S377_REVIEW.md` 갱신 |
| #13~19, #21~23a/b, #24a (Nit 10건) | Nit | **반영 완료** | `In what way?`, `길이 바뀌었어?` 복원, `마셔.` 분할, `I only knew`, `협곡을 내려왔을 땐`, `those shells`, `펼친 노트의 페이지를 넘기지 않고`, 소년 등장+`연소자요?`, `Not the way in.`, `No footsteps of Haren's`, `the memories he had left`, `EPISODES` 매핑(전 맵 IR `source` 정상화) 전부 반영 |
| #12, #20, #23c, #24b/c, #25 (배경 Nit/선택 항목) | Nit | **미반영 (합리적 유지)** | 대체 전용 아트 부재 또는 기존 설계 유지 판단으로 보류된 경미한 Nit이며 문제없음 |

### S378 잔여 지적 (1건, Nit)

- `docs/unreal-migration/S375_REVIEW.md:67` — `One of them, "전에 그런 사람들을 봤어", uses 보다 in the past tense ("I've known people like that before"). The S148 rule bans seeing verbs for Sable; the newer manuscript's wording was kept.` — `S375_REVIEW.md:63` (`The S375 line "전에 그런 사람들을 봤어" now reads "겪었어"`) 및 `3b96dcbd` 실제 코드 — **수정안:** 67행을 `"Updated in S378 to '겪었어' per Gemini's T-1 recommendation, removing the seeing verb."`로 갱신하여 63행과 일치시키십시오.
