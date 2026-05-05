# MEMORIA 폰트 시스템

S71 기준. **현재 시스템 폰트 자동 매칭으로 작동.** 추가 다운로드 없이 빌드 가능.

## 자동 폰트 체인 (theme.tres 기준)

플레이어 PC에 설치된 첫 매칭 폰트를 사용:

### Body (대화·나레이션·일반 텍스트) — Serif
1. `Cormorant Garamond` (직접 설치 권장)
2. `EB Garamond`
3. `Garamond`
4. `Cambria` (Windows 기본)
5. `Constantia` (Windows 기본)
6. `Palatino Linotype` (Windows 기본)
7. `Noto Serif KR` (한국어)
8. `Batang` (Windows 기본 한국어)
9. `Times New Roman` (최후 fallback)

### Title (제목·로고) — Stylized Serif
1. `Cinzel`
2. `Trajan Pro`
3. `Cormorant Garamond`
4. `Constantia`
5. `Cambria`
6. `Noto Serif KR`
7. `Times New Roman`

### UI (버튼·메뉴) — Sans
1. `Segoe UI` (Windows 기본)
2. `Pretendard`
3. `Malgun Gothic` (Windows 기본 한국어)
4. `Helvetica Neue`
5. `Arial`

---

## 진짜 시네마틱 느낌으로 업그레이드 (선택)

플레이어 PC 의존성 없이 일관된 폰트를 보장하려면 TTF를 직접 임베드:

### 무료 추천 폰트 (모두 SIL OFL 라이선스, 상업 사용 OK)

#### 영문 본문
- **Cormorant Garamond** — https://fonts.google.com/specimen/Cormorant+Garamond
  - Garamond 계열 우아한 세리프. 다크 판타지 VN의 정수.
- **EB Garamond** — https://fonts.google.com/specimen/EB+Garamond
  - 클래식 도서 인쇄용. 가독성 최고.
- **Spectral** — https://fonts.google.com/specimen/Spectral
  - 화면 가독성 + 문학적 무드.

#### 영문 제목
- **Cinzel** — https://fonts.google.com/specimen/Cinzel
  - 트라야누스 비문 스타일. *House in Fata Morgana* 분위기.
- **Cormorant SC** — Small Caps 버전. 챕터 타이틀에 좋음.

#### 한국어
- **Pretendard** — https://github.com/orioncactus/pretendard (UI/sans, 무료, OFL)
- **본명조 (Bonmyeongjo)** — 윤재현 디자인, 무료
- **KoPub Batang** — https://www.kopus.org/biz-electronic-font2/ (출판용, 무료)
- **Noto Serif KR** — https://fonts.google.com/noto/specimen/Noto+Serif+KR (Google, OFL)

### 설치 방법

1. 위 링크에서 TTF/OTF 파일 다운로드
2. 이 폴더(`assets/fonts/`)에 복사:
   ```
   assets/fonts/
   ├── CormorantGaramond-Regular.ttf
   ├── CormorantGaramond-Italic.ttf
   ├── Cinzel-SemiBold.ttf
   ├── NotoSerifKR-Regular.ttf
   └── Pretendard-Regular.ttf
   ```
3. `theme.tres`를 텍스트 에디터로 열기
4. `[sub_resource type="SystemFont" ...]` 블록을 `[sub_resource type="FontFile" ...]` 로 교체:

```gdresource
[sub_resource type="FontFile" id="Font_body"]
font_data = preload("res://assets/fonts/CormorantGaramond-Regular.ttf")

[sub_resource type="FontFile" id="Font_title"]
font_data = preload("res://assets/fonts/Cinzel-SemiBold.ttf")

[sub_resource type="FontFile" id="Font_ui"]
font_data = preload("res://assets/fonts/Pretendard-Regular.ttf")
```

또는 더 쉽게 — Godot Editor에서:
- 프로젝트 폴더 새로고침 후
- `theme.tres` 더블클릭
- 각 SystemFont 리소스를 우클릭 → "Convert to Inline" → "FontFile"로 변경
- TTF 드래그 앤 드롭

### 폰트 임베드 시 zip 사이즈 영향
- Cormorant Garamond Regular: ~250KB
- Cinzel SemiBold: ~50KB
- Noto Serif KR Regular: ~5MB (한국어 글리프 많음)
- Pretendard Regular: ~1.5MB

총 약 7~8MB 추가. 데모 zip이 150~300MB라 거의 영향 없음.

---

## 권장 다음 단계

1. **현재 (S71)**: SystemFont 자동 매칭 — 이미 작동 중. 일단 이대로 빌드.
2. **테스터 피드백 수집 후**: 친구가 "폰트가 여전히 약하다"고 하면 위 임베드로 업그레이드.
3. **Steam 출시 전**: 반드시 임베드. 플레이어 PC에 폰트 없으면 fallback 깨질 수 있음.
