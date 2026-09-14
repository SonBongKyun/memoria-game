# Illustrated presentation pass 1 — Chapter 2

Status: verified. The local checkpoint is the commit containing this report.
Source progression remains Phase 1O,
`before:shop_actions`. This is a presentation milestone, not a new narrative phase.
Entry checkpoint: `228aeffe963ea945548dce8d27d02346bc3df24d`.

## Visible changes

- Eight original story illustrations now follow the imported Chapter 2 arrival,
  Malet encounter, sword extraction and reward dialogue. All CG references come
  from the existing typed narrative assets; no dialogue or narrative IR was rewritten.
- Eleven original portraits cover Arrel, Elia and Malet's authored expression keys.
  The existing Malet neutral shop texture is reused; eighteen textures are new.
- Full scene images preserve aspect ratio. A dark, gold edged lower dialogue panel
  separates the speaker, body and input hint. The active portrait follows the
  authored VN side; narration and choice screens clear it.
- Choices support the existing keyboard/controller input and real mouse buttons.
  Buttons retain original choice IDs after filtering; repeated redraw preserves
  selection, pause blocks choice clicks, and an unknown art reference clears the
  previous image instead of displaying an unrelated picture.
- Exploration now has a compact location and movement/interaction prompt. Its
  terrain and moving characters remain the existing foundation placeholders.

## Source and behavior boundary

[Artwork provenance](evidence/presentation1/artwork_sources.json) records the exact
source paths, byte sizes, SHA-256 and portrait map keys. All nineteen source images
were visually inspected. The importer refuses existing destination packages and
imports only the new presentation textures. Runtime loads Unreal packages by asset path;
it does not load source PNGs, JSON or Godot scripts.

CG persists to the next authored CG within each current linear imported sequence.
The authored Malet cellar is the presentation fallback for Malet reaction/reward
sequences without an opening CG. This is a native presentation composition, not a
claim to reproduce every Godot stage animation, CG fade, sound or text effect.
Future narrative branching must account for the actually visited CG history.

No shop transaction, shop-close callback, Chapter 3, battle, save schema, narrative
interpreter effect, player memory domain, run ownership, reward or travel contract
was added or changed. Existing packages and historical reports/evidence are retained.
The original checkout was read only. No push.

## Validation and retained failures

- `build01`: failed on a local variable shadowing UUserWidget::Padding; fixed locally.
- `build02`: passed after correcting the variable and adding visual tests.
- `visual01`: interaction passed, coverage failed because asynchronously compiling
  editor textures returned placeholder dimensions. Runtime presentation now finishes
  editor texture compilation before UMG captures brush dimensions. Cooked runtime
  has no editor compiler dependency.
- `visual02`: 2/2 passed for all imported art references, redraw/run isolation,
  filtered choice identity, mouse submission, pause and missing-image cleanup.
- `campaign01`: 4/4 rendered cases passed; screenshots reviewed. A subsequent font
  and choice alignment refinement was made before final full regression.
- `automation01`: build failed because alignment belongs to UButtonSlot, not UButton.
  The slot API was corrected; no automation ran in that attempt.
- `automation02`: full rendered UE 203/203 passed; independent source/state
  comparison passed 1,500 checks over 1,475 snapshots. Raw execution retained in
  eighteen size-bounded ZIP archives, each member hashed and reread after writing.
- `visual03`: 2/2 passed on the font/alignment refinement.
- Final screenshot review found the left portrait overlapping the input hint.
  One additional UMG placement line moves that hint alongside the body. This is
  the only runtime change after `automation02`; its full 203 result is reused for
  unchanged behavior, not described as a new full run after the hint fix. Targeted
  rendered checks passed on the final layout: `shop01` 12/12, `campaign02` 4/4,
  `visual04` 2/2. These are separate executions, not one new full-suite run.
- Initial staged checking found trailing whitespace in the untouched import log.
  Its exact bytes and the failed check are retained in `import_execution.zip`;
  only that new raw log entry was removed from the index. No log text was edited.
- Host tools: 96/96 passed. Static structure/integrity: 64 checks passed.
- `preservation01` and `preservation02`: original 4,217 files, 21,858 protected worktree files, previous
  23 packages, all 90 prior IR/fixture files and historical evidence preserved;
  exactly 18 new presentation textures, no new narrative IR.

Each UE attempt has its raw logs/report archived with per-member byte hashes.
The source oracle/native domain results from Phase 1O are retained as historical
results, not described as fresh executions for this presentation pass.

## Next visual work

Build a small Verdan exploration scene around the verified movement/interact path:
original character sprites, floor/wall layers, light and foreground occlusion. Keep
walkable geometry readable and physical interaction distances unchanged. Continue
with battle presentation only when its Unreal gameplay loop is connected.

## Final captures and checkpoint checks

Unmodified engine captures: [arrival](evidence/presentation1/arrival_final.png),
[choices](evidence/presentation1/choices_final.png),
[Malet](evidence/presentation1/malet_final.png),
[sword memory](evidence/presentation1/sword_memory_final.png),
[reward](evidence/presentation1/reward_final.png).
Earlier review captures, including the hint overlap, remain alongside the corrected
captures. `reviewed_final_screenshots.json` identifies each final capture's execution.

Static02 passed 64 checks including original 4,217 byte preservation. Final staging
checks compare every raw evidence blob with its local bytes and each new LFS package
pointer with the actual .uasset SHA-256. The detailed result is retained in
`evidence/presentation1/staged_verification.json`; working/staged diff checks must
pass before the local checkpoint. Push and source-frontier expansion are absent.
