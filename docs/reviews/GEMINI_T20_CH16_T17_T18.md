# T-20 원고·캐논 검수 보고서 — 16장(25~26화) · T-17/T-18 검증

- **작성:** Gemini (Antigravity)
- **기준 커밋:** `8714a82f` (`claude/s366-upgrade` 병합 후 `gemini/memoria-review` HEAD `b438a12b`)
- **대조 원고 (전편 완독):**
  - `MEMORIA_KR_Ch025_final.md` (25화. 고해소, 435행)
  - `MEMORIA_KR_Ch026_final.md` (26화. 카이로스 주저하다, 381행)
- **검수 대상 파일 및 캡처:**
  - `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaManuscriptScenes.cpp` (`ms_arcane_*` 7씬 + `ms_kairos_*` 5씬 = 12씬, 312행)
  - `Unreal/Tools/export_chapter_maps.py` (`HANDMADE['arcane_city']`, `MANUSCRIPT['arcane_city']`)
  - `Unreal/Memoria/Source/Memoria/Private/Chapter/MemoriaChapterPresentation.cpp` (출구 `then` 연쇄)
  - `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaManuscript.cpp` (S393 메인 퀘스트)
  - `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaNavigationGoal.cpp` (16장 목표 6단계)
  - `Unreal/Memoria/Source/Memoria/Private/Chapter/MemoriaChapterMap.cpp` (S393 한국어 테이블)
  - `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaNarrativeSubsystem.cpp` (무명 화자 한국어 이름 8종 + `ch16_burned_hand`/`ch16_unsubmitted`)
  - `Unreal/Memoria/Source/Memoria/Private/Presentation/MemoriaNarrativeArtwork.cpp` (`kairos_face_neutral`, `env_gray_belt_panorama`, `ch14_unsent_incident_report`)
  - `docs/unreal-migration/S393_REVIEW.md`
  - 커밋 `8714a82f` (S394, T-17 15장 플라크 수정) 및 `bbde9491` (S392, T-18 저장 머티리얼 `M_EliaHair`/`M_TobiasCoat`)
  - **직접 육안 열람한 캡처:**
    - Opus 레인 `Saved/Validation/S393Arcane/`: `S393Title.png`, `S393GateStreet.png`, `S393FiveBooths.png`, `S393Booths.png`, `S393Bench.png`, `S393Cistern.png`, `S393Tower.png`, `S393Teacup.png`, `S393OnlyWhatYouSaw.png`, `S393Relay.png`, `S393Stopper.png`, `S393Unsubmitted.png`, `S393DidIIntervene.png`, `S393Complete.png` (14종)
    - Opus 레인 `Saved/Validation/S389Wells/S389Wells.png` (S394 플라크 갱신 확인)
    - Grok 레인 `Saved/Validation/S388FieldLooksT18/Captures/S388Looks/Elia.png`, `Walking.png` (T-18 저장 머티리얼 확인)

---

## 요약

- **16장 (`arcane_city`, 25~26화, S393):** **Must-fix 0건, Should-fix 0건, Nit 5건.**
  - 12개 씬 312행 전수 대조에서 대사·화자·무명 단역 이름·아렐 연소(머리를 받쳐 주던 손)·무발도/무전투·토비아스 존댓말·**카이로스↔3인 완전 분리**까지 원고 캐논과 완벽하게 일치합니다.
  - 수작업 그라운드 `HANDMADE['arcane_city']`의 동선(남문 → 첫 광장 빵 창구 → 둘째 광장 남쪽 목조 부스 5동 & 벤치 → 북서 골목 저수조 → 낮은 아치 끝 놋쇠 고리 문)과 출구 `then` 5연쇄 컷어웨이 구조가 25~26화의 거울 서사를 매우 설득력 있게 구현했습니다.
- **16장 아트 결정 4건 판정:** ① `kairos_face_neutral.png` **Pass** (평온/수사 톤 적합, `tobias_face_neutral.png`와 선화 계열 일치), ② `env_gray_belt_panorama.png` **Pass**, ③ `ch14_unsent_incident_report.png` **강력 Pass**, ④ 중계국 `Dark` **Pass**.
- **T-17 수정 (`8714a82f`, S394) 검증:** **100% Pass.** `S389Wells.png` 좌상단 플라크가 **`마른 우물` / `두 노트`**로 정확히 표시됨을 육안 확인.
- **T-18 저장 머티리얼 (`bbde9491`, S392) 검증:** **100% Pass.** `M_EliaHair.uasset`/`M_TobiasCoat.uasset` 정식 저장·하드 참조·에디터 분기 제거 확인 및 `S388FieldLooksT18` 캡처 2종이 T-15 캡처와 동일함을 육안 확인.

---

## Part 1 — 16장 (`arcane_city`, 25~26화) 씬·파일별 검수

### 상태 요약표

| 대상 | 씬 순서 | 화자·캐논 | 텍스트/번역 | CG/아트 | 판정 |
|---|---|---|---|---|---|
| `ms_arcane_gate` (1408~1441행) | No issues | No issues (토비아스 단독 호명, `"동행 둘"`, 검에서 손 뗌, 존댓말) | **No issues** | 파노라마 (Pass) | **No issues** |
| `ms_arcane_squares` (1443~1458행) | No issues | No issues (빵 창구 4명, 목조 부스 5동·종, 셋째 부스 11명+아이) | **No issues** | 파노라마 (Pass) | **No issues** |
| `ms_arcane_booth` (1460~1491행) | No issues | No issues (노인·아이 덧댄 천, `"누구 애냐."`, `"셋째. 퇴실 보조."`, 오른손 중지 지연) | **No issues** | 파노라마 (Pass) | **No issues** |
| `ms_arcane_burn` (1494~1519행) | No issues | No issues (머리 받쳐 주던 손 연소, 4걸음, 왼팔+오른손바닥, `"세 걸음. 벤치까지."`) | **No issues** | 파노라마 (Pass) | **No issues** |
| `ms_arcane_alley` (1521~1547행) | No issues | No issues (오른무릎 꺾임, 기대던 감각 소실, 엘리아 부축, 토비아스 짧은 확인) | **No issues** | 파노라마 (Pass) | **No issues** |
| `ms_arcane_cistern` (1549~1566행) | No issues | No issues (저수조 돌벽, 작은 탑, `"여기서 태우면 탑에 남습니다."`, 책 온기·안 폄) | Nit 1건 (1563행 영문) | 파노라마 (Pass) | Nit only |
| `ms_arcane_room` (1569~1580행) | No issues | No issues (놋쇠 고리 문, 성문 이름, 빗장, `"앉아. 먼저."`) | **No issues** | 파노라마 (선택 Nit: `Dark`) | Nit only |
| `ms_kairos_report` (1583~1598행) | No issues | No issues (12:22, K-7, 아헨 숲 파형, 외곽 셋방, 붉은 노트 `"관찰함. 개입하지 않음."`) | **No issues** | `Unsent` (강력 Pass) | **No issues** |
| `ms_kairos_booth` (1600~1642행) | No issues | No issues (`"검을 뽑았나."/"아닙니다."`, `"직접 본 것만 적게."`, `"오늘 고해는 끝났어."`) | **No issues** | 파노라마 + 신규 얼굴 (Pass) | **No issues** |
| `ms_kairos_gate` (1644~1664행) | No issues | No issues (`"토비아스."`/`동행 둘`, 동부국 등록 서기, K-7 열람 표식) | **No issues** | 파노라마 + 신규 얼굴 (Pass) | **No issues** |
| `ms_kairos_relay` (1666~1694행) | No issues | No issues (오후 4시, 조정관1+8명 급행 하루 반, 노인 확보 배제, K-7, 빈손) | **No issues** | `Dark` (Pass) | **No issues** |
| `ms_kairos_unsent` (1697~1734행) | No issues | No issues (`"비무장 접촉"` 삭제, 연소 잉크 닫힘, `"미제출"`, `"개입했나?"`, 저수조 남행) | **No issues** | `Unsent` (강력 Pass) | **No issues** |
| `export_chapter_maps.py` (`HANDMADE`/`MANUSCRIPT`) | No issues | No issues (25화 동선·5부스·벤치·빵 창구·저수조·놋쇠 고리 문 일치, `lights: []`, 무전투) | **No issues** | 부스 카메라 가림 Nit 1건 | Nit only |
| `MemoriaChapterPresentation.cpp` (`then` 연쇄) | No issues | No issues (방→26화 5씬 순차 실행 후 챕터 종료·완료 카드 `"16장 완료 · 미제출"`) | **No issues** | — | **No issues** |
| `MemoriaManuscript.cpp` / `MemoriaNavigationGoal.cpp` / `MemoriaChapterMap.cpp` | No issues | No issues (목표 6단계·메인 퀘스트·토스트·단서 한국어 완비) | **No issues** | — | **No issues** |
| `MemoriaNarrativeSubsystem.cpp` (`SpeakerKo` + 플래그) | — | No issues (무명 화자 8종 한국어 정확, `ch16_burned_hand`/`ch16_unsubmitted` 토스트 정확) | Nit 1건 (`Old Man` 중복) | — | Nit only |
| `S393_REVIEW.md` | — | No issues (312행·목표·테스트·캡처 모두 사실 일치) | Nit 1건 (152행 T-18 이후 갱신) | — | Nit only |

### 필수 5대 캐논 및 4대 아트 결정 판정
1. **카이로스↔3인 완전 미대면:** 25화 7씬(`ms_arcane_*`)에 카이로스 등장·언급 0건, 26화 5씬(`ms_kairos_*`)에 아렐·엘리아·토비아스 등장 0건(성문 명부 문자 언급만). `exit.then`으로 방 빗장 잠금 직후 26화가 깔끔하게 이어짐 (**Pass**).
2. **연소 내용(머리를 받쳐 주던 손):** `ms_arcane_burn:1495~1498`, `ms_arcane_alley:1534~1540`, 토스트 `"연소: 머리를 받쳐 주던 손"` 원고 그대로 보존 (**Pass**).
3. **무발도·무전투:** 전투·조우 비활성(`battles: []`, `encounters_gate: 'ms_never'`), 성문에서 검에 손 뗌, `"검을 뽑았나."/"아닙니다."`, `"검 사용 없음"` 모두 준수 (**Pass**).
4. **토비아스 존댓말 및 무명 단역 8종:** 토비아스 전 대사 공손한 존댓말 일치; 무명 화자 8종(`성문 서기`, `고해관`, `서기`, `아이`, `노인`, `줄 선 사람`, `심부름꾼`, `접수 서기`) 모두 무초상화(`nullptr`)·정확한 한국어 표기 (**Pass**).
5. **아트 4건 판정 (`S393Arcane/` 14종 캡처 육안 확인):**
   - **`kairos_face_neutral.png` (`S393OnlyWhatYouSaw.png`):** **Pass.** 인장 발광·과장된 연출 없는 담백한 회색 제복 얼굴로 26화의 냉정한 조사·주저 톤에 잘 맞고, `tobias_face_neutral.png`와 선화 화풍도 자연스럽게 어울립니다(구 9장 대사에서도 과한 발광 샷 대신 정갈한 얼굴로 통일됨). 단, `MemoriaNarrativeArtwork.cpp:129`의 `kairos_cold`는 기존 `kairos_edit_v3.png`를 유지 중이므로, 구 챕터에 `kairos_cold`가 남아 있는지 정도만 참고.
   - **아르케인 파노라마 (`env_gray_belt_panorama.png`, `S393FiveBooths.png`, `S393Tower.png`, `S393OnlyWhatYouSaw.png`):** **Pass.** 정오의 잿빛 석조 관료도시 분위기와 무발광·무등불 조건을 완벽히 충족.
   - **미제출 보고서 책상 (`ch14_unsent_incident_report.png`, `S393Teacup.png`, `S393Stopper.png`, `S393Unsubmitted.png`, `S393DidIIntervene.png`):** **강력 Pass.** `"마개는 닫힌 채였다."`, `"미제출로 남깁니까?"`, `"개입했나?"` 장면에서 구도·색감·인물 단독 조건이 최상급.
   - **중계국 검은 배경 (`Dark`, `S393Relay.png`):** **Pass.** 오후 실내 접수대와 바깥 거리·셋방 간의 장면 전환이 명료함.

### 상세 지적 목록 (16장)

#### Must-Fix / Should-Fix
없음.

#### Nit
1. **`S393Booths.png` / `S393Bench.png` — 남쪽 목조 부스 높이에 의한 시선 겹침:** `HANDMADE['arcane_city']`에서 부스 5동이 둘째 광장 남쪽(`y=9`)에 배치되어 있어, 남쪽에서 북쪽을 올려다보는 카메라 각도상 아렐이 `y=8`에서 걸을 때 넷째 부스 지붕이 아렐의 발끝과 약간 겹쳐 보이고(`S393Booths.png`), `y=8` 대기열 5명 중 좌측 인원 일부가 1~2번 부스 뒤에 부분적으로 가려집니다. 원고의 `"둘째 광장 남쪽에는 나무 부스 다섯"` 배치를 충실히 따른 결과이며 통행·플레이에 지장은 없으므로 시각적 참고 Nit로만 기록합니다.
2. **`Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaNarrativeSubsystem.cpp:110,116`:** `SpeakerKo`의 `Names` 맵 초기화 리스트에 `{TEXT("Old Man"), TEXT("노인")}`이 110행과 116행에 중복 기입되어 있습니다(동일 키·값이라 기능상 무해, 추후 정리 시 1건 제거).
3. **`Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaManuscriptScenes.cpp:1563` (`ms_arcane_cistern`):** 영문 `"Arrel looked at the two of them's hands."` — 소유격 표현이 다소 어색하므로 추후 손볼 때 `"Arrel looked at both of their hands."` 또는 `"the hands of the two of them"`으로 다듬으면 자연스럽습니다.
4. **`Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaManuscriptScenes.cpp:1569` (`ms_arcane_room`, 선택 Nit):** 토비아스 방 안(25화 마지막 씬)은 현재 맵 스플래시(도시 전경 파노라마)를 그대로 상속합니다. 17장(27~30화)이 방 안 씬을 `Dark`로 시작할 예정이므로, 추후 원한다면 `ms_arcane_room` 0행도 `Dark`로 전환하면 문턱을 넘어 좁은 방에 들어서는 전환과 17장 연결이 더 자연스러워집니다(현 상태도 Pass).
5. **`docs/unreal-migration/S393_REVIEW.md:152`:** `"Elia's hair tint and Tobias's coat are editor-only until Grok's T-18 lands."` — 현재 통합선(`8714a82f`)에서는 T-18(`bbde9491`)이 이미 반영 완료되었으므로 추후 문서 갱신 시 1줄 정리 권장.

---

## Part 2 — T-17 수정 (`8714a82f`, S394) & T-18 (`bbde9491`, S392) 검증

| 검증 대상 | 확인 내용 | 판정 |
|---|---|---|
| **T-17 수정 (`8714a82f`, S394 — 15장 플라크)** | `export_chapter_maps.py:428` 및 `dry_wells.v1.json`의 `subtitle`이 `'Two Notes'`로 변경되었고, `S389_REVIEW.md:119,123~125`도 갱신됨. **`S389Wells.png` 직접 육안 확인 결과 좌상단 플라크가 `마른 우물` / `두 노트`로 깔끔하게 표시됨.** | **Pass** |
| **T-18 (`bbde9491`, S392 — 저장 머티리얼)** | `Content/Memoria/Presentation/FieldLooks/M_EliaHair.uasset`·`M_TobiasCoat.uasset` 정식 에셋 저장, `UMemoriaFieldCharacterComponent` 생성자에서 `ConstructorHelpers::FObjectFinderOptional` 하드 참조 등록, `InitializeTobias`/`InitializeRigged`에서 `#if WITH_EDITOR` 분기 제거 확인. | **Pass** |
| **T-18 캡처 비교 (`S388FieldLooksT18/Captures/S388Looks/Elia.png`, `Walking.png`)** | 직접 육안 대조 결과 T-15(`S388Looks`) 캡처와 외형·색상·배지 밀착·등 가방·은발·무지팡이 모두 **완벽히 동일함**을 확인. | **Pass** |
