# HANDOFF TO CLAUDE — S310 rigged field characters

Status: **Arrel, Elia and Malet corrected, locally rigged and applied in the Codex Unreal lane. Rendered visual 7/7 and foundation 6/6 pass. Ready for Claude to review and continue when the user resumes Claude.**

## Start here

Working result: C:/Users/jc/orca/workspaces/Game/memoria-unreal-codex
Base commit: c0022579f80afdba063556c6c0de28177d73da5f
Branch: codex/unreal-battle-core-20260926
Changes are uncommitted. No push. Claude lane and original Godot/foundation were not written.

1. Read models/MANIFEST.md and models/RIGGING_GUIDE_KO.md.
2. Review models/_qa/rigged/unreal/three_characters_unreal.png, MarketFront.png, CharacterFront/Side/Back.png, WalkA..D.png, EliaRigged.png and MaletRigged.png.
3. To adopt into the Claude lane, first run the read-only check:
   ~~~powershell
   & 'C:/Users/jc/Documents/Codex/MEMORIA-Unreal-Collaboration/models/_tools/runtime311/Scripts/python.exe' 'C:/Users/jc/Documents/Codex/MEMORIA-Unreal-Collaboration/models/_handoff/apply_s310.py' --target 'C:/Users/jc/orca/workspaces/Game/memoria-unreal-claude'
   ~~~
   Add --apply only after inspecting the handoff. It requires the same base HEAD, verifies every archive hash, rejects conflicting local files, backs up replaced files, and writes only the manifest paths. Codex has NOT applied it to Claude.
4. Run python Unreal/Tools/validate_unreal.py --build-and-test --rendered --test-prefix MemoriaVisual. --automation-timeout 1200 from the adopted lane.
5. Continue action-combat foundation / mannequin retarget integration from the previous handoff. Do not treat the current models as already retargeted attack assets.

## Files and implementation

- All three: models/<id>/<id>_rigged.fbx, _idle.fbx, _walk.fbx and existing albedo/props.
- Corrected five-finger geometry; 77 deformation bones, 4 weights maximum, one 2048² body material.
- Unreal/ArtSource/FieldCharacters: reproducible import sources; 26 new Field3D uassets.
- MemoriaRiggedCharacterAssetsCommandlet imports bodies, animations, textures, materials and props additively.
- MemoriaFieldCharacterComponent loads the rigged figure and bone-attaches props; old cards remain the fallback.
- MemoriaFieldAnimInstance implements distance-sampled idle/walk blending. Add a montage slot for future combat; no attack/dash/hit/death retargeting is included.
- MemoriaVerdanVisualTests now verifies actual skinned feet and the three figures/props, with final game captures.
- models/_handoff/s310_unreal_handoff.zip and s310_files.json carry all task-owned source/doc/art/package changes. No tool runtime, build cache or executable is inside.

## Validation

Final visual run: Unreal\Memoria\Saved\Validation\unreal-run-20260927T114555038702\Automation-20260927T114612432266\index.json, 7/7 PASS.
Foundation run: Unreal\Memoria\Saved\Validation\unreal-run-20260927T114434430941\Automation-20260927T114437360103\index.json, 6/6 PASS (before the final rigid-prop face rotation).
Build: UE 5.8.2 PASS. FBX import: exit 0, all three at 180/166/180cm, idle 2s/walk 1s.
Blender final roundtrip: all loops continuous to <0.1mm; exact values and deformation diagnostics in models/_qa/rigged/blender_roundtrip_deformation.json.
The field replay observed 63 facing/gait samples; no run, memory or narrative state changes; original collision surfaces and interaction behavior preserved.
413-test full registry not rerun. Prior S309 result remains historical.

## Remaining limitations

- Long garments still share parts of the generated topology with the body. Large combat motions need garment/leg retopology and a fresh deformation pass. Stretch diagnostics are in MANIFEST; do not interpret them as production combat approval.
- Approximate side projection, simplified fingers/props and painted glasses remain. No facial rig, cloth simulation, transparent vial material, drawn blade or opening ledger.
- The Rigify helper control rig is not a live constraint driver of the exported Armature; edit the actual Armature or re-run the local script.
- Optional characters sable/tobias/nera/kairos/veil and walking illustration frames remain deferred.
- Initial rejected skin weights, failed builds/imports, first wrong-facing captures and superseded successful captures are preserved for traceability.

Ownership is released after this report; there is no active Unreal/UBT process left by Codex at handoff. Resume Claude only through the user's instruction.


Shared folder: C:\Users\jc\Documents\Codex\MEMORIA-Unreal-Collaboration
