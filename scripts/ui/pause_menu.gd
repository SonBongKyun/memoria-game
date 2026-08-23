## PauseMenu (Autoload), 일시정지 메뉴
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
var _active_artbook_items: Array[Dictionary] = []

const PAUSE_BACKDROP_PATH: String = "res://assets/cg/generated/ui_pause_archive_backdrop_v2.png"
const PAUSE_CONTROL_SLAB_PATH: String = "res://assets/cg/generated/ui_pause_control_slab.png"
const ACHIEVEMENTS_BACKDROP_PATH: String = "res://assets/cg/generated/ui_achievements_chronicle_backdrop.png"
const ENDING_GALLERY_BACKDROP_PATH: String = "res://assets/cg/generated/ui_ending_gallery_backdrop.png"
const WORLD_MAP_BACKDROP_PATH: String = "res://assets/cg/generated/ui_world_map_routes_v1.png"
const INVENTORY_BACKDROP_PATH: String = "res://assets/cg/generated/ui_inventory_archive_v2.png"
const STATUS_BACKDROP_PATH: String = "res://assets/cg/generated/ui_character_status_dossier_v1.png"
const INVENTORY_SLOT_ICON_PATHS: Dictionary = {
	"weapon": "res://assets/ui/equipment/slot_weapon_v1.png",
	"armor": "res://assets/ui/equipment/slot_armor_v1.png",
	"accessory": "res://assets/ui/equipment/slot_accessory_v1.png",
}
const SAVE_ARCHIVE_BACKDROP_PATH: String = "res://assets/cg/generated/ui_save_archive_v1.png"
const FIELD_GUIDE_BACKDROP_PATH: String = "res://assets/cg/generated/ui_field_guide_v1.png"
const EMPTY_SAVE_RECORD_PATH: String = "res://assets/cg/generated/ui_empty_witness_record_v1.png"

const SAVE_CHAPTER_ART := {
	1: "res://assets/cg/generated/story_ch1_twisted_forest_path.png",
	2: "res://assets/cg/generated/chapter_splash_verdan_market.png",
	3: "res://assets/cg/generated/chapter_splash_belt_waystation.png",
	4: "res://assets/cg/generated/chapter_splash_drift_shelter.png",
	5: "res://assets/cg/generated/chapter_splash_crumbling_coast.png",
	6: "res://assets/cg/generated/chapter_splash_the_seam.png",
	7: "res://assets/cg/generated/chapter_splash_seam_outskirts.png",
	8: "res://assets/cg/generated/chapter_splash_forgotten_forest.png",
	9: "res://assets/cg/generated/memory_compass_resonance_cinematic.png",
	10: "res://assets/cg/generated/chapter_splash_bl07_void.png",
}

const TRAVEL_DESTINATIONS: Array[Dictionary] = [
	{"name": "Rim Forest", "scene": "res://scenes/maps/rim_forest.tscn", "chapter": 1, "desc": "The first witnessed route."},
	{"name": "Verdan Market", "scene": "res://scenes/maps/verdan_market.tscn", "chapter": 2, "desc": "Memory, smoke, and exchange."},
	{"name": "Belt Waystation", "scene": "res://scenes/maps/belt_waystation.tscn", "chapter": 3, "desc": "Relay Fourteen on the dead road."},
	{"name": "Drift Shelter", "scene": "res://scenes/maps/drift_shelter.tscn", "chapter": 4, "desc": "A page held against the storm."},
	{"name": "Crumbling Coast", "scene": "res://scenes/maps/crumbling_coast.tscn", "chapter": 5, "desc": "Two paths above a dissolving sea."},
	{"name": "The Seam", "scene": "res://scenes/maps/the_seam.tscn", "chapter": 6, "desc": "A refuge held by seven lanterns."},
	{"name": "Seam Outskirts", "scene": "res://scenes/maps/seam_outskirts.tscn", "chapter": 7, "desc": "The witnessed edge of BL-07."},
	{"name": "Forgotten Forest", "scene": "res://scenes/maps/forgotten_forest.tscn", "chapter": 8, "desc": "Listening wood and feeding rings."},
	{"name": "Colorless Waste", "scene": "res://scenes/maps/colorless_waste.tscn", "chapter": 9, "desc": "Where testimony replaces north."},
	{"name": "BL-07 Void", "scene": "res://scenes/maps/bl07_void.tscn", "chapter": 10, "desc": "The final recorded threshold."},
]

const CHAPTER_EXPANSION_GALLERY_PATH := "res://data/chapter_expansion_gallery.json"
const INTERFACE_EXPANSION_GALLERY_PATH := "res://data/interface_visual_gallery.json"
const ILLUSTRATION_EXPANSION_GALLERY_PATH := "res://data/illustration_expansion_gallery.json"
const ILLUSTRATION_GAPFILL_GALLERY_PATH := "res://data/illustration_gapfill_gallery.json"
const WORLD_POPULATION_GALLERY_PATH := "res://data/world_population_visual_gallery.json"
const OCCULT_BOSS_GALLERY_PATH := "res://data/occult_boss_gallery.json"
const ARTBOOK_ITEMS: Array[Dictionary] = [
	{
		"title": "Ash Hound - Cinder Pursuit",
		"type": "Cinematic Enemy Action Cut-In",
		"path": "res://assets/cg/generated/battle_stage_v2/enemy_ash_hound_battle_v1.png",
		"desc": "A wide combat beat for the recurring ash hound's ember-fissured pursuit."
	},
	{
		"title": "Signal Wisp - Drowned Relay",
		"type": "Cinematic Enemy Action Cut-In",
		"path": "res://assets/cg/generated/battle_stage_v2/enemy_signal_wisp_battle_v1.png",
		"desc": "A wide combat beat for the captive memory flame crossing a drowned relay."
	},
	{
		"title": "Rootbound Echo - The Forest Holds",
		"type": "Cinematic Enemy Action Cut-In",
		"path": "res://assets/cg/generated/battle_stage_v2/enemy_rootbound_echo_battle_v1.png",
		"desc": "A wide combat beat for the hooded echo reaching through the roots that bind it."
	},
	{
		"title": "Void Fragment - Suspended Fault",
		"type": "Cinematic Enemy Action Cut-In",
		"path": "res://assets/cg/generated/battle_stage_v2/enemy_void_fragment_battle_v1.png",
		"desc": "A wide combat beat for a BL-07 fault splitting into orbiting violet shards."
	},
	{
		"title": "Ash Hound - Stage Character",
		"type": "Cinematic Enemy Stage Character",
		"path": "res://assets/cg/generated/battle_stage_v2/enemy_ash_hound_stage_v1.png",
		"desc": "Bone mask, iron charm, and ember fissures remain readable at the live battler scale."
	},
	{
		"title": "Signal Wisp - Stage Character",
		"type": "Cinematic Enemy Stage Character",
		"path": "res://assets/cg/generated/battle_stage_v2/enemy_signal_wisp_stage_v1.png",
		"desc": "The lantern cage, torn signal cloth, and captive memory flame form one readable combat silhouette."
	},
	{
		"title": "Rootbound Echo - Stage Character",
		"type": "Cinematic Enemy Stage Character",
		"path": "res://assets/cg/generated/battle_stage_v2/enemy_rootbound_echo_stage_v1.png",
		"desc": "Faceless smoke, bark robes, and pale bindings survive the stage blend at gameplay scale."
	},
	{
		"title": "Void Fragment - Stage Character",
		"type": "Cinematic Enemy Stage Character",
		"path": "res://assets/cg/generated/battle_stage_v2/enemy_void_fragment_stage_v1.png",
		"desc": "The suspended crystal and its orbiting shards fill the enemy slot without losing their void silhouette."
	},
	{
		"title": "The Seam - Reunion at the Threshold",
		"type": "Generated Story Archive",
		"path": "res://assets/cg/generated/archive_ch6_reunion_v1.png",
		"desc": "The anchor-thread reconnects between Arrel and Elia at the lantern path into the refuge."
	},
	{
		"title": "Witnessed Routes - World Map",
		"type": "Interface Art",
		"path": "res://assets/cg/generated/ui_world_map_routes_v1.png",
		"desc": "A text-safe route atlas connecting all ten playable regions with one amber memory thread."
	},
	{
		"title": "Field Archive - Inventory Panel",
		"type": "Interface Art",
		"path": "res://assets/cg/generated/ui_inventory_archive_v1.png",
		"desc": "The item list, inspection cradle, and equipped record now share one readable archive surface."
	},
	{
		"title": "Witness Archive - Save Records",
		"type": "Interface Art",
		"path": "res://assets/cg/generated/ui_save_archive_v1.png",
		"desc": "Autosave and three manual records are compared as witnessed states before saving or returning."
	},
	{
		"title": "Field Guide - Player Reference",
		"type": "Interface Art",
		"path": "res://assets/cg/generated/ui_field_guide_v1.png",
		"desc": "Controls, exploration reading, battle flow, and memory-burning rules share a quiet reference plate."
	},
	{
		"title": "Drift - Anchoring Session",
		"type": "Generated Story Archive",
		"path": "res://assets/cg/generated/archive_drift_anchoring_v1.png",
		"desc": "Elia gives a damaged memory a place to settle: warm tea, a blank book, and a thread held by hand."
	},
	{
		"title": "Verdan - The Price of a Memory",
		"type": "Generated Story Archive",
		"path": "res://assets/cg/generated/archive_verdan_memory_price_v1.png",
		"desc": "Malet turns an ordinary lived moment into a commodity while Arrel decides what he can afford to lose."
	},
	{
		"title": "Belt - A Hand-Copied Route",
		"type": "Generated Story Archive",
		"path": "res://assets/cg/generated/archive_belt_blank_book_v1.png",
		"desc": "Tobias keeps a route alive in the Blank Book, outside the Authority's official record."
	},
	{
		"title": "The Seam - Lantern Watch",
		"type": "Generated Story Archive",
		"path": "res://assets/cg/generated/archive_seam_lantern_watch_v1.png",
		"desc": "Sable's refuge is held together by practical rituals: a lit gate, a marked route, and a neighbor expected home."
	},
	{
		"title": "Sable - Void Walker Turnaround",
		"type": "Character Sheet",
		"path": "res://assets/game_image/reference/sable_reference_turnaround_v1.png",
		"desc": "Canonical age, hood, lantern, ward token, palette, equipment, and field-pose reference for Sable."
	},
	{
		"title": "The Seam - Resident Roles",
		"type": "World Character Sheet",
		"path": "res://assets/game_image/reference/seam_residents_reference_sheet_v1.png",
		"desc": "Lantern keeper, witness scribe, impossible-garden tender, and gate scout: the work that keeps a refuge alive."
	},
	{
		"title": "Crumbling Coast - The Parting",
		"type": "Generated Story Archive",
		"path": "res://assets/cg/generated/archive_ch5_coastal_parting_v1.png",
		"desc": "Arrel and Elia choose separate routes to divide Kairos's attention, placing the anchor itself at risk."
	},
	{
		"title": "The Seam - Seven Lanterns",
		"type": "Generated Story Archive",
		"path": "res://assets/cg/generated/archive_ch7_seven_lanterns_v1.png",
		"desc": "Seven extinguished lanterns and one survivor turn Sable's warning about BL-07 into a witnessed loss."
	},
	{
		"title": "Colorless Waste - Two Outcomes",
		"type": "Generated Story Archive",
		"path": "res://assets/cg/generated/archive_ch9_kairos_outcomes_v1.png",
		"desc": "Kairos offers two precise futures while refusing the responsibility of choosing who will pay for either one."
	},
	{
		"title": "BL-07 - A Choice of Burdens",
		"type": "Generated Story Archive",
		"path": "res://assets/cg/generated/archive_ch10_burden_choice_v1.png",
		"desc": "Arrel stands between the fading name, the shared witness, and the open wound at the heart of the Seal."
	},
	{
		"title": "Story Archive - World Atlas Desk",
		"type": "Interface Art",
		"path": "res://assets/cg/generated/ui_story_archive_atlas_v2.png",
		"desc": "The text-safe archive table now framing the Story Journal and its ten-route record."
	},
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
		"path": "res://assets/cg/generated/illustration_expansion_v2/world_rewrite_reaching_hand_v3.png",
		"desc": "Act I relationship-memory illustration now used in early story and burn-residue beats."
	},
	{
		"title": "Memory Burn - Name Origin",
		"type": "Generated Memory CG",
		"path": "res://assets/cg/generated/illustration_expansion_v2/world_rewrite_name_origin_v3.png",
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
		"path": "res://assets/cg/generated/illustration_expansion_v2/world_rewrite_first_sword_v3.png",
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
		"path": "res://assets/cg/generated/illustration_expansion_v2/world_rewrite_first_sword_v3.png",
		"desc": "Battle cut-in for burning the memory of first holding a sword."
	},
	{
		"title": "Burn Cut-In: Campfire Song",
		"type": "Generated Battle Cut-In",
		"path": "res://assets/cg/generated/illustration_expansion_v2/world_rewrite_campfire_song_v3.png",
		"desc": "Battle cut-in for burning the campfire song tied to Elia."
	},
	{
		"title": "Burn Cut-In: Reaching Hand",
		"type": "Generated Battle Cut-In",
		"path": "res://assets/cg/generated/illustration_expansion_v2/world_rewrite_reaching_hand_v3.png",
		"desc": "Battle cut-in for burning the memory of a hand reaching out."
	},
	{
		"title": "Burn Cut-In: Arrel's Name",
		"type": "Generated Battle Cut-In",
		"path": "res://assets/cg/generated/illustration_expansion_v2/world_rewrite_name_origin_v3.png",
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
		"title": "Act I: The Sky Opens",
		"type": "Generated Opening Establishing CG",
		"path": "res://assets/cg/generated/story_ch1_rim_omen.png",
		"desc": "Arrel and Elia face the reversed ashfall, a descending Void Beast, and the distant Bureau road."
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
		"path": "res://assets/cg/generated/archive_ch1_camp_humming_v2.png",
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
		"title": "Act II: Seventeen Borrowed Eyes",
		"type": "Generated Character Revelation CG",
		"path": "res://assets/cg/generated/story_ch2_malet_seventeen_eyes.png",
		"desc": "Malet's amber gaze holds the lamplight of seventeen lives he rebuilt himself from."
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
		"path": "res://assets/cg/generated/archive_ch2_information_price_v1.png",
		"desc": "Malet lays out the coast route, Sable's name, and Kairos's warning after the price has been paid."
	},
	{
		"title": "Act II: The Recorder",
		"type": "Generated Character Introduction CG",
		"path": "res://assets/cg/generated/story_ch3_tobias_waystation.png",
		"desc": "Arrel and Elia interrupt Tobias's solitary accounting at the ruined Belt waystation."
	},
	{
		"title": "Act II: Records in the Air",
		"type": "Generated Character Moment CG",
		"path": "res://assets/cg/generated/story_ch3_tobias_record_spill_v2.png",
		"desc": "Tobias loses his bound journals to the Belt wind as Arrel and Elia cross the ruined station threshold."
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
		"path": "res://assets/cg/generated/archive_ch4_reading_loss_v1.png",
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
		"title": "Act II: The Shape You Keep",
		"type": "Generated Relationship Story CG",
		"path": "res://assets/cg/generated/story_ch5_threaded_horizon_v2.png",
		"desc": "On the gray coast, Arrel and Elia almost let go of the one thin anchor that keeps a memory from becoming nothing."
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
		"title": "Act III: What Remains",
		"type": "Generated Burn Aftermath CG",
		"path": "res://assets/cg/generated/story_ch7_residue_witness_v2.png",
		"desc": "Elia steadies Arrel while Sable reads the small residue that proves a chosen burn need not erase the self."
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
		"path": "res://assets/cg/generated/archive_ch8_ring_theory_v1.png",
		"desc": "Tobias traces the forest's hidden order while Sable remembers who was lost."
	},
	{
		"title": "Act III: Whispers as Bait",
		"type": "Generated Forest CG",
		"path": "res://assets/cg/generated/story_ch8_whispers_as_bait.png",
		"desc": "Borrowed faces gather in the bark around the real four travelers."
	},
	{
		"title": "Act III: Hold the Name",
		"type": "Generated Anchoring CG",
		"path": "res://assets/cg/generated/story_ch8_anchor_under_roots_v2.png",
		"desc": "Elia holds Arrel's hands beneath the watching roots and keeps one true name from scattering."
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
		"path": "res://assets/cg/generated/archive_ch3_kairos_marks_v1.png",
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
	{
		"title": "A Grip Before the Flame",
		"type": "Part I Memory CG",
		"path": "res://assets/cg/generated/story_ch2_lost_instructor_grip.png",
		"desc": "A first sword lesson becomes a gold fragment before Malet's extractor closes the drawer."
	},
	{
		"title": "The Book That Remembers Shape",
		"type": "Part I Story CG",
		"path": "res://assets/cg/generated/story_ch3_blank_book_warmth.png",
		"desc": "The Blank Book warms in Arrel's hands, keeping the contour of what fire removes."
	},
	{
		"title": "One Small Memory Anchored",
		"type": "Part I Character CG",
		"path": "res://assets/cg/generated/story_ch4_anchor_binding.png",
		"desc": "Elia presses the warmth of a remembered bakery back into the architecture of a life."
	},
	{
		"title": "South Along the Cliff",
		"type": "Part I Branch CG",
		"path": "res://assets/cg/generated/story_ch5_elia_southbound.png",
		"desc": "Elia takes the southern path without looking back, leaving the anchor loose behind her."
	},
	{
		"title": "The Map Sable Folded",
		"type": "Part I Character CG",
		"path": "res://assets/cg/generated/story_ch6_sable_buried_maps.png",
		"desc": "Sable folds seven losses into one precise map while the Seam darkens outside."
	},
	{
		"title": "The Mouth Behind BL-07",
		"type": "Part I Lore CG",
		"path": "res://assets/cg/generated/story_ch7_hungry_mouth.png",
		"desc": "The party sees the appetite behind the Void before they understand its cost."
	},
	{
		"title": "The Kindness of Passing By",
		"type": "Part I Story CG",
		"path": "res://assets/cg/generated/story_ch8_silent_remnant.png",
		"desc": "A wordless remnant remains in the forest while the party refuses to feed it attention."
	},
	{
		"title": "A Model Walks Away",
		"type": "Part I Story CG",
		"path": "res://assets/cg/generated/story_ch9_kairos_absence.png",
		"desc": "Kairos leaves the two official outcomes behind him in a landscape without color."
	},
	{
		"title": "What the Void Cannot Take",
		"type": "Part I Climax CG",
		"path": "res://assets/cg/generated/story_ch10_choice_at_core.png",
		"desc": "At BL-07's core, Arrel and Elia hold onto the freedom to choose."
	},
	{
		"title": "Sable - Void-Sense Battle Stance",
		"type": "Canonical Battle Character",
		"path": "res://assets/portraits/character_shots/sable_warden_v3.png",
		"desc": "Blind old Sable listens across the battlefield, knife low and every wasted movement removed."
	},
	{
		"title": "Tobias - Record Ward Stance",
		"type": "Canonical Battle Character",
		"path": "res://assets/portraits/character_shots/tobias_ledger_v3.png",
		"desc": "Tobias turns a field record into a precise ward without leaving the battle line."
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
			if not _close_active_archive_modal():
				_close()
			get_viewport().set_input_as_handled()
		elif _can_open_pause_menu():
			_open()
			get_viewport().set_input_as_handled()

func _can_open_pause_menu() -> bool:
	if MemoryUI.is_open:
		return false
	# S209: 회상 기록이 열려 있으면(게임패드 Start 등 cancel이 아닌 입력으로도) 겹치지 않게 한다.
	if has_node("/root/StoryLog") and StoryLog.is_open:
		return false
	if GameManager.current_state == GameManager.GameState.EXPLORATION:
		return true
	# S78: Full-VN pivot 이후에는 대부분의 플레이 시간이 DIALOGUE(SceneFlow) 상태다.
	# Artbook / Save / Options에 접근할 수 있도록 VN 진행 중에도 ESC 메뉴를 허용한다.
	return GameManager.current_state == GameManager.GameState.DIALOGUE and has_node("/root/SceneFlow") and SceneFlow.is_active

func _close_active_archive_modal() -> bool:
	# Close the front-most archive surface before dismissing the whole pause
	# menu. This keeps Esc predictable even while a child button owns focus.
	for modal_name in ["SaveArchiveOverlay", "FieldGuideOverlay", "InventoryOverlay", "WorldMapOverlay"]:
		var modal := get_node_or_null(modal_name)
		if modal != null:
			AudioManager.play_sfx("ui_close")
			modal.queue_free()
			return true
	return false

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
	var button_scroll := ScrollContainer.new()
	button_scroll.size_flags_vertical = Control.SIZE_EXPAND_FILL
	button_scroll.horizontal_scroll_mode = ScrollContainer.SCROLL_MODE_DISABLED
	vbox.add_child(button_scroll)

	btn_container = VBoxContainer.new()
	btn_container.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	btn_container.add_theme_constant_override("separation", 7)
	button_scroll.add_child(btn_container)

	# S65 (A안 피벗): VN 정체성에 맞게 메뉴 슬림화.
	# 숨김: Fast Travel, Stats, Load Autosave (RPG 기능, 스토리 몰입 방해)
	# 유지: Resume, Journal, Codex, Achievements (Steam 기대치), Endings, Options, Save/Load, Title, Quit
	var buttons = [
		{"text": GameManager.loc("resume"), "callback": _close},
		{"text": GameManager.loc("journal"), "callback": _on_journal},
		# S209: 회상 기록 (지나간 대사 되짚기). 게임 중에는 L 키로도 열린다.
		{"text": "STORY LOG" if GameManager.current_locale != "ko" else "회상 기록", "callback": _on_story_log},
		{"text": _ploc("ITEM ARCHIVE", "소지품 기록고"), "callback": _on_inventory},
		{"text": _ploc("WORLD MAP", "세계 지도"), "callback": _on_travel},
		{"text": "FIELD GUIDE" if GameManager.current_locale != "ko" else "필드 가이드", "callback": _on_field_guide},
		{"text": GameManager.loc("codex"), "callback": _on_codex},
		{"text": _ploc("Artbook", "삽화집"), "callback": _on_artbook},
		{"text": GameManager.loc("achievements"), "callback": _on_achievements},
	]
	# S54: Endings button (only if at least 1 ending seen)
	if GameManager.seen_endings.size() > 0:
		buttons.append({"text": GameManager.loc("endings"), "callback": _on_endings})
	buttons.append_array([
		{"text": GameManager.loc("options"), "callback": _on_options},
		{"text": "SAVE ARCHIVE" if GameManager.current_locale != "ko" else "저장 기록고", "callback": _on_save_archive},
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
	last_saved_label.add_theme_font_size_override("font_size", 13)
	last_saved_label.add_theme_color_override("font_color", Color(0.45, 0.55, 0.35))
	vbox.add_child(last_saved_label)

	# 하단 조작법, S56: Dynamic hints based on input mode
	pause_hint_label = Label.new()
	pause_hint_label.name = "HintLabel"
	pause_hint_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	pause_hint_label.add_theme_font_size_override("font_size", 13)
	pause_hint_label.add_theme_color_override("font_color", Color(0.5, 0.47, 0.42))
	UITheme.apply_ui_font(pause_hint_label)
	vbox.add_child(pause_hint_label)
	_update_hint_text(pause_hint_label, last_saved_label)

## S56: Update hint text based on input mode
func _update_hint_text(hint_label: Label, last_saved: Label) -> void:
	if GameManager.current_locale == "ko":
		if InputManager and InputManager.is_controller_mode():
			hint_label.text = InputManager.get_hint("cancel", "닫기")
		else:
			hint_label.text = "F6 빠른 저장  |  F7 빠른 불러오기  |  [Esc] 닫기"
		last_saved.text = SaveManager.get_last_saved_text()
		return
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
	var ch_name = GameManager.localized_chapter_name(ch, String(chapter_name.get(ch, "Unknown")))
	var text = _ploc("Chapter %d, %s%s\n", "%d장 · %s%s\n") % [ch, ch_name, ng_text]
	text += _ploc("HP: %d / %d\n", "HP %d / %d\n") % [hp, max_hp]
	text += _ploc("Memories: %d held, %d burned", "기억 %d개 보유 · %d개 연소") % [memory_count - burn_count, burn_count]
	if WorldRewriteDirector and WorldRewriteDirector.has_method("get_loss_records"):
		text += _ploc("\nLoss records: %d", "\n상실 기록 %d건") % WorldRewriteDirector.get_loss_records().size()

	# S57: Enhanced save slot display with chapter name, HP, grains, and playtime
	var ch_names = {1: "Rim Forest", 2: "Verdan Market", 3: "Belt Waystation", 4: "Drift Shelter", 5: "Crumbling Coast", 6: "The Seam", 7: "Seam Outskirts", 8: "Forgotten Forest", 9: "Colorless Waste", 10: "BL-07 Void", 11: "Epilogue"}

	var save = SaveManager.get_save_info(1)
	if save.is_empty():
		text += _ploc("\n\nSlot 1: [Empty]", "\n\n슬롯 1 · 비어 있음")
	else:
		var s_ch = save.get("chapter", 1)
		var s_ch_name = GameManager.localized_chapter_name(s_ch, String(ch_names.get(s_ch, "Unknown")))
		var s_hp = save.get("hp", 0)
		var s_max_hp = save.get("max_hp", 100)
		var s_grains = save.get("grains", 0)
		var s_location = save.get("location", "")
		text += _ploc("\n\nSlot 1: Ch%d - %s", "\n\n슬롯 1 · %d장 %s") % [s_ch, s_ch_name]
		if s_location != "":
			text += " (%s)" % s_location
		text += _ploc("\n    HP: %d/%d | Grains: %d | %s", "\n    HP %d/%d · %d 그레인 · %s") % [s_hp, s_max_hp, s_grains, save.get("timestamp", "?")]

	# S56/S57: Autosave slot info (enhanced)
	var auto_save = SaveManager.get_save_info(0)
	if not auto_save.is_empty():
		var a_ch = auto_save.get("chapter", 1)
		var a_ch_name = GameManager.localized_chapter_name(a_ch, String(ch_names.get(a_ch, "Unknown")))
		var a_hp = auto_save.get("hp", 0)
		var a_max_hp = auto_save.get("max_hp", 100)
		text += _ploc("\nAutosave: Ch%d - %s | HP: %d/%d | %s", "\n자동 저장 · %d장 %s · HP %d/%d · %s") % [a_ch, a_ch_name, a_hp, a_max_hp, auto_save.get("timestamp", "?")]

	save_info_label.text = text.replace("??", "· ")

## S242: 일시정지 정보 블록과 일부 버튼이 한국어 로케일에서도 영어였다.
func _ploc(en: String, ko: String) -> String:
	return ko if GameManager.current_locale == "ko" else en

func _on_save_archive() -> void:
	AudioManager.play_sfx("ui_select")
	_show_save_archive_panel()

func _show_save_archive_panel() -> void:
	if get_node_or_null("SaveArchiveOverlay") != null:
		return
	var save_overlay := ColorRect.new()
	save_overlay.name = "SaveArchiveOverlay"
	save_overlay.set_anchors_preset(Control.PRESET_FULL_RECT)
	save_overlay.color = Color(0, 0, 0, 0)
	save_overlay.mouse_filter = Control.MOUSE_FILTER_STOP
	add_child(save_overlay)
	_add_modal_backdrop(save_overlay, SAVE_ARCHIVE_BACKDROP_PATH, Color(0.004, 0.004, 0.009, 0.08))

	var content := Control.new()
	content.set_anchors_preset(Control.PRESET_FULL_RECT)
	save_overlay.add_child(content)

	var header := Label.new()
	header.anchor_left = 0.06
	header.anchor_right = 0.94
	header.anchor_top = 0.025
	header.anchor_bottom = 0.09
	header.text = _pause_loc("WITNESS ARCHIVE / SAVE RECORDS", "목격 기록고 / 저장 기록")
	header.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	header.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	header.add_theme_font_size_override("font_size", 22)
	header.add_theme_color_override("font_color", Color(0.92, 0.76, 0.44))
	UITheme.apply_title_font(header)
	content.add_child(header)

	var slot_list := VBoxContainer.new()
	slot_list.anchor_left = 0.065
	slot_list.anchor_right = 0.535
	slot_list.anchor_top = 0.105
	slot_list.anchor_bottom = 0.88
	slot_list.add_theme_constant_override("separation", 6)
	content.add_child(slot_list)

	var slot_buttons: Array[Button] = []
	for slot in range(SaveManager.MAX_SLOTS + 1):
		var slot_btn := Button.new()
		slot_btn.custom_minimum_size = Vector2(0, 112)
		slot_btn.size_flags_vertical = Control.SIZE_EXPAND_FILL
		slot_btn.alignment = HORIZONTAL_ALIGNMENT_LEFT
		slot_btn.toggle_mode = true
		slot_btn.expand_icon = true
		slot_btn.add_theme_font_size_override("font_size", 13)
		slot_btn.add_theme_color_override("font_color", Color(0.76, 0.70, 0.61))
		slot_btn.add_theme_color_override("font_hover_color", Color(1.0, 0.84, 0.52))
		var slot_style := StyleBoxFlat.new()
		slot_style.bg_color = Color(0.018, 0.016, 0.024, 0.46)
		slot_style.border_color = Color(0.36, 0.28, 0.18, 0.42)
		slot_style.set_border_width_all(1)
		slot_style.set_corner_radius_all(3)
		slot_style.set_content_margin_all(10)
		slot_style.content_margin_left = 16
		slot_btn.add_theme_stylebox_override("normal", slot_style)
		var slot_hover := slot_style.duplicate()
		slot_hover.bg_color = Color(0.08, 0.06, 0.08, 0.72)
		slot_hover.border_color = Color(0.78, 0.57, 0.28, 0.82)
		slot_btn.add_theme_stylebox_override("hover", slot_hover)
		slot_btn.add_theme_stylebox_override("focus", slot_hover)
		var slot_pressed := slot_hover.duplicate()
		slot_pressed.bg_color = Color(0.12, 0.085, 0.06, 0.82)
		slot_btn.add_theme_stylebox_override("pressed", slot_pressed)
		slot_btn.add_theme_stylebox_override("hover_pressed", slot_pressed)
		slot_list.add_child(slot_btn)
		slot_buttons.append(slot_btn)

	var preview_title := Label.new()
	preview_title.anchor_left = 0.575
	preview_title.anchor_right = 0.925
	preview_title.anchor_top = 0.12
	preview_title.anchor_bottom = 0.18
	preview_title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	preview_title.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	preview_title.add_theme_font_size_override("font_size", 19)
	preview_title.add_theme_color_override("font_color", Color(0.92, 0.76, 0.46))
	content.add_child(preview_title)

	var preview := TextureRect.new()
	preview.anchor_left = 0.585
	preview.anchor_right = 0.915
	preview.anchor_top = 0.19
	preview.anchor_bottom = 0.53
	preview.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	preview.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_COVERED
	preview.mouse_filter = Control.MOUSE_FILTER_IGNORE
	preview.modulate = Color(0.82, 0.78, 0.74, 0.78)
	content.add_child(preview)

	var details := RichTextLabel.new()
	details.anchor_left = 0.585
	details.anchor_right = 0.915
	details.anchor_top = 0.55
	details.anchor_bottom = 0.77
	details.bbcode_enabled = true
	details.fit_content = false
	details.scroll_active = false
	details.add_theme_font_size_override("normal_font_size", 14)
	details.add_theme_color_override("default_color", Color(0.76, 0.70, 0.62))
	content.add_child(details)

	var action_row := HBoxContainer.new()
	action_row.anchor_left = 0.605
	action_row.anchor_right = 0.895
	action_row.anchor_top = 0.80
	action_row.anchor_bottom = 0.87
	action_row.alignment = BoxContainer.ALIGNMENT_CENTER
	action_row.add_theme_constant_override("separation", 12)
	content.add_child(action_row)

	var save_btn := _make_archive_action_button(_pause_loc("SAVE HERE", "여기에 저장"), Color(0.74, 0.55, 0.28))
	action_row.add_child(save_btn)
	var load_btn := _make_archive_action_button(_pause_loc("RETURN HERE", "이 기록으로 돌아가기"), Color(0.42, 0.63, 0.72))
	action_row.add_child(load_btn)

	var footer := Label.new()
	footer.anchor_left = 0.565
	footer.anchor_right = 0.93
	footer.anchor_top = 0.89
	footer.anchor_bottom = 0.94
	footer.text = _pause_loc("Autosave is protected. Manual records keep a backup copy.", "자동 저장은 보호됩니다. 수동 저장은 이전 기록을 백업합니다.")
	footer.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	footer.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	footer.add_theme_font_size_override("font_size", 13)
	footer.add_theme_color_override("font_color", Color(0.52, 0.48, 0.43))
	content.add_child(footer)

	var close_btn := Button.new()
	close_btn.anchor_left = 0.925
	close_btn.anchor_right = 0.975
	close_btn.anchor_top = 0.025
	close_btn.anchor_bottom = 0.085
	close_btn.text = "X"
	close_btn.flat = true
	close_btn.add_theme_font_size_override("font_size", 18)
	close_btn.add_theme_color_override("font_color", Color(0.68, 0.58, 0.44))
	close_btn.pressed.connect(func():
		AudioManager.play_sfx("ui_close")
		save_overlay.queue_free()
	)
	content.add_child(close_btn)

	var state := {"selected": 1, "confirm_slot": -1}
	for slot in range(slot_buttons.size()):
		slot_buttons[slot].pressed.connect(_select_save_archive_slot.bind(slot, state, slot_buttons, preview, preview_title, details, save_btn, load_btn))
		slot_buttons[slot].focus_entered.connect(func(): AudioManager.play_sfx("ui_hover"))
	save_btn.pressed.connect(_save_archive_write.bind(state, slot_buttons, preview, preview_title, details, save_btn, load_btn))
	load_btn.pressed.connect(_save_archive_load.bind(state, save_overlay))
	_refresh_save_archive_buttons(slot_buttons)
	_select_save_archive_slot(1, state, slot_buttons, preview, preview_title, details, save_btn, load_btn)
	var close_handler = func(event: InputEvent):
		if event.is_action_pressed("cancel") or event.is_action_pressed("menu"):
			save_overlay.queue_free()
			get_viewport().set_input_as_handled()
	save_overlay.gui_input.connect(close_handler)

func _make_archive_action_button(text: String, accent: Color) -> Button:
	var button := Button.new()
	button.text = text
	button.custom_minimum_size = Vector2(172, 42)
	button.add_theme_font_size_override("font_size", 13)
	button.add_theme_color_override("font_color", Color(0.84, 0.79, 0.69))
	var style := StyleBoxFlat.new()
	style.bg_color = Color(0.035, 0.031, 0.043, 0.88)
	style.border_color = Color(accent.r, accent.g, accent.b, 0.62)
	style.set_border_width_all(1)
	style.set_corner_radius_all(3)
	style.set_content_margin_all(8)
	button.add_theme_stylebox_override("normal", style)
	var hover := style.duplicate()
	hover.bg_color = Color(accent.r * 0.20, accent.g * 0.20, accent.b * 0.20, 0.95)
	hover.border_color = Color(accent.r, accent.g, accent.b, 0.95)
	button.add_theme_stylebox_override("hover", hover)
	button.add_theme_stylebox_override("focus", hover)
	return button

func _refresh_save_archive_buttons(buttons: Array[Button]) -> void:
	for slot in range(buttons.size()):
		var info := SaveManager.get_save_info(slot)
		buttons[slot].text = _save_archive_slot_text(slot, info)
		var art_path := EMPTY_SAVE_RECORD_PATH if info.is_empty() else _save_archive_art_path(info)
		buttons[slot].icon = load(art_path) if art_path != "" and ResourceLoader.exists(art_path) else null

func _save_archive_slot_text(slot: int, info: Dictionary) -> String:
	var slot_name := _pause_loc("AUTOSAVE", "자동 저장") if slot == 0 else _pause_loc("MANUAL RECORD %d" % slot, "수동 기록 %d" % slot)
	if info.is_empty():
		return "%s\n%s" % [slot_name, _pause_loc("Empty witness record", "비어 있는 목격 기록")]
	var chapter := int(info.get("chapter", 1))
	var location := _save_archive_location_name(info)
	var stamp := String(info.get("timestamp", "")).replace("T", "  ")
	return "%s\nCh.%02d  ·  %s\nHP %d/%d  ·  %s" % [slot_name, chapter, location, int(info.get("hp", 0)), int(info.get("max_hp", 100)), stamp]

func _select_save_archive_slot(slot: int, state: Dictionary, buttons: Array[Button], preview: TextureRect, preview_title: Label, details: RichTextLabel, save_btn: Button, load_btn: Button) -> void:
	state["selected"] = slot
	state["confirm_slot"] = -1
	for index in range(buttons.size()):
		buttons[index].button_pressed = index == slot
	var info := SaveManager.get_save_info(slot)
	var slot_label := _pause_loc("AUTOSAVE", "자동 저장") if slot == 0 else _pause_loc("MANUAL RECORD %d" % slot, "수동 기록 %d" % slot)
	preview_title.text = slot_label
	var art_path := _save_archive_art_path(info)
	preview.texture = load(art_path) if art_path != "" and ResourceLoader.exists(art_path) else null
	if info.is_empty():
		details.text = "[center][color=#bca77b]%s[/color]\n\n%s[/center]" % [_pause_loc("NO WITNESS RECORDED", "기록된 목격 없음"), _pause_loc("Choose SAVE HERE to preserve the current route.", "현재 경로를 남기려면 여기에 저장을 선택하세요.")]
	else:
		var chapter := int(info.get("chapter", 1))
		details.text = "[color=#d9b76a]%s[/color]\n%s\n\nHP  %d / %d\n%s  %d\n%s  %d\n%s  %s" % [_save_archive_chapter_name(chapter), _save_archive_location_name(info), int(info.get("hp", 0)), int(info.get("max_hp", 100)), _pause_loc("Grains", "그레인"), int(info.get("grains", 0)), _pause_loc("Burned memories", "연소한 기억"), int(info.get("burn_count", 0)), _pause_loc("Recorded", "기록 시각"), String(info.get("timestamp", "")).replace("T", "  ")]
	save_btn.disabled = slot == 0
	save_btn.text = _pause_loc("AUTOSAVE PROTECTED", "자동 저장 보호됨") if slot == 0 else (_pause_loc("OVERWRITE RECORD", "기록 덮어쓰기") if not info.is_empty() else _pause_loc("SAVE HERE", "여기에 저장"))
	load_btn.disabled = info.is_empty()

func _save_archive_write(state: Dictionary, buttons: Array[Button], preview: TextureRect, preview_title: Label, details: RichTextLabel, save_btn: Button, load_btn: Button) -> void:
	var slot := int(state.get("selected", 1))
	if slot == 0:
		return
	if SaveManager.has_save(slot) and int(state.get("confirm_slot", -1)) != slot:
		state["confirm_slot"] = slot
		save_btn.text = _pause_loc("CONFIRM OVERWRITE", "덮어쓰기 확인")
		NotificationToast.show_toast(_pause_loc("Press again to replace this record", "한 번 더 눌러 이 기록을 교체하세요"), NotificationToast.ToastType.WARNING)
		return
	if SaveManager.save_game(slot):
		AudioManager.play_sfx("confirm")
		state["confirm_slot"] = -1
		_refresh_save_archive_buttons(buttons)
		_select_save_archive_slot(slot, state, buttons, preview, preview_title, details, save_btn, load_btn)
		_update_save_info()

func _save_archive_load(state: Dictionary, save_overlay: Control) -> void:
	var slot := int(state.get("selected", 1))
	if not SaveManager.has_save(slot):
		return
	AudioManager.play_sfx("confirm")
	save_overlay.queue_free()
	_close()
	SaveManager.load_game(slot)

func _save_archive_art_path(info: Dictionary) -> String:
	var chapter := int(info.get("chapter", GameManager.current_chapter)) if not info.is_empty() else GameManager.current_chapter
	return String(SAVE_CHAPTER_ART.get(clampi(chapter, 1, 10), "res://assets/cg/generated/ui_loss_record_blank_book_v2.png"))

func _save_archive_chapter_name(chapter: int) -> String:
	var names := {1: "Rim Forest", 2: "Verdan Market", 3: "Belt Waystation", 4: "Drift Shelter", 5: "Crumbling Coast", 6: "The Seam", 7: "Seam Outskirts", 8: "Forgotten Forest", 9: "Colorless Waste", 10: "BL-07 Void", 11: "Epilogue"}
	var names_ko := {1: "림 숲", 2: "베르단 시장", 3: "벨트 중계소", 4: "표류 대피소", 5: "무너지는 해안", 6: "더 심", 7: "심 외곽", 8: "망각의 숲", 9: "무색 황무지", 10: "BL-07 공허", 11: "에필로그"}
	return String(names_ko.get(chapter, "알 수 없는 경로")) if GameManager.current_locale == "ko" else String(names.get(chapter, "Unknown Route"))

func _save_archive_location_name(info: Dictionary) -> String:
	var chapter := int(info.get("chapter", 1))
	var raw := String(info.get("location", _save_archive_chapter_name(chapter)))
	if GameManager.current_locale != "ko":
		return raw
	var localized := {
		"Rim Forest": "림 숲", "Verdan Market": "베르단 시장", "Belt Waystation": "벨트 중계소",
		"Drift Shelter": "표류 대피소", "Crumbling Coast": "무너지는 해안", "The Seam": "더 심",
		"Seam Outskirts": "심 외곽", "Forgotten Forest": "망각의 숲", "Colorless Waste": "무색 황무지",
		"Bl07 Void": "BL-07 공허", "BL-07 Void": "BL-07 공허",
	}
	return String(localized.get(raw, raw))

func _on_field_guide() -> void:
	AudioManager.play_sfx("ui_select")
	_show_field_guide_panel()

func _show_field_guide_panel() -> void:
	if get_node_or_null("FieldGuideOverlay") != null:
		return
	var guide_overlay := ColorRect.new()
	guide_overlay.name = "FieldGuideOverlay"
	guide_overlay.set_anchors_preset(Control.PRESET_FULL_RECT)
	guide_overlay.color = Color(0, 0, 0, 0)
	guide_overlay.mouse_filter = Control.MOUSE_FILTER_STOP
	add_child(guide_overlay)
	_add_modal_backdrop(guide_overlay, FIELD_GUIDE_BACKDROP_PATH, Color(0.004, 0.004, 0.009, 0.06))

	var header := Label.new()
	header.anchor_left = 0.19
	header.anchor_right = 0.81
	header.anchor_top = 0.035
	header.anchor_bottom = 0.12
	header.text = _pause_loc("FIELD GUIDE / HOW TO WITNESS", "필드 가이드 / 목격하는 법")
	header.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	header.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	header.add_theme_font_size_override("font_size", 22)
	header.add_theme_color_override("font_color", Color(0.92, 0.76, 0.44))
	UITheme.apply_title_font(header)
	guide_overlay.add_child(header)

	var left_blocks := [
		{"title": _pause_loc("MOVE & READ THE WORLD", "이동과 세계 읽기"), "body": _pause_loc("WASD / Left Stick  ·  Move\nE / A  ·  Talk, inspect, confirm\nGold diamond on the minimap  ·  Story objective", "WASD / 왼쪽 스틱  ·  이동\nE / A  ·  대화, 조사, 확인\n미니맵의 금빛 마름모  ·  이야기 목표")},
		{"title": _pause_loc("BATTLE RHYTHM", "전투 리듬"), "body": _pause_loc("Choose a field directive, then shape your stance, combo, BREAK, and WITNESS plan around it.\nHigher grades pay more Grains. Consecutive directives build a reward chain; every third success restores Field Focus.\nItems preserve memories; burning one is powerful but permanent.", "현장 전술 지침을 고른 뒤 자세, 연계 공격, BREAK, 기억 읽기를 그 목표에 맞춰 구성하세요.\n높은 등급은 더 많은 그레인을 주고, 지침 연속 달성 3회마다 현장 집중을 되찾습니다.\n아이템은 기억을 지켜 주며, 기억 연소는 강하지만 영구적입니다.")},
		{"title": _pause_loc("QUICK ACCESS", "빠른 접근"), "body": _pause_loc("Tab / M  ·  Memory archive\nQ  ·  Memory Pulse toward nearby echoes\nEsc  ·  Pause  |  F6 / F7  ·  Quick save / load", "Tab / M  ·  기억 서고\nQ  ·  가까운 메아리를 찾는 기억 파동\nEsc  ·  일시정지  |  F6 / F7  ·  빠른 저장 / 불러오기")},
	]
	var right_blocks := [
		{"title": _pause_loc("MEMORY IS BOTH POWER AND WOUND", "기억은 힘이자 상처입니다"), "body": _pause_loc("Burning changes battle, dialogue, and the world. Residue can echo after a burn, but the original memory does not return.", "기억 연소는 전투와 대화, 세계를 바꿉니다. 잔존물이 메아리칠 수는 있어도 원래 기억은 돌아오지 않습니다.")},
		{"title": _pause_loc("FOLLOW WITNESSED ROUTES", "목격된 경로를 따르세요"), "body": _pause_loc("The quest card names the next story thread. The World Map reopens only routes Arrel has already witnessed.", "퀘스트 카드는 다음 이야기 흐름을 알려 줍니다. 월드 맵에서는 아렐이 이미 목격한 경로만 다시 열립니다.")},
		{"title": _pause_loc("KEEP THE VIEW CLEAR", "시야를 선명하게 유지하세요"), "body": _pause_loc("Options can reduce shake, flashes, particles, fog, grain, and decorative overlays without changing difficulty.", "옵션에서 난이도는 유지한 채 화면 흔들림, 번쩍임, 파티클, 안개, 그레인과 장식 오버레이를 줄일 수 있습니다.")},
	]
	for index in range(3):
		_add_field_guide_block(guide_overlay, left_blocks[index], Rect2(0.075, 0.175 + index * 0.222, 0.38, 0.185))
		_add_field_guide_block(guide_overlay, right_blocks[index], Rect2(0.555, 0.175 + index * 0.222, 0.37, 0.185))

	var footer := Label.new()
	footer.anchor_left = 0.20
	footer.anchor_right = 0.80
	footer.anchor_top = 0.89
	footer.anchor_bottom = 0.95
	footer.text = _pause_loc("The safest route is the one someone remembers with you.  ·  [ESC] Close", "가장 안전한 길은 누군가 함께 기억해 주는 길입니다.  ·  [ESC] 닫기")
	footer.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	footer.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	footer.add_theme_font_size_override("font_size", 13)
	footer.add_theme_color_override("font_color", Color(0.68, 0.59, 0.45))
	guide_overlay.add_child(footer)

	var close_btn := Button.new()
	close_btn.anchor_left = 0.925
	close_btn.anchor_right = 0.975
	close_btn.anchor_top = 0.025
	close_btn.anchor_bottom = 0.085
	close_btn.text = "X"
	close_btn.flat = true
	close_btn.add_theme_font_size_override("font_size", 18)
	close_btn.add_theme_color_override("font_color", Color(0.68, 0.58, 0.44))
	close_btn.pressed.connect(func():
		AudioManager.play_sfx("ui_close")
		guide_overlay.queue_free()
	)
	guide_overlay.add_child(close_btn)
	var close_handler = func(event: InputEvent):
		if event.is_action_pressed("cancel") or event.is_action_pressed("menu"):
			guide_overlay.queue_free()
			get_viewport().set_input_as_handled()
	guide_overlay.gui_input.connect(close_handler)

func _add_field_guide_block(host: Control, data: Dictionary, anchors: Rect2) -> void:
	var block := RichTextLabel.new()
	block.anchor_left = anchors.position.x
	block.anchor_top = anchors.position.y
	block.anchor_right = anchors.position.x + anchors.size.x
	block.anchor_bottom = anchors.position.y + anchors.size.y
	block.bbcode_enabled = true
	block.fit_content = false
	block.scroll_active = false
	block.text = "[color=#d9b76a][font_size=17]%s[/font_size][/color]\n[color=#c6b9a3][font_size=13]%s[/font_size][/color]" % [String(data.get("title", "")), String(data.get("body", ""))]
	block.add_theme_color_override("default_color", Color(0.78, 0.73, 0.65))
	host.add_child(block)

func _pause_loc(en: String, ko: String) -> String:
	return ko if GameManager.current_locale == "ko" else en

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

## S209: 회상 기록 열기.
func _on_story_log() -> void:
	AudioManager.play_sfx("ui_select")
	StoryLog.open_log()

func _on_options() -> void:
	AudioManager.play_sfx("ui_select")
	OptionsMenu.open()

func _on_codex() -> void:
	AudioManager.play_sfx("ui_select")
	Codex.open()

func _on_artbook() -> void:
	AudioManager.play_sfx("ui_select")
	_show_artbook_panel()

func _load_artbook_items() -> Array[Dictionary]:
	var combined: Array[Dictionary] = []
	for item in ARTBOOK_ITEMS:
		combined.append(item.duplicate(true))
	for manifest_path: String in [
		CHAPTER_EXPANSION_GALLERY_PATH,
		INTERFACE_EXPANSION_GALLERY_PATH,
		ILLUSTRATION_EXPANSION_GALLERY_PATH,
		ILLUSTRATION_GAPFILL_GALLERY_PATH,
		WORLD_POPULATION_GALLERY_PATH,
		OCCULT_BOSS_GALLERY_PATH,
	]:
		if not FileAccess.file_exists(manifest_path):
			continue
		var parsed: Variant = JSON.parse_string(FileAccess.get_file_as_string(manifest_path))
		if not parsed is Dictionary:
			push_warning("Artbook expansion manifest is not a dictionary: %s" % manifest_path)
			continue
		var expansion_items: Variant = parsed.get("items", [])
		if not expansion_items is Array:
			push_warning("Artbook expansion manifest has no item array: %s" % manifest_path)
			continue
		for item: Variant in expansion_items:
			if item is Dictionary:
				combined.append(item.duplicate(true))
	return combined

func _show_artbook_panel() -> void:
	_active_artbook_items = _load_artbook_items()
	var art_overlay = ColorRect.new()
	art_overlay.set_anchors_preset(Control.PRESET_FULL_RECT)
	art_overlay.color = Color(0.01, 0.01, 0.015, 0.88)
	art_overlay.mouse_filter = Control.MOUSE_FILTER_STOP
	add_child(art_overlay)
	_add_modal_backdrop(art_overlay, "res://assets/cg/generated/ui_story_archive_atlas_v2.png", Color(0.01, 0.008, 0.02, 0.62))

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
	sub.add_theme_font_size_override("font_size", 13)
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
	list_title.text = "FILES · %d" % _active_artbook_items.size()
	list_title.add_theme_font_size_override("font_size", 14)
	list_title.add_theme_color_override("font_color", Color(0.74, 0.65, 0.48))
	left_box.add_child(list_title)

	var search_box := LineEdit.new()
	search_box.placeholder_text = "Search title or category"
	search_box.clear_button_enabled = true
	search_box.add_theme_font_size_override("font_size", 13)
	search_box.add_theme_color_override("font_color", Color(0.84, 0.78, 0.68))
	search_box.add_theme_color_override("font_placeholder_color", Color(0.42, 0.39, 0.36))
	left_box.add_child(search_box)

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
	preview_type.add_theme_font_size_override("font_size", 13)
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

	for i in range(_active_artbook_items.size()):
		var item := _active_artbook_items[i]
		var btn = Button.new()
		btn.text = "%s\n   %s" % [item.get("title", "Untitled"), item.get("type", "Reference")]
		btn.custom_minimum_size = Vector2(0, 48)
		btn.alignment = HORIZONTAL_ALIGNMENT_LEFT
		btn.add_theme_font_size_override("font_size", 13)
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

	search_box.text_changed.connect(func(query: String):
		var needle := query.strip_edges().to_lower()
		for child in list.get_children():
			if child is Button:
				child.visible = needle == "" or String(child.text).to_lower().contains(needle)
	)

	if _active_artbook_items.size() > 0:
		_set_artbook_preview(preview, preview_title, preview_type, preview_desc, _active_artbook_items[0])

	var close_label = Label.new()
	close_label.text = "[ESC] Close"
	close_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	close_label.add_theme_font_size_override("font_size", 13)
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
	if index >= 0 and index < _active_artbook_items.size():
		_set_artbook_preview(preview, title, type_label, desc, _active_artbook_items[index])

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
	progress_label.add_theme_font_size_override("font_size", 13)
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
		desc_lbl.add_theme_font_size_override("font_size", 13)
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
	close_label.add_theme_font_size_override("font_size", 13)
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

func _show_travel_panel_legacy() -> void:
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
	desc.add_theme_font_size_override("font_size", 13)
	desc.add_theme_color_override("font_color", Color(0.5, 0.5, 0.45))
	vbox.add_child(desc)

	# 맵 목록, 챕터에 따라 해금
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
			btn.text = "Ch%d, %s\n    %s" % [map_data["chapter"], map_data["name"], map_data["desc"]]
			btn_style.bg_color = Color(0.08, 0.1, 0.06, 0.9)
			btn_style.border_color = Color(0.35, 0.45, 0.25, 0.5)
			btn_style.set_border_width_all(1)
			btn.add_theme_color_override("font_color", Color(0.65, 0.75, 0.5))
			btn.add_theme_color_override("font_hover_color", Color(0.85, 0.95, 0.6))
		else:
			btn.text = "Ch%d, ???" % map_data["chapter"]
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
	close_label.add_theme_font_size_override("font_size", 13)
	close_label.add_theme_color_override("font_color", Color(0.4, 0.35, 0.3))
	close_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	vbox.add_child(close_label)

	var close_handler = func(event: InputEvent):
		if event.is_action_pressed("cancel") or event.is_action_pressed("menu"):
			travel_overlay.queue_free()
			get_viewport().set_input_as_handled()
	travel_overlay.gui_input.connect(close_handler)

func _show_travel_panel() -> void:
	if get_node_or_null("WorldMapOverlay") != null:
		return
	var travel_overlay := ColorRect.new()
	travel_overlay.name = "WorldMapOverlay"
	travel_overlay.set_anchors_preset(Control.PRESET_FULL_RECT)
	travel_overlay.color = Color(0, 0, 0, 0)
	travel_overlay.mouse_filter = Control.MOUSE_FILTER_STOP
	add_child(travel_overlay)
	_add_modal_backdrop(travel_overlay, WORLD_MAP_BACKDROP_PATH, Color(0.004, 0.004, 0.01, 0.14))
	var depth_stage := HybridDepthStage.create_stage("world_map", HybridDepthStage.StageMode.ATLAS)
	depth_stage.name = "RouteDepthDiorama"
	depth_stage.anchor_left = 0.055
	depth_stage.anchor_right = 0.745
	depth_stage.anchor_top = 0.14
	depth_stage.anchor_bottom = 0.855
	depth_stage.modulate = Color(0.92, 0.88, 0.82, 0.74)
	travel_overlay.add_child(depth_stage)

	var travel_panel := PanelContainer.new()
	travel_panel.name = "WorldMapPanel"
	travel_panel.anchor_left = 0.025
	travel_panel.anchor_right = 0.975
	travel_panel.anchor_top = 0.025
	travel_panel.anchor_bottom = 0.975
	var panel_style := StyleBoxFlat.new()
	panel_style.bg_color = Color(0.008, 0.007, 0.014, 0.12)
	panel_style.border_color = Color(0.62, 0.47, 0.25, 0.64)
	panel_style.set_border_width_all(1)
	panel_style.set_corner_radius_all(4)
	panel_style.set_content_margin_all(8)
	travel_panel.add_theme_stylebox_override("panel", panel_style)
	travel_overlay.add_child(travel_panel)

	var content := Control.new()
	travel_panel.add_child(content)
	var current_chapter := clampi(GameManager.current_chapter, 1, 10)
	var witnessed_count := mini(GameManager.current_chapter, TRAVEL_DESTINATIONS.size())

	var header := Label.new()
	header.anchor_left = 0.045
	header.anchor_right = 0.77
	header.anchor_top = 0.025
	header.anchor_bottom = 0.085
	header.text = "WITNESSED ROUTES / WORLD MAP"
	header.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	header.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	header.add_theme_font_size_override("font_size", 22)
	header.add_theme_color_override("font_color", Color(0.92, 0.76, 0.44))
	UITheme.apply_title_font(header)
	content.add_child(header)

	var subtitle := Label.new()
	subtitle.anchor_left = 0.07
	subtitle.anchor_right = 0.75
	subtitle.anchor_top = 0.085
	subtitle.anchor_bottom = 0.13
	subtitle.text = "Only places held by witness can be crossed again."
	subtitle.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	subtitle.add_theme_font_size_override("font_size", 13)
	subtitle.add_theme_color_override("font_color", Color(0.58, 0.53, 0.46))
	content.add_child(subtitle)

	var depth_caption := Label.new()
	depth_caption.name = "DepthCaption"
	depth_caption.anchor_left = 0.07
	depth_caption.anchor_right = 0.74
	depth_caption.anchor_top = 0.14
	depth_caption.anchor_bottom = 0.18
	depth_caption.text = _pause_loc("LIVE MEMORY RELIEF  /  SELECT A WITNESSED LANDMARK", "기억 입체도  /  목격한 지점을 선택하세요")
	depth_caption.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	depth_caption.add_theme_font_size_override("font_size", 13)
	depth_caption.add_theme_color_override("font_color", Color(0.68, 0.58, 0.42, 0.88))
	depth_caption.mouse_filter = Control.MOUSE_FILTER_IGNORE
	content.add_child(depth_caption)

	var map_note := Label.new()
	map_note.anchor_left = 0.055
	map_note.anchor_right = 0.72
	map_note.anchor_top = 0.865
	map_note.anchor_bottom = 0.94
	map_note.text = "CURRENT WITNESS  /  CHAPTER %02d  /  %d OF %d ROUTES RECORDED" % [current_chapter, witnessed_count, TRAVEL_DESTINATIONS.size()]
	map_note.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	map_note.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	map_note.add_theme_font_size_override("font_size", 13)
	map_note.add_theme_color_override("font_color", Color(0.72, 0.61, 0.43))
	content.add_child(map_note)

	var route_panel := PanelContainer.new()
	route_panel.anchor_left = 0.795
	route_panel.anchor_right = 0.975
	route_panel.anchor_top = 0.075
	route_panel.anchor_bottom = 0.925
	var route_style := StyleBoxFlat.new()
	route_style.bg_color = Color(0.01, 0.009, 0.016, 0.52)
	route_style.border_color = Color(0.48, 0.36, 0.2, 0.48)
	route_style.set_border_width_all(1)
	route_style.set_corner_radius_all(3)
	route_style.set_content_margin_all(8)
	route_panel.add_theme_stylebox_override("panel", route_style)
	content.add_child(route_panel)

	var route_root := VBoxContainer.new()
	route_root.add_theme_constant_override("separation", 7)
	route_panel.add_child(route_root)
	var route_title := Label.new()
	route_title.text = "ROUTE INDEX"
	route_title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	route_title.add_theme_font_size_override("font_size", 14)
	route_title.add_theme_color_override("font_color", Color(0.82, 0.68, 0.43))
	route_root.add_child(route_title)

	var route_scroll := ScrollContainer.new()
	route_scroll.size_flags_vertical = Control.SIZE_EXPAND_FILL
	route_scroll.horizontal_scroll_mode = ScrollContainer.SCROLL_MODE_DISABLED
	route_root.add_child(route_scroll)
	var route_list := VBoxContainer.new()
	route_list.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	route_list.add_theme_constant_override("separation", 5)
	route_scroll.add_child(route_list)

	for map_data in TRAVEL_DESTINATIONS:
		var btn := Button.new()
		var chapter := int(map_data.get("chapter", 1))
		var unlocked := GameManager.current_chapter >= chapter
		btn.custom_minimum_size = Vector2(0, 54)
		btn.alignment = HORIZONTAL_ALIGNMENT_LEFT
		btn.tooltip_text = String(map_data.get("desc", ""))
		btn.clip_text = true
		var btn_style := StyleBoxFlat.new()
		btn_style.set_content_margin_all(7)
		btn_style.set_corner_radius_all(3)
		btn_style.set_border_width_all(1)
		if unlocked:
			var here := "  ·  HERE" if chapter == current_chapter else ""
			btn.text = "%02d  %s%s\n%s" % [chapter, map_data.get("name", "Unknown"), here, map_data.get("desc", "")]
			btn_style.bg_color = Color(0.035, 0.031, 0.045, 0.64)
			btn_style.border_color = Color(0.38, 0.29, 0.17, 0.5)
			btn.add_theme_color_override("font_color", Color(0.76, 0.7, 0.59))
			btn.add_theme_color_override("font_hover_color", Color(1.0, 0.84, 0.49))
		else:
			btn.text = "%02d  UNWITNESSED ROUTE" % chapter
			btn_style.bg_color = Color(0.018, 0.017, 0.022, 0.5)
			btn_style.border_color = Color(0.18, 0.16, 0.15, 0.35)
			btn.add_theme_color_override("font_color", Color(0.31, 0.3, 0.29))
			btn.disabled = true
		btn.add_theme_stylebox_override("normal", btn_style)
		var hover_style := btn_style.duplicate()
		hover_style.bg_color = Color(0.12, 0.09, 0.06, 0.88)
		hover_style.border_color = Color(0.78, 0.57, 0.28, 0.85)
		btn.add_theme_stylebox_override("hover", hover_style)
		btn.add_theme_stylebox_override("focus", hover_style)
		btn.add_theme_font_size_override("font_size", 13)
		if unlocked:
			var scene_path := String(map_data.get("scene", ""))
			var route_chapter := chapter
			btn.pressed.connect(func():
				AudioManager.play_sfx("confirm")
				travel_overlay.queue_free()
				_close()
				SceneTransition.change_scene_styled(scene_path)
			)
			btn.focus_entered.connect(func():
				AudioManager.play_sfx("ui_hover")
				depth_stage.focus_route(route_chapter)
			)
			btn.mouse_entered.connect(func(): depth_stage.focus_route(route_chapter))
		route_list.add_child(btn)

	var close_label := Label.new()
	close_label.text = "[ESC] Close"
	close_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	close_label.add_theme_font_size_override("font_size", 13)
	close_label.add_theme_color_override("font_color", Color(0.45, 0.4, 0.35))
	route_root.add_child(close_label)

	var close_handler = func(event: InputEvent):
		if event.is_action_pressed("cancel") or event.is_action_pressed("menu"):
			travel_overlay.queue_free()
			get_viewport().set_input_as_handled()
	travel_overlay.gui_input.connect(close_handler)
	_animate_modal_panel(travel_panel)

func _on_inventory() -> void:
	AudioManager.play_sfx("ui_select")
	_show_inventory_panel()

func _show_inventory_panel() -> void:
	if get_node_or_null("InventoryOverlay") != null:
		return
	var inventory_overlay := ColorRect.new()
	inventory_overlay.name = "InventoryOverlay"
	inventory_overlay.set_anchors_preset(Control.PRESET_FULL_RECT)
	inventory_overlay.color = Color(0, 0, 0, 0)
	inventory_overlay.mouse_filter = Control.MOUSE_FILTER_STOP
	add_child(inventory_overlay)
	inventory_overlay.set_meta("inventory_state", {"category": "all", "query": "", "sort": 0})
	inventory_overlay.set_meta("inventory_selected_id", "")
	_add_modal_backdrop(inventory_overlay, INVENTORY_BACKDROP_PATH, Color(0.006, 0.005, 0.012, 0.22))

	var inventory_panel := PanelContainer.new()
	inventory_panel.name = "InventoryPanel"
	inventory_panel.anchor_left = 0.035
	inventory_panel.anchor_right = 0.965
	inventory_panel.anchor_top = 0.04
	inventory_panel.anchor_bottom = 0.96
	var panel_style := StyleBoxFlat.new()
	panel_style.bg_color = Color(0.012, 0.01, 0.018, 0.18)
	panel_style.border_color = Color(0.62, 0.47, 0.25, 0.66)
	panel_style.set_border_width_all(1)
	panel_style.set_corner_radius_all(5)
	panel_style.set_content_margin_all(8)
	inventory_panel.add_theme_stylebox_override("panel", panel_style)
	inventory_overlay.add_child(inventory_panel)

	var content := Control.new()
	inventory_panel.add_child(content)

	var header := Label.new()
	header.anchor_left = 0.39
	header.anchor_right = 0.95
	header.anchor_top = 0.012
	header.anchor_bottom = 0.058
	header.text = "FIELD ARCHIVE / CARRIED WITNESSES"
	header.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	header.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	header.add_theme_font_size_override("font_size", 21)
	header.add_theme_color_override("font_color", Color(0.92, 0.76, 0.45))
	UITheme.apply_title_font(header)
	content.add_child(header)

	var item_ids: Array[String] = []
	var total_units := 0
	for item_id_value in GameManager.ITEMS.keys():
		var item_id := String(item_id_value)
		var count := GameManager.get_item_count(item_id)
		if count > 0:
			item_ids.append(item_id)
			total_units += count
	item_ids.sort_custom(_inventory_item_less)

	var intact_memories := 0
	for memory in MemoryManager.memories:
		if not memory.is_burned and not memory.is_faded:
			intact_memories += 1
	var status_strip := Label.new()
	status_strip.name = "InventoryStatusStrip"
	status_strip.anchor_left = 0.39
	status_strip.anchor_right = 0.95
	status_strip.anchor_top = 0.057
	status_strip.anchor_bottom = 0.093
	status_strip.text = "HP %d/%d  |  GRAINS %d  |  MEMORIES %d INTACT / %d BURNED  |  CHAPTER %02d" % [
		int(GameManager.player_data.get("hp", 0)),
		int(GameManager.player_data.get("max_hp", 0)),
		int(GameManager.player_data.get("grains", 0)),
		intact_memories,
		MemoryManager.get_burn_count(),
		GameManager.current_chapter,
	]
	status_strip.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	status_strip.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	status_strip.add_theme_font_size_override("font_size", 13)
	status_strip.add_theme_color_override("font_color", UITheme.TEXT_NARRATION)
	content.add_child(status_strip)

	var list_panel := PanelContainer.new()
	list_panel.anchor_left = 0.045
	list_panel.anchor_right = 0.35
	list_panel.anchor_top = 0.095
	list_panel.anchor_bottom = 0.895
	var list_style := StyleBoxFlat.new()
	list_style.bg_color = Color(0.018, 0.016, 0.024, 0.90)
	list_style.border_color = Color(0.58, 0.44, 0.25, 0.72)
	list_style.set_border_width_all(1)
	list_style.set_corner_radius_all(3)
	list_style.set_content_margin_all(10)
	list_panel.add_theme_stylebox_override("panel", list_style)
	content.add_child(list_panel)

	var list_root := VBoxContainer.new()
	list_root.add_theme_constant_override("separation", 8)
	list_panel.add_child(list_root)

	var list_title := Label.new()
	list_title.name = "InventoryListTitle"
	list_title.text = "CARRIED  |  %d TYPES  |  %d TOTAL" % [item_ids.size(), total_units]
	list_title.add_theme_font_size_override("font_size", 13)
	list_title.add_theme_color_override("font_color", Color(0.91, 0.75, 0.46))
	list_root.add_child(list_title)

	var quick_root := VBoxContainer.new()
	quick_root.name = "InventoryQuickKit"
	quick_root.add_theme_constant_override("separation", 3)
	list_root.add_child(quick_root)
	var quick_label := Label.new()
	quick_label.text = "QUICK KIT  /  BATTLE KEYS 1-3"
	quick_label.add_theme_font_size_override("font_size", 13)
	quick_label.add_theme_color_override("font_color", UITheme.TEXT_DIM)
	quick_root.add_child(quick_label)
	var quick_row := HBoxContainer.new()
	quick_row.add_theme_constant_override("separation", 5)
	quick_root.add_child(quick_row)
	for slot_index in range(GameManager.ITEM_QUICK_SLOT_COUNT):
		var quick_button := Button.new()
		quick_button.name = "InventoryQuickSlot_%d" % (slot_index + 1)
		quick_button.text = "%d  EMPTY" % (slot_index + 1)
		quick_button.custom_minimum_size = Vector2(0, 38)
		quick_button.size_flags_horizontal = Control.SIZE_EXPAND_FILL
		quick_button.add_theme_font_size_override("font_size", 13)
		quick_button.pressed.connect(_on_inventory_quick_pressed.bind(quick_button, inventory_overlay))
		quick_row.add_child(quick_button)

	var search_row := HBoxContainer.new()
	search_row.name = "InventorySearchTools"
	search_row.add_theme_constant_override("separation", 5)
	list_root.add_child(search_row)
	var search := LineEdit.new()
	search.name = "InventorySearch"
	search.placeholder_text = "Search supply..."
	search.clear_button_enabled = true
	search.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	search.custom_minimum_size = Vector2(0, 32)
	search.add_theme_font_size_override("font_size", 13)
	search.text_changed.connect(_on_inventory_search_changed.bind(inventory_overlay))
	search_row.add_child(search)
	var sort_menu := OptionButton.new()
	sort_menu.name = "InventorySort"
	sort_menu.custom_minimum_size = Vector2(92, 32)
	sort_menu.add_item("TYPE")
	sort_menu.add_item("NAME")
	sort_menu.add_item("COUNT")
	sort_menu.item_selected.connect(_on_inventory_sort_changed.bind(inventory_overlay))
	search_row.add_child(sort_menu)

	var item_scroll := ScrollContainer.new()
	item_scroll.size_flags_vertical = Control.SIZE_EXPAND_FILL
	item_scroll.horizontal_scroll_mode = ScrollContainer.SCROLL_MODE_DISABLED
	list_root.add_child(item_scroll)

	var item_list := VBoxContainer.new()
	item_list.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	item_list.add_theme_constant_override("separation", 5)
	item_scroll.add_child(item_list)
	var item_buttons: Array[Button] = []

	var detail_panel := PanelContainer.new()
	detail_panel.anchor_left = 0.385
	detail_panel.anchor_right = 0.95
	detail_panel.anchor_top = 0.095
	detail_panel.anchor_bottom = 0.64
	var detail_style := StyleBoxFlat.new()
	detail_style.bg_color = Color(0.014, 0.013, 0.02, 0.90)
	detail_style.border_color = Color(0.56, 0.43, 0.24, 0.72)
	detail_style.set_border_width_all(1)
	detail_style.set_corner_radius_all(4)
	detail_style.set_content_margin_all(18)
	detail_panel.add_theme_stylebox_override("panel", detail_style)
	content.add_child(detail_panel)

	var detail_root := HBoxContainer.new()
	detail_root.add_theme_constant_override("separation", 20)
	detail_panel.add_child(detail_root)

	var preview := TextureRect.new()
	preview.name = "InventoryPreview"
	preview.custom_minimum_size = Vector2(148, 148)
	preview.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	preview.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
	preview.mouse_filter = Control.MOUSE_FILTER_IGNORE
	detail_root.add_child(preview)

	var copy := VBoxContainer.new()
	copy.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	copy.add_theme_constant_override("separation", 8)
	detail_root.add_child(copy)

	var detail_title := Label.new()
	detail_title.name = "InventoryDetailTitle"
	detail_title.text = "No carried supply"
	detail_title.add_theme_font_size_override("font_size", 22)
	detail_title.add_theme_color_override("font_color", Color(0.94, 0.79, 0.49))
	copy.add_child(detail_title)

	var detail_type := Label.new()
	detail_type.name = "InventoryDetailType"
	detail_type.text = "FIELD RECORD"
	detail_type.add_theme_font_size_override("font_size", 13)
	detail_type.add_theme_color_override("font_color", UITheme.TEXT_DIM)
	copy.add_child(detail_type)

	var detail_body := RichTextLabel.new()
	detail_body.name = "InventoryDetailBody"
	detail_body.bbcode_enabled = true
	detail_body.size_flags_vertical = Control.SIZE_EXPAND_FILL
	detail_body.scroll_active = false
	detail_body.text = "Nothing is currently recorded in the field satchel."
	detail_body.add_theme_font_size_override("normal_font_size", 16)
	detail_body.add_theme_color_override("default_color", UITheme.TEXT_PRIMARY)
	copy.add_child(detail_body)

	var detail_meta := Label.new()
	detail_meta.name = "InventoryDetailMeta"
	detail_meta.add_theme_font_size_override("font_size", 13)
	detail_meta.add_theme_color_override("font_color", UITheme.TEXT_NARRATION)
	copy.add_child(detail_meta)

	var action_row := HBoxContainer.new()
	action_row.name = "InventoryActions"
	action_row.add_theme_constant_override("separation", 8)
	copy.add_child(action_row)
	var use_button := Button.new()
	use_button.name = "InventoryUseNow"
	use_button.text = "USE NOW"
	use_button.custom_minimum_size = Vector2(145, 38)
	use_button.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	use_button.pressed.connect(_on_inventory_use_now.bind(inventory_overlay))
	action_row.add_child(use_button)
	var pin_button := Button.new()
	pin_button.name = "InventoryPinQuick"
	pin_button.text = "PIN TO QUICK KIT"
	pin_button.custom_minimum_size = Vector2(165, 38)
	pin_button.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	pin_button.pressed.connect(_on_inventory_pin_quick.bind(inventory_overlay))
	action_row.add_child(pin_button)

	for item_id in item_ids:
		var item_data: Dictionary = GameManager.ITEMS[item_id]
		var count := GameManager.get_item_count(item_id)
		var btn := Button.new()
		btn.name = "InventoryItem_%s" % item_id
		btn.text = _inventory_item_button_text(item_id)
		btn.tooltip_text = String(item_data.get("desc", ""))
		btn.custom_minimum_size = Vector2(0, 50)
		btn.alignment = HORIZONTAL_ALIGNMENT_LEFT
		btn.icon = GameManager.get_item_icon(item_id)
		btn.icon_alignment = HORIZONTAL_ALIGNMENT_LEFT
		btn.expand_icon = true
		btn.add_theme_font_size_override("font_size", 13)
		btn.add_theme_color_override("font_color", UITheme.TEXT_PRIMARY)
		btn.add_theme_color_override("font_hover_color", Color(1.0, 0.86, 0.52))
		var btn_style := StyleBoxFlat.new()
		btn_style.bg_color = Color(0.035, 0.031, 0.045, 0.90)
		btn_style.border_color = Color(0.46, 0.35, 0.20, 0.68)
		btn_style.set_border_width_all(1)
		btn_style.set_corner_radius_all(3)
		btn_style.set_content_margin_all(7)
		btn.add_theme_stylebox_override("normal", btn_style)
		var hover_style := btn_style.duplicate()
		hover_style.bg_color = Color(0.12, 0.09, 0.065, 0.86)
		hover_style.border_color = Color(0.78, 0.57, 0.28, 0.85)
		btn.add_theme_stylebox_override("hover", hover_style)
		btn.add_theme_stylebox_override("focus", hover_style)
		btn.pressed.connect(_on_inventory_item_pressed.bind(item_id, inventory_overlay))
		btn.focus_entered.connect(func(): AudioManager.play_sfx("ui_hover"))
		btn.set_meta("inventory_id", item_id)
		btn.set_meta("inventory_category", _inventory_item_category(item_id))
		btn.set_meta("inventory_name", String(item_data.get("name", item_id)).to_lower())
		btn.set_meta("inventory_search", (String(item_data.get("name", item_id)) + " " + String(item_data.get("desc", "")) + " " + _inventory_effect_text(item_data)).to_lower())
		item_list.add_child(btn)
		item_buttons.append(btn)

	var filter_empty := Label.new()
	filter_empty.name = "InventoryFilterEmpty"
	filter_empty.text = "No carried supply matches this view."
	filter_empty.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	filter_empty.add_theme_font_size_override("font_size", 13)
	filter_empty.add_theme_color_override("font_color", UITheme.TEXT_DIM)
	filter_empty.visible = false
	item_list.add_child(filter_empty)

	if item_ids.is_empty():
		var empty_label := Label.new()
		empty_label.text = "No supplies carried.\nField rewards and purchases appear here."
		empty_label.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
		empty_label.add_theme_font_size_override("font_size", 13)
		empty_label.add_theme_color_override("font_color", UITheme.TEXT_DIM)
		item_list.add_child(empty_label)
	else:
		inventory_overlay.set_meta("inventory_selected_id", item_ids[0])
		_set_inventory_detail(item_ids[0], preview, detail_title, detail_type, detail_body, detail_meta)

	var filter_row := HBoxContainer.new()
	filter_row.name = "InventoryFilters"
	filter_row.anchor_left = 0.045
	filter_row.anchor_right = 0.35
	filter_row.anchor_top = 0.025
	filter_row.anchor_bottom = 0.086
	filter_row.add_theme_constant_override("separation", 5)
	content.add_child(filter_row)
	var filter_group := ButtonGroup.new()
	for filter_data in [
		{"id": "all", "label": "ALL"},
		{"id": "recovery", "label": "RECOVER"},
		{"id": "tactical", "label": "TACTIC"},
		{"id": "witness", "label": "WITNESS"},
	]:
		var filter_button := Button.new()
		filter_button.name = "InventoryFilter_%s" % String(filter_data.id).to_upper()
		filter_button.text = String(filter_data.label)
		filter_button.toggle_mode = true
		filter_button.button_group = filter_group
		filter_button.size_flags_horizontal = Control.SIZE_EXPAND_FILL
		filter_button.add_theme_font_size_override("font_size", 13)
		filter_button.add_theme_color_override("font_color", UITheme.TEXT_NARRATION)
		filter_button.add_theme_color_override("font_pressed_color", Color(0.98, 0.81, 0.49))
		filter_button.pressed.connect(_set_inventory_category.bind(inventory_overlay, String(filter_data.id)))
		filter_row.add_child(filter_button)
		if String(filter_data.id) == "all":
			filter_button.button_pressed = true

	var equipment_panel := PanelContainer.new()
	equipment_panel.anchor_left = 0.385
	equipment_panel.anchor_right = 0.95
	equipment_panel.anchor_top = 0.665
	equipment_panel.anchor_bottom = 0.895
	var equipment_style := StyleBoxFlat.new()
	equipment_style.bg_color = Color(0.016, 0.014, 0.022, 0.90)
	equipment_style.border_color = Color(0.56, 0.43, 0.24, 0.72)
	equipment_style.set_border_width_all(1)
	equipment_style.set_corner_radius_all(4)
	equipment_style.set_content_margin_all(12)
	equipment_panel.add_theme_stylebox_override("panel", equipment_style)
	content.add_child(equipment_panel)

	var equipment_root := VBoxContainer.new()
	equipment_root.add_theme_constant_override("separation", 8)
	equipment_panel.add_child(equipment_root)
	var equipment_title := Label.new()
	equipment_title.text = "EQUIPPED RECORD  |  ATK +%d  |  DEF +%d" % [GameManager.get_equip_bonus("atk"), GameManager.get_equip_bonus("def")]
	equipment_title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	equipment_title.add_theme_font_size_override("font_size", 13)
	equipment_title.add_theme_color_override("font_color", Color(0.91, 0.75, 0.46))
	equipment_root.add_child(equipment_title)

	var slots := HBoxContainer.new()
	slots.add_theme_constant_override("separation", 10)
	equipment_root.add_child(slots)
	for slot_name in ["weapon", "armor", "accessory"]:
		var slot_card := PanelContainer.new()
		slot_card.size_flags_horizontal = Control.SIZE_EXPAND_FILL
		var slot_style := StyleBoxFlat.new()
		slot_style.bg_color = Color(0.025, 0.022, 0.032, 0.92)
		slot_style.border_color = Color(0.46, 0.35, 0.20, 0.68)
		slot_style.set_border_width_all(1)
		slot_style.set_corner_radius_all(3)
		slot_style.set_content_margin_all(8)
		slot_card.add_theme_stylebox_override("panel", slot_style)
		slots.add_child(slot_card)

		var slot_row := HBoxContainer.new()
		slot_row.add_theme_constant_override("separation", 8)
		slot_card.add_child(slot_row)
		var slot_icon := TextureRect.new()
		slot_icon.name = "InventorySlotIcon_%s" % String(slot_name).capitalize()
		slot_icon.custom_minimum_size = Vector2(46, 46)
		slot_icon.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
		slot_icon.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
		slot_icon.mouse_filter = Control.MOUSE_FILTER_IGNORE
		var slot_icon_path := String(INVENTORY_SLOT_ICON_PATHS.get(slot_name, ""))
		if slot_icon_path != "" and ResourceLoader.exists(slot_icon_path):
			slot_icon.texture = load(slot_icon_path)
		slot_row.add_child(slot_icon)

		var slot_copy := VBoxContainer.new()
		slot_copy.size_flags_horizontal = Control.SIZE_EXPAND_FILL
		slot_row.add_child(slot_copy)
		var slot_label := Label.new()
		slot_label.text = String(slot_name).to_upper()
		slot_label.add_theme_font_size_override("font_size", 13)
		slot_label.add_theme_color_override("font_color", UITheme.TEXT_DIM)
		slot_copy.add_child(slot_label)
		var equipped_id := String(GameManager.equipped.get(slot_name, ""))
		var equipped_label := Label.new()
		if equipped_id != "" and GameManager.EQUIPMENT.has(equipped_id):
			var equip_data: Dictionary = GameManager.EQUIPMENT[equipped_id]
			var upgrade_level := GameManager.get_upgrade_level(equipped_id)
			equipped_label.text = "%s%s\nATK +%d  DEF +%d" % [
				equip_data.get("name", equipped_id),
				" +%d" % upgrade_level if upgrade_level > 0 else "",
				GameManager.get_upgraded_bonus(equipped_id, "atk"),
				GameManager.get_upgraded_bonus(equipped_id, "def"),
			]
		else:
			equipped_label.text = "Unbound\nNo recorded bonus"
		equipped_label.add_theme_font_size_override("font_size", 13)
		equipped_label.add_theme_color_override("font_color", UITheme.TEXT_PRIMARY)
		slot_copy.add_child(equipped_label)

	var footer := Label.new()
	footer.anchor_left = 0.39
	footer.anchor_right = 0.95
	footer.anchor_top = 0.91
	footer.anchor_bottom = 0.97
	footer.text = "[H] Smart Heal  |  Quick Kit = battle keys 1-3  |  [ESC] Close"
	footer.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	footer.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	footer.add_theme_font_size_override("font_size", 13)
	footer.add_theme_color_override("font_color", UITheme.TEXT_DIM)
	content.add_child(footer)

	var close_handler = func(event: InputEvent):
		if event.is_action_pressed("cancel") or event.is_action_pressed("menu"):
			inventory_overlay.queue_free()
			get_viewport().set_input_as_handled()
	inventory_overlay.gui_input.connect(close_handler)
	_refresh_inventory_overlay(inventory_overlay)
	_animate_modal_panel(inventory_panel)

func _inventory_item_category(item_id: String) -> String:
	var item_data: Dictionary = GameManager.ITEMS.get(item_id, {})
	var item_type := String(item_data.get("type", ""))
	match item_type:
		"heal", "cure":
			return "recovery"
		"witness":
			return "witness"
	return "tactical"

func _inventory_item_less(item_a: String, item_b: String) -> bool:
	var order := {"heal": 0, "cure": 1, "witness": 2, "guard": 3, "scan": 4, "burn": 5, "flee": 6}
	var data_a: Dictionary = GameManager.ITEMS.get(item_a, {})
	var data_b: Dictionary = GameManager.ITEMS.get(item_b, {})
	var rank_a := int(order.get(String(data_a.get("type", "")), 99))
	var rank_b := int(order.get(String(data_b.get("type", "")), 99))
	if rank_a != rank_b:
		return rank_a < rank_b
	return String(data_a.get("name", item_a)).naturalnocasecmp_to(String(data_b.get("name", item_b))) < 0

func _inventory_buttons(overlay: Control) -> Array[Button]:
	var result: Array[Button] = []
	for node in overlay.find_children("InventoryItem_*", "Button", true, false):
		if node is Button:
			result.append(node as Button)
	return result

func _inventory_item_button_text(item_id: String) -> String:
	var item_data: Dictionary = GameManager.ITEMS.get(item_id, {})
	var tags: Array[String] = []
	var quick_slot := GameManager.get_item_quick_slots().find(item_id)
	if quick_slot >= 0:
		tags.append("Q%d" % (quick_slot + 1))
	if item_id in GameManager.get_recent_items():
		tags.append("NEW")
	var prefix := "[%s] " % " · ".join(tags) if not tags.is_empty() else ""
	return "%s%s\n  x%d" % [prefix, String(item_data.get("name", item_id)), GameManager.get_item_count(item_id)]

func _set_inventory_category(overlay: Control, category: String) -> void:
	var state: Dictionary = overlay.get_meta("inventory_state", {"category": "all", "query": "", "sort": 0})
	state["category"] = category
	overlay.set_meta("inventory_state", state)
	_apply_inventory_view_overlay(overlay)
	AudioManager.play_sfx("ui_hover")

func _on_inventory_search_changed(query: String, overlay: Control) -> void:
	var state: Dictionary = overlay.get_meta("inventory_state", {"category": "all", "query": "", "sort": 0})
	state["query"] = query.strip_edges().to_lower()
	overlay.set_meta("inventory_state", state)
	_apply_inventory_view_overlay(overlay)

func _on_inventory_sort_changed(index: int, overlay: Control) -> void:
	var state: Dictionary = overlay.get_meta("inventory_state", {"category": "all", "query": "", "sort": 0})
	state["sort"] = index
	overlay.set_meta("inventory_state", state)
	_apply_inventory_view_overlay(overlay)
	AudioManager.play_sfx("ui_hover")

func _apply_inventory_view_overlay(overlay: Control) -> void:
	if not is_instance_valid(overlay):
		return
	var state: Dictionary = overlay.get_meta("inventory_state", {"category": "all", "query": "", "sort": 0})
	var category := String(state.get("category", "all"))
	var query := String(state.get("query", ""))
	var sort_mode := int(state.get("sort", 0))
	var buttons := _inventory_buttons(overlay)
	buttons.sort_custom(func(a: Button, b: Button) -> bool:
		var item_a := String(a.get_meta("inventory_id", ""))
		var item_b := String(b.get_meta("inventory_id", ""))
		if sort_mode == 1:
			return String(a.get_meta("inventory_name", item_a)).naturalnocasecmp_to(String(b.get_meta("inventory_name", item_b))) < 0
		if sort_mode == 2:
			var count_a := GameManager.get_item_count(item_a)
			var count_b := GameManager.get_item_count(item_b)
			if count_a != count_b:
				return count_a > count_b
			return String(a.get_meta("inventory_name", item_a)).naturalnocasecmp_to(String(b.get_meta("inventory_name", item_b))) < 0
		return _inventory_item_less(item_a, item_b)
	)
	if not buttons.is_empty():
		var list_parent := buttons[0].get_parent()
		for index in range(buttons.size()):
			list_parent.move_child(buttons[index], index)
	var visible_count := 0
	for item_button in buttons:
		var item_id := String(item_button.get_meta("inventory_id", ""))
		var owned := GameManager.get_item_count(item_id) > 0
		var category_match := category == "all" or String(item_button.get_meta("inventory_category", "")) == category
		var query_match := query == "" or String(item_button.get_meta("inventory_search", "")).contains(query)
		item_button.visible = owned and category_match and query_match
		if item_button.visible:
			visible_count += 1
	var empty_label := overlay.find_child("InventoryFilterEmpty", true, false) as Label
	if empty_label:
		empty_label.visible = visible_count == 0

func _on_inventory_quick_pressed(button: Button, overlay: Control) -> void:
	var item_id := String(button.get_meta("inventory_id", ""))
	if item_id == "" or GameManager.get_item_count(item_id) <= 0:
		AudioManager.play_sfx("cancel")
		return
	_on_inventory_item_pressed(item_id, overlay)

func _on_inventory_use_now(overlay: Control) -> void:
	var item_id := String(overlay.get_meta("inventory_selected_id", ""))
	var result := GameManager.use_field_item(item_id)
	if not bool(result.get("success", false)):
		AudioManager.play_sfx("cancel")
		return
	AudioManager.play_sfx("ui_select")
	_refresh_inventory_overlay(overlay)

func _on_inventory_pin_quick(overlay: Control) -> void:
	var item_id := String(overlay.get_meta("inventory_selected_id", ""))
	if item_id == "":
		AudioManager.play_sfx("cancel")
		return
	GameManager.toggle_item_quick_slot(item_id)
	AudioManager.play_sfx("ui_select")
	_refresh_inventory_overlay(overlay)

func _refresh_inventory_overlay(overlay: Control) -> void:
	if not is_instance_valid(overlay):
		return
	var buttons := _inventory_buttons(overlay)
	var total_units := 0
	var type_count := 0
	for item_button in buttons:
		var item_id := String(item_button.get_meta("inventory_id", ""))
		var count := GameManager.get_item_count(item_id)
		item_button.text = _inventory_item_button_text(item_id)
		item_button.disabled = count <= 0
		if count > 0:
			type_count += 1
			total_units += count
	var list_title := overlay.find_child("InventoryListTitle", true, false) as Label
	if list_title:
		list_title.text = "CARRIED  |  %d TYPES  |  %d TOTAL" % [type_count, total_units]
	var quick_slots := GameManager.get_item_quick_slots()
	for slot_index in range(GameManager.ITEM_QUICK_SLOT_COUNT):
		var quick_button := overlay.find_child("InventoryQuickSlot_%d" % (slot_index + 1), true, false) as Button
		if quick_button == null:
			continue
		if slot_index < quick_slots.size():
			var item_id := quick_slots[slot_index]
			var item_data: Dictionary = GameManager.ITEMS.get(item_id, {})
			var count := GameManager.get_item_count(item_id)
			quick_button.text = "%d · %s\nx%d" % [slot_index + 1, String(item_data.get("name", item_id)), count]
			quick_button.icon = GameManager.get_item_icon(item_id)
			quick_button.expand_icon = true
			quick_button.disabled = count <= 0
			quick_button.tooltip_text = String(item_data.get("desc", ""))
			quick_button.set_meta("inventory_id", item_id)
		else:
			quick_button.text = "%d · EMPTY" % (slot_index + 1)
			quick_button.icon = null
			quick_button.disabled = true
			quick_button.tooltip_text = "Pin a carried supply from the detail panel."
			quick_button.set_meta("inventory_id", "")
	var selected_id := String(overlay.get_meta("inventory_selected_id", ""))
	if not GameManager.ITEMS.has(selected_id) or GameManager.get_item_count(selected_id) <= 0:
		selected_id = ""
		for item_button in buttons:
			var candidate := String(item_button.get_meta("inventory_id", ""))
			if GameManager.get_item_count(candidate) > 0:
				selected_id = candidate
				break
		overlay.set_meta("inventory_selected_id", selected_id)
	var preview := overlay.find_child("InventoryPreview", true, false) as TextureRect
	var title := overlay.find_child("InventoryDetailTitle", true, false) as Label
	var type_label := overlay.find_child("InventoryDetailType", true, false) as Label
	var body := overlay.find_child("InventoryDetailBody", true, false) as RichTextLabel
	var meta := overlay.find_child("InventoryDetailMeta", true, false) as Label
	var use_button := overlay.find_child("InventoryUseNow", true, false) as Button
	var pin_button := overlay.find_child("InventoryPinQuick", true, false) as Button
	if selected_id != "" and preview and title and type_label and body and meta:
		_set_inventory_detail(selected_id, preview, title, type_label, body, meta)
	if use_button:
		var usable := selected_id != "" and GameManager.can_use_field_item(selected_id)
		use_button.disabled = not usable
		if usable:
			use_button.text = "USE NOW"
		elif selected_id != "" and String(GameManager.ITEMS.get(selected_id, {}).get("type", "")) in ["heal", "cure"]:
			use_button.text = "HP FULL"
		else:
			use_button.text = "BATTLE USE ONLY"
	if pin_button:
		pin_button.disabled = selected_id == ""
		pin_button.text = "REMOVE FROM QUICK KIT" if GameManager.is_item_quick_slotted(selected_id) else "PIN TO QUICK KIT"
	var status_strip := overlay.find_child("InventoryStatusStrip", true, false) as Label
	if status_strip:
		var intact_memories := 0
		for memory in MemoryManager.memories:
			if not memory.is_burned and not memory.is_faded:
				intact_memories += 1
		status_strip.text = "HP %d/%d  |  GRAINS %d  |  MEMORIES %d INTACT / %d BURNED  |  CHAPTER %02d" % [
			int(GameManager.player_data.get("hp", 0)), int(GameManager.player_data.get("max_hp", 0)),
			int(GameManager.player_data.get("grains", 0)), intact_memories,
			MemoryManager.get_burn_count(), GameManager.current_chapter,
		]
	_apply_inventory_view_overlay(overlay)
func _inventory_effect_text(item_data: Dictionary) -> String:
	var item_type := String(item_data.get("type", ""))
	match item_type:
		"heal":
			return "RESTORE %d HP" % int(item_data.get("power", 0))
		"cure":
			return "CLEANSE STATUS / RESTORE %d HP" % int(item_data.get("recovery", 0))
		"burn":
			return "IMPACT %d / BURN %d TURNS" % [int(item_data.get("impact", 0)), int(item_data.get("duration", 2))]
		"flee":
			return "GUARANTEED ESCAPE"
		"witness":
			var witness_copy := "WITNESS +%d / GUARD" % int(item_data.get("power", 1))
			if int(item_data.get("recovery", 0)) > 0:
				witness_copy += " / RESTORE %d HP" % int(item_data.get("recovery", 0))
			return witness_copy
		"guard":
			return "GUARD NEXT BLOW / LIMIT +%d" % int(item_data.get("power", 0))
		"scan":
			return "SCAN TARGET / BREAK +%d" % int(item_data.get("power", 0))
	return "FIELD-READY SUPPLY"

func _on_inventory_item_pressed(item_id: String, overlay: Control) -> void:
	AudioManager.play_sfx("ui_select")
	overlay.set_meta("inventory_selected_id", item_id)
	_refresh_inventory_overlay(overlay)

func _set_inventory_detail(item_id: String, preview: TextureRect, title: Label, type_label: Label, body: RichTextLabel, meta: Label) -> void:
	var item_data: Dictionary = GameManager.ITEMS.get(item_id, {})
	if item_data.is_empty():
		return
	var type_names := {
		"heal": "RESTORATIVE",
		"cure": "REMEDY",
		"burn": "INCENDIARY",
		"flee": "EVASION TOOL",
		"witness": "WITNESS RELIC",
		"guard": "ANCHOR TOOL",
		"scan": "RECORDING TOOL",
	}
	preview.texture = GameManager.get_item_icon(item_id)
	title.text = String(item_data.get("name", item_id))
	type_label.text = "%s  |  %s" % [
		String(type_names.get(String(item_data.get("type", "")), "FIELD SUPPLY")),
		_inventory_effect_text(item_data),
	]
	body.text = "[color=#e1cda5]%s[/color]\n\n[color=#b5aa98]A carried object is useful because someone remembered to prepare it.[/color]" % String(item_data.get("desc", "No field note recorded."))
	meta.text = "CARRIED x%d    |    TRADE VALUE %d GRAINS" % [GameManager.get_item_count(item_id), int(item_data.get("price", 0))]

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
	subtitle.add_theme_font_size_override("font_size", 13)
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
			# Locked, dark with lock icon
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
		desc_lbl.add_theme_font_size_override("font_size", 13)
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
	close_label.add_theme_font_size_override("font_size", 13)
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
	stats_overlay.name = "CharacterStatusOverlay"
	stats_overlay.set_anchors_preset(Control.PRESET_FULL_RECT)
	stats_overlay.color = Color(0, 0, 0, 0)
	stats_overlay.mouse_filter = Control.MOUSE_FILTER_STOP
	add_child(stats_overlay)
	_add_modal_backdrop(stats_overlay, STATUS_BACKDROP_PATH, Color(0.006, 0.005, 0.012, 0.24))

	var status_portrait := TextureRect.new()
	status_portrait.name = "CharacterStatusPortrait"
	status_portrait.anchor_left = 0.105
	status_portrait.anchor_right = 0.29
	status_portrait.anchor_top = 0.18
	status_portrait.anchor_bottom = 0.60
	status_portrait.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	status_portrait.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
	status_portrait.mouse_filter = Control.MOUSE_FILTER_IGNORE
	var status_portrait_path := "res://assets/portraits/character_shots/arrel_story_v2.png"
	if ResourceLoader.exists(status_portrait_path):
		status_portrait.texture = load(status_portrait_path)
	stats_overlay.add_child(status_portrait)

	var status_resources := Label.new()
	status_resources.name = "CharacterStatusResources"
	status_resources.anchor_left = 0.105
	status_resources.anchor_right = 0.29
	status_resources.anchor_top = 0.63
	status_resources.anchor_bottom = 0.84
	status_resources.text = "HP  %d / %d\nGRAINS  %d\nMEMORIES  %d\nBURNED  %d" % [
		int(GameManager.player_data.get("hp", 0)),
		int(GameManager.player_data.get("max_hp", 0)),
		int(GameManager.player_data.get("grains", 0)),
		MemoryManager.memories.size(),
		MemoryManager.get_burn_count(),
	]
	status_resources.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	status_resources.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	status_resources.add_theme_font_size_override("font_size", 13)
	status_resources.add_theme_color_override("font_color", Color(0.84, 0.86, 0.81))
	stats_overlay.add_child(status_resources)

	var stats_panel = PanelContainer.new()
	stats_panel.name = "CharacterStatusPanel"
	stats_panel.anchor_left = 0.31
	stats_panel.anchor_right = 0.91
	stats_panel.anchor_top = 0.09
	stats_panel.anchor_bottom = 0.88
	var style = StyleBoxFlat.new()
	style.bg_color = Color(0.025, 0.021, 0.032, 0.42)
	style.border_color = Color(0.55, 0.43, 0.24, 0.42)
	style.set_border_width_all(1)
	style.set_corner_radius_all(6)
	style.set_content_margin_all(20)
	stats_panel.add_theme_stylebox_override("panel", style)
	stats_overlay.add_child(stats_panel)

	var vbox = VBoxContainer.new()
	vbox.add_theme_constant_override("separation", 10)
	stats_panel.add_child(vbox)

	# Header
	var header = Label.new()
	header.text = "CHARACTER DOSSIER / PLAY STATISTICS"
	header.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	header.add_theme_font_size_override("font_size", 20)
	header.add_theme_color_override("font_color", Color(0.92, 0.76, 0.45))
	UITheme.apply_title_font(header)
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
	var grade_labels := ["D", "C", "B", "A", "S"]
	var best_grade_rank := clampi(int(stats.get("highest_battle_grade", 0)), 0, grade_labels.size() - 1)
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
		{"label": "Best Battle Grade", "value": grade_labels[best_grade_rank]},
		{"label": "S-Rank Victories", "value": str(int(stats.get("s_rank_victories", 0)))},
		{"label": "Best Directive Chain", "value": "x%d" % int(stats.get("best_directive_streak", 0))},
		{"label": "Current Directive Chain", "value": "x%d" % GameManager.get_directive_streak()},
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
		name_label.add_theme_color_override("font_color", Color(0.80, 0.78, 0.72))
		row.add_child(name_label)

		var val_label = Label.new()
		val_label.text = entry.value
		val_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
		val_label.add_theme_font_size_override("font_size", 15)
		val_label.add_theme_color_override("font_color", Color(0.96, 0.82, 0.49))
		val_label.custom_minimum_size = Vector2(120, 0)
		row.add_child(val_label)

	# Close hint
	var close_label = Label.new()
	close_label.text = "[ESC] Close"
	close_label.add_theme_font_size_override("font_size", 13)
	close_label.add_theme_color_override("font_color", Color(0.68, 0.63, 0.55))
	close_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	vbox.add_child(close_label)

	# ESC close handler
	var close_handler = func(event: InputEvent):
		if event.is_action_pressed("cancel") or event.is_action_pressed("menu"):
			stats_overlay.queue_free()
			get_viewport().set_input_as_handled()
	stats_overlay.gui_input.connect(close_handler)
	_animate_modal_panel(stats_panel)

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
	hint.add_theme_font_size_override("font_size", 13)
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
