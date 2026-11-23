# T-7 원고·캐논 검수 보고서 — 12장(19~20화) & S380 검증

- **작성:** Gemini (Antigravity)
- **기준 커밋:** `b8df8e81` (`claude/s366-upgrade` 병합 후 `gemini/memoria-review` HEAD `89b8cce3`)
- **대조 원고:**
  - `MEMORIA_KR_Ch019_final.md` (19화. 미라)
  - `MEMORIA_KR_Ch020_final.md` (20화. 회색 우리)
- **검수 대상 파일:**
  - `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaManuscriptScenes.cpp` (`ms_basin_mira`, `ms_basin_road`, `ms_basin_tower`, `ms_basin_queue`, `ms_basin_walk`, `ms_basin_pen` — 6개 씬)
  - `Unreal/Tools/export_chapter_maps.py` (`basin_station`, 대기열 ambient figures 포함)
  - `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaManuscript.cpp` (S379 메인 퀘스트)
  - `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaNavigationGoal.cpp` (12장 목표)
  - `Unreal/Memoria/Source/Memoria/Private/Chapter/MemoriaChapterMap.cpp` (S379 한국어 테이블)
  - `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaNarrativeSubsystem.cpp` (`Mira`/`Belor` 화자 매핑, 12장 플래그/토스트)
  - `docs/unreal-migration/S379_REVIEW.md`
  - CG 2종 직접 열람 + 메타데이터 교차 확인 (`assets/cg/generated/ui_pause_archive_backdrop.png`, `assets/cg/generated/chapter_splash_belt_waystation.png`, `scripts/ui/pause_menu.gd`)
  - S380 커밋 `72512319` 전체 diff (`docs/reviews/GEMINI_T4_CH6_8_11.md` 전 항목 대조)

---

## 요약

- **행 인덱스 안정성:** 아래 제안하는 모든 대사·서술 수정은 **기존 행을 제자리(in-place)에서 수정**하도록 설계했습니다(`field:<group>:<row>` 인덱스 변동 없음). 행 추가가 필요한 항목은 없습니다.
- **Must-fix:** **0건.** 12장은 19~20화 원고 충실도가 매우 높으며, 세이블 맹인 캐논·토비아스 미등장·한국어 테이블 누락 방지·컷어웨이 화자 처리 모두 정확합니다.
- **Should-fix (4건):**
  1. `MemoriaManuscriptScenes.cpp:725` (`ms_basin_mira`, En): 19화의 K-7 **`관리관`**을 `"warden"`으로 번역해, 15~16화의 곤봉 든 **`감시자`**(`EMemoriaFoeKind::Warden`)와 영문 용어가 충돌합니다(`official` / `administrator`로 교체).
  2. `MemoriaManuscriptScenes.cpp:766` (`ms_basin_queue`, Ko): 원고(20화:117)의 `"창구 아래의 평평한 돌"`이 빠져 `"저 위에"`의 선행사가 사라졌습니다(영문에는 `"that flat stone"` 유지).
  3. `MemoriaManuscriptScenes.cpp:785` (`ms_basin_pen`, En): 원고(20화:231) `"다른 것을 달라고 했을 수도 있었다"`(알약 **하나 더**)를 `"something else"`(다른 종류의 물건)로 오역했습니다(`another one` / `another pill`로 수정).
  4. `docs/unreal-migration/S379_REVIEW.md:74` (및 `MemoriaManuscriptScenes.cpp:34`, `MemoriaNarrativeArtwork.cpp:92`): `ui_pause_archive_backdrop.png`를 `"an open ledger"`로 적었으나 실제 이미지는 **닫힌 가죽 책(백 노트)** + 황동 기억 나침반(아스트롤라베) + 빛나는 결정 파편 + 거미줄 친 어두운 서고 책상입니다(`scripts/ui/pause_menu.gd:494` 교차 확인). 인물 없음은 통과하므로 임시 배경 유지 시 문서·주석의 사실 묘사만 정정하십시오.
- **Nit (3건):** `ms_basin_pen:804` 영문 `"too"` 어감, `chapter_splash_belt_waystation.png` 스플래시/양 우리 재사용, 대기열 NPC 수(원고 35명 → 맵 11명+서기 1명) 및 보행·회전(T-8에서 처리 중).
- **미라·벨로르 무초상화(`nullptr`) 판정:** **적절(Pass).** `assets/portraits/`에 미라·벨로르 전용 초상화가 없으며, 원고 19화에서도 루메아 컷어웨이의 비인격적 관료 공간(`"흰 도포가 열한 번째 책상으로 다가왔다"`)으로 그려집니다. 기존 관리국 초상화(`nera`, `seric`)를 억지로 재사용하지 않고 무초상화로 둔 판단이 캐논상 정확합니다.
- **S380 (`72512319`) T-4 반영 검증:** Must-fix **5/5 완료**, Should-fix **8/8 완료**(단 #11 `ms_forest_valley:234` 한국어 끝문장 `"나는 그게 안 돼."`는 원문 그대로 유지 — 경미한 Nit), 주요 텍스트·문서 Nit 및 S378 잔여(`S375_REVIEW.md:67`) **모두 반영 완료**.

---

## 12장 씬·파일별 상태 요약 (`basin_station`, 19~20화)

| 대상 | 씬 순서 | 화자·캐논 | 텍스트/번역 | CG/아트 | 판정 |
|---|---|---|---|---|---|
| `ms_basin_mira` (19화) | No issues | No issues (미라·벨로르 무초상화 Pass) | Should-fix 1건 (`725`행 En `warden`) | Should-fix (`S379_REVIEW:74` 묘사 오류; 인물 없음 Pass) | Should-fix |
| `ms_basin_road` (20화) | No issues | No issues (세이블 맹인 준수) | **No issues** | — | **No issues** |
| `ms_basin_tower` (20화) | No issues | No issues | **No issues** | — | **No issues** |
| `ms_basin_queue` (20화) | No issues | No issues (세이블 맹인 준수) | Should-fix 1건 (`766`행 Ko 선행사 누락) | — | Should-fix |
| `ms_basin_walk` (20화) | No issues | No issues (세이블 촉각·방향 준수) | **No issues** | — | **No issues** |
| `ms_basin_pen` (20화) | No issues | No issues (`804`행 `그만 보고`=아렐 불침번) | Should-fix 1건 (`785`행 En), Nit 1건 (`804`행 En) | Nit 1건 (`chapter_splash_belt_waystation.png`) | Should-fix |
| `export_chapter_maps.py` (`basin_station`) | No issues | No issues (대기열 1+11명, T-8 연동) | **No issues** (`elia_repeat`·단서·안내 완비) | Nit 1건 (`chapter_splash_belt_waystation.png`) | Nit only |
| `MemoriaManuscript.cpp` (S379) | — | No issues | **No issues** | — | **No issues** |
| `MemoriaNavigationGoal.cpp` (Ch12) | No issues | No issues | **No issues** | — | **No issues** |
| `MemoriaChapterMap.cpp` (S379) | — | No issues | **No issues** | — | **No issues** |
| `MemoriaNarrativeSubsystem.cpp` (S379) | — | No issues (`미라`·`벨로르` 매핑, 플래그·토스트) | **No issues** | — | **No issues** |
| `S379_REVIEW.md` | — | No issues | Should-fix 1건 (`74`행 `open ledger` 오기) | — | Should-fix |

*(참고 — 세이블 맹인 캐논 확인: `ms_basin_pen:804`의 `"너도 그만 보고 누워."`는 20화:321 원문 그대로이며, 아렐이 `"먼저 볼게요."`(`803`행 위 `783`행)라며 맡았던 **불침번**을 아렐더러 그만 서고 누우라는 말이므로 세이블의 시각 동사가 아닙니다. `ms_basin_walk:775` 역시 `"세이블은 그쪽으로 얼굴을 돌리지 않았다"`로 정확히 반영되었습니다.)*

---

## 상세 지적 목록 (12장)

형식: `file:line — game text — manuscript (화, quoted sentence) — proposed fix`

### Must-Fix

없음.

### Should-Fix

1. `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaManuscriptScenes.cpp:725` — `Narration (En): "K-7 at the top of the page. That warden's drift never went past 0.03 degrees. She laid the square against it and measured. 0.3 degrees."` (Ko: `"종이 위쪽에 K-7이라고 적혀 있었다. 그 관리관의 어긋남은 0.03도를 넘지 않았다."`) — **19화 (45~47행)** `"종이 위쪽에 K-7이라고 적혀 있었다. 그 관리관의 어긋남은 0.03도를 넘지 않았다."` — **수정안:** 이 프로젝트에서 `Warden`은 15~16화의 곤봉 든 **`감시자`**(`EMemoriaFoeKind::Warden`) 전용 영문 명칭입니다. 19화의 K-7은 관리국 소속 **`관리관`**(카이로스 측)이므로 `"warden"`을 쓰면 감시자와 용어가 충돌합니다. 행 인덱스 변동 없이 제자리에서 En **`"K-7 at the top of the page. That official's drift never went past 0.03 degrees. She laid the square against it and measured. 0.3 degrees."`**(또는 `"That administrator's drift"`)로 교체하십시오.

2. `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaManuscriptScenes.cpp:766` — `Narration (Ko): "지금은 창구에서 시선을 떼기가 어려웠다. 저 위에 알약을 놓고 누르면 부서질 것 같았다. 서기가 다음 것을 꺼내기 전에 손을 뻗으면 됐다."` (En: `"Now it was hard to look away from the window. Set a pill on that flat stone and press, and it would break. Reach before the clerk took out the next one."`) — **20화 (115~119행)** `"지금은 창구에서 시선을 떼기가 어려웠다."` / `"아렐은 창구 아래의 평평한 돌을 보고 있었다. 저 위에 알약을 놓고 누르면 부서질 것 같았다."` / `"서기가 다음 것을 꺼내기 전에 손을 뻗으면 됐다."` — **수정안:** 한국어에서 `"창구 아래의 평평한 돌"` 문장이 압축되며 빠져 `"저 위에"`가 무엇을 가리키는지 불명확해졌습니다(영문은 `"that flat stone"`을 보존). 행 추가 없이 766행 제자리에서 Ko **`"지금은 창구에서 시선을 떼기가 어려웠다. 창구 아래의 평평한 돌 위에 알약을 놓고 누르면 부서질 것 같았다. 서기가 다음 것을 꺼내기 전에 손을 뻗으면 됐다."`**로 보완하십시오.

3. `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaManuscriptScenes.cpp:785` — `Narration (En): "After dark the window still came back. If he had gone and broken the pill, what would that man have done? He might have asked for something else."` (Ko: `"어두워진 뒤에도 창구가 떠올랐다. 창구로 가서 알약을 부수었다면 그 남자가 어떻게 했을까. 다른 것을 달라고 했을 수도 있었다."`) — **20화 (229~231, 333행)** `"창구로 가서 알약을 부수었다면 그 남자가 어떻게 했을까. 다른 것을 달라고 했을 수도 있었다."` / `"알약을 빼앗긴 남자가 다시 손을 내미는 모습을 생각했다."` — **수정안:** `"다른 것"`은 다른 종류의 물건(`something else`)이 아니라 **다른 알약 한 알(하나 더)**을 뜻합니다. 785행 제자리에서 En **`"After dark the window still came back. If he had gone and broken the pill, what would that man have done? He might have asked for another one."`**(또는 `"another pill"`)로 수정하십시오.

4. `docs/unreal-migration/S379_REVIEW.md:74` (및 `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaManuscriptScenes.cpp:34`, `Unreal/Memoria/Source/Memoria/Private/Presentation/MemoriaNarrativeArtwork.cpp:92`) — `Mira's registry: a records desk with a candle and an open ledger, nobody at it (ui_pause_archive_backdrop, newly imported)` — **19화 (19~41행)** `"루메아 중앙 기록 건물 서관... 책상은 열두 개였다... 흑연 연필 세 자루, 뼈칼, 모서리를 맞추는 얇은 나무 틀"` 및 `scripts/ui/pause_menu.gd:494` (`"Archive desk, Memory Compass, and Blank Book backdrop for the pause-menu shell."`) — **수정안:** `ui_pause_archive_backdrop.png`를 직접 열어 확인한 결과, **열린 장부(open ledger)가 아니라 닫힌 두꺼운 가죽 책(백 노트)**이며, 그 옆에 황동 아스트롤라베(기억 나침반)·빛나는 황금빛 기억 결정 파편·찢어진 양피지·거미줄 친 어두운 서고가 그려져 있습니다. 인물이 전혀 없으므로 캐논 위반(토비아스/아렐·엘리아 혼입)은 없어 임시 배경으로 유지할 수 있으나, 리뷰 문서와 코드 주석의 `"an open ledger"` 오기는 `"a dark archive desk with a candle, brass compass and closed book (placeholder for Mira's registry)"` 등으로 사실에 맞게 정정하십시오. (향후 루메아 제2 레지스트리 전용 낮 사무실 컷 제작 시 교체 권장.)

### Nit

5. `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaManuscriptScenes.cpp:804` — `Sable (En): "You stop watching too, and lie down."` (Ko: `"너도 그만 보고 누워."`) — **20화 (321행)** `"너도 그만 보고 누워."` — **수정안:** 세이블이 불침번을 교대해 주는 상황이므로 영문 `"stop watching too"`는 세이블 자신도 감시를 멈춘다는 어감이 납니다. 원문의 `"너도"`(엘리아처럼 너도 누우라는 뜻)를 살려 제자리에서 En **`"Stop keeping watch and lie down too."`** 또는 **`"You lie down too; stop keeping watch."`**로 다듬으면 자연스럽습니다.

6. `Unreal/Tools/export_chapter_maps.py:274` & `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaManuscriptScenes.cpp:37,782` — `chapter_splash_belt_waystation.png` (`basin_station` 스플래시, `ms_basin_pen` CG) — **20화 (65~73, 199~209행)** 낮의 분지 역참(돌집 네 채·배급 창구) 및 마른 시내 위의 돌벽 양 우리 — **수정안:** 인물 없음(토비아스 배제)은 통과이나 3장 황혼 벨트 역참(시계탑 아치·푸른 등불·바닥 문양)의 재사용입니다. 전용 분지 역참/양 우리 아트 제작 시 교체 권장.

7. `Unreal/Tools/export_chapter_maps.py:297-310` — `ambient_npcs` (서기 `bureau_agent` 1명 + 주민 11명) — **20화 (73행)** `"그 앞에 사람들이 서 있었다. 서른다섯쯤 됐다."` — **수정안:** 맵 밀도상 35명을 11명(+서기 1명)으로 축약한 것은 합리적입니다. 대기열 주민들이 제자리에서 어슬렁거리며 아렐 쪽으로 고개를 돌리는 문제(`S379_REVIEW.md:105`)는 현재 T-8(`FMemoriaChapterNpc` still/face)에서 처리 중이므로 별도 조치 불필요합니다.

---

## 미라·벨로르 무초상화(`nullptr`) 적정성 검토

- **자산 현황:** `assets/portraits/` 및 `assets/portraits/character_shots/` 전체 확인 결과, 초상화가 존재하는 인물은 `arrel`, `elia`, `kairos`, `malet`, `nera`, `sable`, `seric`, `tobias`, `veil` 9명뿐이며 **미라(`Mira`)·벨로르(`Belor`) 초상화는 없습니다.**
- **화자 매핑:** `MemoriaNarrativeSubsystem.cpp:111-112`에 `Mira → 미라`, `Belor → 벨로르`가 등록되어 있고, `ms_basin_mira`에서 무대 디밍(S374 처리)과 함께 화자 이름만 표시됩니다.
- **캐논 적정성 판정 (`Pass`):**
  1. 19화는 주인공 일행과 완전히 떨어진 루메아 중앙 기록 건물의 컷어웨이이며, 미라는 시선을 종이·책상 아래 손가락에만 두고 벨로르 역시 `"흰 도포가 열한 번째 책상으로 다가왔다"`처럼 얼굴보다 제복·기척으로 묘사됩니다.
  2. 기존 관리국 캐릭터(`nera`, `seric`)의 초상화를 대용하면 인물 혼동·캐논 위반이 발생합니다.
  3. 따라서 전용 미라·벨로르 초상화가 제작되기 전까지 `Portrait = nullptr` 유지가 가장 정확하며, 미라가 이후 화에서 재등장해 비중이 커질 경우 그때 전용 초상화를 추가하는 방향을 권장합니다.

---

## 12장 CG 시각 + 메타데이터 검수표

| 자산 경로 | 사용 위치 | 원본 용도 (`pause_menu.gd`) | 실제 이미지 육안 확인 | 판정 |
|---|---|---|---|---|
| `assets/cg/generated/ui_pause_archive_backdrop.png` | `ms_basin_mira:723` (`RegistryDesk`) | `pause_menu.gd:494` `"Archive desk, Memory Compass, and Blank Book backdrop for the pause-menu shell."` | 어두운 거미줄 서고 책상 위 촛불, **닫힌 두꺼운 가죽 책(백 노트)**, 황동 기억 나침반(아스트롤라베), 빛나는 황금 결정 파편, 찢어진 양피지. **인물 없음.** | **Should-Fix (문서·주석 묘사 정정)** / 임시 배경 자체는 인물 없음으로 캐논 위반 없음 |
| `assets/cg/generated/chapter_splash_belt_waystation.png` | `basin_station` 스플래시, `ms_basin_pen:782` (`Pen`) | `pause_menu.gd:40,409` Ch3 벨트 역참 스플래시 | 황혼의 빈 석조 역참·시계탑 아치·푸른 등불·바닥 원형 문양. **인물 없음.** | **Pass / Nit** (3장 스플래시 재사용, 향후 전용 아트 교체 권장) |

---

## S380 (`72512319`) T-4 수정 반영 검증 체크리스트

커밋 `72512319` (`fix(story): apply Gemini's T-4 canon review to Chapters 6-8 and 11 (S380)`)의 전체 diff를 `docs/reviews/GEMINI_T4_CH6_8_11.md`의 24건 + S378 잔여 1건과 항목별로 대조했습니다.

| T-4 항목 | 등급 | S380 반영 여부 | 확인 내용 |
|---|---|---|---|
| #1 `story_ch5_seam_first_light.png` (토비아스+세이블) 제거 | Must-fix | **반영 완료** | `ms_seam_arrival:40` `nullptr`, `MemoriaNarrativeArtwork.cpp` 제거, `T_story_ch5_seam_first_light.uasset` 삭제 |
| #2 `story_ch7_crossing_the_ridgeline.png` (토비아스+4인) 제거 | Must-fix | **반영 완료** | `ms_forest_ridge:132` `ForestPath` 대체, `MemoriaNarrativeArtwork.cpp` 제거, `T_story_ch7_crossing_the_ridgeline.uasset` 삭제 |
| #3 `ms_forest_valley:226-227` 화자 오류 + 영문 주객 전도 수정 | Must-fix | **반영 완료** | 제자리 수정으로 `"마지막을 조금 올리잖아. 대답 안 해도 한 번 기다리고."`를 아렐(`226`행)로 이동, 엘리아(`227`행) 영문 `"It didn't learn that from me. ...It's the way I learned it."`로 정정 |
| #4 `S371_REVIEW.md:43` 토비아스 CG 사실 오기 정정 | Must-fix | **반영 완료** | 취소선 + S380 정정 내역(토비아스·등불·에코 셸 구체 제거 및 임포트 삭제) 반영 |
| #5 `S372_REVIEW.md:46-47` 4인 파티 CG 오기 정정 | Must-fix | **반영 완료** | `the ridgeline` 취소선 + 4인(토비아스 포함) 제거·스플래시 대체 내역 반영 |
| #6 `story_ch3_blank_book_warmth.png` (`ms_farm_house`) 제거 + `S377_REVIEW.md:63` 정정 | Should-fix | **반영 완료** | `ms_farm_house` `nullptr`, 주석 및 `S377_REVIEW.md:63` 정정(3장용 `.uasset`은 정상 유지) |
| #7 `ch06_seam_living_streets_v1.png` (6장 3개 씬) 제거 | Should-fix | **반영 완료** | `ms_seam_name`/`feren`/`morning` `nullptr`, `MemoriaNarrativeArtwork.cpp` 제거, `T_ch06_seam_living_streets_v1.uasset` 삭제 |
| #8 `ch07_echo_shell_voices_v1.png` (`ms_seam_night`) 제거 | Should-fix | **반영 완료** | `ms_seam_night` `nullptr`, `MemoriaNarrativeArtwork.cpp` 제거, `T_ch07_echo_shell_voices_v1.uasset` 삭제 |
| #9 `ms_seam_night:89,92` 세이블 `보다` 관용구 교체 | Should-fix | **반영 완료** | `"거기까지 간 거야"`, `"넷까지 갈 수 있겠지"` 제자리 수정 + `MemoriaSeamS371Tests.cpp` 기대값 동기화 |
| #10 `ms_seam_night:101` `"그 사람들은"` 선행사 보완 | Should-fix | **반영 완료** | Ko `"전에 이걸 쥐어 보던 사람들은"`, En `"The ones who held it before"` 제자리 반영 |
| #11 `ms_forest_valley:234` `"에코 셸."` → `"인내."` 압축 점프 보강 | Should-fix | **반영 완료 (경미한 잔여 Nit 1건)** | `"목소리 말고도 그래."` / `"Not only the voice."` 추가되어 급격한 점프는 해소됨. 단, 한국어 끝문장 `"나는 그게 안 돼."`(10화:153 원문)를 그대로 두어 영문(`"I can't tell which is mine."`) 대비 선행사가 약간 느슨함(미반영이 아닌 원문 유지 판단으로 분류, Nit). |
| #12 `ch05_city_in_bedrock_v1.png` (바다 파도) 제거 + `S373_REVIEW.md` 정정 | Should-fix | **반영 완료** | `ms_coast_hole` `nullptr`, `MemoriaNarrativeArtwork.cpp` 제거, `T_ch05_city_in_bedrock_v1.uasset` 삭제, `S373_REVIEW.md` 정정, 아트 수 80→75 갱신 |
| #13 `export_chapter_maps.py` Ch6~8 `elia_repeat` + `MemoriaChapterMap.cpp` 한국어 등록 | Should-fix | **반영 완료** | Ch6 `"I wanted to stay a little longer."` 교체, Ch6/7/8 3종 모두 `MemoriaChapterMap.cpp` 등록 및 `the_seam.v1.json`/`MemoriaChapterMapSources.inl` 재생성 |
| #14 `ms_farm_night:661` 영문 `two to lie down` | Nit | **반영 완료** | `"with room enough for two to lie down between them"` 반영 |
| #15 `ms_farm_fel:680` 영문 `The road will show.` | Nit | **반영 완료** | `"The road will show."` 반영 |
| #21 `ms_coast_shore:257` 영문 `The colour's draining` | Nit | **반영 완료** | `"The colour's draining here too."` 반영 |
| #23 `ms_coast_city:292` 한국어 `"무슨 글자인지는 모르겠어."` | Nit | **반영 완료** | 제자리 수정 반영 |
| S378 잔여 `S375_REVIEW.md:67` 구 문구 정리 | Nit | **반영 완료** | 취소선 + S378 `"겪었어"` 갱신 내역 반영 |
| #16~20, #22, #24 (전용 CG 추후 교체, 순서 참고, 구 단서) | Nit | **미반영 (합리적 유지)** | 신규 전용 아트 필요 항목 또는 선택적 참고 항목으로 미변경이 적절함 |

### S380 검증 결론
- T-4의 **Must-fix 5건과 Should-fix 8건이 모두 코드·에셋·테스트·리뷰 문서까지 일관되게 반영**되었습니다(총 아트 수 80 → 75, 문제 `.uasset` 5개 삭제 포함).
- 미반영 누락으로 다시 올릴 Must-fix / Should-fix 항목은 **0건**입니다(`#11` 한국어 끝문장 `"나는 그게 안 돼."` 원문 유지 1건만 경미한 참고 Nit로 기록).
