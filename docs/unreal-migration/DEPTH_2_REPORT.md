# Verdan depth prototype 2 — camera and market detail

2026-09-20. Worktree `C:\Users\jc\MemoriaMigration\foundation`.
Entry checkpoint `0d27f6a36280bb03bd6b674fd1720bc4063d7c5a`.
UE5.8.2 / CL56702186. Verified. The local checkpoint is the commit containing this report; no push.

## Result and scope

The bounded Verdan courtyard now keeps the camera closer to the environment as the
player approaches a boundary. It follows the player exactly inside the central
X +/-450 / Y -180..100 area, then eases spatially toward finite limits. The camera
anchor is bounded by X +/-550 / Y -360..280; its existing perspective, 65-degree
FOV and 42-degree downward tilt remain. The foundation test map is unchanged.

Architecture between the camera and Arrel receives a small, feathered cutaway
centered on the actual sprite. This is a masked, lit material using Unreal's
DitherTemporalAA function, driven by the actual camera and sprite positions.
It leaves ground and surfaces behind Arrel opaque. Collision is unchanged.
The mask is built for this fixed camera orientation; it is not a free-camera system.

The intact original `motif_memory_lantern_v1.png` now appears at all four lamps.
The original market illustrations guide the wine/moss cloth hangings, sagging
rope, wooden window shutters, memory bottles and stacked ledgers. The lantern is
imported directly; the other three illustrations are reference only, as recorded
with source hashes in `evidence/depth2/artwork_sources.json`.

Exactly three additive packages: `Depth2/M_FocusSurface`, `T_MemoryLantern` and
`SPR_MemoryLantern`. The material duplicates the previous surface into a new
package; the authoring commandlet refuses existing destinations. All previous86
packages remain byte-for-byte intact; total89. No maps were resaved.

The original seven physical bodies, movement, 16-frame directional gait, Malet
position/range, dialogue, memory domain and shop boundary remain. Existing portrait
and narrative illustration presentation remains. No new narrative IR or asset.

## Validation and iteration record

- `build01` failed on two new C++ pointer/vector type mismatches. The loop now
  respects TObjectPtr and sprite dimensions use FIntPoint. `build02` passed.
- `host01`:96 tool tests passed. `author01` created only the three named new assets.
- `visual01`:3 rendered tests passed, including all16 gait frames, seven physical
  surfaces, wall blocking, Malet's reachable prompt, four imported lantern sprites,
  camera/sprite projection at eight edges/corners, and unchanged run/memory/trace.
- `campaign01`:1/4 passed. Three existing camera-follow assertions failed because
  the initial X +/-220 following area limited a normal 430-unit post-dialogue walk.
  The central following area was widened to +/-450, with a +/-550 limiting anchor.
  Existing campaign test inputs and expectations were not changed. Initial captures,
  logs and failed result remain archived separately.

- Final `visual02`:3 rendered checks passed after the camera adjustment. All eight
  edge/corner captures, Market and the same-location occlusion A/B were reviewed.
- `campaign02`:4/4; `shop01`:12/12; `malet01`:4/4; `foundation01`:6/6 passed.
  Fresh rendered total29/29. All processes exited0 with no fatal diagnostics.
  The unchanged campaign and Malet camera-follow assertions pass; the original
  foundation orthographic camera, collisions and input tests also pass.
- Final `static02`:68 checks passed; `host01`:96 tool tests passed.
- Final `preservation02`:original4217, protected worktree22163, old86 UE packages,
  prior90 IR/fixtures, original manifest and historical reports/evidence preserved.
  Exactly3 new presentation packages; new narrative IR/package0.
- Selective staged paths, raw evidence blob equality, ZIP member hashes, LFS
  pointers and the unchanged session-log prefix are recorded in `staged_review.json`.

![Actual final UE market](evidence/depth2/MarketFinal.png)

![Occluding post reveals Arrel](evidence/depth2/BoundaryClearFinal.png)

## Evidence and limits

Each execution has its own directory under `evidence/depth2`; ZIPs preserve raw
logs, the automation index, fresh state JSON and selected screenshots with SHA256
manifests. Earlier attempts and all Depth1/Phase reports remain unchanged.
Selected PNGs are copied without editing from the successful rendered-test archive.

The full UE203/203 and independent1500 checks over1475 snapshots from Depth1 are
reused historical evidence. They were not rerun as a full suite in Depth2. Protected
runtime, tests and inputs are compared with entry raw hashes; only the presenter and
its visual test changed among existing runtime/test files. Source gameplay/native
oracles were not rerun. Fresh focused checks are listed separately above.

This is still a small editor development slice with 2D characters in a 3D setting.
Camera bounds reduce exterior exposure; exterior ground is still visible at some
edges. Feathered cutaways can show a dithered rim. Architecture is a prototype kit,
not final environment art, and lantern drawings remain small at this camera scale.

Frontier remains `before:shop_actions`: purchases/sales, shop-close callbacks,
Chapter3, battle and packaged distribution are outside this task. No push.
Next visual milestone: refine a coherent environment kit and prototype one rigged
3D character against the existing character illustrations before scaling up.
