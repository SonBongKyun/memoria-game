## S218: 개발자 도감에 쌓인 테스트 항목을 정리한다.
## 자동으로 돌지 않는다. 이 씬을 직접 실행할 때만 동작하며, 지운 항목을 모두 출력한다.
extends Node

func _ready() -> void:
	var before := Codex.enemy_entries.size()
	var removed := Codex.purge_unknown_entries()
	print("CODEX_PURGE before=%d after=%d removed=%d" % [before, Codex.enemy_entries.size(), removed.size()])
	for entry_name: String in removed:
		print("   - %s" % entry_name)
	get_tree().quit(0)
