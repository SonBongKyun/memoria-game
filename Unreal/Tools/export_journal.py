"""S327: extract the story journal's entry tables from scripts/ui/story_journal.gd.

S332 adds what its Quests and Losses tabs read: SideQuest.QUESTS (scripts/utils/side_quest.gd, without the
rewards) and the world rewrite's MEMORY_REWRITE_RULES and DEFAULT_LINES (scripts/systems/world_rewrite_director.gd).

The journal is derived from the run's story flags: each entry shows once its flag is set. This reads the
source tables (EVENT_ENTRIES, NPC_ENTRIES, WORLD_ENTRIES, the choice entries in _populate_choices,
EVENT_ART_BY_FLAG and CHAPTER_NAMES) and writes

  docs/unreal-migration/ir/journal/story_journal.v1.json            (reviewable)
  Unreal/Memoria/Source/Memoria/Private/Journal/MemoriaJournalSources.inl   (compiled in, ASCII)

Usage: python export_journal.py [--check]   (chapter names from scripts/core/game_manager.gd)
"""
import argparse
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / 'scripts/ui/story_journal.gd'
GAME_MANAGER = ROOT / 'scripts/core/game_manager.gd'
SIDE_QUEST = ROOT / 'scripts/utils/side_quest.gd'
WORLD_REWRITE = ROOT / 'scripts/systems/world_rewrite_director.gd'
# enum MemoryGrade { GRADE_5, GRADE_4, GRADE_3, GRADE_2, GRADE_1 }
GRADES = {'GRADE_5': 0, 'GRADE_4': 1, 'GRADE_3': 2, 'GRADE_2': 3, 'GRADE_1': 4}
OUT = ROOT / 'docs/unreal-migration/ir/journal/story_journal.v1.json'
INL = ROOT / 'Unreal/Memoria/Source/Memoria/Private/Journal/MemoriaJournalSources.inl'


def literal(src, start_pattern):
    """The bracketed GDScript literal that follows start_pattern, parsed as JSON."""
    m = re.search(start_pattern, src)
    if not m:
        raise ValueError('Missing ' + start_pattern)
    i = m.end() - 1
    opener = src[i]
    closer = {'[': ']', '{': '}'}[opener]
    depth, j, quote = 0, i, None
    while j < len(src):
        ch = src[j]
        if quote:
            if ch == '\\':
                j += 2
                continue
            if ch == quote:
                quote = None
        elif ch == '"':
            quote = ch
        elif ch == '#':
            j = src.index('\n', j)
            continue
        elif ch in '[{':
            depth += 1
        elif ch in ']}':
            depth -= 1
            if depth == 0:
                break
        j += 1
    text = src[i:j + 1]
    text = re.sub(r'#[^\n"]*\n', '\n', text)                 # comments outside strings (none contain quotes)
    text = re.sub(r'(\{|,)\s*(\d+)\s*:', r'\1 "\2":', text)   # int keys (CHAPTER_NAMES)
    text = re.sub(r',(\s*[\]}])', r'\1', text)               # trailing commas
    return json.loads(text)


def extract():
    src = SOURCE.read_text(encoding='utf-8').replace('\r\n', '\n')
    gm = GAME_MANAGER.read_text(encoding='utf-8').replace('\r\n', '\n')
    # localized_chapter_name: CHAPTER_NAMES_KO in Korean, RICH_PRESENCE_CHAPTERS in English (the S247 canon),
    # the journal's own CHAPTER_NAMES as the fallback.
    chapters = {int(k): v for k, v in literal(src, r'const CHAPTER_NAMES := \{').items()}
    chapters.update({int(k): v for k, v in literal(gm, r'const RICH_PRESENCE_CHAPTERS: Dictionary = \{').items()})
    chapters_ko = {int(k): v for k, v in literal(gm, r'const CHAPTER_NAMES_KO: Dictionary = \{').items()}
    art = literal(src, r'const EVENT_ART_BY_FLAG: Dictionary = \{')
    events = literal(src, r'const EVENT_ENTRIES := \[')
    npcs = literal(src, r'const NPC_ENTRIES := \[')
    world = literal(src, r'const WORLD_ENTRIES := \[')
    choices = literal(src, r'var choice_entries := \[')
    for e in events:
        # _update_journal_summary / _populate_events: the art table first, then the entry's own.
        if e['flag'] in art:
            e['art'] = art[e['flag']]
    # SideQuest.QUESTS: what get_all_quests and the journal read (the rewards stay with the quest's own port).
    quests = []
    for q in literal(SIDE_QUEST.read_text(encoding='utf-8').replace('\r\n', '\n'), r'const QUESTS: Array = \['):
        quests.append({'id': q['id'], 'title': q['title'], 'title_ko': q.get('title_ko', ''), 'desc': q['desc'], 'desc_ko': q.get('desc_ko', ''),
                       'art': q.get('art', ''), 'map': q['map'], 'npc': q.get('npc', ''), 'chapter_req': int(q['chapter_req']),
                       'prereq_flag': q.get('prereq_flag', ''),
                       'steps': [{'flag': s['flag'], 'desc': s['desc'], 'desc_ko': s.get('desc_ko', '')} for s in q['steps']]})
    # WorldRewriteDirector: the per-memory rules and the per-grade default lines behind get_loss_records.
    rewrite = WORLD_REWRITE.read_text(encoding='utf-8').replace('\r\n', '\n')
    rewrite = re.sub(r'Color\(([^)]*)\)', r'[\1]', rewrite)
    rewrite = re.sub(r'MemoryManager\.MemoryGrade\.(GRADE_\d)', r'"\1"', rewrite)
    rules = literal(rewrite, r'const MEMORY_REWRITE_RULES := \{')
    lines = {str(GRADES[k]): v for k, v in literal(rewrite, r'const DEFAULT_LINES := \{').items()}
    lines_ko = {str(GRADES[k]): v for k, v in literal(rewrite, r'const DEFAULT_LINES_KO := \{').items()}
    if sorted(lines) != ['0', '1', '2', '3', '4'] or sorted(lines_ko) != sorted(lines):
        raise ValueError('DEFAULT_LINES must cover the five grades in both languages')
    return {'schema_version': 1, 'source': 'scripts/ui/story_journal.gd',
            'chapters': {str(k): chapters[k] for k in sorted(chapters)},
            'chapters_ko': {str(k): chapters_ko[k] for k in sorted(chapters_ko)},
            'events': events, 'npcs': npcs, 'world': world, 'choices': choices,
            'quests': quests,
            'loss_rules': [dict(rule, memory=memory) for memory, rule in rules.items()],
            'loss_default_lines': lines, 'loss_default_lines_ko': lines_ko}


def inl(value):
    text = json.dumps(value, ensure_ascii=True, sort_keys=True, separators=(',', ':')).replace('\\', '\\\\').replace('"', '\\"')
    chunks, i = [], 0
    while i < len(text):
        j = min(i + 2000, len(text))
        while j < len(text) and text[j - 1] == '\\':
            j += 1
        chunks.append(text[i:j]); i = j
    # Separate literals joined at runtime: one concatenated wide literal would pass MSVC's 64 KB limit.
    lines = ['// Generated by Unreal/Tools/export_journal.py from scripts/ui/story_journal.gd; do not hand edit.',
             'static const TCHAR* const MemoriaJournalSourceParts[] = {']
    lines += ['    TEXT("%s"),' % c for c in chunks] + ['};']
    return '\n'.join(lines) + '\n'


def main():
    p = argparse.ArgumentParser(); p.add_argument('--check', action='store_true')
    a = p.parse_args()
    value = extract()
    outputs = {OUT: json.dumps(value, ensure_ascii=False, indent=1, sort_keys=True) + '\n', INL: inl(value)}
    if a.check:
        stale = [str(p) for p, t in outputs.items() if not p.exists() or p.read_text(encoding='utf-8') != t]
        print('MEMORIA_JOURNAL_' + ('STALE ' + ' '.join(stale) if stale else 'PASS'))
        return 1 if stale else 0
    for path, text in outputs.items():
        path.parent.mkdir(parents=True, exist_ok=True); path.write_text(text, encoding='utf-8', newline='\n')
    print('MEMORIA_JOURNAL_WRITTEN events=%d npcs=%d world=%d choices=%d quests=%d loss_rules=%d' % (
        len(value['events']), len(value['npcs']), len(value['world']), len(value['choices']), len(value['quests']), len(value['loss_rules'])))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
