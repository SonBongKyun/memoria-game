## WorldCache is a one-time, story-located item find. It keeps the new atlas
## rewards in the field rather than turning the expansion into shop-only data.
class_name WorldCache
extends StaticBody2D

@export var map_id: String = ""
@export var cache_id: String = ""
@export var item_id: String = ""
@export var quantity: int = 1

var _claimed := false

func _ready() -> void:
	_claimed = GameManager.get_flag(_claim_flag())
	if _claimed or not GameManager.ITEMS.has(item_id):
		queue_free()
		return
	collision_layer = 12
	collision_mask = 0
	_add_visuals()

func interact() -> void:
	if _claimed or not GameManager.ITEMS.has(item_id):
		return
	_claimed = true
	GameManager.set_flag(_claim_flag())
	GameManager.add_item(item_id, maxi(quantity, 1))
	AudioManager.play_sfx("ui_select")
	queue_free()

func _claim_flag() -> String:
	return "world_cache_%s_%s" % [map_id, cache_id]

func _add_visuals() -> void:
	var collider := CollisionShape2D.new()
	var shape := CircleShape2D.new()
	shape.radius = 16.0
	collider.shape = shape
	collider.position = Vector2(0, 2)
	add_child(collider)
	var shadow := Polygon2D.new()
	shadow.polygon = PackedVector2Array([Vector2(-15, 8), Vector2(-7, 5), Vector2(7, 5), Vector2(15, 8), Vector2(7, 11), Vector2(-7, 11)])
	shadow.color = Color(0.0, 0.0, 0.0, 0.34)
	shadow.z_index = -1
	add_child(shadow)
	var frame := Polygon2D.new()
	frame.polygon = PackedVector2Array([Vector2(0, -20), Vector2(16, -4), Vector2(0, 13), Vector2(-16, -4)])
	frame.color = Color(0.11, 0.09, 0.14, 0.94)
	frame.z_index = 1
	add_child(frame)
	var icon := Sprite2D.new()
	icon.name = "ItemIcon"
	icon.texture = GameManager.get_item_icon(item_id)
	icon.position = Vector2(0, -5)
	icon.scale = Vector2(0.14, 0.14)
	icon.texture_filter = CanvasItem.TEXTURE_FILTER_NEAREST
	icon.z_index = 2
	add_child(icon)
	var glint := Line2D.new()
	glint.points = PackedVector2Array([Vector2(-11, -5), Vector2(0, -16), Vector2(11, -5)])
	glint.width = 1.2
	glint.default_color = Color(0.90, 0.76, 0.38, 0.82)
	glint.z_index = 3
	add_child(glint)
