# Migration handoff — S300 Elia companion in Verdan (Claude lane, 2026-09-26)

- User-assigned continuation of the story work, on top of `351810e4` (Claude lane).
- **Narrative schema (review item).** Field rows may now carry the legacy `dialogue_manager.gd` substitution: `requires_memory`, `burned_text` and `burned_text_ko` (Text group), and `burned_portrait` (Presentation). They are Field-only. The Python extractor and the C++ importer reject them on VN rows and choices.
  - `requires_memory` is not a gate: as in the source, the row always shows, and the line and portrait swap once the memory is in the burned list.
  - All 14 prior IR files are byte-identical under `--check`. The generated types are regenerated.
- **Cohort.** A Field group may name its source file. The C++ cohort table carries the file and its chapter, and the importer derives the source path from the definition id. Imported:
  - `elia_ch2_talk` (chapter 2, position 5)
  - `elia_song_burned` and `elia_sword_burned` (`chapter1_dialogue.json`, positions 14 and 15)
  - `elia_face_sad`, plus Elia's four source field sprites (additive `-run=MemoriaVerdanAssets -AddElia`)
  - The Verdan market splash is reused from the battle art.
- **Companion.** `AMemoriaEliaCompanion` follows Arrel's trail with the `companion.gd` constants (formation 48, speed 112, arrival 7, trail sample 8/96, warp 310, accel 900, sprint 1.48). It is query-only and never blocks.
  - She is talked to only when Arrel faces her. This mirrors the player's RayCast, so she never steals E from Malet or a story point.
  - `InteractWithElia` follows the source order:
    1. The first burned and unheard reaction (song, then sword), which sets `burn_reaction_heard_<group>`.
    2. The first talk `elia_ch2_talk`. The talk is cached per loaded Verdan, and `talked_Elia_elia_ch2_talk` is set when the dialogue ends.
    3. The one-row repeat line "This market smells like rust and regret."
  - Elia rows without their own CG show the Verdan market splash instead of Malet's cellar.
- **Tests.**
  - `Memoria.VerdanStory.SourceTable` now also checks Elia's reactions, key, flag, repeat line and companion constants against the parsed source.
  - New `Memoria.VerdanStory.BurnedTextSubstitution` covers the intact and burned cases in en and ko, and rows without the keys.
  - The rendered journey adds a real facing key press and three E presses: sword reaction; first talk, where the paid route burned the food, so row 0 swaps and row 2 stays intact; and the repeat line.
  - `MemoriaVisual.VerdanExploration` hides the companion only for Arrel's close-up review captures.
- **Results** (UE 5.8.2 rendered, final tree): full `Memoria.` **388/388**, `MemoriaVisual.` 3/3, `MemoriaCheckpointProcess.` 3/3. `generate_narrative_types.py --check`, `narrative_test_fixtures.py --check` and the Verdan fixture `--check` all pass. The Python host tools show 13 passing and the same 3 historical count failures as before this change.

# Migration handoff — S299 Sump Ledger side quest (Claude lane, 2026-09-26)

- User-assigned continuation of the story work, on top of `46f8bcf6` (Claude lane).
- **Content.** The three groups the source actually requests (`sq_sump_ledger_start` 4, `sq_sump_ledger_found` 2 and `sq_sump_ledger_return` 2) were imported through the attested pipeline, along with three quest CGs. The authored `sq_sump_ledger_burn` is never requested by `verdan_market.gd`, and the "return or burn" step has no burn branch in the source, so it stays out.
- **Rules**, from `side_quest.gd` and `_setup_side_quests`, in source order:
  - Available from chapter 3, while not yet started.
  - The trader exists at entry while the quest is available or active. Talking to them starts the quest, reminds while the ledger is missing, or returns it once found.
  - The ledger is placed only if the quest was already active when Verdan was entered. So, as in the source, it appears after a re-entry.
  - Finding it sets `sq_sump_ledger_found`, plays `ui_select` and opens the found group.
  - Returning it sets `sq_sump_ledger_done` and grants the rewards before the return group: 40 Grains, 1 Hi-Potion, and the memory `sq_debt_ash` "Debt Written in Ash" (Grade3, power 55).
  - Source toasts appear in the exploration status for 4 s of exploration: "Find the ledger in the Sump.", "+40 Grains", "+1 Hi-Potion", "Quest Complete: The Sump Ledger".
  - A quest tracker line shows the current source step (en/ko).
- **Deviations.** Walk-in areas become E/A points, as in S298. Source toasts use the existing status box instead of a toast widget. The status box now grows with its lines, and notices are scoped to the world that raised them because world time restarts on each Verdan load.
- **Tests.**
  - `Memoria.VerdanStory.SourceTable` also checks the quest against the parsed Godot source: steps and descriptions (en/ko), chapter gate, rewards, memory fields, areas, requested groups and row counts. It also runs the trader state machine over flags.
  - The rendered journey adds start, reminder, the source re-entry (an explicit `ReturnFromAmbientBattle` fixture), find and return, with reward assertions. Captures: `QuestTraderStart`, `QuestReminder`, `QuestLedgerFound`, `QuestReturn`, `QuestComplete`.
- **Results** (UE 5.8.2 rendered, final tree): full `Memoria.` **387/387**, `MemoriaVisual.` 3/3, `MemoriaCheckpointProcess.` 3/3, `Memoria.VerdanStory.` 1/1. The Godot trigger and quest fixture passes `--check`.

# Migration handoff — S298 Verdan story beats (Claude lane, 2026-09-26)

- Assigned by the user directly in the Claude chat. The request was to keep upgrading from the story and the Godot work. Claude lane, on top of `ffe5199a`.
- **Content.** The five remaining plain-row Ch2 exploration groups from `scenes/maps/verdan_market.gd _setup_exploration_events` were added to the reviewed Field cohort. They went through `narrative_ir.py`, the Godot oracle check and `import_narrative.py`, each with the first import, an unchanged reimport and a reload check.

  | Group | Rows | Asset |
  |---|---|---|
  | `verdan_market_walk` | 8 | `DA_Field_VerdanMarketWalk` |
  | `verdan_old_burner` | 11 | `DA_Field_VerdanOldBurner` |
  | `malet_backstory` | 12 | `DA_Field_MaletBackstory` |
  | `elia_sump_concern` | 8 | `DA_Field_EliaSumpConcern` |
  | `sump_atmosphere` | 6 | `DA_Field_SumpAtmosphere` |

  `elia_ch2_talk` stays out, because burned-text substitution is a later cohort. The Sump Ledger side quest also stays out.
- **Art.** Six source CGs and `malet_face_deal_accepted` were imported. `MemoriaDialogueAssets` now only adds missing packages and never overwrites; no existing package changed.
- **Rules, as in the source:**
  - Beats are armed when Verdan is entered. A beat whose flag is already set is skipped.
  - `malet_backstory` is armed only if `malet_deal_accepted` held on entry.
  - Starting a beat sets its one-time flag, then opens the Field. It only starts in exploration outside battle, and a seen beat never repeats.
- **Deviation (intentional).** Godot fires a beat when the player body enters its Area2D. The Unreal courtyard is the development layout (Malet sits at `DevelopmentLocation`), so the source rects would land on arbitrary pavement and on existing journey and edge-capture routes. Each beat is instead a glowing point Arrel approaches and activates with E/A, like Malet. Godot also highlights triggers on approach (`update_trigger_approach_glow`). The source rects are kept in `MemoriaVerdanStory` and checked against the Godot source.
- **Tests.**
  - New `Memoria.VerdanStory.SourceTable`: `export_verdan_story_triggers.py` parses the Godot trigger table (`--check` runs in the validator), and the test checks the C++ table, the imported row counts and the placement constraints.
  - `Memoria.BattleCore.RenderedJourney` ends with the Old Burner and Malet's backstory on the recovered revisit. It uses physical E and Enter and teleports only under the archive modal, so the encounter meter never counts the move. Captures: `StoryOldBurner`, `StoryMaletBackstory`, `StoryMaletSeventeenEyes`, `StoryReturned`.
  - `MemoriaVisual.ArtworkCoverage` now counts 26 sources and covers the new assets.
- **Results** (UE 5.8.2 rendered, final tree): full `Memoria.` **387/387**, with 0 failures and no material fallback warning. Also `MemoriaVisual.` 3/3 and `Memoria.VerdanStory.` 1/1.
- **S297 fix.** The Verdan ash and ember motes were instanced static meshes, but `M_SoftLight` lacks the instanced-mesh usage flag. The log warned "Default Material will be used in game", and the S297 squares were that fallback. The motes are now plain components sharing one dynamic material per group: the warning is gone, and the rendered capture shows soft motes. The shared material asset was not edited.
- **Carry-over from S297:** full rendered `Memoria.` 386/386 and `MemoriaCheckpointProcess.` 3/3 on `ffe5199a`.

# Migration handoff — S297 Witness path, battle HUD, Verdan field life (Claude lane, 2026-09-26)

- Assigned by the user directly in the Claude chat after Codex S296. Claude lane `SonBongKyun/memoria-unreal-claude`, based on `3130ec5a`.
- **Witness (gameplay).** `FMemoriaBattleModel` now plays the source `player_witness` and `_use_witness_ink` for ambient, non-boss enemies:
  - Each reading advances the requirement (2, or 3 for a void beast; 2 again with `listened_to_humming`/`elia_stays`), records the enemy (scan), adds Limit 8/10 and Momentum 10/8, and guards the next blow.
  - Finishing the reading releases the enemy: no damage, flag `witnessed_<key>`, released log.
  - Victory adds the source preservation bonus (12 void / 8 released / 6 insight) with Field Focus +1, the record bonus +1 when scanned, and grade +10.
  - `scan_first`, `witness_echo` and `no_items` are now supported objectives with rewards.
  - Void drops now include `witness_ink`, as in the source table. The S296 port had left it out.
  - Boss insight, the `quiet_focus` anchor passive and the `enemies_witnessed` stat stay out of this slice.
- **Oracle.** `export_battle_core_oracle.py` adds `player_witness`/`_use_witness_ink`, witness snapshot fields, the source witness lines, and 10 cases × en/ko: 94 cases, `--check` byte-exact. The native test also compares the reward breakdown, the witness flags and the localized witness logs. It seeds gauges from the source start snapshot, which already contains the Field Focus and humming openings.
- **Battle HUD.**
  - Six action slots: ATTACK, BURN, WITNESS n/m, GUARD, ITEM, FLEE. Attack and Burn keep their slots and Flee is still the wrap-left target.
  - BREAK/MOMENTUM/LIMIT bars; witness pips and the echo line in the enemy plate.
  - Burn and witness telegraphs get their own band, and damage numbers are hidden under it.
  - A victory card shows the grade and reward breakdown. On a release it adds the source aftermath line, and the enemy art fades to archive ink with rising motes.
  - The log panel no longer runs under the enemy HP bar.
  - Witness plays `rising_tone`, and release plays `memory_add`.
- **Verdan field life.** Presentation only, with no collision, input or story change: 56 drifting ash flakes, 18 lantern embers and 5 slowly drifting mist patches, reusing `M_SoftLight` (no new assets). The controls hint in the shared SliceHost was left as is.
- **Rendered journey.** `Memoria.BattleCore.RenderedJourney` now performs a physical WITNESS read before the burn. It adds the captures `CombatWitnessCue`, `CombatWitnessRead` and `CombatWitnessReleaseFixture` (a presentation-only fixture). The captures and the Verdan `PlayFeel1` frames were inspected. The embers are outside every capture frame, so they are not visually confirmed.
- **Results** (UE 5.8.2, rendered, this lane):
  - `Memoria.BattleCore.` 96/96
  - `MemoriaVisual.` 3/3
  - Godot oracle `--check` 94/94
  - The full `Memoria.` registry was still running when this commit was made; see `claude-handoff.md` for its result.

# Migration handoff — S296 battle core (2026-09-26)

- Codex implemented the user-authorized Git repair, battle core, UI/audio and integrated journey in `codex/unreal-battle-core-20260926`, based on `4adbbfac`. No Claude/foundation source edits and no remote push.
- Playable: Attack, all five burn grades, Guard, Potion/Antidote/Firebomb, enemy turns/statuses, BREAK/momentum, Last Stand, supported objective and source grade/streak rewards, victory return and burned-memory archive state. Replaced runs/worlds cancel all pending actions.
- Defeat differs from full Godot: choose actual checkpoint restore or full-HP Verdan recovery retaining burned memories. Unsupported objectives explicitly award nothing. Witness, Limit spending, companion actions/echoes/stances and bosses remain outside this bounded slice.
- Fresh source oracle `--check` 74/74 and UE 5.8.2 rendered battle tests 76/76 PASS. Full rendered `Memoria.` regression **366/366 PASS**, 0 failures/fatal diagnostics and one existing engine warning. Human feel/listening is not claimed.
- Git cloud-object quarantine restored access; full `fsck --full --no-dangling` passes and pre-existing refs remain unchanged. Backup/rollback and optional repack rejection are recorded in [S296 report](BATTLE_CORE_S296.md).
- Play instructions: [current slice](PLAYABLE_SLICE.md). Next product work should begin with human battle feel/listening and the source Witness path, after accepting this checkpoint.

# Migration handoff - S295 Codex sound review (2026-09-26)

- Integrated Claude `792034ff..9c3b2033` into the Codex worktree on `codex/unreal-sound-review-20260926`. Previous `SonBongKyun/memoria-unreal-codex` / `fd76c0fe` remains preserved. Claude and foundation checkouts were not edited; no remote push.
- Fixed new shop request callback cancellation/payload lifetime and burn-drama continuation across new game/same-ID save restore. Added loop creation retry, explicit retired component cleanup and current dialogue duck restoration.
- Fresh UE 5.8.2 build + rendered **72/72** pass: Audio3, Campaign4, Visual3, BattleEntry47, ShopTransactions15. Both new regression tests failed before the fix and passed after it. Music/ambience component playback is now asserted in rendered routes. Audio warning/error count 0; fatal diagnostics 0.
- Relevant host tests **17/17** pass; audio source check25 pass. Broader pre-fix Python discovery was **104/119 pass**, with 15 existing obsolete-evidence/count/source-string test failures/errors. These tests remain unchanged and are not silently skipped. Full UE290, separate-process checkpoint tests, packaged audio and human listening were not rerun in this task.
- See [review and exact commands](SOUND_REVIEW_S295.md) and [current play instructions](PLAYABLE_SLICE.md). `BATTLE_CORE_SPEC.md` remains the next implementation proposal; combat is still entry/flee only.

# Migration handoff — first sound pass (S294, 2026-09-26)

- Worked in the Claude lane (`SonBongKyun/memoria-unreal-claude`). The lane was moved from the collaboration snapshot `fd76c0fe` to the published evidence-free `792034ff`; the code is identical and the collaboration files are carried over.
- **Sources.** `Unreal/Tools/generate_audio_sources.py` renders 25 cues from the `scripts/systems/audio_manager.gd` formulas: 22050 Hz mono 16-bit, with a fixed seed per cue. Layered combat cues are premixed with the source delays and layer dB. Output goes to `Unreal/ArtSource/Audio` (WAVs plus a manifest). `validate_unreal.py` now runs its `--check` before building.
- **Assets.** `-run=MemoriaAudioAssets` imports into `/Game/Memoria/Audio` and refuses existing packages:
  - 23 SFX (`Sfx/S_Sfx_<cue>`)
  - the original `ch2_verdan` and `battle_theme` mp3 BGM (`Music/`, 34 MB in LFS)
  - the `wind_light` and `heartbeat` loops (`Ambient/`)
- **Runtime.** `UMemoriaAudioSubsystem` is presentation only: it observes state and never mutates it.
  - Music: Verdan maps and the arrival VN play `ch2_verdan` (-5 dB, as declared in the VN metadata). Battle plays `battle` with a 0.8 s crossfade. Music persists across OpenLevel.
  - Ambience: Verdan plays `wind_light` (-10 dB), removed during battle.
  - VN/Field dialogue ducks the music to -13 dB.
  - Burning a Grade 2/1 memory plays the source burn drama: duck, 0.3 s silence, rising tone, restore, then `burn_ignite`.
  - `memory_add` plays on acquisition.
- **Cues.**
  - From the source: dialogue advance `confirm`, pressing a choice `ui_select`, choice/row focus `ui_hover`, shop requests executed in source order through the new `OnRequestRecorded` (`ui_open`, `confirm`, `memory_add`, `ui_close`), `step_stone` on each planted foot, ambient flee `flee`.
  - Additions not in the source: `battle_intro` on encounter start (defined but never played in Godot), archive open/close, and footsteps timed by the gait instead of a timer.
  - A repeat of the same cue within 50 ms is collapsed, mirroring the source's single player.
- **Tests (registry now 288):**
  - `Memoria.Audio.CatalogAssets`: asset durations equal the manifest; loop flags are correct.
  - `Memoria.Audio.Routing`: foot contacts, request→cue mapping, source volumes.
  - Audio checks in Campaign (VN duck and music, exploration music/ambience/cues), Verdan exploration (footsteps) and `BattleEntry.RenderedRevisitFlow` (every cue across the whole route, battle music, one flee, field return music).
- **Results** (rendered, UE 5.8.2, this lane): `Memoria.` 288/288 (about 27 min), `MemoriaVisual.` 3/3, `MemoriaCheckpointProcess.` 3/3. The audio device initialized on WASAPI; the logs show 0 audio warnings or errors.
- **Not yet done:**
  - No human listening pass: tests verify routing and assets, not how the sounds feel.
  - The heartbeat loop is not wired (it belongs to the battle core).
  - No volume options.
  - A packaged build was not re-verified.
- **Next.** `BATTLE_CORE_SPEC.md` holds the next Codex task (the minimal fun battle loop). Pre-existing deprecation warnings (`UMaterial::bUsedWithInstancedStaticMeshes`) in `MemoriaDepthAssetsCommandlet.cpp` will break on the next engine release.

# Migration handoff — code review follow-up (S293, 2026-09-25)

- First complete rendered registry run: `validate_unreal.py --build-and-test --rendered --test-prefix Memoria. --automation-timeout 5400`. Baseline on 371cd6e2 passed 280/280 in about 28 minutes. The old fixed 900 s automation limit could not finish it; `--automation-timeout` is new and still defaults to 900.
- The Chapter 2 slice run now starts in chapter 2, read from the imported VN/Field metadata. The source reaches Verdan after `ch1_after_forest` sets chapter 2. Before this change the run stayed at 1 until the shop close set 3. `BeginRun`/`BeginStartingMemoryRun` take `StartChapter` (default 1 = New Game).
- Narrative burns use `FMemoriaRunSnapshot::MemoryContext()`, the same derivation as `UMemoriaRunSubsystem::GetMemoryContext()`. A choice burn or cost of an Elia-tied memory breaks Still Hands (source `JourneyOath.on_player_burn`). Authored VN step burns do not. The break is traced as `oath:broken:still`; no toast is wired yet.
- Narrative `add_item` follows source `GameManager.add_item`: identities outside ITEMS are ignored (`item:ignored:<id>`), and recent items are updated. `inventory_changed:<id>` is recorded as an event rather than broadcast mid-step.
- The Arrel gait resolves bone indices once per skeletal asset. Missing bones keep their reference pose and log one warning each, so swapping in another rig cannot index INDEX_NONE.
- Verdan placeholders are hidden by the `MemoriaPlaceholder` tag or an `/Engine/BasicShapes/` mesh, not by exact coordinates. Collision is retained, and no .umap changed.
- New tests (registry is now 286):
  - `Memoria.Narrative.{BurnUsesRunContext,StillHandsOath,AddItemSourceRules}`
  - `Memoria.Presentation.{ArrelGaitBonesResolved,ArrelRigWithoutGaitBones,PlaceholderIdentification}`
  - Campaign now asserts that the slice starts in chapter 2; Verdan exploration asserts that placeholders are hidden.
- Result after the fixes: `Memoria.` 286/286, `MemoriaVisual.` 3/3 and `MemoriaCheckpointProcess.` 3/3, all rendered.
- Still open: the shop/Malet oracle harnesses set `current_chapter=1`. Tests that use them build their own chapter-1 runs, so they stay self-consistent. Regenerating them at chapter 2 would match the real slice state.
- Validation evidence is local-only from now on (`docs/unreal-migration/evidence/` is ignored). Report links to it resolve only in this worktree.

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
