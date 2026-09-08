"""Transient strict probes for the single Phase 1H group."""
import argparse,copy
from narrative_ir import ROOT,extract,fingerprint,canonical
DEST=ROOT/'docs/unreal-migration/fixtures/malet_deal'
def fixtures():
    original=extract('field',group='malet_deal');result={}
    for name,change in (
        ('modified',lambda v:v['definition']['rows'][0]['text'].update(text=v['definition']['rows'][0]['text']['text']+' [temporary probe]')),
        ('reject_position',lambda v:v['definition']['rows'][0]['provenance'].update(group_position=3)),
        ('reject_count',lambda v:v['definition']['rows'].pop()),
        ('reject_index',lambda v:v['definition']['rows'][0].update(original_index=7)),
        ('reject_downstream',lambda v:v['definition'].update(id='malet_reward'))):
        v=copy.deepcopy(original);change(v);v['semantic_sha256']=fingerprint(v);result[name+'.field.v1.json']=v
    return result
def main():
    p=argparse.ArgumentParser();p.add_argument('--check',action='store_true');a=p.parse_args()
    for name,v in fixtures().items():
        target=DEST/name;data=canonical(v)
        if a.check:
            if target.read_bytes()!=data:raise ValueError('Stale fixture '+name)
        else:target.write_bytes(data)
    print('MEMORIA_MALET_DEAL_FIXTURES_PASS')
if __name__=='__main__':main()
