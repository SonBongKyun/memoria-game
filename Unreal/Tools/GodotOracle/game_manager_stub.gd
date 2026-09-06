extends Node
# Oracle environment only. No production profile, autosave, UI, or listeners.
var current_chapter: int = 1
var current_locale: String = "en"
var player_data: Dictionary = {"elia_with_party": true, "grains": 0}
var story_flags: Dictionary = {}
func add_stat(_name: String, _amount: int = 1) -> void:
	pass
func get_flag(key: String) -> bool:
	return bool(story_flags.get(key, false))
func set_flag(key: String, value: bool = true) -> void:
	story_flags[key] = value
