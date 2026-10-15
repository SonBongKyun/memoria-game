# S359 — natural NPC anatomy and humanoid motion

Accepted immutable revision: `R6`. Blender 4.5.3; Unreal 5.8.2 clean legacy FBX import. No raster images were generated or edited.

- Nine ambient NPCs: Traveler, Bureauagent, Guard, Villagerf, Villagerm, Fisherman, Elder, Child, Scholar. Replaced oversized painted head geometry and angular garments with a modeled cranium/jaw/nose/mouth, stitched orbital lids/eyes, fitted hair/ears, continuous shoulder/sleeve surface, shaped trousers and anatomical boots with flat soles. Original articulated hand geometry retained.
- Elia and Malet: new face/hair geometry; original painted garment coordinates, UVs, weights and material interfaces retained below the documented head cut (`height - 24 cm`). Their scanned clothing retains source irregularity. Native tests compare original position/UV corner pairs below the cut.
- Eleven new meshes, thirty-two animations, one opaque lit vertex-color material. Two humanoid foe mesh families remain the immutable S358 meshes, including S357 faces, with improved idle/walk/strike/hit/death motion. Husk/Ash Walker and Thief/Belt Scavenger share those two motion families. No new textures.

All existing 77-bone names, hierarchy and bind transforms are retained. Mesh heights remain unchanged. The face topology helpers come from the unchanged `build_humanoid_faces_s357.py`; output receipts bind that dependency and the executed S359 generator. Faces remain stylized and share base topology; there is no blink or facial-expression animation.

Motion is authored on the actual rig in armature space. Two-bone leg/arm solves place wrists beside the torso and feet on support targets. Gait has a planted support half and a lifted swing half; the cubic swing matches support velocity at toe-off and heel contact. Pelvis/torso counter motion and relaxed elbows/wrists replace the prior excessive arm rotation. NPC idle loops are 4 s; walk source loops are 1.2 s. Foe clips are resampled to their exact previous combat durations, rates and frame counts, preserving hit/strike deadlines.

Distance-driven runtime gait matches the actual scaled foot travel: full cycle `2 * step * worldHeight / 180`, with step 30 cm for adults, 17 for Child, 23 for Ash Walker, 26 for Scavenger. Movement speeds, collision, encounter values, saves, narrative and product cameras/lighting are unchanged.

Color alpha categories: skin/hair 0, leather .5, cloth .75, metal 1. Every exported mesh has exactly one canonical FLOAT_COLOR layer named Color. UVs are saved and restored across Blender CustomData color allocation. Each source FBX is reimported to verify color means, UV corners and bind matrices. The Unreal importer independently rejects missing/white pigment in the actual rendered face/cloth sections. Original painted material slot 0 and accessory material slot 1 remain on named NPCs; new facial color uses slot 2.

Source and rejected candidates are distinct. R1/R2 shoulder/boot candidates, R3 density interruption, R4 white layer-zero pigments, and R5 UV/contact discontinuity attempts are preserved under `Saved/Validation/s359`; only R6 is delivered. Final rendered acceptance: visual30 + Foundation6 + S358 current-NPC compatibility1 + S359 anatomy/contact1 = 38/38 unique, 36 success and 2 success with warnings, exit0/fatal0. All 641 old packages and 394 final executed inputs unchanged. No full274 registry, Shipping, new LOD or performance claim.

Run authoring only against a fresh revision directory. The generator refuses existing mesh targets; the commandlet preflights every destination before importing. Review, exact receipts and unedited native evidence: `docs/unreal-migration/S359_REVIEW.md` and `S359_RESULTS.json`.
