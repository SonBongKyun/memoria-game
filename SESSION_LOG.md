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
- F5 테스트: 탐색 이벤트 발생 확인
- 추가 스토리 분량 필요 시 Ch6 에필로그 확장
