# Character 2 — Arrel surface refinement and planted walking

Status: verified prototype refinement; final byte gate recorded below.
Entry checkpoint: `fa8db119ba5140752d2fa6b642c53abf6352b7e2`.
Worktree: `C:\Users\jc\MemoriaMigration\foundation`, branch `unreal-migration/ue58-foundation`.
Engine: UE5.8.2 / CL56702186. This report belongs to its containing local checkpoint.

## Result and boundary

Arrel's large breastplate/undercoat intersections are removed. Both nested surfaces
now follow the chest bone with physical clearance. The refined silhouette adds a
shaped jaw, smaller eyes, volumetric silver hair, layered shoulder plates, narrower
arm armor, articulated waist/thigh plates and flat boot soles. The initial broad
exposed posterior scalp was covered in a final additive hair pass.

Existing Arrel turnaround and story portrait guide provisional colors and silhouette.
The108-chapter manuscript is being rebuilt through chapter11; neither chapter11 nor
any other chapter is assumed final. No manuscript, dialogue, event or memory effect
was changed. The visual scabbard still grants/equips nothing.

New assets are only `Character2/SK_ArrelRefined` and `Character2/SKEL_ArrelRefined`.
They reuse the two Character1 materials without resaving them. All93 previous UE
packages, including the first character model/skeleton/materials, remain unchanged;
the new total is95. The original generator and v1 JSON still reproduce exactly.
The new v2 source has23 bones,22272 source vertices and7424 triangles.

The surface check intersects actual torso triangles at2048 height/direction samples:
minimum clearance is1.95625 source cm. The last96 added triangles only cover the head;
all prior7328 triangles remain identical, so the measured torso clearance still applies.
See `final_geometry.json`, `final_geometry02.json` and `hair_refinement_scope.json`.
This is not a global claim that every cloth/armor pair avoids intersections in all poses.

## Locomotion and practical limits

The stance part of each stride now advances linearly against actual displacement;
swing uses eased travel, foot lift and ankle roll. Pelvis height and two-bone leg
poses maintain flat contact during steady straight walking. Exponential blending
settles consistently across the tested30/60/120fps. Arm swing and chest lean are toned down.

The unchanged development pawn defaults to1200 units/second, far above this small
character's walk scale. Animation cadence caps at3 cycles/second above approximately
190.4 units/second. High-speed foot sliding remains: this change does not silently
alter movement speed, collision, map scale or story contracts. Turning and start/stop
transitions are not foot locked. A coordinated movement-speed/stride/camera-scale pass
is the next play-feel task. The character still uses faceted prototype art and procedural
bone poses rather than finished textures, authored clips or cloth simulation.

## New verification and retained iterations

- `build01`: passed. `author01`: two new packages imported successfully.
- A breastplate ridge's outward winding was corrected before import. Original new
  source bytes remain in `art_source01.zip` with hashes.
- `visual01`:2/3 passed; only the new contact measurement failed. Its near-ground
  threshold included the start/end of swing. All original movement, boundary, camera,
  reachability, run/memory and narrative-trace checks passed. Its raw captures, report
  and exact test source are retained.
- The measurement now checks both flat ankle orientation and ground height; the
  maximum permitted slip was not relaxed. `visual02`:3/3 passed. At60/120/180 units/s
  across30/60/120fps, measured planted-foot slip is at most0.000028 world units/sample.
  All12 speed/frame-rate cases check finite poses, sole clearance, lift and settled stop;
  the three1200-speed cases deliberately make no no-slip claim.
- `campaign01`:4/4 and `shop01`:12/12 passed on the final runtime implementation,
  before the last mesh-only posterior hair addition.
- Initial Character2 source and both new raw packages are retained in
  `before_nape_refinement.zip`. Only those two untracked task-owned package files
  were rebuilt. No old package, material, rig definition, runtime or test code changed.
- `author02`:success. `visual03`:final3/3 passed after the additive hair refinement.
  Front/back/side and walking frames were visually inspected. Final untouched images
  are under `final_captures/`, with source ZIP member hashes in `final_captures.json`.
- Fresh focused UE19 checks comprise final visual3 plus the16 earlier gameplay checks;
  those16 were not rerun after the hair-only change. Host96 and final static03:71 passed.
- Full UE203 and independent1500 checks over1475 snapshots from Depth1 remain reused
  historical evidence. No full-suite, Godot gameplay or native oracle rerun is claimed.

## Preservation and checkpoint

Entry index/worktree were clean. The only changed prior runtime file is
`MemoriaArrel3DComponent.cpp`; all54 other prior runtime files remain byte-identical.
There are no new narrative IR/assets/packages. Existing collider, seven physical
surfaces, production camera, Malet location/range and `before:shop_actions` remain.

`preservation01` passed:original4217, protected worktree22303, oldUE93, priorIR/fixtures90,
and historical reports/execution evidence. Final preservation02 also passed after the hair refinement with all the same protected counts.
Selective staged paths, raw evidence blobs, source JSON, backup ZIPs, LFS pointers and
session-log prefix are verified in `staged_review.json` before the local commit.

No push, shop transactions/close callback, Chapter3, battle expansion or packaged release.
Next: align movement speed, character stride and camera scale, then continue character art.

![Actual final UE front view](evidence/character2/final_captures/CharacterFront.png)
![Actual final UE back view](evidence/character2/final_captures/CharacterBack.png)
