# Migration handoff — first Arrel skeletal character blockout

Status: verified as a blockout; local checkpoint is the commit containing this handoff.
Worktree `C:\Users\jc\MemoriaMigration\foundation`, branch `unreal-migration/ue58-foundation`.
Entry checkpoint `fd8ba39fc1e2c61c9f8a38f173e9af0b9be1613c`. UE5.8.2 / CL56702186.

[Current report](CHARACTER_1_REPORT.md), [play instructions](PLAYABLE_SLICE.md),
[retained narrative frontier](PHASE_1O_REPORT.md).

- The user is rebuilding the108-chapter manuscript through chapter11. No chapter is
  assumed final. Existing illustrations guide provisional appearance only; old Godot
  story data and art labels are not promoted to the new canon. No manuscript edits.
- Arrel now uses a real23-bone,5757-triangle skinned mesh in Verdan. Actual displacement
  drives body rotation and procedural legs/arms; idle breathing and cape motion are
  included. The deterministic generator and JSON retain editable parts/weights.
- Silver hair, navy fabric, metal armor, blue accents and a visual scabbard follow
  existing concept references. The accessory does not grant/equip a weapon. This is
  an interim blockout; face/hair, breastplate intersections and gait need an art pass.
- New4 packages:two materials, skeletal mesh and skeleton; total93. Old89 packages,
  original4217, protected worktree22223 and IR-fixtures90 preserved. New narrative0.
- Existing movement/collider, seven bodies, camera bounds, Malet interaction, dialogue
  and memory state remain. Occlusion targets the3D body. Malet stays2D. Foundation
  presentation is unchanged. Character-only fill and a small material ambient lift
  improve readability without relighting the world.
- Fresh focused29 checks passed:visual3, campaign4, shop12, Malet4, foundation6.
  The26 gameplay checks precede the final two-material lift; final visual04 was rerun
  afterward. Host96/static70 passed. Full UE203 and independent1500/1475 from Depth1
  remain reused historical evidence, not new full runs. No source/native oracle rerun.
- Initial API compile failure, skeleton-finalization authoring assertion, first visual
  bounds failure, dark captures and original new-package bytes are retained. Staged
  evidence/LFS/source checks: `evidence/character1/staged_review.json`.
- Frontier remains `before:shop_actions`; purchases/sales, close callback, Chapter3,
  battle and packaged distribution are not added. Local checkpoint only; no push.

Next: refine Arrel's face/hair/armor proportions and intersections against provisional
references, then tune locomotion and foot contact. Final costume/story approval waits
for the corresponding manuscript/design; reusable engine work can continue.
