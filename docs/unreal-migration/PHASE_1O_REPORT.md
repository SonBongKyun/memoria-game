# Phase 1O — Malet first shop screen

Status: verified for the bounded first shop screen. The local checkpoint is the commit containing this report. User authorized Phase 1O and staged application of original graphics in this task on 2026-09-14 KST.
Start checkpoint: `ef011256f9aebe2902f69d19cdddeeff951cffe1`.
Worktree: `C:\Users\jc\MemoriaMigration\foundation`, branch `unreal-migration/ue58-foundation`.

## Scope

The real existing canonical VN, memory payments, native map travel, physical walk,
Malet deal, reward callback and potion2/antidote1/firebomb1 route now opens the first
shop screen in the same run. The initial sell list uses the actual Player Memory
availability query, source prices and existing localized catalog text. Source stock
contains two offers but is not acquired into the player's memory catalog.

The UI displays original Malet portrait and shop backdrop from two newly imported
Unreal texture packages. First open has no selection; arrows and row clicks preview
memory descriptions without executing a transaction. Confirm and Back remain inert
at this bounded screen. The shop owns transient presentation state and is discarded
on run replacement or world cleanup without invoking source shop_closed.

Source open_shop requests audio ui_open, achievement check_grains and tutorial
first_shop, in that order. These are recorded requests only: their handlers, hint
persistence, transactions, close callback, chapter3, autosave and profile mutation
are outside this phase. This is not a complete shop, complete chapter, or complete
campaign. The extra English presentation headings are UI chrome, not narrative IR.
Only the initial sell view is exposed; other source tabs are not exposed. The oracle
observes one source shop_closed listener; Unreal exposes no close action/listener
at this frontier, so cleanup cannot accidentally invoke chapter3.

## Source and verification

The isolated Godot oracle executes the exact stock constructor, open_shop,
default sell-list filter/prices/localization, detail clearing and visibility methods.
Visual endpoints collect rows; audio/tutorial/achievement handlers are inert request
sinks. Ten fixtures include en/ko, burned/faded/collateral/core/empty states, nonzero
Grains and repeated opening. Original checkout remains read-only.

The Phase 1N item-complete synchronous snapshot and full ordered prefix remain
observable before shop entry. New exact shop events follow it. Existing full Run,
Player Memory, derived observations and World checks remain, with only the current
frontier advancing from before:shop_open to before:shop_actions. Save schema remains1;
no shop cursor or stock is persisted. Independent binary restoration must not reopen
the shop or replay grants.

## Final evidence

- Fresh UE5.8.2 build and rendered full automation: **203/203** (previous191 + shop12).
  [Exact index](evidence/phase1o/automation03/automation_index.json).
- Independent exact trace, source projection, full-state and binary comparisons:
  **1,500 PASS**, inspecting1,475 snapshots. [Acceptance](evidence/phase1o/acceptance01/acceptance.json).
- Fresh Godot shop oracle10, host96, native memory51 + CTest1, static63 PASS.
- Actual binary save13,121 bytes, schema1; independent restoration preserves every
  section and emits zero inventory signals, toast requests or item grants. Transient
  shop state clears on same-ID restore, new run and real world travel.
- Original protected4,217 files, prior21 UE packages,88 IR/fixture files and17,133
  protected worktree baseline files match. New narrative IR/packages0; new textures2.
  [Preservation](evidence/phase1o/preservation02.json).
- [Final visual review](evidence/phase1o/visual_review03.json) covers real English
  rendering and keyboard selection. KO source data is verified; full Korean visual
  QA, other aspect ratios and packaged-build QA are not claimed.
- [Raw logs](evidence/phase1o/full_logs.zip) and per-execution raw-file manifests
  preserve all attempts. Generated header and fixtures are pinned to LF; evidence
  is byte-preserved. [Staged review](evidence/phase1o/staged_review.json) verifies
  exact raw evidence blobs and the two new LFS package payload hashes.

## Attempts and evidence reuse

Source01 used an invalid test memory ID and timed out; source02 executed10 cases but
its attestation postprocessing used the wrong achievement script path. Both attempts
are preserved. Source05 reran check-only and matched both fixtures and generated
header exactly. Build01 exposed two installed-API differences, corrected without
changing source behavior. Automation01 passed201/203; empty/core-only test snapshots
incorrectly retained definitions for removed owned memories. The test setup was fixed
to honor the existing definition/owned invariant; RestoreRun and source expected
outputs were not weakened. Automation02 passed12/12 shop tests, followed by final
full automation03 passing203/203. Gray default panels were replaced with dark
translucent panels and thin outlines after direct screenshot inspection.

Host01's four failures concerned phase identity arithmetic after adding12 tests;
exact historical sets are still asserted after subtracting the new shop group.
Host02/03 passed96/96. All failures remain in [attempt history](evidence/phase1o/attempts.json).
One existing r.MotionVectorSimulation warning and engine-startup Condition diagnostics
are retained in raw logs, separately from zero final Memoria test failures.

Earlier Godot C-N oracle outputs and original Godot smoke results remain historical;
this phase does not claim to have rerun all original Godot smokes. It freshly executed
the O oracle, all203 UE tests, native checks and the independent1500 comparisons.
The extended validator also rechecked the original N execution evidence: all1413
old checks still pass in its unchanged pre-shop mode. This is an evidence recheck,
not a fresh standalone Phase1N runtime execution. Historical N reports and its CRLF
recovery evidence were not edited, and the recovery helper was not rerun.

## Next development direction

The validated narrative, ownership, memory and save foundations are sufficient to
apply graphics to connected screens incrementally. This phase starts that work with
the real shop. Next: a reviewed memory-sale transaction with correct Grains/memory
state, then the post-shop continuation contract. In parallel with those future
functional scopes, carry the source portraits and backgrounds into dialogue and
exploration. Do not infer full-shop/campaign completion from this first screen.

No push. No purchase/sale, source close handler or chapter3 implementation.
