# S356 character source: Accepted base and HeadRefinement

`Accepted/` contains four base mesh FBX files, twenty animation FBX files, four geometry manifests, two humanoid clip manifests, the original generated two-panel texture and its prompt record. The two NPC meshes preserve complete painted source bodies; both creatures and all twenty safe animation sources were copied byte-for-byte from the prior revision. The final two encounter mesh sources/manifests are in `HeadRefinement/`. The final runtime uses 33 Accepted base packages plus two HeadRefinement mesh packages. Accepted NPC paint, creature surfaces and safe clips passed the coordinator’s focused native checks; the two final head meshes passed the fresh HeadRefinement Detail/Strike/Death native review and focused 4/4 replay. The coordinator owns build/import/rendered evidence.

## Reproduce

Run in PowerShell from `C:/Users/jc/orca/workspaces/Game/memoria-unreal-codex`:

```powershell
& 'C:/Users/jc/Documents/Codex/MEMORIA-Unreal-Collaboration/models/_tools/blender-4.5.3-windows-x64/blender.exe' --background --factory-startup -t 2 --python-exit-code 2 --python Unreal/Tools/build_character_geometry_s356.py
```

Blender is 4.5.3 LTS, hash `67807e1800cc`. The delivered NPC sources were retained from the narrow human authoring pass; creature and clip sources were copied unchanged. The current default command reproduces four base meshes plus twenty clips. Final encounter meshes use `-- Heads`. `-- Clips` authors only the ten humanoid clips. Names restrict generation; use a fresh scratch checkout/revision for reproduction.

The generator reads S354 mesh/skin/export helpers without modifying them. Helper SHA-256: `4774661b239b3139e3835276de2a107bc9147215deb7621175816a357ec1fab2`. Default output goes to `CharacterUpgradeS356/Accepted`; `-- Heads` writes only to `CharacterUpgradeS356/HeadRefinement`; original source FBX, paint pixels, skeletons, animations and S354 source/packages remain immutable.

## Geometry

Measured Blender centimeters and final triangles before Unreal import:

| Character | Height | Triangles | Bones | Result |
| --- | ---: | ---: | ---: | --- |
| Elia | 166.0166 | 26,674 | 77 | Complete original embroidered dress; open back cape, side satchel and small clasp |
| Malet | 180.0000 | 27,652 | 77 | Complete original merchant coat, waistcoat and clear glasses face; side pouches, rolled blanket and watch chain |
| AshWalker (HeadRefinement) | 185.0251 | 13,436 | 77 | Original charred body and void mask on the neck; surface-fitted calcified brow/cheek relief and rooted crown |
| BeltScavenger (HeadRefinement) | 164.9686 | 12,676 | 77 | Original leather body/face; fitted eye/brow/mouth/scar relief, compact nose, salvage pack and greaves |
| VoidWisp | 116.0772 | 3,900 | 20 | Fractured nucleus, crown and six articulated trailing tendrils |
| MemoryLeech | 100.6488 | 20,762 | 12 | Continuous arched annelid, annuli, sucker jaw, hooked teeth and sensory palps |

Elia retains all 12,789 original source vertices and adds 708 accessory vertices. Malet retains all 12,806 and adds 1,252. FBX roundtrip proves exact equality of original body coordinate/UV corner multisets: 76,284 original triangle corners each. Neither body is welded, rebuilt, retextured or hidden under a replacement garment shell. No new hat, hood, scarf, apron front or bodice-covering layer is added.

The original S313 monster source deliberately authors a faceless husk behind an open hood. Both final foe meshes remove the separate HoodShell (360 vertices) and HoodRim (328); Scavenger also removes the mouth scarf (264) and old detached eye sockets (84 each). Ash retains its original void-mask surface (260) at its original center, connected to the neck. A previous forward-13cm mask offset is discarded. Brow/cheek relief follows ray intersections against the actual source face triangles with only 0.1cm center protrusion; crown roots start embedded in that same surface. Scavenger eye/brow/mouth/scar paths follow its source face curvature, and the small nose has embedded base vertices. The detached BackHeadBinding sphere is removed, with no artificial height-restoration geometry.

Final Ash retains 6,662 original vertices and adds 416 accessories. Scavenger retains 5,926 and adds 796. Their actual source heights are **185.0251 and 164.9686 cm**. Numeric runtime `Spec.Height` remains **185 and 165** respectively: the mesh bounds normalize to those unchanged values. The prior Scavenger source height172 included the removed hood/binding and is no longer the final visible-mesh bound. Original rig transforms remain unchanged.
Leech flesh is fused, smoothed and reduced before weights, UVs and color masks are assigned. Source bounds: 74.6367 wide × 126.9432 long × 100.6488 high. At unchanged runtime height 150, this is **111.2333 × 189.1875 × 150**. Authoring asserts longest horizontal dimension ≤225, width ≤130 and ≤25,000 triangles. Wisp remains 135 runtime units high, normalized width 72.0747 and length 73.0260. No collision, reach or numeric encounter tuning changes.

## Paint, materials and rigs

Each human mesh has two sections: slot 0 binds the exact immutable original body material interface; slot 1 binds the new opaque lit `M_{Name}Refined` accessory material. Original paint is never blended with an accessory palette. Original body vertex alpha is 0; new accessory alpha is 1 with explicit cloth/leather/bronze RGB colors. Accessory shaders use those colors directly, with restrained normal weave, roughness and existing hit/rim cues. Runtime updates every slot for hit callbacks and foe tint/glow parameters.

All four original 77-bone names, parent indices and bind transforms remain. Blender FBX roundtrip maximum matrix deltas range from 0.000034 to 0.000124; Unreal preflight uses the established 0.015 tolerance. Humanoid skeleton packages are referenced and never saved. NPC body dimensions remain unchanged; final head bounds are recorded above. Elia/Malet keep original idle/walk and staff/ledger/vials. Scavenger retains the original thief dagger.

Wisp and Leech use untouched `Accepted/SupernaturalAtlas.png`, 1254 RGB: left slate-blue fractured crystal, right desaturated brown-purple annelid skin. SHA-256: `0a608f911de9ba69c90b34d3a24712757baf730c462c79ea94dbbbb415bbf376`. `generation_prompts.json` records original generation. Region colors mask restrained nucleus/sac glow. Leech uses a nearly black sucker and pale brown-ivory teeth; regional glow is 0.01, while original runtime Glow/Crack values remain unchanged. The accepted creature source geometry and animation FBX bytes are unchanged from Final.

## Animation

AshWalker/BeltScavenger and base Husk/Thief use five directly authored compatible clips each: idle, walk, strike, hit and held death. Every original 77-bone local transform is keyed with unit local scale; no mannequin retarget is applied. The importer resamples normalized keys to each immutable original clip's frame rate and exact frame count, asserts identical play length, and checks all compressed bone scales at four sample times. Strike apex remains 0.55; gameplay windup, hit scheduling and animation timing stay unchanged. Original clip packages remain immutable.

The prior native diagnostic found inherited humanoid Hit clips had zero root/head scales, collapsing every body vertex to actor Z=-8. The preceding Final revision's safe clips passed native body/head CPU checks. Accepted reuses all those clip FBX bytes unchanged; the fresh native replay confirms visible bodies during hit/recovery/strike/death.

Each supernatural rig has five roles, at 30 fps:

| Role | Wisp | Leech | Last frame |
| --- | --- | --- | ---: |
| Idle | Idle | Idle | 60 |
| Locomotion | Walk | Walk | 30 |
| Strike | Cast | Bite | 24 |
| Hit | HitReact_Front_Lgt_01 | HitReact_Front_Lgt_01 | 15 |
| Held death | Death_Front_01 | Death_Front_01 | 40 |

Exports use LINEAR vertex colors, front -Y/up Z, unit scale 0.01, `FBX_SCALE_UNITS`, deform bones only, no leaf bones, one-frame bake steps and no animation simplification. Runtime heights, classifications, HP/damage, abilities, collision and rewards remain unchanged.

## Fresh legacy import

After MemoriaEditor builds:

```powershell
& 'C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' 'C:/Users/jc/orca/workspaces/Game/memoria-unreal-codex/Unreal/Memoria/Memoria.uproject' -run=MemoriaCharacterUpgradeS356 -unattended -nop4 -nosplash -AllowCommandletRendering -RenderOffscreen
```

`-AllowCommandletRendering` is required for material rendering resources. The commandlet rejects non-rendering invocation before creating targets.

The normal mode creates **33 immutable base targets** under `/Game/Memoria/Presentation/Field3D/CharacterUpgradeS356/Accepted/`: four meshes (two NPCs, two creatures), two autogenerated creature skeletons, six materials, twenty clips and one texture. It preflights **25 import source files** (four meshes, twenty clips and atlas). Ash/Scavenger materials and clips use their immutable original rigs; normal mode never imports or saves their superseded Accepted mesh bodies.

Then run the same commandlet with `-HeadRefinement`:

```powershell
& 'C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' 'C:/Users/jc/orca/workspaces/Game/memoria-unreal-codex/Unreal/Memoria/Memoria.uproject' -run=MemoriaCharacterUpgradeS356 -HeadRefinement -unattended -nop4 -nosplash -AllowCommandletRendering -RenderOffscreen
```

Head-only mode preflights **two new mesh sources and two new mesh targets** under `CharacterUpgradeS356/HeadRefinement/{AshWalker,BeltScavenger}`. It validates the original77rig/material interfaces, two frozen Accepted accessory materials and all ten compatible Accepted clips before importing either mesh. It saves only those two meshes and references all dependencies without resaving them. Final package count is **33+2=35**. Each mode refuses the entire operation if any of its target packages already exists. Use clean disposable paths for reproduction.

Legacy `UFbxFactory`: explicit mesh/animation type, automatic detection off, no physics/material/texture autoimport, scene conversion/front X/unit conversion enabled, `ImportRotation=(0,-90,0)`, T0-reference pose off, imported normals/tangents, vertex colors replaced, animation default sampling/exported time. Materials must compile before save. Do not use generic Interchange reimport; reproduce in disposable scratch/new paths.

## Evidence and immutable attempts

- `Saved/Validation/s356/accepted_source_inspect.log`: original mesh connected-component measurements, including head surfaces.
- `Saved/Validation/s356/accepted_humanoid_authoring.log`: four Accepted meshes, complete authoring marker, no traceback.
- `Saved/Validation/s356/accepted_roundtrip_final.log`: four roundtrips, exact NPC body coordinate/UV equality, two material slots, alpha masks and measured bind deltas. An earlier diagnostic used an unnecessarily tight 0.0001 threshold and stopped at Ash; its measured 0.000124 delta satisfies the established 0.015 import contract.
- `Saved/Validation/s356/attempt4_character_sources/archive_manifest.json`: all 36 rejected Final source files preserved byte-intact, with 28 unchanged source-copy hashes. Accepted mesh/manifests replace only the four humans.
- `Saved/Validation/s356/characters/author-final-humanoid-clips.log` and `Accepted/{AshWalker,BeltScavenger}/clips.json`: ten safe original-rig clips and unit-scale records.
- `Saved/Validation/s356/characters/author-refined-leech-density.log`: final 20,762-triangle Leech density pass.
- Earlier source/package attempts are preserved under `attempt2_character_sources`, `attempt3_character_sources`, and coordinator-owned `attempt2_character_packages`, `attempt3_character_packages`, `attempt4_character_packages`.

Runtime replay checks real CPU-skinned world vertices, mandatory retained head vertices, camera projection, hit/strike/death and natural return to idle/walk. Imported bounds or bone names alone cannot establish a visible body. Native captures remain required for art acceptance. No LOD or shipping-performance claim is made.
## Final head source reproduction

The serialized head authoring command is the Blender invocation above followed by `-- Heads`. It writes only `HeadRefinement/{AshWalker,BeltScavenger}` and refuses existing head FBX targets. The default generator authors four base meshes and twenty clips; the separate Heads command authors only the final two encounter meshes. Together they reproduce all27import inputs in a fresh checkout. Explicit AshWalker/BeltScavenger mesh arguments refuse and direct callers to `-- Heads`. `HeadRefinement/*/geometry.json` records actual mesh height and unchanged runtime world height. `head_refinement_authoring.log` and `head_refinement_roundtrip.log` under `Saved/Validation/s356` record both successful source exports and two-slot/77-bind roundtrip checks. Accepted source bytes and all twenty safe clip FBX files are untouched by this head pass.

The coordinator archived the two superseded Accepted encounter mesh packages under `attempt5_character_packages`, retaining the other33Acceptedpackages byte-intact. `HeadRefinement/source_inventory.json` records the two final source meshes/manifests. Fresh HeadRefinement native Detail/Strike/Death captures and the focused 4/4 replay passed. These humanoids remain stylized and simpler than the painted hero; final registry results are recorded in docs/unreal-migration/S356_REVIEW.md.

The four superseded Accepted encounter mesh FBX/geometry manifests were archived byte-intact under `Saved/Validation/s356/attempt5_character_sources/archive_manifest.json`. Accepted/source_inventory.json verifies the32remaining delivered source records against their previous frozen hashes; all24base/clip FBX bytes and atlas pixels remain unchanged.
