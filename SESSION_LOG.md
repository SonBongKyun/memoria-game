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
- [ ] Godot F5 실행 테스트 (실제 플레이)
- [ ] 최종 폴리싱 (밸런스 미세조정, 누락 에셋 체크)
