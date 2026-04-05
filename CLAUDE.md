# MEMORIA: The Price of Oblivion — Game Project

## 프로젝트 개요
다크 판타지 2D 탑다운 RPG / 스토리 어드벤처. Godot 4.x + GDScript.
기억을 태워 싸우는 남자의 이야기. 투 더 문 / LISA 스타일.

## 핵심 문서
- **GDD (게임 디자인 문서):** `../각종 문서/MEMORIA_GDD_v1.md` — 메카닉, 구조, 로드맵 전체
- **세션 로그:** `SESSION_LOG.md` — 세션별 작업 기록. 이전에 뭘 했고 다음에 뭘 할지 여기서 확인
- **원고 (영문):** `../원고/Chapter1.md`, `../원고/Chapter2.md`
- **세계관:** `../각종 문서/Memoriasupplement v3.md`, `../각종 문서/MEMORIA_WORLDBUILDING_SUPPLEMENT_v2.md`
- **설정집:** `../메모리아 설정집 + 세계관.docx`, `../메모리아 정보집.docx`
- **이미지 에셋:** `../이미지/` — 캐릭터 컨셉, 배경, 커버 등

## 기술 스택
- 엔진: Godot 4.6.2
- 언어: GDScript
- 아트: Leonardo AI (CG/포트레이트) + Aseprite (픽셀 스프라이트)
- 음악: Suno/Udio + 무료 에셋

## 프로젝트 구조
```
Game/
├── project.godot
├── scenes/          # .tscn 씬 파일
│   ├── main/        # 메인/타이틀
│   ├── player/      # 플레이어 관련
│   ├── ui/          # UI 씬
│   ├── battle/      # 전투 씬
│   └── maps/        # 맵 씬
├── scripts/         # .gd 스크립트
│   ├── core/        # GameManager, Player, SceneTransition
│   ├── systems/     # MemoryManager, DialogueManager
│   └── ui/          # UI 스크립트
├── assets/          # 에셋
│   ├── sprites/     # 스프라이트 (characters, tilesets)
│   ├── cg/          # 풀스크린 CG
│   ├── portraits/   # 대화 포트레이트
│   ├── audio/       # bgm, sfx
│   └── fonts/
└── data/            # JSON 데이터 (대화, 기억 등)
```

## 오토로드 (싱글톤)
- `GameManager` — 게임 상태, 스토리 플래그, 플레이어 데이터
- `MemoryManager` — 기억 연소/잔존/거래 시스템. 게임의 심장.
- `DialogueManager` — 대화 진행, 선택지, 기억 연소 연동
- `SceneTransition` — 씬 전환 페이드 인/아웃
- `DialogueBox` — 대화 UI 표시 (하단 텍스트 박스, 포트레이트, 선택지)
- `MemoryUI` — 기억 서고 UI (Tab/M 키 토글, 등급 필터, 상세 정보)
- `SystemLog` — 관리국 감지 로그 팝업 (기억 연소 시 자동 표시)
- `BattleManager` — 턴제 전투 로직 (적 데이터, 기억 연소 스킬, 행동 처리)

## 현재 진행 상태
- **S01 완료 (2026-04-05):** 프로젝트 세팅, 코어 시스템 4개, 기억 6개, 테스트 씬 동작 확인
- **S02 완료 (2026-04-05):** 플레이스홀더 스프라이트(코드 동적 생성), 아렐 4방향 걷기 애니메이션, 림 외곽 숲 맵, S01 버그 수정
- **S03 완료 (2026-04-05):** NPC 시스템, 대화 UI(DialogueBox 오토로드), JSON 로더, 엘리아 NPC 배치
- **S04 완료 (2026-04-05):** 기억 UI(아렐의 서고), 시스템 로그 팝업, 입력 매핑
- **S05 완료 (2026-04-05):** 포트레이트 이미지 적용, 턴제 전투 시스템, 전투 씬/UI, 전투 트리거
- **다음:** S06 — Godot 실행 테스트, 전투 밸런스, 세이브/로드 시스템

## 개발 규칙
1. 1세션 = 1완결 태스크. SESSION_LOG.md에 기록.
2. 플레이스홀더 우선. 에셋 없으면 색깔 사각형으로 시작.
3. 기억 연소 = 게임의 심장. 모든 시스템은 이 메카닉을 강화하는 방향으로.
4. 누적소실률 게이지(██████░░░░ 스타일) 사용 금지. 간접 표현 사용.

## 세션 시작 방법
1. SESSION_LOG.md 읽어서 현재 진행 상태 확인
2. GDD에서 해당 세션 태스크 확인
3. 작업 실행
4. SESSION_LOG.md에 결과 기록
