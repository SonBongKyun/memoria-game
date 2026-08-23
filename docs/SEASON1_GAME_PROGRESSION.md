# MEMORIA Season 1 Canon Game Progression

기준일: 2026-08-23

상태: migration plan, runtime rewrite 전

연동 문서: [Season 1 Memory Map](SEASON1_MEMORY_MAP.md)

이 문서는 최신 Season 1 영문 원고 Chapter 01~46을 현재 Godot 게임 구조에 옮기기 위한 실행 지도다. 새 Memory Arc를 정의하거나 구현하지 않는다. 사건의 상세한 memory/knowledge 분리는 `SEASON1_MEMORY_MAP.md`가 소유하며, 여기서는 그 상태가 어느 progression 지점에 놓여야 하는지만 다룬다.

## 0. Canon boundary와 판정 규칙

우선순위는 다음과 같다.

1. `../원고/수정본_txt/Chapter 01 - Ash.txt` ~ `Chapter 46 - Residue.txt`
2. `docs/SEASON1_MEMORY_MAP.md`
3. 실제 런타임 진입점, scene, script, dialogue/VN JSON, flag와 gate
4. 이전 문서와 세션 기록

Chapter 39는 파일명에 `DRAFT`가 남아 있다. 사건 순서는 Season 1에 포함하되, 대사 고정과 Memory mutation은 원고가 잠긴 뒤에 한다. 한국어 원고는 현재 1~32장 파일이 있으나 32장은 영문 269줄 대비 139줄로 진행 중이다. 따라서 migration 계약은 완성된 영문 46장을 기준으로 한다.

분류는 다음 의미로 사용한다.

- `KEEP`: 사건, 인물, 인과와 위치가 최신 canon에 가깝다.
- `MOVE`: 활용할 수 있는 구현이 있으나 canon 순서의 다른 위치로 옮겨야 한다.
- `REWRITE`: scene/구조/자산은 유지할 수 있지만 내용과 gate를 다시 써야 한다.
- `REPLACE`: 구버전 사건이므로 최신 사건으로 교체해야 한다.
- `REMOVE`: 최신 canon에 없고 다른 용도로도 유지하면 안 된다.
- `MISSING`: 최신 사건을 표현하는 런타임 콘텐츠가 없다.
- `SPLIT`: 현재 한 game section이 여러 canon chapter를 압축한다.
- `MERGE`: 여러 game section이 하나의 canon 사건을 중복하거나 파편화한다.

## 1. 현재 실제 game progression

### 1.1 실제 진입점은 두 개다

```text
TITLE
├─ NEW GAME
│  └─ Ch1 VN bundle
│     └─ Ch2 Verdan map → Ch3 → ... → Ch10 BL-07
│        └─ The Seam legacy epilogue → Credits
└─ AFTERMATH
   └─ separate Ch11 → ... → Ch24 VN chain → Credits
```

두 경로는 하나의 연속 플레이가 아니다. `NEW GAME`은 Ch10 뒤 The Seam의 구버전 에필로그로 끝나며, Ch11~24는 타이틀의 별도 `AftermathPreviewButton`이 새 상태를 만들어 시작한다. 따라서 현재 저장 데이터에서 “Ch10 다음 Ch11”이라는 자연스러운 progression contract가 없다.

### 1.2 Boot와 New Game

| 단계 | 실제 코드 경로 | 상태 변화와 gate |
|---|---|---|
| Boot | `project.godot` → `scenes/main/main.tscn` | `GameManager` 등 autoload 초기화, 타이틀 표시 |
| New Game | `scenes/main/main.gd::_on_new_game_pressed()` | WorldState와 legacy memory를 reset하고 story flags를 비운다. `current_chapter = 1`, `SceneFlow.pending_scene_id = "ch1_cold_open"` |
| Continue | `SaveManager.load_game(1)` | slot 1의 scene, GameManager, MemoryManager, WorldState, SceneFlow index를 복원 |
| Aftermath | `main.gd::_on_aftermath_preview_pressed()` | 새 상태로 `current_chapter = 11`, `part2_aftershock_preview = true`, `ch11_departure`를 별도 시작 |

### 1.3 New Game 본편의 실제 순서

| Game step | 플레이 구간 | 필수 진행 / 주요 dialogue | gate, item, battle | 다음 trigger |
|---:|---|---|---|---|
| 1 | Ch1 VN: `ch1_cold_open` | 오프닝 자기 정의 선택 | 선택 flag만 기록 | `goto_scene ch1_prologue` |
| 2 | Ch1 VN: `ch1_prologue` | 죽은 짐승, Elia, camp, BL-07 동기 | `daily_campfire_song` 연소 선택 가능 | `goto_scene ch1_forest_walk` |
| 3 | Ch1 VN: `ch1_forest_walk` | stump/shrine/Elia 선택형 순회 | 최소 한 곳 방문 후 진행 | `goto_scene ch1_void_beast` |
| 4 | Ch1 VN: `ch1_void_beast` | Void Beast와 기억 연소 전투를 VN 선택으로 처리 | 실제 battle scene이 아니라 VN 분기. `ch1_void_beast_defeated` 설정 | `goto_scene ch1_after_forest` |
| 5 | Ch1→2 VN | forest 이탈, BL-07와 형제 동기, Verdan 진입 | `ch1_complete`, `current_chapter = 2` | `ch2_market_arrival` → Verdan map |
| 6 | Game Ch2, Verdan Market | Malet 대화 → 거래 수락 → 추출 → 3개 정보 → shop | 거절은 루프되어 결국 수락해야 진행. shop을 닫아야 `ch2_complete`. Malet WorldState seed 발생 | Belt Waystation |
| 7 | Game Ch3, Belt Waystation | Tobias 즉시 등장 → Blank Book → Kairós 벽글 → Tobias 합류 | `has_blank_book`; 출구는 `tobias_in_party` 필요. 필수 item 소비 gate는 아님 | Drift Shelter |
| 8 | Game Ch4, Drift Shelter | reading deterioration → Elia anchoring | 출구는 `ch4_anchoring` 필요. Tobias가 모든 핵심 대화에 이미 동행 | Crumbling Coast |
| 9 | Game Ch5, Crumbling Coast | Kairós 목격 → Elia 분리/유지 선택 → Seam 도착 | 필수 boss 없음. `elia_separates` 또는 `elia_stays` | The Seam |
| 10 | Game Ch6, The Seam | 필요 시 Elia 재합류 → Sable briefing → Sable 합류 → BL-07 입구 | `ch6_briefing_done` 후 Shade Sentinel 필수 boss. 승리 복귀 후 `ch6_complete` | Seam Outskirts |
| 11 | Game Ch7, Seam Outskirts | Sable truth → Echo Shell 획득 → controlled burn trial | `has_echo_shell`; Threshold Shade 필수 trial. 출구는 `ch7_trial_complete` 필요. Sable Vessa WorldState seed | Forgotten Forest |
| 12 | Game Ch8, Forgotten Forest | ghost encounter → Tobias ring theory | 도착 시퀀스 뒤 출구 가능. 필수 boss/item gate 없음 | Colorless Waste |
| 13 | Game Ch9, Colorless Waste | Memory Compass 획득 → Kairós 대면/진실 → Kairós 전투 | Kairós가 필수 boss 의도로 연결됨. `ch9_kairos`가 전투 전에 설정되어 gate 의미가 약함 | BL-07 Void |
| 14 | Game Ch10, BL-07 Void | core 도달 → Weave / Seal / Refuse | Weave는 legacy MemoryManager 보존 상태, Seal은 `core_name_origin` 연소, Refuse는 별도 flag | The Seam |
| 15 | Legacy epilogue | burn 수, hidden flags, Tobias/Kairós flags로 7개 ending 중 하나 선택 | Elia와 Sable 양쪽 epilogue 대화를 모두 봐야 Credits | Credits |

중요한 구조적 사실:

- `rim_forest.tscn`에는 legacy/hybrid Ch1 progression이 남아 있지만 현재 New Game은 그 맵을 방문하지 않고 Ch1을 전부 VN으로 처리한다.
- Ch2~10 map chain은 `current_chapter`와 `chN_*` story flag에 강하게 결합되어 있다.
- Blank Book, Echo Shell, Memory Compass는 획득 알림/flag는 있으나 inventory object를 소비하는 main gate는 아니다.
- Fast Travel은 `current_chapter >= destination.chapter`만으로 10개 맵을 연다. Aftermath의 Ch11 이상 상태에서는 이전 map 전체가 자동 해금된다.
- Save/Load는 scene과 flag뿐 아니라 VN scene ID/index와 WorldState를 보존한다. Migration에서 기존 scene path와 `chN_*` 의미를 바꾸면 old save 호환 레이어가 필요하다.

### 1.4 별도 Aftermath VN의 실제 순서

`ch11_departure → ch12_reader → ch13_third_person → ch14_confessor_intervention → ch15_singer → ch16_nera → ch17_forgetting_storm → ch18_living_funeral → ch19_approach → ch20_monolith → ch21_editors_turn → ch22_core → ch23_conversion → ch24_testimony → Credits`

이 체인은 최신 canon의 제목 일부를 가져왔지만 사건 순서는 크게 다르다. Ch18의 `demo_end`는 `demo_build` flag일 때만 중간 종료하며, 일반 경로는 Ch19로 계속된다. Ch23의 `resolve_part3_ending`은 legacy multi-ending evaluator를 호출한다.

## 2. 최신 원고 Chapter 1~46과 현재 게임 1:1 비교

| Canon | 제목 | 핵심 사건 / 인물 / location | 현재 게임 대응 | 상태 | migration 지시 |
|---:|---|---|---|---|---|
| 1 | Ash | Arrel이 어린 시절 기억을 태워 Void Beast를 죽이고 Elia와 이동. Achen/Rim forest | Ch1 VN 5종과 기존 Rim/Beast 자산 | `KEEP` | VN combat의 비용과 Elia/녹색 나무 인과만 정리하고 핵심은 유지 |
| 2 | Rules of Deal | Verdan에서 Malet에게 첫 검 memory를 팔고 BL-07 route/Kairós 경고를 얻음 | Ch2 Verdan/Malet 거래 | `KEEP` | 거래, Malet fact/memory, 시장 map 유지. 후속만 Ch5/21로 연결 |
| 3 | Weight of Pages | Belt, 버려진 waystation, 읽기 틈, blank book, Editor 문장. Arrel/Elia만 이동 | Game Ch3 waystation이 Tobias를 조기 등장·합류시킴 | `REWRITE` | map/book/wall 자산은 유지하고 Tobias를 제거. Elia와 책 중심으로 재작성 |
| 4 | Drift | 단어·색·어린 시절 drift, Elia의 body anchoring | Game Ch4 Drift Shelter | `REWRITE` | shelter/anchoring 유지, Tobias 대사와 조기 classification 설명 제거 |
| 5 | The Classifier | Kairós POV, forest burn trace, black/red notebooks, Malet report | Game Ch5는 coast와 Elia 분리 | `REPLACE` | 현 Ch5 story를 뒤로 옮기고 Kairós 조사 intercut으로 교체 |
| 6 | The Seam | 살아 있는 색의 settlement, Sable/Haren, Elia가 남은 시간을 질문 | Game Ch6 The Seam | `REWRITE` | map/노년 Sable 유지. 즉시 BL-07 briefing·party/boss gate를 제거 |
| 7 | Sable | 17 seekers, Vessa, 17 bark pieces와 Arrel의 18번째 조각 | Game Ch7에 Vessa/17은 교정됐으나 outskirts trial이 결합 | `REWRITE` | Vessa gameplay 유지. controlled-burn trial/Echo Shell 보상 순서를 canon에 맞게 재배치 |
| 8 | The Listening Wood | Sable 동행, 친숙한 목소리, forest가 다섯 번째 시간을 빼앗음 | Game Ch8 forest, ghost/Tobias ring theory | `REWRITE` | forest map 유지, ghost family와 조기 Tobias를 borrowed-voice 규칙으로 교체 |
| 9 | Borrowed Mouth | 빌린 입의 source 불일치, Echo Shell 검증 | Game Ch9는 Colorless Waste/Kairós boss | `REPLACE` | Ch9 map story를 forest 후반으로 교체. Waste/Kairós 자산은 후반 이동 |
| 10 | The Price | Elia는 문구/자장가를 알지만 source를 모름. Arrel은 돌아온 이름 파편을 숨김 | Game Ch10은 BL-07 core와 결말 | `REPLACE` | 조기 finale 제거. forest aftermath와 name/source setup으로 교체 |
| 11 | What Fear Used To Do | Elia를 잃는 공포의 결손을 인식한 채 threshold로 감 | Aftermath Ch11은 Authority patrol/Executor 탈출 | `REPLACE` | 별도 Aftermath 내용 폐기 또는 다른 구간 자산으로 repurpose |
| 12 | The Editor | Kairós의 chair encounter와 직접 edit, Elia 변화, hilt nick | 현재 독립 구현 없음. Ch21의 “Editor's Turn”은 다른 사건 | `REPLACE` | current BL-07/Colorless visual을 이용한 비전투 Editor encounter 신설 |
| 13 | Arithmetic | Haren, buckle/body pattern, source 없는 기하학, 책의 문장 | 대응 없음 | `MISSING` | Haren 소개와 몸이 아는 것/머리가 모르는 것을 gameplay observation으로 구현 |
| 14 | The Lullaby Returns | Arrel 6음과 Elia 7음, source 분리 | 현재 음악 motif만 산재 | `MISSING` | Ch10 이후 숨겨진 source setup을 정식 scene으로 구현 |
| 15 | What Hands Remember | Sable이 두 아이의 6/7음을 알아보고 Arrel 전용 무기 단서 제공 | current Ch15는 Singer | `MISSING` | The Seam 재방문에서 Sable/Haren scene으로 구현 |
| 16 | Departure | Sable이 checkpoint 밖으로 인도, Arrel의 분노/전략 회복 | current Ch11 제목만 Departure이고 사건은 다름 | `REPLACE` | Seam exit를 canonical departure로 새로 구성 |
| 17 | The Reader | Elia가 relay 절차와 body cost를 읽고 숨김 | current Ch12 Reader에 Pell/Verdan 사건까지 압축 | `REWRITE` | Reader scene을 이 위치로 이동하고 body-cost secret을 분리 |
| 18 | Pell's Hand | Elia의 손바닥이 Pell의 필체를 알아봄 | current Ch12 후반에 Pell 단서 포함 | `SPLIT` | current Ch12를 17/18/21/22로 분해하고 Pell recognition만 이 구간에 배치 |
| 19 | Mira | Lumea Registry의 Mira가 Kairós report 오차를 고치지 않고 사적으로 보존 | current Ch16에 `mira_subplot_1` 이름만 존재 | `MISSING` | 짧은 POV VN intercut으로 추가. 플레이어와 조기 접촉시키지 않음 |
| 20 | The Gray Cage | Authority checkpoint, vessels, Sable의 통제로 통과 | 대응 없음 | `MISSING` | noncombat infiltration/gate segment로 구현 |
| 21 | Verdan Again | 봉인된 Sump, 사라진 Malet, Kairós가 Malet의 Arrel record 확보 | current Ch12 도입의 Sump closure만 일치 | `REWRITE` | Verdan 재방문 map을 만들고 Malet WorldState의 정식 downstream 연결 |
| 22 | Resonance | book이 이름뿐 아니라 장소를 가리키는 channel임을 확인 | current Ch12의 book/Pell line에 일부 압축 | `REWRITE` | Verdan exit 뒤 별도 resonance beat로 분리 |
| 23 | The Third Person | Tobias와 두 번째 unstamped book, Selene/Ami 목록 | current Ch13 Third Person | `MOVE` | Tobias 첫 등장을 이 위치로 이동하고 Ch3 합류 의존 제거 |
| 24 | Two Notebooks | 두 책의 정/역방향 기록이 공명 | current Ch13 한 scene에 Ch23과 함께 압축 | `SPLIT` | current Ch13을 두 segment로 나누어 원인→발견 순서 회복 |
| 25 | The Confessor's Hall | Arrel이 임무보다 한 사람을 살리려 따뜻한 memory를 사용 | current Ch14 Confessor intervention | `MOVE` | 구조/CG를 이동하고 최신 희생 동기와 Kairós 관측으로 rewrite |
| 26 | Kairós Hesitates | Kairós가 selfless burn의 의미를 기억하고 추적을 유보 | `kairos_hesitation_1` flag만 있고 POV 없음 | `MISSING` | Ch25 직후 짧은 Kairós POV를 넣되 boss encounter로 만들지 않음 |
| 27 | The Weight of Quiet | 지하 은신처, 따뜻해지는 books, 노래 연락망 문턱 | 대응 없음 | `MISSING` | Singer 전의 숨 고르기/exploration hub로 구현 |
| 28 | The Singer | 말 대신 slate를 쓰는 Hannah와 book recognition | current Ch15 Singer의 Han/노파 scene | `MOVE` | Singer 자산을 이동하고 Hannah canon identity와 행동으로 rewrite |
| 29 | Underground Songs | 이름·빚·얼굴 검증, 두 번째 노래, 지하망 | current Ch15에 motif 일부 | `SPLIT` | current Singer 단일 chapter를 28~30으로 분리 |
| 30 | The Mother's Lullaby | Hannah/Elia 7음, `Sleep small`, 지워진 source name | current Ch15에 변형된 song merge | `SPLIT` | source를 독자에게 공개하지 않는 canon을 유지하며 payoff 분리 |
| 31 | The Old Editor | Telos가 Kairós의 어긋남을 보고도 수정하지 않음 | 대응 없음 | `MISSING` | 짧은 registry POV intercut. 새 gameplay gate로 만들지 않음 |
| 32 | The Same Next Thing | 이동 수레에서 모두가 같은 다음 일을 선택 | 대응 없음 | `MISSING` | party commitment와 다음 도로 segment를 연결 |
| 33 | Decides, Again, Not To | Kairós가 trace를 redirect하지 않기로 재선택 | current Ch21 hesitation motif와 다름 | `MISSING` | Ch26 state의 두 번째 read-only callback으로 구현 |
| 34 | The Driver Turns | Driver가 명령된 길 대신 일행을 선택 | 대응 없음 | `MISSING` | cart/route gameplay의 비핵심 선택 결과로 구현 |
| 35 | A Face He Doesn't Know | Arrel의 몸이 이름 없는 얼굴을 먼저 알아봄 | 대응 없음 | `MISSING` | road stop에서 body recognition scene 구현 |
| 36 | Nera | 세 번째 unnamed fragment를 발견하고 filing을 유보 | current Ch16 Nera | `MOVE` | Nera asset/portrait를 이동하고 구버전 dossier 사건을 rewrite |
| 37 | The Forgetting Storm | Driver 신호와 books로 집단을 유지하며 폭풍 통과 | current Ch17 storm | `MOVE` | storm CG/VFX를 이동하고 Living Funeral 연결을 제거 |
| 38 | The Echo Shell Vibrates | Echo Shell이 먼 source를 향해 다시 깨어남 | 대응 없음 | `MISSING` | existing shell presentation을 사용한 navigation beat 구현 |
| 39 | The Registrar's Hand | Vael이 이른 report를 off-books로 이동, Nera와 연결 | 대응 없음, 원고는 DRAFT | `MISSING` | 사건 lock 전에는 설계/자산 준비만 하고 dialogue/mutation 보류 |
| 40 | The White Country | Lumea 도착, aqueduct 침투, Mira 선택 회수 | current Ch19 Approach/Ch20 Monolith가 다른 Lumea 진입 | `MOVE` | Lumea CG를 이동하고 Sable death/legacy procession 제거 |
| 41 | The Sea Inside | memory sea, Elia의 body-cost 고백, 완전한 정보 공유 | current Ch22 Core의 relay choice와 일부 겹침 | `MOVE` | sea/core asset 이동, Ch17 secret을 회수하도록 rewrite |
| 42 | The Chair | Grand Archivist의 빈 껍질, `preserve`→`prove` 교리 변형 | current Ch20/22에 Chief Archivist/core가 파편화 | `MERGE` | 두 scene을 합쳐 canonical Chair 한 segment로 재작성 |
| 43 | The Price of a Name | Arrel이 Elia가 준 이름을 반드시 태워 relay를 엶 | current Ch23에서 선택 가능한 ending branch | `REWRITE` | 선택형 route를 canon 필수 사건으로 변경. ending 판정과 분리 |
| 44 | Conversion | memory sea가 도시로 풀리고 소유자 없는 기억이 돌아감 | current Ch23 Conversion | `REWRITE` | conversion visual은 유지, 7-way ending routing 제거 |
| 45 | The Cell Below | Kairós가 자기 file을 쓰고 Vael의 열쇠로 풀려남 | 대응 없음 | `MISSING` | Ch44와 병행/후속 POV scene 추가 |
| 46 | Residue | 이름 없는 Arrel의 기능/몸 습관, Elia의 새 이름 제안 | legacy map epilogue와 Ch24 Testimony | `REPLACE` | Sable death/다중 testimony 제거, 단일 canonical residue ending과 Credits로 교체 |

### 2.1 Current-only `REMOVE` inventory

46개 표의 행은 모두 최신 canon chapter를 기준으로 하므로, 최신 원고에 대응 행 자체가 없는 현 게임 전용 사건은 아래에 별도로 분류한다. scene shell, CG, 전투 자산까지 삭제한다는 뜻이 아니라 **Season 1 main progression에서 해당 사건과 gate를 제거**한다는 뜻이다. 재사용할 수 있는 presentation 자산은 5절의 asset 판정을 따른다.

| 현재 구현 | 판정 | 제거 경계 |
|---|---|---|
| Game Ch3의 Tobias 첫 등장·즉시 합류 | `REMOVE` | Ch3~22의 Tobias 동행, 선행 지식, 필수 exit gate에서 제거. Tobias 자체는 canon Ch23으로 이동 |
| Game Ch5의 Elia 조기 분리 선택 | `REMOVE` | canon Ch5 Kairós Classifier를 밀어내는 분기와 이후 조기 party 상태 제거 |
| Ch6 Shade Sentinel / Ch7 Threshold Shade mandatory gate | `REMOVE` | Sable의 canon 안내와 17/Vessa arc를 boss 승리로 잠그는 필수 gate 제거. 전투 자산은 optional encounter 후보 |
| Ch8 ghost family와 조기 Tobias ring theory | `REMOVE` | Listening Wood의 borrowed voice 규칙과 충돌하는 구버전 설명 제거 |
| Ch9 Kairós mandatory boss | `REMOVE` | canon Ch12/26/33/45의 추적자·관찰자 인과를 깨는 조기 결전 제거 |
| Ch10 BL-07 core 3-way finale와 즉시 epilogue | `REMOVE` | canon Ch11~46을 건너뛰는 Season 1 main-route 종료 제거 |
| Ch18 Living Funeral core event | `REMOVE` | 최신 46장에 없는 사건과 Ch37 storm 연결 제거 |
| Ch19 Sable death, ledger, post-death development | `REMOVE` | 생존 Sable과 Vessa/17 canon을 뒤집는 사건 및 후속 testimony 제거 |
| Ch23 선택형 Name Burn과 7-way ending routing | `REMOVE` | 선택형 분기 계약 제거. Name Burn은 Ch43 필수 사건으로 `REWRITE` |
| Ch24 multi-testimony ending | `REMOVE` | canon Ch46 Residue 단일 인과를 대체하는 결말 제거 |

## 3. 가장 큰 canon 충돌 10개

1. **본편과 Aftermath가 분리됨**: Ch10에서 credits로 끝난 뒤 Ch11이 자동으로 시작하지 않는다.
2. **Tobias가 20장 일찍 합류함**: game Ch3의 지식·전투 동행·party flag가 canon Ch23 이전 모든 장면을 오염시킨다.
3. **The Classifier가 사라짐**: canon Ch5 Kairós/Malet report가 game Ch5 coast separation으로 대체되어 추적 인과가 끊겼다.
4. **BL-07 finale가 33장 일찍 발생함**: game Ch10의 core/Seal/Weave가 canon Ch11~42의 원인과 발견을 건너뛴다.
5. **Sable arc에 구버전 gate가 남음**: Vessa/17은 교정됐지만 Ch6 briefing, immediate party, Ch7 trial, later death/ledger가 최신 Sable과 충돌한다.
6. **Malet record의 정식 callback이 없음**: Ch2 거래 뒤 Ch5 report와 Ch21 record recovery로 이어져야 하나 current WorldState는 Verdan 재대화에서만 소비된다.
7. **Reader/Pell/Tobias가 한 덩어리로 압축됨**: current Ch12~13이 canon 17/18/21/22/23/24를 앞뒤 없이 섞는다.
8. **Singer/lullaby가 13~15장 앞당겨지고 인물이 변형됨**: current Ch15 Han은 canon Ch28~30 Hannah/source arc를 대체하지 못한다.
9. **Nera와 Forgetting Storm이 너무 일찍 등장함**: current Ch16/17은 canon Ch36/37의 선행 조건인 Driver, Kairós 유보, unnamed fragment를 건너뛴다.
10. **Name Burn과 Conversion이 선택형 multi-ending으로 변형됨**: canon Ch43은 필수 사건이고 Ch44~46은 단일 인과인데, current Ch23/24는 7개 ending으로 분기한다.

## 4. Proposed Season 1 game progression

소설 46장을 game chapter 46개로 복제하지 않는다. 아래 13개 game segment가 원고 순서를 보존하면서 map exploration, VN intercut, dialogue와 memory consequence를 배치하는 최소 구조다.

### ACT I — The trail learns his shape

| Segment | Canon | location / gameplay objective | major dialogue | boss / trial | Memory placement | 다음 trigger |
|---|---|---|---|---|---|---|
| G1 Ash | 1 | Achen/Rim forest. Void Beast를 추적하고 실제 전투 또는 현재 VN 전투 중 하나로 단일화 | Elia 발견, camp, BL-07 동기 | **Void Beast 유지** | 기존 legacy burn만. 새 World memory 없음 | forest 이탈 |
| G2 Rules of Deal | 2 | Verdan/Sump. Malet을 찾아 첫 sword memory와 route 거래 | Malet의 3개 정보, Kairós 경고 | 없음 | Malet fact/memory acquire 및 현재 optional manipulation | 거래 완료 |
| G3 Pages and Drift | 3~4 | Belt Waystation → Drift Shelter. Blank Book 발견, reading loss를 견디고 Elia anchoring 받기 | Arrel/Elia, book, Editor 문장 | 필수 boss 없음 | first-sword loss는 legacy/후보 상태로 추적, 새 arc 구현은 보류 | anchoring 완료 |
| G4 The Classifier | 5 | 짧은 Kairós POV/VN intercut. forest trace와 Malet report 분류 | black/red notebooks, combustion signature | 없음 | Malet source의 첫 read-only consequence | Kairós가 추적 시작 |

### ACT II — The Seam and the borrowed voice

| Segment | Canon | location / gameplay objective | major dialogue | boss / trial | Memory placement | 다음 trigger |
|---|---|---|---|---|---|---|
| G5 The Seam | 6~7 | The Seam settlement. Sable/Haren과 만나고 17 bark records 조사 | 남은 시간, Vessa, Arrel의 18번째 조각 | **현재 controlled burn trial 제거** | Sable Vessa fact/memory acquire·local consequence | Sable이 trail을 열어 줌 |
| G6 Listening Wood | 8~10 | Forgotten Forest. 친숙한 voice를 판별하며 path와 시간을 지키기 | borrowed mouth, Echo Shell mismatch, 6/7 notes와 name fragment | forest climax는 가능하나 새 boss를 원고 없이 만들지 않음 | Elia lullaby/name source 후보 acquire만 계획 | price scene 후 threshold 진입 |
| G7 Threshold and Editor | 11~13 | BL-07 threshold/road. fear 결손을 인식하고 Kairós chair encounter에서 살아남기 | Editor의 목록, Elia edit, Haren의 arithmetic | **Kairós boss 금지**, noncombat threat | Editor encounter/source candidate acquire | chair 흔적과 pattern을 가지고 귀환 |
| G8 Hands Remember | 14~16 | Seam/outer checkpoint 재방문. lullaby를 비교하고 Sable과 departure 준비 | 6/7 notes, Sable recognition, Arrel 전용 무기, departure | 없음 | Elia lullaby source의 첫 safe manipulation window | checkpoint 통과 |

### ACT III — Books, records, and songs

| Segment | Canon | location / gameplay objective | major dialogue | boss / trial | Memory placement | 다음 trigger |
|---|---|---|---|---|---|---|
| G9 Reader to Gray Cage | 17~20 | road shelter → Authority checkpoint. relay 절차를 읽고 Sable의 통제로 통과 | body cost secret, Pell's hand, Mira POV | stealth/noncombat gate | relay cost와 Pell recognition acquire | Verdan으로 우회 |
| G10 Verdan and the Third Person | 21~24 | Verdan/Sump → Arkein reading wall. Malet record를 회수하고 Tobias/두 번째 book을 만남 | Kairós record, place resonance, Selene/Ami, two notebooks | 없음 | Malet final callback, Tobias memory acquire | Confessor Hall 위치 확인 |
| G11 Confessor and Underground Songs | 25~31 | Confessor Hall → underground refuge. 사람을 구하고 Hannah network를 검증 | selfless burn, Kairós hesitation, Singer, mother's lullaby, Telos POV | Hall escape encounter는 가능하나 구조 자체를 boss 승리로 잠그지 않음 | Kairós, lullaby, Tobias 후보들의 첫/중간 callbacks | transport 확보 |

### ACT IV — The road chooses back

| Segment | Canon | location / gameplay objective | major dialogue | boss / trial | Memory placement | 다음 trigger |
|---|---|---|---|---|---|---|
| G12 Driver and Storm | 32~39 | cart route → road stops → Forgetting Storm. group과 books를 유지 | Driver의 선택, 낯선 얼굴, Nera fragment, Echo Shell, Vael report | storm survival sequence | Driver/face/Nera/Vael 후보. Ch39 lock 전 mutation 금지 | Echo Shell이 Lumea를 가리킴 |

### ACT V — The white country and residue

| Segment | Canon | location / gameplay objective | major dialogue | boss / trial | Memory placement | 다음 trigger |
|---|---|---|---|---|---|---|
| G13 White Country to Residue | 40~46 | Lumea aqueduct → memory sea → Chair → city → ruined Verdan | Mira 회수, Elia 고백, preserve/prove, name burn, Conversion, Kairós cell, 새 이름 제안 | 원고가 요구하지 않는 mandatory boss 없음. **Name Burn이 story gate** | relay/name arcs 최종 payoff, load 후 deterministic residue | Credits |

## 5. 기존 asset 재사용 지도

### 5.1 Maps와 exploration

| 자산/scene | 판정 | Canon 사용 계획 |
|---|---|---|
| `rim_forest.tscn` | `EDIT` | Ch1 VN과 중복된 시작 구조를 하나로 정리한 뒤 실제 Achen/Rim exploration에 사용 |
| `verdan_market.tscn` + `verdan_ledger_cellar.tscn` | `KEEP AS-IS` / `EDIT` | Ch2 거래는 유지. 같은 map을 Ch21 폐쇄된 Sump 재방문 상태로 확장 |
| `belt_waystation.tscn` | `EDIT` | Ch3 book 발견에서 Tobias 제거. Ch23 Tobias 첫 등장 때 상태가 바뀐 재방문 map으로 재사용 |
| `drift_shelter.tscn` | `EDIT` | Ch4 anchoring 유지. Ch17 reader shelter로 재방문 가능 |
| `crumbling_coast.tscn` | `REPURPOSE` | current Ch5 separation story는 제거하고 canon road/coast/checkpoint travel에 배치 |
| `the_seam.tscn` | `EDIT` | Ch6/7와 Ch14~16 재방문 hub. 노년 Sable과 settlement visual 유지 |
| `seam_outskirts.tscn` | `EDIT` | mandatory trial을 제거하고 departure/checkpoint 접근부로 사용 |
| `forgotten_forest.tscn` + `forest_name_hollow.tscn` | `EDIT` | Ch8~10 Listening Wood/Borrowed Mouth를 위한 핵심 map. ghost/ring theory 교체 |
| `colorless_waste.tscn` + `waste_grey_caravan.tscn` | `REPURPOSE` | 조기 Kairós boss를 제거하고 Ch16/20 또는 Ch32~38의 colorless road/cart 구간으로 이동 |
| `bl07_void.tscn` + `bl07_seed_vault.tscn` | `REPURPOSE` | Ch10 finale 용도를 폐기하고 Ch11~13 threshold/Editor의 불안정 공간으로 제한 |
| optional site 9종 | `REPURPOSE` | canon location과 맞는 경우에만 field vignette로 사용. 기존 lore가 충돌하면 scene shell/canvas만 유지 |
| Lumea/aqueduct | `MISSING` | current VN CG는 있으나 explorable map은 없다. G13 구현 시 별도 결정 필요 |

### 5.2 Characters, portraits, sprites

| 범주 | 판정 | 계획 |
|---|---|---|
| Arrel, Elia | `KEEP AS-IS` | 현 portrait/field/battle identity 유지 |
| Malet | `KEEP AS-IS` | Ch2 및 Ch21 record consumer에 그대로 사용 |
| Sable | `KEEP AS-IS` | canonical old blind Sable assets만 사용. 젊은 Sable draft는 live 복귀 금지 |
| Tobias | `MOVE` | portrait/sprite는 유지하되 첫 field appearance를 Ch23으로 이동 |
| Kairós | `MOVE` / `EDIT` | portrait/CG 유지. Ch5/12/26/33/45 POV/encounter로 이동하고 Ch9 boss identity는 폐기 |
| Nera | `MOVE` | 현 asset을 Ch36 이후로 이동 |
| Vael | `EDIT` | silhouette CG 2장은 존재하지만 portrait/field identity는 부족 |
| Haren, Hannah, Mira, Telos, Driver, Velor, Grand Archivist | `MISSING` | progression이 해당 wave에 도달할 때만 필요한 자산을 제작. 이번 plan에서는 생성하지 않음 |
| Vessa, Pell | `KEEP AS RECORD` | 얼굴을 새로 만들 필요 없이 bark/handwriting/prop 중심 표현 우선 |

### 5.3 Dialogue와 VN

| 현재 묶음 | 판정 | 계획 |
|---|---|---|
| Ch1 VN 5종 | `KEEP AS-IS` / `EDIT` | core 사건 유지, legacy Rim map과 중복만 정리 |
| `chapter2_dialogue.json` | `KEEP AS-IS` | Malet 거래와 current WorldState vertical slice 유지 |
| `chapter3_dialogue.json` | `EDIT` / `MOVE` | book/wall은 Ch3, Tobias material은 Ch23으로 이동 |
| `chapter4_dialogue.json` | `EDIT` | Tobias를 제거하고 anchoring의 body cost를 강화 |
| current Ch5~10 dialogue | `REPURPOSE` / `DEPRECATE` | map atmosphere는 재사용, separation/trial/Kairós boss/early finale는 main canon에서 제거 |
| `ch12_reader.json` | `SPLIT` | canon 17/18/21/22로 분리 |
| `ch13_third_person.json` | `MOVE` / `SPLIT` | canon 23/24로 이동 |
| `ch14_confessor_intervention.json` | `MOVE` / `EDIT` | canon 25로 이동 |
| `ch15_singer.json` | `MOVE` / `SPLIT` | canon 28~30으로 이동, Han을 Hannah canon에 맞게 rewrite |
| `ch16_nera.json`, `ch17_forgetting_storm.json` | `MOVE` / `EDIT` | canon 36/37로 이동 |
| `ch18_living_funeral.json` | `DEPRECATE` | 최신 46장에 없는 core event. 자산만 다른 funeral/Authority scene에 재사용 검토 |
| `ch19_approach.json` | `EDIT` | Sable death/ledger 제거. Lumea 접근 visual은 Ch40으로 이동 |
| `ch20_monolith.json` ~ `ch23_conversion.json` | `MOVE` / `MERGE` / `EDIT` | canon 40~44로 재조합. Belor/Chief Archivist/multi-ending 내용 교체 |
| `ch24_testimony.json`, legacy epilogue dialogues | `DEPRECATE` / `REPLACE` | canon Residue 한 경로로 교체. 현재 ending gallery는 legacy 기록으로 격리할지 별도 결정 |

### 5.4 Systems, combat, UI, sound

| 시스템/자산 | 판정 | 계획 |
|---|---|---|
| DialogueManager, SceneFlow, SaveManager, MemoryManager, Memory World Engine | `KEEP AS-IS` | migration은 data와 좁은 scene gate 수정으로 진행. 대규모 framework 교체 금지 |
| Void Beast battle | `KEEP AS-IS` | Ch1의 유일하게 명확한 mandatory combat climax |
| Shade Sentinel, Threshold Shade, Ch9 Kairós boss gates | `DEPRECATE` | canon main progression을 막지 않게 제거. 적/전투 자산은 optional encounter로 재사용 가능 |
| battle UI/VFX/SFX, DialogueBox, VN UI, transition, save/archive UI | `KEEP AS-IS` | 사건 배치와 무관한 presentation foundation은 유지 |
| Fast Travel | `EDIT` | old numeric chapter unlock 대신 canonical segment/visited route와 호환되도록 migration wave에서 조정 |
| Story Journal, achievements, gallery chapter labels | `EDIT` | wave별로 새 segment와 canon chapter mapping을 반영. 구버전 사건을 사실로 노출하지 않음 |
| 7-way ending evaluator | `DEPRECATE` | Season 1 main route에서는 사용하지 않음. Ch43~46 단일 canon과 분리 |

새 GPT Image 제작은 이번 단계에 필요하지 않다. 현재 core cast와 장소 CG는 충분하며, 새 인물 이미지는 해당 migration wave에서 실제 consumer가 생길 때 identity/reference audit 후 제작하는 편이 안전하다.

## 6. Migration waves

| Wave | 수정 범위 | dependency | 주요 위험 | 필수 regression |
|---:|---|---|---|---|
| 1 | Canon Ch1~4: New Game Ch1 단일화, Ch2 유지, Ch3 Tobias 제거, Ch4 anchoring 정리 | 현재 Ch1 VN/Ch2 Malet baseline | old save의 `tobias_in_party`, Ch3 exit gate, battle party assumptions | New Game→Ch4 실제 run, Malet live/sandbox persistence, Ch1 VN, story flags, save/load |
| 2 | Canon Ch5~10: Kairós Classifier, Seam/Sable, Listening Wood, Price | Wave 1의 book/anchor 상태 | current Ch5 separation, Ch6 boss, Ch7 trial, Ch8 Tobias 의존 제거 | map chain Ch5~10, Sable Vessa branches, forest choices, no mandatory legacy boss |
| 3 | Canon Ch11~20: threshold/Editor, lullaby return, Reader/Pell, Mira, Gray Cage | Wave 2의 Echo Shell/lullaby setup | current Ch10 ending 제거 시 save destination 변경, new Haren/Mira assets | Ch10→Ch11 continuous transition, Editor noncombat gate, reader secret persistence, old save redirect |
| 4 | Canon Ch21~31: Verdan revisit, Malet record, Tobias first appearance, Confessor, Hannah | Wave 3의 books/Gray Cage | current Ch12~15 JSON 분할, Tobias party unlock 재번호, Singer identity | Malet Ch2→5→21 ripple, Tobias first-seen ordering, Confessor consequence, song/lullaby callbacks |
| 5 | Canon Ch32~39: Driver, face, Nera, storm, Echo Shell, Vael | Wave 4 underground network | new cast/location coverage, Ch39 DRAFT, current Ch16/17 flags | road/cart flow, Nera first appearance, storm survival, event order, no Ch39 mutation before lock |
| 6 | Canon Ch40~46: Lumea, Sea, Chair, mandatory Name Burn, Conversion, Cell, Residue | Wave 5 and locked Ch39 | highest save/ending risk, multi-ending removal, `core_name_origin` legacy state | full Season 1 run, Ch43 irreversible gate, Ch44 state, Ch45 POV, Ch46 residue, credits/NG+, legacy save policy |

### Wave discipline

- 한 wave에서 다음 wave의 actor를 미리 party에 넣지 않는다.
- old `chN_*` flag를 즉시 전면 rename하지 않는다. 새 segment가 안정화될 때 compatibility alias 또는 one-time redirect를 둔다.
- current save의 scene path가 삭제될 때는 load 전에 deterministic replacement destination을 정의한다.
- legacy ending을 즉시 파일에서 지우지 않는다. 새 main progression에서 접근 불가능하게 만든 뒤 save/gallery 정책을 별도로 확정한다.
- 각 wave는 실제 start→end gameplay run과 save/load를 통과한 뒤 다음 wave로 넘어간다.

## 7. Memory candidate 배치

ID와 memory/knowledge 의미는 `SEASON1_MEMORY_MAP.md`를 따른다. 아래 표는 progression 위치만 소유한다. “보류”는 canon에 플레이어가 해당 인물의 기억을 조작할 수단이 아직 없으므로 새 장치를 발명하지 않는다는 뜻이다.

| 후보 | acquire point | remove / restore 가능 시점 | first visible consequence | downstream callback | final payoff | Wave |
|---|---|---|---|---|---|---:|
| Elia lullaby source | Ch14, 6/7음 비교 | Ch15 Sable 확인 뒤 안전한 Seam interaction | Ch15 Sable 질문/해석 | Ch30 Hannah, Ch41 고백 | Ch41 source와 신뢰 재해석 | 3→6 |
| Malet BL-07 source | Ch2 거래 | 현재 Ch2 post-trade 재대화 유지; Ch21 record review에서도 확인 가능 | Ch5 Kairós report의 provenance | Ch21 Malet record | Ch21 evidence strength | 1→4 |
| Kairós selfless burn | Ch25 rescue | **보류**. Kairós record에 접근할 canon 수단이 생길 때만 | Ch26 hesitation | Ch33 redirect 유보 | Ch45 자기 file | 4→6 |
| Elia relay body-cost warning | Ch17 Reader | Ch18 Pell scene 이후 Elia와의 안전한 interaction 후보 | Ch18 source 설명의 깊이 | Ch41 confession | Ch42~43 relay 대가 문맥 | 3→6 |
| Arrel name giver/source | Ch10 setup, Ch43 확정 | Ch43 canonical burn만 제거 지점. Season 1 안에서 단순 restore 금지 | Ch43 identity loss | Ch44 functional residue | Ch46 새 이름 제안 | 2→6 |
| Sable Vessa daughter bond | Ch7 bark record | 현재 Ch7 optional interaction 유지 | Ch7 질문/이름 해석 | Ch15 hands/lullaby theme | Ch7 local payoff, 이후는 주제 callback | 2 |
| Elia Pell hand recognition | Ch18 | Ch18 이후 rest interaction 후보 | Ch22 place resonance 설명 | Ch23~24 book provenance | Ch41 relay source | 3→6 |
| Arrel first sword teacher | Ch2 extraction으로 이미 상실 | Ch21 Malet record가 복원 근거를 제공할 때만 | Ch2 grip/body 분리 | Ch5/21 record | Ch30 origin 대조 | 1→4 |
| Arrel Editor chair encounter | Ch12 | Ch13 aftermath의 memory inspection 후보 | Ch13 arithmetic/흠집 | Ch25~26 Kairós 판단 | Ch42 Chair 대조 | 3→6 |
| Hannah lullaby recipient name | Ch30, 독자에게 이름 비공개 | **보류**. 원고가 이름을 숨기는 동안 mutation 금지 | Ch30 노래 의미 | Ch32/38 network | Ch46 residue motif | 4→6 |
| Mira report misalignment | Ch19 | **보류**. 플레이어 접근은 Ch40 이후에만 검토 | Ch40 Mira의 도움/인정 | Ch44 Nera/Mira 관측 | Ch44 Conversion 기록 | 3→6 |
| Tobias Selene/Ami entry | Ch23 | Ch24 notebook comparison | Ch24 relation readout | Ch27~30 underground names | Ch30 network trust | 4 |
| Driver chose group | Ch34 | Ch34 이후 cart rest interaction 후보 | Ch35 route/face 반응 | Ch37 storm signal | Ch46 동행 residue | 5→6 |
| Nera held unnamed fragment | Ch36 | **보류**. Ch39 Vael 연결이 확정된 뒤 | Ch39 report handling | Ch40 Lumea access | Ch44 agency callback | 5→6 |
| Vael moved report off-books | Ch39 | **보류**, DRAFT lock 이후 | Ch39 Nera 연결 | Ch45 key 의미 | Ch45 Kairós release | 5→6 |
| Arrel face/body recognition | Ch35 | Ch35 road stop 직후 inspection 후보 | Ch36 Nera encounter 문맥 | Ch38~39 source narrowing | Ch39 identity 연결 | 5 |

## 8. Major blockers와 먼저 migration할 구간

### 가장 먼저 할 실제 migration

**Wave 1의 Ch3~4 correction**이 첫 구현 단위다.

이유:

- Ch1~2는 최신 canon과 가장 가깝고 Malet Memory gameplay도 이미 안정적이다.
- Tobias의 Ch3 조기 합류는 이후 Ch4~10 dialogue, battle party, flags, journal을 모두 오염시키는 가장 이른 divergence다.
- Belt Waystation과 Drift Shelter map, Blank Book, anchoring 자산은 그대로 살릴 수 있어 content rewrite 대비 기술 위험이 낮다.
- 여기서 Tobias를 제거하고 Ch23로 미루면 뒤 wave의 원인→결과 순서를 안전하게 세울 수 있다.

첫 migration slice의 성공 조건은 `Ch2 Malet 거래 완료 → Tobias 없이 Belt Waystation의 Blank Book 발견 → Elia와 Drift/anchoring → Ch5 Classifier intercut 직전 저장`이다.

### 해결해야 할 blocker

1. New Game main path와 Aftermath path를 언제 하나로 합칠지 결정해야 한다.
2. `current_chapter` 숫자가 map unlock, memory grant, battle party, save display, journal, achievements에 동시에 쓰인다.
3. `tobias_joined`, `sable_joined`가 battle roster와 dialogue 양쪽을 제어하므로 등장 이동은 단순 JSON move가 아니다.
4. current Ch10 scene path에 저장된 save를 새 Ch10/11 경계로 어떻게 redirect할지 정의해야 한다.
5. legacy `core_name_origin` burn과 World State의 future name-source memory를 중복 소유시키면 안 된다.
6. current epilogue/ending gallery의 7개 결과를 삭제할지 legacy archive로 격리할지 정책이 필요하다.
7. Ch39가 DRAFT이므로 Vael/Nera final wording과 mutation은 잠가 둘 수 없다.
8. Haren, Hannah, Mira, Telos, Driver, Velor, Grand Archivist의 live visual identity가 없다.
9. Korean manuscript는 Ch32가 진행 중이고 Ch33~46이 없으므로 late-game localization은 English lock 이후 진행해야 한다.
10. 기존 smoke/validation 일부가 current non-canon anchor text와 old chapter number를 contract로 삼는다. 각 wave에서 runtime과 함께 좁게 갱신해야 한다.

## 9. 이번 plan의 변경 경계

- 새 memory/fact/actor를 등록하지 않았다.
- DialogueManager, SceneFlow, Quest, SaveManager, MemoryManager, WorldRewriteDirector를 수정하지 않았다.
- current gameplay progression과 ending behavior를 아직 바꾸지 않았다.
- 이미 교정된 Malet/Sable Memory gameplay는 기준점으로 유지한다.
- 이미지 생성과 asset 삭제를 하지 않았다.
- 실제 migration은 Wave 1부터 별도 검토 가능한 working set으로 진행한다.
