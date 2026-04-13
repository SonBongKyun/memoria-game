## AudioManager (Autoload)
## BGM 재생/전환, SFX 재생, 앰비언트 루프, 오디오 덕킹. 씬별 자동 BGM 매핑.
extends Node

var bgm_player: AudioStreamPlayer
var bgm_player_b: AudioStreamPlayer  # S57: 크로스페이드용 세컨드 BGM 플레이어
var sfx_player: AudioStreamPlayer
var step_player: AudioStreamPlayer
var ambient_player: AudioStreamPlayer  # S57: 앰비언트 사운드 루프
var heartbeat_player: AudioStreamPlayer  # S57: 보스/위기 하트비트 레이어
var current_bgm: String = ""
var current_ambient: String = ""
var bgm_tween: Tween
var _last_scene_path: String = ""
var _active_bgm_player: AudioStreamPlayer  # 현재 활성 BGM 플레이어 (A/B 교대)
var _bgm_base_volume: float = -5.0
var _bgm_ducked: bool = false  # 대화 중 BGM 덕킹 상태
var _is_boss_fight: bool = false
var _heartbeat_active: bool = false
var _intensity_tween: Tween

# 씬 경로 → BGM 매핑
const SCENE_BGM: Dictionary = {
	"res://scenes/main/main.tscn": "res://assets/audio/bgm/title.mp3",
	"res://scenes/maps/rim_forest.tscn": "res://assets/audio/bgm/ch1_forest.mp3",
	"res://scenes/maps/verdan_market.tscn": "res://assets/audio/bgm/ch2_verdan.mp3",
	"res://scenes/maps/crumbling_coast.tscn": "res://assets/audio/bgm/dialogue_tense.mp3",
	"res://scenes/maps/the_seam.tscn": "res://assets/audio/bgm/exploration.mp3",
	"res://scenes/maps/bl07_void.tscn": "res://assets/audio/bgm/ch5_void.mp3",
}

# S57: 씬 경로 → 앰비언트 사운드 매핑
const SCENE_AMBIENT: Dictionary = {
	"res://scenes/maps/rim_forest.tscn": "wind_forest",
	"res://scenes/maps/verdan_market.tscn": "wind_light",
	"res://scenes/maps/crumbling_coast.tscn": "wind_heavy",
	"res://scenes/maps/the_seam.tscn": "wind_heavy",
	"res://scenes/maps/bl07_void.tscn": "void_hum",
	"res://scenes/maps/belt_waystation.tscn": "wind_light",
	"res://scenes/maps/drift_shelter.tscn": "rain",
	"res://scenes/maps/seam_outskirts.tscn": "wind_heavy",
	"res://scenes/maps/forgotten_forest.tscn": "wind_forest",
	"res://scenes/maps/colorless_waste.tscn": "void_hum",
}

# S57: SFX 피치 변주 대상 (자주 쓰는 사운드)
const SFX_PITCH_VARIATION: Dictionary = {
	"hit": 0.1,
	"confirm": 0.08,
	"step": 0.12,
	"step_sand": 0.1,
	"step_stone": 0.1,
	"step_water": 0.1,
	"ui_hover": 0.06,
	"ui_select": 0.08,
}

const FADE_DURATION: float = 1.0
const CROSSFADE_DURATION: float = 0.8  # S57: BGM 크로스페이드 시간
const DUCK_VOLUME: float = -13.0  # S57: 대화 중 BGM 덕킹 볼륨 (~40%)
const DUCK_FADE: float = 0.3  # 덕킹 페이드 시간

func _ready() -> void:
	# BGM 플레이어 A (주 플레이어)
	bgm_player = AudioStreamPlayer.new()
	bgm_player.bus = "Master"
	bgm_player.volume_db = _bgm_base_volume
	add_child(bgm_player)
	_active_bgm_player = bgm_player

	# BGM 플레이어 B (크로스페이드용)
	bgm_player_b = AudioStreamPlayer.new()
	bgm_player_b.bus = "Master"
	bgm_player_b.volume_db = -40.0
	add_child(bgm_player_b)

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

	# S57: 앰비언트 사운드 플레이어
	ambient_player = AudioStreamPlayer.new()
	ambient_player.bus = "Master"
	ambient_player.volume_db = -10.0
	add_child(ambient_player)

	# S57: 하트비트 레이어 플레이어
	heartbeat_player = AudioStreamPlayer.new()
	heartbeat_player.bus = "Master"
	heartbeat_player.volume_db = -18.0
	add_child(heartbeat_player)

	# 씬 전환 감지
	get_tree().tree_changed.connect(_on_tree_changed)

	# S57: 대화 덕킹 — DialogueManager 시그널 연결
	_connect_dialogue_ducking()

	print("[AudioManager] Ready — crossfade, ambient, ducking, intensity systems active")

## BGM 재생 (S57: 크로스페이드 — A/B 플레이어 교대)
func play_bgm(path: String, fade: bool = true) -> void:
	if path == current_bgm and _active_bgm_player.playing:
		return
	if not ResourceLoader.exists(path):
		push_warning("[AudioManager] BGM not found: %s" % path)
		return

	if _active_bgm_player.playing and fade:
		# S57: 크로스페이드 — 기존 BGM 페이드 아웃 + 새 BGM 페이드 인 (동시)
		if bgm_tween:
			bgm_tween.kill()
		var old_player = _active_bgm_player
		var new_player = bgm_player_b if old_player == bgm_player else bgm_player
		_active_bgm_player = new_player

		# 새 플레이어에 스트림 로드 및 시작 (볼륨 낮은 상태)
		var stream = load(path)
		if not stream:
			return
		new_player.stream = stream
		new_player.volume_db = -40.0
		new_player.pitch_scale = 1.0
		new_player.play()
		current_bgm = path

		# 크로스페이드 트윈 — 이전 페이드 아웃 + 새 페이드 인 동시 실행
		bgm_tween = create_tween()
		bgm_tween.set_parallel(true)
		var target_vol = DUCK_VOLUME if _bgm_ducked else _bgm_base_volume
		bgm_tween.tween_property(old_player, "volume_db", -40.0, CROSSFADE_DURATION)
		bgm_tween.tween_property(new_player, "volume_db", target_vol, CROSSFADE_DURATION)
		bgm_tween.set_parallel(false)
		bgm_tween.tween_callback(func():
			old_player.stop()
		)
	else:
		_start_bgm(path)

func _start_bgm(path: String) -> void:
	var stream = load(path)
	if stream:
		_active_bgm_player.stream = stream
		_active_bgm_player.volume_db = _bgm_base_volume if not _bgm_ducked else DUCK_VOLUME
		_active_bgm_player.pitch_scale = 1.0
		_active_bgm_player.play()
		current_bgm = path

## BGM 정지 (페이드 아웃)
func stop_bgm(fade: bool = true) -> void:
	if not _active_bgm_player.playing:
		return
	if fade:
		if bgm_tween:
			bgm_tween.kill()
		bgm_tween = create_tween()
		bgm_tween.tween_property(_active_bgm_player, "volume_db", -40.0, FADE_DURATION)
		bgm_tween.tween_callback(func():
			_active_bgm_player.stop()
			current_bgm = ""
		)
	else:
		_active_bgm_player.stop()
		bgm_player.stop()
		bgm_player_b.stop()
		current_bgm = ""

## SFX 재생 (코드 생성 — 외부 파일 불필요)
## S57: 자주 쓰는 SFX에 랜덤 피치 변주 적용
func play_sfx(type: String) -> void:
	if not sfx_player:
		return
	var samples = _generate_sfx(type)
	if samples.is_empty():
		return
	var stream = _samples_to_stream(samples)
	sfx_player.stream = stream
	# S57: 피치 변주 — 반복 재생이 단조롭지 않도록
	if SFX_PITCH_VARIATION.has(type):
		var variation: float = SFX_PITCH_VARIATION[type]
		sfx_player.pitch_scale = 1.0 + randf_range(-variation, variation)
	else:
		sfx_player.pitch_scale = 1.0
	sfx_player.play()

## 발걸음 전용 재생 (step_player 사용, 다른 SFX와 겹치지 않음)
## S41: 지형별 발걸음 SFX, S57: 피치 변주
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
	# S57: 발걸음 피치 변주
	if SFX_PITCH_VARIATION.has(sfx_type):
		var variation: float = SFX_PITCH_VARIATION[sfx_type]
		step_player.pitch_scale = 1.0 + randf_range(-variation, variation)
	else:
		step_player.pitch_scale = 1.0
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

## ===================== S57: 앰비언트 사운드 시스템 =====================
## 앰비언트 루프 재생 (프로시저럴 생성 — 바람, 비, 숲소리, 보이드 험)
func play_ambient(key: String) -> void:
	if key == current_ambient and ambient_player.playing:
		return
	var samples = _generate_ambient(key)
	if samples.is_empty():
		return
	var stream = _samples_to_stream_looped(samples)
	ambient_player.stream = stream
	ambient_player.volume_db = -25.0  # 시작 시 조용히
	ambient_player.play()
	current_ambient = key
	# 페이드 인
	var t = create_tween()
	t.tween_property(ambient_player, "volume_db", -10.0, 1.5)

## 앰비언트 정지 (페이드 아웃)
func stop_ambient() -> void:
	if not ambient_player.playing:
		return
	var t = create_tween()
	t.tween_property(ambient_player, "volume_db", -40.0, 1.0)
	t.tween_callback(func():
		ambient_player.stop()
		current_ambient = ""
	)

## 앰비언트 사운드 프로시저럴 생성 (로우패스 필터링된 화이트노이즈 기반)
func _generate_ambient(key: String) -> PackedFloat32Array:
	var samples = PackedFloat32Array()
	var sample_rate = 22050
	var duration = 3.0  # 3초 루프
	var total = int(sample_rate * duration)
	var prev: float = 0.0  # 로우패스 필터 상태

	match key:
		"wind_forest":
			# 부드러운 숲 바람 — 깊은 로우패스 + 느린 진폭 모듈레이션
			for i in range(total):
				var t = float(i) / sample_rate
				var noise = randf_range(-1.0, 1.0)
				# 강한 로우패스 필터 (알파 낮을수록 더 부드러움)
				prev = prev * 0.97 + noise * 0.03
				var mod = (sin(t * 0.8 * TAU) * 0.3 + 0.7)  # 볼륨 모듈레이션
				samples.append(prev * 0.12 * mod)
		"wind_light":
			# 약한 바람 — 매우 부드러운 로우패스
			for i in range(total):
				var t = float(i) / sample_rate
				var noise = randf_range(-1.0, 1.0)
				prev = prev * 0.98 + noise * 0.02
				var mod = (sin(t * 0.5 * TAU) * 0.2 + 0.8)
				samples.append(prev * 0.08 * mod)
		"wind_heavy":
			# 강한 바람 — 로우패스 + 간헐적 돌풍
			for i in range(total):
				var t = float(i) / sample_rate
				var noise = randf_range(-1.0, 1.0)
				prev = prev * 0.95 + noise * 0.05
				# 돌풍 — 주기적 볼륨 급증
				var gust = maxf(sin(t * 0.4 * TAU), 0.0) * 0.4
				var mod = 0.6 + gust
				samples.append(prev * 0.18 * mod)
		"rain":
			# 비 — 고주파 노이즈 + 물방울 딱딱 소리
			var prev_hi: float = 0.0
			for i in range(total):
				var t = float(i) / sample_rate
				var noise = randf_range(-1.0, 1.0)
				# 하이패스 느낌 — 덜 필터링
				prev_hi = prev_hi * 0.85 + noise * 0.15
				# 랜덤 물방울 클릭 (희소)
				var drop = 0.0
				if randf() < 0.001:
					drop = randf_range(0.1, 0.3)
				var mod = (sin(t * 0.3 * TAU) * 0.15 + 0.85)
				samples.append((prev_hi * 0.1 + drop) * mod)
		"void_hum":
			# 보이드 험 — 매우 낮은 주파수 드론 + 불안한 변조
			for i in range(total):
				var t = float(i) / sample_rate
				var drone = sin(t * 35.0 * TAU) * 0.1
				var mod_wave = sin(t * 3.0 * TAU) * 0.05
				var noise = randf_range(-1.0, 1.0)
				prev = prev * 0.98 + noise * 0.02
				samples.append((drone + mod_wave + prev * 0.04) * 0.8)
		_:
			return PackedFloat32Array()
	return samples

## 루프 가능한 WAV 스트림 생성
func _samples_to_stream_looped(samples: PackedFloat32Array) -> AudioStreamWAV:
	var stream = _samples_to_stream(samples)
	stream.loop_mode = AudioStreamWAV.LOOP_FORWARD
	stream.loop_begin = 0
	stream.loop_end = samples.size()
	return stream

## ===================== S57: 대화 오디오 덕킹 =====================
## DialogueManager 시그널에 연결하여 대화 중 BGM 볼륨 낮춤

func _connect_dialogue_ducking() -> void:
	# DialogueManager가 오토로드로 존재하는지 확인
	var dm = get_node_or_null("/root/DialogueManager")
	if dm == null:
		# 씬 로드 후 재시도
		call_deferred("_connect_dialogue_ducking_deferred")
		return
	if dm.has_signal("dialogue_started") and not dm.dialogue_started.is_connected(_on_dialogue_started):
		dm.dialogue_started.connect(_on_dialogue_started)
	if dm.has_signal("dialogue_ended") and not dm.dialogue_ended.is_connected(_on_dialogue_ended):
		dm.dialogue_ended.connect(_on_dialogue_ended)

func _connect_dialogue_ducking_deferred() -> void:
	await get_tree().process_frame
	var dm = get_node_or_null("/root/DialogueManager")
	if dm:
		if dm.has_signal("dialogue_started") and not dm.dialogue_started.is_connected(_on_dialogue_started):
			dm.dialogue_started.connect(_on_dialogue_started)
		if dm.has_signal("dialogue_ended") and not dm.dialogue_ended.is_connected(_on_dialogue_ended):
			dm.dialogue_ended.connect(_on_dialogue_ended)

func _on_dialogue_started() -> void:
	_bgm_ducked = true
	_duck_bgm(DUCK_VOLUME)

func _on_dialogue_ended() -> void:
	_bgm_ducked = false
	_duck_bgm(_bgm_base_volume)

func _duck_bgm(target_vol: float) -> void:
	# 현재 활성 BGM 플레이어의 볼륨을 부드럽게 조절
	if not _active_bgm_player.playing:
		return
	var t = create_tween()
	t.tween_property(_active_bgm_player, "volume_db", target_vol, DUCK_FADE)

## ===================== S57: 전투 음악 인텐시티 =====================
## 보스전 또는 HP < 30%일 때 긴장감 증폭

## 보스전 시작 시 호출 (BattleManager에서 연결)
func set_boss_fight(active: bool) -> void:
	_is_boss_fight = active
	if active:
		_apply_battle_intensity(true)
	else:
		_apply_battle_intensity(false)

## HP 비율에 따른 인텐시티 업데이트 (BattleManager에서 매 턴 호출 가능)
func update_battle_intensity(hp_ratio: float) -> void:
	if hp_ratio < 0.3:
		_apply_battle_intensity(true)
	elif not _is_boss_fight:
		_apply_battle_intensity(false)

func _apply_battle_intensity(intense: bool) -> void:
	if _intensity_tween:
		_intensity_tween.kill()
	_intensity_tween = create_tween()
	_intensity_tween.set_parallel(true)

	if intense:
		# BGM 피치 약간 올림 (1.05x) — 긴장감
		_intensity_tween.tween_property(_active_bgm_player, "pitch_scale", 1.05, 0.5)
		# 하트비트 레이어 시작
		if not _heartbeat_active:
			_start_heartbeat()
	else:
		# 정상 복귀
		_intensity_tween.tween_property(_active_bgm_player, "pitch_scale", 1.0, 0.8)
		# 하트비트 중지
		if _heartbeat_active:
			_stop_heartbeat()

func _start_heartbeat() -> void:
	_heartbeat_active = true
	var samples = _generate_heartbeat()
	var stream = _samples_to_stream_looped(samples)
	heartbeat_player.stream = stream
	heartbeat_player.volume_db = -30.0
	heartbeat_player.play()
	var t = create_tween()
	t.tween_property(heartbeat_player, "volume_db", -14.0, 0.5)

func _stop_heartbeat() -> void:
	_heartbeat_active = false
	var t = create_tween()
	t.tween_property(heartbeat_player, "volume_db", -40.0, 0.8)
	t.tween_callback(func():
		heartbeat_player.stop()
	)

## 하트비트 사운드 생성 (저주파 펄스 — 쿵...쿵... 패턴)
func _generate_heartbeat() -> PackedFloat32Array:
	var samples = PackedFloat32Array()
	var sample_rate = 22050
	# 하트비트 주기: ~1초 (쿵-쿵 패턴)
	var duration = 1.0
	var total = int(sample_rate * duration)
	for i in range(total):
		var t = float(i) / sample_rate
		var val = 0.0
		# 첫 번째 비트 (0.0~0.12s) — 강하게
		if t < 0.12:
			var env = (1.0 - t / 0.12)
			val = sin(t * 50.0 * TAU) * 0.35 * env * env
		# 두 번째 비트 (0.25~0.35s) — 약하게
		elif t >= 0.25 and t < 0.35:
			var bt = t - 0.25
			var env = (1.0 - bt / 0.10)
			val = sin(bt * 45.0 * TAU) * 0.2 * env * env
		samples.append(val)
	return samples

## 씬 전환 시 자동 BGM 교체
func _on_tree_changed() -> void:
	var tree = get_tree()
	if not tree:
		return
	var scene = tree.current_scene
	if not scene or scene.scene_file_path == "":
		return
	var path = scene.scene_file_path
	if path == _last_scene_path:
		return
	_last_scene_path = path

	# BGM 전환 (크로스페이드)
	if path == "res://scenes/battle/battle_scene.tscn":
		play_bgm("res://assets/audio/bgm/battle_theme.mp3")
		# 전투 진입 시 앰비언트 정지
		stop_ambient()
	elif SCENE_BGM.has(path):
		play_bgm(SCENE_BGM[path])

	# S57: 앰비언트 자동 전환
	if SCENE_AMBIENT.has(path):
		play_ambient(SCENE_AMBIENT[path])
	elif path == "res://scenes/battle/battle_scene.tscn":
		pass  # 전투 중 앰비언트 없음
	else:
		stop_ambient()

	# S57: 전투 인텐시티 리셋 (전투 씬 벗어날 때)
	if path != "res://scenes/battle/battle_scene.tscn":
		if _is_boss_fight:
			set_boss_fight(false)
		if _heartbeat_active:
			_stop_heartbeat()
		_active_bgm_player.pitch_scale = 1.0
