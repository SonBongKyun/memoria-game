extends Node

func _ready() -> void:
	assert(SideQuest.QUESTS.size() == 6, "The authored side-quest dossier should cover all six quests")
	var seen_paths: Dictionary = {}
	for quest in SideQuest.QUESTS:
		var art_path := String(quest.get("art", ""))
		assert(art_path != "", "Every side quest needs a dossier illustration")
		assert(ResourceLoader.exists(art_path), "Quest illustration path must resolve: %s" % art_path)
		assert(not seen_paths.has(art_path), "Quest dossier art must be distinctive: %s" % art_path)
		seen_paths[art_path] = true

	for quest_data in SideQuest.get_all_quests():
		assert(quest_data.has("art"), "Journal quest data must expose the dossier illustration")

	print("QUEST_ILLUSTRATION_SMOKE_PASS quests=%d unique_art=%d" % [SideQuest.QUESTS.size(), seen_paths.size()])
	get_tree().quit(0)
