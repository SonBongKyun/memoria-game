#!/usr/bin/env bash
# MEMORIA Demo Packager (S68)
# 빌드 산출물(build/MEMORIA-Demo-v0.1.exe + .pck)을 zip으로 패키징.
# README.txt와 TESTER_GUIDE.md를 함께 묶어서 친구한테 보낼 수 있게 만듦.

set -e

VERSION="v0.1"
NAME="MEMORIA-Demo-${VERSION}"
BUILD_DIR="build"
STAGE_DIR="${BUILD_DIR}/stage"
ZIP_OUT="${BUILD_DIR}/${NAME}-Windows.zip"

# 색깔 출력
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${GREEN}=== MEMORIA Demo Packager ===${NC}"

# 1. 빌드 산출물 확인
if [ ! -f "${BUILD_DIR}/${NAME}.exe" ]; then
    echo -e "${RED}✗ Build not found: ${BUILD_DIR}/${NAME}.exe${NC}"
    echo "  → Run Godot Editor → Project → Export first."
    echo "  → See BUILD_GUIDE.md"
    exit 1
fi

if [ ! -f "${BUILD_DIR}/${NAME}.pck" ]; then
    echo -e "${RED}✗ PCK file not found: ${BUILD_DIR}/${NAME}.pck${NC}"
    echo "  → Did the export complete? Check Godot output."
    exit 1
fi

echo -e "${GREEN}✓ Build artifacts found${NC}"

# 2. 스테이징 디렉터리 준비
rm -rf "${STAGE_DIR}"
mkdir -p "${STAGE_DIR}"

# 3. 실행 파일 복사
cp "${BUILD_DIR}/${NAME}.exe" "${STAGE_DIR}/"
cp "${BUILD_DIR}/${NAME}.pck" "${STAGE_DIR}/"
echo -e "${GREEN}✓ Copied exe + pck${NC}"

# 4. 콘솔 래퍼는 디버그용이라 제외 (zip 부피 줄임)
# console.exe가 있다면 디버그 폴더에만 보관
if [ -f "${BUILD_DIR}/${NAME}.console.exe" ]; then
    mkdir -p "${BUILD_DIR}/debug"
    cp "${BUILD_DIR}/${NAME}.console.exe" "${BUILD_DIR}/debug/"
    echo -e "${YELLOW}  (console.exe → build/debug/, not in zip)${NC}"
fi

# 5. README.txt 생성 (zip 안)
cat > "${STAGE_DIR}/README.txt" <<EOF
MEMORIA: The Price of Oblivion — Demo (Act I — Ash)
Version: ${VERSION}
Build Date: $(date +%Y-%m-%d)

============================================
HOW TO PLAY
============================================
1. Extract this zip to a folder.
2. Double-click MEMORIA-Demo-${VERSION}.exe
3. If Windows Defender SmartScreen blocks:
   "More info" → "Run anyway"

============================================
CONTROLS
============================================
WASD / Arrows      Move
Space / Enter      Talk, advance dialogue
Tab / M            Memory Archive
ESC                Pause / Menu
F6 / F7            Quick Save / Load

============================================
PLAY TIME
============================================
~30 minutes (Act I)

============================================
FEEDBACK (PLEASE!)
============================================
Open TESTER_GUIDE.md and answer the questions.
Send to sbk8659@gmail.com or DM me.

Thank you for playing.

— MEMORIA Studio
EOF
echo -e "${GREEN}✓ README.txt generated${NC}"

# 6. TESTER_GUIDE.md 복사 (있으면)
if [ -f "TESTER_GUIDE.md" ]; then
    cp "TESTER_GUIDE.md" "${STAGE_DIR}/"
    echo -e "${GREEN}✓ Copied TESTER_GUIDE.md${NC}"
else
    echo -e "${YELLOW}! TESTER_GUIDE.md not found at project root — skipping${NC}"
fi

# 7. zip 생성
rm -f "${ZIP_OUT}"
cd "${STAGE_DIR}"

# Windows에서 zip 명령이 있는지 확인 (Git Bash엔 보통 있음)
if command -v zip >/dev/null 2>&1; then
    zip -r "../${NAME}-Windows.zip" . >/dev/null
elif command -v 7z >/dev/null 2>&1; then
    7z a -tzip "../${NAME}-Windows.zip" . >/dev/null
else
    echo -e "${RED}✗ No zip or 7z found in PATH${NC}"
    echo "  Install: pacman -S zip (Git Bash) or use 7-Zip"
    cd ..
    exit 1
fi

cd ../..

# 8. 결과
SIZE=$(du -h "${ZIP_OUT}" | cut -f1)
echo ""
echo -e "${GREEN}=== Done ===${NC}"
echo -e "Output: ${YELLOW}${ZIP_OUT}${NC}  (${SIZE})"
echo ""
echo "Next steps:"
echo "  1. Test locally: unzip and run on a clean folder"
echo "  2. Upload to WeTransfer / Google Drive / itch.io (private)"
echo "  3. Send link to 1~3 friends with TESTER_GUIDE.md instructions"
echo ""

# 9. 스테이징 폴더 정리
rm -rf "${STAGE_DIR}"
