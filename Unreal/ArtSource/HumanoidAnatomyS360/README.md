# S360 continuous human anatomy

This revision replaces tubular ambient anatomy with continuous human topology, new independent 77-bone anatomical binds and compatible motion. Bone names/parents and gameplay heights remain unchanged; bind positions deliberately change. Elia and Malet retain their original painted garments/UVs while fitting the coordinates to human joints. The original authored humanoid foe faces/gear remain, with fitted anatomy, boots and a physical mixed-weight neck bridge.

## Active package/source revisions

- R7: eleven NPC meshes, thirteen skeletons, twenty-two NPC Idle/Walk clips and one vertex-pigment material.
- R8: ten humanoid foe clips, using R7 skeletons. R8 mesh source is retained as an input; its rejected native meshes are outside Content.
- R9: seven Elia combat clips, using her R7 skeleton and original Elia frame counts/rates/durations. R9 foe mesh source is retained for provenance, but its native meshes were rejected for added neck UVs outside the face domain.
- R10: two accepted foe meshes with explicit normalized UVs on the added neck bridge. Exact R8 body/gear/face positions/UVs/materials and R7 binds are retained. No new foe clips or skeletons.

Total active assets: thirteen meshes + thirteen skeletons + thirty-nine clips + one material = sixty-six packages. No new textures. R7 foe meshes/clips were rejected for a detached head, R8 meshes still had a neck gap, and R9 added-neck UVs failed the actual imported face-domain check. Their native packages are quarantined under Saved/Validation/s360 and never used by runtime. R1-R6 source candidates are also outside ArtSource, under Saved. Source receipts for R7/R8/R9 remain necessary inputs to later immutable revisions; source presence does not imply runtime acceptance.

## Vendor assets

The hm08 base and nine macro target files come from the official MakeHuman repository at commit `a8bc2d54ff0ac92e78ff71431b1023eda42bf482`. These graphical assets are CC0. No upstream program code is incorporated. Original bytes, per-file source URLs/SHA256/size, LICENSE.md and LICENSE.ASSETS.md are in Vendor/. [Official pinned license](https://github.com/makehumancommunity/makehuman/blob/a8bc2d54ff0ac92e78ff71431b1023eda42bf482/LICENSE.md).

## Reproduce

Use Blender 4.5.3 and a fresh immutable output/package revision. Main `Unreal/Tools/build_humanoid_anatomy_s360.py` reads the local S358 sources, computes human joints from base joint groups, fits retained clothes, applies age/sex morphs, builds clothed body/face/hands/hair/boots and authors motion. It reuses local S359 export/motion helpers; every receipt pins actual generator and source hashes. R7 authoring must precede R8, R9 and R10.

1. `build_humanoid_anatomy_s360.py`: R7 primary source. Dedicated `MemoriaHumanoidS360` legacy commandlet imports new NPC/foe rigs and compatible source clips.
2. `build_humanoid_foe_fit_s360.py`: R8 foe height/boot fit and compatible clips; commandlet `-RepairFoes`. These clips preserve original S356 combat deadlines.
3. `build_humanoid_necks_actions_s360.py`: R9 physical neck bridges and complete Elia actions; commandlet `-JoinNecks`. The Elia clips remain active.
4. `build_humanoid_neck_uv_s360.py`: R10 same neck geometry/binds with explicit 0–1 bridge UVs; commandlet `-NeckUV` imports only two new meshes. Original tiled clothing UVs remain unchanged.

Clean legacy UFbxFactory import uses explicit axis/unit settings, imported normals/tangents and vertex-color Replace. Never use generic Interchange reimport or overwrite historical packages. Remove rejected new mesh packages from Content into bounded validation quarantine, preserving the R7 rigs and R8/R9 accepted clips. Runtime lookups identify the exact revision above. Final imported/rendered UV checks, not only Blender export assertions, are acceptance requirements.

Actor height/movement/collision/combat numbers/save/story/product camera and lighting contracts remain unchanged. Distance-based gait uses each delivered normalized stride. Child bounds and stride are separate from adults. Tests verify all 77 names/parents, finite ordered anatomical joints, original material graph/UV corners, actual color buffers and skinning, foot support during actor travel, planted-hit cadence plus real joint articulation after natural recovery, and actual neck overlap through motion.

This is simplified game art with shared base forms, plain ambient clothing, procedural hair and vertex pigments. Collar/underarm crease and named-clothing scan irregularity remain. No facial expression/blink rig, hair physics or painted skin textures. Native evidence and validation limits are in docs/unreal-migration/S360_REVIEW.md and S360_RESULTS.json. Old passing reports do not certify this revision.
