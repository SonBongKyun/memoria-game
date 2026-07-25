extends Node

const MANIFEST_PATH := "res://data/illustration_gapfill_gallery.json"
const EXPECTED_TYPES := [
	"Late Story Interstitial",
	"Testimony Epilogue",
	"Deep Field Resonance",
	"Enemy Action Cut-in",
	"World Rewrite",
]

func _ready() -> void:
	assert(FileAccess.file_exists(MANIFEST_PATH), "Illustration gap-fill manifest must exist")
	var parsed: Variant = JSON.parse_string(FileAccess.get_file_as_string(MANIFEST_PATH))
	assert(parsed is Dictionary, "Illustration gap-fill manifest must parse")
	var items: Array = (parsed as Dictionary).get("items", [])
	assert(items.size() == 25, "Gap-fill collection should contain five deliberate sets of five")

	var category_counts: Dictionary = {}
	var seen_paths: Dictionary = {}
	var live_sources := ""
	for source_path: String in [
		"res://data/vn_scenes/ch17_forgetting_storm.json",
		"res://data/vn_scenes/ch18_living_funeral.json",
		"res://data/vn_scenes/ch19_approach.json",
		"res://data/vn_scenes/ch20_monolith.json",
		"res://data/vn_scenes/ch21_editors_turn.json",
		"res://data/vn_scenes/ch24_testimony.json",
		"res://scripts/utils/memory_resonance.gd",
		"res://scripts/utils/world_population.gd",
		"res://scripts/systems/world_rewrite_director.gd",
		"res://scenes/battle/battle_scene.gd",
	]:
		live_sources += FileAccess.get_file_as_string(source_path)

	for item_variant: Variant in items:
		assert(item_variant is Dictionary, "Every gallery entry must be a dictionary")
		var item: Dictionary = item_variant
		var type_name := String(item.get("type", ""))
		var path := String(item.get("path", ""))
		assert(EXPECTED_TYPES.has(type_name), "Unexpected gap-fill category: %s" % type_name)
		category_counts[type_name] = int(category_counts.get(type_name, 0)) + 1
		assert(path != "" and not seen_paths.has(path), "Every generated illustration path must be unique")
		seen_paths[path] = true
		assert(ResourceLoader.exists(path), "Generated illustration must import: %s" % path)
		var texture := load(path) as Texture2D
		assert(texture != null and texture.get_width() > 0 and texture.get_height() > 0, "Generated illustration must decode: %s" % path)
		var aspect := float(texture.get_width()) / float(texture.get_height())
		assert(aspect >= 1.70 and aspect <= 1.82, "Generated story and field art must retain a cinematic 16:9 crop: %s" % path)
		assert(live_sources.contains(path), "Generated art must have a live story, battle, exploration, or rewrite consumer: %s" % path)

	for type_name: String in EXPECTED_TYPES:
		assert(int(category_counts.get(type_name, 0)) == 5, "%s should contain exactly five illustrations" % type_name)
	assert(PauseMenu.ILLUSTRATION_GAPFILL_GALLERY_PATH == MANIFEST_PATH, "Pause artbook must load the new collection")
	print("ILLUSTRATION_GAPFILL_SMOKE_PASS total=25 categories=5 live_consumers=25 aspect=16:9")
	get_tree().quit(0)
