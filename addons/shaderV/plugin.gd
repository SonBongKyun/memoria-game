@tool
extends EditorPlugin

func _get_plugin_name() -> String:
	return "ShaderV"

func _enter_tree() -> void:
	print("[ShaderV] Plugin loaded")

func _exit_tree() -> void:
	pass
