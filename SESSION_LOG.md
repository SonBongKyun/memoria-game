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
- [ ] Godot 4.6에서 프로젝트 열고 실행 테스트
- [ ] 맵 레이아웃 조정 (실제 플레이 느낌 확인 후)
- [ ] NPC 기본 씬 + 상호작용 시스템 (대화 트리거)
- [ ] 대화 UI (DialogueBox 씬) — 화면 하단 텍스트 박스 + 포트레이트
- [ ] DialogueManager에 JSON 파일 로더 연결

### 메모
- 스프라이트는 _ready()에서 코드로 생성됨. 실제 에셋으로 교체 시 _setup_placeholder_sprites() 함수만 제거하면 됨.
- 맵도 ColorRect + StaticBody2D로 코드 생성. 나중에 TileMap으로 전환 가능.
- 기존 main.tscn (테스트 씬)은 삭제하지 않고 유지. 시스템 디버그용으로 사용 가능.
