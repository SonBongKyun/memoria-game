"""Only transient negative/semantic tests derived from the selected source IR."""
import argparse, copy
from narrative_ir import ROOT, extract, fingerprint, canonical
DEST=ROOT/'docs/unreal-migration/fixtures/malet'
def fixtures():
    original=extract('field',group='malet_taste_burned')
    changed=copy.deepcopy(original)
    changed['definition']['rows'][0]['text']['text'] += ' [temporary semantic probe]'
    changed['semantic_sha256']=fingerprint(changed)
    outputs={'modified_semantic.field.v1.json':changed}
    for name,change in (
        ('group_position',lambda v:v['definition']['rows'][0]['provenance'].update(group_position=0)),
        ('count',lambda v:v['definition']['rows'].pop()),
        ('unknown_group',lambda v:v['definition'].update(id='malet_encounter'))):
        value=copy.deepcopy(original); change(value); value['semantic_sha256']=fingerprint(value)
        outputs['reject_'+name+'.field.v1.json']=value
    return outputs
def main():
    p=argparse.ArgumentParser(); p.add_argument('--check',action='store_true'); a=p.parse_args()
    for name,value in fixtures().items():
        target=DEST/name; data=canonical(value)
        if a.check:
            if target.read_bytes()!=data: raise ValueError('Stale fixture '+name)
        else: target.write_bytes(data)
    print('MEMORIA_MALET_FIXTURES_PASS')
if __name__=='__main__': main()
