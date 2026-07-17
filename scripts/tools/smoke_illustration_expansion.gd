extends Node

const MANIFEST_PATH := "res://data/illustration_expansion_gallery.json"
const EXPECTED_TYPES := {
	"Part II Story Moment": 5,
	"Character Bond": 5,
	"Battle Cut-in": 5,
	"World Rewrite": 5,
	"Deep Field Resonance": 5,
}
const CONSUMER_PATHS := [
	"res://data/vn_scenes/ch12_reader.json",
	"res://data/vn_scenes/ch13_third_person.json",
	"res://data/vn_scenes/ch14_confessor_intervention.json",
	"res://data/vn_scenes/ch15_singer.json",
	"res://data/vn_scenes/ch16_nera.json",
	"res://scripts/ui/story_journal.gd",
	"res://scenes/battle/battle_scene.gd",
	"res://scripts/systems/world_rewrite_director.gd",
	"res://scripts/utils/memory_resonance.gd",
]

func _ready() -> void:
	assert(FileAccess.file_exists(MANIFEST_PATH), "Illustration expansion manifest must exist")
	var parsed: Variant = JSON.parse_string(FileAccess.get_file_as_string(MANIFEST_PATH))
	assert(parsed is Dictionary, "Illustration expansion manifest must be valid JSON")
	var items: Array = parsed.get("items", [])
	assert(items.size() == 25, "Illustration expansion must contain exactly 25 curated images")

	var consumer_text := ""
	for consumer_path: String in CONSUMER_PATHS:
		assert(FileAccess.file_exists(consumer_path), "Illustration consumer must exist: %s" % consumer_path)
		consumer_text += FileAccess.get_file_as_string(consumer_path)

	var counts: Dictionary = {}
	var unique_paths: Dictionary = {}
	for item: Variant in items:
		assert(item is Dictionary, "Every gallery item must be a dictionary")
		var category := String(item.get("type", ""))
		var art_path := String(item.get("path", ""))
		assert(EXPECTED_TYPES.has(category), "Unexpected illustration category: %s" % category)
		assert(not unique_paths.has(art_path), "Illustration paths must be unique: %s" % art_path)
		assert(ResourceLoader.exists(art_path), "Illustration resource must resolve: %s" % art_path)
		assert(consumer_text.contains(art_path), "Every illustration must have a live game consumer: %s" % art_path)
		var image := Image.load_from_file(ProjectSettings.globalize_path(art_path))
		assert(image != null and not image.is_empty(), "Illustration must decode: %s" % art_path)
		var aspect := float(image.get_width()) / float(maxi(image.get_height(), 1))
		assert(aspect > 1.70 and aspect < 1.82, "Illustration must retain cinematic framing: %s" % art_path)
		unique_paths[art_path] = true
		counts[category] = int(counts.get(category, 0)) + 1

	for category: String in EXPECTED_TYPES:
		assert(int(counts.get(category, 0)) == int(EXPECTED_TYPES[category]), "%s must contain five images" % category)

	print("ILLUSTRATION_EXPANSION_SMOKE_PASS total=25 categories=5 live_consumers=25")
	get_tree().quit(0)
