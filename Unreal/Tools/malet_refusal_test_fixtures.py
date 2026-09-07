"""Transient strict-validation probes for the two authorized groups."""
import argparse,copy
from narrative_ir import ROOT,extract,fingerprint,canonical
DEST=ROOT/'docs/unreal-migration/fixtures/malet_refusal'
def fixtures():
    outputs={}
    for group in ('malet_encounter','malet_refused'):
        original=extract('field',group=group)
        for name,change in (
            ('modified',lambda v:v['definition']['rows'][0]['text'].update(text=v['definition']['rows'][0]['text']['text']+' [temporary probe]')),
            ('reject_position',lambda v:v['definition']['rows'][0]['provenance'].update(group_position=16)),
            ('reject_choice',lambda v:v['definition']['rows'][0].update(original_index=99)),
            ('reject_downstream',lambda v:v['definition'].update(id='malet_deal'))):
            v=copy.deepcopy(original);change(v);v['semantic_sha256']=fingerprint(v)
            outputs[group+'.'+name+'.json']=v
    return outputs
def main():
    p=argparse.ArgumentParser();p.add_argument('--check',action='store_true');a=p.parse_args()
    for name,v in fixtures().items():
        target=DEST/name;data=canonical(v)
        if a.check:
            if target.read_bytes()!=data:raise ValueError('Stale fixture '+name)
        else:target.write_bytes(data)
    print('MEMORIA_MALET_REFUSAL_FIXTURES_PASS')
if __name__=='__main__':main()
