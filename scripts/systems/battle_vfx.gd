## BattleVFX — S56: Advanced Battle Visual Effects
## Handles: enhanced damage numbers, skill-specific particles, status effect visuals,
## memory burn dramatic sequence, victory fanfare, defeat slow-motion.
## Used as a utility instantiated by BattleScene.
class_name BattleVFX
extends RefCounted

var _scene: Node2D  # battle_scene reference
var _canvas: Control  # canvas_root reference

## Store references for status particle overlays (keyed by "player"/"enemy" + effect)
var _status_particles: Dictionary = {}  # {"player_POISON": GPUParticles2D, ...}
var _status_icon_bars: Dictionary = {}  # {"player": HBoxContainer, "enemy": HBoxContainer}

func _init(scene: Node2D, canvas: Control) -> void:
	_scene = scene
	_canvas = canvas

## ===================== UPGRADE 1: Enhanced Damage Numbers =====================

## Floating damage number with color coding and pop/fade animation
## Colors: white=normal, yellow=critical(100+), green=heal, red=burn_cost,
##         orange=fire skill, purple=void skill, green-tint=poison DoT
func show_damage_number(target: String, amount: int, skill_name: String = "", is_burn_cost: bool = false) -> void:
	var label = Label.new()
	label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER

	var is_heal = amount < 0
	var is_miss = (amount == 0 and not is_heal)  # S59: 0 damage = MISS
	if is_miss:
		label.text = "MISS"
	elif is_heal:
		label.text = "+%d" % abs(amount)
	else:
		label.text = str(amount)

	# Font size scales with damage
	var font_size = 22
	if abs(amount) >= 200:
		font_size = 34
	elif abs(amount) >= 100:
		font_size = 30
	elif abs(amount) >= 50:
		font_size = 26
	label.add_theme_font_size_override("font_size", font_size)

	# Color coding
	var dmg_color: Color
	if is_miss:
		dmg_color = Color(0.6, 0.6, 0.6)  # S59: gray = miss
		font_size = 20
		label.add_theme_font_size_override("font_size", font_size)
	elif is_heal:
		dmg_color = Color(0.3, 1.0, 0.4)  # green = heal
	elif is_burn_cost:
		dmg_color = Color(0.9, 0.2, 0.15)  # red = burn cost
	elif target == "Arrel":
		dmg_color = Color(1.0, 0.3, 0.25)  # red = player hit
	else:
		var sn = skill_name.to_lower()
		if sn.find("burn") >= 0 or sn.find("flame") >= 0 or sn.find("ember") >= 0 or sn.find("fire") >= 0 or sn.find("scorch") >= 0 or sn.find("incinerate") >= 0 or sn.find("pyre") >= 0:
			dmg_color = Color(1.0, 0.55, 0.15)  # orange = fire
		elif sn.find("void") >= 0 or sn.find("cascade") >= 0 or sn.find("zero") >= 0:
			dmg_color = Color(0.7, 0.3, 1.0)  # purple = void
		elif sn.find("poison") >= 0:
			dmg_color = Color(0.4, 0.85, 0.3)  # green = poison DoT
		elif sn.find("combo") >= 0:
			dmg_color = Color(1.0, 0.85, 0.2)  # gold = combo
		elif abs(amount) >= 100:
			dmg_color = Color(1.0, 0.95, 0.3)  # yellow = critical/heavy
		else:
			dmg_color = Color(1.0, 1.0, 1.0)  # white = normal
	label.add_theme_color_override("font_color", dmg_color)

	# Position near target sprite (side-view layout)
	if target == "Arrel":
		label.position = Vector2(180 + randf_range(-25, 25), 270 + randf_range(-15, 15))
	else:
		label.position = Vector2(900 + randf_range(-35, 35), 250 + randf_range(-15, 15))

	# Drop shadow
	var shadow = Label.new()
	shadow.text = label.text
	shadow.add_theme_font_size_override("font_size", font_size)
	shadow.add_theme_color_override("font_color", Color(0, 0, 0, 0.7))
	shadow.position = Vector2(2, 2)
	label.add_child(shadow)

	_canvas.add_child(label)

	# Pop + float + fade animation
	label.scale = Vector2(1.5, 1.5)
	label.pivot_offset = Vector2(30, 12)
	var t = _scene.create_tween().set_parallel(true)
	# Scale punch: big -> normal
	t.tween_property(label, "scale", Vector2(1.0, 1.0), 0.2).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_BACK)
	# S59: Healing numbers float DOWN, others float UP
	if is_heal:
		t.tween_property(label, "position:y", label.position.y + 40, 1.0).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_CUBIC)
	else:
		t.tween_property(label, "position:y", label.position.y - 60, 1.0).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_CUBIC)
	# Slight horizontal drift
	t.tween_property(label, "position:x", label.position.x + randf_range(-15, 15), 1.0)
	# Fade out after lingering
	t.tween_property(label, "modulate:a", 0.0, 0.4).set_delay(0.6)
	t.chain().tween_callback(label.queue_free)

	# S59: Numbers > 200 shake while floating
	if abs(amount) > 200 and not is_heal and not is_miss:
		var shake_t = _scene.create_tween().set_loops(8)
		shake_t.tween_property(label, "position:x", label.position.x + randf_range(-4, 4), 0.06)
		shake_t.tween_property(label, "position:x", label.position.x + randf_range(-4, 4), 0.06)

## Screen flash on critical hits (brief white overlay)
func critical_screen_flash() -> void:
	var flash = ColorRect.new()
	flash.set_anchors_preset(Control.PRESET_FULL_RECT)
	flash.color = Color(1.0, 1.0, 0.95, 0.45)
	flash.mouse_filter = Control.MOUSE_FILTER_IGNORE
	flash.z_index = 95
	_canvas.add_child(flash)
	var t = _scene.create_tween()
	t.tween_property(flash, "color:a", 0.0, 0.2).set_ease(Tween.EASE_OUT)
	t.tween_callback(flash.queue_free)

## Skill-specific particle bursts by element type
func play_element_particles(element: String, target_pos: Vector2 = Vector2(920, 310)) -> void:
	match element:
		"physical":
			_spawn_element_burst(target_pos, Color(0.9, 0.9, 0.95), Color(0.7, 0.7, 0.8), 14, 60.0, 150.0)
		"fire":
			_spawn_element_burst(target_pos, Color(1.0, 0.6, 0.1), Color(1.0, 0.2, 0.0), 18, 40.0, 120.0)
			_spawn_embers(target_pos)
		"void":
			_spawn_element_burst(target_pos, Color(0.6, 0.2, 0.9), Color(0.3, 0.05, 0.5), 16, 50.0, 130.0)
			_spawn_void_wisps(target_pos)

## Internal: spawn a radial burst of colored particles
func _spawn_element_burst(center: Vector2, color1: Color, color2: Color, count: int, min_dist: float, max_dist: float) -> void:
	for i in range(count):
		var particle = ColorRect.new()
		var s = randf_range(3, 7)
		particle.size = Vector2(s, s)
		particle.position = center + Vector2(randf_range(-15, 15), randf_range(-15, 15))
		particle.z_index = 56
		particle.mouse_filter = Control.MOUSE_FILTER_IGNORE
		particle.color = color1.lerp(color2, randf())
		particle.color.a = randf_range(0.7, 1.0)
		_canvas.add_child(particle)

		var angle = randf() * TAU
		var dist = randf_range(min_dist, max_dist)
		var target_pos = particle.position + Vector2(cos(angle), sin(angle)) * dist
		var delay = randf_range(0, 0.08)
		var t = _scene.create_tween().set_parallel(true)
		t.tween_property(particle, "position", target_pos, randf_range(0.3, 0.6)).set_delay(delay).set_ease(Tween.EASE_OUT)
		t.tween_property(particle, "modulate:a", 0.0, randf_range(0.25, 0.5)).set_delay(delay + 0.15)
		t.tween_property(particle, "size", Vector2(1, 1), 0.5).set_delay(delay)
		t.chain().tween_callback(particle.queue_free)

## Fire-specific: rising ember particles
func _spawn_embers(center: Vector2) -> void:
	for i in range(8):
		var ember = ColorRect.new()
		ember.size = Vector2(randf_range(2, 5), randf_range(2, 5))
		ember.position = center + Vector2(randf_range(-30, 30), randf_range(-10, 20))
		ember.color = Color(1.0, randf_range(0.4, 0.8), 0.1, randf_range(0.6, 0.9))
		ember.z_index = 57
		ember.mouse_filter = Control.MOUSE_FILTER_IGNORE
		_canvas.add_child(ember)
		var delay = randf_range(0, 0.2)
		var t = _scene.create_tween().set_parallel(true)
		t.tween_property(ember, "position:y", ember.position.y - randf_range(50, 120), randf_range(0.6, 1.2)).set_delay(delay).set_ease(Tween.EASE_OUT)
		t.tween_property(ember, "position:x", ember.position.x + randf_range(-25, 25), 1.0).set_delay(delay)
		t.tween_property(ember, "modulate:a", 0.0, 0.5).set_delay(delay + 0.3)
		t.chain().tween_callback(ember.queue_free)

## Void-specific: drifting purple wisps
func _spawn_void_wisps(center: Vector2) -> void:
	for i in range(6):
		var wisp = ColorRect.new()
		wisp.size = Vector2(randf_range(4, 9), randf_range(4, 9))
		wisp.position = center + Vector2(randf_range(-40, 40), randf_range(-30, 30))
		wisp.color = Color(0.5, 0.15, 0.8, randf_range(0.4, 0.7))
		wisp.z_index = 57
		wisp.mouse_filter = Control.MOUSE_FILTER_IGNORE
		_canvas.add_child(wisp)
		# Wisps drift slowly in a sinusoidal path
		var delay = randf_range(0, 0.3)
		var end_x = wisp.position.x + randf_range(-60, 60)
		var end_y = wisp.position.y + randf_range(-70, -20)
		var t = _scene.create_tween().set_parallel(true)
		t.tween_property(wisp, "position", Vector2(end_x, end_y), randf_range(0.8, 1.5)).set_delay(delay).set_ease(Tween.EASE_IN_OUT).set_trans(Tween.TRANS_SINE)
		t.tween_property(wisp, "modulate:a", 0.0, 0.6).set_delay(delay + 0.5)
		t.tween_property(wisp, "size", Vector2(2, 2), 1.2).set_delay(delay)
		t.chain().tween_callback(wisp.queue_free)

## Camera shake with intensity scaling by damage
func damage_screen_shake(amount: int) -> void:
	if not OptionsMenu.settings.get("screen_shake", true):
		return
	# Scale: 0-30 = mild, 30-100 = medium, 100-200 = strong, 200+ = extreme
	var intensity = clampf(float(amount) / 50.0, 0.3, 4.0)
	var original_pos = _canvas.position
	var t = _scene.create_tween()
	var frames = int(6 + intensity * 3)
	for i in range(frames):
		var decay = 1.0 - float(i) / frames
		var offset = Vector2(
			randf_range(-8, 8) * intensity * decay,
			randf_range(-6, 6) * intensity * decay
		)
		t.tween_property(_canvas, "position", original_pos + offset, 0.025)
	t.tween_property(_canvas, "position", original_pos, 0.04)

## Victory fanfare visual: confetti particles + golden flash
func play_victory_fanfare() -> void:
	# Golden screen flash
	var flash = ColorRect.new()
	flash.set_anchors_preset(Control.PRESET_FULL_RECT)
	flash.color = Color(1.0, 0.9, 0.5, 0.5)
	flash.mouse_filter = Control.MOUSE_FILTER_IGNORE
	flash.z_index = 75
	_canvas.add_child(flash)
	var ft = _scene.create_tween()
	ft.tween_property(flash, "color:a", 0.0, 0.8).set_ease(Tween.EASE_OUT)
	ft.tween_callback(flash.queue_free)

	# Confetti burst — multi-colored particles raining down
	var confetti_colors = [
		Color(1.0, 0.85, 0.2),   # gold
		Color(0.3, 0.85, 1.0),   # cyan
		Color(1.0, 0.4, 0.5),    # pink
		Color(0.5, 1.0, 0.4),    # lime
		Color(0.9, 0.5, 1.0),    # lavender
		Color(1.0, 0.6, 0.2),    # orange
	]
	for i in range(40):
		var confetti = ColorRect.new()
		confetti.size = Vector2(randf_range(3, 8), randf_range(6, 14))
		confetti.position = Vector2(randf_range(100, 1180), randf_range(-80, -20))
		confetti.color = confetti_colors[randi_range(0, confetti_colors.size() - 1)]
		confetti.color.a = randf_range(0.7, 1.0)
		confetti.rotation = randf_range(-0.5, 0.5)
		confetti.z_index = 76
		confetti.mouse_filter = Control.MOUSE_FILTER_IGNORE
		_canvas.add_child(confetti)

		var fall_y = confetti.position.y + randf_range(500, 750)
		var drift_x = confetti.position.x + randf_range(-80, 80)
		var delay = randf_range(0, 0.5)
		var duration = randf_range(1.5, 3.0)
		var t = _scene.create_tween().set_parallel(true)
		t.tween_property(confetti, "position:y", fall_y, duration).set_delay(delay).set_ease(Tween.EASE_IN).set_trans(Tween.TRANS_QUAD)
		t.tween_property(confetti, "position:x", drift_x, duration).set_delay(delay)
		t.tween_property(confetti, "rotation", confetti.rotation + randf_range(-3.0, 3.0), duration).set_delay(delay)
		t.tween_property(confetti, "modulate:a", 0.0, 0.5).set_delay(delay + duration - 0.5)
		t.chain().tween_callback(confetti.queue_free)

	# Star sparkles at screen center
	for i in range(8):
		var star = Label.new()
		star.text = "*"
		star.add_theme_font_size_override("font_size", randi_range(18, 32))
		star.add_theme_color_override("font_color", Color(1.0, 0.9, 0.4, 0.9))
		star.position = Vector2(randf_range(300, 980), randf_range(100, 500))
		star.z_index = 77
		star.mouse_filter = Control.MOUSE_FILTER_IGNORE
		star.modulate.a = 0.0
		_canvas.add_child(star)
		var st = _scene.create_tween()
		st.tween_property(star, "modulate:a", 1.0, 0.15).set_delay(randf_range(0, 0.3))
		st.tween_property(star, "modulate:a", 0.0, 0.4).set_delay(0.2)
		st.tween_callback(star.queue_free)

## Defeat slow-motion effect: desaturate + slow + dark vignette
func play_defeat_effect() -> void:
	# Desaturation overlay (dark red-gray)
	var desat = ColorRect.new()
	desat.set_anchors_preset(Control.PRESET_FULL_RECT)
	desat.color = Color(0.15, 0.05, 0.05, 0.0)
	desat.mouse_filter = Control.MOUSE_FILTER_IGNORE
	desat.z_index = 90
	_canvas.add_child(desat)

	# Slow-motion: use Engine.time_scale
	Engine.time_scale = 0.3

	# Process-independent tween (real time) so it runs correctly during slow-mo
	var t = _scene.create_tween().set_speed_scale(1.0 / 0.3)
	# Dark overlay fade in (over ~0.6s real time)
	t.tween_property(desat, "color:a", 0.55, 0.6)
	# Hold for dramatic effect
	t.tween_interval(0.8)
	# Return to normal speed
	t.tween_callback(func(): Engine.time_scale = 1.0)
	t.tween_property(desat, "color:a", 0.0, 0.5)
	t.tween_callback(desat.queue_free)


## ===================== UPGRADE 2: Status Effect Visual Overlays =====================

## Create/update particle overlays for active status effects on a sprite
## Call this from _on_status_changed
func update_status_particles(target: String, sprite_container: Control, sprite_node: CanvasItem) -> void:
	if not sprite_container or not sprite_node:
		return

	var statuses = BattleManager.get_statuses(target)
	var has_poison = false
	var has_burn = false
	var has_weaken = false

	for entry in statuses:
		if entry.effect == BattleManager.StatusEffect.POISON:
			has_poison = true
		elif entry.effect == BattleManager.StatusEffect.BURN:
			has_burn = true
		elif entry.effect == BattleManager.StatusEffect.WEAKEN:
			has_weaken = true

	# POISON: Green drip/pulse particles
	_manage_status_particle(target, "POISON", has_poison, sprite_container, func():
		return _create_poison_particles(sprite_container.position, sprite_node.size if sprite_node is Control else Vector2(128, 128))
	)

	# BURN: Orange flickering flame particles
	_manage_status_particle(target, "BURN", has_burn, sprite_container, func():
		return _create_burn_particles(sprite_container.position, sprite_node.size if sprite_node is Control else Vector2(128, 128))
	)

	# WEAKEN: Blue desaturation tint + down arrow
	_manage_status_particle(target, "WEAKEN", has_weaken, sprite_container, func():
		return _create_weaken_overlay(sprite_container, sprite_node)
	)

func _manage_status_particle(target: String, effect_name: String, active: bool, parent: Control, create_fn: Callable) -> void:
	var key = "%s_%s" % [target, effect_name]
	if active and not _status_particles.has(key):
		var node = create_fn.call()
		if node:
			_status_particles[key] = node
	elif not active and _status_particles.has(key):
		var node = _status_particles[key]
		if is_instance_valid(node):
			node.queue_free()
		_status_particles.erase(key)

func _create_poison_particles(base_pos: Vector2, sprite_size: Vector2) -> GPUParticles2D:
	var particles = GPUParticles2D.new()
	var mat = ParticleProcessMaterial.new()
	mat.direction = Vector3(0, 1, 0)  # drip downward
	mat.spread = 20.0
	mat.initial_velocity_min = 15.0
	mat.initial_velocity_max = 40.0
	mat.gravity = Vector3(0, 60, 0)
	mat.scale_min = 1.0
	mat.scale_max = 3.0

	var gradient = GradientTexture1D.new()
	var g = Gradient.new()
	g.set_color(0, Color(0.2, 0.85, 0.3, 0.8))
	g.add_point(0.5, Color(0.15, 0.7, 0.2, 0.5))
	g.set_color(1, Color(0.1, 0.5, 0.15, 0.0))
	gradient.gradient = g
	mat.color_ramp = gradient

	mat.emission_shape = ParticleProcessMaterial.EMISSION_SHAPE_BOX
	mat.emission_box_extents = Vector3(sprite_size.x * 0.4, 5, 0)

	particles.process_material = mat
	particles.amount = 12
	particles.lifetime = 0.8
	particles.one_shot = false
	particles.position = base_pos + Vector2(sprite_size.x * 0.5, sprite_size.y * 0.3)
	particles.z_index = 44
	particles.visibility_rect = Rect2(-100, -100, 200, 200)
	_canvas.add_child(particles)
	particles.emitting = true
	return particles

func _create_burn_particles(base_pos: Vector2, sprite_size: Vector2) -> GPUParticles2D:
	var particles = GPUParticles2D.new()
	var mat = ParticleProcessMaterial.new()
	mat.direction = Vector3(0, -1, 0)  # rise upward
	mat.spread = 40.0
	mat.initial_velocity_min = 20.0
	mat.initial_velocity_max = 60.0
	mat.gravity = Vector3(0, -30, 0)
	mat.scale_min = 1.5
	mat.scale_max = 4.0

	var gradient = GradientTexture1D.new()
	var g = Gradient.new()
	g.set_color(0, Color(1.0, 0.7, 0.2, 0.9))
	g.add_point(0.3, Color(1.0, 0.4, 0.1, 0.7))
	g.add_point(0.6, Color(0.8, 0.2, 0.05, 0.4))
	g.set_color(1, Color(0.3, 0.1, 0.0, 0.0))
	gradient.gradient = g
	mat.color_ramp = gradient

	mat.emission_shape = ParticleProcessMaterial.EMISSION_SHAPE_BOX
	mat.emission_box_extents = Vector3(sprite_size.x * 0.35, sprite_size.y * 0.2, 0)

	particles.process_material = mat
	particles.amount = 16
	particles.lifetime = 0.7
	particles.one_shot = false
	particles.position = base_pos + Vector2(sprite_size.x * 0.5, sprite_size.y * 0.6)
	particles.z_index = 44
	particles.visibility_rect = Rect2(-100, -150, 200, 300)
	_canvas.add_child(particles)
	particles.emitting = true
	return particles

func _create_weaken_overlay(sprite_container: Control, sprite_node: CanvasItem) -> Control:
	# Container for weaken visual
	var overlay = Control.new()
	overlay.z_index = 44
	overlay.mouse_filter = Control.MOUSE_FILTER_IGNORE
	_canvas.add_child(overlay)

	# Apply blue desaturation tint to sprite
	sprite_node.modulate = Color(0.6, 0.6, 0.95, 1.0)

	# Down arrow icon above sprite
	var arrow = Label.new()
	arrow.text = "v"  # down arrow
	arrow.add_theme_font_size_override("font_size", 20)
	arrow.add_theme_color_override("font_color", Color(0.4, 0.4, 0.9, 0.8))
	arrow.position = sprite_container.position + Vector2(50, -20)
	arrow.z_index = 45
	arrow.mouse_filter = Control.MOUSE_FILTER_IGNORE
	overlay.add_child(arrow)

	# Pulsing arrow animation
	var t = _scene.create_tween().set_loops()
	t.tween_property(arrow, "position:y", arrow.position.y - 8, 0.5).set_ease(Tween.EASE_IN_OUT)
	t.tween_property(arrow, "position:y", arrow.position.y, 0.5).set_ease(Tween.EASE_IN_OUT)

	return overlay

## Build a status icon bar above a sprite showing active effects + remaining turns
func build_sprite_status_bar(target: String, sprite_container: Control) -> HBoxContainer:
	# Remove old bar
	var key = target
	if _status_icon_bars.has(key):
		var old = _status_icon_bars[key]
		if is_instance_valid(old):
			old.queue_free()
		_status_icon_bars.erase(key)

	var statuses = BattleManager.get_statuses(target)
	if statuses.is_empty():
		return null

	var bar = HBoxContainer.new()
	bar.add_theme_constant_override("separation", 4)
	bar.z_index = 50
	bar.mouse_filter = Control.MOUSE_FILTER_IGNORE
	# Position above sprite
	if sprite_container:
		bar.position = sprite_container.position + Vector2(10, -30)
	else:
		bar.position = Vector2(100, 200)

	for entry in statuses:
		var icon_panel = PanelContainer.new()
		var style = StyleBoxFlat.new()
		var icon_text = ""
		var icon_color = Color.WHITE

		if entry.effect == BattleManager.StatusEffect.POISON:
			icon_text = "PSN %d" % entry.turns_left
			icon_color = Color(0.3, 0.8, 0.25)
			style.bg_color = Color(0.1, 0.25, 0.1, 0.85)
			style.border_color = Color(0.3, 0.6, 0.2, 0.7)
		elif entry.effect == BattleManager.StatusEffect.BURN:
			icon_text = "BRN %d" % entry.turns_left
			icon_color = Color(1.0, 0.5, 0.15)
			style.bg_color = Color(0.25, 0.1, 0.05, 0.85)
			style.border_color = Color(0.7, 0.3, 0.1, 0.7)
		elif entry.effect == BattleManager.StatusEffect.WEAKEN:
			icon_text = "WKN %d" % entry.turns_left
			icon_color = Color(0.5, 0.5, 0.9)
			style.bg_color = Color(0.1, 0.1, 0.25, 0.85)
			style.border_color = Color(0.3, 0.3, 0.6, 0.7)

		style.set_border_width_all(1)
		style.set_corner_radius_all(3)
		style.set_content_margin_all(2)
		icon_panel.add_theme_stylebox_override("panel", style)
		icon_panel.mouse_filter = Control.MOUSE_FILTER_IGNORE

		var lbl = Label.new()
		lbl.text = icon_text
		lbl.add_theme_font_size_override("font_size", 9)
		lbl.add_theme_color_override("font_color", icon_color)
		lbl.mouse_filter = Control.MOUSE_FILTER_IGNORE
		icon_panel.add_child(lbl)
		bar.add_child(icon_panel)

	_canvas.add_child(bar)
	_status_icon_bars[key] = bar
	return bar

## Clean up all status particles (call on battle end)
func cleanup_status_particles() -> void:
	for key in _status_particles:
		var node = _status_particles[key]
		if is_instance_valid(node):
			node.queue_free()
	_status_particles.clear()
	for key in _status_icon_bars:
		var node = _status_icon_bars[key]
		if is_instance_valid(node):
			node.queue_free()
	_status_icon_bars.clear()


## ===================== UPGRADE 3: Memory Burn Dramatic Sequence =====================

## Play the dramatic memory burn sequence (~1.5-2 seconds)
## Call this BEFORE the actual damage dealing for maximum impact
func play_memory_burn_sequence(memory_title: String, memory_grade: int, player_sprite_container: Control) -> void:
	# Step 1: Freeze frame (0.3s)
	_scene.get_tree().paused = true

	# Step 2: Desaturate screen to near-grayscale
	var desat_overlay = ColorRect.new()
	desat_overlay.set_anchors_preset(Control.PRESET_FULL_RECT)
	desat_overlay.color = Color(0.0, 0.0, 0.0, 0.0)
	desat_overlay.mouse_filter = Control.MOUSE_FILTER_IGNORE
	desat_overlay.z_index = 92
	_canvas.add_child(desat_overlay)

	# Use a grayscale-ish overlay: dark blue-gray
	var t_desat = _scene.create_tween()
	t_desat.tween_property(desat_overlay, "color", Color(0.05, 0.03, 0.08, 0.6), 0.15)

	# Unfreeze after 0.3s
	await _scene.get_tree().create_timer(0.3, true, false, true).timeout
	_scene.get_tree().paused = false

	# Step 3: Blue-white fire particles from player sprite
	if player_sprite_container:
		_spawn_memory_fire(player_sprite_container.position + Vector2(60, 40))

	# Step 4: Memory name text "burns away" — letter-by-letter dissolve
	_play_burn_text(memory_title, memory_grade)

	# Step 5: Screen shake + flash (at peak of sequence)
	await _scene.get_tree().create_timer(0.5).timeout
	_screen_shake_burn()

	var burn_flash = ColorRect.new()
	burn_flash.set_anchors_preset(Control.PRESET_FULL_RECT)
	burn_flash.color = Color(0.6, 0.7, 1.0, 0.4)
	burn_flash.mouse_filter = Control.MOUSE_FILTER_IGNORE
	burn_flash.z_index = 93
	_canvas.add_child(burn_flash)
	var ft = _scene.create_tween()
	ft.tween_property(burn_flash, "color:a", 0.0, 0.3).set_ease(Tween.EASE_OUT)
	ft.tween_callback(burn_flash.queue_free)

	# Step 6: Color returns — fade out desaturation
	await _scene.get_tree().create_timer(0.4).timeout
	var t_restore = _scene.create_tween()
	t_restore.tween_property(desat_overlay, "color:a", 0.0, 0.4).set_ease(Tween.EASE_OUT)
	t_restore.tween_callback(desat_overlay.queue_free)

## Spawn blue-white fire emanating from player sprite
func _spawn_memory_fire(origin: Vector2) -> void:
	# GPU particles for the main fire
	var particles = GPUParticles2D.new()
	var mat = ParticleProcessMaterial.new()
	mat.direction = Vector3(0, -1, 0)
	mat.spread = 60.0
	mat.initial_velocity_min = 40.0
	mat.initial_velocity_max = 120.0
	mat.gravity = Vector3(0, -80, 0)
	mat.scale_min = 2.0
	mat.scale_max = 5.0
	mat.damping_min = 20.0
	mat.damping_max = 60.0

	var gradient = GradientTexture1D.new()
	var g = Gradient.new()
	g.set_color(0, Color(0.7, 0.85, 1.0, 1.0))  # bright blue-white
	g.add_point(0.2, Color(0.5, 0.6, 1.0, 0.9))  # blue
	g.add_point(0.5, Color(0.3, 0.4, 0.9, 0.6))  # deeper blue
	g.set_color(1, Color(0.2, 0.2, 0.6, 0.0))     # fade to nothing
	gradient.gradient = g
	mat.color_ramp = gradient

	mat.emission_shape = ParticleProcessMaterial.EMISSION_SHAPE_SPHERE
	mat.emission_sphere_radius = 20.0

	particles.process_material = mat
	particles.amount = 40
	particles.lifetime = 0.9
	particles.one_shot = true
	particles.explosiveness = 0.4
	particles.position = origin
	particles.z_index = 91
	particles.visibility_rect = Rect2(-200, -300, 400, 400)
	_canvas.add_child(particles)
	particles.emitting = true

	# Additional hand-drawn style particles (small rects)
	for i in range(12):
		var p = ColorRect.new()
		p.size = Vector2(randf_range(3, 8), randf_range(3, 8))
		p.position = origin + Vector2(randf_range(-25, 25), randf_range(-15, 15))
		p.color = Color(0.6, 0.75, 1.0, randf_range(0.6, 0.9))
		p.z_index = 91
		p.mouse_filter = Control.MOUSE_FILTER_IGNORE
		_canvas.add_child(p)

		var angle = randf_range(-PI * 0.8, -PI * 0.2)  # mostly upward
		var dist = randf_range(40, 120)
		var target_pos = p.position + Vector2(cos(angle), sin(angle)) * dist
		var delay = randf_range(0, 0.2)
		var t = _scene.create_tween().set_parallel(true)
		t.tween_property(p, "position", target_pos, randf_range(0.5, 1.0)).set_delay(delay).set_ease(Tween.EASE_OUT)
		t.tween_property(p, "modulate:a", 0.0, 0.4).set_delay(delay + 0.3)
		t.chain().tween_callback(p.queue_free)

	# Cleanup GPU particles after emission
	var timer = _scene.get_tree().create_timer(2.0)
	timer.timeout.connect(func():
		if is_instance_valid(particles):
			particles.queue_free()
	)

## Memory name text that "burns away" letter by letter
func _play_burn_text(memory_title: String, grade: int) -> void:
	# Grade determines color intensity
	var base_color: Color
	match grade:
		0: base_color = Color(0.7, 0.8, 0.9, 1.0)   # Grade 5: pale blue
		1: base_color = Color(0.6, 0.7, 1.0, 1.0)   # Grade 4: light blue
		2: base_color = Color(0.4, 0.55, 1.0, 1.0)  # Grade 3: blue
		3: base_color = Color(0.5, 0.3, 1.0, 1.0)   # Grade 2: blue-violet
		4: base_color = Color(0.8, 0.4, 1.0, 1.0)   # Grade 1: bright violet
		_: base_color = Color(0.6, 0.7, 1.0, 1.0)

	# Container for all letters
	var text_container = Control.new()
	text_container.z_index = 94
	text_container.mouse_filter = Control.MOUSE_FILTER_IGNORE
	_canvas.add_child(text_container)

	# Calculate total width for centering
	var font_size = 24
	var char_width = font_size * 0.55
	var total_width = memory_title.length() * char_width
	var start_x = (1280.0 - total_width) / 2.0  # center on 1280 viewport
	var y_pos = 200.0

	# Create individual letter labels
	var letter_labels: Array = []
	for i in range(memory_title.length()):
		var letter = Label.new()
		letter.text = memory_title[i]
		letter.add_theme_font_size_override("font_size", font_size)
		letter.add_theme_color_override("font_color", base_color)
		letter.position = Vector2(start_x + i * char_width, y_pos)
		letter.modulate.a = 0.0
		letter.mouse_filter = Control.MOUSE_FILTER_IGNORE

		# Shadow for readability
		var shadow = Label.new()
		shadow.text = memory_title[i]
		shadow.add_theme_font_size_override("font_size", font_size)
		shadow.add_theme_color_override("font_color", Color(0, 0, 0, 0.5))
		shadow.position = Vector2(1, 1)
		letter.add_child(shadow)

		text_container.add_child(letter)
		letter_labels.append(letter)

	# Phase A: Letters appear one by one (0.3s total)
	for i in range(letter_labels.size()):
		var letter = letter_labels[i]
		var appear_delay = float(i) / max(letter_labels.size(), 1) * 0.3
		var t = _scene.create_tween()
		t.tween_property(letter, "modulate:a", 1.0, 0.05).set_delay(appear_delay)

	# Phase B: After appearing, each letter "burns away" — dissolve from left to right
	# Start dissolve 0.4s after first appearance
	for i in range(letter_labels.size()):
		var letter = letter_labels[i]
		var dissolve_delay = 0.4 + float(i) / max(letter_labels.size(), 1) * 0.6
		var t = _scene.create_tween().set_parallel(true)
		# Color shifts to orange/red as it burns
		t.tween_property(letter, "modulate", Color(1.5, 0.5, 0.2, 1.0), 0.15).set_delay(dissolve_delay)
		# Then fade + rise
		t.tween_property(letter, "modulate:a", 0.0, 0.2).set_delay(dissolve_delay + 0.15)
		t.tween_property(letter, "position:y", letter.position.y - 15, 0.3).set_delay(dissolve_delay + 0.1)
		# Tiny spark at letter position
		t.tween_callback(func():
			_spawn_letter_spark(letter.position + Vector2(char_width / 2.0, font_size / 2.0))
		).set_delay(dissolve_delay + 0.1)

	# Clean up text container after full sequence
	var cleanup_t = _scene.create_tween()
	cleanup_t.tween_interval(1.8)
	cleanup_t.tween_callback(text_container.queue_free)

## Tiny spark when a letter burns away
func _spawn_letter_spark(pos: Vector2) -> void:
	for j in range(3):
		var spark = ColorRect.new()
		spark.size = Vector2(2, 2)
		spark.position = pos
		spark.color = Color(1.0, randf_range(0.5, 0.9), randf_range(0.1, 0.4), 0.8)
		spark.z_index = 95
		spark.mouse_filter = Control.MOUSE_FILTER_IGNORE
		_canvas.add_child(spark)
		var angle = randf() * TAU
		var dist = randf_range(10, 30)
		var target = pos + Vector2(cos(angle), sin(angle)) * dist
		var st = _scene.create_tween().set_parallel(true)
		st.tween_property(spark, "position", target, 0.3).set_ease(Tween.EASE_OUT)
		st.tween_property(spark, "modulate:a", 0.0, 0.25).set_delay(0.1)
		st.chain().tween_callback(spark.queue_free)

## ===================== S59: Critical Hit Cinematic =====================

## Play a cinematic cut-in effect for massive damage (150+)
## Screen dims, diagonal slash sweeps, camera zoom punch
func play_critical_cinematic() -> void:
	# Step 1: Screen dim to 50%
	var dim_overlay = ColorRect.new()
	dim_overlay.set_anchors_preset(Control.PRESET_FULL_RECT)
	dim_overlay.color = Color(0.0, 0.0, 0.0, 0.0)
	dim_overlay.mouse_filter = Control.MOUSE_FILTER_IGNORE
	dim_overlay.z_index = 96
	_canvas.add_child(dim_overlay)

	var dim_t = _scene.create_tween()
	dim_t.tween_property(dim_overlay, "color:a", 0.5, 0.08).set_ease(Tween.EASE_OUT)

	# Step 2: Diagonal slash line sweep (white gradient, top-right to bottom-left)
	var slash = ColorRect.new()
	slash.size = Vector2(1600, 6)
	slash.position = Vector2(-200, -100)
	slash.rotation = -0.6  # ~34 degrees diagonal
	slash.color = Color(1.0, 1.0, 1.0, 0.9)
	slash.z_index = 97
	slash.mouse_filter = Control.MOUSE_FILTER_IGNORE
	_canvas.add_child(slash)

	var slash_t = _scene.create_tween().set_parallel(true)
	slash_t.tween_property(slash, "position", Vector2(-200, 900), 0.25).set_ease(Tween.EASE_IN).set_trans(Tween.TRANS_QUAD)
	slash_t.tween_property(slash, "color:a", 0.0, 0.15).set_delay(0.12)
	slash_t.chain().tween_callback(slash.queue_free)

	# Step 3: Camera zoom in 10% then back
	var orig_scale = _canvas.scale
	var orig_pivot = _canvas.pivot_offset
	_canvas.pivot_offset = _canvas.size / 2.0 if _canvas.size.length() > 0 else Vector2(640, 360)
	var zoom_t = _scene.create_tween()
	zoom_t.tween_property(_canvas, "scale", Vector2(1.1, 1.1), 0.12).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_BACK)
	zoom_t.tween_property(_canvas, "scale", Vector2(1.0, 1.0), 0.18).set_ease(Tween.EASE_IN_OUT)
	zoom_t.tween_callback(func(): _canvas.pivot_offset = orig_pivot)

	# Step 4: Fade dim overlay back out
	await _scene.get_tree().create_timer(0.3).timeout
	var restore_t = _scene.create_tween()
	restore_t.tween_property(dim_overlay, "color:a", 0.0, 0.2).set_ease(Tween.EASE_OUT)
	restore_t.tween_callback(dim_overlay.queue_free)


## ===================== S59: Enemy Ability Warning =====================

## Show a pulsing red warning text above the enemy sprite before ability executes
func show_ability_warning(ability_name: String, enemy_pos: Vector2) -> void:
	# Map internal ability names to display names
	var display_names: Dictionary = {
		"drain": "Life Drain",
		"shield": "Dark Barrier",
		"multi_hit": "Flurry",
		"poison": "Toxic Cloud",
		"burn_attack": "Scorch",
		"weaken": "Curse",
		"summon": "Shadow Summon",
		"void_pulse": "Void Pulse",
		"despair": "Despair",
		"stun": "Stunning Blow",
		"reflect": "Mirror Barrier",
		"charge": "Charging...",
	}
	var display = display_names.get(ability_name, ability_name.capitalize())

	var warning = Label.new()
	warning.text = "!! %s !!" % display
	warning.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	warning.add_theme_font_size_override("font_size", 16)
	warning.add_theme_color_override("font_color", Color(1.0, 0.25, 0.2, 1.0))
	warning.position = enemy_pos + Vector2(-40, -45)
	warning.z_index = 85
	warning.mouse_filter = Control.MOUSE_FILTER_IGNORE
	warning.modulate.a = 0.0
	_canvas.add_child(warning)

	# Drop shadow for readability
	var shadow = Label.new()
	shadow.text = warning.text
	shadow.add_theme_font_size_override("font_size", 16)
	shadow.add_theme_color_override("font_color", Color(0, 0, 0, 0.6))
	shadow.position = Vector2(1, 1)
	warning.add_child(shadow)

	# Pulse animation: fade in, pulse red, fade out over 0.5s
	var t = _scene.create_tween()
	t.tween_property(warning, "modulate:a", 1.0, 0.08)
	t.tween_property(warning, "modulate", Color(1.5, 0.8, 0.8, 1.0), 0.12)
	t.tween_property(warning, "modulate", Color(1.0, 0.3, 0.3, 1.0), 0.12)
	t.tween_property(warning, "modulate:a", 0.0, 0.15).set_delay(0.05)
	t.tween_callback(warning.queue_free)


## ===================== S59: Battle Background Parallax Shift =====================

## Shift battle background subtly in attack direction during strikes
## direction: -1.0 = shift left (player attacking right), +1.0 = shift right (enemy attacking left)
func parallax_attack_shift(bg_node: Node, direction: float, amount: float = 8.0) -> void:
	if bg_node == null or not is_instance_valid(bg_node):
		return
	var original_x = bg_node.position.x
	var shift = amount * direction
	var t = _scene.create_tween()
	t.tween_property(bg_node, "position:x", original_x + shift, 0.1).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_QUAD)
	t.tween_property(bg_node, "position:x", original_x, 0.25).set_ease(Tween.EASE_IN_OUT).set_trans(Tween.TRANS_SINE)


## Dedicated screen shake for memory burn (stronger, more dramatic)
func _screen_shake_burn() -> void:
	if not OptionsMenu.settings.get("screen_shake", true):
		return
	var original_pos = _canvas.position
	var t = _scene.create_tween()
	for i in range(10):
		var decay = 1.0 - float(i) / 10.0
		var offset = Vector2(
			randf_range(-10, 10) * 2.0 * decay,
			randf_range(-8, 8) * 2.0 * decay
		)
		t.tween_property(_canvas, "position", original_pos + offset, 0.025)
	t.tween_property(_canvas, "position", original_pos, 0.04)
