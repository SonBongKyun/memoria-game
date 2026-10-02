#!/usr/bin/env python3
"""S339: the field HUD's plates, cut from the source's painted UI art.

exploration_hud.gd, notification_toast.gd and battle_scene.gd lay these paintings behind their panels as they
are, black ground and all. Here each is cropped to the plate and the black ground outside it is made
transparent (a flood from the picture's edge through near-black pixels), so the plate can stand over the game.
Output: Unreal/ArtSource/Hud/*.png (RGBA).

    python Unreal/Tools/export_hud_art.py            # write
    python Unreal/Tools/export_hud_art.py --check    # fail if the files differ
"""
import argparse
import hashlib
import io
import sys
from collections import deque
from pathlib import Path

import numpy as np
from PIL import Image, ImageFilter

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "assets" / "cg" / "generated"
OUT = ROOT / "Unreal" / "ArtSource" / "Hud"
SEAL = 7  # pixels a frame's gaps are sealed by before the flood
# name: (source, crop box, output width, how dark counts as ground)
PLATES = {
    "hud_plate": ("ui_exploration_hud_plate.png", (16, 8, 1704, 884), 844, 16),
    "hud_toast": ("ui_notification_toast_frame.png", (100, 226, 1890, 624), 895, 14),
    "hud_ribbon": ("ui_battle_command_ribbon.png", (0, 190, 1983, 606), 1840, 12),
}


def cut(source: str, box, width: int, dark: int) -> Image.Image:
    image = Image.open(SOURCE / source).convert("RGB").crop(box)
    rgb = np.asarray(image).astype(np.int16)
    ground = rgb.max(axis=2) < dark
    h, w = ground.shape
    # The frames are thin lines with gaps, and the panel inside them is as dark as the ground outside. The lines
    # are thickened into a closed wall before the flood, so it stays outside; the flooded region is then grown
    # back by the same amount, through ground only, to reach the lines again.
    reach = 2 * SEAL + 1
    wall = np.asarray(Image.fromarray((~ground).astype(np.uint8) * 255, "L").filter(ImageFilter.MaxFilter(reach))) > 0
    open_ = ~wall
    outside = np.zeros((h, w), dtype=bool)
    queue = deque()
    for x in range(w):
        for y in (0, h - 1):
            if open_[y, x] and not outside[y, x]:
                outside[y, x] = True
                queue.append((y, x))
    for y in range(h):
        for x in (0, w - 1):
            if open_[y, x] and not outside[y, x]:
                outside[y, x] = True
                queue.append((y, x))
    while queue:
        y, x = queue.popleft()
        for ny, nx in ((y - 1, x), (y + 1, x), (y, x - 1), (y, x + 1)):
            if 0 <= ny < h and 0 <= nx < w and open_[ny, nx] and not outside[ny, nx]:
                outside[ny, nx] = True
                queue.append((ny, nx))
    grown = np.asarray(Image.fromarray(outside.astype(np.uint8) * 255, "L").filter(ImageFilter.MaxFilter(reach))) > 0
    outside = grown & ground
    alpha = Image.fromarray(np.where(outside, 0, 255).astype(np.uint8), "L").filter(ImageFilter.GaussianBlur(1.4))
    # The blur must not let the ground back in: keep the flooded region fully clear.
    alpha = Image.fromarray(np.where(outside, 0, np.asarray(alpha)).astype(np.uint8), "L")
    image.putalpha(alpha)
    height = round(image.height * width / image.width)
    return image.resize((width, height), Image.LANCZOS)


def encode(image: Image.Image) -> bytes:
    data = io.BytesIO()
    image.save(data, "PNG", optimize=False)
    return data.getvalue()


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    failed = False
    for name, (source, box, width, dark) in PLATES.items():
        image = cut(source, box, width, dark)
        data = encode(image)
        path = OUT / f"{name}.png"
        if args.check:
            same = path.exists() and path.read_bytes() == data
            print(f"{'OK  ' if same else 'DIFF'} {path.relative_to(ROOT)}")
            failed |= not same
        else:
            OUT.mkdir(parents=True, exist_ok=True)
            path.write_bytes(data)
            print(f"wrote {path.relative_to(ROOT)} {image.size} sha256={hashlib.sha256(data).hexdigest()[:12]}")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
