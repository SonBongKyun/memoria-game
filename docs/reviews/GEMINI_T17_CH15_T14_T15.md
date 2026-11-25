# T-17 원고·캐논 검수 보고서 — 15장(24화) · T-14 통합 · T-15 필드 외형 캡처 검증

- **작성:** Gemini (Antigravity)
- **기준 커밋:** `81ab6528` (`claude/s366-upgrade` 병합 후 `gemini/memoria-review` HEAD `007ee0b3`)
- **대조 원고:** `MEMORIA_KR_Ch024_final.md` (24화. 두 노트, 563행, 전편 완독)
- **검수 대상 파일 및 캡처:**
  - `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaManuscriptScenes.cpp` (`ms_wells_bread`, `ms_wells_pasture`, `ms_wells_dry`, `ms_wells_six`, `ms_wells_angles`, `ms_wells_blood`, `ms_wells_arcane` — 7개 씬, 175행)
  - `Unreal/Tools/export_chapter_maps.py` (`dry_wells`)
  - `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaManuscript.cpp` (S389 메인 퀘스트)
  - `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaNavigationGoal.cpp` (15장 목표 6단계)
  - `Unreal/Memoria/Source/Memoria/Private/Chapter/MemoriaChapterMap.cpp` (S389 한국어 테이블)
  - `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaNarrativeSubsystem.cpp` (`ch15_books_opened`, `ch15_six_originals`, `ch15_toward_arcane`, `party_tobias`)
  - `docs/unreal-migration/S389_REVIEW.md`
  - 커밋 `4bf71345` (S390, T-14 통합 — 11~13장 무화기 밤 검은 배경 전환 및 능선 농가 주석 수정)
  - 커밋 `acb39b24` (S388, T-15 — 은발 엘리아·지팡이 제거·토비아스 배지 밀착) 및 `81ab6528` (S391 — S361/UpgradeSmoke 무지팡이 테스트 정합)
  - **직접 육안 열람한 캡처 3종:**
    - `C:/Users/jc/orca/workspaces/Game/memoria-unreal-grok/Unreal/Memoria/Saved/Validation/S388Looks/Elia.png`
    - `C:/Users/jc/orca/workspaces/Game/memoria-unreal-grok/Unreal/Memoria/Saved/Validation/S388Looks/Walking.png`
    - `C:/Users/jc/orca/workspaces/Game/memoria-unreal-claude/Unreal/Memoria/Saved/Validation/S389Wells/S389Wells.png`

---

## 요약

- **15장 (`dry_wells`, 24화 「두 노트」, S389):** **Must-fix 0건, Should-fix 0건, Nit 2건.** 7개 씬 175행 전수 대조에서 대사·화자·존댓말·조용한 책 개방(무바람·무발광·잉크 번짐 없음)·여섯 줄과 여섯 각도·`"원본"`·코피·아렐 왼손·카이로스 부재까지 원고 캐논과 완벽하게 일치합니다.
- **T-14 통합 (`4bf71345`, S390) 검증:** **100% 반영 완료.** 11장(`roofless_farms`)·12장(`basin_station`) 스플래시 제거, `ms_basin_pen`·`ms_verdan_furrow` 검은 배경(`Dark`) 전환, `Pen` 상수 제거, 미라 기록소(`RegistryDesk`)·베르단 시장(`Market`) 유지, `ridge_farmhouse` 주석 정정, `S371` 테스트 및 `S377`/`S379`/`S382` 리뷰 노트 갱신 모두 이상 없음.
- **T-15 필드 외형 (`acb39b24`, `81ab6528`, S388/S391) 캡처 검증:** **Pass (경미한 Nit 2건).**
  - **엘리아 (`Elia.png`, `Walking.png`, `S389Wells.png`):** 은발/회백발 머리카락이 필드 카메라 거리에서 명확히 읽히고, **지팡이가 완전히 제거**되었으며 얼굴과 기존 복장이 보존됨.
  - **토비아스 (`Walking.png`, `S389Wells.png`):** 얇은 옅은 색 고해 인장 배지가 가슴에 **밀착(flush)** 부착되어 더 이상 허공에 뜨지 않으며, 등 가방이 깔끔하게 붙고 무기·등불 없이 엘리아 뒤를 따름.

---

## Part 1 — 15장 (`dry_wells`, 24화) 씬·파일별 검수

### 상태 요약표

| 대상 | 씬 순서 | 화자·캐논 | 텍스트/번역 | CG/아트 | 판정 |
|---|---|---|---|---|---|
| `ms_wells_bread` (1219~1238행) | No issues | No issues (작은 칼 허락·반납, 아렐 왼손 빵·검띠, 두 책 식음) | **No issues** | `Dark` (Pass) | **No issues** |
| `ms_wells_pasture` (1240~1251행) | No issues | No issues (아렐→엘리아→토비아스 행렬, 포플러, 아렐 발뒤꿈치, 3시간) | **No issues** | `Dark` (Pass) | **No issues** |
| `ms_wells_dry` (1254~1278행) | No issues | No issues (두 마른 우물, 왼손 자갈, 손바닥 셋 간격, **무바람·무발광 자동 개방**) | **No issues** | `Dark` (Pass) | **No issues** |
| `ms_wells_six` (1280~1299행) | No issues | No issues (잉크 번짐·펜 없음, 6곳 전부 묻힌 곳, 사본 보관 지하실 도시 미명시) | **No issues** | `Dark` (Pass) | **No issues** |
| `ms_wells_angles` (1301~1339행) | No issues | No issues (세로 기호·왼손바닥+오른검지, 6각도·거리 없음, **복제품 vs 원본**, 잠들어 듣고 있음) | **No issues** | `Dark` (Pass) | **No issues** |
| `ms_wells_blood` (1341~1365행) | No issues | No issues (엘리아 코피·스카프, 아렐 왼손 칼집, 두 책 모두 덮음, 물통) | **No issues** | `Dark` (Pass) | **No issues** |
| `ms_wells_arcane` (1368~1401행) | No issues | No issues (온기 능선 너머 기울어짐, 아르케인, 배지 1인분 고지, 왼손 검띠, 토비아스 선도) | **No issues** | `Dark` (Pass) | **No issues** |
| `export_chapter_maps.py` (`dry_wells`) | No issues | No issues (`seam_outskirts` → 목초지·포플러·`WELL` 2개, `lights: []`, `ambient_npcs: []`) | Nit 1건 (`title.subtitle`) | `splash: ''` (Pass) | Nit only |
| `MemoriaManuscript.cpp` (S389) | — | No issues (3단계 + 다음 목표 `"아르케인: 성문, 배지는 하나"`) | **No issues** | — | **No issues** |
| `MemoriaNavigationGoal.cpp` (Ch15) | No issues | No issues (6단계 모두 정확) | **No issues** | — | **No issues** |
| `MemoriaChapterMap.cpp` (S389) | — | No issues (`dry_wells`, `elia_repeat`, 토스트 `"여섯 원본"`, 단서 번역 정확) | Nit 1건 (플라크 중복 연동) | — | Nit only |
| `MemoriaNarrativeSubsystem.cpp` (S389) | — | No issues (`ch15_books_opened`, `ch15_six_originals` + 토스트, `ch15_toward_arcane`, `party_tobias`) | **No issues** | — | **No issues** |
| `S389_REVIEW.md` | — | No issues (라인 수 175행·목표·플래그 모두 사실 일치) | Nit 1건 (119행 T-15 이후 갱신) | — | Nit only |

### 필수 6대 캐논 점검 결과
1. **두 책의 조용한 개방 (무바람 · 무발광 · 무번짐):** `ms_wells_dry:1273~1275` (`"표지가 들렸다."`, 머리카락·코트 자락 정지, 삼분의 이 지점 정지) 및 `ms_wells_six:1281` (`"잉크가 번지는 테두리는 없었다. 펜도 없었다."`) 원고 그대로 반영. 175행 전체에 마법 발광·빛 묘사 및 CG 0건 (**Pass**).
2. **여섯 줄과 여섯 각도:** 토비아스 노트 6곳(베르단 남쪽 4마일 셋째 목자 제단 갈라진 돌 밑 · 마른 우물 밑 · 국청 도시 긴 돌 건물 바닥 밑 사본 보관 지하실 · 있었던 과수원 북서 물푸레 일곱 그루 숲 밑 + 2줄) 및 엘리아 책 세로 기호·가운데 기준 6각도·거리 없음 정확히 일치 (**Pass**).
3. **복제품과 `"원본"`:** `ms_wells_angles:1324~1333` — 벨트 6탑과 흰 도시 큰 탑 모두 복제품, 땅 밑 여섯은 `"원본"`, `"잠들어 있어."` / `"그런데 듣고 있어."` / `"서로를."` / `"지금은 우리 쪽도. 조금."` 및 토스트 `"여섯 원본"` 완벽 일치 (**Pass**).
4. **엘리아의 코피:** `ms_wells_blood:1341~1365` — 손가락 피·스카프 마른 모서리·`"내 쪽에서 내놓으면 알아차릴 것 같아."`·`"지금은 안 할 거야."`·왼손으로 책 덮음·물통·출혈 멈춤까지 누락 없음 (**Pass**).
5. **토비아스의 존댓말·태도:** `"이게 남은 겁니다."`, `"이것도 먹어요."`, `"집에서는 더 잘 잘렸습니다."`, `"아까 한 말은 책을 열어 달라는 뜻은 아니었습니다."`, `"혼자 펴진 건 처음입니다."`, `"이 배지는 제 겁니다. 두 사람 몫은 아니에요."`, `"이쪽입니다."` 등 전 대사 존댓말·태도 완벽 일치 (**Pass**).
6. **아렐의 왼손 & 카이로스 부재:** 1227행(왼손 빵)·1233행(오른손 멈춤→왼손 검띠)·1255행(왼손 자갈)·1355행(왼손 칼집)·1397행(왼손 검띠) 전부 반영. 카이로스 등장·언급 0건 (**Pass**).

### 상세 지적 목록 (15장)

#### Must-Fix / Should-Fix
없음.

#### Nit
1. `Unreal/Tools/export_chapter_maps.py:326` & `S389Wells.png` 좌상단 지역 플라크 — `title: {'number': 15, 'name': 'Two Notes', 'subtitle': 'The Dry Wells'}` — **`S389Wells.png` 좌상단 헤더에 `"마른 우물"`이 위아래 두 줄(`마른 우물` / `마른 우물`)로 중복 출력됨** (`dry_wells`와 `The Dry Wells`가 모두 `MemoriaChapterMap.cpp:171`에서 `"마른 우물"`로 매핑되기 때문; 14장은 `능선 아래 농가` / `세 번째 사람`, 13장은 `다시 베르단` / `파일됨`). — **수정안:** 추후 정리 패스에서 `subtitle`을 `'Two Notes'`(표시: `마른 우물` / `두 노트`)로 바꾸면 중복이 사라지고 화 제목(「두 노트」)도 HUD에 살아납니다.
2. `docs/unreal-migration/S389_REVIEW.md:119` — `"Elia's field figure is still blond and carries a staff. Grok's T-15 fixes that (deferred to 2026-11-25)."` — 커밋 당시 사실이었으나, 현재 통합선(`81ab6528`)에서는 T-15가 이미 반영되었고 `S389Wells.png` 자체도 은발·무지팡이 엘리아로 재캡처되어 있습니다. 추후 문서 손볼 일이 있을 때 1줄 메모만 덧붙이면 최신 상태와 일치합니다.

---

## Part 2 — T-14 통합 (`4bf71345`, S390) 검증

| T-14 권고 항목 | `4bf71345` 반영 여부 | 확인 위치 |
|---|---|---|
| 11장 `roofless_farms` 스플래시 제거 (무화기 밤 검은 배경) | **완료** | `export_chapter_maps.py:265` (`splash: ''`), `roofless_farms.v1.json`, `S377_REVIEW.md:93~95` |
| 12장 `basin_station` 스플래시 제거 + `ms_basin_pen` `Dark` 전환 (미라 기록소 유지) | **완료** | `export_chapter_maps.py:304` (`splash: ''`), `MemoriaManuscriptScenes.cpp:791` (`Dark`), `RegistryDesk` 유지, `S379_REVIEW.md:111~114` |
| 13장 `ms_verdan_furrow` `Dark` 전환 (시장 스플래시 유지) | **완료** | `MemoriaManuscriptScenes.cpp:915` (`Dark`), `Market` 유지, `S382_REVIEW.md:102~104` |
| `Pen` (`chapter_splash_belt_waystation.png`) 상수 제거 + `S371` 테스트 허용 확장 | **완료** | `MemoriaManuscriptScenes.cpp:37~40`, `MemoriaSeamS371Tests.cpp:73` |
| 14장 `ridge_farmhouse` 주석 정정 (T-14 Nit #1) | **완료** | `export_chapter_maps.py:366` (`"In the dark Tobias comes down the slope; at first light he stays."`) |

**판정:** T-14 반영 누락 **0건** (**Pass**).

---

## Part 3 — T-15 필드 외형 (`acb39b24`, `81ab6528`) 육안 캡처 검증

직접 열람한 3장(`Elia.png`, `Walking.png`, `S389Wells.png`) 및 `MemoriaFieldCharacterComponent.cpp` / `MemoriaHeroS361Tests.cpp` / `MemoriaUpgradeSmokeSubsystem.cpp` 대조 결과입니다.

| 검증 항목 | 캡처 육안 확인 결과 | 캐논 판정 |
|---|---|---|
| **엘리아 은발 (`Elia.png`, `Walking.png`, `S389Wells.png`)** | 금발이 완전히 사라지고 명확한 **은색/회백색 단발**로 렌더됨. 얼굴·눈·망토·흰 치마 색상은 그대로 유지됨. 특히 실제 게임 카메라 거리(`Walking.png`, `S389Wells.png`)에서 아렐·토비아스 사이에서 은발 엘리아가 한눈에 식별됨. | **Pass** |
| **엘리아 지팡이 제거 (`Elia.png`, `Walking.png`, `S389Wells.png`, `81ab6528`)** | 3장 모두에서 왼손·오른손에 지팡이 프롭 없음. `InitializeRigged`의 `staff` 부착 제거 + `S361`/`UpgradeSmoke`에서 엘리아 무소품(`PropCount == 0`, `BoundEliaStaffs == 0`) 강제 검증 확인. | **Pass** |
| **토비아스 가슴 배지 밀착 (`Walking.png`)** | T-12 당시 허공에 떠 보이던 흰 상자가 얇은 옅은 상아빛 직사각 인장판(`scale X=0.012`, CPU 스킨드 정점 전방 탐색 기반 `TobiasBadgeOffset`)으로 바뀌어 **코트 앞가슴에 깔끔하게 밀착**됨. | **Pass** |
| **토비아스 가방·코트·무무기 (`Walking.png`, `S389Wells.png`)** | 등 뒤 사각 가방이 자연스럽게 부착되어 있고 무기·등불·책 노출 없음. `S389Wells.png` 낮 조명에서 어두운 회갈색 코트와 등 가방, 3인 종대(아렐 → 엘리아 → 토비아스)가 원고 24화:63 그대로 읽힘. | **Pass** |
| **15장 마른 우물 배경 (`S389Wells.png`)** | 두레박 틀 없는 네모 돌 우물 2개(안쪽 어둠), 가운데 넓은 평평한 돌, 좌상단 윗부분 잘린 포플러와 가는 가지, 등불 없는 마른 낮 목초지가 원고 24화 묘사와 매우 잘 어울림. | **Pass** |

#### T-15 경미한 시각 Nit (수정 필수 아님, T-18 작업 시 참고)
1. **`Elia.png` 근접 측면 샷의 어깨 회색 패치:** `EliaHair()`(`MemoriaFieldCharacterComponent.cpp:230`)가 정점 알파 구간 `.10~.35` 전체를 은색(`0.80, 0.82, 0.84`)으로 칠하면서, 도너 메시(`SK_Elia`)의 오른 어깨 덮개 작은 영역도 같은 알파 대역에 걸려 근접 측면 샷(`Elia.png`)에서 회색 패치로 보입니다. 필드 카메라(`Walking.png`, `S389Wells.png`)에서는 거의 눈에 띄지 않으므로 **Pass**이며, 추후 T-18에서 머티리얼 에셋화할 때 참고만 하시면 됩니다.
2. **`Walking.png` 저조도에서의 토비아스 코트 명도:** `S389Wells.png`의 주간 조명에서는 코트가 원고대로 어두운 회갈색으로 잘 읽히나, `Walking.png`의 어두운 조명 아래에서는 코트가 거의 검은색에 가깝게 보입니다(가슴 배지 대비는 오히려 선명함). 패키징 머티리얼(T-18)로 옮길 때 현재 톤을 유지해도 무방하나, 아주 살짝만 밝기를 올려도 회갈색 질감이 저조도에서 더 살아납니다.
