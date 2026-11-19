# S362 hero finish — R1 candidate

This is a new immutable candidate based on accepted `HeroPolishS361/R4`. The
accepted FBX files, generator and receipt are hash pinned and remain read-only.
No source image, rig, animation, garment position, UV, weight or painted identity
is replaced. A native art review is still required before R1 is accepted.

Arrel keeps all33,804triangles, all source material slots and all original paint.
Only759front-face corner normals are blended locally (maximum5.72degrees); the
maximum normal error outside that bounded region is zero. No original vertex is
moved and no false eyeball/blink geometry is introduced on the painted face.

Elia replaces only the18 exact S361 hair components recorded by triangle count,
slot3/alpha.20 and each component's actual bounds. The new hair uses a smaller
recessed scalp core, five asymmetric swept fringes and twelve irregular grouped
clumps. Each new hair component is a closed solid inside the source head bounds.
The original lashes, catchlights, face and embroidered garments are retained.
Skin retains accepted pigment; eye alpha.06 selects a gentler wet material.

Two dedicated upper-lid skin volumes add288vertices per side. `HeroBlink_L` and
`HeroBlink_R` each move240of those vertices, maximum1.76035cm. They expand actual
skin over the eye; they never move or scale the source eyes/face/garments. Both
targets survive FBX roundtrip. No expression, hair physics or new texture is added.
Elia now has64,832triangles, fewer than S361's68,648. The original72,882garment
corners preserve material, position, every UV, weights and RGBA exactly.

## Regeneration

Use Blender4.5.3 CPU background mode with2threads:

```
blender -b -t 2 --python Unreal/Tools/build_hero_finish_s362.py -- --project-root <checkout> --revision R2
```

The tool refuses an existing output revision. Require both `S362_HERO_MESH_COMPLETE`
markers and `S362_HERO_AUTHORING_COMPLETE R2 heroes=2 morphs=2`, successful exit and
no traceback. Source and output receipts contain the executed generator hash,
77source bone matrices, whole-head guards and FBX roundtrip preservation checks.
Never rewrite the accepted R4 inputs or overwrite an existing S362 revision.

## Dedicated legacy import

After coordinator-owned editor build:

```
UnrealEditor-Cmd Memoria.uproject -run=MemoriaHeroS362 -Revision=R1 -unattended -nop4 -AllowCommandletRendering -RenderOffscreen
```

The commandlet refuses existing target packages, imports2meshes/2materials and no
skeleton/clip/texture. Legacy `bImportMorphTargets=true` is explicit. It verifies
the original skeleton object and77reference poses, native surface position/UV/skin
rows, local bounds and both real imported delta buffers. Morph deltas must refer
only to the new slot4/alpha0lid vertices, cover a greater closed vertical extent
and move downward without any source-eye vertex deformation.

Native tests are `MemoriaGraphics.S362.BlinkContract`, `.ImportedMorphs` and
`.RenderedBlinkLifecycle`. The last uses the actual production field component,
captures natural opening/closure and starts real held death from an explicitly
closed blink to check suppression. Native visual before/after, face/profile/back,
walk, attack, hit, held block and death still need the coordinator's full review.

No weapon/cape correction is included without actual posed clearance evidence.
The anatomical drawn grip and all props are retained. A separate sheath/scabbard
attachment adjustment is possible after measuring idle/walk/attack clearance;
moving the drawn sword globally to fix one cape crossing would compromise grip.
