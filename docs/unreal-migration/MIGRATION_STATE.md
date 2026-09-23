# Migration handoff — Verdan art and Windows development preview

S292: the Alley Rat's legacy hound art is supplemented by a provisional, readable rat portrait. Original art and all 101 pre-existing UE packages remain byte-preserved. New battle art imports as exactly one additional texture. The rendered battle suite passed 47/47; a Win64 Development package cooked successfully, contains the new art and Verdan map, and loaded Verdan without fatal diagnostics in a packaged smoke. The ZIP passed full integrity verification. See [ART_2_REPORT.md](ART_2_REPORT.md), [play instructions](PLAYABLE_SLICE.md), and [art2 evidence](evidence/art2).

The 108-chapter rewrite is not confirmed canon. The art is a provisional study. The package is an incomplete development preview; combat attacks/burn/turns, Chapter3, achievement persistence, the 15–20 minute complete slice, and Steam certification remain pending. Release target: separate GitHub prerelease `unreal-verdan-preview-s292-20260923`; the existing Godot demo release is untouched.

The S291 handoff below records the earlier pre-release state and source contracts.

# Migration handoff — Verdan revisit and battle entry

Status: S291 battle entry / guaranteed ambient flee / actual field return implemented.
Final related rendered99/99 passed on UE5.8.2 / CL56702186.
Local uncommitted changes, no commit/push/deployment. Worktree C:/Users/jc/MemoriaMigration/foundation,
branch unreal-migration/ue58-foundation, HEAD969d7aad682b14d03b91885a1134688cbb91ad66.

[Current report](BATTLE_ENTRY_1_REPORT.md), [play instructions](PLAYABLE_SLICE.md),
[archive](ARCHIVE_1_REPORT.md),
[checkpoint](CHECKPOINT_1_REPORT.md), [transactions](SHOP_TRANSACTIONS_1_REPORT.md),
[control tuning](PLAYFEEL_1_REPORT.md).

- The108-chapter manuscript is being rebuilt. Draft text and illustrations remain provisional.
  Notion was reviewed read-only. Unapproved manuscript drafts and the executable Godot contract remain distinct; do not silently change gameplay or promote draft material to canon.
- Closed checkpoint -> Load checkpoint -> Return to Verdan performs actual OpenLevel.
  Completed revisits enable source distance encounters: 32px tiles, 60–100 threshold, 72% warning.
  First visit remains encounter-free. Source threshold/pool/modifier RNG draw order retained.
- Battle entry applies source HP growth/focus/objective/modifier/stat changes within Normal/NG0,
  current unequipped/neutral ambient scope. TotalBattles and HighestMomentumRank are saved fields.
  Elia diary reset is observed as a request; the complete diary subsystem is not ported.
- Entry presentation uses original market/Arrel/Elia art and a provisional Market Thief illustration.
  Six new textures, old95 untouched. Alley Rat's source hound mapping is legacy/provisional.
  Generated image and full prompt/provenance preserved in Unreal/ArtSource/BattleEntry.
- Only flee is actionable: source guaranteed ambient escape, 0.3s cleanup, actual Verdan reentry
  at source (128,288). No reward, save overwrite or memory grant. Battle is modal; archive and Malet
  cannot open. Held repeat keys across OpenLevel are consumed. Run/world replacement cancels timers.
- S287 movement/stride/camera, S288 transactions, S289 disk checkpoint, S290 read-only archive and
  market graphics remain. Continue starts at the supported closed checkpoint. Native save storage
  is separate from Godot; automated tests use isolated leaves only.
- Fresh source46 and independent recheck46 PASS, native neutral source43 comparisons PASS.
  Three non-neutral approaches are source-only. Final UE99 = BattleEntry47 + regression49 +
  separate-process Continue3; exact IDs and source hashes in battle1/validated_coverage.json.
  Host114/static76 PASS. Full280 Memoria registry, packaging and timed human play were not run.
- Entry baseline22599 and before_changes.zip retain prior uncommitted work. Final preservation
  PASS is in battle1/preservation_final.json (original4217, oldUE95, protected22579, prior evidence/fixtures/HEAD). Final documentation-only update rechecked in preservation_final02.json.
- Engine PreInit smoke CHECK15 remains in all three executions, also present in S289/S290.
  Cause and benignness unresolved; separate from selected99 PASS in engine_startup_diagnostics.json.
- Next bounded work: source-derived attack/guard/enemy-turn and combat memory-burn loop with
  visible archive/world consequences. Win/loss/rewards/companion skills, Chapter3 travel,
  achievement persistence and human timed15–20minute integrated play remain outstanding.
  This is battle-entry completion, not complete combat or a finished15–20minute game slice.
