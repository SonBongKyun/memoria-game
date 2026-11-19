# MEMORIA Unreal S365 — 남은 전투 문제 보완과 화면 개선

현재 실시간 언리얼 프로젝트의 타격 누락과 연속 문구 겹침을 고치고, 적의 최근 체력 손실 표시를 추가했다. 실제 대상은 `C:\Users\jc\orca\workspaces\Game\memoria-unreal-codex`이며 UE5.8.2/DX11이다. 이전 S362–S364 미커밋 작업과 Godot 프로젝트를 보존했다. 커밋·푸시·출시는 수행하지 않았다.

## 적용한 동작

- **긴 프레임의 타격 누락 수정:** 이전에는 프레임 끝이 베기 타격 구간(.30–.55) 안에 있을 때만 판정했다. 이제 경과한 구간이 이 타격 구간을 통과했는지 확인한 후 현재 동작을 끝내거나 다음 동작으로 넘어간다. 공격력·타격 범위·동작 속도·예약 구간을 바꾸지 않았다. 같은 적은 한 번만 맞고, 타격 구간 안으로 뒤늦게 들어온 다른 적은 기존 규칙대로 맞는다. 현재 베기 방향과 다음 예약 방향의 분리도 유지한다.
- **전투 문구 가독성:** 방어·패링·적 피해·회복·상태 문구를 실제 폰트와 그림자 크기로 먼저 측정해 서로 겹치지 않는 가까운 위치에 배치한다. 실제 피해량과 원문·색·수명·경과 시간은 유지한다. 화면 안에 들어갈 수 없는 과부하에서는 문구를 버리거나 합산하지 않고 진단에 드러낸다. 고정 HUD와의 모든 겹침까지 방지하는 기능은 아니다.
- **최근 손실 체력 표시:** 적의 현재 HP 색상은 즉시 줄어든다. 방금 잃은 부분만 옅은 금색으로 .16초 유지한 뒤 짧게 따라 내려간다. 게임 시간 기준이며 일시정지 중에는 진행하지 않는다. 회복·최대 HP 변경·죽음·재바인딩 시 잔상을 정리하고, 움직임 감소에서는 즉시 현재 값으로 맞춘다. 저장 데이터나 피해 계산에 관여하지 않는다.

## 실제 검증

| 항목 | 결과 |
|---|---|
| 새 에디터 빌드 | PASS, exit0, 컴파일 오류0 |
| 관련 회귀 | **54/54 unique PASS** (정상53, 경고 포함1), exit0/fatal0. 기존52개와 새2개 |
| 타격 검사 | 실제 PIE·등록된 적 HP·로드한 검 클립. .29→.56 통과, 전체 동작 건너뜀, 작은 프레임 비교, 중복/범위/뒤쪽/타격 종료 후 제외, 구간 내 새 대상, 역방향 예약14·마무리22·강공격30 |
| HUD 검사 | 한/영×1280/640×글자100/125%×기본/감소16조건. 실제 방어→패링→검 피해12 동시 문구와 가장자리 연막 문구, **32 PNG** |
| 체력 잔상 | 실제 현재 HP 즉시 반영, 잔상·회복·재바인딩·소멸·파괴 정리. 최대 HP/일부 경계는 별도 helper 검사 |
| 전후 비교 | 같은 저장 소스/DLL 입력·카메라·옵션·HP·문구·각 팝업 경과 시간으로 **32쌍**, 최대 시간차0 |
| 겹침 | 제어된32화면의 문구 경계 겹침: 기준56쌍 → 개선0쌍 |
| Development | 새 빌드/쿠킹/아카이브 PASS. 실제 격리 실행17 PNG, 연속 방어 문구 비중첩·기존 콤보/연소/처치/3지역/수동 저장/타이틀 이어하기·완료 CSV, exit0/fatal0 |
| 보존 | 기존882패키지 집합/해시·모든 기존 ArtSource·애니메이션 브리지 유지. 이번10개 소스/도구 파일 범위 |

전체 현재 등록은361개이며 전체를 다시 실행한 결과는 아니다. 초기 새2개 검사는 별도 점검 후 최종54개에 포함해 다시 실행했으며 중복 집계하지 않았다. 기준1개는 새 HUD 표현만 꺼둔 같은 코드다. 긴 프레임 수정은 두 실행에 모두 적용돼 있다. 실제 PNG의 헤더와 해시를 확인했고 픽셀 수정 없이 복사했다. 월드 날씨·입자·다른 프로세스의 객체 이름까지 픽셀 동일하다고 주장하지 않는다.

검사는 제어된 AI·실제 전투 함수와 입력 경로를 사용한다. 긴 프레임 검사는 게임 델타를 명시적으로 진행하며 움직인 적의 과거 위치를 복원하거나 검 메시의 실제 접촉을 역산하지 않는다. 가장자리 문구는 실제 아이템 효과를 만든 뒤 실제 카메라를 옮긴 검사이며 원문·위치·경과 시간 값을 조작하지 않는다. 원래 카메라·일시정지·뷰포트·옵션은 검사 뒤 복원한다. 기존 자연 AI·저장·연소·이동 검사도 최종54개에 포함했다.

첫 네이티브 실행은 다른 프로젝트의 엔진 빌드 대기와 겹쳐 플랫폼 조회에서 약3분 지연됐다. 패키지 쿠킹에서도 같은 공유 빌드 대기가 발생했다. 다른 프로세스를 중단하지 않았고 대기 해소 후 정상 완료했다. 기존 S364 로그에도 동일한15개 엔진 초기화 Condition failed 진단이 존재하며 이번 선택 검사 오류와 구분한다. 원본 로그를 남겼다. 첫 비교 스크립트는 엔진 상대 경로를 정규화하지 못해 중단됐고, 저장 폴더를 확인한 원본 복사 경로로 읽도록 보완한 뒤 모든 해시·의미·시간 비교를 통과했다. 게임 소스나 검사 조건은 바꾸지 않았다. 패키지의 기존 선택적 콤보/방어 촬영 단계는 제어된1/60초 엔진 간격이며 자연 FPS 근거가 아니다. 새 성능 향상·Shipping·실물 패드·사람의 전체 플레이 검증은 주장하지 않는다. 기존 망토/다리 갑옷 겹침은 남아 있다.

## 원본 화면

### Case00_Concurrent

기준

![before](C:/Users/jc/orca/workspaces/Game/memoria-unreal-codex/docs/unreal-migration/evidence/s365/Case00_Concurrent_before.png)

개선

![after](C:/Users/jc/orca/workspaces/Game/memoria-unreal-codex/docs/unreal-migration/evidence/s365/Case00_Concurrent_after.png)

### Case03_Concurrent

기준

![before](C:/Users/jc/orca/workspaces/Game/memoria-unreal-codex/docs/unreal-migration/evidence/s365/Case03_Concurrent_before.png)

개선

![after](C:/Users/jc/orca/workspaces/Game/memoria-unreal-codex/docs/unreal-migration/evidence/s365/Case03_Concurrent_after.png)

### Case15_EdgeWord

기준

![before](C:/Users/jc/orca/workspaces/Game/memoria-unreal-codex/docs/unreal-migration/evidence/s365/Case15_EdgeWord_before.png)

개선

![after](C:/Users/jc/orca/workspaces/Game/memoria-unreal-codex/docs/unreal-migration/evidence/s365/Case15_EdgeWord_after.png)

### 실제 Development 연속 방어

![cooked](C:/Users/jc/orca/workspaces/Game/memoria-unreal-codex/docs/unreal-migration/evidence/s365/Cooked_16_ParryOutcome.png)
