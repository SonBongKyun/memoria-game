"""Translate recorded JSON vectors to C++ test data; never evaluate game rules."""
from pathlib import Path
import argparse, hashlib, json

ROOT = Path(__file__).resolve().parents[2]
folder = ROOT/'docs/unreal-migration/fixtures'
inputs = folder/'player_memory_inputs.json'
expected = folder/'player_memory_expected.json'
provenance = json.loads((folder/'player_memory_provenance.json').read_text(encoding='utf-8'))
for key, path in [('input_sha256', inputs), ('expected_sha256', expected)]:
    if hashlib.sha256(path.read_bytes()).hexdigest() != provenance[key]:
        raise SystemExit(f'Fixture provenance mismatch: {path.name}; rerun the Godot oracle, do not edit expected values')
ins = json.loads(inputs.read_text(encoding='utf-8'))['cases']
outs = {c['id']: c for c in json.loads(expected.read_text(encoding='utf-8'))['cases']}
assert set(outs) == {c['id'] for c in ins}

def q(v): return json.dumps(str(v), ensure_ascii=True)
def b(v): return 'true' if v else 'false'
def strings(values): return '{' + ','.join(q(v) for v in values) + '}'
def seq(values): return '{' + ','.join(str(int(v))+'LL' for v in values) + '}'
def flags(values): return '{' + ','.join('{'+q(k)+','+b(v)+'}' for k,v in values.items()) + '}'
def definition(m):
    return '{'+','.join([q(m['id']), q(m.get('title',m['id'])), q(m.get('description','')), f"static_cast<M::Grade>({m['grade']})", str(m['burn_power'])+'LL', q(m.get('story_effect','')), q(m.get('related_npc',''))])+'}'

def list_tokens(t, name, values): t.extend([name,str(len(values)), *map(str,values)])
def flag_tokens(t, name, values):
    t.extend([name,str(len(values))])
    for key,value in sorted(values.items()): t.extend([key,str(int(value))])
def observation(s):
    t=['owned',str(len(s['owned']))]
    for m in s['owned']:
        t.extend([m['id'],str(int(m['burned'])),str(int(m['residue'])),str(int(m['faded'])),str(int(m['erosion']))])
        list_tokens(t,'connections',m['connections'])
    list_tokens(t,'history',s['history']); flag_tokens(t,'passives',s['passives']); list_tokens(t,'guards',s['guards'])
    loan=s['loan']; t.extend(['loan',str(int(bool(loan)))])
    if loan: t.extend([loan['memory_id'],str(int(loan['principal'])),str(int(loan['repay'])),str(int(loan['due_chapter']))])
    list_tokens(t,'extracted',s['extracted']); t.extend(['anchor_vigil',str(int(s['anchor_vigil']))])
    flag_tokens(t,'anchor_passives',s['anchor_passives']); list_tokens(t,'vigil_chapters',[int(v) for v in s['vigil_chapters']])
    for name in ['guard_slots_used','carry_weight','carry_capacity','voluntary_burn_count']: t.extend([name,str(int(s[name]))])
    for name in ['available','available_allow_faded','residue_ids']: list_tokens(t,name,s[name])
    for name in ['effective_powers','grade_ordinals','weights','intact']: list_tokens(t,name,[int(v) for v in s[name]])
    return '{'+strings(t)+','+repr(float(s['overload']))+'}'

lines=['// Generated from recorded Godot outputs. Do not edit expected values.',
       '#pragma once', '#include "MemoryParitySuite.h"', 'namespace Memoria::Tests {']
for i,c in enumerate(ins):
    lines.extend([f'inline Fixture Fixture{i}() {{', 'Fixture F;', 'F.Id='+q(c['id'])+';',
        'F.Context={'+str(c['context']['chapter'])+'LL,'+b(c['context']['elia'])+','+b(c['context']['still'])+'};',
        'F.Definitions={'+','.join(definition(m) for m in c['memories'])+'};'])
    states=['{'+','.join([q(m['id']),b(m.get('burned',False)),b(m.get('residue',False)),b(m.get('faded',False)),str(m.get('erosion',0))+'LL','{}'])+'}' for m in c['memories']]
    lines += ['F.Initial.Owned={'+','.join(states)+'};', 'F.Initial.BurnedHistory='+strings(c.get('history',[]))+';',
              'F.Initial.BurnPassives='+flags(c.get('passives',{}))+';', 'F.Initial.AnchorVigil='+str(c.get('anchor_vigil',0))+'LL;',
              'F.Initial.AnchorPassives='+flags(c.get('anchor_passives',{}))+';', 'F.Initial.VigilChapters='+seq(c.get('vigil_chapters',[]))+';',
              'F.Initial.ErosionGuarded='+strings(c.get('guards',[]))+';', 'F.Initial.GuardSlotsUsed='+str(c.get('guard_slots_used',0))+'LL;',
              'F.Initial.Extracted='+strings(c.get('extracted',[]))+';']
    loan=c.get('loan',{})
    if loan: lines.append('F.Initial.ActiveLoan={true,'+q(loan['memory_id'])+','+','.join(str(loan[k])+'LL' for k in ['principal','repay','due_chapter'])+'};')
    for cmd in c['commands']:
        added=definition(cmd['memory']) if 'memory' in cmd else '{}'
        lines.append('F.Commands.push_back({'+','.join([q(cmd['op']),q(cmd.get('id','')),b(cmd.get('allow_faded',False)),str(cmd.get('chapter',1))+'LL',added])+'});')
    result=outs[c['id']]
    lines.append('F.ExpectedInitial='+observation(result['initial'])+';')
    for step in result['steps']:
        events=[]
        for event in step['events']:
            t=[event['kind'],event['memory_id'],str(int(event['amount'])),str(int(event['weight'])),str(int(event['capacity'])),event['passive_name']]
            list_tokens(t,'affected_ids',event['affected_ids'])
            events.append('{'+strings(t)+','+observation(event['observation'])+'}')
        lines.append('F.Expected.push_back({'+b(step['success'])+',{'+','.join(events)+'},'+observation(step['state'])+'});')
    lines.append('return F; }')
lines += ['inline const std::vector<Fixture>& Fixtures() {',
          'static const std::vector<Fixture> Values={'+','.join(f'Fixture{i}()' for i in range(len(ins)))+'};',
          'return Values; }', '}']
dest=ROOT/'Unreal/Tests/Generated/MemoryParityFixtures.h'
body='\n'.join(lines)+'\n'
parser=argparse.ArgumentParser(); parser.add_argument('--check',action='store_true'); args=parser.parse_args()
if args.check:
    if not dest.exists() or dest.read_text(encoding='utf-8') != body: raise SystemExit('Generated fixture header is stale')
else:
    dest.parent.mkdir(parents=True,exist_ok=True); dest.write_text(body,encoding='utf-8')
print(f'MEMORIA_FIXTURE_HEADER_PASS cases={len(ins)} provenance=verified')
