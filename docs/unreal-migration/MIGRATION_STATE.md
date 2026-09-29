# Migration handoff — S320 Chapter 3, the Belt Waystation (Claude lane, 2026-09-29)

- **Why.** Step B of the user's order: port Chapter 3 onward. The user chose the content-first route. Dialogue, flags and events come from the source data. Maps are simple 3D terrain with source illustrations and light. Foes use field combat. Each chapter gets a flow test.
- **Pipeline** (repeat it for each chapter):
  1. **Dialogue.** Add the chapter's groups to `narrative_ir.py` FIELD_CASES and FIELD_FILE_CHAPTER, and the same cohort to `MemoriaNarrativeImport.cpp`. Extract with `narrative_ir.py --group <g>`, then import with `-run=MemoriaNarrative -Dialect=field`. Delete the report file first: the commandlet returns 1 early if it exists.
  2. **Map.** `Unreal/Tools/export_chapter_maps.py` parses `scenes/maps/<map>.gd` and writes two outputs:
     - `ir/chapters/<map>.v1.json`;
     - `MemoriaChapterMapSources.inl`.
     - The parsed fields are tiles, tile types, solid set, atmosphere, title card, spawn, arrival sequence, exit, triggers, chests, clues, battles, encounters and gates.
     - `--check` verifies both outputs are current. Add maps to `MAPS`.
  3. **Level.** `-run=MemoriaChapterLevels` writes `/Game/Memoria/Maps/L_<Pascal>` (slice game mode, player start at the source spawn).
  4. **Art.** Add CGs and portraits to `MemoriaNarrativeArtwork.cpp`, then run `-run=MemoriaDialogueAssets`.
  5. **Test.** Add a `MemoriaVisual.Chapter<N>` flow test and register it in `visual_test_paths()`.
- **Runtime.**
  - `FMemoriaChapterMapSpec` / `MemoriaChapterMaps`: scale 3 (a 32 px tile is 96 units), `ToWorld`/`ToSource`, `LevelPath`, `MapFromLevel`.
  - `AMemoriaChapterPresentation` builds the field from the spec:
    - **Terrain:** instanced tiles per type; low walls and rubble ruins; hidden blockers on solid tiles.
    - **Light:** a key and a fill light plus fog, from the source atmosphere.
    - **Markers:** chest and clue plinths.
    - **Characters and HUD:** Arrel's rigged figure, Elia, field combat, the exploration HUD place name and the title card (`UMemoriaChapterCardWidget`).
  - **Arrival chain.** The arrival sequence plays in the source's order through `OnFieldFinished`, with the source's flags and toasts ("Obtained: Blank Book").
  - **Triggers.** Story triggers, gated chests (grains and items), clues and battles (`SpawnWave`) fire when Arrel enters them.
  - **Exit.** The exit plays the departure, sets `ch3_complete`, moves the run to Chapter 4 and shows the completion card.
  - `UMemoriaNarrativeSubsystem::EnterChapterMap` takes up the current run. With no run it starts a development run (New Game's player at the map's chapter, trace `chapter:development_run`). `TravelToChapterMap` opens the chapter level.
- **Chapter 2 → 3.** At the closed Chapter 2 boundary the exchange-complete screen offers "Travel on: Chapter 3, The Belt" (choice 4). It carries the same run, memories and grains to the Belt Waystation.
- **Content.**
  - Six Chapter 3 groups (`DA_Field_Ch3*`): waystation_arrival, blank_book_discovery, waystation_night, class_seven_wall_message, belt_atmosphere, waystation_departure.
  - Five Chapter 3 CGs, and the portrait `arrel_pain` → arrel_face_shocked (artwork 54 → 59).
  - Korean labels: 벨트 / 페이지의 무게 / 벨트 중간역 / 획득: 백서.
- **Tests.** New `MemoriaVisual.Chapter3` (the whole chapter flow) and `MemoriaVisual.Chapter3Travel` (the Chapter 2 → 3 road).
- **Results.** Full rendered registry 413/413 (s320-full), visual 17/17 (s320-visual, including Chapter3 and Chapter3Travel).
- **Known gaps.**
  - Chapter memories (`add_chapter_memories(3)`) are not granted.
  - No autosave at chapter transitions; the checkpoint still validates only the closed Chapter 2 boundary.
  - Random encounters after completion are not ported, nor are ambient NPCs and decorations (water tank, cracks).
  - Chapter 4 is not ported: the completion card says the road is still being prepared, and the HUD keeps the waystation's name.
  - Rendered captures get smaller with each PIE start within one editor session: 1286×760 at first, down to 1220×320. Later tests in a suite capture a squashed window, which can overlap the status panel text. This is a harness issue, not the 16:9 layout.

# Migration handoff — S317–S319 pause menu, game over, Elia's techniques (Claude lane, 2026-09-29)

- **Why.** The user asked Claude to keep working alone, in order: (A) close the Chapter 1–2 loop, (B) port Chapter 3 onward, (C) the side systems. These three sessions are A.
- **S317 pause menu** (`UMemoriaPauseWidget`, after `pause_menu.gd`).
  - ESC, or the Menu action, in Verdan exploration opens it; the world pauses.
  - The source's archive backdrop and control slab; the chapter card; six rows: Resume, Options, Save, Load, Title, Quit (asked first).
  - Options shares the title's settings; a language switch also retunes the run (`UMemoriaRunSubsystem::SetLocale`).
  - **Save** writes the only ported checkpoint, the closed Chapter 2 boundary (`CanSaveClosedBoundary` validates without writing). Elsewhere the row is dark with "Not here". Saving anywhere, as the source does, needs broader checkpoint validation.
  - **Load** is Continue in place.
  - **Fix:** an explicit `?Continue` travel now restores over a live run.
  - **Not listed:** Journal, Codex, Artbook, Achievements and Endings, until those systems exist.
- **S318 game over** (`UMemoriaGameOverWidget`, after `game_over.gd`).
  - A fall no longer auto-revives. The death clip and veil play, then "You fell." with Stagger On (30% HP, foes withdraw: `Revive`), Load Save (cancel sound without a save) and Return to Title.
  - `-run=MemoriaBattleEntryAssets` now also imports `T_UiGameOverBackdrop`, `T_UiPauseBackdrop` and `T_UiPauseSlab`.
- **S319 Elia's techniques** (after `elia_diary.gd`). Keys 1–4.
  - Burning the memory her diary entry follows (while she is with the party) unlocks:

    | Key | Technique | Memory | Effect | Cooldown |
    | --- | --- | --- | --- | --- |
    | 1 | Humming Shield | campfire song | halves blows for 2 s | 6 s |
    | 2 | Desperate Reach | reaching hand | stuns foes within 320 for 2 s | 8 s |
    | 3 | Remembered Strike | first sword | 10 + 8 per burned memory on the nearest foe; Elia swings | 6 s |
    | 4 | Anchor Pulse | Elia's hands | 15% HP and cures statuses | 8 s |

  - Anchor Pulse's memory is not in the starting set.
  - Unlocks are derived from the burned history (no new save data), because checkpoint validation requires an empty Diary block.
  - The HUD shows a skill row under the HP bar, the shield ring, and the diary notice after a burn.
- **Tests.** New `MemoriaVisual.PauseMenu`, `GameOver` and `EliaSkills`.
- **Results.** Full rendered registry 413/413 (s319-full), visual 15/15 (s319-visual).

# Migration handoff — S313–S316 foes, source rules, feel, exploration HUD (Claude lane, 2026-09-28)

- **Why.** Codex's quota is low, so its monster models (the S313 request in the shared `claude-handoff.md`) are on hold. The user asked Claude to do what it can alone, in order: stand-in foes, rewards and source data, combat feel, cleanup.
- **S313 foes.**
  - `-run=MemoriaFoeAssets` makes `M_FieldFoe`, a skeletal-usage material with parameters Color, Glow, CrackStrength, RimStrength, Hit and HitColor.
    - Violet cracks follow the pre-skinned position through a vertex interpolator (a pixel-shader read failed to compile, which falls back to the default material).
    - `MemoriaVisual.FieldFoes` now requires a clean compile.
  - The same commandlet retargets UAL2's Zombie_Idle/Walk/Scratch and Sword_Regular_A/B onto the mannequin (`Combat/Foes/A_Mannequin_*`).
  - `EMemoriaFoeKind`/`FoeSpec`:
    - **void husk:** it shambles and scratches;
    - **market thief:** the Verdan source enemy, on Quinn, fast, with a short blade.
  - Monsters spawn deferred with their kind. Verdan encounters bring 2 thieves for the pool's thief entry and 3 husks as the Alley Rat stand-in. F10 calls a thief.
- **S314 source rules** (battle_core `Win` and abilities, a turn read as about 2 s).
  - **Grains:** (void ? 8 : 3) + max HP / 20 per fallen foe.
  - **A won fight:** +20% HP and a 30% drop from the potion table (richer after a void foe); a victory panel.
  - **Statuses:** the thief weakens Arrel (x0.7 for 6 s); the husk (rat stand-in) poisons him (3 ticks, never lethal).
  - **A guard or parry** blocks the status.
  - **Burns:**
    - Ember Affinity x1.1, Void Touch x1.15, Residual Warmth +5;
    - the chain gives +20% per consecutive relationship-or-higher burn;
    - identity and core burns ignite the foes.
  - **Not ported:** the bestiary (Codex) does not exist in Unreal yet.
- **S315 feel.**
  - A landed blow gives:
    - a real-time hit stop (world dilation 0.06 for 0.055 or 0.11 s);
    - camera shake on the Verdan follow camera;
    - a foe flash;
    - spark streaks;
    - the blade trail (HUD-painted).
  - **Heavy cut:** hold J or LMB for 0.45 s, with a charge ring. Sword_Regular_C at 360 degrees, 30 damage, reach 215.
  - **Guard:** hold K or RMB. Sword_Block is held at 38%.
    - Raised within 0.22 s of the blow, it parries: no harm, and the foe is stunned for 1.2 s.
    - Raised earlier, it blocks: 30% of the damage.
  - **Fix:** a parried foe no longer overwrites its stun with recovery.
- **S316 exploration HUD.** `UMemoriaExplorationHudWidget` at the top right, after `exploration_hud.gd`:
  - HP with the ghost bar, the chapter and place, memories held and burned, grains, items;
  - the source's pulse, weapon and quest rows have no Unreal systems yet.
- **Retiring the turn-based battle: deferred, with this plan.**
  - Play already uses field encounters only. Automation keeps the old route through `UseFieldEncounters()` and `SetFieldEncountersForTests`.
  - Removing it touches many suites:
    - `Memoria.BattleEntry.*` (Source oracle, EncounterDistance, OwnerLifetime, SavedStats, RenderedRevisitFlow);
    - `Memoria.BattleCore.*` (Source oracle, RenderedJourney, OwnerLifetime, StagePresentation);
    - the Malet `ShopBattle*` refusal flows;
    - `MemoriaNarrativeVisualTests` encounter steps;
    - the narrative subsystem's `IsActive()` guards;
    - audio battle music;
    - checkpoint stats.
  - **Order:**
    1. Rewrite the encounter-driven tests on field encounters.
    2. Drop the automation opt-out.
    3. Remove the BattleEntry widget and art.
    4. Keep the BattleModel source oracle only while its rules still feed field numbers.
  - This is a session of its own.
- **Checked.** Garments under the sword set: the S312 pose sheet (25 frames) showed no tearing. Field lighting was left as is (no concrete defect).
- **Tests.** New `MemoriaVisual.FieldFoes`, `FieldRewards` (with the exploration panel) and `FieldFeel`. `FieldCombat` counts blows taken for the dodge check.
- **Results.** Full rendered registry: 413/413 (s316-full). Visual suite: 12/12 (s316-visual).

# Migration handoff — S312 memory burn skill and Arrel's sword (Claude lane, 2026-09-28)

- **Why.** The user's plan after S311: memory burn becomes the powerful skill, Arrel fights with his sword, and the monster model goes to Codex (the request is in the shared `claude-handoff.md`).
- **Memory burn (`UMemoriaFieldCombatSubsystem`).**
  - R in Verdan exploration opens the picker and slows the world to 0.12.
    - It lists every memory that can burn now, weakest first, with localized titles from `MemoriaArchive::Build`.
    - 1–9, ↑↓ or the wheel choose; R, Enter or a click burns; Esc or a right click lets it go.
    - Grade 2 and 1 memories (identity, the core) ask twice.
  - The burn goes through `UMemoriaRunSubsystem::BurnMemory`, so the loss is permanent and the passives, erosion, audio drama and saves follow.
  - Then Arrel is invulnerable for a 0.45 s cast and releases the grade's source skill (battle_core `burn_skills`):
    - Ember, Blue Flame Slash, Incinerate, Identity Pyre, Zero Burn;
    - a ring of 260–1100 units;
    - damage is the skill's base plus the effective burn power;
    - a shove of 50–280;
    - a coloured point light.
  - HUD: the ring projected on the floor, a screen flare, the banner (skill and the memory it cost), and the picker panel.
  - The combat HUD now follows the run's locale, like the memory titles.
- **Sword.**
  - Quaternius' Universal Animation Library 2 (CC0, free Standard; downloaded with the user's permission) lives in the shared `models/_raw/ual2/`.
  - `-run=MemoriaSwordRetarget` imports its glb through Interchange into the git-ignored `/Game/Memoria/Imported/UAL2`, then retargets seven clips onto Arrel as `Field3D/Arrel/Combat/A_Arrel_<Clip>`:
    - Sword_Regular_A/B/C, Sword_Heavy_Combo, Sword_Dash, Sword_Block, Hit_Knockback;
    - with its own IK rigs and `RTG_UAL2_Arrel`.
  - `MemoriaRetargetKit.h` now holds the rig and retargeter setup shared with `MemoriaCombatRetarget`.
  - The sheathed prop was split in Blender (`models/_raw/s312_sword/sword_split.py`) into `SM_Arrel_sword_drawn` (the hilt plus a new blade) and `SM_Arrel_scabbard`. Both are imported by the same commandlet.
  - The blade's grip is computed from the hand's knuckle bones (`AttachGrip`), not a hand-tuned socket.
  - `MemoriaCombatClips::ForAction` maps the melee combo to the sword cuts for any character that has the set (Arrel), and the dash to Sword_Dash.
  - The sword is drawn while husks stand or Arrel acts, and sheathed 4 s after the last one falls.
  - The combo rates are 1, 1 and 1.3; the burn cast plays the spinning cut.
- **Tests.**
  - New `MemoriaVisual.FieldBurn`:
    - picker, cancel, ask-twice, the burn is permanent;
    - three husks within reach fall and one beyond is spared;
    - the sword is sheathed after the fight.
  - `MemoriaVisual.FieldCombat` checks the sword clips and the drawn blade.
- **Known.**
  - The Verdan source enemies are the Alley Rat and the Market Thief. The void husk is a stand-in and still a tinted mannequin, which gets the engine default material (BasicShapeMaterial lacks the skeletal usage flag).
  - Burn tuning is first pass.
  - Sword_Heavy_Combo and Sword_Block are retargeted but unused.
  - Garment deformation under the sword motions has not been rechecked.
- **Results.** Full rendered registry: 413/413 (s312-full, MEMORIA_UNREAL_PASS discovered=413). Visual suite: 9/9 (s312-visual).

# Migration handoff — S311 action combat foundation (Claude lane, 2026-09-27)

- **Why.** This is the user's combat pivot: Diablo-style quarter-view action on the field map instead of the Godot turn-based battle. It builds on S310's rigged Arrel, Elia and Malet.
- **Animations.**
  - `Unreal/Tools/install_mannequin.py` copies Epic's UE 5.8 mannequin from the engine templates into the git-ignored `Content/Characters/Mannequins`.
  - `-run=MemoriaCombatRetarget` batch-retargets seven clips onto each rigged character, into `Field3D/<Name>/Combat/A_<Name>_<Clip>`. The clips are Attack_01–03, ChargedAttack, Dash, HitReact and Death.
  - It follows the editor's auto-retarget: auto-characterized IK rigs (Arrel 21 chains, mannequin 29), the default op stack, exact chain mapping, an aligned target retarget pose, IK disabled and root motion from the rig roots.
  - Root causes found on the way:
    - a retargeter made with `NewObject` has no op stack, so every clip came out as the reference pose (`-Inspect`: upperarm 0° before, 46–141° after);
    - replacing already loaded retarget assets in place crashed the batch. Regenerate from deleted folders.
- **Animation layer.** `UMemoriaFieldAnimInstance` blends an action layer (explicit-time evaluator) over idle/walk.
  - The root bone's translation is locked to the reference pose: the mannequin's lunge rode on the root and pulled the mesh off the pawn.
  - `UMemoriaFieldCharacterComponent` gained `PlayAction` (blend in 0.08 s, out 0.12 s, or hold), a free aim yaw, and `InitializeMannequin` (the void husk: a tinted SKM_Manny).
- **Combat (`UMemoriaFieldCombatSubsystem`).**
  - The player's HP is the run's HP.
  - A three-step combo toward the cursor:
    - damage 12/14/22, rate 1.35, hit window 30–55%;
    - range 175 and a ±55° arc;
    - input from 40% of a step chains the next.
  - Dash: Shift, 330 units in 0.32 s, invulnerable, 0.7 s cooldown.
  - Stagger on hit; defeat plays Death, then after 2.5 s restores full HP and withdraws the husks.
- **Monster (`AMemoriaFieldMonster`, void husk).**
  - Stats: HP 60; it chases at 95 (Arrel walks 120); aggro 750.
  - It telegraphs a 0.6 s windup with a red reach disc, then strikes for 9 within 150+40, and recovers for 0.9 s.
  - Hits stagger it; its corpse fades after 2.5 s.
  - It is moved directly, because pawn movement ignores input without a local controller.
- **HUD (`UMemoriaCombatHudWidget`, painted).**
  - HP bar; husk bars; damage popups; the defeat veil; the control hint.
  - Korean or English from the settings.
- **Controls (Verdan exploration).**
  - Left click or J: attack toward the cursor, which is now shown in the field.
  - Shift: dash.
  - F9 (development): call a husk.
- **Encounters.** On a Verdan revisit, an encounter now spawns 2–3 husks in the field when `UseFieldEncounters()` is true. That is always in play; under automation, only when a test opts in, so the stopgap turn-based suites keep their route until they retire.
- **Tests.** New `MemoriaVisual.FieldCombat` (rendered):
  - the retargeted clips load;
  - a J key plus aimed attacks: the combo reaches step 3, at least 4 blows land, and the husk dies;
  - a telegraphed strike wounds Arrel; a Shift dash dodges the next with no damage;
  - close captures of a swing at three moments (`Saved/Validation/FieldCombat/`).
- **Known.**
  - The mannequin's melee set is unarmed: Arrel strikes with his fists while his sword stays sheathed. Sword clips still need a free source.
  - Long garments were not rechecked under combat motions (S310 warning).
  - Husks are grey mannequins in void tint.
  - Numbers are first-pass.
- **Results.** Full rendered registry: **413/413** (unreal-run-20260927T144353783802); visual **8/8** incl. MemoriaVisual.FieldCombat.

# Migration handoff — S310 rigged field characters (Codex, 2026-09-27)

- Arrel/Elia/Malet corrected and rigged locally; 77 bones each; idle/walk and bone props running in the field.
- New Field3D packages and ArtSource/FieldCharacters sources. Native distance-driven animation proxy. HD/pixel fallback retained.
- UE5.8.2 build PASS; rendered MemoriaVisual. 7/7; rendered Memoria.Foundation. 6/6. Actual screenshots and state-preserving replay verified.
- No new commit/push. Claude/original Godot/foundation untouched. Full 413 registry not rerun.
- No combat retargeting yet; joined garment topology and side projection remain limited for large motions.
- Detailed result and adoption steps: [S310_RIGGED_FIELD_REPORT.md](S310_RIGGED_FIELD_REPORT.md); shared codex-review.md / models/MANIFEST.md.
- **Adopted into the Claude lane by Claude (2026-09-27).**
  - `apply_s310.py`: CHECK_OK, 56/56 files applied.
  - Rendered `MemoriaVisual.` 7/7, with `FIELD_CHARACTER arrel|elia|malet rigged`.
  - Full rendered registry **413/413** (`unreal-run-20260927T121720430015`).

---

# Migration handoff — S309 battle screen presentation after battle_scene.gd (Claude lane, 2026-09-27)

- **Why.** The Unreal battle was a teal dashboard of framed boxes, with the burn telegraph overlapping the objective, modifier and Elia cards. The Godot reference is `tmp/visual_audit/battle_forest_shade.png`: combatants stand on a stage, with an ornate command deck and readout frames.
- **Scope.** Presentation only. `UMemoriaBattleEntryWidget`'s state logic is unchanged: `Display`, `Navigate`, `ConfirmIntent`, `ClickAction`, the six commands, and the burn/item lists. So are its probes. The battle subsystem is untouched.
- **Stage.**
  - The encounter backdrop is darkened toward the floor and edges.
  - Arrel, Elia (behind) and the enemy stand as full-figure plates on `STAGE_BASELINE_Y` 424, with `BATTLE_ROLE_PROFILES` boxes, edge softness, oval masks and `plate_modulate` (the enemy lifted 1.5×).
  - Role shadows and glows sit under the feet.
  - `battle_stage_blend.gdshader` is ported as the UI material `M_BattlePlate`, authored by `-run=MemoriaBattleStageAssets`: the same edge, oval and floor-blend math, `Modulate` (which may exceed 1) and a `Region` crop.
  - The pixel Market Thief fallback keeps the thin 0.02 edge and native scale.
- **HUD, after the source layout:**
  - objective card on the tactical plate art;
  - field read (enemy miniature, location, and the modifier or the witness echo);
  - enemy panel (HP, break bar, witness pips);
  - turn banner;
  - player panel (portrait, HP, limit, momentum, combo and statuses);
  - field readout frame with the two latest log lines;
  - the command deck art (region 8,284,1656,356) with numbered two-line commands (`_format_action_button` roles, `_action_base_color`, hover box for focus);
  - burn and item lists in a gold-framed panel;
  - telegraph band over the stage centre;
  - damage numbers over the struck figure;
  - the result card in the middle frame of `ui_battle_victory_reward_panel` (520×490 cover crop, feathered).
- **Import.** The four interface arts were added to `MemoriaBattleEntryArt::Sources`. `-run=MemoriaBattleEntryAssets` is now additive: existing packages are kept.
- **Deviations.**
  - Six commands in a 3×2 deck; the source has eight, with Limit and Auto not ported.
  - Arrows still step linearly.
  - Not ported yet: stance chips, battle speed chip, turn order, cut-ins, and the 3D hybrid depth stage and rim light.
- **Tests.**
  - New `Memoria.BattleCore.StagePresentation`: material and art resolve, the stage figures, and the Korean command deck text.
  - `Memoria.BattleCore.` 101/101.
  - Reviewed captures (`Saved/Validation/Phase1O/ShopBattleCombat_Combat*`): burn, witness read, victory, released, defeat.
- **Results.** Full rendered registry: **413/413** (unreal-run-20260927T061800873639, heavy programs closed); visual 7/7; BattleCore 101/101.
- **Flaky `Memoria.Foundation.MapInputAndModal` (investigated).**
  - It failed in three S309 full runs made while the user ran MapleStory and other heavy programs (commit free 6 GB; one run died of out-of-memory). The injected `D` never registered (`pressed=0`), and frames ran about 5× slower (28 ms against 6 ms).
  - It passed in isolation, after Firebomb, and after BattleCore+Firebomb.
  - An S308-code control run passed, but after the programs were closed, so it was confounded.
  - With the heavy programs closed, the S309 full run passed 413/413.
  - Conclusion: host load and input flush, not S309 code. Rejected: S309 code, since the same code passes under light load.

# Migration handoff — S308 title screen and menu after main.gd (Claude lane, 2026-09-27)

- **Entry.** The game's default map now opens as the title.
  - `GameMapsSettings LocalMapOptions=?Title` takes `L_Ch2VerdanSlice` there; `AMemoriaSliceGameMode::StartPlay` calls `EnterTitle`.
  - Without the option, the map keeps its development VN and `?NewGame` entries.
  - `L_VerdanHost` also accepts `?Continue`, beside `-MemoriaContinue`.
- **Title widget.** `UMemoriaTitleWidget` follows main.gd:
  - the key art `ui_title_memoria_premium.png`, cover-fitted with the 18/14 px overscan, the breath and the pointer parallax;
  - the rift glow;
  - 30 ash motes of the source's 512 px texture, with a fade over their life added;
  - the vignette shader, baked once with `MemoriaUiKit::Paint`;
  - letterbox strips animating from 42 to 24 px;
  - the gold rail and the wordmark stack (eyebrow, MEMORIA with outline and shadow, subtitle, ◆ divider, tagline);
  - the archive menu panel (kicker, heading, gold rule, key hint);
  - numbered items with the source's normal, hover and disabled boxes, the left accent bar, and the 1.8% focus emphasis;
  - the intro choreography, with its timings;
  - Korean and English copy from `GameManager.loc`.
- **Menu.** New Game, Continue, Options, Quit.
  - Continue is disabled without a valid save, and focus skips it.
  - Deviation: the source's Aftermath preview (Part 2) is not listed.
  - New Game starts `StartNewGame(settings locale)`.
  - Continue resumes the newer valid slot (`UMemoriaCheckpointSubsystem::FindContinue`): the chapter autosave in place, or the Verdan checkpoint via `L_VerdanHost?Continue`.
  - Quit calls `QuitGame` outside automation.
  - The confirm that starts the game is held as a gesture, so it cannot advance the first VN line.
- **Options (partial port of options_menu.gd).** `UMemoriaSettingsSubsystem` holds master, BGM and SFX volume (80/70/80), fullscreen and language (Korean first).
  - Settings persist in `GameUserSettings.ini` outside automation.
  - The audio applies master×bus gains live.
  - Not ported: text speed, difficulty, battle speed, accessibility, resolution.
- **Audio.** A `title` track (`title.mp3`) plays on the title, with no ambience or duck.
- **Shared kit.** `MemoriaUiKit` holds `Srgb`, `Gradient` and `Paint`; the VN widget now uses it.
- **Layout units.** Slate lays out in 1080p units at 720p, so source lengths scale by 1.5 and font points by about 1.12.
- **Tests.**
  - `Memoria.Title.ContinueSource`: disabled or empty storage, the chapter autosave offered, a damaged slot refused, and New Game or Continue leaving the title.
  - `Memoria.Title.Settings` and `Memoria.Title.Menu`.
  - `MemoriaVisual.TitleScreen` (rendered PIE with real Slate keys):
    - title music, Continue disabled;
    - Options Left lowering BGM, with the live multiplier checked;
    - Escape, Up and Enter;
    - New Game in Korean at the cold open, the first line not skipped.
    - Captures in `Saved/Validation/Title/`.
  - Artwork coverage is 54.
- **Results.** Full rendered registry **412/412**; visual suite 7/7.
- **Play.** `UnrealEditor.exe Memoria.uproject -game` opens the title.

# Migration handoff — S307 illustrated field art imported and tuned (Claude lane, 2026-09-27)

- **Art.** Codex delivered priority 1 of `FIELD_SPRITE_ART_SPEC.md` to the shared `field_hd/`:
  - `arrel`, `elia` and `malet`, each with down, up and right views, 9 PNGs;
  - report in `codex-review.md`, provenance in `field_hd/MANIFEST.md`.
  - Claude re-checked all 9: SHA-256 matches the manifest; each is 1024×1536 RGBA with transparent corners; feet at y 1480; lower centre x 511.5–512.5.
  - Copied to `assets/sprites/field_hd/<id>/` and imported to `/Game/Memoria/Presentation/FieldHD/` (T_/SPR_ ×9).
- **Walk.** No walk contacts were delivered (optional in the spec), so walking uses the bounce fallback.
- **Scale.** The imported sprites' render bounds are tight to the figure: Arrel 131, Elia 120, Malet 131 units at PPU 10. `InitializeCharacter`'s `WorldHeight` is therefore the figure's real height: Arrel 150, Elia 138 (0.92), Malet 150.
  - `PLAYFEEL_CHARACTER_HEIGHT` 0.138, inside the 10–18% bound.
  - Feet land on the pivot; no anchor change was needed.
- **Resolution (root cause).** The first rendered captures were blurry in the field and blocky close up.
  - Rejected hypotheses: no project texture-group or streaming override exists (config grep); the power-of-two stretch was not the cause (12 mips present).
  - Confirmed cause: at the review capture only 7 of 12 mips were resident (`FIELD_TEXTURE_RESIDENT`), a top mip about 64 px tall; the streamer had not raised these cards.
  - Fix, in the commandlet:
    - `NeverStream` on the 9 HD textures, about 25 MB in total;
    - `StretchToPowerOfTwo` so the 1024×1536 canvas gets a mip chain (Paper2D UVs are normalised; the pivot is unchanged).
  - After the fix: resident 12/12, and the close review shows the painted detail.
- **Commandlet.** `-run=MemoriaFieldCharacterAssets -Force` now deletes and re-creates the packages before anything loads them. Overwriting in place failed with a partly loaded package that could not be saved.
- **Lighting.** Malet stood outside Arrel's character fill and read as murky next to the lit pair.
  - Every figure now gets the same channel-1 fill: `ArrelFillLight` on the player and `MaletFillLight` on Malet's card. Elia shares Arrel's.
  - Intensity went from 4 to 3, so silver armour and the white robe stay off the clip.
  - Lantern and moon lighting are unchanged.
- **Tests.**
  - `MemoriaVisual.FieldCharacters`: `FIELD_CHARACTER arrel|elia|malet hd`; HD textures are filtered; mips > 4 under a real RHI (12 found).
  - `MemoriaVisual.VerdanExploration`:
    - gait requires 8 poses when the art has walk frames; without them it requires 4 facings plus a visible bounce (> 1% of height; measured 3.26);
    - illustrated art must be fully resident at the close review;
    - both fill lights are checked for isolation from world lighting.
  - `UMemoriaFieldCharacterComponent::GetWalkFrameCount()` was added for this.
- **Formation (user-approved deviation).** Elia's Godot formation distance, 48, was about 60% of the HD figure width (~80 units), so she stood inside Arrel's silhouette.
  - The user approved 90: `AMemoriaEliaCompanion::FormationDistance` is 90.
  - `SourceFormationDistance` 48 stays equal to the source fixture, and both are asserted in `Memoria.VerdanStory`.
  - Her talk reach follows it: `InteractionRange` = formation + 32 = 122, keeping the source's 32 margin (80 against 48). At 80 the full run showed she stopped out of reach, and `Memoria.BattleCore.RenderedJourney` could not talk to her.
  - Standing captures now show a clear gap. Short direction changes still overlap briefly while she catches up along the trail.
- **Results.** Full rendered registry **409/409** (`Saved/Validation/unreal-run-20260927T003425991298`); rendered visual suite 6/6; `Memoria.BattleCore.` 100/100 after the talk-reach fix.
- **Known.**
  - Sable, Tobias, Nera, Kairos and Veil have no field art yet (priority 2 and 3 of the spec).
- **Next.**
  - Walk contacts and the other characters when Codex delivers them; the same import applies.
  - Then field lighting polish, the title/menu and the battle screen.

# Migration handoff — S306 field characters: one figure path, awaiting illustrated art (Claude lane, 2026-09-27)

- **Why.** The user played S305 in Verdan. Arrel was a 3D prototype mesh standing beside Elia's and Malet's 2D sprites; the user called it jarring, and said the others were not satisfying either. They chose **high-resolution illustrated sprites**, with the art produced by **Codex**.
- **Art request.** `FIELD_SPRITE_ART_SPEC.md` specifies:
  - ids and priorities, with canon notes (Sable blind);
  - views (down, up and right; left is mirrored) and optional two-frame walk contacts;
  - the canvas: 1024×1536 transparent, feet pivot (512, 1480), fixed height ratios;
  - style (the `character_shots` v2/v3 look), delivery paths and acceptance.
- **Figure.** `UMemoriaFieldCharacterComponent` draws every field character the same way:
  - a camera-facing card (roll 42°, feet on the −8 anchor);
  - facing from real travel, and a gait advanced by distance (58 units per stride) that also drives the `step_stone` footfalls;
  - walk frames (4 for pixel, 2 for HD), a bounce and a standing breath;
  - `FieldHD/SPR_<Id>_<View>` when imported, otherwise the source pixel sprites, point sampled.
  - The card casts no shadow (a grazing lantern made it self-shadow in streaks); the existing ground blobs anchor it.
  - Height: Arrel 150, Elia 0.92×, Malet 1.0× (13.8% of the viewport, inside the 10–18% play-feel bound).
- **Wiring.**
  - Arrel's 3D prototype is no longer spawned in the field. `UMemoriaArrel3DComponent` and its standalone tests are retained.
  - `AMemoriaVerdanPresentation::CharacterFigure()` replaces `CharacterMesh()`.
  - Malet and Elia use the same component; Elia's old sprite is kept but hidden.
- **Import.** `-run=MemoriaFieldCharacterAssets` imports `assets/sprites/field_hd/<id>/*.png`: additive, mips kept, trilinear, pivot from the spec.
- **Tests.**
  - `MemoriaVisual.VerdanExploration`'s skeleton assertions are now figure assertions (card, height, tilt, anchor, point sampling, facing, walk frames, gait settling, head projection from the figure height).
  - New `MemoriaVisual.FieldCharacters` reports `FIELD_CHARACTER <id> hd|pixel|missing`. Arrel, Elia and Malet must resolve; today all three are `pixel`.
- **Also.** The exploration status box uses the interface sans at 15, so the control hints no longer wrap; the S305 serif had made them wrap.
- **Results.** Full rendered registry `MEMORIA_UNREAL_PASS discovered=409 source_parity=51`. Rendered visual suite 6/6 (FieldCharacters added); arrel, elia and malet report `pixel`.
- **Next.** When Codex delivers the art, run the commandlet and review the `PlayFeel1` captures. Then continue with field lighting and the battle screen.

# Migration handoff — S305 VN graphics: the vn_scene.gd presentation (Claude lane, 2026-09-27)

- **Why.** The user played the S304 New Game build and called the graphics far off. This session rebuilds the story presentation of `UMemoriaDevelopmentNarrativeWidget` after `vn_scene.gd`, which serves every VN and Field dialogue. Compact status, development screens and the shop keep their previous layout.
- **Fonts** (`generate_font_sources.py`, `MemoriaFontAssets`, `MemoriaFonts`):
  - The bundled Noto fonts are variable fonts. Unreal renders their default instance, the thinnest master (Sans 100, Serif 200).
  - The tool instances them at the `ui_theme.gd` S230 weights: body Serif 500, titles Serif 600, interface Sans 600.
  - It subsets them to Latin, punctuation, symbols, Hangul and CJK punctuation plus every character in `data/` (three Hanja), bringing Serif from 24 MB to 9.5 MB.
  - Static TTFs go to `Saved/FontSources`; the imported `UFontFace`/`UFont` assets are the committed artifact.
- **Illustration.**
  - The CG is cover-filled (`STRETCH_KEEP_ASPECT_COVERED`), no longer letterboxed at the sides.
  - Procedural gradient layers match the source definitions: focus glow, lower wash, cinematic vignette and ember vignette. The vignette is 1.8× the source alpha, because the source's light-toned modulate reads weaker on Slate's linear tint; the lower wash is 0.3 (source 0.18) under the translucent dialogue interior.
  - Letterbox bars at 60/720.
  - Source colors are converted from sRGB, since Slate tints are linear.
- **Dialogue.**
  - The `ui_vn_memory_frame_overlay` frame art over a dark interior, with the speaker in its name plate. Titles use Serif SemiBold; System is cyan.
  - A rich-text typewriter at 0.025 s per character. The untyped remainder is laid out but hidden, so words never reflow. Confirm while typing completes the line.
  - A pulsing NEXT indicator; the location sits in the top bar and the controls in the bottom bar.
- **Portraits.**
  - Left and right slots with speaker highlight and `PORTRAIT_DIM`, per-character accent frames (`_portrait_accent_for_id`), and a fade-up entrance.
  - Source rules: lines over full-scene story CGs clear the stage (`_should_hide_portraits_for_cg_line`), and Arrel and Elia hold the stage alone (`_uses_single_portrait_composition`).
  - Deviations: the portraits sit above the name plate instead of behind it. The light parchment face art gets an inner vignette.
- **Choices.**
  - Exactly three choices use the `ui_vn_choice_archive_overlay` frame, one per slot, over a dimmed scene, with the title and hint above the ornament. Other counts use the same buttons in a centred stack.
  - Each option shows its effect preview, and mouse hover selects.
- **Glitch.**
  - A memory burn (a new `BurnSerial` from `burn:*:ok` events) plays `_play_burn_glitch`: a red flash, an 8 px chroma split over 0.7 s, and the ember vignette (peak 0.6 instead of 0.85, which flooded the cover-filled frame).
  - Distorted lines get the 1.2 s + 0.4 s 3 px chroma split, the 0.12 s scramble and a violet text style.
- **Tests.**
  - Artwork table 51 → 53 (the two frame overlays).
  - `DialogueInteraction` now asserts the source portrait rules.
  - `Chapter1Journey` adds a speaker capture (`Ch1_09_Speaker`) and skips portrait checks on story-CG lines.
  - The typewriter is off under automation unless a test enables it. `VisibleText` reports the complete line.
- **Results:** full rendered registry `MEMORIA_UNREAL_PASS discovered=409 source_parity=51`; rendered visual suite 5/5. The S305 captures are in `Saved/Validation/Chapter1Journey`: cold open, void impact, framed choice, system log, distorted line, void-beast choice, ledger, speaker, Verdan.
- **Not done here:** the AUTO / fast-forward chips, the page-turn sweep and portrait expression swaps (`_play_portrait_expression_change`); the Verdan field, battle and title screens are the next sessions (S306 title/menu, S307 field, S308 battle).

# Migration handoff — S304 Chapter 1 stage 3: New Game plays Chapter 1, rendered, to Verdan (Claude lane, 2026-09-26)

- P5 stage 3, assigned by the user (Claude working alone). A New Game now plays the whole imported Chapter 1 route in the production host and widget: `ch1_cold_open` → `ch1_prologue` → `ch1_forest_walk` → `ch1_void_beast` → `ch1_after_forest` → `ch2_market_arrival` → `L_VerdanHost`.
- **Entry.** `UMemoriaNarrativeSubsystem::StartNewGame` follows `main.gd` `_on_new_game_pressed`:
  - a fresh chapter 1 run: HP 100/100, 0 Grains, `witness_ink` ×1, quick slots `witness_ink, potion, antidote`, Elia in the party, no flags;
  - the locale carries over, since it is a setting and not run data;
  - the VN uses a resolver (`DA_VN_<PascalId>`), so `goto_scene` crosses the imported scenes.
  - `L_Ch2VerdanSlice` starts it with the `?NewGame` travel option or `-MemoriaNewGame`. The default boot is still the development Ch2 VN, since there is no title screen yet.
- **View.** VN views come from the current definition, not the fixed Ch2 asset.
  - The title comes from metadata (`CHAPTER 1  /  ASH`; Korean in ko).
  - Text goes through `VNDisplay`, so distortion is applied.
  - The CG is carried across steps and scenes (`_change_cg`), with short refs resolved by `CgSource`: `CG_ALIAS_FALLBACKS`, then `game_image`, then `DEFAULT_CG_FALLBACK`.
  - Choices carry the source title, hint and per-option `effect` preview (defaults `DECISION` and the Arrel hint).
  - `system_log` shows as the System speaker, tinted cyan.
- **Presentation cues** (`vn_scene.gd`). They run on a deterministic widget clock driven by world delta, like the source tweens:
  - crossfade to the next CG over the step `fade`;
  - `cg_motion` curves: pull_back, push_in, strike, still, and the ambient Ken Burns zoom;
  - `impact` flash colors and the cinematic nudge, after 72% of the fade;
  - step `sfx` through the audio catalog;
  - scene `bgm` through the audio subsystem (`dialogue_tense`, `ch1_forest`, `ch2_verdan`).
- **Chapter ledger.** `complete_chapter` raises the `_show_chapter_ledger` overlay: title, burns, intact count, anchors x/4 with the name, and the thread line from `weave_unlocked`. It fades 0.4 s in, holds 4.6 s and fades 0.6 s out.
- **Autosave.** `autosave_chapter_transition` writes a separate checkpoint slot, `chapter.memoria.json`. It uses the same framing, SHA-1, atomic commit and test isolation as the Verdan boundary slot, but requires an active VN cursor at chapter 2 or later.
  - The autosave step also jumps to `ch2_market_arrival`, so the save resumes at the arrival's first step and the ledger is not replayed.
  - `ResumeChapterAutosave` restores it through the generalized `PrepareRestore`, which resolves the saved scene.
- **Content.**
  - 16 CGs and 3 portraits imported (the Elia anchor texture is reused), plus 8 portrait keys; artwork table 31 → 51.
  - `void_pulse` rendered from `_generate_sfx`, plus the `dialogue_tense` and `ch1_forest` BGM.
  - `MemoriaAudioAssets` is now additive (existing packages kept), like `MemoriaDialogueAssets`.
- **Tests.**
  - `Memoria.Chapter1.NewGameHost.<case>`: the real host plays each oracle case. Its `vn:` trace equals the Godot SceneFlow events exactly. Every CG and portrait resolves, and the start state, ledger, music and cues are checked.
  - `Memoria.Chapter1.AutosaveResume`: save, restore in a new game instance, then play to Verdan.
  - `MemoriaVisual.Chapter1Presentation`: widget cue timeline.
  - `MemoriaVisual.Chapter1Journey`: rendered PIE; `?NewGame` → Chapter 1 with the song-burn picks → Verdan field, with 8 captures in `Saved/Validation/Chapter1Journey`.
- **Deviations and gaps:**
  - **Ledger always says "nothing".** The source runs `set_chapter 2` before `complete_chapter 1` on the same step, so the ledger's burn snapshot resets first and Chapter 1's ledger never lists its burns. This is ported as the oracle shows. It is a source-order fix candidate, not changed here.
  - **Ledger names are English.** Unreal has no Korean memory titles yet.
  - **Distortion is a text tint.** The source adds a 1.2 s chromatic split on the CG and a pre-glitch scramble; here it is a tint on the line only.
  - **Ambient pan is fixed per step.** The Ken Burns pan is deterministic per step rather than `randf`.
  - **Step sfx plays at once.** The source defers it with the flash.
  - **No title screen or Continue menu yet.** The autosave is resumable through the API and tests only.
- **Results:**
  - Full rendered registry: `MEMORIA_UNREAL_PASS discovered=409 source_parity=51` (403 → 409).
  - Rendered visual suite: 5/5, including `Chapter1Journey` and `Chapter1Presentation`.
  - `generate_audio_sources.py --check`: 26 cues. `test_audio_sources.py`: 5/5.
  - The other Python tool tests have 16 historical snapshot-count failures; all of them also fail at `HEAD`, and none are new.

# Migration handoff — S303 Chapter 1 stage 2: the route plays (Claude lane, 2026-09-26)

- P5 stage 2, assigned by the user (Claude working alone). The imported Chapter 1 route now executes exactly as `scene_flow.gd` does, from `ch1_cold_open` through `ch2_market_arrival` to the Verdan map request.
- **Interpreter.** `FMemoriaVNInterpreter` takes an optional resolver, so `goto_scene` can switch to another imported definition (same-scene `goto_scene` is unchanged). The step order follows `_run_step`:
  - flag, `set_chapter`, `complete_chapter` (event and ledger of memories burned since `set_chapter`), autosave, burn, ending, gate, rewards, action;
  - distortion is applied at display time.
- **`FMemoriaNarrativeContext`:**
  - `SetChapter` advances the run chapter and calls the new domain `AdvanceChapter`.
  - `VNDisplay` applies `distort_if_burned` as the source does: keys are replaced before localization, so a missing `distorted_*_ko` keeps the original Korean.
  - Chapter completion and autosave are recorded as events. Stage 3 hosts turn them into the ledger overlay and the actual save.
- **Memory domain.** `AdvanceChapter` ports `add_chapter_memories`' once-per-chapter bookkeeping:
  - erosion from chapter 3;
  - anchor vigil (intact `WEAVE_SECONDARY` × 2, +3 for `core_name_origin`) and its passives (quiet_focus 8, steady_hand 20, unbroken_edge 36, shared_burden 56, deep_anchor 80);
  - a guard-slot reset.
  - Two new event kinds are appended.
  - Loan maturity and the Oath of Ash payout are deferred: neither system exists in the slice, and neither can be active on this route.
- **Oracle.** New `export_chapter1_oracle.py` runs the pinned Godot SceneFlow and MemoryManager over the six route scenes. It uses the existing isolated harness; only the ledger overlay, chapter-complete and autosave are recorded instead of drawn or saved. Five cases (four branch paths in en, one in ko) cover every option at every Chapter 1 choice and the arrival choice, including:
  - the song burn and its distortion;
  - the humming gate;
  - three forest-hub orders;
  - the void-beast burns and the counter unlocked by the cold-open reading.
  - Fixtures are under `fixtures/chapter1` (LF-pinned).
- **Tests.** `Memoria.Chapter1.OracleRoute.<case>` replays each case with the imported assets and compares:
  - the full event trace (visits, steps with the displayed text, choices, flags, burns, items, Grains, chapter effects);
  - every choice-point and final snapshot (scene, index, flags, Grains, HP, chapter, burned, anchor vigil and passives, guard slots, map).
- **Results:** full rendered registry `MEMORIA_UNREAL_PASS discovered=403 source_parity=51` (398 → 403); Chapter 1 oracle `--check` 5 cases and narrative oracle `--check` 10 cases byte-exact.

# Migration handoff — S302 Chapter 1 stage 1: VN content import (Claude lane, 2026-09-26)

- Roadmap P5: New Game through the current Chapter 1 to the Verdan arrival. The user asked Claude, working alone while Codex is out of quota, to do it in three stages:
  - **Stage 1 (this):** import the content, source-attested.
  - **Stage 2:** play it: the cross-scene interpreter and branch oracle.
  - **Stage 3:** New Game entry and presentation, rendered.
- **Content.** The whole current Chapter 1 route from `scene_flow.gd` was imported through `import_narrative.py`, each scene with a first import, an unchanged reimport and a reload check. Every `goto_scene` in the route stays inside the cohort.

  | Scene | Steps | Asset |
  |---|---|---|
  | `ch1_cold_open` | 8 | `DA_VN_Ch1ColdOpen` |
  | `ch1_prologue` | 46 | `DA_VN_Ch1Prologue` |
  | `ch1_forest_walk` | 41 | `DA_VN_Ch1ForestWalk` |
  | `ch1_void_beast` | 60 | `DA_VN_Ch1VoidBeast` |
  | `ch1_after_forest` | 14 | `DA_VN_Ch1AfterForest` |

  `ch1_after_forest` hands off to `ch2_market_arrival`.
- **Schema (review item, VN-only keys).** Field rows and choices reject these; chapter effects are step-only.
  - Text: `system_log`, choice framing (`choice_title`, `choice_hint`), choice `effect` text (on choices only), and `distort_if_burned` with `distorted_text`/`distorted_narrate`/`distorted_speaker`.
  - Presentation: `cg_motion`, `sfx`, `impact`, `distorted_portrait`, `distorted_cg`.
  - Effects: `set_chapter`, `complete_chapter`, `autosave_chapter_transition`.
  - VN `bgm` is optional.
  - `narrative_ir.VN_CASES` and the C++ VN cohort carry each scene's file, step count and chapter. `goto_scene` may target any cohort scene, with `start_index` bounded by the target.
  - All prior Field and VN IR stays byte-identical, and the types are regenerated.
- **Not yet executed.** The runtime VN interpreter still plays a single definition, and the new keys are only data so far. Stage 2 adds the `scene_flow.gd` step order (chapter effects before gates, distortion after actions), cross-scene `goto_scene`, and a Godot branch oracle over every Ch1 choice.
- **Tests:** new `Memoria.Chapter1.ImportParity` (five scenes: saved asset equals a strict source-attested read) and `Memoria.Chapter1.RouteChain` (handoff chain and source field counts).
- **Results** (UE 5.8.2 rendered): full `Memoria.` **398/398**. The Godot narrative oracle `--check` passes 10/10. Generated types and contract fixtures pass `--check`. Python tools show 13 passing and the same 3 historical failures.

# Migration handoff — S301 self-review of S297–S300 (Claude lane, 2026-09-26)

- Codex is out of quota, so the user asked Claude to continue alone in Claude Code. Claude reviewed its own pushed S297–S300 changes in place of the Codex review.
- **Checked and sound:**
  - World/run lifetime of story points, the quest and Elia: every checkpoint restore travels to a fresh Verdan world, which re-arms them, and a run replacement clears arming.
  - Elia's facing gate against the Malet and story-point interactions.
  - Witness submission guards.
  - The status-box redraw.
  - Schema keys are rejected on VN rows and choices.
- **Fixed:**
  1. Elia's Chapter 1 reactions were recorded in the trace as `chapter2_dialogue.json` requests. The trace now names the authored file.
  2. The Sump Ledger reward and void drops give a Hi-Potion that the battle item list could not use. It is now a source "heal" item using its catalog power (80), listed while carried. The battle oracle adds `hi_potion` and `hi_potion_capped` (en/ko): 98 cases, and the native `Memoria.BattleCore.Source` passes them.
- **Results** (UE 5.8.2 rendered): `Memoria.BattleCore.` 100/100, full `Memoria.` **392/392**.

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
