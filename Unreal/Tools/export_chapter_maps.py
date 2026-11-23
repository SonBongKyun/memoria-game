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
MAPS = {'belt_waystation': (3, 'Ch3'), 'drift_shelter': (4, 'Ch4'), 'the_seam': (6, 'Ch6'), 'forgotten_forest': (7, 'Ch7'), 'crumbling_coast': (8, 'Ch8'), 'haren_clearing': (9, 'Ch9'), 'seam_return': (10, 'Ch10'), 'roofless_farms': (11, 'Ch11'), 'basin_station': (12, 'Ch12'), 'verdan_return': (13, 'Ch13'), 'ridge_farmhouse': (14, 'Ch14')}
# S374: a manuscript map whose ground is another Godot script's (the clearing stands on the rim forest's ring).
# The final manuscript's episodes each manuscript map tells (recorded in the IR's source line).
EPISODES = {'the_seam': '6-7', 'forgotten_forest': '8-10', 'crumbling_coast': '11-12', 'haren_clearing': '13-14',
            'seam_return': '15-16', 'roofless_farms': '17-18', 'basin_station': '19-20', 'verdan_return': '21', 'ridge_farmhouse': '22-23'}
# S375: Chapter 10 comes back to the Seam itself: the same village, packing to leave.
# S377: the fields toward the Belt road stand on the Godot colorless_waste plain; its stone clusters are the farmhouses.
# S379: the basin waystation at a crossroads stands on the Godot belt_waystation yard (without the Belt's props).
# S382: Verdan again, on the Godot verdan_market ground (not the Chapter 2 slice, L_VerdanHost, which carries Malet).
# S385: the farmhouse under the ridge stands on the Godot drift_shelter ground: its walled slab ring, open on the
# south, is the roofless farmhouse of three high walls (22-23화).
GROUND = {'haren_clearing': 'rim_forest', 'seam_return': 'the_seam', 'roofless_farms': 'colorless_waste', 'basin_station': 'belt_waystation',
          'verdan_return': 'verdan_market', 'ridge_farmhouse': 'drift_shelter'}

# S371: maps whose story is the final manuscript's, not the Godot script's. The ground (tiles, colours,
# collision, atmosphere, encounter pool) is still read from scenes/maps/<map>.gd; the arrival chain, the
# story triggers, the exit and the placements below follow 최종본 6화 「더 심」 and 7화 「세이블」.
# Their groups (ms_*) are scenes built at runtime by MemoriaManuscript, so no dialogue assets are needed.
T = 32.0
MANUSCRIPT = {
    'the_seam': {
        'title': {'number': 6, 'name': 'The Seam', 'subtitle': 'Between what was and what will be'},
        # 6화: they come down the ridge road into the village; the north-west path is its end.
        'spawn': [5 * T, 1.5 * T],
        'elia_repeat': "I wanted to stay a little longer.",
        'sequence': [
            {'flag': 'ch6_arrived', 'group': 'ms_seam_arrival', 'memories_chapter': 6, 'flags': [], 'toasts': []},
            {'flag': 'ch6_name_slip', 'group': 'ms_seam_name', 'memories_chapter': None, 'flags': [], 'toasts': []},
            {'flag': 'ch6_feren', 'group': 'ms_seam_feren', 'memories_chapter': None, 'flags': [], 'toasts': []},
        ],
        'triggers': [
            # The night: the inner room by the water hall in the middle of the village (6화 끝 ~ 7화 앞).
            {'rect': {'origin': [12 * T, 10 * T], 'size': [2 * T, T]}, 'group': 'ms_seam_night', 'flag': 'ch6_sable_night', 'gate': 'ch6_feren'},
            # The morning: Sable carving spoons at the table north of the brook (7화).
            {'rect': {'origin': [16 * T, 7 * T], 'size': [2 * T, T]}, 'group': 'ms_seam_morning', 'flag': 'ch6_sable_morning', 'gate': 'ch6_sable_night'},
        ],
        # Haren takes them up the ridge road: the north-east path out of the village.
        'exit': {'center': [20 * T, 1.5 * T], 'size': [2 * T, T], 'requires': 'ch6_sable_morning', 'completes': 'ch6_complete',
                 'group': 'ms_seam_departure', 'next_chapter': 7, 'next_map': 'forgotten_forest', 'next_scene': None,
                 'flags': ['canon_ch7_ridge_ready'],
                 'notice': {'en': 'Somewhere north-north-east, the hole in the bark is waiting.',
                            'ko': '북북동 어딘가에서, 나무껍질의 구멍이 가리키는 곳이 기다린다.'}},
        'objects_gate': 'ch6_complete',
        'chests': [{'origin': [22 * T, 5 * T], 'flag': 'chest_seam_garden', 'rewards': {'grains': 20, 'items': {'potion': 2, 'antidote': 1}}}],
        'clues': [{'origin': [10 * T, 8 * T], 'flag': 'clue_seam_stream',
                   'text': 'A plank bridge over the brook. The pebbles show through, and the pattern of the water over them never stays the same.'}],
        'battles_gate': None, 'battles': [], 'resume_blocked': [],
        'encounters_gate': 'ch6_complete',
        # The lanterns (Tile.LANTERN) are lit: add_tile_lights in the source.
        'lights': [{'tile': 6, 'color': [1.0, 0.85, 0.5, 1.0], 'energy': 0.9, 'scale': 3.0}],
        'ambient_npcs': [{'position': [9 * T + 16, 3 * T + 16], 'preset': 'villager_f'},
                         {'position': [14 * T + 16, 13 * T + 16], 'preset': 'scholar'},
                         {'position': [18 * T + 16, 9 * T + 16], 'preset': 'villager_m'}],
        'ambient_npcs_gate': 'ch6_arrived',
    },
    # S372: 최종본 8화 「듣는 숲」, 9화 「빌려온 입」, 10화 「대가」 on the Godot forgotten_forest ground.
    'forgotten_forest': {
        'title': {'number': 7, 'name': 'The Listening Forest', 'subtitle': 'Voices of those who passed, and of those who are not here'},
        # Over the ridge from the Seam: they come in on the north road and leave the forest to the south (9화).
        'spawn': [8 * T + 16, 1.5 * T],
        'elia_repeat': "Don't turn toward the sounds.",
        'sequence': [
            {'flag': 'ch7_ridge', 'group': 'ms_forest_ridge', 'memories_chapter': 8, 'flags': [], 'toasts': []},
            {'flag': 'ch7_forest_enter', 'group': 'ms_forest_enter', 'memories_chapter': None, 'flags': [], 'toasts': []},
        ],
        'triggers': [
            {'rect': {'origin': [9 * T, 7 * T], 'size': [2 * T, T]}, 'group': 'ms_forest_echoes', 'flag': 'ch7_echoes', 'gate': 'ch7_forest_enter'},
            {'rect': {'origin': [10 * T, 10 * T], 'size': [2 * T, T]}, 'group': 'ms_forest_mother', 'flag': 'ch7_mother', 'gate': 'ch7_echoes'},
            {'rect': {'origin': [11 * T, 13 * T], 'size': [2 * T, T]}, 'group': 'ms_forest_lost_hour', 'flag': 'ch7_lost_hour', 'gate': 'ch7_mother'},
            {'rect': {'origin': [12 * T, 15 * T], 'size': [2 * T, T]}, 'group': 'ms_forest_escape', 'flag': 'ch7_escape', 'gate': 'ch7_lost_hour'},
        ],
        # 9화: the shape between the trees. It rises where the lost hour ends; burning is how Arrel faced it.
        'battles_gate': 'ch7_lost_hour',
        'battles': [{'rect': {'origin': [11 * T, 14 * T], 'size': [2 * T, T]}, 'name': 'Hollow Walker', 'hp': 95, 'atk': 22, 'is_void': False}],
        'exit': {'center': [15 * T + 16, 16.5 * T], 'size': [5 * T, T], 'requires': 'ch7_escape', 'completes': 'ch7_complete',
                 'group': 'ms_forest_valley', 'next_chapter': 8, 'next_map': 'crumbling_coast', 'next_scene': None,
                 'flags': ['canon_ch8_coast_ready'],
                 'notice': {'en': 'Under the slanted slab, the echo shell says the counting is wrong.',
                            'ko': '기울어진 돌판 아래에서, 에코 셸이 셈이 틀렸다고 말한다.'}},
        'objects_gate': 'ch7_complete',
        'chests': [{'origin': [5 * T, 5 * T], 'flag': 'chest_forest_cairn', 'rewards': {'grains': 20, 'items': {'potion': 2, 'antidote': 2}}},
                   {'origin': [20 * T, 3 * T], 'flag': 'chest_forest_canopy', 'rewards': {'grains': 10, 'items': {'firebomb': 2}}}],
        'clues': [{'origin': [14 * T, 6 * T], 'flag': 'clue_forest_roots',
                   'text': "The roots pulse faintly. Not with sap: with residue. These trees are digesting someone's childhood."},
                  {'origin': [6 * T, 13 * T], 'flag': 'clue_forest_carving',
                   'text': 'Scratches in the bark. Someone tried to carve their name. The letters stop halfway: they forgot the rest.'}],
        'resume_blocked': [],
        'encounters_gate': 'ch7_complete',
        # The white mushroom rings (8화) give a faint pale light in the hollows and on the cairn.
        'lights': [{'tile': 3, 'color': [0.78, 0.92, 0.86, 1.0], 'energy': 0.35, 'scale': 2.0},
                   {'tile': 6, 'color': [0.85, 0.95, 0.9, 1.0], 'energy': 0.5, 'scale': 2.5}],
        'ambient_npcs': [], 'ambient_npcs_gate': None,
    },
    # S373: 최종본 11화 「공포가 하던 일」 and 12화 「편집자」 on the Godot crumbling_coast ground. Its pool (WATER, walled
    # in by cliffs) stands as the hole: 'no water could be seen'; blue-white light comes up from it.
    'crumbling_coast': {
        'title': {'number': 8, 'name': 'The Crumbling Coast', 'subtitle': 'What fear used to do'},
        # East from the stone ridges (11화: "동쪽"): they come in on the west side; they leave north.
        'spawn': [1 * T + 16, 9 * T + 16],
        'elia_repeat': 'Nine years, and still.',
        'sequence': [
            {'flag': 'ch8_morning', 'group': 'ms_coast_morning', 'memories_chapter': None, 'flags': [], 'toasts': []},
            {'flag': 'ch8_shore', 'group': 'ms_coast_shore', 'memories_chapter': None, 'flags': [], 'toasts': []},
        ],
        'triggers': [
            {'rect': {'origin': [11 * T, 7 * T], 'size': [3 * T, T]}, 'group': 'ms_coast_hole', 'flag': 'ch8_hole', 'gate': 'ch8_shore'},
            {'rect': {'origin': [11 * T, 5 * T], 'size': [3 * T, T]}, 'group': 'ms_coast_city', 'flag': 'ch8_city', 'gate': 'ch8_hole'},
            {'rect': {'origin': [4 * T, 3 * T], 'size': [2 * T, T]}, 'group': 'ms_coast_tree', 'flag': 'ch8_tree', 'gate': 'ch8_city'},
        ],
        'battles_gate': None, 'battles': [],
        'exit': {'center': [5 * T + 16, 1.5 * T], 'size': [3 * T, T], 'requires': 'ch8_tree', 'completes': 'ch8_complete',
                 'group': 'ms_coast_chair', 'next_chapter': 9, 'next_map': 'haren_clearing', 'next_scene': None,
                 'flags': ['canon_ch9_clearing_ready'],
                 'notice': {'en': 'Before the river the road forks twice more. To the north the rain has stopped.',
                            'ko': '강 전까지 길은 두 번 더 갈라진다. 북쪽에서는 비가 그쳤다.'}},
        'objects_gate': 'ch8_complete',
        'chests': [{'origin': [20 * T, 14 * T], 'flag': 'chest_coast_shore', 'rewards': {'grains': 18, 'items': {'potion': 2, 'firebomb': 1}}}],
        'clues': [{'origin': [2 * T, 9 * T], 'flag': 'clue_coast_grassline',
                   'text': 'Where the grass ends, every blade is cut along one line. Nine years, and the line has not moved.'}],
        'resume_blocked': [],
        'encounters_gate': 'ch8_complete',
        # Low and blue: thirteen lights over the pool's tiles wash out to white at any stronger setting.
        'lights': [{'tile': 3, 'color': [0.5, 0.66, 1.0, 1.0], 'energy': 0.16, 'scale': 2.2}],
        # The coast's colours drain (11화: 엘리아의 코트가 옅어지고 있었다).
        # ...and the Godot splash shows breaking surf, which the manuscript's coast does not have.
        'atmosphere': {'saturation': 0.42, 'mood': 0.46, 'splash': 'res://assets/cg/generated/chapter_expansion/ch09_equalized_waste_v1.png'},
        'ambient_npcs': [], 'ambient_npcs_gate': None,
    },
    # S374: 최종본 13화 「산수」 and 14화 「자장가가 돌아오다」 on the Godot rim_forest ground: its ring of path round a
    # green hollow stands as Haren's clearing. They come up from the south (the right-hand road from the chair) and
    # leave north, past the rock with water in it, through broken bedrock to the dry channel.
    'haren_clearing': {
        'title': {'number': 9, 'name': "Haren's Clearing", 'subtitle': "This isn't memory. It's arithmetic."},
        'spawn': [12 * T + 16, 15 * T + 16],
        'elia_repeat': "I'm watching the same thing you are.",
        'sequence': [
            {'flag': 'ch9_hand', 'group': 'ms_clearing_hand', 'memories_chapter': None, 'flags': [], 'toasts': []},
            {'flag': 'ch9_trees', 'group': 'ms_clearing_trees', 'memories_chapter': None, 'flags': [], 'toasts': []},
        ],
        'triggers': [
            # Haren by the felled tree, in the hollow inside the ring.
            {'rect': {'origin': [10 * T, 10 * T], 'size': [5 * T, T]}, 'group': 'ms_clearing_haren', 'flag': 'ch9_haren', 'gate': 'ch9_trees'},
            # The cut face of the felled tree, at the hollow's north side.
            {'rect': {'origin': [10 * T, 8 * T], 'size': [5 * T, T]}, 'group': 'ms_clearing_rings', 'flag': 'ch9_rings', 'gate': 'ch9_haren'},
            # The low rock across the path with water in its top; past it the next fork, and something pale.
            {'rect': {'origin': [11 * T, 3 * T], 'size': [3 * T, T]}, 'group': 'ms_clearing_rock', 'flag': 'ch9_rock', 'gate': 'ch9_rings'},
        ],
        'battles_gate': None, 'battles': [],
        # North through the broken bedrock to the dry channel and its limestone slab (14화).
        'exit': {'center': [12 * T + 16, 1.5 * T], 'size': [3 * T, T], 'requires': 'ch9_rock', 'completes': 'ch9_complete',
                 'group': 'ms_clearing_channel', 'next_chapter': 10, 'next_map': 'seam_return', 'next_scene': None,
                 'flags': ['canon_ch10_seam_return_ready'],
                 'notice': {'en': 'In the morning, back to the Seam. At least ask her to her face.',
                            'ko': '아침이 되면 더 심으로. 적어도 그 사람 앞에서 물어야지.'}},
        'objects_gate': 'ch9_complete',
        'chests': [{'origin': [4 * T, 3 * T], 'flag': 'chest_clearing_branches', 'rewards': {'grains': 22, 'items': {'potion': 2, 'antidote': 1}}}],
        'clues': [{'origin': [12 * T, 9 * T], 'flag': 'clue_clearing_rings',
                   'text': 'The cut face is pale and still damp. Fine strokes lie between the rings, deeper than a finger could rub away.'}],
        'resume_blocked': [],
        'encounters_gate': 'ch9_complete',
        # Chapter 1's pool is too small for Chapter 9: the listening forest's creatures stand in.
        'encounters_from': 'forgotten_forest',
        'lights': [],
        # 13화: sunlight comes into the forest. Greener than the coast, still short of the Seam.
        'atmosphere': {'saturation': 0.72, 'mood': 0.3, 'splash': 'res://assets/cg/generated/chapter_splash_rim_forest.png'},
        'ambient_npcs': [], 'ambient_npcs_gate': None,
    },
    # S375: 최종본 15화 「손이 기억하는 것」 and 16화 「출발」, back on the Seam's own ground. Haren brings them down
    # the north-east path; the village is packing; Sable on the stone step; the horn from the east; out by the west.
    'seam_return': {
        'title': {'number': 10, 'name': 'The Seam, Leaving', 'subtitle': 'What the hand remembers'},
        'spawn': [20 * T, 1.5 * T],
        'elia_repeat': "Still.",
        'sequence': [
            {'flag': 'ch10_road', 'group': 'ms_return_road', 'memories_chapter': None, 'flags': [], 'toasts': []},
            {'flag': 'ch10_bridge', 'group': 'ms_return_bridge', 'memories_chapter': None, 'flags': [], 'toasts': []},
        ],
        'triggers': [
            # Sable on the low stone step before the house where Elia drank (the inner room of 6화).
            {'rect': {'origin': [12 * T, 10 * T], 'size': [2 * T, T]}, 'group': 'ms_return_step', 'flag': 'ch10_step', 'gate': 'ch10_bridge'},
            # The east road: the horn, the boy, and the road Arrel does not take.
            {'rect': {'origin': [19 * T, 10 * T], 'size': [2 * T, T]}, 'group': 'ms_return_horn', 'flag': 'ch10_horn', 'gate': 'ch10_step'},
            # The square's west end: three groups, three roads.
            {'rect': {'origin': [4 * T, 12 * T], 'size': [3 * T, T]}, 'group': 'ms_return_square', 'flag': 'ch10_square', 'gate': 'ch10_horn'},
        ],
        'battles_gate': None, 'battles': [],
        # West with Sable and Elia, toward the Belt road; the crossroads below the ridge (16화).
        'exit': {'center': [T + 16, 9 * T], 'size': [T, 2 * T], 'requires': 'ch10_square', 'completes': 'ch10_complete',
                 'group': 'ms_return_crossroads', 'next_chapter': 11, 'next_map': 'roofless_farms', 'next_scene': None,
                 'flags': ['canon_ch11_belt_road_ready'],
                 'notice': {'en': 'Three more places like that before the Belt road.',
                            'ko': '벨트 도로까지 저런 곳이 셋 더 있다.'}},
        'objects_gate': 'ch10_complete',
        'chests': [{'origin': [2 * T, 14 * T], 'flag': 'chest_seam_left_behind', 'rewards': {'grains': 24, 'items': {'potion': 2, 'smoke_bomb': 1}}}],
        'clues': [{'origin': [11 * T, 11 * T], 'flag': 'clue_seam_bowl',
                   'text': 'A bowl left at a door. It did not fit in the basket.'}],
        'resume_blocked': [],
        'encounters_gate': 'ch10_complete',
        'lights': [{'tile': 6, 'color': [0.85, 0.95, 0.9, 1.0], 'energy': 0.35, 'scale': 2.2}],
        # Morning, the drying cloths gone from the walls; a little paler than the first visit.
        'atmosphere': {'saturation': 0.82, 'splash': 'res://assets/cg/generated/chapter_splash_the_seam.png'},
        'ambient_npcs': [], 'ambient_npcs_gate': None,
    },
    # S377: 최종본 17화 「읽는 자」 and 18화 「펠의 손」 on the Godot colorless_waste plain: fields toward the Belt road.
    # Its PILLAR clusters stand as the roofless farmhouse's walls; MARKER tiles are standing stones; RIDGE clusters are
    # field-boundary stones. In from the south (over the ridge), out to the north (the dry channel, the second farmhouse).
    'roofless_farms': {
        'title': {'number': 11, 'name': 'The Roofless Farmhouses', 'subtitle': 'The reader'},
        'spawn': [8 * T + 16, 15 * T + 16],
        'elia_repeat': "It's nothing.",
        'sequence': [
            {'flag': 'ch11_road', 'group': 'ms_farm_road', 'memories_chapter': None, 'flags': [], 'toasts': []},
        ],
        'triggers': [
            # Inside the first farmhouse's standing corner: the book opens (17화).
            {'rect': {'origin': [17 * T, 11 * T], 'size': [2 * T, T]}, 'group': 'ms_farm_house', 'flag': 'ch11_house', 'gate': 'ch11_road'},
            # Along the same wall, toward the low broken west side: the night.
            {'rect': {'origin': [17 * T, 12 * T], 'size': [2 * T, T]}, 'group': 'ms_farm_night', 'flag': 'ch11_night', 'gate': 'ch11_house'},
            # The threshold in the morning: the fourth line, a name (18화).
            {'rect': {'origin': [14 * T, 11 * T], 'size': [2 * T, T]}, 'group': 'ms_farm_fel', 'flag': 'ch11_fel', 'gate': 'ch11_night'},
            # Where the road comes down off the long ridge: the grey glove.
            {'rect': {'origin': [9 * T, 4 * T], 'size': [3 * T, T]}, 'group': 'ms_farm_glove', 'flag': 'ch11_glove', 'gate': 'ch11_fel'},
        ],
        'battles_gate': None, 'battles': [],
        'exit': {'center': [10 * T + 16, 1.5 * T], 'size': [3 * T, T], 'requires': 'ch11_glove', 'completes': 'ch11_complete',
                 'group': 'ms_farm_second', 'next_chapter': 12, 'next_map': 'basin_station', 'next_scene': None,
                 'flags': ['canon_ch12_belt_road_ready'],
                 'notice': {'en': 'Before asking who, he would first tell her he had heard.',
                            'ko': '누구냐고 묻기 전에, 자기가 들었다는 말부터 해야 했다.'}},
        'objects_gate': 'ch11_complete',
        'chests': [{'origin': [21 * T, 14 * T], 'flag': 'chest_farm_field', 'rewards': {'grains': 26, 'items': {'potion': 2, 'antidote': 1}}}],
        'clues': [{'origin': [11 * T, 5 * T], 'flag': 'clue_farm_glove',
                   'text': 'A grey leather glove in the ditch, its cuff open. No one has come back for it.'}],
        'resume_blocked': [],
        'encounters_gate': 'ch11_complete',
        'lights': [],
        # Late light over dry fields: straw and dust, paler than the Seam, warmer than the waste the ground came from.
        'atmosphere': {'saturation': 0.55, 'mood': 0.34, 'hue': [0.46, 0.42, 0.32, 1.0], 'light': [0.84, 0.76, 0.6, 1.0],
                       'fog': 'soft', 'splash': 'res://assets/cg/generated/chapter_splash_belt_waystation.png'},
        'ambient_npcs': [], 'ambient_npcs_gate': None,
    },
    # S379: 최종본 19화 「미라」 (a cutaway: the Central Records in Lumea) and 20화 「회색 우리」 on the Godot belt_waystation
    # yard: the basin waystation at a crossroads, the ration window in its stone house, the queue for blankers along
    # the south wall. They come down from the north (the fork, the signal tower), keep to the high ground on the west,
    # walk round to the east and leave by the east road toward the sheep pen on the dry creek.
    'basin_station': {
        'title': {'number': 12, 'name': 'The Basin Waystation', 'subtitle': 'The grey pen'},
        'spawn': [12 * T + 16, 1 * T + 16],
        'elia_repeat': "I didn't touch my coat there.",
        'sequence': [
            {'flag': 'ch12_mira', 'group': 'ms_basin_mira', 'memories_chapter': None, 'flags': [], 'toasts': []},
            {'flag': 'ch12_road', 'group': 'ms_basin_road', 'memories_chapter': None, 'flags': [], 'toasts': []},
        ],
        'triggers': [
            # Below the fork, the signal tower on the east ridge, and the roofs of the basin coming into view.
            {'rect': {'origin': [11 * T, 4 * T], 'size': [3 * T, T]}, 'group': 'ms_basin_tower', 'flag': 'ch12_tower', 'gate': 'ch12_road'},
            # The higher ground south-west of the house, looking at the ration window and its queue along the south wall.
            {'rect': {'origin': [6 * T, 13 * T], 'size': [2 * T, T]}, 'group': 'ms_basin_queue', 'flag': 'ch12_queue', 'gate': 'ch12_tower'},
            # Round the basin on the far side, sixty paces from the window: Sable's hand on his sleeve.
            {'rect': {'origin': [19 * T, 7 * T], 'size': [2 * T, T]}, 'group': 'ms_basin_walk', 'flag': 'ch12_walk', 'gate': 'ch12_queue'},
        ],
        'battles_gate': None, 'battles': [],
        'exit': {'center': [23 * T + 16, 9 * T], 'size': [T, 2 * T], 'requires': 'ch12_walk', 'completes': 'ch12_complete',
                 'group': 'ms_basin_pen', 'next_chapter': 13, 'next_map': 'verdan_return', 'next_scene': None,
                 'flags': ['canon_ch13_verdan_ready'],
                 'notice': {'en': 'What to do at the next window, he had not yet chosen.',
                            'ko': '다음 창구 앞에서 무엇을 할지는 아직 고르지 않았다.'}},
        'objects_gate': 'ch12_complete',
        'chests': [{'origin': [5 * T, 15 * T], 'flag': 'chest_basin_trough', 'rewards': {'grains': 28, 'items': {'potion': 2, 'antidote': 1}}}],
        'clues': [{'origin': [20 * T, 3 * T], 'flag': 'clue_basin_flag',
                   'text': 'The flag on the signal tower hangs at half-mast. Not mourning. The wind holds south-south-west.'}],
        'resume_blocked': [],
        'encounters_gate': 'ch12_complete',
        'lights': [],
        # Grey daylight over a dry basin; the queue, the clerk's window.
        'atmosphere': {'saturation': 0.5, 'mood': 0.4, 'hue': [0.42, 0.42, 0.4, 1.0], 'light': [0.78, 0.76, 0.72, 1.0],
                       'splash': 'res://assets/cg/generated/chapter_splash_belt_waystation.png'},
        # S383 (T-8): the queue holds. Nobody walks off: each keeps a place and looks toward the ration window
        # (the shutter at tile (11.2, 11)); the clerk behind it looks out at the head of the line.
        'ambient_npcs': [{'position': [400.0, 361.6], 'preset': 'bureau_agent', 'still': True, 'face': [400.0, 406.4]},
                         {'position': [374.4, 406.4], 'preset': 'villager_m', 'still': True, 'face': [374.4, 368.0]},
                         {'position': [400.0, 406.4], 'preset': 'villager_f', 'still': True, 'face': [374.4, 368.0]},
                         {'position': [425.6, 406.4], 'preset': 'elder', 'still': True, 'face': [374.4, 368.0]},
                         {'position': [451.2, 406.4], 'preset': 'traveler', 'still': True, 'face': [374.4, 368.0]},
                         {'position': [476.8, 406.4], 'preset': 'villager_f', 'still': True, 'face': [374.4, 368.0]},
                         {'position': [502.4, 406.4], 'preset': 'fisherman', 'still': True, 'face': [374.4, 368.0]},
                         {'position': [528.0, 406.4], 'preset': 'villager_m', 'still': True, 'face': [374.4, 368.0]},
                         {'position': [556.8, 428.8], 'preset': 'elder', 'still': True, 'face': [374.4, 368.0]},
                         {'position': [556.8, 454.4], 'preset': 'villager_f', 'still': True, 'face': [374.4, 368.0]},
                         {'position': [556.8, 480.0], 'preset': 'traveler', 'still': True, 'face': [374.4, 368.0]},
                         {'position': [556.8, 505.6], 'preset': 'villager_m', 'still': True, 'face': [374.4, 368.0]}],
        # Shown from the arrival on (an empty gate would keep them hidden).
        'ambient_npcs_gate': 'ch12_road',
    },
    # S385: 최종본 22화 「공명」 and 23화 「세 번째 사람」 on the Godot drift_shelter ground (Gemini's T-7 brief: Chapter 14
    # = 22-23화). In from the south: the furrows' morning, the fields, the signal tower to the west that they go round,
    # the stumps under the ridge; then the roofless farmhouse: three high walls and a broken corner, the road from the
    # ridge passing its open side. A night with no fire. At first light Tobias comes down the slope, and stays.
    'ridge_farmhouse': {
        'title': {'number': 14, 'name': 'Resonance', 'subtitle': 'The Third Person'},
        'spawn': [10 * T + 16, 16 * T + 16],
        'elia_repeat': "I'm not opening it. ...Not now.",
        'sequence': [
            {'flag': 'ch14_morning', 'group': 'ms_ridge_morning', 'memories_chapter': None, 'flags': [], 'toasts': []},
        ],
        'triggers': [
            # The fields past the low stone wall: a man hoeing who never looks up.
            {'rect': {'origin': [9 * T, 13 * T], 'size': [3 * T, T]}, 'group': 'ms_ridge_fields', 'flag': 'ch14_fields', 'gate': 'ch14_morning'},
            # West, where the field road bends toward the signal tower; over the broken north wall into the stumps.
            {'rect': {'origin': [4 * T, 11 * T], 'size': [2 * T, T]}, 'group': 'ms_ridge_tower', 'flag': 'ch14_tower', 'gate': 'ch14_fields'},
            # Among the stumps below the ridge: the wet boot, the sleep, the bread kept for him.
            {'rect': {'origin': [4 * T, 2 * T], 'size': [2 * T, T]}, 'group': 'ms_ridge_stumps', 'flag': 'ch14_stumps', 'gate': 'ch14_tower'},
            # The farmhouse's open side, the road from the ridge passing it.
            {'rect': {'origin': [8 * T, 10 * T], 'size': [4 * T, T]}, 'group': 'ms_ridge_house', 'flag': 'ch14_house', 'gate': 'ch14_stumps'},
            # Under the high wall: no fire, the book warmer though nobody touches it.
            {'rect': {'origin': [7 * T, 6 * T], 'size': [3 * T, T]}, 'group': 'ms_ridge_night', 'flag': 'ch14_night', 'gate': 'ch14_house'},
            # The broken corner where Arrel keeps watch: someone stops halfway down the slope.
            {'rect': {'origin': [12 * T, 9 * T], 'size': [2 * T, T]}, 'group': 'ms_ridge_slope', 'flag': 'ch14_slope', 'gate': 'ch14_night'},
            # The edge of the road, where the clerk sits on a stone with the book on his knees.
            {'rect': {'origin': [9 * T, 11 * T], 'size': [3 * T, T]}, 'group': 'ms_ridge_ten_years', 'flag': 'ch14_ten_years', 'gate': 'ch14_slope'},
        ],
        'battles_gate': None, 'battles': [],
        # First light inside the walls: names, and a third person sitting down a few steps from Elia.
        'exit': {'center': [10 * T, 9 * T + 16], 'size': [2 * T, T], 'requires': 'ch14_ten_years', 'completes': 'ch14_complete',
                 'group': 'ms_ridge_third', 'next_chapter': 15, 'next_map': None, 'next_scene': None,
                 'flags': ['canon_ch15_two_notes_ready'],
                 'notice': {'en': "I'd like to come with you.", 'ko': '같이 가고 싶습니다.'}},
        'objects_gate': 'ch14_complete',
        'chests': [{'origin': [2 * T, 3 * T], 'flag': 'chest_ridge_stumps', 'rewards': {'grains': 32, 'items': {'potion': 2, 'antidote': 1}}}],
        'clues': [{'origin': [6 * T, 8 * T], 'flag': 'clue_ridge_shelf',
                   'text': 'A shelf warped by rain. Two round marks on top, and under them a long strip less faded than the rest.'}],
        'resume_blocked': [],
        'encounters_gate': 'ch14_complete',
        # Drift's rats are too small for Chapter 14: the waste's creatures stand in.
        'encounters_from': 'colorless_waste',
        # No fire, no lamp (22화: "불은 피우지 않았다"; 23화: "불빛은 없었다").
        'lights': [],
        # Afternoon going to a night without fire; dry fields and stumps under a grey sky. No picture: every candidate
        # shows a lit lantern (the dusk road), a burning ground, other people or the wrong Elia, so the scenes play dark.
        'atmosphere': {'saturation': 0.56, 'mood': 0.46, 'hue': [0.36, 0.37, 0.42, 1.0], 'light': [0.7, 0.72, 0.78, 1.0], 'fog': 'none',
                       'splash': ''},
        'ambient_npcs': [], 'ambient_npcs_gate': None,
    },
    # S382: 최종본 21화 「다시 베르단」 on the Godot verdan_market ground (Gemini's T-7 brief: Chapter 13 = 21화 alone).
    # Verdan has no wall: the road from the fields runs straight into the first stalls. In from the north (the slope where
    # Sable turns back), the market emptying round the square and its reservoir, the Sump's door at the south of the
    # alley ring (DOOR tiles) sealed with cord and red wax, the oil stall on the east corner, the alley, and out east
    # into the old fields.
    'verdan_return': {
        'title': {'number': 13, 'name': 'Verdan, Again', 'subtitle': 'Filed'},
        'spawn': [14 * T + 16, 1 * T + 16],
        'elia_repeat': "It used to be loud even at this hour.",
        'sequence': [
            {'flag': 'ch13_road', 'group': 'ms_verdan_road', 'memories_chapter': None, 'flags': [], 'toasts': []},
            {'flag': 'ch13_farewell', 'group': 'ms_verdan_farewell', 'memories_chapter': None, 'flags': [], 'toasts': []},
        ],
        'triggers': [
            # The market emptying, north of the square: carts going out, the reservoir by the old barn.
            {'rect': {'origin': [13 * T, 8 * T], 'size': [3 * T, T]}, 'group': 'ms_verdan_market', 'flag': 'ch13_market', 'gate': 'ch13_farewell'},
            # The Sump: three flights down to the door, cord and red wax, the Bureau's seal.
            {'rect': {'origin': [13 * T, 15 * T], 'size': [2 * T, T]}, 'group': 'ms_verdan_sump', 'flag': 'ch13_sump', 'gate': 'ch13_market'},
            # The oil stall on the east corner.
            {'rect': {'origin': [22 * T, 10 * T], 'size': [2 * T, T]}, 'group': 'ms_verdan_vendor', 'flag': 'ch13_vendor', 'gate': 'ch13_sump'},
            # The alley past the corner, where the oil smell thins.
            {'rect': {'origin': [25 * T, 13 * T], 'size': [2 * T, T]}, 'group': 'ms_verdan_alley', 'flag': 'ch13_alley', 'gate': 'ch13_vendor'},
        ],
        'battles_gate': None, 'battles': [],
        # Out by a street they had not come in by, east into the old fields; the night where two furrows meet.
        'exit': {'center': [28 * T + 16, 16 * T], 'size': [T, 2 * T], 'requires': 'ch13_alley', 'completes': 'ch13_complete',
                 'group': 'ms_verdan_furrow', 'next_chapter': 14, 'next_map': 'ridge_farmhouse', 'next_scene': None,
                 'flags': ['canon_ch14_resonance_ready'],
                 'notice': {'en': 'North-west. When it gets light, we go.',
                            'ko': '북서쪽. 밝아지면 가 보자.'}},
        'objects_gate': 'ch13_complete',
        'chests': [{'origin': [2 * T, 18 * T], 'flag': 'chest_verdan_cart', 'rewards': {'grains': 30, 'items': {'potion': 2, 'antidote': 1}}}],
        'clues': [{'origin': [12 * T, 16 * T], 'flag': 'clue_verdan_seal',
                   'text': 'Cord from the frame to the door, and red wax where the cords cross. The Bureau seal. No sound below.'}],
        'resume_blocked': [],
        'encounters_gate': 'ch13_complete',
        # The Chapter 2 market's rats are too small for Chapter 13: the waste's creatures stand in.
        'encounters_from': 'colorless_waste',
        'lights': [],
        # Daytime, the market thinning: warm smoke from a few stalls, the rest packing.
        'atmosphere': {'saturation': 0.66, 'mood': 0.34, 'splash': 'res://assets/cg/generated/chapter_splash_verdan_market.png'},
        'ambient_npcs': [{'position': [307.2, 140.8], 'preset': 'villager_f'},
                         {'position': [620.8, 147.2], 'preset': 'villager_m'},
                         {'position': [208.0, 307.2], 'preset': 'elder'},
                         {'position': [691.2, 524.8], 'preset': 'villager_m'},
                         {'position': [787.2, 307.2], 'preset': 'traveler'},
                         {'position': [147.2, 531.2], 'preset': 'villager_f'}],
        'ambient_npcs_gate': 'ch13_road',
    },
}


def manuscript_map(map_id, chapter, prefix, src, ground):
    """A chapter map whose ground is the Godot script's and whose story is the manuscript's."""
    story = MANUSCRIPT[map_id]
    if story.get('encounters_from'):
        src = (ROOT / 'scenes/maps' / (story['encounters_from'] + '.gd')).read_text(encoding='utf-8').replace('\r\n', '\n')
    pool = [json.loads(entry) for entry in re.findall(r'\{"name": [^}]*\}', func_body(src, '_setup_random_encounters'))]
    rng = re.search(r',\s*(\d+),\s*(\d+)\s*\)\s*$', func_body(src, '_setup_random_encounters').strip())
    lights = []
    for spec in story['lights']:
        for y, row in enumerate(ground['tiles']):
            for x, v in enumerate(row):
                if v == spec['tile']:
                    lights.append({'kind': 'lantern', 'light': True, 'color': spec['color'], 'energy': spec['energy'], 'scale': spec['scale'],
                                   'origin': [(x + .5) * ground['tile_size'], (y + .5) * ground['tile_size']]})
    # The village's cliffs stand as walls (the ported maps' WALL), its huts stay HUT and are built by name.
    if GROUND.get(map_id, map_id) == 'the_seam':
        ground = dict(ground, tile_names=['WALL' if n == 'CLIFF' else n for n in ground['tile_names']])
    if map_id == 'crumbling_coast':
        # The coast's cliffs stand as broken ruin stone (the ported maps' RUBBLE).
        ground = dict(ground, tile_names=['RUBBLE' if n == 'CLIFF' else n for n in ground['tile_names']])
    if map_id == 'ridge_farmhouse':
        # The rim is open field, not a wall; the slab ring stands as the farmhouse's high stone walls.
        ground = dict(ground, tile_names=['WALL' if n == 'CONCRETE' else 'EDGE' if n == 'WALL' else n for n in ground['tile_names']])
        tiles = [list(r) for r in ground['tiles']]
        mud, path, wall = (ground['tile_names'].index(n) for n in ('MUD', 'PATH', 'WALL'))
        tiles[5][10] = wall                     # the back wall is whole: three high walls, the south side open
        for x in range(8, 13): tiles[1][x] = mud
        for y in range(2, 5): tiles[y][10] = mud
        for y in range(1, 11): tiles[y][20] = path   # the road down from the ridge, east of the house
        for x in range(11, 21): tiles[11][x] = path  # and along the open side to the field road
        edge = ground['tile_names'].index('EDGE')
        for y in range(8, 11): tiles[y][24] = edge   # the shelter's east way out is closed: they stay the night
        ground = dict(ground, tiles=tiles)
    if map_id == 'roofless_farms':
        # Open fields: the plain's rim is not a wall here (no masonry along the edge); hedges and trees stand beyond it.
        ground = dict(ground, tile_names=['EDGE' if n == 'WALL' else n for n in ground['tile_names']])
    if 'atmosphere' in story:
        ground = dict(ground, atmosphere=dict(ground['atmosphere'], **story['atmosphere']))
    keys = ('title', 'spawn', 'elia_repeat', 'sequence', 'triggers', 'exit', 'objects_gate', 'chests', 'clues', 'battles_gate',
            'battles', 'resume_blocked', 'encounters_gate', 'ambient_npcs', 'ambient_npcs_gate')
    return dict(ground, schema_version=1, map=map_id, chapter=chapter, asset_prefix=prefix,
                source='scenes/maps/%s.gd (ground) + 최종본 %s화 (story)' % (GROUND.get(map_id, map_id), EPISODES.get(map_id, '?')),
                encounters=[{k: e[k] for k in ('name', 'hp', 'atk', 'is_void', 'abilities') if k in e} for e in pool],
                encounter_range=[int(rng.group(1)), int(rng.group(2))] if rng else None,
                decorations=lights, **{k: story[k] for k in keys})


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
    path = ROOT / 'scenes/maps' / (GROUND.get(map_id, map_id) + '.gd')
    src = path.read_text(encoding='utf-8').replace('\r\n', '\n')
    tile = int(re.search(r'const TILE_SIZE: int = (\d+)', src).group(1))
    width = int(re.search(r'const MAP_WIDTH: int = (\d+)', src).group(1))
    height = int(re.search(r'const MAP_HEIGHT: int = (\d+)', src).group(1))
    grid = re.search(r'var map_data: Array = \[(.*?)\n\]', src, re.S).group(1)
    tiles = [[int(v) for v in row.split(',') if v.strip()] for row in re.findall(r'\[([\d,\s]+)\]', grid)]
    if len(tiles) != height or any(len(r) != width for r in tiles):
        raise ValueError('map_data does not match MAP_WIDTH x MAP_HEIGHT')
    defs = re.search(r'_tile_defs = \[(.*?)\n\t\]', src, re.S).group(1)
    # A detail may be a variable (rim_forest: grass_detail := "grass_clean" if ... else "grass"): take its plain value.
    def detail(d):
        if d.startswith('"'): return d.strip('"')
        v = re.search(r'var ' + d + r' := "([^"]+)"(?: if .* else "([^"]+)")?', src)
        return v.group(2) or v.group(1)
    tile_defs = [{'color': color(c), 'detail': detail(d)} for c, d in re.findall(r'"color": (Color\([^)]*\)), "detail": ("[^"]+"|\w+)', defs)]
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
    if map_id in MANUSCRIPT:
        ground = {'dialogue_file': dialogue_file, 'tile_size': tile, 'width': width, 'height': height, 'tiles': tiles,
                  'tile_defs': tile_defs, 'tile_names': tile_names, 'solid': solid, 'atmosphere': atmosphere}
        return manuscript_map(map_id, chapter, prefix, src, ground)

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
