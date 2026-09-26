"""Extract Verdan exploration story beats from the pinned Godot source.

Parses scenes/maps/verdan_market.gd _setup_exploration_events: authored order,
Area2D rect (pixels, TILE_SIZE), dialogue group, one-time flag and the entry
guard. Field row counts come from data/chapter2_dialogue.json. --check is
byte-exact against the committed fixture; the native table is checked by
Memoria.VerdanStory.SourceTable.
"""
from pathlib import Path
import argparse, json, re

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / 'scenes/maps/verdan_market.gd'
DIALOGUE = ROOT / 'data/chapter2_dialogue.json'
DEST = ROOT / 'docs/unreal-migration/fixtures/verdan_story/source_triggers.v1.json'
CALL = re.compile(r'_add_story_trigger\(Vector2\((\d+) \* TILE_SIZE, (\d+) \* TILE_SIZE\), '
                  r'Vector2\(TILE_SIZE \* (\d+), TILE_SIZE \* (\d+)\), "(\w+)", "(\w+)"\)')
GUARD = re.compile(r'^\tif GameManager\.get_flag\("(\w+)"\):$')

def extract():
    text = SOURCE.read_text(encoding='utf-8')
    tile = int(re.search(r'^const TILE_SIZE: int = (\d+)$', text, re.M).group(1))
    body = re.search(r'^func _setup_exploration_events\(\) -> void:\n((?:\t.*\n|\n)*)', text, re.M).group(1)
    dialogues = json.loads(DIALOGUE.read_text(encoding='utf-8'))['dialogues']
    beats, guard = [], None
    for line in body.splitlines():
        if m := GUARD.match(line):
            guard = m.group(1); continue
        if m := CALL.search(line):
            x, y, w, h, group, flag = m.groups()
            nested = line.startswith('\t\t')
            beats.append({'group': group, 'flag': flag, 'requires_flag': guard if nested else None,
                          'rect': [int(x) * tile, int(y) * tile, (int(x) + int(w)) * tile, (int(y) + int(h)) * tile],
                          'rows': len(dialogues[group])})
        if not line.startswith('\t\t'): guard = None if not GUARD.match(line) else guard
    if len(beats) != 5: raise ValueError('Unexpected Verdan story beat count: %d' % len(beats))
    return {'schema_version': 1, 'source': 'scenes/maps/verdan_market.gd', 'tile_size': tile,
            'player_body': [14, 14], 'beats': beats}

def main():
    p = argparse.ArgumentParser(); p.add_argument('--check', action='store_true'); a = p.parse_args()
    data = (json.dumps(extract(), ensure_ascii=False, indent=1, sort_keys=True) + '\n').encode('utf-8')
    if a.check:
        if DEST.read_bytes() != data: raise SystemExit('MEMORIA_VERDAN_STORY_FIXTURE_DIFFERS')
    else:
        DEST.parent.mkdir(parents=True, exist_ok=True); DEST.write_bytes(data)
    print('MEMORIA_VERDAN_STORY_TRIGGERS_PASS beats=%d' % len(extract()['beats']))

if __name__ == '__main__':
    main()
