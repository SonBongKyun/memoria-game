# MEMORIA Unreal S363 — 그래픽 개선 결과

언리얼의 세 지역에 석재 받침·이음·국소 반사 표현을 더하고, 전투 HUD의 아이템 퀵슬롯 겹침과 연소 선택창의 첫 표시 순서를 고쳤다. 기존 Godot 작업과 S362 변경을 보존했다. UE 5.8.2 / DX11, 실제 대상은 `C:\Users\jc\orca\workspaces\Game\memoria-unreal-codex\Unreal\Memoria\Memoria.uproject`이다. 작업은 미커밋 상태이며 푸시·출시는 하지 않았다.

## 반영한 개선

- 베르단: 두 주요 노점의 바닥 받침과 얇은 경계 덮개, 돌 조각, 절제된 접촉·따뜻한 반사 표현. 원래 장애물 범위 안에 받침을 유지했다.
- 벨트: 벽 하단의 엇갈린 석재와 건조한 황토색 이음. 드리프트: 차갑고 젖은 돌 가장자리와 기존 등불에 맞춘 부드러운 바닥 표현. 전체 구도를 바꾸는 재설계가 아닌 가까운 표면과 경계의 보강이다.
- 아이템 퀵슬롯: HP·공격 안내·MEM 원을 포함한 전투 리본과 실제 경계가 겹치면 위로 이동하고 12 Slate 단위 간격을 둔다. 글꼴은 유지하고 셀 높이를 98→104로 늘려 이름을 프레임 안에 넣었다.
- 연소 선택창: 실제 입력 처리 안에서 뷰포트 최상위 호스트를 먼저 올린다. 첫 표시부터 기존 업적 팝업 위에 나오고 닫으면 원래 순서로 돌아간다.

기존 재질 부모·그림·아틀라스와 카메라 가림 처리를 유지한다. 새 석재 인스턴스는 베르단 62개/3묶음, 벨트 280개/2묶음, 드리프트 187개/2묶음이다. 부드러운 표현용 평면은 각각 4/8/8개다. 새 조명·충돌·내비게이션·게임 난수·스토리 규칙을 추가하지 않았다.

## 검증

| 항목 | 새 실행 결과 |
|---|---|
| 에디터 빌드 | build10 PASS |
| 관련 통합 네이티브 | **40/40 unique PASS**, 경고 포함 1개, exit0/fatal0 |
| HUD 실제 화면 | EN/KO × 1280×720/640×360 × 키보드/패드 입력 × 글자100/125%, 16조합·48 PNG. 실제 글자 경계·키/이름·첫 입력 레이어·닫기/복원 검사 |
| 세 지역 전후 | 동일 최종 저장 입력으로 기준3검사와 개선3검사, 6쌍의 실제1280×720 PNG. 플레이어/카메라/FOV 일치, 실제 PNG 헤더·해시 확인 |
| Development 패키지 | 새 빌드·쿠킹·아카이브 PASS, 격리된 실제 실행 PASS·13 PNG·exit0/fatal0·완료된 엔진 CSV |
| 보존 | 기존 활성 패키지882/882 및 모든 기존 ArtSource 해시 일치. 기존 애니메이션 브리지 복원, 거부한 망토 코드 비활성 |

40개는 현재 변경에 관련된 기존36개와 새4개다. 현재 전체354개를 다시 실행한 결과로 표현하지 않는다. 옛 S362 전체350 통과도 이번 변경의 새 전체 검증으로 집계하지 않았다. 검증 실행 중 저장 소스·설정·에셋·도구·DLL 입력 해시가 유지됐고, 패키지 빌드 중에는 바이너리만 변경을 허용했다. 저장 입력 해시는 독립적인 컴파일 출처 증명은 아니다.

실제 패키지는 공격·연소·처치, 첫 선택창, 세 지역 이동, 가드 유지 중 새 HUD, 수동 저장 및 타이틀→수동슬롯→베르단 이어하기를 검사한다. 새 `13_GuardHud`는 기존 R4/Sword_Block 동작과 새 HUD의 근거이며 망토 보정 성공의 근거가 아니다.

## 거부한 후보와 수정 이력

망토 본 보정은 채택하지 않았다. 첫 후보의 교차 샘플은 893→937로 늘었다. 반대 방향 후보는 전체893→870으로 줄었으나 Guard20에서125→136으로 악화돼 엄격한 포즈별 기준을 통과하지 못했다. 후보 코드·측정·실패 화면은 `Saved/Validation/s363/rejected-cloth-code1`, `cloth-native2/3`에 남겼고 기존 애니메이션 코드를 정확히 복원했다. 기존 망토·다리 갑옷 겹침은 남아 있다.

초기 빌드/후보/검사 실패는 보존하고 통과 수에서 제외했다. 검사에서 나온 슬롯 이름 경계는 공간을 늘려 해결했다. 작은 UI 캡처 후 남는 엔진 스크린샷 전역 크기는 검사 시작/요청 범위에서 저장·복원하고 실제 렌더 타깃과 PNG1280×720 검사를 유지했다. 기존 베르단 묶음 예산 실패는 같은 재질의 받침·덮개 묶음을 병합해 해결했으며 기존 제한을 완화하지 않았다.

남은 범위: 물리 패드 장치와 사람의 전체 플레이, Shipping, 장시간 성능은 검증하지 않았다. 이번 국소 표면 표현을 큰 폭의 모델 교체나 안정적인 FPS 향상으로 주장하지 않는다. 실제 엔진 CSV의 GPU 시간은 측정됐으나 짧은 계측 실행이며 전후 최적화 벤치마크가 아니다. 패키지 캡처에는 측정용 CsvProfiler 표기가 보인다. 엔진 시작 및 기존 렌더 경고는 원본 로그와 결과 JSON에 남겼다.

## 실제 화면

아래 이미지는 원본 네이티브 PNG를 편집 없이 복사했다. 카메라와 크기는 일치하지만 날씨·입자·월드 시각이 완전히 같은 픽셀 비교는 아니다.

### verdan_market / Wide

개선 전

![verdan_market Wide before](C:/Users/jc/orca/workspaces/Game/memoria-unreal-codex/docs/unreal-migration/evidence/s363/verdan_market_Wide_Before.png)

개선 후

![verdan_market Wide after](C:/Users/jc/orca/workspaces/Game/memoria-unreal-codex/docs/unreal-migration/evidence/s363/verdan_market_Wide_After.png)

### verdan_market / Detail

개선 전

![verdan_market Detail before](C:/Users/jc/orca/workspaces/Game/memoria-unreal-codex/docs/unreal-migration/evidence/s363/verdan_market_Detail_Before.png)

개선 후

![verdan_market Detail after](C:/Users/jc/orca/workspaces/Game/memoria-unreal-codex/docs/unreal-migration/evidence/s363/verdan_market_Detail_After.png)

### belt_waystation / Wide

개선 전

![belt_waystation Wide before](C:/Users/jc/orca/workspaces/Game/memoria-unreal-codex/docs/unreal-migration/evidence/s363/belt_waystation_Wide_Before.png)

개선 후

![belt_waystation Wide after](C:/Users/jc/orca/workspaces/Game/memoria-unreal-codex/docs/unreal-migration/evidence/s363/belt_waystation_Wide_After.png)

### belt_waystation / Detail

개선 전

![belt_waystation Detail before](C:/Users/jc/orca/workspaces/Game/memoria-unreal-codex/docs/unreal-migration/evidence/s363/belt_waystation_Detail_Before.png)

개선 후

![belt_waystation Detail after](C:/Users/jc/orca/workspaces/Game/memoria-unreal-codex/docs/unreal-migration/evidence/s363/belt_waystation_Detail_After.png)

### drift_shelter / Wide

개선 전

![drift_shelter Wide before](C:/Users/jc/orca/workspaces/Game/memoria-unreal-codex/docs/unreal-migration/evidence/s363/drift_shelter_Wide_Before.png)

개선 후

![drift_shelter Wide after](C:/Users/jc/orca/workspaces/Game/memoria-unreal-codex/docs/unreal-migration/evidence/s363/drift_shelter_Wide_After.png)

### drift_shelter / Detail

개선 전

![drift_shelter Detail before](C:/Users/jc/orca/workspaces/Game/memoria-unreal-codex/docs/unreal-migration/evidence/s363/drift_shelter_Detail_Before.png)

개선 후

![drift_shelter Detail after](C:/Users/jc/orca/workspaces/Game/memoria-unreal-codex/docs/unreal-migration/evidence/s363/drift_shelter_Detail_After.png)

### 전투 퀵슬롯

![전투 퀵슬롯](C:/Users/jc/orca/workspaces/Game/memoria-unreal-codex/docs/unreal-migration/evidence/s363/Case00_Combat.png)

### 작은 화면·패드

![작은 화면·패드](C:/Users/jc/orca/workspaces/Game/memoria-unreal-codex/docs/unreal-migration/evidence/s363/Case07_Combat.png)

### 선택창 첫 표시

![선택창 첫 표시](C:/Users/jc/orca/workspaces/Game/memoria-unreal-codex/docs/unreal-migration/evidence/s363/Case01_PickerFirstPaint.png)

### 패키지 가드·HUD

![패키지 가드·HUD](C:/Users/jc/orca/workspaces/Game/memoria-unreal-codex/docs/unreal-migration/evidence/s363/Cooked_13_GuardHud.png)

## 파일

- [검증 결과 JSON](C:/Users/jc/orca/workspaces/Game/memoria-unreal-codex/docs/unreal-migration/S363_RESULTS.json)
- [전후 화면·해시 목록](C:/Users/jc/orca/workspaces/Game/memoria-unreal-codex/docs/unreal-migration/evidence/s363/manifest.json)
- [Development 실행 파일](C:/Users/jc/orca/workspaces/Game/memoria-unreal-codex/Unreal/Memoria/Saved/Validation/s363/package-dev1/Windows/Memoria/Binaries/Win64/Memoria.exe)
- [통합40검사](C:/Users/jc/orca/workspaces/Game/memoria-unreal-codex/Unreal/Memoria/Saved/Validation/s363/final-native2/result.json)
- [실제 패키지 실행](C:/Users/jc/orca/workspaces/Game/memoria-unreal-codex/Unreal/Memoria/Saved/Validation/s363/packaged-smoke1/result.json)

- [실제 엔진 CSV 분석과 한계](C:/Users/jc/orca/workspaces/Game/memoria-unreal-codex/Unreal/Memoria/Saved/Validation/s363/performance-final.json)
