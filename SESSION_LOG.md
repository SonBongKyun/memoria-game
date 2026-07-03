# MEMORIA — 개발 세션 로그

---

## S01 — 2026-04-05 (프로젝트 초기 세팅)

### 완료
- [x] 프로젝트 폴더 구조 생성 (scenes, scripts, assets, data)
- [x] project.godot 설정 파일 (1280x720, 입력 매핑, 오토로드, 픽셀아트 필터링)
- [x] GameManager (오토로드) — 게임 상태, 스토리 플래그, 플레이어 데이터
- [x] MemoryManager (오토로드) — 기억 데이터 클래스, 연소/잔존 시스템, 초기 기억 6개
- [x] DialogueManager (오토로드) — 대화 진행, 선택지, 기억 연소 연동
- [x] SceneTransition (오토로드) — 페이드 인/아웃
- [x] Player 스크립트 + 씬 — 4방향 이동, 상호작용 레이캐스트, 카메라
- [x] Main 테스트 씬 — 시스템 확인용 (대화/기억/연소 테스트)
- [x] Chapter 1 대화 데이터 (JSON)
- [x] 프로젝트 아이콘 (SVG 플레이스홀더)

### Godot 설치 필요
- https://godotengine.org/download 에서 Godot 4.x 다운로드
- zip 풀면 바로 실행 가능
- 실행 후 "Import" → Game 폴더의 project.godot 선택

### 다음 세션 (S02) 할 일
- [ ] Godot에서 프로젝트 열기 확인
- [ ] 플레이스홀더 스프라이트 생성 (색깔 사각형)
- [ ] AnimatedSprite2D에 걷기 애니메이션 추가 (4방향 x 4프레임)
- [ ] 첫 번째 맵(림 외곽 숲) 기본 타일맵 레이아웃
- [ ] 실제 실행 테스트

### 메모
- .tscn 파일의 서브리소스는 Godot에서 열 때 자동 생성될 수 있음
- 첫 실행 시 에러 뜨면 씬 파일을 Godot 에디터에서 재생성하면 됨
- 기억 시스템의 초기 데이터는 Chapter 1 기준. 챕터 진행에 따라 추가

---

## S02 — 2026-04-05 (플레이스홀더 스프라이트 + 첫 맵 + 버그 수정)

### 완료
- [x] **버그 수정:** player.tscn SubResource 순서 수정 (정의가 사용보다 앞에 오도록)
- [x] **버그 수정:** main.tscn 비표준 unique_id 속성 제거
- [x] **버그 수정:** chapter1_dialogue.json ██████░░░░ 게이지 → 간접 표현 교체 (개발규칙 4번)
- [x] **개선:** MemoryManager enum 역순 비교에 대한 주석 추가
- [x] **플레이스홀더 스프라이트:** player.gd에서 SpriteFrames 동적 생성 (다크 블루 사각형 + 방향별 눈/머리카락)
- [x] **애니메이션:** 8개 애니메이션 구현 (idle/walk x 4방향, walk은 4프레임 발 움직임)
- [x] **첫 맵:** 림 외곽 숲(rim_forest) — 25x18 타일, 풀/길/나무/덤불/물 5종
- [x] **맵 충돌:** 나무/물 타일에 StaticBody2D 충돌 자동 생성
- [x] **나무/덤불 시각적 디테일:** 줄기+수관, 덤불 레이어 표현
- [x] **main_scene 변경:** project.godot → rim_forest.tscn으로 시작 씬 변경

### 변경된 파일
- `scenes/player/player.tscn` — SubResource 순서 수정
- `scenes/main/main.tscn` — unique_id 제거
- `data/chapter1_dialogue.json` — 게이지 표현 수정
- `scripts/systems/memory_manager.gd` — enum 주석 추가
- `scripts/core/player.gd` — 플레이스홀더 스프라이트 + 애니메이션 전면 재작성
- `scenes/maps/rim_forest.tscn` — **신규** 첫 맵 씬
- `scenes/maps/rim_forest.gd` — **신규** 맵 빌드 스크립트
- `project.godot` — main_scene 경로 변경

### 다음 세션 (S03) 할 일
- [x] NPC 기본 씬 + 상호작용 시스템 (대화 트리거) → S03에서 완료
- [x] 대화 UI (DialogueBox 씬) — 화면 하단 텍스트 박스 + 포트레이트 → S03에서 완료
- [x] DialogueManager에 JSON 파일 로더 연결 → S03에서 완료
- [ ] Godot 4.6에서 프로젝트 열고 실행 테스트 → S04로 이월
- [ ] 맵 레이아웃 조정 (실제 플레이 느낌 확인 후) → S04로 이월

### 메모
- 스프라이트는 _ready()에서 코드로 생성됨. 실제 에셋으로 교체 시 _setup_placeholder_sprites() 함수만 제거하면 됨.
- 맵도 ColorRect + StaticBody2D로 코드 생성. 나중에 TileMap으로 전환 가능.
- 기존 main.tscn (테스트 씬)은 삭제하지 않고 유지. 시스템 디버그용으로 사용 가능.

---

## S03 — 2026-04-05 (NPC 시스템 + 대화 UI + JSON 로더)

### 완료
- [x] **NPC 기본 씬:** StaticBody2D 기반, 플레이스홀더 스프라이트(코드 생성), interact() 인터페이스
- [x] **NPC export vars:** npc_name, dialogue_file, dialogue_key, npc_color (에디터에서 설정 가능)
- [x] **엘리아 NPC 배치:** 림 외곽 숲 맵에 엘리아(청회색) 배치, dialogue_key="elia_appears"
- [x] **DialogueManager JSON 로더:** load_dialogue_file(), load_and_start() 추가. 캐싱 지원.
- [x] **DialogueBox (오토로드):** 하단 대화 박스 UI 전체 코드 생성
  - 어두운 반투명 패널 (서고 모티프)
  - 좌측 포트레이트 (캐릭터별 색상 + 이니셜)
  - 타자기 효과 (글자당 0.03초, Space로 즉시 완료)
  - 선택지 버튼 (VBoxContainer, 키보드 포커스 지원)
  - ▼ 다음 대사 표시기
- [x] **Player 입력 정리:** 대화 진행 입력을 DialogueBox._unhandled_input()으로 이전 (충돌 방지)

### 변경된 파일
- `scripts/core/npc.gd` — **신규** NPC 스크립트
- `scenes/npc/npc.tscn` — **신규** NPC 기본 씬
- `scripts/ui/dialogue_box.gd` — **신규** 대화 UI 스크립트
- `scripts/systems/dialogue_manager.gd` — JSON 로더 추가 (load_dialogue_file, load_and_start)
- `scenes/maps/rim_forest.tscn` — 엘리아 NPC 인스턴스 추가
- `scripts/core/player.gd` — 대화 중 입력 처리 제거 (DialogueBox로 이전)
- `project.godot` — DialogueBox 오토로드 추가

### 다음 세션 (S04) 할 일
- [x] 기억 UI (아렐의 서고) → S04에서 완료
- [x] 시스템 로그 팝업 UI → S04에서 완료
- [ ] Godot 4.6에서 프로젝트 열고 실행 테스트 (전체 흐름) → S05로 이월
- [ ] 맵 레이아웃 조정 (실제 플레이 느낌 확인 후) → S05로 이월

### 메모
- DialogueBox는 CanvasLayer 오토로드 (layer 50). 모든 씬에서 자동 사용 가능.
- NPC collision_layer = 12 (NPCs + Interactables). Player의 InteractionRay mask=4 에 대응.
- 포트레이트는 현재 ColorRect + 이니셜. 실제 이미지 적용 시 _update_portrait() 수정.
- JSON 대화 파일은 첫 로드 시 캐싱됨. 같은 파일 재요청 시 파싱 생략.

---

## S04 — 2026-04-05 (기억 UI + 시스템 로그 팝업)

### 완료
- [x] **아렐의 서고 (MemoryUI):** 기억 인벤토리 전체 화면 UI
  - 서고 모티프 — 어두운 나무색 패널, 등급별 색상 코딩
  - 좌측 등급 필터 탭 (All / Grade 5~1)
  - 중앙 기억 카드 스크롤 목록 (등급 좌측 컬러 바, 연소/잔존 상태 표시)
  - 우측 상세 정보 패널 (이름, 등급, 설명, 연소력, 관련 NPC, 스토리 효과, 상태)
  - Tab/M 키로 토글, ESC로 닫기
  - MENU 상태 전환 (열기/닫기 시 GameManager 연동)
- [x] **시스템 로그 (SystemLog):** 관리국 감지 로그 팝업
  - 화면 상단 청록색 모노스페이스 스타일 팝업
  - MemoryManager.memory_burned 시그널 자동 연결
  - ���이드 인 → 3.5초 유지 → 페이드 아웃
  - 연속 연소 시 대기열(queue) 처리
  - BBCode로 등급별 색상 차이 표현
- [x] **입력 매핑:** memory_menu 액션 추가 (Tab + M)
- [x] **오토로드:** MemoryUI (layer 40), SystemLog (layer 60) 등록

### 변경된 파일
- `scripts/ui/memory_ui.gd` — **신규** 기억 서고 UI
- `scripts/ui/system_log.gd` — **신규** 시스템 로그 팝업
- `project.godot` — 오토로드 2개 + memory_menu 입력 매핑 추가

### 오���로드 레이어 정리
| Layer | 오토로드 | 용도 |
|-------|----------|------|
| 40 | MemoryUI | 기억 서고 (전체화면) |
| 50 | DialogueBox | 대화 UI (하단) |
| 60 | SystemLog | 관리국 로그 (상단 팝업) |
| 100 | SceneTransition | 씬 전환 페이드 |

### 다음 세션 (S05) 할 일
- [x] 전투 시스템 기초 → S05에서 완료
- [x] 전투 중 기억 연소 스킬 선택 → S05에서 완료
- [ ] Godot 4.6에서 전체 프로젝트 실행 테스트 → S06로 이월

### 메모
- MemoryUI는 MemoryManager.memories를 직접 읽어서 카드 생성. 기억 추가/연소 시 _refresh_cards() 호출로 동기화.
- SystemLog는 queue 방식이라 연속 3번 연소해도 순차적으로 표시됨.
- memory_menu 키: Tab(4194306) + M(77)

---

## S05 — 2026-04-05 (포트레이트 이미지 + 전투 시스템)

### 완료
- [x] **포트레이트 이미지 적용:** 아렐 3장(neutral/side/wounded) + 엘리아 2장(neutral/concern) 복사
- [x] **DialogueBox 이미지 지원:** TextureRect 기반 포트레이트 표시, portrait 키→파일 매핑, fallback 유지
- [x] **커버 이미지 복사:** Cover.png → assets/cg/cover.png
- [x] **BattleManager (오토로드):** 턴제 전투 로직
  - Enemy 클래스 (이름, HP, 공격력, 공허수 여부)
  - 플레이어 행동: Attack / Burn / Defend / Flee
  - 기억 연소 스킬 5등급 (Ember → Zero Burn)
  - 공허수 = 일반 공격 불가, 기억 연소만 유효
  - 도주 확률 70%, 공허수 도주 불가
- [x] **전투 씬 (battle_scene):** 코드 생성 UI
  - 적 HP바 (우상단) + 플레이어 HP바 (좌하단)
  - 적 플레이스홀더 스프라이트 (붉은 사각형 + 눈)
  - 행동 버튼 4개 (ATTACK / BURN / DEFEND / FLEE)
  - BURN 선택 → 사용 가능한 기억 목록 팝업
  - 전투 로그 (중앙 텍스트)
- [x] **전투 트리거:** 림 외곽 숲 맵에 2개 배치
  - 남쪽 길: Ash Crawler (일반 몬스터, HP 40, ATK 8)
  - 북쪽 숲: Void Beast (공허수, HP 80, ATK 15, 일반 공격 불가)
  - Area2D + body_entered로 자동 전투 진입

### 변경된 파일
- `assets/portraits/arrel_neutral.jpg` — **신규** 아렐 포트레이트
- `assets/portraits/arrel_side.jpg` — **신규**
- `assets/portraits/arrel_wounded.jpg` — **신규**
- `assets/portraits/elia_neutral.jpg` — **신규** 엘리아 포트레이트
- `assets/portraits/elia_concern.jpg` — **신규**
- `assets/cg/cover.png` — **신규** 커버 이미지
- `scripts/ui/dialogue_box.gd` — TextureRect 포트레이트 + 이미지 매핑 추가
- `scripts/systems/battle_manager.gd` — **신규** 전투 로직
- `scenes/battle/battle_scene.tscn` — **신규** 전투 씬
- `scenes/battle/battle_scene.gd` — **신규** 전투 씬 UI
- `scenes/maps/rim_forest.gd` — 전투 트리거 영역 2개 추가
- `project.godot` — BattleManager 오토로드 추가

### 다음 세션 (S06) 할 일
- [x] Godot 실행 테스트 → 완료 (Color alpha 누락 버그 수정 후 실행 성공)
- [x] 전투 후 HP 회복 → S06에서 완료
- [x] 세이브/로드 → S06에서 완료
- [x] NPC 상호작용 버그 → S06에서 수정

### 메모
- 포트레이트 매핑: PORTRAIT_MAP dict에 키→경로. DEFAULT_PORTRAITS로 화자별 기본값 설정.
- 전투 씬은 SceneTransition으로 전환. 전투 종료 후 return_scene으로 자동 복귀.
- 기억 연소 스킬 데미지 = BURN_SKILLS[grade].base_damage + memory.burn_power
- 공허수(Void Beast)는 is_void_beast=true → player_attack()에서 0 데미지 처리.

---

## S06 — 2026-04-05 (NPC 버그 수정 + HP 회복 + 세이브/로드)

### 완료
- [x] **버그 수정: NPC 상호작용 안 됨**
  - InteractionRay에 `force_raycast_update()` 추가 (입력 시점 동기화)
  - ray 길이 20px → 32px 증가 (NPC 감지 범위 확대)
- [x] **전투 후 HP 회복:** 승리 시 max_hp의 20% 회복
- [x] **세이브/로드 시스템 (SaveManager 오토로드)**
  - 3슬롯 JSON 저장 (user://saves/)
  - F6 = 퀵세이브, F7 = 퀵로드
  - 저장: player_data, story_flags, chapter, memory 상태, 현재 씬
  - GameManager/MemoryManager에 import_data() 추가
- [x] **tscn uid 제거:** 모든 씬에서 가짜 uid 제거 (Godot 자동 생성에 맡김)
- [x] **tscn Color alpha 수정:** Color(r,g,b) → Color(r,g,b,a)

### 변경된 파일
- `scripts/core/player.gd` — force_raycast_update + ray 32px
- `scenes/player/player.tscn` — InteractionRay target_position 32
- `scripts/systems/battle_manager.gd` — 승리 시 HP 20% 회복
- `scripts/systems/save_manager.gd` — **신규** 세이브/로드
- `scripts/core/game_manager.gd` — export_data/import_data 추가
- `scripts/systems/memory_manager.gd` — import_data 추가
- `project.godot` — SaveManager 오토로드
- 모든 .tscn — uid 제거, Color alpha 수정

### 다음 세션 (S07) 할 일
- [ ] Godot 재실행 + NPC 대화 테스트
- [ ] 타이틀 화면 (Cover.png, 새 게임/이어하기)
- [ ] CG 이벤트 시스템 (풀스크린 CG 표시)

### 메모
- SaveManager는 F6/F7 키를 _unhandled_input에서 직접 처리 (input map 불필요).
- 세이브 파일 경로: user://saves/save_1.json ~ save_3.json
- get_save_info()로 슬롯 선택 UI에서 저장 시간/챕터/연소 수 표시 가능.

---

## S07 — 2026-04-05 (타이틀 화면 + CG 이벤트 시스템)

### 완료
- [x] **타이틀 화면:** main.tscn을 Cover.png 배경 타이틀로 전환
  - New Game / Continue / Quit 버튼
  - Continue는 세이브 없으면 비활성
  - New Game 시 기억/플래그 초기화
  - main_scene을 main.tscn으로 복원
- [x] **CG 이벤트 시스템 (CgViewer 오토로드)**
  - show_cg(path, text, auto_close, callback) — 풀스크린 CG 표시
  - 페이드 인/아웃, 텍스트 오버레이, Space로 닫기
  - DialogueManager 연동: 대화 라인에 "cg" 키 있으면 자동 CG 표시
- [x] **CG 이미지 2장 복사:** ch1_forest.jpg, ch1_ash_rain.jpg

### 변경된 파일
- `scenes/main/main.tscn` — 타이틀 화면으로 전면 재작성
- `scenes/main/main.gd` — 타이틀 메뉴 로직
- `scripts/ui/cg_viewer.gd` — **신규** CG 이벤트 시스템
- `assets/cg/ch1_forest.jpg` — **신규** Chapter 1 숲 CG
- `assets/cg/ch1_ash_rain.jpg` — **신규** Chapter 1 재비 CG
- `project.godot` — CgViewer 오토로드 + main_scene 복원

### 다음 세션 (S08) 할 일
- [x] Chapter 1 스토리 흐름 연결 → S08에서 완료
- [x] 재비 파티클 이펙트 → S08에서 완료

### 메모
- CgViewer는 layer 45. MemoryUI(40) < CgViewer(45) < DialogueBox(50) < SystemLog(60) < SceneTransition(100).
- 대화 JSON에서 CG 사용: {"speaker": "", "text": "...", "cg": "res://assets/cg/ch1_forest.jpg", "cg_text": "The forest stretched..."}
- 오토로드 총 10개: GameManager, MemoryManager, DialogueManager, SceneTransition, DialogueBox, MemoryUI, SystemLog, BattleManager, SaveManager, CgViewer

---

## S08 — 2026-04-05 (Chapter 1 스토리 흐름 + 재비 파티클)

### 완료
- [x] **재비(Ash Rain) 파티클:** GPUParticles2D 기반, 회색 플레이크 하강, Player에 부착
  - ParticleProcessMaterial: 느린 하강 + turbulence 좌우 흔들림
  - GradientTexture로 서서히 투명해지는 효과
  - set_intensity()로 강도 조절 가능
- [x] **Chapter 1 스토리 시퀀스:** flag 기반 자동 진행
  - 1. 맵 진입 → opening_void_beast 대화 + 숲 CG (자동)
  - 2. opening 종료 → elia_appears 대화 (자동)
  - 3. elia 종료 → 재비 시작 + ash_rain 대화 + 재비 CG (자동)
  - 4. ash_rain 종료 → 자유 탐색 (전투, NPC 재대화 가능)
  - 5. 남쪽 끝 도달 → camp_night 대화 + 선택지
  - 6. camp 종료 → ch1_complete 플래그, chapter=2
- [x] **대화 데이터 확장:** CG 키 추가, 엘리아 재대화(elia_talk) 추가
- [x] **전투 트리거 위치 조정:** 스토리 동선에 안 겹치도록 이동
- [x] **야영 트리거:** 남쪽 길 끝에 Area2D (ash_rain 본 후만 활성)

### 변경된 파일
- `scripts/effects/ash_rain.gd` — **신규** 재비 파티클
- `scenes/maps/rim_forest.gd` — 스토리 시퀀스 + 재비 + 야영 트리거
- `data/chapter1_dialogue.json` — CG 키 + 대화 확장

### 다음 세션 (S09) 할 일
- [x] Ch2 맵 + 스토리 → S09에서 완료
- [x] 엘리아 동행 시스템 → S09에서 완료

### 메모
- 스토리 시퀀스는 CONNECT_ONE_SHOT으로 연결. 대화 종료 시 다음 단계 자동 진행.
- 전투 트리거를 스토리 동선(중앙 길)에서 벗어나도록 좌상/우상으로 이동.

---

## S09 — 2026-04-05 (엘리아 동행 + Ch2 베르단 시장)

### 완료
- [x] **엘리아 동행 시스템 (Companion)**
  - CharacterBody2D 기반, 플레이어를 따라다님
  - MIN_DISTANCE(40px) 이내 멈춤, MAX_DISTANCE(200px) 초과 시 텔레포트
  - 플레이스홀더 스프라이트 (은발 + 파란 눈)
  - interact() 지원 (대화 가능)
  - Player에 "player" 그룹 추가 → get_first_node_in_group()으로 탐색
- [x] **Ch2 베르단 시장 맵 (30x20 타일)**
  - 돌바닥/벽/노점/골목 5종 타일
  - 말렛 NPC 배치 (The Sump 골목 안쪽)
  - 엘리아 Companion 배치
  - 도착 대화 자동 시작
- [x] **Ch2 대화 데이터**
  - verdan_arrival: 베르단 도착 묘사
  - malet_encounter: 말렛 등장 + 거래 제안 (Grade 2 기억 요구)
  - malet_deal: 선택지 (수락/거절) + 기억 추출 장면
  - malet_reward: BL-07 경로, 세이블 이름, 카이로스 경고
  - elia_ch2_talk: 엘리아 재대화
- [x] **Ch1→Ch2 자동 전환:** camp_night 종료 → verdan_market 씬 전환

### 변경된 파일
- `scripts/core/companion.gd` — **신규** 동행 NPC
- `scenes/npc/companion.tscn` — **신규** 동행 NPC 씬
- `scenes/maps/verdan_market.gd` — **신규** Ch2 맵
- `scenes/maps/verdan_market.tscn` — **신규** Ch2 맵 씬
- `data/chapter2_dialogue.json` — **신규** Ch2 대화
- `scenes/maps/rim_forest.gd` — Ch2 전환 추가
- `scripts/core/player.gd` — "player" 그룹 추가

### 다음 세션 (S10) 할 일
- [x] 시각적 품질 개선 → S10에서 완료

### 메모
- Companion은 collision_layer=4(Interactables), Player의 InteractionRay mask가 감지 가능.
- 말렛 대화에서 "Accept the deal" 선택 시 identity_first_sword 기억 자동 연소.
- Ch2 맵의 골목(ALLEY) 타일은 The Sump 입구를 표현.

---

## S10 — 2026-04-05 (시각적 품질 개선)

### 완료
- [x] **전투 배경 이미지:** BattleManager에 bg_image/enemy_image 경로 지원, 전투 씬에서 TextureRect로 표시
- [x] **공허수 이미지:** void_beast.jpg → 전투 시 실제 크리처 이미지 표시 (ColorRect 대체)
- [x] **전투 배경:** 림 외곽 숲 전투 시 ch1_forest.jpg 배경
- [x] **타이틀 버튼 스타일링:** 다크 판타지 스타일 (어두운 배경, 골드 테두리, 호버 강조, 18px 폰트)
- [x] **대화 나레이션 스타일 분리:** 나레이션=어두운 크림색, 시스템 로그=청록색, 일반 대사=밝은 크림색
- [x] **Ch2 도착 CG:** verdan_arrival 첫 대사에 ch2_verdan.jpg CG 연결
- [x] **CG 이미지 3장 추가:** void_beast.jpg, ch2_verdan.jpg, village_seam.jpg

### 변경된 파일
- `assets/cg/void_beast.jpg` — **신규**
- `assets/cg/ch2_verdan.jpg` — **신규**
- `assets/cg/village_seam.jpg` — **신규** (Ch3 대비)
- `scripts/systems/battle_manager.gd` — bg_image/enemy_image 지원
- `scenes/battle/battle_scene.gd` — 배경 이미지 + 적 이미지 TextureRect
- `scenes/maps/rim_forest.gd` — 전투 트리거에 이미지 경로 전달
- `scenes/main/main.gd` — 버튼 스타일링
- `scripts/ui/dialogue_box.gd` — 나레이션/시스템 로그 색상 분리
- `data/chapter2_dialogue.json` — 도착 CG 추가

### 다음 세션 (S11) 할 일
- [x] 새 이미지 배치 + Ch2 완료 → S11에서 완료

---

## S11 — 2026-04-05 (새 이미지 배치 + Ch2 거래 흐름 완성)

### 완료
- [x] **포트레이트 6장 추가:** 말렛, 카이로스, 세이블(2장), 아렐 angry/pain
- [x] **CG 4장 추가:** 카이로스 경고, 기억 추출, Ch1 녹색 나무, Ash Crawler
- [x] **PORTRAIT_MAP 확장:** 5캐릭터 11장 포트레이트, DEFAULT_PORTRAITS 5명
- [x] **Ch2 말렛 거래 흐름 완성:**
  - malet_encounter → malet_deal(선택지) → malet_deal 추출 CG → malet_reward + 카이로스 경고 CG
  - 거절 시 malet_refused 대화 → 재대화로 재시도 가능
  - 모든 대사에 포트레이트 키 연결
- [x] **Ch1 녹색 나무 히든 CG:** camp 종료 → 3초간 녹색 나무 CG → Ch2 전환
- [x] **Ash Crawler 전투 이미지:** 절지류 크리처 이미지 전투 씬에 표시

### 변경된 파일
- `assets/portraits/` — 6장 추가 (malet, kairos, sable x2, arrel angry/pain)
- `assets/cg/` — 4장 추가 (kairos_warning, extraction, green_tree, ash_crawler)
- `scripts/ui/dialogue_box.gd` — PORTRAIT_MAP 11개 + DEFAULT_PORTRAITS 5명
- `data/chapter2_dialogue.json` — 포트레이트 키 + CG + malet_refused 추가
- `scenes/maps/verdan_market.gd` — 말렛 거래 자동 연결 흐름
- `scenes/maps/rim_forest.gd` — 녹색 나무 CG + Ash Crawler 이미지

### 에셋 현황
- 포트레이트: 11장 (아렐 5, 엘리아 2, 말렛 1, 카이로스 1, 세이블 2)
- CG: 9장 (cover, ch1 x3, ch2 x3, void_beast, ash_crawler)

### 다음 세션 (S12) 할 일
- [x] Ch3 콘텐츠 → S12에서 완료

---

## S12 — 2026-04-05 (Ch2→Ch3 전환 + 크럼블링 코스트 맵)

### 완료
- [x] **Ch2 → Ch3 전환:** 말렛 보상 대화 후 자동 크럼블링 코스트 이동
- [x] **크럼블링 코스트 맵 (25x18):** 바위/모래/절벽/물/길 5종 타일, 물 파도 디테일
- [x] **Ch3 스토리 시퀀스:**
  - 도착 대화 + 크럼블링 코스트 CG
  - 카이로스 목격 이벤트 (2초 딜레이 후 자동 + CG)
  - 북쪽 도달 → The Seam 도착 + 세이블 첫 만남 + village CG
- [x] **Ch3 대화 데이터:** coast_arrival, kairos_sighting, seam_arrival, elia_coast_talk
- [x] **Coastal Void Beast 전투:** HP 100, ATK 18 (강화 공허수)
- [x] **크럼블링 코스트 BGM:** dialogue_tense.mp3 (Morning Light)

### 변경된 파일
- `scenes/maps/verdan_market.gd` — Ch3 전환 + chapter=3
- `scenes/maps/crumbling_coast.tscn` — **신규** Ch3 맵
- `scenes/maps/crumbling_coast.gd` — **신규** Ch3 맵 스크립트
- `data/chapter3_dialogue.json` — **신규** Ch3 대화
- `scripts/systems/audio_manager.gd` — 크럼블링 코스트 BGM 매핑

### 전체 플레이 흐름 (Ch1~Ch3)
타이틀 → Ch1 림 외곽 숲 (오프닝→엘리아→재비→전투→야영→녹색나무) → Ch2 베르단 시장 (도착→말렛 거래→추출→보상+카이로스 경고) → Ch3 크럼블링 코스트 (도착→카이로스 목격→전투→The Seam 도착+세이블)

### 다음 세션 (S13) 할 일
- [x] The Seam 맵 (Ch4 세이블 거점) → S13에서 완료
- [x] 전투 시스템 확장 (적 다양화, 보스전) → S13에서 완료

---

## S13 — 2026-04-05 (The Seam 맵 + Ch4 + 전투 확장)

### 완료
- [x] **The Seam 맵 (25x18):** 절벽/돌/오두막/정원/길/물/랜턴 7종 타일
  - 랜턴: 따뜻한 빛 효과 (중심점 + 은은한 후광)
  - 정원: 랜덤 색상 꽃 디테일 (붉은/노란/보라 — 세상에서 사라져가는 색들)
  - 물: 작은 개울 + 파도 디테일
- [x] **Ch4 스토리 시퀀스:**
  - seam_welcome: The Seam 도착 + village CG
  - sable_briefing: BL-07 보이드 홀 조사 미션 + BL-07 내부 CG
  - bl07_entrance: 보이드 홀 앞 + void_portal CG → Shade Sentinel 보스전
  - bl07_aftermath: 보스전 후 에필로그 → Ch5 전환
- [x] **Ch4 대화 데이터:** seam_welcome, sable_briefing, bl07_entrance, bl07_aftermath, sable_talk, elia_seam_talk
- [x] **Ch3→Ch4 전환:** crumbling_coast.gd _on_seam_ended()에서 The Seam으로 자동 전환
- [x] **전투 시스템 확장:**
  - Enemy 클래스에 is_boss, phase, abilities 추가
  - 보스 페이즈 시스템: HP 50% 이하 → 페이즈 2 전환 + 알림
  - 적 특수 능력 3종: drain(HP 흡수), shield(방어 배리어), multi_hit(2연타)
  - 보스/공허수 도주 불가
  - enemy_shielded: 일반 공격 50% 감소, 번 스킬 30% 감소
- [x] **Shade Sentinel 보스전:** HP 180, ATK 24, 공허수+보스, 능력 3종 모두 보유
- [x] **Coastal Void Beast 능력 추가:** drain 능력
- [x] **The Seam BGM:** exploration.mp3 (Sunrise Over Waves) 매핑

### 변경된 파일
- `scenes/maps/the_seam.tscn` — **신규** Ch4 맵
- `scenes/maps/the_seam.gd` — **신규** Ch4 맵 스크립트
- `data/chapter4_dialogue.json` — **신규** Ch4 대화
- `scenes/maps/crumbling_coast.gd` — Ch4 전환 + Void Beast drain 능력
- `scripts/systems/battle_manager.gd` — 보스/페이즈/특수능력 시스템
- `scripts/systems/audio_manager.gd` — The Seam BGM 매핑

### 전체 플레이 흐름 (Ch1~Ch4)
타이틀 → Ch1 림 외곽 숲 (오프닝→엘리아→재비→전투→야영→녹색나무) → Ch2 베르단 시장 (도착→말렛 거래→추출→보상+카이로스 경고) → Ch3 크럼블링 코스트 (도착→카이로스 목격→전투→The Seam 도착+세이블) → Ch4 The Seam (도착→세이블 브리핑→BL-07 보스전→에필로그)

### 에셋 현황
- 포트레이트: 11장 (아렐 5, 엘리아 2, 말렛 1, 카이로스 1, 세이블 2)
- CG: 18장 (cover, ch1 x4, ch2 x4, ch3 x1, village_seam, bl07_interior, void_portal, void_beast, ash_crawler, arrel_combat, item x2)
- BGM: 7트랙 (title, ch1_forest, ch1_camp, ch2_verdan, battle, dialogue_tense, exploration)

### 다음 세션 (S14) 할 일
- [x] Ch5 콘텐츠 → S14에서 완료
- [x] 기억 시스템 확장 → S14에서 완료

---

## S14 — 2026-04-05 (Ch5 The Seal + 기억 시스템 확장)

### 완료
- [x] **S13 버그 수정:** the_seam.gd _setup_battle_triggers() 호출 누락 수정
- [x] **챕터별 기억 추가 시스템:** MemoryManager.add_chapter_memories(chapter)
  - Ch3: "Salt Wind on the Cliffs" (G5), "Walking With Someone" (G4, Elia 관련)
  - Ch4: "The Woman Who Came Back" (G3, Sable 관련), "Colors That Shouldn't Exist" (G5), "Flowers From Every Season" (G4)
  - Ch5: "What You Saw Inside BL-07" (G2, 보이드 경험)
- [x] **BL-07 보이드 내부 맵 (20x20):** 허공/부유파편/길/균열/핵심부 5종 타일
  - 핵심부 맥동 효과 (_process에서 sin 기반 색상 변화)
  - 부유 파편 시각 디테일
  - 2개 전투 트리거: Void Fragment (HP 70), Memory Eater (HP 90, drain+multi_hit)
- [x] **Ch5 스토리 시퀀스 (The Seal 결정):**
  - void_entry: BL-07 진입 + void_portal CG
  - void_core: 핵심부 도달 + 아렐의 깨달음
  - seal_decision: **플레이어 선택** — "이름을 태워 봉인" vs "이름을 지키고 다른 방법을 찾기"
  - seal_complete: Zero Burn 경로 (이름 상실, 엘리아를 알아보지 못함)
  - seal_refused: 보존 경로 (BL-07 미봉인, 시간과의 싸움)
- [x] **Ch5 대화 데이터:** void_entry, void_core, seal_decision, seal_complete, seal_refused, elia_void_talk
- [x] **Ch4→Ch5 전환:** the_seam.gd 보스전 에필로그 → BL-07 내부 자동 전환
- [x] **BL-07 BGM 매핑:** dialogue_tense.mp3
- [x] **선택지 flag 키 수정:** chapter5에서 "flag" → "set_flag" (DialogueManager 호환)

### 변경된 파일
- `scenes/maps/bl07_void.tscn` — **신규** Ch5 맵
- `scenes/maps/bl07_void.gd` — **신규** Ch5 맵 스크립트 (The Seal 메카닉)
- `data/chapter5_dialogue.json` — **신규** Ch5 대화 (분기 엔딩)
- `scripts/systems/memory_manager.gd` — 챕터별 기억 추가 + _has_memory
- `scenes/maps/crumbling_coast.gd` — Ch3 기억 자동 추가
- `scenes/maps/the_seam.gd` — Ch4 기억 추가 + _setup_battle_triggers 호출 + Ch5 전환
- `scripts/systems/audio_manager.gd` — BL-07 BGM 매핑

### 전체 플레이 흐름 (Ch1~Ch5)
타이틀 → Ch1 림 외곽 숲 → Ch2 베르단 시장 → Ch3 크럼블링 코스트 → Ch4 The Seam (세이블 브리핑→보스전) → Ch5 BL-07 내부 (탐색→핵심부→**The Seal 선택**)
- **Zero Burn 경로:** 이름 연소 → BL-07 봉인 → 아렐의 정체성 상실
- **보존 경로:** 이름 보존 → BL-07 미봉인 → 다른 방법 탐색

### 기억 총 현황 (최대)
| 등급 | 초기 | Ch3 추가 | Ch4 추가 | Ch5 추가 | 계 |
|------|------|----------|----------|----------|-----|
| G5 | 2 | 1 | 1 | 0 | 4 |
| G4 | 2 | 1 | 1 | 0 | 4 |
| G3 | 1 | 0 | 1 | 0 | 2 |
| G2 | 1 | 0 | 0 | 1 | 2 |
| G1 | 1 | 0 | 0 | 0 | 1 |
| **계** | **7** | **2** | **3** | **1** | **13** |

### 다음 세션 (S15) 할 일
- [x] 그래픽 업그레이드 → S15에서 완료

---

## S15 — 2026-04-05 (그래픽 대규모 업그레이드)

### 완료
- [x] **PixelSprite 유틸리티 (pixel_sprite.gd):**
  - Image.set_pixel() 기반 상세 픽��아트 캐릭터 생성
  - 32x32 프레임, 4방향 x (idle + 4 walk) = 20프레임 완전 애니메이션
  - 머리카락 텍스처/하이라이트, 눈(홍채+하이라이트), 코/입 힌트
  - 코트 주름/밝은면, 팔 스윙, 발 스텝 애니메이션
  - 프리셋: arrel_config (은청 머리+남색 코트+검), elia_config (은발+갈색 망토+브로치), sable_config (짧��� 검은 머리+실용복+흉터)
  - npc_config(color) — npc_color 기반 자동 생성
- [x] **TilePainter 유틸리티 (tile_painter.gd):**
  - TileMapLayer + TileSetAtlasSource 기반 맵 렌더링
  - 20종 타일 디테일: grass(풀잎+꽃), tree(줄기+수관), bush, water(파도라인), path(자갈), stone(줄눈), wall(벽돌패턴), stall(천막+물건), door, alley(물웅덩이), sand(바람자국), cliff(균열선), rock, hut(지붕+문+창), garden(색색의 꽃), lantern(빛+후광), void(에너지점), fragment(부유파편), crack(보라 균열), core(맥동)
  - create_tilemap() → TileMapLayer 반환
  - add_collisions() → 벽 충돌 StaticBody2D 일괄 생성
- [x] **캐릭터 스프라이트 교체:**
  - player.gd → PixelSprite.arrel_config() (120줄 삭제)
  - companion.gd → Sprite2D→AnimatedSprite2D 전환 + PixelSprite (방향 애니메이션 추가)
  - npc.gd → PixelSprite.npc_config(npc_color), 이름별 전용 config (Sable, Malet)
- [x] **맵 5개 TileMap 전환:**
  - rim_forest.gd — ColorRect 50줄 → TilePainter 10줄
  - verdan_market.gd — ColorRect 35줄 → TilePainter 10줄
  - crumbling_coast.gd — ColorRect 30줄 → TilePainter 8줄
  - the_seam.gd — ColorRect 60줄 (랜턴/정원 포함) → TilePainter 10줄
  - bl07_void.gd — ColorRect + 맥동 → TilePainter + 코어 오버레이

### 변경된 파일
- `scripts/utils/pixel_sprite.gd` — **신규** 캐릭터 스프라이트 유틸리티
- `scripts/utils/tile_painter.gd` — **신규** 타일맵 생성 유틸리티
- `scripts/core/player.gd` — 스프라이트 코드 120줄 ��� 2줄
- `scripts/core/companion.gd` — AnimatedSprite2D + 방향 애니메이션
- `scripts/core/npc.gd` — PixelSprite 기반 생성
- `scenes/maps/rim_forest.gd` — TileMap 전환
- `scenes/maps/verdan_market.gd` — TileMap 전환
- `scenes/maps/crumbling_coast.gd` — TileMap 전환
- `scenes/maps/the_seam.gd` — TileMap 전환
- `scenes/maps/bl07_void.gd` — TileMap 전환

### 그래픽 개선 요약
| Before | After |
|--------|-------|
| 플랫 ColorRect 타일 | 텍스처 있는 TileMap (풀잎, 벽돌, 파도, 꽃 등) |
| 사각형 블록 캐릭터 | 픽���아트 캐릭터 (머리/눈/코/입/옷 디테일) |
| 정적 엘리아 (Sprite2D 1장) | 4방향 걷기 애니메이션 엘리아 |
| 맵당 수백 개 ColorRect 노드 | TileMapLayer 1개 (성능 대폭 개선) |

### 다음 세션 (S16) 할 일
- [ ] Godot 전체 테스트 (그래픽 확인 + Ch1→Ch5)
- [ ] Ch6 콘텐츠 (분기 후 에필로그)
- [ ] UI 테마 개선

---

## S16 — 2026-04-05 (에필로그 + UI 테마 개선)

### 완료
- [x] **전투씬 BGM 제거:**
  - audio_manager.gd: SCENE_BGM에서 battle_scene 매핑 제거
  - 전투씬 진입 시 stop_bgm() 호출 (무음 전투)
- [x] **Ch6 에필로그 대화 데이터:**
  - `data/chapter6_dialogue.json` — 4개 대화 키
  - epilogue_zero_burn: 이름을 잃은 아렐 (34줄, Elia/Sable 대화)
  - epilogue_preservation: 이름을 지킨 아렐 (24줄, 보이드 홀 미해결)
  - elia_epilogue_talk: 보이드 홀의 패턴 발견 (기억 연소가 원인?)
  - sable_epilogue_talk: 동쪽 정착촌 기억 유실 → 후속작 암시
- [x] **Ch6 에필로그 트리거 연결 (the_seam.gd):**
  - Ch5 완료 후 The Seam 복귀 시 자동 에필로그 시작
  - zero_burn_path / seal_refused 플래그에 따라 분기
  - 에필로그 후 Elia/Sable NPC 개별 대화 트리거
  - 에필로그 시 플레이어 위치를 마을 중앙으로 변경
- [x] **UI 테마 개선:**
  - `scripts/utils/ui_theme.gd` — 공통 UI 색상/스타일 상수 유틸리티
  - 캐릭터별 화자 이름 색상 (Arrel=은청, Elia=라벤더, Sable=보라, Malet=앰버, Kairos=청록)
  - DialogueBox에 UITheme 적용 (화자별 이름 색상 + 나레이션/시스템 색상)
  - 전투 HP바 애니메이션 (데미지 시 Tween 부드러운 감소)
  - HP 25% 이하 시 바 색상 빨간색 전환

### 변경된 파일
- `scripts/systems/audio_manager.gd` — 전투 BGM 제거
- `data/chapter6_dialogue.json` — **신규** 에필로그 대화 데이터
- `scenes/maps/the_seam.gd` — Ch6 에필로그 트리거 + NPC 대화
- `scripts/utils/ui_theme.gd` — **신규** 공통 UI 테마 유틸리티
- `scripts/ui/dialogue_box.gd` — 화자별 이름 색상 적용
- `scenes/battle/battle_scene.gd` — HP바 트위닝 + 저HP 색상 변환

### 스토리 완결 구조
```
Ch1 림 외곽 숲 → Ch2 베르단 시장 → Ch3 크럼블링 코스트
→ Ch4 The Seam → Ch5 BL-07 보이드
→ Ch6 에필로그 (Zero Burn / Preservation 분기)
→ NPC 후일담 대화 (Elia: 보이드 홀 원인 발견, Sable: 후속 탐사 암시)
```

### 다음 세션 (S17) 할 일
- [ ] Godot 전체 테스트 (Ch1→Ch6, 양 분기 엔딩 확인)
- [ ] 전투 밸런스 조정 (필요 시)
- [ ] 추가 CG/이미지 배치 (에필로그용)

---

## S17 — 2026-04-05 (전체 테스트 + 버그 수정 + 밸런스)

### 발견된 버그 및 수정
- [x] **전투 재트리거 (Critical):**
  - 문제: 전투 영역에 다시 들어가면 무한 전투 발생
  - 수정: 4개 맵 모두 `_battle_counter` + 1회성 플래그(`battle_rim_1` 등) 추가
  - 영향: rim_forest.gd, crumbling_coast.gd, the_seam.gd, bl07_void.gd
- [x] **게임 오버 소프트 록 (Critical):**
  - 문제: 패배 시 HP 0으로 맵 복귀 → 이후 전투 불가
  - 수정: 패배 시 HP 30% 회복 + "Something pulls you back..." 메시지
- [x] **보스전 시그널 크래시 (Critical):**
  - 문제: the_seam.gd에서 `battle_ended.connect(_on_boss_defeated)` 후 씬 전환 → 노드 해제 → 콜백 크래시
  - 수정: 시그널 대신 플래그 기반 감지 (맵 재진입 시 `ch4_bl07_entered && !ch4_complete` 체크)
  - 추가: 패배 시 `ch4_bl07_entered` 플래그 리셋으로 재도전 가능
- [x] **대화 CG 입력 충돌 (Medium):**
  - 문제: CG 표시 시 입력 대기(waiting_for_input) + 대화 입력 처리 동시 발생
  - 수정: 대화 중 CG는 `_show_cg_background()` (non-blocking) 사용, 대화 끝나면 자동 닫기
- [x] **동행 캐릭터 애니메이션 (Minor):**
  - 문제: 엘리아가 멈추면 마지막 걷기 프레임에서 정지 (idle 미전환)
  - 수정: `dist < MIN_DISTANCE` 시 idle 애니메이션 재생
- [x] **챕터 기억 누락 (Minor):**
  - 문제: the_seam.gd에서 Ch4 기억이 `_start_ch4_sequence()`에서만 추가 → 세이브 로드 시 누락
  - 수정: `_ready()`에서 `add_chapter_memories(4)` 호출 (중복 방지는 `_has_memory()`)

### 밸런스 조정
- [x] **보이드 적 일반 공격 가능:**
  - 변경 전: 보이드 적 = 일반 공격 완전 무효 → 기억 소진 시 진행 불가
  - 변경 후: 보이드 적에게 30% 감쇠 데미지 ("Your blade struggles against the void...")
- [x] **챕터별 플레이어 성장:**
  - 기본 공격력: 15 + (chapter-1) × 3 (Ch1=15, Ch5=27)
  - 최대 HP: 100 + (chapter-1) × 15 (Ch1=100, Ch5=160)
  - 전투 시작 시 자동 적용 (HP 성장 시 15 회복)

### 밸런스 테이블
| 챕터 | 플레이어 HP | 공격력 | 적 HP | 적 ATK | 특수 |
|------|-----------|--------|-------|--------|------|
| Ch1  | 100       | 15-25  | 40/80 | 8/15   | Void Beast |
| Ch3  | 130       | 21-31  | 100   | 18     | Void+Drain |
| Ch4  | 145       | 24-34  | 60/180| 14/24  | Boss: 3 abilities |
| Ch5  | 160       | 27-37  | 70/90 | 16/20  | Drain+Multi |

### 변경된 파일
- `scenes/maps/rim_forest.gd` — 전투 1회성 플래그
- `scenes/maps/crumbling_coast.gd` — 전투 1회성 플래그
- `scenes/maps/the_seam.gd` — 전투 플래그 + 보스전 플래그 기반 감지 + 기억 추가 위치 변경
- `scenes/maps/bl07_void.gd` — 전투 1회성 플래그
- `scripts/systems/battle_manager.gd` — 패배 HP 회복, 보이드 감쇠, 챕터별 성장
- `scripts/ui/cg_viewer.gd` — 대화 중 CG non-blocking 모드
- `scripts/core/companion.gd` — idle 애니메이션 수정

### 다음 세션 (S18) 할 일
- [ ] Godot 실행 테스트 (Ch1→Ch6, 양 분기 실제 플레이)
- [ ] 추가 CG/이미지 (에필로그용)
- [ ] 사운드 추가 (전투 효과음 확장)

---

## S18 — 2026-04-05 (CG 보완 + SFX 확장 + 연출 강화)

### 완료
- [x] **CG 이미지 연결 보완:**
  - `item_memory_ampoule.jpg` → Ch5 seal_decision (보이드 홀 맥동 장면), seal_complete (백색 불꽃 장면)
  - `item_extractor.jpg` → Ch2 malet_encounter (말렛의 가격 제시 장면)
  - 전체 17장 CG 100% 연결 완료
- [x] **SFX 6종 추가 (총 12종):**
  - `shield` — 적 방어막 (저음 울림)
  - `drain` — 생명력 흡수 (역방향 스윕)
  - `phase_change` — 보스 페이즈 전환 (깊은 공명)
  - `defeat` — 플레이어 패배 (하강 톤)
  - `flee` — 도주 (빠른 상승음)
  - `memory_add` — 기억 획득 (맑은 C-E-G 화음)
  - `void_pulse` — 보이드 맥동 (불안한 저주파)
- [x] **SFX 연결:**
  - battle_manager.gd: drain, shield, phase_change, defeat, flee
  - memory_manager.gd: memory_add (add_memory 시 자동 재생)
  - bl07_void.gd: void_pulse (Seal 결정 장면)
- [x] **Ch5 Seal 연출 강화:**
  - Zero Burn: void_pulse SFX → 화면 백색 플래시 (1.2초) → 서서히 복귀 → seal_complete 대화
  - Preservation: void_pulse SFX → 짧은 딜레이 → seal_refused 대화

### 변경된 파일
- `data/chapter2_dialogue.json` — item_extractor.jpg CG 추가
- `data/chapter5_dialogue.json` — item_memory_ampoule.jpg CG 2곳 추가
- `scripts/systems/audio_manager.gd` — SFX 6종 추가 (shield, drain, phase_change, defeat, flee, memory_add, void_pulse)
- `scripts/systems/battle_manager.gd` — 새 SFX 연결 (drain, shield, phase_change, defeat, flee)
- `scripts/systems/memory_manager.gd` — memory_add SFX (기억 추가 시)
- `scenes/maps/bl07_void.gd` — Seal 장면 연출 강화 (void_pulse + 백색 플래시)

### 전체 에셋 현황
| 카테고리 | 수량 | 상태 |
|---------|------|------|
| CG 이미지 | 17장 | 100% 연결 |
| 포트레이트 | 11장 | 100% 연결 |
| SFX | 12종 | 코드 생성 (외부 파일 불필요) |
| BGM | 5곡 | 씬별 매핑 (전투=무음) |

### 다음 세션 (S19) 할 일
- [x] BGM 확장 → S19에서 완료
- [x] 타이틀 화면 흐름 정리 → S19에서 완료
- [x] 코드 검증 → S19에서 완료

---

## S19 — 2026-04-06 (BGM 확장 + 타이틀 수정 + 코드 검증)

### 완료
- [x] **BGM 3트랙 추가 (총 10트랙):**
  - `epilogue.mp3` (Quiet Gravity) → Ch6 에필로그 전용 BGM
  - `ch5_void.mp3` (Invisible Room) → BL-07 보이드 내부 전용 BGM (dialogue_tense에서 교체)
  - `battle_theme.mp3` (Raindrops in D Minor) → 전투 BGM 복원 (무음→BGM)
- [x] **AudioManager BGM 매핑 확장:**
  - BL-07: dialogue_tense → ch5_void.mp3 전환 (보이드 분위기 강화)
  - 전투씬: stop_bgm() → play_bgm("battle_theme.mp3") (전투 BGM 복원)
  - The Seam 에필로그: 진입 시 epilogue.mp3 수동 재생
- [x] **타이틀 New Game 초기화 버그 수정:**
  - 문제: max_hp, grains, elia_with_party가 리셋되지 않음 (이전 플레이 데이터 잔존)
  - 수정: player_data를 완전히 새 Dictionary로 교체
- [x] **코드 레벨 전체 검증:**
  - 전 스크립트 null 접근, 시그널 크래시, 정수 나눗셈 점검
  - 심각한 버그 없음 확인 (S17 수정이 잘 적용됨)

### 변경된 파일
- `assets/audio/bgm/epilogue.mp3` — **신규** (mugic/Quiet Gravity)
- `assets/audio/bgm/ch5_void.mp3` — **신규** (mugic/Invisible Room)
- `assets/audio/bgm/battle_theme.mp3` — **신규** (mugic/Raindrops in D Minor)
- `scripts/systems/audio_manager.gd` — BL-07 BGM 교체 + 전투 BGM 복원
- `scenes/maps/the_seam.gd` — 에필로그 BGM 재생
- `scenes/main/main.gd` — New Game player_data 완전 초기화

### 전체 BGM 현황
| BGM | 파일 | 사용처 |
|-----|------|--------|
| title.mp3 | 타이틀 | 타이틀 화면 |
| ch1_forest.mp3 | 림 외곽 숲 | Ch1 맵 |
| ch1_camp.mp3 | 야영 | Ch1 야영 장면 |
| ch2_verdan.mp3 | 베르단 시장 | Ch2 맵 |
| dialogue_tense.mp3 | 긴장 대화 | Ch3 크럼블링 코스트 |
| exploration.mp3 | 탐색 | Ch4 The Seam |
| ch5_void.mp3 | 보이드 내부 | Ch5 BL-07 |
| epilogue.mp3 | 에필로그 | Ch6 에필로그 |
| battle_theme.mp3 | 전투 | 모든 전투 씬 |
| battle.mp3 | (미사용) | 예비 |

### 다음 세션 (S20) 할 일
- [x] 폴리싱 → S20에서 완료

---

## S20 — 2026-04-06 (폴리싱 — 일시정지 메뉴 + 전투 피드백 + 환경 효과)

### 완료
- [x] **일시정지 메뉴 (PauseMenu 오토로드):**
  - ESC 키로 토글 (EXPLORATION 상태에서만)
  - Resume / Save (Slot 1) / Load (Slot 1) / Return to Title / Quit
  - 현재 챕터, HP, 기억 상태, 세이브 슬롯 정보 표시
  - 세이브 성공 피드백 ("SAVED!" 텍스트 전환)
  - CanvasLayer 55 (DialogueBox↔SystemLog 사이)
  - `process_mode = ALWAYS` — pause 중에도 작동
  - F6/F7 퀵세이브 힌트 표시
- [x] **전투 시각 피드백 (battle_scene.gd):**
  - 데미지 숫자: 피격 시 떠오르며 사라지는 텍스트 (적=노란, 플레이어=빨간)
  - 히트 플래시: 화면 전체 빨간/흰색 번쩍임 (0.2초 페이드)
  - 스크린 셰이크: 4프레임 랜덤 흔들림 (데미지마다 발동)
- [x] **맵 환경 효과 (MapEffects 유틸리티):**
  - `MapEffects.add_water_shimmer()` — 물 타일 위 반짝이는 반투명 라인 (sin 파동)
  - `MapEffects.add_lantern_lights()` — 랜턴 타일 주변 따뜻한 빛 (촛불 깜빡임)
  - `MapEffects.add_void_particles()` — GPUParticles2D 보라색 떠다니는 입자
  - The Seam: 물 반짝임 + 랜턴 빛
  - Crumbling Coast: 물 반짝임
  - BL-07 Void: 보이드 파티클

### 변경/생성된 파일
- `scripts/ui/pause_menu.gd` — **신규** 일시정지 메뉴 오토로드
- `scripts/utils/map_effects.gd` — **신규** 맵 환경 효과 유틸리티
- `project.godot` — PauseMenu 오토로드 추가
- `scenes/battle/battle_scene.gd` — 데미지 숫자 + 히트 플래시 + 스크린 셰이크
- `scenes/maps/the_seam.gd` — 물 반짝임 + 랜턴 빛 효과
- `scenes/maps/crumbling_coast.gd` — 물 반짝임 효과
- `scenes/maps/bl07_void.gd` — 보이드 파티클 효과

### 오토로드 현황 (12개)
| 이름 | 레이어 | 역할 |
|------|--------|------|
| GameManager | - | 게임 상태, 플래그, 플레이어 데이터 |
| MemoryManager | - | 기억 연소 시스템 |
| DialogueManager | - | 대화 진행 |
| SceneTransition | 100 | 페이드 인/아웃 |
| DialogueBox | 50 | 대화 UI |
| MemoryUI | 40 | 기억 서고 |
| SystemLog | 60 | 관리국 로그 팝업 |
| BattleManager | - | 전투 로직 |
| SaveManager | - | 세이브/로드 |
| CgViewer | 45 | CG 표시 |
| AudioManager | - | BGM/SFX |
| PauseMenu | 55 | **신규** 일시정지 메뉴 |

### 다음 세션 (S21) 할 일
- [x] 게임 오버 화면 → S21에서 완료
- [x] 히든 이벤트 → S21에서 완료
- [x] 코드 검증 → S21에서 완료

---

## S21 — 2026-04-06 (게임 오버 화면 + 히든 이벤트 + 플레이 검증)

### 완료
- [x] **게임 오버 화면 (game_over.tscn + .gd):**
  - 패배 시 자동 HP 복귀 대신 게임 오버 화면으로 전환
  - "You fell." + "Something pulls you back from the edge..."
  - 3개 선택지: Stagger On (HP 30%) / Load Save / Return to Title
  - BattleManager 상태 리셋 (IDLE + enemy null)
  - BattleManager._cleanup() 수정: defeat 시 game_over.tscn으로 전환
- [x] **히든 이벤트 2개:**
  - Ch1 림 외곽 숲 — "나무 그루터기" (A.E. 이니셜 각인, 아렐+엘리아 암시)
    - 맵 우측 하단, 1회성 트리거 (hidden_ch1_stump 플래그)
  - Ch4 The Seam — "숨겨진 정원" (금빛 꽃, 아이의 웃음소리 잔상)
    - 좌상단 정원 타일, 1회성 트리거 (hidden_ch4_garden 플래그)
- [x] **코드 레벨 전체 플레이 검증:**
  - Ch1→Ch6 양 분기 흐름 추적 완료
  - 게임 오버 → 재시도/로드/타이틀 흐름 검증
  - 보스전 패배 → 게임 오버 → 재시도 → 보스 재도전 흐름 검증
  - 히든 이벤트 트리거 논리 검증

### 변경/생성된 파일
- `scenes/ui/game_over.tscn` — **신규** 게임 오버 씬
- `scenes/ui/game_over.gd` — **신규** 게임 오버 로직
- `scripts/systems/battle_manager.gd` — 패배 시 game_over 씬 전환
- `data/chapter1_dialogue.json` — hidden_stump 대화 추가
- `data/chapter4_dialogue.json` — hidden_garden 대화 추가
- `scenes/maps/rim_forest.gd` — 히든 이벤트 트리거
- `scenes/maps/the_seam.gd` — 히든 이벤트 트리거

### 전체 플레이 흐름 (최종)
```
타이틀 (New Game / Continue / Quit)
  ↓
Ch1 림 외곽 숲 — 오프닝 → 엘리아 → 재비 → 전투 → [히든: 그루터기] → 야영 → 녹색 나무 CG
  ↓
Ch2 베르단 시장 — 도착 → 말렛 거래(수락/거절) → 추출 → 보상 + 카이로스 경고
  ↓
Ch3 크럼블링 코스트 — 도착 → 카이로스 목격 → 전투 → The Seam 도착
  ↓
Ch4 The Seam — 도착 → 세이블 브리핑 → [히든: 정원] → BL-07 보스전
  ↓
Ch5 BL-07 내부 — 진입 → 탐색+전투 → 핵심부 → **The Seal 결정**
  ├── Zero Burn: 이름 연소 → 백색 플래시 → seal_complete
  └── Preservation: 이름 보존 → seal_refused
  ↓
Ch6 에필로그 — The Seam 복귀 → 분기별 에필로그 → NPC 후일담
  ↓
패배 시: 게임 오버 화면 → Stagger On / Load / Title
```

### 다음 세션 (S22) 할 일
- [x] 그래픽 개선 + 전투씬 오버홀 → S22에서 완료
- [x] 사운드 폴리싱 (UI 조작음) → S22에서 완료

---

## S22 — 2026-04-06 (그래픽 대폭 개선 + 전투씬 오버홀 + UI SFX)

### 완료
- [x] **전투씬 대폭 개선 (battle_scene.gd 완전 재작성):**
  - 전투 인트로 연출: 검은 화면 → 적 이름/부제 표시 → 구분선 애니메이션 → 페이드인
  - 보스/공허수 인트로 차별화 (빨강/보라 + "BOSS"/"Void Beast" 부제)
  - 턴 표시: "— YOUR TURN —" / "— ENEMY TURN —" (컬러 + 페이드 애니메이션)
  - 공격 VFX: 물리 공격 → 대각선 슬래시 이펙트 (2중 라인)
  - 기억 연소 VFX: 불꽃 파티클 12개 (주황~빨강~노랑, 떠오르며 사라짐) + 중앙 플래시
  - 적 아이들 모션: sin파 기반 부드러운 상하 흔들림 (1.5Hz)
  - 상태 아이콘: SHIELD / PHASE / VOID 표시 (적 HP 패널 아래)
  - 데미지 숫자 개선: 크기 스케일링 (50+/100+), 드롭 섀도우, 스케일 펀치
  - 적 스프라이트 깜빡임: 피격 시 밝게 번쩍 → 복귀
  - 스크린 셰이크 개선: 5프레임 감쇠 흔들림 (intensity 파라미터)
  - 배경 비네트 오버레이 (상단/하단 어두운 바)
  - 버튼 pressed 스타일 추가, 전투 UI SFX 연동
- [x] **타일 깊이감 개선 (tile_painter.gd):**
  - `_paint_edge_shading()` 추가: 상단 하이라이트 + 하단/우측 그림자
  - 타일 종류별 셰이딩 강도 차등 (나무/벽 강, 바닥류 약, 물/보이드 최약)
- [x] **씬 전환 다양화 (scene_transition.gd):**
  - `change_scene_battle()` 추가: 다이아몬드 와이프 효과
  - 16x9 그리드, 중앙에서 퍼져나가는 검은 타일 → 페이드인
  - 모든 전투 진입 5곳에 적용 (림 숲, 크럼블링 코스트, The Seam, BL-07)
- [x] **UI SFX 5종 추가 (총 17종):**
  - `ui_hover` — 짧은 고음 틱 (0.04초)
  - `ui_select` — 명확한 확인음 (0.08초, 이중 사인파)
  - `ui_open` — 메뉴 열기 상승 스윕 (0.12초)
  - `ui_close` — 메뉴 닫기 하강 스윕 (0.1초)
  - `battle_intro` — 전투 진입 긴장 저음 (0.6초)
  - 전투 버튼 hover/pressed에 SFX 연동
- [x] **맵 비네트 오버레이:**
  - `MapEffects.add_vignette()` 추가 (상/하/좌/우 어두운 오버레이)
  - 5개 맵 전체 적용, BL-07은 강한 비네트 (0.6)
- [x] **안개 효과:**
  - `MapEffects.add_fog()` / `update_fog()` 추가
  - 림 외곽 숲에 숲 안개 적용 (3개 반투명 레이어, 천천히 이동)
- [x] **Ch2 말렛 거래 버그 수정:**
  - 선택지가 별도 대화키에 있어 도달 불가능했던 문제
  - 선택지를 `malet_encounter` 끝에 이동
  - 거절 후 재대화 시 플래그 초기화

### 변경/생성된 파일
- `scenes/battle/battle_scene.gd` — **완전 재작성** 전투씬 (인트로, VFX, 아이들, 턴표시, 상태)
- `scripts/utils/tile_painter.gd` — 엣지 셰이딩 추가
- `scripts/core/scene_transition.gd` — 다이아몬드 와이프 전환 추가
- `scripts/systems/audio_manager.gd` — UI SFX 5종 추가
- `scripts/utils/map_effects.gd` — 비네트 + 안개 효과 추가
- `scenes/maps/rim_forest.gd` — 비네트 + 안개
- `scenes/maps/crumbling_coast.gd` — 비네트 + 전투 전환 변경
- `scenes/maps/the_seam.gd` — 비네트 + 전투 전환 변경
- `scenes/maps/bl07_void.gd` — 강한 비네트 + 전투 전환 변경
- `scenes/maps/verdan_market.gd` — 비네트 + 말렛 거래 버그 수정
- `data/chapter2_dialogue.json` — 말렛 선택지 위치 수정

### 전체 SFX 현황 (17종)
| SFX | 설명 | 사용처 |
|-----|------|--------|
| confirm | UI 확인음 | 대화/메뉴 |
| cancel | UI 취소음 | 대화/메뉴 |
| burn | 기억 연소 | 전투 스킬 |
| hit | 타격 | 전투 공격 |
| heal | 회복 | 전투 승리 |
| step | 발걸음 | 이동 |
| shield | 적 방어막 | 전투 |
| drain | 생명력 흡수 | 전투 |
| phase_change | 보스 페이즈 | 전투 |
| defeat | 패배 | 전투 |
| flee | 도주 | 전투 |
| memory_add | 기억 획득 | 탐색 |
| void_pulse | 보이드 맥동 | Ch5 |
| ui_hover | 버튼 호버 | 전투/메뉴 |
| ui_select | 버튼 선택 | 전투/메뉴 |
| ui_open | 메뉴 열기 | UI |
| ui_close | 메뉴 닫기 | UI |

### 다음 세션 (S23) 할 일
- [x] 엔딩 크레딧 화면 → S23에서 완료
- [x] PauseMenu에 UI SFX 연동 → S23에서 완료
- [x] 비네트/안개 CanvasLayer 수정 → S23에서 완료

---

## S23 — 2026-04-06 (엔딩 크레딧 + UI SFX 전체 연동 + 비네트 수정 + 코드 검증)

### 완료
- [x] **비네트/안개 CanvasLayer 수정:**
  - 문제: Node2D 자식이라 카메라 이동 시 화면을 벗어남
  - 수정: CanvasLayer 기반으로 변경 (layer 3 비네트, layer 2 안개)
  - Control 앵커 프리셋 사용 (TOP_WIDE, BOTTOM_WIDE 등)
- [x] **엔딩 크레딧 화면 (credits.tscn + .gd):**
  - 스크롤 크레딧 (40px/sec 상승)
  - 분기별 에필로그 한 줄 (Zero Burn / Preservation)
  - SPACE/ENTER 스킵 지원
  - 에필로그 BGM 재사용
  - 에필로그 NPC 둘 다 대화 후 자동 크레딧 진입
  - the_seam.gd에 _check_credits_trigger() 추가
- [x] **UI SFX 전체 연동 (6곳):**
  - PauseMenu: ui_open(열기), ui_close(닫기), ui_hover(버튼 호버)
  - DialogueBox: ui_select(선택지 선택), ui_hover(선택지 호버)
  - GameOver: ui_hover(버튼 호버)
  - Title: ui_hover(버튼 호버)
- [x] **코드 검증 — 4개 버그 수정:**
  - main.gd: 이중 시그널 연결 제거 (tscn + 코드 중복)
  - the_seam.gd: 크레딧 시그널을 NPC 셋업에서 한 번만 연결 + 완료 후 disconnect
  - credits.gd: 스크롤 종료 감지를 캐시된 _total_height로 교체
  - credits.gd: _input → _unhandled_input 변경 (일관성)

### 변경/생성된 파일
- `scripts/utils/map_effects.gd` — 비네트/안개 CanvasLayer 기반 전환
- `scenes/ui/credits.tscn` — **신규** 크레딧 씬
- `scenes/ui/credits.gd` — **신규** 크레딧 스크립트
- `scenes/maps/the_seam.gd` — 에필로그→크레딧 연결
- `scripts/ui/pause_menu.gd` — UI SFX (open/close/hover)
- `scripts/ui/dialogue_box.gd` — 선택지 SFX (hover/select)
- `scenes/ui/game_over.gd` — 버튼 hover SFX
- `scenes/main/main.gd` — 버튼 hover SFX

### 전체 플레이 흐름 (최종)
```
타이틀 (New Game / Continue / Quit)
  ↓
Ch1 림 외곽 숲 — 오프닝 → 엘리아 → 재비 → 전투 → [히든: 그루터기] → 야영 → 녹색 나무 CG
  ↓
Ch2 베르단 시장 — 도착 → 말렛 거래(수락/거절) → 추출 → 보상 + 카이로스 경고
  ↓
Ch3 크럼블링 코스트 — 도착 → 카이로스 목격 → 전투 → The Seam 도착
  ↓
Ch4 The Seam — 도착 → 세이블 브리핑 → [히든: 정원] → BL-07 보스전
  ↓
Ch5 BL-07 내부 — 진입 → 탐색+전투 → 핵심부 → **The Seal 결정**
  ├── Zero Burn: 이름 연소 → 백색 플래시 → seal_complete
  └── Preservation: 이름 보존 → seal_refused
  ↓
Ch6 에필로그 — The Seam 복귀 → 분기별 에필로그 → 엘리아+세이블 대화
  ↓
**엔딩 크레딧** — 스크롤 크레딧 → 분기별 에필로그 한 줄 → 타이틀 복귀
  ↓
패배 시: 게임 오버 화면 → Stagger On / Load / Title
```

### 다음 세션 (S24) 할 일
- [x] 전체 코드 정적 분석 → S24에서 완료
- [x] 최종 폴리싱 → S25에서 완료

---

## S24 — 2026-04-06 (전체 코드 정적 분석 + 버그 수정)

### 완료
- [x] **5개 병렬 코드 감사 에이전트 실행:**
  - 코어 오토로드 (GameManager, MemoryManager, DialogueManager, SceneTransition, SaveManager, BattleManager, AudioManager)
  - UI 스크립트 (DialogueBox, MemoryUI, SystemLog, PauseMenu, CgViewer, Credits, GameOver)
  - 맵 스크립트 (RimForest, VerdanMarket, CrumblingCoast, TheSeam, BL07Void, BattleScene, Main)
  - 유틸/플레이어 (TilePainter, MapEffects, PixelSprite, Player, UITheme, project.godot)
  - 리소스 무결성 검증 (load/preload, change_scene, play_bgm, CG, 포트레이트, 대화 JSON)
- [x] **BUG 7개 수정:**
  1. `crumbling_coast.gd` — `_process()` 안 `await` 제거. 도착 시퀀스를 `_ready()`로 이동
  2. `battle_scene.gd` — `_exit_tree()`에서 오토로드 시그널 5개 disconnect (freed 객체 참조 방지)
  3. `battle_manager.gd` — `_enemy_turn()`에 VICTORY/DEFEAT/FLED 상태 가드 추가 (0.8s 타이머 레이스)
  4. `audio_manager.gd` — `_on_tree_changed()` 씬 경로 캐싱 (프레임당 수백 회 호출 방지)
  5. `the_seam.gd` — `_check_credits_trigger` 이중 연결 방지 (`is_connected` 가드)
  6. `pause_menu.gd` — `_on_save()` await 후 `is_open` 가드 (메뉴 닫힌 후 UI 접근 방지)
  7. `cg_viewer.gd` — `cg_shown` 시그널을 auto-close 전에 emit (시그널 타이밍 수정)
- [x] **WARN 수정:**
  - `battle_manager.gd` — float/int 나눗셈 경고 6곳 수정 (`/ 2.0` → `/ 2` 또는 `* 2 <=`)
  - `audio_manager.gd` — 죽은 코드 제거 (`FORMAT_IMA_ADPCM` 할당)
  - `game_over.gd` — 하드코딩 자식 인덱스 → 직접 버튼 참조 + `is_instance_valid` 가드

### 변경된 파일
- `scenes/maps/crumbling_coast.gd` — _process await 버그 수정
- `scenes/battle/battle_scene.gd` — _exit_tree 시그널 정리
- `scripts/systems/battle_manager.gd` — 상태 가드 + float/int 경고 수정
- `scripts/systems/audio_manager.gd` — 씬 캐싱 + 죽은 코드 제거
- `scenes/maps/the_seam.gd` — 이중 연결 방지 + Void Wraith 밸런스
- `scripts/ui/pause_menu.gd` — await 가드 + MemoryUI 열림 체크
- `scripts/ui/cg_viewer.gd` — cg_shown 시그널 타이밍
- `scenes/ui/game_over.gd` — 버튼 포커스 안전화

---

## S25 — 2026-04-06 (최종 폴리싱 — 밸런스, UX, 정리)

### 완료
- [x] **ESC 이중 바인딩 해결:**
  - PauseMenu에 `MemoryUI.is_open` 체크 추가 (이중 안전)
  - MemoryUI가 MENU 상태로 전환하므로 기본적으로 안전하지만 방어적 코딩
- [x] **엘리아 대화 키 수정:**
  - `rim_forest.tscn` — `elia_appears` → `elia_talk` (스토리 대화 대신 탐색 대화)
- [x] **보이드 비스트 메시지 수정:**
  - "normal attacks won't work" → "normal attacks are weakened" (30% 감쇠와 일치)
- [x] **세이브/로드 안전성:**
  - F6/F7 퀵세이브/로드를 EXPLORATION 상태에서만 허용 (전투/대화 중 저장 방지)
- [x] **Ch4 Void Wraith 밸런스 조정:**
  - HP 60→90, ATK 14→18 (Ch3 Coastal Void Beast와 동등 이상)
- [x] **플레이어 null guard:**
  - `player.gd` `_ready()`에 sprite null 체크 추가
- [x] **미사용 에셋 정리:**
  - `assets/audio/bgm/battle.mp3` 삭제 (battle_theme.mp3과 중복)
  - `assets/audio/bgm/ch1_camp.mp3` 삭제 (미참조)

### 변경/삭제된 파일
- `scripts/core/player.gd` — sprite null guard
- `scripts/ui/pause_menu.gd` — MemoryUI.is_open 체크
- `scenes/maps/rim_forest.tscn` — Elia 대화 키 수정
- `scripts/systems/battle_manager.gd` — Void Beast 메시지 수정
- `scripts/systems/save_manager.gd` — 퀵세이브 상태 가드
- `scenes/maps/the_seam.gd` — Void Wraith 밸런스
- `assets/audio/bgm/battle.mp3` — **삭제** (미사용)
- `assets/audio/bgm/ch1_camp.mp3` — **삭제** (미사용)

### 리소스 검증 결과
- load()/preload() 참조: **전부 유효**
- change_scene() 대상 .tscn: **전부 존재**
- play_bgm() 오디오: **전부 존재**
- CG/포트레이트: **전부 존재**
- 대화 JSON 키: **전부 매칭**

### 다음
- [x] 추가 기능/콘텐츠 → S26에서 완료

---

## S26 — 2026-04-06 (옵션 메뉴 + 위치 세이브 + 엔딩 2종 + 기억 월드 반응)

### 완료
- [x] **옵션 메뉴 (OptionsMenu 오토로드):**
  - CanvasLayer (layer 56, PauseMenu 위)
  - Master/BGM/SFX 볼륨 슬라이더 (0-100, `linear_to_db()`)
  - BGM/SFX는 AudioManager 플레이어 직접 제어
  - 전체화면 토글 (`DisplayServer.window_set_mode`)
  - `user://settings.json`에 설정 저장/로드
  - PauseMenu에 "Options" 버튼 추가
  - 타이틀에 "Options" 버튼 추가

- [x] **플레이어 위치 세이브/로드:**
  - SaveManager에 `loaded_player_pos` 변수 + `player_pos` 세이브 데이터
  - `get_tree().get_nodes_in_group("player")`로 위치 수집
  - 5개 맵 `_position_player()`에 위치 복원 코드 추가
  - 엘리아 동행 위치도 플레이어 기준으로 복원

- [x] **추가 엔딩 2종 (총 4종):**
  - **Ash 엔딩** — `seal_refused` + 기억 4개 이상 연소: 껍데기만 남은 아렐
  - **Seam 비밀 엔딩** — `seal_refused` + 히든 이벤트 2개 발견: 작은 아름다움 속 희망
  - `the_seam.gd` 에필로그 분기 로직 확장
  - `credits.gd` 분기별 에필로그 텍스트 4종
  - `chapter6_dialogue.json`에 epilogue_ash/epilogue_seam 대화 추가

- [x] **기억 연소 월드 반응 시스템:**
  - DialogueManager의 기존 `requires_memory`/`burned_text` 활용
  - Ch1 `elia_talk`: 숲 냄새/첫 검술 기억 반응
  - Ch2 `elia_ch2_talk`: 시장 음식/모닥불 노래 기억 반응
  - Ch3 `elia_coast_talk`: 모닥불 노래 기억 반응
  - Ch4 `elia_seam_talk`: 관계 기억 반응
  - Ch4 `sable_talk`: 보이드 워커 기억 반응
  - 기억을 태우면 NPC 대화가 실제로 변하는 핵심 메카닉 구현

### 변경/생성된 파일
- `scenes/ui/options_menu.gd` — **신규** 옵션 메뉴 오토로드
- `project.godot` — OptionsMenu 오토로드 등록
- `scenes/main/main.gd` — 타이틀 Options 버튼
- `scenes/main/main.tscn` — OptionsButton 노드 추가
- `scripts/ui/pause_menu.gd` — Options 버튼 + 콜백
- `scripts/systems/save_manager.gd` — 플레이어 위치 세이브/로드
- `scenes/maps/rim_forest.gd` — 위치 복원
- `scenes/maps/verdan_market.gd` — 위치 복원
- `scenes/maps/crumbling_coast.gd` — 위치 복원
- `scenes/maps/the_seam.gd` — 위치 복원 + 에필로그 4분기
- `scenes/maps/bl07_void.gd` — 위치 복원
- `scenes/ui/credits.gd` — 4종 에필로그 텍스트
- `data/chapter1_dialogue.json` — 기억 반응 대사
- `data/chapter2_dialogue.json` — 기억 반응 대사
- `data/chapter3_dialogue.json` — 기억 반응 대사
- `data/chapter4_dialogue.json` — 기억 반응 대사
- `data/chapter6_dialogue.json` — Ash/Seam 에필로그

### 엔딩 분기 (최종)
```
Ch5 The Seal 결정:
  ├── Zero Burn: 이름 연소 → epilogue_zero_burn → "He burned everything."
  └── Preservation: 이름 보존 →
       ├── 기억 4+ 연소 → epilogue_ash → "Just ash, drifting."
       ├── 히든 2개 발견 → epilogue_seam → "Something green still grows."
       └── 기본 → epilogue_preservation → "He kept his name."
```

### 다음
- [x] 추가 폴리싱 → S27에서 완료

---

## S27 — 2026-04-06 (챕터 타이틀 + 텍스트 속도 + 발걸음 + UI 폴리싱)

### 완료
- [x] **OptionsMenu ESC 닫기:**
  - `_unhandled_input`에서 "cancel" 액션으로 닫기
  - 입력 소비(`set_input_as_handled`)로 PauseMenu 전파 방지

- [x] **텍스트 속도 옵션:**
  - OptionsMenu에 Text Speed 슬라이더 추가 (1-5단계)
  - 1=Slow(0.06s), 2=Slow+(0.045s), 3=Normal(0.03s), 4=Fast(0.015s), 5=Instant(0s)
  - `settings.json`에 저장/로드
  - DialogueBox에서 런타임으로 OptionsMenu 설정 참조

- [x] **챕터 타이틀 카드:**
  - `MapEffects.show_chapter_title()` — CanvasLayer 기반 오버레이
  - 페이드인(0.5s) → 홀드(2.0s) → 페이드아웃(0.8s) → 자동 제거
  - "CHAPTER X" + 타이틀 + 서브타이틀 3줄 구성
  - 5개 맵 전부 적용:
    - Ch1 "Rim Forest" — "The edge of what remains"
    - Ch2 "Verdan Market" — "Where memories are currency"
    - Ch3 "Crumbling Coast" — "The ground gives way"
    - Ch4 "The Seam" — "Between what was and what will be"
    - Ch5 "BL-07" — "The Void stares back"
  - `await`로 타이틀 카드 완료 후 스토리 시퀀스 시작

- [x] **발걸음 SFX:**
  - AudioManager에 `step_player` 전용 플레이어 추가 (-12dB, SFX와 독립)
  - `play_step()` 메서드 — 다른 SFX와 겹치지 않음
  - Player에서 0.25초 간격으로 이동 중 발소리 재생
  - EXPLORATION 상태에서만 동작

### 변경된 파일
- `scenes/ui/options_menu.gd` — ESC 닫기 + 텍스트 속도 슬라이더
- `scripts/ui/dialogue_box.gd` — 동적 타자기 속도 (OptionsMenu 연동)
- `scripts/utils/map_effects.gd` — `show_chapter_title()` 정적 함수
- `scenes/maps/rim_forest.gd` — Ch1 타이틀 카드
- `scenes/maps/verdan_market.gd` — Ch2 타이틀 카드
- `scenes/maps/crumbling_coast.gd` — Ch3 타이틀 카드
- `scenes/maps/the_seam.gd` — Ch4 타이틀 카드
- `scenes/maps/bl07_void.gd` — Ch5 타이틀 카드
- `scripts/core/player.gd` — 발걸음 타이머 + SFX
- `scripts/systems/audio_manager.gd` — step_player + play_step()

### 다음
- [x] UX 개선 → S28에서 완료

---

## S28 — 2026-04-06 (탐색 HUD + 알림 토스트 시스템)

### 완료
- [x] **탐색 HUD (ExplorationHUD 오토로드):**
  - CanvasLayer (layer 10), 좌상단 고정
  - HP 바 (100px 프로그레스바 + 수치, HP≤25%면 빨간색)
  - 챕터/지역 표시 ("Ch.3 — Crumbling Coast")
  - 기억 카운터 ("Memories: 6 held, 2 burned")
  - 0.5초 타이머 기반 업데이트 (성능 최적화)
  - HP 바 트윈 애니메이션 (0.4s ease-out)
  - GameState.EXPLORATION에서만 표시 (전투/대화/메뉴 중 숨김)
  - UITheme 색상 체계 활용

- [x] **알림 토스트 시스템 (NotificationToast 오토로드):**
  - CanvasLayer (layer 35), 하단 중앙
  - 슬라이드 업 + 페이드인(0.3s) → 홀드(2.0s) → 페이드아웃(0.5s)
  - 3가지 타입: INFO(ℹ 앰버), SUCCESS(✓ 녹색), WARNING(⚠ 오렌지)
  - 대기열 시스템 (연속 알림 순차 처리)
  - 자동 시그널 연결:
    - MemoryManager.memory_added → "✓ Memory acquired: {title}"
    - MemoryManager.memory_burned → "⚠ Memory burned: {title}"
    - SaveManager.save_completed → "✓ Game saved — Slot {n}"
    - SaveManager.load_completed → "ℹ Game loaded — Slot {n}"

### 변경/생성된 파일
- `scripts/ui/exploration_hud.gd` — **신규** 탐색 HUD 오토로드
- `scripts/ui/notification_toast.gd` — **신규** 알림 토스트 오토로드
- `project.godot` — ExplorationHUD + NotificationToast 오토로드 등록

### 오토로드 목록 (최종 14개)
```
GameManager, MemoryManager, DialogueManager, SceneTransition,
DialogueBox, MemoryUI, SystemLog, BattleManager, SaveManager,
CgViewer, PauseMenu, OptionsMenu, ExplorationHUD, NotificationToast
```

### 다음
- [x] 기억 거래 상점 → S29에서 완료
- [x] 엘리아 분리 메카닉 → S30에서 완료

---

## S29 — 2026-04-08 (기억 거래 상점 + Grains 경제 시스템)

### 완료
- [x] **MemoryShop 오토로드 (layer 42):**
  - 풀스크린 상점 UI (오버레이 + 패널)
  - Sell/Buy 탭 전환, 아이템 목록 + 상세 패널
  - 판매: 플레이어 보유 미연소 기억 → Grains 획득 (Grade 1 핵심 기억 판매 불가)
  - 구매: 상인 인벤토리 기억 → Grains 소모
  - 등급별 가격 체계: G5=5/10, G4=15/25, G3=30/50, G2=60/100, G1=150/300
  - ESC 닫기, UI SFX (hover/select/open/close), NotificationToast 연동
  - `shop_closed` / `grains_changed` 시그널

- [x] **Grains 경제 시스템:**
  - 전투 승리 시 Grains 자동 보상 (일반 3+HP/20, 보이드 8+HP/20, 보스 20+HP/20)
  - NotificationToast로 획득량 표시
  - DialogueManager에 `add_grains` 선택지 지원 추가

- [x] **Verdan Market 말렛 상점 연동:**
  - malet_reward 대화 후 상점 자동 오픈
  - 상점 재고: "The Taste of Copper" (G5, 8G), "A Deal in the Dark" (G4, 20G)
  - 상점 닫으면 Ch3 전환

- [x] **ExplorationHUD Grains 표시:**
  - 하단에 "Grains: N" 표시 (금색, 0.5초 갱신)

### 변경/생성된 파일
- `scripts/ui/memory_shop.gd` — **신규** 기억 거래 상점 오토로드
- `project.godot` — MemoryShop 오토로드 등록 (총 15개)
- `scripts/systems/battle_manager.gd` — Grains 전투 보상 + NotificationToast
- `scripts/systems/dialogue_manager.gd` — `add_grains` 선택지 지원
- `scripts/ui/exploration_hud.gd` — Grains 표시 추가
- `scenes/maps/verdan_market.gd` — 말렛 상점 연동

---

## S30 — 2026-04-08 (엘리아 분리 메카닉 + 앵커링 시스템 실체화)

### 완료
- [x] **엘리아 분리 선택 (Ch3 크럼블링 코스트):**
  - 카이로스 목격 후 분리 선택지 자동 발생
  - "Go together" → `elia_stays` 플래그, 동행 유지
  - "Split up" → `elia_separates` 플래그, `elia_with_party=false`
  - 분리 시 엘리아 씬에서 비활성화 (visible=false, physics 중지)
  - 세이브/로드 시에도 분리 상태 유지

- [x] **잔존(Residue) 메카닉 차별화:**
  - 엘리아 동행 시: Grade 3+ 기억 연소 → 잔존(희미한 흔적) 상태로 남음
  - 엘리아 분리 시: 모든 기억 연소 → 완전 소실 (잔존 없음)
  - 기존 MemoryManager의 `elia_with_party` 체크가 자동으로 동작

- [x] **분리 인지 대화 변형:**
  - Ch3 분리/동행 선택 대화 3종 (choice, stays_response, separates_response)
  - Ch4 솔로 도착 대화 (seam_welcome_solo) — 혼자 온 아렐에 대한 세이블 반응
  - Ch3 재합류 대화 (elia_reunion) — 해안길 경유 후 재합류

- [x] **재합류 메카닉 (Ch4 The Seam):**
  - 분리 상태로 The Seam 도착 시 솔로 대사 → 재합류 이벤트 자동 발생
  - `elia_reunited` 플래그 + `elia_with_party=true` 복원
  - 엘리아 다시 표시 + 물리 활성화 + 플레이어 근처 위치
  - 재합류 후 기존 Ch4 시퀀스 정상 진행

### 변경/생성된 파일
- `data/chapter3_dialogue.json` — 분리 선택 대화 4종 추가
- `data/chapter4_dialogue.json` — 솔로 도착 대화 추가
- `scenes/maps/crumbling_coast.gd` — 분리 선택 이벤트 + 상태 반영
- `scenes/maps/the_seam.gd` — 재합류 이벤트 + 솔로 대사 분기

### 엘리아 앵커링 효과 (최종)
```
엘리아 동행 (elia_with_party = true):
  └── Grade 3+ 기억 연소 시 → 잔존(Residue) 상태로 남음 (희미한 효과 유지)

엘리아 분리 (elia_with_party = false):
  └── 모든 기억 연소 시 → 완전 소실 (되돌릴 수 없음)
  └── Ch3 크럼블링 코스트에서만 분리 가능
  └── Ch4 The Seam에서 자동 재합류
```

### 오토로드 목록 (최종 15개)
```
GameManager, MemoryManager, DialogueManager, SceneTransition,
DialogueBox, MemoryUI, SystemLog, BattleManager, SaveManager,
CgViewer, AudioManager, PauseMenu, OptionsMenu, ExplorationHUD,
NotificationToast, MemoryShop
```

### 다음
- [x] 미니맵 + 저널 → S31에서 완료

---

## S31 — 2026-04-08 (미니맵 + 스토리 저널 시스템)

### 완료
- [x] **미니맵 시스템 (Minimap 유틸리티 클래스):**
  - CanvasLayer(9) 기반, 우상단 140x100px 미니맵
  - 맵 타일을 4px 단위로 렌더링 (20+ 타일 종류 색상 매핑)
  - 플레이어 마커 (밝은 파란 6px 점, 실시간 위치 추적)
  - 엘리아 마커 (은빛 4px 점, 분리 시 숨김)
  - GameState.EXPLORATION에서만 표시
  - 5개 맵 전체 통합 (_build_map에서 생성, _process에서 업데이트)
  - 반투명 배경 + 앰버 테두리 (UITheme 일관)

- [x] **스토리 저널 / 코덱스 (StoryJournal 오토로드, layer 57):**
  - 3개 탭: Events(이벤트) / People(NPC) / Choices(선택)
  - Events: 21개 이벤트 엔트리 (Ch1~Ch6, 히든 포함), 챕터별 헤더 구분
  - People: 4명 NPC (Elia/Malet/Sable/Kairos), 캐릭터 색상 반영
  - Choices: 6개 주요 분기 선택 기록
  - story_flags 기반 자동 언락 (직접 기록 불필요)
  - 좌측 스크롤 목록 + 우측 상세 패널
  - ESC 닫기, UI SFX 연동

- [x] **PauseMenu 통합:**
  - "Journal" 버튼 추가 (Resume 바로 아래)
  - 저널 오픈 시 PauseMenu 위에 표시

### 변경/생성된 파일
- `scripts/ui/minimap.gd` — **신규** 미니맵 유틸리티 (class_name Minimap)
- `scripts/ui/story_journal.gd` — **신규** 스토리 저널 오토로드
- `project.godot` — StoryJournal 오토로드 등록 (총 16개)
- `scripts/ui/pause_menu.gd` — Journal 버튼 추가
- `scenes/maps/rim_forest.gd` — 미니맵 통합
- `scenes/maps/verdan_market.gd` — 미니맵 통합
- `scenes/maps/crumbling_coast.gd` — 미니맵 통합
- `scenes/maps/the_seam.gd` — 미니맵 통합
- `scenes/maps/bl07_void.gd` — 미니맵 통합

### 오토로드 목록 (최종 16개)
```
GameManager, MemoryManager, DialogueManager, SceneTransition,
DialogueBox, MemoryUI, SystemLog, BattleManager, SaveManager,
CgViewer, AudioManager, PauseMenu, OptionsMenu, ExplorationHUD,
NotificationToast, MemoryShop, StoryJournal
```

### 다음
- [x] 랜덤 인카운터 + 상태이상 → S32에서 완료

---

## S32 — 2026-04-08 (랜덤 인카운터 + 전투 상태이상)

### 완료
- [x] **전투 상태이상 시스템 (BattleManager 확장):**
  - 3종 상태이상: Poison(독 DoT), Weaken(공격력 30% 감소), Burn(화상 DoT)
  - StatusEffect enum + StatusEntry 클래스 (효과/지속턴/위력)
  - `apply_status()`: 대상(player/enemy)에 상태이상 부여, 중복 시 강한 쪽 갱신
  - `_process_statuses()`: 턴 시작 시 DoT 처리 + 지속턴 감소 + 만료 알림
  - `_get_weaken_multiplier()`: 약화 상태 시 공격력 계수 반환
  - 플레이어 공격/적 공격 모두 약화 적용
  - Grade 2+ 기억 연소 시 적에게 화상 DoT 자동 부여
  - `status_changed` 시그널 → UI 실시간 갱신
  - 독/화상으로 사망 시 정상 승리/패배 처리

- [x] **적 상태이상 능력 4종 추가:**
  - `poison`: 3턴 DoT (공격력 30% + 2~5 랜덤)
  - `burn_attack`: 데미지 + 2턴 화상 DoT
  - `weaken`: 플레이어 공격력 30% 감소 3턴
  - 기존 drain/shield/multi_hit과 병합

- [x] **전투 UI 상태이상 표시 (battle_scene.gd):**
  - 적 상태 컨테이너: 기존 SHIELD/PHASE/VOID + 새 상태이상 아이콘
  - 플레이어 상태 컨테이너: 플레이어 HP 패널 아래 상태이상 표시
  - 색상 코드: POISON(녹색), WEAK(주황), BURN(오렌지)
  - 남은 턴 수 표시 (예: "POISON 2")
  - `status_changed` 시그널 연결 + _exit_tree에서 정리

- [x] **랜덤 인카운터 시스템 (RandomEncounter 유틸리티):**
  - `class_name RandomEncounter` (static utility, Minimap과 동일 패턴)
  - 이동 거리 기반 인카운터 (타일 단위 step 카운터)
  - 맵별 설정: 적 풀, 최소/최대 걸음 수, 배경/적 이미지
  - 챕터 완료 후 재방문 시에만 활성화
  - `setup()` → `update()` 패턴 (맵 _ready에서 초기화, _process에서 체크)

- [x] **5개 맵 랜덤 인카운터 통합:**
  - Rim Forest (Ch1 완료 후): Ash Crawler, Forest Shade(독), Void Beast — 50~90 걸음
  - Verdan Market (Ch2 완료 후): Alley Rat(독), Market Thief(약화) — 60~100 걸음
  - Crumbling Coast (Ch3 완료 후): Coastal Void Beast, Cliff Stalker(독+연타), Shore Wraith(화상+약화) — 40~70 걸음
  - The Seam (Ch4 완료 후): Void Wraith(흡수+약화), Seam Lurker(독+방어) — 45~80 걸음
  - BL-07 Void (Ch5 진입 후): Void Fragment(화상), Memory Eater(흡수+연타+약화), Null Wisp(독+화상) — 30~55 걸음

### 변경/생성된 파일
- `scripts/systems/battle_manager.gd` — 상태이상 시스템 (StatusEffect, apply/process/weaken)
- `scenes/battle/battle_scene.gd` — 상태이상 UI (플레이어+적 상태 컨테이너)
- `scripts/utils/random_encounter.gd` — **신규** 랜덤 인카운터 유틸리티
- `scenes/maps/rim_forest.gd` — 랜덤 인카운터 통합
- `scenes/maps/verdan_market.gd` — 랜덤 인카운터 통합
- `scenes/maps/crumbling_coast.gd` — 랜덤 인카운터 통합
- `scenes/maps/the_seam.gd` — 랜덤 인카운터 통합
- `scenes/maps/bl07_void.gd` — 랜덤 인카운터 통합

### 상태이상 효과 정리
```
Poison (독): 매 턴 고정 데미지, 3턴 지속
Weaken (약화): 공격력 30% 감소, 3턴 지속
Burn (화상): 매 턴 고정 데미지, 2턴 지속
- Grade 2+ 기억 연소 시 적에게 자동 화상 부여
- 같은 효과 중복 시 강한 쪽으로 갱신
```

### 다음
- 전투 아이템 시스템 (포션/해독제) 추천
- New Game+ 모드
- 업적 시스템

---

## S33 — 2026-04-08 (전투 소모 아이템 시스템)

### 완료
- [x] **아이템 정의 + 인벤토리 (GameManager):**
  - `ITEMS` 상수: potion(40HP), hi_potion(80HP), antidote(독/화상 해제), firebomb(2턴 화상), smoke_bomb(확정 도주)
  - `player_data.items` Dictionary (`{id: count}`)
  - `add_item()`, `remove_item()`, `get_item_count()` — NotificationToast 연동

- [x] **전투 중 아이템 사용 (BattleManager):**
  - `player_use_item(item_id)`: heal/cure/burn/flee 4종 처리
  - heal: HP 회복 (max_hp 클램프)
  - cure: 독+화상 상태이상 해제
  - burn: 적에게 화상 DoT 부여
  - flee: 확정 도주 (battle_fled 시그널)
  - 아이템 사용 = 1턴 소모 → 적 반격

- [x] **전투 UI ITEM 버튼 + 아이템 목록 (battle_scene.gd):**
  - 행동 버튼에 ITEM 추가 (ATK/BURN/ITEM/FLEE)
  - 초록색 테마 아이템 리스트 패널 (토글 표시)
  - 보유 아이템만 버튼으로 표시 (이름 + 수량)
  - 클릭 시 사용 + 목록 닫기

- [x] **전투 승리 아이템 드롭:**
  - 30% 확률 아이템 드롭 (적 유형별 가중치)
  - void 적: firebomb/antidote 위주, 일반 적: potion/antidote 위주

- [x] **스타터 아이템:**
  - Ch1 캠프 완료: potion ×1
  - Ch2 말렛 보상: potion ×2, antidote ×1, firebomb ×1

- [x] **ExplorationHUD 아이템 카운터:**
  - HUD에 "Items: N" 행 추가 (초록색, Grains 아래)
  - 0.5초 주기 자동 갱신

### 변경/생성된 파일
- `scripts/core/game_manager.gd` — ITEMS 상수, add/remove/get_item, player_data.items
- `scripts/systems/battle_manager.gd` — player_use_item(), _try_item_drop()
- `scenes/battle/battle_scene.gd` — ITEM 버튼, 아이템 목록 UI
- `scenes/maps/rim_forest.gd` — Ch1 스타터 아이템
- `scenes/maps/verdan_market.gd` — Ch2 말렛 보상 아이템, 상점 아이템
- `scripts/ui/exploration_hud.gd` — items_label 추가

### 다음
- MemoryShop에 아이템 상점 탭 추가
- New Game+ 모드
- 업적 시스템

---

## S34 — 2026-04-08 (MemoryShop 아이템 탭 + New Game+ + 업적 시스템)

### 완료
- [x] **MemoryShop 아이템 탭:**
  - 3번째 탭 "Items" 추가 (Sell Memories / Buy Memories / Items)
  - 아이템 구매: GameManager.ITEMS 전체 목록, Grains로 구매
  - 아이템 판매: 보유 아이템 60% 가격에 Grains로 판매
  - 상세 패널 (이름/설명/가격) + 구매/판매 버튼

- [x] **New Game+ 모드:**
  - 게임 클리어 시 `user://ng_plus.json` 영구 파일 생성
  - 타이틀 화면에 "New Game+" 버튼 동적 추가 (클리어 후에만)
  - NG+ 시작: Grains + 아이템 유지, 스토리/기억 초기화, 회차 증가
  - 적 스케일링: HP/ATK × (1 + 0.3 × 회차) — NG+1 = 1.3배, NG+2 = 1.6배
  - HUD/PauseMenu에 "NG+N" 회차 표시
  - GameManager export/import에 ng_plus_cycle 포함 (세이브 호환)

- [x] **업적 시스템 (AchievementManager 오토로드):**
  - 25종 업적 정의 (전투 6 / 기억 4 / 탐색 3 / 스토리 9 / 경제 2 / NG+ 1)
  - `user://achievements.json` 영구 저장 (세이브 슬롯과 독립)
  - 전투: first_blood, void_slayer, boss_hunter, battle_veteran(10승), survivor(10HP이하 승리), item_master(10회 사용)
  - 기억: first_burn, pyromaniac(5회 연소), identity_crisis(Grade 2), zero_burn(핵심 기억)
  - 탐색: hidden_stump, hidden_garden, explorer(5맵 방문)
  - 스토리: Ch1~5 완료, 4종 엔딩, all_endings(4종 수집)
  - 경제: merchant(말렛 거래), wealthy(100 Grains)
  - PauseMenu "Achievements" 버튼 + 뷰어 패널 (아이콘/제목/설명, 미해금=???)
  - 달성 시 NotificationToast 자동 알림

### 변경/생성된 파일
- `scripts/ui/memory_shop.gd` — Items 탭 (buy_item/sell_item 모드)
- `scripts/core/game_manager.gd` — ng_plus_cycle, NG+ 관련 함수, export/import
- `scripts/systems/battle_manager.gd` — NG+ 적 스케일링, 아이템 사용 업적 추적, Grains 업적
- `scripts/ui/achievement_manager.gd` — **신규** 업적 오토로드
- `scripts/ui/pause_menu.gd` — Achievements 버튼 + 뷰어 패널, NG+ 표시
- `scripts/ui/exploration_hud.gd` — NG+ 회차 표시
- `scenes/main/main.gd` — NG+ 버튼, New Game 아이템 초기화
- `scenes/ui/credits.gd` — 게임 완료 기록, 엔딩 업적
- `scenes/maps/rim_forest.gd` — 맵 방문/챕터 완료/히든 이벤트 업적
- `scenes/maps/verdan_market.gd` — 맵 방문/챕터 완료/거래 업적
- `scenes/maps/crumbling_coast.gd` — 맵 방문/챕터 완료 업적
- `scenes/maps/the_seam.gd` — 맵 방문/히든 정원 업적
- `scenes/maps/bl07_void.gd` — 맵 방문/챕터 완료 업적
- `project.godot` — AchievementManager 오토로드 추가

### 다음
- 도감 시스템 (적/기억 수집도)
- 난이도 선택 (Easy/Normal/Hard)
- 미니게임 (시장 미니 퀴즈)

---

## S35 — 2026-04-08 (도감 + 난이도 + 기억 매칭 퍼즐)

### 완료
- [x] **도감 시스템 (Codex 오토로드):**
  - Bestiary 탭: 만난 적 기록 (이름/타입/HP/ATK/조우 횟수/격파 횟수)
  - Memory Archive 탭: 수집한 기억 카탈로그 (제목/등급/설명/연소 여부)
  - `user://codex.json` 영구 저장 (세이브와 독립)
  - BattleManager.battle_started/ended → 적 자동 기록
  - MemoryManager.memory_added/burned → 기억 자동 기록
  - 2탭 UI: 좌측 리스트 + 우측 상세 패널
  - PauseMenu "Codex" 버튼 추가

- [x] **난이도 선택 (OptionsMenu 확장):**
  - 3단계: Easy (적 0.7배) / Normal (1.0배) / Hard (1.4배)
  - OptionsMenu에 Difficulty 토글 버튼 추가 (순환식)
  - `settings.json`에 저장 → 재시작 시 유지
  - BattleManager.start_battle에서 적 HP/ATK에 난이도 계수 적용
  - NG+ 스케일링과 중첩 (NG+1 Hard = 1.3 × 1.4 = 1.82배)

- [x] **기억 매칭 퍼즐 (MemoryPuzzle 오토로드):**
  - 카드 뒤집기 미니게임 (현재 보유 기억에서 랜덤 쌍 생성)
  - 3~8쌍 설정 가능, 그리드 자동 조절
  - 매칭 성공/실패 시각 피드백 + SFX
  - 클리어 시 Grains 보상 (기본 + 시도 횟수 보너스)
  - 베르단 시장 (Ch2 완료 후): 4쌍, 15G 보상
  - The Seam (Ch4 완료 후): 5쌍, 20G 보상
  - 녹색/보라색 인디케이터로 퍼즐 영역 표시

### 변경/생성된 파일
- `scripts/ui/codex.gd` — **신규** 도감 오토로드
- `scripts/ui/memory_puzzle.gd` — **신규** 기억 매칭 퍼즐 오토로드
- `scenes/ui/options_menu.gd` — 난이도 토글 (difficulty 설정)
- `scripts/systems/battle_manager.gd` — 난이도 스케일링 적용
- `scripts/ui/pause_menu.gd` — Codex 버튼 추가
- `scenes/maps/verdan_market.gd` — 퍼즐 트리거 영역
- `scenes/maps/the_seam.gd` — 퍼즐 트리거 영역
- `project.godot` — Codex, MemoryPuzzle 오토로드 추가

### 다음
- 파티 시스템 (세이블 동행)
- 맵 날씨 효과 (비/눈/안개 변화)
- 전투 콤보 시스템

---

## S36 — 2026-04-08 (파티 시스템 + 날씨 효과 + 전투 콤보)

### 완료
- [x] **파티 시스템 (세이블 전투 동행):**
  - Ch4(The Seam) 브리핑 완료 시 `sable_joined` 플래그 설정
  - BattleManager: `sable_in_party` — Ch4+ & sable_joined일 때 활성
  - 세이블 지원 행동 (40% 확률): 힐(10~20HP) / 타격(8~18dmg) / 약화(20% 2턴)
  - `ally_action` 시그널 → 전투 로그에 세이블 행동 표시
  - 전투 UI에 "SABLE" 동행 아이콘 표시

- [x] **맵 날씨 효과:**
  - MapEffects: `add_rain()`, `add_snow()`, `add_heavy_fog()`, `update_heavy_fog()` 추가
  - 크럼블링 코스트(Ch3): 비 (GPUParticles2D, 빗방울 파티클)
  - The Seam(Ch4): 눈 (느린 낙하 파티클)
  - BL-07 보이드(Ch5): 짙은 안개 (5개 ColorRect 드리프트)

- [x] **전투 콤보 시스템:**
  - 연속 공격 시 콤보 카운터 누적 (2=+15%, 3=+30%, 4+=+50% 데미지)
  - 방어/연소/아이템 사용 시 콤보 리셋
  - `combo_changed` 시그널 → 전투 UI "COMBO x%d" 표시
  - 전투 로그에 콤보 배율 메시지

### 변경/생성된 파일
- `scripts/systems/battle_manager.gd` — 콤보 변수/시그널, 세이블 지원 행동, 콤보 배율
- `scripts/utils/map_effects.gd` — rain/snow/heavy_fog 파티클 함수
- `scenes/battle/battle_scene.gd` — 콤보/세이블 상태 아이콘 표시
- `scenes/maps/crumbling_coast.gd` — 비 효과 추가
- `scenes/maps/the_seam.gd` — 눈 효과, sable_joined 플래그
- `scenes/maps/bl07_void.gd` — 짙은 안개 효과

### 다음
- 맵 인터랙티브 오브젝트 (숨겨진 상자/단서)
- 전투 약점/저항 시스템 (적 속성)
- 기억 조합 시스템 (2개 기억 합성)

---

## S37 — 2026-04-08 (인터랙티브 오브젝트 + 속성 시스템 + 기억 합성)

### 완료
- [x] **맵 인터랙티브 오브젝트:**
  - 5개 맵 전체에 숨겨진 상자(금색 인디케이터) + 단서(청색 인디케이터)
  - 상자: 아이템 + Grains 보상, 1회 획득 (플래그 기반)
  - 단서: 세계관 텍스트 토스트 표시, 탐색 보상 강화
  - 림 포레스트: 포션x2+10G, 돌무더기 단서
  - 베르단 마켓: 화염탄+15G, 썸프 입구 단서
  - 크럼블링 코스트: Hi-Potion+12G, 해독제x2+8G, 카이로스 단서
  - 더 씸: Hi-Potion+연막탄+20G, 개울 단서
  - BL-07: Hi-Potionx2+화염탄+25G, 보이드 속삭임 단서

- [x] **전투 약점/저항 시스템:**
  - 3속성: PHYSICAL(일반공격), FIRE(Grade 5~3 연소), VOID(Grade 2~1 연소)
  - 약점 적중 = +50% 데미지, 저항 적중 = -30% 데미지
  - Enemy 클래스에 `weakness`/`resistance` 속성 추가
  - 기본: 보이드 수 = VOID 약점+PHYSICAL 저항, 일반 적 = FIRE 약점
  - Memory Eater: FIRE 약점, VOID 저항 (역전된 상성)
  - Shade Sentinel 보스: VOID 약점, FIRE 저항
  - 전투 UI에 WEAK/RESIST 아이콘 표시
  - 연소 메뉴에 속성(FIRE/VOID) 표시 → 전략적 기억 선택 유도
  - "It's super effective!" / "It's not very effective..." 로그 메시지

- [x] **기억 조합 시스템 (Synthesis):**
  - 동일 등급 미연소 기억 2개 → 상위 등급 1개로 합성
  - 원본 소실 (연소와 다른 방식의 상실 — 테마 강화)
  - 합성 결과: burn_power = (A + B) * 0.7 + 10 보너스
  - Grade 5→4→3→2 합성 가능, Grade 1(최고)은 합성 불가
  - MemoryUI에 SYNTHESIZE 버튼 (조건 충족 시 표시)
  - 합성 모드: 첫 기억 선택 → 두 번째 기억 선택 → 즉시 합성
  - 등급별 합성 결과 이름: Blended Sensation / Woven Routine / Bound Connection / Forged Identity
  - `memory_synthesized` 시그널 + 토스트 알림

### 변경/생성된 파일
- `scripts/systems/battle_manager.gd` — 속성 시스템 (ELEMENT_BONUS/RESIST, _get_element_multiplier)
- `scripts/systems/memory_manager.gd` — synthesize(), has_synthesizable_pair(), SYNTHESIS_NAMES
- `scripts/ui/memory_ui.gd` — 합성 모드 UI (synth_btn, synthesis_mode, _on_synth_pressed)
- `scenes/battle/battle_scene.gd` — WEAK/RESIST 아이콘, 연소 메뉴 속성 표시
- `scenes/maps/rim_forest.gd` — 인터랙티브 오브젝트 (상자+단서)
- `scenes/maps/verdan_market.gd` — 인터랙티브 오브젝트 (상자+단서)
- `scenes/maps/crumbling_coast.gd` — 인터랙티브 오브젝트 (상자x2+단서)
- `scenes/maps/the_seam.gd` — 인터랙티브 오브젝트 (상자+단서), 보스 약점/저항
- `scenes/maps/bl07_void.gd` — 인터랙티브 오브젝트 (상자+단서), Memory Eater 약점/저항

### 다음
- 맵 간 자유 이동 (월드맵 / 빠른 이동)
- 전투 궁극기 시스템 (게이지 축적 → 강력 일격)
- 기억 잔존 활용 (잔존 기억으로 약한 스킬 재사용)

---

## S38 — 2026-04-08 (Fast Travel + Limit Break + Residue 재사용 + 버그 수정)

### 버그 수정
- [x] **Burn DoT 등급 반전:** `memory.grade <= 2`가 Grade 5~3(낮은 등급)을 선택하던 문제 → `>= MemoryGrade.GRADE_2`로 수정 (Grade 2=3, Grade 1=4만 DoT 부여)
- [x] **보이드 수 이중 감쇠:** resistance="physical" + is_void_beast 0.3배가 중첩되어 0.21배 → resistance="" 로 변경 (is_void_beast 감쇠만 적용)

### 완료
- [x] **Fast Travel 시스템:**
  - PauseMenu에 "Travel" 버튼 추가 (Journal과 Codex 사이)
  - 맵 선택 오버레이: 5개 맵 (림 포레스트~BL-07)
  - 챕터 진행도에 따른 맵 해금 (현재 챕터 이상만 이동 가능)
  - 미해금 맵은 "???" + 비활성화
  - 선택 즉시 SceneTransition으로 이동, ESC로 닫기

- [x] **Limit Break 궁극기:**
  - limit_gauge: 0~100 게이지, 전투 시작 시 리셋
  - 게이지 축적: 공격(+8), 연소(+12), 피격(+15), 방어(+5)
  - 게이지 100% 시 LIMIT 버튼 활성화 (보라색 강조)
  - Memory Cascade: 300 + 챕터보너스(40/ch) + 연소보너스(15/burn) 데미지
  - VOID 속성 공격 + 적 약화 2턴 부여
  - 전투 UI: 게이지 바 (플레이어 HP 우측), 꽉 찬 상태 색상 변경

- [x] **기억 잔존 재사용 (Residue Burn):**
  - 연소된 기억 중 is_residue=true인 기억을 전투에서 재사용
  - 50% 데미지, 기억 소멸 없음 (반복 사용 가능)
  - 연소 메뉴 하단에 [RESIDUE] 섹션으로 표시 (보라색 테마)
  - MemoryManager에 get_residue_memories(), get_residue_memory() 추가
  - BattleManager에 player_burn_residue() 추가

### 변경/생성된 파일
- `scripts/systems/battle_manager.gd` — Limit Break 시스템, Residue Burn, 버그 수정 2건
- `scripts/systems/memory_manager.gd` — get_residue_memories(), get_residue_memory()
- `scenes/battle/battle_scene.gd` — Limit 게이지 UI, LIMIT 버튼, Residue 번 메뉴
- `scripts/ui/pause_menu.gd` — Travel 버튼, 맵 선택 오버레이

### 다음
- 전투 AI 패턴 강화 (적 행동 다양화)
- 맵 시각 다양성 (맵별 고유 오브젝트/디테일)
- 기억 관련 사이드퀘스트

---

## S39 — 2026-04-08 (전술적 AI + 사이드 퀘스트 + 맵 데코레이션)

### 버그 수정
- [x] **SideQuest null 체크:** `_find_quest()`가 `{}`를 반환하지만 호출부에서 `== null`로 비교 → `.is_empty()`로 변경 (4개소)
- [x] **Limit 게이지 비-데미지 능력 축적:** shield/summon/weaken 등 피해 없는 능력에도 `_add_limit(LIMIT_GAIN_HIT)` 호출 → 데미지 능력(drain/multi_hit/burn_attack)에만 호출하도록 이동
- [x] **보스 턴 카운터 드리프트:** `_boss_turn_counter += 1`이 확률 체크 전에 실행되어 능력 미사용 시에도 증가 → 확률 체크 후로 이동

### 완료
- [x] **전투 AI 강화:**
  - 랜덤 능력 선택 → 전술적 `_select_ability()` 도입
  - HP 비율/플레이어 상태/쉴드 유무에 따른 능력 우선순위
  - "summon" 능력 추가 (최대HP 15% 회복 + 플레이어 약화)
  - 보스 페이즈2 분노 패턴: 매 3턴 1.3배 강화 + multi_hit 3연타
  - `_boss_turn_counter` 페이즈2 전용 카운터

- [x] **사이드 퀘스트 시스템:**
  - `SideQuest` 유틸리티 클래스 (class_name, 비-오토로드)
  - 3개 퀘스트: Echoes in the Ash (림), The Sump Ledger (시장), Sable's Vigil (심)
  - 플래그 기반 단계 진행 (GameManager.story_flags)
  - 보상: Grains + 아이템 + 고유 기억
  - 맵별 NPC/단서 트리거 배치 (림/시장/심)
  - 대화 데이터 추가 (chapter1/2/4_dialogue.json)
  - StoryJournal "Quests" 탭 추가 (상태별 색상 표시)
  - "all_quests" 업적 (Memory Hunter) + check_quest_complete()

- [x] **맵 시각 다양성 (5개 맵):**
  - 림 포레스트: 발광 버섯 + 쓰러진 통나무
  - 베르단 시장: 걸린 랜턴 + 연기 효과
  - 크럼블링 코스트: 조수 웅덩이 + 표류목
  - The Seam: 크리스탈 포메이션 + 덩굴
  - BL-07: 기억 파편 + 보이드 균열

### 변경/생성된 파일
- `scripts/systems/battle_manager.gd` — 전술적 AI, summon 능력, 분노 패턴, 버그 2건 수정
- `scripts/utils/side_quest.gd` — **신규** 사이드 퀘스트 유틸리티
- `scripts/ui/story_journal.gd` — Quests 탭 추가
- `scripts/ui/achievement_manager.gd` — all_quests 업적, check_quest_complete()
- `scenes/maps/rim_forest.gd` — 퀘스트 트리거 + 발광 버섯/통나무 데코
- `scenes/maps/verdan_market.gd` — 퀘스트 트리거 + 랜턴/연기 데코
- `scenes/maps/crumbling_coast.gd` — 조수 웅덩이/표류목 데코
- `scenes/maps/the_seam.gd` — 퀘스트 트리거 + 크리스탈/덩굴 데코
- `scenes/maps/bl07_void.gd` — 기억 파편/보이드 균열 데코
- `data/chapter1_dialogue.json` — 퀘스트 대화 3개
- `data/chapter2_dialogue.json` — 퀘스트 대화 4개
- `data/chapter4_dialogue.json` — 퀘스트 대화 2개

### 다음
- 추가 폴리싱 및 밸런스 조정
- 사운드/VFX 보강

---

## S40 — 2026-04-08 (셰이더 시스템 + 그래픽 대폭 개선)

### 완료
- [x] **커스텀 셰이더 5종 신규:**
  - `water_distortion.gdshader` — 이중 사인파 왜곡 + 수면 반짝임
  - `vignette.gdshader` — 부드러운 원형 비네트 (직사각형 → 래디얼)
  - `dissolve.gdshader` — 노이즈 기반 디졸브 + 엣지 글로우 (적 사망)
  - `chromatic_aberration.gdshader` — 색수차 펄스 (Limit Break 연출)
  - `glow_pulse.gdshader` — 맥동 글로우 (랜턴/크리스탈/보이드)

- [x] **맵 비네트 셰이더 전환:**
  - map_effects.gd: 직사각형 4면 비네트 → 셰이더 기반 원형 비네트
  - battle_scene.gd: 전투 비네트도 셰이더 적용
  - 폴백: 셰이더 로드 실패 시 기존 방식 유지

- [x] **물 왜곡 셰이더:**
  - 물 타일 영역에 실시간 웨이브 왜곡 오버레이 자동 배치
  - row 단위 연속 물 구간 그룹핑 (성능 최적화)
  - 기존 ColorRect 반짝임과 셰이더 오버레이 공존

- [x] **전투 VFX 대폭 강화:**
  - 데미지 숫자 색상 분류: 화염(주황)/보이드(보라)/드레인(연초록)/독(녹색)/콤보(금색)/회복(초록)
  - 슬래시 VFX: 길이 확장 애니메이션 + 크로스 슬래시 + 충격 파편 입자
  - 보이드 전용 VFX: 보라색 방사형 파티클 폭발 (기억 연소/Limit Break)
  - 적 사망 디졸브: 셰이더 기반 노이즈 디졸브 + 보라색 엣지 글로우
  - Limit Break 색수차: 전체 화면 크로마틱 펄스 + 강한 셰이크

- [x] **원형 와이프 전환 (아이리스):**
  - scene_transition.gd에 change_scene_iris() 추가
  - 셰이더 기반 원형 닫힘/열림 (부드러운 엣지)
  - CG/보스전 전환에 활용 가능

- [x] **랜턴/보이드 글로우 셰이더:**
  - 랜턴: glow_pulse 셰이더로 자연스러운 촛불 맥동
  - 보이드 파티클: 환경 글로우 오버레이 추가

### 변경/생성된 파일
- `assets/shaders/water_distortion.gdshader` — **신규**
- `assets/shaders/vignette.gdshader` — **신규**
- `assets/shaders/dissolve.gdshader` — **신규**
- `assets/shaders/chromatic_aberration.gdshader` — **신규**
- `assets/shaders/glow_pulse.gdshader` — **신규**
- `scripts/utils/map_effects.gd` — 셰이더 비네트/물 왜곡/랜턴 글로우/보이드 글로우
- `scenes/battle/battle_scene.gd` — 셰이더 비네트, 데미지 색상, 디졸브, 색수차, 보이드 VFX, 슬래시 개선
- `scripts/core/scene_transition.gd` — 원형 와이프 전환

### 다음
- 밸런스 조정 및 추가 폴리싱

---

## S41 — 2026-04-08 (대규모 업그레이드: 플러그인+전투VFX+장비+UX+성능)

### 완료

- [x] **플러그인 다운로드 (3종):**
  - Dialogic 2 — 비주얼 대화 에디터
  - ShaderV — 2D 비주얼 셰이더 노드 라이브러리
  - GODOT-VFX-LIBRARY — 전투 VFX 씬 컬렉션

- [x] **전투 VFX 강화:**
  - 상태이상 비주얼 (적 스프라이트 틴트: 독=초록 맥동, 화상=주황 깜빡, 약화=파란 톤)
  - 콤보 버스트 VFX (금색 텍스트 스케일 펀치 + 방사형 파티클)
  - 턴 순서 미리보기 (상단 PLAYER/ENEMY 턴 큐 3턴 표시)

- [x] **장비 시스템:**
  - 10종 장비 (무기 4, 방어구 3, 액세서리 3)
  - ATK/DEF 스탯 보너스, 전투에 자동 적용
  - 특수 효과: burn_boost(+20% 연소), void_resist(-25% 보이드 피해)
  - MemoryShop "Equip" 탭으로 구매/장착
  - 세이브/로드에 장비 데이터 포함

- [x] **보스 전투 패턴 확장:**
  - void_pulse: 데미지 + 콤보 초기화
  - despair: 독 + 약화 동시 부여
  - 장비 방어력 적 공격 시 피해 감소

- [x] **UX 개선:**
  - 퀘스트 트래커 HUD (ExplorationHUD에 활성 퀘스트 표시)
  - 장비 상태 HUD 표시
  - 세이브 슬롯 정보 확장 (위치/HP/Grains 표시)
  - 지형별 발걸음 SFX (풀/모래/돌/물 4종)

- [x] **성능 최적화:**
  - 셰이더 캐시 시스템 (MapEffects._shader_cache)
  - 물 반짝임 간격 3→5로 줄여 ColorRect 40% 감소
  - 맵 비네트/글로우/물 왜곡 모두 캐시 적용

- [x] **버그 수정 (이전 세션):**
  - 전투 승리 후 화면 프리즈 (SceneTransition await 누락)
  - 미니맵 시그널 누수 (freed 노드 참조)
  - 세이블 적 처치 미감지

### 변경/생성된 파일
- `addons/dialogic/` — **신규** (플러그인)
- `addons/shaderV/` — **신규** (플러그인)
- `addons/vfx_lib/` — **신규** (플러그인)
- `scripts/core/game_manager.gd` — 장비 시스템 (EQUIPMENT, equip/export/import)
- `scripts/systems/battle_manager.gd` — 장비 방어력, 연소 부스트, 보스 능력 2종, 세이블 킬 감지, 전투 후 await
- `scenes/battle/battle_scene.gd` — 상태이상 비주얼, 콤보 버스트, 턴 미리보기
- `scripts/ui/exploration_hud.gd` — 퀘스트 트래커, 장비 표시
- `scripts/ui/memory_shop.gd` — 장비 탭 (구매/장착)
- `scripts/ui/minimap.gd` — freed 노드 가드
- `scripts/core/player.gd` — 지형별 SFX
- `scripts/systems/audio_manager.gd` — 지형 SFX 4종 (sand/stone/water)
- `scripts/systems/save_manager.gd` — 세이브 정보 확장
- `scripts/utils/map_effects.gd` — 셰이더 캐시, 물 반짝임 최적화

### 다음
- 플러그인 Godot 에디터에서 활성화 (Project > Project Settings > Plugins)
- 장비 밸런스 테스트
- Dialogic 2로 대화 시스템 마이그레이션 검토

---

## S42 — 2026-04-08 (그래픽 대규모 업그레이드)

### 목표
캐릭터, 맵, 전투 전반의 시각적 품질 대폭 개선.

### 완료

#### 1. 2D 조명 시스템 (PointLight2D + CanvasModulate)
- `MapEffects`에 `add_ambient_lighting()`, `add_point_light()`, `add_tile_lights()`, `update_point_lights()` 추가
- 프로시저럴 원형 라이트 텍스처 생성 (`_create_light_texture()`)
- 5개 맵 전체 적용:
  - rim_forest: 어두운 숲 분위기 (0.55 ambient) + 버섯 초록 라이트
  - verdan_market: 따뜻한 시장 (0.5 amber ambient) + 노점 라이트
  - crumbling_coast: 폭풍 해안 (0.5 cool ambient)
  - the_seam: 은신처 (0.4 dim ambient) + 랜턴 PointLight2D 자동 배치
  - bl07_void: 보이드 (0.3 purple ambient) + 코어/파편 보라 라이트

#### 2. 패럴랙스 배경
- `MapEffects.add_parallax_background()` — ParallaxBackground + 3레이어 (하늘/산/중경)
- 바이옴별 중경 요소: 나무/바위/건물/크리스탈 실루엣 프로시저럴 생성
- 하늘 그라디언트 (위 밝음→아래 어둠)
- 5개 맵에 바이옴별 색상+요소 적용

#### 3. 캐릭터 스프라이트 48x48 업그레이드
- `PixelSprite` SIZE 32→48으로 확대 (2.25배 픽셀 밀도)
- 눈 2x2 + 하이라이트 + 눈동자 + 눈썹 추가
- 입, 코, 볼 하이라이트, 귀 디테일 강화
- 코트 라펠, 주름, 벨트+버클, 하단 테두리
- 부츠 하이라이트+상단 테두리
- 팔 스윙 진폭 ±2px로 확대
- player.gd, companion.gd, npc.gd — SPRITE_SIZE 48로 업데이트

#### 4. 전투 파티클 VFX (GPUParticles2D)
- `_play_gpu_slash_particles()` — 물리공격 방사형 스파크 (24 particles)
- `_play_gpu_burn_particles()` — 화염 상승 (40 particles + 열기 오버레이)
- `_play_gpu_void_particles()` — 보이드 방사형 폭발 (50 particles + 에너지 링)
- `_play_heal_vfx()` — 힐 상승 초록 파티클 (25 particles)
- `_play_limit_burst_vfx()` — 리밋 브레이크 80 particles + 백색 플래시
- 기존 ColorRect VFX에 GPU 파티클 오버레이로 동시 재생

#### 5. 전투씬 연출 강화
- `_add_battle_atmosphere()` — 배경 먼지 파티클 20개 상시 + 컬러 그레이딩 오버레이
- 적 이름 기반 파티클/그레이딩 색상 (보이드=보라, 일반=재)
- 스크린 셰이크 강화: 프레임 수 = 6+intensity×2, 진폭 ±7px

### 수정/생성 파일
- `scripts/utils/pixel_sprite.gd` — 48x48 전면 리라이트
- `scripts/utils/map_effects.gd` — 조명+패럴랙스 시스템 추가
- `scenes/battle/battle_scene.gd` — GPU 파티클 VFX 6종 + 분위기 시스템
- `scenes/maps/rim_forest.gd` — 조명+패럴랙스 적용
- `scenes/maps/verdan_market.gd` — 조명+패럴랙스 적용
- `scenes/maps/crumbling_coast.gd` — 조명+패럴랙스 적용
- `scenes/maps/the_seam.gd` — 조명+패럴랙스 적용
- `scenes/maps/bl07_void.gd` — 조명+패럴랙스 적용
- `scripts/core/player.gd` — SPRITE_SIZE 48
- `scripts/core/companion.gd` — SPRITE_SIZE 48
- `scripts/core/npc.gd` — SPRITE_SIZE 48

### 다음
- S43 그래픽 심화 개선

---

## S43 — 그래픽 심화 개선: 캐릭터 아웃라인, 타일 텍스처, 애니메이션 타일, 적 스프라이트, 색상 팔레트 (2026-04-08)

### 목표
S42 이후에도 아쉬운 그래픽 품질을 한 단계 더 끌어올리기 위한 5가지 심화 개선.

### 구현 내용

#### 1. 캐릭터 아웃라인 (1px 외곽선)
- `PixelSprite._add_outline()` — 48x48 스프라이트 8방향 이웃 체크, 투명 픽셀에 아웃라인 배치
- `_add_outline_64()` — 64x64 적 스프라이트용
- 모든 캐릭터(아렐/엘리아/세이블/NPC) 자동 적용

#### 2. 타일 텍스처 강화
- `TilePainter` 전면 리라이트: 풀(다층 노이즈+18풀잎+꽃4색), 물(듀얼웨이브+4줄파도), 돌(사인파텍스처+균열+이끼), 나무(수피텍스처+3층캐노피+엣지잎), 길(10자갈+발자국), 모래(듀얼웨이브+조개), 보이드(맥동에너지링)

#### 3. 애니메이션 타일
- `MapEffects.add_grass_sway()` — 풀 타일에 흔들리는 풀잎 배치 (4타일당 1개)
- `MapEffects.update_grass_sway()` — sin() 기반 회전 애니메이션
- `MapEffects.add_fire_particles()` — 랜턴 타일에 GPUParticles2D 불꽃 (6파티클, 주황→빨강)
- 림 외곽 숲: 풀 흔들림, The Seam: 불꽃 파티클

#### 4. 적 스프라이트 생성 (64x64)
- `PixelSprite.create_enemy_sprite()` — 적 종류별 전용 스프라이트
- 5종 전용 드로어: Void Beast(보라 갑각+빛나는 눈), Shadow Wisp(유령형), Memory Eater(턱+빛나는 코어), Shade Sentinel(갑옷+검), Void Stalker(가시+꼬리)
- `_draw_generic_enemy()` — 미등록 적은 이름 해시 기반 색상
- 전투씬 `_build_enemy_sprite()`에서 ColorRect 대신 적용

#### 5. 색상 팔레트 갱신
- 캐릭터: 더 채도 높은 색상 (아렐 파랑 강화, 엘리아 금빛, 세이블 은색)
- 림 외곽 숲: 풀 0.15,0.32,0.12 (진한 녹색), 물 0.08,0.18,0.32 (깊은 파랑)
- 베르단 시장: 돌 0.32,0.3,0.28, 가판대 0.42,0.3,0.18 (따뜻한 톤)

### 버그 수정
- `battle_scene.gd` — `BattleManager.enemy_name` → `BattleManager.current_enemy.name` (크래시 수정)
- `exploration_hud.gd` — `SideQuest.has_method()` 비정적 호출 → `SideQuest.get_all_quests()` 정적 호출로 변경

### 수정/생성 파일
- `scripts/utils/pixel_sprite.gd` — 아웃라인 + 적 스프라이트 + 색상 갱신
- `scripts/utils/tile_painter.gd` — 7종 타일 텍스처 전면 강화
- `scripts/utils/map_effects.gd` — 풀 흔들림 + 불꽃 파티클
- `scenes/battle/battle_scene.gd` — 적 스프라이트 생성 적용
- `scenes/maps/rim_forest.gd` — 풀 흔들림 + 색상 팔레트
- `scenes/maps/verdan_market.gd` — 색상 팔레트
- `scenes/maps/the_seam.gd` — 불꽃 파티클

### 다음
- S44 전투씬 비주얼 오버홀

---

## S44 — 전투씬 사이드뷰 오버홀: 캐릭터/적 128x128, 전투 애니메이션, 삽화 느낌 연출 (2026-04-08)

### 목표
전투씬에 캐릭터와 몬스터가 실제로 표시되는 사이드뷰 레이아웃으로 전면 개편. 삽화에 가까운 비주얼 연출.

### 구현 내용

#### 1. 전투 전용 128x128 대형 스프라이트 (PixelSprite 확장)
- `create_battle_sprite(who)` — 아렐/엘리아/세이블 전투 포즈 (사이드뷰)
  - 아렐: 검을 든 전투 자세, 사이드뷰 (오른쪽 바라봄)
  - 엘리아: 손 모은 기도 자세, 브로치 강조
  - 세이블: 주먹 쥔 전투 자세, 흉터 디테일
- `create_battle_enemy(enemy_type)` — 5종 전투용 대형 적 스프라이트
  - Void Beast (128x128): 갑각 패턴, 4다리, 보이드 연기, 이빨
  - Shadow Wisp: 유령형, 코어 빛, 떠다니는 입자
  - Memory Eater: 곤충형, 등딱지 무늬, 더듬이
  - Shade Sentinel: 보스 갑옷, 검, 보이드 에너지 코어
  - Void Stalker: 3눈 인간형, 뿔/가시, 긴 팔
- `_add_outline_n()` — N사이즈 범용 아웃라인
- `_bellipse()` — 타원 채우기 헬퍼

#### 2. 사이드뷰 전투 레이아웃
- 왼쪽(x=120): 아렐 스프라이트 (200x200 영역, 포트레이트 또는 픽셀)
- 왼쪽 뒤(x=20): 동행자 스프라이트 (엘리아/세이블, 160x160)
- 오른쪽(x=820): 적 스프라이트 (260x260 영역)
- 하단 58%: 전투 지면 (그라운드 플랫폼, 경계선, 그라데이션)
- 각 캐릭터 아래 그림자 (ColorRect, 투명도)
- 각 캐릭터 발밑 광원 (색상별 은은한 빛)

#### 3. 전투 애니메이션
- 아이들 호흡: 모든 캐릭터/적에 sin() 기반 상하 미세 움직임 (각기 다른 위상)
- 공격 돌진: `_player_attack_rush()` — 아렐이 적 방향으로 빠르게 이동 후 복귀 (BACK ease)
- 피격 밀림: 맞은 대상이 반대 방향으로 살짝 밀렸다 돌아옴
- 피격 깜빡임: 플레이어 피격 시 빨간 틴트, 적 피격 시 흰색 플래시

#### 4. 삽화 느낌 연출
- 속도선: `_play_speed_lines()` — 공격 시 화면 가로 빗금 6줄 (빠르게 날아감)
- 임팩트 버스트: `_play_impact_burst()` — 타격점에 원형 플래시 + 방사선 4개
- 발밑 광원: 캐릭터별 색상 (파랑/금색/보라)
- HP 옆 미니 포트레이트: 아렐 초상화 (52x52)
- VFX 좌표 전부 사이드뷰 기반으로 이동 (적 위치 920,310)
- 로그 패널 중앙 하단 재배치

### 수정/생성 파일
- `scripts/utils/pixel_sprite.gd` — 전투 128x128 스프라이트 시스템 전체 추가
- `scenes/battle/battle_scene.gd` — 사이드뷰 레이아웃 + 애니메이션 전면 개편

### 다음
- F5 실행 테스트: 사이드뷰 레이아웃, 캐릭터/적 표시, 공격 돌진 확인

---

## S45 — 2026-04-09 (삽화 56장 통합 + CG/포트레이트 대규모 업그레이드)

### 완료

#### 1. 이미지 식별 및 분류
- `../이미지/` 폴더 64개 파일 전수 조사 (Read 도구로 시각 확인)
- 포트레이트 16장 + CG 40장 = 56장 매핑 완료

#### 2. 포트레이트 16장 추가 (총 27장)
- 아렐: determined, sad, cold, rage, pensive, battle (기존 5 + 신규 6 = 11장)
- 엘리아: hopeful, sad, determined, calm, side, void (기존 2 + 신규 6 = 8장)
- 말렛: desk (기존 1 + 신규 1 = 2장)
- 네라: neutral (신규 NPC)
- 세릭: neutral (신규 NPC)
- 토비아스: neutral (신규 NPC)

#### 3. CG 40장 추가/교체 (총 71장)
- 아렐 전투: combat(업그레이드), combat2, combat3, wounded(업그레이드)
- 엘리아: reading, healing
- 풍경: ch1_forest2, ch1_ash_forest, ch1_ash_rain2, ch1_green_tree2/3
- 베르단: verdan_city, ch2_verdan2/3/4, ch2_verdan_overlook, ch2_sump_interior2
- 추출: ch2_extraction2/3
- 보이드: void_portal2, void_swirl, void_beast2/3, crumbling_coast2
- 세계관: bureau_tower/2, bureau_hall, frozen_city, seam_forest, village_seam2/3
- 아이템: item_sword, item_memory_vial, item_extractor2, authority_pills, memory_artifacts
- 기타: wasteland, ash_crawler2, arrel_combat3
- Cover2.png → cover.png (타이틀 화면 업그레이드)

#### 4. DialogueBox PORTRAIT_MAP 확장
- 16개 신규 포트레이트 키 등록
- DEFAULT_PORTRAITS에 Nera, Seric, Tobias 추가

#### 5. CG 참조 업그레이드 (6개 JSON + 2개 GDScript)
- Ch1: ash_rain → ash_rain2 (아렐+엘리아 동행 장면)
- Ch1: green_tree → green_tree2 (선명한 녹색 나무)
- Ch2: verdan → verdan2, sump_interior → sump_interior2, extraction → extraction2
- Ch3: seam_arrival → village_seam2
- Ch4: village_seam → village_seam2 (2곳), void_portal → void_portal2
- Ch5: void_portal → void_portal2, item_memory_ampoule → item_memory_vial (2곳)
- Ch6: village_seam → village_seam2 (4곳)
- the_seam.gd: village_seam → village_seam2 (전투 배경 3곳)

### 수정/생성 파일
- `assets/portraits/` — 16장 신규 추가
- `assets/cg/` — 40장 신규 추가/교체
- `scripts/ui/dialogue_box.gd` — PORTRAIT_MAP + DEFAULT_PORTRAITS 확장
- `data/chapter1_dialogue.json` — CG 1곳 업그레이드
- `data/chapter2_dialogue.json` — CG 3곳 업그레이드
- `data/chapter3_dialogue.json` — CG 1곳 업그레이드
- `data/chapter4_dialogue.json` — CG 3곳 업그레이드
- `data/chapter5_dialogue.json` — CG 3곳 업그레이드
- `data/chapter6_dialogue.json` — CG 4곳 업그레이드
- `scenes/maps/rim_forest.gd` — green_tree CG 교체
- `scenes/maps/the_seam.gd` — 전투 배경 village_seam2로 교체

### 다음
- F5 테스트: 새 CG/포트레이트 표시 확인
- 추가 포트레이트를 대화 JSON에 적용 (감정별 전환)

---

## S46 — 2026-04-09 (게임성/그래픽 개선 — 전투 타격감, VFX, 아군 조작, 기억 연소 반응, 맵 비주얼)

### 완료

#### 1. 전투 타격감 강화
- 히트스톱: 피해량 비례 0.04~0.12초 일시정지 (process_always 타이머)
- 화면 셰이크 스케일링: 데미지 비례 0.5~3.0x 강도
- 콤보 배율 확장: 2연속 1.15x → 6+ 연속 2.0x (기존 3단계→5단계)
- 콤보 마일스톤 보상: 3/5/7 연속 시 Limit 게이지 보너스 (+5/+10/+20)
- 보스 페이즈2 극적 전환: 0.4초 프리즈 + 빨간 화면 플래시 + "PHASE 2" 경고 + 3.0x 셰이크 + 색수차 + outline_glow 셰이더

#### 2. VFX Library 셰이더 전투 적용
- flash_white.gdshader: 적 피격 시 백색 플래시 (flash_amount 트윈)
- poison.gdshader: 독 상태이상 녹색 펄스 (status_changed 시그널 연동)
- burning.gdshader: 화상 상태이상 열 왜곡 + 엣지 버닝
- outline_glow.gdshader: 보스 페이즈2 빛나는 아웃라인
- dissolve.gdshader: 적 사망 시 디졸브 효과 (준비)

#### 3. 아군 조작 시스템 (세이블)
- BattleManager.set_ally_command(): 플레이어 지정 행동 설정
- ally_command_pending 플래그: 턴 종료 시 자동 랜덤 대신 지정 행동 실행
- 전투 UI: Heal/Strike/Weaken/Guard 4버튼 HBoxContainer
- 선택 하이라이트: 선택된 버튼 노란색, 나머지 회색
- Guard 방어 행동: 50% 데미지 감소 (player_defending 플래그)

#### 4. 기억 연소 월드 반응
- desaturation.gdshader: 연소량 비례 채도 감소 (0~40%) + 보이드 보라 틴트 (5회+ 연소 시)
- MapEffects.add_burn_desaturation(): CanvasLayer 기반 풀스크린 포스트프로세싱
- 5개 맵 전체 적용 (rim_forest, verdan_market, crumbling_coast, the_seam, bl07_void)

#### 5. 맵 비주얼 강화
- 반딧불 파티클: 림 숲(초록 12개) + The Seam(앰버 20개) — GPUParticles2D + 그라디언트 페이드
- 열기 왜곡 셰이더: heat_haze.gdshader (screen_texture + 사인파 왜곡) — 해안/시장 적용
- MapEffects.add_fireflies(): 범용 반딧불 생성기 (색상/수량 파라미터)
- MapEffects.add_heat_haze(): 범용 열 왜곡 효과 (강도 파라미터)
- MapEffects.update_ambient_pulse(): 동적 CanvasModulate 색상 변화 유틸

### 신규/수정 파일
| 파일 | 작업 |
|------|------|
| `scripts/systems/battle_manager.gd` | 콤보 확장, 아군 커맨드, 방어, 보스 페이즈 시그널 |
| `scenes/battle/battle_scene.gd` | 히트스톱/셰이크/VFX 셰이더/아군 UI/보스 페이즈 연출 |
| `assets/shaders/desaturation.gdshader` | **신규** — 기억 연소 채도 감소 |
| `assets/shaders/heat_haze.gdshader` | **신규** — 대기 열 왜곡 |
| `scripts/utils/map_effects.gd` | add_burn_desaturation/add_fireflies/add_heat_haze/update_ambient_pulse |
| `scenes/maps/rim_forest.gd` | 반딧불 + 연소 탈색 |
| `scenes/maps/verdan_market.gd` | 열 왜곡 + 연소 탈색 |
| `scenes/maps/crumbling_coast.gd` | 열 왜곡 + 연소 탈색 |
| `scenes/maps/the_seam.gd` | 반딧불 + 연소 탈색 |
| `scenes/maps/bl07_void.gd` | 연소 탈색 |

### 다음
- F5 테스트: 전투 VFX + 맵 비주얼 확인
- 감정별 포트레이트 대화 JSON 적용

---

## S47 — 2026-04-10 (삽화 93장 대규모 통합)

### 개요
유저가 assets/cg/에 93개의 새 이미지 파일 추가. 한국어/gemini 파일명을 게임용으로 리네이밍하고, 포트레이트 19장 분리, CG ~60장 정리, 전체 대화 JSON + 맵 전투 스크립트에 연결.

### 완료

#### 1. 이미지 리네이밍 및 분류 (~93파일)
- 한국어/gemini 파일명 → 영문 게임용 이름 (예: "기억 원혼" → memory_wraith2.jpg)
- 포트레이트용 19장 → `assets/portraits/`로 복사
- CG용 ~60장 → `assets/cg/`에 정리

#### 2. PORTRAIT_MAP 확장 (dialogue_box.gd)
- 19개 신규 키 추가: arrel_default2, arrel_cold2, arrel_heroic, arrel_wounded2, arrel_burn, arrel_exhausted, elia_wind, elia_default2, elia_void2, elia_calm2, elia_wind2, elia_mature, nera_bureau, malet_smirk, malet_casual, seric_clipboard, sable_portrait, kairos_portrait, tobias_uniform
- 총 PORTRAIT_MAP 46키

#### 3. 대화 JSON CG/포트레이트 업그레이드 (~42개 편집)
- **chapter1_dialogue.json** (6): ch1_twisted_forest, arrel_combat4, ch1_ash_walk, elia_wind, ch1_campfire, ch1_stump2, ch1_arrel_ghost
- **chapter2_dialogue.json** (7): verdan5, sump_entrance, malet_smirk, sump_interior3, item_extractor3, extraction4, arrel_exhausted, ch2_arrel_malet
- **chapter3_dialogue.json** (5): ch3_seam_coast, ch3_arrel_elia_coast, ch3_kairos_cliff, village_seam3, ch3_elia_healing
- **chapter4_dialogue.json** (6): village_seam3, ch4_sable_house, ch5_void_entrance, ch4_sable_porch, ch4_elia_window, ch1_elia_flower
- **chapter5_dialogue.json** (6): ch5_void_entrance, void_islands, ch5_arrel_elia_void, battle_memory_burn, ch5_climax, arrel_burn
- **chapter6_dialogue.json** (12): village_seam4, arrel_exhausted, elia_mature, ch4_arrel_sable, ch5_climax2, arrel_wounded2, great_tree.png, ch4_sable_porch

#### 4. 맵 전투 배경/적 이미지 업그레이드 (5개 맵)
- **rim_forest.gd**: ch1_twisted_forest/twisted_forest2, ash_crawler3, void_beast3
- **crumbling_coast.gd**: ch3_seam_coast, void_beast3
- **the_seam.gd**: village_seam3, memory_wraith2, ch5_void_entrance, void_husk
- **bl07_void.gd**: void_islands.png, memory_wraith3, void_husk2
- **verdan_market.gd**: (S46에서 이미 업그레이드)

### 신규/수정 파일
| 파일 | 작업 |
|------|------|
| `assets/cg/` | ~60 CG 파일 리네이밍 |
| `assets/portraits/` | 19장 신규 포트레이트 복사 |
| `scripts/ui/dialogue_box.gd` | PORTRAIT_MAP 19키 추가 (총 46키) |
| `data/chapter1_dialogue.json` | CG/포트레이트 6곳 업그레이드 |
| `data/chapter2_dialogue.json` | CG/포트레이트 7곳 업그레이드 |
| `data/chapter3_dialogue.json` | CG/포트레이트 5곳 업그레이드 |
| `data/chapter4_dialogue.json` | CG/포트레이트 6곳 업그레이드 |
| `data/chapter5_dialogue.json` | CG/포트레이트 6곳 업그레이드 |
| `data/chapter6_dialogue.json` | CG/포트레이트 12곳 업그레이드 |
| `scenes/maps/rim_forest.gd` | 전투 이미지 업그레이드 |
| `scenes/maps/crumbling_coast.gd` | 전투 이미지 업그레이드 |
| `scenes/maps/the_seam.gd` | 전투 이미지 업그레이드 |
| `scenes/maps/bl07_void.gd` | 전투 이미지 업그레이드 |

### 다음
- F5 테스트: 새 CG/포트레이트 표시 확인
- 추가 이미지 에셋이 있으면 계속 통합

---

## S48 — 2026-04-10 (스토리 대폭 확장 + NPC 반복 대화 수정)

### 개요
스토리가 너무 빨리 끝나는 문제 해결. Ch1~Ch5 전체에 탐색 중 자동 발생하는 대화/이벤트 24개 추가 (약 355줄). NPC 재대화 반복 버그도 수정.

### 완료

#### 1. NPC 대화 반복 방지
- npc.gd: `_talked_keys` + `talked_{name}_{key}` 플래그로 재대화 차단, `repeat_line` export
- companion.gd: 동일 패턴 적용
- 4개 맵에 맥락별 repeat_line 설정 (Elia/Sable/Malet)
- verdan_market: Malet 거절 시 talked 플래그 리셋 (재시도 흐름 보존)

#### 2. Ch1 대화 확장 (5개 신규, ~65줄)
- elia_forest_walk: 연소 후 관자놀이 만지는 습관 + 버단의 남자 이야기
- elia_memory_talk: 기억 연소 느낌 묘사 ("방에 들어갔는데 왜 왔는지 잊은 것")
- elia_anchor_talk: 앵커 시스템 설명 + "3번 전에도 같은 질문을 했다"
- forest_shrine: 기억 사당 — 사람들이 태운 기억을 위해 만든 제단
- dead_burner: Grade 1 번아웃 시체 — 모든 것을 태운 사람의 말로

#### 3. Ch2 대화 확장 (5개 신규, ~70줄)
- verdan_market_walk: 기억 시장 풍경 ("어머니의 사랑 — Grade 3 — 약간 중고")
- verdan_old_burner: 이름을 잊은 노인 — 아렐의 미래 암시
- malet_backstory: 말렛의 과거 — 전직 관리국 3과, 17명의 기억으로 재건
- elia_sump_concern: Grade 2 이상 거래 금지 약속
- sump_atmosphere: 썸프 지하 분위기 (첫 키스 팔이, 다른 사람의 과거에 빠지지 말 것)

#### 4. Ch3 대화 확장 (5개 신규, ~70줄)
- coast_cliff_walk: 절벽 기억 잔류물 — 도시였던 해안선의 에코
- coast_watchtower: 폐 감시탑 — 수천 개의 연소 기록 탈리 마크
- kairos_presence: 에디터의 프로파일링 설명 — "어떤 기억이 전부를 지탱하는지 찾는다"
- coast_void_crack: 보이드 균열 — 잃어버린 기억이 안에 있다고 초대
- elia_before_separation: 앵커 없는 연소의 위험 설명 (잔류물 없음)

#### 5. Ch4 대화 확장 (5개 신규, ~85줄)
- seam_evening: 저녁 풍경 + 요리 냄새 + 아이들 + 웃음
- sable_past: 세이블의 과거 — 애쉬브릿지 7인 팀 전멸 이야기
- seam_residents: 정원사 NPC — 남편이 결혼식 기억을 태웠지만 매일 정원을 가꾸는 이야기
- elia_night_talk: 밤하늘 아래 엘리아의 고백 — "빛이 꺼지는 걸 보는 게 지쳤다"
- sable_preparation: BL-07 진입 전 전술 브리핑 + 실제 화염 랜턴

#### 6. Ch5 대화 확장 (4개 신규, ~65줄)
- void_descent: 허공에 떠 있는 문 — 집을 먹힌 기억
- void_echoes: 보이드가 보여주는 환각 — "예전에는 더 많이 웃었다"
- void_memory_fragments: 주인 잃은 기억 결정체 플랫폼
- void_before_core: 봉인 직전 엘리아의 최종 대화 — "지금의 당신으로 충분하다"

#### 7. 5개 맵 탐색 트리거 연결
- rim_forest: 5개 Area2D 트리거 (기억 사당, 죽은 버너, 엘리아 대화 3개)
- verdan_market: 5개 트리거 + _add_story_trigger 범용 함수
- crumbling_coast: 5개 트리거 (절벽, 감시탑, 카이로스, 균열, 분리 전 대화)
- the_seam: 5개 트리거 (저녁, 세이블, 주민, 엘리아 밤, BL-07 준비)
- bl07_void: 4개 트리거 (하강, 에코, 파편, 코어 직전)

### 신규/수정 파일
| 파일 | 작업 |
|------|------|
| `scripts/core/npc.gd` | talked 플래그 + repeat_line |
| `scripts/core/companion.gd` | talked 플래그 + repeat_line |
| `data/chapter1_dialogue.json` | 5개 신규 대화 (65줄) |
| `data/chapter2_dialogue.json` | 5개 신규 대화 (70줄) |
| `data/chapter3_dialogue.json` | 5개 신규 대화 (70줄) |
| `data/chapter4_dialogue.json` | 5개 신규 대화 (85줄) |
| `data/chapter5_dialogue.json` | 4개 신규 대화 (65줄) |
| `scenes/maps/rim_forest.gd` | 5개 탐색 트리거 |
| `scenes/maps/verdan_market.gd` | 5개 탐색 트리거 + _add_story_trigger |
| `scenes/maps/crumbling_coast.gd` | 5개 탐색 트리거 + _add_story_trigger |
| `scenes/maps/the_seam.gd` | 5개 탐색 트리거 + _add_story_trigger |
| `scenes/maps/bl07_void.gd` | 4개 탐색 트리거 + _add_story_trigger |

### 총 대화량 변화
- Before: 544줄, 49키
- After: ~900줄, 73키 (~65% 증가)

### 다음
- 10챕터 확장 (S49에서 진행)

---

## S49 — 2026-04-10 (10챕터 확장 Phase 1: 구조 변경 + Ch3/Ch4 신규 맵)

### 목표
Part 1 완성을 위해 6챕터 → 10챕터 확장. Phase 1: 구조 재편 + 새 챕터 2개.

### 완료

#### 1. 챕터 구조 전면 재편
기존 6챕터에서 10챕터(+ 에필로그)로 확장:

| 챕터 | 맵 | 이전 챕터 | 상태 |
|------|------|-----------|------|
| Ch1 | rim_forest | Ch1 | 유지 |
| Ch2 | verdan_market | Ch2 | 유지 |
| **Ch3** | **belt_waystation** | — | **신규** |
| **Ch4** | **drift_shelter** | — | **신규** |
| Ch5 | crumbling_coast | Ch3 | 번호 변경 |
| Ch6 | the_seam | Ch4 | 번호 변경 |
| Ch7-9 | (미구현) | — | 향후 |
| Ch10 | bl07_void | Ch5 | 번호 변경 |
| Epilogue | the_seam | Ch6 | 번호 변경 |

#### 2. 대화 파일 재편
- `chapter3_dialogue.json` → **신규** (Belt Waystation, 8키 ~80줄)
- `chapter4_dialogue.json` → **신규** (Drift Shelter, 6키 ~75줄)
- `chapter5_dialogue.json` ← 구 chapter3 (Crumbling Coast)
- `chapter6_dialogue.json` ← 구 chapter4 (The Seam)
- `chapter10_dialogue.json` ← 구 chapter5 (BL-07 Void)
- `epilogue_dialogue.json` ← 구 chapter6 (에필로그)

#### 3. Ch3: Belt Waystation (Weight of Pages)
- **신규 맵** `belt_waystation.tscn` + `.gd` (25x18 타일)
- 타일: 죽은 토양(회색), 갈라진 도로, 폐허, 벽, 길, 건물 내부
- **토비아스 크레인** NPC (관리국 기록관, Class C)
- 스토리 시퀀스: 도착 → 토비아스 만남 → 백서 발견 → 카이로스 벽 낙서 → 토비아스 합류
- 백서(Blank Book) 아이템: 기억의 형태를 기록하는 기록수 섬유
- "Subject demonstrates Class Seven combustion efficiency" 벽 낙서
- 전투 2개 (Belt Scavenger, Void Wisp)
- 상자 2개 + 단서 1개
- 탐색 이벤트 2개 (belt_atmosphere, tobias_records)
- 랜덤 인카운터 3종

#### 4. Ch4: Drift Shelter (Drift)
- **신규 맵** `drift_shelter.tscn` + `.gd` (25x18 타일)
- 타일: 진흙, 잔해, 콘크리트, 벽, 길, 셸터(지붕)
- 메모리 레인 (재비 파티클)
- 스토리 시퀀스: 도착 → 읽기 능력 저하 → 앵커링 세션
- **앵커링 세션**: 엘리아가 아렐의 손을 잡고 기억 구조 안정화
- 관리국 분류 체계 설명 (Class 1~7, 토비아스)
- 밤 대화: 아렐의 비자발적 기억 소실 (11개 micro-memory 2일 내 소실)
- 전투 2개 (Memory Leech, Rubble Rat)
- 상자 1개 + 단서 2개
- 탐색 이벤트 2개

#### 5. 전체 플래그 체계 업데이트
- ch3→ch5, ch4→ch6, ch5→ch10 플래그 이름 변경 (모든 맵/UI/시스템)
- 에필로그 조건: `current_chapter >= 11` + `ch10_complete`
- 스토리 저널: 6개 신규 엔트리 (Belt, Tobias, Blank Book, Wall Writing, Drift, Anchoring)
- Fast Travel: 7개 맵 (Belt, Drift 추가)
- PauseMenu: 챕터 이름 Dictionary 전환 (비연속 챕터 번호 대응)
- credits.gd: hidden_ch4_garden → hidden_ch6_garden

#### 6. 기억 시스템 확장
- Ch3 기억 2개: "The Taste of Dead Earth" (Grade 5), "The Man Who Writes Everything Down" (Grade 4, Tobias)
- Ch4 기억 2개: "Rain That Isn't Rain" (Grade 5), "Warm Hands on Cold Palms" (Grade 3, Elia)
- 기존 Ch3/4/5 기억 → Ch5/6/10으로 재배치

### 신규/수정 파일
| 파일 | 작업 |
|------|------|
| `scenes/maps/belt_waystation.tscn` | **신규** — Ch3 맵 씬 |
| `scenes/maps/belt_waystation.gd` | **신규** — Ch3 맵 스크립트 |
| `scenes/maps/drift_shelter.tscn` | **신규** — Ch4 맵 씬 |
| `scenes/maps/drift_shelter.gd` | **신규** — Ch4 맵 스크립트 |
| `data/chapter3_dialogue.json` | **재작성** — Belt Waystation (8키) |
| `data/chapter4_dialogue.json` | **재작성** — Drift Shelter (6키) |
| `data/chapter5_dialogue.json` | 구 ch3 내용 (챕터 번호만 변경) |
| `data/chapter6_dialogue.json` | 구 ch4 내용 (챕터 번호 + 타이틀 변경) |
| `data/chapter10_dialogue.json` | 구 ch5 내용 (챕터 번호 변경) |
| `data/epilogue_dialogue.json` | 구 ch6 내용 (새 파일명) |
| `scenes/maps/verdan_market.gd` | Ch2→Ch3 전환 대상 변경 |
| `scenes/maps/crumbling_coast.gd` | 챕터 3→5 번호 변경 |
| `scenes/maps/crumbling_coast.tscn` | dialogue_file 경로 변경 |
| `scenes/maps/the_seam.gd` | 챕터 4→6 번호 변경 + 에필로그 조건 |
| `scenes/maps/the_seam.tscn` | dialogue_file 경로 변경 |
| `scenes/maps/bl07_void.gd` | 챕터 5→10 번호 변경 |
| `scenes/maps/bl07_void.tscn` | dialogue_file 경로 변경 |
| `scripts/systems/memory_manager.gd` | Ch3/4 신규 기억 + 재배치 |
| `scripts/ui/story_journal.gd` | 6개 신규 엔트리 + 플래그 업데이트 |
| `scripts/ui/pause_menu.gd` | 챕터 이름 Dict + Fast Travel 7맵 |
| `scenes/ui/credits.gd` | hidden_ch6_garden 플래그 수정 |

### 총 대화량 변화
- Before: ~900줄, 73키
- After: ~1050줄, 87키 (~17% 증가)

### 챕터 전환 체인
```
Ch1 (rim_forest) → Ch2 (verdan_market) → Ch3 (belt_waystation) → Ch4 (drift_shelter)
→ Ch5 (crumbling_coast) → Ch6 (the_seam) → [Ch7-9 미구현, 직접 점프] → Ch10 (bl07_void) → Epilogue
```

### 다음
- Ch7 (The Other Side of the Flame) 구현
- Ch8 (Forest That Forgets) 구현
- Ch9 (Where Colors Stop) 구현
- 토비아스 전투 동행 시스템 (파티 시스템 확장)

---

## S50 — 2026-04-12 (10챕터 확장 Phase 2: Ch7-9 구현 + 전투 능력 확장)

### 목표
10챕터 구조 완성. Ch7-9 맵/대화/시스템 구현 + 전투 신규 능력 + 전환 체인 완성.

### 완료

#### 1. 전투 시스템 확장 — 3개 신규 적 능력
- **stun** (기절): 약한 데미지 + 다음 플레이어 턴 스킵. 콤보 차단 전술용.
- **reflect** (반사): 배리어 + 다음 공격 30% 데미지 반사.
- **charge** (차지): 1턴 대기 → 다음 적 턴 2배 데미지 강타.
- 전술 AI에 중복 회피 로직 통합 (reflect/charge/stun 중복 방지)
- 스턴 시 플레이어 턴 자동 스킵 (로그 메시지 + 딜레이)

#### 2. Ch7: Seam Outskirts (The Other Side of the Flame)
- **맵** `seam_outskirts.tscn` + `.gd` (25x18 타일)
- 세이블 진실: BL-07은 구멍이 아니라 입. 기억을 부른다.
- 에코 셸 획득 (BL-07 희생자들의 마지막 메아리)
- 세이블 시련 전투 (Threshold Shade: drain/stun/reflect)
- 전투 3개 (Void Sentinel, Ash Phantom, Threshold Crawler)
- 상자 1개 + 단서 2개 + 탐색 이벤트 2개
- **대화** `chapter7_dialogue.json` (8키 ~90줄)

#### 3. Ch8: Forgotten Forest (The Forest That Forgets)
- **맵** `forgotten_forest.tscn` + `.gd` (25x18 타일)
- 기억 기생 숲: 나무가 기억을 먹음, 유령(remnant) NPC
- 토비아스의 링 이론 (동심원 17+1개 = 소비 사건 수)
- 엘리아 앵커링 부담 (아렐이 엘리아 이름을 잊음)
- 숲 속삭임 + 유령 아이 + 돌무더기 안전지대
- 전투 3개 (Memory Leech, Hollow Walker, Root Shade)
- 상자 2개 + 단서 2개 + 탐색 이벤트 4개
- **대화** `chapter8_dialogue.json` (9키 ~95줄)

#### 4. Ch9: Colorless Waste (Where Colors Stop)
- **맵** `colorless_waste.tscn` + `.gd` (25x18 타일)
- 완전 탈색 환경 (모노크롬 타일/패럴랙스/조명)
- 메모리 나침반 획득 (아렐 몸이 BL-07에 반응)
- **카이로스 대면**: 직접 대화. Outcome A/B 두 가지 결말 예측.
- 전투 3개 (Colorless Wraith, Depth Crawler, Void Fragment)
- 상자 1개 + 단서 2개 + 탐색 이벤트 3개
- **대화** `chapter9_dialogue.json` (8키 ~90줄)

#### 5. 전환 체인 완성
- `the_seam.gd`: Ch6 완료 → Ch7 (seam_outskirts)로 변경 (기존: bl07_void)
- Ch7 → Ch8 (forgotten_forest) → Ch9 (colorless_waste) → Ch10 (bl07_void)

#### 6. 기억 시스템 확장
- Ch7 기억 2개: "The Taste of Static" (Gr5), "Voices in the Shell" (Gr3, Sable)
- Ch8 기억 2개: "Trees That Remember Being Trees" (Gr5), "A Ghost's Last Sentence" (Gr4)
- Ch9 기억 2개: "The Place Where Color Stopped" (Gr5), "The Memory Compass" (Gr2)

#### 7. UI 전면 업데이트
- **StoryJournal**: CHAPTER_NAMES 11개 챕터 + 13개 신규 이벤트 엔트리
- **AchievementManager**: Ch7/8/9 완료 업적 3개 추가 (총 28종)
- **PauseMenu**: 챕터 이름 11개 + Fast Travel 10개 맵

### 신규/수정 파일
| 파일 | 작업 |
|------|------|
| `data/chapter7_dialogue.json` | **신규** — Seam Outskirts (8키) |
| `data/chapter8_dialogue.json` | **신규** — Forgotten Forest (9키) |
| `data/chapter9_dialogue.json` | **신규** — Colorless Waste (8키) |
| `scenes/maps/seam_outskirts.tscn` | **신규** — Ch7 맵 씬 |
| `scenes/maps/seam_outskirts.gd` | **신규** — Ch7 맵 스크립트 |
| `scenes/maps/forgotten_forest.tscn` | **신규** — Ch8 맵 씬 |
| `scenes/maps/forgotten_forest.gd` | **신규** — Ch8 맵 스크립트 |
| `scenes/maps/colorless_waste.tscn` | **신규** — Ch9 맵 씬 |
| `scenes/maps/colorless_waste.gd` | **신규** — Ch9 맵 스크립트 |
| `scenes/maps/the_seam.gd` | Ch6→Ch7 전환으로 변경 |
| `scripts/systems/battle_manager.gd` | stun/reflect/charge 3개 능력 추가 |
| `scripts/systems/memory_manager.gd` | Ch7/8/9 기억 6개 추가 |
| `scripts/ui/story_journal.gd` | CHAPTER_NAMES + 13개 이벤트 |
| `scripts/ui/achievement_manager.gd` | Ch7/8/9 업적 3개 |
| `scripts/ui/pause_menu.gd` | 챕터 이름 + Fast Travel 3맵 |

### 총 대화량 변화
- Before: ~1050줄, 87키
- After: ~1325줄, 112키 (~26% 증가)

### 챕터 전환 체인 (완성)
```
Ch1 (rim_forest) → Ch2 (verdan_market) → Ch3 (belt_waystation) → Ch4 (drift_shelter)
→ Ch5 (crumbling_coast) → Ch6 (the_seam) → Ch7 (seam_outskirts) → Ch8 (forgotten_forest)
→ Ch9 (colorless_waste) → Ch10 (bl07_void) → Epilogue
```

### 다음
- 토비아스 전투 동행 (Ch3+ 파티 시스템 확장)
- 카이로스 보스전 (Ch9 또는 Ch10 내부)
- 추가 사이드 퀘스트 (Ch7-9 맵용)
- CG/포트레이트 연결 (카이로스 포트레이트 등)

---

## S51 — 2026-04-12 (게임성 대폭 업그레이드 — 6대 시스템)

### 완료
- [x] **Memory Decay/Erosion** — 기억 침식 시스템 (챕터 진행 시 기억 약화, Grade 1 면역, 엘리아 관련 반감, is_faded/erosion 필드)
- [x] **Memory Echo** — 연소 후 전장 잔류 효과 (등급별 7종: Fading Warmth/Lingering Habit/Elia Anchor/Sable Shadow/Bond Fracture/Identity Fracture/Total Erasure)
- [x] **Battle Stance** — 전투 자세 3종 (Remnant/Pyre/Hollow, 챕터별 해금, 공방 배율 + 고유 효과)
- [x] **Void Corruption Modifiers** — 보이드 부패 인카운터 수정자 (연소 횟수 기반, 4등급 12종, 전투 난이도 동적 변화)
- [x] **Elia Diary** — 엘리아 일지 + 비연소 전투 기술 (일지 8항목, 기술 4종: Humming Shield/Desperate Reach/Remembered Strike/Anchor Pulse, 쿨다운 시스템)
- [x] **Memory Resonance** — 기억 공명 탐색 이벤트 (10맵 18지점, 기억 비전투 연소로 탐색 보너스, 맥동 시각 효과)
- [x] MemoryUI 침식/소실 시각화 (FADED/ERODING 상태, 알파 페이드, 침식 비율 표시)
- [x] 전투 연소 목록에서 Faded 기억 필터링 + 침식 반영 유효 파워 표시
- [x] EliaDiary 오토로드 등록 + SaveManager 연동 (세이브/로드)
- [x] Lingering Habit 에코 → 콤보 배율 +20% 연결

### 신규 파일
| 파일 | 설명 |
|------|------|
| `scripts/utils/encounter_modifiers.gd` | EncounterModifier 클래스 — 연소 횟수별 전투 수정자 |
| `scripts/ui/elia_diary.gd` | EliaDiary 오토로드 — 일지 + 전투 기술 4종 |
| `scripts/utils/memory_resonance.gd` | MemoryResonance 클래스 — 맵 공명 지점 설치 |

### 수정 파일
| 파일 | 변경 내용 |
|------|----------|
| `project.godot` | EliaDiary 오토로드 등록 |
| `scripts/systems/memory_manager.gd` | is_faded/erosion 필드, apply_erosion(), get_effective_burn_power(), burn_memory_silent() |
| `scripts/systems/battle_manager.gd` | Echo/Stance/Modifier/Elia 통합, player_use_elia_skill(), 콤보 Lingering Habit 연결 |
| `scenes/battle/battle_scene.gd` | Stance UI, Echo 표시, Elia 기술 UI, Faded 필터, 침식 파워 표시 |
| `scripts/ui/memory_ui.gd` | 침식/소실 시각화 (FADED/ERODING 표시, 알파 그라데이션) |
| `scripts/systems/save_manager.gd` | EliaDiary 세이브/로드 연동 |
| `scenes/maps/*.gd` (10파일) | MemoryResonance.setup_points() 호출 추가 |

### 다음
- 전투 밸런스 미세 조정 (침식 속도, 에코 지속턴, 수정자 확률)
- 카이로스 보스전 (Ch9)
- 엘리아 일지 UI 뷰어 (MemoryUI Diary 탭)

---

## S52 — 2026-04-12 (그래픽 대규모 업그레이드)

### 완료
- [x] **2D 그림자 시스템** — PointLight2D shadow 활성화 + LightOccluder2D 벽/나무 타일 자동 생성
- [x] **컬러 그레이딩** — 맵별 분위기 색조 보정 (10맵 바이옴별 커스텀 tint/brightness)
- [x] **캐릭터 드롭 섀도우** — 발밑 타원형 그림자 (탐색 10맵 + 전투 기존)
- [x] **캐릭터 호흡 애니메이션** — 정지 시 미세 스케일 펄스 (탐색 + 전투 3캐릭터)
- [x] **바이옴별 향상 파티클** — 꽃가루(숲), 재(황무지), 보이드 촉수(보이드) + 업데이트 루프
- [x] **스무스 카메라** — Camera2D 부드러운 추적, 드래그 마진, 환경 미세 흔들림(비/보이드 맵)
- [x] **전투 크리티컬 줌** — 200+ 데미지 시 화면 줌 펀치 + 임팩트 플래시
- [x] **연소 화면 이펙트** — 기억 연소 시 화면 가장자리 화염 비네트
- [x] **전투 호흡 스케일** — 아군/적 idle에 미세 스케일 변화 추가 (기존 Y bob + 신규 XY scale)

### 수정 파일
| 파일 | 변경 내용 |
|------|----------|
| `scripts/utils/map_effects.gd` | 신규 함수 14개 (그림자/오클루더/컬러그레이딩/드롭섀도/꽃가루/재/보이드촉수/카메라) |
| `scripts/core/player.gd` | 정지 시 호흡 스케일 애니메이션 |
| `scenes/battle/battle_scene.gd` | 크리티컬 줌, 연소 화면 이펙트, 호흡 스케일, 엘리아 기술 UI |
| `scenes/maps/*.gd` (10파일) | 컬러 그레이딩 + 스무스 카메라 + 드롭 섀도우 + 바이옴 파티클 |

### 맵별 비주얼 설정
| 맵 | 컬러 그레이딩 | 파티클 | 특수 |
|----|-------------|--------|------|
| rim_forest | 초록 틴트 | 꽃가루 12 | 그림자+오클루더 |
| verdan_market | 따뜻한 틴트 | 먼지 8 | 그림자+오클루더 |
| belt_waystation | 황토 틴트 | 먼지 15 | — |
| drift_shelter | 푸른 틴트 | (비) | 미세 흔들림 0.3 |
| crumbling_coast | 해안 틴트 | 물보라 10 | — |
| the_seam | 보라 틴트 | 잔불 8 | 그림자+오클루더 |
| seam_outskirts | 짙은 보라 | 촉수 4 | 흔들림 0.4 |
| forgotten_forest | 병적 초록 | 포자 18 | 흔들림 0.3 |
| colorless_waste | 무채색 | 잿가루 20 | — |
| bl07_void | 심연 보라 | 촉수 8 | 그림자+오클루더, 흔들림 0.6 |

### 다음
- 전투 밸런스 미세 조정
- 카이로스 보스전
- 추가 CG 연결

---

## S53 — 2026-04-12 (20대 업그레이드 — 게임성+스토리+비주얼+폴리싱)

### 완료

**게임성 (6종)**
- [x] **토비아스 전투 동행** — Ch3-6 서포트 (analyze/archive/protect), 전투 UI 커맨드
- [x] **기억 체인 번** — 연속 연소 시 +20% 누적 데미지 보너스 (_burn_chain)
- [x] **NG++ 보스 강화** — cycle 3+ 보스에 despair/charge/reflect 능력 추가
- [x] **장비 강화 시스템** — 0~3단계 업그레이드, MemoryShop 업그레이드 버튼, Grains 비용
- [x] **사이드 퀘스트 3종 추가** — echo_fragments(Ch7), forest_parasite(Ch8), colorless_compass(Ch9)
- [x] **전투 밸런스 조정** — 세이블 공격력 12-22, 침식률 50% 감소, 에코 지속시간 조정

**스토리 (4종)**
- [x] **대화 9종 추가** — Ch7-9 신규 대화 + 플래시백 3종 (Ch4/Ch6/Ch8)
- [x] **엔딩 2종 추가** — tobias 엔딩 (동행 희생), hollow 엔딩 (완전 연소), 총 6종
- [x] **카이로스 보스전** — Ch9 450HP/38ATK, 6능력, 클리어 후 대화
- [x] **토비아스 합류 이벤트** — Belt Waystation에서 tobias_joined 플래그

**비주얼 (5종)**
- [x] **NPC 아이들 애니메이션** — 8개 맵 NPC에 미세 호흡/움직임 추가
- [x] **타일 블렌딩** — auto_blend_edges() 타일 경계 자연스러운 혼합
- [x] **동적 날씨** — update_weather_intensity(), 번개 효과 (drift_shelter/crumbling_coast)
- [x] **파티클 풀링** — _get_pooled_rect/_return_to_pool 성능 최적화
- [x] **오프스크린 컬링** — cull_offscreen_particles() 화면 밖 파티클 비활성화

**폴리싱 (5종)**
- [x] **대화 상자 애니메이션** — slide-up/down 등장·퇴장
- [x] **일시정지 메뉴 애니메이션** — slide-in/out 효과
- [x] **접근성 옵션 3종** — 폰트 크기, 화면 흔들림 토글, 색맹 모드
- [x] **신규 포트레이트 3종** — kairos_cold, kairos_amused, tobias_concerned
- [x] **Windows 내보내기 설정** — export_presets.cfg

### 수정 파일 (28개)
| 파일 | 변경 내용 |
|------|----------|
| `scripts/systems/battle_manager.gd` | 토비아스 동행, 체인 번, NG++ 보스 강화 |
| `scenes/battle/battle_scene.gd` | 토비아스 커맨드 UI, 접근성 셰이크 체크 |
| `scripts/core/game_manager.gd` | 장비 강화 시스템, NG++ 보너스, 전용 장비 2종 |
| `scripts/ui/memory_shop.gd` | 장비 업그레이드 버튼 |
| `scripts/utils/side_quest.gd` | Ch7-9 사이드 퀘스트 3종 |
| `scripts/systems/memory_manager.gd` | 침식률 밸런스 (50% 감소) |
| `data/chapter4~9_dialogue.json` | 대화 9종 + 플래시백 3종 |
| `data/epilogue_dialogue.json` | tobias/hollow 엔딩 2종 |
| `scenes/maps/the_seam.gd` | 엔딩 분기 2종 추가, 플래시백 |
| `scenes/maps/colorless_waste.gd` | 카이로스 보스전 + NPC 애니메이션 |
| `scenes/maps/belt_waystation.gd` | 토비아스 합류 이벤트 |
| `scenes/maps/*.gd` (8파일) | NPC 아이들, 퀘스트 트리거, 번개 |
| `scripts/utils/map_effects.gd` | 동적 날씨, 번개, 파티클 풀/컬링 |
| `scripts/utils/tile_painter.gd` | auto_blend_edges() |
| `scripts/ui/dialogue_box.gd` | slide 애니메이션, 포트레이트 3종 |
| `scripts/ui/pause_menu.gd` | slide-in/out 애니메이션 |
| `scenes/ui/options_menu.gd` | 접근성 설정 3종 |
| `export_presets.cfg` | Windows Desktop 내보내기 템플릿 |

### 다음
- 전체 플레이 테스트
- 추가 CG 연결
- 사운드 추가

---

## S54 — 2026-04-12 (16대 업그레이드 — 스토리+전투+비주얼+시스템)

### 완료

**스토리/콘텐츠 (4종)**
- [x] **캐릭터 블립 SFX** — 언더테일 스타일 텍스트 출력 사운드, 캐릭터별 피치 (아렐1.0/엘리아1.3/세이블0.7 등)
- [x] **엔딩 갤러리** — PauseMenu에서 달성 엔딩 열람 (6종 CG+설명, 미달성 잠금), seen_endings 영구 저장
- [x] **NPC 스케줄** — 챕터별 NPC 위치/대사 변화 (말렛 Ch2→3→6, 토비아스 Ch3→4-6→7+)
- [x] **대화 연출 태그** — [shake]/[slow]/[fast]/[pause=N] 대화 텍스트 특수 효과

**전투/게임성 (4종)**
- [x] **스킬 트리 (번 패시브)** — 총 연소 횟수 기반 5종 패시브 해금 (5/10/20/30/50회)
- [x] **도감 스캔 강화** — 토비아스 분석 시 약점/저항 표시, Codex 영구 기록, Ash Sight 패시브 연동
- [x] **전투 환경 효과** — 10맵별 전투 보너스 (회피/명중/속성 데미지/상태이상/힐링)
- [x] **보스 러시 모드** — 엔딩 달성 후 해금, 연속 보스전 + 최고 기록 타이머

**비주얼/연출 (4종)**
- [x] **맵 전환 다양화** — 맵별 고유 전환 (글리치/낙엽/먼지/안개), 자동 감지
- [x] **전투 승리 화면** — VICTORY/BOSS DEFEATED 연출 + 보상 요약
- [x] **감정 포트레이트 전환** — 동일 캐릭터 크로스페이드, 다른 캐릭터 슬라이드
- [x] **대화 카메라 효과** — [zoom=N]/[pan=X,Y]/[reset] 태그로 카메라 제어

**시스템/편의 (4종)**
- [x] **튜토리얼 힌트** — 5종 상황별 첫 발생 가이드 팝업, 자동 디스미스
- [x] **자동 전투** — AUTO 버튼 토글, AI 행동 선택 (HP/상태/기억등급 기반)
- [x] **통계 화면** — 10종 플레이 통계 (시간/전투/연소/걸음 등), PauseMenu Stats 버튼
- [x] **다국어 기반** — UI 로컬라이제이션 프레임워크 (en/ko 27키), OptionsMenu 언어 전환

### 수정 파일 (25개 + 신규 1개)
| 파일 | 변경 내용 |
|------|----------|
| `scripts/ui/dialogue_box.gd` | 블립SFX, 포트레이트 전환, 대화 태그, 카메라 효과 |
| `scripts/core/game_manager.gd` | 엔딩 갤러리, NPC 스케줄, 보스러시, 통계, 로컬라이제이션 |
| `scripts/systems/battle_manager.gd` | 스킬트리 적용, 스캔, 환경효과, 자동전투, 통계 |
| `scenes/battle/battle_scene.gd` | 승리화면, 스캔UI, 환경표시, AUTO버튼, 로컬라이즈 |
| `scripts/systems/memory_manager.gd` | 번 패시브 5종, 수집 통계 |
| `scripts/core/scene_transition.gd` | 전환 스타일 4종 (글리치/낙엽/먼지/안개) |
| `scripts/ui/pause_menu.gd` | 엔딩갤러리, 통계, 로컬라이즈, 보스러시 |
| `scenes/ui/options_menu.gd` | 언어 전환 |
| `scripts/ui/codex.gd` | 스캔 데이터 표시 |
| `scripts/core/player.gd` | 걸음 통계 |
| `scripts/systems/save_manager.gd` | TutorialHints 세이브 |
| `scripts/ui/memory_shop.gd` | 튜토리얼 힌트 |
| `scenes/maps/*.gd` (10파일) | 스타일 전환, NPC 스케줄 |
| `scenes/main/main.gd` | 보스러시 버튼 |
| `scenes/ui/game_over.gd` | 스타일 전환 |
| `scripts/ui/tutorial_hints.gd` | **신규** — 튜토리얼 힌트 오토로드 |

### 다음
- 전체 플레이 테스트
- 추가 CG/사운드
- 스토리 확장

---

## S60 — 2026-04-24 (하이브리드 VN 모드 Phase 1 — 삽화 중심 스토리 전환)

### 목적
현재 탐색/전투 중심 RPG → **삽화(CG+포트레이트) 중심의 하이브리드 스토리 어드벤처**로 전환.
방식 1(풀 VN) + 2(하이브리드) + 3(장면집) 혼합: 기본은 VN 스타일 씬 시퀀스로 흐르고, 탐색(mini-exploration)·전투(클라이맥스)는 핵심 앵커에서만 삽입.

### 완료

**코어 시스템 (4종 신규)**
- [x] **SceneFlow** 오토로드 (`scripts/systems/scene_flow.gd`) — JSON 구동 VN 시퀀스 런너. CG/포트레이트/나레이션/선택지/액션(`goto_map`/`goto_battle`/`goto_scene`/`end`) 처리, 탐색·전투 후 VN 복귀 큐
- [x] **VNScene UI** (`scenes/ui/vn_scene.tscn` + `scripts/ui/vn_scene.gd`) — 풀스크린 CG 크로스페이드, 좌/우 포트레이트(말하는 쪽 강조), 대화박스(타이프라이터), 나레이션 모드, 선택지 패널, 레터박스, 시스템 로그 표시
- [x] **VNHost** 빈 씬 (`scenes/main/vn_host.tscn/.gd`) — 순수 VN 구간용 배경 컨테이너, SceneFlow 종료 시 resume 자동 처리
- [x] **시나리오 JSON 3종** (`data/vn_scenes/`)
  - `ch1_prologue.json` — 오프닝~아침까지 ~30 스텝, CG 8종 + 포트레이트 15종 사용, Grade 3 연소 후 BL-07/엘리아 허밍/기억 복기 선택
  - `ch1_after_forest.json` — 탐색 후 엘리아 대화 + Green Tree + 뷰로 타워 원경
  - `ch2_market_arrival.json` — 베르단 시장 진입 VN 인트로, 말렛 언급, goto_map으로 탐색 전환

**하이브리드 연결**
- [x] **타이틀 → VN 시작** (`scenes/main/main.gd`) — New Game 시 vn_host로 전환 후 `SceneFlow.play("ch1_prologue")`
- [x] **VN → 탐색 앵커** — `action: goto_map` + `resume_scene` 으로 VN 일시 중단 후 맵 이동, 복귀 큐에 다음 씬 기록
- [x] **탐색 → VN 복귀** (`scenes/maps/rim_forest.gd`) — resume_queue 존재 시 스토리 스킵, 자유 탐색 + 캠프 트리거에서 vn_host로 복귀 후 `SceneFlow.resume_if_queued()`
- [x] **VN → VN 체인** — `action: goto_scene` 으로 씬 간 연쇄 (ch1_after_forest → ch2_market_arrival)

**기존 자산 활용**
- CG: ch1_twisted_forest / arrel_combat4 / ch1_ash_walk / ch1_ash_rain2 / ch1_campfire / ch1_ash_forest / ch1_green_tree / bureau_tower3 / ch2_verdan_overlook / ch2_verdan5
- 포트레이트: elia_wind/concern/neutral/calm/sad/determined/hopeful, arrel_default2/cold/pensive/neutral/determined
- PORTRAIT_MAP은 DialogueBox 오토로드의 것을 공유

### 수정/신규 파일
| 파일 | 변경 내용 |
|------|----------|
| `scripts/systems/scene_flow.gd` | **신규** — VN 시퀀스 런너 오토로드 |
| `scripts/ui/vn_scene.gd` | **신규** — VN UI (CG/포트레이트/대화/선택지) |
| `scenes/ui/vn_scene.tscn` | **신규** — VN UI 씬 |
| `scenes/main/vn_host.gd/.tscn` | **신규** — VN 전용 배경 컨테이너 |
| `data/vn_scenes/ch1_prologue.json` | **신규** — Ch1 오프닝 VN 시나리오 |
| `data/vn_scenes/ch1_after_forest.json` | **신규** — Ch1 후반 VN 시나리오 |
| `data/vn_scenes/ch2_market_arrival.json` | **신규** — Ch2 진입 VN 시나리오 |
| `scenes/main/main.gd` | New Game → VN 프롤로그 재생 |
| `scenes/maps/rim_forest.gd` | VN 하이브리드 모드 — 스토리 스킵 + 캠프 트리거에서 VN 복귀 |
| `project.godot` | SceneFlow 오토로드 등록 |

### 하이브리드 플레이 흐름 (Ch1~Ch2 Phase 1)
```
타이틀 → [VN] ch1_prologue (보이드 비스트 처치 이후 CG 시퀀스 → 아침)
       → [탐색] rim_forest (보이드 사냥 미니 탐색, 전투 트리거)
       → [VN] ch1_after_forest (엘리아 대사 + Green Tree + 뷰로 타워)
       → [VN] ch2_market_arrival (베르단 오버룩 CG 인트로)
       → [탐색] verdan_market (말렛 거래 등 기존 흐름)
```

### 추가 삽화 권장 (차후 세션에서 요청)
현재 자산으로 Ch1~Ch2 VN 전환 완료. 품질 향상에 도움 될 것:
- **VN 스탠딩 전신 CG** — 현재 포트레이트는 흉상 위주. 전신 스탠딩은 VN 임팩트↑
- **챕터 전환 타이포그래피 카드** — "Chapter 1: Ash" 스타일의 전용 타이틀 CG
- **분위기 전용 배경 CG** — 각 챕터 시작·종료 순간의 "분위기 컷" (감정 여운용)
- **엘리아 감정 추가** (elia_exhausted, elia_anger, elia_tears 등)

### 다음 세션 (S61) 할 일
- [ ] Ch3~Ch10 VN 시나리오 JSON 작성 (기존 dialogue JSON을 VN steps로 변환)
- [ ] 세이브/로드에서 SceneFlow 상태 저장·복원 (current_id / current_index / resume_queue)
- [ ] VN 내 ESC 일시정지 + 대화 로그 + 스킵 기능
- [ ] PortraitMap에 스탠딩용 전신 이미지 별도 지원
- [ ] 전투 앵커 연결 (Ch3 보스 등 `goto_battle` 실제 동작)

### 테스트 포인트 (F5 실행)
1. 타이틀 → New Game → VN 프롤로그 자동 시작되는지
2. 클릭/Enter로 진행, 포트레이트 좌우 배치·말하는 쪽 강조 동작
3. CG 크로스페이드 자연스러운지
4. 캠프 밤 선택지 3종 표시·선택 후 다음 진행
5. 아침 장면 후 rim_forest 맵 진입 → 남쪽 끝 도착 → vn_host로 복귀 → ch1_after_forest 재생
6. ch1_after_forest 종료 후 ch2_market_arrival 자동 연결 → verdan_market 맵 진입

---

## S61 — 2026-04-24 (Memory Distortion — Katana ZERO 서사 트릭)

### 목적
MEMORIA 본질(기억을 태운다)을 **서사 레이어에서도 작동**시키기. 기억을 태우면 이후 그 기억과 연결된 대사·CG·포트레이트가 왜곡된 버전으로 재생. 플레이어의 선택이 게임플레이뿐 아니라 **텍스트 그 자체**를 변형시킴 (Katana ZERO 패턴).

### 완료

**1. SceneFlow 왜곡 로직 확장**
- 스텝 필드 추가: `distort_if_burned`(기억 ID) + `distorted_text` / `distorted_narrate` / `distorted_speaker` / `distorted_portrait` / `distorted_cg`
- 기억이 태워진 상태면 스텝 dict를 duplicate 후 필드 교체, `_distorted: true` 플래그로 VN UI에 신호

**2. VNScene 글리치 VFX**
- **기억 연소 순간** (MemoryManager.memory_burned 시그널) — 강한 VFX:
  - 붉은 플래시 (0.55 알파 → 페이드아웃 0.9s)
  - 색수차 분리 (CG의 R/B 채널 복사본을 좌우로 8px 오프셋 후 수렴)
  - SFX `memory_burn`
  - 텍스트 스크램블 (0.12s, `▓▒░█▄▀#@%&*?!` 로 치환 후 원래 텍스트 복원)
- **왜곡된 대사** — 약한 VFX:
  - CG 색수차 3px 약하게 1.2s 지속 후 페이드
  - 플레이어가 "뭔가 어긋났다"는 감각을 받게

**3. Ch1 프롤로그 왜곡 시퀀스 삽입**
- 재비 장면 중간에 **능동 연소 선택** 추가:
  - "Burn it. The song for passage." → `daily_campfire_song` 태움 + `burned_for_passage` 플래그
  - "Hold on to it. Find another way." → `refused_to_burn` 플래그
- 캠프 밤 선택지 이후 **엘리아 허밍 시퀀스** 추가 (3줄) — 각각 `distort_if_burned: "daily_campfire_song"`로 연소 시 다른 텍스트/포트레이트 재생:
  - 나레이션: "threadbare melody" → "He waited for the melody to mean something. It didn't."
  - 아렐: "...I know that song." → "...Is that a song? I can't tell." (포트레이트 cold로)
  - 엘리아: "Your mother used to hum it..." → "...You used to know it." (포트레이트 sad로)

### 수정 파일
| 파일 | 변경 |
|------|------|
| `scripts/systems/scene_flow.gd` | `_run_step()`에 왜곡 분기 추가 (15줄) |
| `scripts/ui/vn_scene.gd` | 글리치 레이어, `_on_memory_burned`, `_play_burn_glitch`, `_play_subtle_distortion`, `_scramble_text` (~80줄) |
| `data/vn_scenes/ch1_prologue.json` | 능동 연소 선택지 + 왜곡 대사 3줄 추가 |

### 플레이어 경험
- **태우지 않은 플레이:** 엘리아가 어머니 노래를 알아보고 기억한다고 말함. 따뜻함.
- **태운 플레이:** 엘리아가 허밍하지만 그게 뭔지 모름. 엘리아는 "너는 예전엔 알았었어"만 말함. 색수차로 화면이 미세하게 어긋남. 무게 있는 상실감.
- 이 한 장면만으로 "기억 태움 = 서사 변형"의 MEMORIA 정체성이 플레이어에게 전달됨.

### 다음 세션 (S62) 할 일
- [ ] 다른 VN 씬에도 distort_if_burned 필드 확장 (Ch2 말렛 거래, Ch3+ 엘리아 관계 등)
- [ ] 탐색 맵의 DialogueManager에도 같은 왜곡 로직 적용 (현재 VN 전용)
- [ ] 여러 기억이 동시에 태워졌을 때 왜곡 누적 (색수차 농도 증가)
- [ ] 글리치 사운드 `memory_burn` SFX 확인·생성

---

## S61b — 2026-04-24 (VN UI 입력 간섭 버그 수정)

### 문제
타이틀 → New Game → 프롤로그 끝 → rim_forest 맵 → Elia NPC 상호작용 시 화면이 멈춤. ch1_elia_talk.jpg 풀스크린 CG만 표시되고 DialogueBox가 Space/클릭에 반응 안 함.

### 원인
goto_map 액션 후 VN UI(CanvasLayer 50)가 `queue_free`되지만 한 프레임 동안 살아있으면서 `_input` 핸들러가 mouse/space 이벤트를 선점. 탐색 맵의 DialogueBox(같은 layer 50)의 `_unhandled_input`이 이벤트를 받지 못해 대사 진행 불가.

### 수정
- `vn_scene.gd._input()` — SceneFlow.is_active가 false면 즉시 early return (비활성 VN이 입력 가로채기 방지)
- `scene_flow.gd._close_vn_ui()` — queue_free 전에 `visible=false` + `set_process_input(false)` + `set_process_unhandled_input(false)` 호출해 잔여 프레임 입력 완전 차단

---

## S62 — 2026-04-24 (Memory Constellation — 기억 성좌 UI)

### 목적
기억 보관 UI를 **정적 리스트 → 동적 네트워크**로 업그레이드. 기억들이 서로 연결된 별자리처럼 보이고, 하나를 태우면 연결된 기억들에 금이 감. 플레이어가 "이 기억을 잃으면 저 기억도 왜곡된다"는 무게를 시각적으로 인지.

### 완료
- **Memory 클래스 확장** (`memory_manager.gd`) — `connections: Array` 필드 + `_refresh_connections()` 자동 계산
  - 규칙 1: 같은 `related_npc` 끼리 모두 연결 (NPC 단위 서브그래프)
  - 규칙 2: 같은 id prefix(sense/daily/rel/identity/core)의 인접 기억 연결
  - 헬퍼: `find_memory(id)`, `burned_neighbor_count(id)`
- **MemoryConstellation 오토로드** (`scripts/ui/memory_constellation.gd`) — CanvasLayer 42
  - **동심원 배치**: GRADE_5(감각)가 최외곽 → GRADE_1(핵심)이 중심
  - **노드 렌더링**: 등급별 색, 맥동 애니메이션, 호버 시 확대+밝아짐
  - **연결선**: 공통 NPC 있으면 NPC 고유색, 없으면 옅은 회색. 둘 중 하나라도 태워지면 **점선+붉은 톤**으로 "끊어진" 시각화
  - **태워진 기억**: X 마크 + 어두운 링 (완전 소실) / 흐릿한 노드 + `~` (잔존)
  - **금 효과**: 이웃 태워진 수에 비례해 노드에 붉은 균열 선 1~3개
  - **툴팁**: 호버 시 제목/등급/상태/관련 NPC/설명/연소 시 효과 (RichText BBCode)
  - **범례**: 하단에 링·선·금·X 의미 설명
- **MemoryUI 토글 버튼** — 하단 바에 "✦ Constellation" 버튼 추가, 클릭 시 Constellation 오픈 (MemoryUI 자동 숨김→복귀)
- **오토로드 등록** — project.godot에 MemoryConstellation 추가

### 플레이어 경험
- Tab/M으로 Archive 열고 "Constellation" 클릭 → 전체 기억 네트워크 조망
- 한 기억을 태우면 다음에 성좌 열었을 때: 그 기억은 X 처리되고, **연결된 기억들 주변에 균열 선이 자동 생성**됨
- 엘리아 관련 기억 클러스터가 초록선으로 묶여 있는 걸 보면 "이 관계를 파괴하지 않으려면 이 쪽은 태우지 말아야" 판단 가능

---

## S63 — 2026-04-24 (Memory Leverage — 대화 중 기억을 연료로)

### 목적
기억 연소가 전투 스킬에만 묶여 있던 걸 **대화/협상/설득에도 사용**하는 자원으로 확장. 선택지가 "텍스트"뿐 아니라 "이 선택을 위해 이 기억을 태운다"는 거래가 되게.

### 완료
- **VNScene 선택지 업그레이드** (`vn_scene.gd._show_choices`)
  - `cost_memory: "memory_id"` 필드 인식 → 버튼 텍스트에 `✦ [선택]\n    [ Burn: 기억이름 ]` 형태로 표시
  - **시각적 구분**: cost_memory 선택지는 붉은 테두리 + 어두운 바탕 (일반 선택지는 금색). 호버 시 더 강렬한 붉은 톤.
  - **자동 비활성화**: 태울 기억이 이미 태워졌거나 존재하지 않으면 선택지 자체 제외
  - `requires_memory_intact`와 조합 가능 (예: 기억이 살아있어야 선택 가능)
- **SceneFlow 처리** (`scene_flow.gd.select_choice`) — `cost_memory` 필드는 `burn_memory`의 의미적 별칭으로 동일 연소 처리
- **샘플 선택지 삽입**
  - `ch1_prologue.json`: 재비 속 "The song for passage." → cost_memory: daily_campfire_song
  - `ch2_market_arrival.json`: 뷰로 가드 앞에서 3선택지 — 거짓말 / 뇌물로 기억(daily_market_food) 태우기 / 검술 기억 있으면 돌파

### 플레이어 경험
- 선택지 창을 열었을 때 "그냥 선택" 과 "기억을 대가로 얻는 선택"이 시각적으로 명확히 구분
- 붉은 선택지는 매번 "이 기억을 정말 태울 것인가" 질문하게 만듦 (아이템을 쓰는 게 아니라 **자신의 일부를 태우는 거래**)
- Constellation UI와 연동 — 대화에서 기억을 태우면 성좌에서 즉시 X 표시 + 연결된 기억들 균열

---

## S64 — 2026-04-24 (Perception Drift — 세계가 기억에 따라 달라진다)

### 목적
기억 태움이 "내면"뿐 아니라 "외부 세계"에도 영향을 주게. 특정 기억을 태우면 NPC·오브젝트가 다르게 보이거나 사라지거나 나타남. 물리적 세계가 플레이어의 기억 상태에 따라 재구성됨.

### 완료
- **PerceptionFilter 유틸** (`scripts/systems/perception_filter.gd`) — `class_name PerceptionFilter`
  - **정적 메서드** `PerceptionFilter.apply(scene)` — 맵 _ready 말미에 호출
  - **메타 기반 필터**: 노드에 `set_meta("requires_memory_intact", "id")` 또는 `"requires_memory_burned"` 설정 → 자동 visible/collision 제어
  - **그룹 기반 필터**: `perception_intact_<id>` / `perception_burned_<id>` 그룹에 속한 노드들 일괄 처리
  - **NPC 대화 교체**: `burned_dialogue_<memory_id>` 메타로 기억 태움 시 dialogue_key 교체
  - **틴트 효과**: `on_burned_tint_memory` + `on_burned_tint` 메타로 modulate 자동 적용
  - 숨긴 CollisionObject2D는 layer/mask 0으로 리셋해 통과 가능
- **rim_forest 시범 적용** (`_setup_perception_nodes`)
  - **Song Echo**: `daily_campfire_song`을 태운 플레이어에게만 보이는 따뜻한 빛 + 부유 파티클 (캠프 근처). 다가가면 "A faint warmth. A song you no longer know." 토스트
  - **엘리아 창백 틴트**: 노래 태움 시 엘리아 스프라이트 modulate가 차가운 색(0.75, 0.8, 0.85)으로 자동 변경
  - PerceptionFilter.apply(self)를 rim_forest _ready 끝에서 호출

### 플레이어 경험
- 노래를 태우고 맵을 다시 걸으면: 전에 없던 **따뜻한 잔향 불빛**이 캠프 주변에 피어남. 그 빛은 "이 기억을 가진 다른 버전의 당신"의 흔적
- 엘리아가 **살짝 창백하게** 보임. 게임이 직접 "엘리아가 변했다"고 말하지 않지만, 플레이어는 느낄 수 있음
- 다른 맵·다른 챕터에도 `set_meta` 한 줄로 조건부 오브젝트 추가 가능 (확장 비용 낮음)

---

### 수정/신규 파일 (S62/S63/S64 합산)
| 파일 | 변경 |
|------|------|
| `scripts/systems/memory_manager.gd` | connections 필드, _refresh_connections, find_memory, burned_neighbor_count |
| `scripts/ui/memory_constellation.gd` | **신규** — Constellation UI 오토로드 |
| `scripts/ui/memory_ui.gd` | 하단 바에 Constellation 토글 버튼 |
| `scripts/ui/vn_scene.gd` | cost_memory 선택지 UI (붉은 테두리, 라벨 표시, 자동 비활성화) |
| `scripts/systems/scene_flow.gd` | select_choice에 cost_memory 연소 처리 |
| `scripts/systems/perception_filter.gd` | **신규** — 메타/그룹 기반 기억 상태 필터 유틸 |
| `data/vn_scenes/ch1_prologue.json` | 재비 속 선택을 cost_memory 형식으로 |
| `data/vn_scenes/ch2_market_arrival.json` | 뷰로 가드 3선택지 (leverage 예시) |
| `scenes/maps/rim_forest.gd` | Song Echo 파티클/라이트 + 엘리아 틴트 + PerceptionFilter.apply 호출 |
| `project.godot` | MemoryConstellation 오토로드 등록 |

### 다음 세션 (S65) 할 일
- [ ] Constellation 클릭 시 기억 상세 창(설명·연소 효과) 모달 팝업
- [ ] 다른 맵(verdan_market 등)에도 PerceptionFilter 적용 + 2~3개 조건부 NPC/오브젝트
- [ ] DialogueManager(탐색 맵 대화) 선택지에도 cost_memory UI 동기화
- [ ] Constellation에 "연결 흐름" 애니메이션 (선이 흘러가는 느낌)
- [ ] 핵심 기억(Grade 1) 태울 때 전체 성좌가 재편되는 컷씬

---

## S65 — 2026-04-24 (A안 피벗 시작 — The Cut)

### 결정
Steam 흥행을 위해 **Story-VN with Mechanics** (A안) 방향으로 피벗. LISA·OneShot·OMORI처럼 **하나의 본질에 집중**. 자산 비율(CG 130/포트레이트 49/대화 1400줄)은 RPG보다 VN에 가깝고, 솔로 개발 효율 + 차별점(메모리 메카닉 VN) 모두 A안이 유리.

### 이번 세션: The Cut (범위 축소, 코드 보존)

**1. 타이틀 화면 정리** (`scenes/main/main.gd`)
- NG+ 버튼 노출 제거 (조건부 출력 코드 삭제, 콜백은 보존)
- Boss Rush 버튼 노출 제거 (동일)
- 서브타이틀 변경: `"The Price of Oblivion"` → `"The Price of Oblivion  ·  A story of what you choose to forget"`
  - VN 카피 한 줄로 게임 본질 전달, 스토어 페이지 hero copy로도 사용 가능

**2. PauseMenu 슬림화** (`scripts/ui/pause_menu.gd`)
- **숨김**: Fast Travel (RPG 워프), Stats (통계 화면), Load Autosave (Load와 중복)
- **유지**: Resume, Journal, Codex, Achievements (Steam 기대치), Endings, Options, Save, Load, Title, Quit
- 코드는 모두 보존 — UI 진입점만 차단

### 챕터 4압축 설계 (S66+에서 구현)

기존 10챕터(Rim → Belt → Drift → Coast → Seam → Forest → Waste → Seal → Epilogue)를 **4막 구조**로 재편:

| 신규 | 기존 매핑 | 핵심 비트 | 길이 목표 |
|------|----------|---------|----------|
| **Act I — Ash** (Rim Forest) | Ch1 그대로 | 첫 연소·재비·엘리아·캠프 / **첫 보스: Void Beast** | 30분 |
| **Act II — Bargain** (Verdan Market) | Ch2 + Ch3 토비아스 압축 | 말렛 거래·뷰로 가드·세계관 노출 / **보스 없음** (대화 압박 클라이맥스) | 45분 |
| **Act III — Echo** (Seam Outskirts → Forest 압축) | Ch3·Ch4·Ch5·Ch7·Ch8 핵심 장면 | 세이블 진실·기억 기생 숲·환각 / **두 번째 보스: Memory Wraith** | 60분 |
| **Act IV — Origin** (BL-07 Void) | Ch9·Ch10 압축 + 6엔딩 | 카이로스 대면·Seal 결정 / **마지막 보스: Kairos** + 엔딩 분기 | 45분 |

**총 플레이타임: 3시간** (현 10챕터 8시간 → 압축. Steam 짧고 강한 VN 트렌드).
**보스 3전만 유지**: Void Beast / Memory Wraith / Kairos. 나머지 잡몹·랜덤 인카운터 비활성화.

### 삭제 후보 시스템 목록 (S66~S68에서 단계적 비활성화)

코드는 보존, UI/접근만 차단:
- 랜덤 인카운터 (`RandomEncounter`)
- 사이드 퀘스트 (`SideQuest`, 6종)
- 장비 시스템 + 강화
- 크래프팅 (기억 합성은 유지 — 본질 메카닉)
- 콤보 시스템 / Limit Break
- 자동 전투
- 보스 러시
- NG+
- 통계 화면
- 미니맵 (선형 진행이라 불필요)
- 파티 시스템 (세이블 동행 → VN 동행자로만, 전투 동참 없음)

### 유지·강화 시스템

VN 본질에 직결되는 것만 살림:
- 기억 연소 + Constellation (S62) + Leverage (S63) + Perception Drift (S64)
- Memory Distortion 왜곡 (S61)
- VN 씬 흐름 (SceneFlow)
- Codex - Memory Archive
- 6 엔딩 분기
- 다국어 (en/ko)
- 업적 (28종 → 스토리 중심으로 재선별)

### 수정 파일
| 파일 | 변경 |
|------|------|
| `scenes/main/main.gd` | NG+/Boss Rush 버튼 노출 코드 제거, 서브타이틀 변경 |
| `scripts/ui/pause_menu.gd` | Fast Travel/Stats/Load Autosave 메뉴 숨김 |

### 다음 세션 (S66) 할 일 — 옵션 2 채택
- Ch1 (Act I — Ash) 데모 빌드 완성

---

## S66 — 2026-04-24 (Act I — Ash 데모 빌드)

### 목표
첫 30분을 흠 없이 갈고 닦은 **Steam Next Fest 출시 가능한 데모**. rim_forest를 단일 맵으로 정리하고, 보스(Void Beast) 1전 + 핵심 대화 3개 + 캠프 + VN 후일담만 남김.

### 완료

**1. rim_forest 부수 시스템 비활성화** (코드 보존, 호출만 차단)
- `_setup_random_encounters()` 호출 제거 — 잡몹 인카운터 없음
- `_setup_side_quests()` 호출 제거 — 사이드 분기 없음
- `Minimap.update_minimap()` 비활성 — 선형 VN 진행 강조
- `RandomEncounter.update()` 비활성

**2. 핵심 트리거만 남기고 잡요소 제거**
- 히든 이벤트: 6개 → **3개 (그루터기 / 기억 사당 / 엘리아 기억 대화)**
  - 제거: dead_burner(잡 분위기), forest_walk(중복), anchor_talk(중복), MemoryResonance(미니게임)
- 전투 트리거: 2개 → **1개 (Void Beast 보스만)**
  - 제거: Ash Crawler 잡몹 — Act I는 클라이맥스 한 번만

**3. Void Beast 필수화**
- 캠프 트리거에 `ch1_void_beast_defeated` 플래그 체크 추가
- 미처치 시 토스트: *"Something blocks the path. Find what hunts these woods."*
- BattleManager.battle_ended 시그널 연결 — VICTORY 시 자동 플래그 설정

**4. 데모 종료 화면** (`scripts/ui/demo_end.gd` + `scenes/ui/demo_end.tscn`)
- ch1_after_forest VN 마지막 action을 `goto_scene: ch2_market_arrival` → **`demo_end`** 로 변경
- SceneFlow에 `demo_end` 액션 처리 추가 — `res://scenes/ui/demo_end.tscn` 로드
- 화면 구성:
  - 배경: Cover2.png + 어두운 비네트
  - 타이틀: **"Act I — Ash"** (큰 황금색)
  - 부제: **"— End of Demo —"**
  - 본문: 감사 메시지 + 풀버전 티저 (벨트, 시임, 형제, 결정)
  - 통계: *"You burned X memories. Y remain as residue."* (플레이어의 기억 선택 기록)
  - CTA 3버튼: **✦ Wishlist on Steam** (외부 링크) / **Return to Title** / **Quit**
- 순차 페이드인 애니메이션 (각 요소 0.45s 간격)

### Act I 플레이 흐름 (검증)
1. 타이틀 → New Game
2. VN 프롤로그 (ch1_prologue): 첫 연소 묘사 → 엘리아 등장 → 재비 → **연소 선택지 (cost_memory: daily_campfire_song)** → 캠프 밤 → 글리치 VFX (선택 시)
3. rim_forest: 자유 탐색 (3 히든 이벤트 + 엘리아 동행 + Memory UI/Constellation 접근)
4. **Void Beast 보스전** (필수)
5. 캠프 트리거 (남쪽) — VN으로 복귀
6. ch1_after_forest VN: 그린 트리 + 뷰로 타워 시야
7. **Demo End 화면** — 위시리스트 CTA

### 수정 파일
| 파일 | 변경 |
|------|------|
| `scenes/maps/rim_forest.gd` | 인카운터/사이드퀘스트/미니맵 호출 제거, 히든 이벤트 3개로 축소, 잡몹 전투 제거, 보스 필수 게이트 추가, battle_ended 시그널 연결 |
| `data/vn_scenes/ch1_after_forest.json` | 마지막 action을 `goto_scene: ch2_market_arrival` → `demo_end`로 |
| `scripts/systems/scene_flow.gd` | `demo_end` 액션 핸들러 추가 |
| `scripts/ui/demo_end.gd` | **신규** — 데모 종료 화면 |
| `scenes/ui/demo_end.tscn` | **신규** — 데모 종료 씬 |

### Steam Next Fest 빌드 체크리스트 (S67에서 마무리)
- [ ] Steam URL 실제 앱 ID로 교체 (`STEAM_URL` 상수)
- [ ] 데모 종료 화면 BGM 트랙 결정
- [ ] Ch1 30분 풀 플레이 검증 (실시간 측정)
- [ ] 한국어 로케일 점검 (대화·UI)
- [ ] 시작 옵션에서 Steam achievement 등록 확인 (Codex/Achievement 시스템)

### 다음 세션 (S67) 할 일
- [x] Windows export 빌드 시도 (export_presets 정리 + 헤드리스 export 검증)
- [x] Steam 상점 페이지 카피 / 태그 / 트레일러 콘티 / 스크린샷 세트 (STEAM_PAGE.md)

---

## S67 — 2026-04-24 (Windows 빌드 + Steam 상점 키트)

### 목적
S66에서 만든 Act I 데모를 실제 zip으로 배포 가능한 빌드로 굳히고, Steam 상점 페이지에 바로 붙일 마케팅 자료(카피/태그/트레일러 콘티/스크린샷 가이드)를 정리.

### 완료

**1. export_presets.cfg 완전 재작성**
- Windows Desktop (Demo) 프리셋 정의
- application 메타: 회사명 `MEMORIA Studio`, 제품명 `MEMORIA - The Price of Oblivion (Demo)`, 파일/제품 버전 0.9.0.0, 저작권 © 2026
- export_path: `build/MEMORIA-Demo-v0.1.exe`
- exclude_filter: `SESSION_LOG.md, CLAUDE.md, *.tmp, .git/*, .gitignore`
- x86_64 아키텍처, embed_pck=false (별도 .pck 파일)

**2. 헤드리스 export 시도 + 진단**
- 명령: `Godot_v4.6.2-stable_win64_console.exe --headless --export-release "Windows Desktop (Demo)" "build/MEMORIA-Demo-v0.1.exe"`
- **결과: 템플릿 미설치로 실패** — 사용자가 Godot Editor에서 직접 설치해야 함
  - 경로: `C:/Users/jc/AppData/Roaming/Godot/export_templates/4.6.2.stable/` 가 비어있음
  - **해결법: Godot Editor → Editor 메뉴 → Manage Export Templates → Download (~600MB)**
- VFX Library 플러그인 종료 시 autoload/VFX, autoload/EnvVFX 미존재 경고 — 비치명적, 게임 실행에는 무관

**3. Steam 상점 페이지 키트** (`STEAM_PAGE.md` 신규)
- **게임 이름**: MEMORIA: The Price of Oblivion
- **태그라인** (한/영): 기억을 태워 싸우는 다크 판타지 2D 어드벤처 — 잊는 만큼 세계가 바뀐다
- **짧은 설명** (한 217자 / 영 293자): Steam 검색 결과 노출용
- **About this game** (한/영 풀텍스트): 5개 핵심 메카닉 강조 — 기억 연소·대화 거래·세계 재작성·Constellation·6엔딩
- **Steam 태그 15개 우선순위** — Story Rich / Choices Matter / Dark Fantasy / RPG / 2D 핵심 5
- **30초 트레일러 콘티** (8컷, 시간 매핑·자막·캡처 소스): VN 씬→선택지→글리치→성좌→Perception→보스전→타이틀
- **스크린샷 6장 가이드**: 메인 1 + 보조 5 (각각 의도된 메시지 명시)
- **캡슐 이미지 6종 사양**: Main/Small/Header/Library Capsule + Library Hero + Logo
- **출시 전략 노트**: Wishlist 빌드업, 가격대($9.99~14.99), 출시 윈도우 회피, itch.io 동시 배포

### 수정/신규 파일
| 파일 | 변경 |
|------|------|
| `export_presets.cfg` | Windows Demo 프리셋 완전 정의 |
| `STEAM_PAGE.md` | **신규** — Steam 상점 페이지 자료 일체 |

### 사용자 액션 아이템 (수동 작업 필요)
1. **Export Templates 설치** — Godot Editor → Editor → Manage Export Templates → Download
2. (설치 후) 헤드리스 빌드 재시도 또는 Editor → Project → Export → "Export Project" 클릭
3. **Steam 앱 ID 발급** (Steamworks 가입 후) → `demo_end.gd`의 `STEAM_URL` 상수 교체
4. **캡슐 이미지 디자인** — Photoshop/Affinity 등으로 Cover2.png 베이스로 6종 제작
5. **트레일러 캡처** — 빌드 성공 후 OBS로 STEAM_PAGE.md의 8컷 따라 녹화

### 다음 세션 (S68) 후보
- [x] (사용자 빌드 성공 후) 실제 zip 패키징 워크플로우 정리 — S68에서 인프라 미리 준비
- [ ] 한/영 자막 검수 — Ch1 VN 씬 + 탐색 대화 전체 톤 정리
- [ ] 데모 BGM/SFX 누락 점검 (특히 글리치 사운드 `memory_burn` 파일 존재 여부)
- [ ] verdan_market에 PerceptionFilter 적용 — 풀버전 Act II 준비

---

## S68 — 2026-04-24 (빌드 검증 + 테스터 패키지 준비)

### 목적
S67에서 export 인프라를 깔았지만 사용자가 templates를 직접 설치해야 빌드가 굴러감. 그 사이 제가 할 수 있는 것: **빌드 전 위생 검사**, **단계별 가이드**, **테스터 피드백 양식**, **자동 패키징 스크립트** 준비. 빌드 성공 직후 5분 안에 친구한테 zip 보낼 수 있게.

### 완료

**1. 빌드 전 위생 검사 (CLI)**
- Godot 헤드리스 `--check-only --quit` 실행 → **GDScript 파싱 에러 0건, 미정의 참조 0건**. "data.tree is null" 경고는 헤드리스 모드 정상 잡음 (씬 트리 없는 상태에서 스크립트가 트리 접근 시도)
- VN JSON에서 참조하는 **CG 14개 / 포트레이트 13개 모두 존재** 확인
- 새로 짠 5개 스크립트(vn_scene/scene_flow/demo_end/memory_constellation/perception_filter) export 시 빌드 깨뜨릴 위험 없음

**2. BUILD_GUIDE.md** — 5단계 빌드 가이드
- STEP 1: Export Templates 설치 (Editor → Manage Export Templates → Download, ~600MB)
- STEP 2: 빌드 실행 (Editor GUI 또는 CLI)
- STEP 3: 빌드 결과 확인 (.exe + .pck + .console.exe)
- STEP 4: zip 패키징 (`./package_demo.sh`)
- STEP 5: 친구한테 보내기 (WeTransfer / Google Drive / itch.io 비공개 추천)
- 트러블슈팅 4건 (흰 화면, 템플릿 없음, 한국어 깨짐, Defender 차단) + 코드 서명 미적용 사실 명시

**3. TESTER_GUIDE.md** — 30분 플레이 후 답하는 피드백 양식
- 5섹션 구조: 첫인상 / 스토리 / **핵심 메카닉 (가장 비중)** / 조작감·버그 / 종합
- 핵심 질문:
  - 재 장면 cost_memory 선택지 — 뭘 골랐고 *왜* 골랐는지
  - 선택 *이후* 변화를 어디서 느꼈는지 (Perception Drift 동작 검증)
  - Constellation 한 번이라도 눌러봤는지 (UI 발견 가능성 검증)
  - Void Beast 보스전 난이도 + 의도 명확성
- 종합: 다음 챕터 유료 구매 의향 / 친구 추천 / 흥행 점수 10점 만점
- 조작 키 매핑 표 + 시작/종료 지점 명시 (사용자 혼란 방지)

**4. package_demo.sh** — 빌드 → zip 자동화 (bash, Git Bash 호환)
- 9단계 파이프라인:
  1. 빌드 산출물 존재 검증 (없으면 친절한 에러 + BUILD_GUIDE.md 참조)
  2. 스테이징 디렉터리 생성 (`build/stage/`)
  3. exe + pck 복사
  4. console.exe는 `build/debug/`로 별도 보관 (zip 부피 감소)
  5. README.txt 자동 생성 (실행법 + 조작 + 피드백 안내)
  6. TESTER_GUIDE.md 복사
  7. zip 생성 (zip 또는 7z 자동 감지)
  8. 결과 출력 (파일 경로 + 사이즈 + 다음 단계 가이드)
  9. 스테이징 폴더 정리
- 최종 산출물: `build/MEMORIA-Demo-v0.1-Windows.zip` (예상 150~300MB)

### 신규 파일
| 파일 | 용도 |
|------|------|
| `BUILD_GUIDE.md` | 사용자가 빌드부터 zip까지 따라 할 단계별 가이드 |
| `TESTER_GUIDE.md` | 친구한테 zip과 함께 보낼 피드백 양식 |
| `package_demo.sh` | 빌드 산출물을 친구 발송 가능 zip으로 자동 패키징 |

### 사용자 액션 아이템 (S68 마무리용)
1. **Godot Editor 열기 → Editor → Manage Export Templates → Download** (한 번만, ~600MB)
2. **Project → Export → "Windows Desktop (Demo)" → Export Project**
3. 본인 PC에서 `build/MEMORIA-Demo-v0.1.exe` 더블클릭 → 5분 동작 확인
4. Git Bash 또는 WSL에서 `./package_demo.sh` 실행
5. 생성된 zip을 WeTransfer/Drive/itch.io에 업로드
6. 친구 1~3명에게 링크 + TESTER_GUIDE 안내 전송

### 다음 세션 (S69) 후보
- [x] (S69에서) VN 시각 폴리싱 — Ken Burns / 선택지 덤 / 연소 잔열 / 필름 그레인
- [ ] **테스터 피드백 1차 수집 후 분석 + 우선순위 버그/UX 패치**
- [ ] 비주얼 스타일 결정 (AI 일러스트 단일화 vs 픽셀 유지)
- [ ] 챕터별 시그니처 BGM 1트랙 (Suno/Udio)
- [ ] 한/영 자막 검수 (Ch1 VN 전체 톤 정리)
- [ ] Steamworks 가입 + 앱 ID 발급 (사용자 액션, 1주~2주)

---

## S69 — 2026-04-24 (VN 시각 폴리싱 — 작은 4개로 큰 차이)

### 목적
빌드 인프라는 끝났고 테스터 회수 전 마지막으로 시각적 매력을 끌어올리기. 코드 변경 최소·임팩트 최대를 노린 4종.

### 완료

**1. CG Ken Burns** (`vn_scene.gd._start_ken_burns`)
- CG가 표시될 때마다 9~13초에 걸쳐 1.0 → 1.05 줌 + ±18px 팬 (sine ease in/out)
- 정적 일러스트가 살아 움직이는 느낌. 무의식적 시네마틱 효과.
- 매 CG마다 랜덤 팬 방향 → 같은 컷 봐도 매번 다르게 느낌

**2. 선택지 등장 시 배경 덤** (`vn_scene.gd._dim_background_for_choice`)
- `_show_choices` 호출 시 CG modulate를 0.6/0.6/0.65, 포트레이트는 0.5/0.5/0.55로 0.45s 페이드
- `_on_choice_selected` 시 1.0/1.0/1.0/1.0으로 복귀
- 효과: 결정의 무게가 시각적으로 강조 — 선택지가 덮인 공간이 화면 중심이 됨
- cost_memory 붉은 테두리와 합쳐지면 더 무겁게 읽힘

**3. 기억 연소 잔열 비네트** (`vn_scene.gd._build_glitch_layer` 확장)
- `_ember_vignette` TextureRect — GradientTexture2D radial fill (가장자리만 따뜻한 오렌지/붉은색)
- `_play_burn_glitch`에 트윈 추가:
  - 0~0.5s: 알파 0 → 0.85 (잔열 차오름)
  - 0.5~1.7s: 1.2s 유지 (인지 가능한 시간)
  - 1.7~5.2s: 알파 0.85 → 0 (3.5s 천천히 식음)
- 글리치 직후 화면 가장자리만 타고 난 듯한 연한 빛 → 플레이어가 "방금 뭔가 잃었다"는 정서적 잔여를 시각으로 받음

**4. 필름 그레인 셰이더** (`vn_scene.gd._build_glitch_layer` 확장)
- 풀스크린 ColorRect + 인라인 GLSL 셰이더 (`hash21` 노이즈, 시간으로 패턴 갱신)
- 강도 0.045 (매우 미묘 — 의식적으로 눈에 띄진 않지만 무의식엔 영향)
- 16 FPS로 노이즈 패턴 갱신 (영화 필름 그레인 느낌)
- `u_time` 셰이더 파라미터를 `_process`에서 매 프레임 업데이트
- 효과: AI 일러스트의 평면적 매끈함이 사라지고 "프레임"이라는 감각 + 시간성

### 수정 파일
| 파일 | 변경 줄 (대략) |
|------|------|
| `scripts/ui/vn_scene.gd` | +90줄 (Ken Burns / dim / ember / grain shader / process 시간) |

### 검증
- Godot 헤드리스 `--check-only` 통과 (GDScript 파싱 + 셰이더 컴파일 에러 0건)

### 누적 효과 (테스터가 무의식적으로 느낄 것)
- VN이 "정지된 일러스트북"에서 **"움직이는 영화"**로 격상
- 선택지가 단순 UI가 아니라 **분기점**으로 인지됨
- 기억 연소가 **"정보"**가 아니라 **"감각적 손실"**로 바뀜
- 전체 톤이 AI 생성 매끈함에서 **시네마틱 그레이드**로 이동

### 다음 세션 (S70) 후보 — S69까지로 데모 폴리싱 마무리, 테스터 회수 대기
- [ ] (테스터 1~3명 회수 후) 피드백 우선순위 분석
- [ ] 비주얼 스타일 결정 (현재 AI 포트레이트 + 필름 그레인 + Ken Burns 조합 평가 후 통일 여부)
- [ ] 챕터별 시그니처 BGM 1트랙 (Suno/Udio, ~1주)
- [ ] Steamworks 가입 (사용자 액션)

---

## S70 — 2026-04-24 (Full VN 전환 — 환세취호전 미학 탈출)

### 결정 배경
사용자 정직한 자기 진단: *"환세취호전 같은 느낌이야"*. 코드로 그린 타일 맵·사각형 캐릭터가 AI 일러스트와 미학적 충돌. 약점을 폴리싱하는 대신 **삭제**하기로. RPG → Visual Novel 장르 본격 전환.

### 완료

**1. 탐색 맵 진입 차단 — 흐름 재배선**
- `ch1_prologue.json` 마지막 action: `goto_map: rim_forest` → **`goto_scene: ch1_forest_walk`**
- 새 챕터 1 흐름:
  ```
  타이틀 → ch1_prologue (재비, 캠프 밤)
       ↓
  ch1_forest_walk (숲 수색 — 메뉴 형식 VN)
       ↓
  ch1_void_beast (보스전 — VN 스타일)
       ↓
  ch1_after_forest (그린 트리, 뷰로 타워)
       ↓
  demo_end
  ```
- rim_forest.tscn 자체는 코드 보존 — 호출만 끊김

**2. ch1_forest_walk.json — 메뉴식 VN 수색**
- 클리어링 도착 → 4개 선택지로 어디 갈지 결정:
  - **The carved stump** (A.E. 각인, 정체성 비트)
  - **The forest shrine** (버너의 무덤, 세계관 노출)
  - **Talk with Elia** (관계 비트, distort_if_burned 포함)
  - **Push deeper** (요구: 최소 1곳 방문 후 활성화)
- 각 방문 후 자동으로 메뉴 복귀 (`goto_scene: ch1_forest_walk, start_index: 5`)
- `requires_not_flag`로 이미 본 곳은 메뉴에서 제거
- 약 3~5분 소요. 플레이어가 *읽고 싶은 만큼* 읽고 진행

**3. ch1_void_beast.json — VN 스타일 보스전**
- 턴제 BattleManager 미사용. 순수 VN 메카닉.
- **2 라운드 + 종결** 구조:
  - Round 1: 3선택지 (검 / 기억 태움 [sense_warm_light] / 패턴 읽기)
  - Round 2: 3선택지 (속도 / 기억 태움 [sense_forest_smell] / 읽은 패턴 활용 — Round 1에서 vb_read 플래그 셋했어야 노출)
  - Endgame: 마무리 + 엘리아의 무게감 있는 한 줄 ("...You burned for it.")
- 기억 태움이 **단순 능력**이 아니라 *서사 무게*로 표현 — "그 선택의 대가가 어떻게 드러나는가"가 텍스트로 명시
- 종료 시 `ch1_void_beast_defeated` 플래그 자동 셋 → ch1_after_forest 진입

**4. SceneFlow 확장: 선택지 조건 게이팅** (`vn_scene.gd._show_choices`)
- `requires_flag: "flag_id"` — 해당 플래그가 set된 경우만 노출
- `requires_not_flag: "flag_id"` — 해당 플래그가 *미*set인 경우만 노출
- 기존 `requires_memory_intact` / `cost_memory`와 결합 가능
- 메뉴식 VN 진행에 필수 (이미 방문한 곳 자동 제거 등)

**5. 장르 표시 변경** (`STEAM_PAGE.md`)
- 우선순위 태그 1~3 변경:
  - 이전: Story Rich / Choices Matter / Dark Fantasy / **RPG** / 2D
  - 현재: **Visual Novel** / Story Rich / Choices Matter / Dark Fantasy / Psychological
- Steam 알고리즘이 VN 카테고리로 분류하도록

### 검증
- Godot 헤드리스 `--check-only` 통과
- JSON 5개 파일 (prologue/forest_walk/void_beast/after_forest/market_arrival) 전부 유효

### 수정/신규 파일
| 파일 | 변경 |
|------|------|
| `data/vn_scenes/ch1_forest_walk.json` | **신규** — 메뉴식 숲 수색 (49 steps) |
| `data/vn_scenes/ch1_void_beast.json` | **신규** — VN 보스전 (51 steps) |
| `data/vn_scenes/ch1_prologue.json` | 마지막 action을 ch1_forest_walk로 |
| `scripts/ui/vn_scene.gd` | requires_flag / requires_not_flag 선택지 게이팅 |
| `STEAM_PAGE.md` | 태그 우선순위 — Visual Novel 최상단 |

### 사라진 것
- 탐색 맵 진입 (rim_forest 코드는 살아있지만 흐름에서 끊김)
- 턴제 BattleManager 호출 (Ch1 한정)
- ExplorationHUD / Minimap 노출 (EXPLORATION 상태 안 들어가니 자동 미표시)
- 코드 픽셀 스프라이트 노출
- 자동 생성 타일 맵 노출

### 남은 강점만 살아남음
- AI CG 130장 + 포트레이트 49장
- 기억 연소 메카닉 (Constellation / Leverage / Distortion / Perception Drift)
- 분기형 텍스트
- VN 시각 폴리싱 (Ken Burns / 선택지 덤 / 잔열 / 필름 그레인)

### 플레이어 경험 변화
- 이전: VN 프롤로그 → 탑다운 맵 탐색 (**약점**) → 턴제 전투 (**약점**) → VN 후일담
- 현재: VN 프롤로그 → VN 수색 (메뉴식) → VN 보스전 (선택지식) → VN 후일담
- **전체가 시네마틱 일관 흐름**. 환세취호전 미학 → House in Fata Morgana / VA-11 Hall-A 미학.

### 다음 세션 (S71) 후보
- [ ] rim_forest.tscn 등 더 이상 안 쓰는 맵 파일 정리 (보존 vs 삭제 결정)
- [ ] BattleManager·ExplorationHUD·Minimap 코드도 데모 빌드에서 제외
- [ ] Ch1 전체 풀 플레이 검증 (실제 플레이 시간 측정)
- [ ] BGM Ch1 시그니처 1트랙 (Suno/Udio) — VN 분위기 강화
- [ ] (사용자) Godot Editor 빌드 → 친구 테스트

---

## S73 — 2026-04-24 (Stuck 버그 수정 + 책 페이지 넘김)

### 친구 1차 피드백
1. *"어느 시점에서 진행이 잘 안된다"* → 진행 멈춤 버그
2. *"대화 넘길 때 책처럼 넘겨지는 그래픽이 있으면 좋겠다"* → VN 페이지 턴 효과 요청

### 1. ch1_void_beast 무한루프 버그 (중대)

**증상**: 보스전 Round 1에서 어떤 선택을 해도 결국 끝나지 않고 같은 텍스트가 반복.

**원인**: JSON step 인덱스 잘못 매핑.
- Round 1 choice gotos: 7/14/21 → 잘못 (각 라우트가 CG/intro narrate를 건너뜀)
- **치명적 루프**: 모든 Round 1 라우트의 출구 `goto_scene start_index 23` → step 23은 read 라우트 *중간* ("When the low strike came..."). 모든 라우트가 거기 떨어져 → step 24~25 진행 → step 26 = `goto_scene start_index 23` → 다시 step 23 → **무한루프**
- Round 2 choice gotos도 28/35/42 잘못. 42는 action 스텝 자체였음.

**수정**:
- Round 1 choice goto: `[6, 13, 20]` (각 라우트의 CG/intro)
- Round 1 exit start_index: `28` (Round 2 CG)
- Round 2 choice goto: `[32, 37, 43]`
- Round 2 exit start_index: `50` (엔딩 CG)
- 8군데 모두 수정. 이제 모든 라우트가 정상 종료 → ch1_after_forest 진입.

### 2. 책 페이지 넘김 효과 (`vn_scene.gd._play_page_turn`)
- 새 라인 표시 직전 종이 엣지 그라디언트 좌→우 0.32s 스윕
- 그라디언트 5단계: 어두운 그림자 → 밝은 페이지 엣지(0.85) → 살짝 어두운 종이 → 투명
- 회전 -2° → +1.5° 동안 변화 (종이가 휘는 인상)
- 알파: 0 → 0.85(0.12s) → 0(0.20s)
- SFX `page_turn` 트리거 (오디오 매니저에 트랙 있으면 재생)
- 연속된 같은 텍스트는 발동 안 함 (`_last_displayed_text` 체크)
- 위치: 화면 하단 250px 영역에 배치 — 텍스트 박스 영역 위로 스윕

### 검증
- void_beast 인덱스 매핑: Python으로 모든 step 덤프해서 검증 완료
- Godot 헤드리스 `--check-only` 통과

### 수정 파일
| 파일 | 변경 |
|------|------|
| `data/vn_scenes/ch1_void_beast.json` | 8개 인덱스 재매핑 |
| `scripts/ui/vn_scene.gd` | 페이지 턴 오버레이 + `_play_page_turn` (~50줄) |

### 친구한테 다시 보낼 때 기대 효과
- 진행 멈춤 사라짐 → 보스전 끝까지 완주 가능
- 대사 한 줄 넘길 때마다 미세하게 종이 한 장 넘기는 인상 → VN 정체성 강화


---

## S77 — 2026-05-19 (game image asset intake and first integration)

### Done
- Imported 36 PNG files from `../이미지/game image/` into the Godot project.
- Runtime-ready illustrations copied to `assets/cg/game_image/`:
  - character full-body CG: Arrel, Elia, Tobias, Nera, Kairos, Veil
  - environment CG: frost city, memory hall, wasteland city, void cathedral, Bureau spires, frozen archive
  - dramatic Arrel ruins/rest CG
- Reference sheets copied to `assets/game_image/reference/`:
  - turnaround sheets
  - expression sheets
  - sprite-sheet references
  - UI and skill icon reference sheets
- Added `assets/game_image/README.md` explaining asset source, runtime CG, and reference-sheet usage.
- Title slideshow now includes the new high-resolution environment and Arrel CGs.
- Ch1 VN scene integration:
  - `ch1_prologue.json`: opening Arrel beat now uses `arrel_fullbody.png`.
  - `ch1_void_beast.json`: post-battle beat now uses `arrel_ruins_rest.png`.
  - `ch1_after_forest.json`: forest exit and Bureau reveal now use `env_wasteland_city.png` and `env_bureau_spires.png`.

### Verification
- VN JSON parse check passed.
- Godot `--check-only` produced only the existing `data.tree is null` plugin noise; no new parse errors or missing-resource script errors.

### Notes
- The new sheets are not sliced into animation frames yet. They are preserved as reference assets because using full reference sheets directly in gameplay would look like concept-art UI, not an in-world scene.
- Next useful step: build an in-game Artbook / Character Dossier screen that displays the turnaround and expression sheets intentionally.

---

## S78 — 2026-05-19 (Artbook / Character Dossier)

### Done
- Added an Artbook entry to the pause menu.
- Added a full-screen Artbook / Character Dossier panel inside `scripts/ui/pause_menu.gd`.
- Artbook displays selected `game image` reference sheets and CG plates:
  - character turnaround sheets
  - expression sheets
  - skill icon atlas reference
  - environment plates
  - Arrel ruins illustration
- Added preview title, asset type label, image preview, and short art-direction notes for each entry.
- Since the current build is mostly VN-driven, PauseMenu can now open during active `SceneFlow` dialogue as well as normal exploration. This makes Artbook / Options / Save accessible during the VN demo flow.

### Verification
- All Artbook `res://` paths exist.
- Godot `--check-only` reports no new parse/missing-script errors; only existing plugin/resource shutdown noise remains.

### Notes
- This intentionally treats the new sheets as in-game dossier/reference material rather than slicing them into gameplay animations immediately.
- Next polish step: add unlock conditions per character/act, or add a title-screen Artbook button for browsing without starting the demo.

---

## S79 - 2026-05-19 (Godot launch fix)

### Fixed
- Restored the `project.godot` plugin configuration so `enabled=PackedStringArray(...)` is under `[editor_plugins]` instead of `[gui]`.
- Added an `is_inside_tree()` guard to `AudioManager._on_tree_changed()` so shutdown/tree-change callbacks do not call `get_tree()` after the autoload leaves the scene tree.

### Verification
- Ran Godot 4.6.2 headless against the project.
- Main scene initialized through all autoloads and reached the title/menu flow.
- The previous repeated `AudioManager._on_tree_changed()` `data.tree is null` backtrace no longer appears.
- Remaining resource-leak messages only appear during forced `--quit-after` shutdown.

---

## S80 - 2026-05-19 (restore full-game progression after demo build)

### Fixed
- Replaced the Act I `demo_end` route in `ch1_after_forest.json` with a transition into `ch2_market_arrival`.
- Added SceneFlow support for progression metadata on VN steps:
  - `set_chapter`
  - `complete_chapter`
  - `autosave_chapter_transition`
- Updated `ch2_market_arrival.json` to enter `verdan_market.tscn` without queuing the missing `ch2_malet_deal` VN scene.
- Added `ch2_arrival_vn_seen` handling in `verdan_market.gd` so the VN arrival can hand off to the existing map-based Ch2/Malet progression without replaying the old arrival dialogue.
- Replaced missing `market_bustle.ogg` with the existing `ch2_verdan.mp3` BGM.

### Verification
- VN scene JSON parse check passed.
- Direct Godot headless launch reached title/autoload initialization with no new script errors.
- Direct Godot headless load of `verdan_market.tscn` succeeded.
- Remaining resource-leak messages are from forced `--quit-after` shutdown only.

---

## S81 - 2026-05-19 (calm GAME START title screen)

### Done
- Imported `../이미지/game image/GAME START.png` as `assets/cg/game_image/game_start.png`.
- Rebuilt the main title screen around the single GAME START illustration.
- Removed the previous stacked title effects from `scenes/main/main.gd`:
  - background slideshow
  - god rays
  - foreground ash particles
  - title burst
  - letter cascade
  - ornament/grain overlays
  - splash sequence
- Repositioned the actual menu buttons over the menu frame already baked into the image.
- Renamed title menu labels to match the image language: `NEW GAME`, `CONTINUE`, `SETTINGS`, `EXIT`.
- Kept only a soft fade-in, title BGM, a very light veil, and subtle hover/focus treatment.

### Verification
- Godot headless launch reached the title scene with no new script errors.
- `game_start.png.import` generated successfully.
- Confirmed the removed noisy title effect functions no longer exist in `main.gd`.

---

## S82 - 2026-05-20 (new illustration and character sheet integration)

### Done
- Imported the newly added `game image` batch into project runtime folders:
  - `assets/cg/game_image/` for story CG, chapter plates, item CG, ending gallery plates, and world map art.
  - `assets/game_image/reference/` for UI references, enemy sheets, item sheets, and character sprite/expression references.
- Sliced Malet's expression sheet into eight usable HD dialogue portraits and remapped Malet dialogue keys to the new portraits.
- Replaced lower-quality dialogue CG references across Ch1, Ch2, Ch3, Ch4, Ch6, Ch8, Ch9, Ch10, and VN scenes with the stronger new illustrations.
- Updated ending gallery CG paths to use the new illustration set instead of missing placeholder ending files.
- Updated the Ch9 Kairos battle setup to use the new sealed-city Kairos illustration and fixed the boss enemy constructor call.
- Expanded the pause-menu Artbook with new CG plates, Malet sheets, enemy sheets, UI references, item sheet, and the new world map.
- Removed unreferenced old low-quality CG files and matching `.import` files after verifying they were no longer referenced.
- Replaced the missing `rim_ambient.ogg` VN BGM reference with the existing `ch1_forest.mp3`.

### Verification
- Godot import generated `.import` files for the newly added PNG assets.
- JSON parse and JSON resource checks passed.
- Full `res://` string scan across `data/`, `scripts/`, and `scenes/` reports 0 missing resource references.
- `git diff --check` passed; only normal CRLF working-copy warnings appeared.
- Godot headless launch and direct `colorless_waste.tscn` load report no new script/parse/missing-resource errors.
- The remaining `resources still in use at exit` message appears only during forced `--quit-after` shutdown.

---

## S83 - 2026-05-20 (Arrel / Elia sheet-only art cleanup)

### Done
- Sliced the new Arrel expression sheet into eight runtime portraits:
  - neutral, soft smile, angry, sad, shocked, determined, battle-ready, memory-fading.
- Sliced the new Elia expression sheet into eight runtime portraits:
  - neutral, gentle smile, worried, sad, surprised, determined, healing-focus, memory-restoration.
- Remapped every Arrel / Elia dialogue portrait key in `DialogueBox` to the new sheet-derived portrait files.
- Updated battle-scene portrait references to use the new sheet-derived Arrel / Elia portraits.
- Built sheet-derived runtime CG plates for Arrel, Elia, and Arrel+Elia duo moments.
- Replaced all Arrel / Elia runtime CG references in dialogue/VN data with sheet-derived plates.
- Removed old Arrel / Elia standalone CG files and old portrait files after confirming no runtime references remained.
- Kept only the new Arrel / Elia reference/expression sheets plus the newly generated sheet-derived runtime assets.
- Updated Artbook entries and the asset intake README to reflect the sheet-only Arrel / Elia pipeline.

### Verification
- Old Arrel / Elia runtime reference scan reports 0 remaining references outside the new sheet pipeline.
- Full `res://` resource scan reports 0 missing references.
- JSON parse check passed.
- Godot import generated `.import` files for all new sheet-derived portraits and CG plates.

---

## S84 - 2026-05-20 (sheet sprites in gameplay and current CG-only cleanup)

### Done
- Sliced the new Arrel and Elia sprite sheets into runtime gameplay frames under:
  - `assets/sprites/characters/arrel_sheet/`
  - `assets/sprites/characters/elia_sheet/`
- Added sheet-frame loading to `PixelSprite` so Arrel and Elia can use real image frames in exploration and battle while keeping generated placeholder fallback behavior.
- Updated the player to use Arrel sheet frames in the actual map view.
- Updated Elia's companion sprite to use Elia sheet frames in the actual map view, while preserving Sable's existing generated companion sprite.
- Updated battle presentation so Arrel and Elia use sheet-derived animated battle bodies instead of portrait stand-ins.
- Switched the remaining runtime CG references away from loose legacy files under `assets/cg/` and into the curated `assets/cg/game_image/` set.
- Removed the old loose `assets/cg/` illustrations after confirming runtime references no longer depend on them.
- Restored the custom theme font resource after Godot import rewrote it during asset import.

### Verification
- JSON parse check passed.
- Full `res://` resource scan across `data/`, `scripts/`, and `scenes/` reports 0 missing references.
- Old Arrel / Elia portrait file reference scan reports 0 remaining old portrait file references.
- Loose legacy `assets/cg/` runtime image reference scan reports no actual old image references; only VN scene fallback directory construction remains.
- `git diff --check` passed; only normal CRLF working-copy warnings appeared.
- Godot headless launch, direct battle scene load, and direct Rim Forest scene load report no new script/parse/missing-resource errors.
- The remaining `resources still in use at exit` message appears only during forced `--quit-after` shutdown.

---

## S85 - 2026-05-20 (title and opening dialogue polish)

### Done
- Added a restrained cinematic layer to the title screen:
  - radial vignette for depth,
  - subtle menu backing for readability,
  - slow memory-dust particles over the existing `GAME START` illustration.
- Kept the title composition calm and image-led instead of rebuilding the screen with noisy UI effects.
- Refined the first Arrel / Elia exchange in `ch1_prologue` with a clearer recognition beat and more physical detail.
- Replaced early prologue short CG keys with explicit current-image paths:
  - `sheet_arrel_elia_duo.png`
  - `memory_loss_warning.png`
  - `void_beast_confrontation.png`
- Added direct portrait aliases for sheet-derived Arrel shock and Elia worried/smile expressions.

### Verification
- `ch1_prologue.json` parses successfully.
- New focused resource scan reports 0 missing references.
- `git diff --check` passed for the touched files; only normal CRLF working-copy warnings appeared.
- Godot headless launch reports no new script/parse/missing-resource errors.
- The remaining `resources still in use at exit` message appears only during forced `--quit-after` shutdown.

---

## S86 - 2026-05-21 (character scale and illustrated map atmosphere)

### Done
- Reduced Arrel's actual exploration sprite from the oversized runtime scale to a persistent sheet base scale.
- Fixed movement squash/stretch, sprint stretch, idle breathing, fidget, and afterimage effects so they respect the smaller base sprite scale instead of expanding back to `Vector2(1, 1)`.
- Reduced Elia's exploration companion sheet sprite scale to match the smaller player silhouette.
- Reduced Arrel and Elia battle body sprite scales so the side-view battle scene no longer feels dominated by oversized character art.
- Added `MapEffects.add_illustration_atmosphere()`, a reusable low-alpha CG overlay layer for using curated illustrations directly in map gameplay without hiding the tilemap.
- Applied illustrated atmosphere layers to:
  - Rim Forest: `chapter_sealed_zone.png`
  - BL-07 Void: `nera_void_cavern.png`
  - Colorless Waste: `kairos_sealed_city.png`
  - Crumbling Coast: `sealed_gate_plaza.png`
  - The Seam: `tobias_memory_corridor.png`

### Verification
- Focused resource scan reports 0 missing references for the touched files.
- `git diff --check` passed; only normal CRLF working-copy warnings appeared.
- Godot headless launch, direct Rim Forest load, and direct battle scene load report no new script/parse/missing-resource errors.
- The remaining `resources still in use at exit` message appears only during forced `--quit-after` shutdown.

---

## S87 - 2026-05-22 (illustrated mid-chapter atmosphere and Guard Focus)

### Done
- Extended the reusable illustrated map atmosphere pass to the remaining mid/late chapter field maps:
  - Verdan Market: `malet_bureau_overlook.png`
  - Belt Waystation: `world_map_memoria.png`
  - Drift Shelter: `memory_loss_warning.png`
  - Forgotten Forest: `void_beast_confrontation.png`
  - Seam Outskirts: `sealed_city_ruins.png`
- Upgraded Defend into `Guard Focus` so it now has a stronger tactical role:
  - keeps the existing incoming damage reduction,
  - shortens active player status effects by 1 turn when pressured,
  - restores a small amount of HP when wounded and not status pressured,
  - grants a larger Limit gain when already stable.
- Added battle feedback for `Guard Focus`:
  - player-side `GUARD` status icon while defending,
  - floating Guard Focus callout for status relief, HP restore, or Limit gain,
  - brief shield wash around Arrel when the action is chosen.
- Reworked `MemoryResonance` into a clean ASCII script after finding broken comment/function formatting in the file.
- Improved memory resonance markers with a larger pulse plate, bright core, and four directional sparks so field rewards are more discoverable.
- Restored the custom font theme resource after Godot import simplified it again.

### Verification
- Focused resource scan reports 0 missing references for the newly wired illustration layers.
- `MemoryResonance` scan reports no comment-swallowed function declarations after the cleanup.
- `git diff --check` passed; only normal CRLF working-copy warnings appeared.
- Godot executable was not available on PATH; local/common install-path search timed out before locating it, so scene smoke tests were not run in this pass.

---

## S88 - 2026-05-22 (dialogue interface cleanup)

### Done
- Cleaned up the runtime dialogue interface so dialogue, portraits, speaker names, and choices no longer feel visually mixed together.
- Changed the dialogue box from a nearly full-width strip into a centered lower panel with stronger margins and calmer contrast.
- Added a framed portrait well and hid it on narration/system lines so empty portrait space no longer clutters narration.
- Added a subtle divider between speaker name and dialogue text, improved line spacing, and replaced the noisy next-line marker with a quiet `ENTER` hint.
- Restyled choices as wider numbered buttons above the dialogue panel with more consistent spacing and calmer borders.

### Verification
- Focused dialogue diff review confirmed the interface changes stayed scoped to layout/style behavior, without the earlier accidental comment encoding churn.
- S93 changed-script `res://` reference scan reports 0 missing resources.
- `git diff --check` passed; only normal CRLF working-copy warnings appeared.
- Godot executable was still not available on PATH, so headless scene smoke tests were not run.

---

## S89 - 2026-05-22 (image-referenced graphics pass)

### Done
- Expanded image-referenced presentation using the high-resolution `env_*` CG set.
- Reworked map illustration atmosphere so CG plates read more strongly:
  - added a brighter upper detail band,
  - added lower readability shading so player/tile silhouettes remain legible,
  - kept the existing slow alpha pulse for atmosphere.
- Reassigned several map atmosphere references to more location-specific environment CG:
  - Verdan Market: `env_bureau_spires.png`
  - Belt Waystation: `env_wasteland_city.png`
  - Drift Shelter: `env_frozen_archive.png`
  - Forgotten Forest / The Seam: `env_memory_hall.png`
  - Seam Outskirts / BL-07: `env_void_cathedral.png`
- Upgraded battle backgrounds:
  - explicit battle background images now render with stronger presence,
  - battles with no explicit image now resolve a fallback CG from the return map,
  - added top/side art depth plates plus readability wash and horizon shadow for a more illustrated stage.

### Verification
- Full `res://` reference scan reports 0 missing resources.
- `git diff --check` passed; only normal CRLF working-copy warnings appeared.
- Godot executable was not available on PATH, so headless scene smoke tests were not run.

---

## S90 - 2026-05-22 (VN illustration depth pass)

### Done
- Continued the image-referenced graphics upgrade on the visual-novel scene layer.
- Added a CG detail plate over the upper third of VN scenes so full-screen illustrations read with more depth instead of acting like flat backgrounds.
- Added a persistent lower readability wash behind the text area to keep dialogue legible over bright or busy CG.
- Added soft character grounding shadows under left/right portraits so standing portraits feel attached to the scene.
- Added subtle portrait scale-in motion when a new portrait enters.
- Refined the VN dialogue panel styling with darker glass, quieter borders, and slightly roomier margins.

### Verification
- Full `res://` reference scan reports 0 missing resources.
- `git diff --check` passed; only normal CRLF working-copy warnings appeared.
- Godot executable was not available on PATH, so headless scene smoke tests were not run.

---

## S91 - 2026-05-22 (interface refinement pass)

### Done
- Refined the shared exploration dialogue UI:
  - calmer glass-panel borders and margins,
  - slightly clearer speaker hierarchy,
  - text shadow for stronger readability over illustrated maps,
  - quieter pulsing `NEXT` indicator instead of a static prompt.
- Restyled exploration choices as slimmer dark cards with a left accent rule and softer staggered fade-in animation.
- Matched the visual-novel choices to the same calmer card language.
- Softened system log and tutorial hint panels so they feel closer to the rest of the current UI.

### Verification
- Full `res://` reference scan reports 0 missing resources.
- `git diff --check` passed; only normal CRLF working-copy warnings appeared.
- Godot executable was not available on PATH, so headless scene smoke tests were not run.

---

## S92 - 2026-05-23 (Memory Pulse exploration update)

### Done
- Added `Memory Pulse`, a new active exploration action bound to `Q` and gamepad face button 3.
- Player can now send out a pulse while exploring:
  - creates two expanding pulse rings around Arrel,
  - briefly flashes nearby Memory Resonance echoes,
  - reports the closest echo by memory title and distance in paces,
  - uses a 6-second cooldown.
- Extended `MemoryResonance` triggers with scan metadata and a `pulse_scan()` helper.
- Added temporary `ECHO` callouts to scanned resonance points.
- Added HUD support for Memory Pulse readiness/cooldown.
- Added a first-use tutorial hint for Memory Pulse.

### Verification
- Full `res://` reference scan reports 0 missing resources.
- `git diff --check` passed; only normal CRLF working-copy warnings appeared.
- Focused scan confirmed the new `memory_pulse` input, player action, HUD status, resonance scan helper, and tutorial hint are wired.
- Godot executable was not available on PATH, so headless scene smoke tests were not run.

---

## S93 - 2026-05-23 (foreground image utilization pass)

### Done
- Reworked chapter title cards so they now use location CG directly:
  - soft full-screen art backdrop,
  - prominent centered art plate,
  - existing chapter text layered over the illustration.
- Added chapter-to-image mapping for all major locations.
- Added an exploration location art card to the HUD:
  - appears when entering a new map,
  - shows a cropped CG thumbnail for the current region,
  - uses the current chapter/location text as context.
- Expanded `ExplorationHUD` location metadata to include all major field maps and their art references.

### Verification
- Full `res://` reference scan reports 0 missing resources.
- `git diff --check` passed; only normal CRLF working-copy warnings appeared.
- Focused scan confirmed chapter-title art layers and exploration location art card wiring.
- Godot executable was not available on PATH, so headless scene smoke tests were not run.

## S94 - 2026-05-23 (dialogue illustration staging pass)

### Done
- Added large speaker-stage illustrations to `DialogueBox`:
  - Arrel/Elia now use profile CG as translucent left/right stage art during dialogue.
  - Kairos/Nera/Tobias use full-body CG when speaking.
  - Other speakers fall back to their portrait art so every spoken line has a stronger visual presence.
- Added smooth speaker-stage transitions:
  - active speaker fades/slides in from the side,
  - inactive side fades out,
  - narration/system lines clear the stage art.
- Upgraded `CgViewer` presentation:
  - full-screen CG now gets top/bottom cinematic wash overlays,
  - CG is slightly enlarged and positioned for a less flat slideshow feel,
  - overlays fade with the CG so dialogue readability is cleaner.

### Verification
- Changed-script `res://` reference scan reports 0 missing resources.
- `git diff --check` passed; only normal CRLF working-copy warnings appeared.
- Focused scan confirmed speaker-stage art functions and CG wash overlay wiring.
- Godot executable was not available on PATH, so headless scene smoke tests were not run.

## S95 - 2026-05-23 (battle illustration integration pass)

### Done
- Added battle-stage illustration layers to `BattleScene`:
  - Arrel battle CG now sits faintly behind the player side.
  - Enemy side resolves to enemy image, character full-body art, void beast CG, or the current battle background.
  - Left/right wash panels keep the art readable behind sprites and UI.
- Added attack cut-in illustrations:
  - normal Arrel attacks flash `sheet_arrel_battle_ready`,
  - memory burn attacks flash `memory_loss_warning`,
  - enemy attacks flash the resolved enemy-side illustration.
- Kept cut-ins short and translucent so they add impact without blocking tactical UI.

### Verification
- Changed-script `res://` reference scan reports 0 missing resources.
- `git diff --check` passed; only normal CRLF working-copy warnings appeared.
- Focused scan confirmed battle-stage art and action cut-in wiring.
- Replaced an unsafe TextureRect stretch constant with Godot's existing `STRETCH_KEEP_ASPECT_CENTERED`.
- Godot executable was not available on PATH, so headless scene smoke tests were not run.

## S96 - 2026-05-23 (battle break gameplay loop)

### Done
- Added an enemy BREAK system to `BattleManager`:
  - weakness hits add major break pressure,
  - neutral hits add minor pressure,
  - resisted hits add no pressure,
  - bosses gain break pressure more slowly.
- Broken enemies:
  - lose their next turn,
  - take +35% damage while broken,
  - trigger battle log feedback and a first-time tutorial hint.
- Added BREAK UI to the enemy battle panel:
  - dedicated BREAK bar under enemy HP,
  - BROKEN label/status icon while the enemy is staggered,
  - animated bar updates and enemy squash feedback on break.
- Applied break pressure to Attack, Burn skills, and Elia's offensive void skill.

### Verification
- Changed-script `res://` reference scan reports 0 missing resources.
- `git diff --check` passed; only normal CRLF working-copy warnings appeared.
- Focused scan confirmed BREAK variables, signals, UI handlers, and tutorial hint wiring.
- Godot executable was not available on PATH, so headless scene smoke tests were not run.

## S97 - 2026-05-23 (title and early dialogue stability pass)

### Done
- Fixed the title menu overlap risk:
  - moved the GAME START button stack lower into the intended frame,
  - widened the backing plate,
  - reduced button height/font size and increased spacing so four buttons no longer crowd each other.
- Calmed the early Arrel/Elia dialogue composition:
  - speaker-stage art is smaller, dimmer, and constrained away from the lower dialogue box,
  - full-screen CG dialogue lines now suppress side speaker art so the scene does not stack multiple competing illustrations.
- Hardened dialogue CG handling:
  - restored the CG caption panel reference that could be skipped by a malformed/commented line,
  - added null-safe caption panel access,
  - added close-state guards so repeated dialogue advances or deferred CG closing do not race the fade tween.

### Verification
- `git diff --check` passed; only normal CRLF working-copy warnings appeared.
- Changed-script `res://` reference scan reports 0 missing resources.
- Focused scan confirmed title sizing, CG-line suppression, and CG close guard wiring.
- Godot executable was not available on PATH, so headless scene smoke tests were not run.

## S98 - 2026-05-23 (VN crash and Arrel/Elia composition fix)

### Done
- Rewrote `CgViewer` with clean ASCII comments and explicit control flow:
  - removed a broken commented `if auto_close_sec > 0` line that could leave `await` under an invalid block,
  - replaced meta-based caption panel lookup with a direct `_text_panel` reference,
  - kept close/fade race guards for repeated dialogue advance.
- Fixed several `VNScene` lines where code had been swallowed into comments:
  - restored CG change checks, continue indicator creation, ember vignette creation, Ken Burns startup, burn flash, page-turn positioning, and choice dimming color handling.
- Reduced early Arrel/Elia visual clutter in the actual VN flow:
  - VN portraits are smaller,
  - Arrel/Elia conversations now use one active portrait instead of keeping both sides visible,
  - the duplicated CG detail overlay is disabled so the same illustration is not stacked over itself.
- Kept the DialogueBox safeguard from S97:
  - Arrel/Elia full-stage side illustrations remain disabled outside the VN flow too.

### Verification
- `git diff --check` passed; only normal CRLF working-copy warnings appeared.
- Changed-script `res://` reference scan reports 0 missing resources.
- Focused scan confirmed clean `CgViewer` auto-close flow, single-portrait VN composition, and disabled Arrel/Elia stage art.
- Godot executable was not available on PATH, so headless scene smoke tests were not run.

## S99 - 2026-05-24 (progression crash cleanup)

### Done
- Rechecked the latest Godot logs and older crash evidence:
  - current log reached Chapter 2 exploration without a fatal stack trace,
  - older logs showed `Lambda capture at index 2 was freed` after Verdan exploration triggers.
- Hardened `MapEffects.add_npc_wander`:
  - recurring tween callbacks now keep a `WeakRef` instead of a direct captured NPC node,
  - this prevents delayed wander callbacks from touching freed map/NPC nodes after scene transitions.
- Cleaned `SceneFlow` after the previous encoding/comment cleanup:
  - kept a single `resume_queue` declaration,
  - kept a single `current_index += 1` in `goto_scene`.
- Improved VN CG resilience:
  - short CG names now search `assets/cg/game_image/` before `assets/cg/`,
  - legacy Ch1 CG names map to existing images,
  - unresolved short CG names fall back to a safe existing chapter image instead of emitting missing-resource warnings.
- Removed an accidental duplicate `_start_ken_burns` function declaration in `VNScene`.

### Verification
- `git diff --check` passed; only normal CRLF working-copy warnings appeared.
- All JSON files under `data/` parse successfully with PowerShell JSON parsing.
- Focused duplicate-function scan found no duplicate top-level `func` declarations in the touched files.
- Changed-script `res://` reference scan reports 0 missing resources.
- Godot executable was not available on PATH, so headless scene smoke tests were not run.

## S100 - 2026-05-24 (second error audit and callback hardening)

### Done
- Ran a project-only error audit excluding addon/example files:
  - duplicate top-level GDScript function scan,
  - `res://` resource existence scan,
  - full `data/**/*.json` parse check,
  - callback/timer review for delayed node access.
- Removed the missing Windows native icon reference from `project.godot`:
  - `config/icon` still points to the existing `res://icon.svg`,
  - `config/windows_native_icon` no longer points to absent `res://icon.ico`.
- Cleaned a remaining VN animation duplication:
  - `_swap_cg()` now starts Ken Burns once instead of twice on the same CG node.
- Hardened delayed callbacks that could fire after scene/UI teardown:
  - battle cut-in hide callback checks node validity,
  - hit flash material cleanup uses `WeakRef`,
  - enemy warning glow cleanup checks `enemy_sprite`,
  - memory burn preview hide callback checks popup nodes,
  - delayed layered SFX playback checks its `AudioStreamPlayer` before playing.

### Verification
- `git diff --check` passed; only normal CRLF working-copy warnings appeared.
- All JSON files under `data/` parse successfully.
- Project-only `res://` scan reports 0 missing resources.
- Project-only duplicate-function scan reports 0 duplicate top-level `func` declarations.
- Godot executable was not available on PATH, so headless scene smoke tests were not run.

## S101 - 2026-05-24 (integrated graphics, gameplay, and story pass)

### Done
- Upgraded the title screen presentation:
  - bumped the displayed version to `v0.9.2`,
  - added a restrained gold menu rule and small caption above the menu frame,
  - kept the calmer `GAME START` image-first composition without moving buttons back into the crowded area.
- Improved VN choice presentation:
  - added optional `choice_title` and `choice_hint` support,
  - widened the choice column and added visible effect text per choice,
  - removed duplicate setup lines left in `VNScene`.
- Added reusable SceneFlow reward hooks:
  - choices and regular steps can now grant Grains, items, or HP through JSON fields,
  - reward processing now happens after step gating so skipped conditional steps cannot grant rewards.
- Strengthened the Chapter 1 story branch:
  - the first memory-spend decision now shows its mechanical consequence,
  - burning the campfire song now has its own follow-up narration and Elia reaction,
  - the old "No fire" narration is gated to the refusal branch only.
- Connected early story choices to gameplay:
  - burning the song gives the next battle +18 Limit and +22 BREAK pressure,
  - refusing to burn grants a Smoke Bomb and guards the first enemy blow in the next battle,
  - listening to Elia's humming can heal and gives +10 Limit in the next battle,
  - alternate reflection choices can grant Grains or a Firebomb.

### Verification
- All JSON files under `data/` parse successfully.
- Project-only `res://` scan reports 0 missing resources.
- Project-only duplicate-function scan reports 0 duplicate top-level `func` declarations.
- Focused swallowed-comment scan on changed GDScript files found no suspicious commented-out code.
- `git diff --check` passed; only normal CRLF working-copy warnings appeared.
- Godot executable was not available on PATH, so headless scene smoke tests were not run.

## S102 - 2026-05-24 (title screen art-only rollback)

### Done
- Simplified the title screen back to an illustration-first presentation:
  - removed code-generated shade, vignette/gradient, menu backing, dust particles, caption, gold rule, and version label,
  - kept `GAME START.png` as the only visible intro image,
  - converted title menu buttons into invisible hit targets so they no longer overlap the artwork text.
- Removed now-unused title variables after hiding the visible overlay UI.

### Verification
- `git diff --check` passed; only normal CRLF working-copy warnings appeared.
- Focused scan confirmed no title overlay/gradient variables remain in `scenes/main/main.gd`.
- Chapter 1 VN JSON still parses successfully.
- Godot executable was not available on PATH, so headless scene smoke tests were not run.

## S103 - 2026-05-24 (Godot warning-as-error launch fix)

### Done
- Fixed launch-blocking warning-as-error failures reported by the Godot editor:
  - typed `WeakRef` variables explicitly in delayed callbacks,
  - typed `get_ref()` callback locals explicitly in `MapEffects`, `BattleScene`, and `AudioManager`,
  - removed Variant inference warnings from `MemoryResonance` setup, trigger, scan, and reward code.
- Verified the title rollback did not reintroduce overlay references.

### Verification
- Focused scan found no remaining `:= weakref(...)`, `:= get_ref(...)`, or `:= INF` patterns.
- `git diff --check` passed; only normal CRLF working-copy warnings appeared.
- Godot 4.6.2 headless project load passed.
- Godot 4.6.2 headless `res://scenes/main/main.tscn` load passed.
- Headless quit still reports expected immediate-exit resource cleanup warnings, but no compile or parse errors.

## S104 - 2026-05-24 (new game VN start crash fix)

### Done
- Fixed the crash reported after pressing GAME START:
  - removed a duplicated empty `if step.has("cg"):` block in `VNScene._on_step_changed`,
  - removed a duplicated local `var c` declaration in `_dim_background_for_choice`.
- Added and removed a temporary smoke scene to test the real startup path:
  - set `SceneFlow.pending_scene_id = "ch1_prologue"`,
  - transitioned through `res://scenes/main/vn_host.tscn`,
  - allowed the VN UI and first prologue step to load.

### Verification
- Godot 4.6.2 headless `res://scenes/ui/vn_scene.tscn` load passed after the fix.
- Godot 4.6.2 smoke run for `GAME START -> vn_host -> ch1_prologue` passed with no output errors.
- `git diff --check` passed before the final smoke run; only normal CRLF working-copy warnings appeared.

## S105 - 2026-05-24 (supporting character and boss art routing)

### Done
- Expanded dialogue-stage art beyond Arrel and Elia:
  - added Sable, Seric, and Veil stage-art routing,
  - added Veil portrait aliases and default portrait support,
  - added distinct dialogue blip pitch values for Nera, Seric, and Veil.
- Improved battle art selection:
  - added name-based enemy art fallback for Kairos, Nera, Tobias, Veil, and Void/Shade/Sentinel/Wraith/Fragment/Lurker enemies,
  - made enemy sprites use the same fallback art when `BattleManager.enemy_image` is empty,
  - updated the Chapter 9 Kairos boss fight to use `kairos_fullbody.png` as the enemy image while keeping `kairos_sealed_city.png` as the battle background.

### Verification
- Project-only `res://` scan reports 0 missing resources.
- `git diff --check` passed; only normal CRLF working-copy warnings appeared.
- Godot 4.6.2 headless `res://scenes/main/main.tscn` load passed with no script or parse errors.
- Temporary battle-art smoke scene loaded Kairos and Shade Sentinel battle scenes with no script or parse errors, then was removed.
- Headless quit still reports expected immediate-exit resource cleanup warnings, but no compile or parse errors.

## S106 - 2026-05-25 (remaining art coverage and battle codex routing)

### Done
- Rechecked image coverage across `assets/cg/game_image`, `assets/portraits`, and `assets/game_image/reference`.
- Centralized enemy image fallback in `BattleManager.resolve_enemy_image_by_name`:
  - Kairos/Nera/Tobias/Veil route to full-body art,
  - Void Beast/Shade/Sentinel/Threshold route to the void confrontation art,
  - Void Wisp/Wraith/Fragment/Lurker route to the void creature sheet,
  - Crawler/Soldier route to the memory-lost soldier sheet,
  - Guardian route to the forgotten guardian sheet.
- Made `BattleManager.start_battle` assign fallback enemy images when callers pass an empty image path.
- Updated `Codex` bestiary previews to use the same fallback image routing, including older entries without saved `image_path`.
- Added a visible Tobias backline support sprite in battle when Tobias is in the party.

### Verification
- Project-only `res://` scan reports 0 missing resources.
- `git diff --check` passed; only normal CRLF working-copy warnings appeared.
- Godot 4.6.2 headless `res://scenes/main/main.tscn` load produced no script or parse errors.
- Temporary art-routing smoke scene loaded Kairos and Void Wraith battle scenes with Tobias support active and produced no script or parse errors, then was removed.

## S107 - 2026-05-25 (integrated battle presentation and reward loop update)

### Done
- Upgraded battle openings:
  - enemy art now appears inside the battle intro overlay,
  - weakness/resistance/class tags are shown before the first turn,
  - `BattleManager` emits a short tactical hint at encounter start.
- Strengthened the 30-second battle loop:
  - scanning an enemy now matters economically,
  - victories grant a small Codex tactical bonus when the current enemy was scanned,
  - the animated victory screen now shows the Codex Bonus row.
- Kept the image routing unified:
  - `BattleScene` now delegates enemy art fallback to `BattleManager.resolve_enemy_image_by_name`,
  - this keeps battle sprites, cut-ins, and Codex previews aligned.

### Verification
- Project-only `res://` scan reports 0 missing resources.
- `git diff --check` passed; only normal CRLF working-copy warnings appeared.
- Godot 4.6.2 headless `res://scenes/main/main.tscn` load produced no script or parse errors.
- Temporary S107 battle smoke scene loaded Kairos and Void Wraith battle openings, Tobias support, and the Codex Bonus victory screen with no script or parse errors, then was removed.

## S108 - 2026-05-25 (Arrel face-only portrait crop fix)

### Done
- Created face-only Arrel portrait crops for all current Arrel emotion states:
  - neutral, angry, battle ready, determined, memory fading, sad, shocked, and soft smile.
- Rewired `DialogueBox.PORTRAIT_MAP` so Arrel dialogue portraits use the cropped face images instead of the larger sheet images.
- Rewired the battle HP panel portrait to use `arrel_face_neutral.png`.
- Kept the original full illustration/sheet assets intact for CG, stage, and cut-in use.

### Verification
- Godot 4.6.2 headless asset smoke loaded all new `arrel_face_*.png` textures with no script or parse errors, then the temporary smoke scene was removed.
- Godot 4.6.2 headless `res://scenes/main/main.tscn` load produced no script or parse errors.
- Project-only `res://` scan reports 0 missing resources.
- Focused scan found no remaining gameplay/UI references to the old Arrel sheet portrait files.
- `git diff --check` passed; only normal CRLF working-copy warnings appeared.

## S109 - 2026-05-25 (Elia face-only portrait consistency pass)

### Done
- Created face-only Elia portrait crops for all current Elia emotion states:
  - neutral, determined, gentle smile, healing focus, memory restoration, sad, surprised, and worried.
- Rewired `DialogueBox.PORTRAIT_MAP` so Elia dialogue portraits use the cropped face images instead of larger sheet images.
- Rewired the battle ally portrait fallback to use `elia_face_neutral.png`.
- Kept the original Elia full illustration/sheet assets intact for CG, stage, and cut-in use.

### Verification
- Godot 4.6.2 headless asset smoke loaded all new `elia_face_*.png` textures with no script or parse errors, then the temporary smoke scene was removed.
- Godot editor headless import pass generated `.import` metadata for the new Elia face crops.
- Godot 4.6.2 headless `res://scenes/main/main.tscn` load produced no script or parse errors.
- Project-only `res://` scan reports 0 missing resources.
- Focused scan confirms Elia gameplay/UI portrait references now use `elia_face_*.png`.

## S110 - 2026-05-25 (major NPC face portrait and Malet visibility pass)

### Done
- Created face-only portrait crops for major non-Arrel/Elia characters:
  - Malet: neutral, amused, calculating, deal accepted, disappointed, price revealed, smile, and warning.
  - Sable, Tobias, Kairos, Nera, and Seric: neutral/key-state face crops.
- Rewired `DialogueBox.PORTRAIT_MAP` so Malet/Mallet, Sable, Tobias, Kairos, Nera, and Seric dialogue portraits use the new face crops instead of raw full portrait files.
- Added Mallet spelling aliases to prevent image fallback gaps when the merchant name is entered with the alternate spelling.
- Made major NPC map sprites prefer the new portrait crops over generic PixelSprite placeholders when a matching face crop exists.
- Added a merchant portrait slot to `MemoryShop`, so Malet's image appears directly in the memory exchange UI.
- Updated battle ally/support fallback art for Sable and Tobias to use the new face crops.

### Verification
- Godot editor headless import pass generated `.import` metadata for all 17 new face crops and reported no new script or parse errors.
- Godot 4.6.2 headless game boot reached the main menu with no script or parse errors.
- Focused scan confirms old raw major-NPC portrait files are no longer referenced by gameplay/UI scripts.
- Focused `git diff --check` passed for the files touched in this pass; a pre-existing trailing-whitespace warning remains in `scripts/ui/vn_scene.gd`.

## S111 - 2026-05-25 (character presentation micro-polish)

### Done
- Added speaker-colored portrait framing in `DialogueBox`, including a bottom accent strip that changes per active speaker.
- Added extra speaker color handling for Mallet, Nera, Seric, Tobias, and Veil so dialogue and journal surfaces avoid the generic fallback color.
- Improved major NPC map presentation:
  - portrait-based NPCs now sit slightly higher,
  - get a small ground shadow,
  - and receive a thin speaker-colored frame so they read as authored character objects instead of loose pasted images.
- Refined the MemoryShop header with a merchant caption line, giving Malet's shop a more intentional broker/merchant presentation.

### Verification
- Godot 4.6.2 headless game boot reached the main menu with no script or parse errors.
- Godot 4.6.2 headless `res://scenes/maps/verdan_market.tscn` load instantiated Malet successfully with no script or parse errors.
- Focused `git diff --check` passed for the files touched in this pass; only normal CRLF working-copy warnings appeared.

## S112 - 2026-05-26 (premium global graphics lens pass)

### Done
- Added a reusable screen-space premium lens shader:
  - subtle paper grain,
  - stronger cinematic edge darkening,
  - faint letterbox weight,
  - slow diagonal light-shaft shimmer.
- Added `MapEffects.add_premium_map_lens()` and connected it across all 10 exploration maps with biome-specific tint/strength settings.
- Added the same lens language to battle presentation so combat shares the map/VN visual tone.
- Restored the VN CG detail overlay path by removing the early return in `_sync_cg_presentation_layers()`, so the top-detail layer can actually animate again.
- Fixed a discovered `forgotten_forest` fog-call parse error while validating the upgraded map pass.

### Verification
- `git diff --check` passed for the files touched in this pass; only normal CRLF working-copy warnings appeared.
- Godot 4.6.2 headless project boot passed.
- Godot 4.6.2 headless `res://scenes/battle/battle_scene.tscn` load passed.
- Godot 4.6.2 headless `res://scenes/ui/vn_scene.tscn` load passed.
- Godot 4.6.2 headless load passed for all 10 map scenes:
  - rim_forest, verdan_market, belt_waystation, drift_shelter, crumbling_coast,
  - the_seam, seam_outskirts, forgotten_forest, colorless_waste, bl07_void.
- Headless quit still reports expected immediate-exit resource cleanup warnings, but no compile or parse errors.

## S113 - 2026-05-27 (tactical battle objective gameplay loop)

### Done
- Added per-encounter Tactical Objectives to make ordinary battles more deliberate:
  - Pressure Point: trigger BREAK before victory,
  - Clean Hands: win without burning a memory,
  - Archivist's Eye: scan/analyze before victory,
  - Measured Assault: reach Combo x3 before victory.
- Added objective state tracking in `BattleManager`:
  - objective generation at battle start,
  - complete/fail state updates,
  - mid-battle checks for BREAK, scan, combo, and memory burn failure,
  - victory-time objective reward finalization.
- Added objective rewards:
  - bonus Grains,
  - occasional item rewards tied to the objective type,
  - objective rows in the animated victory reward screen.
- Added an in-battle objective panel so the current tactical goal is visible during combat and changes color when completed or lost.

### Verification
- Focused `git diff --check` passed for the changed battle files; only normal CRLF working-copy warnings appeared.
- Godot 4.6.2 headless project boot passed.
- Godot 4.6.2 headless `res://scenes/battle/battle_scene.tscn` load passed.
- Temporary tactical-objective smoke scene confirmed the real `BattleManager.start_battle()` path creates an objective and resolves a positive objective reward, then the temporary files were removed.
- Headless quit still reports expected immediate-exit resource cleanup warnings, but no compile or parse errors.

## S114 - 2026-05-28 (memory compass world-rewrite system)

### Done
- Added `MemoryCompass` as a new exploration autoload that turns the original Memory Compass lore into a live play surface:
  - contextual needle states for Rim Forest, Verdan, Belt Waystation, Drift Shelter, The Seam, Forgotten Forest, Colorless Waste, and BL-07,
  - memory-density driven fallback states when the current scene has no bespoke compass profile,
  - lore lines pulled from the project's core themes: soil remembers rain, stone remembers pressure, and BL-07 melts direction itself.
- Made memory burns visibly rewrite the world outside battle:
  - every `memory_burned` signal now refreshes the current scene's `PerceptionFilter`,
  - a screen pulse and compass shock call out the exact contour that vanished,
  - Elia, Tobias, Sable, and identity-grade memories get special burn language.
- Added a compact cinematic compass panel below the existing location card, with animated needle drift, burn-pulse feedback, and `C` key hide/show support.

### Verification
- `git diff --check -- project.godot scripts/ui/memory_compass.gd` passed; only normal CRLF working-copy warnings appeared.
- Godot 4.6.2 headless project boot passed.
- Temporary MemoryCompass smoke scene burned a real memory through `MemoryManager.burn_memory_silent()`, triggered connected systems, and exited with no script or parse errors, then the temporary files were removed.
- Headless quit still reports expected immediate-exit resource cleanup warnings, but no compile or parse errors.

## S115 - 2026-05-28 (world rewrite director and visible loss echoes)

### Done
- Added `WorldRewriteDirector` as a new autoload that turns memory loss into a directed world event instead of a passive state change.
- Added bespoke rewrite rules for major original-memory contours:
  - Verdan taste/vendor recognition,
  - Elia's campfire song and anchoring warmth,
  - the reaching hand relationship memory,
  - Arrel's first sword identity memory,
  - Arrel's name origin,
  - Tobias's record memory,
  - Sable's witness memory.
- Memory burns now set durable story flags such as `world_rewrite_elia_hum_unmoored` and `world_forgot_<memory_id>`, giving future dialogue, maps, endings, and encounters concrete hooks for irreversible consequences.
- Memory burns and fades now spawn visible in-map loss echoes:
  - spectral shards near the player/current scene,
  - short authored consequence text,
  - grade-based color and intensity,
  - automatic fade/float cleanup.
- Connected the director back into `MemoryCompass`, so the compass now reports the authored consequence from the rewrite director instead of only showing generic burn text.
- Added residual absence behavior when entering a scene after memories have already been burned, making revisited spaces feel like they have adjusted around what Arrel lost.

### Verification
- `git diff --check -- project.godot scripts/ui/memory_compass.gd scripts/systems/world_rewrite_director.gd` passed; only normal CRLF working-copy warnings appeared.
- Godot 4.6.2 headless project boot passed.
- Temporary world-rewrite smoke scene burned `daily_campfire_song`, confirmed the director set `world_rewrite_elia_hum_unmoored`, and exited with no script or parse errors, then the temporary files were removed.
- Headless quit still reports expected immediate-exit resource cleanup warnings, but no compile or parse errors.

## S116 - 2026-05-28 (living loss chronicle)

### Done
- Extended `WorldRewriteDirector` with a readable loss-record API:
  - `get_loss_records()` returns burned/faded memory records,
  - `get_rewrite_report(memory_id)` exposes the authored world consequence for future systems,
  - each record includes memory title, grade class, world consequence, compass reading, and durable story hook.
- Added a new `Losses` tab to `StoryJournal`, turning irreversible memory burn into a readable player biography instead of a temporary notification.
- The Losses tab now lists every burned/faded memory with its consequence text from the world rewrite director, so players can review exactly how Arrel and the world have changed.
- The system keeps the original "Blank Book / record-tree contour" theme alive mechanically: the game now records the shape of what was lost, even when the memory itself is gone.

### Verification
- `git diff --check -- scripts/systems/world_rewrite_director.gd scripts/ui/story_journal.gd` passed; only normal CRLF working-copy warnings appeared.
- Godot 4.6.2 headless project boot passed.
- Temporary loss-journal smoke scene burned `identity_first_sword`, confirmed an identity loss record was generated, opened and closed `StoryJournal`, and exited with no script or parse errors, then the temporary files were removed.
- Headless quit still reports expected immediate-exit resource cleanup warnings, but no compile or parse errors.

## S117 - 2026-06-18 (generated rewrite art integration)

### Done
- Generated 10 local dark-fantasy CG/system-art PNGs under `assets/cg/generated/`:
  - `memory_burn_first_sword.png`
  - `memory_burn_elia_song.png`
  - `memory_burn_reaching_hand.png`
  - `memory_burn_arrel_name.png`
  - `world_rewrite_verdan_market.png`
  - `world_rewrite_tobias_record_tree.png`
  - `world_rewrite_elia_anchor.png`
  - `world_rewrite_sable_witness.png`
  - `ui_memory_compass_close.png`
  - `ui_loss_record_blank_book.png`
- Connected generated CGs to `WorldRewriteDirector` rewrite rules so key memory burns now show a short fullscreen art flash before fading back to gameplay.
- Added fallback generated art for uncatalogued Grade 1/2 and low-grade loss records, so future memories still get visual treatment even before bespoke art exists.
- Extended `WorldRewriteDirector` loss records with `art` paths.
- Added a `StoryJournal` detail preview image for Losses entries, so the loss chronicle now shows visual memory evidence instead of text only.

### Verification
- Visually inspected generated `memory_burn_first_sword.png` and `world_rewrite_elia_anchor.png`.
- `git diff --check -- scripts/systems/world_rewrite_director.gd scripts/ui/story_journal.gd assets/cg/generated` passed; only normal CRLF working-copy warnings appeared.
- Godot 4.6.2 headless project boot passed and imported the generated PNG assets.
- Temporary generated-art smoke scene burned `identity_first_sword`, confirmed a `WorldRewriteArtFlash` node was created, confirmed loss record art paths exist, opened and closed `StoryJournal`, then was removed.
- Headless quit still reports expected immediate-exit resource cleanup warnings, but no compile or parse errors.

## S118 - 2026-06-18 (interface cohesion upgrade)

### Done
- Upgraded `MemoryUI` into a more informative archive surface:
  - added an archive-state summary line with held/burned/fading/eroding memory counts,
  - surfaced the latest world rewrite/loss-record count at the top of the archive,
  - added generated loss-art previews to the memory detail panel when a memory has rewrite evidence,
  - added authored world-consequence text beside each applicable memory.
- Added a compact status summary to `StoryJournal` showing current chapter, held memories, burned memories, and recorded losses.
- Improved journal detail refresh behavior so stale loss artwork is cleared when switching lists or tabs.
- Added generated Memory Compass artwork as a subtle background plate behind the live compass UI, keeping the text/needle readable.
- Added recorded-loss count to the pause menu's current-run status block.

### Verification
- `git diff --check -- scripts/ui/memory_ui.gd scripts/ui/story_journal.gd scripts/ui/pause_menu.gd SESSION_LOG.md` passed; only normal CRLF working-copy warnings appeared.
- `scripts/ui/memory_compass.gd` is still an untracked project file, so it was checked separately for trailing whitespace and passed.
- Godot 4.6.2 headless project boot passed with no script or parse errors.
- Temporary UI smoke scene burned `identity_first_sword`, opened and closed `MemoryUI`, opened and closed `StoryJournal`, and exited with no script or parse errors, then the temporary files were removed.

## S119 - 2026-06-18 (fast stability audit)

### Done
- Ran a fast project stability pass after the S95-S118 checkpoint.
- Verified dialogue JSON parsing with explicit UTF-8 handling to avoid false failures from PowerShell's default code page.
- Re-ran project-only integrity scans excluding addon/example noise:
  - `res://` references in active project assets/data/scenes/scripts,
  - duplicate top-level GDScript function declarations,
  - strict merge-conflict markers,
  - staged/working-tree whitespace checks.
- Ran Godot 4.6.2 headless smoke coverage across:
  - project boot,
  - main menu,
  - battle scene,
  - VN scene,
  - all 10 exploration maps.
- Added and removed a temporary runtime smoke scene that exercised:
  - `SceneFlow.play()` / `advance()`,
  - `MemoryManager.burn_memory_silent()`,
  - `WorldRewriteDirector.get_loss_records()`,
  - `MemoryUI` open/close,
  - `StoryJournal` open/close,
  - `BattleManager.start_battle()`.

### Verification
- UTF-8 JSON parse passed for all 16 files under `data/`.
- Project `res://` reference scan passed for 105 active project files.
- Project duplicate top-level function scan passed.
- Strict conflict marker scan passed.
- `git diff --check` passed.
- Godot smoke suite passed for project boot, main menu, battle scene, VN scene, and all 10 exploration maps.
- Temporary runtime systems smoke passed and the temporary files were removed.
- No stability fixes were required in this pass.

## S120 - 2026-06-18 (world rewrite art crash fix)

### Done
- Fixed a runtime crash when a memory burn tried to show generated world-rewrite art.
- Root cause:
  - `WorldRewriteDirector._show_rewrite_art()` was assigning `modulate` directly on a `CanvasLayer`.
  - Godot 4 `CanvasLayer` does not expose `modulate`, so the game crashed with `Invalid access to property or key 'modulate' on a base object of type 'CanvasLayer'`.
- Changed the flash structure so `CanvasLayer` only owns layer ordering, while a child `Control` root owns `modulate` fade-in/fade-out and contains the art, wash, and shade nodes.

### Verification
- Temporary rewrite-art crash smoke directly called `_show_rewrite_art()` and confirmed `WorldRewriteArtFlash` creates a fadeable `Control` root with no script errors.
- Temporary burn-path smoke called `MemoryManager.burn_memory("identity_first_sword")`, confirmed world-rewrite art flash and loss records are generated, and exited with no script errors.
- Godot 4.6.2 headless project boot passed with no script or parse errors.
- `git diff --check -- scripts/systems/world_rewrite_director.gd SESSION_LOG.md` passed; only normal CRLF working-copy warnings appeared.
- Temporary smoke files were removed.

## S121 - 2026-06-19 (VN protagonist illustration overlap fix)

### Done
- Fixed a clean-composition issue in `VNScene` where protagonist dialogue could show a character-focused CG and side portrait at the same time.
- Added a CG-aware portrait suppression rule:
  - when Arrel/Elia dialogue uses a CG path that already contains that speaker, `arrel_elia`, or `duo`, the side portrait slots are cleared for that line,
  - normal background CG dialogue keeps portraits,
  - the next regular Arrel/Elia line still restores the single active-side portrait composition.
- Added `_clear_portraits()` helper so both portrait slots and shadows are reset through the existing portrait state path.

### Verification
- Godot 4.6.2 headless `res://scenes/ui/vn_scene.tscn` load passed.
- Godot 4.6.2 headless project boot passed with no script or parse errors.
- Static check confirmed `ch1_prologue` step 5 (`sheet_arrel_elia_duo.png` + Elia dialogue) is covered by the new suppression rule.
- `git diff --check -- scripts/ui/vn_scene.gd` passed; only normal CRLF working-copy warnings appeared.

## S122 - 2026-06-19 (VN cinematic graphics polish)

### Done
- Upgraded `VNScene` presentation layers so existing CG/portrait art reads more cinematic without needing new assets.
- Added a warm lower focus glow behind the dialogue area to give CG scenes more depth and keep the eye near the active text area.
- Added a full-screen radial vignette over CG layers so scene edges feel framed instead of flat.
- Added speaker-colored portrait rim frames:
  - Arrel, Elia, Sable, Malet/Mallet, and Tobias receive distinct accent colors,
  - active speaker frames brighten while inactive frames dim,
  - frames hide through the same state path as portraits so the S121 overlap fix stays clean.

### Verification
- `git diff --check -- scripts/ui/vn_scene.gd` passed; only normal CRLF working-copy warnings appeared.
- Godot 4.6.2 headless `res://scenes/ui/vn_scene.tscn` load passed.
- Godot 4.6.2 headless project boot passed with no script or parse errors.

## S123 - 2026-06-19 (generated story CG expansion)

### Done
- Generated and integrated 5 new GPT-image story CGs under `assets/cg/generated/`:
  - `story_ch3_waystation_blank_book.png`
  - `story_ch4_drift_anchor.png`
  - `story_ch8_memory_forest_remnant.png`
  - `story_ch9_colorless_compass.png`
  - `story_ch10_bl07_core_choice.png`
- Connected the new CGs to high-impact story moments:
  - Ch3 Blank Book discovery,
  - Ch4 ash-rain anchoring,
  - Ch8 Memory Forest remnant encounter,
  - Ch9 Colorless Waste / Memory Compass discovery,
  - Ch10 BL-07 core and seal-decision beat.
- Added the new generated story CGs to the Pause Menu artbook list so they can be reviewed outside the dialogue flow.
- Hardened `VNScene` composition rules so full-scene generated story CGs clear both side portraits, preventing the earlier character-illustration overlap issue from returning on the new CGs.

### Verification
- Visually inspected all 5 generated story CGs after copying them into the project.
- UTF-8 JSON parse passed for all dialogue files under `data/` and `data/vn_scenes/`.
- `git diff --check` passed for the edited dialogue data, `pause_menu.gd`, `vn_scene.gd`, and generated CG paths; only normal CRLF working-copy warnings appeared.
- Godot 4.6.2 console executable was found and version-checked successfully, but both headless project boot and `--check-only --script` crashed immediately with engine-level signal 11 before script output. Runtime scene validation is therefore blocked in this shell until Godot can start cleanly again.

## S124 - 2026-06-20 (generated UI/UX art overhaul)

### Done
- Generated and integrated 5 new GPT-image UI/UX backdrop PNGs under `assets/cg/generated/`:
  - `ui_title_memoria_premium.png`
  - `ui_pause_archive_backdrop.png`
  - `ui_memory_archive_backdrop.png`
  - `ui_story_journal_backdrop.png`
  - `ui_memory_shop_backdrop.png`
- Reworked the title screen from image-baked invisible hit targets into readable in-engine UI:
  - new text-free generated title background,
  - visible MEMORIA title/subtitle/tagline,
  - visible New Game / Continue / Options / Quit buttons,
  - clearer hover/focus/disabled button states.
- Added generated backdrops to major menu surfaces:
  - PauseMenu now uses the archive desk / Memory Compass backdrop and moves the menu panel right to reveal the art,
  - MemoryUI now uses the Blank Book / memory-shard archive backdrop,
  - StoryJournal now uses the loss-chronicle journal backdrop,
  - MemoryShop now uses the Verdan market counter backdrop and leaves the merchant-side art visible.
- Registered the 5 new UI backdrops in the PauseMenu Artbook alongside the generated story CGs.

### Verification
- Visually inspected all 5 generated UI backdrops after copying them into the project.
- Verified all new UI PNG files have valid PNG signatures.
- `git diff --check -- scenes/main/main.gd scripts/ui/pause_menu.gd scripts/ui/memory_ui.gd scripts/ui/story_journal.gd scripts/ui/memory_shop.gd assets/cg/generated` passed; only normal CRLF working-copy warnings appeared.
- UTF-8 JSON parse still passed for all dialogue files under `data/` and `data/vn_scenes/`.
- Godot 4.6.2 console executable still reports the correct version, but headless project boot and `--check-only --script scenes/main/main.gd` continue to crash immediately with engine-level signal 11 before script output. Runtime visual validation and import generation for the newest UI PNGs are blocked in this shell until Godot starts cleanly again.

## S125 - 2026-06-20 (fantasy font and dialogue CG pass)

### Done
- Generated and integrated 4 new GPT-image dialogue CGs under `assets/cg/generated/`:
  - `dialogue_ch1_elia_finds_arrel.png`
  - `dialogue_ch2_malet_memory_trade.png`
  - `dialogue_ch5_elia_cliff_choice.png`
  - `dialogue_ch7_sable_echo_shell.png`
- Connected the new dialogue CGs to character-heavy story beats:
  - Elia finding Arrel after the first burn,
  - Malet naming the price of passage in Ch2,
  - Arrel and Elia facing the split-or-stay cliff choice,
  - Sable revealing the Echo Shell before BL-07.
- Expanded `VNScene` portrait suppression so generated dialogue CGs behave like full-scene story CGs and clear side portraits, preventing character art from overlapping the illustration.
- Strengthened the fantasy typography pass:
  - `assets/fonts/theme.tres` now prioritizes serif/fantasy-friendly font chains,
  - `UITheme` now exposes title/body font helpers,
  - title screen, VN speaker names, dialogue speaker names, and dialogue body text now receive the new font styling directly.
- Registered the 4 dialogue CGs in the PauseMenu Artbook for review outside the story flow.

### Verification
- Visually inspected the 4 generated dialogue CGs after copying them into the project.
- Verified all 4 new dialogue PNG files have valid PNG signatures.
- UTF-8 JSON parse passed for all files under `data/`.
- `git diff --check` passed for the edited dialogue data, font/theme files, UI scripts, session log, and generated dialogue CG paths; only normal CRLF working-copy warnings appeared.
- Godot 4.6.2 console executable still reports the correct version, but headless project boot and `--check-only --script scripts/utils/ui_theme.gd` continue to crash immediately with engine-level signal 11 before script output. Runtime visual validation and import generation for the newest dialogue PNGs are blocked in this shell until Godot starts cleanly again.

## S126 - 2026-06-21 (large gameplay patch: combat resonance and tactical objectives)

### Done
- Added the Combat Resonance momentum system to `BattleManager`:
  - tactical play now builds Resonance during a fight,
  - weakness pressure, BREAK, clean attacks, combo continuity, memory burns, residue burns, Guard Focus, stance shifts, Elia techniques, and companion support all feed the meter,
  - higher Resonance ranks slightly increase player damage and award post-battle Grains bonuses.
- Expanded tactical objectives from a small pool into a broader combat-challenge layer:
  - existing objectives remain: BREAK, scan/analyze, clean hands, combo x3,
  - new objectives include swift finish, no-item victory, stance shifting, resonance climb, echo weave, limit release, and companion coordination,
  - objectives can now reward Grains, items, and extra HP recovery.
- Upgraded battle result rewards:
  - victory rewards now include objective heal rewards and Resonance bonus Grains,
  - structured reward data includes objective heal, momentum rank, momentum label, and momentum bonus.
- Updated battle UI:
  - tactical objective panel now shows live Resonance rank/percent,
  - Resonance color changes by rank,
  - victory reward panel now shows Resonance bonus as its own line.
- Added progression hooks:
  - new play stats: `highest_momentum_rank`, `objectives_completed`, `momentum_surges`,
  - PauseMenu statistics now surface those values,
  - new achievements: `Field Tactician` and `Overbright`,
  - new tutorial hint explains Resonance the first time it matters.

### Verification
- UTF-8 JSON parse passed for all files under `data/`.
- `git diff --check` passed for the edited gameplay/UI/stat files; only normal CRLF working-copy warnings appeared.
- Godot 4.6.2 headless project boot passed with no script or parse errors. Shutdown still reports normal ObjectDB/resource cleanup noise.
- Temporary battle smoke scene verified:
  - autoload project boot,
  - battle start,
  - tactical objective generation,
  - stance switching,
  - player attack,
  - Resonance increased to `Kindled 25.0`.
- Temporary smoke files were removed after validation.

## S127 - 2026-06-21 (GPT-image interface overhaul)

### Done
- Generated and integrated 6 new text-free GPT-image interface assets under `assets/cg/generated/`:
  - `ui_dialogue_ornate_frame.png`
  - `ui_battle_tactical_plate.png`
  - `ui_battle_victory_reward_panel.png`
  - `ui_burn_preview_ritual_panel.png`
  - `ui_options_observatory_backdrop.png`
  - `ui_game_over_void_backdrop.png`
- Upgraded the dialogue UI with a generated lower-third ornate frame behind the existing portrait/text layout, preserving readable in-engine text and avoiding character-illustration overlap.
- Upgraded battle interface surfaces:
  - tactical objective HUD now has a generated brass/obsidian backplate,
  - victory rewards now animate over a generated reward-frame layer,
  - memory-burn confirmation now uses a ritual-frame layer behind the cost/risk text and buttons.
- Upgraded menu/recovery surfaces:
  - OptionsMenu now opens over a generated archive-observatory backdrop,
  - GameOver now uses a generated void/memory-shatter backdrop with a readable center panel.
- Registered the 6 new interface assets in the PauseMenu Artbook as generated UI frames/backdrops.

### Verification
- Visually inspected the generated dialogue frame and burn-preview ritual frame after copying them into the project.
- Verified all 6 new interface PNG files have valid PNG signatures.
- `git diff --check -- scripts/ui/dialogue_box.gd scenes/ui/options_menu.gd scenes/ui/game_over.gd scenes/battle/battle_scene.gd scripts/ui/pause_menu.gd assets/cg/generated` passed; only normal CRLF working-copy warnings appeared.
- Godot 4.6.2 headless project boot passed with no script or parse errors. Shutdown still reports the known ObjectDB/resource cleanup noise.
- Representative scene loads passed:
  - `res://scenes/ui/game_over.tscn`
  - `res://scenes/maps/verdan_market.tscn`
  - `res://scenes/battle/battle_scene.tscn`

## S128 - 2026-06-21 (extra interface finish: command ribbon and pause slab)

### Done
- Generated and integrated 2 additional text-free GPT-image interface assets under `assets/cg/generated/`:
  - `ui_battle_command_ribbon.png`
  - `ui_pause_control_slab.png`
- Upgraded the battle command bar:
  - added a generated wide command ribbon behind the bottom action buttons,
  - synchronized the ribbon visibility with `action_container`,
  - added a subtle breathing alpha to the ribbon while commands are available,
  - tightened action button colors, borders, outlines, and hover scale feedback.
- Upgraded the pause menu:
  - added a generated vertical control slab behind the menu stack,
  - animated the slab together with the existing slide-in/slide-out menu panel,
  - lowered the existing panel opacity so the generated ornament reads through without hurting text readability.
- Registered the 2 new interface assets in the PauseMenu Artbook.

### Verification
- Visually inspected the new battle command ribbon and pause control slab after copying them into the project.
- Verified both new PNG files have valid PNG signatures.
- `git diff --check -- scenes/battle/battle_scene.gd scripts/ui/pause_menu.gd assets/cg/generated/ui_battle_command_ribbon.png assets/cg/generated/ui_pause_control_slab.png` passed; only normal CRLF working-copy warnings appeared.
- Godot 4.6.2 headless project boot passed with no script or parse errors. Shutdown still reports the known ObjectDB/resource cleanup noise.
- Representative scene loads passed:
  - `res://scenes/battle/battle_scene.tscn`
  - `res://scenes/maps/verdan_market.tscn`

## S129 - 2026-06-21 (extra interface finish: exploration HUD, toast, tutorial banner)

### Done
- Generated and integrated 3 additional text-free GPT-image interface assets under `assets/cg/generated/`:
  - `ui_exploration_hud_plate.png`
  - `ui_notification_toast_frame.png`
  - `ui_tutorial_hint_banner.png`
- Upgraded the exploration HUD:
  - added a generated top-left HUD plate behind HP, chapter, memory, grains, items, pulse, equipment, and quest text,
  - synchronized the generated plate with exploration-only visibility,
  - matched the plate to the existing slide-in animation,
  - lowered the old panel opacity so the generated frame reads through while preserving text readability.
- Upgraded notification toasts:
  - added a generated bottom-center toast frame behind save/load, memory, and warning messages,
  - matched the generated frame to the existing slide/fade toast animation,
  - softened the old flat panel style so the new frame carries the visual weight.
- Upgraded tutorial hints:
  - added a generated top-center hint banner behind first-time contextual tutorial text,
  - animated the banner with the existing hint panel,
  - lowered the old hint panel opacity for a more integrated fantasy UI look.
- Registered all 3 new interface assets in the PauseMenu Artbook.

### Verification
- Visually inspected the new exploration HUD plate and tutorial hint banner after copying them into the project.
- Verified all 3 new PNG files have valid PNG signatures.
- `git diff --check -- scripts/ui/exploration_hud.gd scripts/ui/notification_toast.gd scripts/ui/tutorial_hints.gd scripts/ui/pause_menu.gd assets/cg/generated/ui_exploration_hud_plate.png assets/cg/generated/ui_notification_toast_frame.png assets/cg/generated/ui_tutorial_hint_banner.png` passed; only normal CRLF working-copy warnings appeared.
- Godot 4.6.2 headless project boot passed with no script or parse errors. Shutdown still reports the known ObjectDB/resource cleanup noise.
- Representative scene load passed:
  - `res://scenes/maps/verdan_market.tscn`

## S130 - 2026-06-21 (GPT-image memory-burn cut-ins)

### Done
- Generated and integrated 2 additional text-free GPT-image battle cut-ins under `assets/cg/generated/`:
  - `memory_burn_compass.png`
  - `memory_burn_void_walker.png`
- Connected 6 memory-burn illustrations to the battle flow:
  - `identity_first_sword` -> `memory_burn_first_sword.png`
  - `daily_campfire_song` -> `memory_burn_elia_song.png`
  - `rel_hand_reaching` -> `memory_burn_reaching_hand.png`
  - `core_name_origin` -> `memory_burn_arrel_name.png`
  - `identity_compass` -> `memory_burn_compass.png`
  - `identity_void_walker` -> `memory_burn_void_walker.png`
- Added keyword fallback selection so future/synthesized memories can still pick a fitting burn cut-in when their title includes sword, song, hand, name, compass, or void cues.
- Extended the existing battle action cut-in layer with an optional hold duration, then reused it for memory-burn cut-ins before the existing burn VFX and damage execution.
- Registered all 6 memory-burn cut-ins in the PauseMenu Artbook.

### Verification
- Visually inspected the new `memory_burn_compass.png` and `memory_burn_void_walker.png` assets after copying them into the project.
- Verified all 6 memory-burn PNG files have valid PNG signatures.
- `git diff --check -- scenes/battle/battle_scene.gd scripts/ui/pause_menu.gd assets/cg/generated/memory_burn_compass.png assets/cg/generated/memory_burn_void_walker.png` passed; only normal CRLF working-copy warnings appeared.
- Godot 4.6.2 headless project boot passed with no script or parse errors. Shutdown still reports the known ObjectDB/resource cleanup noise.
- Representative scene load passed:
  - `res://scenes/battle/battle_scene.tscn`

## S131 - 2026-06-21 (opening VN readability and first-map clarity)

### Done
- Fixed the opening immersion break where production/reference sheet CGs appeared during early dialogue:
  - replaced Chapter 1 `sheet_arrel_profile`, `sheet_arrel_elia_duo`, `sheet_elia_profile`, and `sheet_elia_memory_restoration` references with story-appropriate CGs in `ch1_prologue`, `ch1_void_beast`, `ch1_forest_walk`, and `chapter1_dialogue`.
  - changed the `ch1_stump2` fallback alias to use the generated Elia/Arrel story illustration instead of a reference sheet.
- Reduced overbearing VN presentation layers:
  - lowered the lower wash, focus glow, and vignette defaults,
  - added CG-specific presentation profiles so text plates, generated story CGs, and reference fallback art receive different overlay strength,
  - hid portraits when a full-scene generated dialogue/story CG is active so character art no longer stacks awkwardly over the illustration.
- Hardened VN close behavior:
  - `SceneFlow` now asks `VNScene` to clear its visual layers before queue-free,
  - `VNScene.prepare_for_close()` hides CG, portrait, text, choice, glitch, grain, and page-turn overlays immediately,
  - disconnected VN signals on exit to avoid stale callbacks after scene transitions.
- Improved first playable Rim Forest readability:
  - reduced stacked vignette, fog, depth gradient, and premium lens darkness,
  - raised ambient lighting and player fog-light radius/energy,
  - kept the forest mood while making the opening game screen easier to read after dialogue.

### Verification
- Confirmed the targeted Chapter 1 files no longer reference the immersion-breaking sheet CGs.
- UTF-8 JSON parse passed for all edited Chapter 1 dialogue/VN files.
- `git diff --check` passed for all edited files; only normal CRLF working-copy warnings appeared.
- Godot 4.6.2 headless project boot passed with no script or parse errors.
- Representative scene loads passed:
  - `res://scenes/main/vn_host.tscn`
  - `res://scenes/maps/rim_forest.tscn`
- Godot still reports the existing headless shutdown ObjectDB/resource cleanup noise, with no parse/script failure tied to this patch.

## S132 - 2026-06-22 (large graphics pass: story CG cleanup and map lens rebalance)

### Done
- Removed remaining runtime `res://assets/cg/game_image/sheet_*` CG usage from dialogue and major presentation fallbacks:
  - replaced late Chapter 2, 4, 5, 6, 7, 8, 10, and Epilogue sheet-CG beats with generated story/dialogue/memory illustrations.
  - updated `DialogueBox` speaker-stage fallbacks for Arrel and Elia to use portrait assets instead of sheet-derived CG plates.
  - updated battle stage/pre-attack art to use `memory_burn_first_sword.png` instead of the old battle-ready sheet plate.
  - updated ending/gallery/chapter-title fallbacks to use generated memory/world-rewrite art where appropriate.
- Strengthened generated illustration coverage across the story:
  - Ch2 Elia concern now reuses the Malet memory-trade illustration.
  - Ch4 night/anchor beats now use `story_ch4_drift_anchor.png`; home-flashback uses `memory_burn_reaching_hand.png`.
  - Ch5 separation/stay/reunion beats now use `dialogue_ch5_elia_cliff_choice.png`; first-sword recall uses `memory_burn_first_sword.png`.
  - Ch6 Seam garden/night Elia beats now use Seam/world-rewrite art.
  - Ch7 trial/void-edge beats now use memory-burn and Sable echo-shell art.
  - Ch8 anchor/flashback beats now use Elia-anchor and memory-forest art.
  - Ch10/epilogue BL-07 beats now use `story_ch10_bl07_core_choice.png`.
- Rebalanced map graphics across major maps:
  - lowered stacked vignette/lens darkness on Belt Waystation, Colorless Waste, BL-07 Void, Crumbling Coast, Drift Shelter, Forgotten Forest, Seam Outskirts, The Seam, and Verdan Market.
  - raised ambient lighting enough for gameplay readability while preserving each biome's palette.
  - reduced atmospheric illustration opacity where it was competing with tile readability.
- Refreshed Artbook entries so previously sheet-derived showcase slots now point at generated memory/dialogue art.

### Verification
- Confirmed no runtime `res://assets/cg/game_image/sheet_*` references remain in `data/`, `scripts/`, or `scenes/`.
- Confirmed all referenced `res://assets/...` paths resolve to existing files.
- UTF-8 JSON parse passed for all `data/**/*.json` files.
- `git diff --check` passed for the edited graphics/data files; only normal CRLF working-copy warnings appeared.
- Godot 4.6.2 headless project boot passed with no script or parse errors.
- Representative scene load sweep passed:
  - `res://scenes/main/vn_host.tscn`
  - `res://scenes/battle/battle_scene.tscn`
  - `res://scenes/maps/rim_forest.tscn`
  - `res://scenes/maps/verdan_market.tscn`
  - `res://scenes/maps/belt_waystation.tscn`
  - `res://scenes/maps/drift_shelter.tscn`
  - `res://scenes/maps/crumbling_coast.tscn`
  - `res://scenes/maps/the_seam.tscn`
  - `res://scenes/maps/seam_outskirts.tscn`
  - `res://scenes/maps/forgotten_forest.tscn`
  - `res://scenes/maps/colorless_waste.tscn`
  - `res://scenes/maps/bl07_void.tscn`
- Godot still reports the existing headless shutdown ObjectDB/resource cleanup noise and a few pre-existing anchor layout warnings on late-game maps, with no script or parse failure tied to this patch.

## S133 - 2026-06-22 (illustration overlap fix and Korean opening patch)

### Done
- Fixed generated illustration overlap during gameplay:
  - forced `MapEffects.add_illustration_atmosphere()` onto a background-only CanvasLayer (`layer <= -20`),
  - lowered atmospheric illustration opacity and removed the full-screen shade/readability overlays that were competing with the playable map,
  - changed `WorldRewriteDirector` memory/rewrite illustration flashes from full-screen overlays into a small, short-lived right-side echo card below HUD/UI layers.
- Added Korean-first localization behavior:
  - default locale is now Korean for this local build,
  - existing settings without the new Korean patch marker are migrated to `ko`,
  - dialogue/VN UI now resolves speaker names, text, narration, system logs, choices, choice hints, effects, and memory-distorted lines through localized fields.
- Korean-patched the immediate Chapter 1 opening flow:
  - `ch1_prologue`,
  - `ch1_forest_walk`,
  - `ch1_void_beast`,
  - the matching early `chapter1_dialogue` blocks for the legacy DialogueManager route.

### Verification
- UTF-8 JSON parse passed for all `data/**/*.json` files.
- `git diff --check` passed for the edited overlap/localization files; only normal CRLF working-copy warnings appeared.
- Godot 4.6.2 headless project boot passed with no script or parse errors.
- Representative scene loads passed:
  - `res://scenes/main/vn_host.tscn`
  - `res://scenes/maps/rim_forest.tscn`
  - `res://scenes/maps/verdan_market.tscn`
  - `res://scenes/battle/battle_scene.tscn`
- Godot still reports the existing headless shutdown ObjectDB/resource cleanup noise, plus the known CanvasItem RID cleanup warning on map loads, with no parse/script failure tied to this patch.

## S134 - 2026-06-23 (gameplay stability sweep and combat flow upgrade)

### Done
- Hardened battle entry from VN/SceneFlow:
  - `goto_battle` now accepts string enemy IDs, dictionaries, or full `BattleManager.Enemy` objects,
  - added built-in enemy presets for early/story combat IDs so VN-driven battles do not fail silently,
  - `SceneFlow` now hands off to the battle scene after starting the requested battle.
- Fixed combat-result flow blockers:
  - corrected Kairos and Seam trial callbacks so they check `BattleState.VICTORY` instead of treating enum results like booleans,
  - stopped void-corruption turn ticks from continuing into ally/enemy turns after they already caused victory/defeat,
  - connected turn-limit corruption defeat to the new Last Stand safety instead of hard-failing immediately.
- Added a gameplay upgrade: Last Stand Resonance.
  - once per battle, lethal or critical-low HP pressure can leave Arrel at 1 HP,
  - grants a guarded next hit, Limit gain, resonance/momentum gain, toast feedback, and survivor achievement progress,
  - designed as a dramatic anti-frustration system rather than a free heal loop.
- Fixed VN choice softlock risk:
  - if all conditional choices are filtered out, the VN now shows a localized "continue" fallback instead of leaving the player stuck.
- Expanded Korean-first UX coverage:
  - battle objective titles/descriptions and reward labels,
  - resonance/momentum labels,
  - exploration HUD map names, chapter text, memory/grains/items/pulse labels, location cards, and main quest fallbacks.

### Verification
- `git diff --check` passed; only normal CRLF working-copy warnings appeared.
- Godot 4.6.2 headless project boot passed with no script or parse errors.
- Representative scene loads passed:
  - `res://scenes/main/vn_host.tscn`
  - `res://scenes/maps/rim_forest.tscn`
  - `res://scenes/battle/battle_scene.tscn`
- Confirmed edited Korean strings remain valid UTF-8.
- Search pass found no remaining battle-ended callback that treats `BattleState` enum results as plain booleans.
- Godot still reports the existing headless shutdown ObjectDB/resource cleanup noise, plus the known CanvasItem RID cleanup warning on map loads, with no parse/script failure tied to this patch.

## S135 - 2026-06-25 (GPT image cinematic integration pass)

### Done
- Generated and integrated 6 new GPT image assets into the project:
  - `cinematic_last_stand_resonance.png` — Last Stand battle cut-in.
  - `chapter_splash_rim_forest.png` — Ch1 opening/VN, Rim Forest chapter card, HUD location art, Rim Forest combat backdrop.
  - `chapter_splash_verdan_market.png` — Ch2 chapter card, HUD location art, Verdan combat backdrop.
  - `chapter_splash_the_seam.png` — Ch6 chapter card, HUD location art, The Seam combat backdrop.
  - `cinematic_kairos_watcher_confrontation.png` — Kairos boss/enemy stage art.
  - `memory_compass_resonance_cinematic.png` — Colorless Waste chapter/HUD art, Kairos battle backdrop, Memory Resonance reward CG.
- Ran a Godot import pass so all 6 generated PNG files have matching `.import` metadata.
- Upgraded battle presentation:
  - added `BattleManager.last_stand_resonance` signal,
  - connected `battle_scene.gd` to play the new Last Stand cut-in, blue flash, screen shake, shield pulse, and layered SFX,
  - updated Kairos image resolution to prefer the new GPT boss confrontation art.
- Upgraded exploration/chapter presentation:
  - `ExplorationHUD.MAP_ART` now uses generated GPT art for Rim Forest, Verdan Market, The Seam, and Colorless Waste.
  - `MapEffects.show_chapter_title()` now uses generated GPT art for chapters 1, 2, 6, and 9.
  - Rim Forest atmosphere and random encounter battle backgrounds now use the new Ch1 splash art.
- Upgraded story/VN presentation:
  - `VNScene` default/fallback forest CG now points to the generated Rim Forest splash.
  - `ch1_prologue` first visual beat and legacy `chapter1_dialogue` forest CG beats now use the new Rim Forest art.
  - generated chapter/cinematic/system images are treated as story CGs for more balanced wash/glow/vignette handling.
- Upgraded Memory Resonance feedback:
  - field resonance now briefly opens the Memory Compass CG with localized caption text before applying the reward.

### Verification
- Confirmed all 6 new generated assets and their `.import` files exist in `assets/cg/generated/`.
- UTF-8 JSON parse passed for all `data/**/*.json` files.
- `git diff --check` passed; only normal CRLF working-copy warnings appeared.
- Godot 4.6.2 headless project boot passed with no script or parse errors.
- Godot import pass successfully scanned and reimported the 6 new PNG assets.
- Representative scene loads passed:
  - `res://scenes/main/vn_host.tscn`
  - `res://scenes/battle/battle_scene.tscn`
  - `res://scenes/maps/rim_forest.tscn`
  - `res://scenes/maps/verdan_market.tscn`
  - `res://scenes/maps/the_seam.tscn`
  - `res://scenes/maps/colorless_waste.tscn`
- Existing headless shutdown ObjectDB/resource cleanup noise remains. The Godot import run also printed pre-existing VFX plugin/autoload warnings and shaderV duplicate UID warnings; the asset import itself completed and subsequent headless boot passed.

## S136 - 2026-06-26 (GPT image chapter splash expansion pass)

### Done
- Generated and integrated 6 new GPT image chapter splash assets:
  - `chapter_splash_belt_waystation.png` - Ch3 Belt Waystation art.
  - `chapter_splash_drift_shelter.png` - Ch4 Drift Shelter art.
  - `chapter_splash_crumbling_coast.png` - Ch5 Crumbling Coast art.
  - `chapter_splash_seam_outskirts.png` - Ch7 Seam Outskirts / threshold art.
  - `chapter_splash_forgotten_forest.png` - Ch8 Forgotten Forest art.
  - `chapter_splash_bl07_void.png` - Ch10 BL-07 Void core art.
- Ran a Godot import pass so all 6 new generated PNG files have matching `.import` metadata.
- Expanded chapter and exploration presentation:
  - `MapEffects.show_chapter_title()` now uses generated GPT art for chapters 3, 4, 5, 7, 8, and 10.
  - `ExplorationHUD.MAP_ART` now uses generated GPT art for Belt Waystation, Drift Shelter, Crumbling Coast, Seam Outskirts, Forgotten Forest, and BL-07 Void.
  - map atmosphere plates for those regions now use the matching generated art at low opacity so they stay behind gameplay.
- Expanded battle presentation:
  - battle scene return-map background resolution now covers the new generated chapter splash set.
  - The Seam, Crumbling Coast, and BL-07 explicit battle starts/random encounters now prefer generated art instead of reused generic environment plates.
  - Shade Sentinel now uses the generated The Seam backdrop through its preset.
- Expanded story/VN and gallery coverage:
  - late Ch2, Ch3, Ch5, Ch6, Ch7, Ch10, epilogue, and selected VN transition beats now use the generated chapter art where the old image was only acting as a generic environment plate.
  - ending gallery images for Preservation, Seam, and Hollow now point to generated chapter art.
  - PauseMenu artbook entries now expose the new generated chapter splash set.

### Verification
- Confirmed all 6 new generated assets and their `.import` files exist in `assets/cg/generated/`.
- Confirmed all referenced `res://assets/...` paths in `data/`, `scripts/`, and `scenes/` resolve to existing files.
- UTF-8 JSON parse passed for all `data/**/*.json` files.
- `git diff --check` passed; only normal CRLF working-copy warnings appeared.
- Godot 4.6.2 import pass scanned and reimported the 6 new PNG assets.
- Godot 4.6.2 headless project boot passed with no script or parse errors.
- Representative scene load script passed:
  - `res://scenes/main/vn_host.tscn`
  - `res://scenes/battle/battle_scene.tscn`
  - `res://scenes/maps/belt_waystation.tscn`
  - `res://scenes/maps/drift_shelter.tscn`
  - `res://scenes/maps/crumbling_coast.tscn`
  - `res://scenes/maps/the_seam.tscn`
  - `res://scenes/maps/seam_outskirts.tscn`
  - `res://scenes/maps/forgotten_forest.tscn`
  - `res://scenes/maps/colorless_waste.tscn`
  - `res://scenes/maps/bl07_void.tscn`
- Existing headless shutdown ObjectDB/resource cleanup noise remains. The Godot import run also printed the pre-existing VFX plugin/autoload and ShaderV duplicate UID warnings; import and subsequent boot/scene-load checks passed.

## S137 - 2026-06-27 (GPT image UI/UX archive interface pass)

### Done
- Generated and integrated 4 new text-free GPT-image interface backdrops under `assets/cg/generated/`:
  - `ui_codex_archive_backdrop.png` - split bestiary / memory-record archive environment.
  - `ui_memory_constellation_backdrop.png` - mnemonic observatory with subdued orbital guides.
  - `ui_achievements_chronicle_backdrop.png` - memorial ledger wall for achievement records.
  - `ui_ending_gallery_backdrop.png` - six-niche ruined reliquary for branching endings.
- Upgraded Codex presentation and information hierarchy:
  - added the generated archive backdrop and a lighter translucent content shell,
  - added localized context copy plus live creature/memory record counts,
  - added a restrained fade/slide entrance and title typography treatment.
- Upgraded Memory Constellation UX:
  - replaced the flat fill with the generated observatory backdrop,
  - localized the title, subtitle, close action, and legend,
  - made grade radii adapt to the actual 1280x720 canvas so outer nodes remain on-screen,
  - staggered grade phases so sparse memory sets no longer collapse into one vertical line,
  - fixed the bottom legend anchor and styled the close control consistently.
- Upgraded Achievements and Ending Gallery:
  - layered generated archive art behind translucent functional panels,
  - strengthened titles, subtitles, progress context, card boundaries, and monochrome milestone glyphs,
  - expanded ending cards to align with the six generated gallery niches,
  - added shared modal entrance motion for a more intentional screen transition.
- Registered all 4 new UI backdrops in the PauseMenu Artbook.

### Verification
- Visually captured and inspected Codex, Memory Constellation, Achievements, and Ending Gallery at the project viewport size (1280x720); generated art, real text, scroll areas, cards, and node graph remained legible.
- Ran a complete Godot `--import` pass; all 4 PNG files received matching `.import` metadata.
- `git diff --check` passed for the edited UI scripts and session log; only normal CRLF working-copy warnings appeared.
- Godot 4.6.2 headless project boot passed with no script or parse errors.
- Representative scene loads passed for `res://scenes/maps/verdan_market.tscn` and `res://scenes/main/vn_host.tscn`.
- Existing headless shutdown ObjectDB/resource cleanup noise and pre-existing VFX/ShaderV plugin warnings remain unchanged.

## S138 - 2026-06-27 (GPT image character battle cinematic pass)

### Done
- Used the built-in GPT image generator with existing character sheets, portraits, chapter art, and combat CGs as visual references to create 7 new text-free 16:9 battle cinematics:
  - `cinematic_arrel_memory_cascade.png` - Arrel's Memory Cascade ultimate.
  - `cinematic_elia_anchor_pulse.png` - Elia repairing fractured memory geometry.
  - `cinematic_sable_echo_strike.png` - Sable's shadow-crossing support action.
  - `cinematic_tobias_record_ward.png` - Tobias converting records into a battlefield ward.
  - `cinematic_void_beast_memory_devour.png` - Void Beast tearing luminous identity fragments from the field.
  - `cinematic_shade_sentinel_phase2.png` - Shade Sentinel's second-crown phase transformation.
  - `cinematic_kairos_authority_edit.png` - Kairos calmly cutting and rearranging recorded reality.
- Expanded the reusable battle cut-in layer instead of adding a parallel cinematic system:
  - Memory Cascade now receives its own high-opacity ultimate cut-in before the existing chromatic burst and impact VFX.
  - all 4 Elia techniques map to context-appropriate cinematics through `ally_action`.
  - Sable and Tobias support actions now trigger character-specific cut-ins and action labels.
  - Void Beast attacks use the new memory-devour art; Shade Sentinel and Kairos use unique phase-two plates.
  - action cut-in tweens now continue during the existing paused boss phase-transition beat.
- Promoted the new Void Beast and Shade Sentinel art into their encounter presets and name-based fallback resolver.
- Added all 7 cinematics to the PauseMenu Artbook with role-specific descriptions.

### Verification
- Ran Godot 4.6.2 `--import`; all 7 PNG files were scanned, reimported, and received matching `.import` metadata.
- `git diff --check` passed; only normal CRLF working-copy warnings appeared.
- Godot 4.6.2 headless project boot passed with no script or parse errors.
- Direct scene loads passed for `res://scenes/battle/battle_scene.tscn` and `res://scenes/maps/verdan_market.tscn`.
- A temporary runtime harness started a Shade Sentinel battle and exercised Elia, Sable, Tobias, enemy attack, phase-two, and Memory Cascade cut-in paths without runtime errors; the harness was removed afterward.
- Existing forced headless shutdown ObjectDB/resource cleanup noise remains. The import pass also printed the pre-existing VFX plugin/autoload and ShaderV duplicate UID warnings; asset import and subsequent runtime checks passed.

## S139 - 2026-06-27 (GPT image Act I first-impression overhaul)

### Done
- Used the built-in GPT image generator with the current Arrel/Elia turnarounds and established Rim Forest/battle art as references to create 9 new text-free 16:9 Act I story images:
  - `story_ch1_opening_aftermath.png` - Arrel over the dissolving remains of the opening Void Beast.
  - `story_ch1_elia_reunion.png` - the lantern-lit reunion with both protagonists' current designs.
  - `story_ch1_ash_rain_touch.png` - the first ash flake dissolving against Arrel's cheek.
  - `story_ch1_camp_humming.png` - the fireless night camp and Elia's broken memory-song thread.
  - `story_ch1_twisted_forest_path.png` - the first playable route beneath rib-like roots.
  - `story_ch1_memory_shrine.png` - the petrified stump, cairn, and anonymous residual echoes.
  - `story_ch1_void_beast_emergence.png` - the first boss uncoiling from the canopy.
  - `story_ch1_first_burn_strike.png` - the pale-gold "idea of heat" memory-burn cut.
  - `story_ch1_green_tree_dawn.png` - the Chapter 1 ending reveal of one living tree.
- Rebuilt the first-impression flow from New Game through the end of Chapter 1:
  - New Game now opens directly on the new battle aftermath instead of an empty environment plate.
  - prologue, reunion, Ash Rain, branching camp, dawn, forest-walk, optional shrine/stump, first boss, first burn, and chapter-ending checkpoints now receive scene-specific art.
  - preserved all existing choice targets and progression logic while changing only CG fields and aliases.
- Extended the same art direction into playable presentation:
  - Rim Forest exploration atmosphere and HUD location art now use the new twisted-path plate.
  - Rim Forest combat backgrounds use the same environment plate and the existing S138 Void Beast cinematic.
  - the legacy non-VN Chapter 1 dialogue path received matching opening, reunion, Ash Rain, camp, walking, stump, and shrine art.
- Corrected the most visible early-character inconsistency by anchoring new story CGs to silver-haired Arrel and honey-blonde bob-haired Elia from the current turnaround sheets.
- Registered all 9 Act I images in the PauseMenu Artbook.

### Verification
- UTF-8 JSON parse passed for all 16 `data/**/*.json` files after the CG rewiring.
- Godot 4.6.2 `--import` scanned and reimported all 9 PNG files and generated matching `.import` metadata.
- A temporary runtime harness loaded 9 representative early-game VN checkpoints through the real `SceneFlow` and `VNScene` CG resolver; every resolved path existed and loaded without runtime errors. The harness was removed afterward.
- Godot 4.6.2 headless project boot passed with no script or parse errors.
- Direct scene loads passed for `res://scenes/maps/rim_forest.tscn` and `res://scenes/battle/battle_scene.tscn`.
- `git diff --check` passed; only normal CRLF working-copy warnings appeared.
- Existing forced headless shutdown ObjectDB/resource cleanup noise remains. The import pass also printed the pre-existing VFX plugin/autoload and ShaderV duplicate UID warnings; import and subsequent runtime checks passed.

## S140 - 2026-06-27 (GPT image typography and archive interface refinement)

### Done
- Used the built-in GPT image generator and the established S137-S139 blackened-silver / midnight-blue / pale-gold art direction to create 3 text-free chroma-key UI overlays:
  - `ui_vn_memory_frame_overlay.png` - a compact lower-screen dialogue frame with a dedicated speaker tab.
  - `ui_vn_choice_archive_overlay.png` - a central memory-record choice frame with three restrained decision bands.
  - `ui_exploration_archive_overlay.png` - a compact top-left exploration HUD archive frame.
- Converted all three generated assets to transparent PNGs with the imagegen skill's chroma-key helper, preserving soft anti-aliased metal edges without green spill.
- Reworked typography roles across the interface:
  - narrative dialogue, speaker names, and choice prose retain the literary serif stack,
  - controls, hints, continue prompts, HUD data, and floating feedback now use a dedicated sans-serif UI stack,
  - increased VN body size/line spacing and tightened HUD size hierarchy for Korean and English readability.
- Integrated the generated overlays into the live UI:
  - VN dialogue and choice frames swap automatically with dialogue state,
  - speaker-name placement now aligns with the generated frame tab,
  - exploration HUD frame participates in the existing slide-in animation and encloses the widened information panel.
- Preserved the legacy dialogue path while applying the same improved prose spacing, UI prompt font, and serif choice treatment.

### Verification
- Ran a complete Godot 4.6.2 `--import` pass; all 3 PNG files were scanned, reimported, and received matching `.import` metadata.
- Godot 4.6.2 headless project boot passed with no script or parse errors.
- Direct scene loads passed for `res://scenes/main/vn_host.tscn` and `res://scenes/maps/rim_forest.tscn`.
- Captured and visually inspected live 1280x720 renders for Act I dialogue, the first major VN choice, and Rim Forest exploration HUD; text remained legible and generated frame geometry aligned after final speaker-tab/HUD adjustments.
- Existing forced shutdown ObjectDB/resource cleanup noise, CanvasItem RID cleanup warning, and pre-existing VFX/ShaderV import warnings remain unchanged.

## S141 - 2026-06-28 (GPT image Chapter 2 narrative bridge pass)

### Done
- Committed the completed S138-S140 presentation overhaul as `ac6e7d5` (`feat(presentation): overhaul Act I visuals`) before starting the next art pass.
- Audited all current story CG references and identified Chapter 2 as the clearest illustration-density gap after the Act I overhaul.
- Used the built-in GPT image generator with the current Arrel/Elia duo sheet, Malet portrait, Kairos cinematic, Verdan splash, and existing cellar CG as explicit identity/style references to create 6 new text-free 16:9 story images:
  - `story_ch2_verdan_gate.png` - Arrel and Elia stopped at the southern Bureau checkpoint above Verdan.
  - `story_ch2_memory_market.png` - bottled affection, grief, and identity offered across the market stalls.
  - `story_ch2_old_burner.png` - the nameless old man as a quiet mirror of Arrel's possible future.
  - `story_ch2_malet_cellar.png` - the corrected three-character negotiation with current Arrel, Elia, and unhooded Malet designs.
  - `story_ch2_first_sword_extraction.png` - the first-sword memory leaving Arrel as a pale filament.
  - `story_ch2_kairos_warning.png` - Kairos revealed as a cold Bureau projection while Elia grips her cup.
- Rewired the Chapter 2 visual flow so the new art follows the emotional sequence from arrival and market horror through Malet's bargain, extraction, and the four-day Kairos threat.
- Replaced the most visible legacy character-inconsistent Chapter 2 CG usages with the new identity-anchored Malet cellar art.
- Registered all 6 images in the PauseMenu Artbook.

### Verification
- UTF-8 JSON parse passed for all `data/**/*.json` files.
- Confirmed every new `res://assets/...` reference resolves to an existing project file.
- Ran Godot 4.6.2 `--import`; all 6 PNG files were scanned, imported, and received matching `.import` metadata.
- Godot 4.6.2 headless project boot passed with no script or parse errors.
- Direct scene loads passed for `res://scenes/main/vn_host.tscn` and `res://scenes/maps/verdan_market.tscn`.
- Captured and visually inspected live 1280x720 VN renders for the Verdan guard checkpoint and memory-market entry; character focal points remain clear above the S140 dialogue frame.
- Existing forced shutdown ObjectDB/resource cleanup noise, CanvasItem RID cleanup warning, and pre-existing VFX/ShaderV import warnings remain unchanged.

## S142 - 2026-06-28 (GPT image midgame illustration continuity pass)

### Done
- Audited Chapter 3-6 dialogue groups and CG references after the Act I/Chapter 2 passes, prioritizing long text sequences with no illustration and repeated generic environment plates.
- Used the built-in GPT image generator with the current Arrel/Elia duo sheet, Tobias turnaround, Sable cut-in, Kairos cinematic, and matching chapter environments as explicit identity/style references to create 8 new text-free 16:9 story illustrations:
  - `story_ch3_tobias_waystation.png` - Tobias startled at his paper-covered waystation desk.
  - `story_ch3_tobias_joins.png` - the first three-person party image on the Belt road.
  - `story_ch4_reading_deterioration.png` - Arrel discovers the Blank Book is becoming unreadable to him.
  - `story_ch4_night_counting_losses.png` - Elia admits she has counted eleven involuntary micro-memory losses.
  - `story_ch5_kairos_ridge_sighting.png` - Kairos watches from the distant coast ridge without giving chase.
  - `story_ch5_seam_first_light.png` - the party meets the current short-silver-haired Sable as color returns at The Seam.
  - `story_ch6_sable_briefing.png` - Sable maps BL-07 and the Shade Sentinel in her workshop.
  - `story_ch6_stars_forgetting.png` - Arrel and Elia sit above The Seam while the sky loses its stars.
- Rewired Chapter 3-6 story beats so the illustrations form a continuous visual arc: party formation, cognitive deterioration, quiet loss, Kairos surveillance, sanctuary arrival, mission briefing, and the BL-07 eve.
- Updated Sable's outdated Chapter 5 entrance description from short dark hair to short silver hair to match the newer battle/story art direction.
- Preserved solo Seam arrival and hidden-garden branches on their original neutral artwork instead of leaking group-specific imagery into them.
- Registered all 8 illustrations in the PauseMenu Artbook.

### Verification
- UTF-8 JSON parse passed for all `data/**/*.json` files.
- Confirmed every referenced `res://assets/...` path resolves to an existing project file.
- Ran Godot 4.6.2 `--import`; all 8 PNG files were scanned, imported, and received matching `.import` metadata.
- Godot 4.6.2 headless project boot passed with no script or parse errors.
- Direct scene loads passed for Belt Waystation, Drift Shelter, Crumbling Coast, and The Seam.
- Captured and visually inspected live 1280x720 legacy-dialogue renders for Tobias's introduction and Elia's stars-forgetting conversation; focal subjects remain clear around the dialogue presentation layers.
- Existing forced shutdown ObjectDB/resource cleanup noise, CanvasItem RID cleanup warning, and pre-existing VFX/ShaderV import warnings remain unchanged.

## S143 - 2026-06-28 (GPT image late-game narrative illustration pass)

### Done
- Audited the Chapter 7-10 dialogue groups against the current CG tree and targeted repeated environment plates, generic item art, and unillustrated late-game turning points.
- Used the built-in GPT image generator in `stylized-concept` mode with the latest Arrel/Elia duo, Tobias, short-silver-haired Sable, Kairos, and chapter environment anchors to create 13 new text-free late-game story illustrations:
  - `story_ch7_fading_names_monument.png` - the first expedition team's names dissolving from the Seam Outskirts memorial.
  - `story_ch7_sable_confession.png` - Sable admitting why she deserted Authority Memory Disposal.
  - `story_ch7_echo_shell_whispers.png` - Arrel hearing fragments of consumed lives through the Echo Shell.
  - `story_ch8_forest_crossing.png` - the four-person party entering the memory-parasitic forest.
  - `story_ch8_ghost_child.png` - the child remnant cupping the absence of a forgotten name.
  - `story_ch8_ring_cairn.png` - Tobias discovering BL-07's accelerating consumption rings.
  - `story_ch9_kairos_confrontation.png` - the full party facing Kairos in the Colorless Waste.
  - `story_ch9_first_void_memory.png` - Arrel touching the last thought of the first Void Hole's witness.
  - `story_ch9_bl07_threshold.png` - BL-07 appearing as a door more real than the Waste.
  - `story_ch10_void_echoes.png` - the Void surrounding Arrel and Elia with fragments of almost-memory.
  - `story_ch10_orphan_memories.png` - the party finding crystallized lives kept by BL-07.
  - `story_ch10_seal_complete.png` - Arrel surviving the seal without recognition or identity.
  - `story_ch10_seal_refused.png` - Arrel pulling back from the final burn and choosing borrowed time.
- Reworked the Kairos confrontation once with an identity-preserving correction so Sable retains her current chin-length silver hair instead of drifting toward the older long-haired design.
- Connected all 13 images to their matching Chapter 7-10 dialogue entry points, replacing generic object/environment artwork only where the new scene-specific CG is narratively exact.
- Preserved the existing Chapter 10 core-choice image and pre-seal Void architecture so the new complete/refused ending plates read as distinct consequences rather than replacements for the decision itself.
- Registered the complete late-game set in the PauseMenu Artbook as the Act III visual arc.

### Verification
- UTF-8 JSON parse passed for all `data/**/*.json` files and every referenced `res://assets/...` path resolved.
- Godot 4.6.2 `--import` scanned and imported all 13 PNG files and generated matching `.import` metadata.
- Godot 4.6.2 headless project boot passed without script or parse errors.
- Direct scene loads passed for `forgotten_forest.tscn`, `colorless_waste.tscn`, and `bl07_void.tscn`.
- Captured and visually inspected live 1280x720 standard-renderer dialogue compositions for Chapter 7 Echo Shell and Chapter 10 Void Echoes; focal faces remain clear above the dialogue presentation layers.
- `git diff --check` passed before the final log update; only normal CRLF working-copy warnings appeared.
- Existing forced shutdown ObjectDB/resource cleanup and CanvasItem RID warnings remain unchanged.

## S144 - 2026-06-29 (GPT image ending and epilogue atlas pass)

### Done
- Audited all 129 existing `assets/cg/**/*.png` files and every dialogue group's current `cg` references before selecting new scenes; exact-file duplicate detection also found no duplicate PNG hashes.
- Identified the six ending branches and two optional epilogue conversations as the highest-value non-overlapping gap because they still reused generic Seam, core-choice, placeholder rewrite, or item imagery.
- Used the built-in GPT image generator in `stylized-concept` mode with current Arrel, Elia, Tobias, Sable, ending, and Seam references to create 14 new text-free story illustrations:
  - `ending_zero_burn_canyon_watch.png` - the unnamed man watching colors he cannot name.
  - `ending_zero_burn_trying_name.png` - Arrel trying the lost name as a new choice.
  - `ending_preservation_return.png` - the party returning while BL-07 remains open.
  - `ending_preservation_building_hands.png` - hands that can build instead of burn.
  - `ending_ash_hollow_days.png` - correct answers coming from emotional absence.
  - `ending_ash_sunset_shell.png` - Arrel watching light fade without recognition.
  - `ending_seam_ordinary_moments.png` - small moments surviving the Void's appetite.
  - `ending_seam_impossible_garden.png` - a green shoot becoming the first credible answer.
  - `ending_tobias_night_press.png` - Tobias printing Ring Theory through the night.
  - `ending_tobias_twelve_archivists.png` - twelve copies escaping Authority suppression.
  - `ending_hollow_water.png` - Arrel forgetting the taste and meaning of water.
  - `ending_hollow_name_room.png` - one name echoing through an emptied life.
  - `epilogue_elia_collective_pattern.png` - Elia connecting private burns to shared-history loss.
  - `epilogue_sable_eastern_settlement.png` - Sable pointing toward the next settlement in danger.
- Replaced every generic epilogue CG at its exact narrative beat and added a second visual transition inside each major ending, without replacing the Chapter 10 choice or immediate-consequence art.
- Corrected the Zero Burn prose from outdated silver-haired Elia to her current honey-blonde design.
- Updated all six Ending Gallery thumbnails and descriptions to use branch-specific art and accurate outcome text.
- Added all 14 images to the PauseMenu Artbook.
- Added `ILLUSTRATION_CATALOG.md` with the verified 143-CG baseline, non-overlap rules, S144 scene-to-file mapping, and a role-based path toward the long-term 1,000-image target.

### Verification
- UTF-8 JSON parsing passed for all `data/**/*.json` files and all referenced project asset paths resolved.
- Verified `TOTAL_CG=143`, `NEW_SET=14`, and no exact SHA-256 duplicate PNG files under `assets/cg/`.
- Godot 4.6.2 `--import` scanned and imported all 14 new PNG files and generated matching `.import` metadata.
- Godot 4.6.2 headless project boot passed without script or parse errors.
- Direct scene load passed for `res://scenes/maps/the_seam.tscn`.
- Captured and visually inspected live 1280x720 standard-renderer dialogue compositions for the Zero Burn and Seam endings; subjects and story details remain legible around the narration/dialogue presentation layers.
- `git diff --check` passed before the final log update; only normal CRLF working-copy warnings appeared.
- Existing forced shutdown ObjectDB/resource cleanup and CanvasItem RID warnings remain unchanged.

## S145 - 2026-06-29 (GPT image Chapter 7-9 exploration and choice pass)

### Done
- Continued directly from the ending atlas by re-auditing all 143 existing CG files and the unillustrated Chapter 7-9 dialogue groups, selecting 16 scenes with distinct story, location, cast, and camera purposes.
- Used the built-in GPT image generator in `stylized-concept` mode with the current Arrel, Elia, Tobias, Sable, Kairos, and chapter environment references to create 16 new text-free 16:9 illustrations:
  - `story_ch7_controlled_burn_trial.png` - Sable testing whether Arrel can survive a deliberate burn.
  - `story_ch7_last_field_preparations.png` - the four travelers preparing below the final ridge.
  - `story_ch7_paper_forgetting_ink.png` - paper losing the concept of holding a written mark.
  - `story_ch7_crossing_the_ridgeline.png` - The Seam's last color falling behind the party.
  - `story_ch8_eighteenth_ring.png` - Tobias tracing the forest's organized consumption rings.
  - `story_ch8_whispers_as_bait.png` - false familiar faces gathering in the bark around the real party.
  - `story_ch8_white_stone_shelter.png` - Sable touching the memory-null cairn while the others give her space.
  - `story_ch8_end_of_color.png` - the forest ending abruptly at the Achromatic Waste.
  - `story_ch8_forgotten_moss.png` - matter that remembers neither growth nor decay.
  - `story_ch8_ghost_mother.png` - a mother-shaped remnant cradling an absence.
  - `story_ch8_parasitic_heart.png` - the party confronting the immense breathing knot at the forest's center.
  - `story_ch9_human_chain.png` - the party holding one another against direction-erasing wind.
  - `story_ch9_name_under_pull.png` - BL-07 reeling Arrel inward along a memory tether.
  - `story_ch9_kairos_withdrawal.png` - Kairos retreating through fractured Authority records.
  - `story_ch9_memory_depth_markers.png` - compressed lives forming towering depth markers in the Waste.
  - `story_ch9_final_colorless_view.png` - Arrel and Elia looking back across a world reduced to existence alone.
- Connected every image to its exact Chapter 7, 8, or 9 dialogue beat and registered all 16 as distinct PauseMenu Artbook entries.
- Extended `ILLUSTRATION_CATALOG.md` with the full S145 dialogue-to-asset mapping and updated the verified project baseline from 143 to 159 CG PNG files.

### Verification
- UTF-8 JSON parsing passed for `chapter7_dialogue.json`, `chapter8_dialogue.json`, and `chapter9_dialogue.json`; all 16 new dialogue references and all 16 Artbook references resolve to existing assets.
- Verified `TOTAL_CG=159`, `NEW_SET=16`, and no exact SHA-256 duplicate PNG files under `assets/cg/`.
- Godot 4.6.2 `--import` scanned and imported all 16 new PNG files and generated matching `.import` metadata.
- Godot 4.6.2 headless project boot passed without script or parse errors.
- Direct scene loads passed for `seam_outskirts.tscn`, `forgotten_forest.tscn`, and `colorless_waste.tscn`.
- Captured and visually inspected live 1280x720 standard-renderer dialogue compositions for Chapter 8 `forest_whispers` and Chapter 9 `depth_markers`; subjects, environmental storytelling, and the lower-screen UI safety area remain clear.
- Existing forced shutdown ObjectDB/resource cleanup noise, two pre-existing anchor warnings, and the VFX/ShaderV import warnings remain unchanged.

## S146 - 2026-06-29 (Gameplay + story: The Weave — 7th ending & memory-gated dialogue)

### Done
- Built a cohesive gameplay-and-story upgrade that reinforces the burn-vs-keep core, designed so the new beats have ready illustration slots for the ongoing GPT/codex image work.
- **Memory-driven dialogue engine** (`scripts/systems/dialogue_manager.gd`): brought the NPC dialogue runner to parity with the VN SceneFlow system.
  - Choice/line gating: `requires_memory_intact`, `requires_memory_gone`, `requires_flag`, `requires_not_flag`, `requires_weave`. Failing lines are skipped; failing choices are filtered out, and `select_choice` now operates on the filtered list (`_current_choices`) so indices stay correct.
  - Memory Leverage: `cost_memory` (semantic burn with a "Memory spent" toast).
  - Reward parity: `add_item` / `add_item_count` / `heal_player`, plus data-driven `record_ending` and `set_flag` on lines.
  - Legacy `requires_memory` + `burned_text` text-swap behavior is preserved untouched.
- **The Weave — 7th ending** (a preservation/"true" path that rewards playing against the burn grain):
  - `scripts/systems/memory_manager.gd`: added `is_intact()`, anchor constants (`WEAVE_PRIMARY` = the name, `WEAVE_SECONDARY` = sword/Elia-gesture/anchor-hands/Sable-trust), `intact_anchor_count()`, and `weave_unlocked()` (name intact + fewer than 4 total burns + 3 of 4 secondary anchors intact).
  - `data/chapter10_dialogue.json`: added two `requires_weave` Elia/Arrel hint lines and a gated third seal choice (`seal_weave`), plus the full `seal_weave` resolution group.
  - `data/epilogue_dialogue.json`: added `epilogue_weave` (Sable realizes preservation can seal a hole; colors return to The Seam).
  - `scenes/maps/bl07_void.gd`: `_on_seal_decision_ended` now routes `seal_weave` to a new warm-light `_execute_weave()` / `_on_weave_complete()` that keeps the name.
  - `scenes/maps/the_seam.gd`: highest-priority `epilogue_weave` branch + `record_ending("weave")` / `unlock("ending_weave")`.
  - Registered the ending everywhere: `GameManager.ENDING_DATA["weave"]`, PauseMenu ending gallery id list, `AchievementManager` `ending_weave`, and two `StoryJournal` entries (major event + choice log).
- Emergent tie-in (no new code): accepting Malet's early deal burns `identity_first_sword`, which lowers the anchor count — so taking shortcuts quietly forecloses the Weave path.

### New illustration slots (graceful text-only fallback until generated — for codex)
- `story_ch10_seal_weave.png` — Arrel reaching for every kept memory at once, not just the name.
- `story_ch10_seal_weave_fire.png` — the seal-fire braided from every color the Seam ever bled.
- `story_ch10_seal_weave_after.png` — Arrel intact but thinner, anchored, the seal closed behind him.
- `ending_weave_sealed_gate.png` — the closed BL-07 gate at The Seam.
- `ending_weave_sable_ledger.png` — Sable's pattern ledger with the impossible new column.
- `ending_weave_anchor_hand.png` — the steady weight of the part of him now holding the door shut.
- `ending_weave_colors_return.png` — colors growing back over quiet stone (Ending Gallery thumbnail).

### Verification
- UTF-8 JSON parse passed for all `data/**/*.json`; confirmed `seal_weave`, `epilogue_weave`, and the three-option weave-gated seal choice are present and correctly flagged.
- Godot 4.6.2 `--headless --import` completed; the only error is the pre-existing `addons/vfx_lib/plugin.gd:7` dialog-parent noise.
- Godot 4.6.2 headless boot (`--quit-after 3`): **0** SCRIPT ERROR / Parse Error lines across all edited scripts.
- Verified the dialogue UI feeds the filtered choice index straight into `select_choice`, so gated choices map correctly.
- Missing new CG paths are guarded by `ResourceLoader.exists()` in `dialogue_box.gd`, so the build runs today and the art drops in later.

### Balance note (next pass)
- The Weave gate (<4 burns + 3/4 anchors intact by Ch10) is intentionally a hard "preservation run." Needs a live playthrough to confirm it's reachable without trivializing combat; tune `WEAVE_MAX_BURNS` / anchor threshold if it proves too strict.

## S147 - 2026-06-30 (Code audit + GPT image optional-story and The Weave illustration pass)

### Done
- Reviewed the complete dirty worktree after the S146 Claude Code handoff instead of treating the new Weave implementation as isolated code.
- Fixed a boss-rush cleanup race found during the wider audit:
  - boss-rush progression now listens to `battle_cleanup_finished` rather than the early `battle_ended` signal,
  - final victory can no longer overwrite the menu state with exploration after asynchronous reward cleanup,
  - boss-rush defeat returns cleanly to the title instead of falling through to the normal game-over route.
- Hardened dialogue, VN, save, and memory state boundaries:
  - rejected negative/stale choice indices,
  - clamped VN resume indices,
  - guarded valid-JSON-but-wrong-schema save previews,
  - made memory import tolerate malformed/legacy records and rebuild an empty pool safely,
  - repaired the debug store-stat dialogue count to traverse the real `dialogues` dictionary.
- Reviewed S146's seven-ending registration and fixed the stale four-ending achievement table:
  - added Preservation, Tobias, and Hollow achievement definitions,
  - corrected Zero Burn's achievement ID,
  - updated Every Path to require all seven real endings.
- Removed the repeated Control anchor warning in `WorldRewriteDirector` by using a full-rect anchors-and-offsets preset without assigning a conflicting explicit size.
- Used the built-in GPT image generator with current character/environment references to create 11 clean, text-free 16:9 story CGs:
  - `story_ch1_echo_fragment.png`
  - `story_ch1_ashen_figure_restored.png`
  - `story_ch2_sump_breathing_walls.png`
  - `story_ch2_nervous_trader_ledger.png`
  - `story_ch10_seal_weave.png`
  - `story_ch10_seal_weave_fire.png`
  - `story_ch10_seal_weave_after.png`
  - `ending_weave_sealed_gate.png`
  - `ending_weave_sable_ledger.png`
  - `ending_weave_anchor_hand.png`
  - `ending_weave_colors_return.png`
- Every prompt explicitly excluded film/photo grain, paper/canvas texture, speckle and color noise, dithering, compression artifacts, chromatic aberration, dirty overlays, and oversharpening.
- Rejected and regenerated the first Colors Return image because Sable drifted into a brown-haired male silhouette; the shipped image restores her current chin-length silver-haired identity.
- Connected the four optional-story images to their exact Chapter 1/2 dialogue beats and filled all seven image slots Claude prepared for `seal_weave` / `epilogue_weave`.
- Registered all 11 images in the PauseMenu Artbook and updated `ILLUSTRATION_CATALOG.md` to the verified 170-CG baseline.

### Verification
- Purpose-built boss-rush cleanup test passed for both victory and defeat paths (`BOSS_RUSH_CLEANUP_TEST_OK`).
- Purpose-built Weave reachability test passed after applying real Chapter 3-10 erosion:
  - pristine preservation state exposes three seal choices,
  - one secondary anchor burn still satisfies 3/4,
  - two secondary anchor burns close the Weave path and reduce the choice list to two (`WEAVE_PATH_TEST_OK`).
- UTF-8 JSON parsing passed for all `data/**/*.json`; 728 scanned `res://` references resolved with zero missing files.
- Verified `TOTAL_CG=170`, all 11 new images are RGB 1672x941, all have matching `.import` metadata, and there are zero exact SHA-256 duplicate CG groups.
- Godot 4.6.2 import scanned and imported all new PNGs. The command still exits nonzero only because of the pre-existing VFX Library editor popup/autoload teardown errors and ShaderV duplicate UID warnings.
- Godot 4.6.2 headless project boot passed with zero `SCRIPT ERROR` / `Parse Error` lines.
- Direct scene loads passed for `vn_host.tscn`, `bl07_void.tscn`, and `the_seam.tscn`.
- `git diff --check` passed; only normal CRLF working-copy notices remain.

## S148 - 2026-06-30 (GPT image Chapter 2-6 optional-story and journey illustration pass)

### Done
- Re-audited the Chapter 2-6 dialogue files and the complete `assets/cg/` tree, then filled all 16 remaining dialogue groups in that range that had no story CG.
- Used the built-in GPT image generator with current character and environment references to create 16 clean, text-free 16:9 illustrations:
  - `story_ch2_ledger_found.png`
  - `story_ch2_ledger_return.png`
  - `story_ch2_ledger_burned.png`
  - `story_ch3_kairos_wall_warning.png`
  - `story_ch3_dead_belt_road.png`
  - `story_ch3_tobias_battle_notes.png`
  - `story_ch4_ash_rain_shelter.png`
  - `story_ch4_burner_classification.png`
  - `story_ch4_ash_rain_departure.png`
  - `story_ch5_warm_cliff_path.png`
  - `story_ch5_scratched_watchtower.png`
  - `story_ch6_bl07_after_sentinel.png`
  - `story_ch6_seam_gardener.png`
  - `story_ch6_sable_final_preparations.png`
  - `story_ch6_void_watcher_request.png`
  - `story_ch6_sable_vigil_reward.png`
- Explicitly excluded film/photo grain, paper/canvas texture, speckle and color noise, dithering, compression artifacts, chromatic aberration, dirty-lens overlays, muddy detail, and oversharpening from every generation prompt.
- Rejected the first Chapter 5 cliff and watchtower drafts because Arrel and Elia's identities drifted; edited both against the current short silver-haired Arrel and honey-blonde bob-haired Elia reference before integration.
- Connected each image to the first line of its exact dialogue group, preserving the lower 28 percent as a quiet dialogue-UI area rather than stacking multiple CG swaps inside one conversation.
- Registered all 16 illustrations as distinct PauseMenu Artbook entries and updated `ILLUSTRATION_CATALOG.md` to the verified 186-CG baseline.

### Verification
- UTF-8 JSON parsing passed for all dialogue data; all 16 dialogue mappings resolve to the intended file exactly once.
- Verified `TOTAL_CG=186`, `NEW_SET=16`, `MISSING_REFS=0`, and zero exact SHA-256 duplicate groups under `assets/cg/`.
- All 16 new images are RGB 1672x941 and have matching Godot `.import` metadata.
- Godot 4.6.2 import scanned and imported all 16 files. The pre-existing VFX Library popup/autoload teardown and ShaderV duplicate UID warnings remain unchanged.
- Godot 4.6.2 headless project boot passed with zero `SCRIPT ERROR` / `Parse Error` lines.
- Direct scene loads passed for `verdan_market.tscn`, `belt_waystation.tscn`, `drift_shelter.tscn`, `crumbling_coast.tscn`, and `the_seam.tscn`.
- `git diff --check` passed; only normal CRLF working-copy notices remain.

## S149 - 2026-07-01 (VN demo save/load and stability hardening)

### Done
- Audited the current demo entry path and confirmed the active core flow is `main.tscn` -> New Game -> `SceneFlow.pending_scene_id = "ch1_prologue"` -> `vn_host.tscn` -> JSON VN scenes.
- Added serializable SceneFlow state containing only scene IDs, indices, resume queue, pending state, and active status; runtime JSON dictionaries and VN UI nodes are intentionally rebuilt.
- Added VN resume preparation so loading a save made in `vn_host.tscn` restores the saved VN scene and text-step index before scene transition.
- Bumped saves to `0.3.0`, normalized missing `scene_flow` data for old saves, and added a safe Chapter 1 prologue fallback for legacy VN-host saves without flow state.
- Extended save-slot information with optional `vn_scene_id` and `vn_step` fields.
- Made faded memories unavailable to normal burn lists and rejected normal/silent burns unless an explicit `allow_faded` override is used.
- Hid faded-memory cost choices in the VN UI and made SceneFlow pay a memory cost before applying flags or rewards.
- Deprecated `goto_battle` inside SceneFlow with a warning and safe advance instead of calling the incompatible legacy battle API.
- Added `scripts/tools/validate_vn_scenes.py` for JSON structure, scene links, choice indices, flags, memory IDs, CG assets/aliases, and portrait IDs.
- Added `VN_DEMO_SCOPE_REPORT.md` with required autoloads, current legacy dependencies, and later archive/disable recommendations. No legacy systems were removed.

### Verification
- VN validator passed: 5 files, 174 steps, 0 errors, 0 warnings.
- All JSON under `data/` parsed successfully.
- Temporary Godot regression scene passed active VN export/resume at `ch1_prologue` step 12, legacy-save fallback, faded burn refusal/override, and deprecated `goto_battle` safety (`VN_STABILITY_SMOKE=PASS`).
- Godot 4.6.2 headless project boot and direct `vn_host.tscn` load passed with zero `SCRIPT ERROR` / `Parse Error` lines.
- The project-only `--check-only` invocation did not self-terminate in this Godot build, so verification used headless boot, direct scene load, the dedicated regression scene, and the JSON validator.
- `git diff --check` passed; only normal CRLF working-copy notices remain.

## S150 - 2026-07-01 (Part II Aftermath vertical slice + nine-illustration integration)

### Done
- Read the supplied Story and Art conversion guides and mapped Part II Act 3 to a playable VN continuation rather than reopening the legacy map/battle loop.
- Audited the six newly supplied 1672x941 RGB illustrations (`66.png`-`71.png`) and imported them non-destructively as:
  - `ch11_executor_strike.png`
  - `env_gray_belt_panorama.png`
  - `ch13_notebook_resonance.png`
  - `ch14_confessor_hall.png`
  - `ch17_oblivion_storm.png`
  - `ch18_living_funeral.png`
- Used the built-in GPT image generator to create three clean, text-free, noise-excluded 16:9 bridge CGs:
  - `ch11_elia_bloodwork.png`
  - `ch12_sump_closed.png`
  - `ch14_arrel_burn_slash.png`
- Added a dedicated title-menu entry, `Part II: Aftermath`, that starts a clean Chapter 11 preview without disturbing the normal New Game/Continue routes.
- Added four linked VN scenes covering Chapter 11 Departure, Chapter 12 The Reader, Chapter 13 The Third Person, and Chapter 14 The Confessor's Hall: 93 new steps with two memory-cost branches and a preview-ending handoff.
- Added five Part II memories and connected chapter transitions to automatic chapter-memory grants, making the new burn decisions use the real MemoryManager rather than cosmetic flags.
- Expanded the VN validator with the guide's voice constraints: Arrel lines stay at eight words or fewer, Kairós uses no contractions, and Han/Singer receives no direct dialogue.
- Registered all nine CGs in the Artbook and documented their source/story role in `ILLUSTRATION_CATALOG.md`; Chapter 17/18 images remain Artbook-only future storyboards so later reveals are not externalized early.

### Verification
- VN validator passed: 9 files, 267 steps, 0 errors, 0 warnings.
- Purpose-built Part II smoke scene passed all four chapter loads, automatic memory grants, and the Chapter 11/14 burn-cost branches (`PART2_AFTERSHOCK_SMOKE=PASS`).
- Verified 195 total CGs, zero exact SHA-256 duplicate groups, and matching Godot `.import` metadata for all nine new 1672x941 RGB assets.
- Godot 4.6.2 headless project boot and direct `vn_host.tscn` load both exited 0 with zero `SCRIPT ERROR` / `Parse Error` lines.
- Godot import scanned all nine new PNGs. Only the pre-existing VFX Library popup/autoload teardown and ShaderV duplicate-UID warnings remain.

## S151 - 2026-07-02 (Part II Storm chapters + nine-illustration integration)

### Done
- Audited the four newly supplied 1672x941 RGB illustrations (`72.png`-`75.png`) and assigned them by canon-safe story role:
  - `ch15_lullaby_moment.png` enters the active Chapter 15 Han sequence.
  - `env_lumea_sanctum.png`, `ch20_archivist_hollow.png`, and `ch21_kairos_confront.png` remain Artbook-only future storyboards until their chapters are implemented.
- Used the built-in GPT image generator with current character sheets and supplied story plates to create five clean, text-free story CGs:
  - `ch15_echo_shell_awakening.png`
  - `ch16_eastward_road.png`
  - `ch16_nera_checkpoint.png`
  - `ch17_memory_fracture.png`
  - `ch18_tobias_close.png`
- Explicitly excluded film/photo grain, paper/canvas texture, speckle and color noise, dithering, compression artifacts, chromatic aberration, dirty-lens overlays, muddy detail, excessive bloom, and oversharpening from every generation prompt.
- Extended the playable VN chain from Chapter 14 through Chapter 18 with 97 new steps:
  - Chapter 15 `The Singer`: Han's silent humming, the Echo Shell awakening, Celah/eastern-isles hint, and a lullaby burn choice.
  - Chapter 16 `Nera`: eastbound storm omen, route burn choice, Nera's checkpoint appearance without prematurely defining her unlocked voice, and Mira's 0.3-degree report thread.
  - Chapter 17 `The Forgetting Storm`: two real memory-cost survival choices, party memory fracture, and Arrel's indirect bloodline clue.
  - Chapter 18 `Living Funeral`: Tobias rescue/loss branch driven by a Grade 2 identity-memory decision.
- Added five chapter memories and connected all new burn choices to the real MemoryManager cost path.
- Activated the earlier Chapter 17/18 plates in runtime story data, registered all nine new assets in the Artbook, and updated `ILLUSTRATION_CATALOG.md` to the 204-CG baseline.

### Verification
- VN validator passed: 13 files, 364 steps, 0 errors, 0 warnings.
- Purpose-built no-autosave Storm smoke scene passed all four scene loads, chapter-memory grants, memory burns, and branch destinations (`PART2_STORM_SMOKE=PASS`).
- Godot 4.6.2 import scanned and imported all nine PNGs successfully; only the known VFX Library popup/autoload and ShaderV UID warnings appeared.
- Godot 4.6.2 headless project boot and direct `vn_host.tscn` load passed with zero `SCRIPT ERROR` / `Parse Error` lines.
- Verified 204 total CGs, matching `.import` metadata for all nine new assets, zero missing VN CG references, and zero exact SHA-256 duplicate groups.
- `git diff --check` passed; only normal CRLF working-copy notices remain.

## S152 - 2026-07-02 (15 supplied illustrations placed across active and future story beats)

### Done
- Audited user-supplied `76.png`-`90.png`; all 15 are unique 1672x941 RGB plates.
- Placed six illustrations directly into the existing VN sequence at information-changing beats rather than stacking redundant swaps:
  - Chapter 13: `ch13_relay_decoded.png`, `ch13_relay_breakthrough.png`.
  - Chapter 15: `ch15_han_memory_gift.png`.
  - Chapter 17: `ch17_storm_horizon.png`, `ch17_arrel_resist.png`.
  - Chapter 18: `ch18_tobias_platform.png`.
- Registered nine later or alternate plates in the Artbook without runtime references:
  - `ch15_han_last_hum.png`.
  - `env_lumea_inner_court.png`.
  - `ch20_archivist_memory_gallery.png`, `ch20_archivist_offer.png`, `ch20_archivist_warning.png`.
  - `ch20_celah_preserved.png`, `ch20_monolith_interior.png`.
  - `ch21_kairos_threshold.png`, `ch22_monolith_core.png`.
- Kept Chapters 19-22 reveal order intact: Celah, the Archivist, Kairós, and the Monolith core remain Artbook-only future storyboards.
- Added all 15 plates to the PauseMenu Artbook and updated `ILLUSTRATION_CATALOG.md` to the 219-CG baseline.

### Verification
- VN validator passed: 13 files, 364 steps, 0 errors, 0 warnings.
- Verified all 15 PNGs and matching `.import` metadata, six exact active VN references, 219 total CGs, and zero SHA-256 duplicate groups.
- Godot 4.6.2 import completed with zero `SCRIPT ERROR` / `Parse Error` lines; known VFX Library and ShaderV editor warnings remain unchanged.
- Godot 4.6.2 headless project boot and direct `vn_host.tscn` load both exited 0 with zero critical parse errors.
- `git diff --check` passed; only normal CRLF working-copy notices remain.

## S153 - 2026-07-02 (Korean-complete story pass + characterized NPC/monster presentation)

### Done
- Added Korean companion text for every shipped story field across all 24 dialogue/VN JSON files: 1,423 titles, lines, narration beats, choices, effects, burned variants, and system logs now have a Korean runtime path.
- Added a reusable Korean localization generator with protected direction tags/placeholders, a MEMORIA terminology glossary, and a strict coverage validator.
- Expanded Korean speaker and enemy display-name registries, then localized title-menu copy, Part II entry, options/accessibility labels, game-over copy, chapter cards, NPC repeat lines, and common notification patterns.
- Replaced the old map-NPC portrait-card rendering with the same animated 48px four-direction character system used by the party. Malet, Elia, Sable, Tobias, Bureau staff, guards, traders, elders, and other NPC roles now receive distinct palettes/silhouettes instead of framed face illustrations.
- Fixed enemy archetype normalization so spaced display names such as `Void Beast`, `Memory Eater`, crawlers, walkers, wraiths, sentinels, rats, and humanoid scavengers resolve to authored 128px monster silhouettes rather than the generic fallback.
- Removed full reference-sheet images from ordinary battle presentation; named boss/human cinematics remain, while normal monsters now render as isolated in-game characters.
- Generated and integrated `assets/cg/game_image/malet_fullbody_stage.png`, a clean low-noise, text-free full-body Malet stage portrait based on the current expression and turnaround references. Dialogue stage art now uses it instead of the environmental Bureau-overlook illustration.

### Verification
- Korean localization validator passed: 24 files, 1,423 fields, 15 speakers, 0 errors.
- VN validator passed: 13 files, 364 steps, 0 errors, 0 warnings.
- Godot 4.6.2 headless editor import completed with zero `SCRIPT ERROR` / `Parse Error` lines; only the known VFX Library teardown warnings appeared.
- Direct `verdan_market.tscn` load exited 0 and reported `[NPC] Malet ready`, confirming the new animated NPC path boots in the representative merchant scene.
- `git diff --check` passed; only normal CRLF working-copy notices remain.
