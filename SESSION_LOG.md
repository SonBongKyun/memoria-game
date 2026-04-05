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
- [ ] Godot 4.6에서 전체 프로젝트 실행 테스트
- [ ] 전투 시스템 기초 (턴제 전투, 기본 공격, HP바)
- [ ] 전투 중 기억 연소 스킬 선택

### 메모
- MemoryUI는 MemoryManager.memories를 직접 읽어서 카드 생성. 기억 추가/연소 시 _refresh_cards() 호출로 동기화.
- SystemLog는 queue 방식이라 연속 3번 연소해도 순차적으로 표시됨.
- memory_menu 키: Tab(4194306) + M(77)
