"""Extract the game-ready Malet chibi rows from the canonical reference sheet.

The source is an art-direction board rather than a packed atlas. This keeps the
authored silhouettes, removes the connected parchment background, normalizes
the feet baseline, and mirrors the authored move row for left-facing movement.
"""

from pathlib import Path
from collections import deque
import colorsys
from PIL import Image, ImageOps

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "assets/game_image/reference/malet_sprite_sheet_reference.png"
OUTPUT = ROOT / "assets/sprites/characters/malet_sheet"
CANVAS = 160

ROWS = {
    "idle": [(96, 528, 120, 144), (202, 528, 120, 144), (307, 528, 120, 144), (413, 528, 120, 144)],
    "move": [(552, 528, 124, 144), (659, 528, 124, 144), (766, 528, 124, 144), (873, 528, 124, 144)],
    "cast": [(981, 528, 120, 144), (1087, 528, 120, 144), (1193, 528, 120, 144), (1299, 528, 120, 144)],
}


def remove_parchment(frame: Image.Image, include_effects: bool = False) -> Image.Image:
    rgba = frame.convert("RGBA")
    pixels = rgba.load()
    width, height = rgba.size
    mask = [[False] * width for _ in range(height)]
    for y in range(rgba.height):
        for x in range(rgba.width):
            r, g, b, _ = pixels[x, y]
            _, saturation, value = colorsys.rgb_to_hsv(r / 255, g / 255, b / 255)
            skin_tone = r < 232 and r - g > 8 and g - b > 5
            mask[y][x] = value < 0.72 or saturation > 0.22 or b > r + 12 or skin_tone

    seen = [[False] * width for _ in range(height)]
    components: list[list[tuple[int, int]]] = []
    for sy in range(height):
        for sx in range(width):
            if not mask[sy][sx] or seen[sy][sx]:
                continue
            queue = deque([(sx, sy)])
            seen[sy][sx] = True
            component: list[tuple[int, int]] = []
            while queue:
                x, y = queue.popleft()
                component.append((x, y))
                for nx in range(max(0, x - 1), min(width, x + 2)):
                    for ny in range(max(0, y - 1), min(height, y + 2)):
                        if mask[ny][nx] and not seen[ny][nx]:
                            seen[ny][nx] = True
                            queue.append((nx, ny))
            components.append(component)

    viable = [c for c in components if len(c) >= 12]
    if not viable:
        raise RuntimeError("No foreground found in Malet frame")
    center_x = width / 2
    main = max(viable, key=lambda c: len(c) - abs(sum(x for x, _ in c) / len(c) - center_x) * 2)
    min_x = min(x for x, _ in main)
    max_x = max(x for x, _ in main)
    min_y = min(y for _, y in main)
    max_y = max(y for _, y in main)
    keep: set[tuple[int, int]] = set(main)
    for component in viable:
        if component is main:
            continue
        cx = sum(x for x, _ in component) / len(component)
        cy = sum(y for _, y in component) / len(component)
        if include_effects and min_x - 42 <= cx <= max_x + 42 and min_y - 28 <= cy <= max_y + 30:
            keep.update(component)

    for y in range(height):
        for x in range(width):
            if (x, y) not in keep:
                r, g, b, _ = pixels[x, y]
                pixels[x, y] = (r, g, b, 0)
    alpha = rgba.getchannel("A")
    bbox = alpha.getbbox()
    if bbox is None:
        raise RuntimeError("No foreground found in Malet frame")
    return rgba.crop(bbox)


def normalize(frame: Image.Image) -> Image.Image:
    scale = min(128 / frame.height, 132 / frame.width)
    size = (max(1, round(frame.width * scale)), max(1, round(frame.height * scale)))
    resized = frame.resize(size, Image.Resampling.LANCZOS)
    canvas = Image.new("RGBA", (CANVAS, CANVAS), (0, 0, 0, 0))
    x = (CANVAS - resized.width) // 2
    y = 150 - resized.height
    canvas.alpha_composite(resized, (x, y))
    return canvas


def crop_at(source: Image.Image, spec: tuple[int, int, int, int], include_effects: bool = False) -> Image.Image:
    cx, top, width, height = spec
    box = (cx - width // 2, top, cx + width // 2, top + height)
    return normalize(remove_parchment(source.crop(box), include_effects))


def main() -> None:
    OUTPUT.mkdir(parents=True, exist_ok=True)
    source = Image.open(SOURCE).convert("RGB")
    extracted: dict[str, list[Image.Image]] = {}
    for animation, specs in ROWS.items():
        frames = [crop_at(source, spec, animation == "cast") for spec in specs]
        extracted[animation] = frames
        for index, frame in enumerate(frames, 1):
            frame.save(OUTPUT / f"{animation}_{index:02d}.png", optimize=True)
    for index, frame in enumerate(extracted["move"], 1):
        ImageOps.mirror(frame).save(OUTPUT / f"move_left_{index:02d}.png", optimize=True)
    print(f"MALET_SHEET_EXTRACT_PASS frames=16 output={OUTPUT}")


if __name__ == "__main__":
    main()
