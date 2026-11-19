"""S362 hero finishing. Blender 4.5 CPU authoring, accepted S361/R4 inputs only.

blender -b -t 2 --python this_file -- --project-root <checkout> --revision R1
No original FBX/package, image, rig or clip is changed. Output revisions are immutable.
Elia's recorded S361 hair is replaced; dedicated new eyelid geometry carries blink
morphs. Arrel's painted face keeps its vertices, UVs, weights and colors: only
strictly local face corner normals are blended to remove scanning facets.
"""
from collections import Counter, defaultdict
from pathlib import Path
import argparse
import hashlib
import importlib.util
import json
import math
import sys

import bpy
from mathutils import Matrix, Vector


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def write_json(path, value):
    path.write_text(json.dumps(value, indent=2) + '\n', encoding='utf-8')


def load_helpers(root):
    path = root / 'Unreal/ArtSource/HeroPolishS361/R4/generator_snapshot.py'
    spec = importlib.util.spec_from_file_location('accepted_s361_r4', path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module, path


def import_accepted(s, root, name):
    path = root / 'Unreal/ArtSource/HeroPolishS361/R4' / name / (name + '.fbx')
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.fbx(filepath=str(path), colors_type='LINEAR')
    rigs = [o for o in bpy.data.objects if o.type == 'ARMATURE']
    meshes = [o for o in bpy.data.objects if o.type == 'MESH']
    assert len(rigs) == len(meshes) == 1, (name, 'Unexpected accepted source objects')
    s.RIG, s.BODY = rigs[0], meshes[0]
    s.BODY.parent = None
    s.BODY.matrix_world = Matrix.Identity(4)
    s.RIG.matrix_world = Matrix.Identity(4)
    s.RIG.animation_data_clear()
    for bone in s.RIG.pose.bones:
        bone.matrix_basis = Matrix.Identity(4)
    bpy.context.scene.unit_settings.system = 'METRIC'
    bpy.context.scene.unit_settings.scale_length = .01
    points = [v.co for v in s.BODY.data.vertices]
    s.H = max(p.z for p in points) - min(p.z for p in points)
    s.K = s.H / 180.0
    s.TOP = max(p.z for p in points)
    s.CURRENT_NAME = name
    s.SOURCE_BOUNDS = s.bounds(points)
    cy = s.bn('head').y
    s.HAIR_LIMITS = [[-10.5 * s.K, 10.5 * s.K],
                     [cy - 12 * s.K, cy + 12 * s.K],
                     [s.TOP - 31 * s.K, s.TOP + .1]]
    assert 160 < s.H < 185 and len(s.RIG.data.bones) == 77
    assert len(s.BODY.data.materials) == (2 if name == 'Arrel' else 4)
    assert s.BODY.data.shape_keys is None, 'Accepted source unexpectedly has morphs'
    s.normalize_colors(s.BODY)
    s.PARTS, s.COMPONENTS = [s.BODY], []
    s.CRAFT = bpy.data.materials.new('S362_HeroFinish')
    s.CRAFT.use_nodes = True
    vc = s.CRAFT.node_tree.nodes.new('ShaderNodeVertexColor')
    vc.layer_name = 'Color'
    s.CRAFT.node_tree.links.new(vc.outputs['Color'],
        s.CRAFT.node_tree.nodes['Principled BSDF'].inputs['Base Color'])
    s.CRAFT.node_tree.nodes['Principled BSDF'].inputs['Roughness'].default_value = .48
    return path


def surface_signature(s, ob, materials=None, decimals=5):
    """Include pigment and every UV channel; compare retained surfaces verbatim."""
    me = ob.data
    uvs = [u.name for u in me.uv_layers]
    pigment = s.colors(me)
    result = Counter()
    for face in me.polygons:
        if materials is not None and face.material_index not in materials:
            continue
        for li in face.loop_indices:
            v = me.vertices[me.loops[li].vertex_index]
            position = tuple(round(a, decimals) for a in v.co)
            uv = tuple(tuple(round(a, decimals) for a in me.uv_layers[n].data[li].uv) for n in uvs)
            weights = tuple(sorted((ob.vertex_groups[g.group].name, round(g.weight, 6))
                                   for g in v.groups if g.weight > 1e-7))
            rgba = tuple(round(a, 5) for a in pigment[li])
            result[(face.material_index, position, uv, weights, rgba)] += 1
    return uvs, result


def assert_signature(before, after, ratio=1.0):
    assert before[0] == after[0], ('UV channel order changed', before[0], after[0])
    total = sum(before[1].values())
    retained = sum((before[1] & after[1]).values())
    actual = retained / max(1, total)
    assert actual >= ratio, ('Retained surface loss', retained, total, actual)
    return {'corners': total, 'retained': retained, 'ratio': actual,
            'includes': 'material,position,all UVs,weights,RGBA', 'uv_channels': before[0]}


def remove_recorded_hair(s, root):
    receipt = json.loads((root / 'Unreal/ArtSource/HeroPolishS361/R4/Elia/geometry.json').read_text())
    expected = [c for c in receipt['components'] if c['category'] == 'hair']
    assert len(expected) == 18
    me = s.BODY.data
    rgba = s.colors(me)
    actual = []
    for region in s.connected_regions(s.BODY, material=3):
        if all(abs(rgba[li][3] - .20) < .001
               for fi in region['polygons'] for li in me.polygons[fi].loop_indices):
            actual.append(region)
    assert len(actual) == len(expected), ('Recorded hair island count', len(actual))
    remaining = list(expected)
    removed = set()
    matches = []
    for region in actual:
        candidates = [c for c in remaining if c['triangles'] == region['triangles']
                      and max(abs(a - b) for aa, bb in zip(c['bounds_cm'], region['bounds_cm'])
                              for a, b in zip(aa, bb)) < .002]
        assert len(candidates) == 1, ('Not a uniquely recorded S361 hair island', region['bounds_cm'])
        record = candidates[0]
        remaining.remove(record)
        removed.update(region['polygons'])
        matches.append({'name': record['name'], 'triangles': region['triangles'],
                        'bounds_cm': region['bounds_cm']})
    assert not remaining
    assert sum(r['triangles'] for r in actual) == 25204
    # Hash/signature all surviving polygons, including old lashes/catchlights.
    kept = set(range(len(me.polygons))) - removed
    before = polygon_signature(s, s.BODY, kept)
    s.remove_polygons(s.BODY, removed)
    after = polygon_signature(s, s.BODY, set(range(len(s.BODY.data.polygons))))
    assert_signature(before, after)
    return {'policy': 'Only18 exact R4 hair components, slot3 alpha.20',
            'triangles': 25204, 'matches': matches,
            'other_surfaces_exactly_retained': True}


def polygon_signature(s, ob, selected):
    me = ob.data
    uv_names = [u.name for u in me.uv_layers]
    rgba = s.colors(me)
    result = Counter()
    for f in me.polygons:
        if f.index not in selected:
            continue
        for li in f.loop_indices:
            v = me.vertices[me.loops[li].vertex_index]
            result[(f.material_index, tuple(v.co),
                    tuple(tuple(me.uv_layers[n].data[li].uv) for n in uv_names),
                    tuple(sorted((ob.vertex_groups[g.group].name, round(g.weight, 6))
                                 for g in v.groups if g.weight > 1e-7)),
                    tuple(rgba[li]))] += 1
    return uv_names, result


def arrel_local_normals(s):
    me = s.BODY.data
    old_normals = [n.vector.copy() for n in me.corner_normals]
    head = {v.index for v in me.vertices if sum(g.weight for g in v.groups
            if s.BODY.vertex_groups[g.group].name == 'head') > .70}
    assert len(head) > 200
    bb = s.bounds([me.vertices[i].co for i in head])
    face_center_z = bb[2][0] + (bb[2][1] - bb[2][0]) * .40
    face_half = min(abs(bb[0][0]), abs(bb[0][1])) * .62
    normals = defaultdict(lambda: Vector((0, 0, 0)))
    for f in me.polygons:
        if f.material_index != 0:
            continue
        for vi in f.vertices:
            if vi in head:
                key = tuple(round(a, 5) for a in me.vertices[vi].co)
                normals[key] += f.normal * f.area
    for n in normals.values():
        if n.length > .0001:
            n.normalize()
    updated, changed, max_angle = list(old_normals), [], 0.0
    for f in me.polygons:
        if f.material_index != 0:
            continue
        for li in f.loop_indices:
            vi = me.loops[li].vertex_index
            if vi not in head:
                continue
            p, old = me.vertices[vi].co, old_normals[li]
            # Front cheek/nose/chin only. Preserve hair, ears, eye paint and
            # garment normals outside this spatially bounded blend.
            if old.y >= -.28 or p.y >= s.bn('head').y or abs(p.x) > face_half:
                continue
            local = ((p.z - face_center_z) / (7.0 * s.K)) ** 2 + (p.x / max(face_half, .1)) ** 4
            amount = .70 * math.exp(-local)
            if amount < .07:
                continue
            target = normals[tuple(round(a, 5) for a in p)]
            if target.length < .5 or target.dot(old) < .2:
                continue
            n = old.lerp(target, amount).normalized()
            if (n - old).length > .00001:
                updated[li] = n
                changed.append(li)
                max_angle = max(max_angle, old.angle(n))
    assert len(changed) >= 30, ('No local painted-face normal refinement', len(changed))
    me.normals_split_custom_set(updated)
    actual = [n.vector.copy() for n in me.corner_normals]
    changed_set = set(changed)
    outside_error = max(((actual[i] - old_normals[i]).length for i in range(len(actual))
                         if i not in changed_set), default=0)
    assert outside_error < 2e-4, ('Non-face normals changed', outside_error)
    return {'changed_face_corners': len(changed), 'max_normal_angle_degrees': math.degrees(max_angle),
            'outside_face_max_normal_error': outside_error, 'positions_changed': 0,
            'UVs_changed': 0, 'paint_changed': 0, 'material_slots_changed': 0,
            'head_bounds_cm': bb, 'face_center_z_cm': face_center_z}


def eye_regions(s):
    eyes = []
    for region in s.connected_regions(s.BODY, material=2):
        bb = region['bounds_cm']
        if region['unique_vertices'] == 610 and bb[2][1] - bb[2][0] < 3 * s.K:
            eyes.append({'vertices': region['vertices'],
                         'center': Vector([(a + b) / 2 for a, b in bb]),
                         'radius': (bb[2][1] - bb[2][0]) / 2})
    assert len(eyes) == 2, ('Two retained eye spheres required', len(eyes))
    return sorted(eyes, key=lambda e: e['center'].x)


def refine_elia_face(s, eyes):
    # Mark retained eye surfaces for a gentler, more reflective material. Do not
    # change eye size/position, iris center, skin topology or original garments.
    eye_ids = set().union(*(e['vertices'] for e in eyes))
    old = s.colors(s.BODY.data)
    pigment = s.BODY.data.color_attributes['Color']
    eye_loops = 0
    for face in s.BODY.data.polygons:
        if face.material_index != 2:
            continue
        for li in face.loop_indices:
            vi = s.BODY.data.loops[li].vertex_index
            color = old[li]
            if vi in eye_ids:
                pigment.data[li].color = (*color[:3], .06)
                eye_loops += 1
    assert eye_loops > 2000
    return {'retained_eye_loops_marked_for_wet_finish': eye_loops,
            'eye_position_size_topology_unchanged': True,
            'skin_pigment_source': 'accepted S361/R4', 'eye_alpha_material_marker': .06}


def authored_elia_clumps(s, eye_z):
    k, top, cy = s.K, s.TOP, s.bn('head').y
    face = s.Surface(s.BODY, lambda f: f.material_index == 2)
    def hair_color(p):
        # Soft cool root-to-tip variation, not a uniform plastic helmet.
        lift = s.clamp((p.z - (top - 24 * k)) / (22 * k))
        flow = .022 * math.sin(p.x / k * .71 + p.z / k * .19)
        return (.53 + .045 * lift + flow, .55 + .047 * lift + flow,
                .575 + .050 * lift + flow)
    s.hair_color = hair_color
    # Recessed upper core prevents scalp gaps; its lower edge is hidden by
    # authored, nonuniform groups. No repeated scallop frequency or even tips.
    rows, sectors = 25, 80
    verts, faces = [], []
    for row in range(rows):
        t = row / rows
        for i in range(sectors):
            angle = math.tau * i / sectors
            front = max(0, math.cos(angle))
            rear = top - (17.8 + .65 * math.sin(angle + .7) + .42 * math.cos(3 * angle - .2)) * k
            opening = eye_z + (5.9 + 1.0 * math.sin(angle + .38)) * k
            bottom = rear * (1 - front ** 1.35) + opening * front ** 1.35
            z = bottom + (top - .07 * k - bottom) * t
            direction = Vector((math.sin(angle), -math.cos(angle), 0))
            center = Vector((0, cy, z))
            hit, normal, index, distance = face.tree.ray_cast(center + direction * 35 * k, -direction, 70 * k)
            if hit is None:
                radius = max(.15, 7.6 * math.sqrt(max(.001, 1 - ((z - (top - 12.5 * k)) / (12.8 * k)) ** 2))) * k
                hit = center + direction * radius
            verts.append(center + direction * ((hit - center).length + .16 * k))
    tip = len(verts)
    verts.append(Vector((-.45 * k, cy, top - .06 * k)))
    for row in range(rows - 1):
        for i in range(sectors):
            a, b = row * sectors + i, row * sectors + (i + 1) % sectors
            faces.append((a, b, b + sectors, a + sectors))
    for i in range(sectors):
        faces.append(((rows - 1) * sectors + i, (rows - 1) * sectors + (i + 1) % sectors, tip))
    core = s.make_part('Elia_RecessedAsymmetricScalpCore', verts, faces,
                       s.fixed('head'), hair_color, 'hair', .10 * k)
    cap = s.Surface(core)
    # Different part roots, widths and tip heights; the long sweep and shorter
    # broken lock remain intentionally asymmetric across the forehead.
    fringe = [
        [(-3.0, top - 1.9*k), (-1.9, top - 5.0*k), (4.2, eye_z + 1.60*k), (.32, 2.30, .035)],
        [(-3.7, top - 2.7*k), (-5.0, top - 6.0*k), (-5.9, eye_z + 2.4*k), (.27, 1.72, .030)],
        [(-1.8, top - 1.8*k), (2.4, top - 5.6*k), (6.3, eye_z + .30*k), (.29, 1.73, .030)],
        [(-.6, top - 3.0*k), (4.3, top - 7.0*k), (7.0, eye_z - 1.9*k), (.25, 1.31, .025)],
        [(-4.0, top - 4.2*k), (-6.2, top - 8.5*k), (-6.8, eye_z - .45*k), (.24, 1.05, .025)],
    ]
    for i, spec in enumerate(fringe):
        controls = [(x * k, cy - 10*k, z) for x, z in spec[:3]]
        s.swept_fringe('Elia_AuthoredSweep_%02d' % i, face, cap, controls,
                       [w * k for w in spec[3]], cy)
    # (angle, root-depth, belly-depth, tip-depth, breadth, sweep-angle)
    # These authored groups deliberately break mirror symmetry and regular tip spacing.
    groups = [
        (.88, 5.0, 15.0, 24.2, 1.72, .08),
        (1.17, 6.8, 14.7, 23.1, 1.32, -.07),
        (1.51, 7.6, 15.9, 24.6, 1.63, .10),
        (1.94, 6.7, 15.0, 22.4, 1.72, -.09),
        (2.40, 8.2, 17.0, 24.8, 1.48, .07),
        (2.91, 6.1, 15.9, 23.5, 1.89, -.10),
        (3.40, 8.4, 17.3, 25.1, 1.42, .09),
        (3.88, 6.4, 15.4, 22.9, 1.76, -.08),
        (4.31, 7.9, 16.6, 24.0, 1.35, .12),
        (4.70, 5.8, 14.4, 22.1, 1.58, -.07),
        (5.10, 7.2, 15.6, 23.8, 1.26, .10),
        (5.38, 5.4, 13.7, 21.5, 1.34, -.06),
    ]
    for i, (angle, root_depth, belly_depth, tip_depth, breadth, sweep) in enumerate(groups):
        direction = Vector((math.sin(angle), -math.cos(angle), 0))
        center = Vector((0, cy, top - root_depth * k))
        hit, normal, index, distance = cap.tree.ray_cast(center + direction * 25*k, -direction, 50*k)
        assert hit is not None, ('Clump root missed recessed core', i)
        middle_dir = Vector((math.sin(angle + sweep), -math.cos(angle + sweep), 0))
        tip_dir = Vector((math.sin(angle + sweep * 1.7), -math.cos(angle + sweep * 1.7), 0))
        middle = Vector((0, cy, top - belly_depth*k)) + middle_dir * (8.35*k)
        end = Vector((0, cy, top - tip_depth*k)) + tip_dir * ((6.5 + .25*math.sin(i*1.8))*k)
        s.curved_hair_lock('Elia_AuthoredGroupedClump_%02d' % i,
            [hit - direction*.035*k, middle, end],
            [.46*k, breadth*k, .025*k], [.10*k, .38*k, .022*k],
            Vector((math.cos(angle), math.sin(angle), 0)))
    return {'style': 'asymmetric side part, five swept fringes, twelve grouped clumps',
            'repeated_tip_frequency': None, 'group_specs': groups,
            'new_components': list(s.COMPONENTS)}


def dedicated_lids(s, eyes):
    result = []
    for index, eye in enumerate(eyes):
        center, radius = eye['center'], eye['radius']
        rows, columns = 6, 24
        opened, closed, faces = [], [], []
        for row in range(rows):
            v = row / (rows - 1)
            for column in range(columns):
                u = column / (columns - 1)
                dx = (u * 2 - 1) * radius * .97
                vertical = math.sqrt(max(.001, radius*radius - dx*dx))
                # The open strip sits above the iris. Closing expands the same
                # actual lid skin down the front sphere, never moving the eye.
                open_z = vertical * (.96 - .25 * v)
                closed_z = vertical * (.96 - 1.91 * v)
                for target, dz in ((opened, open_z), (closed, closed_z)):
                    dy = -math.sqrt(max(.0001, radius*radius - dx*dx - dz*dz)) - .035*s.K
                    target.append(center + Vector((dx, dy, dz)))
        for row in range(rows - 1):
            for column in range(columns - 1):
                a = row * columns + column
                faces.append((a, a + 1, a + columns + 1, a + columns))
        # Closed thin volume, with the back layer tucked behind its skin surface.
        count = len(opened)
        for arr in (opened, closed):
            arr.extend([p + Vector((0, .020*s.K, 0)) for p in arr[:count]])
        original_faces = list(faces)
        faces.extend([tuple(vi + count for vi in reversed(f)) for f in original_faces])
        perimeter = list(range(columns))
        perimeter += [row*columns + columns-1 for row in range(1, rows)]
        perimeter += list(range((rows-1)*columns + columns-2, (rows-1)*columns-1, -1))
        perimeter += [row*columns for row in range(rows-2, 0, -1)]
        for a, b in zip(perimeter, perimeter[1:] + perimeter[:1]):
            faces.append((a, b, b + count, a + count))
        side = 'L' if center.x > 0 else 'R'
        lid = s.make_part('Elia_DedicatedUpperLid_' + side, opened, faces,
                         s.fixed('head'), (.47, .385, .325), 'skin')
        group = lid.vertex_groups.new(name='S362_Lid_' + side)
        group.add(list(range(len(opened))), 1.0, 'REPLACE')
        # Store target coordinates in a point attribute; joining preserves it.
        attribute = lid.data.attributes.new('S362_ClosedLid', 'FLOAT_VECTOR', 'POINT')
        for item, point in zip(attribute.data, closed):
            item.vector = point
        result.append({'side': side, 'name': lid.name, 'vertices': len(opened),
                       'center_cm': list(center), 'radius_cm': radius,
                       'opened_iris_clearance_radius': .71,
                       'closed_coverage_radius': .95})
    return result


def join_and_morph(s, name, destination, bones, original_materials, garment_before):
    for obj in s.PARTS:
        s.normalize_colors(obj)
    bpy.ops.object.select_all(action='DESELECT')
    for obj in s.PARTS:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = s.BODY
    if len(s.PARTS) > 1:
        bpy.ops.object.join()
    s.BODY.name = name + '_HeroFinishS362'
    expected_materials = original_materials + ([s.CRAFT.name] if name == 'Elia' else [])
    assert [m.name for m in s.BODY.data.materials] == expected_materials
    for modifier in list(s.BODY.modifiers):
        s.BODY.modifiers.remove(modifier)
    if any(len(f.vertices) != 3 for f in s.BODY.data.polygons):
        modifier = s.BODY.modifiers.new('AddedLidHairTriangles', 'TRIANGULATE')
        bpy.context.view_layer.objects.active = s.BODY
        bpy.ops.object.modifier_apply(modifier=modifier.name)
    s.BODY.parent = s.RIG
    s.BODY.matrix_parent_inverse = Matrix.Identity(4)
    s.BODY.matrix_basis = Matrix.Identity(4)
    s.BODY.modifiers.new('UnchangedHeroSkin', 'ARMATURE').object = s.RIG
    s.normalize_colors(s.BODY)
    morphs = []
    if name == 'Elia':
        attribute = s.BODY.data.attributes['S362_ClosedLid']
        targets = [item.vector.copy() for item in attribute.data]
        s.BODY.shape_key_add(name='Basis', from_mix=False)
        for side in ('L', 'R'):
            group = s.BODY.vertex_groups['S362_Lid_' + side]
            ids = {v.index for v in s.BODY.data.vertices if any(g.group == group.index and g.weight > .5 for g in v.groups)}
            assert len(ids) == 288, (side, 'Dedicated lid vertex set', len(ids))
            key = s.BODY.shape_key_add(name='HeroBlink_' + side, from_mix=False)
            for vi in ids:
                key.data[vi].co = targets[vi]
            moved = [i for i, v in enumerate(key.data) if (v.co - s.BODY.data.vertices[i].co).length > 1e-6]
            assert moved and set(moved).issubset(ids)
            morphs.append({'name': key.name, 'dedicated_vertices': len(ids), 'moved_vertices': len(moved),
                           'max_displacement_cm': max((key.data[i].co - s.BODY.data.vertices[i].co).length for i in moved),
                           'moves_original_surface_vertices': False})
        s.BODY.data.attributes.remove(s.BODY.data.attributes['S362_ClosedLid'])
        # Marker groups are authoring metadata only, never skeleton influences.
        for side in ('L', 'R'):
            s.BODY.vertex_groups.remove(s.BODY.vertex_groups['S362_Lid_' + side])
    bind = s.compare_bones(bones, s.bone_snapshot(s.RIG))
    garments = assert_signature(garment_before,
        surface_signature(s, s.BODY, {0, 1} if name == 'Elia' else {0, 1}))
    envelope = s.check_source_xy_envelope(name + '_S362_before_export', s.bounds([v.co for v in s.BODY.data.vertices]))
    assert max(v.co.z for v in s.BODY.data.vertices) <= s.TOP + .1
    assert len(s.BODY.data.polygons) < 100000
    bpy.ops.object.select_all(action='DESELECT')
    s.BODY.select_set(True)
    s.RIG.select_set(True)
    bpy.context.view_layer.objects.active = s.RIG
    bpy.ops.export_scene.fbx(filepath=str(destination), use_selection=True,
        object_types={'ARMATURE', 'MESH'}, axis_forward='-Y', axis_up='Z',
        apply_unit_scale=True, apply_scale_options='FBX_SCALE_UNITS',
        add_leaf_bones=False, use_armature_deform_only=True, path_mode='RELATIVE',
        bake_anim=False, mesh_smooth_type='FACE', colors_type='LINEAR',
        use_mesh_modifiers=False)
    return {'binds': bind, 'garments': garments, 'source_xy_envelope': envelope,
            'triangles': len(s.BODY.data.polygons), 'vertices': len(s.BODY.data.vertices),
            'material_slots': expected_materials, 'morphs': morphs}


def roundtrip(s, name, destination, before, bones, garment_before):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.fbx(filepath=str(destination), colors_type='LINEAR')
    rigs = [o for o in bpy.data.objects if o.type == 'ARMATURE']
    meshes = [o for o in bpy.data.objects if o.type == 'MESH']
    assert len(rigs) == len(meshes) == 1
    rig, obj = rigs[0], meshes[0]
    bind = s.compare_bones(bones, s.bone_snapshot(rig), 5e-4)
    assert len(obj.data.polygons) == before['triangles']
    assert [m.name for m in obj.data.materials] == before['material_slots']
    # FBX stores float values; use the same explicit5dp contract as S361.
    garments = assert_signature(garment_before, surface_signature(s, obj, {0, 1}), .9999)
    morphs = []
    if name == 'Elia':
        assert obj.data.shape_keys is not None
        basis = obj.data.shape_keys.key_blocks['Basis']
        for side in ('L', 'R'):
            key = obj.data.shape_keys.key_blocks.get('HeroBlink_' + side)
            assert key is not None, ('FBX discarded eyelid morph', side)
            moved = [i for i, v in enumerate(key.data) if (v.co - basis.data[i].co).length > 1e-5]
            assert 100 <= len(moved) <= 400
            bb = s.bounds([basis.data[i].co for i in moved])
            assert bb[2][1] - bb[2][0] < 3*s.K
            assert all(abs(v.co.x) < 6*s.K for i, v in enumerate(basis.data) if i in moved)
            morphs.append({'name': key.name, 'moved_vertices': len(moved), 'local_bounds_cm': bb,
                           'max_displacement_cm': max((key.data[i].co - basis.data[i].co).length for i in moved)})
    else:
        assert obj.data.shape_keys is None
    return {'binds': bind, 'garments': garments, 'morphs': morphs,
            'triangles': len(obj.data.polygons), 'vertices': len(obj.data.vertices),
            'bounds_cm': s.bounds([v.co for v in obj.data.vertices])}


def main():
    args = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
    parser = argparse.ArgumentParser()
    parser.add_argument('--project-root', type=Path, required=True)
    parser.add_argument('--revision', default='R1')
    options = parser.parse_args(args)
    root = options.project_root.resolve()
    assert options.revision.startswith('R') and options.revision[1:].isdigit()
    s, helper_path = load_helpers(root)
    out = root / 'Unreal/ArtSource/HeroFinishS362' / options.revision
    assert not out.exists(), ('Immutable revision exists', str(out))
    inputs = [root / 'Unreal/ArtSource/HeroPolishS361/R4' / n / (n + '.fbx') for n in ('Arrel', 'Elia')]
    inputs += [helper_path, root / 'Unreal/ArtSource/HeroPolishS361/R4/Elia/geometry.json']
    frozen = {str(p.relative_to(root)): sha(p) for p in inputs}
    out.mkdir(parents=True, exist_ok=False)
    snapshot = out / 'generator_snapshot.py'
    snapshot.write_bytes(Path(__file__).read_bytes())
    reports = []
    for name in ('Arrel', 'Elia'):
        source = import_accepted(s, root, name)
        bones = s.bone_snapshot(s.RIG)
        materials = [m.name for m in s.BODY.data.materials]
        garments = surface_signature(s, s.BODY, {0, 1})
        if name == 'Arrel':
            all_surface = surface_signature(s, s.BODY)
            detail = {'local_face_normals': arrel_local_normals(s)}
            assert_signature(all_surface, surface_signature(s, s.BODY))
        else:
            removed = remove_recorded_hair(s, root)
            eyes = eye_regions(s)
            detail = {'replaced_hair': removed, 'face': refine_elia_face(s, eyes)}
            detail['hair'] = authored_elia_clumps(s, sum(e['center'].z for e in eyes) / 2)
            detail['dedicated_lids'] = dedicated_lids(s, eyes)
        components = list(s.COMPONENTS)
        folder = out / name
        folder.mkdir()
        destination = folder / (name + '.fbx')
        authored = join_and_morph(s, name, destination, bones, materials, garments)
        imported = roundtrip(s, name, destination, authored, bones, garments)
        report = {'schema': 1, 'session': 'S362', 'revision': options.revision, 'name': name,
                  'source': str(source.relative_to(root)), 'source_sha256': sha(source),
                  'generator_sha256': sha(snapshot), 'accepted_helper_sha256': sha(helper_path),
                  'output_sha256': sha(destination), 'source_bones': bones,
                  'source_material_slots': materials, 'components': components,
                  'detail': detail, 'authored': authored, 'fbx_roundtrip': imported,
                  'contract': 'S361/R4 inputs read-only. All77binds, garment positions/UVs/weights/materials '
                              'and clips retained. Elia exact recorded hair replacement + dedicated lid morphs; '
                              'Arrel local normal-only face refinement, original painted identity unchanged.'}
        write_json(folder / 'geometry.json', report)
        reports.append({'name': name, 'triangles': imported['triangles'],
                        'morphs': imported['morphs'], 'output_sha256': sha(destination)})
        print('S362_HERO_MESH_COMPLETE', name, imported['triangles'], flush=True)
    assert frozen == {str(p.relative_to(root)): sha(p) for p in inputs}, 'Accepted input changed'
    files = [{'path': str(p.relative_to(out)), 'bytes': p.stat().st_size, 'sha256': sha(p)}
             for p in sorted(out.rglob('*')) if p.is_file()]
    write_json(out / 'source_inventory.json', {'schema': 1, 'session': 'S362',
        'revision': options.revision, 'source_frozen_sha256': frozen,
        'generator_sha256': sha(snapshot), 'heroes': reports, 'files': files})
    print('S362_HERO_AUTHORING_COMPLETE', options.revision, 'heroes=2 morphs=2', flush=True)


if __name__ == '__main__':
    main()
