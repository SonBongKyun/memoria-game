## MapGateway — a compact interactable route marker for optional exploration sites.
class_name MapGateway
extends StaticBody2D

@export_file("*.tscn") var destination_scene: String = ""
@export var label_en: String = "Explore"
@export var label_ko: String = "탐색"
@export var accent: Color = Color(0.72, 0.62, 0.34, 1.0)

var _busy := false

func _ready() -> void:
	collision_layer = 12
	collision_mask = 0
	var collider := CollisionShape2D.new()
	var shape := RectangleShape2D.new()
	shape.size = Vector2(24, 18)
	collider.position = Vector2(0, 4)
	collider.shape = shape
	add_child(collider)
	var shadow := Polygon2D.new()
	shadow.polygon = PackedVector2Array([Vector2(-15, 10), Vector2(-8, 7), Vector2(8, 7), Vector2(15, 10), Vector2(8, 13), Vector2(-8, 13)])
	shadow.color = Color(0.0, 0.0, 0.0, 0.34)
	shadow.z_index = -1
	add_child(shadow)
	var arch := Line2D.new()
	arch.width = 2.0
	arch.default_color = accent
	arch.points = PackedVector2Array([Vector2(-10, 8), Vector2(-10, -4), Vector2(-5, -11), Vector2(0, -14), Vector2(5, -11), Vector2(10, -4), Vector2(10, 8)])
	arch.z_index = 2
	add_child(arch)
	var core := Polygon2D.new()
	core.polygon = PackedVector2Array([Vector2(0, -10), Vector2(5, -4), Vector2(0, 2), Vector2(-5, -4)])
	core.color = Color(accent.r, accent.g, accent.b, 0.82)
	core.z_index = 3
	add_child(core)
	var marker := Label.new()
	marker.text = "◆"
	marker.add_theme_font_override("font", UITheme.make_ui_font())
	marker.add_theme_font_size_override("font_size", 12)
	marker.add_theme_color_override("font_color", Color(accent.r, accent.g, accent.b, 0.82))
	marker.add_theme_color_override("font_shadow_color", Color(0, 0, 0, 0.9))
	marker.add_theme_constant_override("shadow_offset_x", 1)
	marker.add_theme_constant_override("shadow_offset_y", 1)
	marker.position = Vector2(-6, -33)
	marker.z_index = 4
	add_child(marker)

func interact() -> void:
	if _busy or destination_scene == "" or not ResourceLoader.exists(destination_scene):
		return
	_busy = true
	var label := label_ko if GameManager.current_locale == "ko" else label_en
	NotificationToast.show_toast(label, NotificationToast.ToastType.INFO)
	SceneTransition.change_scene_styled(destination_scene)
