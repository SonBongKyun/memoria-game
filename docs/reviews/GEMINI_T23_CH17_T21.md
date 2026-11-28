# T-23 원고·캐논 검수 보고서 — 17장(27~30화) · T-21 한나 연동 · T-20 Nit 검증

- **작성:** Gemini (Antigravity)
- **기준 커밋:** `128dc475` (`claude/s366-upgrade` 병합 후 `gemini/memoria-review` HEAD `696db793`)
- **대조 원고 (전편 완독):**
  - `MEMORIA_KR_Ch027_final.md` (27화. 조용함의 무게, 375행)
  - `MEMORIA_KR_Ch028_final.md` (28화. 노래하는 자, 445행)
  - `MEMORIA_KR_Ch029_final.md` (29화. 지하의 노래들, 389행)
  - `MEMORIA_KR_Ch030_final.md` (30화. 어머니의 자장가, 375행)
- **검수 대상 파일 및 캡처:**
  - `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaManuscriptScenes.cpp` (`ms_leave_*` 13씬, 414행 + `ms_arcane_room` T-20 수정)
  - `Unreal/Tools/export_chapter_maps.py` (`MANUSCRIPT['arcane_leave']`)
  - `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaManuscript.cpp` (S396 메인 퀘스트)
  - `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaNavigationGoal.cpp` (17장 목표 3단계)
  - `Unreal/Memoria/Source/Memoria/Private/Chapter/MemoriaChapterMap.cpp` (S396 한국어 테이블)
  - `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaNarrativeSubsystem.cpp` (`ch17_relay_read`, `ch17_lullaby_words`, `hanna_gone` + `SetLeavingPoint`, T-20 `Old Man` 중복 제거)
  - `Unreal/Memoria/Source/Memoria/Private/Chapter/MemoriaChapterPresentation.cpp` (`party_hanna` → `AMemoriaHannaCompanion::Join`)
  - `docs/unreal-migration/S396_REVIEW.md` (S397 섹션 포함) & `docs/unreal-migration/S393_REVIEW.md` (T-20 갱신)
  - **직접 육안 열람한 캡처 (12종):**
    - Opus 레인 `Saved/Validation/S396Leave/`: `S396Title.png`, `S396Candle.png`, `S396BookClosed.png`, `S396HannaFollows.png`, `S396Cistern.png`, `S396BootDrags.png`, `S396HannaLeaving.png`, `S396Two.png`, `S396Complete.png` (9종)
    - Grok 레인 `Saved/Validation/S395Hanna/`: `Hanna.png`, `Walking.png`, `Leaving.png` (3종)

---

## 요약

- **17장 (`arcane_leave`, 27~30화, S396/S397):** **Must-fix 0건, Should-fix 0건, Nit 4건.**
  - 13개 씬 414행 전수 대조에서 **한나 무발화(감정표 문구 전부 나레이션 원문 보존)** · 책의 5대 문장(`"중계는 손으로 열리지 않는다."`~`"끝까지 열면 죽는다."`) · 토비아스 존댓말 및 10년 전 루메아 · 자장가 가사(`"작은 아이야. 천천히. 그다음에 강."`, 미가창) · 남문 통과(`"동행은요?"` / `"둘입니다."`)까지 원고 캐논과 완벽하게 일치합니다.
  - 16장 수작업 아르케인 그라운드를 놋쇠 고리 문에서 남문으로 역방향 재활용(`arcane_leave`)한 설계가 30화 후반 도보 탈출 동선과 정확히 맞물립니다.
- **아트 결정 3건 판정:** ① 방 안 10씬 전부 `Dark` **강력 Pass**, ② 거리 2씬 아르케인 파노라마 **Pass**, ③ 16장 `ms_arcane_room` `Dark` 전환(및 T-20 Nit 4건 전부 반영) **Pass**.
- **T-21 한나 필드 외형·합류·퇴장 (S397) 검증:** **Pass** (회색 코트 · 흰 가닥 섞인 짧은 머리 · 모자 띠 짙은 자국 · 등 가방 · 무무기/무등불/무발광 · 문 앞 합류 4인 종대 · 타일 `(20,10)` 다른 골목 퇴장 모두 캐논 부합; 근접 샷 모자 큐브 클리핑 및 초기 벽 오프셋 스폰은 비차단 참고 Nit).

---

## Part 1 — 17장 (`arcane_leave`, 27~30화) 씬·파일별 검수

### 상태 요약표

| 대상 | 씬 순서 | 화자·캐논 | 텍스트/번역 | CG/아트 | 판정 |
|---|---|---|---|---|---|
| `ms_leave_quiet` (1743~1766행, 27화) | No issues | No issues (짧은 초·왼손 물컵·`"읽는 것도 피 나."`·아렐 벽 기대 잠듦) | **No issues** | `Dark` (Pass) | **No issues** |
| `ms_leave_relay` (1769~1798행, 27화) | No issues | No issues (`"중계는 손으로 열리지 않는다."`~`"끝까지 열면 죽는다."`·코피·왼손 컵) | **No issues** | `Dark` (Pass) | **No issues** |
| `ms_leave_cost` (1800~1844행, 27화) | No issues | No issues (아렐에게 비밀·`"내가 먼저 알아야 해."`·문 밖 3→4음·눈 감은 채 왼손 검자루) | **No issues** | `Dark` (Pass) | **No issues** |
| `ms_leave_hanna` (1847~1880행, 28화) | No issues | No issues (회색 코트·모자 띠 자국·손바닥 인사·**한나 무발화**·감정표 원문 보존) | Nit 2건 (`1866`, `1876`) | `Dark` (Pass) | Nit only |
| `ms_leave_east` (1882~1912행, 28화) | No issues | No issues (`"걸어서 간다."`·`"동쪽."`·`"여기서는 못 쓴다."`·`"네가 정해."`·`"마차를 보는 데까지야."`) | **No issues** | `Dark` (Pass) | **No issues** |
| `ms_leave_wait` (1914~1957행, 28화) | No issues | No issues (`"머물러. 두 시간."`·2음 흥얼거림·엘리아 오른손 목·`"십 년입니다."`·동쪽 처음) | **No issues** | `Dark` (Pass) | **No issues** |
| `ms_leave_printer` (1960~2022행, 29화) | No issues | No issues (10년 전 루메아 하오체 감정표·돌벽 4→5음·`"가운데가 닮았어."`, `2000`행 `"문 쪽이 아니야."` 원고 29화:163~165 원문 그대로!) | **No issues** | `Dark` (Pass) | **No issues** |
| `ms_leave_songs` (2025~2065행, 29화) | No issues | No issues (`"시계가 없습니다."`·한나 귀환·`"다섯 음을 지났는데, 한나는 멈추지 않았다."`) | **No issues** | `Dark` (Pass) | **No issues** |
| `ms_leave_lullaby` (2068~2141행, 30화) | No issues | No issues (여덟째 마일 마당·검은 빵 3덩이·`"자장가야."`·**`"작은 아이야. 천천히. 그다음에 강."`**·`"오늘은 안 불러요."`) | **No issues** | `Dark` (Pass) | **No issues** |
| `ms_leave_door` (2143~2152행, 30화) | No issues | No issues (`"나는 남문까지."`·의자 버팀·놋쇠 고리 손으로 받음·`party_hanna` 설정) | **No issues** | `Dark` (Pass) | **No issues** |
| `ms_leave_cistern` (2155~2161행, 30화) | No issues | No issues (저수조 위쪽 길·아렐 부츠 끌림·한나 되돌아와 나란히 걸음) | **No issues** | 파노라마 (Pass) | **No issues** |
| `ms_leave_gate` (2164~2169행, 30화) | No issues | No issues (남문 안쪽 낮은 돌·`'둘째 마일'` 재표기·다른 골목 퇴장·`hanna_gone` 설정) | **No issues** | 파노라마 (Pass) | **No issues** |
| `ms_leave_out` (2172~2176행, 30화) | No issues | No issues (`"동행은요?"` / `"둘입니다."`·토비아스 아렐 왼편·엘리아 보폭 맞춤) | **No issues** | 파노라마 (Pass) | **No issues** |
| `export_chapter_maps.py` (`arcane_leave`) | No issues | No issues (16장 타일 역방향, 문 스폰→저수조→낮은 돌→남문 출구, 고해소 줄 제거, 무전투·무등불) | **No issues** | 파노라마 스플래시 (Pass) | **No issues** |
| `MemoriaManuscript.cpp` / `NavigationGoal` / `ChapterMap` / `NarrativeSubsystem` | No issues | No issues (목표 3단계·메인 퀘스트·토스트 `"떠오름: 작은 아이야, 천천히, 강"`·완료 카드 `"17장 완료 · 남문"`·한나 합류/퇴장 훅 정확) | **No issues** | — | **No issues** |
| `S396_REVIEW.md` (S397 포함) | — | No issues (414행·13씬·T-21 연동·벽 오프셋 한계까지 사실 일치) | **No issues** | — | **No issues** |

### 필수 5대 캐논 및 3대 아트 결정 판정
1. **한나 무발화 (Hanna never speaks):** 414행 전체에 `Hanna` 화자 행 **0건**. 감정표의 모든 문구가 원고 원문 그대로 순수 나레이션(`Speaker = nullptr`)으로 표시됨(`S396BookClosed.png` 확인). 초상화 없음, 구 `Han` 에셋 미사용 (**Pass**).
2. **책의 5대 문장 (`ms_leave_relay:1777~1792`):** `"중계는 손으로 열리지 않는다."` → `"중계는 들려진 소리로 열린다."` → `"여는 몸은 보관장소였던 몸이다."` → `"여섯은 좌석이다."` / `"여는 자를 알아볼 좌석은 하나다."` → `"좌석은 알 것이다."` → `"몸은 중계의 비용이다."` → `"끝까지 열면 죽는다."` 100% 원문 일치 (**Pass**).
3. **토비아스 존댓말 & 10년 전 루메아:** 전편 존댓말 유지(`29화:163~165`의 돌발 혼잣말 `"문 쪽이 아니야."`까지 원고 그대로 반영), 루메아 10년 전 감정표 하오체(`"앉으시오."`, `"이레 뒤에 사라졌소."`, `"활자판을 가져갈 때 혀도 가져갔소."`, `"우리가 다다르는 것들을."`) 완벽 보존 (**Pass**).
4. **자장가 가사 & 남문 통과:** `"작은 아이야. 천천히. 그다음에 강."`(노래가 아닌 평상시 말 높이, `"오늘은 안 불러요."`) 및 `"동행은요?"` / `"둘입니다."` 원고 그대로 반영 (**Pass**).
5. **아트 결정 3건 (`S396Leave/` 9종 육안 확인):**
   - 방 안 10씬 전부 `Dark` (`S396Candle.png`, `S396BookClosed.png`): **강력 Pass** (도시 파노라마 침투 0건).
   - 거리 2씬 아르케인 파노라마 (`S396BootDrags.png`, `S396Two.png`): **Pass** (실내→야외 거리 전환 선명).
   - 16장 `ms_arcane_room` `Dark` 전환 (`128dc475`): **Pass** (+ T-20 Nit 4건 `"both of their hands"`, `Old Man` 단일 등록, `S393_REVIEW.md` T-18 완료 메모까지 전부 반영 확인).

---

## Part 2 — T-21 한나 필드 외형·합류·퇴장 (`577ea45c`, `128dc475`) 검증

직접 열람한 `S395Hanna/` 3종(`Hanna.png`, `Walking.png`, `Leaving.png`) 및 `S396Leave/` 4종(`S396Title.png`, `S396HannaFollows.png`, `S396Cistern.png`, `S396HannaLeaving.png`) 대조 결과입니다.

| 검증 항목 | 캡처 육안 및 코드 확인 결과 | 판정 |
|---|---|---|
| **한나 외형 (`Hanna.png`, `Walking.png`, `S396HannaFollows.png`)** | 밝은 **회색 코트**(토비아스 어두운 코트와 명확히 구분), 뒤통수 **흰 가닥 섞인 짧은 머리**, 한쪽 띠에 **짙은 사각 표찰 자국**이 있는 모자, 등 뒤 **사각 가방**, 무기·등불·발광 0건. 필드 카메라 거리(`Walking.png`, `S396HannaFollows.png`, `S396Cistern.png`)에서 4인 종대(아렐→엘리아→토비아스→한나)가 즉시 식별됨. | **Pass** |
| **합류 위치 (`ms_leave_door` 종료, `S396HannaFollows.png`, `S396Cistern.png`)** | `ms_leave_door` 종료 시 `party_hanna` 설정 + `AMemoriaHannaCompanion::Join` 즉시 호출로 리로드 없이 놋쇠 고리 문 앞 합류, 저수조 위쪽 길까지 안정적으로 추종. | **Pass** |
| **퇴장 위치 (`HannaLeavingPoint` = 타일 `(20,10)`, `S396HannaLeaving.png`)** | `HANDMADE['arcane_city']`에서 `(20,10)`은 두 광장 사이를 지나 동쪽 막다른 골목 끝으로, 남문 거리(`x=11~13, y=14~17`)와 방향이 완전히 다르고 빵 창구 대기열(`y=12`)과도 2타일 이상 떨어져 충돌하지 않음. 원고 30화:361 **`"성문을 등지고, 그들이 온 길과 다른 골목으로 들어갔다"`**로 자연스럽게 읽히며, `S396HannaLeaving.png`에서 성문 통과 전 완전 퇴장 확인. | **Pass** |

### 상세 지적 목록 (17장 + T-21)

#### Must-Fix / Should-Fix
없음.

#### Nit
1. **`Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaManuscriptScenes.cpp:1876` (`ms_leave_hanna`, 28화:121~123):** `{TEXT("Tobias"), TEXT("tobias_neutral"), nullptr, TEXT("\"Don't wait to see who finds you first.\""), TEXT("누가 먼저 찾을지 기다리지 마라.")}` — 영문에는 큰따옴표가 붙어 한나의 감정표를 대신 읽는 대사임이 드러나지만, 한국어에는 인용부호가 빠져 있어 앞선 28화:121(`"아렐이 중간에서 멈추자 토비아스가 소리 내 읽었다."`) 없이 보면 토비아스가 갑자기 반말을 쓰는 것처럼 읽힐 수 있습니다. — **수정안:** 한국어도 `TEXT("‘누가 먼저 찾을지 기다리지 마라.’")` 또는 `TEXT("「누가 먼저 찾을지 기다리지 마라.」")`로 인용 표시를 넣으면 즉시 명확해집니다.
2. **`Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaManuscriptScenes.cpp:1866` (`ms_leave_hanna`, 28화:79):** 영문 `"Her feelings board."` — 다른 문서(`TEAM-BOARD.md`)는 `emotion board`, 본문 타 행은 `board`를 사용합니다. 사소한 영문 어휘 통일 Nit입니다.
3. **`S395Hanna/Hanna.png` & `Leaving.png` (T-21 근접 외형 참고 Nit):** 극근접 샷에서 큐브 조합 모자의 평평한 윗면으로 흰 머리 정수리가 살짝 비쳐 나오고, 모자 띠 옆의 짙은 표찰 자국이 작은 검은 상자처럼 바깥으로 조금 돌출됩니다. 실제 게임 카메라 거리(`S396HannaFollows.png`, `S396Cistern.png`)에서는 모자 실루엣이 자연스럽게 읽히므로 비차단 참고 Nit입니다.
4. **`S396Title.png` & `S396HannaLeaving.png` (카메라·스폰 오프셋 참고 Nit):** 문 앞 초기 스폰 직후 아렐이 서쪽으로 걷기 전까지 토비아스·한나가 문 남쪽 벽 타일 안에 잠깐 겹쳐 보임(`S396_REVIEW.md:98` 기명시), 남문 상인방 아래를 지날 때 카메라 투영으로 아렐이 상인방 위에 겹쳐 보임(`S396HannaLeaving.png`, 16장 부스 투영과 동일). 모두 진행·충돌 무해.
