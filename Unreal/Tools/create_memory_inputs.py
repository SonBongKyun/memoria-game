"""Author deterministic INPUTS only. Expected values must come from Godot."""
from pathlib import Path
import json

ROOT = Path(__file__).resolve().parents[2]
DEST = ROOT / 'docs/unreal-migration/fixtures/player_memory_inputs.json'
cases = []

def memory(name, grade=0, power=100, **extra):
    return dict(id=name, grade=grade, burn_power=power, **extra)

def case(name, memories, commands, chapter=1, elia=True, still=False, **extra):
    cases.append(dict(id=name, context=dict(chapter=chapter, elia=elia, still=still),
                      memories=memories, commands=commands, **extra))

case('grade_ordinals_and_weights', [memory(f'g{i}', i) for i in range(5)], [dict(op='query')])
for chapter in [-2, 0, 1, 2, 10, 11, 12, 24]:
    case(f'capacity_chapter_{str(chapter).replace("-", "minus_")}', [memory('isolated')], [dict(op='query')], chapter=chapter)
for grade in range(5):
    for elia in [False, True]:
        case(f'normal_grade_{grade}_elia_{int(elia)}', [memory('isolated', grade)],
             [dict(op='burn', id='isolated'), dict(op='query'), dict(op='burn', id='isolated')], elia=elia)
case('silent_has_no_new_residue', [memory('isolated', 3)], [dict(op='silent', id='isolated'), dict(op='query')])
case('silent_preserves_preexisting_residue_bit', [memory('isolated', 2, residue=True)], [dict(op='silent', id='isolated')])
case('normal_does_not_clear_residue_bit_when_elia_absent', [memory('isolated', 0, residue=True)], [dict(op='burn', id='isolated')], elia=False)
case('faded_rejection_then_explicit_override', [memory('isolated', 2, faded=True, erosion=80)],
     [dict(op='burn', id='isolated'), dict(op='silent', id='isolated'), dict(op='burn', id='missing'), dict(op='burn', id='isolated', allow_faded=True)])
case('silent_allows_faded_explicitly', [memory('isolated', 2, faded=True)], [dict(op='silent', id='isolated', allow_faded=True)])
case('collateral_rejects_both_burn_modes_and_excludes_weight', [memory('pledged', 3), memory('owned', 1)],
     [dict(op='burn', id='pledged'), dict(op='silent', id='pledged', allow_faded=True)],
     loan=dict(memory_id='pledged', principal=70, repay=98, due_chapter=3))
case('burn_history_order', [memory('alpha'), memory('beta'), memory('gamma')],
     [dict(op='burn', id='gamma'), dict(op='silent', id='alpha'), dict(op='burn', id='beta')])
case('erosion_rank_elia_truncation_and_core_immunity', [memory(f'g{i}', i, related_npc='Elia') for i in range(5)],
     [dict(op='erode', chapter=3), dict(op='erode', chapter=1)], chapter=8)
case('erosion_skips_burned_and_faded_counts_zero', [memory('burned', burned=True), memory('faded', faded=True), memory('elia', 3, related_npc='Elia')],
     [dict(op='erode', chapter=1)], history=['burned'])
case('erosion_guard_before_still_hands', [memory('guarded', 2, related_npc='Elia'), memory('kept', 2, related_npc='Elia'), memory('other')],
     [dict(op='erode', chapter=8), dict(op='erode', chapter=8)], guards=['guarded'], still=True, chapter=8, guard_slots_used=1)
case('core_guard_not_consumed', [memory('core', 4)], [dict(op='erode', chapter=8)], guards=['core'])
case('erosion_collateral_still_erodes', [memory('pledged', 1)], [dict(op='erode', chapter=4)],
     loan=dict(memory_id='pledged', principal=26, repay=36, due_chapter=3))
case('fading_threshold_and_effective_power_floor', [memory('threshold', 0, 10, erosion=6), memory('zero', 1, 0), memory('negative', 0, -5)],
     [dict(op='erode', chapter=1), dict(op='query')], elia=False)
case('overload_fractional_rounding', [memory(f'n{i}', 2) for i in range(6)], [dict(op='erode', chapter=3)])
case('overload_multiplier_caps_at_two', [memory(f'n{i}', 3) for i in range(10)], [dict(op='erode', chapter=5)], elia=False)
case('capacity_uses_runtime_chapter_erosion_uses_argument', [memory(f'n{i}', 3) for i in range(6)], [dict(op='erode', chapter=2)], chapter=10)
case('cascade_group_and_neighbor_order', [memory('rel_a', 2, related_npc='Elia'), memory('daily_a', 1, 10, related_npc='Elia'),
     memory('rel_b', 1, related_npc='Other'), memory('rel_c', 0), memory('core_z', 4, related_npc='Elia')], [dict(op='burn', id='rel_a')])
case('cascade_guard_and_fading_before_burn_notification', [memory('rel_a', 3), memory('rel_b', 1, 10), memory('daily_c', 1, 10, related_npc='Other'),
     memory('rel_d', 0, 10, related_npc='Other')], [dict(op='burn', id='rel_a'), dict(op='burn', id='rel_d')], guards=['rel_b'])
case('cascade_affects_collateral_and_still_hand_memory', [memory('rel_a', 2, related_npc='Elia'), memory('rel_b', 2, related_npc='Elia')],
     [dict(op='burn', id='rel_a')], still=True, loan=dict(memory_id='rel_b', principal=44, repay=62, due_chapter=3))
case('cascade_skips_already_faded_burned_and_core', [memory('rel_a', 2, related_npc='Elia'), memory('daily_b', 1, faded=True, related_npc='Elia'),
     memory('daily_c', 1, burned=True, related_npc='Elia'), memory('core_d', 4, related_npc='Elia')], [dict(op='silent', id='rel_a')], history=['daily_c'])
for threshold in [5, 10, 20, 30, 50]:
    mems = [memory(f'b{i}', 0, burned=True) for i in range(threshold - 1)] + [memory('last', 2)]
    history = [m['id'] for m in mems[:-1]]
    case(f'passive_threshold_{threshold}', mems, [dict(op='passives'), dict(op='burn', id='last'), dict(op='passives')], history=history)
case('extractions_excluded_from_passives', [memory(f'b{i}', burned=True) for i in range(6)], [dict(op='passives')],
     history=[f'b{i}' for i in range(6)], extracted=['b0', 'b1'])
case('extraction_count_clamped_not_set_difference', [memory('a', burned=True)], [dict(op='passives')], history=['a'], extracted=['a', 'unknown'])
case('passive_key_presence_even_when_false', [memory(f'b{i}', burned=True) for i in range(5)], [dict(op='passives')],
     history=[f'b{i}' for i in range(5)], passives={'ember_affinity': False})
case('preservation_fields_survive_memory_command', [memory('kept', 2)], [dict(op='query')],
     anchor_vigil=36, anchor_passives={'quiet_focus': True}, vigil_chapters=[1, 2, 3], guard_slots_used=2)
case('acquisition_notifications_precede_graph_refresh', [memory('rel_a', 2)], [dict(op='add', memory=memory('rel_b', 1)), dict(op='query')])
case('case_sensitive_ids_and_npc_names', [memory('Id', 2, related_npc='elia'), memory('id', 2, related_npc='Elia')],
     [dict(op='erode', chapter=4), dict(op='burn', id='ID'), dict(op='silent', id='Id')], chapter=5)
case('unicode_identity_preserved', [memory('기억_하나', 2, title='뻗어 나가던 손', related_npc='Elia'), memory('기억_둘', 1)],
     [dict(op='burn', id='기억_하나')])

DEST.parent.mkdir(parents=True, exist_ok=True)
DEST.write_text(json.dumps({'schema_version': 1, 'cases': cases}, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
print(f'MEMORIA_MEMORY_INPUTS_CREATED cases={len(cases)} (no expected outputs generated)')
