# Migration handoff — S346 Verdan's market ring (Claude lane, 2026-10-04)

- **Why.** After S338–S345 the chapter maps were dressed and sounded, while Verdan, the hub the player reaches first, was a bare paved square with two stalls. `verdan_market.gd` sets stalls round its square (STALL tiles, each with a warm PointLight2D), and `map_verdan_market_canvas_v1.png` paints a ring of lantern-lit stalls under wine, slate and moss cloth.
- **What stands now** (`AMemoriaVerdanPresentation::BuildMarketRing`).
  - Ten more stalls of the existing kind: four along the north edge, three against each side curb, none along the south edge (it faces the camera). They keep clear of the story's places (the memory stalls, the old man at the west edge, Malet's table, Elia, the sump stairs) and of the eight edge spots `MemoriaVisual.VerdanExploration` stands Arrel on.
  - Each has a lamp under its canopy, a warm point light (no shadow) and a warm patch on the paving in front of its counter.
  - Codex's S343 kit where it is imported: four lantern posts on the side curbs beside the pillars (each with its light), and two crate stacks between the north stalls. Their material is `M_EnvProp` with the walls' opening round Arrel.
  - Three lanterns hung from the north rope, with their lights.
  - 17 market lights in all, flickering a little like the existing lanterns. The four original lanterns, their two shadows and the fill lights are unchanged.
  - The stalls' bottles share three glass materials instead of one per stall (30 geometry batches).
- **Not changed.** No collision: the presentation owns none, and the ring's stalls can be walked through, as the two story stalls always could. The courtyard, buildings, camera limits, story points and Malet are as they were.
- **Tests.** `MemoriaVisual.VerdanExploration`: 18 roofs (six buildings, twelve stalls), 10 ring stalls, 6 kit props, 17 market lights; the lantern and shadow counts exclude the market lights. Captures read: the square from its centre, the north, the corners, the west and east edges, and at Malet.
- **Known gaps.** Arrel standing just north of a side stall shows over its canopy (the opening follows his line of sight, which passes above the roof). The source's five market NPCs are not placed: they have no models (requested from Codex, 2026-10-04). The smoke wisps from the alleys are not ported.
- **Results.** `MemoriaVisual.VerdanExploration` and `FieldCharacters` pass on this tree. The full registry follows.

# Migration handoff — S345 the chapter maps' sound (Claude lane, 2026-10-04)

- **Why.** The user asked for the work to go on after Codex integrated S344 (`486b6c42` in its branch; nothing asked of Claude). The Belt Waystation and Drift Shelter had no music, no ambience and no footsteps: the audio subsystem knew only Verdan and the title.
- **What the source does** (`scripts/systems/audio_manager.gd`, `scripts/core/player.gd`).
  - `SCENE_AMBIENT` gives `belt_waystation` the light wind and `drift_shelter` the rain (both procedural 3 s loops).
  - `SCENE_BGM` names neither map, so the previous scene's track plays on: Verdan's after the road, the title's after a Continue, the battle theme after a fight.
  - Neither map defines `get_terrain_at`, so `_get_terrain_type` falls back to "grass" and `play_step` plays the plain `step`.
- **Port.**
  - Sources: `generate_audio_sources.py` renders `step` (0.06 s) and `rain` (3 s loop) from the source formulas, seeded per cue as before; the other 26 files are byte-identical.
  - Catalog: cue `step` (-12 dB, pitch ±0.12, step_player); tracks `exploration` (`assets/audio/bgm/exploration.mp3`, -5 dB) and `rain` (-10 dB). `-run=MemoriaAudioAssets` adds `S_Sfx_step`, `S_Bgm_exploration`, `S_Amb_rain` and keeps the rest.
  - `UMemoriaAudioSubsystem::SyncContext`: the Belt plays `wind_light`, Drift `rain`, both under `exploration`; a fight still switches to the battle theme and silences the air, and the map's sound returns after it.
  - **Deviation:** the music. The source's fall-through would carry the title or battle theme into these maps; the port plays the source's own `exploration.mp3` there instead, a track no ported scene uses.
  - `AMemoriaChapterPresentation::Tick` plays `step` on each footfall of Arrel's gait, as Verdan plays `step_stone`.
- **Tests.**
  - `test_audio_sources.py` (host): the new durations and loops.
  - `Memoria.Audio.CatalogAssets` covers the three new packages through the catalog.
  - `MemoriaVisual.BeltDressing` and `DriftDressing`: the exploration track and the map's air are set and playing; Arrel walks 250 units and the step plays (8 and 9 footfalls).
- **Known gaps.** No listening pass: the tests check routing, not how it sounds. The source's low-HP filter, reverb maps and thunder are not ported. Foes, NPCs and Elia make no footsteps.
- **Results.** Full rendered registry 274/274 (`s345_full`), visual 30/30 (`s345_visual`), on `22b42cdc`; host `test_audio_sources` 5/5.

# Migration handoff — S344 Codex's environment kit standing in the chapter maps (Claude lane, 2026-10-03)

- **Why.** S338 built what the map canvases paint from boxes and asked Codex for models. Codex delivered the twelve in S343 (`models/S343_ENVIRONMENT_MANIFEST.md`) and named Claude as the owner of their import and placement. The user asked for the next work to go on once Codex was done.
- **Before this session.** At the user's request the lane was pushed as it stood (`545fecaf`), and Codex's finished S343 integration branch (`codex/unreal-s342-integration-env-s343-20261003`, `360ffff2`) was pushed under its own name. That branch is **not merged into this lane**: the merge was refused by the session's permission check, as the S337 cherry-pick was. This lane therefore still lacks S337's polish assets and S343's four combat repairs (scorch cleanup, smoke escape chain, the charger's wall sweep and rush contact). S344 touches none of the files those repairs changed.
- **Import** (`-run=MemoriaEnvironmentAssets`, `Unreal/ArtSource/Environment`, 27 delivered files copied byte for byte).
  - Twelve static meshes `SM_Env<Name>` under `/Game/Memoria/Presentation/Environment`, by the S335 convention (convert scene and unit, force front X, turn back a quarter, no generated collision). The commandlet refuses a model whose bounds differ from the manifest by more than 1 cm or that does not stand on z 0; all twelve matched exactly, with the delivered triangle counts.
  - The delivery repeats byte-identical colour files; each distinct atlas is imported once: `T_EnvBelt` (the Belt's six), `T_EnvDrift` (tarp, ruin wall, dead tree), `T_EnvSmall` (lantern post, gramophone), `T_EnvGrass`, and the glow masks `T_EnvBeltGlow`, `T_EnvSmallGlow` (linear).
  - One material, `M_EnvProp`: the atlas tinted, a warm glow through the mask, a little of its own colour as fill, two-sided, masked by the atlas's alpha (the grass) and by the opening round Arrel that `M_FocusSurface` has. `-Rebuild` writes it again.
  - Facing and scale were checked in a game view before placing the rest: at yaw 0 a model's front faces the camera (south), and the signal post stands 330 cm beside Arrel's 150.
- **Placement** (`MemoriaChapterEnvironment.cpp`: `Kit`, `Prop`, `Fence`, `Claimed`, `BuildGrass`). The blocks are unchanged: one per solid tile.
  - **Belt Waystation.** The rail line (26 sections) beyond the embankment; the signal post and the banner pole on the embankment; the platform shelter between the embankment and the rails on a stone plinth; a crate stack on each southern ruin; the chain fence on the kerb along the west, south and east, cut into whole lengths that end on a post and broken where the road leaves; three lantern posts; a ruined wall on each remaining ruin tile (two tiles long where two lie together, a stub on a lone one); dry grass on the dead soil. Nine kinds.
  - **Drift Shelter.** Two patched awnings on poles over the back row of the slab ring, one each side of the path, with a lantern on each front pole; a camp beyond the east wall (a full canopy over crates, with a lantern); crate stacks on the north-east rubble and by the west wall; the gramophone on the north-west rubble; ruined walls on the other rubble tiles; dead trees beyond the walls; two lantern posts; thin grass in the mud. Seven kinds. All twelve models stand in one map or the other.
  - **Why no canopy over the shelter's floor.** From this camera a roof at 2 m hides the head of anyone standing more than about 45 cm under it, and the opening in the material follows Arrel only. A canopy over walkable ground would cut the heads off NPCs and foes. The canopies therefore stand over solid tiles or beyond the border.
  - **Lights.** A model's lantern glows through its mask and takes a point light without the old glowing pane (`Lamp(..., bPane)`). The Belt has 8 lamps (the shelter has two lanterns), Drift 7.
  - **What stays built from boxes:** the things the kit has no model for and that stand far off (the Belt's relay pylons and slag ridges, Drift's far ruin masses), the embankment, the kerb, Drift's border walls and the relay house's masonry and rafters.
- **Tests.** `MemoriaVisual.BeltDressing` and `MemoriaVisual.DriftDressing` extended: the kinds of kit model per map (9 and 7), at least 40 placed, the lamp counts, the blocks still one per solid tile; captures added at the Belt's north-west ruin and south-east corner, and at Drift's gramophone, the camp beyond the wall and inside the shelter. Fourteen captures were read.
- **Known gaps.**
  - No collision of their own: a prop wider than its solid tile overhangs walkable ground by up to about 30 cm (the crate stacks, the awnings' front poles), and Arrel can brush through that edge.
  - The fence lengths are stretched or shortened by up to 12% to end on posts.
  - The awnings are the tarp model at 60% depth; the ruin stubs are the wall at 60%.
  - No LODs, no wind on cloth or grass, no normal or roughness maps (as delivered).
  - A source quirk seen on the way and left alone: Drift's chest lies on a rubble tile (17, 3), which is solid in the source too.
- **Results.** Full rendered registry 274/274 (`s344_full`), visual 30/30 (`s344_visual`), on `277b92e4`.

# Migration handoff — S342 the maps' encounter pools as foes of their own (Claude lane, 2026-10-02)

- **Why.** Second gameplay step after S341. Every fight in the field was against one of two foes that fight the same way (walk up, wind up, strike): the husk and the thief. The Belt Waystation's and Drift Shelter's pools name six different enemies with their own HP, attack and abilities; the port spawned husks for the void ones and thieves for the rest, whatever their names.
- **The six** (`FoeSpec`, `EMemoriaFoeKind`), each by its source name, HP and attack (`belt_waystation.gd`, `drift_shelter.gd` `RandomEncounter.setup`), Korean names from `GameManager.ENEMY_NAMES_KO`:

| Foe | Source HP / atk | Here | Fights as | Its blow |
|---|---|---|---|---|
| Belt Scavenger (벨트 약탈자) | 55 / 12 | 50 HP, 7 | brawler, quick | weakens |
| Void Wisp (보이드 도깨비불) | 45 / 14, void | 40 HP, 8 | **caster** | drains |
| Dust Crawler (먼지 크롤러) | 40 / 10 | 36 HP, 6 | **charger**, pack of three | poisons |
| Memory Leech (기억 거머리) | 50 / 13, void | 45 HP, 8 | brawler | drains |
| Rubble Rat (잔해쥐) | 35 / 9 | 32 HP, 5 | **charger**, pack of three | poisons |
| Ash Walker (재의 방랑자) | 60 / 11 | 55 HP, 8 | brawler, slow | scorches and weakens |

  - Health is near the source's HP and the blow about 0.6 of its attack, as the thief's 50 and 12 became 45 and 7. These are first-pass numbers for the user's tuning.
- **Two new ways to fight** (`AMemoriaFieldMonster`).
  - **Caster.** It closes to 430 units, backs away when Arrel comes within 230, and from between casts a slow orb (480 units a second, 2.4 s) at where he stands. The orb can be stepped out of, dodged through, guarded or parried; it fades if its thrower is dead when it arrives.
  - **Charger.** Within 380 units it commits to a lane toward Arrel, shown as a red lane on the ground for the windup, then runs 520 units down it and strikes him once if he is still in it. Walls stop it; Arrel and the other foes do not.
- **Two new abilities** (`battle_manager.gd`).
  - **Drain:** the foe gets back half the harm its blow did.
  - **Scorch** (`burn_attack`): two turns of attack × 0.2 + 3 on Arrel, never felling him, like the poison. The antidote ends it, as the source's cures poison and burn.
- **Telling them apart.** There are two foe models. A pool foe wears one of them at its own height with its own crack glow and rim colour (`FMemoriaFoeLook::bRetint`), and its name stands over its health bar. The husk and the thief are unchanged.
- **Where they rise** (`AMemoriaChapterPresentation`). A random encounter spawns the pool entry's own foe in its pack (three chargers, two of the others); a one-time battle area spawns one fewer. The bestiary records the source's HP and attack for them. Verdan's pool is unchanged.
- **Tests.** `MemoriaVisual.FoeVariety` (new, registered).
  - Every pool entry of both maps finds a foe of its own with the source's HP, attack and kind; an unknown name falls back by its kind.
  - The wisp casts from its range and never closes; its orb does the wisp's harm; the next one is stepped out of; a wounded wisp's blow gives it back half the harm.
  - The crawler commits to a lane pointing at Arrel, its rush strikes him once and poisons him, and it runs on past.
  - The ash walker's blow scorches and weakens; the scorch bites; the antidote ends the scorch and the poison.
  - Captures read: the six in a row, the orb in flight, the lane, the rush, the scorch. The Belt's revisit capture now shows two Belt Scavengers under their name.
- **Known gaps.**
  - Six foes on two models: they differ by size, glow and name, not by shape. New models would be a request to Codex (the Belt's and Drift's pools were listed as not requested in the S331 art request).
  - An orb passes through walls.
  - The foes do not avoid each other, and a charger can rush out of the lit part of a map before it turns back.
  - No numbers are tuned by play yet.
- **Results.** Full rendered registry 274/274 (`s342_full`), visual 30/30 (`s342_visual`), on this tree; they cover S341 and S342 together.

# Migration handoff — S341 items used in the field (Claude lane, 2026-10-02)

- **Why.** The user asked for gameplay work to continue after the field finish (S338–S340). The plainest hole: items dropped, were counted in the HUD, and could not be used. The turn-based battle that used them was retired in S329 and nothing took its place.
- **Rules** (`GameManager.ITEMS`, `battle_manager.gd player_use_item`), in `MemoriaCombatTuning::FieldItems`:
  - `potion` restores 40 HP, `hi_potion` 80.
  - `antidote` ends the poison and restores 12 HP.
  - `firebomb` does 12 where it bursts, then burns for 15 a turn for two turns. In the field it is thrown toward the cursor (up to 620 units, 0.38 s in the air) and reaches every foe within 190 units; the source's battle had one foe.
  - `smoke_bomb` is a sure escape: the foes are gone and the fight pays nothing (the source ends the battle as fled).
  - `witness_ink`: the source advances WITNESS, guards the next blow and adds Limit. Only the guard exists in the field, so here it turns the next unguarded blow aside. A deviation, recorded.
- **Use** (`UMemoriaFieldCombatSubsystem::UseQuickItem`, `UseItem`).
  - Five quick slots on **Z X C V B**: healing, antidote, firebomb, smoke bomb, witness ink. The healing slot takes the potion, or the hi-potion once 60 HP or more is missing, or whichever is left.
  - An item that would do nothing is kept and a word over Arrel says why: a potion at full HP, an antidote with nothing to cure, a smoke bomb with no foe, ink over a ward that still holds.
  - 0.8 s between two items. Not while down, casting a burn or choosing one.
  - A used item is recorded (`item:used:<id>` in the trace, the achievements' items-used count) and leaves the inventory at zero (`UMemoriaRunSubsystem::ConsumeItem`, the source's `remove_item`).
- **On screen.** A tray at the bottom right of the field HUD: each slot's icon (the source's `assets/ui/items`, through `export_hud_art.py` and `-run=MemoriaHudAssets`), its key and its count; an empty slot is dim, and the tray darkens during the wait. The bomb flies as a hot point on a low arc with its landing ring drawn; healing, the ward and the smoke leave their rings and light (S340's marks).
- **Tests.** `MemoriaVisual.FieldItems` (new, registered): the source's numbers; each refusal spends nothing; the potion restores only what is missing; the wait between items; the antidote cures and its entry leaves the inventory; the hi-potion for a deep wound; the ink turns one blow aside; the firebomb's 12 on impact and 15 twice; the smoke bomb ends the fight unpaid; six items used. Captures read: the tray, the heal, the bomb in flight and its burst, the smoke.
- **Known gaps.**
  - The quick slots are fixed. The source's Quick Kit lets the player pin three items from the pause menu's inventory, which is not ported.
  - Gamepad buttons for the slots are not bound.
  - The other source items (lantern salve, root balm, signal jammer and the rest) are not in the drop table and have no effect here.
- **Results.** `MemoriaVisual.FieldItems` and `MemoriaVisual.FieldRewards` pass on this tree. The full registry runs at the end of S342.

# Migration handoff — S340 blows that land, and NPCs that live a little (Claude lane, 2026-10-02)

- **Why.** Third of the three steps agreed with the user (see S338). After the maps and the HUD, what still read as unfinished was movement: the ambient NPCs stood like posts, and a blow showed only sparks and a number.
- **Blows** (`UMemoriaFieldCombatSubsystem`, `UMemoriaCombatHudWidget`).
  - **A mark where it lands.** Every blow that lands (Arrel's cuts, Elia's remembered strike, a parry, a block, a wound to Arrel) leaves a ring that opens and fades at the point of impact. A heavy cut, a parry and a kill also throw a cross of light.
  - **Light.** The blow lights the ground and the figures round it for an instant: one point light, moved to each blow, fading over 0.18 s. In the dark maps of S338 this is the strongest part of the change.
  - **A kill** breaks the foe open in its own colour (a second burst and a wide ring), and the corpse sinks out of the world over its last 0.9 s instead of vanishing.
  - **Numbers** land large and settle; a heavy or killing blow's number stays larger and brighter.
  - **A wound** reddens the screen's edge for half a second; under a third of his HP the edge keeps a slow pulse.
  - The marks and the light run on real time, so they open during the hit stop.
- **Ambient NPCs** (`AMemoriaChapterPresentation::TickAmbientNpcs`).
  - Each idles, strolls at an unhurried walk to a spot within 150 units of where the source stands it, and idles again. The way must be open ground, clear of the water tank, and not on top of Arrel.
  - When Arrel comes within 170 units it stops and turns to face him. While a fight is on it stands and watches the nearest foe.
  - The rigged models turn freely and walk with the retargeted walk S335 imported; the pixel cards (the fallback) keep their four facings.
  - This is the port's own: the source's ambient NPCs are still sprites with no behaviour. Positions at the start, visibility and the gate are unchanged.
- **Tests.**
  - `MemoriaVisual.ChapterRevisit`: left alone for eleven seconds the NPCs have walked, each is within reach of its place and on open ground; Arrel then stands beside the guard and it turns to face him (new capture `RevisitNpcs`).
  - `MemoriaVisual.FieldFeel`: a landed blow leaves a mark and a flash of light.
  - Captures read: the hit flash lighting the paving, the parry's ring and cross, the guard turned to Arrel and the bureau agent mid-stride.
- **Known gaps.**
  - NPCs do not avoid each other or Elia, and they have nothing to say.
  - Drift Shelter's NPC presets (villager_f, scholar, villager_m) still have no models or cards, so no one stands there; the code would move them as it does the Belt's.
  - The effects are drawn by the HUD and one light; there is no particle system.
- **Results.** Full rendered registry 274/274 (`s340_full`), visual 28/28 (`s340_visual`), on this tree; they cover S338, S339 and S340 together.

# Migration handoff — S339 the field HUD on the source's plates (Claude lane, 2026-10-02)

- **Why.** Second of the three steps agreed with the user (see S338). The field HUD read as a debug overlay: a plain box at the top left holding the place name, a control hint, the interaction prompt and every toast; a plain status panel; a bare HP bar with four text slots and a line of control hints under it.
- **The source's art.** The Godot game has paintings made for exactly these panels: `ui_exploration_hud_plate.png` (exploration_hud.gd), `ui_notification_toast_frame.png` (notification_toast.gd) and `ui_battle_command_ribbon.png` (battle_scene.gd). It lays them behind its panels with their black ground.
  - `Unreal/Tools/export_hud_art.py` crops each to its plate and makes the ground outside it transparent (`Unreal/ArtSource/Hud`, `--check`). The frames are thin lines with gaps and the panel inside is as dark as the ground outside, so the lines are thickened into a closed wall before the flood and the flooded region grown back afterwards.
  - `-run=MemoriaHudAssets` imports them to `Content/Memoria/Presentation/Hud` (`-Refresh` imports again).
- **What changed on screen.**
  - **Status, top right** (`UMemoriaExplorationHudWidget`): drawn on the plate. The HP gauge (with its trailing ghost) sits in the plate's slot, each row stands over its rule beside a small icon in the plate's frame. The rows' text is unchanged (tests read it).
  - **Place, top left** (`UMemoriaFieldHudWidget`, new): the place and its subtitle on the toast frame, in the run's language ("벨트 중간역 / 페이지의 무게"; it was English capitals in every language). Under it the quest's line and the toasts, as chips. The interaction prompt is a chip over the bottom centre, with the name, a key cap and the verb. The control hint is a faint line at the bottom left.
  - `AMemoriaSliceController::StatusWidget` is now this widget. It was a compact view of the development narrative widget.
  - **Combat, bottom centre** (`UMemoriaCombatHudWidget`): the command ribbon. Its seven cells hold Elia's four techniques (key, name on two lines, cooldown draining from the cell), the burn, the guard and the dodge; the last three light while they are in use. The left orb holds Arrel's HP with a ring that empties, the right orb the memories he still holds. The HP gauge stands over the ribbon, the statuses beside it. The control line under the bar is gone: the cells carry the keys, and the attack's hint stands over the gauge during a fight.
  - **Moved to make room:** the tutorial hint now takes the band between the two top plates (it overlapped the status panel before this session too); the achievement popup comes to rest under the status plate.
- **Shared code.** `MemoriaHudKit` (plates, image, text, gauge, ring, diamond). Each widget falls back to a plain panel if its plate is not imported.
- **Tests.** The two dressing tests now also check that the status panel has its plate, the combat bar its ribbon, and that the field HUD names the place and subtitle in the run's language.
  - Captures read: the Belt and Drift with toasts; Verdan with the prompt at Malet; a fight with the ribbon, statuses and unlocked techniques; the victory panel; the tutorial hint and the achievement popup side by side.
- **Known gaps.**
  - The victory panel, the burn picker and the defeat veil keep their plain panels.
  - The orbs show a number and a ring, not a filling sphere.
  - Interaction prompts are still authored in English in code ("Malet | E / A: talk").
  - The memory orb's ring is full at twelve memories, a number chosen for the ring and not taken from the source.
- **Results.** Visual tests run on this tree and passing: BeltDressing, DriftDressing, ChapterRevisit, Chapter4, FieldCombat, EliaSkills, FieldBurn, FieldRewards, VerdanExploration, TutorialHints, Achievements, PauseMenu, Journal, GameOver. The full registry runs at the end of S340.

# Migration handoff — S338 the chapter maps dressed (Claude lane, 2026-10-02)

- **Why.** The user's words: the game "still feels like a prototype". The story scenes read as finished; the field of Chapters 3 and 4 was flat single-colour tiles, a building of black boxes and cube rubble. Order agreed with the user: the maps (this session), the field HUD (S339), NPC movement and hit effects (S340).
- **Numbering.** Codex used S337 in its own lane on the same day (`7922b395`, "polish ambient materials and chapter terrain"). That commit is not in this lane: bringing it in was refused by the session's permission check, so it waits for the user or for Codex's integration. This session replaces the terrain that S337 polished; see "For integration" below.
- **The source's art direction.** The Godot maps do not show their tiles. Each hides them (`terrain_alpha 0.0`) under one painted canvas (`MapEffects.add_map_canvas`, `assets/environment/map_canvases/map_belt_waystation_canvas_v1.png` and `map_drift_shelter_canvas_v2.png`). A flat painting cannot lie under the quarter-view camera (its props would lie flat), so the port builds what the canvas paints.
- **Ground.**
  - `Unreal/Tools/export_chapter_ground.py` cuts open ground from the canvases (a soil and a paved patch per map, made to repeat) and the Belt's round dial, into `Unreal/ArtSource/ChapterGround` (`--check`).
  - `-run=MemoriaChapterGroundAssets` imports them to `Content/Memoria/Presentation/Chapter` and authors `M_ChapterGround` (`-Rebuild` writes the material again).
  - The whole ground, and the land around the map, is one surface. The tile grid reaches the material as a mask built at run time (paved, interior floor, building walls); noise pushes the mask's edges about so roads end in a worn line and not along the grid. The material adds relief from the painting's light and dark, timber planks or cooled paving indoors, grime at walls, puddles in Drift, and the dial as an inlay before the relay house's door.
- **Walls and border** (`MemoriaChapterEnvironment.cpp`: the class's terrain, light and air, moved out of the main file).
  - The relay house: masonry on a plinth with capstones, corners and door jambs tall, the runs between worn down, a timber lintel, wall plates and rafters (two fallen in), a lantern at each side of the door. Walls between the camera and Arrel open round him (`M_FocusSurface`, as in Verdan).
  - Ruin tiles: a wall stub and Codex's rubble model in the wall's stone. Drift's concrete ring: low blocks out of true.
  - The Belt's border: the rail line's embankment along the north, a worn kerb with stone posts and chains on the other sides. Drift's: broken walls, high at the back and low at the front.
- **Stand-ins for the canvas's props,** built from boxes, each on a solid tile or beyond the border so none is in Arrel's way.
  - The Belt: the rail line with sleepers, relay pylons and far slag ridges, the signal post with its wheel and pennants, the platform shelter, the banner pole, freight on two ruins, lamp posts at the road's ends.
  - Drift: the tarp on four poles with lanterns, dead trees and far ruin masses, stores on the rubble, two lamp posts.
  - Codex was asked for models of twelve of these (`claude-handoff.md`, 2026-10-02).
- **Light and air.**
  - Found: a spawned directional light keeps its class's own downward tilt under the rotation it is spawned with. The key light had fallen almost straight down since S320 and every wall face was black (Codex's S337 report notes the same dark faces). Each light is now turned after spawning: the Belt's key is low and warm from the south-west, Drift's is a cold back light.
  - Two faint shadowless lights (from above, and along the camera's line) stand in for the sky. A sky light was tried and removed: its capture arrived late in some sessions and washed the map out in others.
  - Lamps flicker; the Belt carries dust and Drift rain, in a box that travels with Arrel; a vignette and bloom on the lens; fog thinner than before.
- **Unchanged.** Where Arrel can walk: the blocks are still one per solid tile, built first and unseen. Story, saves, encounters, markers, decorations and NPCs are as they were.
- **Tests.** `MemoriaVisual.BeltDressing` and `MemoriaVisual.DriftDressing` (new, registered in `validate_unreal.py`).
  - Each plays the arrival chain, checks the blocks, the painted ground, the lamp count (7 and 6) and the air, then stands Arrel at six and five places and captures each (`Saved/Validation/ChapterDressing`).
  - Captures read: the rail and platform, the signal post, the freight, the door and its dial, the interior, the exit; Drift's tarp, stores, south, east and west.
- **For integration (Codex).**
  - Your S337 `BuildTerrain` and `BuildLight` changes conflict with this session, which replaces both.
  - This session's code uses your S337 assets where they exist: `SM_BeveledBlock` for walls and slabs and `M_StoneSurface` for the slab ring, with a cube and the masonry material as the fallback (this lane has neither asset).
  - Your six polished Field3D materials are untouched by this session.
  - `CheckChapterVisualPolish` should still find the stone material, the bevelled block and the rubble model after integration; it was not run here.
- **Known gaps.**
  - The stand-ins are boxes. The tarp is the weakest: three flat sheets.
  - The soil patch is 192 px of canvas repeated. A slow noise trades two takes of it and lets old paving show through, but a keen eye finds the repeat.
  - Verdan is unchanged (it already had its own environment).
- **Results.** Seven chapter-map visual tests pass on this tree (Chapter3, Chapter3Travel, Chapter4, ChapterContinue, ChapterRevisit, FieldEncounters, and the two new ones). The full registry runs at the end of S340.

# Migration handoff — S336 follow-up: Codex integrated S310–S335; two records corrected (Claude lane, 2026-10-02)

- **Where things stand.** Codex re-reviewed S334, accepted both save fixes and integrated S310–S335 in its lane (`codex/unreal-s335-integration-20261002`, gameplay at `c2afd34b`). Its fresh run there: build, full 274/274, visual 26/26, host 35/35 (`codex-review.md`, S336). No code changed in this session; this entry is documentation only.
- **Correction: capture sizes.** S335 wrote that captures are 1280×720. That holds for the tests that capture through the game viewport (125 files in this lane's validation folder). The older full-registry replay tests capture the play window with its 34 px title bar and come out at 1280×754 (946 files). Both sizes are stable: the shrinking that S335 fixed is gone, but "every capture is 1280×720" was too broad.
- **Cause found: the 15 `LogAutomationTest: Error: Condition failed` lines at startup.** Codex recorded them as unresolved. They are the engine's own Core smoke tests, not Memoria's.
  - `Engine/Source/Runtime/Core/Tests/Experimental/UnifiedError/UnifiedErrorTests.cpp` compares error messages with English text. This machine runs the editor in Korean (`Using OS detected language (ko-KR)`), where the same messages are translated (`[빈 오류]`), so 15 comparisons fail.
  - Checked by running the editor twice without graphics on the same test: 15 lines in Korean, 0 with `-culture=en`. The lines have been in every automation log since 2026-09-26. Commandlets do not run the smoke tests and show none.
  - Nothing was changed. Forcing English in the validation tool would also change the language the game's own tests run in, so the lines stay and are now explained.

# Migration handoff — S335 Codex's ambient NPC models and props adopted (Claude lane, 2026-10-01)

- **Why.** Codex delivered the art requested after S331 (shared `models/`, S334): three NPCs on Arrel's 77-bone rig and three static props. This session imports them and puts them in the chapter maps.
- **Source files.** Copied to `Unreal/ArtSource/FieldCharacters/npc_traveler`, `npc_bureau_agent`, `npc_guard` and `Unreal/ArtSource/FieldProps`. All 12 files match the SHA-256 values of `models/_qa/s334/final_delivery_audit.json`.
- **Import.** New `-run=MemoriaAmbientModels` (additive; run after `install_mannequin.py`).
  - NPCs, under `Field3D/Traveler`, `Bureauagent` and `Guard`: the skeletal mesh and its skeleton, the base colour, a material in the leads' S310 style, and an idle and a walk retargeted from Epic's mannequin (`MM_Idle`, `MF_Unarmed_Walk_Fwd`). They are named as the field figure looks for them (`A_<Name>_Idle`, `A_<Name>_Walk`), so `InitializeCharacter` finds the model before the pixel card with no change to the figure's code. The import checks 77 bones; heights came in at 174, 176 and 178 cm.
  - Props, under `Field3D/Props`: `SM_WaterTank`, `SM_Campfire`, `SM_Rubble`, each with its texture and material. Sizes came in as the manifest states.
  - The props keep the leads' small fill. Without it the tank's shadowed side went black (tried, read in a capture, and reverted).
- **In the maps** (`AMemoriaChapterPresentation`).
  - The ambient NPCs are the models, scaled to Arrel's height, standing idle. The pixel cards from S331 remain as the fallback when the models are not imported.
  - The water tank is the model on the tank's ground spot. It carries its own 3-degree lean, so the source's rotation is not applied on top. The old cylinder stays hidden as the block.
  - The campfire is the model, with the ember block small in its centre and the source's light above it.
  - The rubble heaps are the model, each at a random turn.
  - The road cracks stay as they were: no model was requested for them.
- **The capture window.** The editor saves the play window's size on every close, a title bar smaller each time, so captures shrank through a suite and from run to run (the saved size had reached 1208×240). Under `-MemoriaCapture` the test module now puts the size back to 1280×720 on every frame between play sessions. Eleven captures in one editor session all came out at 1280×720. This closes the known gap that earlier sessions recorded as the capture window shrinking.
- **Tests.**
  - `MemoriaVisual.ChapterRevisit`: the tank is a model and the three NPCs are rigged.
  - `MemoriaVisual.Chapter4`: Drift Shelter's nine props are models; two new captures (the campfire, a rubble heap).
  - Captures read at 1280×720: the Belt with the tank and the three NPCs; the encounter; Drift Shelter's campfire with its glow; the rubble.
- **Known gaps.**
  - The NPCs only stand. The retargeted walk exists because the figure requires one; nothing moves them.
  - The rubble model is light concrete and small beside the terrain's dark placeholder stones (the scattered cubes on ruin and rubble tiles). Those stones could take the same model; that is a terrain change and was not done here.
  - The bureau agent stands inside the waystation's walls, as the source places him, and is partly hidden by them from the quarter view.
  - Not delivered: the alley rat (Codex deferred it again).
- **Results.** Full rendered registry 274/274 (s335_full), visual 26/26 (s335_visual), covering S334 and S335 together. All 104 captures of the visual run are 1280×720.

# Migration handoff — S334 Codex's review of S330: two save-safety fixes (Claude lane, 2026-10-01)

- **Why.** Codex reviewed S330–S333 (`codex-review.md`, S334) and withheld integration for two findings in S330's map save. Both were reproduced with new tests before any fix, then fixed.
- **P1: a refused map Continue entered the chapter anyway.**
  - *Reproduced.* A live Chapter 3 run in the Belt Waystation travels to Drift Shelter with `?Continue` while no map save exists. Before the fix the game entered Drift Shelter and the run's chapter rose from 3 to 4.
  - *Cause.* `AMemoriaSliceGameMode::StartPlay` handled the refusal only when no run was live. With a live run it fell through to `EnterChapterMap`, which raises the chapter, resets the story's context and spawns the map's presentation.
  - *Fix.* A refused `ContinueChapterMap` never goes on into the chapter, with a live run or without one. The game returns to the title, and the title's footer says the load failed (`UMemoriaTitleWidget::SetLoadFailed`, from `UMemoriaNarrativeSubsystem::ConsumeMapLoadFailure`). A live run stays as it was.
  - *Limit.* By then the previous level is gone, so Arrel's place in it is not kept: a live run reaches the title as it does through the pause menu's Title. The pause menu's Load and the title's Continue both validate the slot before they travel, so this needs the file to fail between that check and the arrival.
  - *Not covered by a rendered test:* the same refusal with no live run. The branch is now the same one; a play session always has a run by the time a test can travel.
- **P2: a map save could carry data the restore drops.**
  - *Reproduced.* Hand-framed map saves holding a diary schema, a diary body, a hints schema, a hints body, or a ledger count on an inactive flow were each validated, offered by Continue and restored.
  - *Cause.* `ValidateMapSnapshot` checked none of them, and `RestoreSave` carries only the run, the memories and world cognition.
  - *Fix.* `ValidateMapSnapshot` refuses all five, as the Verdan boundary slot does.
- **Stale host tests** (Codex's note: six modules imported `battle_entry_test_paths`, removed in S329).
  - The two Codex repaired in its lane (`test_checkpoint_tools.py`, `test_shop_transaction_tools.py`) are adopted here byte for byte.
  - The other four (`test_malet_antidote_tools.py`, `_firebomb_`, `_potion_`, `_world_seed_`) drop the import and the frozen registry subtraction; each now requires its own suite's identities in the current registry.
  - Those four also held source-shape checks that later sessions had made stale: field pickups add items in `GrantFieldItem` (S320) and recent items go through `RecordRecentItem`. The checks now look at `GrantRewardItem`'s body, which is the reward's one shared mutation. One module read sources without naming UTF-8.
  - All six pass: 35 host tests.
- **Tests.**
  - New `Memoria.Checkpoint.MapContinueRefused`: the P1 scenario through `StartPlay`. The chapter is not entered; the game is at the title with the failure shown and Continue dark; the run's id, chapter, flags, memories, erosion, grains and items are unchanged.
  - `Memoria.Checkpoint.ChapterMap`: a hand-framed map save is accepted unchanged, and each of the five unsupported fields alone is refused by validation, by Continue and by restore.
  - Both failed before the fixes and pass after. Capture read at 1280×720: the title with the failure in its footer.
- **Found on the way.** The capture window was small because the editor saves the play window's size on every close and it creeps smaller (it had reached 1208×240 in `Saved/Config/WindowsEditor/EditorPerProjectUserSettings.ini`). Resetting `NewWindowWidth` and `NewWindowHeight` before a run gives 1280×720 captures. S335 puts that reset into the validation tool.
- **Results.** Validated together with S335 (see its results).

# Migration handoff — S333 the turn-based battle's leftover content removed (Claude lane, 2026-10-01)

- **Why.** The fourth of the remaining items. S329 retired the turn-based battle's code and kept its art as unused content. The user asked for that content to be cleared.
- **Removed** (17 files, about 17 MB, all recoverable from history):
  - Content under `Content/Memoria/Presentation/BattleEntry`: `M_BattlePlate`, `T_ArrelBattle`, `T_AlleyRatLegacy`, `T_AlleyRatStudy`, `T_MarketThiefSource`, `T_MarketThiefStudy`, `T_UiCommandDeck`, `T_UiFieldReadout`, `T_UiTacticalPlate`, `T_UiVictoryPanel`. No other asset or source file referenced them.
  - Their rows in `MemoriaBattleEntryArt`, the three study-source helpers, and the commandlet's `-StudyOnly` and `-AlleyRatStudyOnly` switches.
  - `Unreal/ArtSource/BattleEntry` (the two study drawings and their provenance) and the fixture `docs/unreal-migration/fixtures/battle_entry_art/market_thief.png`.
  - The two tools that only produced those: `export_battle_entry_art.py` and `author_battle_entry_assets.py`.
- **Kept.**
  - `T_VerdanMarket` and `T_EliaAnchor`: the dialogue's art table reuses them from this folder.
  - The seven UI plates (pause, game over, hint, journal, codex, achievements).
  - The names `MemoriaBattleEntryArt` and `-run=MemoriaBattleEntryAssets`. The table is now only UI art; renaming it and moving its packages is a refactor of its own.
  - The Godot source images under `assets/`, which the port never owned.
  - `validate_battle_entry_preservation.py` and `archive_battle_entry_execution.py`, session tools of S291 like the other sessions' archive tools.
- **Check.** `-run=MemoriaBattleEntryAssets -CheckOnly` loads the nine remaining textures.
- **Known stale, older than this session.** `test_checkpoint_tools.py` and `test_shop_transaction_tools.py` import `battle_entry_test_paths`, which S329 removed from `validate_unreal.py`. They are not part of the validation run. They need their registry arithmetic rewritten or to be retired.
- **Results.** Full rendered registry 273/273 (s333_full), visual 26/26 (s333_visual). This run covers S332 and S333 together.

# Migration handoff — S332 the journal's Quests and Losses tabs and its illustrations (Claude lane, 2026-10-01)

- **Why.** The third of the remaining gaps: `story_journal.gd`'s Quests, Losses and Leads tabs, and the pictures its entries name.
- **The IR** (`export_journal.py`) gains `quests` (`SideQuest.QUESTS` without the rewards), `loss_rules` (`WorldRewriteDirector.MEMORY_REWRITE_RULES`) and `loss_default_lines` in both languages (`DEFAULT_LINES`, by grade).
- **Quests** (`MemoriaJournal::Quests`, `QuestStatus`, `QuestRecords`).
  - The status is `SideQuest.get_all_quests`': complete on the last step's flag, active once started, available from the quest's chapter and prerequisite flag, else locked.
  - The tab lists `_populate_quests`' rows: `[NEW]`, the active quest with its current step, `[DONE]`.
  - **Only quests on maps the port has are listed.** Echoes in the Ash is available to the source from Chapter 2, but Rim Forest is not a field map here, and the journal should not send the player somewhere that cannot be reached. On the port's route that leaves the Sump Ledger.
  - In Korean an available quest ends with its first step's own text; the source builds an English sentence there. An active quest's description is localized too (the source leaves it in English).
- **Losses** (`MemoriaJournal::LossRecords`): `WorldRewriteDirector.get_loss_records`, one record per burned memory and per faded one, in the archive's order.
  - A memory with a rule takes the rule's title, line, compass reading, colour, picture and story flag; any other takes its grade's default line and colour, the compass line `Lost:` or `Eroding:` with the memory's source title, and the grade's fallback picture.
  - A faded memory's record carries the source's `Fading:` prefixes. With S330's erosion, the two starting Grade 5 memories fade on arriving in Chapter 4 and appear here.
  - The source's record body is English in both locales. The port translates its labels and the grade name in Korean; the compass reading and the story flag stay as authored.
  - The summary line now counts the losses, as the source's does.
- **Leads: not ported.** The source's tab counts untouched caches, relics and resonance points per map (`WorldPopulation`, `MemoryResonance`). None of those exist in the port's world, so the tab would list things that cannot be found. It should come with those systems.
- **Illustrations.** `MemoriaJournal::ArtSources` is the journal's own table: 17 pictures no dialogue shows (16 new textures under `Content/Memoria/Presentation/Journal`, about 42 MB, and the Verdan canvas reused). `-run=MemoriaDialogueAssets` imports them by the same additive rule as the dialogue's.
  - Chosen for the port's route: the archive plates and map canvases of the Chapter 2 to 4 entries, the Sump Ledger's picture, the rewrite pictures for the memories Arrel can hold there, and the two fallbacks.
  - Left out: pictures of entries whose flags the canon route never sets (the legacy ten-chapter route and Chapters 6 on).
  - `MemoriaJournal::LoadArt` looks in the dialogue's table, the UI plates and then this table.
- **The screen.** Six tabs. The detail's picture now gives up height so a long body shows whole (the source scrolls it; a loss record runs to a dozen lines).
- **Tests.**
  - New `Memoria.Journal.Records`: the quest table against the source, the four statuses, the port-map filter, the rows in both languages; loss records by rule, by default and fading; every picture of the journal's table loads.
  - `MemoriaVisual.Journal` goes on through the two new tabs: the Losses tab holds the burn of the road to Verdan, and in Chapter 3 the Quests tab offers the Sump Ledger.
  - Captures read: both tabs in Korean with their pictures; the loss record's last line shows.
- **Correction to S331.** `docs/unreal-migration/evidence/` is ignored by git, so the ambient NPC export's harness and hashes are local evidence only. The tool and the exported sprites are committed.
- **Results.** Validated together with S333 (see its results).

# Migration handoff — S331 the chapter maps' props, ambient NPCs and revisit encounters (Claude lane, 2026-10-01)

- **Why.** The second of the remaining Chapter 3–5 gaps: `_setup_map_decorations` and `_setup_random_encounters` of `belt_waystation.gd` and `drift_shelter.gd`. S330 made a closed chapter's map reachable in play (a save loaded there), so its revisit content now has a way to be seen.
- **The IR** (`export_chapter_maps.py`) gains:
  - `decorations`: every `ColorRect` and `PointLight2D` the function adds outside a gate, with its loops evaluated (kind is the script's variable name). The Belt has the leaning water tank and three road cracks; Drift Shelter has the campfire, its light and eight rubble heaps.
  - `ambient_npcs` and `ambient_npcs_gate`: the three presets per map and the flag that shows them (`ch3_complete`; Drift Shelter's `_can_resume_ch4_exploration`, whose blocked flags join `resume_blocked`).
  - `encounter_range`: `RandomEncounter.setup`'s `min_steps` and `max_steps` (50 and 90 on both maps).
- **Props** (`AMemoriaChapterPresentation::BuildDecorations`). The source lays flat translucent rectangles under the actors; here each stands as a simple prop:
  - the tank, a cylinder as wide as its rectangle and as tall as the rectangle is long, leaning by the source's rotation. It is the one prop that blocks, and its colour is lifted so it does not read as a black mass;
  - the cracks, dark seams on the road;
  - the campfire, a bright ember block, and its `PointLight2D` as a point light;
  - the rubble, low stones.
- **Ambient NPCs.** The source draws them with `PixelSprite.create_npc_sprite`.
  - New `Unreal/Tools/export_ambient_npcs.py` runs that painter unchanged in an isolated Godot project (`_draw_character` and what it calls, extracted from `pixel_sprite.gd`) and saves each preset's four idle views to `Unreal/ArtSource/Ambient`, with the harness and hashes under `docs/unreal-migration/evidence/ambient_npcs`.
  - Exported: traveler, bureau_agent, guard (the Belt's). Drift Shelter's presets are not: its gate cannot open on the canon route, and two of the three (`villager_f`, `villager_m`) resolve to authored field sprites in the source.
  - The sprites join the Verdan art table (`MemoriaVerdanArt`) and are imported by `-run=MemoriaVerdanAssets -AddAmbient` (additive, point-sampled; 12 textures and 12 sprites).
  - They stand still at their tiles, hidden until their gate opens. `MemoriaVerdanArt::LoadSprite` now returns null for a name outside the table instead of logging a failed load (a figure without walk frames).
- **Revisit encounters** (`AMemoriaChapterPresentation::UpdateEncounters`).
  - `FMemoriaEncounterModel` takes a map's own range and pool size (`MinSteps`, `MaxSteps`, `PoolSize`); the defaults are Verdan's, so its behaviour and its oracle are unchanged.
  - Once the map's encounter gate opens, walking fills the model: the warning notice at 72%, then the foes rise in the field. Void entries come as three husks, the others as two thieves, as in Verdan. The battle statistic counts, and a fight holds the distance.
  - On the canon route this is the Belt Waystation after Chapter 3. Drift Shelter's gate is closed by the later canon flags, as in the source.
- **Tests.** New `MemoriaVisual.ChapterRevisit`: both maps' IR against the scripts' values; the props built; no NPCs and no encounters on the first visit; on the revisit the three NPCs, the model's range and pool, the Korean warning before the foes, one battle counted, and no second encounter during the fight.
  - Captures read: the Belt with its tank, cracks and NPCs; the encounter's husks; Drift Shelter's campfire and light (`Chapter4Field`).
- **Known gaps.**
  - `WorldPopulation.populate` (caches, curios, voices, hunts) and the world atlas gateways are separate systems and are not ported.
  - The dust and ash particles of `_setup_map_decorations` are not ported.
  - The ambient NPCs are the source's 48 px pixel figures on cards, beside rigged 3D leads. Models for them would be a Codex art request.
  - The Belt's pool names (Belt Scavenger, Void Wisp, Dust Crawler) map onto the two field foes; their own stats and abilities are not carried.
  - The chapter title card runs on the screen's clock, so a fixed-step test must wait for it to close before a capture.
- **Results.** Full rendered registry 272/272 (s331_full), visual 26/26 (s331_visual).

# Migration handoff — S330 chapter memories and saving in the chapter maps (Claude lane, 2026-10-01)

- **Why.** The user asked for the remaining Chapter 3–5 gaps in order. This is the first: the memories a chapter brings, and the chapter-transition autosave. Burning memories is the game's core, and Chapters 3 and 4 brought none.
- **Chapter memories** (`memory_manager.gd add_chapter_memories`).
  - New exporter `Unreal/Tools/export_chapter_memories.py` reads the `match chapter:` cases for Chapters 3 to 5 and their `MEMORY_TEXT_KO` rows. It writes `docs/unreal-migration/ir/chapter_memories.v1.json` and `Private/Domain/MemoriaChapterMemorySources.inl` (`--check` reports stale output).
  - `MemoriaChapterMemories::For(Chapter)` and `::Korean(Id, Out)` serve that table.
  - `UMemoriaPlayerMemoryDomain::AddChapterMemories` is the source function: the once-per-chapter bookkeeping (`AdvanceChapter`: erosion from Chapter 3, anchor vigil, guard slots), then the chapter's memories that are not held yet. `UMemoriaRunSubsystem::AddChapterMemories` calls it for the live run. The VN's `set_chapter` now goes through it too, as `scene_flow.gd` does; Chapters 1 and 2 bring none, so the Chapter 1 route is unchanged.
  - The chapter maps' IR gains `sequence[].memories_chapter`, from the `MemoryManager.add_chapter_memories(N)` call in each `_start_*` function. The Belt Waystation's arrival grants Chapter 3's two memories and Drift Shelter's grants Chapter 4's.
  - Chapter 5's two memories are in the table but nothing grants them: the source calls `add_chapter_memories(5)` only from `crumbling_coast.gd`, which the canon route does not visit.
  - **Erosion now applies.** Before this session a chapter map set the run's chapter directly, so `AdvanceChapter` never ran for Chapters 3 and 4. It runs now, as in the source: every unburned memory below Grade 1 erodes by the chapter number (3, then 4), and a memory fades at 70% of its burn power. A starting Grade 5 memory with burn power 10 fades on arriving in Chapter 4. This is the source's rule; it is the first time the port's player meets it.
  - Toasts: the source raises "Memory acquired: <title>" per memory. The field shows notices only between dialogues, so the port holds them until the arrival chain's last dialogue ends.
  - The archive shows the chapter memories' Korean title, description and effect (the starting catalog carries no text for them).
- **Saving in the chapter maps.**
  - A third slot, `map.memoria.json` (`UMemoriaCheckpointSubsystem::SaveChapterMap` / `RestoreChapterMap` / `PeekChapterMap` / `CanSaveChapterMap`). It holds the run, the map's scene (`res://scenes/maps/<map>.tscn`) and Arrel's source position. A save is accepted only for a ported chapter map, a place inside it, a run at or past the map's chapter, and no VN cursor. It uses the same framing, SHA-1 and verified temp-file rename as the other slots.
  - `FindContinue` now picks the newest of three valid slots, ordered by the time each save records. File times were too coarse to order two saves made in the same second.
  - **Autosave** (`SaveManager.autosave_on_chapter_transition`), with the source's "Autosaved" toast:
    - at the end of each map's departure dialogue, in that map at Arrel's place, as the source does;
    - at `ch5_classifier`'s last step (`complete_chapter`, autosave, `goto_map`): that step leaves no VN cursor to resume, so the save is Drift Shelter's field at its spawn.
  - **Not ported:** the autosave in `ch5_classifier_entry.gd`'s `_ready`. The Drift Shelter departure save already resumes into the classifier from its first line, and Malet's report resolves to the same outcome from the same world state.
  - **Continue and Load.** The title's Continue, the pause menu's Load and the game over screen's Load open the saved map with `?Continue`. The game mode restores the run before the map is entered, and the presentation puts Arrel at the saved place. A refused load with no live run goes back to the title.
  - The pause menu's Save writes the map slot in a chapter map. In Verdan it still writes the closed-boundary checkpoint.
  - Fixed on the way: Load from a chapter map with the Verdan checkpoint as the newest save did nothing (`ContinueCheckpoint` works only in `L_VerdanHost`). It now travels there.
- **A closed chapter's map.** The source's departure autosave lands the player in the completed map. There the source opens the map's chests, clues, battle areas and random encounters, and goes on through its world atlas, which is not ported.
  - In the port the exit of a closed chapter is the road onward: stepping into it travels to the next map (`RoadOpen`). It acts on entering, as `body_entered` does, so a save loaded while standing in the exit does not travel at once.
  - A notice names the road on arrival ("Chapter 3 complete. The road at the exit goes on: Drift Shelter").
  - This also makes the Belt Waystation's post-chapter chests and clues reachable in play for the first time.
- **Tests.**
  - `Memoria.Chapter.Memories`: the table against the source's values, the grant, the single erosion, no second grant (also for a burned memory), Chapter 4, and the archive rows in both languages.
  - `Memoria.Checkpoint.ChapterMap`: what a map save accepts and refuses, the disk round trip into a fresh game instance, the newest-slot order, and damaged or foreign files.
  - `MemoriaVisual.ChapterContinue`: Chapter 3's arrival grants and toasts, the pause menu's Save, the departure autosave, Load back into the closed Belt Waystation at the saved place, the chest, the exit's road to Drift Shelter, and Chapter 4's grant.
  - `MemoriaVisual.Chapter5` now also checks the two Drift Shelter autosaves and that Continue offers the map.
  - Captures read: the memory toasts in Korean, the road notice, and the archive's two new rows with their Korean text.
- **Known gaps.**
  - A notice raised between two chained dialogues shows for about a second (the chain's delay); only the memory toasts wait for the chain's end. The Blank Book toast still shows that briefly.
  - Loan maturity and the Oath of Ash payout in `add_chapter_memories` need systems that are not ported.
  - The chapter ledger still lists burned memories by their English titles.
  - The capture window in a single-test run is about 1226×360; text was readable but layout at 1280×720 was not re-checked here.
- **Results.** Full rendered registry 272/272 (s330_full), visual 25/25 (s330_visual).

# Migration handoff — S329 turn-based retirement, step 2: the battle removed (Claude lane, 2026-09-30)

- **Why.** The user chose a staged removal. S328 moved the encounter behaviour onto field combat and tested it there. This session removes the turn-based battle, which only automation still used.
- **Removed** (all recoverable from history):
  - `UMemoriaBattleEntrySubsystem`, `MemoriaBattleModel` and their generated `MemoriaBattleSource.inl` / `MemoriaBattleCoreSource.inl`;
  - `UMemoriaBattleEntryWidget`;
  - the battle stage commandlet and its test;
  - `MemoriaBattleCoreTests`, the `MemoriaBattleEntryContractSpec`;
  - the source-battle oracle exporters `export_battle_core_oracle.py` and `export_battle_entry_oracle.py`, `test_battle_entry_tools.py`, and their fixtures (`fixtures/battle_core`, `fixtures/battle_entry`);
  - the automation opt-out (`UseFieldEncounters` / `SetFieldEncountersForTests`): field encounters are the only encounter path, in play and in tests.
  - The registry drops the BattleEntry (47) and BattleCore (101) suites: 417 → 270, with one new test.
  - The docs (`BATTLE_CORE_S296.md`, `BATTLE_CORE_SPEC.md`) stay as history.
- **Changed.**
  - **Slice controller:** no battle widget, flee or battle actions. A revisit encounter always raises field foes.
  - **Narrative:** the battle guards on Malet, story beats and Elia are gone. `ReturnFromAmbientBattle` stays as the revisit's re-entry at the authored spawn (the Sump Ledger step uses it).
  - **Audio:** `battle_theme` and the `battle_intro` cue now follow live field foes, like the source's battle scene. The flee cue and the battle-return state went with the battle.
- **Kept.**
  - `FMemoriaEncounterModel`: the source distance rules the field uses.
  - `MemoriaBattleEntryArt` and `-run=MemoriaBattleEntryAssets`: the UI art table the pause, game over, hint, achievement, codex and journal screens load from. Its battle-only textures and `M_BattlePlate` stay as unused content for now.
  - The art tools (`export_battle_entry_art.py`, `author_battle_entry_assets.py`) and the historical session tools.
- **Journey preserved.** The Malet journey's battle modes (`ShopBattle`, `ShopBattleCombat`) also carried non-battle coverage after their fights. It is now one mode, `ShopRevisit`, and one test, `Memoria.Verdan.RevisitJourney`:
  - the closed boundary's checkpoint and its physical reentry;
  - the held-key guard after that travel (formerly after the flee);
  - the two source story beats, the Sump Ledger and Elia.
  - Stages 16–26 (encounter, battle, flee, victory, defeat and recovery) are removed. The encounter part is covered by S328's `MemoriaVisual.FieldEncounters`.
- **Results.** Full rendered registry 270/270 (s329-full), visual 24/24 (s329-visual).

# Migration handoff — S328 turn-based retirement, step 1: field encounters tested (Claude lane, 2026-09-30)

- **Why.** Step D, which the user chose to do in stages.
  - Play already uses field encounters only.
  - The turn-based battle (`UMemoriaBattleEntrySubsystem`, `MemoriaBattleModel`, the battle widget) remains only for automation, behind `UseFieldEncounters()`. Its suites are 148 of the 417 registered tests: BattleEntry 47, BattleCore 101.
  - Before removing them, the encounter behaviour they covered must be tested on the field path.
- **What field encounters now carry.**
  - `UMemoriaRunSubsystem::RecordBattleStarted`: a field encounter counts the source's `battle_started` statistic (`TotalBattles`, saved), as the turn-based entry did.
  - While foes live, the encounter distance holds (the Advance call is treated as not exploring). The source's battle was a separate scene, so no walking fed the next encounter during a fight. Before, the field let steps accumulate through a fight.
- **Kept for the field.** The source distance model `FMemoriaEncounterModel` (random_encounter.gd: a 60–100 tile threshold, the warning at 72%, the pool index). It is encounter rules, not battle.
- **Test.** New `MemoriaVisual.FieldEncounters`. On the Verdan revisit (through the closed boundary's checkpoint), pacing fills the model:
  - the warning comes first;
  - the encounter raises three husks or two thieves;
  - `TotalBattles` rises by one;
  - walking on through the fight starts no second encounter.
  - Its capture, first requested on the trigger frame, showed the editor viewport. It is now taken 20 frames later and shows the revisit with the foes.
- **Next (S329).** Drop the automation opt-out (`SetFieldEncountersForTests` / `UseFieldEncounters`) and remove the turn-based code. That means the BattleEntry subsystem, BattleModel, widget, stage commandlet, audio battle routing, the narrative's `IsActive` guards, the Malet battle journey modes and the BattleEntry/BattleCore suites.
  - `MemoriaBattleEntryArt` stays: despite its name it is the UI art table the pause, hint, achievement, codex and journal screens load from.
- **Results.** Full rendered registry 417/417 (s328-full), visual 24/24 (s328-visual).

# Migration handoff — S327 story journal (Claude lane, 2026-09-30)

- **Why.** Step C finishes: the journal is the last of the four side systems (hints, achievements, codex, journal).
- **Data.**
  - `Unreal/Tools/export_journal.py` extracts `story_journal.gd`'s tables: 46 events (with `EVENT_ART_BY_FLAG` applied), 5 people, 14 world notes and 7 choices.
  - It also extracts the chapter names of `GameManager.localized_chapter_name` from `game_manager.gd`: `CHAPTER_NAMES_KO`, and `RICH_PRESENCE_CHAPTERS` over the journal's `CHAPTER_NAMES`.
  - Outputs: `docs/unreal-migration/ir/journal/story_journal.v1.json` and `Private/Journal/MemoriaJournalSources.inl`, with a `--check` mode.
  - The `.inl` is ASCII, split into separate literals joined at runtime (57k characters would pass MSVC's 64 KB literal limit).
- **Runtime.**
  - `MemoriaJournal` parses the tables. The journal is derived from the run's story flags, as in the source, so it needs no save data.
  - Korean comes from each entry's `_ko` field, else the English, as in `_field`. People without a `name_ko` use `localized_speaker`'s names.
- **Screen** (`UMemoriaJournalWidget`, the pause menu's new 저널 row before 도감, following the source's order; 9 rows now).
  - The journal backdrop (`T_UiJournalBackdrop`) under the veil, then "일지 · 기억 운반자의 현장 기록".
  - The summary: "N장 / name  보유  연소  삽화 i/u".
  - Four tabs: 사건 / 인물 / 세계 / 선택. Events and World get the source's chapter headers ("N장 · name", "N장에서 알게 된 것 · name"). People put the role first. Choices show a note while empty.
  - The detail shows the entry's illustration when the port carries that picture (the dialogue artwork or UI art).
  - Arrows, TAB and the side keys turn tabs; up and down choose; ESC returns.
- **Not carried.**
  - The Quests, Losses and Leads tabs: side quests, the world rewrite's loss records and curios are not ported. The summary's loss count goes with them.
  - Most `archive_*` illustrations are not imported yet, so "삽화" counts only the pictures the port has.
  - The choices-empty note gains a Korean line (the source's was English only).
- **Tests.**
  - New `Memoria.Journal.Source` covers table sizes, the first event in both languages, the art table, chapter names, `name_ko` and the speaker fallback.
  - New `MemoriaVisual.Journal`: in Verdan after the arrival, the pause row opens the journal. It checks the summary names Chapter 2, "2장 · 베르단 시장" then its event and description, Malet among the people with his role first, Chapter 2's world note, and no choices; ESC returns.
- **Results.** Full rendered registry 417/417 (s327-full), visual 23/23 (s327-visual).

# Migration handoff — S326 Codex's foe models in the field (Claude lane, 2026-09-30)

- **Why.** Codex delivered the S313 request: the void husk, and the market thief with its dagger. See `codex-review.md` in the shared folder and `models/MANIFEST.md`. The field foes stop being the tinted mannequin. The optional alley rat was deferred by Codex.
- **Sources.** The six files are copied into `Unreal/ArtSource/FieldCharacters/void_husk` and `market_thief`, beside S310's characters: the rigged FBXs, base colours, the husk's emissive mask, and the dagger.
- **`-run=MemoriaFoeModels`** (new; run after `-run=MemoriaFoeAssets`; additive).
  - **Imports** `Field3D/Husk` and `Field3D/Thief` with S310's import basis:
    - SK meshes (77 bones checked);
    - base colours;
    - the husk mask, linear grayscale as `codex-review.md` asks;
    - `SM_Thief_dagger`, which shares the thief atlas.
  - **Materials** `M_Husk` / `M_Thief`:
    - the painted colour and S310's small fill;
    - a fresnel edge in the spec's colour (violet / lantern-warm; strength .12, exponent 4) so the dark bodies read at night;
    - the husk's cracks, mask × Glow (.32, .008, .72) × CrackStrength 2.5, the QA values;
    - the Hit flash the combat drives.
  - **Retargeting** from the mannequin (one IK rig, `RTG_Mannequin_Husk` / `_Thief`):
    - the UAL2 foe set: zombie idle, walk and scratch, sword A and B;
    - the melee set's hit and death;
    - Epic's unarmed idle and walk, renamed `A_<Name>_Idle` / `_Walk`.
  - A kept model rebuilds only a deleted material and puts it back on the body and the prop.
- **Runtime.**
  - `FMemoriaFoeSpec::Model` ("Husk" / "Thief") → `FMemoriaFoeLook::Model`.
  - `InitializeFoe` wears the model when its assets exist: its idle and walk (the husk's zombie pair), MIDs for the Hit flash, and the thief's dagger in hand. Otherwise the S313 mannequin remains as the fallback.
  - `IsFoeModel()` reports which one is worn. Strike, hit and death clips resolve per model id.
- **Fixes found while checking.**
  - The mask sampler was first Grayscale. `M_Husk` failed to compile and rendered as the default, which `MemoriaVisual.FieldFoes`' compile check caught; it is now LinearGrayscale.
  - The first rim (.6, exponent 3) washed the bodies into flat violet and gold in the captures, so it was lowered.
- **Tests.**
  - `MemoriaVisual.FieldFoes` now requires both models (SK_Husk / SK_Thief), their own materials, the husk's zombie clips on the model, the thief's own dagger, the per-model strike clips, and a clean `M_Husk` compile.
  - `MemoriaVisual.FieldCombat` expects the husk's model.
- **Limits** (as Codex reported): stylized local models, no cloth simulation or facial rig, and rigid panels may intersect at extreme twists. The alley rat is still the husk stand-in's poison role.
- **Results.** Full rendered registry 416/416 (s326-full), visual 22/22 (s326-visual).

# Migration handoff — S325 codex, the bestiary and memory archive (Claude lane, 2026-09-30)

- **Why.** Step C continues. `codex.gd` is the game's codex (도감). It has nothing to do with the Codex agent.
- **`UMemoriaCodexSubsystem`.**
  - **Storage.** It keeps the Bestiary and the Memory Archive across runs in their own file: `user://codex.json` → `Saved/Memoria/codex.json`. Never in tests or commandlets, which is the source's `suppress_recording`.
  - **Bestiary.** A field wave is one encounter with its kind's name, void flag, health and damage. Each foe that falls is a defeat.
  - **Memory archive.** Memories are recorded from the run's archive as the run holds them: the title and description in the run's language at that moment, and the raw grade. They are marked when burned. The archive is rebuilt only when the run, the held count or the burn count changes.
  - **Roster.** The unmet list is the source's `GameManager.ENEMY_NAMES_KO` roster, which is also the Korean name table, plus the field's stand-in void husk ("보이드 허스크"). The denominator is the roster plus anything recorded, as in S217.
- **Screen** (`UMemoriaCodexWidget`, the pause menu's new 도감 row before Achievements, following the source's order).
  - The archive backdrop (`T_UiCodexBackdrop`) under the veil, then "도감 / CODEX", the subtitle, and "생물 기록 n · 기억 기록 m".
  - The Bestiary / Memory Archive tabs, the list on the left and the detail panel (350 wide) on the right.
  - **Bestiary list:** "기록 n / total", then the recorded foes with their defeat badges (◦ 10, ○ 25, ● 50), then "미조우 n" and "???" rows.
  - **Archive list:** stars, the title, and a burned mark.
  - **Details.** A foe shows its type, base HP, base attack, encounters and defeats. A memory shows its grade and stars, held or burned, and its description.
  - **Input.** TAB or the side arrows switch tabs, the up and down arrows choose, the list follows the choice, and ESC returns to the menu.
- **Deviations.**
  - **Stars.** The source's `_get_star_rating` gave `5 - grade` (the sensory Grade 5 got five stars, the core Grade 1 one). This contradicts its own comment and its gold colour for Grade 1. The comment's intent is kept: Grade 5 one star, Grade 1 five.
  - **Detail labels** are in both languages. The source's were English apart from the unscanned block.
  - **Not carried:** scans (Tobias' Analyze) and the enemy picture preview. The unmet hint keeps only "교전을 시작하면 기본 정보가 기록됩니다".
- **Pause menu.** 8 rows: Resume, Options, Save, Load, 도감, 업적, Title, Quit. The achievements test now finds its row by label.
- **Tests.**
  - New `Memoria.Codex.Rules` covers counts per name, the roster and denominator, Korean names, stars and badges.
  - New `MemoriaVisual.Codex` checks the arrival's memories are archived and the bribe's is burned. A husk met and burned down is recorded (one encounter, one defeat). The pause row opens the screen with the right list and detail; TAB turns to the archive; ESC returns.
- **Language.** Memory titles are stored in the run's language when first held. The Verdan test run is English while the interface setting is Korean, so its archive rows are English beside Korean labels, as in the existing memory archive screen.
- **Timing.** The code was written and compiled while Codex held the GPU for S313. Validation waited for the GPU to be released.
- **Results.**
  - Full rendered registry 416/416 (s325-full), visual 22/22 (s325-visual).
  - The codex backdrop was imported after that run. `MemoriaVisual.Codex` was rerun with it (Success, no glyph fallbacks).

# Migration handoff — S324 achievements (Claude lane, 2026-09-30)

- **Why.** Step C continues.
- **`UMemoriaAchievementSubsystem`** (after `achievement_manager.gd`).
  - **Table.** The source's 38 achievements in its order, with the titles, descriptions and icons as authored. They are English in both locales, as the source shows them.
  - **Storage.** Unlocks and the counters (battles_won, items_used, maps_visited) persist across runs in their own file: `user://achievements.json` → `Saved/Memoria/achievements.json`. Never in tests or commandlets.
  - **Behaviour.** An unlock happens once; unknown ids are refused. The seventh ending unlocks all_endings.
- **What unlocks now:**
  - a won field fight: `first_blood`, `battle_veteran` at ten, `survivor` at 10 HP or less as the fight ends (before the win's heal);
  - each burn in the run (polled): `first_burn`, `pyromaniac` at five, `identity_crisis` (grade 2), `zero_burn` (`core_name_origin`);
  - `chapter_complete_1`/`_5` from a VN's `complete_chapter` step, and `_2` to `_4` from `chN_complete`;
  - `merchant` (`ch2_malet_done`) and `wealthy` (100 Grains);
  - map visits (Verdan and the chapter maps) toward `explorer` at five.
- **What cannot unlock yet** (listed as "???" until their content exists): `item_master` (no items usable in field combat), `void_slayer`, `boss_hunter`, `perfect_tactics`, `resonance_master`, the hidden places, chapters 6–10, the endings, `all_quests` and `new_game_plus`.
- **Popup** (`UMemoriaAchievementPopupWidget`, z 95 like the source's layer).
  - One at a time from a queue, top right: "ACHIEVEMENT UNLOCKED", the title and the description, in the source's colours.
  - It drops from -80 to 12 over .4 s (back-ease), holds 4 s, leaves over .3 s, and plays `memory_add`.
  - The source's popup label was a placeholder "X"; the glyph here is the list's.
- **List** (`UMemoriaAchievementsWidget`, the pause menu's new Achievements row, after Load).
  - The chronicle backdrop (`T_UiAchievementsBackdrop`) under a veil, then the panel: "ACHIEVEMENTS (n / 38)", the Korean or English completion line, and the rows.
  - Each row has its glyph, the title or "???", and the description, gold when unlocked and grey when not.
  - Arrows, PageUp/PageDown or the wheel scroll it; ESC returns to the menu.
  - Glyphs are limited to the bundled font's coverage: ✕, ♛ and ◈ became ▼, ◆ and ■. The close hint follows the locale.
- **Pause menu.** 7 rows; Save and Load keep their rows and Quit stays last.
- **Tests.**
  - New `Memoria.Achievements.Rules` covers once only, unknown refused, veteran at ten, survivor at 10 HP, explorer at five maps, the chapter id and all_endings.
  - New `MemoriaVisual.Achievements` checks that the arrival's bribe burn is First Burn and the Verdan visit is recorded. A husk burned down is First Blood, with its popup. The pause row opens the list with the right header and rows; the arrows scroll to the end; ESC returns and ESC resumes.
- **Pause layout.**
  - With the seventh row, a short window (the 474 px validation capture) ran the last row into the controls line.
  - Rows now keep their 62 px while they fit, and otherwise shrink (to at least 34) to end above the saved note and the hint.
  - Options uses the same rule.
- **Results.**
  - The first run (s324) was cut off with an empty log (the session was interrupted).
  - The rerun passed: full rendered registry 415/415 (s324-full), visual 21/21 (s324-visual).
  - The row-fit change came after that run. It was verified with `MemoriaVisual.PauseMenu` (Success; the capture shows seven rows, the note and the hint clear of each other).

# Migration handoff — S323 tutorial hints (Claude lane, 2026-09-30)

- **Why.** Step C, the side systems, begins here: journal, codex/bestiary, achievements, tutorial hints. The hints come first because the action field's controls are otherwise only in the HUD's control line.
- **`UMemoriaTutorialSubsystem`** (after `tutorial_hints.gd`).
  - A hint shows once, the first time its moment comes. It holds for 4 s, and any key or click lets it go (fade .25 s).
  - Shown ids persist per profile in GameUserSettings.ini (`/Script/Memoria.MemoriaTutorial`, `ShownHints`), never in tests or commandlets, like the settings subsystem.
  - A new hint replaces the one on screen, as in the source.
- **Hints carried.**
  - `first_burn` and `first_shop` keep the source's words.
  - `first_battle` names the action controls, in the combat HUD's terms (strike, hold for a spinning slash, guard/parry, dodge, R to burn).
  - `first_status_effect` says statuses wear off with time instead of lasting turns.
  - Not carried: the source hints for turn-based systems the action field does not have (approach, BREAK, resonance, directive, equipment, pulse). `ShowHint` refuses them.
- **Triggers** (`AMemoriaSliceController::UpdateHints`, polled each frame; battle_manager.gd's moments):
  - `first_battle`: the first live foes;
  - `first_burn`: the first field burn;
  - `first_status_effect`: the first poison or weaken on Arrel.
  - `first_shop` is defined but, as in the source, never raised.
- **Deviation.** The key that dismisses a hint is not swallowed: in real-time combat it is also a strike or a dodge. The source consumes it.
- **Widget** (`UMemoriaHintWidget`, z 65: over the pause menu, under game over).
  - The source's banner frame (`ui_tutorial_hint_banner`, imported by `-run=MemoriaBattleEntryAssets` as `T_UiHintBanner`) and the dark panel with a gold border and centred, word-wrapped text (15 px at 720p).
  - It slides 70 in with a back-ease over .35 s, and fades on the way out.
  - The source's top band (.20–.80) would cover the status panel, so the hint takes the gap between the status panel (to .37) and the HUD (from .865). This follows the source's own rule of placing it where nothing overlaps (it moved the hint in battle for the same reason).
- **Tests.**
  - New `MemoriaVisual.TutorialHints` covers the first foes, a dash that dismisses the hint and still dashes, the first burn in Korean in the source's words, leaving after four seconds, and no repeat.
  - Other field tests now show the first-battle hint in their captures, without changing their results.
- **Results.**
  - Visual 20/20 (s323-visual).
  - The first full run (s323-full) produced no report. The editor hit an engine assertion, `IsInGameThread()` in SceneViewport.cpp:196, from the Slate RHI renderer on the render thread. It happened while `Memoria.Foundation.MapInputAndModal` tore down its PIE window, two frames after starting.
  - That test tears down just as quickly in the passing s322b run, and it never creates a hint. The crash was read as an intermittent engine teardown race.
  - The unchanged rerun passed: full rendered registry 414/414 (s323b-full). If it recurs, look at that test's immediate teardown.

# Migration handoff — S322 Chapter 5, The Classifier (Claude lane, 2026-09-30)

- **Why.** Step B continues. Chapter 5 is not a map: `ch5_classifier_entry.gd` consumes the Chapter 4 boundary, freezes Malet's report as one Kairos fact, and hands presentation to the VN `ch5_classifier` (22 steps).
- **Where the source's canon stops.** The VN's last step sets `canon_ch6_seam_ready`, completes Chapter 5 and returns to Drift Shelter. There the HUD names "Next: Chapter 6, The Seam". Chapter 6 is not wired in the source, so with S322 the port reaches the end of the source's current connected storyline (Chapters 1–5).
- **Report rule** (`MemoriaClassifier`, after `resolve_malet_report_outcome`).
  - **Once decided, kept.** If Kairos already holds either report fact, that outcome stands.
  - **Identified** only when Malet knows `fact.bl07.route_request_received` and still holds an active `memory.malet.bl07_request_source` whose source is `player.arrel`. Otherwise the requester stays **unknown**.
  - Kairos learns `fact.kairos.malet_report_identified_arrel` or `..._requester_unknown`. Later changes to Malet's memory never rewrite it.
  - The flags `ch5_malet_report_identified_arrel` / `_requester_unknown` only select the VN's lines 12–13 or 14–15.
  - `UMemoriaWorldCognition` gains `KnowsFact` (held as true) and `FindMemory`.
- **Entry** (`UMemoriaNarrativeSubsystem::EnterClassifier`, via `EnterStoryScene`). It runs only from `canon_ch5_classifier_ready` (or a begun Chapter 5 entry), and never after `canon_ch6_seam_ready`. It clears the boundary, sets `ch5_classifier_started` and `ch5_kairos_seen`, moves to Chapter 5, resolves the report, and plays the VN.
- **VN return.** A VN `goto_map` to a ported chapter map now opens that map's level. The importers (narrative_ir.py, the C++ cohort) accept `drift_shelter.tscn` as a goto_map target.
- **Presentation.**
  - After the Chapter 4 completion card (6 s) the classifier begins. Arriving with the boundary readied also starts it.
  - After Chapter 5, Drift Shelter shows "5장 완료 / 심 / 6장으로 가는 길은 아직 준비 중입니다" and the notice "다음 여정: 6장, 심".
- **Fixes.**
  - `EnterChapterMap` resets the event cursor with its fresh context. Before, field events after a travel could go unflushed.
  - Speaker names follow `localized_speaker` (`SPEAKER_NAMES_KO`) through a new `FMemoriaNarrativeView::SpeakerLabel`. `Speaker` stays the source key the widget's layout reads. This affects every dialogue, including Verdan and Chapters 3–4.
  - The classifier VN shows its own title ("5장 / 분류자") instead of the Verdan default.
- **Content.** `DA_VN_Ch5Classifier`, the CG `cinematic_kairos_authority_edit`, and Kairos's portraits `kairos_neutral` and `kairos_cold` (artwork 64 → 67).
- **Tests.**
  - New `Memoria.Narrative.ClassifierReport` covers identified, unknown, and history not rewritten.
  - New `MemoriaVisual.Chapter5` plays from the Chapter 4 exit through the whole VN (identified lines only) and back to Drift Shelter's Chapter 6 card, and checks the classifier plays once.
  - `MemoriaVisual.Chapter4` now ends when the classifier begins.
- **Test fix.** `MemoriaVisual.PauseMenu` held the pre-travel world as a raw `UWorld*`. After travel the new world can reuse the old one's address, and the test then waits forever for a world change it cannot see. It failed only in the full suite (s322-visual: the revisit reached `exploration:ready` and the test still timed out at step 8), and passed alone. It now holds a `TWeakObjectPtr`.
- **Results.** The first run (s322) failed for two reasons: the unregistered `ClassifierReport` test, and the PauseMenu timeout above. After both fixes: full rendered registry 414/414 (s322b-full), visual 19/19 (s322b-visual).
- **Known gaps.**
  - The chapter-transition autosave at the entry and at the VN's end is skipped (checkpoint validation covers only the Chapter 1→2 and closed Chapter 2 boundaries).
  - Chapter 6 onward is not connected in the source's canon. `docs/SEASON1_GAME_PROGRESSION.md` says the New Game main route stops after Chapter 5 at `canon_ch6_seam_ready`, and Canon Chapters 6–10 are Wave 2B (pending).
  - The legacy maps (the_seam, seam_outskirts, ...) are kept but unreachable, and are marked for rewrite: the mandatory Shade Sentinel/Threshold gates are to be removed. Porting them would port content the source means to replace.

# Migration handoff — S321 Chapter 4, Drift Shelter (Claude lane, 2026-09-29)

- **Why.** Step B continues: Chapter 4 through the S320 pipeline.
- **Exporter** (`export_chapter_maps.py`, map `drift_shelter`, prefix Ch4):
  - **Exit destination.** The exit can lead to a story scene rather than a map. It resolves the `_enter_*` helper's scene constant, giving `next_scene` = `scenes/story/ch5_classifier_entry.tscn`.
  - **Exit handler.** It also records the end handler's flags (`canon_ch5_classifier_ready`) and its bilingual notice.
  - **Section gates.** A section can be guarded by `_can_resume_ch4_exploration()` instead of a single flag. The gate becomes its required flag, and `resume_blocked` lists the later canon flags that close it.
  - **Encoding.** The `.inl` is now ASCII (Korean as JSON `\u` escapes), and chunks never split an escape. The first Chapter 4 build failed with "newline in constant" on the `.inl`.
- **Runtime.**
  - **Gates.** `GateOpen` requires the section flag and no `ResumeBlocked` flag. Chests, clues and battles use it.
  - **Departure.** It sets the exit flags and raises the notice in the run's locale.
  - **Completion card.** It names the destination: the next map's place name, or the story scene's title ("The Classifier" / "분류자").
  - **Road on.** When the next map is ported, the card holds 4 s, then `TravelToChapterMap` (`change_scene_chapter_complete`). Chapter 3 now travels on to Drift Shelter with the same run.
  - **Terrain.** RUBBLE scatters stones like RUIN, and CONCRETE stands as low slabs.
  - **Korean.** The table moved to `MemoriaChapterMaps::Korean`, so the dialogue location title is now "4장 / 드리프트" rather than "4장 / Drift". Titles follow `game_manager.gd` RUNTIME_TEXT_KO; place names follow the map headers ("드리프트 셸터"). The two Chapter 4 clue translations are new.
- **Content.**
  - Five Chapter 4 groups (`DA_Field_Ch4*`): drift_arrival, reading_deterioration, anchoring_session, night_watch, drift_departure.
  - Five CGs (artwork 59 → 64) and the `L_DriftShelter` level.
- **Source behaviour kept.**
  - The departure keeps `current_chapter = 4` and readies Chapter 5's classifier scene.
  - With `canon_ch5_classifier_ready` set, Drift Shelter's chests, clues, battles and encounters never open, as in the source (which moves straight on to Chapter 5).
  - Chapter 5 is a story scene, not ported: the card says the road is still being prepared, and no travel starts.
- **Tests.**
  - New `MemoriaVisual.Chapter4` covers the chain order, the closed clue before and after the chapter, the departure flags and notice, and no travel.
  - `MemoriaVisual.Chapter3` now follows the road into Drift Shelter and checks the run carries over.
- **Results.** Full rendered registry 413/413 (s321-full), visual 18/18 (s321-visual, including Chapter4 and the extended Chapter3).
- **Known gaps.**
  - The S320 gaps remain: chapter memories, autosave, encounters, NPCs and decorations.
  - The anchoring session's StoryJournal event waits for step C (journal).
  - The WorldAtlas gateway (waymarker shrine) is not ported.
  - Chapter 5 (`ch5_classifier_entry`) is a VN-style story scene and will need the VN pipeline, not the chapter map pipeline.

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
