# Migration handoff — Verdan camera and market detail

Status: verified; local checkpoint is the commit containing this handoff.
Worktree `C:\Users\jc\MemoriaMigration\foundation`, branch `unreal-migration/ue58-foundation`.
Entry checkpoint `0d27f6a36280bb03bd6b674fd1720bc4063d7c5a`. UE5.8.2 / CL56702186.

[Current report](DEPTH_2_REPORT.md), [play instructions](PLAYABLE_SLICE.md),
[retained narrative frontier](PHASE_1O_REPORT.md).

- Actual 3D courtyard with six buildings, two stalls, lit materials and shadows;
  original Arrel/Malet sprites remain, with16 directional gait frames for Arrel.
- Camera follows exactly in the central area, then eases toward finite boundary
  limits. Eight edge/corner cases keep the whole character on screen.
  A feathered cutaway reveals Arrel behind architecture without changing collision.
- Four lanterns use an intact original illustration. Wine/moss cloth hangings,
  shutters, bottles and ledgers develop the market illustration's visual direction.
  New3 presentation packages; total89. Old86 packages remain byte-identical.
- Fresh rendered29/29:visual3, campaign4, shop12, Malet4, foundation6. Host96/static68
  passed. Final original4217/protected22163/old86/IR-fixtures90 preservation passed.
  Raw staged evidence/LFS verification: `evidence/depth2/staged_review.json`.
- Full UE203/203 and independent1500/1475 from Depth1 are reused historical evidence,
  not fresh full runs. Source gameplay/native oracles were not rerun. Initial build
  failure and the first campaign camera-follow failure remain archived. The latter
  was fixed by widening the central following area; old test expectations remain.
- This remains a 2.5D prototype with original 2D characters. Exterior ground remains
  visible at some edges; cutaways have a dithered rim. No production 3D character yet.
- Frontier stays `before:shop_actions`. Purchases/sales, close callback, Chapter3,
  autosave, battle and packaged distribution are not added. No push.

Next visual milestone: refine a coherent environment-art kit from existing
illustrations and prototype one rigged3D character before expanding the cast.
Original Godot, prior packages/fixtures and historical evidence stay immutable.
