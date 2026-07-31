## StoryJournal, 스토리 저널 / 코덱스
## PauseMenu에서 접근. 게임 진행 중 자동으로 기록되는 이벤트/NPC/선택 목록.
## ESC로 닫기.
extends CanvasLayer

var is_open: bool = false

# ── UI 노드 ──
var backdrop: TextureRect
var overlay: ColorRect
var main_panel: PanelContainer
var tab_events: Button
var tab_npcs: Button
var tab_world: Button
var tab_choices: Button
var tab_quests_btn: Button
var tab_losses_btn: Button
var item_list: VBoxContainer
var item_scroll: ScrollContainer
var detail_title: Label
var detail_art: TextureRect
var detail_body: RichTextLabel
var close_hint: Label
var journal_summary_label: Label

var _current_tab: String = "events"
const JOURNAL_BACKDROP_PATH: String = "res://assets/cg/generated/ui_story_journal_backdrop_v3.png"

# ── 저널 엔트리 ──
# 자동으로 story_flags 기반으로 생성
const CHAPTER_NAMES := {
	1: "Rim Forest", 2: "Verdan Market", 3: "Belt Waystation", 4: "Drift Shelter",
	5: "Crumbling Coast", 6: "The Seam", 7: "Seam Outskirts", 8: "Forgotten Forest",
	9: "Colorless Waste", 10: "BL-07 Void", 11: "Epilogue", 12: "Verdan Underlock",
	13: "Eastern Reading Wall", 14: "Confessor Hall", 15: "Mneme Cell", 16: "East Road",
	17: "Forgetting Storm", 18: "Living Funeral", 19: "Lumea Approach", 20: "Monolith",
	21: "Records Chamber", 22: "The Core", 23: "Conversion", 24: "Testimony",
}

const EVENT_ART_BY_FLAG: Dictionary = {
	"ch1_camp_done": "res://assets/cg/generated/archive_ch1_camp_humming_v2.png",
	"ch2_malet_done": "res://assets/cg/generated/archive_ch2_information_price_v1.png",
	"ch3_kairos_writing": "res://assets/cg/generated/archive_ch3_kairos_marks_v1.png",
	"ch4_reading_loss": "res://assets/cg/generated/archive_ch4_reading_loss_v1.png",
	"elia_reunited": "res://assets/cg/generated/archive_ch6_reunion_v1.png",
	"ch8_tobias_theory": "res://assets/cg/generated/archive_ch8_ring_theory_v1.png",
}

# 이벤트 엔트리 (flag → 표시 정보)
const EVENT_ENTRIES := [
	{"flag": "ch1_opening_done", "chapter": 1, "title": "Awakening in the Forest", "desc": "Arrel came to in the Rim Forest, blade drawn, a dead Void Beast dissolving at his feet. No memory of the fight.", "art": "res://assets/portraits/character_shots/arrel_battle_v3.png"},
	{"flag": "ch1_elia_appeared", "chapter": 1, "title": "Elia Appears", "desc": "A woman called Elia found him, or rather, had been following him. She spoke as if she knew him. He couldn't remember.", "art": "res://assets/portraits/character_shots/elia_anchor_v3.png"},
	{"flag": "ch1_ash_rain_seen", "chapter": 1, "title": "Ash Rain", "desc": "Gray flakes began to fall. Not snow, ash. The residue of dissolved memories, raining from the empty sky."},
	{"flag": "hidden_ch1_stump", "chapter": 1, "title": "[Hidden] The Old Stump", "desc": "A tree stump with rings too numerous to count. Something about it felt important, older than the Collapse."},
	{"flag": "ch1_camp_done", "chapter": 1, "title": "Camp at the Forest Edge", "desc": "Night. Elia hummed a melody. Arrel's hand trembled. In the morning, they headed south toward Verdan."},
	{"flag": "ch2_arrived", "chapter": 2, "title": "Verdan Market", "desc": "The Gray Belt's largest settlement. Smoke, noise, and the smell of things being traded that shouldn't be."},
	{"flag": "malet_deal_accepted", "chapter": 2, "title": "Malet's Price, Paid", "art": "res://assets/cg/generated/archive_verdan_memory_price_v1.png", "desc": "Malet extracted the memory of holding a sword for the first time. A courtyard, dust, a hand closing fingers around a grip. Gone."},
	{"flag": "malet_deal_refused", "chapter": 2, "title": "Malet's Price, Refused", "desc": "Arrel refused to sell his sword memory. The amber-eyed dealer's dismissal was absolute."},
	{"flag": "ch2_malet_done", "chapter": 2, "title": "Information Acquired", "desc": "Three things from Malet: a route through the Coast, a name (Sable), and a warning, Editor Kairos. Four days."},
	{"flag": "ch3_arrived", "chapter": 3, "title": "The Belt Waystation", "desc": "A dead road through dead earth. Bureau Relay Station 14 stands alone, and someone new sits inside."},
	{"flag": "ch3_tobias_met", "chapter": 3, "title": "Tobias Crane", "desc": "A Bureau Recorder with ink-stained fingers. Twenty years of memory transactions. He sees things others don't.", "art": "res://assets/portraits/character_shots/tobias_ledger_v3.png"},
	{"flag": "has_blank_book", "chapter": 3, "title": "The Blank Book", "art": "res://assets/cg/generated/archive_belt_blank_book_v1.png", "desc": "Record-tree fiber. It absorbs the shape of memories, the contour remains even after the memory burns."},
	{"flag": "ch3_kairos_writing", "chapter": 3, "title": "Wall Writing", "desc": "'Subject demonstrates Class Seven combustion efficiency.' Scratched into concrete. Recent. Someone is watching."},
	{"flag": "ch4_arrived", "chapter": 4, "title": "Drift", "desc": "A collapsed overpass. Memory rain falling. The letters in the Blank Book swim and blur."},
	{"flag": "ch4_reading_loss", "chapter": 4, "title": "Reading Deterioration", "desc": "Words move when they shouldn't. A known side effect, Tobias says. 'Recovery not guaranteed.'"},
	{"flag": "ch4_anchoring", "chapter": 4, "title": "Anchoring Session", "art": "res://assets/cg/generated/archive_drift_anchoring_v1.png", "desc": "Warm hands. The smell of bread. A page pressed back into its binding. Elia tethers the architecture."},
	{"flag": "ch5_arrived", "chapter": 5, "title": "The Crumbling Coast", "desc": "Where land forgets how to be solid. Salt air and dissolved memory leaching into the sea."},
	{"flag": "ch5_kairos_seen", "chapter": 5, "title": "Kairos Observed", "desc": "A figure on the ridge. Still as stone. Not chasing, classifying. Elia said that was worse.", "art": "res://assets/portraits/character_shots/kairos_edit_v3.png"},
	{"flag": "elia_separates", "chapter": 5, "title": "Elia Took the Coastal Path", "art": "res://assets/cg/generated/archive_ch5_coastal_parting_v1.png", "desc": "To split Kairos's attention, Elia went south alone. Without her, burned memories would leave no residue. Just absence."},
	{"flag": "elia_stays", "chapter": 5, "title": "Together Through the Coast", "desc": "They stayed together. Two signatures, easier to track, but harder to break."},
	{"flag": "ch6_arrived", "chapter": 6, "title": "The Seam", "desc": "Color between gray cliffs. Amber, crimson, green. A settlement in the cracks of what was and what will be."},
	{"flag": "elia_reunited", "chapter": 6, "title": "Reunion", "desc": "Elia stood at the Seam's entrance. The coastal path worked, Kairos went south. The anchor tightened."},
	{"flag": "hidden_ch6_garden", "chapter": 6, "title": "[Hidden] The Impossible Garden", "desc": "White petals veined with gold. Warm to the touch. A fragment of someone handing a flower, small hands, a child's laugh."},
	{"flag": "ch6_briefing_done", "chapter": 6, "title": "Sable's Briefing", "desc": "BL-07 forming south. If it opens, the Seam dies. A Shade Sentinel guards the entrance. Investigation required.", "art": "res://assets/cg/generated/archive_seam_lantern_watch_v1.png"},
	{"flag": "ch6_briefing_done", "chapter": 6, "title": "The Warden's Watch", "desc": "Sable never promised safety. She stood beside Arrel anyway, keeping watch while the Seam slept.", "art": "res://assets/cg/generated/illustration_expansion_v2/bond_arrel_sable_watch_v1.png"},
	{"flag": "ch6_bl07_entered", "chapter": 6, "title": "The Shade Sentinel", "desc": "Dark. Wrong. The Void's immune response. Between them and BL-07, it coalesced."},
	{"flag": "ch7_arrived", "chapter": 7, "title": "The Threshold", "desc": "Beyond The Seam's walls, the cliffs opened into a jagged plateau. The air tasted of static."},
	{"flag": "ch7_sable_truth", "chapter": 7, "title": "Sable's Truth", "art": "res://assets/cg/generated/archive_ch7_seven_lanterns_v1.png", "desc": "BL-07 isn't a hole. It's a mouth. It calls memories. It's hungry. Seven people went in with Sable. She came out alone."},
	{"flag": "has_echo_shell", "chapter": 7, "title": "The Echo Shell", "desc": "A spiraling shell covered in luminescent veins. It holds the last words of everyone BL-07 consumed."},
	{"flag": "ch7_trial_complete", "chapter": 7, "title": "Sable's Trial", "desc": "A controlled burn on the Threshold. The memory passed like a wave of heat. Arrel held. Most people don't."},
	{"flag": "ch8_arrived", "chapter": 8, "title": "The Forest That Forgets", "desc": "Trees that weren't growing, standing because they forgot how to fall. Body-temperature bark. Memory-parasitic ecosystem."},
	{"flag": "ch8_ghost", "chapter": 8, "title": "The Remnants", "desc": "Shapes between the trees. Mouths open in soundless words. What's left when BL-07 takes everything except the shape."},
	{"flag": "ch8_tobias_theory", "chapter": 8, "title": "Ring Theory", "desc": "The forest grows in concentric rings. Each ring is a feeding event, a generation of people erased. Eighteen rings total."},
	{"flag": "ch9_arrived", "chapter": 9, "title": "Where Colors Stop", "desc": "Color ended like a sentence cut short. Not gray, the concept of color simply withdrew from the world."},
	{"flag": "ch9_compass", "chapter": 9, "title": "The Memory Compass", "desc": "BL-07 recognizes Class Seven combustion. Arrel's body became a compass, pulled toward the Void Hole's core."},
	{"flag": "ch9_kairos", "chapter": 9, "title": "Kairos Confrontation", "art": "res://assets/cg/generated/archive_ch9_kairos_outcomes_v1.png", "desc": "The Editor appeared in the Waste. Not to stop them, to observe the convergence. Two outcomes. Both terrible."},
	{"flag": "ch9_kairos_truth", "chapter": 9, "title": "Kairos's Calculation", "art": "res://assets/cg/generated/archive_ch9_kairos_outcomes_v1.png", "desc": "Outcome A: BL-07 collapses, subject loses everything. Outcome B: BL-07 doubles. Sable said they'd find a third."},
	{"flag": "ch10_complete", "chapter": 10, "title": "The Seal", "art": "res://assets/cg/generated/archive_ch10_burden_choice_v1.png", "desc": "BL-07's core. A decision: burn everything to close it, or keep your name and find another way."},
	{"flag": "zero_burn_path", "chapter": 10, "title": "Zero Burn, Name Consumed", "desc": "He burned 'Arrel.' The name that meant something. The Void Hole collapsed. He didn't know who he was anymore."},
	{"flag": "seal_refused", "chapter": 10, "title": "Preservation, Name Kept", "desc": "He refused to burn his name. BL-07 remains unsolved. But he remembers who he is."},
	{"flag": "seal_weave", "chapter": 10, "title": "The Weave, Everything Kept", "desc": "He offered every memory he had preserved, all at once. BL-07 sealed without erasing him. Part of him now holds the door shut."},
	{"flag": "epilogue_complete", "chapter": 11, "title": "Epilogue", "desc": "The Seam. Aftermath. What remains after everything is either burned or saved."},
	{"flag": "ch13_trusted_tobias", "chapter": 13, "title": "The Record Tree Between Them", "desc": "Arrel trusted Tobias with the shape of a memory neither man could safely carry alone.", "art": "res://assets/cg/generated/illustration_expansion_v2/bond_arrel_tobias_record_tree_v1.png"},
	{"flag": "storm_survived", "chapter": 17, "title": "A Thread Through the Storm", "desc": "When the forgetting storm took the road, Arrel and Elia kept one another present by touch and voice.", "art": "res://assets/cg/generated/illustration_expansion_v2/bond_arrel_elia_storm_thread_v1.png"},
	{"flag": "nera_first_emotion", "chapter": 21, "title": "The Mirror Refused Its Order", "desc": "Elia recognized the first human hesitation beneath Nera's immaculate Authority mask.", "art": "res://assets/cg/generated/illustration_expansion_v2/bond_elia_nera_mirror_v1.png"},
	{"flag": "kairos_final", "chapter": 21, "title": "The Missing Piece", "desc": "Arrel and Kairos faced the same broken record and chose opposite meanings for its empty space.", "art": "res://assets/cg/generated/illustration_expansion_v2/bond_arrel_kairos_missing_piece_v1.png"},
]

# NPC 엔트리
const NPC_ENTRIES := [
	{"flag": "ch1_elia_appeared", "name": "Elia", "role": "Anchor / Companion", "art": "res://assets/portraits/character_shots/elia_anchor_v3.png",
	 "desc": "Silver-haired woman who follows Arrel. Her presence slows memory decay, an 'anchoring' effect. Knows more than she says."},
	{"flag": "ch2_arrived", "name": "Malet", "role": "Memory Dealer",
	 "desc": "Small, thin, dressed in gray. Amber eyes from high-grade Memory Ampoules. Trades information for memories in the Sump beneath Verdan."},
	{"flag": "ch3_tobias_met", "name": "Tobias Crane", "role": "Bureau Recorder / Class C", "art": "res://assets/portraits/character_shots/tobias_ledger_v3.png",
	 "desc": "Ink-stained fingers, spectacles. Twenty years recording memory transactions. Walked the Belt alone to update records. Knows the Bureau inside out."},
	{"flag": "ch6_arrived", "name": "Sable", "role": "Seam Leader / Void Walker", "art": "res://assets/portraits/character_shots/sable_warden_v3.png",
	 "desc": "An older Void Walker with clouded gray eyes, silver hair, and a dark-plum hood. Walked into a Void Hole and walked out. Leads the Seam through practical routines, mutual witness, and hard choices."},
	{"flag": "ch5_kairos_seen", "name": "Kairos", "role": "Editor / Pursuer", "art": "res://assets/portraits/character_shots/kairos_edit_v3.png",
	 "desc": "An Authority Editor. Quiet. Methodical. Classifies rather than chases. His presence means the Authority knows about Arrel's burning."},
]

# World entries are deliberately chapter-gated. They explain the pressure around the party
# without revealing later-book answers before the player has reached the relevant place.
const WORLD_ENTRIES := [
	{"flag": "ch1_ash_rain_seen", "chapter": 1, "title": "Ash Rain, What the Sky Keeps", "art": "res://assets/cg/generated/story_ch1_ash_rain_touch.png",
	 "desc": "Ash Rain is not weather. When extracted memories are broken, their emotional residue disperses into the air and eventually falls as gray flakes. It can carry a pressure, a scent, or the edge of a feeling, never a complete life. The Rim has learned to close its windows before it starts."},
	{"flag": "ch2_arrived", "chapter": 2, "title": "Grains, Ampoules, and Debt", "art": "res://assets/environment/map_canvases/map_verdan_market_canvas_v1.png",
	 "desc": "A memory can be weighed, graded, sealed in an ampoule, and traded as Grains. It can buy food, passage, or a lie someone needs to hear. But a purchased memory is only a visitor in another mind: it may be felt, never truly burned as fuel. That boundary is why the market can profit from memory without giving everyone Arrel's power."},
	{"flag": "ch2_malet_done", "chapter": 2, "title": "Information Is a Delayed Weapon", "art": "res://assets/cg/generated/archive_ch2_information_price_v1.png",
	 "desc": "Malet profits from more than secrets. A route disclosed today can outrun an Authority courier by one night; a name withheld can keep a settlement alive until morning. In the Gray Belt, information has value because every system arrives late, and someone always pays for the time between knowing and acting."},
	{"flag": "ch3_arrived", "chapter": 3, "title": "The Belt Watches in Delays", "art": "res://assets/environment/map_canvases/map_belt_signal_yard_canvas_v1.png",
	 "desc": "The Authority's signal towers catch the flare left by memory combustion. In the Belt, Arrel's unusually bright signature is a beacon; in the Rim, sparse relays turn it into a rumor. The system is frightening, not perfect: scribes must still write, couriers must still travel, and frightened citizens decide whether to report what they saw."},
	{"flag": "has_blank_book", "chapter": 3, "title": "Record-Tree Paper", "art": "res://assets/cg/generated/story_ch3_waystation_blank_book.png",
	 "desc": "The Blank Book is made from the fiber of a record-tree, a First-Age plant that does not answer the Authority's record ink. Its pages cannot be scanned by the Monolith and must be copied by hand. That freedom has a cost: destroy the physical page and there is no archive beneath it, no recovery, and no official proof it ever existed."},
	{"flag": "ch4_anchoring", "chapter": 4, "title": "Anchoring Is a Practice, Not a Cure", "art": "res://assets/environment/map_canvases/map_drift_shelter_canvas_v2.png",
	 "desc": "An anchor does not restore what was burned. Elia uses ordinary, repeatable sensory facts, the warmth of bread, a hand around a cup, a familiar song, to give damaged memory somewhere to settle. It keeps a person from coming apart in the moment. It cannot promise that the page will still be there tomorrow."},
	{"flag": "ch5_arrived", "chapter": 5, "title": "The Coast Returns Pressure, Not People", "art": "res://assets/cg/generated/archive_ch5_coastal_parting_v1.png",
	 "desc": "Dissolved memories wash back as salt-light, familiar gestures, and dreams that belong to no single survivor. Coastal families leave two route markers at every dangerous fork: one for the traveler and one for the witness expected to remember which way they went. A returned feeling may deserve care without being mistaken for a restored life."},
	{"flag": "ch6_arrived", "chapter": 6, "title": "The Seam's Refuge Pact", "art": "res://assets/environment/map_canvases/map_the_seam_canvas_v1.png",
	 "desc": "The Seam survives because people share what the Authority turns into leverage: names, meals, routes, and witnesses. No one is asked to prove a loss with an official record. Each resident is expected to remember one practical thing for someone else, a safe path, a medicine recipe, a face that must not vanish."},
	{"flag": "ch6_briefing_done", "chapter": 6, "title": "A Refuge Is Made of Work", "art": "res://assets/game_image/reference/seam_residents_reference_sheet_v1.png",
	 "desc": "The Seam has no permanent monument to its dead. It has lantern rotations, witness ledgers, a garden roster, and scouts who bring route knots home. Sable assigns every newcomer one task that preserves another person's tomorrow. Here, maintenance is not background labor; it is how an undocumented community refuses to disappear."},
	{"flag": "ch7_sable_truth", "chapter": 7, "title": "What a Void Hole Returns", "art": "res://assets/cg/character_shots/echo_shell_shot_v2.png",
	 "desc": "BL-07 does not simply kill. It strips a life into pressure, gesture, and unfinished voice. Echo Shells are not the people they imitate, but neither are they empty monsters. Treating them as disposable makes the Void's work easier; listening to them risks letting their hunger answer back."},
	{"flag": "ch8_tobias_theory", "chapter": 8, "title": "The Listening Wood", "art": "res://assets/cg/generated/archive_ch8_ring_theory_v1.png",
	 "desc": "The forest grows around repeated erasures. Its roots remember heat and fear as if they were water, while its paths borrow familiar shapes to lead travelers deeper. The rings Tobias counted are not years. They are feeding events, places where too many people were reduced to echoes at once."},
	{"flag": "ch9_compass", "chapter": 9, "title": "Witnesses in the Colorless Waste", "art": "res://assets/environment/map_canvases/map_colorless_waste_canvas_v2.png",
	 "desc": "Here a compass points toward the densest surviving memory rather than north. Caravans trade testimony in pairs: one person speaks, another confirms they heard it. It is a fragile economy built against revision. In a place color itself can forget, an unwitnessed story is already halfway gone."},
	{"flag": "ch9_kairos_truth", "chapter": 9, "title": "Prediction Is Not Responsibility", "art": "res://assets/cg/generated/archive_ch9_kairos_outcomes_v1.png",
	 "desc": "Editors are trained to make suffering legible as probability: acceptable loss, unstable subject, necessary revision. Kairos can describe both futures with precision, but a correct forecast does not choose who bears its cost. The Authority's deepest habit is not ignorance. It is treating responsibility as a category someone else will fill."},
	{"flag": "ch10_complete", "chapter": 10, "title": "The Seal Is a Choice of Burdens", "art": "res://assets/cg/generated/archive_ch10_burden_choice_v1.png",
	 "desc": "Closing a wound in the world is not the same as making it whole. Every answer at BL-07 asks who carries the cost afterward: Arrel's name, the people waiting beyond the door, or a future problem left alive. The world changes most when someone refuses to call that cost inevitable."},
]

func _ready() -> void:
	layer = 57  # OptionsMenu(56) 위
	_build_ui()
	_hide_ui()
	print("[StoryJournal] Ready, Codex")

func _unhandled_input(event: InputEvent) -> void:
	if not is_open:
		return
	if event.is_action_pressed("cancel"):
		close_journal()
		get_viewport().set_input_as_handled()

func open_journal() -> void:
	if is_open:
		return
	is_open = true
	AudioManager.play_sfx("ui_open")
	_current_tab = "events"
	_refresh_list()
	_show_ui()

func close_journal() -> void:
	if not is_open:
		return
	is_open = false
	AudioManager.play_sfx("ui_close")
	_hide_ui()

## ===================== UI 구축 =====================

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
	backdrop.modulate = Color(0.86, 0.78, 0.70, 0.94)
	if ResourceLoader.exists(JOURNAL_BACKDROP_PATH):
		backdrop.texture = load(JOURNAL_BACKDROP_PATH)
	root.add_child(backdrop)

	overlay = ColorRect.new()
	overlay.set_anchors_preset(Control.PRESET_FULL_RECT)
	overlay.color = Color(0.010, 0.010, 0.015, 0.66)
	root.add_child(overlay)

	main_panel = PanelContainer.new()
	main_panel.anchor_left = 0.06
	main_panel.anchor_right = 0.94
	main_panel.anchor_top = 0.04
	main_panel.anchor_bottom = 0.96
	main_panel.add_theme_stylebox_override("panel", UITheme.make_panel_style(
		Color(0.050, 0.042, 0.038, 0.90),
		Color(0.62, 0.45, 0.24, 0.72),
		2, 6, 16
	))
	root.add_child(main_panel)

	var main_vbox = VBoxContainer.new()
	main_vbox.add_theme_constant_override("separation", 10)
	main_panel.add_child(main_vbox)

	# ── 헤더 ──
	var header = Label.new()
	header.text = "JOURNAL, Field Notes of a Memory Carrier"
	header.add_theme_font_size_override("font_size", 18)
	header.add_theme_color_override("font_color", UITheme.TEXT_ACCENT)
	main_vbox.add_child(header)

	journal_summary_label = Label.new()
	journal_summary_label.add_theme_font_size_override("font_size", 12)
	journal_summary_label.add_theme_color_override("font_color", Color(0.64, 0.58, 0.48))
	journal_summary_label.autowrap_mode = TextServer.AUTOWRAP_WORD
	main_vbox.add_child(journal_summary_label)

	# ── 탭 ──
	var tab_row = HBoxContainer.new()
	tab_row.add_theme_constant_override("separation", 8)
	main_vbox.add_child(tab_row)

	tab_events = _create_tab("Events", "events")
	tab_row.add_child(tab_events)
	tab_npcs = _create_tab("People", "npcs")
	tab_row.add_child(tab_npcs)
	tab_world = _create_tab("World", "world")
	tab_row.add_child(tab_world)
	tab_choices = _create_tab("Choices", "choices")
	tab_row.add_child(tab_choices)
	tab_quests_btn = _create_tab("Quests", "quests")
	tab_row.add_child(tab_quests_btn)
	tab_losses_btn = _create_tab("Losses", "losses")
	tab_row.add_child(tab_losses_btn)

	# 구분선
	var sep = HSeparator.new()
	sep.add_theme_color_override("separator", UITheme.BORDER_DIM)
	main_vbox.add_child(sep)

	# ── 본문 ──
	var content = HBoxContainer.new()
	content.size_flags_vertical = Control.SIZE_EXPAND_FILL
	content.add_theme_constant_override("separation", 12)
	main_vbox.add_child(content)

	# 좌측: 목록
	item_scroll = ScrollContainer.new()
	item_scroll.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	item_scroll.horizontal_scroll_mode = ScrollContainer.SCROLL_MODE_DISABLED
	content.add_child(item_scroll)

	item_list = VBoxContainer.new()
	item_list.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	item_list.add_theme_constant_override("separation", 4)
	item_scroll.add_child(item_list)

	# 우측: 상세
	var detail_panel = PanelContainer.new()
	detail_panel.custom_minimum_size = Vector2(380, 0)
	detail_panel.add_theme_stylebox_override("panel", UITheme.make_panel_style(
		Color(0.06, 0.05, 0.04, 0.8), UITheme.BORDER_DIM, 1, 4, 16
	))
	content.add_child(detail_panel)

	var detail_vbox = VBoxContainer.new()
	detail_vbox.add_theme_constant_override("separation", 10)
	detail_panel.add_child(detail_vbox)

	detail_title = Label.new()
	detail_title.text = "Select an entry..."
	detail_title.add_theme_font_size_override("font_size", 16)
	detail_title.add_theme_color_override("font_color", Color(0.8, 0.72, 0.58))
	detail_vbox.add_child(detail_title)

	var sep2 = HSeparator.new()
	sep2.add_theme_color_override("separator", UITheme.BORDER_DIM)
	detail_vbox.add_child(sep2)

	detail_art = TextureRect.new()
	detail_art.custom_minimum_size = Vector2(360, 142)
	detail_art.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	detail_art.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_COVERED
	detail_art.mouse_filter = Control.MOUSE_FILTER_IGNORE
	detail_art.visible = false
	detail_vbox.add_child(detail_art)

	detail_body = RichTextLabel.new()
	detail_body.bbcode_enabled = false
	detail_body.fit_content = true
	detail_body.scroll_active = true
	detail_body.size_flags_vertical = Control.SIZE_EXPAND_FILL
	detail_body.add_theme_font_size_override("normal_font_size", 14)
	detail_body.add_theme_color_override("default_color", Color(0.7, 0.65, 0.6))
	detail_vbox.add_child(detail_body)

	# ── 하단 ──
	close_hint = Label.new()
	close_hint.text = "[ESC] Close Journal"
	close_hint.add_theme_font_size_override("font_size", 12)
	close_hint.add_theme_color_override("font_color", UITheme.TEXT_DIM)
	close_hint.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	main_vbox.add_child(close_hint)

func _create_tab(text: String, tab_name: String) -> Button:
	var btn = Button.new()
	btn.text = text
	btn.custom_minimum_size = Vector2(120, 28)
	var style = UITheme.make_button_style()
	btn.add_theme_stylebox_override("normal", style)
	btn.add_theme_stylebox_override("hover", UITheme.make_hover_style(style))
	btn.add_theme_font_size_override("font_size", 13)
	btn.add_theme_color_override("font_color", UITheme.TEXT_DIM)
	btn.add_theme_color_override("font_hover_color", UITheme.TEXT_ACCENT)
	btn.pressed.connect(func():
		_current_tab = tab_name
		_refresh_list()
		AudioManager.play_sfx("ui_select")
	)
	btn.mouse_entered.connect(func(): AudioManager.play_sfx("ui_hover"))
	return btn

## ===================== 목록 갱신 =====================

func _refresh_list() -> void:
	_update_tab_styles()

	for child in item_list.get_children():
		child.queue_free()

	detail_title.text = "Select an entry..."
	detail_body.text = ""
	if detail_art:
		detail_art.texture = null
		detail_art.visible = false
	_update_journal_summary()

	match _current_tab:
		"events":
			_populate_events()
		"npcs":
			_populate_npcs()
		"world":
			_populate_world()
		"choices":
			_populate_choices()
		"quests":
			_populate_quests()
		"losses":
			_populate_losses()

func _update_journal_summary() -> void:
	if not journal_summary_label:
		return
	var burn_count := MemoryManager.get_burn_count() if MemoryManager else 0
	var held_count := MemoryManager.memories.size() - burn_count if MemoryManager else 0
	var loss_count := 0
	if WorldRewriteDirector and WorldRewriteDirector.has_method("get_loss_records"):
		loss_count = WorldRewriteDirector.get_loss_records().size()
	var unlocked_events := 0
	var illustrated_events := 0
	for entry in EVENT_ENTRIES:
		if not GameManager.get_flag(String(entry.get("flag", ""))):
			continue
		unlocked_events += 1
		var art_path := String(EVENT_ART_BY_FLAG.get(String(entry.get("flag", "")), entry.get("art", "")))
		if art_path != "" and ResourceLoader.exists(art_path):
			illustrated_events += 1
	var ch_name: String = String(CHAPTER_NAMES.get(GameManager.current_chapter, "Unknown"))
	journal_summary_label.text = "Ch.%d / %s    Held: %d    Burned: %d    Losses: %d    Illustrated: %d/%d" % [
		GameManager.current_chapter, ch_name, held_count, burn_count, loss_count, illustrated_events, unlocked_events
	]

func _update_tab_styles() -> void:
	for tab_data in [{"btn": tab_events, "name": "events"}, {"btn": tab_npcs, "name": "npcs"}, {"btn": tab_world, "name": "world"}, {"btn": tab_choices, "name": "choices"}, {"btn": tab_quests_btn, "name": "quests"}, {"btn": tab_losses_btn, "name": "losses"}]:
		var btn: Button = tab_data.btn
		var active: bool = (_current_tab == tab_data.name)
		if active:
			btn.add_theme_color_override("font_color", UITheme.TEXT_ACCENT)
			var style = btn.get_theme_stylebox("normal").duplicate()
			style.border_color = UITheme.TEXT_ACCENT
			style.set_border_width_all(1)
			btn.add_theme_stylebox_override("normal", style)
		else:
			btn.add_theme_color_override("font_color", UITheme.TEXT_DIM)
			btn.add_theme_stylebox_override("normal", UITheme.make_button_style())

func _populate_events() -> void:
	var last_chapter := 0
	for entry in EVENT_ENTRIES:
		if not GameManager.get_flag(entry.flag):
			continue
		# 챕터 헤더
		if entry.chapter != last_chapter:
			last_chapter = entry.chapter
			var header = Label.new()
			header.text = "Chapter %d: %s" % [entry.chapter, CHAPTER_NAMES.get(entry.chapter, "")]
			header.add_theme_font_size_override("font_size", 12)
			header.add_theme_color_override("font_color", UITheme.TEXT_NARRATION)
			item_list.add_child(header)
		var is_hidden = entry.title.begins_with("[Hidden]")
		var color = Color(0.6, 0.5, 0.7) if is_hidden else Color(0.75, 0.7, 0.65)
		var art_path := String(EVENT_ART_BY_FLAG.get(String(entry.get("flag", "")), entry.get("art", "")))
		_add_list_button(entry.title, color, entry.title, entry.desc, art_path)

func _populate_npcs() -> void:
	for npc in NPC_ENTRIES:
		if not GameManager.get_flag(npc.flag):
			continue
		var speaker_color = UITheme.get_speaker_color(npc.name)
		var full_desc = "%s\n\n%s" % [npc.role, npc.desc]
		_add_list_button(npc.name, speaker_color, npc.name, full_desc, String(npc.get("art", "")))

func _populate_world() -> void:
	var last_chapter := 0
	for entry in WORLD_ENTRIES:
		if not GameManager.get_flag(entry.flag):
			continue
		if entry.chapter != last_chapter:
			last_chapter = entry.chapter
			var header = Label.new()
			header.text = "Learned in Chapter %d: %s" % [entry.chapter, CHAPTER_NAMES.get(entry.chapter, "")]
			header.add_theme_font_size_override("font_size", 12)
			header.add_theme_color_override("font_color", Color(0.55, 0.60, 0.78))
			item_list.add_child(header)
		_add_list_button(entry.title, Color(0.50, 0.57, 0.78), entry.title, entry.desc, String(entry.get("art", "")))

	if item_list.get_child_count() == 0:
		var empty = Label.new()
		empty.text = "The world is still becoming legible. Travel, listen, and keep a record."
		empty.autowrap_mode = TextServer.AUTOWRAP_WORD
		empty.add_theme_font_size_override("font_size", 13)
		empty.add_theme_color_override("font_color", UITheme.TEXT_DIM)
		item_list.add_child(empty)

func _populate_choices() -> void:
	# 선택 기록, 주요 분기점
	var choice_entries := [
		{"flag": "malet_deal_accepted", "title": "Accepted Malet's Deal", "art": "res://assets/cg/generated/archive_verdan_memory_price_v1.png", "desc": "Traded the memory of first holding a sword for information. The courtyard, the dust, the hand, gone."},
		{"flag": "malet_deal_refused", "title": "Refused Malet's Deal", "desc": "Kept the sword memory. Left the Sump without Malet's help. (But returned later.)"},
		{"flag": "elia_separates", "title": "Sent Elia South", "art": "res://assets/cg/generated/archive_ch5_coastal_parting_v1.png", "desc": "Split up at the Crumbling Coast to confuse Kairos. Burned memories during separation left no residue."},
		{"flag": "elia_stays", "title": "Kept Elia Close", "desc": "Traveled the Coast together. The anchor stayed. Memories burned still left traces."},
		{"flag": "zero_burn_path", "title": "Burned Your Name", "art": "res://assets/cg/generated/archive_ch10_burden_choice_v1.png", "desc": "Zero Burn. The ultimate sacrifice. BL-07 closed, but the person called 'Arrel' ceased to exist."},
		{"flag": "seal_refused", "title": "Kept Your Name", "art": "res://assets/cg/generated/archive_ch10_burden_choice_v1.png", "desc": "Refused the Seal. BL-07 remains, but so does the person who remembers."},
		{"flag": "seal_weave", "title": "Wove the Seal", "art": "res://assets/cg/generated/archive_ch10_burden_choice_v1.png", "desc": "The Weave. Sealed BL-07 by offering every preserved memory at once, name and self kept intact. The price was never being able to set them down."},
	]
	for entry in choice_entries:
		if not GameManager.get_flag(entry.flag):
			continue
		_add_list_button(entry.title, Color(0.7, 0.6, 0.45), entry.title, entry.desc, String(entry.get("art", "")))

	if item_list.get_child_count() == 0:
		var empty = Label.new()
		empty.text = "No major choices recorded yet."
		empty.add_theme_font_size_override("font_size", 13)
		empty.add_theme_color_override("font_color", UITheme.TEXT_DIM)
		item_list.add_child(empty)

func _populate_quests() -> void:
	var quests = SideQuest.get_all_quests()
	var has_any = false
	for q in quests:
		var status = q["status"]
		if status == "locked":
			continue
		has_any = true
		var color: Color
		var prefix: String
		var desc_text: String
		match status:
			"complete":
				color = Color(0.4, 0.7, 0.4)
				prefix = "[DONE] "
				desc_text = q["desc"]
			"active":
				color = Color(0.85, 0.7, 0.4)
				prefix = ""
				desc_text = q["desc"] + "\n\nCurrent: " + q["step_desc"]
			"available":
				color = Color(0.55, 0.5, 0.45)
				prefix = "[NEW] "
				desc_text = q["desc"] + "\n\nTalk to %s at %s." % [q["npc"], q["map"].replace("_", " ").capitalize()]
		_add_list_button(prefix + q["title"], color, q["title"], desc_text, String(q.get("art", "")))

	if not has_any:
		var empty = Label.new()
		empty.text = "No quests discovered yet."
		empty.add_theme_font_size_override("font_size", 13)
		empty.add_theme_color_override("font_color", UITheme.TEXT_DIM)
		item_list.add_child(empty)

func _populate_losses() -> void:
	var records: Array[Dictionary] = []
	if WorldRewriteDirector and WorldRewriteDirector.has_method("get_loss_records"):
		records = WorldRewriteDirector.get_loss_records()

	for record in records:
		var color: Color = record.get("color", Color(0.75, 0.58, 0.42))
		_add_list_button(String(record.get("title", "Uncatalogued Loss")), color, String(record.get("title", "Uncatalogued Loss")), String(record.get("body", "")), String(record.get("art", "")))

	if records.is_empty():
		var empty = Label.new()
		empty.text = "No irreversible losses recorded yet."
		empty.add_theme_font_size_override("font_size", 13)
		empty.add_theme_color_override("font_color", UITheme.TEXT_DIM)
		item_list.add_child(empty)

func _add_list_button(text: String, color: Color, title: String, desc: String, art_path: String = "") -> void:
	var btn = Button.new()
	btn.text = text
	btn.alignment = HORIZONTAL_ALIGNMENT_LEFT
	btn.custom_minimum_size = Vector2(0, 32)

	var style = StyleBoxFlat.new()
	style.bg_color = Color(color.r * 0.2, color.g * 0.2, color.b * 0.2, 0.4)
	style.border_width_left = 3
	style.border_color = Color(color.r * 0.4, color.g * 0.4, color.b * 0.4, 0.3)
	style.set_content_margin_all(6)
	style.set_corner_radius_all(2)
	btn.add_theme_stylebox_override("normal", style)

	var hover = style.duplicate()
	hover.bg_color = Color(color.r * 0.3, color.g * 0.3, color.b * 0.3, 0.6)
	hover.border_color = color
	btn.add_theme_stylebox_override("hover", hover)
	btn.add_theme_stylebox_override("focus", hover)

	btn.add_theme_font_size_override("font_size", 13)
	btn.add_theme_color_override("font_color", Color(0.72, 0.68, 0.62))
	btn.add_theme_color_override("font_hover_color", color.lightened(0.3))

	btn.pressed.connect(func():
		detail_title.text = title
		detail_body.text = desc
		if detail_art:
			if art_path != "" and ResourceLoader.exists(art_path):
				detail_art.texture = load(art_path)
				detail_art.visible = true
			else:
				detail_art.texture = null
				detail_art.visible = false
	)
	btn.mouse_entered.connect(func(): AudioManager.play_sfx("ui_hover"))
	item_list.add_child(btn)

func _show_ui() -> void:
	if backdrop:
		backdrop.visible = true
	overlay.visible = true
	main_panel.visible = true

func _hide_ui() -> void:
	if backdrop:
		backdrop.visible = false
	overlay.visible = false
	main_panel.visible = false
