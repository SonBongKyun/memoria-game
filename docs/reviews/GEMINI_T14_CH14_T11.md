# T-14 원고·캐논 검수 보고서 — 14장(22~23화) · T-11/은발 엘리아 검증 · 무발광 배경 탐색

- **작성:** Gemini (Antigravity)
- **기준 커밋:** `8c469ba6` (`claude/s366-upgrade` 병합 후 `gemini/memoria-review` HEAD `ab987e82`)
- **대조 원고:**
  - `MEMORIA_KR_Ch022_final.md` (22화. 공명, 369행)
  - `MEMORIA_KR_Ch023_final.md` (23화. 세 번째 사람, 421행)
  - `MEMORIA_KR_Ch021_final.md` (T-11 수정 검증용)
- **검수 대상 파일:**
  - `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaManuscriptScenes.cpp` (`ms_ridge_morning`, `ms_ridge_fields`, `ms_ridge_tower`, `ms_ridge_stumps`, `ms_ridge_house`, `ms_ridge_night`, `ms_ridge_slope`, `ms_ridge_ten_years`, `ms_ridge_third` — 9개 씬, 256행)
  - `Unreal/Tools/export_chapter_maps.py` (`ridge_farmhouse`)
  - `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaManuscript.cpp` (S385 메인 퀘스트)
  - `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaNavigationGoal.cpp` (14장 목표 8단계)
  - `Unreal/Memoria/Source/Memoria/Private/Chapter/MemoriaChapterMap.cpp` (S385 한국어 테이블)
  - `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaNarrativeSubsystem.cpp` (`ch14_book_awake`, `party_tobias`, `Joined: Tobias`)
  - `docs/unreal-migration/S385_REVIEW.md`
  - 커밋 `8c469ba6`(S387 T-11 수정) 및 `18f4be39`(S386 `assets/portraits/elia_face_silver_v1.png`)
  - `assets/cg/**` 전수 목록 중 무인물·무등불·무발광 후보 27종 직접 육안 열람

---

## 요약

- **14장 (`ridge_farmhouse`, 22~23화, S385):** **Must-fix 0건, Should-fix 0건, Nit 2건.** 지금까지 이식된 챕터 가운데 원고 충실도와 캐논 정합성이 가장 높습니다. 9개 씬 256행 전수 대조에서 대사·화자·존댓말·오른손/왼손 묘사·닫힌 책·무화기·토비아스 최초 등장 경계 모두 오류 없음.
- **14장 아트 결정 2건 판정:**
  1. **검은 배경(`Dark = TEXT("")`) 사용:** **강력 Pass (정답).** 22~23화의 `"불은 피우지 않았다"` / `"불빛은 없었다"` / 닫힌 책 캐논을 해치지 않으며, 기존 등불 배경(`chapter_splash_belt_waystation.png`) 재사용 피로도 제거.
  2. **토비아스 아르케인 회상에 `world_rewrite_tobias_ink_v3.png` 사용:** **Pass.** `ms_ridge_slope` 4~7행·`ms_ridge_ten_years` 0~2행의 순수 나레이션 회상 구간에만 정확히 한정하고 현재 시점과 셀렌 노트 개봉 시점(3행)에 즉시 `Dark`로 복귀, 대사 오버레이 없음.
- **T-11 수정 (`8c469ba6`, S387) 검증:** Must-fix 1/1, Should-fix 1/1, 텍스트·문서 Nit 3/3 **전부 반영 완료**(미반영 0건). 특히 `"그 사내?"`와 `"같아?" / "응."`을 압축 대신 원고 원문 행으로 복원하고 `S382` 테스트 인덱스를 동기화한 판단이 원고에 더 충실함.
- **은발 엘리아 초상화 (`assets/portraits/elia_face_silver_v1.png`, S386 `18f4be39`):** **Pass.** 은색 단발·푸른 눈·갈색 망토·지팡이/닻 없음으로 23화 캐논(`"은색 머리카락이 코트 어깨에 걸려 있었다"`)과 일치.
- **배경 탐색(Backdrop Hunt) 결과:** `assets/cg/**` 전체에서 야외·밭·고랑·농가·능선·회색 길 후보 27종을 직접 육안 열람한 결과, **무인물 + 무등불/무화기 + 무발광 3조건을 모두 충족하는 기존 CG는 `0`건**입니다. 따라서 전용 야외 무화기 아트가 제작되기 전까지 **11~13장의 불 없는 밤 씬(`roofless_farms`, `ms_basin_pen`, `ms_verdan_furrow`)도 14장처럼 `Dark`(검은 배경)로 통일하는 방안**을 권장합니다.

---

## Part 1 — 14장 (`ridge_farmhouse`, 22~23화) 씬·파일별 검수

### 상태 요약표

| 대상 | 씬 순서 | 화자·캐논 | 텍스트/번역 | CG/아트 | 판정 |
|---|---|---|---|---|---|
| `ms_ridge_morning` (22화) | No issues | No issues (왼손 검띠, 책 닫힘, 토비아스 미언급) | **No issues** | `Dark` (Pass) | **No issues** |
| `ms_ridge_fields` (22화) | No issues | No issues (침묵 농부 서술만) | **No issues** | `Dark` (Pass) | **No issues** |
| `ms_ridge_tower` (22화) | No issues | No issues (오른손 가방 미끄러짐→왼팔) | **No issues** | `Dark` (Pass) | **No issues** |
| `ms_ridge_stumps` (22화) | No issues | No issues (오른손 끈 빠짐→왼손 묶기) | **No issues** | `Dark` (Pass) | **No issues** |
| `ms_ridge_house` (22화) | No issues | No issues (3벽 농가, 굽은 흉터, 책 안 염) | **No issues** | `Dark` (Pass) | **No issues** |
| `ms_ridge_night` (22화) | No issues | No issues (불 없음, 빛 없음, 왼손 가까이 검) | **No issues** | `Dark` (Pass) | **No issues** |
| `ms_ridge_slope` (23화) | No issues | No issues (첫 줄 토비아스 진입, 왼손 발도, 존댓말, 은발, 책 닫힘) | **No issues** | `Dark` + `Ink`(4~7행 회상만, Pass) | **No issues** |
| `ms_ridge_ten_years` (23화) | No issues | No issues (셀렌·아미, 고해소, `"삭제를 먹습니다"`, 존댓말, 책 식음) | **No issues** | `Ink`(0~2행 회상만) + `Dark` (Pass) | **No issues** |
| `ms_ridge_third` (23화) | No issues | No issues (오른손 딱지, 왼손 검, 이름 교환, 합류, 칼 대신 빵) | **No issues** | `Dark` (Pass) | **No issues** |
| `export_chapter_maps.py` (`ridge_farmhouse`) | No issues | No issues (`lights: []`, `ambient_npcs: []`, 3벽 타일) | Nit 1건 (`321`행 주석) | `splash: ''` (Pass) | Nit only |
| `MemoriaManuscript.cpp` (S385) | — | No issues | **No issues** | — | **No issues** |
| `MemoriaNavigationGoal.cpp` (Ch14) | No issues | No issues (8단계 정확) | **No issues** | — | **No issues** |
| `MemoriaChapterMap.cpp` (S385) | — | No issues (`elia_repeat`·단서·안내·토스트 완비) | **No issues** | — | **No issues** |
| `MemoriaNarrativeSubsystem.cpp` (S385) | — | No issues (`ch14_book_awake`, `party_tobias`, `Joined: Tobias`) | **No issues** | — | **No issues** |
| `S385_REVIEW.md` | — | No issues (행 인덱스·S386 갱신 모두 사실 일치) | **No issues** | — | **No issues** |

### 필수 5대 캐논 점검 결과
1. **23화 이전 토비아스 미언급:** `ms_ridge_morning` ~ `ms_ridge_night` (6개 씬, 947~1072행) 전체에 토비아스 이름·초상화·발화 0건. **23화 첫 줄(`ms_ridge_slope:1075`, `"토비아스는 비탈 중간에서 멈췄다."`)**에서 정확히 등장 (**Pass**).
2. **토비아스 말투(존댓말)와 태도:** `"서 있습니다."`, `"예."`, `"책 때문에 왔습니다."`, `"저도 하나 갖고 있습니다."`, `"당신한테도 있습니까?"`, `"죄송합니다. 질문을 받는 게 익숙하지 않아서."`, `"같이 가고 싶습니다."`, `"이제 가져가도 됩니까?"` 등 전 대사가 원고의 공손·조심스러운 존댓말과 완벽 일치 (**Pass**).
3. **두 책 모두 닫힘 유지:** 22화 `"표지는 한 번도 들리지 않았다"`(956행)·`"안 열 거야."`(1031행)·`"밖으로 새어 나오는 빛도..."`(1057행) 및 23화 `"표지는 들리지 않았다."`(1107행)·`"들지 않았다."`(1150행)·`"지금 열 생각 없어."`(1158행) 모두 준수 (**Pass**).
4. **무화기·무등불:** `lights: []`, Drift 셸터 등불/지붕/차양 스킵, `ms_ridge_night:1036` `"불은 피우지 않았다."`, `ms_ridge_slope:1077` `"불빛은 없었다."` 모두 준수 (**Pass**).
5. **아렐 왼손 발도 및 오른손 경직:** 951행(왼손 검띠)·981행(가방 끈 미끄러짐→왼팔)·992/996행(부츠 끈→왼손)·1071행(오른손 찬 기운→왼손 가까이 검)·1084행(**`"칼집을 옆구리에 끼우고 왼손으로 자루를 당겼다."`**)·1176행(오른손 마른 딱지, 검은 여전히 왼손) 전부 정확히 반영 (**Pass**).

### 상세 지적 목록 (14장)

#### Must-Fix / Should-Fix
없음.

#### Nit
1. `Unreal/Tools/export_chapter_maps.py:321` — 주석 `"At first light Tobias comes down the slope, and stays."` — **23화 (9, 313행)** `"불빛은 없었다."` → `"빛이 농가 벽 위로 내려왔다."` — **수정안:** 토비아스가 비탈을 내려오는 시점은 동트기 전 어둠 속이고, 동이 트는 것은 대치·대화 뒤 이름 교환 시점(`ms_ridge_third`)입니다. 플레이어 노출은 없는 코드 주석이므로 추후 `"In the dark Tobias comes down the slope; at first light he stays."` 정도로 다듬으면 정확합니다.
2. `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaManuscriptScenes.cpp:46,1079,1121` — `Ink` (`world_rewrite_tobias_ink_v3.png`) — **23화 (17~25, 139~149행)** — **판정 (`Pass / 경미한 Nit`):** T-7에서 SAFE로 판정한 자산이며, 아르케인 서기 책상 회상 나레이션 구간(`ms_ridge_slope` 4~7행, `ms_ridge_ten_years` 0~2행)에만 정확히 한정해 대사 오버레이 없이 사용한 배치는 매우 적절합니다. 단, 그림 속 펜촉에 미세한 금빛 불꽃 1점이 있다는 점만 참고 Nit로 유지합니다.

---

## Part 2 — T-11 수정 (`8c469ba6`, S387) & 은발 엘리아 (`18f4be39`, S386) 검증

### T-11 수정 체크리스트 (`8c469ba6`)

| T-11 항목 | 등급 | `8c469ba6` 반영 여부 | 확인 내용 |
|---|---|---|---|
| #1 `ms_verdan_furrow:933` `"지금은 좀 자."` 화자 `Elia`→`Arrel` | Must-fix | **반영 완료** | `Arrel` / `arrel_neutral` 수정 + `MemoriaVerdanReturnS382Tests.cpp` 검증 추가 |
| #2 `ms_verdan_alley:900` 아렐 `"그 사내?"` 누락 | Should-fix | **반영 완료 (원문 복원)** | 제안한 압축 수정 대신 21화:329~333 원고 그대로 2행(`Arrel: "그 사내?"` + `Elia: "모른다고 했어."`) 복원 + 테스트 인덱스(`alley 13→15`) 동기화. 원고 충실도 측면에서 더 나은 해결책. |
| #3 `ms_verdan_vendor:876` En `"Went under."` | Nit | **반영 완료** | `"Taken below."`로 수정 |
| #4 `ms_verdan_furrow:931` `"같아?" / "응."` 압축 | Nit | **반영 완료 (원문 복원)** | 21화:489~497 원고 그대로 2행(`Arrel: "같아?"`, `Elia: "응. ...이건 확실해. 북서쪽."`) 복원 + 테스트 인덱스(`furrow 26→28`) 동기화 |
| #5 `S382_REVIEW.md:69` 인용 `"그들은 들어온 길로..."` | Nit | **반영 완료** | `"들어온 길로 나가지 않았다"`로 정정 + S387 내역 기록 |
| #6 `chapter_splash_belt_waystation.png` 고랑 밤 배경 | Nit | **의도적 보류 (T-14 연계)** | `S382_REVIEW.md:135`에 등불 없는 대체 배경 필요성 명시 → 아래 Part 3 배경 탐색으로 결론 도출 |

### 은발 엘리아 초상화 (`assets/portraits/elia_face_silver_v1.png`, S386 `18f4be39`) 검증
- **직접 육안 확인:** 은빛 단발머리(`은색 머리카락`), 차분한 푸른 눈, 어두운 갈색 망토(`갈색 코트`), 닻 문양 없음, 마법 지팡이 없음, 금발 잔재 없음.
- **캐논 판정:** **Pass.** 23화:89(`"은색 머리카락이 코트 어깨에 걸려 있었다"`) 및 최종본 전체의 엘리아 외형과 정확히 일치합니다. 모든 `elia_*` 표정 키가 단일 초상화를 공유하는 점(`sable_calm`과 동일 패턴)과 배경의 보름달/첨탑 실루엣은 경미한 Nit 수준이며 `S385_REVIEW.md:154`에 이미 투명하게 기록되어 있습니다.

---

## Part 3 — 무인물·무등불·무발광 배경 탐색 (Backdrop Hunt)

### 탐색 조건 및 대상
- **목표:** 11장(`roofless_farms`)·12장(`ms_basin_pen`)·13장(`ms_verdan_furrow`) 밤/야외 씬에 쓰인 `chapter_splash_belt_waystation.png`(켜진 등불·푸른 발광 문양 포함)를 대체할 기존 CG가 `assets/cg/**`에 있는지 확인.
- **필수 3조건:** **(1) 인물 없음**, **(2) 등불·모닥불·촛불 없음**, **(3) 마법 발광·빛줄기·보라 폭풍·수정 발광 없음** + **배경 적합성**(마른 밭·고랑·지붕 없는 농가·황혼/밤 능선·회색 길).

### 육안 검수 결과표 (총 27종 열람 — SAFE 0건)

| 자산 경로 | 인물 | 등불/불 | 발광/특수효과 | 실제 육안 확인 내용 | 판정 |
|---|---|---|---|---|---|
| `assets/cg/generated/chapter_splash_belt_waystation.png` | 없음 | **있음 (등불)** | **있음 (푸른 문양)** | 석조 역참 아치·켜진 등불·바닥 푸른 마법진 (현재 11~13장 사용 중) | **탈락** |
| `assets/cg/generated/chapter_splash_drift_shelter.png` | 없음 | **있음 (등불 다수)** | **있음 (황금 닻)** | 폭우 속 천막·등불·책장·벽의 빛나는 황금 닻 | **탈락** |
| `assets/cg/generated/chapter_splash_rim_forest.png` | 없음 | 없음 | **있음 (황금 수정·탑)** | 어두운 숲길 양옆의 빛나는 호박색 결정과 발광 탑 | **탈락** |
| `assets/cg/generated/chapter_splash_crumbling_coast.png` | 없음 | 없음 | 미세 불티 | **거센 바다 파도** + 절벽 위 고딕 성문 (마른 내륙 밭·농가와 불일치) | **탈락** |
| `assets/cg/generated/chapter_splash_seam_outskirts.png` | 없음 | 없음 | **있음 (보라 빛줄기)** | 공중에 떠 있는 부유석과 보랏빛 허공 | **탈락** |
| `assets/cg/generated/ch16_moving_horizon.png` | 없음 | 없음 | **있음 (보라 폭풍벽)** | 침수된 습지 석교 너머 거대한 보랏빛 발광 폭풍벽 | **탈락** |
| `assets/cg/generated/ch16_flooded_shortcut.png` | **있음 (3명)** | 없음 | **있음 (황금 격자+보라 폭풍)** | 물 위 빛나는 길에 3인 실루엣 + 보랏빛 폭풍벽 | **탈락** |
| `assets/cg/generated/ch17_storm_horizon.png` | **있음 (아렐)** | 없음 | **있음 (보라 번개)** | 보랏빛 번개 폭풍 속 검 짚은 갑옷 아렐 | **탈락** |
| `assets/cg/generated/memory_key_forest_rain.png` | 없음 | 없음 | **있음 (보라 소용돌이+황금 풀)** | 하늘에 거대한 보랏빛 소용돌이 + 빛나는 풀 언덕 | **탈락** |
| `assets/cg/generated/illustration_expansion_v3/resonance_colorless_waste_deep_v3.png` | 없음 | 없음 | **있음 (청/금 빛줄기)** | 황동 나침반에서 뻗어 나오는 푸른·황금빛 마법 길 | **탈락** |
| `assets/cg/generated/resonance_colorless_waste_echo.png` | 없음 | 없음 | **있음 (청/금 빛줄기)** | 회색 돌밭 위 나침반과 빛나는 마법 선 | **탈락** |
| `assets/cg/generated/illustration_expansion_v3/resonance_drift_shelter_deep_v3.png` | 없음 | **있음 (등불)** | **있음 (황금 발자국/손자국)** | 비 오는 천막 안 등불 + 테이블·바닥의 빛나는 손/발자국 | **탈락** |
| `assets/cg/generated/resonance_drift_shelter_echo.png` | 없음 | **있음 (등불)** | **있음 (황금 손자국)** | 젖은 천막 안 등불 + 테이블 위 빛나는 양손 자국 | **탈락** |
| `assets/cg/generated/illustration_expansion_v2/resonance_belt_waystation_deep_v2.png` | 없음 | 없음 | **있음 (황금 나무+푸른 발자국)** | 열린 책에서 솟는 황금빛 나무 + 바닥의 푸른 발자국 | **탈락** |
| `assets/cg/generated/illustration_expansion_v3/resonance_crumbling_coast_deep_v3.png` | 없음 | 없음 | **있음 (황금 발자국·손바닥)** | 부서지는 바다 절벽 + 양 갈래 빛나는 발자국과 손바닥 비석 | **탈락** |
| `assets/cg/game_image/env_wasteland_city.png` | **있음 (원경 1명)** | 없음 | 푸른 안개 | 좌하단에 **`"SEALED ZONE / DISTRICT OF FORGOTTEN MEMORIES"` 영문 워터마크 텍스트** + 폐허 도시 | **탈락** |
| `assets/cg/generated/ch18_broken_funeral_platform.png` | 없음 | **있음 (화로 불)** | **있음 (책 금빛 균열)** | 고딕 성당 안 불타는 화로 + 사슬 묶인 빛나는 거대 책 | **탈락** |
| `assets/cg/generated/story_ch8_white_stone_shelter.png` | **있음 (4명)** | 없음 | 미세 반딧불 | 세이블·아렐·엘리아·토비아스 4인과 돌무더기 | **탈락** |
| `assets/cg/generated/story_ch9_final_colorless_view.png` | **있음 (2명)** | 없음 | 없음 | 갑옷 아렐·금발 엘리아 뒷모습 | **탈락** |
| `assets/cg/generated/story_ch8_end_of_color.png` | **있음 (4명)** | 없음 | 없음 | 아렐·금발 엘리아·토비아스·4번째 여검사 | **탈락** |
| `assets/cg/generated/chapter_expansion/ch07_threshold_ridgeline_v1.png` | **있음 (2명)** | **있음 (등불)** | 없음 | 절벽 끝 등불 든 인물과 아렐 | **탈락** |
| `assets/cg/generated/story_ch6_stars_forgetting.png` / `ch06_stars_forgetting_v1.png` | **있음 (2~3명)** | **있음 (등불/도시불)** | 있음 | 아렐·금발 엘리아(+ 등불 든 세이블) | **탈락** |
| `assets/cg/generated/story_ch4_ash_rain_shelter.png` / `departure.png` | **있음 (3명)** | **있음 (등불)** | 없음 | 아렐·금발 엘리아·토비아스 + 등불 | **탈락** |
| `assets/cg/generated/story_ch1_twisted_forest_path.png` / `green_tree_dawn.png` / `rim_omen.png` | **있음 (2명)** | 등불(`omen`) | 수정/보라 번개 | 갑옷 아렐·금발 엘리아 + 발광 수정/괴수 | **탈락** |
| `assets/cg/generated/epilogue_sable_eastern_settlement.png` / `ending_zero_burn_canyon_watch.png` / `ending_ash_hollow_days.png` / `sunset_shell.png` / `ch15_dawn_refrain.png` / `ch23_partial_shoreline.png` / `ch09_depth_markers_v1.png` / `story_ch5_threaded_horizon_v2.png` | **있음 (2~3명)** | 마을불/촛불 | 보라 원/금빛 실/균열 | 모두 주역 인물 포함 (대부분 갑옷 아렐·금발 엘리아) | **탈락** |

### 배경 탐색 결론 및 권장 조치
- **`SAFE` 기존 대체 배경: `0`건.** 현재 `assets/cg/**`에는 인물·등불·발광 효과가 모두 없는 야외 마른 밭/고랑/지붕 없는 농가/양 우리/능선 밤 배경이 존재하지 않습니다.
- **권장 조치 (즉시 적용 가능, 신규 임포트 0건):**
  - 14장(S385)에서 검증된 **`Dark` (`TEXT("")`, 검은 배경)**를 **11~13장의 불 없는 야외·밤 씬에도 동일하게 적용**하는 것을 권장합니다:
    - 11장 `roofless_farms`: `splash: ''` (지붕 없는 농가/밭 — 17~18화에도 등불 없음)
    - 12장 `basin_station`: `ms_basin_pen` (`Pen` → `Dark`, 마른 시내 위 회색 양 우리 밤)
    - 13장 `verdan_return`: `ms_verdan_furrow` (`Pen` → `Dark`, 묵은 밭 고랑 밤 — 시장 스플래시에서 검은 밤 화면으로 전환되어 고랑의 어둠과 완벽 부합)
  - 향후 전용 배경 아트를 새로 1~2장 제작할 경우, **① 마른 돌담 밭과 지붕 없는 농가(황혼·무등불·무인물)** 및 **② 묵은 고랑·능선의 회색 밤길(무등불·무발광·무인물)** 2종이면 11~14장 전체를 커버할 수 있습니다.
