"""S361 hero craft polish; Blender 4.5 background, immutable revision output.

Run: blender -b -t 2 --python Unreal/Tools/build_hero_polish_s361.py -- --revision R1
The source body, garment UVs, skin weights and all 77 bind matrices are immutable.
Only Elia's two explicitly identified S360 hair islands are replaced. No downloads,
image editing, new rig, animation, gameplay edits or inference are performed here.
"""
from pathlib import Path
from collections import Counter, defaultdict
import argparse
import hashlib
import json
import math
import sys

import bpy
import bmesh
from mathutils import Matrix, Vector
from mathutils.bvhtree import BVHTree

ROOT = next(p for p in Path(__file__).resolve().parents
            if (p / 'Unreal/ArtSource/FieldCharacters').is_dir())
SOURCES = {
    'Arrel': ROOT / 'Unreal/ArtSource/FieldCharacters/arrel/arrel_rigged.fbx',
    'Elia': ROOT / 'Unreal/ArtSource/HumanoidAnatomyS360/R7/Elia/Elia.fbx',
}
CATEGORIES = {'skin': 0.0, 'hair': .20, 'cloth': .50, 'metal': .75, 'gem': 1.0}
TAU = math.tau
PARTS = []
COMPONENTS = []
RIG = None
BODY = None
CRAFT = None
H = K = 1.0
TOP = 1.0
CURRENT_NAME = ''
SOURCE_BOUNDS = None
HAIR_LIMITS = None


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def write_json(path, value):
    path.write_text(json.dumps(value, indent=2) + '\n', encoding='utf-8')


def clamp(x, low=0.0, high=1.0):
    return max(low, min(high, x))


def bounds(points):
    return [[min(p[i] for p in points), max(p[i] for p in points)] for i in range(3)]


def check_hair_envelope(label, points):
    """All added hair must remain local to its actual source head, including after modifiers."""
    bb = bounds(points)
    assert all(math.isfinite(c) for p in points for c in p), (label, 'Non-finite hair position')
    for axis in range(3):
        assert bb[axis][0] >= HAIR_LIMITS[axis][0] and bb[axis][1] <= HAIR_LIMITS[axis][1], (
            CURRENT_NAME, label, 'Added hair escaped head envelope', axis, bb, HAIR_LIMITS)
    return {'status': 'PASS', 'bounds_cm': bb, 'allowed_bounds_cm': HAIR_LIMITS}


def check_source_xy_envelope(label, bb):
    for axis in (0, 1):
        assert (bb[axis][0] >= SOURCE_BOUNDS[axis][0] - .5 * K and
                bb[axis][1] <= SOURCE_BOUNDS[axis][1] + .5 * K), (
                    label, 'Mesh escaped original XY envelope', axis, bb, SOURCE_BOUNDS)
    return {'status': 'PASS', 'tolerance_cm': .5 * K, 'source_bounds_cm': SOURCE_BOUNDS}


def bone_snapshot(rig):
    return [{'name': b.name, 'parent': b.parent.name if b.parent else None,
             'matrix': [list(r) for r in b.matrix_local]}
            for b in rig.data.bones]


def compare_bones(before, after, tolerance=0.0):
    assert len(before) == len(after) == 77, (len(before), len(after))
    assert [(b['name'], b['parent']) for b in before] == [
        (b['name'], b['parent']) for b in after], 'Bone names/order/parents changed'
    err = max(abs(x - y) for a, b in zip(before, after)
              for ra, rb in zip(a['matrix'], b['matrix']) for x, y in zip(ra, rb))
    assert err <= tolerance, ('Bind matrix changed', err, tolerance)
    return {'names_parents_and_order_equal': True, 'bones': 77,
            'matrix_scalar_exact_equal': err == 0.0, 'max_matrix_scalar_error': err,
            'tolerance': tolerance}


def colors(me):
    layer = me.color_attributes.get('Color')
    if layer is None and len(me.color_attributes):
        layer = me.color_attributes[0]
    if layer is None:
        return [(1.0, 1.0, 1.0, 0.0)] * len(me.loops)
    if layer.domain == 'CORNER':
        return [tuple(v.color) for v in layer.data]
    assert layer.domain == 'POINT', layer.domain
    return [tuple(layer.data[l.vertex_index].color) for l in me.loops]


def normalize_colors(ob):
    """Allocate CustomData first; reacquire UV RNA references after allocation."""
    me = ob.data
    uvs = {u.name: [tuple(v.uv) for v in u.data] for u in me.uv_layers}
    pigment = colors(me)
    while len(me.color_attributes):
        me.color_attributes.remove(me.color_attributes[0])
    me.color_attributes.new(name='Color', type='FLOAT_COLOR', domain='CORNER')
    for row, c in zip(me.color_attributes['Color'].data, pigment):
        row.color = c
    for name, values in uvs.items():
        for row, uv in zip(me.uv_layers[name].data, values):
            row.uv = uv
    assert len(me.color_attributes) == 1
    assert all([tuple(v.uv) for v in me.uv_layers[n].data] == a
               for n, a in uvs.items()), 'UV data lost during Color allocation'


def retained_signature(ob, material_indices, decimals=None):
    """A multiset of real polygon corners, with every UV channel and skin row."""
    me = ob.data
    uv_names = [u.name for u in me.uv_layers]
    def values(seq):
        return tuple(round(float(v), decimals) if decimals is not None else float(v)
                     for v in seq)
    signatures = Counter()
    for f in me.polygons:
        if f.material_index not in material_indices:
            continue
        for li in f.loop_indices:
            v = me.vertices[me.loops[li].vertex_index]
            uvs = tuple(values(me.uv_layers[n].data[li].uv) for n in uv_names)
            weights = tuple(sorted((ob.vertex_groups[g.group].name,
                                    round(float(g.weight), 6)) for g in v.groups
                                   if g.weight > 1e-7))
            signatures[(f.material_index, values(v.co), uvs, weights)] += 1
    return uv_names, signatures


def preservation(before, after):
    assert before[0] == after[0], ('UV channel names/order changed', before[0], after[0])
    n = sum(before[1].values())
    retained = sum((before[1] & after[1]).values())
    ratio = retained / max(1, n)
    assert ratio >= .99, ('Original garment/head position/UV/skin loss', ratio)
    return {'source_corners': n, 'retained_corners': retained,
            'position_uv_all_channels_and_skin_ratio': ratio,
            'uv_channel_names': before[0]}


def import_source(name):
    global RIG, BODY, H, K, TOP, PARTS, COMPONENTS, CRAFT
    global CURRENT_NAME, SOURCE_BOUNDS, HAIR_LIMITS
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.fbx(filepath=str(SOURCES[name]), colors_type='LINEAR')
    rigs = [o for o in bpy.data.objects if o.type == 'ARMATURE']
    meshes = [o for o in bpy.data.objects if o.type == 'MESH']
    assert len(rigs) == len(meshes) == 1, ('Unexpected source objects', name)
    RIG, BODY = rigs[0], meshes[0]
    # Match the established S359/S360 centimetre FBX normalization exactly.
    BODY.parent = None
    BODY.matrix_world = Matrix.Identity(4)
    RIG.matrix_world = Matrix.Identity(4)
    RIG.animation_data_clear()
    for p in RIG.pose.bones:
        p.matrix_basis = Matrix.Identity(4)
    bpy.context.scene.unit_settings.system = 'METRIC'
    bpy.context.scene.unit_settings.scale_length = .01
    H = max(v.co.z for v in BODY.data.vertices) - min(v.co.z for v in BODY.data.vertices)
    TOP = max(v.co.z for v in BODY.data.vertices)
    K = H / 180.0
    CURRENT_NAME = name
    SOURCE_BOUNDS = bounds([v.co for v in BODY.data.vertices])
    if name == 'Elia':
        cy = bn('head').y
        HAIR_LIMITS = [[-10.5 * K, 10.5 * K], [cy - 12 * K, cy + 12 * K],
                       [TOP - 31 * K, TOP + .1]]
    else:
        head_ids = {v.index for v in BODY.data.vertices if any(
            BODY.vertex_groups[g.group].name == 'head' and g.weight > .60 for g in v.groups)}
        head_bounds = bounds([BODY.data.vertices[i].co for i in head_ids])
        HAIR_LIMITS = [[a - .6 * K, b + .6 * K] for a, b in head_bounds]
        HAIR_LIMITS[2][1] = TOP + .1
    assert 160 < H < 185, (name, H)
    assert len(BODY.data.materials) == (1 if name == 'Arrel' else 3)
    assert len(RIG.data.bones) == 77
    normalize_colors(BODY)
    PARTS, COMPONENTS = [BODY], []
    CRAFT = bpy.data.materials.new('S361_HeroCraft_Regions')
    CRAFT.use_nodes = True
    nodes = CRAFT.node_tree.nodes
    vc = nodes.new('ShaderNodeVertexColor')
    vc.layer_name = 'Color'
    CRAFT.node_tree.links.new(vc.outputs['Color'], nodes['Principled BSDF'].inputs['Base Color'])
    nodes['Principled BSDF'].inputs['Roughness'].default_value = .46
    return SOURCES[name]


def fixed(bone):
    return lambda p: {bone: 1.0}


def bn(name):
    return RIG.data.bones[name].head_local.copy()


def make_part(label, vertices, faces, weight, color, category, thickness=None):
    if category == 'hair':
        check_hair_envelope(label + '_before_modifiers', vertices)
    me = bpy.data.meshes.new(label)
    me.from_pydata(vertices, [], faces)
    me.update()
    ob = bpy.data.objects.new(label, me)
    bpy.context.collection.objects.link(ob)
    me.materials.append(CRAFT)
    uv_names = [u.name for u in BODY.data.uv_layers]
    assert uv_names, 'The retained hero source must have its original UVs'
    for uv_name in uv_names:
        me.uv_layers.new(name=uv_name)
    me.color_attributes.new(name='Color', type='FLOAT_COLOR', domain='CORNER')
    for f in me.polygons:
        f.use_smooth = True
        for li in f.loop_indices:
            p = me.vertices[me.loops[li].vertex_index].co
            c = color(p) if callable(color) else color
            me.color_attributes['Color'].data[li].color = tuple(c[:3]) + (CATEGORIES[category],)
            for uv_name in uv_names:
                me.uv_layers[uv_name].data[li].uv = (p.x / (30 * K), p.z / (60 * K))
    for v in me.vertices:
        row = weight(v.co)
        total = sum(row.values())
        assert total > 0
        for n, w in row.items():
            if w > 1e-7:
                (ob.vertex_groups.get(n) or ob.vertex_groups.new(name=n)).add(
                    [v.index], w / total, 'REPLACE')
    bm = bmesh.new()
    bm.from_mesh(me)
    bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
    bm.to_mesh(me)
    bm.free()
    if thickness:
        bpy.context.view_layer.objects.active = ob
        mod = ob.modifiers.new('ClosedThinHairShell', 'SOLIDIFY')
        mod.thickness = thickness
        mod.offset = -1.0
        # Even-offset miters extrapolate without bound at the bob's narrow
        # scalloped rim corners (R3's long nape needle). Unit-normal offsets keep
        # the inward shell within its real thickness; edge clamping adds a guard.
        mod.use_even_offset = False
        mod.use_quality_normals = True
        mod.thickness_clamp = 1.0
        bpy.ops.object.modifier_apply(modifier=mod.name)
        me = ob.data
    bm = bmesh.new()
    bm.from_mesh(me)
    bad_edges = sum(not e.is_manifold for e in bm.edges)
    bm.free()
    assert bad_edges == 0, (label, 'New component must be a closed solid', bad_edges)
    normalize_colors(ob)
    hair_envelope = (check_hair_envelope(label + '_after_modifiers',
                                        [v.co for v in me.vertices])
                     if category == 'hair' else None)
    PARTS.append(ob)
    COMPONENTS.append({'name': label, 'category': category,
                       'alpha': CATEGORIES[category], 'material_slot': 'appended',
                       'vertices': len(me.vertices), 'polygons': len(me.polygons),
                       'triangles': sum(len(f.vertices) - 2 for f in me.polygons),
                       'bounds_cm': bounds([v.co for v in me.vertices]),
                       'hair_envelope': hair_envelope,
                       'non_manifold_edges': bad_edges})
    return ob


class Surface:
    def __init__(self, ob, predicate=None):
        self.ob = ob
        me = ob.data
        self.points = [v.co.copy() for v in me.vertices]
        self.faces = [tuple(f.vertices) for f in me.polygons
                      if predicate is None or predicate(f)]
        assert self.faces
        self.tree = BVHTree.FromPolygons(self.points, self.faces)

    def front(self, x, z, offset=0):
        hit, normal, index, distance = self.tree.ray_cast(
            Vector((x, -H, z)), Vector((0, 1, 0)), H * 2)
        if hit is None:
            return None
        return hit + Vector((0, -offset, 0))

    def skin(self, p):
        hit, normal, index, distance = self.tree.find_nearest(p)
        assert hit is not None
        ids = self.faces[index]
        # Source FBXs are triangulated. A fan handles defensive n-gon imports.
        best = min((ids[0], ids[i], ids[i + 1]) for i in range(1, len(ids) - 1))
        a, b, c = [self.points[i] for i in best]
        ab, ac, ap = b - a, c - a, hit - a
        denom = ab.dot(ab) * ac.dot(ac) - ab.dot(ac) ** 2
        if abs(denom) < 1e-12:
            bary = [1., 0., 0.]
        else:
            v = (ac.dot(ac) * ap.dot(ab) - ab.dot(ac) * ap.dot(ac)) / denom
            w = (ab.dot(ab) * ap.dot(ac) - ab.dot(ac) * ap.dot(ab)) / denom
            bary = [clamp(1 - v - w), clamp(v), clamp(w)]
        result = defaultdict(float)
        for vi, amount in zip(best, bary):
            for g in self.ob.data.vertices[vi].groups:
                result[self.ob.vertex_groups[g.group].name] += amount * g.weight
        row = sorted(result.items(), key=lambda item: -item[1])[:4]
        total = sum(w for n, w in row)
        return {n: w / total for n, w in row if w > 1e-7}


def tube(label, points, radius, surface, color, category, weight=None, sides=10):
    ps = list(map(Vector, points))
    vertices, faces = [], []
    for j, p in enumerate(ps):
        tangent = (ps[min(j + 1, len(ps) - 1)] - ps[max(j - 1, 0)]).normalized()
        ref = Vector((0, -1, 0))
        if abs(tangent.dot(ref)) > .95:
            ref = Vector((1, 0, 0))
        a = (ref - tangent * ref.dot(tangent)).normalized()
        b = tangent.cross(a)
        r = radius[j] if isinstance(radius, list) else radius
        for i in range(sides):
            angle = TAU * i / sides
            vertices.append(p + a * math.cos(angle) * r + b * math.sin(angle) * r)
    for j in range(len(ps) - 1):
        for i in range(sides):
            a, b = j * sides + i, j * sides + (i + 1) % sides
            faces.append((a, b, b + sides, a + sides))
    faces.append(tuple(range(sides - 1, -1, -1)))
    faces.append(tuple(range((len(ps) - 1) * sides, len(ps) * sides)))
    return make_part(label, vertices, faces, weight or surface.skin, color, category)


def ellipsoid(label, center, radii, color, category, weight, nu=20, nv=12):
    vertices, faces = [], []
    center = Vector(center)
    vertices.append(center + Vector((0, 0, radii[2])))
    for j in range(1, nv):
        phi = math.pi * j / nv
        for i in range(nu):
            a = TAU * i / nu
            vertices.append(center + Vector((radii[0] * math.sin(phi) * math.cos(a),
                                              radii[1] * math.sin(phi) * math.sin(a),
                                              radii[2] * math.cos(phi))))
    bottom = len(vertices)
    vertices.append(center - Vector((0, 0, radii[2])))
    for i in range(nu):
        faces.append((0, 1 + i, 1 + (i + 1) % nu))
    for j in range(nv - 2):
        for i in range(nu):
            a, b = 1 + j * nu + i, 1 + j * nu + (i + 1) % nu
            faces.append((a, a + nu, b + nu, b))
    for i in range(nu):
        faces.append((bottom, 1 + (nv - 2) * nu + (i + 1) % nu,
                      1 + (nv - 2) * nu + i))
    return make_part(label, vertices, faces, weight, color, category)


def connected_regions(ob, material=2):
    """UV seams can split vertices; exact positions reconnect authored islands."""
    me = ob.data
    parent = list(range(len(me.vertices)))
    def find(i):
        while parent[i] != i:
            parent[i] = parent[parent[i]]
            i = parent[i]
        return i
    def union(a, b):
        a, b = find(a), find(b)
        if a != b:
            parent[b] = a
    used, selected = {}, []
    for f in me.polygons:
        if f.material_index != material:
            continue
        selected.append(f)
        for vi in f.vertices:
            key = tuple(round(v, 5) for v in me.vertices[vi].co)
            if key in used:
                union(vi, used[key])
            else:
                used[key] = vi
            union(f.vertices[0], vi)
    components = defaultdict(lambda: {'vertices': set(), 'polygons': []})
    for f in selected:
        group = components[find(f.vertices[0])]
        group['vertices'].update(f.vertices)
        group['polygons'].append(f.index)
    for c in components.values():
        unique = {tuple(round(v, 5) for v in me.vertices[i].co) for i in c['vertices']}
        c['unique_vertices'] = len(unique)
        c['triangles'] = sum(len(me.polygons[i].vertices) - 2 for i in c['polygons'])
        c['bounds_cm'] = bounds([me.vertices[i].co for i in c['vertices']])
    return list(components.values())


def remove_polygons(ob, polygon_indices):
    """Copy surviving loops verbatim, including all UVs, weights and normals."""
    old = ob.data
    group_definitions = [(g.name, g.lock_weight) for g in ob.vertex_groups]
    pigment = colors(old)
    uvs = {u.name: [tuple(v.uv) for v in u.data] for u in old.uv_layers}
    normals = [tuple(n.vector) for n in old.corner_normals]
    kept = [f for f in old.polygons if f.index not in polygon_indices]
    ids = sorted({vi for f in kept for vi in f.vertices})
    remap = {v: i for i, v in enumerate(ids)}
    weights = [[(g.group, g.weight) for g in old.vertices[i].groups] for i in ids]
    me = bpy.data.meshes.new('Elia_OriginalFaceAndGarments')
    me.from_pydata([old.vertices[i].co.copy() for i in ids], [],
                   [tuple(remap[i] for i in f.vertices) for f in kept])
    me.update()
    for mat in old.materials:
        me.materials.append(mat)
    for name in uvs:
        me.uv_layers.new(name=name)
    me.color_attributes.new(name='Color', type='FLOAT_COLOR', domain='CORNER')
    copied_normals = []
    for source, dest in zip(kept, me.polygons):
        dest.material_index, dest.use_smooth = source.material_index, source.use_smooth
        for a, b in zip(source.loop_indices, dest.loop_indices):
            for name in uvs:
                me.uv_layers[name].data[b].uv = uvs[name][a]
            me.color_attributes['Color'].data[b].color = pigment[a]
            copied_normals.append(normals[a])
    ob.data = me
    # Mesh reassignment can clear object vertex-group definitions in Blender 4.5.
    # Recreate the exact original order before restoring index-addressed skin rows.
    ob.vertex_groups.clear()
    for name, locked in group_definitions:
        group = ob.vertex_groups.new(name=name)
        group.lock_weight = locked
    assert [g.name for g in ob.vertex_groups] == [n for n, locked in group_definitions]
    for i, row in enumerate(weights):
        for group, w in row:
            ob.vertex_groups[group].add([i], w, 'REPLACE')
    me.normals_split_custom_set(copied_normals)
    normalize_colors(ob)


def elia_face_and_hair():
    regions = connected_regions(BODY)
    summary = [{k: v for k, v in c.items() if k not in ('vertices', 'polygons')}
               for c in regions]
    cap = [c for c in regions if c['unique_vertices'] == 2785 and c['triangles'] == 5472]
    rear = [c for c in regions if c['unique_vertices'] == 931 and c['triangles'] == 1728]
    assert len(cap) == len(rear) == 1, ('S360 hair islands not uniquely identified', summary)
    eyes = [c for c in regions if c['unique_vertices'] == 610
            and c['bounds_cm'][2][1] - c['bounds_cm'][2][0] < 3 * K]
    assert len(eyes) == 2, ('Two original spherical eyes not identified', summary)
    eye_data = []
    for c in eyes:
        bb = c['bounds_cm']
        eye_data.append({'center': Vector([(a + b) / 2 for a, b in bb]),
                         'radius': (bb[2][1] - bb[2][0]) / 2})
    removed = set(cap[0]['polygons']) | set(rear[0]['polygons'])
    removed_report = {'policy': 'Only exact S360 FittedStrandHair and LayeredRearHair islands',
                      'components': [{k: v for k, v in c.items()
                                      if k not in ('vertices', 'polygons')} for c in (cap[0], rear[0])],
                      'removed_triangles': sum(c['triangles'] for c in (cap[0], rear[0])),
                      'all_material_2_regions_before': summary}
    remove_polygons(BODY, removed)
    eye_z = sum(e['center'].z for e in eye_data) / 2
    eye_ids, brow_ids = set(), set()
    # Re-identify after exact removal; indices legitimately changed, geometry did not.
    for c in connected_regions(BODY):
        if c['unique_vertices'] == 610:
            eye_ids.update(c['vertices'])
        if c['unique_vertices'] <= 90 and c['bounds_cm'][2][0] > eye_z + .7 * K:
            brow_ids.update(c['vertices'])
    pigment = BODY.data.color_attributes['Color']
    old_colors = colors(BODY.data)
    lip_rows = []
    for f in BODY.data.polygons:
        if f.material_index != 2:
            continue
        for li in f.loop_indices:
            vi = BODY.data.loops[li].vertex_index
            p, c = BODY.data.vertices[vi].co, old_colors[li]
            if abs(p.x) < 2.6 * K and p.y < 1 * K and eye_z - 10 * K < p.z < eye_z - 2 * K:
                amount = max(0.0, .355 - c[1])
                if amount > .015:
                    lip_rows.append((p.z, amount))
    lip_z = (sum(z * a for z, a in lip_rows) / sum(a for z, a in lip_rows)
             if lip_rows else eye_z - 6.0 * K)
    for f in BODY.data.polygons:
        if f.material_index != 2:
            continue
        for li in f.loop_indices:
            vi = BODY.data.loops[li].vertex_index
            p = BODY.data.vertices[vi].co
            if vi in eye_ids:
                e = min(eye_data, key=lambda e: (p - e['center']).length_squared)
                d, r = p - e['center'], e['radius']
                rr = math.hypot(d.x, d.z) / r
                if d.y < -.40 * r and rr < .47:
                    c = (.008, .013, .021) if rr < .20 else (
                        (.09, .16, .23) if rr < .40 else (.025, .047, .072))
                else:
                    c = (.68, .70, .70)
            elif vi in brow_ids:
                c = (.12, .14, .165)
            else:
                original = old_colors[li]
                c = [original[0] * 1.04, original[1] * 1.09, original[2] * 1.12]
                front = clamp((2.0 * K - p.y) / (4.0 * K))
                cheeks = math.exp(-((abs(p.x) - 3.3 * K) / (1.7 * K)) ** 2
                                  - ((p.z - (eye_z - 3.0 * K)) / (1.5 * K)) ** 2) * front
                c = [c[0] + .020 * cheeks, c[1] - .005 * cheeks, c[2] - .003 * cheeks]
                lip = math.exp(-((p.z - lip_z) / (.45 * K)) ** 2
                               - (p.x / (1.9 * K)) ** 6) * front
                c = [a * (1 - lip * .28) + b * lip * .28
                     for a, b in zip(c, (.38, .22, .23))]
            pigment.data[li].color = tuple(clamp(a) for a in c[:3]) + (0.0,)
    face = Surface(BODY, lambda f: f.material_index == 2)
    for i, e in enumerate(eye_data):
        center, r = e['center'], e['radius']
        # Small closed upper-lash geometry follows the existing eye surface itself.
        ps = []
        for j in range(13):
            dx = -.82 * r + 1.64 * r * j / 12
            dz = (.33 + .16 * math.sin(math.pi * j / 12)) * r
            dy = -math.sqrt(max(.01, r * r - dx * dx - dz * dz))
            ps.append(center + Vector((dx, dy - .018 * K, dz)))
        tube('Elia_UpperLash_' + str(i), ps, .022 * K, face,
             (.035, .043, .057), 'skin', fixed('head'), sides=8)
        dx, dz = -.15 * r, .19 * r
        dy = -math.sqrt(max(.01, r * r - dx * dx - dz * dz))
        p = center + Vector((dx, dy - .015 * K, dz))
        ellipsoid('Elia_EyeCatchlight_' + str(i), p,
                  (.082 * K, .025 * K, .082 * K), (.86, .91, .94), 'gem', fixed('head'), 12, 8)
    elia_bob(face, eye_z)
    removed_report['eye_centers_cm'] = [list(e['center']) for e in eye_data]
    removed_report['lip_center_z_cm'] = lip_z
    return removed_report


def hair_color(p):
    factor = .94 + .045 * math.sin(p.x * 2.6 / K + p.z * .21 / K)
    return (.55 * factor, .57 * factor, .59 * factor)


def conformed_lock(label, surface, controls, widths, color=hair_color, depth=.12):
    """Closed shallow locks hug a source surface, with each loop projected separately."""
    ps = list(map(Vector, controls))
    rows, sides = 25, 10
    vertices, faces = [], []
    for row in range(rows):
        t = row / (rows - 1)
        # Quadratic Bezier gives swept strands a soft side part rather than straight spikes.
        p = ps[0] * (1 - t) ** 2 + ps[1] * 2 * (1 - t) * t + ps[2] * t * t
        width = widths[0] * (1 - t) ** 2 + widths[1] * 2 * (1 - t) * t + widths[2] * t * t
        for i in range(sides):
            angle = TAU * i / sides
            x, z = p.x + math.cos(angle) * width, min(TOP - .035 * K, p.z)
            hit = surface.front(x, z)
            if hit is None:
                hit, normal, index, distance = surface.tree.find_nearest(Vector((x, p.y, z)))
                assert hit is not None and distance < 3.5 * K, (label, 'Unattached hair', x, z, distance)
                # Nearest-point fallback is for the narrowing crown only.
                x, z = hit.x, hit.z
            vertices.append(Vector((x, hit.y - .035 * K + math.sin(angle) * depth * K, z)))
    for row in range(rows - 1):
        for i in range(sides):
            a, b = row * sides + i, row * sides + (i + 1) % sides
            faces.append((a, b, b + sides, a + sides))
    faces.extend([tuple(range(sides - 1, -1, -1)),
                  tuple(range((rows - 1) * sides, rows * sides))])
    return make_part(label, vertices, faces, fixed('head'), color, 'hair')


def elia_bob(face, eye_z):
    head = bn('head')
    cy = head.y
    crown = TOP - .045 * K
    rows, ns = 34, 96
    vertices, faces = [], []
    for row in range(rows):
        t = row / rows
        for i in range(ns):
            angle = TAU * i / ns
            front = max(0.0, math.cos(angle))
            # The shell ends well ABOVE the visible bangs. Its asymmetric arched
            # opening cannot form the old horizontal helmet band across the brow.
            tip = (.5 + .5 * math.cos(14 * angle + .35)) ** 4
            rear_hem = TOP - (21.3 + 1.6 * tip + .38 * math.sin(3 * angle + .25)) * K
            high_opening = eye_z + (5.4 + 1.15 * math.sin(angle + .30)) * K
            blend = front ** 1.3
            bottom = rear_hem * (1 - blend) + high_opening * blend
            z = bottom + (crown - bottom) * t
            d = Vector((math.sin(angle), -math.cos(angle), 0))
            center = Vector((0, cy, z))
            hit, normal, index, distance = face.tree.ray_cast(center + d * 35 * K, -d, 70 * K)
            if hit is None:
                # The true scalp narrows smoothly at its crown; no added height.
                r = max(.18, 7.8 * math.sqrt(max(.001, 1 - ((z - (TOP - 12.5 * K)) / (12.8 * K)) ** 2))) * K
                hit = center + d * r
            radial = hit - center
            r = radial.length
            # A bob's lower edge falls past the cheek without following the tapering jaw.
            min_r = (7.5 - .7 * clamp((z - (TOP - 23 * K)) / (8 * K))) * K
            skirt = clamp(((TOP - 13.5 * K) - z) / (6 * K)) * (1 - front ** 3)
            r = r * (1 - skirt) + max(r, min_r) * skirt
            wave = (.34 + .11 * math.sin(angle * 21 + t * 2.4)) * K
            vertices.append(center + d * (r + wave))
    tip = len(vertices)
    vertices.append((-.5 * K, cy, crown))
    for row in range(rows - 1):
        for i in range(ns):
            a, b = row * ns + i, row * ns + (i + 1) % ns
            faces.append((a, b, b + ns, a + ns))
    for i in range(ns):
        faces.append(((rows - 1) * ns + i, (rows - 1) * ns + (i + 1) % ns, tip))
    cap = make_part('Elia_SidePartedBobShell', vertices, faces, fixed('head'), hair_color, 'hair', .14 * K)
    cap_surface = Surface(cap)
    # Four broad diagonal locks define the visible forehead silhouette. Project
    # against retained skin AND the cap; a missing cap ray must never hit its back.
    sweeps = [
        [(-3.1, TOP - 2.1 * K), (-2.7, TOP - 4.6 * K), (4.6, eye_z + 1.45 * K), (.40, 3.10, .045)],
        [(-3.5, TOP - 2.7 * K), (-5.2, TOP - 6.5 * K), (-6.2, eye_z + 2.9 * K), (.35, 2.20, .040)],
        [(-2.3, TOP - 2.2 * K), (1.0, TOP - 5.6 * K), (6.1, eye_z + .65 * K), (.40, 2.50, .040)],
        [(-1.3, TOP - 2.7 * K), (3.6, TOP - 5.9 * K), (7.2, eye_z - .8 * K), (.30, 2.00, .035)],
    ]
    for i, sweep in enumerate(sweeps):
        controls = [(x * K, cy - 10 * K, z) for x, z in sweep[:3]]
        swept_fringe('Elia_SweptFringe_' + str(i), face, cap_surface, controls,
                     [w * K for w in sweep[3]], cy)
    # Curved, full-volume framing pieces bow outside the cheek and curl back in
    # above the jaw. Roots intersect the bob; tips deliberately do not hang as rods.
    for sign in (-1, 1):
        for j in range(2):
            angle = sign * (.94 + .17 * j)
            direction = Vector((math.sin(angle), -math.cos(angle), 0))
            root_z = TOP - (6.5 + .6 * j) * K
            center = Vector((0, cy, root_z))
            hit, normal, index, distance = cap_surface.tree.ray_cast(
                center + direction * 25 * K, -direction, 50 * K)
            assert hit is not None, ('Curved bob root missed shell', sign, j)
            root = hit - direction * .055 * K
            middle = Vector((sign * (9.2 + .20 * j) * K,
                             cy - (1.9 - .70 * j) * K, eye_z - (2.0 + .30 * j) * K))
            end = Vector((sign * (6.5 + .60 * j) * K,
                          cy - (1.9 - .50 * j) * K, eye_z - (7.0 + .70 * j) * K))
            curved_hair_lock('Elia_CurvedCheekLock_' + str(sign) + '_' + str(j),
                             [root, middle, end], [.38 * K, 1.30 * K, .035 * K],
                             [.13 * K, .32 * K, .030 * K], Vector((1, 0, 0)))
    # Distinct rear layers finish at alternating tapered tips below the scalloped
    # shell. These overlap at their roots and stay inside the existing head budget.
    for j in range(9):
        angle = math.pi * (.56 + .88 * j / 8)
        direction = Vector((math.sin(angle), -math.cos(angle), 0))
        root_z = TOP - (8.4 + .35 * (j % 3)) * K
        center = Vector((0, cy, root_z))
        hit, normal, index, distance = cap_surface.tree.ray_cast(
            center + direction * 25 * K, -direction, 50 * K)
        assert hit is not None, ('Rear layered hair root missed shell', j)
        root = hit - direction * .04 * K
        middle = Vector((0, cy, TOP - 16.2 * K)) + direction * (8.9 * K)
        end = Vector((0, cy, TOP - (22.5 + .55 * ((j * 3) % 4)) * K))
        end += direction * ((6.5 + .18 * (j % 3)) * K)
        curved_hair_lock('Elia_TaperedRearLayer_' + str(j), [root, middle, end],
                         [.32 * K, 1.05 * K, .030 * K],
                         [.095 * K, .24 * K, .025 * K],
                         Vector((math.cos(angle), math.sin(angle), 0)))


def swept_fringe(label, face, cap, controls, widths, cy):
    """A broad curved bang follows front scalp/skin rather than the back of an open cap."""
    ps = list(map(Vector, controls))
    rows, sides = 33, 12
    vertices, faces = [], []
    for row in range(rows):
        t = row / (rows - 1)
        p = ps[0] * (1 - t) ** 2 + ps[1] * 2 * (1 - t) * t + ps[2] * t * t
        tangent = (ps[1] - ps[0]) * (1 - t) + (ps[2] - ps[1]) * t
        across = Vector((-tangent.z, 0, tangent.x)).normalized()
        width = widths[0] * (1 - t) ** 2 + widths[1] * 2 * (1 - t) * t + widths[2] * t * t
        depth = (.075 + .055 * math.sin(math.pi * t)) * K
        for i in range(sides):
            angle = TAU * i / sides
            q = p + across * (math.cos(angle) * width)
            q.z = min(q.z, TOP - .08 * K)
            hit = face.front(q.x, q.z)
            if hit is None:
                hit, normal, index, distance = face.tree.find_nearest(
                    Vector((q.x, cy - 15 * K, q.z)))
                assert hit is not None and distance < 12 * K, (label, 'Scalp projection missed', q)
                # Near a temple, the closest actual skin surface defines attachment.
                q.x, q.z = hit.x, hit.z
            front_y = hit.y - .35 * K
            cap_hit = cap.front(q.x, q.z)
            if cap_hit is not None and hit.y - 1.5 * K <= cap_hit.y <= hit.y + .35 * K:
                front_y = min(front_y, cap_hit.y - .025 * K)
            # The cap's far side is expressly rejected; this prevents the R2 Y flips.
            assert front_y < cy + .5 * K, (label, 'Hair projected to rear scalp', front_y, cy)
            vertices.append(Vector((q.x, front_y + math.sin(angle) * depth, q.z)))
    for row in range(rows - 1):
        for i in range(sides):
            a, b = row * sides + i, row * sides + (i + 1) % sides
            faces.append((a, b, b + sides, a + sides))
    faces.extend([tuple(range(sides - 1, -1, -1)),
                  tuple(range((rows - 1) * sides, rows * sides))])
    return make_part(label, vertices, faces, fixed('head'), hair_color, 'hair')


def curved_hair_lock(label, controls, widths, depths, guide):
    """Closed three-dimensional tapered lock, broad across the hair flow."""
    ps = list(map(Vector, controls))
    rows, sides = 29, 12
    vertices, faces = [], []
    for row in range(rows):
        t = row / (rows - 1)
        p = ps[0] * (1 - t) ** 2 + ps[1] * 2 * (1 - t) * t + ps[2] * t * t
        tangent = ((ps[1] - ps[0]) * (1 - t) + (ps[2] - ps[1]) * t).normalized()
        a = (guide - tangent * guide.dot(tangent)).normalized()
        b = tangent.cross(a)
        width = widths[0] * (1 - t) ** 2 + widths[1] * 2 * (1 - t) * t + widths[2] * t * t
        depth = depths[0] * (1 - t) ** 2 + depths[1] * 2 * (1 - t) * t + depths[2] * t * t
        for i in range(sides):
            angle = TAU * i / sides
            vertices.append(p + a * (math.cos(angle) * width) + b * (math.sin(angle) * depth))
    for row in range(rows - 1):
        for i in range(sides):
            a, b = row * sides + i, row * sides + (i + 1) % sides
            faces.append((a, b, b + sides, a + sides))
    faces.extend([tuple(range(sides - 1, -1, -1)),
                  tuple(range((rows - 1) * sides, rows * sides))])
    return make_part(label, vertices, faces, fixed('head'), hair_color, 'hair')


def arrel_polish():
    head_ids = {v.index for v in BODY.data.vertices if any(
        BODY.vertex_groups[g.group].name == 'head' and g.weight > .60 for g in v.groups)}
    assert len(head_ids) > 200, len(head_ids)
    head = Surface(BODY, lambda f: all(v in head_ids for v in f.vertices))
    head_bounds = bounds([BODY.data.vertices[i].co for i in head_ids])
    crown = min(TOP, head_bounds[2][1])
    hh = head_bounds[2][1] - head_bounds[2][0]
    half = min(abs(head_bounds[0][0]), abs(head_bounds[0][1]))
    cy = bn('head').y
    # The original painted fringe already defines Arrel's identity. Only the sparse
    # temple strands are added; the three R2 crown hooks were visually rejected.
    for sign in (-1, 1):
        conformed_lock('Arrel_TempleStrand_' + str(sign), head,
                       [(sign * half * .48, cy, crown - hh * .13),
                        (sign * half * .72, cy, crown - hh * .23),
                        (sign * half * .75, cy, crown - hh * .39)],
                       [.18 * K, .48 * K, .035 * K], depth=.075)
    surface = Surface(BODY)
    shoulder_z = (bn('upperarm_l').z + bn('upperarm_r').z) / 2
    width = (abs(bn('upperarm_l').x) + abs(bn('upperarm_r').x)) / 2
    silver, gold = (.30, .37, .43), (.39, .275, .12)
    trim_count = 0
    for sign in (-1, 1):
        ps = []
        for i in range(29):
            t = i / 28
            x = sign * (width * (.17 + .43 * t))
            z = shoulder_z + (1.2 - 16.2 * t + 2.3 * math.sin(t * math.pi)) * K
            p = surface.front(x, z, .11 * K)
            assert p is not None, ('Breastplate trim outside original garment', x, z)
            ps.append(p)
        tube('Arrel_ConformedBreastplateEdge_' + str(sign), ps, .20 * K, surface, silver, 'metal')
        tube('Arrel_GoldBreastplateInlay_' + str(sign),
             [p + Vector((0, -.065 * K, 0)) for p in ps], .065 * K, surface, gold, 'metal', sides=8)
        trim_count += 2
        # A continuous shoulder arc is fitted to the front of the existing pauldron.
        ps = []
        for i in range(25):
            t = i / 24
            x = sign * (width + (-4.8 + 10.0 * t) * K)
            z = shoulder_z + (1.2 - 3.4 * math.sin(math.pi * t)) * K
            p = surface.front(x, z, .13 * K)
            if p is not None:
                ps.append(p)
        if len(ps) >= 12:
            tube('Arrel_PauldronRolledEdge_' + str(sign), ps, .18 * K, surface, gold, 'metal')
            trim_count += 1
        x, z = sign * width * .29, shoulder_z - 3.8 * K
        p = surface.front(x, z, .18 * K)
        assert p is not None
        ellipsoid('Arrel_CapeClaspFrame_' + str(sign), p, (.72 * K, .27 * K, .9 * K), gold, 'metal', surface.skin)
        ellipsoid('Arrel_CapeClaspStone_' + str(sign), p + Vector((0, -.23 * K, 0)),
                  (.38 * K, .16 * K, .52 * K), (.055, .14, .22), 'gem', surface.skin)
    # Small wrist rims follow local forearm skin, away from either hand or sword grip.
    for side in ('l', 'r'):
        a, b = bn('lowerarm_' + side), bn('hand_' + side)
        axis = (b - a).normalized()
        center = a.lerp(b, .78)
        group_names = {'lowerarm_' + side, 'hand_' + side}
        ids = {v.index for v in BODY.data.vertices if sum(g.weight for g in v.groups
                if BODY.vertex_groups[g.group].name in group_names) > .55}
        local = Surface(BODY, lambda f: all(i in ids for i in f.vertices))
        ref = Vector((0, -1, 0))
        tangent = (ref - axis * ref.dot(axis)).normalized()
        bitangent = axis.cross(tangent)
        ps = []
        for i in range(65):
            ang = TAU * i / 64
            direction = tangent * math.cos(ang) + bitangent * math.sin(ang)
            hit, normal, face_index, distance = local.tree.ray_cast(
                center + direction * 15 * K, -direction, 30 * K)
            if hit is None:
                break
            ps.append(hit + direction * .11 * K)
        if len(ps) == 65:
            # A tube with duplicate end positions is sealed separately in the ring builder.
            closed_ring('Arrel_EngravedVambraceRim_' + side, ps[:-1], .13 * K, local, gold)
            trim_count += 1
    return {'source_head_bounds_cm': head_bounds, 'armor_trim_components': trim_count,
            'policy': 'Original painted anime face, head, hair and every garment corner retained'}


def closed_ring(label, ps, radius, surface, color):
    vertices, faces = [], []
    sides, n = 10, len(ps)
    for j, p in enumerate(ps):
        tangent = (ps[(j + 1) % n] - ps[(j - 1) % n]).normalized()
        ref = Vector((0, 0, 1))
        if abs(tangent.dot(ref)) > .95:
            ref = Vector((0, -1, 0))
        a = (ref - tangent * ref.dot(tangent)).normalized()
        b = tangent.cross(a)
        for i in range(sides):
            angle = TAU * i / sides
            vertices.append(p + radius * (a * math.cos(angle) + b * math.sin(angle)))
    for j in range(n):
        for i in range(sides):
            faces.append((j * sides + i, j * sides + (i + 1) % sides,
                          ((j + 1) % n) * sides + (i + 1) % sides,
                          ((j + 1) % n) * sides + i))
    return make_part(label, vertices, faces, surface.skin, color, 'metal')


def join_export(name, dest, original_bones, original_materials, sig_exact, sig_quantized):
    for ob in PARTS:
        normalize_colors(ob)
    bpy.ops.object.select_all(action='DESELECT')
    for ob in PARTS:
        ob.select_set(True)
    bpy.context.view_layer.objects.active = BODY
    bpy.ops.object.join()
    BODY.name = name + '_HeroPolishS361'
    assert [m.name for m in BODY.data.materials] == original_materials + [CRAFT.name]
    for mod in list(BODY.modifiers):
        BODY.modifiers.remove(mod)
    if any(len(f.vertices) != 3 for f in BODY.data.polygons):
        mod = BODY.modifiers.new('NewPartTriangles', 'TRIANGULATE')
        bpy.ops.object.modifier_apply(modifier=mod.name)
    BODY.parent = RIG
    BODY.matrix_parent_inverse = Matrix.Identity(4)
    BODY.matrix_basis = Matrix.Identity(4)
    mod = BODY.modifiers.new('UnchangedHeroSkin', 'ARMATURE')
    mod.object = RIG
    normalize_colors(BODY)
    compare_bones(original_bones, bone_snapshot(RIG))
    materials = {0} if name == 'Arrel' else {0, 1}
    preserved_exact = preservation(sig_exact, retained_signature(BODY, materials))
    preserved_quantized = preservation(sig_quantized, retained_signature(BODY, materials, 5))
    current_bounds = bounds([v.co for v in BODY.data.vertices])
    xy_envelope = check_source_xy_envelope(name + '_before_FBX', current_bounds)
    assert current_bounds[2][1] <= TOP + .1, (name, 'Unwanted height growth', current_bounds[2][1], TOP)
    assert len(BODY.data.polygons) <= 100000
    appended = len(original_materials)
    layer = BODY.data.color_attributes['Color']
    new_colors = [tuple(layer.data[li].color) for f in BODY.data.polygons
                  if f.material_index == appended for li in f.loop_indices]
    assert new_colors and sum(max(c[:3]) - min(c[:3]) > .008 for c in new_colors) / len(new_colors) > .5
    assert any(abs(c[3] - .2) < .001 for c in new_colors), 'Hair category lost'
    bpy.ops.object.select_all(action='DESELECT')
    BODY.select_set(True)
    RIG.select_set(True)
    bpy.context.view_layer.objects.active = RIG
    bpy.ops.export_scene.fbx(filepath=str(dest), use_selection=True,
        object_types={'ARMATURE', 'MESH'}, axis_forward='-Y', axis_up='Z',
        apply_unit_scale=True, apply_scale_options='FBX_SCALE_UNITS',
        add_leaf_bones=False, use_armature_deform_only=True, path_mode='RELATIVE',
        bake_anim=False, mesh_smooth_type='FACE', colors_type='LINEAR')
    before = {'vertices': len(BODY.data.vertices), 'triangles': len(BODY.data.polygons),
              'bounds_cm': current_bounds, 'materials': [m.name for m in BODY.data.materials],
              'preserved_exact': preserved_exact, 'preserved_rounded_5dp': preserved_quantized,
              'source_xy_envelope': xy_envelope,
              'authored_binds': compare_bones(original_bones, bone_snapshot(RIG))}
    return before


def roundtrip(name, dest, before, original_bones, sig_quantized, original_materials):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.fbx(filepath=str(dest), colors_type='LINEAR')
    rigs = [o for o in bpy.data.objects if o.type == 'ARMATURE']
    meshes = [o for o in bpy.data.objects if o.type == 'MESH']
    assert len(rigs) == len(meshes) == 1
    rig, ob = rigs[0], meshes[0]
    compare = compare_bones(original_bones, bone_snapshot(rig), 5e-4)
    assert len(ob.data.polygons) == before['triangles'] <= 100000
    assert [m.name for m in ob.data.materials] == before['materials']
    assert len(ob.data.color_attributes) == 1 and ob.data.color_attributes[0].name == 'Color'
    preserved = preservation(sig_quantized, retained_signature(
        ob, {0} if name == 'Arrel' else {0, 1}, 5))
    layer = ob.data.color_attributes['Color']
    appended = len(original_materials)
    cats = Counter()
    pigment = []
    hair_points = []
    for f in ob.data.polygons:
        if f.material_index == appended:
            for li in f.loop_indices:
                c = tuple(layer.data[li].color)
                cats[round(c[3], 3)] += 1
                pigment.append(c)
                if abs(c[3] - CATEGORIES['hair']) < .001:
                    hair_points.append(ob.data.vertices[ob.data.loops[li].vertex_index].co.copy())
    assert pigment and any(abs(c[3] - .2) < .001 for c in pigment)
    assert all(min(abs(a - c[3]) for a in CATEGORIES.values()) < .001 for c in pigment)
    imported_bounds = bounds([v.co for v in ob.data.vertices])
    xy_envelope = check_source_xy_envelope(name + '_FBX_roundtrip', imported_bounds)
    hair_envelope = check_hair_envelope(name + '_FBX_roundtrip', hair_points)
    return {'imported_binds': compare, 'retained_garments_position_uv_and_skin_rounded_5dp': preserved,
            'vertices': len(ob.data.vertices), 'triangles': len(ob.data.polygons),
            'bounds_cm': imported_bounds, 'source_xy_envelope': xy_envelope,
            'hair_envelope': hair_envelope,
            'all_uv_channels': [u.name for u in ob.data.uv_layers],
            'appended_color_corner_categories': dict(cats),
            'appended_pigment_distinct_rgb': len({tuple(round(x, 5) for x in c[:3]) for c in pigment}),
            'material_slot_names_equal': True}


def main():
    args = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
    parser = argparse.ArgumentParser()
    parser.add_argument('--revision', default='R1')
    options = parser.parse_args(args)
    assert options.revision.startswith('R') and options.revision[1:].isdigit()
    out = ROOT / 'Unreal/ArtSource/HeroPolishS361' / options.revision
    assert not out.exists(), ('Immutable revision exists; choose a fresh revision', str(out))
    frozen = {str(p.relative_to(ROOT)): sha(p) for p in SOURCES.values()}
    out.mkdir(parents=True, exist_ok=False)
    snapshot = out / 'generator_snapshot.py'
    snapshot.write_bytes(Path(__file__).read_bytes())
    generator_sha = sha(snapshot)
    reports = []
    for name in ('Arrel', 'Elia'):
        source = import_source(name)
        original_bones = bone_snapshot(RIG)
        original_materials = [m.name for m in BODY.data.materials]
        original_bounds = bounds([v.co for v in BODY.data.vertices])
        preserved_mats = {0} if name == 'Arrel' else {0, 1}
        sig_exact = retained_signature(BODY, preserved_mats)
        sig_quantized = retained_signature(BODY, preserved_mats, 5)
        detail = arrel_polish() if name == 'Arrel' else elia_face_and_hair()
        component_reports = list(COMPONENTS)
        dest_dir = out / name
        dest_dir.mkdir()
        dest = dest_dir / (name + '.fbx')
        authored = join_export(name, dest, original_bones, original_materials, sig_exact, sig_quantized)
        imported = roundtrip(name, dest, authored, original_bones, sig_quantized, original_materials)
        assert imported['bounds_cm'][2][1] <= original_bounds[2][1] + .1
        assert abs(imported['bounds_cm'][2][0] - original_bounds[2][0]) < 1e-4
        report = {'schema': 1, 'session': 'S361', 'revision': options.revision, 'name': name,
                  'source': str(source.relative_to(ROOT)), 'source_sha256': sha(source),
                  'generator_snapshot': '../generator_snapshot.py', 'generator_sha256': generator_sha,
                  'output_sha256': sha(dest), 'source_bounds_cm': original_bounds,
                  'source_bones': original_bones, 'original_material_slots': original_materials,
                  'appended_material_slot': len(original_materials), 'alpha_categories': CATEGORIES,
                  'components': component_reports, 'detail': detail,
                  'authored': authored, 'fbx_roundtrip': imported,
                  'contract': 'All source binds, garments, hand/grip, ground and body unchanged. '
                              'Elia only replaces the identified hair islands; material2 pigment changes. '
                              'All added components are closed solids. No animation or new textures.'}
        write_json(dest_dir / 'geometry.json', report)
        reports.append({'name': name, 'triangles': imported['triangles'],
                        'source_sha256': sha(source), 'output_sha256': sha(dest),
                        'authored_bind_error': authored['authored_binds']['max_matrix_scalar_error'],
                        'roundtrip_bind_error': imported['imported_binds']['max_matrix_scalar_error'],
                        'retained_garment_ratio': imported[
                            'retained_garments_position_uv_and_skin_rounded_5dp'][
                                'position_uv_all_channels_and_skin_ratio']})
        print('S361_HERO_MESH_COMPLETE', name, imported['triangles'], flush=True)
    assert frozen == {str(p.relative_to(ROOT)): sha(p) for p in SOURCES.values()}, 'Original source changed'
    files = [{'path': str(p.relative_to(out)), 'bytes': p.stat().st_size, 'sha256': sha(p)}
             for p in sorted(out.rglob('*')) if p.is_file() and p.name != 'source_inventory.json']
    write_json(out / 'source_inventory.json', {'schema': 1, 'session': 'S361',
               'revision': options.revision, 'generator_sha256': generator_sha,
               'source_frozen_sha256': frozen, 'heroes': reports, 'files': files})
    print('S361_HERO_AUTHORING_COMPLETE', options.revision, 'heroes=2', flush=True)


if __name__ == '__main__':
    main()
