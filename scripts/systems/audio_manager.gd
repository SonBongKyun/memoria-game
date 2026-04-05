## AudioManager (Autoload)
## BGM 재생/전환, SFX 재생. 씬별 자동 BGM 매핑.
extends Node

var bgm_player: AudioStreamPlayer
var sfx_player: AudioStreamPlayer
var current_bgm: String = ""
var bgm_tween: Tween

# 씬 경로 → BGM 매핑
const SCENE_BGM: Dictionary = {
	"res://scenes/main/main.tscn": "res://assets/audio/bgm/title.mp3",
	"res://scenes/maps/rim_forest.tscn": "res://assets/audio/bgm/ch1_forest.mp3",
	"res://scenes/maps/verdan_market.tscn": "res://assets/audio/bgm/ch2_verdan.mp3",
	"res://scenes/battle/battle_scene.tscn": "res://assets/audio/bgm/battle.mp3",
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
	var samples = _generate_sfx(type)
	if samples.is_empty():
		return
	var stream = _samples_to_stream(samples)
	sfx_player.stream = stream
	sfx_player.play()

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
		"step":  # 발걸음
			duration = 0.06
			for i in range(int(sample_rate * duration)):
				var t = float(i) / sample_rate
				samples.append(randf_range(-0.15, 0.15) * (1.0 - t / duration))

	return samples

func _samples_to_stream(samples: PackedFloat32Array) -> AudioStreamWAV:
	var stream = AudioStreamWAV.new()
	stream.format = AudioStreamWAV.FORMAT_IMA_ADPCM
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
	if scene and scene.scene_file_path != "":
		var path = scene.scene_file_path
		if SCENE_BGM.has(path):
			play_bgm(SCENE_BGM[path])
