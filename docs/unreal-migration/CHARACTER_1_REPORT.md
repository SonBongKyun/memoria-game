# Arrel character prototype 1 — first skeletal field character

2026-09-20. Worktree `C:\Users\jc\MemoriaMigration\foundation`.
Entry checkpoint `fd8ba39fc1e2c61c9f8a38f173e9af0b9be1613c`.
UE5.8.2 / CL56702186. Verified as a skeletal character blockout. Local checkpoint is the commit containing this report; no push.

## Scope and manuscript boundary

The user is rebuilding the 108-chapter manuscript and is currently working through
chapter11. No chapter is assumed final. This task uses existing illustrations only
as provisional visual references; it does not promote legacy Godot story data or
illustration labels into the revised canon. Dialogue, quests, memory effects and
chapter structure remain unchanged.

Arrel receives an actual skinned skeletal mesh in the bounded Verdan field.
The reference-guided silhouette uses ash-silver hair, navy fabric, metal armor,
blue accents, a cape and a removable visual scabbard. The accessory does not equip
or grant an item. This is a faceted character blockout, not a finished anime model,
likeness recreation, or final costume approval.

The deterministic Python generator retains editable body parts, materials, bone
positions and weights. Its JSON source has23 bones and5757 triangles. References
and hashes are recorded in geometry_check01.json. No source illustration was edited,
no third-party model was downloaded and no new external authoring dependency was added.

The editor-only commandlet creates exactly four new Character1 packages: two lit
vertex-color materials, SK_ArrelPrototype and SKEL_ArrelPrototype. Existing destinations
are refused. All prior89 packages are protected; total93. No existing map is resaved.

The visual component uses the real skeleton and skin weights. Actual pawn displacement
controls facing and procedural gait; two-bone leg poses, opposing arm motion, idle
breathing and cape motion are generated locally. There is no baked AnimationSequence,
Animation Blueprint, root motion, ragdoll, or combat animation in this first prototype.
Movement and seven original physical bodies remain authoritative. The existing pawn
sprite is hidden only in the Verdan presentation; Malet, narrative art and Foundation
map retain their previous presentation. The occlusion reveal targets the 3D body. A character-only fill and a small
vertex-color ambient contribution keep this stylized blockout readable under the
existing dark field lighting.

## Iteration and validation

- Geometry regeneration reproduces exact source bytes; all triangle areas and skin
  weight sums were checked. Host96 and initial static70 checks passed.
- `build01` failed because PoseableMesh uses the SkinnedAsset accessor rather than
  SkeletalMeshComponent's accessor. `build02` passed after updating that API usage.
- `author01` stopped with an engine assertion before saving packages. The reference
  skeleton modifier had not finalized its derived bone array before conversion.
  Its scope now ends before conversion, with an explicit finalized-bone-count guard.
  `build03` passed. The failed execution and diagnostics are retained.
- `author02` successfully saved the four new packages. `visual01` passed dialogue/art
  checks and all gait/direction/boundary checks, but failed the initial character-bounds
  assertion. Captures also showed the character too dark against the existing night
  lighting. Bounds are now invalidated/refreshed at initialization; a soft, unshadowed
  fill affects only character lighting channel1, preserving the world lighting.
- `visual02`:all3 tests passed. The measured character half-extents are approximately
  26/14.7/60.4 world units; the volume check accounts for its narrower side profile.
  The image review still found the dark surfaces hard to read, so the two new
  Character1 materials receive a small vertex-color ambient contribution. Initial
  raw packages are retained in initial_character_packages.zip with hashes. The scoped
  AddAmbientLift operation accepts only those two packages and refuses repeated use.

- `author03` changed only the two new character materials. Mesh and skeleton bytes
  remain identical to their initial saved packages (`material_lift_scope.json`).
- `visual03`:3/3 passed after the material change. `campaign01`:4/4,
  `shop01`:12/12, `malet01`:4/4, `foundation01`:6/6 passed earlier on the same runtime
  implementation, before that two-material lift. These are fresh focused29 checks;
  the26 gameplay checks were not rerun after the material-only change.
- The final visual replay adds a temporary low-angle camera for front/side/back
  review, then returns to the ordinary pawn camera. Production camera behavior is
  unchanged. `visual04`:final3/3 passed, and the field plus three review angles were inspected.
- Final static03:70 passed. Final preservation02:original4217, protected worktree22223,
  old89 UE packages, prior90 IR/fixtures and all historical reports/evidence preserved.
  Exactly4 new presentation packages; new narrative IR/package0.
- Selective staged paths, raw evidence equality, ZIP member hashes, deterministic
  source bytes, LFS pointers and session-log prefix are recorded in staged_review.json.

## Evidence and remaining work

Each execution is archived under evidence/character1 with raw logs and source hashes.
Final PNGs are unedited copies of rendered UE captures. Historical failures, reports,
packages and IR/fixtures remain unchanged. The entry index/worktree were clean.

Full UE203 and independent1500 checks over1475 snapshots from Depth1 remain reused
historical evidence. They are not described as new Character1 full-suite executions.
The new focused runtime checks are listed above. Source gameplay/native oracles were
not rerun. Narrative frontier remains `before:shop_actions`.

The faceted face/hair, visible breastplate/undercoat intersections, simplified joints and procedural gait require
an art/animation pass before production. Existing small collision bounds remain;
this task does not redefine movement or scale the entire world around the model.
Character appearance can be revised when the corresponding manuscript/design is final.
No push, packaged release, Chapter3, shop transaction, or new narrative implementation.

![Actual UE character inspection camera](evidence/character1/CharacterFront.png)

![Actual ordinary field camera](evidence/character1/MarketFront.png)
