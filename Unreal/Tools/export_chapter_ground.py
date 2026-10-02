#!/usr/bin/env python3
"""S337: the chapter maps' ground textures, cut from the source's painted map canvases.

The Godot maps hide their tiles under one painted canvas (MapEffects.add_map_canvas, terrain_alpha 0.0).
A flat painting cannot lie under the quarter-view camera (its props would lie flat), so the port takes
only the open ground from it: a soil patch and a paved patch per map, made to repeat, and the Belt's
round dial as a ground inlay. Output: Unreal/ArtSource/ChapterGround/*.png (512 px, sRGB).

    python Unreal/Tools/export_chapter_ground.py            # write
    python Unreal/Tools/export_chapter_ground.py --check    # fail if the files differ
"""
import argparse
import hashlib
import io
import sys
from pathlib import Path

from PIL import Image, ImageChops

ROOT = Path(__file__).resolve().parents[2]
CANVASES = ROOT / "assets" / "environment" / "map_canvases"
OUT = ROOT / "Unreal" / "ArtSource" / "ChapterGround"
SIZE = 512
# name: (canvas, box in canvas pixels). The boxes hold open ground only: no props, fences or lamps.
PATCHES = {
    "belt_soil": ("map_belt_waystation_canvas_v1.png", (222, 398, 414, 590)),
    "belt_paved": ("map_belt_waystation_canvas_v1.png", (275, 565, 467, 757)),
    "drift_soil": ("map_drift_shelter_canvas_v2.png", (300, 410, 492, 602)),
    "drift_paved": ("map_drift_shelter_canvas_v2.png", (745, 400, 905, 560)),
}
# The Belt canvas's central dial: a flat round inlay, kept whole with a soft round edge.
DIAL = ("map_belt_waystation_canvas_v1.png", (490, 555, 760, 825), 0.80, 0.94)


def seamless(image: Image.Image) -> Image.Image:
    """Blend the patch with itself rolled by half its size, so that its edges meet when it repeats.

    The rolled copy is continuous across the patch's border (its own seam runs through the middle), so it
    takes over towards the border and the original keeps the middle.
    """
    w, h = image.size
    rolled = ImageChops.offset(image, w // 2, h // 2)
    mask = Image.new("L", (w, h))
    px = mask.load()
    for y in range(h):
        for x in range(w):
            dx = min(x, w - 1 - x) / (w * 0.5)
            dy = min(y, h - 1 - y) / (h * 0.5)
            t = min(dx, dy)  # 0 at the border, 1 in the middle
            t = max(0.0, min(1.0, (t - 0.08) / 0.34))
            px[x, y] = int(255 * t * t * (3 - 2 * t))
    return Image.composite(image, rolled, mask)


def build() -> dict:
    files = {}
    for name, (canvas, box) in PATCHES.items():
        patch = Image.open(CANVASES / canvas).convert("RGB").crop(box)
        files[name] = seamless(patch).resize((SIZE, SIZE), Image.LANCZOS)
    canvas, box, inner, outer = DIAL
    dial = Image.open(CANVASES / canvas).convert("RGB").crop(box).resize((SIZE, SIZE), Image.LANCZOS)
    alpha = Image.new("L", (SIZE, SIZE))
    px = alpha.load()
    for y in range(SIZE):
        for x in range(SIZE):
            r = (((x + .5) / SIZE - .5) ** 2 + ((y + .5) / SIZE - .5) ** 2) ** .5 * 2
            t = max(0.0, min(1.0, (outer - r) / (outer - inner)))
            px[x, y] = int(255 * t * t * (3 - 2 * t))
    dial.putalpha(alpha)
    files["belt_dial"] = dial
    return files


def encode(image: Image.Image) -> bytes:
    data = io.BytesIO()
    image.save(data, "PNG", optimize=False)
    return data.getvalue()


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    failed = False
    for name, image in build().items():
        path = OUT / f"{name}.png"
        data = encode(image)
        if args.check:
            same = path.exists() and path.read_bytes() == data
            print(f"{'OK  ' if same else 'DIFF'} {path.relative_to(ROOT)}")
            failed |= not same
        else:
            OUT.mkdir(parents=True, exist_ok=True)
            path.write_bytes(data)
            mean = image.convert("RGB").resize((1, 1), Image.BOX).getpixel((0, 0))
            print(f"wrote {path.relative_to(ROOT)} sha256={hashlib.sha256(data).hexdigest()[:12]} mean={mean}")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
