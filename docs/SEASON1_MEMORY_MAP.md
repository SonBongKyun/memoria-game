# MEMORIA Season 1 Memory Map

기준일: 2026-08-24

이 문서는 Memory World Engine의 서사 소스 맵이다. 원고를 복제하지 않고, 게임 상태로 분리할 수 있는 `knowledge`와 `memory`, 그리고 이후 파급 지점만 기록한다.

게임 progression과 migration 순서는 [Season 1 Canon Game Progression](SEASON1_GAME_PROGRESSION.md)에서 관리한다.

## 1. Narrative source of truth

우선순위는 다음과 같다.

1. `../원고/시즌1/Chapter 01 - Ash.txt`부터 `Chapter 46 - Residue.txt`까지의 영문 원고
2. 현재 게임의 `data/`, `scenes/`, dialogue JSON, VN JSON, progression
3. GDD, 세션 로그, 이전 분석

2026-08-24 재확인 결과 영문 Season 1 원고는 1~46장이 번호 누락 없이 모두 존재한다. Chapter 39의 실제 파일명과 본문 제목은 `The Registrar's Hand`이며 현재 파일명에 `DRAFT` 표시는 없다.

한국어 Season 1 파일도 01~46이 존재하지만, 시스템 ID와 사건 판정은 완성 영문 46장을 기준으로 한다.

## 2. 비교 라벨

- **A**: 최신 원고의 사건, 인물, 인과가 현재 게임과 대체로 일치
- **B**: 최신 원고에는 있으나 게임에 아직 구현되지 않음
- **C**: 이전 원고의 사건 또는 설정이 남아 있음
- **D**: 최신 원고와 직접 충돌
- **E**: 핵심 사건은 있으나 chapter 위치가 이동하거나 여러 장으로 압축됨

복수 라벨은 한 구현 안에 정사 요소와 구버전 요소가 섞였다는 뜻이다.

## 3. Chapter 1~46 canon 및 구현 gap

| Ch | 제목 / 주요 인물 | 정사 사건, 기억, 지식과 출처, 이후 참조 | 현재 게임 |
|---:|---|---|---|
| 1 | **Ash** / Arrel, Elia | Arrel이 어린 시절 기억을 태워 Void Beast를 죽인다. Elia를 향해 움직이는 손과 함께 부른 음은 명시 기억보다 오래 남는다. 녹색 나무와 연소 흔적은 2, 5장 이후의 관측 근거가 된다. | **A**. 전투·연소·Elia·녹색 나무의 핵심이 구현됨. |
| 2 | **Rules of Deal** / Arrel, Elia, Malet | Arrel이 BL-07 경로를 묻고 첫 검의 기원 기억을 Malet에게 판다. Malet은 요청자와 거래를 직접 목격하며 Kairós에게 보고한다. 5, 21장에서 회수된다. | **A/E**. 거래는 정사에 가깝지만 후속 재방문 시점은 압축됨. |
| 3 | **Weight of Pages** / Arrel, Elia | 읽기 능력의 틈, 버려진 역참, 쓰이지 않은 책, Editor의 문장이 처음 결합한다. 책의 정체와 기능은 17~18, 22장으로 이어진다. | **D**. 게임 Ch3에는 아직 만나지 않은 Tobias와 후대 사건이 앞당겨져 있음. |
| 4 | **Drift** / Arrel, Elia | 색·단어·어린 시절이 흔들리고 Elia가 자기 몸을 대가로 anchoring한다. Arrel은 이름을 묻는 순간 주변 기억까지 잃는다. 11~15, 41장에 누적된다. | **A/D**. drift와 anchoring은 있으나 Tobias 동행 등 인물 시점이 충돌. |
| 5 | **The Classifier** / Kairós | Kairós가 연소 자국과 Elia의 anchor를 분류한다. 공식 검은 수첩과 사적 붉은 수첩, Malet 보고가 이후 8, 26, 31, 33, 45장의 축이 된다. | **A**. Ch4 뒤 Kairós POV VN으로 구현. Malet 보고의 provenance는 persistent WorldState로 한 번 고정됨. |
| 6 | **The Seam** / Arrel, Elia, Sable, Haren | 살아 있는 색의 Seam, 늙고 눈먼 Sable, Arrel의 남은 시간을 묻는 Elia. Sable의 지도와 인물상은 7, 15~16장으로 이어진다. | **A/D**. 장소와 만남은 있으나 party/briefing 성격과 일부 Sable 설정은 구버전. |
| 7 | **Sable** / Sable, Arrel, Elia | Sable은 BL-07을 찾아간 17명이 한 명도 돌아오지 않았음을 안다. 그중 Vessa가 자신의 딸이라는 개인 기억을 가진다. 17개 나무껍질과 아직 구멍을 태우지 않은 Arrel의 18번째 조각이 핵심이다. Vessa라는 이름의 직접 언급은 이 장에 집중된다. | **D → 부분 A**. Cleaner·아이 삭제 명령·12인 추모비는 삭제했고 Vessa/17 상태를 라이브 Memory gameplay로 교정. trial/outskirts 구조는 여전히 구버전. |
| 8 | **The Listening Wood** / Arrel, Elia, Sable, Kairós | 숲이 친숙한 목소리로 유인하고 다섯 번째 시간을 빼앗는다. Sable의 규칙과 Kairós의 추적이 9~10장으로 이어진다. | **C/D**. 망각 숲은 있으나 유령·Tobias 이론이 섞이고 실제 사건 순서가 다름. |
| 9 | **Borrowed Mouth** / Arrel, Elia | 숲의 목소리가 누구의 입에서 왔는지 불확실해지고 Echo Shell이 불일치를 잡는다. 10, 14, 38장 callback의 근거다. | **D**. 게임 Ch9는 Colorless Waste와 Kairós 전투. |
| 10 | **The Price** / Arrel, Elia | Elia는 문구와 자장가를 알지만 누가 말했는지는 모른다. Arrel은 돌아온 이름 파편을 숨긴다. knowledge와 source memory 분리의 첫 강한 사례다. | **D**. 게임 Ch10은 BL-07 core와 Part 1 결말로 36장을 앞당김. |
| 11 | **What Fear Used To Do** / Arrel, Elia | Arrel은 Elia를 잃는 공포가 사라졌다는 사실을 발견한 채 BL-07 문턱에 선다. | **B**. 독립 장면 없음. |
| 12 | **The Editor** / Arrel, Elia, Kairós | Kairós가 직접 기억 목록을 읽고 Elia의 손과 사건 흔적을 수정한다. 남은 칼자루 흠집은 13장 이후의 증거다. | **C/D**. 현 VN의 Editor 대면은 더 늦은 사건과 결합되어 정사 장면과 다름. |
| 13 | **Arithmetic** / Arrel, Elia, Haren | 몸의 buckle 반복, 출처 없는 기하학적 지식, Elia 목소리를 쓰는 책이 모두 '산수'라는 해석으로 모인다. | **B**. 현 `ch13_third_person`은 최신 23장 사건. |
| 14 | **The Lullaby Returns** / Arrel, Elia | Arrel의 여섯 음과 Elia의 일곱 음이 갈라진 근원을 드러낸다. 음을 안다는 fact와 누가 가르쳤는지의 memory가 분리된다. 15, 30장에 회수된다. | **B**. 일부 음악 motif만 구버전 VN에 존재. |
| 15 | **What Hands Remember** / Sable, Arrel, Elia, Haren | Sable은 여섯/일곱 음이 두 아이에게 나뉘었다고 알아본다. 이름을 말하지 않고, Arrel 전용 무기의 존재가 드러난다. | **B**. 현 `ch15_singer`는 최신 28~30장을 압축. |
| 16 | **Departure** / Arrel, Elia, Sable | Sable이 일행을 checkpoint 밖으로 인도한다. Arrel에게 사라졌던 분노와 전략적 선택이 돌아온다. | **E/C**. 현 VN Ch11 제목은 Departure지만 내용과 순서가 구버전. |
| 17 | **The Reader** / Elia, Arrel, Sable | Elia가 책의 relay 절차와 'body is what relay costs'를 읽고 비용을 숨긴다. 18, 41장의 핵심 source memory. | **C/E**. 현 VN Ch12가 Reader라는 이름을 쓰지만 Verdan/Pell 사건을 결합. |
| 18 | **Pell's Hand** / Elia, Arrel | 책이 Pell을 쓰고 Elia의 손바닥이 필체를 알아본다. 사실상 source recognition은 21~22, 41장에 이어진다. | **E**. Pell 이름과 필체 단서가 현 VN Ch12에 압축됨. |
| 19 | **Mira** / Mira | Mira가 Kairós 보고의 각도 오차를 관측하고 수정 대신 사적 기록을 남긴다. 40장의 선택으로 회수된다. | **B/C**. 이름과 오차 motif는 현 VN Ch16에 짧게 있으나 독립 사건이 아님. |
| 20 | **The Gray Cage** / Arrel, Elia, Sable | Authority 대기열과 그릇, checkpoint를 Sable의 통제로 통과한다. | **B**. |
| 21 | **Verdan Again** / Arrel, Elia, Malet(기록) | Sump가 봉인되고 Malet은 Editor에게 끌려갔다. Kairós는 첫 검 판매와 지도 구매를 포함한 Malet의 Arrel 기록을 가진다. Ch2 거래의 정식 downstream이다. | **E**. 현 VN Ch12 도입에 Sump 봉인·사라진 Malet은 있으나 전체 기록 payoff와 WorldState 조건은 미구현. |
| 22 | **Resonance** / Arrel, Elia | 책이 이름뿐 아니라 장소를 가리키는 channel임을 배운다. | **E/C**. 현 VN Ch12 후반과 부분 중첩. |
| 23 | **The Third Person** / Tobias, Arrel, Elia | Tobias와 두 번째 unstamped book, 지워진 사람 목록이 등장한다. Selene/Ami가 24장으로 이어진다. | **E**. 현 VN Ch13에 압축. |
| 24 | **Two Notebooks** / Tobias, Elia, Arrel | 두 책이 공명하고 한쪽의 삭제 목록과 다른 쪽의 역방향 기록이 연결된다. | **E**. 현 VN Ch13에 함께 압축. |
| 25 | **The Confessor's Hall** / Arrel, Elia, Tobias | Arrel이 임무보다 한 사람을 살리기 위해 따뜻한 기억을 쓴다. 26, 33장의 Kairós 판단 근거다. | **E/C**. 현 VN Ch14에 비슷한 개입이 있으나 세부 인과가 다름. |
| 26 | **Kairós Hesitates** / Kairós | Kairós는 연소 사실뿐 아니라 그것이 탈출이 아닌 구조에 쓰였음을 기억하고 추적을 유보한다. | **C/E**. 일부 태도는 현 후반 VN에 있으나 독립 관점과 원인은 미구현. |
| 27 | **The Weight of Quiet** / Arrel 일행 | 지하 은신처, 따뜻해지는 책, 노래 연락망의 문턱. | **B**. |
| 28 | **The Singer** / Hannah, Tobias, Elia | 말 대신 석판을 쓰는 Hannah가 노래로 책들에게 인식된다. | **E/C**. 현 VN Ch15의 Han은 이 사건을 압축·변형. |
| 29 | **Underground Songs** / Hannah, Tobias, Elia | 지하망의 이름·빚·얼굴 검증과 두 번째 노래. | **E/C**. 현 VN Ch15에 일부 motif만 존재. |
| 30 | **The Mother's Lullaby** / Elia, Hannah, Tobias, Arrel | Hannah의 일곱 음이 Elia의 일곱 음과 합쳐지고 `Sleep small`과 지워진 source name이 돌아온다. 원고는 그 이름을 독자에게도 밝히지 않는다. | **C/E**. 현 VN Ch15은 노래를 쓰지만 이 source/인물 관계를 다르게 처리. |
| 31 | **The Old Editor** / Telos, Kairós(기록) | Telos가 25년 만에 서랍을 열고 Kairós의 어긋남을 보고도 수정하지 않는다. | **B**. |
| 32 | **The Same Next Thing** / Arrel, Elia, Tobias, Hannah | 이동 수레에서 한 사람씩 같은 다음 일을 선택하며 집단이 굳어진다. | **B**. |
| 33 | **Decides, Again, Not To** / Kairós | Kairós가 여러 trace를 redirect하지 않기로 다시 선택한다. 26장의 기억과 45장의 자기 기록으로 이어진다. | **B/C**. 현 VN Ch21의 유보된 기록 motif와는 유사하지만 사건 배치가 다름. |
| 34 | **The Driver Turns** / Driver, Arrel 일행 | 운전자가 명령된 길 대신 일행을 선택한다. 이후 신호와 46장의 동행으로 회수된다. | **B**. |
| 35 | **A Face He Doesn't Know** / Arrel | Arrel의 몸이 이름 없는 얼굴을 먼저 알아본다. 사실 인식과 person memory가 분리된다. | **B**. |
| 36 | **Nera** / Nera | Nera가 세 번째 unnamed fragment를 발견하고 즉시 filing하지 않고 보류한다. 39, 44장으로 이어진다. | **E/C**. 현 VN Ch16에 Nera가 있으나 사건은 구버전. |
| 37 | **The Forgetting Storm** / Arrel 일행, Driver | 망각 폭풍과 운전자의 신호, 책을 통한 집단 유지. | **E/C**. 현 VN Ch17에 폭풍이 있으나 이후 living funeral로 갈라져 정사와 다름. |
| 38 | **The Echo Shell Vibrates** / Arrel, Elia | 수개월 잠잠했던 Echo Shell이 먼 source를 향해 깨어나고 Elia가 형태를 알아본다. | **B**. |
| 39 | **The Registrar's Hand** / Vael, Nera | Vael이 이른 보고를 사적으로 보류·이동하고 Nera와 연결한다. 파일명이 `DRAFT`라 문구 위험도는 높다. | **B**. |
| 40 | **The White Country** / Arrel 일행, Mira | Lumea 도착, aqueduct 침투, relay와 counterpart의 상호 인식. Mira의 19장 선택이 회수된다. | **E/C**. 현 VN Ch19~20의 Monolith 접근이 다른 구조로 압축. |
| 41 | **The Sea Inside** / Arrel, Elia | 보존된 memory sea. Elia는 relay의 body cost를 17장부터 알고 숨겼다고 고백하고, 둘은 완전한 정보 공유를 선택한다. | **E/C**. 현 VN Ch22에 relay 선택은 있으나 숨긴 source memory의 전체 arc가 없음. |
| 42 | **The Chair** / Arrel, Elia, Grand Archivist | 창립자의 빈 껍질과 `preserve`가 `prove`로 바뀐 교리의 차이가 드러난다. | **E/C**. 현 Monolith/core 장면과 일부 역할이 겹치나 인물·교리가 다름. |
| 43 | **The Price of a Name** / Arrel, Elia, Velor, Kairós | Arrel이 Elia가 준 자신의 이름을 태워 relay를 연다. 기능은 남고 이름과 self-source가 사라진다. | **E/D**. 현 VN Ch23에도 이름 연소 선택이 있지만 선택 가능 결말로 바뀌어 정사 필연성과 충돌. |
| 44 | **Conversion** / Arrel 일행, Nera, Velor | memory sea가 도시에 풀리고 소유자 없는 기억이 돌아간다. Nera의 관측과 빈 의자가 36장의 보류를 회수한다. | **E/C**. 현 VN Ch23에 conversion이 있으나 다중 엔딩 구조로 변형. |
| 45 | **The Cell Below** / Kairós, Vael | Kairós가 자기 파일을 쓰고 Vael의 열쇠로 풀려난다. '사람으로 이루어졌음을 잊은 기억'이라는 시스템 인식과 여섯 relay가 열린다. | **B**. |
| 46 | **Residue** / nameless Arrel, Elia, Tobias, Driver | 이름 없는 Arrel은 목록과 수를 읽는 몸의 습관을 유지한다. Elia는 원하면 새 이름을 주겠다고 한다. | **C/D**. 현 Epilogue/Testimony가 motif를 사용하지만 정사 결말 뒤 여러 분기와 Sable 사망/ledger 설정을 추가. |

### 구조적 결론

현재 게임은 최신 46장을 순서대로 구현한 빌드가 아니다. 탐색형 Ch1~10 뒤에 별도의 24장 VN arc가 붙은 이전 압축 구조다. 특히 Tobias의 Ch3 조기 합류, Ch10의 BL-07 결말, Ch15의 Singer, Ch16의 Nera, Ch23의 Conversion은 최신 사건을 10~20장 이상 앞당긴다. 따라서 새 Memory gameplay는 **최신 원고 chapter를 기준으로 설계**하고, 현재 게임의 장 번호는 임시 배치 위치로만 취급해야 한다.

## 4. 현재 Memory gameplay canon 판정

### Malet

- `npc.malet`: 정사 인물과 일치.
- `fact.bl07.route_request_received`: Ch2에서 누군가 BL-07 경로를 요청했다는 identity-free 사건 knowledge. 정사 canonical ID.
- `fact.arrel.seeks_bl07`: 이전 구현이 요청 사건과 Arrel identity를 한 ID에 묶었던 legacy ID. 기존 save에서 삭제하지 않고 import 시 canonical fact를 덧붙인다.
- `memory.malet.bl07_request_source`: 요청자가 Arrel이었다는 거래 출처 기억으로 정사.
- Chapter 2 거래와 active/removed/restored 재대화: 게임화된 선택으로 허용 가능.
- Chapter 3 이후 Verdan 재방문: 최신 원고의 실제 회수 시점은 **Ch21**이므로 **E**.
- Malet 쪽지를 Sable이 읽는 Chapter 6 ripple: 최신 원고 근거가 없어 **D**. 라이브 연결을 제거했다.
- 정식 장기 ripple은 `Ch2 거래 → Ch5 Kairós의 Malet 보고 → Ch21 Kairós가 확보한 Malet 기록`이다. Ch5는 보고 당시 상태를 `fact.kairos.malet_report_identified_arrel` 또는 `fact.kairos.malet_report_requester_unknown`으로 정확히 한 번 고정한다. 현 게임 Ch12 VN에 Sump 봉인과 Malet 실종 setup은 있으나 Ch21 consumer는 아직 구현하지 않는다.

### Sable

- `npc.sable`: ID는 유지할 수 있다. 최신 인물은 태어날 때 이름이 Halda였던 늙고 눈먼 여성이다.
- `fact.authority.child_memory_erasure_order`, `memory.sable.child_memory_erasure_target`: 최신 46장에 사건이 없어 **D**.
- Cleaner 과거, 아이와 부모 삭제 명령, 12인 추모비, Malet과 조합: 모두 삭제 또는 대체 대상이며 라이브 Memory gameplay에서 제거했다.
- 대체 fact: `fact.bl07.seventeen_seekers_never_returned`.
- 대체 memory: `memory.sable.vessa_daughter_bond`.
- 분리 의미: Sable은 열일곱 명이 BL-07을 찾아갔다가 돌아오지 않았다는 사실과 `Vessa`라는 기록을 유지하면서도, Vessa가 자기 딸이었다는 개인적 관계만 잃을 수 있다.
- 복원은 기존 tombstone 계약대로 제거 직전의 관계 record를 되살린다.

## 5. 플레이어 조작 후보 16개

아래 ID는 **후보 계약**이다. 실제 구현 전에는 ActorRegistry나 WorldState에 seed하지 않는다. `Main`은 핵심 진행에 영향을 줄 잠재력이 있어 높은 보호가 필요하고, `Optional`은 질문·정보·반응·보조 경로에 한정할 수 있다는 뜻이다.

| 우선 | Memory ID 후보 / Actor / 최초 Ch | 남는 Fact와 source | 개인 Memory | remove → restore | 이후 참조 / 영향 / consequence | Ripple / 위험 |
|---:|---|---|---|---|---|---|
| 1 | `memory.elia.lullaby_source_name` / `npc.elia` / 14 | `fact.relay.seven_note_lullaby`는 Elia 자신의 몸과 Hannah의 노래가 증명 | 일곱 음을 누가 가르쳤는지와 `Sleep small`에 붙은 source | 음과 기능은 남고 가르친 사람만 사라짐 → 이름·관계가 돌아옴 | 15, 30, 41 / Elia, Hannah, Sable / 질문·고백의 깊이, relay 정보 신뢰 | **Very High / Medium** |
| 2 | `memory.malet.bl07_request_source` / `npc.malet` / 2 | `fact.bl07.route_request_received`; BL-07 경로 요청 사건 | 요청자가 Arrel인 거래 기억 | 요청 사실·경로는 남고 요청자만 사라짐 → provenance 복귀 | 5, 21 / Malet, Kairós, Arrel / Ch21 기록의 증거 강도 | **High / Low-Med**; Ch5 consumer 구현 |
| 3 | `memory.kairos.arrel_selfless_burn` / `npc.kairos` / 25 | `fact.arrel.burn_detected`; 분류 계기판 | Arrel이 도주가 아니라 타인을 구하려 기억을 썼다는 목격 해석 | burn 수치는 남고 동기만 사라짐 → hesitation의 인간적 원인 복귀 | 26, 33, 45 / Kairós, Vael / 추적 유보·사적 기록 반응 | **Very High / High** |
| 4 | `memory.elia.relay_body_cost_warning` / `npc.elia` / 17 | `fact.relay.requires_body_cost`; 책의 절차 | 비용을 읽고 Arrel에게 숨기기로 한 순간 | 규칙은 알지만 언제·왜 숨겼는지 상실 → 41장 고백 복구 | 18, 41~43 / Elia, Arrel / 신뢰 대화·선택 문맥 | **Very High / High** |
| 5 | `memory.arrel.name_given_by_elia` / `player.arrel` / 10 이전, 43 확정 | `fact.arrel.current_name_is_arrel`; 일행의 호칭 | Elia가 그 이름을 준 관계와 origin | 이름을 기능적으로 쓰되 giver/source를 잃음 → 관계 의미 복귀 | 10, 14~15, 43, 46 / Arrel, Elia / 이름 연소와 새 이름 결말 | **Very High / Very High** |
| 6 | `memory.sable.vessa_daughter_bond` / `npc.sable` / 7 | `fact.bl07.seventeen_seekers_never_returned`; bark box와 Sable 증언 | Vessa가 자신의 딸이며 짐을 꾸려 준 개인 기억 | 17명·Vessa 이름은 남고 관계만 사라짐 → 딸과 deepest groove의 이유 복귀 | 7; 주제 callback 15, 20~21 / Sable / 개인 질문과 기록 해석 | **Medium / Low**; 현재 구현 |
| 7 | `memory.elia.pell_hand_recognition` / `npc.elia` / 18 | `fact.relay.book_has_guidance`; 책과 손바닥 반응 | Pell의 필체를 몸이 알아본 순간 | 절차는 읽되 writer provenance가 사라짐 → Pell 연결 복귀 | 21~22, 41 / Elia, Tobias / 장소·책 source 질문 | **High / Medium** |
| 8 | `memory.arrel.first_sword_teacher` / `player.arrel` / 2 | `fact.arrel.body_knows_sword`; 몸과 칼자루 | 첫 검을 쥐여 준 손과 기원 | 기술은 남고 가르친 사람·처음이 사라짐 → Ch21 기록과 origin 복귀 | 5, 21, 30 / Arrel, Malet, Kairós / 전투 독백·기록 대조 | **High / High** |
| 9 | `memory.arrel.editor_chair_encounter` / `player.arrel` / 12 | `fact.editor.altered_elia`; 흠집과 신체 이상 | Kairós chair에서 실제로 겪은 절차 | 후유증은 알지만 가해 장면/source 상실 → 사건 순서 복구 | 13~15, 26 / Arrel, Elia, Kairós / 선택 질문·증거 조합 | **High / Medium** |
| 10 | `memory.hannah.lullaby_recipient_name` / `npc.hannah` / 30 | `fact.relay.seven_note_lullaby`; Hannah의 노래와 slate | 원고가 독자에게 숨긴 이름과 그 이름의 의미 | 노래는 남고 recipient/source 관계만 사라짐 → 공개 가능한 시점에만 복귀 | 30, 32, 38, 46 / Hannah, Elia, Tobias / optional reveal | **High / High**; 원고가 이름을 숨겨 구현 보류 |
| 11 | `memory.mira.kairos_report_misalignment` / `npc.mira` / 19 | `fact.kairos.report_is_misaligned`; 계측값 | 오차를 고치지 않고 따로 적기로 한 관측 순간 | 수치는 남고 선택의 주체성만 약화 → 40장 인정 복귀 | 40 / Mira, Lumea relay / 접근 도움·보고 해석 | **Medium / Low-Med** |
| 12 | `memory.tobias.selene_ami_entry` / `npc.tobias` / 23 | `fact.archive.contains_erased_people`; unstamped book | Selene와 Ami를 한 mother/daughter entry로 읽은 개인 충격 | 목록은 남고 두 이름의 관계가 흐려짐 → 관계 독해 복귀 | 24, 27~30 / Tobias, Elia / 추가 목록 설명 | **Medium / Low** |
| 13 | `memory.driver.chose_the_group` / `npc.driver` / 34 | `fact.driver.changed_route`; 실제 주행과 신호 | 명령 대신 일행을 선택한 순간 | 경로 변경은 남고 자신이 선택했다는 소유감 상실 → 동행 의미 복귀 | 35, 37, 46 / Driver, Arrel 일행 / 신호·보조 탈출 반응 | **Medium / Medium** |
| 14 | `memory.nera.held_unnamed_fragment` / `npc.nera` / 36 | `fact.registry.has_third_unnamed_fragment`; 문서 | 즉시 filing하지 않고 보류한 자기 선택 | fragment는 알지만 자신이 유예했다는 기억 상실 → 39/44의 agency 복귀 | 39, 44 / Nera, Vael / 추가 보고·관측 반응 | **High / Medium** |
| 15 | `memory.vael.moved_report_off_books` / `npc.vael` / 39 | `fact.registry.report_arrived_early`; 보고서 | 보고를 공식선 밖으로 옮긴 선택과 Nera 연결 | 보고 존재는 남고 보호 의도/source 상실 → 45의 열쇠 의미 복귀 | 45 / Vael, Nera, Kairós / cell release 맥락 | **Medium-High / High**; Ch39 DRAFT |
| 16 | `memory.arrel.face_body_recognition` / `player.arrel` / 35 | `fact.arrel.has_seen_the_face`; 몸의 선행 반응 | 이름 없이도 얼굴을 아는 구체적 만남 | 낯익음만 남고 인물·장소 연결 상실 → 만남 context 복귀 | 36~39 / Arrel, Nera/Vael 후보 / optional identification | **Medium / Medium** |

선별하지 않은 감각 파편과 일회성 분위기 기억은 기존 `MemoryManager`의 전투 자원으로 남기는 편이 낫다. Memory World Engine에는 후속 consumer가 명확한 인물·출처·선택 기억만 올린다.

### Consequence 범위

| Memory 후보 | Optional consequence | Main-story consequence 가능성 |
|---|---|---|
| Elia lullaby source | Sable/Hannah 질문, source를 말하거나 숨기는 반응 | **중간**. relay 신뢰 맥락은 바꿀 수 있으나 작동 자체는 막지 않아야 함 |
| Malet BL-07 source | Ch21 기록의 증거 강도와 Kairós 보조 대사 | **낮음**. 이동·필수 단서는 knowledge로 보장 |
| Kairós selfless burn | 추적 유보 이유, 붉은 수첩 추가 읽기 | **높음**. 33/45장 선택 동기와 연결되므로 초기에는 read-only 분기만 허용 |
| Elia relay cost warning | 41장 고백 문구와 신뢰 질문 | **높음**. relay 진입 자체는 잠그지 말 것 |
| Arrel name giver | Elia 반응, 이름 독백, 복원 대화 | **매우 높음**. 43/46장 결말 핵심이므로 progression 확정 전 구현 금지 |
| Sable Vessa bond | 개인 질문 ↔ 열일곱 기록 질문 교체 | **없음**. 필수 경로·아이템·trial은 불변 |
| Elia Pell hand | 책의 작성자/source 추가 정보 | **낮음**. 장소 channel은 fact로 유지 |
| Arrel first sword teacher | 전투 독백과 Ch21 record 대조 | **중간**. 검 사용 능력은 body knowledge로 유지 |
| Arrel Editor encounter | 흠집·후유증 증거 조합 | **중간**. Editor 대면 사실은 다른 증거로 유지 |
| Hannah lullaby recipient | 이름 공개 시점의 optional reveal | **미정**. 원고가 이름을 숨기므로 현재 구현 보류 |
| Mira report misalignment | Lumea 접근 지원과 사적 ledger 반응 | **낮음** |
| Tobias Selene/Ami entry | mother/daughter 목록 설명 | **없음** |
| Driver chose group | 신호, 재합류, 보조 탈출 반응 | **중간**. 운송 성공 자체는 보장 |
| Nera held fragment | 추가 보고와 44장 관측 반응 | **중간**. Conversion은 막지 않음 |
| Vael off-books report | Ch45 열쇠와 해방 의도 해석 | **높음**. Ch39 DRAFT 확정 전 mutation 금지 |
| Arrel face recognition | 인물 확인 optional branch | **낮음** |

## 6. 강한 ripple graph TOP 5

### 1. Elia의 일곱 음과 source

`Ch14: Elia는 일곱 음을 안다, source는 모름`
→ `Ch15: Sable이 여섯/일곱이 두 아이에게 갈렸음을 확인`
→ `Ch30: Hannah의 일곱 음, 지워진 이름, Sleep small`
→ `Ch41: relay 지식과 Elia가 숨긴 비용의 출처를 재해석`

Fact는 계속 relay를 작동시키지만 source memory 유무가 Sable/Hannah 질문, Elia의 고백, Arrel의 신뢰 해석을 바꿀 수 있다.

### 2. Malet 거래 기록

`Ch2: Arrel의 BL-07 요청 + 첫 검 거래`
→ `Ch5: Malet의 coded report를 Kairós가 분류`
→ `Ch21: Kairós가 Malet의 전체 Arrel record를 확보`

`fact.bl07.route_request_received`는 요청 사건을 유지하고 `memory.malet.bl07_request_source`만 requester identity를 소유한다. Ch5 report 시점에 memory가 active/restored면 `fact.kairos.malet_report_identified_arrel`, removed면 `fact.kairos.malet_report_requester_unknown`이 생성된다. 이 historical result는 이후 Malet memory 변화와 독립적이며 Ch21의 future consumer가 읽는다. Sable은 이 graph의 consumer가 아니다.

### 3. Kairós가 본 selfless burn

`Ch25: Arrel이 사람을 살리기 위해 기억을 씀`
→ `Ch26: Kairós가 결과 계산을 멈추고 추적을 유보`
→ `Ch33: 여러 trace를 다시 redirect하지 않음`
→ `Ch45: 자기 파일과 시스템에 대한 결론`

burn telemetry라는 knowledge와, 그 burn을 인간적 선택으로 기억하는 episodic memory를 분리할 수 있다.

### 4. Relay body cost

`Ch17: Elia가 body cost를 읽고 숨김`
→ `Ch18/22: 책과 장소 channel을 계속 사용`
→ `Ch41: 숨긴 사실을 고백하고 완전한 정보 공유를 선택`
→ `Ch42~43: relay와 이름의 실제 대가`

규칙 지식이 남아도 '언제부터 알고 누구를 보호하려 숨겼는가'가 사라지면 같은 고백의 의미가 달라진다.

### 5. Arrel이라는 이름의 giver/source

`Ch10: 돌아온 이름 파편을 숨김`
→ `Ch14~15: 여섯 음과 두 아이의 source가 드러남`
→ `Ch43: Elia가 준 이름을 태워 relay를 엶`
→ `Ch46: 기능과 몸은 남고 새 이름을 받을지 질문받음`

가장 큰 payoff지만 메인 결말을 직접 건드리므로 마지막에 구현해야 한다.

## 7. 구현 순서 권고

1. 현재 구현된 Sable Vessa/17 local arc와 Ch5 Malet report historical consequence를 안정화한다.
2. 최신 Ch21이 게임 progression에 자리 잡을 때 두 Kairós report fact 중 하나를 Malet record consumer에 연결한다.
3. Elia의 Ch14→15→30 lullaby source arc를 첫 다중-chapter 핵심 memory로 구현한다.
4. Kairós의 Ch25→26→33 hesitation arc를 read-only/optional consequence부터 연결한다.
5. Arrel name arc는 최신 Ch41~46 progression이 고정된 뒤에만 다룬다.

현재 ActorRegistry에는 실제 consumer가 있는 `player.arrel`, `npc.malet`, `npc.sable`, `npc.kairos`만 유지한다. 다른 후보 인물을 catalog에 미리 대량 등록하지 않는다.
