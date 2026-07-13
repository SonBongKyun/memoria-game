"""Extract the six runtime consumable icons from the GPT Image item sheet.

The source sheet is intentionally kept outside tracked assets (tmp/imagegen) because
it has a chroma-key background.  This utility leaves a small border around each
illustration so remove_chroma_key.py can create clean, reusable PNG icons.
"""

from __future__ import annotations

import argparse
from pathlib import Path

from PIL import Image


ICON_IDS = (
    "potion",
    "hi_potion",
    "antidote",
    "firebomb",
    "smoke_bomb",
    "grains",
)


def main() -> None:
    parser = argparse.ArgumentParser(description="Split the 3x2 MEMORIA item sheet.")
    parser.add_argument("source", type=Path, help="3x2 chroma-key item sheet")
    parser.add_argument("output_dir", type=Path, help="Directory for keyed crops")
    args = parser.parse_args()

    sheet = Image.open(args.source).convert("RGBA")
    args.output_dir.mkdir(parents=True, exist_ok=True)
    cell_width = sheet.width / 3.0
    cell_height = sheet.height / 2.0

    for index, icon_id in enumerate(ICON_IDS):
        column = index % 3
        row = index // 3
        # Keep a little magenta around every icon for clean anti-aliased keying.
        left = round((column + 0.075) * cell_width)
        top = round((row + 0.055) * cell_height)
        right = round((column + 0.925) * cell_width)
        bottom = round((row + 0.945) * cell_height)
        crop = sheet.crop((left, top, right, bottom))
        crop.save(args.output_dir / f"{icon_id}_key.png")


if __name__ == "__main__":
    main()
