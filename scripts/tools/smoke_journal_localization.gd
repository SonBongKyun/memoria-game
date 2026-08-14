extends Node

## S247: 저널 항목이 한국어 없이 나가는 것을 막는다.
##
## S244에서 65개 항목을 옮겼지만, 읽는 쪽(_field)이 `_ko`가 없으면 조용히 영어로
## 되돌아간다. 그 폴백은 화면이 비지 않게 하려고 일부러 둔 것이고 유지할 값이
## 있다. 다만 그 탓에 새 항목에 번역을 빠뜨려도 아무도 알려 주지 않는다.
##
## 그래서 검사를 스모크에 둔다. 앞으로 항목을 추가하면서 `_ko`를 잊으면 여기서 걸린다.

func _ready() -> void:
	var missing: Array[String] = []
	for entry in StoryJournal.EVENT_ENTRIES:
		_require(entry, ["title", "desc"], missing, "EVENT")
	for entry in StoryJournal.NPC_ENTRIES:
		_require(entry, ["role", "desc"], missing, "NPC")
	for entry in StoryJournal.WORLD_ENTRIES:
		_require(entry, ["title", "desc"], missing, "WORLD")

	for line in missing:
		print("JOURNAL_LOCALE_MISSING %s" % line)
	assert(missing.is_empty(), "한국어가 빠진 저널 항목 %d개" % missing.size())
	print("JOURNAL_LOCALIZATION_SMOKE_PASS entries=%d" % [
		StoryJournal.EVENT_ENTRIES.size() + StoryJournal.NPC_ENTRIES.size() + StoryJournal.WORLD_ENTRIES.size()])
	get_tree().quit(0)

func _require(entry: Dictionary, keys: Array, missing: Array[String], kind: String) -> void:
	var label := String(entry.get("title", entry.get("name", "?")))
	for key in keys:
		if String(entry.get(key, "")) == "":
			continue
		if String(entry.get(String(key) + "_ko", "")) == "":
			missing.append("%s %s.%s_ko" % [kind, label, key])
