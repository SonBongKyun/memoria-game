# Verdan depth prototype 1 — 2.5D environment

2026-09-20. Worktree `C:\Users\jc\MemoriaMigration\foundation`.
Entry checkpoint `3bc4b582a10e782f74ed5daa4e0357bdbbfe60c0`.
UE5.8.2 / CL56702186. Verified. The local checkpoint is the commit containing this report; no push.

## Result and scope

The user's request to move toward 2.5D/3D is implemented first in the bounded Verdan
courtyard. Six buildings with pitched roofs, timber frames, chimneys and windows;
two physical stalls with awnings and bottles; lanterns, crates and barrels are actual
instanced 3D geometry. Stone/wood/fabric materials receive real light and shadows.
The floor samples an existing original-art texture. No new external art dependency.

The slice uses a perspective camera at 65 degrees horizontal FOV and 42 degrees
downward tilt. Original Arrel/Malet sprites face the camera and receive lighting;
Arrel retains the original 16-frame directional gait. Narrative illustrations and
portraits from the previous passes remain. These are 2D characters in a 3D setting,
not rigged 3D characters or production-finished environment art.

The seven original physical bodies, movement, 16x16 pawn collider, Malet position
and interaction range remain. New decoration has no collision or navigation impact;
stalls occupy the original two obstacle footprints, with awnings above them, and
buildings stand beyond the existing four boundaries. The foundation test map keeps
its original orthographic camera. No maps or previously authored packages were saved.

Four new packages under `Content/Memoria/Presentation/Depth`: `M_Surface`, `M_Paving`,
`M_Glow`, `SM_PitchedRoof`. Existing82 are protected; total86. The importer refuses
existing destinations by default; its explicit EnableInstancing repair touches only
the three named Depth materials, whose initial raw bytes are archived.

## Validation and iteration record

- `build01`: initial runtime/authoring compile passed.
- `author01`: created four assets. Initial raw packages preserved in
  `evidence/depth1/initial_depth_packages.zip` with SHA256 manifest.
- `visual01`: test compile failed because TestNotNull cannot infer TObjectPtr;
  corrected to an explicit pointer-presence assertion. No runtime pass claimed.
- `visual02`: rendered test failed. Instancing material usage was missing; shader
  preparation pumped engine frames without simulation ticks. Existing frame-based
  replay ran ahead of the game. Retained the failed screenshot and full diagnostics.
  Tests now advance only on actual PIE time. Bounds include noncolliding visual
  components; the perspective size check compares equal horizontal spans.
- `build02`, `author02`: corrected material usage; scoped resave of three new
  materials. Protected prior assets were not resaved.
- `visual03`: rendered3 passed, including all16 gait frames, perspective screen axes,
  real geometry/lighting, original wall blocking and reachable Malet. Captures showed
  roof cropping and a directional-light priority warning. Camera framing was widened,
  light priority made explicit, and emissive lantern cores no longer block their own light.

- `visual04`: wider view and lighting refinement passed3. `visual05`: final darker
  outer-ground material passed3. All seven final captures were retained; Market,
  NearMalet and Boundary were visually inspected.
- Fresh final `automation01`: rendered UE203/203, including the unchanged foundation
  orthographic/input tests, campaign4, shop12 and source parity51. Exit0; no fatal diagnostics.
- Fresh `independent01`:1500 checks over1475 fresh JSON snapshots, including complete
  run/player/world state, ordered traces and actual binary-save roundtrips, all passed.
  Existing attested source oracle inputs were reused; Godot gameplay and native-domain
  executables were not rerun. Previous UE results are retained history, not the final proof.
- Host96 and final static67 passed. Final staged review is recorded separately.

![Actual UE5.8.2 Verdan depth prototype](evidence/depth1/Market.png)

Final preservation (`preservation02.json`): original4217, protected worktree22090,
prior82 UE packages and90 IR/fixtures, original manifest, all historical reports and
execution evidence preserved. Exactly4 new presentation packages; new narrative
IR/package0. Final diff/raw-index/LFS checks are recorded in `staged_review.json`.

The entry worktree/index were clean. Only the listed presentation implementation,
editor authoring/test tools, current handoff/play guide/session append, four new
packages and this task's evidence enter the local checkpoint. No push.

## Evidence and limits

Every attempt is retained separately under `evidence/depth1`. ZIP members retain raw
engine output, execution index, complete fresh JSON states and selected screenshots,
with per-file hashes. Generated Saved files are temporary; historical reports and
archives are immutable. Initial source manifest and entering raw hashes are retained.

Narrative frontier remains `before:shop_actions`: purchases/sales, shop-close callback,
Chapter3, autosave and battle are not added. No source/native Godot gameplay execution
is implied by the visual change. This remains an editor development slice, with no
new packaged release, free camera, vertical movement or full 3D character animation.
The follow camera is not yet clamped to the courtyard; approaching an outer edge can
reveal empty exterior ground. Perimeter posts can partly occlude a character against
the wall. Camera bounds and occluder fading remain a subsequent presentation task.

Next visual milestone: replace the simple architecture with a coherent authored kit,
add carefully placed environment details, and validate one character's 3D silhouette
and locomotion against the existing concept illustrations before scaling up.
