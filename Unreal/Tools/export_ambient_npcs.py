"""S331: draw the source's ambient NPC presets with the original procedural painter.

PixelSprite.create_npc_sprite(preset) builds a 48 px pixel figure from a colour config (create_frames ->
_draw_character). This runs that painter, unchanged, in an isolated disposable Godot project and saves each
preset's idle frames:

  Unreal/ArtSource/Ambient/<preset>_<direction>.png      (48x48 RGBA, the source's idle frame)
  docs/unreal-migration/evidence/ambient_npcs/export.json

Only the presets the ported chapter maps place are exported (PRESETS).

Usage: python export_ambient_npcs.py --godot <Godot console binary> [--work <dir>]
"""
from pathlib import Path
import argparse
import hashlib
import json
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / 'scripts/utils/pixel_sprite.gd'
OUTPUT = ROOT / 'Unreal/ArtSource/Ambient'
EVIDENCE = ROOT / 'docs/unreal-migration/evidence/ambient_npcs'
# belt_waystation.gd and drift_shelter.gd _setup_map_decorations. villager_f and villager_m resolve to authored
# field sprites (nera, malet) in create_npc_sprite, so they are not procedural and are not exported here.
PRESETS = ('traveler', 'bureau_agent', 'guard')
DIRECTIONS = ('down', 'up', 'left', 'right')
ENTRY = ('_draw_character', 'get_npc_preset')


def functions(source):
    """Every top-level static func of the file, by name."""
    found = {}
    for m in re.finditer(r'^static func (\w+)\(.*?(?=^(?:static func |static var |func |const |##|#)|\Z)', source, re.M | re.S):
        found[m.group(1)] = m.group(0).rstrip() + '\n'
    return found


def closure(funcs, entry):
    """The functions reachable from the entry points."""
    seen, todo = [], list(entry)
    while todo:
        name = todo.pop()
        if name in seen:
            continue
        if name not in funcs:
            raise ValueError('Missing source function ' + name)
        seen.append(name)
        for other in funcs:
            if other not in seen and re.search(r'\b' + re.escape(other) + r'\(', funcs[name].split('\n', 1)[1]):
                todo.append(other)
    return sorted(seen)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--godot', type=Path, required=True)
    ap.add_argument('--work', type=Path)
    args = ap.parse_args()
    raw = SOURCE.read_bytes()
    source = raw.decode('utf-8').replace('\r\n', '\n')
    funcs = functions(source)
    names = closure(funcs, ENTRY)
    constants = [line for line in source.splitlines() if re.match(r'const (SIZE|HALF): int = \d+$', line)]
    assert len(constants) == 2, constants
    work = args.work or Path(tempfile.mkdtemp(prefix='memoria_ambient_'))
    work.mkdir(parents=True, exist_ok=True)
    out = work / 'out'
    out.mkdir(exist_ok=True)
    (work / 'project.godot').write_text('config_version=5\n[application]\nconfig/name="IsolatedAmbientNpcs"\n', encoding='utf-8')
    body = '\nfunc _initialize() -> void:\n'
    body += '\tfor preset in %s:\n' % json.dumps(list(PRESETS))
    body += '\t\tvar config: Dictionary = get_npc_preset(preset)\n'
    body += '\t\tfor direction in %s:\n' % json.dumps(list(DIRECTIONS))
    body += '\t\t\tvar image: Image = _draw_character(config, direction, 0)\n'
    body += '\t\t\tassert(image != null and image.get_width() == SIZE and image.get_height() == SIZE)\n'
    body += '\t\t\tassert(image.save_png("%s/" + preset + "_" + direction + ".png") == OK)\n' % out.as_posix()
    body += '\tprint("AMBIENT_NPC_EXPORT_PASS %d presets")\n\tquit(0)\n' % len(PRESETS)
    harness = 'extends SceneTree\n' + '\n'.join(constants) + '\n\n' + '\n'.join(funcs[n] for n in names) + body
    (work / 'export.gd').write_text(harness, encoding='utf-8')
    command = [str(args.godot), '--headless', '--path', str(work), '--script', 'export.gd']
    result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=120)
    log = result.stdout
    if result.returncode != 0 or b'AMBIENT_NPC_EXPORT_PASS' not in log or re.search(rb'SCRIPT ERROR|Parse Error|ERROR:|FATAL', log):
        print(log.decode('utf-8', 'replace'))
        raise SystemExit('AMBIENT_NPC_EXPORT_FAILED')
    OUTPUT.mkdir(parents=True, exist_ok=True)
    EVIDENCE.mkdir(parents=True, exist_ok=True)
    outputs = {}
    for preset in PRESETS:
        for direction in DIRECTIONS:
            name = '%s_%s.png' % (preset, direction)
            data = (out / name).read_bytes()
            (OUTPUT / name).write_bytes(data)
            outputs[name] = hashlib.sha256(data).hexdigest()
    (EVIDENCE / 'export_harness.gd').write_text(harness.replace(out.as_posix(), '<out>'), encoding='utf-8', newline='\n')
    report = {'status': 'PASS', 'source': SOURCE.relative_to(ROOT).as_posix(), 'source_sha256': hashlib.sha256(raw).hexdigest(),
              'source_functions': names, 'presets': list(PRESETS), 'directions': list(DIRECTIONS), 'outputs': outputs}
    (EVIDENCE / 'export.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8', newline='\n')
    print('AMBIENT_NPC_EXPORT_PASS', len(outputs), 'files')


if __name__ == '__main__':
    main()
