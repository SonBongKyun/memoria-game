"""Strict v1 starting-memory IR. No Godot source evaluation or writes here."""
from pathlib import Path
import hashlib
import json
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]
IR = ROOT / 'docs/unreal-migration/ir/starting_memory_catalog.v1.json'
ORACLE = ROOT / 'docs/unreal-migration/ir/starting_memory_oracle.v1.json'
VERSION = 'memoria-starting-memory/1'
KIND = 'player_memory.starting_catalog'
PACKAGE = '/Game/Memoria/Generated/Memory/DA_StartingMemoryCatalog'
SOURCES = ('scripts/systems/memory_manager.gd', 'scripts/utils/journey_oath.gd', 'scenes/main/main.gd', 'scripts/core/game_manager.gd')
FIELDS = ('id', 'title', 'description', 'grade', 'burn_power', 'story_effect', 'related_npc', 'localization_ko')

def canonical(value):
    return (json.dumps(value, ensure_ascii=False, sort_keys=True, separators=(',', ':'), allow_nan=False) + '\n').encode('utf-8')

def sha(data):
    return hashlib.sha256(data).hexdigest()

def source_bytes(data):
    # Source text hashes use UTF-8/LF, without changing any checkout file.
    text = data.decode('utf-8')
    if text.startswith('\ufeff') or '\r' in text.replace('\r\n', ''):
        raise ValueError('Unsupported source BOM or bare CR')
    return text.replace('\r\n', '\n').encode('utf-8')

def strict_load(data):
    def pairs(values):
        result = {}
        for key, value in values:
            if key in result:
                raise ValueError('Duplicate JSON key: ' + key)
            result[key] = value
        return result
    value = json.loads(data.decode('utf-8'), object_pairs_hook=pairs,
                       parse_constant=lambda x: (_ for _ in ()).throw(ValueError('Nonfinite JSON number')))
    if canonical(value) != data:
        raise ValueError('IR is not canonical UTF-8/LF JSON')
    return value

def exact(value, keys, label):
    if type(value) is not dict or set(value) != set(keys):
        raise ValueError(label + ': unknown or missing fields')

def text(value, label, nonempty=False):
    if type(value) is not str or '\x00' in value or any(0xd800 <= ord(c) <= 0xdfff for c in value) or (nonempty and not value):
        raise ValueError(label + ': invalid text')

def fingerprint(entries):
    # Semantic payload includes order, all authored definition fields and KO
    # presence. Provenance/package metadata deliberately do not change semantics.
    return sha(canonical({'content_kind': KIND, 'entries': entries, 'schema_version': 1}))

def validate(value, root=ROOT, verify_sources=True):
    exact(value, ('schema_version', 'content_kind', 'source_repository_revision', 'sources', 'extractor_version', 'entries', 'semantic_sha256'), 'IR')
    if type(value['schema_version']) is not int or value['schema_version'] != 1 or value['content_kind'] != KIND or value['extractor_version'] != VERSION:
        raise ValueError('Unsupported schema/content kind/extractor')
    if not re.fullmatch('[0-9a-f]{40}', value['source_repository_revision']):
        raise ValueError('Invalid source revision')
    if type(value['sources']) is not list or [s.get('path') for s in value['sources']] != list(SOURCES):
        raise ValueError('Source inventory/order mismatch')
    for source in value['sources']:
        exact(source, ('path', 'sha256_utf8_lf'), 'Source')
        if not re.fullmatch('[0-9a-f]{64}', source['sha256_utf8_lf']):
            raise ValueError('Invalid source hash')
        if verify_sources:
            actual = source_bytes((root/source['path']).read_bytes())
            if sha(actual) != source['sha256_utf8_lf']:
                raise ValueError('Source hash differs: ' + source['path'])
            committed = subprocess.check_output(['git', '-C', str(root), 'show', value['source_repository_revision'] + ':' + source['path']], stderr=subprocess.PIPE)
            if source_bytes(committed) != actual:
                raise ValueError('Source differs from recorded repository revision')
    if type(value['entries']) is not list or not value['entries']:
        raise ValueError('Empty or malformed entries')
    ids = set()
    for row in value['entries']:
        exact(row, FIELDS, 'Definition')
        for key in ('id', 'title', 'description', 'story_effect', 'related_npc'):
            text(row[key], key, key in ('id', 'title', 'description'))
        if row['id'] in ids:
            raise ValueError('Exact duplicate memory ID: ' + row['id'])
        ids.add(row['id'])
        if type(row['grade']) is not int or not 0 <= row['grade'] <= 4:
            raise ValueError('Invalid raw grade')
        if type(row['burn_power']) is not int or not 0 < row['burn_power'] <= 2147483647:
            raise ValueError('Invalid burn power')
        ko = row['localization_ko']
        exact(ko, ('title', 'description', 'story_effect'), 'KO')
        text(ko['title'], 'KO title', True); text(ko['description'], 'KO description', True)
        if ko['story_effect'] is not None:
            text(ko['story_effect'], 'KO effect')
    if value['semantic_sha256'] != fingerprint(value['entries']):
        raise ValueError('Semantic hash differs')
    return value

def load(path=IR, root=ROOT, verify_sources=True):
    return validate(strict_load(path.read_bytes()), root, verify_sources)
