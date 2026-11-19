# MEMORIA Unreal S364 — 그래픽·게임성 개선

실시간 전투의 콤보 입력과 공격 방향, 방어 결과 표현, 탐색 대상 표시를 개선했다. 실제 대상은 `C:\Users\jc\orca\workspaces\Game\memoria-unreal-codex\Unreal\Memoria\Memoria.uproject`, UE5.8.2/DX11이다. S362·S363과 Godot 작업을 보존했으며 미커밋 상태다. 푸시·출시는 수행하지 않았다.

## 적용한 동작

- **콤보 반응:** 기존 예약 구간 직전에 누른 다음 공격을 게임 시간 기준120ms 동안 한 칸에 보관한다. 마지막 입력 방향을 한 번만 사용하며, 너무 이른 입력은 만료된다. 버튼 유지로 공격을 추가하지 않는다.
- **베기 방향:** 시작한 베기의 화면 방향과 피해 판정을 같은 값으로 고정한다. 뒤쪽 공격을 예약해도 현재 앞쪽 베기의 피해 방향이 바뀌지 않는다. 다음 베기를 시작할 때 예약 방향을 사용한다.
- **안전한 취소:** 메뉴·연소 선택·가드·회피·피격·사망·월드 등록·복원에서 미래 예약을 정리한다. 메뉴를 열 때 이미 실행 중인 베기의 수명은 유지한다. 피해·공격 속도·타격 구간·패링 창·연소 대가·저장 형식은 유지했다.
- **방어 결과:** 청색 방패, 금색 패링 문장, 잉크 수호 표식을 구분한다. 일반 방어에는 이미 확정된 실제 HP 손실을 `Blocked · -2 HP` / `막음 · -2 HP`처럼 표시한다. HP1에서 손실0이면 손실 숫자를 만들지 않는다. 새 효과는 움직임 감소에서 팽창·글자 튐·상승을 멈추고, 번쩍임 감소에서 중심 광량을 낮춘다.
- **탐색 대상:** 기존 범위·시야·우선순위 판정이 선택한 실제 대상에 금색 표시를 붙인다. 대상이 없거나 가려짐·모달·연소 선택·일시정지·화면 밖이면 숨긴다. 말렛·엘리아·이야기 지점은 기존 E/A 입력으로 실행된다.
- **정확한 대화 안내:** 엘리아의 표시 조건과 실제 실행 조건을 공유한다. 현재 대화를 지원하지 않는 벨트·드리프트에서는 실행되지 않을 안내를 숨긴다. 다른 지역에 베르단 대화를 복제하거나 새 이야기·보상을 만들지 않았다.

## 실제 검증

| 항목 | 결과 |
|---|---|
| 에디터·게임 빌드 | package-dev-build5 PASS |
| 관련 통합 검사 | **52/52 unique PASS**, 경고 포함1개, exit0/fatal0 |
| 콤보 | 실제 PIE·로드한 검 동작·앞뒤 적의 실제 HP: 앞12/뒤14. 이른 입력의 수용·만료·최신 방향·긴 프레임·한 번 소비, 취소8경로와 실제 키보드/Enhanced Input 패드 경로 |
| 방어 화면 | 한/영·1280×720/640×360·기본/감소 옵션·HP1 포함38장. 실제 HP·상태이상·문구·글자 경계·기호 검증 |
| 대상 표시 | 말렛/엘리아/이야기 지점 ×1280/640×키보드/패드12장. 실제 E/A 실행, 범위·시야·파괴·모달·일시정지·소모 후 거절, 두 챕터 대화 거절 |
| 저장·재진입 | 기존 RevisitJourney, 수동 저장·타이틀 이어하기, 복원·연소 계산·아이템/보상 등 관련 기존47개 포함 |
| 전후 비교 | 동일 저장 소스·DLL 입력으로 기준4검사 통과. 방어38쌍+대상12쌍, 실제 PNG 헤더·해시·카메라·옵션·HP 일치. 방어 경과 시간 차이0 |
| Development | 새 빌드·쿠킹·아카이브 PASS, 실제 격리 실행 PASS·정확한17 PNG·exit0/fatal0·완료된 엔진 CSV |
| 보존 | 기존 활성882패키지의 집합/해시, 모든 기존 ArtSource와 애니메이션 브리지 유지 |

현재 전체 등록은359개이며 전체를 다시 실행한 결과로 표현하지 않는다. 52개는 기존47개와 새5개의 관련 검사다. 원본 모델·리그·UV·가중치·클립을 변경하지 않았다. 검증 중 저장 소스·설정·에셋·도구·DLL1978개 입력이 유지됐고, 패키지 빌드는 바이너리만 변경을 허용했다. 저장 입력 해시는 독립적인 컴파일 출처 증명은 아니다.

52개와 전후 비교 후 패키지 촬영 대기만 보강했다. 이때 바뀐 저장 입력은 선택적 검증 소스1개와 바이너리뿐이며 일반 게임플레이·표현 소스는 같다. build4에서 새5개를 다시 통과했고 새 패키지를 재생성했다. 중복된5개를52개에 더해 유일 검사 수를 부풀리지 않았다.

실제 패키지에서도 원래 예약 구간보다 이른 0.3077 지점의 J 입력이 버퍼에 들어가 두 번째 검 동작을 시작했다. 제어된 방어 접촉에서는 실제 손실6HP와 표시 문구 `Blocked · -6 HP`가 일치했고, 새로 누른 가드의 패링은 HP 손실0과 금색 표시를 확인했다. 실제 말렛 대상 표시, 기존 공격·연소·처치·지역 이동·수동 저장·타이틀→베르단 이어하기도 완료했다.

## 검증 범위와 이력

방어 접촉과 패키지의 콤보는 격리된 제어 검사다. 테스트 중 적 AI를 잠시 멈추고 실제 전투 함수로 접촉을 처리했으며, 자연 AI 전투는 기존 FieldFeel 등 별도 회귀에서 확인했다. 키 입력 경로 재생이며 실물 컨트롤러나 사람의 전체 플레이를 대신하지 않는다. 120ms는 게임 시간이며 하드웨어 지연 측정값이 아니다.
대상 관찰 검사는 RunSnapshot과 이야기·보상 상태를 확인한다. 별도 MemoryDomain 전체 바이트 불변까지 이 관찰 하나로 증명하지 않는다. 연소의 실제 대가와 예상 피해 계약은 별도 기존 회귀로 검증했다.
전후 기준은 새 표현을 꺼둔 같은 코드이며 콤보·대화 조건 수정은 두 실행에 모두 들어 있다. 동일한 대상 역할·위치·카메라·HP·효과 경과 시간을 확인했지만 다른 프로세스의 월드 시각·날씨·입자가 완전히 같은 픽셀이라고 주장하지 않는다.
첫 빌드의 UE5.8 Pawn 포인터 선언 두 줄, 테스트의 실제 검 동작 매핑을 수정했다. 초기 검사5개는 통과했으나 실제 화면의 특수 마이너스 기호가 누락돼 기본 기호로 바꾼 후 최종52개와 기준4개를 실행했다. 원자료와 중간 결과를 남겼으며 최종 통과 수에 중복 집계하지 않았다.
초기 패키지 검사1/2는 PNG 대기와 재개 프레임 지연으로 예약 전 입력 구간을 놓쳐 실패했다. 검사 첫 사진에만 일시정지를 소유하고 PNG 완료·실패·종료에서 원래 상태를 복원했다. 실제 입력 버퍼 길이와 예약 구간·공격 시간은 바꾸지 않았다. 02_Attack은 동작 시작 직후의 정지 사진이며 진행된 베기 자세의 단독 근거가 아니다. 일시정지만으로 부족해 검사2/3단계만 엔진 고정1/60초 간격으로 재생하고, 두 새 프레임에서 실제 간격을 확인한 뒤 첫 입력을 시작했다. 원래 시간 설정은 연소·이동 전에, 실패·종료 때에도 복원했다. 이 구간은 제어된 검사 시간이며 자연 프레임 성능 근거로 사용하지 않는다. 이후 실제 입력·두 번째 클립·방어·표시를 패키지4에서17화면과 모든 게임 동작을 확인했지만 마지막 완료 검사에 이전13장 기준이 남아 실패했다. 요청에 따른 정확한12+1+4 합산으로 고쳤으며 기본12장·S363의13장 검사도 그대로 유지했다. 최종 패키지5에서 CSV 기록 종료까지 다시 검사했다.
기존 망토·다리 갑옷 겹침은 남아 있다. 새 모델·옷감 물리·후속 장·Shipping·장시간 성능을 제공하는 변경은 아니다. 엔진 CSV는 짧은 실제 Development 계측이며 안정 FPS나 전후 성능 향상으로 해석하지 않는다. 패키지 화면의 녹색 CsvProfiler 표기는 계측 실행에 따른 것이다. 기존 시작/메모리 예산 경고는 원본 로그에 남겼다.

## 전후 화면

아래20장은 실제 PNG를 픽셀 수정 없이 복사했다. 전체50쌍과 원본 해시는 결과 JSON 및 원본 검증 폴더에 있다.

### Case00_Block_T10.png

기준 화면

![baseline](C:/Users/jc/orca/workspaces/Game/memoria-unreal-codex/docs/unreal-migration/evidence/s364/defense_Case00_Block_T10_Before.png)

개선 화면

![after](C:/Users/jc/orca/workspaces/Game/memoria-unreal-codex/docs/unreal-migration/evidence/s364/defense_Case00_Block_T10_After.png)

### Case00_Parry_T10.png

기준 화면

![baseline](C:/Users/jc/orca/workspaces/Game/memoria-unreal-codex/docs/unreal-migration/evidence/s364/defense_Case00_Parry_T10_Before.png)

개선 화면

![after](C:/Users/jc/orca/workspaces/Game/memoria-unreal-codex/docs/unreal-migration/evidence/s364/defense_Case00_Parry_T10_After.png)

### Case00_Ward_T10.png

기준 화면

![baseline](C:/Users/jc/orca/workspaces/Game/memoria-unreal-codex/docs/unreal-migration/evidence/s364/defense_Case00_Ward_T10_Before.png)

개선 화면

![after](C:/Users/jc/orca/workspaces/Game/memoria-unreal-codex/docs/unreal-migration/evidence/s364/defense_Case00_Ward_T10_After.png)

### Case07_Block_T10.png

기준 화면

![baseline](C:/Users/jc/orca/workspaces/Game/memoria-unreal-codex/docs/unreal-migration/evidence/s364/defense_Case07_Block_T10_Before.png)

개선 화면

![after](C:/Users/jc/orca/workspaces/Game/memoria-unreal-codex/docs/unreal-migration/evidence/s364/defense_Case07_Block_T10_After.png)

### Case07_BlockHP1_T10.png

기준 화면

![baseline](C:/Users/jc/orca/workspaces/Game/memoria-unreal-codex/docs/unreal-migration/evidence/s364/defense_Case07_BlockHP1_T10_Before.png)

개선 화면

![after](C:/Users/jc/orca/workspaces/Game/memoria-unreal-codex/docs/unreal-migration/evidence/s364/defense_Case07_BlockHP1_T10_After.png)

### Case00_Elia_After

기준 화면

![baseline](C:/Users/jc/orca/workspaces/Game/memoria-unreal-codex/docs/unreal-migration/evidence/s364/focus_Case00_Elia_Baseline_Before.png)

개선 화면

![after](C:/Users/jc/orca/workspaces/Game/memoria-unreal-codex/docs/unreal-migration/evidence/s364/focus_Case00_Elia_After_After.png)

### Case07_Story_After

기준 화면

![baseline](C:/Users/jc/orca/workspaces/Game/memoria-unreal-codex/docs/unreal-migration/evidence/s364/focus_Case07_Story_Baseline_Before.png)

개선 화면

![after](C:/Users/jc/orca/workspaces/Game/memoria-unreal-codex/docs/unreal-migration/evidence/s364/focus_Case07_Story_After_After.png)

### Case08_Malet_After

기준 화면

![baseline](C:/Users/jc/orca/workspaces/Game/memoria-unreal-codex/docs/unreal-migration/evidence/s364/focus_Case08_Malet_Baseline_Before.png)

개선 화면

![after](C:/Users/jc/orca/workspaces/Game/memoria-unreal-codex/docs/unreal-migration/evidence/s364/focus_Case08_Malet_After_After.png)

### 실제 패키지 14_ComboChain

![14_ComboChain](C:/Users/jc/orca/workspaces/Game/memoria-unreal-codex/docs/unreal-migration/evidence/s364/Cooked_14_ComboChain.png)

### 실제 패키지 15_BlockOutcome

![15_BlockOutcome](C:/Users/jc/orca/workspaces/Game/memoria-unreal-codex/docs/unreal-migration/evidence/s364/Cooked_15_BlockOutcome.png)

### 실제 패키지 16_ParryOutcome

![16_ParryOutcome](C:/Users/jc/orca/workspaces/Game/memoria-unreal-codex/docs/unreal-migration/evidence/s364/Cooked_16_ParryOutcome.png)

### 실제 패키지 17_InteractionFocus

![17_InteractionFocus](C:/Users/jc/orca/workspaces/Game/memoria-unreal-codex/docs/unreal-migration/evidence/s364/Cooked_17_InteractionFocus.png)

## 근거 파일

- [결과 JSON](C:/Users/jc/orca/workspaces/Game/memoria-unreal-codex/docs/unreal-migration/S364_RESULTS.json)
- [원본 화면·해시 목록](C:/Users/jc/orca/workspaces/Game/memoria-unreal-codex/docs/unreal-migration/evidence/s364/manifest.json)
- [Development 실행 파일](C:/Users/jc/orca/workspaces/Game/memoria-unreal-codex/Unreal/Memoria/Saved/Validation/s364/package-dev5/Windows/Memoria/Binaries/Win64/Memoria.exe)
- [통합52검사](C:/Users/jc/orca/workspaces/Game/memoria-unreal-codex/Unreal/Memoria/Saved/Validation/s364/final-native1/result.json)
- [검증 도구 수정 후 신규5재검사](C:/Users/jc/orca/workspaces/Game/memoria-unreal-codex/Unreal/Memoria/Saved/Validation/s364/native-post-smoke-repair1/result.json)
- [패키지17화면 검사](C:/Users/jc/orca/workspaces/Game/memoria-unreal-codex/Unreal/Memoria/Saved/Validation/s364/packaged-smoke4/result.json)
- [엔진 CSV 분석과 한계](C:/Users/jc/orca/workspaces/Game/memoria-unreal-codex/Unreal/Memoria/Saved/Validation/s364/performance-final.json)
