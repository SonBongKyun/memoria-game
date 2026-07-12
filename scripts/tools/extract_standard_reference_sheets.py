"""Extract shared idle/move rows from canonical character reference boards."""

from pathlib import Path
from PIL import Image, ImageOps

from extract_malet_sheet import remove_parchment, normalize

ROOT = Path(__file__).resolve().parents[2]
REFERENCE = ROOT / "assets/game_image/reference"
OUTPUT_ROOT = ROOT / "assets/sprites/characters"
CHARACTERS = ("tobias", "kairos", "nera", "veil")

IDLE = [(112, 198, 154, 174), (274, 198, 154, 174), (436, 198, 154, 174), (598, 198, 154, 174)]
MOVE = [(774, 198, 168, 174), (942, 198, 168, 174), (1110, 198, 168, 174), (1278, 198, 168, 174)]


def crop(source: Image.Image, spec: tuple[int, int, int, int]):
    cx, top, width, height = spec
    region = source.crop((cx - width // 2, top, cx + width // 2, top + height))
    return normalize(remove_parchment(region))


def main() -> None:
    total = 0
    for character in CHARACTERS:
        source_path = REFERENCE / f"{character}_sprite_sheet_reference.png"
        if not source_path.exists():
            continue
        source = Image.open(source_path).convert("RGB")
        output = OUTPUT_ROOT / f"{character}_sheet"
        output.mkdir(parents=True, exist_ok=True)
        idle_frames = [crop(source, spec) for spec in IDLE]
        move_frames = [crop(source, spec) for spec in MOVE]
        for index, frame in enumerate(idle_frames, 1):
            frame.save(output / f"idle_{index:02d}.png", optimize=True)
        for index, frame in enumerate(move_frames, 1):
            frame.save(output / f"move_{index:02d}.png", optimize=True)
            ImageOps.mirror(frame).save(output / f"move_left_{index:02d}.png", optimize=True)
        total += 12
    print(f"STANDARD_SHEET_EXTRACT_PASS characters={len(CHARACTERS)} frames={total}")


if __name__ == "__main__":
    main()
