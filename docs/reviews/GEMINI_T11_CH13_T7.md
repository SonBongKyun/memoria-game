# T-11 원고·캐논 검수 보고서 — 13장(21화) & T-7 수정(`7f3662db`) 검증

- **작성:** Gemini (Antigravity)
- **기준 커밋:** `d98719d0` (`claude/s366-upgrade` 병합 후 `gemini/memoria-review` HEAD `06ff260b`)
- **대조 원고:**
  - `MEMORIA_KR_Ch021_final.md` (21화. 다시 베르단, 519행)
  - `MEMORIA_KR_Ch019_final.md`, `MEMORIA_KR_Ch020_final.md` (T-7 수정 대조용)
- **검수 대상 파일:**
  - `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaManuscriptScenes.cpp` (`ms_verdan_road`, `ms_verdan_farewell`, `ms_verdan_market`, `ms_verdan_sump`, `ms_verdan_vendor`, `ms_verdan_alley`, `ms_verdan_furrow` — 7개 씬)
  - `Unreal/Tools/export_chapter_maps.py` (`verdan_return`)
  - `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaManuscript.cpp` (S382 메인 퀘스트)
  - `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaNavigationGoal.cpp` (13장 목표)
  - `Unreal/Memoria/Source/Memoria/Private/Chapter/MemoriaChapterMap.cpp` (S382 한국어 테이블)
  - `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaNarrativeSubsystem.cpp` (`Vendor→상인`, 13장 플래그/토스트)
  - `docs/unreal-migration/S382_REVIEW.md`
  - 배경 2종(`chapter_splash_verdan_market.png`, `chapter_splash_belt_waystation.png`) 및 `7f3662db`(S383 T-7 수정) 전체 diff

---

## 요약

- **행 인덱스 안정성:** 아래 제안하는 모든 수정은 **기존 행을 제자리(in-place)에서 수정**하도록 설계했습니다(`field:ms_verdan_*:N` 인덱스 변동 없음, `MemoriaVerdanReturnS382Tests.cpp` 통과 유지). 행 추가가 필요한 항목은 없습니다.
- **Must-fix (1건):**
  1. `MemoriaManuscriptScenes.cpp:933` (`ms_verdan_furrow`, 화자 오류): `"지금은 좀 자."` / `"Get some sleep for now."`가 **`Elia` (`elia_neutral`)**로 잘못 배정되어 있습니다. 원고 21화:501~513에서 `"밝아지면 가 보자."`(501행)와 `"지금은 좀 자."`(507행)는 **둘 다 아렐의 대사**이며, 그 직후 엘리아가 눈을 감고 잠들고 아렐이 밤새 북서쪽을 향해 앉아 불침번을 섭니다(22화:27~31 연결). **`Arrel` (`arrel_neutral`)**로 제자리 수정해야 합니다.
- **Should-fix (1건):**
  2. `MemoriaManuscriptScenes.cpp:900` (`ms_verdan_alley`, 압축 시 중간 대사 탈락): 899행 아렐 `"누가 알게 될 것 같아?"` 바로 다음에 900행 엘리아가 `"누군지 모르겠어. ...모른다고 했어."` (`"I don't know who. ...I said I don't know."`)라고 한 호흡에 말합니다. 원고 21화:331의 아렐 대사 **`"그 사내?"`**가 생략되어 아무도 되묻지 않았는데 엘리아가 `"모른다고 했어."`라고 받아치는 어색함이 생깁니다. 행 추가 없이 900행 제자리에서 `"누군지 모르겠어. 그 사내인지 아닌지도."`로 정리하거나 899~900행에 `"그 사내?"`를 제자리 복원하십시오.
- **Nit (4건):** `ms_verdan_vendor:876` 영문 `"Went under."`(파산 어감 → `"Gone below."`/`"Taken below."`), `ms_verdan_furrow:931` 경미한 압축(`...같아.`), `S382_REVIEW.md:69` 인용 문구 미세 오기(`"그들은 들어온 길로..."` → `"들어온 길로..."`), `chapter_splash_belt_waystation.png` 3개 챕터 연속 재사용.
- **필수 캐논 검증 결과:**
  - **세이블 발화 범위:** `ms_verdan_road`(818, 820행) 및 `ms_verdan_farewell`(828~844행)에서만 발화하며, 돌아선 844행 이후 단 한 번도 등장·발화하지 않음(**Pass**). 모든 세이블 대사 시각 동사 0건(**Pass**).
  - **기름 노점 상인:** 화자명 `Vendor` → `상인`, 초상화 `nullptr`(무명·무초상화) 준수(**Pass**).
  - **말레트·토비아스 부재 / 성벽 없음 / 닫힌 책:** 모두 준수(**Pass**).
- **T-7 수정 커밋 (`7f3662db`) 검증:** T-7의 **Should-fix 4/4 및 텍스트 Nit 1/1 전부 제자리 반영 완료**(미반영 Must/Should-fix **0건**). 추가로 분지 대기열 정지/방향 Nit도 T-8(`f97fc008` + `a534caa1`)로 완전 해결됨.

---

## 13장 씬·파일별 상태 요약 (`verdan_return`, 21화)

| 대상 | 씬 순서 | 화자·캐논 | 텍스트/번역 | CG/배경 | 판정 |
|---|---|---|---|---|---|
| `ms_verdan_road` | No issues | No issues (세이블 맹인·성벽 없음·꼭대기 깃발) | **No issues** | — | **No issues** |
| `ms_verdan_farewell` | No issues | No issues (세이블 마지막 발화 844행, 이후 부재) | **No issues** | — | **No issues** |
| `ms_verdan_market` | No issues | No issues | **No issues** | SAFE (`chapter_splash_verdan_market.png`) | **No issues** |
| `ms_verdan_sump` | No issues | No issues (끈+붉은 밀랍 인장, 말레트 부재) | **No issues** | — | **No issues** |
| `ms_verdan_vendor` | No issues | No issues (`Vendor`→`상인`, 무명·무초상화) | 한국어 No issues / En Nit 1건 (`876`행) | — | En Nit only |
| `ms_verdan_alley` | No issues | No issues (갈색 코트 사내 무명 유지, 책 안 염) | Should-fix 1건 (`900`행 `"그 사내?"` 누락) | — | Should-fix |
| `ms_verdan_furrow` | No issues | **Must-fix 1건** (`933`행 화자 `Elia`→`Arrel`) | Nit 1건 (`931`행 경미 압축) | RISKY/허용 (`chapter_splash_belt_waystation.png`) | **Must-fix** |
| `export_chapter_maps.py` (`verdan_return`) | No issues | No issues (말레트/토비아스 없음, 주민 6명) | **No issues** (`elia_repeat`·단서·안내 완비) | SAFE (`verdan_market` 스플래시) | **No issues** |
| `MemoriaManuscript.cpp` (S382) | — | No issues | **No issues** | — | **No issues** |
| `MemoriaNavigationGoal.cpp` (Ch13) | No issues | No issues | **No issues** | — | **No issues** |
| `MemoriaChapterMap.cpp` (S382) | — | No issues | **No issues** | — | **No issues** |
| `MemoriaNarrativeSubsystem.cpp` (S382) | — | No issues (`Vendor→상인`, 플래그·토스트 2종) | **No issues** | — | **No issues** |
| `S382_REVIEW.md` | — | No issues | Nit 1건 (`69`행 인용문) | — | Nit only |

---

## 상세 지적 목록 (13장)

형식: `file:line — game text — manuscript (화, quoted sentence) — proposed fix`

### Must-Fix

1. `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaManuscriptScenes.cpp:933` — `Elia (elia_neutral): "지금은 좀 자."` / `En: "Get some sleep for now."` (932행 `Arrel: "밝아지면 가 보자."` 직후) — **21화 (501~513행)** `“밝아지면 가 보자.” 그가 말했다. / 엘리아가 고개를 들었다. / “지금은 좀 자.” / 그녀는 책을 넣으려다 그대로 두었다... 가방을 당겨 등에 대고 앉자... 엘리아는 눈을 감았다.` (및 519행 `아렐은 북서쪽을 향해 앉았다.`, 22화:27~31 엘리아 `“잤어?”` / 아렐 `“많이는 못 잤어.”`) — **수정안:** `"지금은 좀 자."`는 아렐이 엘리아에게 재워 주며 하는 말입니다(그 직후 엘리아가 눈을 감고, 아렐은 밤새 북서쪽을 향해 불침번을 섭니다). 행 추가·이동 없이 933행 제자리에서 화자와 초상화를 **`{TEXT("Arrel"), TEXT("arrel_neutral"), nullptr, TEXT("Get some sleep for now."), TEXT("지금은 좀 자.")}`**로 수정하십시오. *(참고: `MemoriaVerdanReturnS382Tests.cpp`는 933행의 화자를 특정 검사하지 않으므로 테스트 깨짐 없음.)*

### Should-Fix

2. `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaManuscriptScenes.cpp:900` — `Arrel (899): "누가 알게 될 것 같아?"` → `Elia (900): "누군지 모르겠어. ...모른다고 했어."` (`En: "I don't know who. ...I said I don't know."`) — **21화 (327~335행)** 아렐 `“누가 알게 될 것 같아?”` → 엘리아 `“누군지 모르겠어.”` → **아렐 `“그 사내?”`** → 엘리아 `“모른다고 했어.”` — **수정안:** 아렐이 `"그 사내?"`라고 되묻는 행이 압축되며 빠져, 엘리아가 아무 추궁 없이 같은 대사 안에서 `"누군지 모르겠어. ...모른다고 했어."`라고 화를 내는 부자연스러운 연결이 되었습니다. 행 인덱스를 유지하려면 아래 둘 중 하나로 제자리 수정하십시오:
   - **권장 (단일 행 수정, 900행):** Ko **`"누군지 모르겠어. 그 사내인지 아닌지도."`** / En **`"I don't know who. Not even whether it's that man."`**
   - **대안 (899~900행 동시 수정):** 899행 Arrel `"누가 알게 될 것 같아? 그 사내?"` (`"Someone would find out? That man?"`), 900행 Elia `"누군지 모르겠어. 그 사내인지도 몰라."` (`"I don't know who. I don't know if it's him either."`)

### Nit

3. `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaManuscriptScenes.cpp:876` — `Vendor (En): "Went under."` (Ko: `"밑으로 갔어."`) — **21화 (229~237행)** `“밑으로 갔어.”` / `“어디로.”` / `“파일됐어.”` — **수정안:** 영문 `"Went under."`는 상점이 `"망했다/파산했다"`는 관용구로 읽힐 수 있습니다. 876행 제자리에서 **`"Gone below."`** 또는 **`"Taken below."`**로 다듬으면 이어지는 `"Where?"` / `"Filed."`와 더 자연스럽게 연결됩니다.

4. `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaManuscriptScenes.cpp:931` — `Elia: "내가 가고 싶은 쪽으로 느끼는 건가 싶어서 다시 해 봤어. ...같아. 이건 확실해. 북서쪽."` — **21화 (489~497행)** 엘리아 `“...다시 해 봤어.”` → 아렐 `“같아?”` → 엘리아 `“응. 이건 확실해. 북서쪽.”` — **수정안:** 아렐의 `"같아?"`가 생략되면서 엘리아가 `"같아."`로 이어 받습니다. 의미는 통하므로 현행 유지 가능하며, 더 매끄럽게 다듬으려면 제자리에서 `"다시 해 봤어. ...똑같아. 이건 확실해. 북서쪽."` 정도가 자연스럽습니다.

5. `docs/unreal-migration/S382_REVIEW.md:69` — `A crowded market under "그들은 들어온 길로 나가지 않았다" was wrong` — **21화 (371행)** 및 `ms_verdan_furrow:908` `"들어온 길로 나가지 않았다."` — **수정안:** 원고·코드에는 `"그들은"`이 없으므로 인용문을 `"들어온 길로 나가지 않았다"`로 맞추는 편이 정확합니다.

6. `Unreal/Memoria/Source/Memoria/Private/Narrative/MemoriaManuscriptScenes.cpp:39-41,908` — `Pen` (`chapter_splash_belt_waystation.png`) — **21화 (401~519행)** 묵은 밭 고랑의 밤 — **배경 검수 판정 (`RISKY / 임시 허용`):**
   - `Market` (`chapter_splash_verdan_market.png`, `ms_verdan_market:849` 및 맵 스플래시): T-7에서 **SAFE**로 판정한 자산. 21화의 낮·텅 빈 시장보다 번화한 야간 톤이지만 명명된 인물·말레트·토비아스 없음. 도시 씬 배경으로 적합(**Pass**).
   - `Pen` (`chapter_splash_belt_waystation.png`, `ms_verdan_furrow:908`): 고랑 씬에서 시장 스플래시를 내리고 빈 황혼 길로 전환한 판단(`S382_REVIEW.md:69`)은 정확합니다. 인물이 없어 캐논 위반은 없으나, 11장(`roofless_farms`)·12장(`basin_station`/`ms_basin_pen`)·13장(`ms_verdan_furrow`)까지 3개 챕터 연속 재사용되고 있으며 포장된 석조 역참·푸른 등불이 묵은 밭 고랑과 이질적입니다. 향후 전용 야외 밭/황혼 배경 제작 시 우선 교체 권장.

---

## T-7 수정 커밋 (`7f3662db`) 검증 체크리스트

커밋 `7f3662db` (`fix(ch12): apply Gemini T-7 review of 19-20화 (S383)`) 전체 diff 및 연계 커밋(`a534caa1`)을 `docs/reviews/GEMINI_T7_CH12_S380.md` 7건과 대조했습니다.

| T-7 항목 | 등급 | `7f3662db` 반영 여부 | 확인 내용 |
|---|---|---|---|
| #1 `MemoriaManuscriptScenes.cpp:725` `That warden's drift` → `official` | Should-fix | **반영 완료** | `"That official's drift"`로 제자리 수정 (행 인덱스 불변) |
| #2 `MemoriaManuscriptScenes.cpp:766` Ko `"창구 아래의 평평한 돌 위에"` 보완 | Should-fix | **반영 완료** | `"지금은 창구에서 시선을 떼기가 어려웠다. 창구 아래의 평평한 돌 위에 알약을 놓고 누르면 부서질 것 같았다."` 제자리 반영 |
| #3 `MemoriaManuscriptScenes.cpp:785` En `something else` → `another one` | Should-fix | **반영 완료** | `"He might have asked for another one."` 제자리 반영 |
| #4 `S379_REVIEW.md:74` (+ `MemoriaManuscriptScenes.cpp:34`, `MemoriaNarrativeArtwork.cpp:92`) `open ledger` 오기 정정 | Should-fix | **반영 완료** | 3개 파일 모두 `"a dark archive desk with a candle, a brass compass and a closed book, nobody at it"` + placeholder 명시로 정정 |
| #5 `MemoriaManuscriptScenes.cpp:804` En `"Stop keeping watch and lie down too."` | Nit | **반영 완료** | 제자리 반영 |
| #6 `chapter_splash_belt_waystation.png` 재사용 | Nit | **미변경 (합리적 유지)** | 전용 아트 부재로 보류 |
| #7 분지 대기열 35→11+1명 & 보행/회전 | Nit | **T-8로 해결 완료** | `f97fc008` + `a534caa1`에서 `basin_station` 서기+11명 전원에 `'still': True, 'face': [...]` 적용 확인 |

### T-7 검증 결론
- T-7의 **Should-fix 4건과 텍스트 Nit 1건이 모두 `7f3662db`에 제자리 반영**되었으며(행 인덱스 변동 0건), 대기열 정지/방향 문제도 T-8(`a534caa1`)로 완전 해결되었습니다. 미반영 재상정 항목은 **0건**입니다.
