"""Instance the bundled variable fonts at the weights the Godot UITheme requests.

Unreal renders a variable font at its default instance, which for both bundled
fonts is the thinnest master (Sans 100, Serif 200). ui_theme.gd requests wght axis
instances instead (S230): body Serif 500, titles Serif 600, interface Sans 600.
Faces are subset to Latin, punctuation, symbols, Hangul and CJK punctuation, plus
every character in the game data (a few Hanja), dropping the ~14k unused Hanja.
The static faces are written under Saved/ (not committed); the imported font
assets are the committed artifact. Usage: generate_font_sources.py [--out DIR]
"""
from pathlib import Path
import argparse
import hashlib

from fontTools import subset
from fontTools.ttLib import TTFont
from fontTools.varLib import instancer

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'Unreal/Memoria/Saved/FontSources'
FACES = {
    # face name: (source, wght) — ui_theme.gd BODY_WEIGHT, TITLE_WEIGHT, UI_WEIGHT
    'MemoriaSerifMedium': ('assets/fonts/NotoSerifKR-VF.ttf', 500),
    'MemoriaSerifSemiBold': ('assets/fonts/NotoSerifKR-VF.ttf', 600),
    'MemoriaSansSemiBold': ('assets/fonts/NotoSansKR-VF.ttf', 600),
}


RANGES = [(0x20, 0x24F), (0x2000, 0x206F), (0x20A0, 0x20CF), (0x2100, 0x214F), (0x2190, 0x21FF), (0x2460, 0x24FF),
          (0x2500, 0x25FF), (0x2600, 0x27BF), (0x1100, 0x11FF), (0x3000, 0x303F), (0x3130, 0x318F), (0xAC00, 0xD7A3),
          (0xFF00, 0xFFEF)]


def unicodes():
    codes = {c for a, b in RANGES for c in range(a, b + 1)}
    for f in sorted((ROOT / 'data').rglob('*.json')):
        codes |= {ord(ch) for ch in f.read_text(encoding='utf-8') if ord(ch) >= 0x20}
    return codes


def main():
    p = argparse.ArgumentParser(); p.add_argument('--out', type=Path, default=OUT); a = p.parse_args()
    a.out.mkdir(parents=True, exist_ok=True)
    for name, (source, weight) in FACES.items():
        font = instancer.instantiateVariableFont(TTFont(ROOT / source), {'wght': weight}, updateFontNames=False)
        options = subset.Options(); options.layout_features = ['*']; options.name_IDs = ['*']; options.notdef_outline = True
        sub = subset.Subsetter(options); sub.populate(unicodes=unicodes()); sub.subset(font)
        target = a.out / (name + '.ttf')
        font.save(target)
        print('MEMORIA_FONT_SOURCE %s wght=%d bytes=%d sha256=%s' % (name, weight, target.stat().st_size, hashlib.sha256(target.read_bytes()).hexdigest()[:16]))


if __name__ == '__main__':
    main()
