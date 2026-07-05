## PauseMenu (Autoload) — 일시정지 메뉴
## ESC 키로 토글. Resume / Save / Load / Title / Quit.
extends CanvasLayer

var is_open: bool = false
var _panel_original_x: float = 0.0  # S53: 슬라이드 애니메이션용
var _anim_tween: Tween = null  # S53

# UI 노드
var backdrop: TextureRect
var control_slab: TextureRect
var overlay: ColorRect
var panel: PanelContainer
var btn_container: VBoxContainer
var save_info_label: Label
var title_label: Label
var pause_hint_label: Label
var last_saved_label: Label

const PAUSE_BACKDROP_PATH: String = "res://assets/cg/generated/ui_pause_archive_backdrop.png"
const PAUSE_CONTROL_SLAB_PATH: String = "res://assets/cg/generated/ui_pause_control_slab.png"
const ACHIEVEMENTS_BACKDROP_PATH: String = "res://assets/cg/generated/ui_achievements_chronicle_backdrop.png"
const ENDING_GALLERY_BACKDROP_PATH: String = "res://assets/cg/generated/ui_ending_gallery_backdrop.png"

const ARTBOOK_ITEMS: Array[Dictionary] = [
	{
		"title": "Arrel - Turnaround",
		"type": "Character Sheet",
		"path": "res://assets/game_image/reference/arrel_reference_turnaround.png",
		"desc": "Wandering frostblade. Full costume, gear detail, palette, and side-view animation references."
	},
	{
		"title": "Elia - Turnaround",
		"type": "Character Sheet",
		"path": "res://assets/game_image/reference/elia_reference_turnaround.png",
		"desc": "Anchor, companion, and emotional counterweight. Costume and side-view animation reference."
	},
	{
		"title": "Nera - Turnaround",
		"type": "Character Sheet",
		"path": "res://assets/game_image/reference/nera_reference_turnaround.png",
		"desc": "Bureau-adjacent silhouette and dark formal palette reference."
	},
	{
		"title": "Tobias - Turnaround",
		"type": "Character Sheet",
		"path": "res://assets/game_image/reference/tobias_reference_turnaround.png",
		"desc": "Archivist/support-role visual reference with restrained dark academic styling."
	},
	{
		"title": "Kairos - Turnaround",
		"type": "Character Sheet",
		"path": "res://assets/game_image/reference/kairos_reference_turnaround.png",
		"desc": "Supreme strategist. Sharp black uniform, controlled posture, and command-read silhouette."
	},
	{
		"title": "Veil - Turnaround",
		"type": "Character Sheet",
		"path": "res://assets/game_image/reference/veil_reference_turnaround.png",
		"desc": "Pale, spectral costume reference for a memory-adjacent presence."
	},
	{
		"title": "Arrel - Expressions",
		"type": "Expression Sheet",
		"path": "res://assets/game_image/reference/arrel_expression_sheet.png",
		"desc": "Dialogue portrait reference for cold resolve, pain, exhaustion, and guarded emotion."
	},
	{
		"title": "Elia - Expressions",
		"type": "Expression Sheet",
		"path": "res://assets/game_image/reference/elia_expression_sheet.png",
		"desc": "Dialogue portrait reference for concern, hope, sadness, and restrained warmth."
	},
	{
		"title": "Kairos - Expressions",
		"type": "Expression Sheet",
		"path": "res://assets/game_image/reference/kairos_expression_sheet.png",
		"desc": "Dialogue portrait reference for authority, calculation, anger, and command focus."
	},
	{
		"title": "Malet - Expressions",
		"type": "Expression Sheet",
		"path": "res://assets/game_image/reference/malet_expression_sheet.png",
		"desc": "Memory broker portrait sheet now used for Ch2 dialogue emotion swaps."
	},
	{
		"title": "Malet - Sprite Reference",
		"type": "Sprite Sheet",
		"path": "res://assets/game_image/reference/malet_sprite_sheet_reference.png",
		"desc": "Top-down and side-view reference for a future market broker sprite pass."
	},
	{
		"title": "Memory Lost Soldier",
		"type": "Enemy Sprite Sheet",
		"path": "res://assets/game_image/reference/memory_lost_soldier_sprite_sheet.png",
		"desc": "Frame reference for memory-corrupted humanoid enemies."
	},
	{
		"title": "Void Creature Sheet",
		"type": "Enemy Sprite Sheet",
		"path": "res://assets/game_image/reference/void_creature_sprite_sheet.png",
		"desc": "Silhouette and animation reference for future void enemy variants."
	},
	{
		"title": "Forgotten Guardian",
		"type": "Boss Sheet",
		"path": "res://assets/game_image/reference/forgotten_guardian_sheet.png",
		"desc": "Boss-scale armor, weapon, and material reference for late-game guardian encounters."
	},
	{
		"title": "Skill Icon Atlas",
		"type": "UI Reference",
		"path": "res://assets/game_image/reference/skill_icon_atlas_reference.png",
		"desc": "Future source for memory-burn, void, frost, and Bureau ability icons."
	},
	{
		"title": "Item Icon Sheet",
		"type": "UI Reference",
		"path": "res://assets/game_image/reference/item_icon_sheet.png",
		"desc": "High-polish item, relic, and memory-object icon reference."
	},
	{
		"title": "Battle Effects Pack",
		"type": "VFX Reference",
		"path": "res://assets/game_image/reference/battle_effects_pack_reference.png",
		"desc": "Slash, crystal, void, and memory-burn VFX timing and palette reference."
	},
	{
		"title": "Dialogue Screen Reference",
		"type": "UI Reference",
		"path": "res://assets/game_image/reference/dialogue_screen_reference.png",
		"desc": "Reference for a future dialogue UI pass with portrait framing and memory stats."
	},
	{
		"title": "Battle Screen Reference",
		"type": "UI Reference",
		"path": "res://assets/game_image/reference/battle_screen_reference.png",
		"desc": "Reference for future battle HUD layout, intent panels, and command clusters."
	},
	{
		"title": "World Map",
		"type": "World Reference",
		"path": "res://assets/cg/game_image/world_map_memoria.png",
		"desc": "Full-world route plate now used for the Ch2 transition toward Verdan."
	},
	{
		"title": "Frost City",
		"type": "Environment CG",
		"path": "res://assets/cg/game_image/env_frost_city.png",
		"desc": "Cold urban ruin palette for later acts and title-screen atmosphere."
	},
	{
		"title": "Forgotten Forest",
		"type": "Generated Chapter Splash",
		"path": "res://assets/cg/generated/chapter_splash_forgotten_forest.png",
		"desc": "Memory-parasite forest splash now used for Ch8 cards, HUD location art, and combat backdrops."
	},
	{
		"title": "Bureau Spires",
		"type": "Environment CG",
		"path": "res://assets/cg/game_image/env_bureau_spires.png",
		"desc": "Bureau skyline reference now used in the Act I demo ending beat."
	},
	{
		"title": "Memory Burn - Reaching Hand",
		"type": "Generated Memory CG",
		"path": "res://assets/cg/generated/memory_burn_reaching_hand.png",
		"desc": "Act I relationship-memory illustration now used in early story and burn-residue beats."
	},
	{
		"title": "Memory Burn - Name Origin",
		"type": "Generated Memory CG",
		"path": "res://assets/cg/generated/memory_burn_arrel_name.png",
		"desc": "Identity-loss illustration for high-cost burns and the Ash ending gallery."
	},
	{
		"title": "Elia Finds Arrel",
		"type": "Generated Dialogue CG",
		"path": "res://assets/cg/generated/dialogue_ch1_elia_finds_arrel.png",
		"desc": "Opening duo illustration replacing the old sheet-derived dialogue plate."
	},
	{
		"title": "Memory Crystal",
		"type": "Item CG",
		"path": "res://assets/cg/game_image/memory_crystal_item.png",
		"desc": "High-value memory object now used in the Ch2 extraction trade."
	},
	{
		"title": "Void Beast Confrontation",
		"type": "Battle CG",
		"path": "res://assets/cg/game_image/void_beast_confrontation.png",
		"desc": "Act I combat illustration replacing older forest combat placeholders."
	},
	{
		"title": "Memory Burn - First Sword",
		"type": "Generated Battle CG",
		"path": "res://assets/cg/generated/memory_burn_first_sword.png",
		"desc": "Battle-stage and memory-burn illustration replacing the old battle-ready sheet plate."
	},
	{
		"title": "Kairos in the Sealed City",
		"type": "Character CG",
		"path": "res://assets/cg/game_image/kairos_sealed_city.png",
		"desc": "Updated Kairos threat plate for Ch2 and Ch9 confrontation beats."
	},
	{
		"title": "Sealed City Ruins",
		"type": "Environment CG",
		"path": "res://assets/cg/game_image/sealed_city_ruins.png",
		"desc": "Bleak urban environment plate now used for route and Bureau foreshadowing."
	},
	{
		"title": "Crumbling Coast",
		"type": "Generated Chapter Splash",
		"path": "res://assets/cg/generated/chapter_splash_crumbling_coast.png",
		"desc": "Coastal threshold splash now used for Ch5 cards, HUD location art, and random encounter backdrops."
	},
	{
		"title": "Belt Waystation",
		"type": "Generated Chapter Splash",
		"path": "res://assets/cg/generated/chapter_splash_belt_waystation.png",
		"desc": "Ash-wind transit ruin splash now used for Ch3 cards, HUD location art, map atmosphere, and battle backdrops."
	},
	{
		"title": "Drift Shelter",
		"type": "Generated Chapter Splash",
		"path": "res://assets/cg/generated/chapter_splash_drift_shelter.png",
		"desc": "Rain-lit refuge splash now used for Ch4 cards, HUD location art, map atmosphere, and battle backdrops."
	},
	{
		"title": "Seam Outskirts",
		"type": "Generated Chapter Splash",
		"path": "res://assets/cg/generated/chapter_splash_seam_outskirts.png",
		"desc": "Fractured threshold splash now used for Ch7 cards, HUD location art, map atmosphere, and late VN threshold beats."
	},
	{
		"title": "BL-07 Void",
		"type": "Generated Chapter Splash",
		"path": "res://assets/cg/generated/chapter_splash_bl07_void.png",
		"desc": "Void-core chamber splash now used for Ch10 cards, HUD location art, combat backdrops, and hollow ending imagery."
	},
	{
		"title": "Blank Book at the Waystation",
		"type": "Generated Story CG",
		"path": "res://assets/cg/generated/story_ch3_waystation_blank_book.png",
		"desc": "Tobias reveals record-tree fiber and the first real shape of memory restoration."
	},
	{
		"title": "Ash-Rain Anchor",
		"type": "Generated Story CG",
		"path": "res://assets/cg/generated/story_ch4_drift_anchor.png",
		"desc": "Elia anchors Arrel beneath the collapsed overpass while the cost of burning spreads."
	},
	{
		"title": "The Forest Remnant",
		"type": "Generated Story CG",
		"path": "res://assets/cg/generated/story_ch8_memory_forest_remnant.png",
		"desc": "A silent remnant watches the party pass through the rings of the Memory Forest."
	},
	{
		"title": "Colorless Compass",
		"type": "Generated Story CG",
		"path": "res://assets/cg/generated/story_ch9_colorless_compass.png",
		"desc": "Arrel becomes the party's living compass through the Waste where colors stop."
	},
	{
		"title": "BL-07 Core Choice",
		"type": "Generated Story CG",
		"path": "res://assets/cg/generated/story_ch10_bl07_core_choice.png",
		"desc": "At the core of BL-07, white memory fire gathers around the final decision."
	},
	{
		"title": "Elia Finds Arrel",
		"type": "Generated Dialogue CG",
		"path": "res://assets/cg/generated/dialogue_ch1_elia_finds_arrel.png",
		"desc": "Elia finds Arrel after the first burn, turning the opening reunion into a full illustrated beat."
	},
	{
		"title": "Malet's Offer",
		"type": "Generated Dialogue CG",
		"path": "res://assets/cg/generated/dialogue_ch2_malet_memory_trade.png",
		"desc": "Malet names the price of passage while memory ampoules and amber light frame the Ch2 bargain."
	},
	{
		"title": "The Cliff Choice",
		"type": "Generated Dialogue CG",
		"path": "res://assets/cg/generated/dialogue_ch5_elia_cliff_choice.png",
		"desc": "Arrel and Elia face the split-or-stay decision on the Crumbling Coast."
	},
	{
		"title": "The Echo Shell",
		"type": "Generated Dialogue CG",
		"path": "res://assets/cg/generated/dialogue_ch7_sable_echo_shell.png",
		"desc": "Sable offers the Echo Shell, making the BL-07 truth scene feel like a major relic reveal."
	},
	{
		"title": "Premium Title Backdrop",
		"type": "Generated UI Backdrop",
		"path": "res://assets/cg/generated/ui_title_memoria_premium.png",
		"desc": "New text-free title background supporting readable in-engine title and menu controls."
	},
	{
		"title": "Pause Archive Backdrop",
		"type": "Generated UI Backdrop",
		"path": "res://assets/cg/generated/ui_pause_archive_backdrop.png",
		"desc": "Archive desk, Memory Compass, and Blank Book backdrop for the pause-menu shell."
	},
	{
		"title": "Memory Archive Backdrop",
		"type": "Generated UI Backdrop",
		"path": "res://assets/cg/generated/ui_memory_archive_backdrop.png",
		"desc": "Open Blank Book and memory-shard constellation backdrop for Arrel's archive."
	},
	{
		"title": "Loss Chronicle Backdrop",
		"type": "Generated UI Backdrop",
		"path": "res://assets/cg/generated/ui_story_journal_backdrop.png",
		"desc": "Burned journal surface for the field notes and recorded-losses interface."
	},
	{
		"title": "Memory Exchange Backdrop",
		"type": "Generated UI Backdrop",
		"path": "res://assets/cg/generated/ui_memory_shop_backdrop.png",
		"desc": "Verdan memory-market counter and ampoule shelves for the trading interface."
	},
	{
		"title": "Dialogue Ornate Frame",
		"type": "Generated UI Frame",
		"path": "res://assets/cg/generated/ui_dialogue_ornate_frame.png",
		"desc": "Lower-third dialogue frame with portrait recess and memory-glass ornaments."
	},
	{
		"title": "Battle Tactical Plate",
		"type": "Generated UI Frame",
		"path": "res://assets/cg/generated/ui_battle_tactical_plate.png",
		"desc": "Compact objective HUD plate used behind battle tactical goal text."
	},
	{
		"title": "Victory Reward Panel",
		"type": "Generated UI Frame",
		"path": "res://assets/cg/generated/ui_battle_victory_reward_panel.png",
		"desc": "Post-battle reward frame for grains, objective bonuses, and memory rewards."
	},
	{
		"title": "Burn Preview Ritual Panel",
		"type": "Generated UI Frame",
		"path": "res://assets/cg/generated/ui_burn_preview_ritual_panel.png",
		"desc": "Memory-burn confirmation frame emphasizing cost, risk, and irreversible choice."
	},
	{
		"title": "Options Observatory Backdrop",
		"type": "Generated UI Backdrop",
		"path": "res://assets/cg/generated/ui_options_observatory_backdrop.png",
		"desc": "Archive observatory backdrop for the options and accessibility menu."
	},
	{
		"title": "Game Over Void Backdrop",
		"type": "Generated UI Backdrop",
		"path": "res://assets/cg/generated/ui_game_over_void_backdrop.png",
		"desc": "Void-lit shattered-memory backdrop for the defeat recovery screen."
	},
	{
		"title": "Battle Command Ribbon",
		"type": "Generated UI Frame",
		"path": "res://assets/cg/generated/ui_battle_command_ribbon.png",
		"desc": "Wide command-bar frame used behind the bottom battle action buttons."
	},
	{
		"title": "Pause Control Slab",
		"type": "Generated UI Frame",
		"path": "res://assets/cg/generated/ui_pause_control_slab.png",
		"desc": "Vertical ornament slab layered behind the pause-menu command stack."
	},
	{
		"title": "Exploration HUD Plate",
		"type": "Generated UI Frame",
		"path": "res://assets/cg/generated/ui_exploration_hud_plate.png",
		"desc": "Top-left exploration HUD frame for HP, memory, grains, items, and quest status."
	},
	{
		"title": "Notification Toast Frame",
		"type": "Generated UI Frame",
		"path": "res://assets/cg/generated/ui_notification_toast_frame.png",
		"desc": "Bottom-center toast frame for memory, save/load, and warning notifications."
	},
	{
		"title": "Tutorial Hint Banner",
		"type": "Generated UI Frame",
		"path": "res://assets/cg/generated/ui_tutorial_hint_banner.png",
		"desc": "Top-center contextual hint banner for first-time tutorial prompts."
	},
	{
		"title": "Codex Archive Backdrop",
		"type": "Generated UI Backdrop",
		"path": "res://assets/cg/generated/ui_codex_archive_backdrop.png",
		"desc": "Split bestiary and memory-archive environment used behind the Codex interface."
	},
	{
		"title": "Memory Constellation Backdrop",
		"type": "Generated UI Backdrop",
		"path": "res://assets/cg/generated/ui_memory_constellation_backdrop.png",
		"desc": "Mnemonic observatory with subdued orbital guides behind the live memory graph."
	},
	{
		"title": "Achievement Chronicle Backdrop",
		"type": "Generated UI Backdrop",
		"path": "res://assets/cg/generated/ui_achievements_chronicle_backdrop.png",
		"desc": "Memorial ledger wall framing unlocked and hidden achievement records."
	},
	{
		"title": "Ending Gallery Backdrop",
		"type": "Generated UI Backdrop",
		"path": "res://assets/cg/generated/ui_ending_gallery_backdrop.png",
		"desc": "Six-niche ruined reliquary behind the branching ending collection."
	},
	{
		"title": "Burn Cut-In: First Sword",
		"type": "Generated Battle Cut-In",
		"path": "res://assets/cg/generated/memory_burn_first_sword.png",
		"desc": "Battle cut-in for burning the memory of first holding a sword."
	},
	{
		"title": "Burn Cut-In: Campfire Song",
		"type": "Generated Battle Cut-In",
		"path": "res://assets/cg/generated/memory_burn_elia_song.png",
		"desc": "Battle cut-in for burning the campfire song tied to Elia."
	},
	{
		"title": "Burn Cut-In: Reaching Hand",
		"type": "Generated Battle Cut-In",
		"path": "res://assets/cg/generated/memory_burn_reaching_hand.png",
		"desc": "Battle cut-in for burning the memory of a hand reaching out."
	},
	{
		"title": "Burn Cut-In: Arrel's Name",
		"type": "Generated Battle Cut-In",
		"path": "res://assets/cg/generated/memory_burn_arrel_name.png",
		"desc": "Battle cut-in for the dangerous core-name memory."
	},
	{
		"title": "Burn Cut-In: Memory Compass",
		"type": "Generated Battle Cut-In",
		"path": "res://assets/cg/generated/memory_burn_compass.png",
		"desc": "Battle cut-in for burning the compass memory near the Colorless Waste."
	},
	{
		"title": "Burn Cut-In: Void Walker",
		"type": "Generated Battle Cut-In",
		"path": "res://assets/cg/generated/memory_burn_void_walker.png",
		"desc": "Battle cut-in for late-game BL-07 and void-walker memory loss."
	},
	{
		"title": "Arrel: Memory Cascade",
		"type": "Generated Ultimate Cut-In",
		"path": "res://assets/cg/generated/cinematic_arrel_memory_cascade.png",
		"desc": "Arrel releases every surviving fragment at once during the Memory Cascade limit break."
	},
	{
		"title": "Elia: Anchor Pulse",
		"type": "Generated Companion Cut-In",
		"path": "res://assets/cg/generated/cinematic_elia_anchor_pulse.png",
		"desc": "Elia stitches fractured memory geometry together with an anchoring pulse."
	},
	{
		"title": "Sable: Echo Strike",
		"type": "Generated Companion Cut-In",
		"path": "res://assets/cg/generated/cinematic_sable_echo_strike.png",
		"desc": "Sable crosses the Seam in a single pale-blue and violet echo strike."
	},
	{
		"title": "Tobias: Record Ward",
		"type": "Generated Companion Cut-In",
		"path": "res://assets/cg/generated/cinematic_tobias_record_ward.png",
		"desc": "Tobias turns archived records into analysis, protection, and battlefield leverage."
	},
	{
		"title": "Void Beast: Memory Devour",
		"type": "Generated Enemy Cut-In",
		"path": "res://assets/cg/generated/cinematic_void_beast_memory_devour.png",
		"desc": "A void beast tears luminous identity fragments out of the battlefield."
	},
	{
		"title": "Shade Sentinel: Second Crown",
		"type": "Generated Boss Phase Cut-In",
		"path": "res://assets/cg/generated/cinematic_shade_sentinel_phase2.png",
		"desc": "The Sentinel opens its second crown and divides across failed memories."
	},
	{
		"title": "Kairos: Authority Edit",
		"type": "Generated Boss Phase Cut-In",
		"path": "res://assets/cg/generated/cinematic_kairos_authority_edit.png",
		"desc": "Kairos calmly cuts, catalogs, and rearranges the battlefield's reality."
	},
	{
		"title": "Act I: Aftermath",
		"type": "Generated Opening Story CG",
		"path": "res://assets/cg/generated/story_ch1_opening_aftermath.png",
		"desc": "Arrel studies the hand that survived after the first void-beast kill."
	},
	{
		"title": "Act I: Elia's Lantern",
		"type": "Generated Character Story CG",
		"path": "res://assets/cg/generated/story_ch1_elia_reunion.png",
		"desc": "Elia finds Arrel in the ash and keeps carefully outside the blade's reach."
	},
	{
		"title": "Act I: Ash Rain",
		"type": "Generated Story CG",
		"path": "res://assets/cg/generated/story_ch1_ash_rain_touch.png",
		"desc": "A stranger's residual warmth dissolves against Arrel's cheek."
	},
	{
		"title": "Act I: The Missing Fire",
		"type": "Generated Character Story CG",
		"path": "res://assets/cg/generated/story_ch1_camp_humming.png",
		"desc": "Elia's melody crosses the dark camp and breaks before it reaches Arrel."
	},
	{
		"title": "Act I: Twisted Path",
		"type": "Generated Environment CG",
		"path": "res://assets/cg/generated/story_ch1_twisted_forest_path.png",
		"desc": "The first playable path beneath the rib-like roots of the Rim Forest."
	},
	{
		"title": "Act I: Memory Shrine",
		"type": "Generated Exploration CG",
		"path": "res://assets/cg/generated/story_ch1_memory_shrine.png",
		"desc": "A petrified stump and cairn hold the shape of lives the forest forgot."
	},
	{
		"title": "Act I: It Uncoils",
		"type": "Generated Boss Introduction CG",
		"path": "res://assets/cg/generated/story_ch1_void_beast_emergence.png",
		"desc": "The first Void Beast descends from the canopy above the narrowing path."
	},
	{
		"title": "Act I: The Idea of Heat",
		"type": "Generated Memory-Burn CG",
		"path": "res://assets/cg/generated/story_ch1_first_burn_strike.png",
		"desc": "A sensory memory becomes one pale-gold cut through the void."
	},
	{
		"title": "Act I: One Green Tree",
		"type": "Generated Chapter Ending CG",
		"path": "res://assets/cg/generated/story_ch1_green_tree_dawn.png",
		"desc": "At dawn, one living tree remains where the ash forest ends."
	},
	{
		"title": "Act II: Verdan Gate",
		"type": "Generated Arrival Story CG",
		"path": "res://assets/cg/generated/story_ch2_verdan_gate.png",
		"desc": "Arrel and Elia meet the Bureau checkpoint above Verdan's crowded southern stairs."
	},
	{
		"title": "Act II: Memories for Sale",
		"type": "Generated Environment Story CG",
		"path": "res://assets/cg/generated/story_ch2_memory_market.png",
		"desc": "Verdan's merchants bottle affection, grief, and identity beneath smoke-black awnings."
	},
	{
		"title": "Act II: The Nameless Man",
		"type": "Generated Character Story CG",
		"path": "res://assets/cg/generated/story_ch2_old_burner.png",
		"desc": "Arrel faces a quiet mirror of the road ahead at the market's edge."
	},
	{
		"title": "Act II: Malet's Cellar",
		"type": "Generated Dialogue Story CG",
		"path": "res://assets/cg/generated/story_ch2_malet_cellar.png",
		"desc": "Malet names his price beneath Verdan, surrounded by ledgers and borrowed light."
	},
	{
		"title": "Act II: The First Sword",
		"type": "Generated Memory Extraction CG",
		"path": "res://assets/cg/generated/story_ch2_first_sword_extraction.png",
		"desc": "A first lesson becomes a pale filament, then an empty space in Arrel."
	},
	{
		"title": "Act II: Four Days",
		"type": "Generated Threat Reveal CG",
		"path": "res://assets/cg/generated/story_ch2_kairos_warning.png",
		"desc": "Kairos appears as a cold Bureau projection while the distance closes."
	},
	{
		"title": "Act II: The Recorder",
		"type": "Generated Character Introduction CG",
		"path": "res://assets/cg/generated/story_ch3_tobias_waystation.png",
		"desc": "Arrel and Elia interrupt Tobias's solitary accounting at the ruined Belt waystation."
	},
	{
		"title": "Act II: Three on the Belt",
		"type": "Generated Party Transition CG",
		"path": "res://assets/cg/generated/story_ch3_tobias_joins.png",
		"desc": "Tobias shoulders his records and the journey becomes a three-person road."
	},
	{
		"title": "Act II: The Words Move",
		"type": "Generated Memory Deterioration CG",
		"path": "res://assets/cg/generated/story_ch4_reading_deterioration.png",
		"desc": "The Blank Book stays still while Arrel's ability to read it comes apart."
	},
	{
		"title": "Act II: Eleven Small Losses",
		"type": "Generated Character Story CG",
		"path": "res://assets/cg/generated/story_ch4_night_counting_losses.png",
		"desc": "Under the Drift overpass, Elia admits she has counted every involuntary burn."
	},
	{
		"title": "Act II: He Is Classifying",
		"type": "Generated Threat Sighting CG",
		"path": "res://assets/cg/generated/story_ch5_kairos_ridge_sighting.png",
		"desc": "Kairos watches from the coastal ridge without needing to give chase."
	},
	{
		"title": "Act II: The First Color",
		"type": "Generated Sanctuary Arrival CG",
		"path": "res://assets/cg/generated/story_ch5_seam_first_light.png",
		"desc": "The party meets Sable where impossible color survives inside The Seam."
	},
	{
		"title": "Act II: BL-07 on the Map",
		"type": "Generated Mission Briefing CG",
		"path": "res://assets/cg/generated/story_ch6_sable_briefing.png",
		"desc": "Sable marks the forming Void Hole and the Sentinel standing before it."
	},
	{
		"title": "Act II: Stars Forgetting",
		"type": "Generated Night Story CG",
		"path": "res://assets/cg/generated/story_ch6_stars_forgetting.png",
		"desc": "Elia asks only that Arrel return while the sky loses its lights."
	},
	{
		"title": "Act III: The Stone Remembers",
		"type": "Generated Memorial Story CG",
		"path": "res://assets/cg/generated/story_ch7_fading_names_monument.png",
		"desc": "Sable and the party stop before twelve names dissolving from stone."
	},
	{
		"title": "Act III: Everyone Has a Line",
		"type": "Generated Confession Story CG",
		"path": "res://assets/cg/generated/story_ch7_sable_confession.png",
		"desc": "Sable names the order that finally drove her from the Authority."
	},
	{
		"title": "Act III: Voices in the Shell",
		"type": "Generated Echo Story CG",
		"path": "res://assets/cg/generated/story_ch7_echo_shell_whispers.png",
		"desc": "The Echo Shell returns fragments of lives consumed near BL-07."
	},
	{
		"title": "Act III: Say Your Name",
		"type": "Generated Forest Arrival CG",
		"path": "res://assets/cg/generated/story_ch8_forest_crossing.png",
		"desc": "The party enters a forest that feeds on the edges of identity."
	},
	{
		"title": "Act III: The Shape of a Name",
		"type": "Generated Memory Remnant CG",
		"path": "res://assets/cg/generated/story_ch8_ghost_child.png",
		"desc": "A child remembers having known a name that no longer exists."
	},
	{
		"title": "Act III: Hunger in Rings",
		"type": "Generated Investigation CG",
		"path": "res://assets/cg/generated/story_ch8_ring_cairn.png",
		"desc": "Tobias discovers that BL-07 is accelerating rather than merely expanding."
	},
	{
		"title": "Act III: The Convergence",
		"type": "Generated Kairos Confrontation CG",
		"path": "res://assets/cg/generated/story_ch9_kairos_confrontation.png",
		"desc": "Kairos arranges the Colorless Waste around one clinical observation."
	},
	{
		"title": "Act III: The First Opening",
		"type": "Generated Ancient Memory CG",
		"path": "res://assets/cg/generated/story_ch9_first_void_memory.png",
		"desc": "Arrel touches the last thought of someone who saw the first Void Hole."
	},
	{
		"title": "Act III: A Door More Real",
		"type": "Generated Threshold CG",
		"path": "res://assets/cg/generated/story_ch9_bl07_threshold.png",
		"desc": "BL-07 becomes the only absolute shape left in the Waste."
	},
	{
		"title": "Act III: What the Void Shows",
		"type": "Generated Memory Echo CG",
		"path": "res://assets/cg/generated/story_ch10_void_echoes.png",
		"desc": "Borrowed grief surrounds Arrel and Elia in fragments of almost-memory."
	},
	{
		"title": "Act III: Orphaned Beauty",
		"type": "Generated Void Archive CG",
		"path": "res://assets/cg/generated/story_ch10_orphan_memories.png",
		"desc": "The party finds the lives BL-07 kept instead of destroying."
	},
	{
		"title": "Act III: The Cost Was Everything",
		"type": "Generated Seal Ending CG",
		"path": "res://assets/cg/generated/story_ch10_seal_complete.png",
		"desc": "The world closes while the man who was Arrel no longer knows Elia."
	},
	{
		"title": "Act III: Borrowed Time",
		"type": "Generated Refusal Ending CG",
		"path": "res://assets/cg/generated/story_ch10_seal_refused.png",
		"desc": "Arrel steps back from the easy burn and chooses an uncertain future."
	},
	{
		"title": "Epilogue: Colors Without Names",
		"type": "Generated Zero Burn CG",
		"path": "res://assets/cg/generated/ending_zero_burn_canyon_watch.png",
		"desc": "The man who was Arrel watches a dawn whose colors he cannot name."
	},
	{
		"title": "Epilogue: Trying the Name",
		"type": "Generated Zero Burn CG",
		"path": "res://assets/cg/generated/ending_zero_burn_trying_name.png",
		"desc": "A lost name becomes a choice to stay rather than a recovered memory."
	},
	{
		"title": "Epilogue: The Open Wound",
		"type": "Generated Preservation CG",
		"path": "res://assets/cg/generated/ending_preservation_return.png",
		"desc": "Arrel and Elia return while Sable already knows, without sight, that BL-07 is still open."
	},
	{
		"title": "Epilogue: Hands That Build",
		"type": "Generated Preservation CG",
		"path": "res://assets/cg/generated/ending_preservation_building_hands.png",
		"desc": "The hands that burned become hands that can research and build."
	},
	{
		"title": "Epilogue: Correct Answers",
		"type": "Generated Ash Ending CG",
		"path": "res://assets/cg/generated/ending_ash_hollow_days.png",
		"desc": "Arrel answers from the place where a person used to be."
	},
	{
		"title": "Epilogue: Light, Fading",
		"type": "Generated Ash Ending CG",
		"path": "res://assets/cg/generated/ending_ash_sunset_shell.png",
		"desc": "He watches sunset as ash settles behind the surviving name."
	},
	{
		"title": "Epilogue: Too Small to Burn",
		"type": "Generated Seam Ending CG",
		"path": "res://assets/cg/generated/ending_seam_ordinary_moments.png",
		"desc": "Ordinary moments remain beyond the Void's appetite."
	},
	{
		"title": "Epilogue: Something Green",
		"type": "Generated Seam Ending CG",
		"path": "res://assets/cg/generated/ending_seam_impossible_garden.png",
		"desc": "A single shoot in dead stone suggests a different answer."
	},
	{
		"title": "Epilogue: The Night Press",
		"type": "Generated Tobias Ending CG",
		"path": "res://assets/cg/generated/ending_tobias_night_press.png",
		"desc": "Tobias turns Ring Theory into twelve suppressible copies."
	},
	{
		"title": "Epilogue: Twelve Archivists",
		"type": "Generated Tobias Ending CG",
		"path": "res://assets/cg/generated/ending_tobias_twelve_archivists.png",
		"desc": "The record moves beyond the Authority's reach."
	},
	{
		"title": "Epilogue: The Taste of Water",
		"type": "Generated Hollow Ending CG",
		"path": "res://assets/cg/generated/ending_hollow_water.png",
		"desc": "An ordinary glass becomes evidence of a vanished sense."
	},
	{
		"title": "Epilogue: A Word in an Empty Room",
		"type": "Generated Hollow Ending CG",
		"path": "res://assets/cg/generated/ending_hollow_name_room.png",
		"desc": "Elia repeats one name into a life that no longer answers."
	},
	{
		"title": "Epilogue: Collective Forgetting",
		"type": "Generated Epilogue Theory CG",
		"path": "res://assets/cg/generated/epilogue_elia_collective_pattern.png",
		"desc": "Elia connects private burns to the holes in shared history."
	},
	{
		"title": "Epilogue: Three Days East",
		"type": "Generated Journey Hook CG",
		"path": "res://assets/cg/generated/epilogue_sable_eastern_settlement.png",
		"desc": "Sable points toward a settlement forgetting without fire."
	},
	{
		"title": "Act III: A Controlled Flame",
		"type": "Generated Trial CG",
		"path": "res://assets/cg/generated/story_ch7_controlled_burn_trial.png",
		"desc": "Sable asks Arrel to prove that his mind can survive a deliberate burn."
	},
	{
		"title": "Act III: Last Field Preparations",
		"type": "Generated Party CG",
		"path": "res://assets/cg/generated/story_ch7_last_field_preparations.png",
		"desc": "The four travelers prepare beneath the ridge while BL-07 grows toward them."
	},
	{
		"title": "Act III: Paper Forgets Ink",
		"type": "Generated Threshold CG",
		"path": "res://assets/cg/generated/story_ch7_paper_forgetting_ink.png",
		"desc": "Tobias discovers that even paper loses the idea of holding a mark."
	},
	{
		"title": "Act III: Crossing the Ridgeline",
		"type": "Generated Journey CG",
		"path": "res://assets/cg/generated/story_ch7_crossing_the_ridgeline.png",
		"desc": "The Seam's last colors fall behind as the party enters the dead country."
	},
	{
		"title": "Act III: The Eighteenth Ring",
		"type": "Generated Ring Theory CG",
		"path": "res://assets/cg/generated/story_ch8_eighteenth_ring.png",
		"desc": "Tobias traces the forest's hidden order while Sable remembers who was lost."
	},
	{
		"title": "Act III: Whispers as Bait",
		"type": "Generated Forest CG",
		"path": "res://assets/cg/generated/story_ch8_whispers_as_bait.png",
		"desc": "Borrowed faces gather in the bark around the real four travelers."
	},
	{
		"title": "Act III: White Stone Shelter",
		"type": "Generated Memorial CG",
		"path": "res://assets/cg/generated/story_ch8_white_stone_shelter.png",
		"desc": "Sable finds a memory-null cairn and allows herself one quiet touch."
	},
	{
		"title": "Act III: The End of Color",
		"type": "Generated Boundary CG",
		"path": "res://assets/cg/generated/story_ch8_end_of_color.png",
		"desc": "The forest ends without transition and the Achromatic Waste begins."
	},
	{
		"title": "Act III: Forgotten Moss",
		"type": "Generated Environmental CG",
		"path": "res://assets/cg/generated/story_ch8_forgotten_moss.png",
		"desc": "Tobias lifts a remnant that remembers neither growth nor decay."
	},
	{
		"title": "Act III: The Ghost Mother",
		"type": "Generated Echo CG",
		"path": "res://assets/cg/generated/story_ch8_ghost_mother.png",
		"desc": "A translucent mother cradles the shape of someone the forest consumed."
	},
	{
		"title": "Act III: The Parasitic Heart",
		"type": "Generated Forest Heart CG",
		"path": "res://assets/cg/generated/story_ch8_parasitic_heart.png",
		"desc": "The party reaches the immense breathing knot at the forest's center."
	},
	{
		"title": "Act III: Human Chain",
		"type": "Generated Waste CG",
		"path": "res://assets/cg/generated/story_ch9_human_chain.png",
		"desc": "The party holds one another against a wind that erases direction and meaning."
	},
	{
		"title": "Act III: The Pull Beneath a Name",
		"type": "Generated Compass CG",
		"path": "res://assets/cg/generated/story_ch9_name_under_pull.png",
		"desc": "BL-07 reels Arrel inward along a line only memory can feel."
	},
	{
		"title": "Act III: Kairos Withdraws",
		"type": "Generated Aftermath CG",
		"path": "res://assets/cg/generated/story_ch9_kairos_withdrawal.png",
		"desc": "Kairos retreats through fractured records after his composure finally breaks."
	},
	{
		"title": "Act III: Memory Depth Markers",
		"type": "Generated Lore CG",
		"path": "res://assets/cg/generated/story_ch9_memory_depth_markers.png",
		"desc": "Columns of compressed lives mark how deep the travelers have entered the Waste."
	},
	{
		"title": "Act III: Final Colorless View",
		"type": "Generated Vista CG",
		"path": "res://assets/cg/generated/story_ch9_final_colorless_view.png",
		"desc": "Arrel and Elia look back once at a world reduced to existence alone."
	},
	{
		"title": "Act I: The Counting Fragment",
		"type": "Generated Side-Quest Memory CG",
		"path": "res://assets/cg/generated/story_ch1_echo_fragment.png",
		"desc": "A child's counting voice survives inside a warm crystal beneath the ash roots."
	},
	{
		"title": "Act I: A Face in the Ash",
		"type": "Generated Side-Quest Resolution CG",
		"path": "res://assets/cg/generated/story_ch1_ashen_figure_restored.png",
		"desc": "Two returned fragments let the Ashen Figure remember a face for one moment."
	},
	{
		"title": "Act I: The Sump Breathes",
		"type": "Generated Environment Story CG",
		"path": "res://assets/cg/generated/story_ch2_sump_breathing_walls.png",
		"desc": "Verdan's undercity behaves like a living organ around its bottled memories."
	},
	{
		"title": "Act I: The Missing Ledger",
		"type": "Generated Side-Quest Introduction CG",
		"path": "res://assets/cg/generated/story_ch2_nervous_trader_ledger.png",
		"desc": "A frightened trader asks that his forbidden record be returned or burned."
	},
	{
		"title": "The Weave: Everything Kept",
		"type": "Generated True-Path Climax CG",
		"path": "res://assets/cg/generated/story_ch10_seal_weave.png",
		"desc": "Arrel gathers every intact memory instead of offering the Void a wound."
	},
	{
		"title": "The Weave: Every Color",
		"type": "Generated Seal-Fire CG",
		"path": "res://assets/cg/generated/story_ch10_seal_weave_fire.png",
		"desc": "The Seam's colors braid across BL-07 and stitch the tear closed."
	},
	{
		"title": "The Weave: Still Arrel",
		"type": "Generated Ending Aftermath CG",
		"path": "res://assets/cg/generated/story_ch10_seal_weave_after.png",
		"desc": "The gate closes and Arrel still recognizes the hand on his shoulder."
	},
	{
		"title": "The Weave: The Closed Gate",
		"type": "Generated Epilogue Gate CG",
		"path": "res://assets/cg/generated/ending_weave_sealed_gate.png",
		"desc": "Sable faces the first truly silent BL-07 gate while Arrel and Elia return."
	},
	{
		"title": "The Weave: The Eighteenth Pattern",
		"type": "Generated Sable Lore CG",
		"path": "res://assets/cg/generated/ending_weave_sable_ledger.png",
		"desc": "Sable's fingers find preservation in her pin-pricked ledger, where seventeen burned attempts had failed."
	},
	{
		"title": "The Weave: Holding the Door",
		"type": "Generated Anchor CG",
		"path": "res://assets/cg/generated/ending_weave_anchor_hand.png",
		"desc": "Part of Arrel remains load-bearing, made bearable by Elia's steady hand."
	},
	{
		"title": "The Weave: Colors Return",
		"type": "Generated Ending Gallery CG",
		"path": "res://assets/cg/generated/ending_weave_colors_return.png",
		"desc": "Arrel and Elia watch color grow back across quiet stone while blind old Sable listens to it happen."
	},
	{
		"title": "The Ledger Behind Stone",
		"type": "Generated Side-Quest Discovery CG",
		"path": "res://assets/cg/generated/story_ch2_ledger_found.png",
		"desc": "A forbidden ledger waits behind loose stone in Verdan's breathing undercity."
	},
	{
		"title": "The Ledger Returned",
		"type": "Generated Side-Quest Choice CG",
		"path": "res://assets/cg/generated/story_ch2_ledger_return.png",
		"desc": "The nervous trader receives the record he feared would surface."
	},
	{
		"title": "The Ledger Burned",
		"type": "Generated Memory-Burn CG",
		"path": "res://assets/cg/generated/story_ch2_ledger_burned.png",
		"desc": "Forbidden pages catch as though they had been waiting for the flame."
	},
	{
		"title": "Kairos' Wall Warning",
		"type": "Generated Investigation CG",
		"path": "res://assets/cg/generated/story_ch3_kairos_wall_warning.png",
		"desc": "A scratched warning interrupts the climb through the Belt waystation."
	},
	{
		"title": "The Dead Belt Road",
		"type": "Generated Environment CG",
		"path": "res://assets/cg/generated/story_ch3_dead_belt_road.png",
		"desc": "A trade route that joined six settlements survives only as a scar."
	},
	{
		"title": "Tobias' Battle Notes",
		"type": "Generated Character CG",
		"path": "res://assets/cg/generated/story_ch3_tobias_battle_notes.png",
		"desc": "Tobias records the residue left by Arrel's combat burn."
	},
	{
		"title": "Shelter in Ash-Rain",
		"type": "Generated Chapter Arrival CG",
		"path": "res://assets/cg/generated/story_ch4_ash_rain_shelter.png",
		"desc": "The party finds one dry pocket beneath a collapsed overpass."
	},
	{
		"title": "The Burner's Classification",
		"type": "Generated Lore CG",
		"path": "res://assets/cg/generated/story_ch4_burner_classification.png",
		"desc": "Tobias explains the system that turns remembered lives into grades."
	},
	{
		"title": "Departure Under Gray Ash",
		"type": "Generated Journey CG",
		"path": "res://assets/cg/generated/story_ch4_ash_rain_departure.png",
		"desc": "Morning leaves every surface gray as the group prepares to move."
	},
	{
		"title": "The Warmer Cliff Path",
		"type": "Generated Journey CG",
		"path": "res://assets/cg/generated/story_ch5_warm_cliff_path.png",
		"desc": "Arrel and Elia follow a narrow coast path toward a warmer light."
	},
	{
		"title": "The Scratched Watchtower",
		"type": "Generated Investigation CG",
		"path": "res://assets/cg/generated/story_ch5_scratched_watchtower.png",
		"desc": "Lantern light exposes a ruined tower covered in desperate marks."
	},
	{
		"title": "After the Sentinel",
		"type": "Generated Battle Aftermath CG",
		"path": "res://assets/cg/generated/story_ch6_bl07_after_sentinel.png",
		"desc": "The guardian dissolves, but BL-07's wound refuses to close."
	},
	{
		"title": "The Seam's Gardener",
		"type": "Generated Resident Story CG",
		"path": "res://assets/cg/generated/story_ch6_seam_gardener.png",
		"desc": "Impossible flowers outlast names in the hands of two old survivors."
	},
	{
		"title": "Sable's Final Preparations",
		"type": "Generated Mission Briefing CG",
		"path": "res://assets/cg/generated/story_ch6_sable_final_preparations.png",
		"desc": "Sable gives Arrel one real flame to carry into an unreal wound."
	},
	{
		"title": "The Void Watcher",
		"type": "Generated Side-Quest Briefing CG",
		"path": "res://assets/cg/generated/story_ch6_void_watcher_request.png",
		"desc": "A deliberate sentinel studies The Seam from beyond the fractured gate."
	},
	{
		"title": "An Oath Freely Given",
		"type": "Generated Side-Quest Reward CG",
		"path": "res://assets/cg/generated/story_ch6_sable_vigil_reward.png",
		"desc": "Sable entrusts Arrel with a memory weighted by years of kept promises."
	},
	{
		"title": "Executor in the Square",
		"type": "Part II Story CG",
		"path": "res://assets/cg/generated/ch11_executor_strike.png",
		"desc": "Arrel witnesses an Authority blade erase the motor memory from an old man's arm."
	},
	{
		"title": "The Gray Belt",
		"type": "Part II Environment CG",
		"path": "res://assets/cg/generated/env_gray_belt_panorama.png",
		"desc": "The road west becomes a city-wide prison of repeating stone and banners."
	},
	{
		"title": "The Blood She Hid",
		"type": "Generated Part II Character CG",
		"path": "res://assets/cg/generated/ch11_elia_bloodwork.png",
		"desc": "Elia pays the private physical cost of reading a notebook that rejects ordinary eyes."
	},
	{
		"title": "The Sump Closed",
		"type": "Generated Part II Environment CG",
		"path": "res://assets/cg/generated/ch12_sump_closed.png",
		"desc": "Authority chains seal Verdan's once-living undercity market."
	},
	{
		"title": "Two Blank Notebooks",
		"type": "Part II Lore CG",
		"path": "res://assets/cg/generated/ch13_notebook_resonance.png",
		"desc": "Two empty books reveal that they are functioning pieces of a buried relay network."
	},
	{
		"title": "The Confessor's Hall",
		"type": "Part II Environment CG",
		"path": "res://assets/cg/generated/ch14_confessor_hall.png",
		"desc": "A shadowless extraction chamber removes memories as administrative corrections."
	},
	{
		"title": "The Vow Becomes Heat",
		"type": "Generated Part II Action CG",
		"path": "res://assets/cg/generated/ch14_arrel_burn_slash.png",
		"desc": "Arrel converts his reason to intervene into one precise golden slash."
	},
	{
		"title": "The Forgetting Storm",
		"type": "Part II Storm Story CG",
		"path": "res://assets/cg/generated/ch17_oblivion_storm.png",
		"desc": "The violet storm tests which memories hold every other memory in place."
	},
	{
		"title": "The Living Funeral",
		"type": "Part II Storm Story CG",
		"path": "res://assets/cg/generated/ch18_living_funeral.png",
		"desc": "Public extraction turns a person's memories and name into an administrative spectacle."
	},
	{
		"title": "The Singer's Lullaby",
		"type": "Part II Storm Story CG",
		"path": "res://assets/cg/generated/ch15_lullaby_moment.png",
		"desc": "Han hums the damaged melody that Elia recognizes before she knows why."
	},
	{
		"title": "The Echo Shell Awakens",
		"type": "Generated Part II Storm CG",
		"path": "res://assets/cg/generated/ch15_echo_shell_awakening.png",
		"desc": "Han's wordless resonance wakes voices buried inside the violet shell."
	},
	{
		"title": "The Eastward Road",
		"type": "Generated Part II Environment CG",
		"path": "res://assets/cg/generated/ch16_eastward_road.png",
		"desc": "Three travelers cross drowned salt flats toward a horizon bruised violet."
	},
	{
		"title": "Nera at the Checkpoint",
		"type": "Generated Part II Character CG",
		"path": "res://assets/cg/generated/ch16_nera_checkpoint.png",
		"desc": "A perfect warrant meets the first fraction of doubt in its author."
	},
	{
		"title": "Memory Fracture",
		"type": "Generated Part II Storm CG",
		"path": "res://assets/cg/generated/ch17_memory_fracture.png",
		"desc": "Arrel holds the storm away while ordinary memories break into glass around the party."
	},
	{
		"title": "The Record Outlives the Hand",
		"type": "Generated Part II Character CG",
		"path": "res://assets/cg/generated/ch18_tobias_close.png",
		"desc": "Tobias keeps hold of the notebook as the Living Funeral reaches for his name."
	},
	{
		"title": "Lumea, White Sanctum",
		"type": "Part III Future Storyboard CG",
		"path": "res://assets/cg/generated/env_lumea_sanctum.png",
		"desc": "A reserved Chapter 19 establishing plate for the Authority's white eastern sanctuary."
	},
	{
		"title": "The Hollow Archivist",
		"type": "Part III Future Storyboard CG",
		"path": "res://assets/cg/generated/ch20_archivist_hollow.png",
		"desc": "A reserved Chapter 20 plate for the empty keeper inside the Monolith."
	},
	{
		"title": "Kairós at the Editor's Turn",
		"type": "Part III Future Storyboard CG",
		"path": "res://assets/cg/generated/ch21_kairos_confront.png",
		"desc": "A reserved Chapter 21 confrontation plate for Kairós's final calculation."
	},
	{
		"title": "The Relay Decoded",
		"type": "Part II Lore CG",
		"path": "res://assets/cg/generated/ch13_relay_decoded.png",
		"desc": "Tobias draws a buried continental relay map from two books trained to look empty."
	},
	{
		"title": "A Register Writes Back",
		"type": "Part II Lore CG",
		"path": "res://assets/cg/generated/ch13_relay_breakthrough.png",
		"desc": "Black ink and white memory-light expose the register already forming Arrel's signature."
	},
	{
		"title": "Han's Memory Gift",
		"type": "Part II Character CG",
		"path": "res://assets/cg/generated/ch15_han_memory_gift.png",
		"desc": "Han lets fragments of the eastern song pass through her scar without becoming words."
	},
	{
		"title": "Storm on the Horizon",
		"type": "Part II Environment CG",
		"path": "res://assets/cg/generated/ch17_storm_horizon.png",
		"desc": "The violet weather crosses an empty world before it reaches the party."
	},
	{
		"title": "Arrel Against Forgetting",
		"type": "Part II Action CG",
		"path": "res://assets/cg/generated/ch17_arrel_resist.png",
		"desc": "Arrel stays upright where the storm expects every memory structure to collapse."
	},
	{
		"title": "Tobias Takes the Platform",
		"type": "Part II Story CG",
		"path": "res://assets/cg/generated/ch18_tobias_platform.png",
		"desc": "Tobias chooses the center of the Living Funeral before the Authority can drag him there."
	},
	{
		"title": "Han's Last Hum",
		"type": "Part II Alternate Storyboard CG",
		"path": "res://assets/cg/generated/ch15_han_last_hum.png",
		"desc": "A reserved quiet Han plate for a later return to the song beneath Arkein."
	},
	{
		"title": "Lumea's Inner Court",
		"type": "Part III Future Storyboard CG",
		"path": "res://assets/cg/generated/env_lumea_inner_court.png",
		"desc": "A reserved Chapter 19 interior approach to the Authority's white sanctuary."
	},
	{
		"title": "The Archivist's Memory Gallery",
		"type": "Part III Future Storyboard CG",
		"path": "res://assets/cg/generated/ch20_archivist_memory_gallery.png",
		"desc": "A reserved Chapter 20 gallery where preserved lives hang as crystalline exhibits."
	},
	{
		"title": "The Archivist's Offer",
		"type": "Part III Future Storyboard CG",
		"path": "res://assets/cg/generated/ch20_archivist_offer.png",
		"desc": "A reserved Chapter 20 plate for the hollow keeper's courteous invitation."
	},
	{
		"title": "The Archivist's Warning",
		"type": "Part III Future Storyboard CG",
		"path": "res://assets/cg/generated/ch20_archivist_warning.png",
		"desc": "A reserved Chapter 20 warning framed by memories that contradict the speaker."
	},
	{
		"title": "Inside the Monolith",
		"type": "Part III Future Storyboard CG",
		"path": "res://assets/cg/generated/ch20_monolith_interior.png",
		"desc": "A reserved Chapter 20 establishing plate for impossible interior geometry."
	},
	{
		"title": "Celah Preserved",
		"type": "Part III Future Storyboard CG",
		"path": "res://assets/cg/generated/ch20_celah_preserved.png",
		"desc": "A reserved Chapter 20 reveal of a life held between extraction and memory."
	},
	{
		"title": "Kairós at the Threshold",
		"type": "Part III Future Storyboard CG",
		"path": "res://assets/cg/generated/ch21_kairos_threshold.png",
		"desc": "A reserved Chapter 21 plate for the calculation before Kairós chooses a side."
	},
	{
		"title": "The Monolith Core",
		"type": "Part III Future Storyboard CG",
		"path": "res://assets/cg/generated/ch22_monolith_core.png",
		"desc": "A reserved Chapter 22 master plate for the light beneath every edited memory."
	},
	{
		"title": "Sable - Canon Portrait",
		"type": "Character Identity Reference",
		"path": "res://assets/game_image/reference/sable_canon_master.png",
		"desc": "Canonical Sable as Halda: an old blind woman who left her sight inside the Void Hole."
	},
	{
		"title": "The Monolith Opens",
		"type": "Part III Story CG",
		"path": "res://assets/cg/generated/ch19_monolith_gates.png",
		"desc": "A white-robed procession opens Lumea's black wall for six forbidden seconds."
	},
	{
		"title": "A Walk the Body Remembers",
		"type": "Part III Story CG",
		"path": "res://assets/cg/generated/ch19_vael_silhouette.png",
		"desc": "Arrel's guard rises before his missing memory can name the stranger across Lumea's plaza."
	},
	{
		"title": "The First Feeling",
		"type": "Part III Character CG",
		"path": "res://assets/cg/generated/ch21_nera_hesitation.png",
		"desc": "Nera's warrant trembles when nineteen years of disciplined blankness finally breaks."
	},
	{
		"title": "The Desk That Was Not There",
		"type": "Part III Story CG",
		"path": "res://assets/cg/generated/ch20_archivist_desk.png",
		"desc": "The Chief Archivist repeats one sentence at the center of the memory sea."
	},
	{
		"title": "What Fire Is For",
		"type": "Part III Decision CG",
		"path": "res://assets/cg/generated/ch22_conversion_threshold.png",
		"desc": "Arrel reaches the primal log with every kept and burned memory behind him."
	},
	{
		"title": "The First Outward Wave",
		"type": "Part III Climax CG",
		"path": "res://assets/cg/generated/ch23_conversion_wave.png",
		"desc": "Three centuries of extraction hesitate, reverse, and begin to give."
	},
	{
		"title": "The Last Lullaby",
		"type": "Part III Epilogue CG",
		"path": "res://assets/cg/generated/ch24_last_lullaby.png",
		"desc": "A child hums an ancient melody without knowing where it came from."
	},
	{
		"title": "Rim Echo: Footsteps",
		"type": "Field Focus Environment CG",
		"path": "res://assets/cg/generated/resonance_rim_forest_echo.png",
		"desc": "The Rim forest briefly remembers footsteps that no longer belong to anyone."
	},
	{
		"title": "Verdan Echo: The Last Bowl",
		"type": "Field Focus Environment CG",
		"path": "res://assets/cg/generated/resonance_verdan_market_echo.png",
		"desc": "A vanished meal leaves warm steam behind in Verdan's night market."
	},
	{
		"title": "Coast Echo: Salt Hand",
		"type": "Field Focus Environment CG",
		"path": "res://assets/cg/generated/resonance_crumbling_coast_echo.png",
		"desc": "The Crumbling Coast preserves a human handprint in wind-driven salt."
	},
	{
		"title": "Forest Echo: Unfinished Sentence",
		"type": "Field Focus Environment CG",
		"path": "res://assets/cg/generated/resonance_forgotten_forest_echo.png",
		"desc": "A hollow tree exhales the beginning of a sentence it cannot finish."
	},
	{
		"title": "Belt Echo: Ink and Footprint",
		"type": "Field Focus Environment CG",
		"path": "res://assets/cg/generated/resonance_belt_waystation_echo.png",
		"desc": "Dead soil keeps one footprint while spilled memory-ink returns to an abandoned ledger."
	},
	{
		"title": "Drift Echo: Warm Hands",
		"type": "Field Focus Environment CG",
		"path": "res://assets/cg/generated/resonance_drift_shelter_echo.png",
		"desc": "A dry circle of warmth remains on the shelter table after Elia's hands are gone."
	},
	{
		"title": "Seam Echo: A Route by Touch",
		"type": "Field Focus Environment CG",
		"path": "res://assets/cg/generated/resonance_the_seam_echo.png",
		"desc": "White flowers remember a route traced by touch and held in trust."
	},
	{
		"title": "Outskirts Echo: One Clear Note",
		"type": "Field Focus Environment CG",
		"path": "res://assets/cg/generated/resonance_seam_outskirts_echo.png",
		"desc": "The Echo Shell crystallizes one note into a bridge across the broken road."
	},
	{
		"title": "Waste Echo: Named Color",
		"type": "Field Focus Environment CG",
		"path": "res://assets/cg/generated/resonance_colorless_waste_echo.png",
		"desc": "A memory compass briefly returns blue and amber to the stones it can still name."
	},
	{
		"title": "BL-07 Echo: The Unerased Step",
		"type": "Field Focus Environment CG",
		"path": "res://assets/cg/generated/resonance_bl07_void_echo.png",
		"desc": "The Void bends around the shape of one human footprint it could not erase."
	},
	{
		"title": "A Regular's Way Through",
		"type": "Memory Key Environment CG",
		"path": "res://assets/cg/generated/memory_key_verdan_passage.png",
		"desc": "A remembered taste reveals the service passage hidden behind Verdan's abandoned stalls."
	},
	{
		"title": "Opened, Not Shattered",
		"type": "Memory Key Environment CG",
		"path": "res://assets/cg/generated/memory_key_confessor_hinge.png",
		"desc": "The memory of a first sword grip opens the extraction cradle with one exact cut."
	},
	{
		"title": "One First-Age Refrain",
		"type": "Memory Key Lore CG",
		"path": "res://assets/cg/generated/memory_key_first_age_refrain.png",
		"desc": "A campfire song and Han's lullaby meet inside the Echo Shell as one ancient refrain."
	},
	{
		"title": "The Weather It Cannot Fake",
		"type": "Memory Key Environment CG",
		"path": "res://assets/cg/generated/memory_key_forest_rain.png",
		"desc": "True rain on forest earth opens a quiet path through the counterfeit storm."
	},
	{
		"title": "One Signature",
		"type": "Memory Key Story CG",
		"path": "res://assets/cg/generated/memory_key_single_signature.png",
		"desc": "Lumea's scanner mistakes two linked hands for one surviving human signature."
	},
	{
		"title": "One Surviving Page",
		"type": "Memory Key Lore CG",
		"path": "res://assets/cg/generated/memory_key_surviving_page.png",
		"desc": "Three people standing together buy one second for a page to escape the fire."
	},
	{
		"title": "An Anchor, Not a Wound",
		"type": "Memory Key Climax CG",
		"path": "res://assets/cg/generated/memory_key_relay_anchor.png",
		"desc": "Remembered warmth lets the primal relay open around joined hands without taking from either."
	},
	{
		"title": "The Passage Behind the Stall",
		"type": "Part II Environment CG",
		"path": "res://assets/cg/generated/ch12_hidden_passage.png",
		"desc": "A remembered trace of warmth exposes the Sump route before the Authority patrol turns back."
	},
	{
		"title": "When the Horizon Moved",
		"type": "Part II Environment CG",
		"path": "res://assets/cg/generated/ch16_moving_horizon.png",
		"desc": "The Forgetting Storm advances against wind and tide until the eastern road has nowhere left to go."
	},
	{
		"title": "The Platform Forgot",
		"type": "Part II Story CG",
		"path": "res://assets/cg/generated/ch18_broken_funeral_platform.png",
		"desc": "The Living Funeral releases its restraints while Tobias's written record survives the escape."
	},
	{
		"title": "Fire Running Backward",
		"type": "Part III Lore CG",
		"path": "res://assets/cg/generated/ch20_reverse_memory_fire.png",
		"desc": "At the Monolith's heart, memory stops feeding the archive and begins to flow outward."
	},
	{
		"title": "The Cadence Burned Away",
		"type": "Part II Branch CG",
		"path": "res://assets/cg/generated/ch11_burned_stride.png",
		"desc": "Speed arrives as the remembered half-step before each corner disappears."
	},
	{
		"title": "The Long Way Still Hurts",
		"type": "Part II Branch CG",
		"path": "res://assets/cg/generated/ch11_maintenance_alley.png",
		"desc": "Keeping the memory means carrying the delay through the Belt's repeating service walls."
	},
	{
		"title": "Three Hours Under Verdan",
		"type": "Part II Branch CG",
		"path": "res://assets/cg/generated/ch12_black_service_stair.png",
		"desc": "The party keeps every face and pays for it in black water beneath the patrol."
	},
	{
		"title": "The Third Conduit",
		"type": "Part II Branch CG",
		"path": "res://assets/cg/generated/ch14_third_conduit.png",
		"desc": "Tobias finds the load-bearing premise that opens the cradle without burning a vow."
	},
	{
		"title": "The Road That Does Not Remain",
		"type": "Part II Branch CG",
		"path": "res://assets/cg/generated/ch16_flooded_shortcut.png",
		"desc": "A burned route reveals one impossible crossing and erases every safe turn behind it."
	},
	{
		"title": "A Witness Without a Name",
		"type": "Part II Branch CG",
		"path": "res://assets/cg/generated/ch18_witness_without_name.png",
		"desc": "Tobias's testimony survives in the notebook after the square forgets who gave it."
	},
	{
		"title": "Emptier Than Memory",
		"type": "Part III Branch CG",
		"path": "res://assets/cg/generated/ch20_absence_parts_sea.png",
		"desc": "The memory sea parts around the absence left by three burned companion shadows."
	},
	{
		"title": "One Hand's Width",
		"type": "Part III Branch CG",
		"path": "res://assets/cg/generated/ch20_hans_note_rim.png",
		"desc": "Han's low note holds the sea back just far enough for all three travelers to carry on."
	},
	{
		"title": "The Stair Outside Every Ledger",
		"type": "Part III Story CG",
		"path": "res://assets/cg/generated/ch21_unlisted_stair.png",
		"desc": "Beyond the last official shelf, an unwritten route descends toward the color of a first word."
	},
	{
		"title": "The Book Was an Address",
		"type": "Part III Lore CG",
		"path": "res://assets/cg/generated/ch22_book_becomes_address.png",
		"desc": "The primal log completes itself as a doorway built for a reader who can hold everyone."
	},
	{
		"title": "The Last Note Spent",
		"type": "Part II Branch CG",
		"path": "res://assets/cg/generated/ch15_burned_last_note.png",
		"desc": "A buried route opens as the final note of Han's lullaby leaves Arrel forever."
	},
	{
		"title": "Refrain Until Dawn",
		"type": "Part II Branch CG",
		"path": "res://assets/cg/generated/ch15_dawn_refrain.png",
		"desc": "Han preserves the melody by repeating it through every candle and into dawn."
	},
	{
		"title": "No Margin at the Checkpoint",
		"type": "Part II Branch CG",
		"path": "res://assets/cg/generated/ch16_checkpoint_pursuit.png",
		"desc": "The map survives while the patrol closes on Nera's deliberately open channel."
	},
	{
		"title": "A City Without a First Impression",
		"type": "Part III Branch CG",
		"path": "res://assets/cg/generated/ch19_blank_first_impression.png",
		"desc": "Lumea becomes speed after Arrel burns the meaning of seeing it for the first time."
	},
	{
		"title": "A Song Thin as Wire",
		"type": "Part III Branch CG",
		"path": "res://assets/cg/generated/ch19_han_wire_note.png",
		"desc": "Han holds one note across the final barrier and pays the price in her own body."
	},
	{
		"title": "Seventeen Names Delivered",
		"type": "Part III Story CG",
		"path": "res://assets/cg/generated/ch19_sables_ledger_arrives.png",
		"desc": "A runner carries Sable's final count into the shadowless city."
	},
	{
		"title": "Seventeen Names Stand",
		"type": "Part III Branch CG",
		"path": "res://assets/cg/generated/ch21_seventeen_name_wall.png",
		"desc": "The ledger becomes seventeen lines of heat between Belor and the living."
	},
	{
		"title": "Thirty Years in White Flame",
		"type": "Part III Branch CG",
		"path": "res://assets/cg/generated/ch21_notebook_white_flame.png",
		"desc": "Kairós's withheld arithmetic burns with the precision of an official correction."
	},
	{
		"title": "Let Her Be the Reader",
		"type": "Part III Branch CG",
		"path": "res://assets/cg/generated/ch22_relay_accepts_elia.png",
		"desc": "Arrel steps back and the primal log bends toward Elia's chosen answer."
	},
	{
		"title": "No Doorway Alone",
		"type": "Part III Branch CG",
		"path": "res://assets/cg/generated/ch22_anchor_refusal.png",
		"desc": "One joined hand keeps the open doorway anchored to a person."
	},
	{
		"title": "The Name at the Bottom",
		"type": "Part III Climax CG",
		"path": "res://assets/cg/generated/ch23_name_unspooled.png",
		"desc": "Arrel releases his own name and the extraction current reverses into gift."
	},
	{
		"title": "A Life Refusing to Come Apart",
		"type": "Part III Climax CG",
		"path": "res://assets/cg/generated/ch23_braided_conversion.png",
		"desc": "Every memory kept whole braids into the line that repairs the conversion."
	},
	{
		"title": "A Shoreline That Can Move",
		"type": "Part III Climax CG",
		"path": "res://assets/cg/generated/ch23_partial_shoreline.png",
		"desc": "The first wave thins at the far towns, leaving a beginning with a visible edge."
	},
	{
		"title": "Anger Under the Reading Wall",
		"type": "Part II Story CG",
		"path": "res://assets/cg/generated/ch13_tobias_reading_wall.png",
		"desc": "Tobias arrives at Arkein carrying ten years of copied margins and one justified accusation."
	},
	{
		"title": "The Register Closes",
		"type": "Part II Consequence CG",
		"path": "res://assets/cg/generated/ch14_signature_registered.png",
		"desc": "The dying Confessor Hall completes a precise record of how Arrel chose to burn."
	},
	{
		"title": "Arithmetic Not Sent",
		"type": "Part II Story CG",
		"path": "res://assets/cg/generated/ch14_unsent_incident_report.png",
		"desc": "Kairós closes the incident report before hesitation can acquire an official name."
	},
	{
		"title": "At the Storm's Center",
		"type": "Part II Story CG",
		"path": "res://assets/cg/generated/ch17_storm_center_fall.png",
		"desc": "Elia and Tobias fall while Arrel remains upright beneath an older rule."
	},
	{
		"title": "Still Angry, Still Here",
		"type": "Part II Branch CG",
		"path": "res://assets/cg/generated/ch18_tobias_rescued_aftermath.png",
		"desc": "Tobias leaves the broken platform alive, diminished in habit but not in anger."
	},
	{
		"title": "The Shape the Crowd Forgot",
		"type": "Part II Branch CG",
		"path": "res://assets/cg/generated/ch18_crowd_forgets_tobias.png",
		"desc": "An empty platform holds the public absence that only Arrel and Elia can name."
	},
	{
		"title": "A Name Beneath the Thumb",
		"type": "Part II Story CG",
		"path": "res://assets/cg/generated/ch12_pell_name_returns.png",
		"desc": "A hidden name returns only where Elia's hand keeps contact with the page."
	},
	{
		"title": "The Page Already Gone",
		"type": "Part II Story CG",
		"path": "res://assets/cg/generated/ch16_blank_dossier_page.png",
		"desc": "Nera opens her dossier and finds the first precise absence inside it."
	},
	{
		"title": "Edges After the Storm",
		"type": "Part II Story CG",
		"path": "res://assets/cg/generated/ch17_storm_afterimage.png",
		"desc": "The storm recedes east while the world returns with subtly altered edges."
	},
	{
		"title": "The Monolith Answers",
		"type": "Part II Story CG",
		"path": "res://assets/cg/generated/ch18_monolith_answers.png",
		"desc": "A single violet line turns the distant monolith into a reply."
	},
]

func _ready() -> void:
	layer = 55  # DialogueBox(50)와 SystemLog(60) 사이
	_build_ui()
	_hide_ui()
	if InputManager and not InputManager.input_mode_changed.is_connected(_on_input_mode_changed):
		InputManager.input_mode_changed.connect(_on_input_mode_changed)
	process_mode = Node.PROCESS_MODE_ALWAYS
	print("[PauseMenu] Ready")

func _unhandled_input(event: InputEvent) -> void:
	if event.is_action_pressed("menu"):
		# 메뉴/대화/전투/컷씬 중에는 열지 않음
		if is_open:
			_close()
			get_viewport().set_input_as_handled()
		elif _can_open_pause_menu():
			_open()
			get_viewport().set_input_as_handled()

func _can_open_pause_menu() -> bool:
	if MemoryUI.is_open:
		return false
	if GameManager.current_state == GameManager.GameState.EXPLORATION:
		return true
	# S78: Full-VN pivot 이후에는 대부분의 플레이 시간이 DIALOGUE(SceneFlow) 상태다.
	# Artbook / Save / Options에 접근할 수 있도록 VN 진행 중에도 ESC 메뉴를 허용한다.
	return GameManager.current_state == GameManager.GameState.DIALOGUE and has_node("/root/SceneFlow") and SceneFlow.is_active

func _open() -> void:
	if is_open:
		return
	is_open = true
	get_tree().paused = true
	_update_save_info()
	_refresh_footer_hints()
	if backdrop:
		backdrop.visible = true
	if control_slab:
		control_slab.visible = true
	overlay.visible = true
	panel.visible = true
	# S53: 메뉴 슬라이드 인 애니메이션
	_panel_original_x = panel.position.x
	if control_slab:
		control_slab.modulate.a = 0.0
		control_slab.position.x = -300
	panel.modulate.a = 0.0
	panel.position.x = _panel_original_x - 300
	if _anim_tween and _anim_tween.is_valid():
		_anim_tween.kill()
	_anim_tween = create_tween().set_parallel(true)
	_anim_tween.set_pause_mode(Tween.TWEEN_PAUSE_PROCESS)
	if control_slab:
		_anim_tween.tween_property(control_slab, "modulate:a", 0.78, 0.25).set_ease(Tween.EASE_OUT)
		_anim_tween.tween_property(control_slab, "position:x", 0.0, 0.3).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_BACK)
	_anim_tween.tween_property(panel, "modulate:a", 1.0, 0.25).set_ease(Tween.EASE_OUT)
	_anim_tween.tween_property(panel, "position:x", _panel_original_x, 0.3).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_BACK)
	AudioManager.play_sfx("ui_open")
	# 첫 버튼 포커스
	if btn_container.get_child_count() > 0:
		btn_container.get_child(0).grab_focus()

func _close() -> void:
	if not is_open:
		return
	is_open = false
	AudioManager.play_sfx("ui_close")
	# S53: 메뉴 슬라이드 아웃 애니메이션
	if _anim_tween and _anim_tween.is_valid():
		_anim_tween.kill()
	_anim_tween = create_tween().set_parallel(true)
	_anim_tween.set_pause_mode(Tween.TWEEN_PAUSE_PROCESS)
	if control_slab:
		_anim_tween.tween_property(control_slab, "modulate:a", 0.0, 0.2)
		_anim_tween.tween_property(control_slab, "position:x", -300.0, 0.2).set_ease(Tween.EASE_IN)
	_anim_tween.tween_property(panel, "modulate:a", 0.0, 0.2)
	_anim_tween.tween_property(panel, "position:x", _panel_original_x - 300, 0.2).set_ease(Tween.EASE_IN)
	_anim_tween.chain().tween_callback(func():
		_hide_ui()
		get_tree().paused = false
	)

func _hide_ui() -> void:
	if backdrop:
		backdrop.visible = false
	if control_slab:
		control_slab.visible = false
	if overlay:
		overlay.visible = false
	if panel:
		panel.visible = false

func _build_ui() -> void:
	var root = Control.new()
	root.set_anchors_preset(Control.PRESET_FULL_RECT)
	root.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(root)

	backdrop = TextureRect.new()
	backdrop.set_anchors_preset(Control.PRESET_FULL_RECT)
	backdrop.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	backdrop.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_COVERED
	backdrop.mouse_filter = Control.MOUSE_FILTER_IGNORE
	backdrop.modulate = Color(0.86, 0.82, 0.78, 0.94)
	if ResourceLoader.exists(PAUSE_BACKDROP_PATH):
		backdrop.texture = load(PAUSE_BACKDROP_PATH)
	root.add_child(backdrop)

	# 어두운 오버레이
	overlay = ColorRect.new()
	overlay.set_anchors_preset(Control.PRESET_FULL_RECT)
	overlay.color = Color(0, 0, 0, 0.58)
	overlay.mouse_filter = Control.MOUSE_FILTER_STOP
	root.add_child(overlay)

	if ResourceLoader.exists(PAUSE_CONTROL_SLAB_PATH):
		control_slab = TextureRect.new()
		control_slab.texture = load(PAUSE_CONTROL_SLAB_PATH)
		control_slab.anchor_left = 0.555
		control_slab.anchor_right = 0.95
		control_slab.anchor_top = 0.075
		control_slab.anchor_bottom = 0.925
		control_slab.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
		control_slab.stretch_mode = TextureRect.STRETCH_SCALE
		control_slab.mouse_filter = Control.MOUSE_FILTER_IGNORE
		control_slab.modulate = Color(1.0, 0.92, 0.78, 0.78)
		root.add_child(control_slab)

	# 중앙 패널
	panel = PanelContainer.new()
	panel.anchor_left = 0.585
	panel.anchor_right = 0.92
	panel.anchor_top = 0.12
	panel.anchor_bottom = 0.88
	var style = StyleBoxFlat.new()
	style.bg_color = Color(0.030, 0.026, 0.040, 0.78)
	style.border_color = Color(0.72, 0.54, 0.30, 0.46)
	style.set_border_width_all(2)
	style.set_corner_radius_all(6)
	style.set_content_margin_all(24)
	panel.add_theme_stylebox_override("panel", style)
	root.add_child(panel)

	var vbox = VBoxContainer.new()
	vbox.add_theme_constant_override("separation", 12)
	panel.add_child(vbox)

	# 타이틀
	title_label = Label.new()
	title_label.text = GameManager.loc("paused")
	title_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	title_label.add_theme_font_size_override("font_size", 22)
	title_label.add_theme_color_override("font_color", Color(0.92, 0.75, 0.45))
	title_label.add_theme_color_override("font_shadow_color", Color(0, 0, 0, 0.85))
	title_label.add_theme_constant_override("shadow_outline_size", 2)
	vbox.add_child(title_label)

	# 구분선
	var sep = HSeparator.new()
	sep.add_theme_constant_override("separation", 8)
	vbox.add_child(sep)

	# 게임 상태 정보
	var info_panel = PanelContainer.new()
	var info_style = StyleBoxFlat.new()
	info_style.bg_color = Color(0.08, 0.07, 0.1, 0.8)
	info_style.set_content_margin_all(12)
	info_style.set_corner_radius_all(3)
	info_panel.add_theme_stylebox_override("panel", info_style)
	vbox.add_child(info_panel)

	save_info_label = Label.new()
	save_info_label.add_theme_font_size_override("font_size", 13)
	save_info_label.add_theme_color_override("font_color", Color(0.6, 0.55, 0.5))
	info_panel.add_child(save_info_label)

	# 구분선
	var sep2 = HSeparator.new()
	sep2.add_theme_constant_override("separation", 8)
	vbox.add_child(sep2)

	# 버튼들
	btn_container = VBoxContainer.new()
	btn_container.add_theme_constant_override("separation", 8)
	vbox.add_child(btn_container)

	# S65 (A안 피벗): VN 정체성에 맞게 메뉴 슬림화.
	# 숨김: Fast Travel, Stats, Load Autosave (RPG 기능 — 스토리 몰입 방해)
	# 유지: Resume, Journal, Codex, Achievements (Steam 기대치), Endings, Options, Save/Load, Title, Quit
	var buttons = [
		{"text": GameManager.loc("resume"), "callback": _close},
		{"text": GameManager.loc("journal"), "callback": _on_journal},
		{"text": GameManager.loc("codex"), "callback": _on_codex},
		{"text": "Artbook", "callback": _on_artbook},
		{"text": GameManager.loc("achievements"), "callback": _on_achievements},
	]
	# S54: Endings button (only if at least 1 ending seen)
	if GameManager.seen_endings.size() > 0:
		buttons.append({"text": GameManager.loc("endings"), "callback": _on_endings})
	buttons.append_array([
		{"text": GameManager.loc("options"), "callback": _on_options},
		{"text": GameManager.loc("save"), "callback": _on_save},
		{"text": GameManager.loc("load"), "callback": _on_load},
		{"text": GameManager.loc("title_return"), "callback": _on_title},
		{"text": GameManager.loc("quit"), "callback": _on_quit},
	])

	for data in buttons:
		var btn = Button.new()
		btn.text = data.text
		btn.custom_minimum_size = Vector2(0, 40)

		var btn_style = StyleBoxFlat.new()
		btn_style.bg_color = Color(0.1, 0.08, 0.12, 0.9)
		btn_style.border_color = Color(0.35, 0.28, 0.2, 0.5)
		btn_style.set_border_width_all(1)
		btn_style.set_corner_radius_all(3)
		btn_style.set_content_margin_all(8)
		btn.add_theme_stylebox_override("normal", btn_style)

		var hover_style = btn_style.duplicate()
		hover_style.bg_color = Color(0.15, 0.12, 0.18, 0.95)
		hover_style.border_color = Color(0.7, 0.55, 0.3, 0.8)
		btn.add_theme_stylebox_override("hover", hover_style)
		btn.add_theme_stylebox_override("focus", hover_style)

		var press_style = btn_style.duplicate()
		press_style.bg_color = Color(0.18, 0.14, 0.1, 0.95)
		press_style.border_color = Color(0.85, 0.65, 0.3, 1.0)
		btn.add_theme_stylebox_override("pressed", press_style)

		btn.add_theme_font_size_override("font_size", 15)
		btn.add_theme_color_override("font_color", Color(0.7, 0.65, 0.55))
		btn.add_theme_color_override("font_hover_color", Color(0.95, 0.82, 0.5))

		btn.pressed.connect(data.callback)
		btn.focus_entered.connect(func(): AudioManager.play_sfx("ui_hover"))
		# S57: Hover sound on mouse enter + button press scale feedback
		btn.mouse_entered.connect(func(): AudioManager.play_sfx("ui_hover"))
		btn.pivot_offset = Vector2(btn.custom_minimum_size.x / 2.0, btn.custom_minimum_size.y / 2.0)
		btn.button_down.connect(func():
			var t = create_tween().set_pause_mode(Tween.TWEEN_PAUSE_PROCESS)
			t.tween_property(btn, "scale", Vector2(0.95, 0.95), 0.05)
		)
		btn.button_up.connect(func():
			var t = create_tween().set_pause_mode(Tween.TWEEN_PAUSE_PROCESS)
			t.tween_property(btn, "scale", Vector2(1.0, 1.0), 0.08).set_ease(Tween.EASE_OUT)
		)
		btn_container.add_child(btn)

	# S56: Last saved indicator
	last_saved_label = Label.new()
	last_saved_label.name = "LastSavedLabel"
	last_saved_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	last_saved_label.add_theme_font_size_override("font_size", 11)
	last_saved_label.add_theme_color_override("font_color", Color(0.45, 0.55, 0.35))
	vbox.add_child(last_saved_label)

	# 하단 조작법 — S56: Dynamic hints based on input mode
	pause_hint_label = Label.new()
	pause_hint_label.name = "HintLabel"
	pause_hint_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	pause_hint_label.add_theme_font_size_override("font_size", 11)
	pause_hint_label.add_theme_color_override("font_color", Color(0.5, 0.47, 0.42))
	UITheme.apply_ui_font(pause_hint_label)
	vbox.add_child(pause_hint_label)
	_update_hint_text(pause_hint_label, last_saved_label)

## S56: Update hint text based on input mode
func _update_hint_text(hint_label: Label, last_saved: Label) -> void:
	if InputManager and InputManager.is_controller_mode():
		hint_label.text = InputManager.get_hint("cancel", "닫기" if GameManager.current_locale == "ko" else "Close")
	else:
		hint_label.text = "F6 빠른 저장  |  F7 빠른 불러오기  |  [Esc] 닫기" if GameManager.current_locale == "ko" else "F6 Quick Save  |  F7 Quick Load  |  [Esc] Close"
	last_saved.text = SaveManager.get_last_saved_text()

func _on_input_mode_changed(_mode) -> void:
	_refresh_footer_hints()

func _refresh_footer_hints() -> void:
	if pause_hint_label and last_saved_label:
		_update_hint_text(pause_hint_label, last_saved_label)

func _update_save_info() -> void:
	var chapter_name = {1: "Rim Forest", 2: "Verdan Market", 3: "Belt Waystation", 4: "Drift Shelter", 5: "Crumbling Coast", 6: "The Seam", 7: "Seam Outskirts", 8: "Forgotten Forest", 9: "Colorless Waste", 10: "BL-07 Void", 11: "Epilogue"}
	var ch = GameManager.current_chapter
	var hp = GameManager.player_data.hp
	var max_hp = GameManager.player_data.max_hp
	var burn_count = MemoryManager.get_burn_count()
	var memory_count = MemoryManager.memories.size()

	var ng_text = ""
	if GameManager.ng_plus_cycle > 0:
		ng_text = " (NG+%d)" % GameManager.ng_plus_cycle
	var ch_name = chapter_name.get(ch, "Unknown")
	var text = "Chapter %d — %s%s\n" % [ch, ch_name, ng_text]
	text += "HP: %d / %d\n" % [hp, max_hp]
	text += "Memories: %d held, %d burned" % [memory_count - burn_count, burn_count]
	if WorldRewriteDirector and WorldRewriteDirector.has_method("get_loss_records"):
		text += "\nLoss records: %d" % WorldRewriteDirector.get_loss_records().size()

	# S57: Enhanced save slot display with chapter name, HP, grains, and playtime
	var ch_names = {1: "Rim Forest", 2: "Verdan Market", 3: "Belt Waystation", 4: "Drift Shelter", 5: "Crumbling Coast", 6: "The Seam", 7: "Seam Outskirts", 8: "Forgotten Forest", 9: "Colorless Waste", 10: "BL-07 Void", 11: "Epilogue"}

	var save = SaveManager.get_save_info(1)
	if save.is_empty():
		text += "\n\nSlot 1: [Empty]"
	else:
		var s_ch = save.get("chapter", 1)
		var s_ch_name = ch_names.get(s_ch, "Unknown")
		var s_hp = save.get("hp", 0)
		var s_max_hp = save.get("max_hp", 100)
		var s_grains = save.get("grains", 0)
		var s_location = save.get("location", "")
		text += "\n\nSlot 1: Ch%d - %s" % [s_ch, s_ch_name]
		if s_location != "":
			text += " (%s)" % s_location
		text += "\n    HP: %d/%d | Grains: %d | %s" % [s_hp, s_max_hp, s_grains, save.get("timestamp", "?")]

	# S56/S57: Autosave slot info (enhanced)
	var auto_save = SaveManager.get_save_info(0)
	if not auto_save.is_empty():
		var a_ch = auto_save.get("chapter", 1)
		var a_ch_name = ch_names.get(a_ch, "Unknown")
		var a_hp = auto_save.get("hp", 0)
		var a_max_hp = auto_save.get("max_hp", 100)
		text += "\nAutosave: Ch%d - %s | HP: %d/%d | %s" % [a_ch, a_ch_name, a_hp, a_max_hp, auto_save.get("timestamp", "?")]

	save_info_label.text = text

func _on_save() -> void:
	SaveManager.save_game(1)
	AudioManager.play_sfx("confirm")
	_update_save_info()
	# 세이브 완료 피드백
	title_label.text = "SAVED!"
	title_label.add_theme_color_override("font_color", Color(0.4, 0.7, 0.45))
	await get_tree().create_timer(0.8).timeout
	if not is_open:
		return
	title_label.text = GameManager.loc("paused")
	title_label.add_theme_color_override("font_color", Color(0.75, 0.65, 0.45))

func _on_load() -> void:
	if not SaveManager.has_save(1):
		AudioManager.play_sfx("cancel")
		return
	_close()
	SaveManager.load_game(1)

## S56: Load autosave
func _on_load_autosave() -> void:
	if not SaveManager.has_save(0):
		AudioManager.play_sfx("cancel")
		NotificationToast.show_toast("No autosave found", NotificationToast.ToastType.WARNING)
		return
	_close()
	SaveManager.load_game(0)

func _on_title() -> void:
	_close()
	SceneTransition.change_scene("res://scenes/main/main.tscn")

func _on_journal() -> void:
	AudioManager.play_sfx("ui_select")
	StoryJournal.open_journal()

func _on_options() -> void:
	AudioManager.play_sfx("ui_select")
	OptionsMenu.open()

func _on_codex() -> void:
	AudioManager.play_sfx("ui_select")
	Codex.open()

func _on_artbook() -> void:
	AudioManager.play_sfx("ui_select")
	_show_artbook_panel()

func _show_artbook_panel() -> void:
	var art_overlay = ColorRect.new()
	art_overlay.set_anchors_preset(Control.PRESET_FULL_RECT)
	art_overlay.color = Color(0.01, 0.01, 0.015, 0.88)
	art_overlay.mouse_filter = Control.MOUSE_FILTER_STOP
	add_child(art_overlay)

	var art_panel = PanelContainer.new()
	art_panel.anchor_left = 0.05
	art_panel.anchor_right = 0.95
	art_panel.anchor_top = 0.04
	art_panel.anchor_bottom = 0.96
	var style = StyleBoxFlat.new()
	style.bg_color = Color(0.045, 0.038, 0.055, 0.985)
	style.border_color = Color(0.68, 0.54, 0.32, 0.75)
	style.set_border_width_all(2)
	style.set_corner_radius_all(6)
	style.set_content_margin_all(18)
	art_panel.add_theme_stylebox_override("panel", style)
	art_overlay.add_child(art_panel)

	var root = VBoxContainer.new()
	root.add_theme_constant_override("separation", 10)
	art_panel.add_child(root)

	var header = Label.new()
	header.text = "ARTBOOK / CHARACTER DOSSIER"
	header.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	header.add_theme_font_size_override("font_size", 21)
	header.add_theme_color_override("font_color", Color(0.92, 0.76, 0.44))
	root.add_child(header)

	var sub = Label.new()
	sub.text = "Concept sheets, expression studies, and atmosphere plates"
	sub.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	sub.add_theme_font_size_override("font_size", 12)
	sub.add_theme_color_override("font_color", Color(0.58, 0.52, 0.45))
	root.add_child(sub)

	var sep = HSeparator.new()
	root.add_child(sep)

	var body = HBoxContainer.new()
	body.size_flags_vertical = Control.SIZE_EXPAND_FILL
	body.add_theme_constant_override("separation", 18)
	root.add_child(body)

	var left_panel = PanelContainer.new()
	left_panel.custom_minimum_size = Vector2(260, 0)
	left_panel.size_flags_vertical = Control.SIZE_EXPAND_FILL
	var left_style = StyleBoxFlat.new()
	left_style.bg_color = Color(0.025, 0.022, 0.032, 0.78)
	left_style.border_color = Color(0.32, 0.25, 0.16, 0.5)
	left_style.set_border_width_all(1)
	left_style.set_corner_radius_all(4)
	left_style.set_content_margin_all(10)
	left_panel.add_theme_stylebox_override("panel", left_style)
	body.add_child(left_panel)

	var left_box = VBoxContainer.new()
	left_box.add_theme_constant_override("separation", 8)
	left_panel.add_child(left_box)

	var list_title = Label.new()
	list_title.text = "FILES"
	list_title.add_theme_font_size_override("font_size", 14)
	list_title.add_theme_color_override("font_color", Color(0.74, 0.65, 0.48))
	left_box.add_child(list_title)

	var scroll = ScrollContainer.new()
	scroll.size_flags_vertical = Control.SIZE_EXPAND_FILL
	scroll.horizontal_scroll_mode = ScrollContainer.SCROLL_MODE_DISABLED
	left_box.add_child(scroll)

	var list = VBoxContainer.new()
	list.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	list.add_theme_constant_override("separation", 6)
	scroll.add_child(list)

	var right_panel = PanelContainer.new()
	right_panel.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	right_panel.size_flags_vertical = Control.SIZE_EXPAND_FILL
	var right_style = StyleBoxFlat.new()
	right_style.bg_color = Color(0.018, 0.017, 0.024, 0.92)
	right_style.border_color = Color(0.42, 0.34, 0.22, 0.65)
	right_style.set_border_width_all(1)
	right_style.set_corner_radius_all(4)
	right_style.set_content_margin_all(12)
	right_panel.add_theme_stylebox_override("panel", right_style)
	body.add_child(right_panel)

	var preview_box = VBoxContainer.new()
	preview_box.add_theme_constant_override("separation", 10)
	right_panel.add_child(preview_box)

	var preview_title = Label.new()
	preview_title.add_theme_font_size_override("font_size", 18)
	preview_title.add_theme_color_override("font_color", Color(0.9, 0.78, 0.52))
	preview_box.add_child(preview_title)

	var preview_type = Label.new()
	preview_type.add_theme_font_size_override("font_size", 12)
	preview_type.add_theme_color_override("font_color", Color(0.5, 0.48, 0.42))
	preview_box.add_child(preview_type)

	var preview = TextureRect.new()
	preview.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	preview.size_flags_vertical = Control.SIZE_EXPAND_FILL
	preview.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	preview.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
	preview.mouse_filter = Control.MOUSE_FILTER_IGNORE
	preview_box.add_child(preview)

	var preview_desc = RichTextLabel.new()
	preview_desc.bbcode_enabled = true
	preview_desc.fit_content = true
	preview_desc.scroll_active = false
	preview_desc.add_theme_font_size_override("normal_font_size", 13)
	preview_desc.add_theme_color_override("default_color", Color(0.74, 0.69, 0.61))
	preview_box.add_child(preview_desc)

	for i in range(ARTBOOK_ITEMS.size()):
		var item := ARTBOOK_ITEMS[i]
		var btn = Button.new()
		btn.text = "%s\n   %s" % [item.get("title", "Untitled"), item.get("type", "Reference")]
		btn.custom_minimum_size = Vector2(0, 48)
		btn.alignment = HORIZONTAL_ALIGNMENT_LEFT
		btn.add_theme_font_size_override("font_size", 12)
		btn.add_theme_color_override("font_color", Color(0.72, 0.68, 0.58))
		btn.add_theme_color_override("font_hover_color", Color(0.98, 0.84, 0.52))
		var btn_style = StyleBoxFlat.new()
		btn_style.bg_color = Color(0.06, 0.052, 0.075, 0.9)
		btn_style.border_color = Color(0.28, 0.22, 0.15, 0.45)
		btn_style.set_border_width_all(1)
		btn_style.set_corner_radius_all(3)
		btn_style.set_content_margin_all(8)
		btn.add_theme_stylebox_override("normal", btn_style)
		var hover_style = btn_style.duplicate()
		hover_style.bg_color = Color(0.12, 0.095, 0.08, 0.95)
		hover_style.border_color = Color(0.74, 0.54, 0.27, 0.85)
		btn.add_theme_stylebox_override("hover", hover_style)
		btn.add_theme_stylebox_override("focus", hover_style)
		btn.pressed.connect(_on_artbook_item_pressed.bind(i, preview, preview_title, preview_type, preview_desc))
		btn.mouse_entered.connect(func(): AudioManager.play_sfx("ui_hover"))
		list.add_child(btn)

	if ARTBOOK_ITEMS.size() > 0:
		_set_artbook_preview(preview, preview_title, preview_type, preview_desc, ARTBOOK_ITEMS[0])

	var close_label = Label.new()
	close_label.text = "[ESC] Close"
	close_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	close_label.add_theme_font_size_override("font_size", 11)
	close_label.add_theme_color_override("font_color", Color(0.42, 0.37, 0.31))
	root.add_child(close_label)

	var close_handler = func(event: InputEvent):
		if event.is_action_pressed("cancel") or event.is_action_pressed("menu"):
			art_overlay.queue_free()
			get_viewport().set_input_as_handled()
	art_overlay.gui_input.connect(close_handler)

func _set_artbook_preview(preview: TextureRect, title: Label, type_label: Label, desc: RichTextLabel, item: Dictionary) -> void:
	var path: String = item.get("path", "")
	title.text = item.get("title", "Untitled")
	type_label.text = item.get("type", "Reference")
	desc.text = "[i]%s[/i]" % item.get("desc", "")

	if path != "" and ResourceLoader.exists(path):
		preview.texture = load(path)
	else:
		preview.texture = null
		desc.text = "[color=#c77855]Missing file:[/color] %s" % path

func _on_artbook_item_pressed(index: int, preview: TextureRect, title: Label, type_label: Label, desc: RichTextLabel) -> void:
	AudioManager.play_sfx("ui_select")
	if index >= 0 and index < ARTBOOK_ITEMS.size():
		_set_artbook_preview(preview, title, type_label, desc, ARTBOOK_ITEMS[index])

func _add_modal_backdrop(host: Control, path: String, wash: Color = Color(0.01, 0.008, 0.02, 0.34)) -> void:
	var art := TextureRect.new()
	art.set_anchors_preset(Control.PRESET_FULL_RECT)
	art.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	art.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_COVERED
	art.mouse_filter = Control.MOUSE_FILTER_IGNORE
	if ResourceLoader.exists(path):
		art.texture = load(path)
	host.add_child(art)

	var shade := ColorRect.new()
	shade.set_anchors_preset(Control.PRESET_FULL_RECT)
	shade.color = wash
	shade.mouse_filter = Control.MOUSE_FILTER_IGNORE
	host.add_child(shade)

func _animate_modal_panel(target: Control) -> void:
	target.modulate.a = 0.0
	var resting_y := target.position.y
	target.position.y += 14.0
	var tween := create_tween().set_parallel(true)
	tween.tween_property(target, "modulate:a", 1.0, 0.24).set_ease(Tween.EASE_OUT)
	tween.tween_property(target, "position:y", resting_y, 0.32).set_trans(Tween.TRANS_QUART).set_ease(Tween.EASE_OUT)

func _on_achievements() -> void:
	AudioManager.play_sfx("ui_select")
	_show_achievements_panel()

func _show_achievements_panel() -> void:
	# 업적 패널 (PauseMenu 위에 오버레이)
	var ach_overlay = ColorRect.new()
	ach_overlay.set_anchors_preset(Control.PRESET_FULL_RECT)
	ach_overlay.color = Color(0, 0, 0, 0)
	ach_overlay.mouse_filter = Control.MOUSE_FILTER_STOP
	add_child(ach_overlay)
	_add_modal_backdrop(ach_overlay, ACHIEVEMENTS_BACKDROP_PATH, Color(0.01, 0.008, 0.018, 0.42))

	var ach_panel = PanelContainer.new()
	ach_panel.anchor_left = 0.12
	ach_panel.anchor_right = 0.88
	ach_panel.anchor_top = 0.05
	ach_panel.anchor_bottom = 0.95
	var style = StyleBoxFlat.new()
	style.bg_color = Color(0.035, 0.03, 0.05, 0.80)
	style.border_color = Color(0.55, 0.42, 0.25, 0.78)
	style.set_border_width_all(2)
	style.set_corner_radius_all(6)
	style.set_content_margin_all(16)
	ach_panel.add_theme_stylebox_override("panel", style)
	ach_overlay.add_child(ach_panel)

	var vbox = VBoxContainer.new()
	vbox.add_theme_constant_override("separation", 8)
	ach_panel.add_child(vbox)

	# 타이틀
	var header = Label.new()
	var all_achs = AchievementManager.get_all_achievements()
	var unlocked_count = 0
	for a in all_achs:
		if a["unlocked"]:
			unlocked_count += 1
	header.text = "ACHIEVEMENTS  (%d / %d)" % [unlocked_count, all_achs.size()]
	header.add_theme_font_size_override("font_size", 22)
	header.add_theme_color_override("font_color", Color(0.85, 0.7, 0.45))
	header.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	UITheme.apply_title_font(header)
	vbox.add_child(header)

	var progress_label := Label.new()
	progress_label.text = ("기억 속에 새겨진 이정표  ·  달성률 %.0f%%" if GameManager.current_locale == "ko" else "Milestones engraved in memory  ·  %.0f%% complete") % [float(unlocked_count) / maxf(1.0, float(all_achs.size())) * 100.0]
	progress_label.add_theme_font_size_override("font_size", 12)
	progress_label.add_theme_color_override("font_color", Color(0.56, 0.54, 0.52))
	progress_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	vbox.add_child(progress_label)

	var sep = HSeparator.new()
	vbox.add_child(sep)

	# 스크롤 리스트
	var scroll = ScrollContainer.new()
	scroll.size_flags_vertical = Control.SIZE_EXPAND_FILL
	scroll.horizontal_scroll_mode = ScrollContainer.SCROLL_MODE_DISABLED
	vbox.add_child(scroll)

	var list = VBoxContainer.new()
	list.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	list.add_theme_constant_override("separation", 4)
	scroll.add_child(list)

	for ach in all_achs:
		var row_panel := PanelContainer.new()
		var row_style := StyleBoxFlat.new()
		row_style.bg_color = Color(0.08, 0.065, 0.09, 0.66) if ach["unlocked"] else Color(0.035, 0.03, 0.045, 0.56)
		row_style.border_color = Color(0.58, 0.44, 0.24, 0.42) if ach["unlocked"] else Color(0.2, 0.18, 0.2, 0.28)
		row_style.border_width_left = 3
		row_style.set_content_margin_all(8)
		row_style.set_corner_radius_all(3)
		row_panel.add_theme_stylebox_override("panel", row_style)
		list.add_child(row_panel)

		var row = HBoxContainer.new()
		row.add_theme_constant_override("separation", 10)
		row_panel.add_child(row)

		# 단색 문양을 사용해 플랫폼별 컬러 이모지 편차를 피한다.
		var icon_map = {"sword": "⚔", "skull": "☠", "crown": "♛", "shield": "◈", "heart": "♥", "potion": "◇", "flame": "♨", "eye": "◉", "map": "✧", "book": "▤", "star": "★", "coin": "◎", "cycle": "↻"}
		var icon_label = Label.new()
		icon_label.text = icon_map.get(ach.get("icon", ""), "•")
		icon_label.add_theme_font_size_override("font_size", 16)
		icon_label.add_theme_color_override("font_color", Color(0.82, 0.67, 0.38) if ach["unlocked"] else Color(0.3, 0.28, 0.3))
		icon_label.custom_minimum_size = Vector2(28, 0)
		row.add_child(icon_label)

		var info = VBoxContainer.new()
		info.size_flags_horizontal = Control.SIZE_EXPAND_FILL
		row.add_child(info)

		var title_lbl = Label.new()
		title_lbl.add_theme_font_size_override("font_size", 14)
		info.add_child(title_lbl)

		var desc_lbl = Label.new()
		desc_lbl.add_theme_font_size_override("font_size", 11)
		info.add_child(desc_lbl)

		if ach["unlocked"]:
			title_lbl.text = ach["title"]
			title_lbl.add_theme_color_override("font_color", Color(0.85, 0.75, 0.5))
			desc_lbl.text = ach["desc"]
			desc_lbl.add_theme_color_override("font_color", Color(0.6, 0.55, 0.5))
		else:
			title_lbl.text = "???"
			title_lbl.add_theme_color_override("font_color", Color(0.35, 0.3, 0.28))
			desc_lbl.text = ach["desc"]
			desc_lbl.add_theme_color_override("font_color", Color(0.3, 0.28, 0.25))

	# 닫기 힌트
	var close_label = Label.new()
	close_label.text = "[ESC] Close"
	close_label.add_theme_font_size_override("font_size", 11)
	close_label.add_theme_color_override("font_color", Color(0.4, 0.35, 0.3))
	close_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	vbox.add_child(close_label)

	# ESC로 닫기
	var close_handler = func(event: InputEvent):
		if event.is_action_pressed("cancel") or event.is_action_pressed("menu"):
			ach_overlay.queue_free()
			get_viewport().set_input_as_handled()
	ach_overlay.gui_input.connect(close_handler)
	# 패널 클릭으로도 닫기
	ach_overlay.mouse_filter = Control.MOUSE_FILTER_STOP
	_animate_modal_panel(ach_panel)

func _on_travel() -> void:
	AudioManager.play_sfx("ui_select")
	_show_travel_panel()

func _show_travel_panel() -> void:
	var travel_overlay = ColorRect.new()
	travel_overlay.set_anchors_preset(Control.PRESET_FULL_RECT)
	travel_overlay.color = Color(0, 0, 0, 0.7)
	travel_overlay.mouse_filter = Control.MOUSE_FILTER_STOP
	add_child(travel_overlay)

	var travel_panel = PanelContainer.new()
	travel_panel.anchor_left = 0.25
	travel_panel.anchor_right = 0.75
	travel_panel.anchor_top = 0.15
	travel_panel.anchor_bottom = 0.85
	var style = StyleBoxFlat.new()
	style.bg_color = Color(0.06, 0.05, 0.08, 0.98)
	style.border_color = Color(0.4, 0.5, 0.3, 0.7)
	style.set_border_width_all(2)
	style.set_corner_radius_all(6)
	style.set_content_margin_all(20)
	travel_panel.add_theme_stylebox_override("panel", style)
	travel_overlay.add_child(travel_panel)

	var vbox = VBoxContainer.new()
	vbox.add_theme_constant_override("separation", 10)
	travel_panel.add_child(vbox)

	var header = Label.new()
	header.text = "FAST TRAVEL"
	header.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	header.add_theme_font_size_override("font_size", 18)
	header.add_theme_color_override("font_color", Color(0.7, 0.8, 0.55))
	vbox.add_child(header)

	var sep = HSeparator.new()
	vbox.add_child(sep)

	var desc = Label.new()
	desc.text = "Select a destination. Travel is instant."
	desc.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	desc.add_theme_font_size_override("font_size", 12)
	desc.add_theme_color_override("font_color", Color(0.5, 0.5, 0.45))
	vbox.add_child(desc)

	# 맵 목록 — 챕터에 따라 해금
	var maps = [
		{"name": "Rim Forest", "scene": "res://scenes/maps/rim_forest.tscn", "chapter": 1, "desc": "Where it all began."},
		{"name": "Verdan Market", "scene": "res://scenes/maps/verdan_market.tscn", "chapter": 2, "desc": "A place of trade and memory."},
		{"name": "Belt Waystation", "scene": "res://scenes/maps/belt_waystation.tscn", "chapter": 3, "desc": "Bureau Relay Station 14. The dead road."},
		{"name": "Drift Shelter", "scene": "res://scenes/maps/drift_shelter.tscn", "chapter": 4, "desc": "Where the architecture crumbles."},
		{"name": "Crumbling Coast", "scene": "res://scenes/maps/crumbling_coast.tscn", "chapter": 5, "desc": "Cliffs falling into the void."},
		{"name": "The Seam", "scene": "res://scenes/maps/the_seam.tscn", "chapter": 6, "desc": "Where color bleeds through."},
		{"name": "Seam Outskirts", "scene": "res://scenes/maps/seam_outskirts.tscn", "chapter": 7, "desc": "The Threshold. BL-07's edge."},
		{"name": "Forgotten Forest", "scene": "res://scenes/maps/forgotten_forest.tscn", "chapter": 8, "desc": "Trees that remember being trees."},
		{"name": "Colorless Waste", "scene": "res://scenes/maps/colorless_waste.tscn", "chapter": 9, "desc": "Where the concept of color withdrew."},
		{"name": "BL-07 Void", "scene": "res://scenes/maps/bl07_void.tscn", "chapter": 10, "desc": "The space between spaces."},
	]

	var current_ch = GameManager.current_chapter
	for map_data in maps:
		var btn = Button.new()
		var unlocked = current_ch >= map_data["chapter"]
		btn.custom_minimum_size = Vector2(0, 44)

		var btn_style = StyleBoxFlat.new()
		btn_style.set_content_margin_all(10)
		btn_style.set_corner_radius_all(4)

		if unlocked:
			btn.text = "Ch%d — %s\n    %s" % [map_data["chapter"], map_data["name"], map_data["desc"]]
			btn_style.bg_color = Color(0.08, 0.1, 0.06, 0.9)
			btn_style.border_color = Color(0.35, 0.45, 0.25, 0.5)
			btn_style.set_border_width_all(1)
			btn.add_theme_color_override("font_color", Color(0.65, 0.75, 0.5))
			btn.add_theme_color_override("font_hover_color", Color(0.85, 0.95, 0.6))
		else:
			btn.text = "Ch%d — ???" % map_data["chapter"]
			btn_style.bg_color = Color(0.06, 0.06, 0.06, 0.7)
			btn_style.border_color = Color(0.2, 0.2, 0.2, 0.3)
			btn_style.set_border_width_all(1)
			btn.add_theme_color_override("font_color", Color(0.3, 0.3, 0.3))
			btn.disabled = true

		btn.add_theme_stylebox_override("normal", btn_style)
		var hover_s = btn_style.duplicate()
		hover_s.bg_color = Color(0.12, 0.16, 0.08, 0.95)
		hover_s.border_color = Color(0.6, 0.7, 0.35, 0.8)
		btn.add_theme_stylebox_override("hover", hover_s)
		btn.add_theme_stylebox_override("focus", hover_s)
		btn.add_theme_font_size_override("font_size", 13)

		if unlocked:
			var scene_path = map_data["scene"]
			btn.pressed.connect(func():
				AudioManager.play_sfx("confirm")
				travel_overlay.queue_free()
				_close()
				SceneTransition.change_scene_styled(scene_path)
			)
			btn.focus_entered.connect(func(): AudioManager.play_sfx("ui_hover"))

		vbox.add_child(btn)

	# 닫기
	var close_label = Label.new()
	close_label.text = "[ESC] Close"
	close_label.add_theme_font_size_override("font_size", 11)
	close_label.add_theme_color_override("font_color", Color(0.4, 0.35, 0.3))
	close_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	vbox.add_child(close_label)

	var close_handler = func(event: InputEvent):
		if event.is_action_pressed("cancel") or event.is_action_pressed("menu"):
			travel_overlay.queue_free()
			get_viewport().set_input_as_handled()
	travel_overlay.gui_input.connect(close_handler)

## S54: Ending Gallery
func _on_endings() -> void:
	AudioManager.play_sfx("ui_select")
	_show_endings_gallery()

func _show_endings_gallery() -> void:
	var end_overlay = ColorRect.new()
	end_overlay.set_anchors_preset(Control.PRESET_FULL_RECT)
	end_overlay.color = Color(0, 0, 0, 0)
	end_overlay.mouse_filter = Control.MOUSE_FILTER_STOP
	add_child(end_overlay)
	_add_modal_backdrop(end_overlay, ENDING_GALLERY_BACKDROP_PATH, Color(0.008, 0.006, 0.015, 0.30))

	var end_panel = PanelContainer.new()
	end_panel.anchor_left = 0.1
	end_panel.anchor_right = 0.9
	end_panel.anchor_top = 0.05
	end_panel.anchor_bottom = 0.95
	var style = StyleBoxFlat.new()
	style.bg_color = Color(0.025, 0.02, 0.04, 0.72)
	style.border_color = Color(0.62, 0.47, 0.24, 0.72)
	style.set_border_width_all(2)
	style.set_corner_radius_all(6)
	style.set_content_margin_all(20)
	end_panel.add_theme_stylebox_override("panel", style)
	end_overlay.add_child(end_panel)

	var vbox = VBoxContainer.new()
	vbox.add_theme_constant_override("separation", 12)
	end_panel.add_child(vbox)

	# Header
	var header = Label.new()
	header.text = "ENDING GALLERY  (%d / %d)" % [GameManager.seen_endings.size(), GameManager.ENDING_DATA.size()]
	header.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	header.add_theme_font_size_override("font_size", 20)
	header.add_theme_color_override("font_color", Color(0.85, 0.7, 0.4))
	UITheme.apply_title_font(header)
	vbox.add_child(header)

	var subtitle := Label.new()
	subtitle.text = "기억된 결말과 아직 닿지 못한 가능성" if GameManager.current_locale == "ko" else "Remembered conclusions and paths not yet reached."
	subtitle.add_theme_font_size_override("font_size", 12)
	subtitle.add_theme_color_override("font_color", Color(0.56, 0.52, 0.5))
	subtitle.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	vbox.add_child(subtitle)

	var sep = HSeparator.new()
	vbox.add_child(sep)

	# Grid of endings
	var grid = GridContainer.new()
	grid.columns = 3
	grid.add_theme_constant_override("h_separation", 16)
	grid.add_theme_constant_override("v_separation", 16)
	vbox.add_child(grid)

	var ending_ids = ["zero_burn", "preservation", "ash", "seam", "tobias", "hollow", "weave"]
	for eid in ending_ids:
		var seen = eid in GameManager.seen_endings
		var data = GameManager.ENDING_DATA.get(eid, {})

		var card_panel := PanelContainer.new()
		card_panel.custom_minimum_size = Vector2(250, 170)
		card_panel.size_flags_horizontal = Control.SIZE_EXPAND_FILL
		var card_style := StyleBoxFlat.new()
		card_style.bg_color = Color(0.06, 0.048, 0.075, 0.72) if seen else Color(0.025, 0.022, 0.032, 0.76)
		card_style.border_color = Color(0.58, 0.43, 0.22, 0.58) if seen else Color(0.19, 0.17, 0.2, 0.4)
		card_style.set_border_width_all(1)
		card_style.set_corner_radius_all(4)
		card_style.set_content_margin_all(7)
		card_panel.add_theme_stylebox_override("panel", card_style)
		grid.add_child(card_panel)

		var card = VBoxContainer.new()
		card.custom_minimum_size = Vector2(236, 156)
		card.add_theme_constant_override("separation", 6)
		card_panel.add_child(card)

		# Thumbnail area
		var thumb = ColorRect.new()
		thumb.custom_minimum_size = Vector2(236, 104)
		if seen:
			# Try to load CG image
			var cg_path = data.get("cg", "")
			if cg_path != "" and ResourceLoader.exists(cg_path):
				var tex_rect = TextureRect.new()
				tex_rect.custom_minimum_size = Vector2(236, 104)
				tex_rect.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
				tex_rect.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_COVERED
				tex_rect.texture = load(cg_path)
				card.add_child(tex_rect)
			else:
				# Fallback colored rect
				thumb.color = Color(0.15, 0.12, 0.18)
				card.add_child(thumb)
		else:
			# Locked — dark with lock icon
			thumb.color = Color(0.06, 0.05, 0.07)
			card.add_child(thumb)
			var lock_label = Label.new()
			lock_label.text = "?"
			lock_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
			lock_label.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
			lock_label.add_theme_font_size_override("font_size", 32)
			lock_label.add_theme_color_override("font_color", Color(0.25, 0.2, 0.18))
			lock_label.set_anchors_preset(Control.PRESET_FULL_RECT)
			thumb.add_child(lock_label)

		# Title
		var title_lbl = Label.new()
		title_lbl.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
		title_lbl.add_theme_font_size_override("font_size", 13)
		if seen:
			title_lbl.text = data.get("name", eid)
			title_lbl.add_theme_color_override("font_color", Color(0.85, 0.75, 0.5))
		else:
			title_lbl.text = "???"
			title_lbl.add_theme_color_override("font_color", Color(0.3, 0.25, 0.2))
		card.add_child(title_lbl)

		# Description (only if seen)
		var desc_lbl = Label.new()
		desc_lbl.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
		desc_lbl.add_theme_font_size_override("font_size", 10)
		desc_lbl.autowrap_mode = TextServer.AUTOWRAP_WORD
		desc_lbl.custom_minimum_size = Vector2(236, 0)
		if seen:
			desc_lbl.text = data.get("desc", "")
			desc_lbl.add_theme_color_override("font_color", Color(0.55, 0.5, 0.45))
		else:
			desc_lbl.text = "Reach this ending to unlock."
			desc_lbl.add_theme_color_override("font_color", Color(0.25, 0.22, 0.2))
		card.add_child(desc_lbl)

	# Close hint
	var close_label = Label.new()
	close_label.text = "[ESC] Close"
	close_label.add_theme_font_size_override("font_size", 11)
	close_label.add_theme_color_override("font_color", Color(0.4, 0.35, 0.3))
	close_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	vbox.add_child(close_label)

	var close_handler = func(event: InputEvent):
		if event.is_action_pressed("cancel") or event.is_action_pressed("menu"):
			end_overlay.queue_free()
			get_viewport().set_input_as_handled()
	end_overlay.gui_input.connect(close_handler)
	_animate_modal_panel(end_panel)

## S55: Statistics Screen
func _on_stats() -> void:
	AudioManager.play_sfx("ui_select")
	_show_stats_panel()

func _show_stats_panel() -> void:
	var stats_overlay = ColorRect.new()
	stats_overlay.set_anchors_preset(Control.PRESET_FULL_RECT)
	stats_overlay.color = Color(0, 0, 0, 0.7)
	stats_overlay.mouse_filter = Control.MOUSE_FILTER_STOP
	add_child(stats_overlay)

	var stats_panel = PanelContainer.new()
	stats_panel.anchor_left = 0.2
	stats_panel.anchor_right = 0.8
	stats_panel.anchor_top = 0.05
	stats_panel.anchor_bottom = 0.95
	var style = StyleBoxFlat.new()
	style.bg_color = Color(0.06, 0.05, 0.08, 0.98)
	style.border_color = Color(0.45, 0.55, 0.35, 0.7)
	style.set_border_width_all(2)
	style.set_corner_radius_all(6)
	style.set_content_margin_all(20)
	stats_panel.add_theme_stylebox_override("panel", style)
	stats_overlay.add_child(stats_panel)

	var vbox = VBoxContainer.new()
	vbox.add_theme_constant_override("separation", 10)
	stats_panel.add_child(vbox)

	# Header
	var header = Label.new()
	header.text = "PLAY STATISTICS"
	header.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	header.add_theme_font_size_override("font_size", 20)
	header.add_theme_color_override("font_color", Color(0.75, 0.85, 0.55))
	vbox.add_child(header)

	var sep = HSeparator.new()
	vbox.add_child(sep)

	# Scrollable stat list
	var scroll = ScrollContainer.new()
	scroll.size_flags_vertical = Control.SIZE_EXPAND_FILL
	scroll.horizontal_scroll_mode = ScrollContainer.SCROLL_MODE_DISABLED
	vbox.add_child(scroll)

	var list = VBoxContainer.new()
	list.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	list.add_theme_constant_override("separation", 6)
	scroll.add_child(list)

	var stats = GameManager.play_stats
	# S56: Completion percentage
	var completion = AchievementManager.get_completion_percentage()
	var stat_display = [
		{"label": "Play Time", "value": GameManager.format_play_time()},
		{"label": "Completion", "value": "%.1f%%" % completion},
		{"label": "Achievements", "value": "%d / %d" % [AchievementManager.unlocked.size(), AchievementManager.ACHIEVEMENTS.size()]},
		{"label": "Endings Seen", "value": "%d / %d" % [GameManager.seen_endings.size(), GameManager.ENDING_DATA.size()]},
		{"label": "", "value": ""},
		{"label": "Total Battles", "value": str(int(stats.total_battles))},
		{"label": "Enemies Defeated", "value": str(int(stats.enemies_defeated))},
		{"label": "Bosses Defeated", "value": str(int(stats.bosses_defeated))},
		{"label": "Memories Burned", "value": str(int(stats.total_burns))},
		{"label": "Memories Collected", "value": str(int(stats.memories_collected))},
		{"label": "Grains Earned", "value": str(int(stats.total_grains_earned))},
		{"label": "Steps Taken", "value": str(int(stats.steps_taken))},
		{"label": "Highest Combo", "value": str(int(stats.highest_combo))},
		{"label": "Highest Resonance", "value": str(int(stats.get("highest_momentum_rank", 0)))},
		{"label": "Objectives Completed", "value": str(int(stats.get("objectives_completed", 0)))},
		{"label": "Resonance Surges", "value": str(int(stats.get("momentum_surges", 0)))},
		{"label": "Items Used", "value": str(int(stats.items_used))},
		{"label": "", "value": ""},
		{"label": "Current Chapter", "value": str(GameManager.current_chapter)},
		{"label": "NG+ Cycle", "value": str(GameManager.ng_plus_cycle)},
		{"label": "Last Saved", "value": SaveManager.get_last_saved_text()},
	]

	for entry in stat_display:
		var row = HBoxContainer.new()
		row.add_theme_constant_override("separation", 8)
		list.add_child(row)

		var name_label = Label.new()
		name_label.text = entry.label
		name_label.size_flags_horizontal = Control.SIZE_EXPAND_FILL
		name_label.add_theme_font_size_override("font_size", 15)
		name_label.add_theme_color_override("font_color", Color(0.6, 0.58, 0.52))
		row.add_child(name_label)

		var val_label = Label.new()
		val_label.text = entry.value
		val_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
		val_label.add_theme_font_size_override("font_size", 15)
		val_label.add_theme_color_override("font_color", Color(0.9, 0.8, 0.5))
		val_label.custom_minimum_size = Vector2(120, 0)
		row.add_child(val_label)

	# Close hint
	var close_label = Label.new()
	close_label.text = "[ESC] Close"
	close_label.add_theme_font_size_override("font_size", 11)
	close_label.add_theme_color_override("font_color", Color(0.4, 0.35, 0.3))
	close_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	vbox.add_child(close_label)

	# ESC close handler
	var close_handler = func(event: InputEvent):
		if event.is_action_pressed("cancel") or event.is_action_pressed("menu"):
			stats_overlay.queue_free()
			get_viewport().set_input_as_handled()
	stats_overlay.gui_input.connect(close_handler)

## S59: Quit confirmation dialog
func _on_quit() -> void:
	AudioManager.play_sfx("ui_select")
	_show_quit_confirmation()

func _show_quit_confirmation() -> void:
	var confirm_overlay = ColorRect.new()
	confirm_overlay.set_anchors_preset(Control.PRESET_FULL_RECT)
	confirm_overlay.color = Color(0, 0, 0, 0.7)
	confirm_overlay.mouse_filter = Control.MOUSE_FILTER_STOP
	add_child(confirm_overlay)

	var confirm_panel = PanelContainer.new()
	confirm_panel.anchor_left = 0.3
	confirm_panel.anchor_right = 0.7
	confirm_panel.anchor_top = 0.35
	confirm_panel.anchor_bottom = 0.65
	var style = StyleBoxFlat.new()
	style.bg_color = Color(0.06, 0.05, 0.08, 0.98)
	style.border_color = Color(0.7, 0.4, 0.3, 0.7)
	style.set_border_width_all(2)
	style.set_corner_radius_all(6)
	style.set_content_margin_all(24)
	confirm_panel.add_theme_stylebox_override("panel", style)
	confirm_overlay.add_child(confirm_panel)

	var vbox = VBoxContainer.new()
	vbox.add_theme_constant_override("separation", 16)
	confirm_panel.add_child(vbox)

	var question = Label.new()
	question.text = "Are you sure you want to quit?"
	question.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	question.add_theme_font_size_override("font_size", 18)
	question.add_theme_color_override("font_color", Color(0.85, 0.7, 0.5))
	vbox.add_child(question)

	var hint = Label.new()
	hint.text = "Unsaved progress will be lost."
	hint.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	hint.add_theme_font_size_override("font_size", 12)
	hint.add_theme_color_override("font_color", Color(0.55, 0.45, 0.4, 0.7))
	vbox.add_child(hint)

	var btn_row = HBoxContainer.new()
	btn_row.alignment = BoxContainer.ALIGNMENT_CENTER
	btn_row.add_theme_constant_override("separation", 20)
	vbox.add_child(btn_row)

	var yes_btn = Button.new()
	yes_btn.text = "Yes, Quit"
	yes_btn.custom_minimum_size = Vector2(120, 40)
	var yes_style = StyleBoxFlat.new()
	yes_style.bg_color = Color(0.25, 0.1, 0.08, 0.9)
	yes_style.border_color = Color(0.7, 0.35, 0.25, 0.6)
	yes_style.set_border_width_all(1)
	yes_style.set_corner_radius_all(3)
	yes_style.set_content_margin_all(8)
	yes_btn.add_theme_stylebox_override("normal", yes_style)
	var yes_hover = yes_style.duplicate()
	yes_hover.border_color = Color(0.9, 0.45, 0.3, 0.9)
	yes_btn.add_theme_stylebox_override("hover", yes_hover)
	yes_btn.add_theme_stylebox_override("focus", yes_hover)
	yes_btn.add_theme_font_size_override("font_size", 15)
	yes_btn.add_theme_color_override("font_color", Color(0.85, 0.55, 0.4))
	yes_btn.add_theme_color_override("font_hover_color", Color(1.0, 0.7, 0.5))
	yes_btn.pressed.connect(func():
		get_tree().quit()
	)
	yes_btn.mouse_entered.connect(func(): AudioManager.play_sfx("ui_hover"))
	btn_row.add_child(yes_btn)

	var no_btn = Button.new()
	no_btn.text = "No, Stay"
	no_btn.custom_minimum_size = Vector2(120, 40)
	var no_style = StyleBoxFlat.new()
	no_style.bg_color = Color(0.1, 0.08, 0.12, 0.9)
	no_style.border_color = Color(0.35, 0.45, 0.3, 0.6)
	no_style.set_border_width_all(1)
	no_style.set_corner_radius_all(3)
	no_style.set_content_margin_all(8)
	no_btn.add_theme_stylebox_override("normal", no_style)
	var no_hover = no_style.duplicate()
	no_hover.border_color = Color(0.5, 0.7, 0.4, 0.9)
	no_btn.add_theme_stylebox_override("hover", no_hover)
	no_btn.add_theme_stylebox_override("focus", no_hover)
	no_btn.add_theme_font_size_override("font_size", 15)
	no_btn.add_theme_color_override("font_color", Color(0.6, 0.75, 0.5))
	no_btn.add_theme_color_override("font_hover_color", Color(0.75, 0.9, 0.6))
	no_btn.pressed.connect(func():
		AudioManager.play_sfx("ui_close")
		confirm_overlay.queue_free()
	)
	no_btn.mouse_entered.connect(func(): AudioManager.play_sfx("ui_hover"))
	btn_row.add_child(no_btn)

	# ESC closes the confirmation (No)
	var close_handler = func(event: InputEvent):
		if event.is_action_pressed("cancel") or event.is_action_pressed("menu"):
			AudioManager.play_sfx("ui_close")
			confirm_overlay.queue_free()
			get_viewport().set_input_as_handled()
	confirm_overlay.gui_input.connect(close_handler)

	# Focus the No button by default
	no_btn.grab_focus()
