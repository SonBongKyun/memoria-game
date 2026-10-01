"""S320: extract a chapter field map from its Godot map script (content-first chapter port).

The chapter maps share one authored shape: a tile grid, an atmosphere budget, the arrival chain of
dialogue groups gated by flags, story triggers, an exit that closes the chapter, and post-chapter
chests, clues, one-time battles and a random encounter pool. This reads those declarations from
scenes/maps/<map>.gd and writes

  docs/unreal-migration/ir/chapters/<map>.v1.json     (reviewable)
  Unreal/Memoria/Source/Memoria/Private/Chapter/MemoriaChapterMapSources.inl   (compiled in)

Usage: python export_chapter_maps.py [--check] [map ...]   (default: every map in MAPS)
"""
import argparse
import ast
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'docs/unreal-migration/ir/chapters'
INL = ROOT / 'Unreal/Memoria/Source/Memoria/Private/Chapter/MemoriaChapterMapSources.inl'
# map id -> (chapter, dialogue file group prefix used for the Unreal assets)
MAPS = {'belt_waystation': (3, 'Ch3'), 'drift_shelter': (4, 'Ch4')}


def num(expr, tile):
    """Evaluate the small arithmetic the maps use: 3 * TILE_SIZE, TILE_SIZE * 2, 23.5 * TILE_SIZE."""
    expr = expr.replace('TILE_SIZE', str(tile)).strip()
    tree = ast.parse(expr, mode='eval')
    for node in ast.walk(tree):
        if not isinstance(node, (ast.Expression, ast.BinOp, ast.UnaryOp, ast.Constant, ast.Add, ast.Sub, ast.Mult, ast.Div, ast.USub)):
            raise ValueError('Unsupported expression: ' + expr)
    return float(eval(compile(tree, '<map>', 'eval')))


def vec(text, tile):
    m = re.fullmatch(r'\s*Vector2\((.+?),(.+)\)\s*', text)
    if not m:
        raise ValueError('Not a Vector2: ' + text)
    return [num(m.group(1), tile), num(m.group(2), tile)]


def color(text):
    parts = [float(v) for v in re.findall(r'[-\d.]+', text)]
    return (parts + [1.0])[:4] if len(parts) >= 3 else None


class V:
    """A Vector2 for evaluating the map scripts' position arithmetic."""
    def __init__(self, x, y):
        self.x, self.y = float(x), float(y)

    def __add__(self, other):
        return V(self.x + other.x, self.y + other.y)

    def __sub__(self, other):
        return V(self.x - other.x, self.y - other.y)

    def __mul__(self, k):
        return V(self.x * k, self.y * k)

    __rmul__ = __mul__

    def list(self):
        return [self.x, self.y]


class Prop:
    """A decoration being read; later lines may refer to its position (light.position = fire.position + ...)."""


def evaluate(expr, env):
    tree = ast.parse(expr.strip(), mode='eval')
    allowed = (ast.Expression, ast.BinOp, ast.UnaryOp, ast.Constant, ast.Add, ast.Sub, ast.Mult, ast.Div, ast.USub,
               ast.Name, ast.Load, ast.Call, ast.Attribute)
    for node in ast.walk(tree):
        if not isinstance(node, allowed) or (isinstance(node, ast.Call) and getattr(node.func, 'id', '') != 'Vector2'):
            raise ValueError('Unsupported decoration expression: ' + expr)
    return eval(compile(tree, '<decoration>', 'eval'), {'__builtins__': {}}, env)


def decorations(body, tile):
    """_setup_map_decorations: every ColorRect (kind: the source's variable name) and PointLight2D it adds
    outside a gate, in source pixels. Gated blocks (the ambient NPCs) are read separately."""
    lines = [(len(l) - len(l.lstrip('\t')), l.strip()) for l in body.split('\n') if l.strip() and not l.strip().startswith('#')]
    out = []

    def run(block, env):
        i = 0
        while i < len(block):
            indent, text = block[i]
            j = i + 1
            while j < len(block) and block[j][0] > indent:
                j += 1
            loop = re.match(r'for (\w+) in range\((\d+)\):', text)
            each = re.match(r'for (\w+) in \[(.*)\]:', text)
            if loop or each:
                values = range(int(loop.group(2))) if loop else [evaluate(v, env) for v in split_args(each.group(2))]
                for value in values:
                    run(block[i + 1:j], dict(env, **{(loop or each).group(1): value}))
                i = j
                continue
            if text.startswith('if '):
                i = j
                continue
            new = re.match(r'var (\w+) = (ColorRect|PointLight2D)\.new\(\)', text)
            if new:
                prop = Prop()
                prop.record = {'kind': new.group(1), 'light': new.group(2) == 'PointLight2D'}
                env[new.group(1)] = prop
                out.append(prop.record)
            field = re.match(r'(\w+)\.(size|position|color|rotation|energy|texture_scale) = (.+)', text)
            if field and isinstance(env.get(field.group(1)), Prop):
                prop, key, value = env[field.group(1)], field.group(2), field.group(3)
                if key == 'color':
                    prop.record['color'] = color(value)
                elif key in ('size', 'position'):
                    vector = evaluate(value, env)
                    setattr(prop, key, vector)
                    prop.record['origin' if key == 'position' else 'size'] = vector.list()
                else:
                    prop.record['scale' if key == 'texture_scale' else key] = float(value)
            i += 1

    run(lines, {'Vector2': V, 'TILE_SIZE': tile})
    for record in out:
        if 'origin' not in record or 'color' not in record:
            raise ValueError('Incomplete decoration: ' + repr(record))
    return out


def func_body(src, name):
    m = re.search(r'^func ' + re.escape(name) + r'\(.*?\).*?:\n((?:\t.*\n|\s*\n)*)', src, re.M)
    return m.group(1) if m else ''


def split_args(text):
    """Split a call's argument list at top-level commas."""
    out, depth, cur, quote = [], 0, '', None
    for ch in text:
        if quote:
            cur += ch
            if ch == quote:
                quote = None
            continue
        if ch in '"\'':
            quote = ch
        elif ch in '([{':
            depth += 1
        elif ch in ')]}':
            depth -= 1
        if ch == ',' and depth == 0:
            out.append(cur.strip()); cur = ''
        else:
            cur += ch
    if cur.strip():
        out.append(cur.strip())
    return out


def calls(body, name):
    """Every call of `name(...)` in a body, with its raw argument text (balanced parentheses)."""
    found = []
    for m in re.finditer(re.escape(name) + r'\(', body):
        i, depth = m.end(), 1
        while depth and i < len(body):
            depth += {'(': 1, ')': -1}.get(body[i], 0); i += 1
        found.append((m.start(), body[m.end():i - 1]))
    return found


def gate_of(body, position):
    """The `if GameManager.get_flag("x"):` that encloses a call, when the call is indented under it."""
    line_start = body.rfind('\n', 0, position) + 1
    indent = len(body[line_start:position]) - len(body[line_start:position].lstrip('\t'))
    for line in reversed(body[:line_start].split('\n')):
        stripped = line.lstrip('\t')
        level = len(line) - len(stripped)
        if stripped and level < indent:
            m = re.match(r'if GameManager\.get_flag\("([^"]+)"\):', stripped)
            return m.group(1) if m else None
    return None


def extract(map_id):
    chapter, prefix = MAPS[map_id]
    path = ROOT / 'scenes/maps' / (map_id + '.gd')
    src = path.read_text(encoding='utf-8').replace('\r\n', '\n')
    tile = int(re.search(r'const TILE_SIZE: int = (\d+)', src).group(1))
    width = int(re.search(r'const MAP_WIDTH: int = (\d+)', src).group(1))
    height = int(re.search(r'const MAP_HEIGHT: int = (\d+)', src).group(1))
    grid = re.search(r'var map_data: Array = \[(.*?)\n\]', src, re.S).group(1)
    tiles = [[int(v) for v in row.split(',') if v.strip()] for row in re.findall(r'\[([\d,\s]+)\]', grid)]
    if len(tiles) != height or any(len(r) != width for r in tiles):
        raise ValueError('map_data does not match MAP_WIDTH x MAP_HEIGHT')
    defs = re.search(r'_tile_defs = \[(.*?)\n\t\]', src, re.S).group(1)
    tile_defs = [{'color': color(c), 'detail': d} for c, d in re.findall(r'"color": (Color\([^)]*\)), "detail": "([^"]+)"', defs)]
    enum = re.search(r'enum Tile \{([^}]*)\}', src).group(1)
    tile_names = [t.strip() for t in enum.split(',') if t.strip()]
    collide_names = re.search(r'add_collisions\([^\[]*\[([^\]]*)\]', src).group(1)
    solid = [tile_names.index(n.strip().removeprefix('Tile.')) for n in collide_names.split(',') if n.strip()]
    atmosphere = {}
    body = re.search(r'var atmosphere := \{(.*?)\n\t\}', src, re.S)
    if body:
        for key, value in re.findall(r'"(\w+)": ([^\n]+?),?\n', body.group(1) + '\n'):
            value = value.rstrip(',')
            atmosphere[key] = color(value) if value.startswith('Color') else json.loads(value)
    title = re.search(r'show_chapter_title\(self, (\d+), "([^"]*)", "([^"]*)"\)', src)
    spawn = re.search(r'player\.position = (Vector2\([^)]*\))', func_body(src, '_position_player'))
    repeat = re.search(r'elia\.repeat_line = "([^"]*)"', src)
    dialogue_file = re.search(r'const DIALOGUE_FILE: String = "res://([^"]+)"', src).group(1)

    def handler_effects(name):
        """Flags, toasts and follow-ups a dialogue_ended handler performs."""
        body = func_body(src, name)
        return {'flags': re.findall(r'GameManager\.set_flag\("([^"]+)"', body),
                'toasts': re.findall(r'show_toast\("([^"]+)"', body)}

    # The arrival chain: _ready_sequence lists the flags; each _start_* sets its flag and starts its group.
    sequence = []
    ready = func_body(src, '_ready_sequence')
    for flag, starter in re.findall(r'get_flag\("([^"]+)"\):\n(?:\t\t.*\n)*?\t\t(_start_\w+)\(\)', ready):
        body = func_body(src, starter)
        group = re.search(r'load_and_start\(DIALOGUE_FILE, "([^"]+)"\)', body)
        handler = re.search(r'dialogue_ended\.connect\((\w+)', body)
        after = handler_effects(handler.group(1)) if handler else {'flags': [], 'toasts': []}
        # MemoryManager.add_chapter_memories(N): the memories the chapter brings come with this step.
        memories = re.search(r'MemoryManager\.add_chapter_memories\((\d+)\)', body)
        sequence.append({'flag': flag, 'group': group.group(1) if group else None,
                         'memories_chapter': int(memories.group(1)) if memories else None, **after})
    # The exit: a trigger area, the flag that opens it, and the departure that closes the chapter.
    exit_body = func_body(src, '_setup_exit_trigger')
    exit_rect = {'center': vec(re.search(r'area\.position = (Vector2\([^\n]*\))\n', exit_body).group(1), tile),
                 'size': vec(re.search(r'rect\.size = (Vector2\([^\n]*\))\n', exit_body).group(1), tile)}
    requires = re.search(r'GameManager\.get_flag\("([^"]+)"\) and not GameManager\.get_flag\("([^"]+)"\)', exit_body)
    depart_name = re.search(r'\t\t\t(_depart_\w+)\(\)', exit_body).group(1)
    depart = func_body(src, depart_name)
    depart_handler = re.search(r'dialogue_ended\.connect\((\w+)', depart).group(1)
    ended = func_body(src, depart_handler)
    # Where the road goes: another chapter map (belt_waystation.gd) or a story scene through a
    # scene constant and an _enter_* helper (drift_shelter.gd enters Chapter 5's classifier scene).
    next_map = re.search(r'"res://scenes/maps/(\w+)\.tscn"', ended)
    next_scene = None
    if not next_map:
        enter = re.search(r'\t(_enter_\w+)\(\)', ended)
        const = re.search(r'change_scene\w*\((\w+)\)', func_body(src, enter.group(1))).group(1) if enter else None
        scene = re.search(r'const ' + const + r': String = "res://([^"]+)"', src) if const else None
        next_scene = scene.group(1) if scene else None
    # The notice raised as the chapter closes, in both locales (drift_shelter.gd's boundary text).
    notice = re.search(r'var (\w+) := "([^"]+)"\n(?:\t.*\n)*?\tif GameManager\.current_locale == "ko":\n\t\t\1 = "([^"]+)"', ended)
    exit_def = dict(exit_rect, requires=requires.group(1), completes=requires.group(2),
                    group=re.search(r'load_and_start\(DIALOGUE_FILE, "([^"]+)"\)', depart).group(1),
                    next_chapter=int(re.search(r'current_chapter = (\d+)', ended).group(1)),
                    next_map=next_map.group(1) if next_map else None, next_scene=next_scene,
                    flags=re.findall(r'GameManager\.set_flag\("([^"]+)"', ended),
                    notice={'en': notice.group(2), 'ko': notice.group(3)} if notice else None)
    triggers = []
    events = func_body(src, '_setup_exploration_events')
    for position, args in calls(events, '_add_story_trigger'):
        a = split_args(args)
        triggers.append({'rect': {'origin': vec(a[0], tile), 'size': vec(a[1], tile)}, 'group': ast.literal_eval(a[2]),
                         'flag': ast.literal_eval(a[3]), 'gate': gate_of(events, position)})
    objects = func_body(src, '_setup_interactive_objects')
    resume_blocked = []

    def section_gate(setup):
        """The flag that opens a post-chapter section: `if GameManager.get_flag("x"):` over its setup call
        (belt_waystation.gd), or a `_can_resume_*()` guard requiring x and blocking later canon flags."""
        m = re.search(r'if GameManager\.get_flag\("([^"]+)"\):\n\t\t' + setup, src)
        if m:
            return m.group(1)
        m = re.search(r'if (_can_resume_\w+)\(\):\n(?:\t\t.*\n)*?\t\t' + setup, src)
        if not m:
            return None
        guard = func_body(src, m.group(1))
        blocked = re.findall(r'not GameManager\.get_flag\("([^"]+)"\)', guard)
        resume_blocked[:] = sorted(set(resume_blocked) | set(blocked))
        return [f for f in re.findall(r'GameManager\.get_flag\("([^"]+)"\)', guard) if f not in blocked][0]

    object_gate = section_gate('_setup_interactive_objects')
    chests, clues = [], []
    for _, args in calls(objects, '_add_chest'):
        a = split_args(args)
        rewards = a[2].replace('{', '{').replace('}', '}')
        chests.append({'origin': vec(a[0], tile), 'flag': ast.literal_eval(a[1]), 'rewards': json.loads(rewards)})
    for _, args in calls(objects, '_add_clue'):
        a = split_args(args)
        clues.append({'origin': vec(a[0], tile), 'flag': ast.literal_eval(a[1]), 'text': ast.literal_eval(a[2])})
    battles = []
    battle_gate = section_gate('_setup_battle_triggers')
    for _, args in calls(func_body(src, '_setup_battle_triggers'), '_add_battle_area'):
        a = split_args(args)
        battles.append({'rect': {'origin': vec(a[0], tile), 'size': vec(a[1], tile)}, 'name': ast.literal_eval(a[2]),
                        'hp': int(a[3]), 'atk': int(a[4]), 'is_void': a[5].strip() == 'true'})
    pool = []
    encounters = func_body(src, '_setup_random_encounters')
    encounter_gate = re.search(r'if not GameManager\.get_flag\("([^"]+)"\):\n\t\treturn', encounters)
    for entry in re.findall(r'\{"name": [^}]*\}', encounters):
        pool.append(json.loads(entry.replace('false', 'false').replace('true', 'true')))
    # RandomEncounter.setup(pool, scene, bg, enemy, min_steps, max_steps).
    encounter_range = re.search(r',\s*(\d+),\s*(\d+)\s*\)\s*$', encounters.strip())
    # _setup_map_decorations: the props, and the ambient NPCs with the gate that shows them.
    decor = func_body(src, '_setup_map_decorations')
    npcs = [{'position': [(v + .5) * tile for v in vec(p, 1)], 'preset': preset}
            for p, preset in re.findall(r'\{"pos": (Vector2\([^)]*\)), "preset": "(\w+)"\}', decor)]
    npc_gate = re.search(r'if GameManager\.get_flag\("([^"]+)"\):\n\t\tvar ambient_npcs', decor)
    if npc_gate:
        npc_gate = npc_gate.group(1)
    else:
        guard = re.search(r'if (_can_resume_\w+)\(\):\n\t\tvar ambient_npcs', decor)
        if guard:
            guard_body = func_body(src, guard.group(1))
            blocked = re.findall(r'not GameManager\.get_flag\("([^"]+)"\)', guard_body)
            resume_blocked[:] = sorted(set(resume_blocked) | set(blocked))
            npc_gate = [f for f in re.findall(r'GameManager\.get_flag\("([^"]+)"\)', guard_body) if f not in blocked][0]
    return {
        'schema_version': 1, 'map': map_id, 'chapter': chapter, 'asset_prefix': prefix,
        'source': 'scenes/maps/' + map_id + '.gd', 'dialogue_file': dialogue_file,
        'title': {'number': int(title.group(1)), 'name': title.group(2), 'subtitle': title.group(3)} if title else None,
        'tile_size': tile, 'width': width, 'height': height, 'tiles': tiles,
        'tile_defs': tile_defs, 'tile_names': tile_names, 'solid': solid,
        'atmosphere': atmosphere, 'spawn': vec(spawn.group(1), tile), 'elia_repeat': repeat.group(1) if repeat else '',
        'sequence': sequence, 'exit': exit_def, 'triggers': triggers,
        'objects_gate': object_gate, 'chests': chests, 'clues': clues,
        'battles_gate': battle_gate, 'battles': battles, 'resume_blocked': resume_blocked,
        'encounters_gate': encounter_gate.group(1) if encounter_gate else None, 'encounters': pool,
        'encounter_range': [int(encounter_range.group(1)), int(encounter_range.group(2))] if encounter_range else None,
        'decorations': decorations(decor, tile), 'ambient_npcs': npcs, 'ambient_npcs_gate': npc_gate,
    }


def canonical(value):
    return json.dumps(value, ensure_ascii=False, sort_keys=True, separators=(',', ':'))


def inl(values):
    lines = ['// Generated by Unreal/Tools/export_chapter_maps.py from scenes/maps/*.gd; do not hand edit.',
             '// Each map is its IR JSON (docs/unreal-migration/ir/chapters), one literal per map.']
    for map_id, value in values.items():
        # ASCII only: MSVC reads the source in the system code page, so Korean text travels as JSON \u escapes.
        text = json.dumps(value, ensure_ascii=True, sort_keys=True, separators=(',', ':')).replace('\\', '\\\\').replace('"', '\\"')
        # Chunks never split an escape (a chunk ending in a lone backslash would escape the closing quote).
        chunks, i = [], 0
        while i < len(text):
            j = min(i + 2000, len(text))
            while j < len(text) and text[j - 1] == '\\':
                j += 1
            chunks.append(text[i:j]); i = j
        lines.append('static const TCHAR* MemoriaChapterMap_%s =' % map_id)
        lines += ['    TEXT("%s")' % c for c in chunks[:-1]] + ['    TEXT("%s");' % chunks[-1]]
    lines.append('static const TPair<const TCHAR*, const TCHAR*> MemoriaChapterMapSources[] = {')
    lines += ['    {TEXT("%s"), MemoriaChapterMap_%s},' % (m, m) for m in values]
    lines.append('};')
    return '\n'.join(lines) + '\n'


def main():
    p = argparse.ArgumentParser(); p.add_argument('--check', action='store_true'); p.add_argument('maps', nargs='*')
    a = p.parse_args()
    values = {m: extract(m) for m in (a.maps or MAPS)}
    outputs = {OUT / (m + '.v1.json'): json.dumps(v, ensure_ascii=False, indent=1, sort_keys=True) + '\n' for m, v in values.items()}
    outputs[INL] = inl({m: extract(m) for m in MAPS})
    if a.check:
        stale = [str(p) for p, text in outputs.items() if not p.exists() or p.read_text(encoding='utf-8') != text]
        print('MEMORIA_CHAPTER_MAPS_' + ('STALE ' + ' '.join(stale) if stale else 'PASS'))
        return 1 if stale else 0
    for path, text in outputs.items():
        path.parent.mkdir(parents=True, exist_ok=True); path.write_text(text, encoding='utf-8', newline='\n')
    print('MEMORIA_CHAPTER_MAPS_WRITTEN ' + ' '.join(values))
    return 0


if __name__ == '__main__':
    sys.exit(main())
