# S289 — Verdan checkpoint 1

Date: 2026-09-21. Workspace: C:/Users/jc/MemoriaMigration/foundation.
Local changes only; no commit, push, package authoring, manuscript edit or legacy save import.
The 108-chapter rebuild remains provisional. S287 movement/stride/camera and S288 transaction contracts are retained.

## Implemented boundary

Closing Malet's shop commits ch2_complete / chapter3, then writes the current Verdan
checkpoint before the observed achievement requests and before the deferred chapter-transition timer.
The file contains the run, acquired memory definitions, owned/burned/residue state, item inventory
and recent order, Grains, world cognition, and the player's source coordinates.
It is a native Unreal format; it never reads or overwrites Godot autosave/slots.

Storage: Unreal/Memoria/Saved/SaveGames/MemoriaSlice/autosave.memoria.json.
The envelope version, checksum, maximum size, DTO schema, current starting-memory catalog/revision,
supported destination/boundary, chapter flag, coordinates, and both domain snapshots are checked.
Validation occurs before live restoration. A verified temporary file replaces the destination with
Windows MoveFileExW; the prior valid primary is preserved as .bak. Corrupt primaries never rotate
over a good backup. A valid backup can recover a corrupt or missing primary; repair status is shown.
This is corruption recovery, not encryption/authentication or a power-loss durability certification.

The completion screen reports the actual save outcome and offers Load checkpoint / Save checkpoint again.
The launcher accepts -Continue and opens L_VerdanHost with -MemoriaContinue.
A successful load returns to the closed-shop development boundary without replaying arrival, rewards,
purchases, burns or achievement requests. Missing/incompatible saves preserve the current run;
startup without a valid checkpoint offers retry or an explicit new slice.
The continuation presentation releases its context on world cleanup.

Automation/commandlet sessions do not access production storage. Tests explicitly select an alphanumeric
leaf below Saved/Validation/CheckpointTests. The startup test argument is development-test only and
uses the same restricted resolver. Native save creation and consumer processes are separate.

## Source-derived evidence

export_checkpoint_oracle.py extracts and executes the original current-version save_game/load_game,
autosave_on_chapter_transition/autosave, backup, parser/recovery and source-scene resolver, alongside
the existing exact shop/close path and real MemoryManager. Six deterministic fixtures capture:
close-save timing, second-save backup, corrupt-primary recovery, load, invalid destination before mutation,
and menu/synthetic/invalid-slot guards. source02 and source_check03 both passed.

The harness isolates the profile and output path, uses inert profile/UI/diary/hints endpoints, an identity
current-version migration function and a bounded GameManager import collector. It does not certify
the full original profile, old-version migration, diary/hints or a Godot-to-Unreal save importer.
Original source and prior fixtures remain untouched.

Native source parity asserts chapter/flags, inventory, balance, ordered memory identity, grades, burn power,
authored titles/descriptions, burn/residue/fade/erosion state and burn history against the executed source.
Independent disk round trips additionally compare the complete native run/player/world observations,
including acquired definitions and nondefault position, with no inventory/toast replay.

## Validation

- UE 5.8.2 build passed.
- Final checkpoint automation04: 6/6 rendered tests passed, including real physical sale/buy/close/load.
- Separate process05: 3/3 passed (disk read, actual GameMode -Continue startup and missing-save startup).\n- Existing rendered regressions: transactions06 14/14, shop07 12/12, campaign08 4/4 passed.\n- Final unique related coverage39/39 is in evidence/checkpoint1/validated_coverage.json; archived runtime\n  sources in all five final executions match the final working bytes.
- Final host104 passed; static63 and launcher PowerShell syntax passed. Exact identities and execution limits accompany the coverage file.
- before_changes.zip and entering_baseline.json preserve this session's entry state.
- source01 duplicate-constant harness failure, build01 TObjectPtr errors and automation02 Windows macro/
  JSON/API type errors are retained. automation03 first successful6 and automation04 final6 remain separate.
- Final raw archives include runtime/test source, engine logs, state JSON, actual checkpoint/backup bytes
  and captures. No assertion or tolerance was relaxed to obtain a pass.

## Remaining work

Continue ends at before:chapter_transition_delay. Chapter3 travel, achievement persistence,
manual slots, other save boundaries, battle/revisit/archive feedback and a human timed15–20minute
integrated playthrough are not completed by this change. Full223 Memoria regression was not run.
The earlier S288 full217 timeout/NullRHI limits remain historical and are not recast as a pass.

## Screens

[Saved](evidence/checkpoint1/final_captures/saved.png),
[loaded](evidence/checkpoint1/final_captures/loaded.png),
[fresh process Continue](evidence/checkpoint1/final_captures/continue.png),
[missing save](evidence/checkpoint1/final_captures/missing.png).
These are unmodified native captures copied from the archived final executions; hashes are in final_captures/manifest.json.

## Preservation and rollback

Original4217 files, protected pre-existing worktree22469 files, existing UE95 packages, prior fixtures/evidence
and HEAD are unchanged; no new packages or IR. The preservation results list every authorized prior-file edit.
All earlier uncommitted S287/S288 work was hashed before this session. The raw .gitattributes prefix was preserved.
To roll back S289 specifically, use before_changes.zip and entering_baseline.json to restore only its authorized
modified files and remove only its listed new files; do not reset all uncommitted work.
