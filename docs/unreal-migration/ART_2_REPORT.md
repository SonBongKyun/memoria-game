# S292 — 베르단 적 삽화 보완과 Windows 개발 프리뷰

## 화면에서 발견한 결손

S291 실제 전투 캡처에서는 `Alley Rat` 이름에 원본의 `enemy_ash_hound_stage_v1.png` 그림이 표시됐다. 이는 베르단 재방문 전투의 두 적 중 하나에서 바로 드러나는 종 불일치다. 이미 연결된 Market Thief·아렐·엘리아·시장 배경은 실제 화면에서 확인했고 신규 설정을 추가할 결손으로 보지 않았다.

## 변경

- 내장 `image_gen`으로 일반 쥐의 임시 세로 전투 그림을 생성했다. 첫 시안은 작은 화면에서 적이 너무 작아 최종 사용하지 않고 로컬 `evidence/art2/alley_rat_candidate_v1.png`에 보존했다. 더 가까운 구도의 두 번째 그림을 `Unreal/ArtSource/BattleEntry/alley_rat_study_v2.png`로 선택했다. 정확한 프롬프트·생성기·SHA-256·비정사 상태는 같은 위치의 provenance JSON에 있다.
- 새 그림을 `/Game/Memoria/Presentation/BattleEntry/T_AlleyRatStudy`로만 증분 임포트했다. 원본 하운드 아트, 원본 적 데이터, 기존 여섯 UE 전투 텍스처를 그대로 보존한다. 새 패키지를 읽을 수 없으면 기존 매핑이 화면 대체 수단으로 남는다.
- 일반 재방문 조우에서 Alley Rat의 전투 초상에만 시안을 연결했다. 메커닉·HP·보상·대사·종족 설정은 바꾸지 않았다. 미승인 재집필 자료는 로컬에서 읽기 전용으로 검토하고 젖은 골목·등불 분위기만 참고했다.
- 패키징 전 `DefaultEngine.ini`의 `GameDefaultMap` 한 줄을 Foundation 테스트 맵에서 실제 `L_Ch2VerdanSlice`로 바꿨다. [수정 전 설정](evidence/art2/DefaultEngine.before.ini)로 한 줄을 되돌릴 수 있고 Editor 시작 맵은 유지했다.

## 검증

- 새 에셋 1개 임포트와 별도 Unreal 프로세스의 전체 7개 텍스처 재읽기 통과. S291 기존 6개 패키지 해시는 [진입 기준](evidence/art2/baseline.json)과 모두 같다.
- Unreal 5.8.2 게임 코드 컴파일과 최종 렌더 전투 검증 **47/47 통과**. 실제 [Alley Rat 전투 화면](evidence/art2/automation01/BattleAlleyRat.png)과 세부 [공개 검증 요약](evidence/art2/validation_public.json)를 보존했다. 캡처는 화면 저장 시점에 도주 상태 `FLED`가 보이는 그림이며 첫 턴 캡처로 부르지 않는다.
- 원본 4,217개·S291까지 존재한 Unreal 패키지 101개 바이트 동일. S292 새 텍스처 1개만 추가. [보존 검사](evidence/art2/preservation.json) PASS.

## 배포 범위

Windows **개발 프리뷰**다. 현재 게임은 베르단 도입·말렛 거래·체크포인트·서고·재방문 전투 진입과 도주까지 작동한다. 공격·방어·적 턴·전투 기억 연소·승패·Chapter3·업적 영구 저장·완성된 15~20분 구간은 아직 구현·인증되지 않았다. 정식 Steam 데모로 표시하지 않는다. 기존 `demo-v0.1.0` 공개 프리릴리스는 변경하지 않는다.

## 패키지와 배포 결과

- Unreal UAT `BuildCookRun` Win64 Development `-build -cook -stage -pak -archive -CookAll` 성공(ExitCode 0). [공개 검증 요약](evidence/art2/validation_public.json).
- 패키지 manifest에 `L_Ch2VerdanSlice`, `T_AlleyRatStudy`, `T_MarketThiefStudy`와 DefaultEngine 설정이 포함됨. [패키지 내용 검사](evidence/art2/packaged_content_check.json) PASS.
- 패키지의 실제 `Memoria.exe`를 숨김 창으로 기동해 베르단 맵의 `LoadMap`/`Bringing World`를 관찰하고 8초 뒤 검사 프로세스를 종료했다. 관찰 구간 Fatal/Assertion/Unhandled Exception 0. [공개 검증 요약](evidence/art2/validation_public.json). 프리뷰 기동 검사이며 사람의 완전한 경로 플레이 인증이 아니다.
- PDB와 디버그 manifest만 ZIP에서 제외하고 원본 패키지는 `Unreal/Memoria/Saved/ReleasePreview/S292/Windows`에 보존. 배포 파일: `MEMORIA-Unreal-Verdan-S292-Windows-Development.zip` (342,081,925 bytes, SHA-256 `7013469696567978bc139734f9cb7332073bad63a2658bcb2dec2e6dac3e7108`). ZIP 전체 무결성 PASS. [아티팩트 기록](evidence/art2/release_artifact.json).
- Unreal Editor 시작부에서 기존 S289–S291와 같은 PreInit `Condition failed` 15건이 이 실행에도 있음. 정확한 원인과 무해 여부는 미확정이며 선택 전투 47개 PASS 및 배포 실행 PASS와 구분한다.
- 원격 배포 대상: 기존 공개 Godot 데모와 별도인 GitHub prerelease `unreal-verdan-preview-s292-20260923`. [배포 설명 초안](evidence/art2/release_notes.md)에 범위를 명시했다. 실제 업로드 성공 여부는 GitHub 릴리스에서 확인한다.

원시 실행 ZIP·전체 로그·미승인 Notion 조회 기록은 로컬 작업 트리에만 보관한다. 공개 브랜치에는 검증 요약과 실제 화면을 싣는다.
