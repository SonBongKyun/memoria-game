# Migration handoff — Verdan 2.5D depth prototype

Status: verified; local checkpoint is the commit containing this handoff.
Worktree `C:\Users\jc\MemoriaMigration\foundation`, branch `unreal-migration/ue58-foundation`.
Entry checkpoint `3bc4b582a10e782f74ed5daa4e0357bdbbfe60c0`. UE5.8.2 / CL56702186.

[Current report](DEPTH_1_REPORT.md), [play instructions](PLAYABLE_SLICE.md),
[retained narrative frontier](PHASE_1O_REPORT.md).

- The user's 2.5D/3D request is implemented first as an actual 3D Verdan courtyard:
  six buildings, eight pitched roofs including two stalls, four real lantern lights,
  lit materials, shadows and fog. Perspective camera:65 FOV,42-degree downward tilt.
  Existing original Arrel/Malet sprites remain camera-facing 2D art with16 gait frames.
  Existing narrative illustrations and portraits are retained.
- Movement, collider, seven original physical surfaces, Malet position/range and
  source gameplay remain. The foundation map keeps its orthographic camera.
  New4 presentation packages; total86. Prior82 packages and90 IR/fixtures preserved.
- Fresh final rendered visual3 and full UE203 passed. Independent1500 checks over1475
  fresh snapshots passed. Host96/static67 passed. Final original4217/protected22090,
  prior82 packages/90 IR-fixtures and historical evidence preservation passed.
  Exact staged raw evidence and LFS verification is recorded in depth1/staged_review.json.
- Existing source oracle fixtures are reused inputs; Godot gameplay/native executables
  were not rerun. Initial compile/replay failures and earlier captures are retained.
- This is a first 2.5D prototype, not finished environment art or skeletal3D characters.
  The courtyard follow camera can expose empty exterior ground at edges; posts can
  partially occlude sprites. Camera bounds/occluder handling remain subsequent work.
- Frontier stays `before:shop_actions`. Purchases/sales, close callback, Chapter3,
  autosave, battle and packaged distribution are not added. No push.

Next visual milestone: camera bounds/occlusion and a coherent environment-art kit
based on the existing illustrations; prototype one3D character before expanding.
Original Godot source, existing packages/fixtures and historical evidence stay immutable.
