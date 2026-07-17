extends Node

const MANIFEST_PATH := "res://data/chapter_expansion_gallery.json"

func _ready() -> void:
	assert(FileAccess.file_exists(MANIFEST_PATH), "Chapter expansion manifest must exist")
	var parsed: Variant = JSON.parse_string(FileAccess.get_file_as_string(MANIFEST_PATH))
	assert(parsed is Dictionary, "Chapter expansion manifest must be a dictionary")
	var items: Array = parsed.get("items", [])
	assert(items.size() == 50, "Chapter expansion must contain exactly 50 CG entries")

	var chapter_counts: Dictionary = {}
	var seen_paths: Dictionary = {}
	var dialogue_cache: Dictionary = {}
	for raw_item: Variant in items:
		assert(raw_item is Dictionary, "Every chapter expansion entry must be a dictionary")
		var item: Dictionary = raw_item
		var chapter := int(item.get("chapter", 0))
		var art_path := String(item.get("path", ""))
		var dialogue_id := String(item.get("dialogue_id", ""))
		var anchor_text := String(item.get("anchor_text", ""))
		chapter_counts[chapter] = int(chapter_counts.get(chapter, 0)) + 1
		assert(not seen_paths.has(art_path), "Chapter expansion art paths must be unique: %s" % art_path)
		seen_paths[art_path] = true
		assert(ResourceLoader.exists(art_path), "Chapter expansion CG must resolve: %s" % art_path)
		var texture := load(art_path) as Texture2D
		assert(texture != null, "Chapter expansion CG must load as Texture2D: %s" % art_path)
		assert(texture.get_width() >= 1600 and texture.get_height() >= 900, "Chapter expansion CG must remain presentation resolution: %s" % art_path)
		var aspect := float(texture.get_width()) / float(texture.get_height())
		assert(aspect > 1.70 and aspect < 1.82, "Chapter expansion CG must remain 16:9: %s" % art_path)

		if not dialogue_cache.has(chapter):
			var dialogue_path := "res://data/chapter%d_dialogue.json" % chapter
			var chapter_data: Variant = JSON.parse_string(FileAccess.get_file_as_string(dialogue_path))
			assert(chapter_data is Dictionary, "Chapter dialogue must parse: %s" % dialogue_path)
			dialogue_cache[chapter] = chapter_data
		var dialogues: Dictionary = dialogue_cache[chapter].get("dialogues", {})
		assert(dialogues.has(dialogue_id), "Placed CG dialogue id must exist: %s" % dialogue_id)
		var placement_count := 0
		for raw_line: Variant in dialogues[dialogue_id]:
			if raw_line is Dictionary:
				var line: Dictionary = raw_line
				if String(line.get("text", "")) == anchor_text and String(line.get("cg", "")) == art_path:
					placement_count += 1
		assert(placement_count == 1, "Each expansion CG must be placed exactly once: %s" % art_path)

	for chapter in range(1, 11):
		assert(int(chapter_counts.get(chapter, 0)) == 5, "Each chapter must contain exactly five expansion CGs: chapter %d" % chapter)

	var artbook_items: Array = PauseMenu.call("_load_artbook_items")
	var artbook_paths: Dictionary = {}
	for raw_item: Variant in artbook_items:
		if raw_item is Dictionary:
			artbook_paths[String(raw_item.get("path", ""))] = true
	for art_path: String in seen_paths:
		assert(artbook_paths.has(art_path), "Every expansion CG must be exposed by the Artbook: %s" % art_path)

	print("CHAPTER_EXPANSION_SMOKE_PASS chapters=10 assets=%d placed=%d" % [seen_paths.size(), items.size()])
	get_tree().quit(0)
