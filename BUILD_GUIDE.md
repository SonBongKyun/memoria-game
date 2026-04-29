# MEMORIA Demo — 빌드 가이드 (S68)

데모(Act I — Ash) Windows 빌드를 만들고 친구한테 보내기까지.

소요 시간: **30~45분** (다운로드 시간 포함).

---

## STEP 1 — Godot Export Templates 설치

빌드의 유일한 블로커. 한 번만 하면 됨.

1. Godot Editor 실행 (`C:/Users/jc/Downloads/Godot_v4.6.2-stable_win64.exe/Godot_v4.6.2-stable_win64.exe`)
2. 프로젝트 열기 (메모리아 Game 폴더)
3. 상단 메뉴 **Editor → Manage Export Templates**
4. **Download and Install** 버튼 클릭 (~600MB, 5~10분)
5. 완료되면 창 닫기

확인 방법: `C:/Users/jc/AppData/Roaming/Godot/export_templates/4.6.2.stable/` 폴더가 생기고 그 안에 `windows_release_x86_64.exe` 등이 있으면 OK.

---

## STEP 2 — 빌드 실행

### 옵션 A — Godot Editor에서 (추천, 처음이면)

1. **Project → Export...** 메뉴
2. 좌측 리스트에서 **"Windows Desktop (Demo)"** 선택
3. 우하단 **Export Project** 버튼 클릭
4. 저장 위치: `build/MEMORIA-Demo-v0.1.exe` (이미 export_path에 설정됨)
5. **Export with Debug** 체크 해제 (릴리즈 빌드)
6. 1~2분 기다리면 끝

### 옵션 B — 명령줄 (재빌드 시 빠름)

프로젝트 폴더(Game/)에서:

```bash
"/c/Users/jc/Downloads/Godot_v4.6.2-stable_win64.exe/Godot_v4.6.2-stable_win64_console.exe" \
  --headless \
  --export-release "Windows Desktop (Demo)" \
  "build/MEMORIA-Demo-v0.1.exe"
```

(또는 PowerShell에서 콘솔 .exe를 직접 실행)

---

## STEP 3 — 빌드 결과 확인

`build/` 폴더에 다음이 생성됨:
- `MEMORIA-Demo-v0.1.exe` (실행 파일)
- `MEMORIA-Demo-v0.1.pck` (게임 데이터)
- `MEMORIA-Demo-v0.1.console.exe` (디버그 콘솔 래퍼, 선택적)

### 로컬 테스트
- `MEMORIA-Demo-v0.1.exe` 더블클릭 → 게임 실행되는지 확인
- 타이틀 → New Game → 프롤로그 시작되는지 5분만 보고 닫기

---

## STEP 4 — zip 패키징 (친구한테 보내기 전)

`./package_demo.sh` 실행하면 자동으로 `MEMORIA-Demo-v0.1-Windows.zip` 생성. 그 안에:
- `MEMORIA-Demo-v0.1.exe`
- `MEMORIA-Demo-v0.1.pck`
- `README.txt` (실행 방법 + 알려진 이슈)
- `TESTER_GUIDE.md` (피드백 양식)

zip 파일 사이즈 예상: **150~300MB** (CG 130장 + 포트레이트 49장 + BGM 10트랙).

---

## 자주 발생하는 문제

### Q1. 빌드는 됐는데 실행하면 흰 화면 / 즉시 종료
**원인**: pck 파일이 exe와 같은 폴더에 없거나, Windows Defender 차단
**해결**: 둘 다 같은 폴더에 두고, 우클릭 → 속성 → "차단 해제" 체크

### Q2. "Templates not found" 에러
**원인**: STEP 1 미완료 또는 버전 불일치
**해결**: Godot 버전이 정확히 **4.6.2-stable** 인지 확인. Editor 좌상단 버전 표시.

### Q3. 익스포트는 됐는데 한국어가 깨짐
**원인**: 폰트 임포트 누락
**해결**: 프로젝트 → `addons/`, `assets/fonts/` 안에 한국어 지원 폰트가 있는지 확인. 없으면 설정에서 영어로 강제하거나 NotoSans-KR 추가.

### Q4. Windows Defender / 백신 차단
**원인**: Godot 빌드는 코드 서명이 안 되어있음 (개인 인증서 $200)
**해결**: 친구에게 사전에 "Windows Defender SmartScreen 경고는 무시하고 'Run anyway' 누르세요"라고 전달. 코드 서명은 Steam 출시 전에 별도 작업.

---

## STEP 5 — 친구한테 보내기

zip 파일을 다음 중 하나로 전송:
- **Discord**: 25MB 한도 → 부족. Discord Nitro 500MB 또는 분할 압축
- **WeTransfer**: 무료 2GB, 7일 보관, 링크 공유 — **가장 쉬움**
- **Google Drive**: 링크 공유, 권한 "링크가 있는 모든 사용자"
- **itch.io 비공개 업로드**: 링크 받은 사람만 접근, 나중에 공개 전환 가능 — 추천

함께 보낼 메시지 예시:
```
이거 30분짜리 게임 데모인데 한 번만 끝까지 해보고 솔직하게 답해줄 수 있어?
zip 풀고 MEMORIA-Demo-v0.1.exe 더블클릭하면 되고,
다 끝나면 안에 있는 TESTER_GUIDE.md 보고 답변 보내줘.
디파인더 경고 뜨면 "Run anyway" 누르면 됨.
```

---

## 체크리스트

- [ ] Godot Export Templates 설치 (~600MB)
- [ ] Project → Export → Windows Desktop (Demo) 빌드 성공
- [ ] 본인 PC에서 .exe 더블클릭 후 5분 플레이 OK
- [ ] `./package_demo.sh` 실행해서 zip 생성
- [ ] zip 파일 친구 1~3명에게 전송
- [ ] TESTER_GUIDE.md 응답 회수 시한 (보통 3~5일)
