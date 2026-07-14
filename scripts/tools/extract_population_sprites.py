"""Split one 3x2 chroma-key roster sheet into field-sized keyed PNGs.

The source image stays in Codex's generated-image store.  This helper only
produces temporary keyed crops; remove_chroma_key.py performs alpha conversion
before the results are moved into the game asset folders.
"""

from __future__ import annotations

import argparse
from pathlib import Path

from PIL import Image


def main() -> None:
    parser = argparse.ArgumentParser(description="Split a 3x2 population roster sheet.")
    parser.add_argument("source", type=Path)
    parser.add_argument("output_dir", type=Path)
    parser.add_argument("--ids", required=True, help="Six comma-separated output ids")
    args = parser.parse_args()

    ids = [value.strip() for value in args.ids.split(",") if value.strip()]
    if len(ids) != 6:
        raise SystemExit("--ids must contain exactly six ids")

    sheet = Image.open(args.source).convert("RGBA")
    args.output_dir.mkdir(parents=True, exist_ok=True)
    cell_width = sheet.width / 3.0
    cell_height = sheet.height / 2.0

    for index, sprite_id in enumerate(ids):
        column = index % 3
        row = index // 3
        # Preserve a rim of chroma key so alpha conversion keeps antialiased edges.
        left = round((column + 0.055) * cell_width)
        top = round((row + 0.045) * cell_height)
        right = round((column + 0.945) * cell_width)
        bottom = round((row + 0.955) * cell_height)
        crop = sheet.crop((left, top, right, bottom)).resize((128, 160), Image.Resampling.LANCZOS)
        crop.save(args.output_dir / f"{sprite_id}_key.png")


if __name__ == "__main__":
    main()
