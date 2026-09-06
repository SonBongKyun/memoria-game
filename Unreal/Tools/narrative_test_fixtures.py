"""Isolated negative and synthetic contract data; never campaign import input."""
import copy
from narrative_ir import BASE, ROOT, CASES, extract, normalize, fingerprint, canonical, validate, CHOICE_PHASE
from export_narrative_oracle import inputs

FIXTURES=ROOT/'docs/unreal-migration/fixtures/narrative'

def synthetic(case, baseline):
    d=case['dialect']; out=copy.deepcopy(baseline); key='steps' if d=='vn' else 'rows'; rows=out['definition'][key]
    for i,row in enumerate(rows):
        raw=case['rows'][i] if i<len(case['rows']) else {'text':'Unvisited synthetic padding'}
        fields,choices=normalize(raw,d); row.update(fields); row['choices']=[]
        for j,c in enumerate(choices):
            f,_=normalize(c,d,True); f.pop('choices_present')
            prov=copy.deepcopy(row['provenance']); prov['original_choice_index']=j
            row['choices'].append(dict(id=row['id']+f'/choice/{j}',original_index=j,provenance=prov,effect_phase=CHOICE_PHASE[d],**f))
    out['semantic_sha256']=fingerprint(out); validate(out,verify_sources=False); return out

def mutations(base):
    def make(label, mutate):
        value=copy.deepcopy(base); mutate(value)
        try: value['semantic_sha256']=fingerprint(value)
        except (KeyError,TypeError): pass
        return label,value
    row=lambda v:v['definition']['steps'][10]
    choice=lambda v:row(v)['choices'][1]
    return [make(n,f) for n,f in [
        ('schema',lambda v:v.update(schema_version=2)),
        ('kind',lambda v:v.update(content_kind='narrative.field')),
        ('dialect',lambda v:v.update(dialect='FIELD')),
        ('revision',lambda v:v.update(source_revision='bad')),
        ('missing_provenance',lambda v:row(v).pop('provenance')),
        ('wrong_source_hash',lambda v:v['sources'][0].update(sha256_utf8_lf='0'*64)),
        ('duplicate_id',lambda v:choice(v).update(id=row(v)['choices'][0]['id'])),
        ('case_id',lambda v:choice(v).update(id=choice(v)['id'].upper())),
        ('bool_index',lambda v:choice(v).update(original_index=True)),
        ('string_index',lambda v:choice(v).update(original_index='1')),
        ('duplicate_index',lambda v:choice(v).update(original_index=0)),
        ('choice_order',lambda v:row(v)['choices'].reverse()),
        ('storage_index',lambda v:row(v).update(storage_index=9)),
        ('wrong_phase',lambda v:row(v).update(effect_phase='gate_then_effects')),
        ('wrong_gate',lambda v:row(v)['gate'].update(requires_flag=False)),
        ('unknown_semantic',lambda v:choice(v)['effects'].update(surprise=1)),
        ('effect_type',lambda v:choice(v)['effects'].update(add_grains='7')),
        ('null_text',lambda v:choice(v)['text'].update(text=None)),
        ('bad_jump',lambda v:choice(v).update(jump=13)),
        ('negative_jump',lambda v:choice(v).update(jump=-1)),
        ('jump_bool',lambda v:choice(v).update(jump=True)),
        ('mutable',lambda v:row(v).update(is_active=True)),
        ('bad_continuation',lambda v:v['definition']['steps'][12]['action'].update(resume_index=2)),
        ('invalid_target',lambda v:v['definition']['steps'][12]['action'].update(path='res://scenes/maps/other.tscn')),
    ]]

def generated():
    baseline={d:extract(d) for d in CASES}
    values={c['id']+'.json':synthetic(c,baseline[c['dialect']]) for c in inputs() if c['synthetic']}
    values.update({'reject_'+n+'.json':v for n,v in mutations(baseline['vn'])})
    changed=copy.deepcopy(baseline['vn']); changed['definition']['steps'][0]['text']['narrate']+=' [temporary contract mutation]'; changed['semantic_sha256']=fingerprint(changed)
    values['modified_semantic.json']=changed
    return values

if __name__=='__main__':
    import argparse
    p=argparse.ArgumentParser(); p.add_argument('--check',action='store_true'); a=p.parse_args()
    FIXTURES.mkdir(parents=True,exist_ok=True)
    for name,value in generated().items():
        path=FIXTURES/name; data=canonical(value)
        if a.check:
            if path.read_bytes()!=data: raise SystemExit('Test fixture changed: '+name)
        else: path.write_bytes(data)
    print('MEMORIA_NARRATIVE_CONTRACT_FIXTURES_PASS')
