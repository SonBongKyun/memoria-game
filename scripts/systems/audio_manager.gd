## AudioManager (Autoload)
## BGM 재생/전환, SFX 재생. 씬별 자동 BGM 매핑.
extends Node

var bgm_player: AudioStreamPlayer
var sfx_player: AudioStreamPlayer
var step_player: AudioStreamPlayer
var current_bgm: String = ""
var bgm_tween: Tween
var _last_scene_path: String = ""

# 씬 경로 → BGM 매핑
const SCENE_BGM: Dictionary = {
	"res://scenes/main/main.tscn": "res://assets/audio/bgm/title.mp3",
	"res://scenes/maps/rim_forest.tscn": "res://assets/audio/bgm/ch1_forest.mp3",
	"res://scenes/maps/verdan_market.tscn": "res://assets/audio/bgm/ch2_verdan.mp3",
	"res://scenes/maps/crumbling_coast.tscn": "res://assets/audio/bgm/dialogue_tense.mp3",
	"res://scenes/maps/the_seam.tscn": "res://assets/audio/bgm/exploration.mp3",
	"res://scenes/maps/bl07_void.tscn": "res://assets/audio/bgm/ch5_void.mp3",
}

const FADE_DURATION: float = 1.0

func _ready() -> void:
	# BGM 플레이어
	bgm_player = AudioStreamPlayer.new()
	bgm_player.bus = "Master"
	bgm_player.volume_db = -5.0
	add_child(bgm_player)

	# SFX 플레이어
	sfx_player = AudioStreamPlayer.new()
	sfx_player.bus = "Master"
	sfx_player.volume_db = -3.0
	add_child(sfx_player)

	# 발걸음 전용 플레이어 (낮은 볼륨, SFX와 겹치지 않음)
	step_player = AudioStreamPlayer.new()
	step_player.bus = "Master"
	step_player.volume_db = -12.0
	add_child(step_player)

	# 씬 전환 감지
	get_tree().tree_changed.connect(_on_tree_changed)
	print("[AudioManager] Ready")

## BGM 재생 (페이드 인)
func play_bgm(path: String, fade: bool = true) -> void:
	if path == current_bgm and bgm_player.playing:
		return
	if not ResourceLoader.exists(path):
		push_warning("[AudioManager] BGM not found: %s" % path)
		return

	if bgm_player.playing and fade:
		# 페이드 아웃 → 교체 → 페이드 인
		if bgm_tween:
			bgm_tween.kill()
		bgm_tween = create_tween()
		bgm_tween.tween_property(bgm_player, "volume_db", -40.0, FADE_DURATION * 0.6)
		bgm_tween.tween_callback(func():
			_start_bgm(path)
			bgm_tween = create_tween()
			bgm_tween.tween_property(bgm_player, "volume_db", -5.0, FADE_DURATION * 0.4)
		)
	else:
		_start_bgm(path)

func _start_bgm(path: String) -> void:
	var stream = load(path)
	if stream:
		bgm_player.stream = stream
		bgm_player.volume_db = -5.0
		bgm_player.play()
		current_bgm = path

## BGM 정지 (페이드 아웃)
func stop_bgm(fade: bool = true) -> void:
	if not bgm_player.playing:
		return
	if fade:
		if bgm_tween:
			bgm_tween.kill()
		bgm_tween = create_tween()
		bgm_tween.tween_property(bgm_player, "volume_db", -40.0, FADE_DURATION)
		bgm_tween.tween_callback(func():
			bgm_player.stop()
			current_bgm = ""
		)
	else:
		bgm_player.stop()
		current_bgm = ""

## SFX 재생 (코드 생성 — 외부 파일 불필요)
func play_sfx(type: String) -> void:
	if not sfx_player:
		return
	var samples = _generate_sfx(type)
	if samples.is_empty():
		return
	var stream = _samples_to_stream(samples)
	sfx_player.stream = stream
	sfx_player.play()

## 발걸음 전용 재생 (step_player 사용, 다른 SFX와 겹치지 않음)
## S41: 지형별 발걸음 SFX
func play_step(terrain: String = "grass") -> void:
	if not step_player:
		return
	var sfx_type = "step"
	match terrain:
		"sand": sfx_type = "step_sand"
		"stone": sfx_type = "step_stone"
		"water": sfx_type = "step_water"
	var samples = _generate_sfx(sfx_type)
	if samples.is_empty():
		return
	var stream = _samples_to_stream(samples)
	step_player.stream = stream
	step_player.play()

## 간단한 SFX 생성 (사인파 기반)
func _generate_sfx(type: String) -> PackedFloat32Array:
	var samples = PackedFloat32Array()
	var sample_rate = 22050
	var duration = 0.0
	var freq = 440.0

	match type:
		"confirm":  # UI 확인음
			duration = 0.1
			for i in range(int(sample_rate * duration)):
				var t = float(i) / sample_rate
				samples.append(sin(t * 880.0 * TAU) * 0.3 * (1.0 - t / duration))
		"cancel":  # UI 취소음
			duration = 0.15
			for i in range(int(sample_rate * duration)):
				var t = float(i) / sample_rate
				samples.append(sin(t * 330.0 * TAU) * 0.25 * (1.0 - t / duration))
		"burn":  # 기억 연소 — 불타는 느낌
			duration = 0.4
			for i in range(int(sample_rate * duration)):
				var t = float(i) / sample_rate
				var env = (1.0 - t / duration)
				var noise = randf_range(-0.15, 0.15)
				samples.append((sin(t * 220.0 * TAU) * 0.2 + noise) * env)
		"hit":  # 전투 타격
			duration = 0.12
			for i in range(int(sample_rate * duration)):
				var t = float(i) / sample_rate
				var env = (1.0 - t / duration) * (1.0 - t / duration)
				samples.append(randf_range(-0.4, 0.4) * env)
		"heal":  # 회복음
			duration = 0.3
			for i in range(int(sample_rate * duration)):
				var t = float(i) / sample_rate
				var env = sin(t / duration * PI)
				samples.append(sin(t * 660.0 * TAU) * 0.15 * env + sin(t * 990.0 * TAU) * 0.1 * env)
		"step":  # 발걸음 (풀)
			duration = 0.06
			for i in range(int(sample_rate * duration)):
				var t = float(i) / sample_rate
				samples.append(randf_range(-0.15, 0.15) * (1.0 - t / duration))
		"step_sand":  # 모래 발걸음 — 부드럽고 길게
			duration = 0.09
			for i in range(int(sample_rate * duration)):
				var t = float(i) / sample_rate
				samples.append(randf_range(-0.1, 0.1) * (1.0 - t / duration) * 0.8)
		"step_stone":  # 돌 발걸음 — 날카롭고 짧게
			duration = 0.05
			for i in range(int(sample_rate * duration)):
				var t = float(i) / sample_rate
				var env = (1.0 - t / duration) * (1.0 - t / duration)
				samples.append((sin(t * 1200.0 * TAU) * 0.08 + randf_range(-0.12, 0.12)) * env)
		"step_water":  # 물 발걸음 — 스플래시
			duration = 0.1
			for i in range(int(sample_rate * duration)):
				var t = float(i) / sample_rate
				var env = sin(t / duration * PI) * 0.7
				samples.append((randf_range(-0.2, 0.2) + sin(t * 300.0 * TAU) * 0.05) * env)
		"shield":  # 적 방어막 — 저음 울림
			duration = 0.35
			for i in range(int(sample_rate * duration)):
				var t = float(i) / sample_rate
				var env = sin(t / duration * PI) * 0.8
				samples.append((sin(t * 150.0 * TAU) * 0.2 + sin(t * 75.0 * TAU) * 0.15) * env)
		"drain":  # 생명력 흡수 — 역방향 스윕
			duration = 0.35
			for i in range(int(sample_rate * duration)):
				var t = float(i) / sample_rate
				var env = t / duration  # 역페이드 (점점 커짐)
				var f = lerpf(600.0, 150.0, t / duration)
				samples.append(sin(t * f * TAU) * 0.25 * env)
		"phase_change":  # 보스 페이즈 전환 — 깊은 공명
			duration = 0.6
			for i in range(int(sample_rate * duration)):
				var t = float(i) / sample_rate
				var env = sin(t / duration * PI)
				var wave = sin(t * 110.0 * TAU) * 0.2 + sin(t * 55.0 * TAU) * 0.15
				var noise = randf_range(-0.05, 0.05)
				samples.append((wave + noise) * env)
		"defeat":  # 패배 — 하강하는 톤
			duration = 0.5
			for i in range(int(sample_rate * duration)):
				var t = float(i) / sample_rate
				var env = (1.0 - t / duration)
				var f = lerpf(440.0, 110.0, t / duration)
				samples.append(sin(t * f * TAU) * 0.2 * env)
		"flee":  # 도주 — 빠른 상승음
			duration = 0.2
			for i in range(int(sample_rate * duration)):
				var t = float(i) / sample_rate
				var env = (1.0 - t / duration)
				var f = lerpf(330.0, 880.0, t / duration)
				samples.append(sin(t * f * TAU) * 0.25 * env)
		"memory_add":  # 기억 획득 — 맑은 화음
			duration = 0.4
			for i in range(int(sample_rate * duration)):
				var t = float(i) / sample_rate
				var env = sin(t / duration * PI) * 0.8
				samples.append((sin(t * 523.0 * TAU) * 0.12 + sin(t * 659.0 * TAU) * 0.1 + sin(t * 784.0 * TAU) * 0.08) * env)
		"void_pulse":  # 보이드 맥동 — 불안한 저주파
			duration = 0.5
			for i in range(int(sample_rate * duration)):
				var t = float(i) / sample_rate
				var env = sin(t / duration * PI)
				var wave = sin(t * 45.0 * TAU) * 0.3
				var mod = sin(t * 7.0 * TAU) * 0.1
				samples.append((wave + mod) * env)
		"ui_hover":  # UI 호버 — 짧은 고음 틱
			duration = 0.04
			for i in range(int(sample_rate * duration)):
				var t = float(i) / sample_rate
				var env = (1.0 - t / duration)
				samples.append(sin(t * 1200.0 * TAU) * 0.12 * env)
		"ui_select":  # UI 선택 — 명확한 확인음
			duration = 0.08
			for i in range(int(sample_rate * duration)):
				var t = float(i) / sample_rate
				var env = (1.0 - t / duration) * (1.0 - t / duration)
				samples.append((sin(t * 880.0 * TAU) * 0.2 + sin(t * 1320.0 * TAU) * 0.1) * env)
		"ui_open":  # 메뉴 열기 — 상승 스윕
			duration = 0.12
			for i in range(int(sample_rate * duration)):
				var t = float(i) / sample_rate
				var env = sin(t / duration * PI) * 0.8
				var f = lerpf(400.0, 900.0, t / duration)
				samples.append(sin(t * f * TAU) * 0.15 * env)
		"ui_close":  # 메뉴 닫기 — 하강 스윕
			duration = 0.1
			for i in range(int(sample_rate * duration)):
				var t = float(i) / sample_rate
				var env = (1.0 - t / duration)
				var f = lerpf(800.0, 350.0, t / duration)
				samples.append(sin(t * f * TAU) * 0.12 * env)
		"battle_intro":  # 전투 진입 — 긴장 저음
			duration = 0.6
			for i in range(int(sample_rate * duration)):
				var t = float(i) / sample_rate
				var env = sin(t / duration * PI) * 0.7
				var wave = sin(t * 80.0 * TAU) * 0.2 + sin(t * 120.0 * TAU) * 0.15
				samples.append(wave * env)

	return samples

func _samples_to_stream(samples: PackedFloat32Array) -> AudioStreamWAV:
	var stream = AudioStreamWAV.new()
	# float → 16bit PCM
	var byte_data = PackedByteArray()
	for s in samples:
		var val = int(clampf(s, -1.0, 1.0) * 32767)
		byte_data.append(val & 0xFF)
		byte_data.append((val >> 8) & 0xFF)
	stream.data = byte_data
	stream.format = AudioStreamWAV.FORMAT_16_BITS
	stream.mix_rate = 22050
	stream.stereo = false
	return stream

## 씬 전환 시 자동 BGM 교체
func _on_tree_changed() -> void:
	var scene = get_tree().current_scene
	if not scene or scene.scene_file_path == "":
		return
	var path = scene.scene_file_path
	if path == _last_scene_path:
		return
	_last_scene_path = path
	if path == "res://scenes/battle/battle_scene.tscn":
		play_bgm("res://assets/audio/bgm/battle_theme.mp3")
	elif SCENE_BGM.has(path):
		play_bgm(SCENE_BGM[path])
