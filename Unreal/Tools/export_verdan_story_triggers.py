"""Extract Verdan exploration story beats and the Sump Ledger quest from pinned Godot source.

Parses scenes/maps/verdan_market.gd _setup_exploration_events (authored order, Area2D
rect in pixels, dialogue group, one-time flag, entry guard) and _setup_side_quests
(Nervous Trader and ledger areas, requested groups), plus the sump_ledger entry of
scripts/utils/side_quest.gd QUESTS (steps, rewards). Field row counts come from
data/chapter2_dialogue.json. --check is byte-exact against the committed fixture;
the native tables are checked by Memoria.VerdanStory.SourceTable.
"""
from pathlib import Path
import argparse, json, re

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / 'scenes/maps/verdan_market.gd'
QUESTS = ROOT / 'scripts/utils/side_quest.gd'
DIALOGUE = ROOT / 'data/chapter2_dialogue.json'
DEST = ROOT / 'docs/unreal-migration/fixtures/verdan_story/source_triggers.v1.json'
CALL = re.compile(r'_add_story_trigger\(Vector2\((\d+) \* TILE_SIZE, (\d+) \* TILE_SIZE\), '
                  r'Vector2\(TILE_SIZE \* (\d+), TILE_SIZE \* (\d+)\), "(\w+)", "(\w+)"\)')
GUARD = re.compile(r'^\tif GameManager\.get_flag\("(\w+)"\):$')

def body(text, name):
    return re.search(r'^func ' + name + r'\(\) -> void:\n((?:\t.*\n|\n)*)', text, re.M).group(1)

def beats(text, tile, dialogues):
    result, guard = [], None
    for line in body(text, '_setup_exploration_events').splitlines():
        m = GUARD.match(line)
        if m:
            guard = m.group(1); continue
        m = CALL.search(line)
        if m:
            x, y, w, h, group, flag = m.groups()
            result.append({'group': group, 'flag': flag, 'requires_flag': guard if line.startswith('\t\t') else None,
                           'rect': [int(x) * tile, int(y) * tile, (int(x) + int(w)) * tile, (int(y) + int(h)) * tile],
                           'rows': len(dialogues[group])})
        elif not line.startswith('\t\t'):
            guard = None
    if len(result) != 5: raise ValueError('Unexpected Verdan story beat count: %d' % len(result))
    return result

def gd_literal(block):
    # A GDScript dictionary literal: drop line comments and trailing commas, then read as JSON.
    block = re.sub(r'#[^\n"]*$', '', block, flags=re.M)
    block = re.sub(r',(\s*[}\]])', r'\1', block)
    return json.loads(block)

def square(source, tile, node):
    # node.position = Vector2(x * TILE_SIZE + TILE_SIZE / 2.0, y * TILE_SIZE + TILE_SIZE / 2.0)
    m = re.search(node + r'\.position = Vector2\((\d+) \* TILE_SIZE \+ TILE_SIZE / 2\.0, (\d+) \* TILE_SIZE \+ TILE_SIZE / 2\.0\)', source)
    return int(m.group(1)) * tile + tile // 2, int(m.group(2)) * tile + tile // 2

def sump_ledger(text, tile, dialogues):
    quests = QUESTS.read_text(encoding='utf-8')
    start = quests.index('\t{\n\t\t"id": "sump_ledger"')
    end = quests.index('\n\t},', start) + 3
    quest = gd_literal(quests[start:end].strip().rstrip(','))
    source = body(text, '_setup_side_quests')
    trader = float(re.search(r'\trect\.size = Vector2\(TILE_SIZE \* ([\d.]+), TILE_SIZE \* [\d.]+\)', source).group(1)) * tile
    if not re.search(r'\tlr\.size = Vector2\(TILE_SIZE, TILE_SIZE\)', source): raise ValueError('Ledger area size changed')
    tx, ty = square(source, tile, 'area')
    lx, ly = square(source, tile, 'ledger_area')
    requested = re.findall(r'load_and_start\("res://data/chapter2_dialogue\.json", "(\w+)"\)', source)
    rect = lambda x, y, size: [int(x - size / 2), int(y - size / 2), int(x + size / 2), int(y + size / 2)]
    return {'id': quest['id'], 'title': quest['title'], 'title_ko': quest['title_ko'], 'chapter_req': quest['chapter_req'],
            'steps': [s['flag'] for s in quest['steps']], 'step_desc': [s['desc'] for s in quest['steps']],
            'step_desc_ko': [s['desc_ko'] for s in quest['steps']], 'reward_grains': quest['reward_grains'],
            'reward_items': quest['reward_items'], 'reward_memory': quest['reward_memory'],
            'trader_rect': rect(tx, ty, trader), 'ledger_rect': rect(lx, ly, tile),
            'requested_groups': requested, 'rows': {g: len(dialogues[g]) for g in requested}}

def extract():
    text = SOURCE.read_text(encoding='utf-8')
    tile = int(re.search(r'^const TILE_SIZE: int = (\d+)$', text, re.M).group(1))
    dialogues = json.loads(DIALOGUE.read_text(encoding='utf-8'))['dialogues']
    return {'schema_version': 1, 'source': 'scenes/maps/verdan_market.gd', 'tile_size': tile,
            'player_body': [14, 14], 'beats': beats(text, tile, dialogues), 'sump_ledger': sump_ledger(text, tile, dialogues)}

def main():
    p = argparse.ArgumentParser(); p.add_argument('--check', action='store_true'); a = p.parse_args()
    value = extract()
    data = (json.dumps(value, ensure_ascii=False, indent=1, sort_keys=True) + '\n').encode('utf-8')
    if a.check:
        if DEST.read_bytes() != data: raise SystemExit('MEMORIA_VERDAN_STORY_FIXTURE_DIFFERS')
    else:
        DEST.parent.mkdir(parents=True, exist_ok=True); DEST.write_bytes(data)
    print('MEMORIA_VERDAN_STORY_TRIGGERS_PASS beats=%d quest=%s' % (len(value['beats']), value['sump_ledger']['id']))

if __name__ == '__main__':
    main()
