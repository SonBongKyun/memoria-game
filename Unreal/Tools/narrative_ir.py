"""Bounded narrative v1: distinct dialects; strict source-attested extraction.

Canonical primitives are shared with Phase 1C. Fade seconds become exact integer
milliseconds (no rounding); authored key presence and all text are retained.
"""
from pathlib import Path
import copy
import json
import re
import subprocess
from decimal import Decimal
from starting_memory_ir import ROOT, canonical, sha, source_bytes, strict_load, exact, text

BASE = ROOT / 'docs/unreal-migration/ir/narrative'
VERSION = 'memoria-narrative/1'
CASES = {
    'vn': ('data/vn_scenes/ch2_market_arrival.json', 'ch2_market_arrival', 'DA_VN_Ch2MarketArrival'),
    'field': ('data/chapter2_dialogue.json', 'verdan_arrival', 'DA_Field_VerdanArrival'),
}
FIELD_CASES = {
    'verdan_arrival': (5, 0, 'DA_Field_VerdanArrival'),
    'malet_taste_burned': (3, 16, 'DA_Field_MaletTasteBurned'),
    'malet_encounter': (10, 1, 'DA_Field_MaletEncounter'),
    'malet_refused': (3, 4, 'DA_Field_MaletRefused'),
    'malet_deal': (5, 2, 'DA_Field_MaletDeal'),
    'malet_reward': (8, 3, 'DA_Field_MaletReward'),
    # S298 Verdan exploration story beats (verdan_market.gd _setup_exploration_events).
    'verdan_market_walk': (8, 6, 'DA_Field_VerdanMarketWalk'),
    'verdan_old_burner': (11, 7, 'DA_Field_VerdanOldBurner'),
    'malet_backstory': (12, 8, 'DA_Field_MaletBackstory'),
    'elia_sump_concern': (8, 9, 'DA_Field_EliaSumpConcern'),
    'sump_atmosphere': (6, 10, 'DA_Field_SumpAtmosphere'),
}

def selected_case(dialect, group=None):
    if dialect == 'field':
        group = group or CASES['field'][1]
        if group not in FIELD_CASES: raise ValueError('Unreviewed Field group')
        return CASES['field'][0], group, FIELD_CASES[group][2]
    if dialect != 'vn' or group not in (None, CASES['vn'][1]): raise ValueError('Unreviewed VN sequence')
    return CASES['vn']

DEPENDENCIES = ('scripts/systems/dialogue_manager.gd', 'scripts/systems/scene_flow.gd',
                'scripts/ui/vn_scene.gd', 'scenes/main/vn_host.gd', 'scenes/maps/verdan_market.gd',
                'scripts/systems/memory_manager.gd', 'scripts/utils/journey_oath.gd',
                'scripts/core/game_manager.gd')
TEXT = ('speaker', 'text', 'text_ko', 'narrate', 'narrate_ko')
PRESENTATION = ('cg', 'portrait', 'side', 'fade_ms')
GATE = ('requires_flag', 'requires_not_flag', 'requires_memory_intact', 'requires_memory_gone')
EFFECTS = ('set_flag', 'record_ending', 'burn_memory', 'cost_memory', 'allow_faded_burn',
           'add_grains', 'add_item', 'add_item_count', 'heal_player')
ACTION = ('action', 'path', 'id', 'start_index', 'resume_scene', 'resume_index')
INTEGER = {'fade_ms', 'add_grains', 'add_item_count', 'heal_player', 'start_index', 'resume_index'}
BOOLEAN = {'allow_faded_burn'}
PHASE = {'field': 'gate_then_effects', 'vn': 'effects_then_gate_then_rewards'}
CHOICE_PHASE = {'field': 'flags_burn_cost_then_rewards_nonblocking', 'vn': 'cost_then_flags_burn_rewards_blocking'}

def parse_source(data):
    def pairs(items):
        result = {}
        for k, v in items:
            if k in result: raise ValueError('Duplicate source key: ' + k)
            result[k] = v
        return result
    return json.loads(source_bytes(data), object_pairs_hook=pairs,
                      parse_constant=lambda x: (_ for _ in ()).throw(ValueError('Nonfinite source number')))

def ir_path(dialect, group=None):
    return BASE / (selected_case(dialect, group)[1] + '.' + dialect + '.v1.json')

def package(dialect, group=None):
    return '/Game/Memoria/Generated/Narrative/' + selected_case(dialect, group)[2]

def fingerprint(value):
    # Include index map, phases and text IDs; provenance hashes/revision excluded.
    semantic = {k: copy.deepcopy(value[k]) for k in ('schema_version', 'content_kind', 'dialect', 'definition')}
    definition = semantic['definition']
    for row in definition['steps' if value['dialect'] == 'vn' else 'rows']:
        row.pop('provenance')
        for choice in row['choices']: choice.pop('provenance')
    return sha(canonical(semantic))

def allowed(dialect, choice):
    texts = ('text', 'text_ko') if choice else (TEXT if dialect == 'vn' else ('speaker', 'text', 'text_ko'))
    gates = ('requires_flag', 'requires_not_flag') if dialect == 'vn' and not choice else (GATE[:3] if dialect == 'vn' else GATE)
    effects = EFFECTS if choice else (('set_flag', 'record_ending') if dialect == 'field' else tuple(k for k in EFFECTS if k != 'cost_memory'))
    if dialect == 'field': effects = tuple(k for k in effects if k != 'allow_faded_burn')
    return {'text': texts, 'presentation': () if choice else PRESENTATION,
            'gate': gates, 'effects': effects, 'action': ACTION if dialect == 'vn' and not choice else ()}

def normalize(data, dialect, choice=False):
    if type(data) is not dict: raise ValueError('Authored record must be object')
    data = copy.deepcopy(data)
    if 'fade' in data:
        if type(data['fade']) not in (int, float): raise ValueError('fade must be number')
        ms = Decimal(str(data.pop('fade'))) * 1000
        if ms != ms.to_integral_value() or ms < 0: raise ValueError('Fade not exactly representable in milliseconds')
        data['fade_ms'] = int(ms)
    sets = allowed(dialect, choice)
    out = {group: {k: data.pop(k) for k in keys if k in data} for group, keys in sets.items()}
    jump = 'goto' if dialect == 'vn' else 'jump_to'
    if choice and jump in data and type(data[jump]) is not int:
        raise ValueError('Authored jump must be an integer, never null/bool/string')
    out['jump'] = data.pop(jump, None) if choice else None
    out['choices_present'] = ('choice' if dialect == 'vn' else 'choices') in data
    choices = data.pop('choice' if dialect == 'vn' else 'choices', []) if not choice else []
    if data: raise ValueError('Unhandled authored fields: ' + ','.join(data))
    return out, choices

def extract(dialect, root=ROOT, group=None):
    path, seq, _ = selected_case(dialect, group)
    paths = (path,) + DEPENDENCIES
    rev = subprocess.check_output(['git', '-C', str(root), 'log', '-1', '--format=%H', '--', *paths], text=True).strip()
    sources = [{'path': p, 'sha256_utf8_lf': sha(source_bytes((root/p).read_bytes()))} for p in paths]
    raw = parse_source((root/path).read_bytes())
    if dialect == 'vn':
        exact(raw, ('id', 'title', 'title_ko', 'chapter', 'bgm', 'steps'), 'Selected VN root')
        if raw['id'] != seq: raise ValueError('VN source identity changed')
        rows, position = raw['steps'], 0
        meta = {k: raw[k] for k in ('title', 'title_ko', 'chapter', 'bgm')}
    else:
        exact(raw, ('chapter', 'title', 'title_ko', 'dialogues'), 'Field file root')
        rows, position = raw['dialogues'][seq], list(raw['dialogues']).index(seq)
        meta = {k: raw[k] for k in ('title', 'title_ko', 'chapter')}
    def provenance(i, c=-1):
        return dict(source_file=path, source_hash=sources[0]['sha256_utf8_lf'], source_revision=rev,
                    dialect=dialect, group_position=position, original_index=i, original_choice_index=c)
    entries = []
    for i, raw_row in enumerate(rows):
        fields, raw_choices = normalize(raw_row, dialect)
        sid = f'{dialect}/{seq}/{i}'
        row = dict(id=sid, storage_index=i, original_index=i, provenance=provenance(i),
                   effect_phase=PHASE[dialect], **fields, choices=[])
        for j, raw_choice in enumerate(raw_choices):
            choice_fields, _ = normalize(raw_choice, dialect, True)
            choice_fields.pop('choices_present')
            row['choices'].append(dict(id=f'{sid}/choice/{j}', original_index=j,
                provenance=provenance(i,j), effect_phase=CHOICE_PHASE[dialect], **choice_fields))
        entries.append(row)
    value = dict(schema_version=1, content_kind='narrative.'+dialect, dialect=dialect,
                 extractor_version=VERSION, source_revision=rev, sources=sources,
                 definition=dict(id=seq, index_mapping_version=1, metadata=meta,
                                 **{'steps' if dialect == 'vn' else 'rows': entries}))
    value['semantic_sha256'] = fingerprint(value)
    validate(value, root)
    return value

def validate(value, root=ROOT, verify_sources=True):
    exact(value, ('schema_version','content_kind','dialect','extractor_version','source_revision','sources','definition','semantic_sha256'), 'Envelope')
    d = value['dialect']
    if type(d) is not str or d not in CASES or type(value['schema_version']) is not int or value['schema_version'] != 1 or value['content_kind'] != 'narrative.'+d or value['extractor_version'] != VERSION:
        raise ValueError('Unsupported narrative schema/kind/dialect/extractor')
    if type(value['source_revision']) is not str or not re.fullmatch('[0-9a-f]{40}',value['source_revision']): raise ValueError('Invalid revision')
    definition = value['definition']
    if type(definition) is not dict: raise ValueError('Invalid definition')
    source_path, sequence, _ = selected_case(d, definition.get('id'))
    paths = (source_path,) + DEPENDENCIES
    if type(value['sources']) is not list or len(value['sources']) != len(paths): raise ValueError('Invalid sources')
    for s,p in zip(value['sources'], paths):
        exact(s, ('path','sha256_utf8_lf'), 'Source')
        if s['path'] != p or type(s['sha256_utf8_lf']) is not str or not re.fullmatch('[0-9a-f]{64}',s['sha256_utf8_lf']): raise ValueError('Invalid source hash/path')
        if verify_sources:
            actual = source_bytes((root/p).read_bytes())
            if sha(actual) != s['sha256_utf8_lf']: raise ValueError('Source hash differs: '+p)
            git = subprocess.check_output(['git','-C',str(root),'show',value['source_revision']+':'+p], stderr=subprocess.PIPE)
            if source_bytes(git) != actual: raise ValueError('Source revision disagrees: '+p)
    definition = value['definition']; key = 'steps' if d == 'vn' else 'rows'
    exact(definition, ('id','index_mapping_version','metadata',key), 'Definition')
    if definition['id'] != sequence or type(definition['index_mapping_version']) is not int or definition['index_mapping_version'] != 1: raise ValueError('Invalid sequence/index map')
    meta = definition['metadata']
    exact(meta, ('title','title_ko','chapter','bgm') if d == 'vn' else ('title','title_ko','chapter'), 'Metadata')
    for k,v in meta.items():
        if k == 'chapter':
            if type(v) is not int or v != 2: raise ValueError('Invalid chapter')
        else: text(v,k)
    rows = definition[key]
    if type(rows) is not list or len(rows) != (13 if d == 'vn' else FIELD_CASES[sequence][0]): raise ValueError('Bounded source row count changed')
    ids=set()
    def record(r,i,j=-1):
        choice=j>=0
        fields=('id','original_index','provenance','effect_phase','text','presentation','gate','effects','action','jump')
        exact(r,fields if choice else fields+('storage_index','choices_present','choices'), 'Record')
        sid=f'{d}/{definition["id"]}/{i}'+(f'/choice/{j}' if choice else '')
        if r['id'] != sid or r['id'] in ids: raise ValueError('Invalid/duplicate stable ID')
        ids.add(r['id'])
        if type(r['original_index']) is not int or r['original_index'] != (j if choice else i): raise ValueError('Original index/order differs')
        p=r['provenance']
        expected=dict(source_file=paths[0],source_hash=value['sources'][0]['sha256_utf8_lf'],source_revision=value['source_revision'],dialect=d,group_position=0 if d == 'vn' else FIELD_CASES[sequence][1],original_index=i,original_choice_index=j)
        exact(p,expected,'Provenance')
        if canonical(p)!=canonical(expected): raise ValueError('Invalid provenance/index')
        if r['effect_phase'] != (CHOICE_PHASE[d] if choice else PHASE[d]): raise ValueError('Wrong effect phase')
        for group,keys in allowed(d,choice).items():
            if type(r[group]) is not dict or not set(r[group]) <= set(keys): raise ValueError('Unknown semantic field in '+group)
            for k,v in r[group].items():
                if k in INTEGER:
                    if type(v) is not int or not 0 <= v <= 2147483647: raise ValueError('Wrong integer: '+k)
                elif k in BOOLEAN:
                    if type(v) is not bool: raise ValueError('Wrong boolean')
                else: text(v,k, group in ('gate','effects','action') and k != 'path')
        if r['jump'] is not None and (not choice or type(r['jump']) is not int or not 0 <= r['jump'] < len(rows)): raise ValueError('Invalid zero-based jump')
        a=r['action']
        if a:
            action=a.get('action')
            legal={'goto_map':{'action','path','resume_scene','resume_index'},'goto_scene':{'action','id','start_index'},'end':{'action'}}
            if action not in legal or not set(a)<=legal[action]: raise ValueError('Invalid action/continuation representation')
            if action=='goto_map' and ('path' not in a or a['path']!='res://scenes/maps/verdan_market.tscn'): raise ValueError('Invalid map target')
            if action=='goto_scene' and a.get('id')!=definition['id']: raise ValueError('Unmigrated sequence target')
            if 'resume_index' in a and 'resume_scene' not in a: raise ValueError('Orphan resume index')
            if 'resume_scene' in a and a['resume_scene']!=definition['id']: raise ValueError('Invalid resume ID')
            for k in ('start_index','resume_index'):
                if k in a and a[k]>=len(rows): raise ValueError('Invalid continuation index')
        if not choice:
            if type(r['storage_index']) is not int or r['storage_index']!=i or type(r['choices_present']) is not bool or type(r['choices']) is not list: raise ValueError('Invalid storage/choice representation')
            if r['choices'] and not r['choices_present']: raise ValueError('Choice presence mismatch')
            for cidx,c in enumerate(r['choices']): record(c,i,cidx)
    for i,r in enumerate(rows): record(r,i)
    if value['semantic_sha256']!=fingerprint(value): raise ValueError('Semantic fingerprint differs')
    if verify_sources:
        raw=parse_source((root/paths[0]).read_bytes())
        authored=raw['steps'] if d=='vn' else raw['dialogues'][definition['id']]
        if canonical(meta)!=canonical({k:raw[k] for k in meta}): raise ValueError('Source metadata differs')
        if len(authored)!=len(rows): raise ValueError('Authored count differs')
        for actual,r in zip(authored,rows):
            fields,choices=normalize(actual,d)
            if canonical(fields)!=canonical({k:r[k] for k in fields}) or len(choices)!=len(r['choices']): raise ValueError('Typed payload differs from authored source')
            for actual_choice,c in zip(choices,r['choices']):
                f,_=normalize(actual_choice,d,True); f.pop('choices_present')
                if canonical(f)!=canonical({k:c[k] for k in f}): raise ValueError('Choice payload differs from authored source')
    return value

def load(path, verify_sources=True):
    return validate(strict_load(Path(path).read_bytes()), verify_sources=verify_sources)

def main():
    import argparse
    p=argparse.ArgumentParser(); p.add_argument('--check',action='store_true'); p.add_argument('--evidence-dir',type=Path,required=True)
    p.add_argument('--group', choices=tuple(FIELD_CASES))
    a=p.parse_args()
    if a.evidence_dir.exists(): p.error('Use fresh evidence directory')
    a.evidence_dir.mkdir(parents=True)
    report={'status':'PASS','cases':{},'check_only':a.check}
    for d in (('field',) if a.group else CASES):
        value=extract(d, group=a.group); data=canonical(value); target=ir_path(d, a.group)
        if a.check:
            load(target)
            if target.read_bytes()!=data: raise ValueError('Source extraction differs: '+d)
        else:
            target.parent.mkdir(parents=True,exist_ok=True); target.write_bytes(data)
        rows=value['definition']['steps' if d=='vn' else 'rows']
        report['cases'][d]={'ir_sha256':sha(data),'semantic_sha256':value['semantic_sha256'],'source_revision':value['source_revision'],
             'sources':value['sources'],'raw_source_sha256':sha((ROOT/CASES[d][0]).read_bytes()),
             'original_indices':[r['original_index'] for r in rows],
             'choice_indices':[[c['original_index'] for c in r['choices']] for r in rows]}
    report['selected_group'] = a.group
    (a.evidence_dir/'extraction.json').write_bytes(canonical(report)); print('MEMORIA_NARRATIVE_EXTRACTION_PASS')

if __name__=='__main__': main()
