# S290 — Memory archive and Verdan graphics

Date: 2026-09-22. Workspace: C:/Users/jc/MemoriaMigration/foundation.
Local changes only. Original Godot/manuscripts/art and prior S287–S289 work are preserved.
The 108-chapter manuscript rebuild and all reused illustrations remain provisional.

## Player-visible work

Tab/M opens a read-only memory archive during Verdan exploration, Malet's shop,
and the supported closed-shop checkpoint. Authored dialogue retains input ownership.
All owned memories remain visible in source order, including burned, residue and faded entries.
Six filters show all memories or one original grade. Selecting a card presents its original
localized title, description and story effect. The UI consistently uses the source card's state
precedence; the legacy source detail panel incorrectly labels unburned faded/eroding entries INTACT.
This is a documented presentation correction, without changing the memory domain.
No burn command, synthesis, loans or cumulative loss gauge is introduced.

The archive uses a restrained ink/teal/gold panel, dimmed burned cards, a scrollable list and
selected-memory detail, and two already imported provisional chapter illustrations.
No artwork package or authored text was regenerated. Generic market art is contextual atmosphere,
not a newly established depiction of each memory. The existing sword illustration is used only
for identity_first_sword.

Tab/M/Escape/gamepad B closes the archive. Up/down selects, left/right filters; mouse buttons
use the same read-only model. Physical movement stops while reading. On close the current
shop/checkpoint/exploration view returns. Consumed Slate keys remain suppressed until physical
release, preventing held Enter/E/Escape from triggering a transaction, checkpoint load or shop
close on return. Run replacement and controller teardown release archive observers/delegates.

Verdan now has a worn central slab path and cross-course, flat edge drainage and recessed grates,
small damp/dirt accents, fuller stall shelves and parcels, and canopy seams/valance and braces.
Lighting uses a softer directional shadow, cooler ambient fill and warmer lanterns.
The seven physical collision bodies, four lantern lights, original route and Malet reach,
S287 walk speed/stride/camera, and story/trade/save semantics are retained.

## Executed source scope

export_archive_oracle.py executes the actual original archive list/filter/card/detail methods
in an isolated Godot harness with the real MemoryManager. Eight fixtures include English/Korean,
real sensory/relational burns, synthetic faded/eroding display states, grade filters and empty state.
source02 and independent source_check03 passed; exact harness ZIP, logs and source hashes remain.
The harness excludes archive artwork/rewrite/carry/track summaries and unrelated profile endpoints.
No original source writes occur. Newly generated localization constants are derived from execution.

## Validation

Final rendered regression04:49/49 passed (archive10, visual3, checkpoint6, transaction14, shop12, campaign4).
UE5.8.2 build passed. Archive02's intermediate10/10 is preserved separately. The final captures were
visually reviewed: card number/title overlap fixed; bright plain slabs replaced with the existing
paving texture and subtle tonal variation. No test assertion or tolerance was relaxed.
Regression03's member-shadowing compile error is retained before its local-variable rename.
Source01 failed before execution on unsupported method extraction; source02 fixed the isolated extractor.
Separate-process process05:3/3 passed (prior-process disk read, actual Continue startup, missing-save startup).
Final unique related coverage is52/52 in evidence/archive1/validated_coverage.json. Both final execution
archives contain native sources matching the final working bytes. This is not a full233 regression run.
The first build (visual01) failed on UE5.8 checked formatting and button focus initialization APIs;
its sources and logs are preserved before fixes. Review additionally identified and fixed
Slate repeated-close and held-key leakage, with actual Slate input regression coverage added.
Host checks:108 passed. Static checks:64 passed. Preservation check01 passed original4217,
protected pre-existing22529, existing UE95 packages, prior evidence/fixtures, and unchanged HEAD.

## Remaining work

The supported end remains before:chapter_transition_delay. Revisit battle, battle memory burn,
Chapter3 travel and achievement persistence remain future work. This session does not complete
or certify a human timed15–20minute integrated playthrough. The full Memoria registry was not run.

## Evidence and rollback

See evidence/archive1/ for entry baseline22545 and before_changes.zip, source runs and exact
runtime execution archives. Roll back only S290's authorized files/new-file set against this
entry backup; do not reset the earlier uncommitted S287–S289 changes. No push or commit.


## Final captures

[Market front](evidence/archive1/final_captures/MarketFront.png),
[near Malet](evidence/archive1/final_captures/NearMalet.png),
[archive after trades](evidence/archive1/final_captures/archive_after_trade.png),
[archive after checkpoint restore](evidence/archive1/final_captures/archive_restored.png).
All seven files in final_captures are unmodified native captures from regression04/raw_execution.zip;
manifest.json records member names and SHA256 values. Paving remains intentionally subtle at play scale.


## Final preservation and engine diagnostic scope

preservation_final.json passed all checks: original4217, protected pre-existing22529,
existingUE95, no new packages, historical evidence/fixtures, original manifest and unchanged HEAD.
static_final passed64, host02.log passed108, and git diff --check passed.

The engine prints15 `LogAutomationTest: Error: Condition failed` messages during PreInit startup smoke,
before selected automation begins. The same15 messages exist in the retained S289 automation04 log.
UE's LowLevelTestAdapter CHECK macro and LaunchEngineLoop RunSmokeTests path explain the stage;
the specific smoke cases and cause remain unidentified. They are not classified as harmless.
This report claims52 selected regression tests passed, not an entirely clean engine startup or full233 suite.
See engine_startup_diagnostics.json for code pointers and the prior archive comparison.
