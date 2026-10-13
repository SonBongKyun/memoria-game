# S357 humanoid faces and material regions

This revision refines the Ash Walker and Belt Scavenger on their original 77-bone rigs. It reads the immutable S356 `HeadRefinement` FBX files, replaces connected components weighted entirely to `head` plus the original 32-vertex rigid neck cylinder. The latter is replaced by the continuous blended neck to remove overlap at the collar; its exact bounds and reason are recorded as `removed_neck_component_exception`. All other body and equipment coordinates, UV corners and skin weights are retained. Original source files, skeletons and animation clips are not modified.

## Authored forms

- **Belt Scavenger:** one continuous neck and cranial surface with a shaped jaw/chin, cheek planes, orbital depressions, brows, nose bridge/alae/nostrils, philtrum and compressed lips. Actual eye openings receive fitted annular lids and recessed almond eyes with modeled iris/pupil color regions. Ear bowls, close-cropped hair, a restrained beard tint and an old cheek scar break the former smooth oval silhouette.
- **Ash Walker:** a continuous dark neck and calcified skull/mask with deep empty sockets, angular cheeks, eroded planes and surface-fitted fissures. Four tapered crown prongs begin inside the skull. This remains the existing faceless void-husk design, with no human eye or mouth added.

The new head surfaces use a nonuniform Hermite cranial profile and local sculpt displacements. They are not replaced by an ellipsoid plus separate tube eyebrows. Eyelid, socket and scar surfaces follow the same analytic face surface; no image contains painted facial landmarks.

Every new component is listed with measured bounds and triangle count in `R4/<Name>/geometry.json`. All retained body/equipment triangle corners have a deterministic coordinate/UV/weight digest checked before and after joining. The original 77 bone names, parent names and bind matrices are recorded and asserted unchanged. Source bounds remain within 0.035 cm of S356; runtime heights remain Ash 185 and Scavenger 165. Each complete character must stay below 35,000 triangles.

## Material contract

Exactly three sections are exported in this order:

1. `M_<Name>Body`: original body geometry and UVs with deliberately authored linear vertex albedo and region categories.
2. `M_<Name>Gear`: retained S356 equipment geometry and UVs with the same region contract.
3. `M_<Name>Face`: the complete new neck/cranium and all head features, normalized local UVs.

Vertex alpha is categorical: skin 0, calcified bone 0.25, leather 0.5, cloth 0.75, metal 1. Vertex RGB stores linear albedo. The new four-swatch surface atlas supplies subtle material variation; it does not supply face anatomy. The importer creates new material interfaces and retains runtime hit, glow and crack parameter control.

`SurfaceAtlas.png` is a built-in imagegen original copied intact. Its full generation prompt/provenance is saved alongside it by the coordinator. The geometry generator does not read, generate or edit raster pixels.

## Reproduction

From a clean disposable copy of the Unreal worktree, with no `R4/AshWalker` or `R4/BeltScavenger` output directories:

```powershell
& 'C:/Users/jc/Documents/Codex/MEMORIA-Unreal-Collaboration/models/_tools/blender-4.5.3-windows-x64/blender.exe' --background --factory-startup -t 2 --python-exit-code 2 --python Unreal/Tools/build_humanoid_faces_s357.py
```

An optional final argument after `--` restricts to `AshWalker` or `BeltScavenger`. Existing character output directories are refused before authoring. The generator uses LINEAR vertex colors, front -Y/up Z, centimeters, `FBX_SCALE_UNITS`, deform bones only, no leaf bones, and no animation export. New rigid facial features use the existing `head` bone. The integrated neck transitions from `neck_01` to `head` over pivot Z -7.7 through +2 cm; upper cranial vertices are fully head weighted.

`source_inventory.json` records each newly exported file hash and the generator hash. `geometry.json` includes component measurements, removal records, material-section triangle counts, category counts, face landmarks, source/output hashes and all original bind matrices. Blender source previews are topology review only; Unreal rendered captures and gameplay animation checks are required before acceptance. No LOD, Shipping or performance certification is implied.

## Rejected R1 and channel repair

R1 source previews exposed UV fractions in vertex RGBA before Unreal import. Adding the float color layer reallocated Blender custom data while the script retained a stale UV RNA reference. R2 retrieves both layers by name after all allocation, verifies every newly stored UV/color corner, and checks category colors after component creation, joining and triangulation. A fresh LINEAR FBX reimport verifies every section's UV corner multiset, RGB means, categorical alpha, original 77 bone names and bind matrices. R1 is retained as rejected evidence and must not be imported as final content. R2 also narrows the Scavenger posterior cranium, reduces nasal projection and eyelid contrast, and removes all source face quads intersecting the orbital opening so no old skin triangles remain inside a socket.

## R3 scalp and facial attachment refinement

R2 color/UV/LINEAR-roundtrip checks passed, but source preview rejected its open hair-cap crown and eyebrow surface intersections. R3 retains R2 as evidence. The Scavenger scalp now has 96 angular columns, 24 nonuniform rows concentrated toward the crown, outward shell clearance, a fully closed apex, and a recessed skin pole below it. The outer hair apex preserves the original total height exactly. Eyebrows have 0.22 cm fitted surface clearance to prevent skin breakthrough, and the nasal tip is broader and less projected. All original rig, material-channel and remaining body-preservation checks remain mandatory. R3 still requires fresh source and native rendered acceptance.

## R4 actual-surface attachment and sealed orbital borders

R3 source diagnostics found 77 Scavenger eye rays passing through small gaps, eyelid vertices up to 0.064 cm inside skin, nostril detail up to 0.23 cm inside skin, and crease detail up to 0.069 cm inside skin. R4 freezes actual cranial triangulation first. Each cut orbital boundary is extracted as a closed edge loop; a fitted lid strip duplicates every exact skin boundary coordinate and bridges to the inner eye contour. Dense frontal eye rays must report zero holes. All lip, nostril, scar, eyebrow and Ash fissure ribbons then raycast the actual combined cranial/ocular BVH. Explicit feature triangles are checked at vertices, edge quarters/midpoints and centroids, with bounded local correction until clearance is at least 0.025 cm. Geometry JSON records per-feature clearance/lift, orbital sample counts and exact boundary vertex counts. R1-R3 source snapshots remain rejected evidence. Native rendered review remains required.

## Unreal import and runtime

The new commandlet creates exactly two mesh packages, six material packages and the shared atlas under `/Game/Memoria/Presentation/Field3D/HumanoidFacesS357/`. Import a fresh R4 target with `-run=MemoriaHumanoidS357 -Revision=R4 -AllowCommandletRendering -RenderOffscreen`. It preflights all target packages, original skeleton/normal/clip dependencies and three named slots before saving. Existing targets are refused; use a disposable copy without these generated targets or a new explicit revision. Do not overwrite accepted packages.

`Husk` and `AshWalker` use the new Ash Walker mesh; `Thief` and `BeltScavenger` use the new Scavenger mesh. Their identity, original props, numerical specs and S356 safe clips keep their existing paths. The final native outcome, warnings and before/after captures are documented in `docs/unreal-migration/S357_REVIEW.md` and `S357_RESULTS.json`; source authoring receipts alone do not certify gameplay or final art quality.
### R4 orbital shading seam correction

The first complete R4 source pass sealed geometric eye holes, but separate cranium/lid vertices retained a visible rectangular shading boundary. The corrected candidate joins only the new cranium and two lids, then welds their exact shared outer-boundary vertex pairs. It checks unchanged coordinate/UV/weight and RGBA corner digests. The final mesh explicitly restores all original retained body/gear split normals while new face loops use normals from the joined topology; flat crown shading is retained. Each shared orbital vertex must have continuous loop normals within 0.0001, and original body normal delta must remain below 0.0001. Actual deltas and the exact weld scope are recorded in geometry JSON. The rejected shaded candidate and its executed generator remain in the coordinator's Saved evidence archive.