## BattleScene, 턴제 전투 화면 (S44: 사이드뷰 오버홀)
## BattleManager의 시그널을 받아 UI 표시.
## S44: 사이드뷰 레이아웃, 캐릭터/적 128x128 스프라이트, 전투 애니메이션
## S56: BattleVFX 통합 (향상된 데미지 넘버, 상태이상 파티클, 기억 연소 드라마틱 시퀀스)
## S57: Battle Juice Overhaul, knockback, hit-freeze, screen flash, combo counter, turn dim, death anim, hurt bounce
extends Control

const BATTLE_ITEM_TRAY_PATH: String = "res://assets/cg/generated/ui_battle_item_tray_v3.png"

## S209: 무대 기준선.
## 아군/지원/적 스프라이트의 발과 그림자는 모두 이 y좌표(1280x720 기준)에 맞춘다.
## 예전에는 각 컨테이너가 제각기 다른 높이에 그림자를 두어 엘리아는 지면보다
## 48px 위에 떠 있었고, 캐릭터 전체가 배경 위에 붙인 스티커처럼 보였다.
const STAGE_BASELINE_Y: float = 424.0
const STAGE_FLOOR_ANCHOR: float = 0.60

## S230: 전투 HUD 레이아웃 계약 (1280x720 기준).
##
## 예전에는 목표 카드, 전투 큐, 파티 명령 레일 4종, 적 판독, 플레이어 상태가
## 각자 앵커 숫자를 들고 같은 띠를 밟았다. 실제로 측정해 보면 겹침이 열 군데였고,
## 그중 셋은 글자를 잘라 먹었다("그림자 파수꾼" -> "림자 파수꾼", "세이블:" -> "이블:",
## 커맨드 덱 장식에 먹힌 파티 태그). 이제 열과 띠를 여기서 한 번만 정의한다.
##
##   왼쪽 열   : 전술 목표(상단) / 아렐 상태 묶음(하단)
##   가운데 열 : 턴 순서 칩(최상단) / 전투 큐 / 파티 지시
##   오른쪽 열 : 적 판독
##
## 전투원 판이 서는 대역(플레이어 y 183~433, 적 y 115~411)은 비워 둔다.
const HUD_CHIP_TOP: float = 6.0
const HUD_CHIP_BOTTOM: float = 32.0
const HUD_LEFT_COL_L: float = 0.02
const HUD_LEFT_COL_R: float = 0.32
const HUD_CENTER_COL_L: float = 0.36
const HUD_CENTER_COL_R: float = 0.65
const HUD_RIGHT_COL_L: float = 0.66
const HUD_RIGHT_COL_R: float = 0.965
const HUD_ENEMY_TOP: float = 8.0
const HUD_CUE_TOP: float = 38.0
const HUD_PARTY_TOP: float = 140.0
const HUD_TURN_BANNER_TOP: float = 300.0
const HUD_COMBO_TOP: float = 352.0
## 아렐 판의 발끝(424)보다 아래에서 시작해, 커맨드 덱 장식 윗선(545.8) 위에서 끝난다.
const HUD_PLAYER_CLUSTER_TOP: float = 438.0
const HUD_DECK_TOP: float = 545.8

## S228: Cinematic Battle Stage 2.0 role contract.
##
## The tabletop used to keep the 2D container, its contact shadow, and the 3D
## floor anchor in separate coordinate lists.  A size tweak could therefore
## leave a battler standing beside its own 3D shadow.  Every battler-facing
## value now lives here: the canvas foot point, the local container/plate fit,
## draw order, and the HybridDepthStage projection seed all travel together.
const BATTLE_ROLE_PROFILES: Dictionary = {
	"player": {
		"anchor_name": "PlayerAnchor",
		"stage_key": "player",
		"stage_anchor": HybridDepthStage.ANCHOR_PLAYER,
		"canvas_foot": Vector2(246.0, STAGE_BASELINE_Y),
		"container_size": Vector2(292.0, 292.0),
		"local_foot": Vector2(146.0, 278.0),
		# The portrait begins below the tactical directive band while its feet stay
		# locked to the primary contact point. It remains substantially larger than
		# the rear-line plates without competing with the objective readout.
		"plate_max_size": Vector2(248.0, 248.0),
		"plate_local_foot": Vector2(146.0, 278.0),
		"stage_order": -1,
		"shadow_radii": Vector2(78.0, 12.0),
		"shadow_alpha": 0.42,
		"glow_radii": Vector2(91.0, 16.0),
		"glow_color": Color(0.20, 0.38, 0.68, 0.13),
		# The Seam tableau is deliberately low-key; a restrained lift preserves
		# Arrel's face and blade as the first readable player-side silhouette.
		"plate_modulate": Color(1.10, 1.06, 1.02, 1.0),
		"edge_softness": 0.12,
		"oval_mask": 0.16,
		"rim": 0.0,
	},
	"ally": {
		"anchor_name": "AllyAnchor",
		"stage_key": "ally",
		"stage_anchor": HybridDepthStage.ANCHOR_ALLY,
		"canvas_foot": Vector2(113.0, STAGE_BASELINE_Y - 16.0),
		"container_size": Vector2(172.0, 202.0),
		"local_foot": Vector2(86.0, 192.0),
		"plate_max_size": Vector2(160.0, 184.0),
		"plate_local_foot": Vector2(86.0, 190.0),
		"stage_order": -3,
		"shadow_radii": Vector2(50.0, 8.0),
		"shadow_alpha": 0.27,
		"glow_radii": Vector2(58.0, 10.0),
		"glow_color": Color(0.54, 0.38, 0.68, 0.08),
		"plate_modulate": Color(1.0, 0.98, 0.95, 0.92),
		"edge_softness": 0.24,
		"oval_mask": 0.86,
		"rim": 0.0,
	},
	"support": {
		"anchor_name": "SupportAnchor",
		"stage_key": "support",
		"stage_anchor": HybridDepthStage.ANCHOR_SUPPORT,
		"canvas_foot": Vector2(364.0, STAGE_BASELINE_Y - 22.0),
		"container_size": Vector2(170.0, 200.0),
		"local_foot": Vector2(85.0, 190.0),
		"plate_max_size": Vector2(154.0, 178.0),
		"plate_local_foot": Vector2(85.0, 188.0),
		"stage_order": -2,
		"shadow_radii": Vector2(50.0, 8.0),
		"shadow_alpha": 0.24,
		"glow_radii": Vector2(58.0, 10.0),
		"glow_color": Color(0.55, 0.48, 0.30, 0.08),
		"plate_modulate": Color(0.98, 0.96, 0.93, 0.90),
		"edge_softness": 0.24,
		"oval_mask": 0.86,
		"rim": 0.0,
	},
	"enemy": {
		"anchor_name": "EnemyAnchor",
		"stage_key": "enemy",
		"stage_anchor": HybridDepthStage.ANCHOR_ENEMY,
		"canvas_foot": Vector2(1016.0, STAGE_BASELINE_Y),
		"container_size": Vector2(360.0, 310.0),
		"local_foot": Vector2(180.0, 300.0),
		"plate_max_size": Vector2(350.0, 296.0),
		"plate_local_foot": Vector2(180.0, 296.0),
		"stage_order": -1,
		"shadow_radii": Vector2(108.0, 15.0),
		"shadow_alpha": 0.40,
		"glow_radii": Vector2(116.0, 17.0),
		"glow_color": Color(0.50, 0.15, 0.50, 0.10),
		"plate_modulate": Color(1.50, 1.45, 1.44, 0.96),
		# S241: 0.14로 내려 봤으나 되돌렸다. edge_softness가 적의 구조를 먹는다는
		# 가설이었는데, RMS는 0.0391→0.0414로 6%만 올랐고(예측은 0.06 근처) 대신
		# 밝기가 0.0368→0.0279로 24% 떨어졌다. 판이 불투명해지며 뒤로 비치던 밝은
		# 무대가 사라진 것일 뿐, 원화의 디테일이 돌아온 게 아니다. 역할별
		# edge_softness와 RMS 유지율의 상관은 교란이었다.
		"edge_softness": 0.30,
		"oval_mask": 0.80,
		"rim": 0.0,
	},
}

# UI 노드
var bg: ColorRect
var enemy_panel: PanelContainer
var enemy_name_label: Label
var enemy_scan_chip: Label
var enemy_hp_bar: ProgressBar
var enemy_hp_ghost: ProgressBar  # S230: 최근 피해 잔상
var enemy_hp_label: Label
var enemy_break_bar: ProgressBar
var enemy_break_label: Label
var player_panel: PanelContainer
var player_hp_bar: ProgressBar
var player_hp_ghost: ProgressBar
var player_hp_label: Label
var party_orders_panel: PanelContainer
var party_orders_rows: VBoxContainer
var turn_banner: PanelContainer
var log_label: RichTextLabel
var field_readout_art: TextureRect
var field_readout_header: Label
var action_ribbon_art: TextureRect
var action_container: GridContainer
var witness_btn: Button
var burn_list_container: VBoxContainer
var item_list_container: VBoxContainer
var _battle_quick_item_buttons: Array[Button] = []
var enemy_sprite: CanvasItem  # 적 스프라이트
var enemy_sprite_container: Control  # 적 아이들 모션용 컨테이너

var log_lines: Array = []
var _last_battle_message: String = ""
var _action_buttons: Dictionary = {}
const MAX_LOG_LINES: int = 2
const UI_COMMAND_RIBBON_PATH: String = "res://assets/cg/generated/ui_battle_command_ribbon.png"
const UI_COMMAND_DECK_PATH: String = "res://assets/cg/generated/ui_battle_command_deck_v4.png"
const UI_FIELD_READOUT_PATH: String = "res://assets/cg/generated/ui_battle_field_readout_v4.png"
const UI_TACTICAL_PLATE_PATH: String = "res://assets/cg/generated/ui_battle_tactical_plate.png"
const UI_VICTORY_PANEL_PATH: String = "res://assets/cg/generated/ui_battle_victory_reward_panel.png"
const UI_BURN_PREVIEW_PANEL_PATH: String = "res://assets/cg/generated/ui_burn_preview_ritual_panel.png"
const LAST_STAND_CUTIN_PATH: String = "res://assets/cg/generated/cinematic_last_stand_resonance.png"
const MEMORY_CASCADE_CUTIN_PATH: String = "res://assets/cg/generated/cinematic_arrel_memory_cascade.png"
const ARREL_BLADE_CUTIN_PATH: String = "res://assets/cg/generated/battle_cutin_arrel_blade_arc_v4.png"
const ARREL_GUARD_CUTIN_PATH: String = "res://assets/cg/generated/battle_cutin_arrel_guard_v4.png"
const ARREL_WITNESS_CUTIN_PATH: String = "res://assets/cg/generated/battle_cutin_arrel_witness_v4.png"
const ARREL_CHAIN_BURN_CUTIN_PATH: String = "res://assets/cg/generated/illustration_expansion_v2/battle_cutin_arrel_chain_burn_v5.png"
const ELIA_ANCHOR_CUTIN_PATH: String = "res://assets/cg/generated/battle_cutin_elia_anchor_v4.png"
const TOBIAS_ACTION_CUTIN_PATH: String = "res://assets/cg/generated/battle_cutin_tobias_faultline_v4.png"
const SABLE_ACTION_CUTIN_PATH: String = "res://assets/cg/generated/battle_cutin_sable_threadstrike_v4.png"
const ELIA_HUMMING_SHIELD_CUTIN_PATH: String = "res://assets/cg/generated/illustration_expansion_v2/battle_cutin_elia_humming_shield_v5.png"
const TOBIAS_ARCHIVE_COUNTER_CUTIN_PATH: String = "res://assets/cg/generated/illustration_expansion_v2/battle_cutin_tobias_archive_counter_v5.png"
const SABLE_WARDEN_INTERCEPT_CUTIN_PATH: String = "res://assets/cg/generated/illustration_expansion_v2/battle_cutin_sable_warden_intercept_v5.png"
const KAIROS_REDACTION_CUTIN_PATH: String = "res://assets/cg/character_shots/kairos_occult_editor_v1.png"
const LEGACY_KAIROS_REDACTION_CUTIN_PATH: String = "res://assets/cg/generated/illustration_expansion_v2/battle_cutin_kairos_redaction_v5.png"
const BREAK_FAULTLINE_CUTIN_PATH: String = "res://assets/cg/generated/battle_cutin_break_faultline_v4.png"
const SABLE_BATTLE_FULLBODY_PATH: String = "res://assets/portraits/character_shots/sable_warden_v3.png"
const TOBIAS_BATTLE_FULLBODY_PATH: String = "res://assets/portraits/character_shots/tobias_ledger_v3.png"
const ELIA_BATTLE_FULLBODY_PATH: String = "res://assets/portraits/character_shots/elia_anchor_v3.png"
const ARREL_BATTLE_FULLBODY_PATH: String = "res://assets/portraits/character_shots/arrel_battle_v3.png"
const VOID_BEAST_ACTION_CUTIN_PATH: String = "res://assets/cg/character_shots/void_beast_occult_rite_v1.png"
const ECHO_SHELL_ACTION_CUTIN_PATH: String = "res://assets/cg/character_shots/echo_shell_reach_v3.png"
const ENEMY_ACTION_CUTIN_PATHS: Dictionary = {
	"ash bone hound": "res://assets/cg/generated/battle_stage_v2/enemy_ash_hound_battle_v1.png",
	"ash hound": "res://assets/cg/generated/battle_stage_v2/enemy_ash_hound_battle_v1.png",
	"signal wisp": "res://assets/cg/generated/battle_stage_v2/enemy_signal_wisp_battle_v1.png",
	"archive wisp": "res://assets/cg/generated/battle_stage_v2/enemy_signal_wisp_battle_v1.png",
	"seam wisp": "res://assets/cg/generated/battle_stage_v2/enemy_signal_wisp_battle_v1.png",
	"threshold wisp": "res://assets/cg/generated/battle_stage_v2/enemy_signal_wisp_battle_v1.png",
	"rootbound echo": "res://assets/cg/generated/battle_stage_v2/enemy_rootbound_echo_battle_v1.png",
	"void fragment": "res://assets/cg/generated/battle_stage_v2/enemy_void_fragment_battle_v1.png",
	"hollow fragment": "res://assets/cg/generated/battle_stage_v2/enemy_void_fragment_battle_v1.png",
	"ash crawler": "res://assets/cg/generated/illustration_expansion_v3/enemy_cutin_ash_crawler_v1.png",
	"forest shade": "res://assets/cg/generated/illustration_expansion_v3/enemy_cutin_forest_shade_v1.png",
	"threshold shade": "res://assets/cg/generated/illustration_expansion_v3/enemy_cutin_threshold_shade_v1.png",
	"void watcher": "res://assets/cg/generated/illustration_expansion_v3/enemy_cutin_void_watcher_v1.png",
	"memory eater": "res://assets/cg/generated/illustration_expansion_v3/enemy_cutin_memory_eater_v1.png",
}
const ELIA_ACTION_CUTIN_PATHS: Dictionary = {
	"humming_shield": ELIA_HUMMING_SHIELD_CUTIN_PATH,
	"desperate_reach": "res://assets/cg/generated/illustration_expansion_v2/world_rewrite_reaching_hand_v3.png",
	"remembered_strike": "res://assets/cg/generated/illustration_expansion_v2/world_rewrite_first_sword_v3.png",
	"anchor_pulse": ELIA_ANCHOR_CUTIN_PATH,
}
const TOBIAS_ACTION_CUTIN_PATHS: Dictionary = {
	"archive": TOBIAS_ARCHIVE_COUNTER_CUTIN_PATH,
	"protect": TOBIAS_ACTION_CUTIN_PATH,
	"analyze": TOBIAS_ACTION_CUTIN_PATH,
}
const SABLE_ACTION_CUTIN_PATHS: Dictionary = {
	"protect": SABLE_WARDEN_INTERCEPT_CUTIN_PATH,
	"guard": SABLE_WARDEN_INTERCEPT_CUTIN_PATH,
	"strike": SABLE_ACTION_CUTIN_PATH,
	"weaken": SABLE_ACTION_CUTIN_PATH,
}
const BOSS_PHASE_CUTIN_PATHS: Dictionary = {
	"Shade Sentinel": "res://assets/cg/character_shots/shade_sentinel_ritual_seal_v1.png",
	"Kairos, Authority Editor": KAIROS_REDACTION_CUTIN_PATH,
}
const MEMORY_BURN_CUTIN_PATHS: Dictionary = {
	"identity_first_sword": "res://assets/cg/generated/illustration_expansion_v2/world_rewrite_first_sword_v3.png",
	"daily_campfire_song": "res://assets/cg/generated/illustration_expansion_v2/world_rewrite_campfire_song_v3.png",
	"rel_hand_reaching": "res://assets/cg/generated/illustration_expansion_v2/world_rewrite_reaching_hand_v3.png",
	"core_name_origin": "res://assets/cg/generated/illustration_expansion_v2/world_rewrite_name_origin_v3.png",
	"daily_elia_hands": "res://assets/cg/generated/illustration_expansion_v2/world_rewrite_elia_anchor_v3.png",
	"identity_compass": "res://assets/cg/generated/memory_burn_compass.png",
	"identity_void_walker": "res://assets/cg/generated/memory_burn_void_walker.png",
}
const ITEM_ACTION_CUTIN_PATHS: Dictionary = {
	"heal": "res://assets/cg/generated/gameplay_moments/item_recover_cutin_v1.png",
	"cure": "res://assets/cg/generated/gameplay_moments/item_cure_cutin_v1.png",
	"burn": "res://assets/cg/generated/gameplay_moments/item_ignite_cutin_v1.png",
	"flee": "res://assets/cg/generated/gameplay_moments/item_withdraw_cutin_v1.png",
	"witness": "res://assets/cg/generated/gameplay_moments/item_witness_cutin_v1.png",
	"guard": "res://assets/cg/generated/gameplay_moments/item_anchor_guard_cutin_v1.png",
	"scan": "res://assets/cg/generated/gameplay_moments/item_fault_scan_cutin_v1.png",
}
var hp_tween_player: Tween
var hp_tween_enemy: Tween
var _hp_display_primed: bool = false
var canvas_root: Control  # 전투 UI 루트 (셰이크용)
var hit_flash_rect: ColorRect  # 히트 플래시 오버레이

# 전투 인트로 / VFX
var intro_overlay: ColorRect
var turn_label: Label
var enemy_status_container: Container
var player_status_container: Container
var slash_rect: ColorRect  # 공격 슬래시 VFX
var burn_vfx_container: Control  # 연소 VFX 컨테이너

# Limit Break UI
var limit_bar: ProgressBar
var limit_label: Label
var limit_btn: Button

# S44: 사이드뷰 캐릭터 스프라이트
var player_sprite: CanvasItem  # 아렐 스프라이트
var player_sprite_container: Control  # 아이들 모션용
# S151: 스프라이트 기본 스케일, 시트(0.625)/절차(0.78) 어느 쪽이든 트윈이 이 기준으로 복귀
var _player_sprite_base_scale: Vector2 = Vector2.ONE
var ally_sprite: CanvasItem  # 동행자 스프라이트 (엘리아/세이블)
var ally_sprite_container: Control
var _displayed_ally_identity: String = ""
var tobias_sprite_container: Control
var tobias_sprite: CanvasItem
## S212: 전투원 앵커 (3D 카메라 결합).
## 1.0이어야 한다. 접지 그림자와 포커스 링은 3D 무대 안에 있어서 카메라와 함께
## 온전히 움직이는데, 캐릭터만 감쇠시키면 자기 그림자에서 미끄러져 나온다.
## 원근 시차는 이미 앵커의 깊이(z)에서 나온다. 앞에 선 캐릭터는 뒤쪽 기둥보다
## 자연히 더 많이 움직이므로, 여기서 따로 줄일 필요가 없다.
const BATTLER_PARALLAX: float = 1.0
var _battler_anchors: Array[Control] = []
var _foreground_depth_stage: HybridDepthStage = null
var _player_base_pos: Vector2 = Vector2.ZERO  # 돌진 복귀용
var _enemy_base_pos: Vector2 = Vector2.ZERO
var _ally_base_pos: Vector2 = Vector2.ZERO
var _tobias_base_pos: Vector2 = Vector2.ZERO
var player_shadow: Polygon2D
var enemy_shadow: Polygon2D
var ally_shadow: Polygon2D
var tobias_shadow: Polygon2D
var _battle_stage_left: TextureRect
var _battle_stage_right: TextureRect
var _battle_stage_left_wash: ColorRect
var _battle_stage_right_wash: ColorRect
var _hybrid_depth_stage: HybridDepthStage
var _action_cutin: TextureRect
var _action_cutin_wash: ColorRect
var _action_cutin_tween: Tween
var combat_cue_panel: PanelContainer
var combat_cue_art: TextureRect
var combat_cue_title: Label
var combat_cue_detail: Label
var _combat_cue_tween: Tween
var _ground_rect: ColorRect  # 전투 지면
var player_portrait_rect: TextureRect  # HP 옆 포트레이트
var _player_readout_column: VBoxContainer  # S230: HP/리밋/상태를 쌓는 한 열
var limit_value_label: Label

# 적 아이들 모션
var _idle_time: float = 0.0
var _enemy_base_y: float = 0.0
var _painterly_semantic_tweens: Dictionary = {}
var _painterly_semantic_generations: Dictionary = {}
var _current_battler_focus: String = "neutral"

# S42: 전투 분위기 컬러 그레이딩
var _color_grade_rect: ColorRect
var _battle_particles: GPUParticles2D  # 배경 파티클
var _battle_parallax_layers: Array = []  # S53: 전투 패럴랙스
var _resolved_battle_bg_image: String = ""

# S46: 타격감 강화
var _enemy_shader_mat: ShaderMaterial  # 적 VFX 셰이더
var _player_shader_mat: ShaderMaterial  # ��레이어 VFX 셰이더
var ally_cmd_container: HFlowContainer  # 세이블 명령 UI
var tobias_cmd_container: HFlowContainer  # 토비아스 명령 UI

# S55: Scan + Environment display
var scan_info_container: HBoxContainer  # 스캔 약점/저항 표시 (적 HP 아래)
var env_label: Label  # 환경 보너스 표시
var objective_panel: PanelContainer
var objective_art: TextureRect
var objective_title_label: Label
var objective_desc_label: Label
var objective_meta_label: Label
var objective_briefing_overlay: Control
var objective_briefing_panel: PanelContainer
var objective_briefing_buttons: VBoxContainer

# S55: Auto Battle
var auto_label: Label  # [AUTO] 표시
var auto_btn: Button  # 자동전투 버튼

# S209: 전투 배속 칩
var _battle_speed_btn: Button

# S56: BattleVFX 유틸리티
var battle_vfx: BattleVFX

# S57: Battle Juice, turn transition dim overlay, combo display
var _turn_dim_overlay: ColorRect
var _combo_display_label: Label

# S58: Burn Preview Popup, risk/reward decision UI
var _burn_preview_panel: PanelContainer
var _burn_preview_art: TextureRect
var _burn_preview_dimmer: ColorRect
var _burn_preview_confirm_btn: Button
var _burn_preview_cancel_btn: Button
var _burn_preview_timer: SceneTreeTimer  # delay before buttons become clickable
var _burn_preview_transition_tween: Tween
var _burn_preview_pulse_tween: Tween
var _burn_preview_generation: int = 0
var _pending_burn_id: String = ""  # memory ID waiting for confirmation

# S54: Victory screen
var _last_objective_status: String = ""  # S226: fires the complete/fail cue exactly once
var _victory_panel: PanelContainer
var _victory_art: TextureRect
var _victory_rewards: Array = []  # collected reward lines from battle_log
var _opening_carryovers_presented: bool = false

func _ready() -> void:
	_build_ui()
	_connect_signals()
	_on_limit_changed(BattleManager.limit_gauge)
	# S56: BattleVFX 초기화 (_build_ui 이후 canvas_root 유효)
	battle_vfx = BattleVFX.new(self, canvas_root)
	# 인트로 연출 후 HP 표시
	_play_intro()

func _present_field_focus_opening() -> void:
	var opening_delay := 0.82 if BattleManager.field_entry_mode != "neutral" else 0.30
	await BattleManager.pace_timer(opening_delay).timeout
	if not is_inside_tree():
		return
	var message := "[현장 집중] 메아리의 방향이 전투에 남았다 · 공명 25 / 리미트 20" if GameManager.current_locale == "ko" else "[FIELD FOCUS] The mapped echo carries into battle · Resonance 25 / Limit 20"
	_on_battle_log(message)
	_show_turn_indicator(_bl("FIELD FOCUS", "필드 포커스"), Color(0.82, 0.72, 0.42))

func _present_field_entry() -> void:
	await BattleManager.pace_timer(0.08).timeout
	if not is_inside_tree():
		return
	var mode := BattleManager.field_entry_mode
	var title := ""
	var detail := ""
	var cutin_path := ""
	var accent := Color(0.58, 0.82, 1.0)
	match mode:
		"ambush":
			title = _bl("PHASE AMBUSH", "위상 기습")
			detail = _bl("First contact seized · BREAK pressure primed", "접촉 주도권 확보 · BREAK 압력 예열")
			cutin_path = ARREL_BLADE_CUTIN_PATH
			accent = Color(1.0, 0.62, 0.30)
		"guarded":
			title = _bl("GUARDED ENTRY", "대비 진입")
			detail = _bl("Collision read · first hostile blow guarded", "충돌 예측 완료 · 적의 첫 일격 방어")
			cutin_path = ARREL_GUARD_CUTIN_PATH
			accent = Color(0.48, 0.84, 0.96)
		"witness":
			title = _bl("WITNESS ENTRY", "증언 진입")
			detail = _bl("The memory inside the threat is already exposed", "위협 안의 기억을 전투 전에 드러냄")
			cutin_path = ARREL_WITNESS_CUTIN_PATH
			accent = Color(0.78, 0.64, 1.0)
		_:
			return
	# S226: The opening the approach bought is stated, not implied.
	var bonus_text := BattleManager.get_field_entry_bonus_text()
	if bonus_text != "":
		detail = "%s\n%s" % [detail, bonus_text]
	_on_battle_log("[APPROACH] %s" % detail)
	_show_turn_indicator(title, accent)
	_show_combat_cue(title, detail, cutin_path, accent, 1.45)
	_play_action_cutin(cutin_path, true, 0.78, 0.28)
	TutorialHints.show_hint("first_approach")

func _process(delta: float) -> void:
	_idle_time += delta
	_update_battler_anchors()
	var clean_view: bool = OptionsMenu.is_clean_gameplay_visuals()
	var reduce_motion: bool = OptionsMenu.is_reduce_motion()
	var battler_idle_motion: bool = not clean_view and not reduce_motion
	if action_ribbon_art and action_container:
		action_ribbon_art.visible = action_container.visible
		if action_ribbon_art.visible:
			action_ribbon_art.modulate.a = 0.92 if clean_view else 0.94 + sin(_idle_time * 1.15) * 0.018
	# 적 아이들 모션 (호흡, 상하 + 미세 스케일)
	if enemy_sprite_container and enemy_sprite_container.visible and battler_idle_motion:
		enemy_sprite_container.position.y = _enemy_base_pos.y + sin(_idle_time * 1.5) * 3.0
		enemy_sprite_container.scale = Vector2(1.0 + sin(_idle_time * 1.5) * 0.008, 1.0 - sin(_idle_time * 1.5) * 0.006)
	# 플레이어 아이들 모션 (호흡 + 미세 스케일)
	if player_sprite_container and battler_idle_motion:
		player_sprite_container.position.y = _player_base_pos.y + sin(_idle_time * 1.8 + 0.5) * 2.0
		player_sprite_container.scale = Vector2(1.0 + sin(_idle_time * 1.8 + 0.5) * 0.006, 1.0 - sin(_idle_time * 1.8 + 0.5) * 0.005)
	# 동행자 아이들
	if ally_sprite_container and ally_sprite_container.visible and battler_idle_motion:
		ally_sprite_container.position.y = _ally_base_pos.y + sin(_idle_time * 1.3 + 1.2) * 2.5
		ally_sprite_container.scale = Vector2(1.0 + sin(_idle_time * 1.3 + 1.2) * 0.007, 1.0 - sin(_idle_time * 1.3 + 1.2) * 0.005)
	if tobias_sprite_container and tobias_sprite_container.visible and battler_idle_motion:
		tobias_sprite_container.position.y = _tobias_base_pos.y + sin(_idle_time * 1.1 + 2.0) * 2.0
		tobias_sprite_container.scale = Vector2(1.0 + sin(_idle_time * 1.1 + 2.0) * 0.005, 1.0 - sin(_idle_time * 1.1 + 2.0) * 0.004)
	# S53: 전투 패럴랙스 미세 이동
	if not clean_view:
		for layer in _battle_parallax_layers:
			if layer and is_instance_valid(layer):
				var speed = layer.get_meta("parallax_speed", 0.5)
				layer.position.x = sin(_idle_time * speed * 0.3) * 15 * speed
	# Both accessibility modes keep the authored stage hierarchy at its profile
	# base. Reduce Motion also suppresses semantic transition tweens elsewhere;
	# Clean View simply removes decorative drift and clutter.
	if reduce_motion or clean_view:
		if enemy_sprite_container:
			enemy_sprite_container.position = _enemy_base_pos
			enemy_sprite_container.scale = Vector2.ONE
		if player_sprite_container:
			player_sprite_container.position = _player_base_pos
			player_sprite_container.scale = Vector2.ONE
		if ally_sprite_container:
			ally_sprite_container.position = _ally_base_pos
			ally_sprite_container.scale = Vector2.ONE
		if tobias_sprite_container:
			tobias_sprite_container.position = _tobias_base_pos
			tobias_sprite_container.scale = Vector2.ONE

func _resolve_battle_bg_image() -> String:
	if BattleManager.battle_bg_image != "" and ResourceLoader.exists(BattleManager.battle_bg_image):
		return BattleManager.battle_bg_image

	var scene := BattleManager.return_scene
	var scene_bg_map := {
		"rim_forest": "res://assets/cg/generated/story_ch1_twisted_forest_path.png",
		"verdan_market": "res://assets/cg/generated/chapter_splash_verdan_market.png",
		"belt_waystation": "res://assets/cg/generated/chapter_splash_belt_waystation.png",
		"drift_shelter": "res://assets/cg/generated/chapter_splash_drift_shelter.png",
		"crumbling_coast": "res://assets/cg/generated/chapter_splash_crumbling_coast.png",
		"the_seam": "res://assets/cg/generated/chapter_splash_the_seam.png",
		"seam_outskirts": "res://assets/cg/generated/chapter_splash_seam_outskirts.png",
		"forgotten_forest": "res://assets/cg/generated/chapter_splash_forgotten_forest.png",
		"colorless_waste": "res://assets/cg/generated/memory_compass_resonance_cinematic.png",
		"bl07_void": "res://assets/cg/generated/chapter_splash_bl07_void.png",
	}

	for key in scene_bg_map.keys():
		var candidate: String = scene_bg_map[key]
		if key in scene and ResourceLoader.exists(candidate):
			return candidate
	return ""

## ===================== UI 빌드 =====================

func _build_ui() -> void:
	# 배경 (이미지 또는 단색)
	bg = ColorRect.new()
	bg.set_anchors_preset(Control.PRESET_FULL_RECT)
	bg.color = Color(0.04, 0.04, 0.06)
	add_child(bg)

	_resolved_battle_bg_image = _resolve_battle_bg_image()
	if _resolved_battle_bg_image != "":
		var bg_tex = TextureRect.new()
		bg_tex.set_anchors_preset(Control.PRESET_FULL_RECT)
		bg_tex.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
		bg_tex.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_COVERED
		bg_tex.texture = load(_resolved_battle_bg_image)
		# S215: 전투 배경은 손으로 그린 큰 일러스트다. 프로젝트 기본 필터가 Nearest라
		# 축소하면서 바위와 첨탑 가장자리가 지직거렸다. 픽셀아트가 아니므로 선형으로 줄인다.
		bg_tex.texture_filter = CanvasItem.TEXTURE_FILTER_LINEAR_WITH_MIPMAPS
		# S209: 배경 후퇴.
		# 전투 배경은 인물이 그려진 풀 일러스트라, 거의 원본 밝기로 깔면 전투원보다
		# 배경 속 사람이 더 크고 선명하게 읽혔다. 밝기와 채도를 낮춰 무대 뒤로 물린다.
		bg_tex.modulate = Color(0.60, 0.58, 0.62, 0.94) if OptionsMenu.is_clean_gameplay_visuals() else Color(0.50, 0.47, 0.52, 0.88)
		add_child(bg_tex)
		_add_background_recession()
		if not OptionsMenu.is_clean_gameplay_visuals():
			_add_battle_art_depth(_resolved_battle_bg_image)
	_build_hybrid_depth_stage()

	# 배경 비네트 오버레이
	_add_battle_vignette()
	# S42: 배경 분위기 파티클 + 컬러 그레이딩
	_add_battle_atmosphere()
	_add_premium_battle_lens()
	# S53: 전투 패럴랙스 레이어
	_add_battle_parallax()

	# S44: 전투 지면 (그라운드 플랫폼)
	_build_battle_ground()

	var canvas = CanvasLayer.new()
	canvas.layer = 10
	add_child(canvas)

	var root = Control.new()
	root.set_anchors_preset(Control.PRESET_FULL_RECT)
	canvas.add_child(root)
	canvas_root = root

	# 히트 플래시 오버레이 (최상단에 나중에 추가)
	hit_flash_rect = ColorRect.new()
	hit_flash_rect.set_anchors_preset(Control.PRESET_FULL_RECT)
	hit_flash_rect.color = Color(1, 0.2, 0.15, 0)
	hit_flash_rect.mouse_filter = Control.MOUSE_FILTER_IGNORE
	hit_flash_rect.z_index = 100

	# 슬래시 VFX 레이어 (투명 초기)
	slash_rect = ColorRect.new()
	slash_rect.set_anchors_preset(Control.PRESET_FULL_RECT)
	slash_rect.color = Color(1, 1, 1, 0)
	slash_rect.mouse_filter = Control.MOUSE_FILTER_IGNORE
	slash_rect.z_index = 50

	# 연소 VFX 컨테이너
	burn_vfx_container = Control.new()
	burn_vfx_container.set_anchors_preset(Control.PRESET_FULL_RECT)
	burn_vfx_container.mouse_filter = Control.MOUSE_FILTER_IGNORE
	burn_vfx_container.z_index = 45

	_build_battle_stage_art(root)
	_build_action_cutin(root)
	_build_combat_cue_panel(root)

	# S44: 사이드뷰, 플레이어 스프라이트 (왼쪽)
	_build_player_sprite(root)

	# S44: 사이드뷰, 동행자 스프라이트 (왼쪽, 플레이어 뒤)
	_build_ally_sprite(root)
	_build_tobias_support_sprite(root)

	# S44: 적 스프라이트 (오른쪽), 128x128 대형
	_build_enemy_sprite(root)

	# 적 이름 + HP (상단 오른쪽)
	_build_enemy_panel(root)

	# 전투 로그 (중앙)
	_build_log_panel(root)

	# 아렐 상태 묶음 (좌하단): 초상 + HP + 리밋 + 상태 칩을 한 카드로 쌓는다
	_build_player_panel(root)
	_build_limit_gauge(root)
	_build_player_status(root)

	# 행동 버튼 (하단)
	_build_action_buttons(root)

	# 기억 연소 목록 (숨김 상태)
	_build_burn_list(root)

	# 아이템 목록 (숨김 상태)
	_build_item_list(root)

	# 턴 표시 라벨
	_build_turn_label(root)

	# S41: 턴 순서 미리보기
	_build_turn_preview(root)

	# S230: 자세 / 엘리아 / 세이블 / 토비아스 지시를 한 카드에 위에서 아래로 쌓는다.
	# 네 레일이 각자 앵커로 같은 띠를 밟던 문제(엘리아 레일과 토비아스 레일은
	# 좌표가 완전히 같았다)를 컨테이너 배치로 없앤다.
	_build_party_orders_panel(root)
	_build_stance_ui(party_orders_rows)       # S51: 자세
	_build_elia_skill_ui(party_orders_rows)   # S51: 엘리아 기술
	_build_ally_command_ui(party_orders_rows) # S46: 세이블 명령
	_build_tobias_command_ui(party_orders_rows) # S53: 토비아스 명령
	_build_echo_display(root)

	# VFX 레이어 추가
	root.add_child(burn_vfx_container)
	root.add_child(slash_rect)
	root.add_child(hit_flash_rect)

	# S55: Auto Battle 라벨
	_build_auto_label(root)
	# S209: 전투 배속 칩
	_build_battle_speed_chip(root)
	_build_tactical_objective_panel(root)
	_build_objective_briefing(root)

	# S58: Burn Preview Popup (hidden by default)
	_build_burn_preview(root)

	# S57: 턴 전환 딤 오버레이
	_turn_dim_overlay = ColorRect.new()
	_turn_dim_overlay.set_anchors_preset(Control.PRESET_FULL_RECT)
	_turn_dim_overlay.color = Color(0, 0, 0, 0)
	_turn_dim_overlay.mouse_filter = Control.MOUSE_FILTER_IGNORE
	_turn_dim_overlay.z_index = 85
	root.add_child(_turn_dim_overlay)

	# S57: 콤보 카운터 디스플레이 (상시 표시용)
	# S230: 예전 자리(y 30~70)는 전투 큐 패널 한복판이라 두 연출이 겹쳐 읽혔다.
	# 턴 배너 바로 아래, 무대 가운데 빈 대역으로 내린다.
	_combo_display_label = Label.new()
	_combo_display_label.name = "ComboReadout"
	_combo_display_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_combo_display_label.anchor_left = 0.5
	_combo_display_label.anchor_right = 0.5
	_combo_display_label.anchor_top = 0.0
	_combo_display_label.offset_left = -120
	_combo_display_label.offset_right = 120
	_combo_display_label.offset_top = HUD_COMBO_TOP
	_combo_display_label.offset_bottom = HUD_COMBO_TOP + 40.0
	UITheme.style_label(_combo_display_label, UITheme.make_ui_font(), 24, Color(1.0, 0.85, 0.2))
	_combo_display_label.modulate.a = 0.0
	_combo_display_label.z_index = 88
	_combo_display_label.mouse_filter = Control.MOUSE_FILTER_IGNORE
	root.add_child(_combo_display_label)

	# 인트로 오버레이 (최상단)
	_build_intro_overlay(root)

	# S212: 전투원 배치가 모두 끝난 뒤, 3D 표식을 2D 발 위치에 맞춘다.
	_sync_battler_anchors_to_stage()

	# S213: 전경 3D 레이어. 전투원 "위"에 합성되므로 마지막에 올린다.
	_build_foreground_depth(root)
	var initial_focus := "player" if BattleManager.state == BattleManager.BattleState.PLAYER_TURN else "enemy" if BattleManager.state == BattleManager.BattleState.ENEMY_TURN else "neutral"
	_set_battle_stage_focus(initial_focus)

## ===================== 배경 비네트 =====================

func _add_battle_vignette() -> void:
	if OptionsMenu.is_clean_gameplay_visuals():
		return
	# 셰이더 기반 원형 비네트 (S40)
	var vignette = ColorRect.new()
	vignette.set_anchors_preset(Control.PRESET_FULL_RECT)
	vignette.color = Color(0, 0, 0, 0)  # 셰이더가 알파를 제어
	vignette.z_index = -1
	vignette.mouse_filter = Control.MOUSE_FILTER_IGNORE

	var shader_path = "res://assets/shaders/vignette.gdshader"
	if ResourceLoader.exists(shader_path):
		var mat = ShaderMaterial.new()
		mat.shader = load(shader_path)
		mat.set_shader_parameter("intensity", 0.6)
		mat.set_shader_parameter("outer_radius", 0.9)
		mat.set_shader_parameter("inner_radius", 0.3)
		vignette.material = mat
	add_child(vignette)

func _add_battle_art_depth(texture_path: String) -> void:
	if texture_path == "" or not ResourceLoader.exists(texture_path):
		return

	var top_plate = TextureRect.new()
	top_plate.anchor_left = 0.0
	top_plate.anchor_right = 1.0
	top_plate.anchor_top = 0.0
	top_plate.anchor_bottom = 0.42
	top_plate.offset_left = -26
	top_plate.offset_right = 26
	top_plate.offset_top = -18
	top_plate.offset_bottom = 18
	top_plate.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	top_plate.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_COVERED
	top_plate.texture = load(texture_path)
	top_plate.modulate = Color(0.95, 0.9, 0.82, 0.28)
	top_plate.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(top_plate)

	var side_plate = TextureRect.new()
	side_plate.anchor_left = 0.55
	side_plate.anchor_right = 1.0
	side_plate.anchor_top = 0.08
	side_plate.anchor_bottom = 0.7
	side_plate.offset_left = -20
	side_plate.offset_right = 40
	side_plate.offset_top = -20
	side_plate.offset_bottom = 20
	side_plate.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	side_plate.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_COVERED
	side_plate.texture = load(texture_path)
	side_plate.modulate = Color(0.88, 0.78, 0.68, 0.18)
	side_plate.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(side_plate)

	var readability = ColorRect.new()
	readability.set_anchors_preset(Control.PRESET_FULL_RECT)
	readability.color = Color(0.015, 0.012, 0.02, 0.28)
	readability.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(readability)

	var horizon = ColorRect.new()
	horizon.anchor_left = 0.0
	horizon.anchor_right = 1.0
	horizon.anchor_top = 0.53
	horizon.anchor_bottom = 0.62
	horizon.color = Color(0.0, 0.0, 0.0, 0.24)
	horizon.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(horizon)

func _build_hybrid_depth_stage() -> void:
	var profile := HybridDepthStage.profile_from_scene(BattleManager.return_scene)
	_hybrid_depth_stage = HybridDepthStage.create_stage(profile, HybridDepthStage.StageMode.BATTLE)
	_hybrid_depth_stage.name = "HybridDepthStage"
	# S211: 3D 무대가 실제로 보이는 밝기.
	# 0.36에서는 저폴리 기둥 몇 개가 어두운 배경에 묻혀 3D가 켜져 있다는 사실조차
	# 알 수 없었다. 이제 바닥과 깊이 레이어가 무대 공간을 실제로 만들어 준다.
	_hybrid_depth_stage.modulate.a = 0.72 if OptionsMenu.is_clean_gameplay_visuals() else 0.82
	add_child(_hybrid_depth_stage)

## S53: 전투 배경 패럴랙스 레이어
func _add_battle_parallax() -> void:
	if OptionsMenu.is_clean_gameplay_visuals():
		_battle_parallax_layers.clear()
		return
	# Layer 1: 먼 실루엣 (느린 이동)
	var far_layer = ColorRect.new()
	far_layer.set_anchors_preset(Control.PRESET_FULL_RECT)
	far_layer.color = Color(0.05, 0.03, 0.08, 0.08)
	far_layer.z_index = -2
	far_layer.set_meta("parallax_speed", 0.3)
	far_layer.set_meta("base_x", 0.0)
	far_layer.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(far_layer)
	move_child(far_layer, 0)

	# Layer 2: 안개/먼지 (중간 이동)
	var mid_layer = ColorRect.new()
	mid_layer.set_anchors_preset(Control.PRESET_FULL_RECT)
	mid_layer.color = Color(0.1, 0.08, 0.12, 0.045)
	mid_layer.z_index = -1
	mid_layer.set_meta("parallax_speed", 0.8)
	mid_layer.set_meta("base_x", 0.0)
	mid_layer.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(mid_layer)
	move_child(mid_layer, 1)

	_battle_parallax_layers = [far_layer, mid_layer]

## ===================== Auto Battle (S55) =====================

func _add_premium_battle_lens() -> void:
	if OptionsMenu.is_clean_gameplay_visuals():
		return
	var lens = ColorRect.new()
	lens.set_anchors_preset(Control.PRESET_FULL_RECT)
	lens.color = Color(0, 0, 0, 0)
	lens.mouse_filter = Control.MOUSE_FILTER_IGNORE
	lens.z_index = 2
	var shader_path = "res://assets/shaders/premium_lens.gdshader"
	if ResourceLoader.exists(shader_path):
		var mat = ShaderMaterial.new()
		mat.shader = load(shader_path)
		var enemy_name: String = BattleManager.current_enemy.name.to_lower() if BattleManager.current_enemy else ""
		var tint := Color(0.78, 0.58, 0.34, 1.0)
		if "void" in enemy_name or "shade" in enemy_name or "kairos" in enemy_name:
			tint = Color(0.46, 0.30, 0.82, 1.0)
		mat.set_shader_parameter("tint_color", tint)
		mat.set_shader_parameter("vignette_strength", 0.28)
		mat.set_shader_parameter("tint_strength", 0.028)
		mat.set_shader_parameter("grain_strength", 0.006)
		mat.set_shader_parameter("letterbox_strength", 0.10)
		mat.set_shader_parameter("shaft_strength", 0.035)
		lens.material = mat
	add_child(lens)

	var top_rule = ColorRect.new()
	top_rule.anchor_left = 0.0
	top_rule.anchor_right = 1.0
	top_rule.anchor_top = 0.0
	top_rule.anchor_bottom = 0.0
	top_rule.offset_top = 82
	top_rule.offset_bottom = 84
	top_rule.color = Color(0.95, 0.72, 0.36, 0.18)
	top_rule.mouse_filter = Control.MOUSE_FILTER_IGNORE
	top_rule.z_index = 3
	add_child(top_rule)

## S230: 자동 전투 표시는 화면 한가운데 최상단(턴 순서 칩 자리)이 아니라
## 배속 칩 옆, 좌상단 칩 줄에 붙는다.
func _build_auto_label(root: Control) -> void:
	var chip := PanelContainer.new()
	chip.name = "AutoBattleChip"
	chip.anchor_left = 0.138
	chip.anchor_right = 0.212
	chip.anchor_top = 0.0
	chip.offset_top = HUD_CHIP_TOP
	chip.offset_bottom = HUD_CHIP_BOTTOM
	chip.mouse_filter = Control.MOUSE_FILTER_IGNORE
	chip.z_index = 63
	chip.visible = false
	var style := StyleBoxFlat.new()
	style.bg_color = Color(0.020, 0.036, 0.024, 0.96)
	style.border_color = Color(0.36, 0.78, 0.46, 0.72)
	style.set_border_width_all(1)
	style.set_corner_radius_all(3)
	style.set_content_margin_all(4)
	chip.add_theme_stylebox_override("panel", style)
	root.add_child(chip)

	auto_label = Label.new()
	auto_label.text = _bl("AUTO", "자동")
	auto_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	auto_label.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	UITheme.style_meta_label(auto_label, Color(0.52, 0.92, 0.62))
	chip.add_child(auto_label)
	# 기존 코드는 auto_label.visible을 켜고 끈다. 칩 전체가 따라오게 묶는다.
	auto_label.visibility_changed.connect(func(): chip.visible = auto_label.visible)
	auto_label.visible = false

func _make_interface_texture(path: String, alpha: float = 1.0) -> TextureRect:
	var art = TextureRect.new()
	if ResourceLoader.exists(path):
		art.texture = load(path)
		art.visible = true
	else:
		art.visible = false
	art.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	art.stretch_mode = TextureRect.STRETCH_SCALE
	art.mouse_filter = Control.MOUSE_FILTER_IGNORE
	art.modulate = Color(1.0, 0.94, 0.82, alpha)
	return art

func _make_interface_texture_region(path: String, region: Rect2, alpha: float = 1.0) -> TextureRect:
	var art := _make_interface_texture(path, alpha)
	if art.texture != null:
		var atlas := AtlasTexture.new()
		atlas.atlas = art.texture
		atlas.region = region
		art.texture = atlas
	return art

## ===================== S209: 전투 배속 =====================

## 24챕터를 재주행하거나 랜덤 인카운터를 반복할 때, 결과는 이미 아는데 연출 대기만
## 남는 구간이 있다. 배속은 판정/피해/확률을 전혀 건드리지 않고 "기다리는 시간"만 줄인다.
func _build_battle_speed_chip(root: Control) -> void:
	_battle_speed_btn = Button.new()
	_battle_speed_btn.name = "BattleSpeedChip"
	_battle_speed_btn.anchor_left = 0.014
	_battle_speed_btn.anchor_right = 0.128
	_battle_speed_btn.anchor_top = 0.0
	_battle_speed_btn.offset_top = 6
	_battle_speed_btn.offset_bottom = 32
	_battle_speed_btn.focus_mode = Control.FOCUS_NONE
	_battle_speed_btn.z_index = 63
	UITheme.style_label(_battle_speed_btn, UITheme.make_meta_font(), UITheme.SIZE_META, Color(0.90, 0.92, 1.0))
	_battle_speed_btn.add_theme_color_override("font_hover_color", Color(0.96, 0.86, 0.58))

	var style := StyleBoxFlat.new()
	style.bg_color = Color(0.024, 0.022, 0.036, 0.96)
	style.border_color = Color(0.46, 0.42, 0.56, 0.52)
	style.set_border_width_all(1)
	style.set_corner_radius_all(3)
	style.set_content_margin_all(4)
	_battle_speed_btn.add_theme_stylebox_override("normal", style)
	var hover := style.duplicate()
	hover.border_color = Color(0.92, 0.70, 0.36, 0.85)
	_battle_speed_btn.add_theme_stylebox_override("hover", hover)
	_battle_speed_btn.add_theme_stylebox_override("pressed", hover)
	_battle_speed_btn.add_theme_stylebox_override("disabled", style.duplicate())

	_battle_speed_btn.pressed.connect(_on_battle_speed_pressed)
	root.add_child(_battle_speed_btn)
	_update_battle_speed_chip()

func _update_battle_speed_chip() -> void:
	if _battle_speed_btn == null or not is_instance_valid(_battle_speed_btn):
		return
	var label := BattleManager.get_battle_speed_label()
	_battle_speed_btn.text = _bl("SPEED %s  [Tab]", "속도 %s  [Tab]") % label
	_battle_speed_btn.tooltip_text = _bl(
		"Shortens presentation waits only. Damage, hit chance, and rewards never change.",
		"연출 대기 시간만 줄입니다. 피해량, 명중, 보상은 달라지지 않습니다."
	)
	var accent := Color(0.80, 0.82, 0.90) if BattleManager.get_battle_speed() <= 1.0 else Color(0.96, 0.84, 0.52)
	_battle_speed_btn.add_theme_color_override("font_color", accent)

func _on_battle_speed_pressed() -> void:
	AudioManager.play_sfx("ui_select")
	BattleManager.cycle_battle_speed()
	_update_battle_speed_chip()
	_on_battle_log(_bl("Battle speed %s.", "전투 속도 %s.") % BattleManager.get_battle_speed_label())

func _build_tactical_objective_panel(root: Control) -> void:
	objective_panel = PanelContainer.new()
	objective_panel.name = "TacticalObjective"
	objective_panel.anchor_left = HUD_LEFT_COL_L
	objective_panel.anchor_right = HUD_LEFT_COL_R
	# S230: 목표 문장은 한 줄일 때도 두 줄일 때도 있다. 높이는 내용이 정하고,
	# 액자 그림이 그 결과를 따라오게 한다. (예전에는 액자가 고정 띠에 못 박혀 있어
	# 목표가 길어지면 패널만 아래로 자라 액자 밖으로 삐져나왔다.)
	objective_panel.anchor_top = 0.0
	objective_panel.anchor_bottom = 0.0
	objective_panel.offset_top = 68.0
	objective_panel.offset_bottom = 68.0
	objective_panel.grow_vertical = Control.GROW_DIRECTION_END
	objective_panel.mouse_filter = Control.MOUSE_FILTER_IGNORE
	objective_panel.z_index = 62
	objective_art = _make_interface_texture(UI_TACTICAL_PLATE_PATH, 0.70)
	objective_art.z_index = 61
	root.add_child(objective_art)
	objective_panel.resized.connect(_sync_objective_frame)
	objective_panel.item_rect_changed.connect(_sync_objective_frame)
	var style = StyleBoxFlat.new()
	style.bg_color = Color(0.020, 0.016, 0.026, 0.94)
	style.border_color = Color(0.82, 0.64, 0.34, 0.72)
	style.set_border_width_all(1)
	style.set_corner_radius_all(4)
	style.set_content_margin_all(10)
	objective_panel.add_theme_stylebox_override("panel", style)
	root.add_child(objective_panel)

	var box = VBoxContainer.new()
	box.add_theme_constant_override("separation", 3)
	objective_panel.add_child(box)

	objective_title_label = Label.new()
	objective_title_label.text = _bl("TACTICAL OBJECTIVE", "전술 목표")
	objective_title_label.text_overrun_behavior = TextServer.OVERRUN_TRIM_ELLIPSIS
	UITheme.style_ui_label(objective_title_label, Color(0.98, 0.78, 0.42, 1.0), UITheme.SIZE_UI)
	box.add_child(objective_title_label)

	objective_desc_label = Label.new()
	objective_desc_label.text = _bl("Awaiting encounter data...", "교전 데이터 대기 중...")
	objective_desc_label.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	UITheme.style_ui_label(objective_desc_label, UITheme.TEXT_PRIMARY, UITheme.SIZE_LABEL)
	box.add_child(objective_desc_label)

	objective_meta_label = Label.new()
	objective_meta_label.text = _bl("Resonance: Cold 0%", "공명: 냉각 0%")
	UITheme.style_meta_label(objective_meta_label, Color(0.72, 0.86, 1.0, 1.0))
	box.add_child(objective_meta_label)
	_sync_objective_frame()

## 액자 그림을 목표 카드의 실제 사각형에 맞춘다.
func _sync_objective_frame() -> void:
	if objective_panel == null or objective_art == null or not is_instance_valid(objective_art):
		return
	objective_art.set_anchors_preset(Control.PRESET_TOP_LEFT)
	objective_art.position = objective_panel.position - Vector2(12.0, 10.0)
	objective_art.size = objective_panel.size + Vector2(24.0, 20.0)

func _build_objective_briefing(root: Control) -> void:
	objective_briefing_overlay = Control.new()
	objective_briefing_overlay.name = "DirectiveBriefingOverlay"
	objective_briefing_overlay.set_anchors_preset(Control.PRESET_FULL_RECT)
	objective_briefing_overlay.mouse_filter = Control.MOUSE_FILTER_STOP
	objective_briefing_overlay.z_index = 108
	objective_briefing_overlay.visible = false
	root.add_child(objective_briefing_overlay)

	var dimmer := ColorRect.new()
	dimmer.name = "DirectiveBriefingDimmer"
	dimmer.set_anchors_preset(Control.PRESET_FULL_RECT)
	dimmer.color = Color(0.008, 0.006, 0.014, 0.78)
	dimmer.mouse_filter = Control.MOUSE_FILTER_STOP
	objective_briefing_overlay.add_child(dimmer)

	objective_briefing_panel = PanelContainer.new()
	objective_briefing_panel.name = "DirectiveBriefingPanel"
	objective_briefing_panel.set_anchors_preset(Control.PRESET_CENTER)
	objective_briefing_panel.offset_left = -300
	objective_briefing_panel.offset_right = 300
	objective_briefing_panel.offset_top = -205
	objective_briefing_panel.offset_bottom = 205
	objective_briefing_panel.mouse_filter = Control.MOUSE_FILTER_STOP
	var panel_style := StyleBoxFlat.new()
	panel_style.bg_color = Color(0.026, 0.022, 0.038, 0.97)
	panel_style.border_color = Color(0.78, 0.60, 0.30, 0.88)
	panel_style.set_border_width_all(2)
	panel_style.set_corner_radius_all(8)
	panel_style.set_content_margin_all(22)
	objective_briefing_panel.add_theme_stylebox_override("panel", panel_style)
	objective_briefing_overlay.add_child(objective_briefing_panel)

	var briefing_box := VBoxContainer.new()
	briefing_box.add_theme_constant_override("separation", 9)
	objective_briefing_panel.add_child(briefing_box)

	var title := Label.new()
	title.text = _bl("SELECT FIELD DIRECTIVE", "현장 전술 지침 선택")
	title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	title.add_theme_font_size_override("font_size", 23)
	title.add_theme_color_override("font_color", Color(0.96, 0.82, 0.50))
	briefing_box.add_child(title)

	var subtitle := Label.new()
	subtitle.text = _bl(
		"Choose how this encounter will be remembered. Complete the directive for bonus rewards.",
		"이 교전을 어떤 기록으로 남길지 선택하세요. 지침을 달성하면 추가 보상을 얻습니다."
	)
	subtitle.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	subtitle.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	subtitle.add_theme_font_size_override("font_size", 15)
	subtitle.add_theme_color_override("font_color", UITheme.TEXT_NARRATION)
	briefing_box.add_child(subtitle)

	var chain_label := Label.new()
	chain_label.name = "DirectiveChainLabel"
	chain_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	chain_label.add_theme_font_size_override("font_size", 13)
	chain_label.add_theme_color_override("font_color", Color(0.72, 0.86, 1.0))
	briefing_box.add_child(chain_label)

	objective_briefing_buttons = VBoxContainer.new()
	objective_briefing_buttons.name = "DirectiveChoiceButtons"
	objective_briefing_buttons.add_theme_constant_override("separation", 7)
	briefing_box.add_child(objective_briefing_buttons)

func _on_tactical_objective_options_changed(options: Array) -> void:
	if objective_briefing_overlay == null or objective_briefing_buttons == null:
		return
	for child in objective_briefing_buttons.get_children():
		child.queue_free()

	if options.is_empty():
		objective_briefing_overlay.visible = false
		var intro_finished := intro_overlay == null or not intro_overlay.visible
		if intro_finished and BattleManager.state == BattleManager.BattleState.PLAYER_TURN:
			action_container.visible = true
			if action_container.get_child_count() > 0:
				action_container.get_child(0).grab_focus()
			_present_opening_carryovers.call_deferred()
		return

	var chain_label := objective_briefing_panel.find_child("DirectiveChainLabel", true, false) as Label
	if chain_label:
		var chain := GameManager.get_directive_streak()
		var focus_note := _bl(" · Field Focus unlocked a third reading", " · 현장 집중이 세 번째 선택지를 열었습니다") if BattleManager.field_focus_opening else ""
		chain_label.text = _bl("Directive Chain x%d", "전술 연속 달성 x%d") % chain + focus_note

	for i in range(options.size()):
		var option: Dictionary = options[i]
		var choice := Button.new()
		choice.name = "DirectiveChoice%d" % (i + 1)
		choice.custom_minimum_size = Vector2(0, 72)
		choice.alignment = HORIZONTAL_ALIGNMENT_LEFT
		choice.add_theme_font_size_override("font_size", 15)
		choice.add_theme_color_override("font_color", UITheme.TEXT_PRIMARY)
		choice.add_theme_color_override("font_hover_color", Color(1.0, 0.90, 0.58))
		var reward_parts: Array[String] = ["+%d Grains" % int(option.get("reward_grains", 0))]
		if int(option.get("reward_heal", 0)) > 0:
			reward_parts.append("HP +%d" % int(option.get("reward_heal", 0)))
		var reward_item := String(option.get("reward_item", ""))
		if reward_item != "" and GameManager.ITEMS.has(reward_item):
			reward_parts.append(String(GameManager.ITEMS[reward_item].get("name", reward_item)))
		var shown_title := _localized_objective_title(String(option.get("title", "Objective")))
		var shown_desc := _localized_objective_desc(String(option.get("desc", "")))
		choice.text = "%d. %s\n%s  |  %s" % [i + 1, shown_title, shown_desc, " · ".join(reward_parts)]
		choice.tooltip_text = shown_desc
		var normal_style := StyleBoxFlat.new()
		normal_style.bg_color = Color(0.055, 0.048, 0.070, 0.96)
		normal_style.border_color = Color(0.42, 0.36, 0.28, 0.72)
		normal_style.set_border_width_all(1)
		normal_style.set_corner_radius_all(5)
		normal_style.set_content_margin_all(11)
		choice.add_theme_stylebox_override("normal", normal_style)
		var hover_style := normal_style.duplicate()
		hover_style.bg_color = Color(0.11, 0.085, 0.060, 0.98)
		hover_style.border_color = Color(0.92, 0.70, 0.34, 0.95)
		choice.add_theme_stylebox_override("hover", hover_style)
		choice.add_theme_stylebox_override("focus", hover_style)
		var chosen_index := i
		choice.pressed.connect(func(): _choose_tactical_objective(chosen_index))
		objective_briefing_buttons.add_child(choice)

	var half_height := 205.0 if options.size() >= 3 else 170.0
	objective_briefing_panel.offset_top = -half_height
	objective_briefing_panel.offset_bottom = half_height
	objective_briefing_overlay.visible = true
	action_container.visible = false
	if ally_cmd_container:
		ally_cmd_container.visible = false
	if tobias_cmd_container:
		tobias_cmd_container.visible = false
	if stance_container:
		stance_container.visible = false
	if elia_skill_container:
		elia_skill_container.visible = false
	objective_briefing_panel.modulate.a = 0.0
	objective_briefing_panel.scale = Vector2(0.94, 0.94)
	objective_briefing_panel.pivot_offset = Vector2(300, half_height)
	var tween := create_tween().set_parallel(true)
	tween.tween_property(objective_briefing_panel, "modulate:a", 1.0, 0.20)
	tween.tween_property(objective_briefing_panel, "scale", Vector2.ONE, 0.24).set_trans(Tween.TRANS_BACK).set_ease(Tween.EASE_OUT)
	await get_tree().process_frame
	if objective_briefing_buttons.get_child_count() > 0:
		objective_briefing_buttons.get_child(0).grab_focus()

func _choose_tactical_objective(index: int) -> void:
	if not BattleManager.select_tactical_objective(index):
		return
	AudioManager.play_sfx("ui_select")
	_show_turn_indicator(_bl("DIRECTIVE ACCEPTED", "전술 지침 수락"), Color(0.92, 0.72, 0.34))

func _on_auto_battle_changed(enabled: bool) -> void:
	if auto_label:
		auto_label.visible = enabled
	if auto_btn:
		auto_btn.text = _format_action_button(
			"auto",
			"7 · " + (_bl("AUTO: ON", "자동: 켬") if enabled else GameManager.loc("auto"))
		)

## ===================== 인트로 시스템 =====================

func _build_intro_overlay(root: Control) -> void:
	intro_overlay = ColorRect.new()
	intro_overlay.set_anchors_preset(Control.PRESET_FULL_RECT)
	intro_overlay.color = Color(0, 0, 0, 1.0)
	intro_overlay.z_index = 200
	intro_overlay.mouse_filter = Control.MOUSE_FILTER_STOP
	root.add_child(intro_overlay)

func _play_intro() -> void:
	# 액션 버튼 숨김
	action_container.visible = false

	# S212: 전투 시작 시 3D 무대를 한 번 밀어 넣어 공간을 보여 준다.
	if _hybrid_depth_stage != null and is_instance_valid(_hybrid_depth_stage):
		_hybrid_depth_stage.play_entrance()

	var enemy = BattleManager.current_enemy
	if not enemy:
		_finish_intro()
		return

	# 1단계: 검은 화면에서 적 이름 표시
	var name_display = Label.new()
	name_display.text = GameManager.localized_enemy_name(enemy.name)
	name_display.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	name_display.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	name_display.set_anchors_preset(Control.PRESET_CENTER)
	name_display.offset_left = -300
	name_display.offset_right = 300
	name_display.offset_top = -40
	name_display.offset_bottom = 40
	name_display.add_theme_font_size_override("font_size", 32)
	name_display.modulate.a = 0.0

	# 보스/공허수는 다른 색
	if enemy.is_boss:
		name_display.add_theme_color_override("font_color", Color(0.9, 0.3, 0.2))
	elif enemy.is_void_beast:
		name_display.add_theme_color_override("font_color", Color(0.6, 0.2, 0.7))
	else:
		name_display.add_theme_color_override("font_color", Color(0.85, 0.75, 0.6))

	intro_overlay.add_child(name_display)

	# 부제 (보스/공허수일 때)
	var sub_label = Label.new()
	sub_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	sub_label.set_anchors_preset(Control.PRESET_CENTER)
	sub_label.offset_left = -300
	sub_label.offset_right = 300
	sub_label.offset_top = 30
	sub_label.offset_bottom = 60
	sub_label.add_theme_font_size_override("font_size", 14)
	sub_label.modulate.a = 0.0

	if enemy.is_boss:
		sub_label.text = _bl("BOSS", "보스")
		sub_label.add_theme_color_override("font_color", Color(0.7, 0.2, 0.15))
	elif enemy.is_void_beast:
		sub_label.text = _bl("Void Beast", "보이드 비스트")
		sub_label.add_theme_color_override("font_color", Color(0.45, 0.15, 0.5))
	else:
		sub_label.text = _bl("Hostile Creature", "적대적 존재")
		sub_label.add_theme_color_override("font_color", Color(0.5, 0.45, 0.4))

	intro_overlay.add_child(sub_label)

	var art_preview: TextureRect = TextureRect.new()
	var art_path: String = _resolve_enemy_stage_art()
	if art_path != "" and ResourceLoader.exists(art_path):
		art_preview.set_anchors_preset(Control.PRESET_CENTER)
		art_preview.offset_left = -170
		art_preview.offset_right = 170
		art_preview.offset_top = -260
		art_preview.offset_bottom = -70
		art_preview.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
		art_preview.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
		art_preview.texture = load(art_path)
		art_preview.modulate = Color(1.0, 0.92, 0.78, 0.0)
		art_preview.mouse_filter = Control.MOUSE_FILTER_IGNORE
		intro_overlay.add_child(art_preview)

	var intel_label: Label = Label.new()
	intel_label.text = _format_enemy_intro_intel(enemy)
	intel_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	intel_label.set_anchors_preset(Control.PRESET_CENTER)
	intel_label.offset_left = -320
	intel_label.offset_right = 320
	intel_label.offset_top = 68
	intel_label.offset_bottom = 96
	intel_label.add_theme_font_size_override("font_size", 13)
	intel_label.add_theme_color_override("font_color", Color(0.72, 0.68, 0.56))
	intel_label.modulate.a = 0.0
	intro_overlay.add_child(intel_label)

	# 구분선 효과 (좌우로 펼쳐지는 선)
	var line_left = ColorRect.new()
	line_left.set_anchors_preset(Control.PRESET_CENTER)
	line_left.offset_left = 0
	line_left.offset_right = 0
	line_left.offset_top = 22
	line_left.offset_bottom = 24
	line_left.color = Color(0.6, 0.4, 0.3, 0.6)
	intro_overlay.add_child(line_left)

	var line_right = ColorRect.new()
	line_right.set_anchors_preset(Control.PRESET_CENTER)
	line_right.offset_left = 0
	line_right.offset_right = 0
	line_right.offset_top = 22
	line_right.offset_bottom = 24
	line_right.color = Color(0.6, 0.4, 0.3, 0.6)
	intro_overlay.add_child(line_right)

	# 애니메이션 시퀀스
	var t = create_tween()

	# 선 펼침
	t.set_parallel(true)
	t.tween_property(line_left, "offset_left", -160, 0.4).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_CUBIC)
	t.tween_property(line_right, "offset_right", 160, 0.4).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_CUBIC)

	# 이름 페이드 인
	t.tween_property(name_display, "modulate:a", 1.0, 0.3).set_delay(0.15)
	t.tween_property(sub_label, "modulate:a", 0.7, 0.3).set_delay(0.3)
	t.tween_property(art_preview, "modulate:a", 0.45, 0.35).set_delay(0.10)
	t.tween_property(intel_label, "modulate:a", 0.82, 0.3).set_delay(0.35)

	t.set_parallel(false)

	# The old sequence faded every label out one after another, turning every
	# ordinary encounter into a 3+ second interruption.  Keep the dramatic
	# identification beat, then clear all elements together.
	t.tween_interval(0.55)

	# 전체 페이드 아웃
	t.set_parallel(true)
	t.tween_property(intro_overlay, "color:a", 0.0, 0.5).set_ease(Tween.EASE_IN)
	t.tween_property(name_display, "modulate:a", 0.0, 0.34)
	t.tween_property(sub_label, "modulate:a", 0.0, 0.34)
	t.tween_property(art_preview, "modulate:a", 0.0, 0.30)
	t.tween_property(intel_label, "modulate:a", 0.0, 0.30)
	t.chain().tween_callback(_finish_intro)

func _format_enemy_intro_intel(enemy: BattleManager.Enemy) -> String:
	var parts: Array[String] = []
	if enemy.weakness != "":
		parts.append(_bl("Weak ", "약점 ") + enemy.weakness.to_upper())
	if enemy.resistance != "":
		parts.append(_bl("Resist ", "저항 ") + enemy.resistance.to_upper())
	if enemy.is_boss:
		parts.append(_bl("No Escape", "탈출 불가"))
	elif enemy.is_void_beast:
		parts.append(_bl("Void-Class", "보이드 등급"))
	if parts.is_empty():
		return _bl("Read the rhythm. Force BREAK.", "리듬을 읽고 BREAK를 강제하라.")
	return " / ".join(parts)

func _finish_intro() -> void:
	if intro_overlay:
		intro_overlay.mouse_filter = Control.MOUSE_FILTER_IGNORE
		intro_overlay.visible = false
	_update_hp_displays()
	# 약간의 딜레이 후 플레이어 턴 시작
	await BattleManager.pace_timer(0.2).timeout
	if objective_briefing_overlay and objective_briefing_overlay.visible:
		action_container.visible = false
		if objective_briefing_buttons and objective_briefing_buttons.get_child_count() > 0:
			objective_briefing_buttons.get_child(0).grab_focus()
		return
	_present_opening_carryovers()
	action_container.visible = true
	if action_container.get_child_count() > 0:
		action_container.get_child(0).grab_focus()

func _present_opening_carryovers() -> void:
	if _opening_carryovers_presented:
		return
	if intro_overlay and intro_overlay.visible:
		return
	if objective_briefing_overlay and objective_briefing_overlay.visible:
		return
	_opening_carryovers_presented = true
	if BattleManager.field_entry_mode != "neutral":
		_present_field_entry.call_deferred()
	if BattleManager.field_focus_opening:
		_present_field_focus_opening.call_deferred()

## ===================== 턴 표시 =====================

## S230: 턴/사건 배너는 그림 위에 맨 글자로 떠 있었다.
## 어두운 무대에서는 배경 대비가 구간마다 달라 "당신의 턴"이 거의 사라졌다.
## 읽을 바탕을 깔고, 파티 지시 카드 아래 빈 대역으로 내린다.
func _build_turn_label(root: Control) -> void:
	turn_banner = PanelContainer.new()
	turn_banner.name = "TurnBanner"
	turn_banner.anchor_left = 0.36
	turn_banner.anchor_right = 0.64
	turn_banner.anchor_top = 0.0
	turn_banner.anchor_bottom = 0.0
	turn_banner.offset_top = HUD_TURN_BANNER_TOP
	turn_banner.offset_bottom = HUD_TURN_BANNER_TOP
	turn_banner.grow_vertical = Control.GROW_DIRECTION_END
	turn_banner.mouse_filter = Control.MOUSE_FILTER_IGNORE
	turn_banner.z_index = 80
	turn_banner.modulate.a = 0.0
	var style := StyleBoxFlat.new()
	style.bg_color = Color(0.012, 0.010, 0.020, 0.86)
	style.border_color = Color(0.72, 0.60, 0.38, 0.62)
	style.set_border_width_all(1)
	style.set_corner_radius_all(4)
	style.content_margin_left = 14
	style.content_margin_right = 14
	style.content_margin_top = 7
	style.content_margin_bottom = 7
	turn_banner.add_theme_stylebox_override("panel", style)
	root.add_child(turn_banner)

	turn_label = Label.new()
	turn_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	turn_label.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	turn_label.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	UITheme.style_label(turn_label, UITheme.make_ui_font(), UITheme.SIZE_HEADING, Color(0.94, 0.84, 0.62))
	turn_label.mouse_filter = Control.MOUSE_FILTER_IGNORE
	turn_banner.add_child(turn_label)

## S175: 전투 UI 로케일 헬퍼, ko면 한국어, 아니면 영어.
func _bl(en: String, ko: String) -> String:
	return ko if GameManager.current_locale == "ko" else en

func _show_turn_indicator(text: String, color: Color = Color(0.94, 0.84, 0.62)) -> void:
	if turn_label == null or turn_banner == null:
		return
	turn_label.text = text
	turn_label.add_theme_color_override("font_color", color)
	# 테두리도 사건 색을 따라간다. 브레이크/연소/치명타를 색만으로도 구분한다.
	var style := turn_banner.get_theme_stylebox("panel") as StyleBoxFlat
	if style:
		style.border_color = Color(color.r, color.g, color.b, 0.66)
	turn_banner.modulate.a = 0.0
	# S209: 예전에는 여기서 position을 (0,0)으로 덮어써서, 중앙 배너로 설계된 턴 표시가
	# 매번 화면 좌상단으로 튀어나갔다. 앵커가 정한 위치를 그대로 쓴다.

	var t = create_tween()
	t.tween_property(turn_banner, "modulate:a", 1.0, 0.15)
	t.tween_interval(0.4)
	t.tween_property(turn_banner, "modulate:a", 0.0, 0.25)

## ===================== S41: 상태이상 비주얼 (적 스프라이트 틴트) =====================

var _status_tween: Tween
var _status_overlay: ColorRect  # 상태이상 오버레이 (적 스프라이트 위)

func _update_enemy_status_visual() -> void:
	if not enemy_sprite:
		return
	# 상태이상에 따라 적 스프라이트에 시각적 틴트 적용
	var has_poison = false
	var has_burn = false
	var has_weaken = false
	for entry in BattleManager.get_statuses("enemy"):
		if entry.effect == BattleManager.StatusEffect.POISON:
			has_poison = true
		elif entry.effect == BattleManager.StatusEffect.BURN:
			has_burn = true
		elif entry.effect == BattleManager.StatusEffect.WEAKEN:
			has_weaken = true

	if _status_tween and _status_tween.is_running():
		_status_tween.kill()

	if has_poison:
		# 독: 초록 틴트 맥동
		_status_tween = create_tween().set_loops()
		_status_tween.tween_property(enemy_sprite, "modulate", Color(0.6, 1.2, 0.6, 1.0), 0.5)
		_status_tween.tween_property(enemy_sprite, "modulate", Color(0.8, 1.0, 0.8, 1.0), 0.5)
	elif has_burn:
		# 화상: 주황 깜빡임
		_status_tween = create_tween().set_loops()
		_status_tween.tween_property(enemy_sprite, "modulate", Color(1.3, 0.7, 0.4, 1.0), 0.3)
		_status_tween.tween_property(enemy_sprite, "modulate", Color(1.0, 0.85, 0.7, 1.0), 0.4)
	elif has_weaken:
		# 약화: 파란 톤
		enemy_sprite.modulate = Color(0.7, 0.7, 1.2, 1.0)
	else:
		_restore_battler_base_modulate(enemy_sprite)

## ===================== S41: 콤보 버스트 VFX =====================

func _play_combo_burst(combo: int) -> void:
	if combo < 2:
		return
	var center = Vector2(400, 200)
	# 콤보 텍스트 (큰 금색)
	var lbl = Label.new()
	lbl.text = _bl("COMBO x%d!", "콤보 x%d!") % combo
	lbl.add_theme_font_size_override("font_size", 22 + combo * 2)
	lbl.add_theme_color_override("font_color", Color(1.0, 0.85, 0.2))
	lbl.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	lbl.position = center - Vector2(80, 20)
	lbl.z_index = 70
	lbl.modulate.a = 0.0
	lbl.scale = Vector2(0.5, 0.5)
	lbl.pivot_offset = Vector2(80, 20)
	canvas_root.add_child(lbl)

	var lt = create_tween().set_parallel(true)
	lt.tween_property(lbl, "modulate:a", 1.0, 0.1)
	lt.tween_property(lbl, "scale", Vector2(1.2, 1.2), 0.15).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_BACK)
	lt.chain().tween_property(lbl, "scale", Vector2(1.0, 1.0), 0.1)
	lt.chain().tween_interval(0.4)
	lt.chain().tween_property(lbl, "modulate:a", 0.0, 0.3)
	lt.chain().tween_callback(lbl.queue_free)

	# 파티클 버스트 (금색 방사형)
	for i in range(8 + combo * 2):
		var spark = ColorRect.new()
		spark.size = Vector2(4, 4)
		spark.position = center
		spark.color = Color(1.0, 0.8 + randf() * 0.2, 0.2, 0.9)
		spark.z_index = 68
		canvas_root.add_child(spark)
		var angle = randf() * TAU
		var dist = randf_range(40, 100 + combo * 15)
		var target_pos = center + Vector2(cos(angle), sin(angle)) * dist
		var st = create_tween().set_parallel(true)
		st.tween_property(spark, "position", target_pos, 0.4).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_CUBIC)
		st.tween_property(spark, "modulate:a", 0.0, 0.3).set_delay(0.15)
		st.chain().tween_callback(spark.queue_free)

## ===================== S41: 턴 순서 미리보기 =====================

var turn_preview_container: HBoxContainer

func _build_turn_preview(root: Control) -> void:
	turn_preview_container = HBoxContainer.new()
	turn_preview_container.name = "TurnOrderStrip"
	# S230: 좌상단 칩 줄과 같은 높이의 가운데 칸. 전투 큐(y 38~)와 겹치지 않는다.
	turn_preview_container.anchor_left = 0.36
	turn_preview_container.anchor_right = 0.64
	turn_preview_container.anchor_top = 0.0
	turn_preview_container.offset_top = HUD_CHIP_TOP
	turn_preview_container.offset_bottom = HUD_CHIP_BOTTOM
	turn_preview_container.alignment = BoxContainer.ALIGNMENT_CENTER
	turn_preview_container.add_theme_constant_override("separation", 6)
	turn_preview_container.mouse_filter = Control.MOUSE_FILTER_IGNORE
	root.add_child(turn_preview_container)

func _update_turn_preview() -> void:
	if not turn_preview_container:
		return
	for child in turn_preview_container.get_children():
		child.queue_free()

	# 현재 턴 + 다음 2턴 예측 (S175: 로케일 라벨)
	var ally_tag := _bl("ALLY", "아군")
	var enemy_tag := _bl("ENEMY", "적")
	var turns: Array = []
	if BattleManager.state == BattleManager.BattleState.PLAYER_TURN:
		turns = [ally_tag, enemy_tag, ally_tag]
	elif BattleManager.state == BattleManager.BattleState.ENEMY_TURN:
		turns = [enemy_tag, ally_tag, enemy_tag]
	else:
		return

	for i in range(turns.size()):
		var is_current = (i == 0)
		var lbl = Label.new()
		lbl.text = turns[i]
		var col = Color(0.72, 0.84, 1.0) if turns[i] == ally_tag else Color(1.0, 0.60, 0.52)
		if not is_current:
			col = col.darkened(0.24)
		UITheme.style_meta_label(lbl, col, UITheme.SIZE_LABEL if is_current else UITheme.SIZE_META)
		lbl.mouse_filter = Control.MOUSE_FILTER_IGNORE

		var panel = PanelContainer.new()
		var style = StyleBoxFlat.new()
		style.bg_color = Color(0.05, 0.04, 0.07, 0.86 if is_current else 0.62)
		style.border_color = col * Color(1, 1, 1, 0.6 if is_current else 0.28)
		style.set_border_width_all(1)
		style.set_corner_radius_all(3)
		style.set_content_margin_all(4)
		panel.add_theme_stylebox_override("panel", style)
		panel.mouse_filter = Control.MOUSE_FILTER_IGNORE
		panel.add_child(lbl)
		turn_preview_container.add_child(panel)

		if is_current:
			# 현재 턴 표시자에 화살표
			var arrow = Label.new()
			arrow.text = ">"
			arrow.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
			UITheme.style_meta_label(arrow, col)
			arrow.mouse_filter = Control.MOUSE_FILTER_IGNORE
			turn_preview_container.add_child(arrow)

## ===================== 상태 아이콘 =====================

## S230: 상태 칩은 더 이상 떠다니지 않는다.
## 적 칩은 적 판독 카드 안, 아렐 칩은 아렐 상태 묶음 안에서 만들어진다.
## (예전에는 각각 적 브레이크 게이지와 커맨드 덱 장식 위에 겹쳐 그려졌다.)
func _build_player_status(_root: Control) -> void:
	if player_status_container != null or _player_readout_column == null:
		return
	player_status_container = _make_status_flow()
	_player_readout_column.add_child(player_status_container)

func _update_status_icons() -> void:
	# 적 상태 아이콘
	if enemy_status_container:
		for child in enemy_status_container.get_children():
			enemy_status_container.remove_child(child)
			child.queue_free()

		var enemy = BattleManager.current_enemy
		if enemy:
			if BattleManager.enemy_shielded:
				_add_status_icon(enemy_status_container, _bl("SHIELD", "보호막"), Color(0.42, 0.62, 0.92, 0.95))
			if enemy.is_boss and enemy.phase > 1:
				_add_status_icon(enemy_status_container, _bl("PHASE %d", "%d단계") % enemy.phase, Color(0.94, 0.42, 0.30, 0.95))
			if enemy.is_void_beast:
				_add_status_icon(enemy_status_container, _bl("VOID", "보이드"), Color(0.72, 0.40, 0.90, 0.95))
			if BattleManager.enemy_broken_turns > 0:
				_add_status_icon(enemy_status_container, _bl("BROKEN", "붕괴"), Color(1.0, 0.68, 0.18, 0.95))
			# 약점/저항 표시, Ash Sight 패시브 또는 스캔된 적만 상세 표시
			var show_details = MemoryManager.has_passive("ash_sight") or enemy.name in BattleManager.scanned_enemies
			if show_details:
				if enemy.weakness != "":
					var w_color = _get_element_color(enemy.weakness)
					_add_status_icon(enemy_status_container, _bl("Weak %s", "약점 %s") % _localized_element(enemy.weakness), w_color)
				if enemy.resistance != "":
					var r_color = _get_element_color(enemy.resistance)
					_add_status_icon(enemy_status_container, _bl("Resist %s", "저항 %s") % _localized_element(enemy.resistance), r_color)
			# 적 상태이상
			for entry in BattleManager.get_statuses("enemy"):
				var info = _get_status_display(entry.effect)
				_add_status_icon(enemy_status_container, "%s %d" % [info.text, entry.turns_left], info.color)

	# 플레이어 상태 아이콘
	if player_status_container:
		for child in player_status_container.get_children():
			player_status_container.remove_child(child)
			child.queue_free()

		for entry in BattleManager.get_statuses("player"):
			var info = _get_status_display(entry.effect)
			_add_status_icon(player_status_container, "%s %d" % [info.text, entry.turns_left], info.color)

		# 콤보 표시
		if BattleManager.combo_count >= 2:
			_add_status_icon(player_status_container, _bl("COMBO x%d", "연계 x%d") % BattleManager.combo_count, Color(0.96, 0.78, 0.28, 0.95))

		# 동행 표시
		if BattleManager.sable_in_party:
			_add_status_icon(player_status_container, GameManager.localized_speaker("Sable"), Color(0.62, 0.72, 0.92, 0.9))
		if BattleManager.tobias_in_party:
			_add_status_icon(player_status_container, GameManager.localized_speaker("Tobias"), Color(0.90, 0.80, 0.62, 0.9))
		if BattleManager.player_defending:
			_add_status_icon(player_status_container, _bl("GUARD", "방어"), Color(0.62, 0.80, 0.98, 0.95))

## 속성 이름은 전투 내내 같은 말로 나와야 한다. (약점 칩 / 판독 리본 / 도감)
func _localized_element(element: String) -> String:
	if GameManager.current_locale != "ko":
		return element.to_upper()
	match element.to_lower():
		"fire": return "화염"
		"void": return "보이드"
		"physical": return "물리"
	return element.to_upper()

func _get_status_display(effect: int) -> Dictionary:
	if effect == BattleManager.StatusEffect.POISON:
		return {"text": _bl("POISON", "중독"), "color": Color(0.44, 0.84, 0.34, 0.95)}
	elif effect == BattleManager.StatusEffect.WEAKEN:
		return {"text": _bl("WEAK", "약화"), "color": Color(0.88, 0.68, 0.32, 0.95)}
	elif effect == BattleManager.StatusEffect.BURN:
		return {"text": _bl("BURN", "화상"), "color": Color(0.98, 0.54, 0.22, 0.95)}
	return {"text": "???", "color": Color(0.70, 0.70, 0.70, 0.95)}

func _add_status_icon(container: Container, text: String, color: Color) -> void:
	var label = Label.new()
	label.text = text
	UITheme.style_meta_label(label, color)

	var panel = PanelContainer.new()
	panel.mouse_filter = Control.MOUSE_FILTER_IGNORE
	var style = StyleBoxFlat.new()
	style.bg_color = Color(0.025, 0.022, 0.034, 0.94)
	style.border_color = Color(color.r, color.g, color.b, 0.72)
	style.set_border_width_all(1)
	style.set_corner_radius_all(3)
	style.content_margin_left = 6
	style.content_margin_right = 6
	style.content_margin_top = 2
	style.content_margin_bottom = 2
	panel.add_theme_stylebox_override("panel", style)
	panel.add_child(label)
	container.add_child(panel)

## ===================== UI 패널 빌드 =====================

## S230: HP 막대 뒤에 깔리는 "최근 피해" 잔상.
## 앞 막대는 바로 줄고, 잔상은 잠깐 머물다 따라 내려간다.
## 숫자를 못 보는 상태(미판독 적)에서도 이번 타격이 얼마나 들어갔는지 읽힌다.
func _make_hp_bar_stack(height: float, fill_color: Color, ghost_color: Color, track_color: Color) -> Dictionary:
	var stack := Control.new()
	stack.custom_minimum_size = Vector2(0, height)
	stack.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	stack.mouse_filter = Control.MOUSE_FILTER_IGNORE

	var ghost := ProgressBar.new()
	ghost.set_anchors_preset(Control.PRESET_FULL_RECT)
	ghost.show_percentage = false
	ghost.mouse_filter = Control.MOUSE_FILTER_IGNORE
	var ghost_fill := StyleBoxFlat.new()
	ghost_fill.bg_color = ghost_color
	ghost_fill.set_corner_radius_all(3)
	ghost.add_theme_stylebox_override("fill", ghost_fill)
	var track := StyleBoxFlat.new()
	track.bg_color = track_color
	track.border_color = Color(0.0, 0.0, 0.0, 0.55)
	track.set_border_width_all(1)
	track.set_corner_radius_all(3)
	ghost.add_theme_stylebox_override("background", track)
	stack.add_child(ghost)

	var bar := ProgressBar.new()
	bar.set_anchors_preset(Control.PRESET_FULL_RECT)
	bar.show_percentage = false
	bar.mouse_filter = Control.MOUSE_FILTER_IGNORE
	var fill := StyleBoxFlat.new()
	fill.bg_color = fill_color
	fill.set_corner_radius_all(3)
	bar.add_theme_stylebox_override("fill", fill)
	bar.add_theme_stylebox_override("background", StyleBoxEmpty.new())
	stack.add_child(bar)

	return {"stack": stack, "bar": bar, "ghost": ghost}

func _make_status_flow(separation: int = 5) -> HFlowContainer:
	var flow := HFlowContainer.new()
	flow.add_theme_constant_override("h_separation", separation)
	flow.add_theme_constant_override("v_separation", 3)
	return flow

func _build_enemy_panel(root: Control) -> void:
	# 오른쪽 열 전체를 하나의 판독 카드로 묶는다. 높이는 내용이 정하고,
	# 위에서 아래로만 자라므로 적 판(y 115~)을 밀고 들어가지 않는다.
	enemy_panel = PanelContainer.new()
	enemy_panel.name = "EnemyReadout"
	enemy_panel.anchor_left = HUD_RIGHT_COL_L
	enemy_panel.anchor_right = HUD_RIGHT_COL_R
	enemy_panel.anchor_top = 0.0
	enemy_panel.anchor_bottom = 0.0
	enemy_panel.offset_top = HUD_ENEMY_TOP
	enemy_panel.offset_bottom = HUD_ENEMY_TOP
	enemy_panel.grow_vertical = Control.GROW_DIRECTION_END
	enemy_panel.mouse_filter = Control.MOUSE_FILTER_IGNORE

	var style = StyleBoxFlat.new()
	style.bg_color = Color(0.040, 0.022, 0.026, 0.93)
	style.border_color = Color(0.72, 0.30, 0.28, 0.66)
	style.set_border_width_all(1)
	style.set_corner_radius_all(4)
	style.set_content_margin_all(8)
	enemy_panel.add_theme_stylebox_override("panel", style)
	root.add_child(enemy_panel)

	var vbox = VBoxContainer.new()
	vbox.add_theme_constant_override("separation", 4)
	enemy_panel.add_child(vbox)

	# 1행: 이름 + HP 수치. 긴 이름은 잘리지 않고 말줄임으로 접힌다.
	var name_row := HBoxContainer.new()
	name_row.add_theme_constant_override("separation", 8)
	vbox.add_child(name_row)

	enemy_name_label = Label.new()
	enemy_name_label.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	enemy_name_label.text_overrun_behavior = TextServer.OVERRUN_TRIM_ELLIPSIS
	UITheme.style_label(enemy_name_label, UITheme.make_ui_font(), UITheme.SIZE_UI, Color(1.0, 0.66, 0.58))
	name_row.add_child(enemy_name_label)

	enemy_hp_label = Label.new()
	enemy_hp_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	enemy_hp_label.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	UITheme.style_ui_label(enemy_hp_label, Color(0.98, 0.82, 0.78), UITheme.SIZE_LABEL)
	name_row.add_child(enemy_hp_label)

	# 2행: HP 막대 + 피해 잔상.
	var hp_stack := _make_hp_bar_stack(18.0, UITheme.HP_ENEMY, Color(0.96, 0.62, 0.50, 0.62), Color(0.10, 0.055, 0.055, 0.94))
	enemy_hp_bar = hp_stack.bar
	enemy_hp_ghost = hp_stack.ghost
	vbox.add_child(hp_stack.stack)

	# 3행: 브레이크 라벨 + 게이지 + 판독 상태. 세 줄을 한 줄로 접었다.
	var break_row := HBoxContainer.new()
	break_row.add_theme_constant_override("separation", 6)
	vbox.add_child(break_row)

	enemy_break_label = Label.new()
	enemy_break_label.text = _bl("BREAK", "브레이크")
	enemy_break_label.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	enemy_break_label.custom_minimum_size = Vector2(62, 0)
	UITheme.style_meta_label(enemy_break_label, Color(0.98, 0.80, 0.48))
	break_row.add_child(enemy_break_label)

	enemy_break_bar = ProgressBar.new()
	enemy_break_bar.custom_minimum_size = Vector2(0, 10)
	enemy_break_bar.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	enemy_break_bar.size_flags_vertical = Control.SIZE_SHRINK_CENTER
	enemy_break_bar.show_percentage = false
	enemy_break_bar.mouse_filter = Control.MOUSE_FILTER_IGNORE
	enemy_break_bar.max_value = BattleManager.BREAK_MAX
	enemy_break_bar.value = BattleManager.enemy_break_gauge
	var break_fill = StyleBoxFlat.new()
	break_fill.bg_color = Color(0.95, 0.62, 0.18)
	break_fill.set_corner_radius_all(2)
	enemy_break_bar.add_theme_stylebox_override("fill", break_fill)
	var break_bg = StyleBoxFlat.new()
	break_bg.bg_color = Color(0.12, 0.09, 0.06, 0.94)
	break_bg.border_color = Color(0.0, 0.0, 0.0, 0.5)
	break_bg.set_border_width_all(1)
	break_bg.set_corner_radius_all(2)
	enemy_break_bar.add_theme_stylebox_override("background", break_bg)
	break_row.add_child(enemy_break_bar)

	enemy_scan_chip = Label.new()
	enemy_scan_chip.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	enemy_scan_chip.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	enemy_scan_chip.custom_minimum_size = Vector2(58, 0)
	UITheme.style_meta_label(enemy_scan_chip, Color(0.66, 0.72, 0.82))
	break_row.add_child(enemy_scan_chip)

	# 4행: 상태 칩. 개수에 따라 줄바꿈되고, 없으면 높이 0으로 접힌다.
	enemy_status_container = _make_status_flow()
	vbox.add_child(enemy_status_container)

## S44: 전투 지면 (그라운드 플랫폼, 원근감)
## S209: 배경과 무대 사이의 공기 원근.
## 화면 위쪽(먼 배경)일수록 짙게, 전투원이 서는 대역은 그대로 두어
## 배경 일러스트를 지우지 않으면서도 깊이를 만든다.
func _add_background_recession() -> void:
	var steps: int = 5
	for i in range(steps):
		var t: float = float(i) / float(steps)
		var band = ColorRect.new()
		band.anchor_left = 0.0
		band.anchor_right = 1.0
		band.anchor_top = 0.0 + 0.40 * t
		band.anchor_bottom = 0.0 + 0.40 * (t + 1.0 / float(steps))
		band.color = Color(0.020, 0.018, 0.032, 0.30 - 0.05 * t)
		band.mouse_filter = Control.MOUSE_FILTER_IGNORE
		band.z_index = -8
		add_child(band)

## S209: 무대 바닥.
## 예전 구현은 화면 폭 전체를 가로지르는 단색 띠 + 2px 밝은 선이라 배경 그림 위에
## 자를 대고 그은 UI 이음매처럼 보였고, 전투원들은 그 선 위에 떠 있었다.
## 이제는 위로 갈수록 투명해지는 다층 그라데이션과 전투원 발밑의 타원 무대광으로
## 지면을 암시한다. 직선 경계선은 없다.
func _build_battle_ground() -> void:
	_ground_rect = ColorRect.new()
	_ground_rect.anchor_left = 0.0
	_ground_rect.anchor_right = 1.0
	_ground_rect.anchor_top = STAGE_FLOOR_ANCHOR
	_ground_rect.anchor_bottom = 1.0
	# S211: 3D 무대가 지면을 담당하므로, 2D 바닥 띠는 대비를 잡아 주는 정도로만 남긴다.
	# 예전 값(0.62)은 3D 바닥의 원근 격자를 그대로 덮어 버렸다.
	_ground_rect.color = Color(0.035, 0.030, 0.052, 0.34)
	_ground_rect.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(_ground_rect)

	# 지면 진입부: 위(먼 곳)에서 아래(가까운 곳)로 점점 짙어지는 6단 페이드.
	# 단색 띠의 윗변을 지워 배경 일러스트와 자연스럽게 이어지게 한다.
	var fade_steps: int = 6
	for i in range(fade_steps):
		var t: float = float(i) / float(fade_steps)
		var band = ColorRect.new()
		band.anchor_left = 0.0
		band.anchor_right = 1.0
		band.anchor_top = STAGE_FLOOR_ANCHOR - 0.085 + 0.085 * t
		band.anchor_bottom = STAGE_FLOOR_ANCHOR - 0.085 + 0.085 * (t + 1.0 / float(fade_steps))
		band.color = Color(0.035, 0.030, 0.052, 0.10 + 0.09 * t)
		band.mouse_filter = Control.MOUSE_FILTER_IGNORE
		add_child(band)

	# 무대 중앙 광원: 전투원이 서는 지면 대역만 아주 옅게 들어올린다.
	var stage_pool = _make_battle_ellipse(
		Vector2(640, STAGE_BASELINE_Y + 6.0),
		Vector2(560, 54),
		Color(0.30, 0.27, 0.38, 0.055),
		48
	)
	stage_pool.z_index = -1
	add_child(stage_pool)

## S44: 플레이어 스프라이트 (왼쪽, 사이드뷰)
func _build_battle_stage_art(root: Control) -> void:
	var left_path := "res://assets/cg/generated/illustration_expansion_v2/world_rewrite_first_sword_v3.png"
	var right_path := _resolve_enemy_stage_art()

	_battle_stage_left_wash = _make_stage_wash(true)
	root.add_child(_battle_stage_left_wash)
	_battle_stage_left = _make_stage_art_rect(true)
	if ResourceLoader.exists(left_path):
		_battle_stage_left.texture = load(left_path)
		_battle_stage_left.visible = true
	root.add_child(_battle_stage_left)

	_battle_stage_right_wash = _make_stage_wash(false)
	root.add_child(_battle_stage_right_wash)
	_battle_stage_right = _make_stage_art_rect(false)
	if right_path != "" and ResourceLoader.exists(right_path):
		_battle_stage_right.texture = load(right_path)
		_battle_stage_right.visible = true
	root.add_child(_battle_stage_right)

func _make_stage_art_rect(is_left: bool) -> TextureRect:
	var rect = TextureRect.new()
	rect.anchor_top = 0.05
	rect.anchor_bottom = 0.70
	if is_left:
		rect.anchor_left = -0.02
		rect.anchor_right = 0.40
	else:
		rect.anchor_left = 0.60
		rect.anchor_right = 1.02
	rect.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	rect.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_COVERED
	rect.mouse_filter = Control.MOUSE_FILTER_IGNORE
	# S215: 무대 양옆 배경판도 큰 원화다. 기본 Nearest로 축소하면 가장자리가 지직거린다.
	rect.texture_filter = CanvasItem.TEXTURE_FILTER_LINEAR_WITH_MIPMAPS
	rect.modulate = Color(1.0, 0.92, 0.82, 0.12 if OptionsMenu.is_clean_gameplay_visuals() else 0.15)
	var blend_shader_path := "res://assets/shaders/battle_stage_blend.gdshader"
	if ResourceLoader.exists(blend_shader_path):
		var blend_material := ShaderMaterial.new()
		blend_material.shader = load(blend_shader_path)
		rect.material = blend_material
	rect.z_index = -4
	rect.visible = false
	return rect

func _make_stage_wash(is_left: bool) -> ColorRect:
	var wash = ColorRect.new()
	wash.anchor_top = 0.08
	wash.anchor_bottom = 0.72
	if is_left:
		wash.anchor_left = 0.0
		wash.anchor_right = 0.43
	else:
		wash.anchor_left = 0.57
		wash.anchor_right = 1.0
	wash.color = Color(0.0, 0.0, 0.0, 0.18)
	wash.mouse_filter = Control.MOUSE_FILTER_IGNORE
	wash.z_index = -5
	return wash

func _resolve_enemy_stage_art() -> String:
	if BattleManager.enemy_image != "" and ResourceLoader.exists(BattleManager.enemy_image):
		return BattleManager.enemy_image
	var enemy_name: String = BattleManager.current_enemy.name if BattleManager.current_enemy else ""
	var named_art: String = _resolve_enemy_art_by_name(enemy_name)
	if named_art != "":
		return named_art
	return _resolved_battle_bg_image

func _resolve_enemy_action_cutin(enemy_name: String) -> String:
	var lower_name := enemy_name.to_lower()
	for enemy_key: String in ENEMY_ACTION_CUTIN_PATHS:
		if enemy_key in lower_name:
			var cutin_path := String(ENEMY_ACTION_CUTIN_PATHS[enemy_key])
			if ResourceLoader.exists(cutin_path):
				return cutin_path
	if "void beast" in lower_name and ResourceLoader.exists(VOID_BEAST_ACTION_CUTIN_PATH):
		return VOID_BEAST_ACTION_CUTIN_PATH
	if "shade sentinel" in lower_name:
		return String(BOSS_PHASE_CUTIN_PATHS.get("Shade Sentinel", _resolve_enemy_stage_art()))
	if "echo shell" in lower_name and ResourceLoader.exists(ECHO_SHELL_ACTION_CUTIN_PATH):
		return ECHO_SHELL_ACTION_CUTIN_PATH
	return _resolve_enemy_stage_art()

func _resolve_enemy_art_by_name(enemy_name: String) -> String:
	return BattleManager.resolve_enemy_image_by_name(enemy_name)

func _build_action_cutin(root: Control) -> void:
	_action_cutin_wash = ColorRect.new()
	_action_cutin_wash.set_anchors_preset(Control.PRESET_FULL_RECT)
	_action_cutin_wash.color = Color(0.0, 0.0, 0.0, 0.0)
	_action_cutin_wash.mouse_filter = Control.MOUSE_FILTER_IGNORE
	_action_cutin_wash.z_index = 70
	_action_cutin_wash.visible = false
	root.add_child(_action_cutin_wash)

	_action_cutin = TextureRect.new()
	_action_cutin.anchor_top = 0.08
	_action_cutin.anchor_bottom = 0.62
	_action_cutin.anchor_left = 0.08
	_action_cutin.anchor_right = 0.92
	_action_cutin.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	_action_cutin.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_COVERED
	_action_cutin.mouse_filter = Control.MOUSE_FILTER_IGNORE
	_action_cutin.modulate = Color(1.0, 0.92, 0.78, 0.0)
	_action_cutin.z_index = 71
	_action_cutin.visible = false
	root.add_child(_action_cutin)

## A compact, persistent beat card turns battle information into a readable
## cadence: threat -> response -> consequence. It remains visible in clean view.
func _build_combat_cue_panel(root: Control) -> void:
	combat_cue_panel = PanelContainer.new()
	combat_cue_panel.name = "CombatCue"
	# S226: the approach banner now carries a second line with the concrete
	# opening it bought, so the cue frame has to hold two lines without clipping.
	# S230: 예전에는 오른쪽 끝(0.68 -> x 870)이 적 판독 카드(x 845~) 위로 넘어가
	# 적 이름 첫 글자를 덮었다("그림자 파수꾼" -> "림자 파수꾼").
	# 이제 가운데 열 안에 머물고, 높이는 내용이 정한다.
	combat_cue_panel.anchor_left = HUD_CENTER_COL_L
	combat_cue_panel.anchor_right = HUD_CENTER_COL_R
	combat_cue_panel.anchor_top = 0.0
	combat_cue_panel.anchor_bottom = 0.0
	combat_cue_panel.offset_top = HUD_CUE_TOP
	combat_cue_panel.offset_bottom = HUD_CUE_TOP
	combat_cue_panel.grow_vertical = Control.GROW_DIRECTION_END
	combat_cue_panel.mouse_filter = Control.MOUSE_FILTER_IGNORE
	combat_cue_panel.z_index = 66
	combat_cue_panel.visible = false
	combat_cue_panel.modulate = Color(1.0, 1.0, 1.0, 0.0)
	var style := StyleBoxFlat.new()
	style.bg_color = Color(0.016, 0.022, 0.038, 0.97)
	style.border_color = Color(0.54, 0.72, 0.96, 0.76)
	style.set_border_width_all(1)
	style.set_corner_radius_all(5)
	style.set_content_margin_all(6)
	combat_cue_panel.add_theme_stylebox_override("panel", style)
	root.add_child(combat_cue_panel)

	var row := HBoxContainer.new()
	row.add_theme_constant_override("separation", 8)
	combat_cue_panel.add_child(row)
	combat_cue_art = TextureRect.new()
	combat_cue_art.custom_minimum_size = Vector2(84, 46)
	combat_cue_art.size_flags_vertical = Control.SIZE_SHRINK_CENTER
	combat_cue_art.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	combat_cue_art.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_COVERED
	combat_cue_art.mouse_filter = Control.MOUSE_FILTER_IGNORE
	row.add_child(combat_cue_art)

	var copy := VBoxContainer.new()
	copy.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	copy.size_flags_vertical = Control.SIZE_SHRINK_CENTER
	copy.add_theme_constant_override("separation", 2)
	row.add_child(copy)
	combat_cue_title = Label.new()
	UITheme.style_meta_label(combat_cue_title, Color(0.80, 0.90, 1.0, 0.96))
	combat_cue_title.text_overrun_behavior = TextServer.OVERRUN_TRIM_ELLIPSIS
	copy.add_child(combat_cue_title)
	combat_cue_detail = Label.new()
	UITheme.style_ui_label(combat_cue_detail, UITheme.TEXT_PRIMARY, UITheme.SIZE_LABEL)
	combat_cue_detail.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	combat_cue_detail.max_lines_visible = 3
	copy.add_child(combat_cue_detail)

func _show_combat_cue(title: String, detail: String, art_path: String, accent: Color, hold_time: float = 1.1) -> void:
	if combat_cue_panel == null or combat_cue_title == null or combat_cue_detail == null:
		return
	if _combat_cue_tween:
		_combat_cue_tween.kill()
	combat_cue_title.text = title
	combat_cue_detail.text = detail
	combat_cue_title.add_theme_color_override("font_color", accent)
	var style := combat_cue_panel.get_theme_stylebox("panel") as StyleBoxFlat
	if style:
		style.border_color = Color(accent.r, accent.g, accent.b, 0.78)
	if combat_cue_art:
		combat_cue_art.texture = load(art_path) if art_path != "" and ResourceLoader.exists(art_path) else null
		combat_cue_art.visible = combat_cue_art.texture != null
	combat_cue_panel.visible = true
	combat_cue_panel.modulate = Color(1.0, 1.0, 1.0, 0.0)
	combat_cue_panel.scale = Vector2(0.96, 0.96)
	_combat_cue_tween = create_tween()
	_combat_cue_tween.set_parallel(true)
	_combat_cue_tween.tween_property(combat_cue_panel, "modulate:a", 1.0, 0.12)
	_combat_cue_tween.tween_property(combat_cue_panel, "scale", Vector2.ONE, 0.16).set_trans(Tween.TRANS_CUBIC).set_ease(Tween.EASE_OUT)
	_combat_cue_tween.set_parallel(false)
	_combat_cue_tween.tween_interval(hold_time)
	_combat_cue_tween.tween_property(combat_cue_panel, "modulate:a", 0.0, 0.22)
	_combat_cue_tween.tween_callback(func():
		if is_instance_valid(combat_cue_panel):
			combat_cue_panel.visible = false
	)

func _play_action_cutin(path: String, from_left: bool = true, alpha: float = 0.72, hold_time: float = 0.16) -> void:
	if path == "" or not ResourceLoader.exists(path) or _action_cutin == null:
		return
	if _action_cutin_tween:
		_action_cutin_tween.kill()
	_action_cutin.texture = load(path)
	_action_cutin.visible = true
	_action_cutin_wash.visible = true
	_action_cutin.modulate.a = 0.0
	_action_cutin_wash.color.a = 0.0
	_action_cutin.position.x = -36.0 if from_left else 36.0
	_action_cutin_tween = create_tween()
	_action_cutin_tween.set_pause_mode(Tween.TWEEN_PAUSE_PROCESS)
	_action_cutin_tween.set_parallel(true)
	_action_cutin_tween.tween_property(_action_cutin_wash, "color:a", 0.32, 0.10)
	_action_cutin_tween.tween_property(_action_cutin, "modulate:a", alpha, 0.12)
	_action_cutin_tween.tween_property(_action_cutin, "position:x", 0.0, 0.16).set_trans(Tween.TRANS_CUBIC).set_ease(Tween.EASE_OUT)
	_action_cutin_tween.set_parallel(false)
	_action_cutin_tween.tween_interval(hold_time)
	_action_cutin_tween.set_parallel(true)
	_action_cutin_tween.tween_property(_action_cutin, "modulate:a", 0.0, 0.18)
	_action_cutin_tween.tween_property(_action_cutin_wash, "color:a", 0.0, 0.20)
	_action_cutin_tween.set_parallel(false)
	_action_cutin_tween.tween_callback(func():
		if is_instance_valid(_action_cutin):
			_action_cutin.visible = false
		if is_instance_valid(_action_cutin_wash):
			_action_cutin_wash.visible = false
	)

func _resolve_memory_burn_cutin(memory_id: String, memory_title: String) -> String:
	var clean_id := memory_id.strip_edges()
	if MEMORY_BURN_CUTIN_PATHS.has(clean_id):
		var exact_path := String(MEMORY_BURN_CUTIN_PATHS[clean_id])
		if ResourceLoader.exists(exact_path):
			return exact_path

	var lookup := ("%s %s" % [clean_id, memory_title]).to_lower()
	var fallback_path := ""
	if lookup.find("sword") >= 0:
		fallback_path = "res://assets/cg/generated/illustration_expansion_v2/world_rewrite_first_sword_v3.png"
	elif lookup.find("song") >= 0 or lookup.find("campfire") >= 0:
		fallback_path = "res://assets/cg/generated/illustration_expansion_v2/world_rewrite_campfire_song_v3.png"
	elif lookup.find("hand") >= 0 or lookup.find("reach") >= 0:
		fallback_path = "res://assets/cg/generated/illustration_expansion_v2/world_rewrite_reaching_hand_v3.png"
	elif lookup.find("name") >= 0 or lookup.find("arrel") >= 0 or lookup.find("origin") >= 0:
		fallback_path = "res://assets/cg/generated/illustration_expansion_v2/world_rewrite_name_origin_v3.png"
	elif lookup.find("compass") >= 0:
		fallback_path = "res://assets/cg/generated/memory_burn_compass.png"
	elif lookup.find("void") >= 0 or lookup.find("bl-07") >= 0 or lookup.find("bl07") >= 0:
		fallback_path = "res://assets/cg/generated/memory_burn_void_walker.png"

	if fallback_path != "" and ResourceLoader.exists(fallback_path):
		return fallback_path
	return ""

func _play_memory_burn_cutin(cutin_path: String, memory_grade: int) -> void:
	var cutin_alpha: float = clampf(0.80 + float(memory_grade) * 0.035, 0.80, 0.94)
	_play_action_cutin(cutin_path, true, cutin_alpha, 0.46)
	await BattleManager.pace_timer(0.78).timeout

## ===================== S212: 전투원의 3D 결합 =====================
## 전투원 컨테이너를 앵커 노드로 한 번 감싼다. 기존 코드는 컨테이너의 위치를
## 그대로 트윈하고(_player_base_pos 등 절대 좌표 대상), 앵커만 카메라 이동량을
## 받는다. 기준 자세에서 오프셋은 0이므로 지금 레이아웃은 조금도 달라지지 않고,
## 카메라가 흔들리거나 좌우 포커스를 옮길 때만 캐릭터가 무대와 함께 움직인다.
## S212: 3D 바닥 그림자가 접지를 담당하면, 화면에 붙어 있던 2D 타원은 보조로 낮춘다.
## 노드는 남긴다 (스프라이트 이동 트윈과 기존 계약이 이 참조를 쓴다).
func _flat_shadow_alpha(base: float) -> float:
	return base * 0.35 if _hybrid_depth_stage != null and is_instance_valid(_hybrid_depth_stage) else base

## S213: 전경 3D 레이어.
##
## 시차는 가까운 물체에서 가장 크게 읽힌다. 지금까지 3D는 전부 캐릭터 뒤에 있어서
## 깊이의 절반만 쓰고 있었다. 카메라 앞쪽 기둥을 캐릭터 위에 겹치면, 카메라가 조금만
## 움직여도 공간이 확실히 살아난다.
##
## 전투는 정보를 읽는 화면이므로 화면 양쪽 가장자리(한쪽 7.2%)에만 둔다. 전투원은
## 화면의 18%와 78% 지점에 서고, 커맨드 덱과 각 패널은 더 높은 z_index로 그 위에
## 그려지므로 정보가 가려지지 않는다.
##
## Clean Gameplay View에서도 끄지 않는다. 기본값이 켜짐이라, 거기서 빼면 대부분의
## 플레이어는 이 레이어를 평생 못 본다. 대신 강도를 낮춰 그 옵션의 취지를 지킨다.
func _build_foreground_depth(root: Control) -> void:
	var profile := HybridDepthStage.profile_from_scene(BattleManager.return_scene)
	_foreground_depth_stage = HybridDepthStage.create_stage(profile, HybridDepthStage.StageMode.FOREGROUND)
	_foreground_depth_stage.name = "ForegroundDepthStage"
	_foreground_depth_stage.modulate.a = 0.26 if OptionsMenu.is_clean_gameplay_visuals() else 0.42
	_foreground_depth_stage.z_index = 30
	_foreground_depth_stage.follow_stage = _hybrid_depth_stage
	root.add_child(_foreground_depth_stage)

func _battle_role_profile(role: String) -> Dictionary:
	return BATTLE_ROLE_PROFILES.get(role, {})

func _role_canvas_foot(role: String) -> Vector2:
	return _battle_role_profile(role).get("canvas_foot", Vector2.ZERO)

func _role_local_foot(role: String) -> Vector2:
	return _battle_role_profile(role).get("local_foot", Vector2.ZERO)

func _role_plate_foot(role: String) -> Vector2:
	return _battle_role_profile(role).get("plate_local_foot", _role_local_foot(role))

func _make_battler_anchor(root: Control, role: String) -> Control:
	var profile := _battle_role_profile(role)
	var anchor := Control.new()
	anchor.name = String(profile.get("anchor_name", role.capitalize() + "Anchor"))
	anchor.mouse_filter = Control.MOUSE_FILTER_IGNORE
	anchor.z_index = int(profile.get("stage_order", -1))
	anchor.set_meta("battle_role", role)
	anchor.set_meta("world_anchor", profile.get("stage_anchor", Vector3.ZERO))
	root.add_child(anchor)
	_battler_anchors.append(anchor)
	return anchor

func _make_role_battler_container(root: Control, role: String) -> Control:
	var profile := _battle_role_profile(role)
	var container := Control.new()
	container.name = "%sBattlerContainer" % role.capitalize()
	container.size = profile.get("container_size", Vector2.ZERO)
	container.position = _role_canvas_foot(role) - _role_local_foot(role)
	container.mouse_filter = Control.MOUSE_FILTER_IGNORE
	container.set_meta("battle_role", role)
	_make_battler_anchor(root, role).add_child(container)
	return container

## S212: 3D 앵커를 2D 발 위치에서 역산해 두 좌표계를 일치시킨다.
##
## 앵커를 손으로 적어 두면 2D 배치를 조금만 손봐도 접지 그림자와 포커스 링이
## 캐릭터에서 떨어져 나간다. 각 전투원이 실제로 발을 딛는 화면 지점을 무대
## 바닥에 역투영해서, 3D 쪽 표식이 언제나 같은 자리를 가리키게 한다.
func _sync_battler_anchors_to_stage() -> void:
	if _hybrid_depth_stage == null or not is_instance_valid(_hybrid_depth_stage):
		return
	for anchor: Control in _battler_anchors:
		if anchor == null or not is_instance_valid(anchor):
			continue
		var role := String(anchor.get_meta("battle_role", ""))
		var profile := _battle_role_profile(role)
		if profile.is_empty():
			continue
		var world: Vector3 = _hybrid_depth_stage.canvas_to_floor(_role_canvas_foot(role))
		anchor.set_meta("world_anchor", world)
		_hybrid_depth_stage.place_battler_anchor(String(profile.get("stage_key", role)), world)

## 카메라가 움직인 만큼 전투원 앵커를 따라 옮긴다.
func _update_battler_anchors() -> void:
	if _hybrid_depth_stage == null or not is_instance_valid(_hybrid_depth_stage):
		return
	for anchor: Control in _battler_anchors:
		if anchor == null or not is_instance_valid(anchor):
			continue
		var world_anchor: Vector3 = anchor.get_meta("world_anchor", Vector3.ZERO)
		anchor.position = _hybrid_depth_stage.anchor_offset(world_anchor) * BATTLER_PARALLAX

## S209: 전투원 일러스트 판을 "실제로 그려지는 크기"로 맞춰 발끝을 기준선에 세운다.
## TextureRect의 KEEP_ASPECT_CENTERED는 16:9 일러스트를 정사각형에 가까운 상자 안에서
## 세로 가운데 정렬한다. 그래서 적 그림의 아랫변이 상자 바닥보다 60~70px 위에 뜨고,
## 그림자만 지면에 남아 적이 공중에 떠 있는 것처럼 보였다.
func _fit_battle_plate(rect: TextureRect, max_size: Vector2, center_x: float, feet_y: float) -> void:
	var tex := rect.texture
	if tex == null:
		return
	var tex_size := Vector2(float(tex.get_width()), float(tex.get_height()))
	if tex_size.x <= 0.0 or tex_size.y <= 0.0:
		return
	var fit: float = minf(max_size.x / tex_size.x, max_size.y / tex_size.y)
	var fitted: Vector2 = tex_size * fit
	rect.size = fitted
	rect.position = Vector2(center_x - fitted.x * 0.5, feet_y - fitted.y)
	rect.pivot_offset = fitted * 0.5

func _fit_role_battle_plate(rect: TextureRect, role: String) -> void:
	var profile := _battle_role_profile(role)
	var plate_foot := _role_plate_foot(role)
	_fit_battle_plate(
		rect,
		profile.get("plate_max_size", Vector2(160.0, 160.0)),
		plate_foot.x,
		plate_foot.y
	)

func _register_battler_actor(actor: CanvasItem, role: String, art_kind: String) -> void:
	if actor == null:
		return
	actor.set_meta("battle_role", role)
	actor.set_meta("battle_art_kind", art_kind)
	actor.set_meta("semantic_state", "idle")
	actor.set_meta("semantic_tint", Color.WHITE)
	actor.set_meta("focus_tint", Color.WHITE)
	actor.set_meta("semantic_base_scale", actor.scale)
	actor.set_meta("semantic_base_rotation", actor.rotation)
	# Keep authored plate/fallback coloration separate from focus and semantic
	# self_modulate. Hit/status feedback must always restore this exact value.
	actor.set_meta("battle_base_modulate", actor.modulate)
	if actor is Control:
		(actor as Control).pivot_offset = (actor as Control).size * 0.5
	if actor is AnimatedSprite2D:
		_initialize_animated_battler_fallback(actor as AnimatedSprite2D)

func _initialize_animated_battler_fallback(asp: AnimatedSprite2D) -> void:
	# Registration is the sole live fallback initialization path.  It gives
	# Reduce Motion a static authored idle frame immediately, while normal mode
	# keeps the pre-existing looping SpriteFrames idle behavior.
	if asp == null or not is_instance_valid(asp):
		return
	if not asp.sprite_frames:
		asp.stop()
		return
	if OptionsMenu.is_reduce_motion():
		_apply_animated_reduce_motion_semantic_state(asp, "idle")
		return
	# This helper may also be used when an already-idle fallback leaves Reduce
	# Motion.  Never let that accessibility sync erase a persistent down state.
	if String(asp.get_meta("semantic_state", "idle")) == "down":
		return
	var idle_animation := _animated_static_frame_animation(asp, "idle")
	if idle_animation == "":
		asp.stop()
		return
	_next_painterly_semantic_generation(asp)
	asp.play(idle_animation)

func _battler_base_modulate(actor: CanvasItem) -> Color:
	if actor == null or not is_instance_valid(actor):
		return Color.WHITE
	var authored: Variant = actor.get_meta("battle_base_modulate", actor.modulate)
	return authored if authored is Color else actor.modulate

func _restore_battler_base_modulate(actor: CanvasItem) -> void:
	if actor == null or not is_instance_valid(actor):
		return
	actor.modulate = _battler_base_modulate(actor)

func _make_painterly_battle_plate(role: String, texture_path: String) -> TextureRect:
	if texture_path == "" or not ResourceLoader.exists(texture_path):
		return null
	var profile := _battle_role_profile(role)
	var rect := TextureRect.new()
	rect.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	rect.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
	rect.texture = load(texture_path)
	# Painterly RGB character plates are mipmapped. Their stage blend supplies
	# vignette and ambient tint, while the alpha-gradient rim stays disabled.
	rect.texture_filter = CanvasItem.TEXTURE_FILTER_LINEAR_WITH_MIPMAPS
	rect.modulate = profile.get("plate_modulate", Color.WHITE)
	_fit_role_battle_plate(rect, role)
	_apply_battle_portrait_blend(
		rect,
		float(profile.get("edge_softness", 0.20)),
		float(profile.get("oval_mask", 0.0)),
		float(profile.get("rim", 0.0))
	)
	_register_battler_actor(rect, role, "painterly")
	rect.set_meta("canonical_plate_path", texture_path)
	return rect

func _make_role_shadow(role: String) -> Polygon2D:
	var profile := _battle_role_profile(role)
	return _make_battle_ellipse(
		_role_local_foot(role),
		profile.get("shadow_radii", Vector2(50.0, 8.0)),
		Color(0, 0, 0, _flat_shadow_alpha(float(profile.get("shadow_alpha", 0.25))))
	)

func _make_role_glow(role: String, color_override: Color = Color.TRANSPARENT) -> Polygon2D:
	var profile := _battle_role_profile(role)
	var color: Color = color_override if color_override.a > 0.0 else profile.get("glow_color", Color.TRANSPARENT)
	return _make_battle_ellipse(
		_role_plate_foot(role),
		profile.get("glow_radii", Vector2(54.0, 10.0)),
		color
	)

## S209: AnimatedSprite2D의 중심 y를 계산해 프레임 아랫변이 `feet_y`에 오게 한다.
## 스프라이트는 중앙 정렬로 그려지므로, 스케일을 바꾸면 발끝도 같이 내려간다.
func _feet_anchored_y(sprite: AnimatedSprite2D, feet_y: float) -> float:
	var frame_height: float = 160.0
	var frames := sprite.sprite_frames
	if frames and frames.has_animation("idle") and frames.get_frame_count("idle") > 0:
		var tex := frames.get_frame_texture("idle", 0)
		if tex:
			frame_height = float(tex.get_height())
	return feet_y - frame_height * sprite.scale.y * 0.5

func _build_player_sprite(root: Control) -> void:
	player_sprite_container = _make_role_battler_container(root, "player")
	# S209: 주인공이 화면에서 가장 작게 읽히던 문제 수정.
	# 컨테이너 원점은 발끝 기준선에서 200px 위. 그림자는 정확히 기준선에 놓인다.
	_player_base_pos = player_sprite_container.position

	# 그림자
	player_shadow = _make_role_shadow("player")
	player_shadow.z_index = -2
	player_sprite_container.add_child(player_shadow)

	var canonical_plate := _make_painterly_battle_plate("player", ARREL_BATTLE_FULLBODY_PATH)
	if canonical_plate:
		player_sprite_container.add_child(canonical_plate)
		player_sprite = canonical_plate
		_player_sprite_base_scale = canonical_plate.scale
		var canonical_glow := _make_role_glow("player")
		canonical_glow.z_index = -1
		player_sprite_container.add_child(canonical_glow)
		return

	var anim_sprite = AnimatedSprite2D.new()
	# S151: 실사 시트 우선 (160px, idle/attack/cast/hurt/down), 없으면 절차 생성 폴백
	var arrel_sheet = PixelSprite.load_sheet_frames("arrel")
	if arrel_sheet:
		anim_sprite.sprite_frames = arrel_sheet
		# S209: 아렐은 무대의 시선 기준. 동행자/적보다 확실히 크게 세운다.
		anim_sprite.scale = Vector2(1.34, 1.34)
	else:
		anim_sprite.sprite_frames = PixelSprite.create_battle_sprite_frames("arrel")
		anim_sprite.scale = Vector2(1.02, 1.02)
	# 프레임 높이에서 발끝 위치를 역산해 기준선에 세운다 (스케일이 바뀌어도 뜨지 않는다).
	anim_sprite.position = Vector2(_role_plate_foot("player").x, _feet_anchored_y(anim_sprite, _role_plate_foot("player").y))
	anim_sprite.texture_filter = CanvasItem.TEXTURE_FILTER_NEAREST
	# S214: 주인공도 무대의 빛을 받는다. 타원 마스크와 가장자리 페이드는 끄고
	# (스프라이트는 이미 알파로 잘려 있다) 조명 성분만 쓴다.
	_apply_battle_portrait_blend(anim_sprite, 0.02, 0.0, 0.42)
	_register_battler_actor(anim_sprite, "player", "animated_fallback")
	anim_sprite.set_meta("canonical_fallback", true)
	player_sprite_container.add_child(anim_sprite)
	player_sprite = anim_sprite
	_player_sprite_base_scale = anim_sprite.scale

	# 발밑 광원 (은은한 파란 빛)
	var glow := _make_role_glow("player")
	glow.z_index = -1
	player_sprite_container.add_child(glow)

## S44: 동행자 스프라이트 (왼쪽 뒤)
func _build_ally_sprite(root: Control) -> void:
	ally_sprite_container = _make_role_battler_container(root, "ally")
	# S209: 동행자는 아렐보다 한 걸음 뒤. 그림자는 기준선보다 12px 위(원근).
	ally_sprite_container.visible = false
	_ally_base_pos = ally_sprite_container.position

	_displayed_ally_identity = ""
	# 동행자가 있는지 확인
	var has_ally = BattleManager.sable_in_party or GameManager.player_data.elia_with_party
	if not has_ally:
		return

	ally_sprite_container.visible = true
	var who = "sable" if BattleManager.sable_in_party else "elia"
	_displayed_ally_identity = who.capitalize()

	# 그림자
	ally_shadow = _make_role_shadow("ally")
	ally_shadow.z_index = -2
	ally_sprite_container.add_child(ally_shadow)

	# 동행자 포트레이트 체크
	var portrait_map = {"elia": ELIA_BATTLE_FULLBODY_PATH, "sable": SABLE_BATTLE_FULLBODY_PATH}
	var p_path = portrait_map.get(who, "")
	if p_path != "" and ResourceLoader.exists(p_path):
		var tex_rect := _make_painterly_battle_plate("ally", p_path)
		# S214: 원화 판은 선형 + 밉맵으로 축소한다.
		# 프로젝트 기본 필터가 Nearest라, 1672px 원화를 300px대로 줄이는 동안 픽셀이
		# 그대로 버려져 엘리아의 머리카락 같은 디더 알파가 점점이 튀었다.
		ally_sprite_container.add_child(tex_rect)
		ally_sprite = tex_rect
	elif who == "elia":
		var anim_sprite = AnimatedSprite2D.new()
		# S151: 엘리아 실사 시트 우선, 없으면 절차 생성 폴백
		var elia_sheet = PixelSprite.load_sheet_frames("elia")
		if elia_sheet:
			anim_sprite.sprite_frames = elia_sheet
			anim_sprite.scale = Vector2(0.92, 0.92)  # 아렐(1.34)보다 작게 두어 원근을 만든다
		else:
			anim_sprite.sprite_frames = PixelSprite.create_battle_sprite_frames("elia")
			anim_sprite.scale = Vector2(0.86, 0.86)
		anim_sprite.position = Vector2(_role_plate_foot("ally").x, _feet_anchored_y(anim_sprite, _role_plate_foot("ally").y))
		anim_sprite.texture_filter = CanvasItem.TEXTURE_FILTER_NEAREST
		_register_battler_actor(anim_sprite, "ally", "animated_fallback")
		anim_sprite.set_meta("canonical_fallback", true)
		ally_sprite_container.add_child(anim_sprite)
		ally_sprite = anim_sprite
	else:
		# S57: Use AnimatedSprite2D with battle sprite frames for animation support
		var anim_sprite = AnimatedSprite2D.new()
		anim_sprite.sprite_frames = PixelSprite.create_battle_sprite_frames(who)
		anim_sprite.scale = Vector2(0.86, 0.86)
		anim_sprite.position = Vector2(_role_plate_foot("ally").x, _feet_anchored_y(anim_sprite, _role_plate_foot("ally").y))
		anim_sprite.texture_filter = CanvasItem.TEXTURE_FILTER_NEAREST
		_register_battler_actor(anim_sprite, "ally", "animated_fallback")
		anim_sprite.set_meta("canonical_fallback", true)
		ally_sprite_container.add_child(anim_sprite)
		ally_sprite = anim_sprite
	if ally_sprite:
		ally_sprite.set_meta("displayed_character", _displayed_ally_identity)

	# 발밑 광원
	var glow_color = Color(0.5, 0.3, 0.6, 0.08) if who == "sable" else Color(0.6, 0.5, 0.2, 0.08)
	var glow := _make_role_glow("ally", glow_color)
	glow.z_index = -1
	ally_sprite_container.add_child(glow)

## S44: 적 스프라이트 (오른쪽, 128x128 대형)
func _build_tobias_support_sprite(root: Control) -> void:
	tobias_sprite_container = _make_role_battler_container(root, "support")
	# S209: 지원 인원도 같은 기준선에. 아군 대열에서 가장 뒤(-20px 원근).
	tobias_sprite_container.visible = BattleManager.tobias_in_party
	_tobias_base_pos = tobias_sprite_container.position
	if not BattleManager.tobias_in_party:
		return

	tobias_shadow = _make_role_shadow("support")
	tobias_shadow.z_index = -2
	tobias_sprite_container.add_child(tobias_shadow)

	var tobias_path: String = TOBIAS_BATTLE_FULLBODY_PATH
	if not ResourceLoader.exists(tobias_path):
		tobias_path = "res://assets/portraits/character_shots/tobias_story_v2.png"
	if ResourceLoader.exists(tobias_path):
		var tex_rect := _make_painterly_battle_plate("support", tobias_path)
		tobias_sprite_container.add_child(tex_rect)
		tobias_sprite = tex_rect
	else:
		var anim_sprite := AnimatedSprite2D.new()
		anim_sprite.sprite_frames = PixelSprite.create_battle_sprite_frames("tobias")
		anim_sprite.scale = Vector2(0.82, 0.82)
		anim_sprite.position = Vector2(_role_plate_foot("support").x, _feet_anchored_y(anim_sprite, _role_plate_foot("support").y))
		anim_sprite.texture_filter = CanvasItem.TEXTURE_FILTER_NEAREST
		_register_battler_actor(anim_sprite, "support", "animated_fallback")
		anim_sprite.set_meta("canonical_fallback", true)
		tobias_sprite_container.add_child(anim_sprite)
		tobias_sprite = anim_sprite
	if tobias_sprite:
		tobias_sprite.set_meta("displayed_character", "Tobias")

	var glow := _make_role_glow("support")
	glow.z_index = -1
	tobias_sprite_container.add_child(glow)

func _build_enemy_sprite(root: Control) -> void:
	enemy_sprite_container = _make_role_battler_container(root, "enemy")
	# S209: 적도 같은 기준선. 컨테이너 높이 300 → 그림자가 정확히 기준선에 온다.
	_enemy_base_pos = enemy_sprite_container.position

	# 그림자
	enemy_shadow = _make_role_shadow("enemy")
	enemy_shadow.z_index = -2
	enemy_sprite_container.add_child(enemy_shadow)

	var enemy_name: String = BattleManager.current_enemy.name if BattleManager.current_enemy else ""
	var enemy_art_path: String = BattleManager.enemy_image if BattleManager.enemy_image != "" and ResourceLoader.exists(BattleManager.enemy_image) else _resolve_enemy_art_by_name(enemy_name)
	if enemy_art_path != "":
		var tex_rect := _make_painterly_battle_plate("enemy", enemy_art_path)
		# S209: 예전에는 240x230 안에 16:9 장면 일러스트가 들어가면서 실제로는
		# 240x135짜리 작은 카드로 그려졌고, 사각 테두리가 그대로 보였다.
		# 표시 면적을 키우고 타원 마스크로 테두리를 지운다.
		enemy_sprite_container.add_child(tex_rect)
		enemy_sprite = tex_rect
	else:
		# S44: 128x128 대형 적 스프라이트
		var enemy_type = BattleManager.current_enemy.name if BattleManager.current_enemy else "generic"
		var tex = PixelSprite.create_battle_enemy(enemy_type)
		var tex_rect = TextureRect.new()
		tex_rect.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
		tex_rect.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
		tex_rect.texture = tex
		tex_rect.texture_filter = CanvasItem.TEXTURE_FILTER_NEAREST
		_fit_role_battle_plate(tex_rect, "enemy")
		_register_battler_actor(tex_rect, "enemy", "procedural_fallback")
		tex_rect.set_meta("canonical_fallback", true)
		enemy_sprite_container.add_child(tex_rect)
		enemy_sprite = tex_rect

	# 발밑 광원 (적은 빨간/보라 톤)
	var enemy_lower_name: String = enemy_name.to_lower()
	var glow_c = Color(0.5, 0.15, 0.5, 0.10) if "void" in enemy_lower_name or "shade" in enemy_lower_name else Color(0.5, 0.2, 0.15, 0.08)
	var glow := _make_role_glow("enemy", glow_c)
	glow.z_index = -1
	enemy_sprite_container.add_child(glow)

func _make_battle_ellipse(center: Vector2, radii: Vector2, color: Color, segments: int = 32) -> Polygon2D:
	var ellipse := Polygon2D.new()
	var points := PackedVector2Array()
	for i in range(segments):
		var angle := TAU * float(i) / float(segments)
		points.append(center + Vector2(cos(angle) * radii.x, sin(angle) * radii.y))
	ellipse.polygon = points
	ellipse.color = color
	return ellipse

## S214: 무대 조명을 전투원 머티리얼에 심는다.
##
## 3D 무대에는 따뜻한 키라이트와 바이옴 색 보조광이 있는데 2D 전투원만 자기 원화
## 밝기 그대로 그려져서, 무대를 아무리 손봐도 "배경 위에 붙인 그림"으로 읽혔다.
## 키라이트 방향의 림라이트와 옅은 바이옴 물들임만 얹는다. 원화의 명암은 건드리지
## 않으므로 그림 자체는 망가지지 않는다.
##
## 방향은 HybridDepthStage의 키라이트(rotation -52, -28)와 맞춘 화면 좌표계 값이다.
## 둘 중 하나만 바꾸면 빛이 어긋나므로 함께 조정할 것.
const STAGE_KEY_DIRECTION: Vector2 = Vector2(-0.55, -0.83)
const STAGE_KEY_COLOR: Color = Color(1.0, 0.82, 0.58)

## `rim`은 선명하게 잘린 스프라이트에만 준다.
## 원화 포트레이트(엘리아/토비아스 등)는 알파 가장자리가 흐리고 점점이 흩어져 있어서,
## 림라이트를 얹으면 실루엣이 지직거리는 노이즈로 변한다. 두 아트 스타일을 같은
## 조명으로 다룰 수 없다는 뜻이므로, 판에는 바이옴 물들임만 적용한다.
func _apply_stage_lighting(material: ShaderMaterial, rim: float) -> void:
	if material == null:
		return
	material.set_shader_parameter("key_direction", STAGE_KEY_DIRECTION)
	material.set_shader_parameter("key_color", STAGE_KEY_COLOR)
	material.set_shader_parameter("rim_strength", rim)
	material.set_shader_parameter("ambient_tint", _stage_ambient_tint())
	material.set_shader_parameter("ambient_strength", 0.10)

## 현재 전장의 바이옴 색. 전투원을 그 색으로 아주 살짝 물들여 무대와 묶는다.
func _stage_ambient_tint() -> Color:
	var profile := HybridDepthStage.profile_from_scene(BattleManager.return_scene)
	var data: Dictionary = HybridDepthStage.PROFILE_DATA.get(profile, {})
	var accent: Color = data.get("accent", Color(0.72, 0.58, 0.32))
	# 원본 색을 유지하면서 색조만 얹도록 흰색 쪽으로 끌어올린다.
	return Color(1, 1, 1).lerp(accent, 0.35)

func _apply_battle_portrait_blend(rect: CanvasItem, edge_softness: float = 0.20, oval_mask: float = 0.0, rim: float = 0.0) -> void:
	var shader_path := "res://assets/shaders/battle_stage_blend.gdshader"
	if not ResourceLoader.exists(shader_path):
		return
	var blend_material := ShaderMaterial.new()
	blend_material.shader = load(shader_path)
	blend_material.set_shader_parameter("edge_softness", edge_softness)
	blend_material.set_shader_parameter("lower_fade", 0.92)
	blend_material.set_shader_parameter("oval_mask", oval_mask)
	_apply_stage_lighting(blend_material, rim)
	rect.material = blend_material
	# S209: 상태이상/보스 페이즈 VFX가 material을 교체한 뒤 null로 되돌리면 무대 블렌드가
	# 사라져 사각 테두리가 다시 드러난다. 원본을 노드에 보관해 복구할 수 있게 한다.
	rect.set_meta("stage_blend_material", blend_material)

## S209: VFX 셰이더를 벗길 때 null 대신 무대 블렌드로 되돌린다.
func _restore_plate_material(node: CanvasItem) -> void:
	if node == null or not is_instance_valid(node):
		return
	node.material = node.get_meta("stage_blend_material", null)

func _multiply_actor_tints(left: Color, right: Color) -> Color:
	return Color(left.r * right.r, left.g * right.g, left.b * right.b, left.a * right.a)

func _refresh_battler_actor_tint(actor: CanvasItem) -> void:
	if actor == null or not is_instance_valid(actor):
		return
	var focus: Color = actor.get_meta("focus_tint", Color.WHITE)
	var semantic: Color = actor.get_meta("semantic_tint", Color.WHITE)
	actor.self_modulate = _multiply_actor_tints(focus, semantic)

func _set_battler_focus_tint(actor: CanvasItem, tint: Color) -> void:
	if actor == null or not is_instance_valid(actor):
		return
	actor.set_meta("focus_tint", tint)
	_refresh_battler_actor_tint(actor)

func _apply_battler_focus_emphasis(side: String) -> void:
	_current_battler_focus = side
	var player_active := side == "player"
	var enemy_active := side == "enemy" or side == "enemy_break"
	# Focus is brightness-only: authored plate color remains in `modulate` and
	# semantic cues continue to compose through `self_modulate` without a hue
	# shift that would misrepresent the current actor state.
	var player_tint := Color(1.12, 1.12, 1.12, 1.0) if player_active else Color(0.82, 0.82, 0.82, 1.0) if enemy_active else Color.WHITE
	var enemy_tint := Color(1.12, 1.12, 1.12, 1.0) if enemy_active else Color(0.82, 0.82, 0.82, 1.0) if player_active else Color.WHITE
	var rear_tint := Color(0.80, 0.80, 0.80, 1.0) if player_active or enemy_active else Color(0.94, 0.94, 0.94, 1.0)
	_set_battler_focus_tint(player_sprite, player_tint)
	_set_battler_focus_tint(enemy_sprite, enemy_tint)
	_set_battler_focus_tint(ally_sprite, rear_tint)
	_set_battler_focus_tint(tobias_sprite, rear_tint)

func _set_battle_stage_focus(side: String) -> void:
	if _hybrid_depth_stage != null and is_instance_valid(_hybrid_depth_stage):
		_hybrid_depth_stage.set_battle_focus(side)
	# Visual emphasis is intentionally brightness-only.  HybridDepthStage owns
	# the camera/floor focus; battler containers never receive focus-position tweens.
	_apply_battler_focus_emphasis(side)

func _clear_painterly_semantic_tween(actor: CanvasItem) -> void:
	if actor == null:
		return
	var key := actor.get_instance_id()
	var active := _painterly_semantic_tweens.get(key) as Tween
	if active and is_instance_valid(active):
		active.kill()
	_painterly_semantic_tweens.erase(key)

func _next_painterly_semantic_generation(actor: CanvasItem) -> int:
	if actor == null or not is_instance_valid(actor):
		return -1
	var key := actor.get_instance_id()
	var generation := int(_painterly_semantic_generations.get(key, 0)) + 1
	_painterly_semantic_generations[key] = generation
	actor.set_meta("semantic_generation", generation)
	return generation

func _painterly_semantic_generation(actor: CanvasItem) -> int:
	if actor == null or not is_instance_valid(actor):
		return -1
	return int(_painterly_semantic_generations.get(actor.get_instance_id(), actor.get_meta("semantic_generation", -1)))

func _reduce_motion_semantic_hold_duration(state: String) -> float:
	match state:
		"cast":
			return 0.22
		"hurt":
			return 0.16
	return 0.18

func _schedule_reduce_motion_semantic_settle(actor: CanvasItem, state: String, generation: int) -> void:
	# This is deliberately a timer rather than a Tween: Reduce Motion keeps one
	# readable static cue, then returns to idle without interpolated movement.
	var settle_timer := get_tree().create_timer(_reduce_motion_semantic_hold_duration(state))
	settle_timer.timeout.connect(func():
		_settle_painterly_semantic_state(actor, state, generation)
	)

func _semantic_tint_for_state(state: String) -> Color:
	match state:
		"attack":
			return Color(1.10, 0.98, 0.92, 1.0)
		"cast":
			return Color(0.82, 0.94, 1.14, 1.0)
		"hurt":
			return Color(1.14, 0.72, 0.70, 1.0)
		"down":
			return Color(0.52, 0.54, 0.62, 0.84)
	return Color.WHITE

func _apply_painterly_semantic_state(actor: CanvasItem, state: String, hold: bool = false) -> void:
	if actor == null or not is_instance_valid(actor):
		return
	var generation := _next_painterly_semantic_generation(actor)
	_clear_painterly_semantic_tween(actor)
	var role := String(actor.get_meta("battle_role", "player"))
	var base_scale: Vector2 = actor.get_meta("semantic_base_scale", actor.scale)
	var base_rotation: float = float(actor.get_meta("semantic_base_rotation", actor.rotation))
	var direction := -1.0 if role == "player" or role == "ally" or role == "support" else 1.0
	var target_scale := base_scale
	var target_rotation := base_rotation
	match state:
		"attack":
			target_scale = base_scale * Vector2(1.025, 0.985)
			target_rotation = base_rotation + direction * 0.045
		"cast":
			target_scale = base_scale * Vector2(1.018, 1.035)
			target_rotation = base_rotation + direction * 0.026
		"hurt":
			target_scale = base_scale * Vector2(0.975, 1.025)
			target_rotation = base_rotation - direction * 0.050
		"down":
			target_scale = base_scale * Vector2(1.08, 0.78)
			target_rotation = base_rotation + direction * 1.12
	actor.set_meta("semantic_state", state)
	actor.set_meta("semantic_tint", _semantic_tint_for_state(state))
	actor.scale = target_scale
	actor.rotation = target_rotation
	_refresh_battler_actor_tint(actor)
	if state == "down" or state == "idle":
		return
	if OptionsMenu.is_reduce_motion():
		_schedule_reduce_motion_semantic_settle(actor, state, generation)
		return
	if hold:
		return
	var duration := 0.28 if state == "attack" else 0.36 if state == "cast" else 0.22
	var semantic_tween := create_tween().bind_node(actor).set_parallel(true)
	_painterly_semantic_tweens[actor.get_instance_id()] = semantic_tween
	semantic_tween.tween_property(actor, "scale", base_scale, duration).set_trans(Tween.TRANS_SINE).set_ease(Tween.EASE_OUT)
	semantic_tween.tween_property(actor, "rotation", base_rotation, duration).set_trans(Tween.TRANS_SINE).set_ease(Tween.EASE_OUT)
	semantic_tween.chain().tween_callback(_settle_painterly_semantic_state.bind(actor, state, generation))

func _settle_painterly_semantic_state(actor: CanvasItem, state: String, generation: int = -1) -> void:
	# AnimatedSprite2D fallbacks retain their existing animation-finished path.
	# Painterly action/hurt states clear only after the pre-existing mechanical
	# tween resolves, so no semantic tween fights the authoritative lunge/squash.
	if actor == null or not is_instance_valid(actor) or actor is AnimatedSprite2D or state == "down" or state == "idle":
		return
	if String(actor.get_meta("semantic_state", "")) != state:
		return
	if generation >= 0 and _painterly_semantic_generation(actor) != generation:
		return
	_apply_painterly_semantic_state(actor, "idle", true)

func _animated_static_frame_animation(asp: AnimatedSprite2D, requested_state: String) -> String:
	if asp == null or not is_instance_valid(asp) or not asp.sprite_frames:
		return ""
	if asp.sprite_frames.has_animation(requested_state):
		return requested_state
	if asp.sprite_frames.has_animation("idle"):
		return "idle"
	return ""

func _set_animated_static_frame(asp: AnimatedSprite2D, requested_state: String) -> String:
	var resolved_state := _animated_static_frame_animation(asp, requested_state)
	if resolved_state == "":
		asp.stop()
		return ""
	# Pause rather than play: accessibility still shows an authored semantic frame,
	# but never advances SpriteFrames while Reduce Motion is enabled.
	asp.pause()
	asp.animation = StringName(resolved_state)
	var frame_count := asp.sprite_frames.get_frame_count(resolved_state)
	if frame_count > 0:
		if resolved_state == "down":
			asp.frame = frame_count - 1
		elif resolved_state == "idle":
			asp.frame = 0
		else:
			asp.frame = int(float(frame_count - 1) * 0.5)
		asp.frame_progress = 0.0
	asp.pause()
	return resolved_state

func _apply_animated_reduce_motion_semantic_state(asp: AnimatedSprite2D, state: String) -> void:
	if asp == null or not is_instance_valid(asp):
		return
	var generation := _next_painterly_semantic_generation(asp)
	_clear_painterly_semantic_tween(asp)
	var resolved_state := _set_animated_static_frame(asp, state)
	var base_scale: Vector2 = asp.get_meta("semantic_base_scale", asp.scale)
	var base_rotation: float = float(asp.get_meta("semantic_base_rotation", asp.rotation))
	asp.scale = base_scale
	asp.rotation = base_rotation
	asp.set_meta("semantic_state", resolved_state)
	asp.set_meta("semantic_tint", _semantic_tint_for_state(resolved_state))
	_refresh_battler_actor_tint(asp)
	if resolved_state == "" or resolved_state == "idle" or resolved_state == "down":
		return
	var settle_timer := get_tree().create_timer(_reduce_motion_semantic_hold_duration(resolved_state))
	settle_timer.timeout.connect(func():
		_settle_animated_reduce_motion_semantic_state(asp, resolved_state, generation)
	)

func _settle_animated_reduce_motion_semantic_state(asp: AnimatedSprite2D, state: String, generation: int) -> void:
	if asp == null or not is_instance_valid(asp) or state == "idle" or state == "down":
		return
	if String(asp.get_meta("semantic_state", "")) != state:
		return
	if _painterly_semantic_generation(asp) != generation:
		return
	_apply_animated_reduce_motion_semantic_state(asp, "idle")

func _build_log_panel(root: Control) -> void:
	field_readout_art = _make_interface_texture_region(
		UI_FIELD_READOUT_PATH,
		Rect2(34, 282, 1894, 226),
		0.94
	)
	field_readout_art.name = "BattleFieldReadoutArt"
	field_readout_art.anchor_left = 0.35
	field_readout_art.anchor_right = 0.85
	field_readout_art.anchor_top = 0.665
	field_readout_art.anchor_bottom = 0.772
	field_readout_art.z_index = 35
	root.add_child(field_readout_art)

	var panel := PanelContainer.new()
	panel.name = "BattleFieldReadout"
	panel.anchor_left = 0.385
	panel.anchor_right = 0.815
	panel.anchor_top = 0.688
	panel.anchor_bottom = 0.748
	panel.mouse_filter = Control.MOUSE_FILTER_IGNORE
	panel.z_index = 36
	var style := StyleBoxFlat.new()
	style.bg_color = Color(0.012, 0.018, 0.028, 0.88)
	style.set_content_margin_all(4)
	panel.add_theme_stylebox_override("panel", style)
	root.add_child(panel)

	var row := HBoxContainer.new()
	row.add_theme_constant_override("separation", 12)
	panel.add_child(row)

	field_readout_header = Label.new()
	field_readout_header.name = "BattleFieldReadoutHeader"
	field_readout_header.text = _bl("FIELD READ", "전장 판독")
	field_readout_header.custom_minimum_size = Vector2(104, 0)
	field_readout_header.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	UITheme.style_ui_label(field_readout_header, Color(0.66, 0.86, 1.0), UITheme.SIZE_LABEL)
	row.add_child(field_readout_header)

	log_label = RichTextLabel.new()
	log_label.name = "BattleFieldReadoutText"
	log_label.bbcode_enabled = true
	log_label.scroll_active = false
	log_label.fit_content = true
	log_label.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	log_label.size_flags_vertical = Control.SIZE_EXPAND_FILL
	# S230: 전장 판독은 이야기 본문이 아니라 인터페이스 문장이다. 산세리프로 읽는다.
	UITheme.apply_readability_finish(log_label, 15, UITheme.TEXT_PRIMARY, false)
	row.add_child(log_label)
	_set_field_readout(
		_bl("FIELD READ", "전장 판독"),
		_bl("Choose an action to preview its purpose and cost.", "행동을 가리키면 목적과 대가를 미리 읽을 수 있습니다."),
		Color(0.48, 0.78, 1.0)
	)

## S230: 아렐의 상태를 한 카드로 모은다.
## 예전에는 HP 패널, 리밋 레일, 상태 칩이 각자 앵커로 떠 있었고,
## 상태 칩 줄(y 564~585)은 커맨드 덱 장식(y 546~) 위에 얹혀 "SABLE" 태그가 잘렸다.
## 이제 하나의 패널이 발판 아래(438)에서 시작해 덱 윗선(546) 전에 끝난다.
func _build_player_panel(root: Control) -> void:
	player_panel = PanelContainer.new()
	player_panel.name = "PlayerReadout"
	player_panel.anchor_left = HUD_LEFT_COL_L
	player_panel.anchor_right = 0.35
	player_panel.anchor_top = 0.0
	player_panel.anchor_bottom = 0.0
	player_panel.offset_top = HUD_PLAYER_CLUSTER_TOP
	player_panel.offset_bottom = HUD_PLAYER_CLUSTER_TOP
	player_panel.grow_vertical = Control.GROW_DIRECTION_END
	player_panel.mouse_filter = Control.MOUSE_FILTER_IGNORE

	var style = StyleBoxFlat.new()
	style.bg_color = Color(0.030, 0.034, 0.052, 0.93)
	style.border_color = Color(0.36, 0.50, 0.74, 0.62)
	style.set_border_width_all(1)
	style.set_corner_radius_all(4)
	style.set_content_margin_all(8)
	player_panel.add_theme_stylebox_override("panel", style)
	root.add_child(player_panel)

	var hbox = HBoxContainer.new()
	hbox.add_theme_constant_override("separation", 10)
	player_panel.add_child(hbox)

	# S44: 미니 포트레이트 (HP 옆)
	var portrait_path = "res://assets/portraits/arrel_face_neutral.png"
	if ResourceLoader.exists(portrait_path):
		player_portrait_rect = TextureRect.new()
		player_portrait_rect.custom_minimum_size = Vector2(50, 50)
		player_portrait_rect.size_flags_vertical = Control.SIZE_SHRINK_BEGIN
		player_portrait_rect.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
		player_portrait_rect.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_COVERED
		player_portrait_rect.texture = load(portrait_path)
		hbox.add_child(player_portrait_rect)

	_player_readout_column = VBoxContainer.new()
	_player_readout_column.add_theme_constant_override("separation", 4)
	_player_readout_column.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	hbox.add_child(_player_readout_column)

	var name_row := HBoxContainer.new()
	name_row.add_theme_constant_override("separation", 8)
	_player_readout_column.add_child(name_row)

	var name_label = Label.new()
	name_label.text = GameManager.localized_speaker("Arrel")
	name_label.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	name_label.text_overrun_behavior = TextServer.OVERRUN_TRIM_ELLIPSIS
	UITheme.style_label(name_label, UITheme.make_ui_font(), UITheme.SIZE_UI, Color(0.76, 0.87, 1.0))
	name_row.add_child(name_label)

	player_hp_label = Label.new()
	player_hp_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	player_hp_label.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	UITheme.style_ui_label(player_hp_label, Color(0.82, 0.90, 1.0), UITheme.SIZE_LABEL)
	name_row.add_child(player_hp_label)

	var hp_stack := _make_hp_bar_stack(18.0, UITheme.HP_PLAYER, Color(0.62, 0.86, 1.0, 0.55), Color(0.055, 0.060, 0.085, 0.94))
	player_hp_bar = hp_stack.bar
	player_hp_ghost = hp_stack.ghost
	_player_readout_column.add_child(hp_stack.stack)

func _build_action_buttons(root: Control) -> void:
	action_ribbon_art = _make_interface_texture_region(
		UI_COMMAND_DECK_PATH,
		Rect2(8, 284, 1656, 356),
		0.96
	)
	action_ribbon_art.name = "BattleCommandDeckArt"
	action_ribbon_art.anchor_left = 0.035
	action_ribbon_art.anchor_right = 0.965
	action_ribbon_art.anchor_top = 0.758
	action_ribbon_art.anchor_bottom = 0.995
	action_ribbon_art.z_index = 37
	root.add_child(action_ribbon_art)

	action_container = GridContainer.new()
	action_container.columns = 4
	action_container.anchor_left = 0.145
	action_container.anchor_right = 0.855
	action_container.anchor_top = 0.802
	action_container.anchor_bottom = 0.978
	action_container.add_theme_constant_override("h_separation", 10)
	action_container.add_theme_constant_override("v_separation", 7)
	action_container.z_index = 38
	root.add_child(action_container)

	var actions = [
		{"id": "attack", "text": GameManager.loc("attack"), "callback": _on_attack},
		{"id": "witness", "text": "기억 읽기" if GameManager.current_locale == "ko" else "WITNESS", "callback": _on_witness},
		{"id": "burn", "text": GameManager.loc("burn"), "callback": _on_burn_menu},
		{"id": "defend", "text": GameManager.loc("defend"), "callback": _on_defend},
		{"id": "item", "text": GameManager.loc("item"), "callback": _on_item_menu},
		{"id": "limit", "text": GameManager.loc("limit"), "callback": _on_limit_break},
		{"id": "auto", "text": GameManager.loc("auto"), "callback": _on_auto_battle},
		{"id": "flee", "text": GameManager.loc("flee"), "callback": _on_flee},
	]

	for action in actions:
		var action_id := String(action.id)
		var command_number := action_container.get_child_count() + 1
		var btn := Button.new()
		btn.name = "BattleAction_" + action_id.capitalize()
		var command_label := "%d · %s" % [command_number, String(action.text)]
		btn.set_meta("command_label", command_label)
		btn.text = _format_action_button(action_id, command_label)
		btn.custom_minimum_size = Vector2(205, 48)
		btn.set_meta("action_id", action_id)
		btn.tooltip_text = _action_description(action_id)

		var style = StyleBoxFlat.new()
		style.bg_color = _action_base_color(action_id)
		style.border_color = Color(0.60, 0.45, 0.26, 0.42)
		style.set_border_width_all(1)
		style.set_corner_radius_all(4)
		style.set_content_margin_all(8)
		btn.add_theme_stylebox_override("normal", style)

		var hover = style.duplicate()
		hover.bg_color = Color(0.18, 0.13, 0.20, 0.92)
		hover.border_color = Color(0.95, 0.68, 0.34, 0.90)
		hover.set_border_width_all(2)
		btn.add_theme_stylebox_override("hover", hover)
		btn.add_theme_stylebox_override("focus", hover)

		var pressed_s = style.duplicate()
		pressed_s.bg_color = Color(0.32, 0.20, 0.26, 0.98)
		pressed_s.border_color = Color(0.9, 0.65, 0.35, 1.0)
		btn.add_theme_stylebox_override("pressed", pressed_s)

		# S209: 비활성 상태도 불투명 배경을 유지해야 커맨드 덱 장식이 글자를 관통하지 않는다.
		var disabled_s = style.duplicate()
		disabled_s.bg_color = Color(0.030, 0.026, 0.038, 0.96)
		disabled_s.border_color = Color(0.34, 0.28, 0.20, 0.40)
		btn.add_theme_stylebox_override("disabled", disabled_s)
		btn.add_theme_color_override("font_disabled_color", Color(0.60, 0.55, 0.50, 0.92))

		btn.add_theme_font_size_override("font_size", 15)
		btn.add_theme_color_override("font_color", UITheme.TEXT_PRIMARY)
		btn.add_theme_color_override("font_hover_color", Color(0.95, 0.8, 0.5))
		btn.add_theme_color_override("font_pressed_color", Color(1.0, 0.9, 0.6))
		btn.add_theme_color_override("font_outline_color", Color(0, 0, 0, 0.72))
		btn.add_theme_constant_override("outline_size", 1)

		btn.pressed.connect(action.callback)
		_action_buttons[action_id] = btn
		if action_id == "witness":
			witness_btn = btn
			witness_btn.tooltip_text = "적 안에 갇힌 기억을 읽어 비연소 해결을 시도합니다." if GameManager.current_locale == "ko" else "Read the trapped memory and seek a victory without burning."
		elif action_id == "limit":
			limit_btn = btn  # S175: 참조 저장, 로케일 무관 활성/비활성 갱신
		elif action_id == "auto":
			auto_btn = btn
		btn.focus_entered.connect(_on_action_focus.bind(action_id))
		btn.focus_exited.connect(_restore_field_readout)
		btn.mouse_entered.connect(_on_action_hover.bind(btn, action_id))
		btn.mouse_exited.connect(_on_action_hover_exit.bind(btn))
		action_container.add_child(btn)
	_update_limit_button()
	_update_objective_action_warnings()

## S226: A command that would lose the active directive wears the warning before
## it is pressed, not after.
func _update_objective_action_warnings() -> void:
	for action_id in _action_buttons:
		var btn: Button = _action_buttons[action_id]
		if btn == null or not is_instance_valid(btn):
			continue
		var relation := BattleManager.get_objective_action_relation(String(action_id))
		var conflicts := String(relation.get("kind", "")) == "fail"
		var normal := btn.get_theme_stylebox("normal") as StyleBoxFlat
		if normal:
			normal.border_color = Color(0.95, 0.30, 0.24, 0.92) if conflicts else Color(0.60, 0.45, 0.26, 0.42)
			normal.set_border_width_all(2 if conflicts else 1)
		btn.add_theme_color_override("font_color", Color(1.0, 0.64, 0.54) if conflicts else UITheme.TEXT_PRIMARY)
		var command_label := String(btn.get_meta("command_label", btn.text))
		if conflicts:
			btn.text = "%s\n%s" % [command_label, String(relation.get("text", ""))]
			btn.tooltip_text = String(relation.get("text", ""))
		else:
			btn.text = _format_action_button(String(action_id), command_label)
			btn.tooltip_text = _action_description(String(action_id))

func _action_base_color(action_id: String) -> Color:
	match action_id:
		"witness":
			return Color(0.035, 0.075, 0.105, 0.90)
		"burn":
			return Color(0.115, 0.038, 0.060, 0.91)
		"defend":
			return Color(0.040, 0.070, 0.090, 0.90)
		"limit":
			return Color(0.092, 0.050, 0.112, 0.91)
	return Color(0.045, 0.040, 0.058, 0.88)

func _format_action_button(action_id: String, title: String) -> String:
	var role := ""
	match action_id:
		"attack": role = _bl("Damage + BREAK", "피해 + 브레이크")
		"witness": role = _bl("Preserve identity", "정체성 보존")
		"burn": role = _bl("Power / permanent cost", "화력 / 영구 대가")
		"defend": role = _bl("Half damage + Limit", "피해 절반 + 리밋")
		"item": role = _bl("Use a field kit", "전투 도구 사용")
		"limit": role = _bl("Memory Cascade", "메모리 캐스케이드")
		"auto": role = _bl("Delegate this fight", "전투 위임")
		"flee": role = _bl("Leave encounter", "전투 이탈")
	return "%s\n%s" % [title, role]

func _on_action_focus(action_id: String) -> void:
	AudioManager.play_sfx("ui_hover")
	_show_action_forecast(action_id)

func _on_action_hover(btn: Button, action_id: String) -> void:
	_show_action_forecast(action_id)
	var tw := create_tween()
	tw.tween_property(btn, "scale", Vector2(1.025, 1.025), 0.06).set_ease(Tween.EASE_OUT)

func _on_action_hover_exit(btn: Button) -> void:
	var tw := create_tween()
	tw.tween_property(btn, "scale", Vector2.ONE, 0.08).set_ease(Tween.EASE_OUT)
	if not btn.has_focus():
		_restore_field_readout()

## S226: Forecast priority is fixed: what this does to the objective, then the
## immediate effect, then the permanent cost.
func _show_action_forecast(action_id: String) -> void:
	var relation := BattleManager.get_objective_action_relation(action_id)
	var relation_kind := String(relation.get("kind", ""))
	var detail := _action_description(action_id)
	var header := _action_forecast_title(action_id)
	var accent := _action_forecast_color(action_id)
	if relation_kind != "":
		detail = "%s\n%s" % [String(relation.get("text", "")), detail]
		if relation_kind == "fail":
			header = _bl("OBJECTIVE AT RISK", "목표 위험")
			accent = _objective_relation_color(relation_kind)
	_set_field_readout(header, detail, accent)

func _action_forecast_title(action_id: String) -> String:
	match action_id:
		"attack": return _bl("STEEL FORECAST", "검격 예측")
		"witness": return _bl("PRESERVATION ROUTE", "보존 경로")
		"burn": return _bl("IRREVERSIBLE COST", "되돌릴 수 없는 대가")
		"defend": return _bl("SAFE TEMPO", "안전한 박자")
		"item": return _bl("FIELD KIT", "전투 도구")
		"limit": return _bl("CASCADE STATUS", "캐스케이드 상태")
		"auto": return _bl("AUTO TACTIC", "자동 전술")
		"flee": return _bl("EXIT CHECK", "이탈 판정")
	return _bl("FIELD READ", "전장 판독")

## S226: One colour language for "this helps / this costs / this loses the objective".
func _objective_relation_color(kind: String) -> Color:
	match kind:
		"advance":
			return Color(0.62, 0.98, 0.66)
		"risk":
			return Color(1.0, 0.78, 0.36)
		"fail":
			return Color(1.0, 0.36, 0.30)
	return UITheme.TEXT_PRIMARY

func _action_forecast_color(action_id: String) -> Color:
	match action_id:
		"burn": return Color(1.0, 0.44, 0.36)
		"witness": return Color(0.56, 0.84, 1.0)
		"limit": return Color(0.92, 0.62, 1.0)
		"defend": return Color(0.60, 0.82, 1.0)
		"flee": return Color(0.72, 0.68, 0.62)
	return Color(0.96, 0.74, 0.36)

func _action_description(action_id: String) -> String:
	match action_id:
		"attack":
			var break_left := maxi(0, ceili(BattleManager.BREAK_MAX - BattleManager.enemy_break_gauge))
			var weakness := String(BattleManager.current_enemy.weakness).to_upper() if BattleManager.current_enemy and BattleManager.current_enemy.weakness != "" else _bl("unknown", "미확인")
			return _bl("Build BREAK. %d pressure remains; weakness: %s.", "브레이크를 쌓습니다. 남은 압박 %d, 약점: %s.") % [break_left, weakness]
		"witness":
			var witness_state := BattleManager.get_witness_state()
			return _bl("Read the trapped echo %d/%d. Ordinary foes can be released without burning.", "갇힌 메아리를 읽습니다 %d/%d. 일반 적은 연소 없이 해방할 수 있습니다.") % [int(witness_state.progress), int(witness_state.required)]
		"burn":
			var available_count := MemoryManager.get_available_memories().filter(func(memory): return not memory.is_faded).size()
			return _bl("%d memories available. High power, but the chosen memory is gone after battle.", "태울 수 있는 기억 %d개. 강력하지만 선택한 기억은 전투 뒤에도 돌아오지 않습니다.") % available_count
		"defend":
			return _bl("Halve the next blow, steady ailments, and build Limit without sacrificing memory.", "다음 피해를 절반으로 줄이고 상태이상을 다스리며 기억 희생 없이 리밋을 쌓습니다.")
		"item":
			var item_total := 0
			for count in GameManager.player_data.items.values():
				item_total += int(count)
			return _bl("%d carried tools. Heal, cleanse, disrupt, or create an opening.", "보유 도구 %d개. 회복, 정화, 방해로 틈을 만듭니다.") % item_total
		"limit":
			return _bl("Memory Cascade charge: %d/100. No memory is burned.", "메모리 캐스케이드 충전: %d/100. 기억은 연소하지 않습니다.") % int(BattleManager.limit_gauge)
		"auto":
			return _bl("Use the current stance, items, and preservation rules automatically.", "현재 자세, 아이템, 보존 규칙을 기준으로 자동 행동합니다.")
		"flee":
			if BattleManager.current_enemy and BattleManager.current_enemy.is_boss:
				return _bl("This threshold is sealed. Boss encounters cannot be abandoned.", "이 경계는 봉쇄되었습니다. 보스 전투에서는 이탈할 수 없습니다.")
			return _bl("Attempt to leave. Failure gives the enemy the rhythm.", "전투 이탈을 시도합니다. 실패하면 적에게 흐름을 내줍니다.")
	return ""

func _set_field_readout(header: String, detail: String, accent: Color) -> void:
	if field_readout_header:
		field_readout_header.text = header
		field_readout_header.add_theme_color_override("font_color", accent)
	if log_label:
		log_label.text = detail

func _restore_field_readout() -> void:
	if _last_battle_message != "":
		_set_field_readout(_bl("LAST BEAT", "최근 전황"), _last_battle_message, Color(0.74, 0.72, 0.68))
	else:
		_set_field_readout(
			_bl("FIELD READ", "전장 판독"),
			_bl("Choose an action to preview its purpose and cost.", "행동을 가리키면 목적과 대가를 미리 읽을 수 있습니다."),
			Color(0.48, 0.78, 1.0)
		)

func _build_burn_list(root: Control) -> void:
	var scroll = ScrollContainer.new()
	scroll.anchor_left = 0.15
	scroll.anchor_right = 0.85
	scroll.anchor_top = 0.35
	scroll.anchor_bottom = 0.78
	scroll.visible = false

	var panel = PanelContainer.new()
	panel.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	panel.size_flags_vertical = Control.SIZE_EXPAND_FILL
	var style = StyleBoxFlat.new()
	style.bg_color = Color(0.06, 0.04, 0.08, 0.96)
	style.border_color = Color(0.5, 0.3, 0.2, 0.7)
	style.set_border_width_all(1)
	style.set_corner_radius_all(4)
	style.set_content_margin_all(12)
	panel.add_theme_stylebox_override("panel", style)
	scroll.add_child(panel)

	burn_list_container = VBoxContainer.new()
	burn_list_container.add_theme_constant_override("separation", 6)
	panel.add_child(burn_list_container)

	root.add_child(scroll)
	burn_list_container.set_meta("scroll_parent", scroll)

## ===================== 시그널 연결 =====================

func _connect_signals() -> void:
	BattleManager.battle_log.connect(_on_battle_log)
	BattleManager.damage_dealt.connect(_on_damage_dealt)
	BattleManager.item_used.connect(_on_item_used)
	BattleManager.player_turn_started.connect(_on_player_turn)
	BattleManager.enemy_turn_started.connect(_on_enemy_turn)
	BattleManager.battle_ended.connect(_on_battle_ended)
	BattleManager.status_changed.connect(_on_status_changed)
	BattleManager.guard_focus.connect(_on_guard_focus)
	BattleManager.limit_changed.connect(_on_limit_changed)
	BattleManager.phase_changed.connect(_on_phase_changed)  # S46
	BattleManager.echo_activated.connect(_on_echo_activated)  # S51
	BattleManager.stance_changed.connect(_on_stance_changed)  # S51
	BattleManager.enemy_scanned.connect(_on_enemy_scanned)  # S55: scan display
	BattleManager.break_changed.connect(_on_break_changed)
	BattleManager.enemy_broken.connect(_on_enemy_broken)
	BattleManager.environment_info.connect(_on_environment_info)  # S55: env display
	BattleManager.auto_battle_changed.connect(_on_auto_battle_changed)  # S55: auto battle
	BattleManager.pre_attack.connect(_on_pre_attack)  # S58: anticipation
	BattleManager.victory_rewards_ready.connect(_on_victory_rewards_ready)  # S58: animated rewards
	BattleManager.enemy_ability_telegraph.connect(_on_enemy_ability_telegraph)  # S59: ability warning
	BattleManager.tactical_objective_changed.connect(_on_tactical_objective_changed)
	BattleManager.tactical_objective_options_changed.connect(_on_tactical_objective_options_changed)
	BattleManager.momentum_changed.connect(_on_momentum_changed)
	BattleManager.last_stand_resonance.connect(_on_last_stand_resonance)
	BattleManager.ally_action.connect(_on_ally_action)
	BattleManager.witness_changed.connect(_on_witness_changed)

	if BattleManager.current_enemy:
		_setup_enemy_display()
	if not BattleManager.tactical_objective.is_empty():
		_on_tactical_objective_changed(BattleManager.tactical_objective)
	if not BattleManager.tactical_objective_options.is_empty():
		_on_tactical_objective_options_changed(BattleManager.tactical_objective_options)
	_on_momentum_changed(BattleManager.momentum, BattleManager.momentum_rank, BattleManager._get_momentum_label())
	var witness_state := BattleManager.get_witness_state()
	_on_witness_changed(witness_state.progress, witness_state.required, "", witness_state.complete)

func _exit_tree() -> void:
	_stop_burn_preview_motion()
	# 오토로드 시그널 연결 해제, 씬 재진입 시 freed 객체 참조 방지
	if BattleManager.battle_log.is_connected(_on_battle_log):
		BattleManager.battle_log.disconnect(_on_battle_log)
	if BattleManager.damage_dealt.is_connected(_on_damage_dealt):
		BattleManager.damage_dealt.disconnect(_on_damage_dealt)
	if BattleManager.item_used.is_connected(_on_item_used):
		BattleManager.item_used.disconnect(_on_item_used)
	if BattleManager.player_turn_started.is_connected(_on_player_turn):
		BattleManager.player_turn_started.disconnect(_on_player_turn)
	if BattleManager.enemy_turn_started.is_connected(_on_enemy_turn):
		BattleManager.enemy_turn_started.disconnect(_on_enemy_turn)
	if BattleManager.battle_ended.is_connected(_on_battle_ended):
		BattleManager.battle_ended.disconnect(_on_battle_ended)
	if BattleManager.status_changed.is_connected(_on_status_changed):
		BattleManager.status_changed.disconnect(_on_status_changed)
	if BattleManager.guard_focus.is_connected(_on_guard_focus):
		BattleManager.guard_focus.disconnect(_on_guard_focus)
	if BattleManager.limit_changed.is_connected(_on_limit_changed):
		BattleManager.limit_changed.disconnect(_on_limit_changed)
	if BattleManager.phase_changed.is_connected(_on_phase_changed):
		BattleManager.phase_changed.disconnect(_on_phase_changed)
	if BattleManager.enemy_scanned.is_connected(_on_enemy_scanned):
		BattleManager.enemy_scanned.disconnect(_on_enemy_scanned)
	if BattleManager.break_changed.is_connected(_on_break_changed):
		BattleManager.break_changed.disconnect(_on_break_changed)
	if BattleManager.enemy_broken.is_connected(_on_enemy_broken):
		BattleManager.enemy_broken.disconnect(_on_enemy_broken)
	if BattleManager.environment_info.is_connected(_on_environment_info):
		BattleManager.environment_info.disconnect(_on_environment_info)
	if BattleManager.auto_battle_changed.is_connected(_on_auto_battle_changed):
		BattleManager.auto_battle_changed.disconnect(_on_auto_battle_changed)
	if BattleManager.pre_attack.is_connected(_on_pre_attack):
		BattleManager.pre_attack.disconnect(_on_pre_attack)
	if BattleManager.victory_rewards_ready.is_connected(_on_victory_rewards_ready):
		BattleManager.victory_rewards_ready.disconnect(_on_victory_rewards_ready)
	if BattleManager.enemy_ability_telegraph.is_connected(_on_enemy_ability_telegraph):
		BattleManager.enemy_ability_telegraph.disconnect(_on_enemy_ability_telegraph)
	if BattleManager.tactical_objective_changed.is_connected(_on_tactical_objective_changed):
		BattleManager.tactical_objective_changed.disconnect(_on_tactical_objective_changed)
	if BattleManager.tactical_objective_options_changed.is_connected(_on_tactical_objective_options_changed):
		BattleManager.tactical_objective_options_changed.disconnect(_on_tactical_objective_options_changed)
	if BattleManager.momentum_changed.is_connected(_on_momentum_changed):
		BattleManager.momentum_changed.disconnect(_on_momentum_changed)
	if BattleManager.last_stand_resonance.is_connected(_on_last_stand_resonance):
		BattleManager.last_stand_resonance.disconnect(_on_last_stand_resonance)
	if BattleManager.ally_action.is_connected(_on_ally_action):
		BattleManager.ally_action.disconnect(_on_ally_action)
	if BattleManager.witness_changed.is_connected(_on_witness_changed):
		BattleManager.witness_changed.disconnect(_on_witness_changed)
	# S56: Cleanup battle VFX status particles
	if battle_vfx:
		battle_vfx.cleanup_status_particles()

func _setup_enemy_display() -> void:
	var enemy = BattleManager.current_enemy
	enemy_name_label.text = GameManager.localized_enemy_name(enemy.name)
	enemy_hp_bar.max_value = enemy.max_hp
	enemy_hp_bar.value = enemy.hp
	if enemy_hp_ghost:
		enemy_hp_ghost.max_value = enemy.max_hp
		enemy_hp_ghost.value = enemy.hp
	if enemy_break_bar:
		enemy_break_bar.max_value = BattleManager.BREAK_MAX
		enemy_break_bar.value = BattleManager.enemy_break_gauge

	if enemy.is_void_beast:
		enemy_name_label.add_theme_color_override("font_color", Color(0.65, 0.25, 0.65))
	if enemy.is_boss:
		enemy_name_label.add_theme_color_override("font_color", Color(0.9, 0.35, 0.25))

## S230: 앞 막대는 바로, 잔상은 잠깐 머문 뒤 따라 내려간다.
## 회복이면 잔상이 먼저 올라가 앞 막대를 기다린다.
func _drive_hp_pair(bar: ProgressBar, ghost: ProgressBar, value: float, animate: bool) -> Tween:
	if bar == null:
		return null
	if ghost:
		ghost.max_value = bar.max_value
	if not animate:
		bar.value = value
		if ghost:
			ghost.value = value
		return null
	var tween := create_tween().set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_CUBIC)
	tween.tween_property(bar, "value", value, 0.4)
	if ghost:
		if value < ghost.value:
			tween.parallel().tween_property(ghost, "value", value, 0.45).set_delay(0.32)
		else:
			ghost.value = value
	return tween

func _update_hp_displays(animate: bool = false) -> void:
	# S230: 첫 표시는 절대 애니메이션하지 않는다.
	# 예전에는 막대가 0에서 현재 HP까지 쓸려 올라가, 전투 시작 순간에
	# "회복 중"으로 읽히고 피해 잔상과 색이 두 갈래로 갈라졌다.
	if not _hp_display_primed:
		animate = false
		_hp_display_primed = true
	# 플레이어 HP
	player_hp_bar.max_value = GameManager.player_data.max_hp
	var p_hp = GameManager.player_data.hp
	player_hp_label.text = "HP %d / %d" % [p_hp, GameManager.player_data.max_hp]

	var p_ratio = float(p_hp) / max(GameManager.player_data.max_hp, 1)
	var p_fill = player_hp_bar.get_theme_stylebox("fill") as StyleBoxFlat
	if p_fill:
		p_fill.bg_color = UITheme.HP_LOW if p_ratio <= 0.25 else UITheme.HP_PLAYER
	# 위험 구간은 색만이 아니라 수치 색까지 같이 경고한다.
	player_hp_label.add_theme_color_override("font_color", Color(1.0, 0.66, 0.56) if p_ratio <= 0.25 else Color(0.82, 0.90, 1.0))

	if hp_tween_player:
		hp_tween_player.kill()
	hp_tween_player = _drive_hp_pair(player_hp_bar, player_hp_ghost, float(p_hp), animate)

	# 적 HP, Ash Sight 패시브 또는 스캔 시에만 수치 표시
	if BattleManager.current_enemy:
		var e = BattleManager.current_enemy
		var revealed: bool = MemoryManager.has_passive("ash_sight") or e.name in BattleManager.scanned_enemies
		enemy_hp_label.text = ("HP %d / %d" % [e.hp, e.max_hp]) if revealed else _bl("HP ? / ?", "HP ? / ?")
		if enemy_scan_chip:
			# S230: "HP ??? / ???"만 보여 주면 왜 가려졌는지 알 수 없다.
			# 판독 여부를 상태로 적어, 스캔이 무엇을 여는지 알린다.
			enemy_scan_chip.text = _bl("READ", "판독됨") if revealed else _bl("UNREAD", "미판독")
			enemy_scan_chip.add_theme_color_override("font_color", Color(0.66, 0.88, 0.78) if revealed else Color(0.70, 0.66, 0.62))

		enemy_hp_bar.max_value = e.max_hp
		var e_ratio = float(e.hp) / max(e.max_hp, 1)
		var e_fill = enemy_hp_bar.get_theme_stylebox("fill") as StyleBoxFlat
		if e_fill:
			e_fill.bg_color = UITheme.HP_LOW if e_ratio <= 0.25 else UITheme.HP_ENEMY

		if hp_tween_enemy:
			hp_tween_enemy.kill()
		hp_tween_enemy = _drive_hp_pair(enemy_hp_bar, enemy_hp_ghost, float(e.hp), animate)

	if enemy_break_bar:
		enemy_break_bar.max_value = BattleManager.BREAK_MAX
		enemy_break_bar.value = BattleManager.enemy_break_gauge
	if enemy_break_label:
		var broken := BattleManager.enemy_broken_turns > 0
		enemy_break_label.text = _bl("BROKEN", "붕괴") if broken else _bl("BREAK", "브레이크")
		enemy_break_label.add_theme_color_override(
			"font_color",
			Color(1.0, 0.82, 0.28) if broken else Color(0.86, 0.72, 0.44)
		)

	# 상태 아이콘 업데이트
	_update_status_icons()

## ===================== 전투 로그 =====================

func _on_break_changed(value: float, max_value: float) -> void:
	if not enemy_break_bar:
		return
	enemy_break_bar.max_value = max_value
	var tw = create_tween()
	tw.tween_property(enemy_break_bar, "value", value, 0.22).set_trans(Tween.TRANS_CUBIC).set_ease(Tween.EASE_OUT)
	if enemy_break_label:
		enemy_break_label.text = "BROKEN" if BattleManager.enemy_broken_turns > 0 else "BREAK"

func _on_enemy_broken(enemy_name: String) -> void:
	_set_battle_stage_focus("enemy_break")
	_update_status_icons()
	_show_turn_indicator("BREAK: %s" % enemy_name, Color(1.0, 0.72, 0.24))
	_show_combat_cue(
		_bl("FAULT LINE OPEN", "균열 노출"),
		_bl("The enemy loses its next turn. Press the opening.", "적이 다음 턴을 잃습니다. 틈을 밀어붙이세요."),
		BREAK_FAULTLINE_CUTIN_PATH,
		Color(0.96, 0.76, 0.30),
		1.4
	)
	_play_action_cutin(BREAK_FAULTLINE_CUTIN_PATH, true, 0.80, 0.30)
	if enemy_sprite_container:
		var tw = create_tween()
		tw.tween_property(enemy_sprite_container, "scale", Vector2(1.12, 0.88), 0.08).set_trans(Tween.TRANS_BACK)
		tw.tween_property(enemy_sprite_container, "scale", Vector2(1.0, 1.0), 0.16).set_trans(Tween.TRANS_ELASTIC)
	_screen_shake(1.7)

func _on_battle_log(message: String) -> void:
	log_lines.append(message)
	if log_lines.size() > MAX_LOG_LINES:
		log_lines = log_lines.slice(-MAX_LOG_LINES)
	_last_battle_message = message
	_set_field_readout(_bl("LAST BEAT", "최근 전황"), message, Color(0.74, 0.72, 0.68))
	# S54: 보상 메시지 수집 (승리 화면용)
	if "Gained" in message or "Recovered" in message or "Found:" in message:
		_victory_rewards.append(message)

func _on_item_used(item_id: String, item_type: String) -> void:
	var cutin_path := String(ITEM_ACTION_CUTIN_PATHS.get(item_type, ""))
	if cutin_path == "" or not ResourceLoader.exists(cutin_path):
		return
	var item_def: Dictionary = GameManager.ITEMS.get(item_id, {})
	var item_name := String(item_def.get("name", item_id))
	var cue_title := ""
	var cue_detail := ""
	var accent := _get_item_type_color(item_type)
	match item_type:
		"heal":
			cue_title = _bl("FIELD RECOVERY", "현장 회복")
			cue_detail = _bl("%s stitches strength back into the present.", "%s이(가) 현재에 힘을 다시 꿰맨다.") % item_name
		"cure":
			cue_title = _bl("STATUS PURGED", "상태 정화")
			cue_detail = _bl("%s washes poison and cinders from the body.", "%s이(가) 독과 잿불을 씻어낸다.") % item_name
		"burn":
			cue_title = _bl("CINDER RELEASE", "잿불 방출")
			cue_detail = _bl("%s turns one opening into a burning threat.", "%s이(가) 빈틈을 불타는 위협으로 바꾼다.") % item_name
		"flee":
			cue_title = _bl("TACTICAL WITHDRAWAL", "전술 철수")
			cue_detail = _bl("%s breaks the hostile line before it closes.", "%s이(가) 적의 포위선이 닫히기 전에 끊어낸다.") % item_name
		"witness":
			cue_title = _bl("WITNESS TRACE", "증언 추적")
			cue_detail = _bl("%s reveals what force alone would erase.", "%s이(가) 힘만으로는 지워질 진실을 드러낸다.") % item_name
		"guard":
			cue_title = _bl("ANCHOR SET", "앵커 고정")
			cue_detail = _bl("%s seals the next impact and steadies resolve.", "%s이(가) 다음 충격을 봉인하고 의지를 붙든다.") % item_name
		"scan":
			cue_title = _bl("FAULT MAPPED", "균열 분석")
			cue_detail = _bl("%s turns a hidden fracture into BREAK pressure.", "%s이(가) 숨은 균열을 브레이크 압박으로 바꾼다.") % item_name
	_show_combat_cue(cue_title, cue_detail, cutin_path, accent, 0.92)
	_play_action_cutin(cutin_path, true, 0.82, 0.34)

func _on_guard_focus(trigger: String, value: int) -> void:
	_update_status_icons()
	if not player_sprite_container:
		return

	var label = Label.new()
	match trigger:
		"status":
			label.text = _bl("GUARD FOCUS\nAilment -1", "방어 집중\n상태이상 -1")
		"heal":
			label.text = _bl("GUARD FOCUS\n+%d HP", "방어 집중\n+%d HP") % value
		"limit":
			label.text = _bl("GUARD FOCUS\nLimit +%d", "방어 집중\n리밋 +%d") % value
		_:
			label.text = _bl("GUARD FOCUS", "방어 집중")
	label.add_theme_font_size_override("font_size", 16)
	label.add_theme_color_override("font_color", Color(0.72, 0.88, 1.0, 1.0))
	label.add_theme_color_override("font_outline_color", Color(0.02, 0.04, 0.08, 0.85))
	label.add_theme_constant_override("outline_size", 2)
	label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	label.position = player_sprite_container.position + Vector2(-78, -122)
	add_child(label)

	var shield = ColorRect.new()
	shield.size = Vector2(150, 84)
	shield.position = player_sprite_container.position + Vector2(-75, -86)
	shield.color = Color(0.25, 0.55, 0.9, 0.13)
	shield.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(shield)

	var tw = create_tween().set_parallel(true)
	tw.tween_property(label, "position:y", label.position.y - 24.0, 0.75).set_trans(Tween.TRANS_SINE).set_ease(Tween.EASE_OUT)
	tw.tween_property(label, "modulate:a", 0.0, 0.75).set_delay(0.28)
	tw.tween_property(shield, "scale", Vector2(1.35, 1.35), 0.45).set_trans(Tween.TRANS_SINE).set_ease(Tween.EASE_OUT)
	tw.tween_property(shield, "modulate:a", 0.0, 0.55)
	tw.finished.connect(func():
		if is_instance_valid(label):
			label.queue_free()
		if is_instance_valid(shield):
			shield.queue_free()
	)

func _on_last_stand_resonance(lethal: bool) -> void:
	_update_status_icons()
	_play_action_cutin(LAST_STAND_CUTIN_PATH, true, 0.94, 0.82 if lethal else 0.58)
	_show_turn_indicator(_bl("LAST STAND", "최후의 저항"), Color(0.62, 0.82, 1.0))
	_screen_shake(3.2 if lethal else 2.1)
	if has_node("/root/AudioManager"):
		AudioManager.play_sfx("phase_change")
		AudioManager.play_sfx("heal")
	if hit_flash_rect:
		hit_flash_rect.color = Color(0.34, 0.64, 1.0, 0.34)
		var flash_t := create_tween()
		flash_t.tween_property(hit_flash_rect, "color:a", 0.0, 0.38).set_trans(Tween.TRANS_SINE)
	if player_sprite_container:
		var shield = ColorRect.new()
		shield.size = Vector2(190, 104)
		shield.position = player_sprite_container.position + Vector2(-95, -96)
		shield.color = Color(0.34, 0.66, 1.0, 0.16)
		shield.mouse_filter = Control.MOUSE_FILTER_IGNORE
		shield.z_index = 20
		add_child(shield)
		var tw := create_tween().set_parallel(true)
		tw.tween_property(shield, "scale", Vector2(1.65, 1.65), 0.62).set_trans(Tween.TRANS_SINE).set_ease(Tween.EASE_OUT)
		tw.tween_property(shield, "modulate:a", 0.0, 0.72)
		tw.finished.connect(func():
			if is_instance_valid(shield):
				shield.queue_free()
		)

func _on_damage_dealt(target: String, amount: int, skill_name: String) -> void:
	_update_hp_displays(true)

	# S57: Hit-freeze frame via Engine.time_scale (feels more impactful than pause)
	var hit_stop_dur = 0.0
	if amount >= 200:
		hit_stop_dur = 0.12
	elif amount >= 80:
		hit_stop_dur = 0.08
	elif amount >= 30:
		hit_stop_dur = 0.05
	if hit_stop_dur > 0 and not OptionsMenu.is_reduce_motion():
		var prev_scale = Engine.time_scale
		Engine.time_scale = 0.0
		await BattleManager.pace_timer(hit_stop_dur, true, false, true).timeout
		Engine.time_scale = prev_scale

	# S58: Impact squash on enemy hit (stretch vertically = getting compressed by blow)
	if target != "Arrel" and enemy_sprite:
		_play_actor_anim(enemy_sprite, "hurt", true)
		var enemy_hurt_generation := _painterly_semantic_generation(enemy_sprite)
		if not OptionsMenu.is_reduce_motion():
			var impact_t = create_tween()
			impact_t.tween_property(enemy_sprite, "scale", Vector2(1.15, 0.85), 0.04).set_ease(Tween.EASE_OUT)
			impact_t.tween_property(enemy_sprite, "scale", Vector2(0.95, 1.05), 0.06).set_ease(Tween.EASE_OUT)
			impact_t.tween_property(enemy_sprite, "scale", Vector2(1.0, 1.0), 0.08).set_ease(Tween.EASE_IN_OUT)
			impact_t.tween_callback(_settle_painterly_semantic_state.bind(enemy_sprite, "hurt", enemy_hurt_generation))
	# S58: Player squash on getting hit (squash = pain compression)
	elif target == "Arrel" and player_sprite:
		# S151: 시트 hurt 동사 재생 + 기본 스케일 기준 상대 스쿼시
		_play_actor_anim(player_sprite, "hurt", true)
		var player_hurt_generation := _painterly_semantic_generation(player_sprite)
		if not OptionsMenu.is_reduce_motion():
			var hurt_squash = create_tween()
			hurt_squash.tween_property(player_sprite, "scale", _player_sprite_base_scale * Vector2(0.85, 1.15), 0.05).set_ease(Tween.EASE_OUT)
			hurt_squash.tween_property(player_sprite, "scale", _player_sprite_base_scale * Vector2(1.05, 0.95), 0.07).set_ease(Tween.EASE_OUT)
			hurt_squash.tween_property(player_sprite, "scale", _player_sprite_base_scale, 0.1).set_ease(Tween.EASE_IN_OUT)
			hurt_squash.tween_callback(_settle_painterly_semantic_state.bind(player_sprite, "hurt", player_hurt_generation))

	# S56: Enhanced damage numbers via BattleVFX
	if battle_vfx:
		battle_vfx.show_damage_number(target, amount, skill_name)
	else:
		_show_damage_number(target, amount, skill_name)
	# S46: VFX Library 셰이더 피격 플래시 (flash_white)
	_apply_hit_shader(target, amount)
	_hit_flash(target)
	# S56: Enhanced camera shake scaling with damage amount
	if battle_vfx:
		battle_vfx.damage_screen_shake(amount)
	else:
		var shake_intensity = clampf(float(amount) / 60.0, 0.5, 3.0)
		_screen_shake(shake_intensity)

	# S57: Screen flash on big hits, white at 80+, red at 150+
	if amount >= 80 and target != "Arrel":
		_play_big_hit_screen_flash(amount)

	# S52: 크리티컬 히트 줌 펀치 (200+ 데미지)
	if amount >= 200 and target != "Arrel":
		_critical_zoom_punch()

	# S59: Critical hit cinematic cut-in (150+ damage)
	if amount >= 150 and target != "Arrel" and battle_vfx:
		battle_vfx.play_critical_cinematic()

	# S56: Skill-specific element particle effects
	if skill_name != "" and target != "Arrel":
		_play_attack_vfx(skill_name)
		# S56: Element-specific particle burst
		if battle_vfx:
			var element = _detect_skill_element(skill_name)
			battle_vfx.play_element_particles(element)
	elif target != "Arrel":
		_play_slash_vfx()
		_play_speed_lines()  # S44: 속도선
		# S56: Physical sparks
		if battle_vfx:
			battle_vfx.play_element_particles("physical")

## S58: Anticipation + Follow-through attack sequence
## Plays wind-up squash, lunge strike, then follow-through return
## Called BEFORE damage is dealt, BattleManager awaits timer in parallel
func _on_pre_attack(attacker: String, target: String, skill_name: String) -> void:
	var player_attacker := attacker == "Arrel" or attacker.to_lower() == "player"
	if _hybrid_depth_stage != null and is_instance_valid(_hybrid_depth_stage):
		var depth_direction := 1.0 if player_attacker else -1.0
		var depth_strength := 1.2 if skill_name == "Memory Cascade" else 0.82
		_hybrid_depth_stage.pulse_impact(depth_direction, depth_strength)
	if player_attacker:
		var cutin_path := ARREL_BLADE_CUTIN_PATH
		var cue_title := _bl("MEMORY BLADE", "기억의 검격")
		var cue_detail := _bl("A clean strike creates BREAK pressure.", "정확한 검격이 브레이크 압박을 쌓습니다.")
		if skill_name == "Memory Cascade":
			cutin_path = MEMORY_CASCADE_CUTIN_PATH
			cue_title = _bl("MEMORY CASCADE", "메모리 캐스케이드")
			cue_detail = _bl("Every remembered wound answers at once.", "기억된 모든 상처가 한꺼번에 응답합니다.")
		elif skill_name != "" and skill_name != "Attack" and BattleManager._burn_chain >= 2:
			cutin_path = ARREL_CHAIN_BURN_CUTIN_PATH
			cue_title = _bl("MEMORY CHAIN x%d" % BattleManager._burn_chain, "기억 연쇄 x%d" % BattleManager._burn_chain)
			cue_detail = _bl("One sacrifice ignites the next before the ash can settle.", "첫 희생의 재가 가라앉기 전에 다음 기억이 불붙습니다.")
		elif skill_name != "" and skill_name != "Attack":
			cutin_path = "res://assets/cg/game_image/memory_loss_warning.png"
			cue_title = _bl("MEMORY BURN", "기억 연소")
			cue_detail = _bl("Power rises. The cost remains after the fight.", "힘은 오르지만, 대가는 전투 뒤에도 남습니다.")
		_show_combat_cue(cue_title, cue_detail, cutin_path, Color(0.62, 0.82, 1.0), 0.78)
		_play_action_cutin(cutin_path, true, 0.70)
	else:
		_play_action_cutin(_resolve_enemy_action_cutin(attacker), false, 0.62)

	# S59: Battle background parallax shift in attack direction
	if battle_vfx and bg:
		var direction = 1.0 if player_attacker else -1.0  # player attacks right, enemy attacks left
		battle_vfx.parallax_attack_shift(bg, direction, 8.0)
	if player_attacker:
		# --- Player attacking enemy ---
		if not player_sprite or not player_sprite_container:
			return
		# S151: 시트 동사 재생, 평타는 attack, 연소/스킬은 cast
		var verb := "attack" if (skill_name == "" or skill_name == "Attack") else "cast"
		_play_actor_anim(player_sprite, verb, true)
		var player_action_generation := _painterly_semantic_generation(player_sprite)
		if OptionsMenu.is_reduce_motion():
			return
		var rush_target = Vector2(_enemy_base_pos.x - 180, _player_base_pos.y - 10)
		# Phase 1: Anticipation, squash (wind-up, lean back)
		var antic_t = create_tween()
		antic_t.tween_property(player_sprite, "scale", _player_sprite_base_scale * Vector2(1.1, 0.9), 0.1).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_QUAD)
		antic_t.set_parallel(true)
		antic_t.tween_property(player_sprite_container, "position:x", _player_base_pos.x - 8, 0.1).set_ease(Tween.EASE_OUT)
		await antic_t.finished
		# Phase 2: Strike, stretch + lunge forward
		var strike_t = create_tween()
		strike_t.set_parallel(true)
		strike_t.tween_property(player_sprite, "scale", _player_sprite_base_scale * Vector2(0.9, 1.1), 0.08).set_ease(Tween.EASE_IN).set_trans(Tween.TRANS_BACK)
		strike_t.tween_property(player_sprite_container, "position", rush_target, 0.08).set_ease(Tween.EASE_IN).set_trans(Tween.TRANS_BACK)
		await strike_t.finished
		# Phase 3: Impact freeze (brief hold at contact point, damage applied by BattleManager during this)
		await BattleManager.pace_timer(0.05).timeout
		# Phase 4: Follow-through, return with overshoot
		var return_t = create_tween()
		return_t.tween_property(player_sprite_container, "position", _player_base_pos + Vector2(-5, 0), 0.12).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_CUBIC)
		return_t.tween_property(player_sprite_container, "position", _player_base_pos, 0.08).set_ease(Tween.EASE_IN_OUT)
		# Scale reset runs in parallel with the position return
		var scale_t = create_tween()
		scale_t.tween_property(player_sprite, "scale", _player_sprite_base_scale, 0.15).set_ease(Tween.EASE_OUT)
		scale_t.tween_callback(_settle_painterly_semantic_state.bind(player_sprite, verb, player_action_generation))
	else:
		# --- Enemy attacking player ---
		if not enemy_sprite or not enemy_sprite_container:
			return
		var enemy_verb := "attack" if (skill_name == "" or skill_name == "Attack") else "cast"
		_play_actor_anim(enemy_sprite, enemy_verb, true)
		var enemy_action_generation := _painterly_semantic_generation(enemy_sprite)
		if OptionsMenu.is_reduce_motion():
			return
		# Phase 1: Anticipation, enemy squash (coil back)
		var antic_t = create_tween()
		antic_t.tween_property(enemy_sprite, "scale", Vector2(1.1, 0.9), 0.1).set_ease(Tween.EASE_OUT)
		antic_t.set_parallel(true)
		antic_t.tween_property(enemy_sprite_container, "position:x", _enemy_base_pos.x + 8, 0.1).set_ease(Tween.EASE_OUT)
		await antic_t.finished
		# Phase 2: Strike, enemy lunges LEFT toward player
		var strike_t = create_tween()
		strike_t.set_parallel(true)
		strike_t.tween_property(enemy_sprite, "scale", Vector2(0.9, 1.1), 0.08).set_ease(Tween.EASE_IN)
		strike_t.tween_property(enemy_sprite_container, "position:x", _player_base_pos.x + 200, 0.08).set_ease(Tween.EASE_IN).set_trans(Tween.TRANS_BACK)
		await strike_t.finished
		# Phase 3: Hold at impact
		await BattleManager.pace_timer(0.05).timeout
		# Phase 4: Follow-through, enemy returns with overshoot
		var return_t = create_tween()
		return_t.tween_property(enemy_sprite_container, "position:x", _enemy_base_pos.x + 5, 0.12).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_CUBIC)
		return_t.tween_property(enemy_sprite_container, "position:x", _enemy_base_pos.x, 0.08).set_ease(Tween.EASE_IN_OUT)
		# Scale reset runs in parallel with the position return
		var scale_t = create_tween()
		scale_t.tween_property(enemy_sprite, "scale", Vector2(1.0, 1.0), 0.15).set_ease(Tween.EASE_OUT)
		scale_t.tween_callback(_settle_painterly_semantic_state.bind(enemy_sprite, enemy_verb, enemy_action_generation))

func _on_player_turn() -> void:
	_set_battle_stage_focus("player")
	# S57: Turn transition dim effect
	_play_turn_dim()
	_show_turn_indicator(_bl("YOUR TURN", "당신의 턴"), Color(0.5, 0.65, 0.85))
	_update_turn_preview()  # S41
	_update_objective_action_warnings()  # S226: directive conflicts read before the choice
	# S59: Show enemy intent hint in battle log
	var hint = BattleManager.get_next_turn_hint()
	if hint != "":
		_on_battle_log(_bl("[INTEL] ", "[정보] ") + hint)
		_show_combat_cue(
			_bl("READ THE FIELD", "전장 관측"),
			hint,
			_resolve_enemy_stage_art(),
			Color(0.64, 0.80, 0.96),
			1.15
		)
	# S57: Enhanced combo counter display (persistent + escalating)
	if BattleManager.combo_count >= 2:
		_play_combo_burst(BattleManager.combo_count)
		_show_combo_counter(BattleManager.combo_count)
	# S55: Auto Battle, 자동 행동 (짧은 딜레이 후)
	if BattleManager.auto_battle:
		if objective_briefing_overlay and objective_briefing_overlay.visible:
			return
		await BattleManager.pace_timer(0.25).timeout
		action_container.visible = false
		BattleManager.auto_battle_action()
		return
	await BattleManager.pace_timer(0.5).timeout
	if objective_briefing_overlay and objective_briefing_overlay.visible:
		action_container.visible = false
		return
	action_container.visible = true
	if ally_cmd_container:
		ally_cmd_container.visible = BattleManager.sable_in_party
	if tobias_cmd_container:
		tobias_cmd_container.visible = BattleManager.tobias_in_party
	if stance_container:
		stance_container.visible = true
	if elia_skill_container:
		elia_skill_container.visible = GameManager.player_data.elia_with_party
		_refresh_elia_skills()
	_refresh_echo_display()
	_update_limit_button()
	if action_container.get_child_count() > 0:
		action_container.get_child(0).grab_focus()

func _on_enemy_turn() -> void:
	_set_battle_stage_focus("enemy")
	# S57: Turn transition dim effect
	_play_turn_dim()
	_show_turn_indicator(_bl("ENEMY TURN", "적의 턴"), Color(0.8, 0.4, 0.35))
	_update_turn_preview()  # S41
	if tobias_cmd_container:
		tobias_cmd_container.visible = false
	if stance_container:
		stance_container.visible = false
	if elia_skill_container:
		elia_skill_container.visible = false

## S59: Enemy ability telegraph, show warning VFX before special ability
func _on_enemy_ability_telegraph(ability_name: String, _delay: float) -> void:
	if battle_vfx and enemy_sprite_container:
		var enemy_pos = enemy_sprite_container.position
		battle_vfx.show_ability_warning(ability_name, enemy_pos)
	# Show turn hint about what's coming
	var hint = BattleManager.get_next_turn_hint()
	if hint != "":
		_on_battle_log(hint)
	var enemy_name := BattleManager.current_enemy.name if BattleManager.current_enemy else _bl("Unknown threat", "미확인 위협")
	_show_combat_cue(
		_bl("THREAT / ", "위협 / ") + _ability_display_name(ability_name),
		_ability_response_hint(ability_name),
		_resolve_enemy_action_cutin(enemy_name),
		Color(1.0, 0.54, 0.40),
		1.35
	)

func _ability_display_name(ability_name: String) -> String:
	var names_en := {
		"drain": "DRAIN", "shield": "SHIELD", "multi_hit": "MULTI HIT", "poison": "POISON",
		"burn_attack": "SCORCH", "weaken": "WEAKEN", "summon": "SUMMON", "void_pulse": "VOID PULSE",
		"despair": "DESPAIR", "stun": "STUN", "reflect": "REFLECT", "charge": "CHARGE",
	}
	var names_ko := {
		"drain": "흡수", "shield": "방벽", "multi_hit": "연격", "poison": "독", "burn_attack": "작열",
		"weaken": "약화", "summon": "소환", "void_pulse": "보이드 파동", "despair": "절망",
		"stun": "기절", "reflect": "반사", "charge": "충전",
	}
	return String(names_ko.get(ability_name, ability_name.replace("_", " "))) if GameManager.current_locale == "ko" else String(names_en.get(ability_name, ability_name.replace("_", " ").to_upper()))

func _ability_response_hint(ability_name: String) -> String:
	match ability_name:
		"charge":
			return _bl("Guard, interrupt with BREAK, or end the fight before the release.", "방어하거나 브레이크로 끊거나, 해방 전에 끝내세요.")
		"shield", "reflect":
			return _bl("Read the opening before committing your strongest strike.", "강한 일격을 넣기 전에 틈을 읽으세요.")
		"drain", "summon":
			return _bl("Pressure the source before it restores itself.", "회복하기 전에 근원을 압박하세요.")
		"poison", "burn_attack", "weaken":
			return _bl("A clean guard or item can preserve your next turn.", "방어나 아이템으로 다음 턴을 보존할 수 있습니다.")
		"stun", "multi_hit", "void_pulse", "despair":
			return _bl("Do not leave a fragile opening exposed.", "취약한 빈틈을 그대로 두지 마세요.")
	return _bl("Watch the rhythm, then answer with the right cost.", "리듬을 읽고, 알맞은 대가로 응답하세요.")

func _on_status_changed() -> void:
	_update_status_icons()
	_update_enemy_status_visual()  # S41: 상태이상 스프라이트 틴트
	_update_status_shaders()       # S46: VFX Library 상태이상 셰이더
	# S56: Status effect particle overlays on sprites
	if battle_vfx:
		battle_vfx.update_status_particles("enemy", enemy_sprite_container, enemy_sprite)
		battle_vfx.update_status_particles("player", player_sprite_container, player_sprite)
		# S56: Status icon bars above sprites with remaining turns
		battle_vfx.build_sprite_status_bar("enemy", enemy_sprite_container)
		battle_vfx.build_sprite_status_bar("player", player_sprite_container)

func _on_battle_ended(_result) -> void:
	action_container.visible = false
	if ally_cmd_container:
		ally_cmd_container.visible = false
	if tobias_cmd_container:
		tobias_cmd_container.visible = false
	_hide_burn_list()
	_hide_item_list()
	_hide_burn_preview()  # S58: dismiss preview if open
	# S56: Cleanup status effect particles
	if battle_vfx:
		battle_vfx.cleanup_status_particles()
	# S57: Enhanced enemy death, fade + shrink + particle burst, then dissolve
	if _result == BattleManager.BattleState.VICTORY and enemy_sprite:
		_play_enemy_death_animation()
		_play_enemy_dissolve()
	# S56: Victory fanfare VFX (confetti + golden flash)
	if _result == BattleManager.BattleState.VICTORY and battle_vfx:
		battle_vfx.play_victory_fanfare()
	# S56: Defeat dramatic slow-motion effect
	if _result == BattleManager.BattleState.DEFEAT and battle_vfx:
		battle_vfx.play_defeat_effect()
	# S151: 패배 시 쓰러짐 동사 (시트 보유 시)
	if _result == BattleManager.BattleState.DEFEAT:
		_play_actor_anim(player_sprite, "down")
	# S58: Victory rewards screen is now built by _on_victory_rewards_ready signal
	# (emitted from BattleManager._cleanup after rewards are computed)

## ===================== S151: 시트 동사 재생 헬퍼 =====================
## 시트 프레임(AnimatedSprite2D)일 때만 동사 재생. 논루프 동사는 끝나면 idle 복귀.
## 절차 생성 스프라이트(해당 애니 없음)에서는 조용히 무시, 폴백 안전.
func _play_actor_anim(actor: CanvasItem, anim: String, hold_painterly_state: bool = false) -> void:
	if actor == null or not is_instance_valid(actor):
		return
	if actor is AnimatedSprite2D:
		var asp: AnimatedSprite2D = actor
		if OptionsMenu.is_reduce_motion():
			_apply_animated_reduce_motion_semantic_state(asp, anim)
			return
		# A new normal-mode verb invalidates any delayed Reduce Motion settle before
		# preserving the existing SpriteFrames playback/completion behavior.
		_next_painterly_semantic_generation(asp)
		if not asp.sprite_frames or not asp.sprite_frames.has_animation(anim):
			return
		asp.play(anim)
		if not asp.sprite_frames.get_animation_loop(anim):
			var finished_callback := _on_actor_anim_finished.bind(asp)
			if not asp.animation_finished.is_connected(finished_callback):
				asp.animation_finished.connect(finished_callback)
		return
	# TextureRect character plates do not have sprite frames.  Give them the same
	# vocabulary with contained scale, tint, tilt, and persistent defeat state.
	_apply_painterly_semantic_state(actor, anim, hold_painterly_state)

func _on_actor_anim_finished(asp: AnimatedSprite2D) -> void:
	# down(패배)은 마지막 프레임 유지, 일어나면 어색함
	if not is_instance_valid(asp) or not asp.sprite_frames:
		return
	if asp.animation == "down":
		return
	if asp.sprite_frames.has_animation("idle"):
		asp.play("idle")

## ===================== 행동 콜백 =====================

func _on_attack() -> void:
	AudioManager.play_sfx("ui_select")
	if BattleManager.auto_battle:
		BattleManager.auto_battle = false
		BattleManager.auto_battle_changed.emit(false)
	action_container.visible = false
	_hide_burn_list()
	_hide_item_list()
	BattleManager.player_attack()

func _on_burn_menu() -> void:
	AudioManager.play_sfx("ui_select")
	_hide_item_list()
	_toggle_burn_list()

func _on_item_menu() -> void:
	AudioManager.play_sfx("ui_select")
	_hide_burn_list()
	_toggle_item_list()

func _on_defend() -> void:
	AudioManager.play_sfx("ui_select")
	action_container.visible = false
	_hide_burn_list()
	_hide_item_list()
	_show_combat_cue(
		_bl("HOLD THE LINE", "방어 태세"),
		_bl("Half the next blow and build Limit safely.", "다음 피해를 절반으로 줄이고 리밋을 안전하게 쌓습니다."),
		ARREL_GUARD_CUTIN_PATH,
		Color(0.62, 0.82, 1.0),
		1.0
	)
	_play_action_cutin(ARREL_GUARD_CUTIN_PATH, true, 0.80, 0.28)
	_play_actor_anim(player_sprite, "cast")
	BattleManager.player_defend()

func _on_witness() -> void:
	AudioManager.play_sfx("ui_select")
	action_container.visible = false
	_hide_burn_list()
	_hide_item_list()
	var witness_state := BattleManager.get_witness_state()
	_show_combat_cue(
		_bl("READ THE ECHO", "기억 읽기"),
		_bl("WITNESS %d/%d. Understanding can end a fight without burning.", "기억 읽기 %d/%d. 이해는 연소 없이 전투를 끝낼 수 있습니다.") % [int(witness_state.progress) + 1, int(witness_state.required)],
		ARREL_WITNESS_CUTIN_PATH,
		Color(0.82, 0.84, 1.0),
		1.15
	)
	_play_action_cutin(ARREL_WITNESS_CUTIN_PATH, true, 0.82, 0.34)
	_play_actor_anim(player_sprite, "cast")
	BattleManager.player_witness()

func _on_witness_changed(progress: int, required: int, echo_line: String, complete: bool) -> void:
	if witness_btn == null:
		return
	var base := "기억 읽기" if GameManager.current_locale == "ko" else "WITNESS"
	witness_btn.text = "2 · %s  %d/%d\n%s" % [base, progress, required, _bl("Preserve identity", "정체성 보존")]
	witness_btn.disabled = complete and BattleManager.current_enemy != null and BattleManager.current_enemy.is_boss
	if echo_line != "":
		witness_btn.tooltip_text = echo_line

func _on_auto_battle() -> void:
	AudioManager.play_sfx("ui_select")
	BattleManager.toggle_auto_battle()
	# If just enabled, immediately run auto action
	if BattleManager.auto_battle:
		action_container.visible = false
		_hide_burn_list()
		_hide_item_list()
		BattleManager.auto_battle_action()

func _on_flee() -> void:
	AudioManager.play_sfx("ui_select")
	action_container.visible = false
	_hide_burn_list()
	_hide_item_list()
	BattleManager.player_flee()

## ===================== 기억 연소 목록 =====================

func _toggle_burn_list() -> void:
	var scroll = burn_list_container.get_meta("scroll_parent") as ScrollContainer
	if scroll.visible:
		_hide_burn_list()
		return

	for child in burn_list_container.get_children():
		child.queue_free()

	var available = MemoryManager.get_available_memories().filter(func(m): return not m.is_faded)
	var residues = MemoryManager.get_residue_memories()
	if available.is_empty() and residues.is_empty():
		var empty_label = Label.new()
		empty_label.text = _bl("No memories left to burn.", "태울 기억이 남지 않았다.")
		empty_label.add_theme_font_size_override("font_size", 13)
		empty_label.add_theme_color_override("font_color", Color(0.5, 0.4, 0.35))
		burn_list_container.add_child(empty_label)
	else:
		if not available.is_empty():
			var title = Label.new()
			title.text = _bl("Select a memory to burn", "태울 기억을 선택")
			title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
			title.add_theme_font_size_override("font_size", 13)
			title.add_theme_color_override("font_color", Color(0.75, 0.5, 0.35))
			burn_list_container.add_child(title)

			# S226: If the active directive forbids burning, say so above the list.
			var burn_relation := BattleManager.get_objective_action_relation("burn")
			if String(burn_relation.get("kind", "")) == "fail":
				var conflict = Label.new()
				conflict.text = String(burn_relation.get("text", ""))
				conflict.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
				conflict.add_theme_font_size_override("font_size", 13)
				conflict.add_theme_color_override("font_color", _objective_relation_color("fail"))
				burn_list_container.add_child(conflict)

			for memory in available:
				var skill = BattleManager.BURN_SKILLS.get(memory.grade, BattleManager.BURN_SKILLS[0])
				var elem = skill.get("element", "fire").to_upper()
				var eff_power = MemoryManager.get_effective_burn_power(memory)
				var erosion_tag = "" if memory.erosion == 0 else " ⚠"
				var display_grade := 5 - int(memory.grade)
				var aftershock := BattleManager.get_burn_aftershock_preview(memory.grade)
				var aftershock_turns := int(aftershock.get("turns", 0))
				var risk_tag := ""
				if aftershock_turns > 0:
					risk_tag = (" · 잔상 %d회" if GameManager.current_locale == "ko" else " · AFTERIMAGE %d") % aftershock_turns
				var btn = Button.new()
				btn.text = _bl(
					"[%s|%s] LOSE: %s · G%d · DMG %d+%d%s%s",
					"[%s|%s] 소실: %s · 등급 %d · 피해 %d+%d%s%s"
				) % [
					skill.name, elem, MemoryManager.localized_memory_title(memory),
					display_grade,
					skill.base_damage, eff_power, erosion_tag, risk_tag
				]
				btn.alignment = HORIZONTAL_ALIGNMENT_LEFT

				var style = StyleBoxFlat.new()
				style.bg_color = Color(0.08, 0.06, 0.1, 0.85)
				style.set_content_margin_all(8)
				style.set_corner_radius_all(3)
				btn.add_theme_stylebox_override("normal", style)
				var hover_s = style.duplicate()
				hover_s.bg_color = Color(0.18, 0.1, 0.16, 0.95)
				hover_s.border_color = Color(0.7, 0.4, 0.3, 0.7)
				hover_s.set_border_width_all(1)
				btn.add_theme_stylebox_override("hover", hover_s)
				btn.add_theme_stylebox_override("focus", hover_s)
				btn.add_theme_font_size_override("font_size", 12)
				btn.add_theme_color_override("font_color", Color(0.7, 0.6, 0.55))
				btn.add_theme_color_override("font_hover_color", Color(0.95, 0.7, 0.4))

				var mid = memory.id
				var mem_ref = memory  # capture reference for preview
				btn.pressed.connect(func():
					AudioManager.play_sfx("ui_select")
					_hide_burn_list()
					# S58: Show burn preview popup instead of immediate execution
					_show_burn_preview(mem_ref)
				)
				btn.focus_entered.connect(func(): AudioManager.play_sfx("ui_hover"))
				burn_list_container.add_child(btn)

		# 잔존 기억 (Residue), 50% 데미지로 재사용
		if not residues.is_empty():
			var res_title = Label.new()
			res_title.text = _bl("Residue (50% power, no loss)", "잔존 (위력 50%, 소실 없음)")
			res_title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
			res_title.add_theme_font_size_override("font_size", 12)
			res_title.add_theme_color_override("font_color", Color(0.5, 0.4, 0.6))
			burn_list_container.add_child(res_title)

			for memory in residues:
				var skill = BattleManager.BURN_SKILLS.get(memory.grade, BattleManager.BURN_SKILLS[0])
				var half_dmg = int((skill.base_damage + memory.burn_power) * 0.5)
				var btn = Button.new()
				btn.text = _bl("[RESIDUE] %s, %s (DMG: ~%d)", "[잔존] %s, %s (피해 ~%d)") % [skill.name, MemoryManager.localized_memory_title(memory), half_dmg]
				btn.alignment = HORIZONTAL_ALIGNMENT_LEFT

				var style = StyleBoxFlat.new()
				style.bg_color = Color(0.06, 0.05, 0.1, 0.85)
				style.set_content_margin_all(8)
				style.set_corner_radius_all(3)
				btn.add_theme_stylebox_override("normal", style)
				var hover_s = style.duplicate()
				hover_s.bg_color = Color(0.12, 0.08, 0.18, 0.95)
				hover_s.border_color = Color(0.5, 0.3, 0.6, 0.7)
				hover_s.set_border_width_all(1)
				btn.add_theme_stylebox_override("hover", hover_s)
				btn.add_theme_stylebox_override("focus", hover_s)
				btn.add_theme_font_size_override("font_size", 12)
				btn.add_theme_color_override("font_color", Color(0.5, 0.4, 0.6))
				btn.add_theme_color_override("font_hover_color", Color(0.75, 0.55, 0.8))

				var mid = memory.id
				btn.pressed.connect(func():
					AudioManager.play_sfx("ui_select")
					action_container.visible = false
					_hide_burn_list()
					BattleManager.player_burn_residue(mid)
				)
				btn.focus_entered.connect(func(): AudioManager.play_sfx("ui_hover"))
				burn_list_container.add_child(btn)

	var cancel_btn = Button.new()
	cancel_btn.text = "[ Cancel ]"
	cancel_btn.add_theme_font_size_override("font_size", 12)
	cancel_btn.add_theme_color_override("font_color", Color(0.5, 0.45, 0.4))
	cancel_btn.pressed.connect(func():
		AudioManager.play_sfx("cancel")
		_hide_burn_list()
	)
	burn_list_container.add_child(cancel_btn)

	scroll.visible = true

func _hide_burn_list() -> void:
	var scroll = burn_list_container.get_meta("scroll_parent") as ScrollContainer
	if scroll:
		scroll.visible = false

## ===================== 아이템 목록 =====================

func _build_item_list(root: Control) -> void:
	var tray_backdrop := TextureRect.new()
	tray_backdrop.name = "BattleItemTrayBackdrop"
	tray_backdrop.anchor_left = 0.08
	tray_backdrop.anchor_right = 0.92
	tray_backdrop.anchor_top = 0.20
	tray_backdrop.anchor_bottom = 0.94
	tray_backdrop.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	tray_backdrop.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
	tray_backdrop.mouse_filter = Control.MOUSE_FILTER_IGNORE
	tray_backdrop.visible = false
	if ResourceLoader.exists(BATTLE_ITEM_TRAY_PATH):
		tray_backdrop.texture = load(BATTLE_ITEM_TRAY_PATH)
	root.add_child(tray_backdrop)

	var tray_title := Label.new()
	tray_title.name = "BattleItemTrayTitle"
	tray_title.anchor_left = 0.35
	tray_title.anchor_right = 0.65
	tray_title.anchor_top = 0.23
	tray_title.anchor_bottom = 0.29
	tray_title.text = _bl("FIELD SUPPLIES", "필드 보급품")
	tray_title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	tray_title.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	tray_title.add_theme_font_size_override("font_size", 15)
	tray_title.add_theme_color_override("font_color", Color(0.92, 0.76, 0.45))
	tray_title.visible = false
	root.add_child(tray_title)

	var scroll = ScrollContainer.new()
	scroll.anchor_left = 0.23
	scroll.anchor_right = 0.77
	scroll.anchor_top = 0.33
	scroll.anchor_bottom = 0.77
	scroll.visible = false

	var panel = PanelContainer.new()
	panel.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	panel.size_flags_vertical = Control.SIZE_EXPAND_FILL
	var style = StyleBoxFlat.new()
	style.bg_color = Color(0.018, 0.022, 0.028, 0.76)
	style.border_color = Color(0.56, 0.43, 0.24, 0.54)
	style.set_border_width_all(1)
	style.set_corner_radius_all(4)
	style.set_content_margin_all(12)
	panel.add_theme_stylebox_override("panel", style)
	scroll.add_child(panel)

	item_list_container = VBoxContainer.new()
	item_list_container.add_theme_constant_override("separation", 6)
	panel.add_child(item_list_container)

	root.add_child(scroll)
	item_list_container.set_meta("scroll_parent", scroll)
	item_list_container.set_meta("tray_backdrop", tray_backdrop)
	item_list_container.set_meta("tray_title", tray_title)

func _toggle_item_list() -> void:
	var scroll = item_list_container.get_meta("scroll_parent") as ScrollContainer
	if scroll.visible:
		_hide_item_list()
		return

	for child in item_list_container.get_children():
		child.queue_free()
	_battle_quick_item_buttons.clear()

	var quick_header := Label.new()
	quick_header.text = _bl("QUICK KIT  /  KEYS 1-3", "퀵 키트  /  단축키 1-3")
	quick_header.add_theme_font_size_override("font_size", 13)
	quick_header.add_theme_color_override("font_color", UITheme.TEXT_ACCENT)
	item_list_container.add_child(quick_header)
	var quick_row := HBoxContainer.new()
	quick_row.name = "BattleQuickItemRow"
	quick_row.add_theme_constant_override("separation", 6)
	item_list_container.add_child(quick_row)
	var quick_slots := GameManager.get_item_quick_slots()
	for slot_index in range(GameManager.ITEM_QUICK_SLOT_COUNT):
		var quick_button := Button.new()
		quick_button.name = "BattleQuickItem_%d" % (slot_index + 1)
		quick_button.custom_minimum_size = Vector2(0, 50)
		quick_button.size_flags_horizontal = Control.SIZE_EXPAND_FILL
		quick_button.add_theme_font_size_override("font_size", 12)
		if slot_index < quick_slots.size():
			var quick_id := quick_slots[slot_index]
			var quick_def: Dictionary = GameManager.ITEMS.get(quick_id, {})
			var quick_count := GameManager.get_item_count(quick_id)
			quick_button.text = "%d · %s\nx%d" % [slot_index + 1, String(quick_def.get("name", quick_id)), quick_count]
			quick_button.icon = GameManager.get_item_icon(quick_id)
			quick_button.expand_icon = true
			quick_button.disabled = quick_count <= 0
			quick_button.tooltip_text = String(quick_def.get("desc", ""))
			quick_button.set_meta("item_id", quick_id)
			quick_button.pressed.connect(_use_battle_item.bind(quick_id))
		else:
			quick_button.text = "%d · EMPTY" % (slot_index + 1)
			quick_button.disabled = true
			quick_button.set_meta("item_id", "")
		quick_button.focus_entered.connect(func(): AudioManager.play_sfx("ui_hover"))
		quick_row.add_child(quick_button)
		_battle_quick_item_buttons.append(quick_button)

	var supplies_header := Label.new()
	supplies_header.text = _bl("ALL CARRIED SUPPLIES", "전체 소지품")
	supplies_header.add_theme_font_size_override("font_size", 12)
	supplies_header.add_theme_color_override("font_color", UITheme.TEXT_DIM)
	item_list_container.add_child(supplies_header)

	var items = GameManager.player_data.items
	if items.is_empty():
		var empty_label = Label.new()
		empty_label.text = _bl("No items.", "아이템이 없다.")
		empty_label.add_theme_font_size_override("font_size", 13)
		empty_label.add_theme_color_override("font_color", UITheme.TEXT_DIM)
		item_list_container.add_child(empty_label)
	else:
		var ordered_items: Array[String] = []
		for item_id_value in items.keys():
			if GameManager.ITEMS.has(String(item_id_value)) and int(items[item_id_value]) > 0:
				ordered_items.append(String(item_id_value))
		ordered_items.sort_custom(func(item_a: String, item_b: String) -> bool:
			var slot_a := quick_slots.find(item_a)
			var slot_b := quick_slots.find(item_b)
			if slot_a >= 0 or slot_b >= 0:
				if slot_a < 0:
					return false
				if slot_b < 0:
					return true
				return slot_a < slot_b
			return String(GameManager.ITEMS[item_a].get("name", item_a)).naturalnocasecmp_to(String(GameManager.ITEMS[item_b].get("name", item_b))) < 0
		)
		for item_id in ordered_items:
			var count = items[item_id]
			var item_def = GameManager.ITEMS.get(item_id)
			if item_def == null:
				continue
			var btn = Button.new()
			btn.alignment = HORIZONTAL_ALIGNMENT_LEFT
			var item_type := String(item_def.get("type", ""))
			var item_color := _get_item_type_color(item_type)
			btn.text = "[%s] %s x%d\n%s" % [_get_item_type_label(item_type), item_def["name"], count, item_def["desc"]]
			btn.custom_minimum_size = Vector2(0, 58)
			var item_icon := GameManager.get_item_icon(item_id)
			if item_icon:
				btn.icon = item_icon
				btn.expand_icon = true
			btn.tooltip_text = item_def["desc"]

			var s = StyleBoxFlat.new()
			s.bg_color = Color(0.035 + item_color.r * 0.10, 0.035 + item_color.g * 0.10, 0.045 + item_color.b * 0.10, 0.90)
			s.set_content_margin_all(8)
			s.set_corner_radius_all(3)
			s.border_color = Color(item_color.r, item_color.g, item_color.b, 0.28)
			s.set_border_width_all(1)
			btn.add_theme_stylebox_override("normal", s)
			var hover_s = s.duplicate()
			hover_s.bg_color = Color(0.08 + item_color.r * 0.16, 0.08 + item_color.g * 0.16, 0.09 + item_color.b * 0.16, 0.98)
			hover_s.border_color = Color(item_color.r, item_color.g, item_color.b, 0.88)
			hover_s.set_border_width_all(1)
			btn.add_theme_stylebox_override("hover", hover_s)
			btn.add_theme_stylebox_override("focus", hover_s)
			btn.add_theme_font_size_override("font_size", 14)
			btn.add_theme_color_override("font_color", UITheme.TEXT_PRIMARY)
			btn.add_theme_color_override("font_hover_color", Color(item_color.r, item_color.g, item_color.b, 1.0))

			btn.pressed.connect(_use_battle_item.bind(item_id))
			btn.focus_entered.connect(func(): AudioManager.play_sfx("ui_hover"))
			item_list_container.add_child(btn)

	var cancel_btn = Button.new()
	cancel_btn.text = "[ Cancel ]"
	cancel_btn.add_theme_font_size_override("font_size", 14)
	cancel_btn.add_theme_color_override("font_color", UITheme.TEXT_DIM)
	cancel_btn.pressed.connect(func():
		AudioManager.play_sfx("cancel")
		_hide_item_list()
	)
	item_list_container.add_child(cancel_btn)

	scroll.visible = true
	if not _battle_quick_item_buttons.is_empty() and not _battle_quick_item_buttons[0].disabled:
		_battle_quick_item_buttons[0].grab_focus()
	var tray_backdrop := item_list_container.get_meta("tray_backdrop") as TextureRect
	if tray_backdrop:
		tray_backdrop.visible = true
	var tray_title := item_list_container.get_meta("tray_title") as Label
	if tray_title:
		tray_title.visible = true

func _use_battle_item(item_id: String) -> void:
	if item_id == "" or GameManager.get_item_count(item_id) <= 0:
		AudioManager.play_sfx("cancel")
		return
	AudioManager.play_sfx("ui_select")
	action_container.visible = false
	_hide_item_list()
	BattleManager.player_use_item(item_id)

func _get_item_type_color(item_type: String) -> Color:
	match item_type:
		"heal": return Color(0.34, 0.88, 0.60)
		"cure": return Color(0.34, 0.78, 0.94)
		"burn": return Color(1.0, 0.48, 0.20)
		"flee": return Color(0.72, 0.72, 0.78)
		"witness": return Color(0.70, 0.50, 1.0)
		"guard": return Color(0.96, 0.76, 0.34)
		"scan": return Color(0.38, 0.82, 0.94)
	return Color(0.72, 0.70, 0.56)

func _get_item_type_label(item_type: String) -> String:
	match item_type:
		"heal": return "RECOVER"
		"cure": return "PURGE"
		"burn": return "IGNITE"
		"flee": return "ESCAPE"
		"witness": return "WITNESS"
		"guard": return "ANCHOR"
		"scan": return "SCAN"
	return "TOOL"

func _hide_item_list() -> void:
	var scroll = item_list_container.get_meta("scroll_parent") as ScrollContainer
	if scroll:
		scroll.visible = false
	var tray_backdrop := item_list_container.get_meta("tray_backdrop") as TextureRect
	if tray_backdrop:
		tray_backdrop.visible = false
	var tray_title := item_list_container.get_meta("tray_title") as Label
	if tray_title:
		tray_title.visible = false

## ===================== 시각 피드백 =====================

## 데미지 숫자 표시 (떠오르며 사라짐, 크기 스케일링)
func _show_damage_number(target: String, amount: int, skill_name: String = "") -> void:
	var label = Label.new()
	label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER

	# 회복인 경우 (음수 amount = 힐)
	var is_heal = amount < 0
	if is_heal:
		label.text = "+%d" % abs(amount)
		_play_heal_vfx()  # S42: 힐 파티클
	else:
		label.text = str(amount)

	# 데미지 크기에 따른 폰트 스케일
	var font_size = 22
	if abs(amount) >= 100:
		font_size = 30
	elif abs(amount) >= 50:
		font_size = 26
	label.add_theme_font_size_override("font_size", font_size)

	# S40: 스킬/상황별 색상 분류
	var dmg_color: Color
	if is_heal:
		dmg_color = Color(0.3, 1.0, 0.4)  # 회복 = 초록
	elif target == "Arrel":
		dmg_color = Color(1.0, 0.3, 0.25)  # 플레이어 피격 = 빨강
	else:
		# 스킬별 색상
		var sn = skill_name.to_lower()
		if sn.find("burn") >= 0 or sn.find("flame") >= 0 or sn.find("ember") >= 0 or sn.find("fire") >= 0 or sn.find("scorch") >= 0:
			dmg_color = Color(1.0, 0.5, 0.15)  # 화염 = 주황
		elif sn.find("void") >= 0 or sn.find("cascade") >= 0 or sn.find("residue") >= 0:
			dmg_color = Color(0.7, 0.3, 1.0)  # 보이드 = 보라
		elif sn.find("drain") >= 0:
			dmg_color = Color(0.5, 0.9, 0.5)  # 드레인 = 연초록
		elif sn.find("poison") >= 0:
			dmg_color = Color(0.4, 0.85, 0.3)  # 독 = 독녹색
		elif sn.find("combo") >= 0:
			dmg_color = Color(1.0, 0.85, 0.2)  # 콤보 = 금색
		else:
			dmg_color = Color(1.0, 0.9, 0.4)  # 기본 = 연노랑
	label.add_theme_color_override("font_color", dmg_color)

	# S44: 사이드뷰 위치 기반 데미지 숫자
	if target == "Arrel":
		label.position = Vector2(180 + randf_range(-20, 20), 280 + randf_range(-10, 10))
	else:
		label.position = Vector2(900 + randf_range(-30, 30), 260 + randf_range(-10, 10))

	# 드롭 섀도우 효과
	var shadow = Label.new()
	shadow.text = str(amount)
	shadow.add_theme_font_size_override("font_size", font_size)
	shadow.add_theme_color_override("font_color", Color(0, 0, 0, 0.6))
	shadow.position = Vector2(2, 2)
	label.add_child(shadow)

	canvas_root.add_child(label)

	# 떠오르며 사라지는 애니메이션 + 스케일 펀치
	label.scale = Vector2(1.3, 1.3)
	var t = create_tween().set_parallel(true)
	t.tween_property(label, "position:y", label.position.y - 50, 0.9).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_CUBIC)
	t.tween_property(label, "scale", Vector2(1.0, 1.0), 0.2).set_ease(Tween.EASE_OUT)
	t.tween_property(label, "modulate:a", 0.0, 0.5).set_delay(0.4)
	t.chain().tween_callback(label.queue_free)

## 히트 플래시 (적 피격 = 흰색, 플레이어 피격 = 빨간색)
func _hit_flash(target: String) -> void:
	if target == "Arrel":
		hit_flash_rect.color = Color(1, 0.15, 0.1, 0.3)
	else:
		hit_flash_rect.color = Color(1, 1, 1, 0.25)

	var t = create_tween()
	t.tween_property(hit_flash_rect, "color:a", 0.0, 0.2)

	# S44: 스프라이트 깜빡임 + 피격 밀림
	if target != "Arrel" and enemy_sprite:
		var flash_t = create_tween()
		flash_t.tween_property(enemy_sprite, "modulate", Color(3, 3, 3, 1), 0.05)
		flash_t.tween_property(enemy_sprite, "modulate", _battler_base_modulate(enemy_sprite), 0.15)
		# S57: Attack knockback, enemy slides back 15px then returns (0.15s)
		if enemy_sprite_container:
			var push_t = create_tween()
			push_t.tween_property(enemy_sprite_container, "position:x", _enemy_base_pos.x + 15, 0.06).set_ease(Tween.EASE_OUT)
			push_t.tween_property(enemy_sprite_container, "position:x", _enemy_base_pos.x - 3, 0.06).set_ease(Tween.EASE_OUT)
			push_t.tween_property(enemy_sprite_container, "position:x", _enemy_base_pos.x, 0.03).set_ease(Tween.EASE_IN_OUT)
	elif target == "Arrel" and player_sprite:
		# S57: Enhanced player hurt, red flash 0.3s + bounce
		var flash_t = create_tween()
		flash_t.tween_property(player_sprite, "modulate", Color(2.5, 0.4, 0.3, 1), 0.05)
		flash_t.tween_property(player_sprite, "modulate", Color(1.5, 0.6, 0.5, 1), 0.1)
		flash_t.tween_property(player_sprite, "modulate", _battler_base_modulate(player_sprite), 0.15)
		# S57: 피격 밀림 (왼쪽으로 15px) + bounce back
		if player_sprite_container:
			var push_t = create_tween()
			push_t.tween_property(player_sprite_container, "position:x", _player_base_pos.x - 15, 0.06).set_ease(Tween.EASE_OUT)
			push_t.tween_property(player_sprite_container, "position:x", _player_base_pos.x + 4, 0.1).set_ease(Tween.EASE_OUT)
			push_t.tween_property(player_sprite_container, "position:x", _player_base_pos.x, 0.1).set_ease(Tween.EASE_IN_OUT)
			# S57: Slight Y-axis bounce (hurt jolt)
			var bounce_t = create_tween()
			bounce_t.tween_property(player_sprite_container, "position:y", _player_base_pos.y - 6, 0.06).set_ease(Tween.EASE_OUT)
			bounce_t.tween_property(player_sprite_container, "position:y", _player_base_pos.y, 0.15).set_ease(Tween.EASE_IN_OUT).set_trans(Tween.TRANS_BOUNCE)

## 스크린 셰이크 (S42 강화: 더 많은 프레임 + 회전 흔들림)
func _screen_shake(intensity: float = 1.0) -> void:
	# S53: 접근성, 화면 흔들림 비활성화 옵션
	if OptionsMenu.is_clean_gameplay_visuals() or not OptionsMenu.settings.get("screen_shake", true):
		return
	var original_pos = canvas_root.position
	var t = create_tween()
	var frames = int(6 + intensity * 2)
	for i in range(frames):
		var decay = 1.0 - float(i) / frames
		var offset = Vector2(
			randf_range(-7, 7) * intensity * decay,
			randf_range(-5, 5) * intensity * decay
		)
		t.tween_property(canvas_root, "position", original_pos + offset, 0.03)
	t.tween_property(canvas_root, "position", original_pos, 0.04)

## ===================== S44: 전투 애니메이션 =====================

## 플레이어 공격 돌진 (적 방향으로 빠르게 이동 후 복귀)
func _player_attack_rush() -> void:
	if not player_sprite_container:
		return
	var rush_target = Vector2(_enemy_base_pos.x - 180, _player_base_pos.y - 10)
	var t = create_tween()
	t.tween_property(player_sprite_container, "position", rush_target, 0.12).set_ease(Tween.EASE_IN).set_trans(Tween.TRANS_BACK)
	t.tween_interval(0.1)  # 잠깐 멈춤 (임팩트)
	t.tween_property(player_sprite_container, "position", _player_base_pos, 0.25).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_CUBIC)

## 속도선 (공격 시 화면에 빗금)
func _play_speed_lines() -> void:
	if OptionsMenu.is_clean_gameplay_visuals():
		return
	for i in range(6):
		var line = ColorRect.new()
		line.size = Vector2(randf_range(200, 500), 2)
		line.position = Vector2(-100, randf_range(50, 500))
		line.rotation = -0.15
		line.color = Color(1, 1, 1, randf_range(0.08, 0.2))
		line.z_index = 40
		line.mouse_filter = Control.MOUSE_FILTER_IGNORE
		canvas_root.add_child(line)
		var t = create_tween().set_parallel(true)
		t.tween_property(line, "position:x", 1400.0, randf_range(0.15, 0.3)).set_delay(randf_range(0, 0.05))
		t.tween_property(line, "modulate:a", 0.0, 0.2).set_delay(0.1)
		t.chain().tween_callback(line.queue_free)

## 임팩트 버스트 (타격 순간 방사형 빛)
func _play_impact_burst(pos: Vector2) -> void:
	# 중앙 원형 플래시
	var burst = ColorRect.new()
	burst.size = Vector2(8, 8)
	burst.position = pos - Vector2(4, 4)
	burst.color = Color(1, 0.95, 0.8, 0.9)
	burst.z_index = 62
	canvas_root.add_child(burst)
	var bt = create_tween().set_parallel(true)
	bt.tween_property(burst, "size", Vector2(80, 80), 0.15).set_ease(Tween.EASE_OUT)
	bt.tween_property(burst, "position", pos - Vector2(40, 40), 0.15).set_ease(Tween.EASE_OUT)
	bt.tween_property(burst, "color:a", 0.0, 0.25)
	bt.chain().tween_callback(burst.queue_free)
	# 방사 선 4개
	for j in range(4):
		var ray = ColorRect.new()
		ray.size = Vector2(3, 0)
		ray.position = pos
		ray.rotation = j * PI / 4.0 + randf_range(-0.2, 0.2)
		ray.pivot_offset = Vector2(1.5, 0)
		ray.color = Color(1, 0.9, 0.7, 0.7)
		ray.z_index = 61
		canvas_root.add_child(ray)
		var rt = create_tween().set_parallel(true)
		rt.tween_property(ray, "size:y", randf_range(60, 120), 0.12).set_ease(Tween.EASE_OUT)
		rt.tween_property(ray, "modulate:a", 0.0, 0.2).set_delay(0.08)
		rt.chain().tween_callback(ray.queue_free)

## ===================== 공격 VFX =====================

## 물리 공격 슬래시 이펙트 (S42 개선: GPU 파티클 추가)
func _play_slash_vfx() -> void:
	_play_gpu_slash_particles()  # S42: GPU 파티클
	# S44: 임팩트 버스트
	_play_impact_burst(Vector2(900, 320))
	var center = Vector2(900, 320)  # S44: 사이드뷰 적 위치

	# 메인 슬래시, 길이 확장 애니메이션
	var slash = ColorRect.new()
	slash.size = Vector2(0, 4)
	slash.position = center + Vector2(-60, -30)
	slash.rotation = -0.55
	slash.pivot_offset = Vector2(0, 2)
	slash.color = Color(1, 1, 1, 0.9)
	slash.z_index = 60
	canvas_root.add_child(slash)

	var t = create_tween()
	t.tween_property(slash, "size:x", 220.0, 0.1).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_CUBIC)
	t.tween_property(slash, "modulate:a", 0.0, 0.2).set_ease(Tween.EASE_IN)
	t.tween_callback(slash.queue_free)

	# 크로스 슬래시 (지연)
	var slash2 = ColorRect.new()
	slash2.size = Vector2(0, 3)
	slash2.position = center + Vector2(-40, 10)
	slash2.rotation = 0.45
	slash2.pivot_offset = Vector2(0, 1.5)
	slash2.color = Color(1, 0.9, 0.7, 0.7)
	slash2.z_index = 60
	canvas_root.add_child(slash2)

	var t2 = create_tween()
	t2.tween_interval(0.06)
	t2.tween_property(slash2, "size:x", 190.0, 0.1).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_CUBIC)
	t2.tween_property(slash2, "modulate:a", 0.0, 0.18).set_ease(Tween.EASE_IN)
	t2.tween_callback(slash2.queue_free)

	# 충격 파편 (흰색 입자 4~6개)
	for i in range(randi_range(4, 6)):
		var spark = ColorRect.new()
		spark.size = Vector2(3, 3)
		spark.position = center + Vector2(randf_range(-20, 20), randf_range(-15, 15))
		spark.color = Color(1, 1, 1, 0.8)
		spark.z_index = 58
		canvas_root.add_child(spark)
		var angle = randf() * TAU
		var dist = randf_range(25, 60)
		var target_pos = spark.position + Vector2(cos(angle), sin(angle)) * dist
		var st = create_tween().set_parallel(true)
		st.tween_property(spark, "position", target_pos, 0.3).set_ease(Tween.EASE_OUT)
		st.tween_property(spark, "modulate:a", 0.0, 0.25).set_delay(0.08)
		st.chain().tween_callback(spark.queue_free)

## 기억 연소 VFX, 스킬별 분류 (S40)
func _play_attack_vfx(skill_name: String) -> void:
	var sn = skill_name.to_lower()
	# 보이드 스킬 → 보라 파티클
	if sn.find("void") >= 0 or sn.find("cascade") >= 0 or sn.find("residue") >= 0:
		_play_void_vfx()
	else:
		# 연소/기본 → 불꽃 VFX
		_play_burn_vfx()

## 불꽃 VFX (S42: GPU 파티클 추가, S52: 화면 가장자리 불꽃)
func _play_burn_vfx() -> void:
	if OptionsMenu.is_clean_gameplay_visuals():
		return
	_play_gpu_burn_particles()  # S42: GPU 파티클
	_burn_edge_flare()  # S52: 화면 가장자리 화염
	var center = Vector2(920, 310)

	# 여러 불꽃 입자 생성
	for i in range(12):
		var particle = ColorRect.new()
		particle.size = Vector2(randf_range(4, 10), randf_range(4, 10))
		particle.position = center + Vector2(randf_range(-40, 40), randf_range(-30, 30))
		particle.z_index = 55

		# 불 색상 (주황~빨강~노랑)
		var fire_colors = [
			Color(1.0, 0.6, 0.1, 0.9),
			Color(1.0, 0.35, 0.1, 0.85),
			Color(1.0, 0.85, 0.3, 0.8),
			Color(0.9, 0.2, 0.05, 0.7),
		]
		particle.color = fire_colors[randi_range(0, fire_colors.size() - 1)]

		canvas_root.add_child(particle)

		# 위로 떠오르며 사라짐
		var delay = randf_range(0, 0.15)
		var t = create_tween().set_parallel(true)
		t.tween_property(particle, "position:y", particle.position.y - randf_range(30, 80), randf_range(0.4, 0.8)).set_delay(delay).set_ease(Tween.EASE_OUT)
		t.tween_property(particle, "position:x", particle.position.x + randf_range(-20, 20), randf_range(0.4, 0.8)).set_delay(delay)
		t.tween_property(particle, "modulate:a", 0.0, randf_range(0.3, 0.6)).set_delay(delay + 0.2)
		t.tween_property(particle, "size", Vector2(1, 1), 0.6).set_delay(delay)
		t.chain().tween_callback(particle.queue_free)

	# 중앙 플래시
	var flash = ColorRect.new()
	flash.size = Vector2(100, 100)
	flash.position = center - Vector2(50, 50)
	flash.color = Color(1, 0.7, 0.2, 0.4)
	flash.z_index = 54
	canvas_root.add_child(flash)

	var ft = create_tween()
	ft.tween_property(flash, "modulate:a", 0.0, 0.35)
	ft.tween_callback(flash.queue_free)

## ===================== S40: 적 디졸브 사망 이펙트 =====================

func _play_enemy_dissolve() -> void:
	var shader_path = "res://assets/shaders/dissolve.gdshader"
	if not ResourceLoader.exists(shader_path) or not enemy_sprite:
		return
	var mat = ShaderMaterial.new()
	mat.shader = load(shader_path)
	mat.set_shader_parameter("progress", 0.0)
	mat.set_shader_parameter("edge_color", Color(0.6, 0.2, 0.8, 1.0))
	mat.set_shader_parameter("edge_width", 0.08)
	enemy_sprite.material = mat
	var t = create_tween()
	t.tween_method(func(val): mat.set_shader_parameter("progress", val), 0.0, 1.0, 1.2).set_ease(Tween.EASE_IN).set_trans(Tween.TRANS_QUAD)

## ===================== S40: 색수차 이펙트 (Limit Break) =====================

var _chromatic_overlay: ColorRect

func _play_chromatic_aberration(duration: float = 1.5) -> void:
	if OptionsMenu.is_clean_gameplay_visuals():
		return
	var shader_path = "res://assets/shaders/chromatic_aberration.gdshader"
	if not ResourceLoader.exists(shader_path):
		return
	if _chromatic_overlay and is_instance_valid(_chromatic_overlay):
		_chromatic_overlay.queue_free()
	_chromatic_overlay = ColorRect.new()
	_chromatic_overlay.set_anchors_preset(Control.PRESET_FULL_RECT)
	_chromatic_overlay.color = Color(0, 0, 0, 0)  # 셰이더가 screen_texture에서 직접 샘플링
	_chromatic_overlay.mouse_filter = Control.MOUSE_FILTER_IGNORE
	_chromatic_overlay.z_index = 80
	var mat = ShaderMaterial.new()
	mat.shader = load(shader_path)
	mat.set_shader_parameter("strength", 0.008)
	mat.set_shader_parameter("pulse_speed", 5.0)
	mat.set_shader_parameter("use_pulse", true)
	_chromatic_overlay.material = mat
	canvas_root.add_child(_chromatic_overlay)
	# 강도 페이드: 강하게 시작 → 점점 감소
	var t = create_tween()
	t.tween_method(func(val): mat.set_shader_parameter("strength", val), 0.012, 0.0, duration).set_ease(Tween.EASE_OUT)
	t.tween_callback(_chromatic_overlay.queue_free)

## ===================== S40: 보이드 스킬 VFX (보라색 파티클 폭발) =====================

func _play_void_vfx() -> void:
	if OptionsMenu.is_clean_gameplay_visuals():
		return
	_play_gpu_void_particles()  # S42: GPU 파티클
	var center = Vector2(920, 310)
	for i in range(16):
		var particle = ColorRect.new()
		var s = randf_range(3, 8)
		particle.size = Vector2(s, s)
		particle.position = center + Vector2(randf_range(-50, 50), randf_range(-40, 40))
		particle.z_index = 55
		var void_colors = [
			Color(0.5, 0.15, 0.8, 0.9),
			Color(0.3, 0.1, 0.6, 0.85),
			Color(0.7, 0.3, 1.0, 0.8),
			Color(0.2, 0.05, 0.4, 0.7),
		]
		particle.color = void_colors[randi_range(0, void_colors.size() - 1)]
		canvas_root.add_child(particle)
		# 방사형으로 퍼지며 사라짐
		var angle = randf() * TAU
		var dist = randf_range(40, 100)
		var target_pos = particle.position + Vector2(cos(angle), sin(angle)) * dist
		var delay = randf_range(0, 0.1)
		var t = create_tween().set_parallel(true)
		t.tween_property(particle, "position", target_pos, randf_range(0.5, 0.9)).set_delay(delay).set_ease(Tween.EASE_OUT)
		t.tween_property(particle, "modulate:a", 0.0, randf_range(0.4, 0.7)).set_delay(delay + 0.15)
		t.tween_property(particle, "size", Vector2(1, 1), 0.7).set_delay(delay)
		t.chain().tween_callback(particle.queue_free)
	# 보라색 플래시
	var flash = ColorRect.new()
	flash.size = Vector2(120, 120)
	flash.position = center - Vector2(60, 60)
	flash.color = Color(0.4, 0.1, 0.6, 0.35)
	flash.z_index = 54
	canvas_root.add_child(flash)
	var ft = create_tween()
	ft.tween_property(flash, "modulate:a", 0.0, 0.4)
	ft.tween_callback(flash.queue_free)

## ===================== Limit Break UI =====================

## S230: 리밋 게이지는 아렐 상태 카드의 한 줄이 된다.
## 예전에는 별도 패널이 HP 패널과 어긋난 왼쪽 끝(76.8 vs 25.6)에서 시작했고,
## 수치가 없어 "언제 차는지"를 게이지 길이로만 짐작해야 했다.
func _build_limit_gauge(_root: Control) -> void:
	if _player_readout_column == null:
		return
	var hbox = HBoxContainer.new()
	hbox.add_theme_constant_override("separation", 6)
	_player_readout_column.add_child(hbox)

	limit_label = Label.new()
	limit_label.text = _bl("LIMIT", "리밋")
	limit_label.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	# S209: 가변 폰트 메트릭 때문에 마지막 글자("밋")가 잘려 "리미"로 보이던 문제 수정.
	limit_label.custom_minimum_size = Vector2(46, 0)
	limit_label.clip_text = false
	UITheme.style_meta_label(limit_label, Color(0.86, 0.72, 0.98))
	hbox.add_child(limit_label)

	limit_bar = ProgressBar.new()
	limit_bar.custom_minimum_size = Vector2(80, 12)
	limit_bar.max_value = 100.0
	limit_bar.value = 0.0
	limit_bar.show_percentage = false
	limit_bar.mouse_filter = Control.MOUSE_FILTER_IGNORE
	limit_bar.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	limit_bar.size_flags_vertical = Control.SIZE_SHRINK_CENTER

	var fill = StyleBoxFlat.new()
	fill.bg_color = Color(0.7, 0.3, 0.8)
	fill.set_corner_radius_all(2)
	limit_bar.add_theme_stylebox_override("fill", fill)
	var bg_s = StyleBoxFlat.new()
	bg_s.bg_color = Color(0.06, 0.045, 0.085, 0.94)
	bg_s.border_color = Color(0.0, 0.0, 0.0, 0.5)
	bg_s.set_border_width_all(1)
	bg_s.set_corner_radius_all(2)
	limit_bar.add_theme_stylebox_override("background", bg_s)
	hbox.add_child(limit_bar)

	limit_value_label = Label.new()
	limit_value_label.text = "0%"
	limit_value_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	limit_value_label.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	limit_value_label.custom_minimum_size = Vector2(42, 0)
	UITheme.style_meta_label(limit_value_label, Color(0.80, 0.70, 0.92))
	hbox.add_child(limit_value_label)

func _on_limit_changed(value: float) -> void:
	var ready_now := value >= BattleManager.LIMIT_MAX
	if limit_bar:
		limit_bar.value = value
		# 게이지 꽉 차면 색상 변경
		var fill = limit_bar.get_theme_stylebox("fill") as StyleBoxFlat
		if fill:
			fill.bg_color = Color(1.0, 0.6, 0.9) if ready_now else Color(0.7, 0.3, 0.8)
	if limit_label:
		limit_label.add_theme_color_override("font_color", Color(1.0, 0.72, 0.94) if ready_now else Color(0.72, 0.60, 0.84))
	# S230: 게이지 길이만으로 짐작하던 진행도를 숫자로도 읽게 한다.
	if limit_value_label:
		limit_value_label.text = _bl("READY", "준비") if ready_now else "%d%%" % int(value)
		limit_value_label.add_theme_color_override("font_color", Color(1.0, 0.78, 0.96) if ready_now else Color(0.76, 0.68, 0.88))
	# LIMIT 버튼 활성화/비활성화
	_update_limit_button()

func _update_limit_button() -> void:
	# S175: 버그 수정, 예전엔 버튼 텍스트 == "LIMIT"로 비교해 한국어("리밋")에서
	# 갱신이 전혀 안 됐다. 이제 저장된 참조를 사용.
	if limit_btn == null or not is_instance_valid(limit_btn):
		return
	limit_btn.disabled = BattleManager.limit_gauge < BattleManager.LIMIT_MAX
	# S209: 예전에는 버튼 전체를 modulate로 반투명하게 만들어, 아래 커맨드 덱 장식이
	# 글자 사이로 비쳐 "6 · 리밋"이 읽히지 않았다. 이제 배경은 불투명하게 유지하고
	# 글자색만 낮춘다.
	limit_btn.modulate = Color(1, 1, 1, 1) if not limit_btn.disabled else Color(1, 1, 1, 1)
	limit_btn.self_modulate = Color(1.0, 0.86, 1.0, 1.0) if not limit_btn.disabled else Color(1, 1, 1, 1)
	limit_btn.add_theme_color_override("font_disabled_color", Color(0.62, 0.56, 0.66, 0.95))
	limit_btn.text = "6 · %s  %d%%\n%s" % [GameManager.loc("limit"), int(BattleManager.limit_gauge), _bl("Memory Cascade", "메모리 캐스케이드")]

func _on_limit_break() -> void:
	if BattleManager.limit_gauge < BattleManager.LIMIT_MAX:
		AudioManager.play_sfx("cancel")
		return
	AudioManager.play_sfx("ui_select")
	action_container.visible = false
	_hide_burn_list()
	_hide_item_list()
	_play_action_cutin(MEMORY_CASCADE_CUTIN_PATH, true, 0.94, 0.48)
	_show_turn_indicator(_bl("ARREL / MEMORY CASCADE", "아렐 / 메모리 캐스케이드"), Color(0.72, 0.88, 1.0))
	await BattleManager.pace_timer(0.42).timeout
	# S40: Limit Break 색수차 연출
	_play_chromatic_aberration(2.0)
	_screen_shake(2.5)
	# S42: 리밋 브레이크 폭발 파티클
	_play_limit_burst_vfx()
	BattleManager.player_limit_break()

## ===================== S42: 전투 분위기 + 강화된 VFX =====================

## 배경 분위기 파티클 (떠다니는 먼지/잿가루)
func _add_battle_atmosphere() -> void:
	if OptionsMenu.is_clean_gameplay_visuals():
		return
	# 떠다니는 먼지 파티클
	_battle_particles = GPUParticles2D.new()
	var mat = ParticleProcessMaterial.new()
	mat.direction = Vector3(0.5, -0.3, 0)
	mat.spread = 60.0
	mat.initial_velocity_min = 5.0
	mat.initial_velocity_max = 15.0
	mat.gravity = Vector3(2, -3, 0)
	mat.scale_min = 0.5
	mat.scale_max = 2.0

	# 적에 따라 파티클 색상 결정
	var enemy_name = BattleManager.current_enemy.name.to_lower() if BattleManager.current_enemy else ""
	var p_color: Color
	if "void" in enemy_name or "shade" in enemy_name:
		p_color = Color(0.4, 0.15, 0.6, 0.25)
	elif "sentinel" in enemy_name:
		p_color = Color(0.3, 0.1, 0.5, 0.3)
	else:
		p_color = Color(0.5, 0.45, 0.4, 0.15)

	var gradient = GradientTexture1D.new()
	var g = Gradient.new()
	g.set_color(0, Color(p_color.r, p_color.g, p_color.b, 0.0))
	g.add_point(0.3, p_color)
	g.add_point(0.7, Color(p_color.r, p_color.g, p_color.b, p_color.a * 0.6))
	g.set_color(1, Color(p_color.r, p_color.g, p_color.b, 0.0))
	gradient.gradient = g
	mat.color_ramp = gradient

	mat.emission_shape = ParticleProcessMaterial.EMISSION_SHAPE_BOX
	mat.emission_box_extents = Vector3(640, 360, 0)

	_battle_particles.process_material = mat
	_battle_particles.amount = 20
	_battle_particles.lifetime = 6.0
	_battle_particles.position = Vector2(640, 360)
	_battle_particles.z_index = -1
	_battle_particles.visibility_rect = Rect2(-700, -400, 1400, 800)
	add_child(_battle_particles)

	# 컬러 그레이딩 오버레이 (전투 분위기)
	_color_grade_rect = ColorRect.new()
	_color_grade_rect.set_anchors_preset(Control.PRESET_FULL_RECT)
	_color_grade_rect.mouse_filter = Control.MOUSE_FILTER_IGNORE
	_color_grade_rect.z_index = -2
	if "void" in enemy_name or "shade" in enemy_name:
		_color_grade_rect.color = Color(0.1, 0.05, 0.15, 0.06)
	else:
		_color_grade_rect.color = Color(0.05, 0.03, 0.0, 0.04)
	add_child(_color_grade_rect)

## 물리 공격 GPUParticles2D 이펙트 (S42: 기존 ColorRect 대체)
func _play_gpu_slash_particles() -> void:
	if OptionsMenu.is_clean_gameplay_visuals():
		return
	var center = Vector2(900, 300)
	var particles = GPUParticles2D.new()
	var mat = ParticleProcessMaterial.new()
	mat.direction = Vector3(-1, 0, 0)
	mat.spread = 25.0
	mat.initial_velocity_min = 150.0
	mat.initial_velocity_max = 300.0
	mat.gravity = Vector3(0, 50, 0)
	mat.scale_min = 0.5
	mat.scale_max = 2.0
	mat.damping_min = 100.0
	mat.damping_max = 200.0

	var gradient = GradientTexture1D.new()
	var g = Gradient.new()
	g.set_color(0, Color(1, 1, 1, 1.0))
	g.add_point(0.3, Color(1, 0.9, 0.7, 0.8))
	g.set_color(1, Color(1, 0.7, 0.3, 0.0))
	gradient.gradient = g
	mat.color_ramp = gradient

	mat.emission_shape = ParticleProcessMaterial.EMISSION_SHAPE_SPHERE
	mat.emission_sphere_radius = 5.0
	particles.process_material = mat
	particles.amount = 24
	particles.lifetime = 0.4
	particles.one_shot = true
	particles.explosiveness = 0.9
	particles.position = center
	particles.z_index = 60
	particles.visibility_rect = Rect2(-200, -200, 400, 400)
	canvas_root.add_child(particles)
	particles.emitting = true

	# 자동 정리
	var timer = BattleManager.pace_timer(1.0)
	timer.timeout.connect(particles.queue_free)

## 불꽃 GPUParticles2D (S42)
func _play_gpu_burn_particles() -> void:
	if OptionsMenu.is_clean_gameplay_visuals():
		return
	var center = Vector2(920, 310)
	var particles = GPUParticles2D.new()
	var mat = ParticleProcessMaterial.new()
	mat.direction = Vector3(0, -1, 0)
	mat.spread = 40.0
	mat.initial_velocity_min = 40.0
	mat.initial_velocity_max = 120.0
	mat.gravity = Vector3(0, -80, 0)
	mat.scale_min = 1.0
	mat.scale_max = 4.0

	var gradient = GradientTexture1D.new()
	var g = Gradient.new()
	g.set_color(0, Color(1, 0.9, 0.3, 1.0))
	g.add_point(0.2, Color(1, 0.6, 0.1, 0.9))
	g.add_point(0.5, Color(0.9, 0.3, 0.05, 0.7))
	g.set_color(1, Color(0.3, 0.1, 0.05, 0.0))
	gradient.gradient = g
	mat.color_ramp = gradient

	mat.emission_shape = ParticleProcessMaterial.EMISSION_SHAPE_BOX
	mat.emission_box_extents = Vector3(40, 20, 0)
	particles.process_material = mat
	particles.amount = 40
	particles.lifetime = 0.7
	particles.one_shot = true
	particles.explosiveness = 0.7
	particles.position = center
	particles.z_index = 55
	particles.visibility_rect = Rect2(-200, -200, 400, 400)
	canvas_root.add_child(particles)
	particles.emitting = true

	# 열기 왜곡 오버레이
	var heat = ColorRect.new()
	heat.size = Vector2(120, 80)
	heat.position = center - Vector2(60, 50)
	heat.color = Color(1.0, 0.5, 0.1, 0.2)
	heat.z_index = 54
	canvas_root.add_child(heat)
	var ht = create_tween()
	ht.tween_property(heat, "color:a", 0.0, 0.5)
	ht.tween_callback(heat.queue_free)

	var timer = BattleManager.pace_timer(1.5)
	timer.timeout.connect(particles.queue_free)

## 보이드 GPUParticles2D (S42)
func _play_gpu_void_particles() -> void:
	if OptionsMenu.is_clean_gameplay_visuals():
		return
	var center = Vector2(920, 310)
	var particles = GPUParticles2D.new()
	var mat = ParticleProcessMaterial.new()
	mat.direction = Vector3(0, 0, 0)
	mat.spread = 180.0
	mat.initial_velocity_min = 60.0
	mat.initial_velocity_max = 150.0
	mat.gravity = Vector3(0, 0, 0)
	mat.scale_min = 1.0
	mat.scale_max = 3.5
	mat.damping_min = 50.0
	mat.damping_max = 100.0

	var gradient = GradientTexture1D.new()
	var g = Gradient.new()
	g.set_color(0, Color(0.7, 0.3, 1.0, 1.0))
	g.add_point(0.3, Color(0.5, 0.15, 0.8, 0.8))
	g.add_point(0.6, Color(0.3, 0.1, 0.6, 0.5))
	g.set_color(1, Color(0.15, 0.05, 0.3, 0.0))
	gradient.gradient = g
	mat.color_ramp = gradient

	mat.emission_shape = ParticleProcessMaterial.EMISSION_SHAPE_SPHERE
	mat.emission_sphere_radius = 10.0
	particles.process_material = mat
	particles.amount = 50
	particles.lifetime = 0.8
	particles.one_shot = true
	particles.explosiveness = 0.85
	particles.position = center
	particles.z_index = 55
	particles.visibility_rect = Rect2(-200, -200, 400, 400)
	canvas_root.add_child(particles)
	particles.emitting = true

	# 중앙 에너지 링
	var ring = ColorRect.new()
	ring.size = Vector2(8, 8)
	ring.position = center - Vector2(4, 4)
	ring.color = Color(0.6, 0.2, 1.0, 0.8)
	ring.z_index = 56
	canvas_root.add_child(ring)
	var rt = create_tween().set_parallel(true)
	rt.tween_property(ring, "size", Vector2(100, 100), 0.3).set_ease(Tween.EASE_OUT)
	rt.tween_property(ring, "position", center - Vector2(50, 50), 0.3).set_ease(Tween.EASE_OUT)
	rt.tween_property(ring, "color:a", 0.0, 0.4)
	rt.chain().tween_callback(ring.queue_free)

	var timer = BattleManager.pace_timer(1.5)
	timer.timeout.connect(particles.queue_free)

## 힐 GPUParticles2D (S42: 새로운 힐 이펙트)
func _play_heal_vfx() -> void:
	var center = Vector2(200, 360)  # S44: 플레이어 위치 근처
	var particles = GPUParticles2D.new()
	var mat = ParticleProcessMaterial.new()
	mat.direction = Vector3(0, -1, 0)
	mat.spread = 30.0
	mat.initial_velocity_min = 30.0
	mat.initial_velocity_max = 80.0
	mat.gravity = Vector3(0, -40, 0)
	mat.scale_min = 1.0
	mat.scale_max = 3.0

	var gradient = GradientTexture1D.new()
	var g = Gradient.new()
	g.set_color(0, Color(0.3, 1.0, 0.5, 0.9))
	g.add_point(0.4, Color(0.5, 1.0, 0.7, 0.7))
	g.set_color(1, Color(0.7, 1.0, 0.9, 0.0))
	gradient.gradient = g
	mat.color_ramp = gradient

	mat.emission_shape = ParticleProcessMaterial.EMISSION_SHAPE_BOX
	mat.emission_box_extents = Vector3(30, 5, 0)
	particles.process_material = mat
	particles.amount = 25
	particles.lifetime = 0.8
	particles.one_shot = true
	particles.explosiveness = 0.5
	particles.position = center
	particles.z_index = 55
	particles.visibility_rect = Rect2(-200, -200, 400, 400)
	canvas_root.add_child(particles)
	particles.emitting = true

	var timer = BattleManager.pace_timer(1.5)
	timer.timeout.connect(particles.queue_free)

## 리밋 브레이크 폭발 VFX (S42)
func _play_limit_burst_vfx() -> void:
	if OptionsMenu.is_clean_gameplay_visuals():
		return
	var center = Vector2(920, 310)
	# 큰 폭발 파티클
	var particles = GPUParticles2D.new()
	var mat = ParticleProcessMaterial.new()
	mat.direction = Vector3(0, 0, 0)
	mat.spread = 180.0
	mat.initial_velocity_min = 100.0
	mat.initial_velocity_max = 350.0
	mat.gravity = Vector3(0, 30, 0)
	mat.scale_min = 2.0
	mat.scale_max = 6.0
	mat.damping_min = 50.0
	mat.damping_max = 150.0

	var gradient = GradientTexture1D.new()
	var g = Gradient.new()
	g.set_color(0, Color(1.0, 0.9, 1.0, 1.0))
	g.add_point(0.2, Color(1.0, 0.6, 0.9, 0.9))
	g.add_point(0.5, Color(0.8, 0.3, 0.7, 0.6))
	g.set_color(1, Color(0.5, 0.1, 0.4, 0.0))
	gradient.gradient = g
	mat.color_ramp = gradient

	mat.emission_shape = ParticleProcessMaterial.EMISSION_SHAPE_SPHERE
	mat.emission_sphere_radius = 15.0
	particles.process_material = mat
	particles.amount = 80
	particles.lifetime = 1.0
	particles.one_shot = true
	particles.explosiveness = 0.95
	particles.position = center
	particles.z_index = 65
	particles.visibility_rect = Rect2(-400, -400, 800, 800)
	canvas_root.add_child(particles)
	particles.emitting = true

	# 화면 백색 플래시
	var flash = ColorRect.new()
	flash.set_anchors_preset(Control.PRESET_FULL_RECT)
	flash.color = Color(1, 0.9, 1, 0.6)
	flash.mouse_filter = Control.MOUSE_FILTER_IGNORE
	flash.z_index = 90
	canvas_root.add_child(flash)
	var ft = create_tween()
	ft.tween_property(flash, "color:a", 0.0, 0.8).set_ease(Tween.EASE_OUT)
	ft.tween_callback(flash.queue_free)

	var timer = BattleManager.pace_timer(2.0)
	timer.timeout.connect(particles.queue_free)

## ===================== S46: 타격감 강화 + VFX Library 셰이더 =====================

## VFX Library, flash_white 피격 셰이더 (적/플레이어 스프라이트에 흰색 플래시)
func _apply_hit_shader(target: String, amount: int) -> void:
	var shader_path = "res://addons/vfx_lib/shaders/flash_white.gdshader"
	if not ResourceLoader.exists(shader_path):
		return
	var sprite_node: Control = null
	if target != "Arrel" and enemy_sprite:
		sprite_node = enemy_sprite
	elif target == "Arrel" and player_sprite:
		sprite_node = player_sprite
	if not sprite_node:
		return
	var mat = ShaderMaterial.new()
	mat.shader = load(shader_path)
	var flash_strength = clampf(float(amount) / 100.0, 0.4, 1.0)
	mat.set_shader_parameter("flash_amount", flash_strength)
	mat.set_shader_parameter("flash_color", Color(1, 1, 1, 1) if target != "Arrel" else Color(1, 0.4, 0.3, 1))
	sprite_node.material = mat
	# 플래시 페이드아웃
	var sprite_ref: WeakRef = weakref(sprite_node)
	var t = create_tween()
	t.tween_method(func(val): mat.set_shader_parameter("flash_amount", val), flash_strength, 0.0, 0.25)
	t.tween_callback(func():
		# S214: null로 지우면 무대 블렌드(타원 마스크, 가장자리 페이드)까지 함께
		# 사라져서, 한 대 맞은 뒤부터 적이 각진 사각형 카드로 변한다.
		# 기본 공격마다 발생하므로 사실상 전투 내내 그 상태로 남았다.
		var node: CanvasItem = sprite_ref.get_ref() as CanvasItem
		_restore_plate_material(node)
	)

## VFX Library, 상태이상 셰이더 (독/화상/약화) 적 스프라이트에 적용
func _apply_status_shader() -> void:
	if not enemy_sprite:
		return
	# 독, poison 셰이더
	if BattleManager.has_status("enemy", BattleManager.StatusEffect.POISON):
		var shader_path = "res://addons/vfx_lib/shaders/poison.gdshader"
		if ResourceLoader.exists(shader_path):
			var mat = ShaderMaterial.new()
			mat.shader = load(shader_path)
			mat.set_shader_parameter("poison_amount", 0.6)
			mat.set_shader_parameter("poison_color", Color(0.3, 1.0, 0.3, 1.0))
			mat.set_shader_parameter("pulse_speed", 3.0)
			enemy_sprite.material = mat
			return
	# 화상, burning 셰이더
	if BattleManager.has_status("enemy", BattleManager.StatusEffect.BURN):
		var shader_path = "res://addons/vfx_lib/shaders/burning.gdshader"
		if ResourceLoader.exists(shader_path):
			var mat = ShaderMaterial.new()
			mat.shader = load(shader_path)
			mat.set_shader_parameter("burn_amount", 0.5)
			mat.set_shader_parameter("fire_color1", Color(1.0, 0.8, 0.2, 1.0))
			mat.set_shader_parameter("fire_color2", Color(1.0, 0.3, 0.0, 1.0))
			mat.set_shader_parameter("distortion_strength", 0.02)
			enemy_sprite.material = mat
			return
	# 약화, 그레이스케일 틴트 (기존 VFX lib grayscale 사용)
	if BattleManager.has_status("enemy", BattleManager.StatusEffect.WEAKEN):
		enemy_sprite.modulate = Color(0.7, 0.6, 0.8, 1.0)
		return
	# 상태이상 없으면 클리어
	_restore_plate_material(enemy_sprite)
	_restore_battler_base_modulate(enemy_sprite)

## 보스 페이즈 2 드라마틱 전환 (프리즈 프레임 + 화면 변색 + 적 분노 광원)
func _on_phase_changed(enemy_name: String, phase: int) -> void:
	if phase != 2:
		return
	var phase_cutin := String(BOSS_PHASE_CUTIN_PATHS.get(enemy_name, ""))
	if phase_cutin != "":
		_play_action_cutin(phase_cutin, false, 0.94, 0.78)
	# 1. 프리즈 프레임 (0.4초)
	get_tree().paused = true
	# 2. 화면 적색 플래시
	var flash = ColorRect.new()
	flash.set_anchors_preset(Control.PRESET_FULL_RECT)
	flash.color = Color(0.8, 0.1, 0.05, 0.6)
	flash.mouse_filter = Control.MOUSE_FILTER_IGNORE
	flash.z_index = 95
	canvas_root.add_child(flash)
	# 3. 경고 텍스트
	var warn = Label.new()
	warn.text = _bl("PHASE 2", "페이즈 2")
	warn.add_theme_font_size_override("font_size", 36)
	warn.add_theme_color_override("font_color", Color(1, 0.3, 0.2))
	warn.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	warn.set_anchors_preset(Control.PRESET_CENTER)
	warn.position = Vector2(640 - 100, 280)
	warn.z_index = 96
	canvas_root.add_child(warn)
	# 프리즈 해제 후 페이드
	await BattleManager.pace_timer(0.4, true, false, true).timeout
	get_tree().paused = false
	# 강한 셰이크
	_screen_shake(3.0)
	# 색수차
	_play_chromatic_aberration(2.0)
	# 적 분노 틴트, outline_glow 셰이더
	var glow_path = "res://addons/vfx_lib/shaders/outline_glow.gdshader"
	if enemy_sprite and ResourceLoader.exists(glow_path):
		var mat = ShaderMaterial.new()
		mat.shader = load(glow_path)
		mat.set_shader_parameter("outline_color", Color(1.0, 0.2, 0.1, 1.0))
		mat.set_shader_parameter("outline_width", 3.0)
		mat.set_shader_parameter("glow_intensity", 1.5)
		enemy_sprite.material = mat
		# 2초 후 페이드
		var gt = create_tween()
		gt.tween_interval(2.0)
		gt.tween_callback(func():
			_restore_plate_material(enemy_sprite)
		)
	# 경고 페이드아웃
	var wt = create_tween()
	wt.tween_property(warn, "modulate:a", 0.0, 1.0).set_delay(0.5)
	wt.tween_callback(warn.queue_free)
	var ft = create_tween()
	ft.tween_property(flash, "color:a", 0.0, 0.6)
	ft.tween_callback(flash.queue_free)

func _displayed_actor_for_ally_action(ally_name: String) -> CanvasItem:
	if ally_name == "Tobias":
		return tobias_sprite
	if ally_name == _displayed_ally_identity:
		return ally_sprite
	return null

func _on_ally_action(ally_name: String, action: String, _value: int) -> void:
	var cutin_path := ""
	var accent := Color(0.74, 0.82, 0.94)
	match ally_name:
		"Elia":
			cutin_path = String(ELIA_ACTION_CUTIN_PATHS.get(action, "res://assets/cg/generated/cinematic_elia_anchor_pulse.png"))
			accent = Color(1.0, 0.80, 0.45)
		"Sable":
			cutin_path = String(SABLE_ACTION_CUTIN_PATHS.get(action, SABLE_ACTION_CUTIN_PATH))
			accent = Color(0.66, 0.78, 1.0)
		"Tobias":
			cutin_path = String(TOBIAS_ACTION_CUTIN_PATHS.get(action, TOBIAS_ACTION_CUTIN_PATH))
			accent = Color(0.84, 0.69, 0.45)
	var actor := _displayed_actor_for_ally_action(ally_name)
	var semantic := "attack" if action == "strike" or action == "remembered_strike" else "cast"
	if actor:
		_play_actor_anim(actor, semantic)
	if cutin_path == "":
		return
	_play_action_cutin(cutin_path, true, 0.88, 0.42)
	# S175: 동행 액션 인디케이터 로케일화 (영문 대문자 대신 한국어 이름/기술명)
	var who := GameManager.localized_speaker(ally_name) if GameManager.current_locale == "ko" else ally_name.to_upper()
	var act_ko := {
		"humming_shield": "허밍 방패", "desperate_reach": "절박한 손길",
		"remembered_strike": "기억의 일격", "anchor_pulse": "닻의 파동",
		"heal": "치유", "strike": "일격", "weaken": "약화", "guard": "수호",
		"analyze": "분석", "archive": "기록 개방", "protect": "결계",
	}
	var act: String = String(act_ko.get(action, action.replace("_", " "))) if GameManager.current_locale == "ko" else action.replace("_", " ").to_upper()
	_show_combat_cue(
		"%s / %s" % [who, act],
		_bl("A companion creates a new line through the pressure.", "동료가 압박 속에 새로운 길을 만듭니다."),
		cutin_path,
		accent,
		1.0
	)
	_show_turn_indicator("%s / %s" % [who, act], accent)
	_screen_shake(0.45)

## S46: 상태이상 셰이더 업데이트 (status_changed 시그널에 연동)
func _update_status_shaders() -> void:
	_apply_status_shader()

## ===================== S46: 세이블 명령 UI =====================

## ===================== S230: 파티 지시 카드 =====================

## 가운데 열 하나에 자세/엘리아/세이블/토비아스 줄을 세로로 쌓는다.
## 각 줄은 자신의 visible만 관리하면 되고, 세로 위치는 컨테이너가 정한다.
func _build_party_orders_panel(root: Control) -> void:
	party_orders_panel = PanelContainer.new()
	party_orders_panel.name = "PartyOrders"
	party_orders_panel.anchor_left = HUD_CENTER_COL_L
	party_orders_panel.anchor_right = HUD_CENTER_COL_R
	party_orders_panel.anchor_top = 0.0
	party_orders_panel.anchor_bottom = 0.0
	party_orders_panel.offset_top = HUD_PARTY_TOP
	party_orders_panel.offset_bottom = HUD_PARTY_TOP
	party_orders_panel.grow_vertical = Control.GROW_DIRECTION_END
	# 넘치더라도 왼쪽(토비아스 판)이 아니라 오른쪽 빈 무대로만 자라게 한다.
	party_orders_panel.grow_horizontal = Control.GROW_DIRECTION_END
	party_orders_panel.visible = false

	var style := StyleBoxFlat.new()
	style.bg_color = Color(0.022, 0.020, 0.032, 0.92)
	style.border_color = Color(0.62, 0.50, 0.32, 0.60)
	style.set_border_width_all(1)
	style.set_corner_radius_all(5)
	style.set_content_margin_all(8)
	party_orders_panel.add_theme_stylebox_override("panel", style)
	root.add_child(party_orders_panel)

	party_orders_rows = VBoxContainer.new()
	party_orders_rows.name = "PartyOrderRows"
	party_orders_rows.add_theme_constant_override("separation", 5)
	party_orders_panel.add_child(party_orders_rows)

## 한 줄의 뼈대: [역할 이름][버튼...]. 역할 이름 폭을 고정해 버튼 열이 정렬된다.
## 흐름 컨테이너를 쓰는 이유: 영어 라벨("Analyze"/"Weaken")은 한국어보다 넓다.
## 고정 가로 상자였다면 줄이 카드보다 넓어져 패널이 좌우로 부풀고,
## 왼쪽 토비아스 판 위로 넘어갔을 것이다. 넘치면 아래로 접힌다.
func _make_party_order_row(role_text: String, role_color: Color) -> HFlowContainer:
	var row := HFlowContainer.new()
	row.add_theme_constant_override("h_separation", 5)
	row.add_theme_constant_override("v_separation", 4)
	var lbl := Label.new()
	lbl.text = role_text
	lbl.custom_minimum_size = Vector2(74, 0)
	lbl.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	UITheme.style_meta_label(lbl, role_color)
	row.add_child(lbl)
	return row

func _make_party_order_button(label_text: String, action_id: String, bg: Color, border: Color, font_color: Color) -> Button:
	var btn := Button.new()
	btn.text = label_text
	# S230: 예전에는 눌린 버튼을 btn.text.to_lower() == action 으로 찾았다.
	# 한국어 라벨을 달자마자 하이라이트가 영원히 꺼지는 구조여서, 식별자를 메타로 옮긴다.
	btn.set_meta("party_action", action_id)
	btn.custom_minimum_size = Vector2(62, 27)
	btn.focus_mode = Control.FOCUS_NONE
	UITheme.style_label(btn, UITheme.make_meta_font(), UITheme.SIZE_META, font_color)
	var style := StyleBoxFlat.new()
	style.bg_color = bg
	style.border_color = border
	style.set_border_width_all(1)
	style.set_corner_radius_all(3)
	style.content_margin_left = 8
	style.content_margin_right = 8
	style.content_margin_top = 4
	style.content_margin_bottom = 4
	btn.add_theme_stylebox_override("normal", style)
	var hover := style.duplicate()
	hover.bg_color = bg.lightened(0.22)
	hover.border_color = Color(0.95, 0.74, 0.40, 0.92)
	btn.add_theme_stylebox_override("hover", hover)
	btn.add_theme_stylebox_override("focus", hover)
	var pressed := style.duplicate()
	pressed.bg_color = bg.lightened(0.34)
	btn.add_theme_stylebox_override("pressed", pressed)
	# S230: disabled 스타일박스를 지정하지 않으면 Godot 기본 테마의 거의 투명한 판이
	# 쓰여서, 잠긴 자세 버튼이 통째로 사라진 것처럼 보였다.
	# 잠긴 칸은 카드 바탕보다 "밝아야" 칸으로 읽힌다. 바탕과 같은 어둠으로 칠하면
	# 버튼이 사라진 것처럼 보이고, 플레이어는 자기가 뭘 못 쓰는지조차 알 수 없다.
	var disabled := style.duplicate()
	disabled.bg_color = Color(0.075, 0.070, 0.090, 0.94)
	disabled.border_color = Color(0.44, 0.42, 0.48, 0.70)
	btn.add_theme_stylebox_override("disabled", disabled)
	btn.add_theme_color_override("font_disabled_color", Color(0.62, 0.59, 0.65, 0.95))
	return btn

## 선택된 지시에 표시를 남긴다. 라벨이 아니라 메타 식별자로 비교한다.
func _mark_party_selection(row: Container, action: String) -> void:
	if row == null:
		return
	for child in row.get_children():
		if not (child is Button):
			continue
		var btn := child as Button
		if not btn.has_meta("party_action"):
			continue
		var selected := String(btn.get_meta("party_action")) == action
		btn.modulate = Color(0.62, 1.0, 0.68) if selected else Color.WHITE
		var style := btn.get_theme_stylebox("normal") as StyleBoxFlat
		if style:
			style.set_border_width_all(2 if selected else 1)

## 표시할 줄이 하나도 없으면 카드 자체를 접는다.
func _refresh_party_orders_panel() -> void:
	if party_orders_panel == null or party_orders_rows == null:
		return
	var any_visible := false
	for child in party_orders_rows.get_children():
		if child is CanvasItem and (child as CanvasItem).visible:
			any_visible = true
			break
	party_orders_panel.visible = any_visible

func _build_ally_command_ui(rows: Control) -> void:
	if not BattleManager.sable_in_party or rows == null:
		return
	ally_cmd_container = _make_party_order_row(_bl("Sable", "세이블"), Color(0.78, 0.90, 1.0))
	ally_cmd_container.name = "SableOrders"

	var cmds := [
		[_bl("Heal", "치유"), "heal"],
		[_bl("Strike", "일격"), "strike"],
		[_bl("Weaken", "약화"), "weaken"],
		[_bl("Guard", "수호"), "guard"],
	]
	for cmd in cmds:
		var action_name := String(cmd[1])
		var btn := _make_party_order_button(
			String(cmd[0]), action_name,
			Color(0.10, 0.14, 0.22, 0.94), Color(0.42, 0.58, 0.74, 0.72), Color(0.84, 0.90, 0.98)
		)
		btn.pressed.connect(func(): _on_ally_cmd(action_name))
		ally_cmd_container.add_child(btn)

	ally_cmd_container.visible = false
	ally_cmd_container.visibility_changed.connect(_refresh_party_orders_panel)
	rows.add_child(ally_cmd_container)

func _on_ally_cmd(action: String) -> void:
	BattleManager.set_ally_command(action)
	AudioManager.play_sfx("ui_select")
	_mark_party_selection(ally_cmd_container, action)

## ===================== S53: 토비아스 명령 UI =====================

func _build_tobias_command_ui(rows: Control) -> void:
	if not BattleManager.tobias_in_party or rows == null:
		return
	tobias_cmd_container = _make_party_order_row(_bl("Tobias", "토비아스"), Color(0.96, 0.84, 0.62))
	tobias_cmd_container.name = "TobiasOrders"

	var cmds := [
		[_bl("Analyze", "분석"), "analyze"],
		[_bl("Archive", "기록"), "archive"],
		[_bl("Protect", "보호"), "protect"],
	]
	for cmd in cmds:
		var action_name := String(cmd[1])
		var btn := _make_party_order_button(
			String(cmd[0]), action_name,
			Color(0.15, 0.12, 0.07, 0.94), Color(0.62, 0.52, 0.32, 0.72), Color(0.94, 0.88, 0.74)
		)
		btn.pressed.connect(func(): _on_tobias_cmd(action_name))
		tobias_cmd_container.add_child(btn)

	tobias_cmd_container.visible = false
	tobias_cmd_container.visibility_changed.connect(_refresh_party_orders_panel)
	rows.add_child(tobias_cmd_container)

func _on_tobias_cmd(action: String) -> void:
	BattleManager.set_tobias_command(action)
	AudioManager.play_sfx("ui_select")
	_mark_party_selection(tobias_cmd_container, action)

## ===================== S51: 스탠스 전환 UI =====================

var stance_container: HFlowContainer
var stance_buttons: Array[Button] = []

func _build_stance_ui(rows: Control) -> void:
	if rows == null:
		return
	stance_container = _make_party_order_row(_bl("Stance", "자세"), Color(0.96, 0.84, 0.64))
	stance_container.name = "BattleStanceRail"

	var stances = [
		[BattleManager.Stance.REMNANT, _bl("Remnant", "잔재"), Color(0.6, 0.55, 0.45)],
		[BattleManager.Stance.PYRE, _bl("Pyre", "화염"), Color(0.85, 0.4, 0.25)],
		[BattleManager.Stance.HOLLOW, _bl("Hollow", "공허"), Color(0.4, 0.35, 0.7)],
	]

	for s in stances:
		var accent: Color = s[2]
		var btn := _make_party_order_button(
			String(s[1]), "stance_%d" % int(s[0]),
			Color(accent.r * 0.28, accent.g * 0.28, accent.b * 0.28, 0.94),
			Color(accent.r * 0.62, accent.g * 0.62, accent.b * 0.62, 0.70),
			Color(0.90, 0.86, 0.80)
		)
		# 해금 체크
		var stance_val = s[0]
		var info = BattleManager.STANCE_INFO[stance_val]
		if GameManager.current_chapter < info["unlock_chapter"]:
			btn.disabled = true
			# S230: 잠김은 modulate로 지우는 게 아니라 disabled 스타일박스로 말한다.
			# 언제 열리는지도 함께 알려 준다.
			btn.tooltip_text = _bl("Unlocks in Chapter %d", "%d장에서 해금") % int(info["unlock_chapter"])
		else:
			btn.focus_mode = Control.FOCUS_ALL
			btn.pressed.connect(func(): _on_stance_select(stance_val))
			btn.focus_entered.connect(func(): AudioManager.play_sfx("ui_hover"))
		stance_container.add_child(btn)
		stance_buttons.append(btn)

	stance_container.visible = false
	stance_container.visibility_changed.connect(_refresh_party_orders_panel)
	rows.add_child(stance_container)
	_update_stance_highlight()

func _on_stance_select(stance: int) -> void:
	BattleManager.switch_stance(stance)
	AudioManager.play_sfx("ui_select")
	_update_stance_highlight()

func _on_stance_changed(_stance: int) -> void:
	_update_stance_highlight()

func _update_stance_highlight() -> void:
	for i in range(stance_buttons.size()):
		var btn = stance_buttons[i]
		if btn.disabled:
			# S230: 예전에는 잠긴 자세를 알파 0.4로 지웠다. 파티 지시 카드 위에서는
			# 배경과 구분되지 않아 "버튼이 없다"로 읽혔다. 이제 disabled 스타일박스가
			# 잠긴 칸을 보여 주므로, 여기서 더 지우지 않는다.
			btn.modulate = Color.WHITE
			continue
		var is_active = (i == BattleManager.current_stance)
		btn.modulate = Color(1.0, 1.0, 0.7) if is_active else Color.WHITE
		var style := btn.get_theme_stylebox("normal") as StyleBoxFlat
		if style:
			style.set_border_width_all(2 if is_active else 1)

## ===================== S51: 에코 표시 =====================

var echo_display: VBoxContainer

func _build_echo_display(root: Control) -> void:
	echo_display = VBoxContainer.new()
	echo_display.anchor_left = 0.7
	echo_display.anchor_right = 0.98
	echo_display.anchor_top = 0.42
	echo_display.anchor_bottom = 0.65
	echo_display.add_theme_constant_override("separation", 2)
	root.add_child(echo_display)

func _on_echo_activated(_echo_type: String, _desc: String) -> void:
	_refresh_echo_display()

func _refresh_echo_display() -> void:
	if not echo_display:
		return
	for child in echo_display.get_children():
		child.queue_free()
	if BattleManager.active_echoes.is_empty():
		return
	var header = Label.new()
	header.text = _bl("Active Echoes", "활성 메아리")
	header.add_theme_font_size_override("font_size", 12)
	header.add_theme_color_override("font_color", UITheme.TEXT_ACCENT)
	echo_display.add_child(header)
	var echo_colors = {
		"fading_warmth": Color(0.4, 0.7, 0.4),
		"lingering_habit": Color(0.55, 0.5, 0.35),
		"elia_anchor": Color(0.4, 0.5, 0.7),
		"sable_shadow": Color(0.5, 0.4, 0.6),
		"bond_fracture": Color(0.6, 0.45, 0.45),
		"identity_fracture": Color(0.6, 0.3, 0.7),
		"total_erasure": Color(0.9, 0.6, 0.2),
	}
	for echo in BattleManager.active_echoes:
		var lbl = Label.new()
		var echo_name = echo["type"].replace("_", " ").capitalize()
		lbl.text = "%s (%dt)" % [echo_name, echo.get("turns", 0)]
		lbl.add_theme_font_size_override("font_size", 12)
		lbl.add_theme_color_override("font_color", echo_colors.get(echo["type"], Color(0.6, 0.6, 0.6)))
		echo_display.add_child(lbl)

## ===================== S51: 엘리아 기술 UI =====================

var elia_skill_container: HFlowContainer

func _build_elia_skill_ui(rows: Control) -> void:
	if not GameManager.player_data.elia_with_party or rows == null:
		return
	# S230: 예전에는 이 레일과 토비아스 레일의 앵커가 완전히 같아서,
	# 셋이 모두 동행 중이면 두 줄이 정확히 겹쳐 그려졌다.
	elia_skill_container = _make_party_order_row(_bl("Elia", "엘리아"), Color(0.88, 0.78, 1.0))
	elia_skill_container.name = "EliaTechniqueRail"
	elia_skill_container.visible = false
	elia_skill_container.visibility_changed.connect(_refresh_party_orders_panel)
	rows.add_child(elia_skill_container)

func _refresh_elia_skills() -> void:
	if not elia_skill_container:
		return
	# 헤더(역할 라벨) 외 기존 버튼 제거
	for i in range(elia_skill_container.get_child_count() - 1, 0, -1):
		var stale := elia_skill_container.get_child(i)
		elia_skill_container.remove_child(stale)
		stale.queue_free()

	var skills = EliaDiary.get_available_skills()
	if skills.is_empty():
		# S209: 쓸 기술이 없으면 레일 자체를 감춘다. 예전에는 무대 한가운데에
		# "엘리아 (사용 가능한 기술 없음)"이라는 죽은 라벨이 상시 떠 있었다.
		elia_skill_container.visible = false
		return
	elia_skill_container.visible = true

	for skill in skills:
		var ready: bool = bool(skill["ready"])
		var cd_text: String = _bl(" [READY]", " [준비]") if ready else _bl(" [%dT]", " [%d턴]") % skill["cooldown"]
		var skill_id: String = String(skill["id"])
		var btn := _make_party_order_button(
			"%s%s" % [skill["name"], cd_text], skill_id,
			Color(0.14, 0.09, 0.20, 0.94) if ready else Color(0.075, 0.062, 0.092, 0.90),
			Color(0.62, 0.46, 0.78, 0.72) if ready else Color(0.30, 0.26, 0.34, 0.44),
			Color(0.88, 0.80, 0.98) if ready else Color(0.56, 0.52, 0.60)
		)
		btn.tooltip_text = skill["desc"]
		btn.disabled = not ready
		btn.pressed.connect(func(): _on_elia_skill(skill_id))
		elia_skill_container.add_child(btn)

func _on_elia_skill(skill_id: String) -> void:
	AudioManager.play_sfx("ui_select")
	BattleManager.player_use_elia_skill(skill_id)
	_refresh_elia_skills()

## ===================== S52: 전투 VFX 강화 =====================

## 크리티컬 히트 줌 펀치, 강력한 공격 시 화면 줌인→복귀
func _critical_zoom_punch() -> void:
	# 전투 루트 스케일로 줌 효과 근사
	var original_scale = canvas_root.scale
	var original_pivot = canvas_root.pivot_offset
	canvas_root.pivot_offset = canvas_root.size / 2.0 if canvas_root.size != Vector2.ZERO else Vector2(640, 360)

	# 빠른 줌인
	var t = create_tween()
	t.tween_property(canvas_root, "scale", Vector2(1.08, 1.08), 0.1).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_CUBIC)
	t.tween_property(canvas_root, "scale", Vector2(1.0, 1.0), 0.3).set_ease(Tween.EASE_IN_OUT).set_trans(Tween.TRANS_ELASTIC)

	# 크리티컬 플래시, 밝은 임팩트 프레임
	var flash = ColorRect.new()
	flash.set_anchors_preset(Control.PRESET_FULL_RECT)
	flash.color = Color(1.0, 0.95, 0.8, 0.35)
	flash.mouse_filter = Control.MOUSE_FILTER_IGNORE
	canvas_root.add_child(flash)
	var ft = create_tween()
	ft.tween_property(flash, "color:a", 0.0, 0.25).set_ease(Tween.EASE_OUT)
	ft.tween_callback(flash.queue_free)

## 연소 임팩트, 기억 연소 시 화면 가장자리 불타는 효과
func _burn_edge_flare() -> void:
	if OptionsMenu.is_clean_gameplay_visuals():
		return
	var flare = ColorRect.new()
	flare.set_anchors_preset(Control.PRESET_FULL_RECT)
	flare.mouse_filter = Control.MOUSE_FILTER_IGNORE

	# 비네트 스타일 화염 테두리
	var shader_path = "res://assets/shaders/vignette.gdshader"
	if ResourceLoader.exists(shader_path):
		var mat = ShaderMaterial.new()
		mat.shader = load(shader_path)
		mat.set_shader_parameter("color", Color(0.9, 0.4, 0.1, 0.5))
		mat.set_shader_parameter("radius", 0.6)
		mat.set_shader_parameter("softness", 0.4)
		flare.material = mat
	else:
		flare.color = Color(0.8, 0.3, 0.1, 0.15)

	canvas_root.add_child(flare)
	var t = create_tween()
	t.tween_property(flare, "modulate:a", 0.0, 0.6).set_ease(Tween.EASE_OUT)
	t.tween_callback(flare.queue_free)

## 속성 임팩트 배경 플래시 (속성별 색상)
func _element_flash(element: String) -> void:
	var flash_color: Color
	match element:
		"fire": flash_color = Color(1.0, 0.4, 0.1, 0.2)
		"void": flash_color = Color(0.5, 0.2, 0.8, 0.2)
		"physical": flash_color = Color(0.9, 0.9, 0.9, 0.15)
		_: flash_color = Color(0.8, 0.7, 0.3, 0.15)

	var flash = ColorRect.new()
	flash.set_anchors_preset(Control.PRESET_FULL_RECT)
	flash.color = flash_color
	flash.mouse_filter = Control.MOUSE_FILTER_IGNORE
	canvas_root.add_child(flash)
	var t = create_tween()
	t.tween_property(flash, "color:a", 0.0, 0.3).set_ease(Tween.EASE_OUT)
	t.tween_callback(flash.queue_free)

## ===================== S55: Element Color Helper =====================

func _get_element_color(element: String) -> Color:
	match element:
		"physical": return Color(0.8, 0.75, 0.6, 0.9)
		"fire": return Color(0.9, 0.4, 0.15, 0.9)
		"void": return Color(0.6, 0.25, 0.75, 0.9)
	return Color(0.6, 0.6, 0.6, 0.9)

## ===================== S55: Bestiary Scan Display =====================

func _on_enemy_scanned(enemy_name: String, weakness: String, resistance: String) -> void:
	# Flash scan effect
	_show_turn_indicator("SCAN COMPLETE", Color(0.4, 0.8, 0.9))
	# Refresh status icons to show weakness/resistance
	_update_status_icons()
	# Also refresh HP display (now shows numbers)
	_update_hp_displays()

## ===================== S55: Battle Environment Display =====================

func _on_environment_info(env_name: String, bonus_text: String) -> void:
	# Show environment name + bonus as a temporary label at top
	if env_name == "":
		return
	var lbl = Label.new()
	lbl.text = "[%s] %s" % [env_name, bonus_text]
	lbl.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	lbl.set_anchors_preset(Control.PRESET_TOP_WIDE)
	lbl.offset_top = 28
	lbl.offset_bottom = 48
	lbl.add_theme_font_size_override("font_size", 13)
	lbl.add_theme_color_override("font_color", Color(0.72, 0.92, 0.78, 1.0))
	lbl.modulate.a = 0.0
	lbl.z_index = 60
	lbl.mouse_filter = Control.MOUSE_FILTER_IGNORE
	canvas_root.add_child(lbl)
	var t2 = create_tween()
	t2.tween_property(lbl, "modulate:a", 0.8, 0.3)
	t2.tween_interval(2.5)
	t2.tween_property(lbl, "modulate:a", 0.0, 0.5)
	t2.tween_callback(lbl.queue_free)

func _on_tactical_objective_changed(objective: Dictionary) -> void:
	if objective_panel == null or objective_title_label == null or objective_desc_label == null:
		return
	var status: String = objective.get("status", "active")
	var title: String = objective.get("title", "Objective")
	var desc: String = objective.get("desc", "")
	var shown_title := _localized_objective_title(title)
	objective_title_label.text = ("목표 - %s" % shown_title) if GameManager.current_locale == "ko" else "OBJECTIVE - %s" % title.to_upper()
	var progress_text := String(objective.get("progress_text", ""))
	objective_desc_label.text = _localized_objective_desc(desc)
	if progress_text != "" and status == "active":
		objective_desc_label.text += "  ·  " + progress_text
	if status == "active":
		var payoff_parts: Array[String] = ["+%dG" % int(objective.get("reward_grains", 0))]
		if int(objective.get("reward_heal", 0)) > 0:
			payoff_parts.append("HP+%d" % int(objective.get("reward_heal", 0)))
		var payoff_item := String(objective.get("reward_item", ""))
		if payoff_item != "" and GameManager.ITEMS.has(payoff_item):
			payoff_parts.append(String(GameManager.ITEMS[payoff_item].get("name", payoff_item)))
		objective_desc_label.text += ("  ·  보상 " if GameManager.current_locale == "ko" else "  ·  PAYOFF ") + "/".join(payoff_parts)
	var style = objective_panel.get_theme_stylebox("panel") as StyleBoxFlat
	if style:
		match status:
			"complete":
				style.border_color = Color(0.45, 0.95, 0.58, 0.82)
				style.bg_color = Color(0.018, 0.072, 0.038, 0.94)
				objective_desc_label.text = "완료. 보너스 확보." if GameManager.current_locale == "ko" else "Complete. Bonus secured."
				objective_desc_label.add_theme_color_override("font_color", Color(0.62, 1.0, 0.72, 0.92))
			"failed":
				style.border_color = Color(0.80, 0.28, 0.22, 0.76)
				style.bg_color = Color(0.078, 0.020, 0.022, 0.94)
				objective_desc_label.text = "실패. 전투를 끝내야 합니다." if GameManager.current_locale == "ko" else "Lost. Finish the fight."
				objective_desc_label.add_theme_color_override("font_color", Color(1.0, 0.55, 0.48, 0.88))
			_:
				style.border_color = Color(0.72, 0.55, 0.25, 0.52)
				style.bg_color = Color(0.020, 0.016, 0.026, 0.94)
				objective_desc_label.add_theme_color_override("font_color", UITheme.TEXT_PRIMARY)
	var tw = create_tween()
	tw.tween_property(objective_panel, "scale", Vector2(1.03, 1.03), 0.08).set_trans(Tween.TRANS_CUBIC)
	tw.tween_property(objective_panel, "scale", Vector2(1.0, 1.0), 0.14).set_trans(Tween.TRANS_CUBIC)
	_announce_objective_resolution(status, shown_title, objective)
	_update_objective_action_warnings()

## S226: Completing or losing the directive is a moment, not a log line.
func _announce_objective_resolution(status: String, shown_title: String, objective: Dictionary) -> void:
	if status == _last_objective_status:
		return
	_last_objective_status = status
	match status:
		"complete":
			var payoff_parts: Array[String] = ["+%dG" % int(objective.get("reward_grains", 0))]
			if int(objective.get("reward_heal", 0)) > 0:
				payoff_parts.append("HP+%d" % int(objective.get("reward_heal", 0)))
			_show_combat_cue(
				_bl("DIRECTIVE COMPLETE", "지침 달성"),
				"%s  ·  %s" % [shown_title, "/".join(payoff_parts)],
				"",
				Color(0.52, 0.98, 0.62),
				1.25
			)
			AudioManager.play_sfx("memory_add")
		"failed":
			_show_combat_cue(
				_bl("DIRECTIVE LOST", "지침 실패"),
				_bl("%s cannot be recovered in this fight.", "%s은(는) 이 전투에서 되돌릴 수 없다.") % shown_title,
				"",
				Color(1.0, 0.36, 0.30),
				1.35
			)
			AudioManager.play_sfx("cancel")
			_screen_shake(0.5)

func _on_momentum_changed(value: float, rank: int, label: String) -> void:
	if objective_meta_label == null:
		return
	var meta_prefix := "공명" if GameManager.current_locale == "ko" else "Resonance"
	objective_meta_label.text = "%s: %s %d%%" % [meta_prefix, label, int(value)]
	var aftershock := BattleManager.get_burn_aftershock_state()
	if bool(aftershock.get("active", false)):
		var aftershock_text := "연소 잔상 %d회" if GameManager.current_locale == "ko" else "Burn afterimage %d"
		objective_meta_label.text += "  ·  " + (aftershock_text % int(aftershock.get("turns", 0)))
	var color := Color(0.58, 0.74, 0.92, 0.82)
	match rank:
		1:
			color = Color(0.78, 0.70, 0.48, 0.90)
		2:
			color = Color(0.95, 0.66, 0.38, 0.94)
		3:
			color = Color(1.0, 0.47, 0.34, 0.98)
		4:
			color = Color(0.88, 0.92, 1.0, 1.0)
	if bool(aftershock.get("active", false)):
		color = Color(1.0, 0.48, 0.34, 1.0)
	objective_meta_label.add_theme_color_override("font_color", color)
	if rank >= 3:
		var tw = create_tween()
		tw.tween_property(objective_meta_label, "scale", Vector2(1.05, 1.05), 0.08)
		tw.tween_property(objective_meta_label, "scale", Vector2(1.0, 1.0), 0.16)

## S226: One translation table lives in BattleManager so the card, the forecast,
## the burn warning, and the victory panel never drift apart.
func _localized_objective_title(title: String) -> String:
	return BattleManager.localize_objective_title(title)

func _localized_objective_desc(desc: String) -> String:
	return BattleManager.localize_objective_desc(desc)

## ===================== S54/S58: Victory Rewards Screen (animated) =====================

var _rewards_can_dismiss: bool = false  # S58: true after all reveals done

func _show_victory_screen() -> void:
	# S58: Now just a placeholder, real rewards screen is built by _on_victory_rewards_ready
	pass

## S58: Animated post-battle rewards screen
func _on_victory_rewards_ready(rewards: Dictionary) -> void:
	var is_boss: bool = rewards.get("is_boss", false)
	var grains: int = rewards.get("grains", 0)
	var tactical_bonus: int = rewards.get("tactical_bonus", 0)
	var objective_bonus: int = rewards.get("objective_bonus", 0)
	var objective_title: String = rewards.get("objective_title", "")
	var objective_item: String = rewards.get("objective_item", "")
	var objective_heal: int = rewards.get("objective_heal", 0)
	var momentum_bonus: int = rewards.get("momentum_bonus", 0)
	var momentum_rank: int = rewards.get("momentum_rank", 0)
	var momentum_label: String = rewards.get("momentum_label", "Cold")
	var battle_grade: String = rewards.get("battle_grade", "D")
	var battle_score: int = rewards.get("battle_score", 0)
	var grade_bonus: int = rewards.get("grade_bonus", 0)
	var directive_streak: int = rewards.get("directive_streak", 0)
	var streak_bonus: int = rewards.get("streak_bonus", 0)
	var streak_focus: int = rewards.get("streak_focus", 0)
	var streak_item: String = rewards.get("streak_item", "")
	var preservation_bonus: int = rewards.get("preservation_bonus", 0)
	var field_focus_gained: int = rewards.get("field_focus_gained", 0)
	var resolution: String = rewards.get("resolution", "defeat")
	var heal: int = rewards.get("heal", 0)
	var item_name: String = rewards.get("item", "")
	var dropped_item_id := ""
	if item_name != "":
		for candidate_id in GameManager.ITEMS:
			if String(GameManager.ITEMS[candidate_id].get("name", "")) == item_name:
				dropped_item_id = candidate_id
				break
	var enemy_name: String = rewards.get("enemy_name", "Unknown")
	_rewards_can_dismiss = false

	# Semi-transparent backdrop
	var backdrop = ColorRect.new()
	backdrop.set_anchors_preset(Control.PRESET_FULL_RECT)
	backdrop.color = Color(0.0, 0.0, 0.0, 0.0)
	backdrop.mouse_filter = Control.MOUSE_FILTER_STOP
	backdrop.z_index = 80
	canvas_root.add_child(backdrop)
	var t_bg = create_tween()
	t_bg.tween_property(backdrop, "color:a", 0.55, 0.3)

	# Victory panel (wider for new layout)
	_victory_panel = PanelContainer.new()
	_victory_panel.set_anchors_preset(Control.PRESET_CENTER)
	_victory_panel.offset_left = -210
	_victory_panel.offset_right = 210
	# S226: room for the one consequence line that closes the fight.
	_victory_panel.offset_top = -205
	_victory_panel.offset_bottom = 205
	_victory_panel.z_index = 85
	_victory_art = _make_interface_texture(UI_VICTORY_PANEL_PATH, 0.86)
	_victory_art.set_anchors_preset(Control.PRESET_CENTER)
	_victory_art.offset_left = -260
	_victory_art.offset_right = 260
	_victory_art.offset_top = -245
	_victory_art.offset_bottom = 245
	_victory_art.z_index = 84
	canvas_root.add_child(_victory_art)

	var panel_style = StyleBoxFlat.new()
	panel_style.bg_color = Color(0.035, 0.030, 0.052, 0.80)
	panel_style.border_color = Color(0.75, 0.6, 0.3, 0.9) if not is_boss else Color(0.9, 0.3, 0.2, 0.9)
	panel_style.set_border_width_all(2)
	panel_style.set_corner_radius_all(8)
	panel_style.set_content_margin_all(18)
	_victory_panel.add_theme_stylebox_override("panel", panel_style)
	canvas_root.add_child(_victory_panel)

	var vbox = VBoxContainer.new()
	vbox.add_theme_constant_override("separation", 6)
	_victory_panel.add_child(vbox)

	# --- TITLE ---
	var title = Label.new()
	title.text = ("메아리 해방" if GameManager.current_locale == "ko" else "ECHO RELEASED") if resolution == "witness" else ("BOSS DEFEATED" if is_boss else "VICTORY")
	title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	var title_color = Color(0.95, 0.4, 0.3) if is_boss else Color(0.9, 0.8, 0.5)
	title.add_theme_color_override("font_color", title_color)
	title.add_theme_font_size_override("font_size", 28 if is_boss else 24)
	title.add_theme_color_override("font_outline_color", Color(0.15, 0.1, 0.05))
	title.add_theme_constant_override("outline_size", 2)
	vbox.add_child(title)

	# Title scale-in animation
	title.pivot_offset = Vector2(210, 16)
	title.scale = Vector2(0.3, 0.3)
	var t_title = create_tween()
	t_title.tween_property(title, "scale", Vector2(1.0, 1.0), 0.4).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_BACK)

	# Defeated enemy subtitle
	var enemy_line = Label.new()
	enemy_line.text = (("해방: %s" if GameManager.current_locale == "ko" else "Released: %s") % enemy_name) if resolution == "witness" else "Defeated: %s" % enemy_name
	enemy_line.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	enemy_line.add_theme_font_size_override("font_size", 13)
	enemy_line.add_theme_color_override("font_color", Color(0.5, 0.45, 0.4, 0.0))
	vbox.add_child(enemy_line)

	# Separator
	var sep = HSeparator.new()
	sep.add_theme_color_override("separator", Color(0.4, 0.35, 0.25, 0.6))
	vbox.add_child(sep)

	# --- TACTICAL GRADE (objective, resonance, witness, break, combo, speed) ---
	var growth_row = HBoxContainer.new()
	growth_row.add_theme_constant_override("separation", 8)
	vbox.add_child(growth_row)

	var growth_lbl = Label.new()
	growth_lbl.text = _bl("Tactical Grade", "전술 등급")
	growth_lbl.add_theme_font_size_override("font_size", 12)
	growth_lbl.add_theme_color_override("font_color", Color(0.6, 0.55, 0.45, 0.0))
	growth_row.add_child(growth_lbl)

	var growth_bar = ProgressBar.new()
	growth_bar.custom_minimum_size = Vector2(150, 14)
	growth_bar.max_value = 100
	growth_bar.value = 0
	growth_bar.show_percentage = false
	var growth_fill = StyleBoxFlat.new()
	growth_fill.bg_color = Color(0.7, 0.55, 0.25)
	growth_fill.set_corner_radius_all(3)
	growth_bar.add_theme_stylebox_override("fill", growth_fill)
	var growth_bg = StyleBoxFlat.new()
	growth_bg.bg_color = Color(0.12, 0.1, 0.08)
	growth_bg.set_corner_radius_all(3)
	growth_bar.add_theme_stylebox_override("background", growth_bg)
	growth_bar.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	growth_row.add_child(growth_bar)

	var grade_value := Label.new()
	grade_value.text = "%s  %d" % [battle_grade, battle_score]
	grade_value.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	grade_value.custom_minimum_size = Vector2(58, 0)
	grade_value.add_theme_font_size_override("font_size", 17)
	grade_value.add_theme_color_override("font_color", Color(0.98, 0.82, 0.38, 0.0))
	growth_row.add_child(grade_value)

	# --- GRAINS EARNED ---
	var grains_row = HBoxContainer.new()
	grains_row.add_theme_constant_override("separation", 8)
	vbox.add_child(grains_row)
	var grains_token = GameManager.get_ui_icon("grains")
	if grains_token:
		var grains_art = TextureRect.new()
		grains_art.custom_minimum_size = Vector2(26, 26)
		grains_art.texture = grains_token
		grains_art.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
		grains_art.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
		grains_row.add_child(grains_art)

	var grains_icon = Label.new()
	grains_icon.text = _bl("Grains Earned", "획득 그레인")
	grains_icon.add_theme_font_size_override("font_size", 13)
	grains_icon.add_theme_color_override("font_color", Color(0.85, 0.7, 0.35, 0.0))
	grains_icon.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	grains_row.add_child(grains_icon)

	var grains_value = Label.new()
	grains_value.text = "0"
	grains_value.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	grains_value.add_theme_font_size_override("font_size", 16)
	grains_value.add_theme_color_override("font_color", Color(1.0, 0.9, 0.4, 0.0))
	grains_value.add_theme_color_override("font_outline_color", Color(0.2, 0.15, 0.0))
	grains_value.add_theme_constant_override("outline_size", 2)
	grains_row.add_child(grains_value)

	var bonus_row = HBoxContainer.new()
	bonus_row.add_theme_constant_override("separation", 8)
	bonus_row.modulate.a = 0.0
	vbox.add_child(bonus_row)

	var bonus_lbl = Label.new()
	bonus_lbl.text = _bl("Performance", "전투 성과")
	bonus_lbl.add_theme_font_size_override("font_size", 13)
	bonus_lbl.add_theme_color_override("font_color", Color(0.45, 0.78, 0.9))
	bonus_lbl.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	bonus_row.add_child(bonus_lbl)

	var bonus_val = Label.new()
	var performance_parts: Array[String] = []
	performance_parts.append("%s +%d" % [battle_grade, grade_bonus])
	if directive_streak > 0:
		performance_parts.append(_bl("Chain x%d +%d", "연속 x%d +%d") % [directive_streak, streak_bonus])
	if preservation_bonus > 0:
		performance_parts.append(_bl("Preserve +%d", "보존 +%d") % preservation_bonus)
	var preservation_focus := maxi(field_focus_gained - streak_focus, 0)
	if preservation_focus > 0:
		performance_parts.append("Focus +%d" % preservation_focus)
	if streak_focus > 0:
		performance_parts.append(_bl("Chain Focus +%d", "연속 Focus +%d") % streak_focus)
	if streak_item != "" and GameManager.ITEMS.has(streak_item):
		performance_parts.append(String(GameManager.ITEMS[streak_item].get("name", streak_item)))
	bonus_val.text = " / ".join(performance_parts)
	bonus_val.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	bonus_val.add_theme_font_size_override("font_size", 12)
	bonus_val.add_theme_color_override("font_color", Color(0.55, 0.86, 1.0))
	bonus_row.add_child(bonus_val)

	var objective_row = HBoxContainer.new()
	objective_row.add_theme_constant_override("separation", 8)
	objective_row.modulate.a = 0.0
	vbox.add_child(objective_row)

	var objective_lbl = Label.new()
	objective_lbl.text = "목표" if GameManager.current_locale == "ko" else "Objective"
	objective_lbl.add_theme_font_size_override("font_size", 13)
	objective_lbl.add_theme_color_override("font_color", Color(0.72, 0.92, 0.55))
	objective_lbl.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	objective_row.add_child(objective_lbl)

	var objective_val = Label.new()
	var objective_text := "+%d" % objective_bonus if objective_bonus > 0 else ("실패" if GameManager.current_locale == "ko" else "Missed")
	if objective_bonus > 0 and objective_title != "":
		objective_text = "%s +%d" % [_localized_objective_title(objective_title), objective_bonus]
	if objective_item != "" and GameManager.ITEMS.has(objective_item):
		objective_text += " / %s" % GameManager.ITEMS[objective_item]["name"]
	if objective_heal > 0:
		objective_text += " / HP +%d" % objective_heal
	objective_val.text = objective_text
	objective_val.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	objective_val.add_theme_font_size_override("font_size", 12)
	objective_val.add_theme_color_override("font_color", Color(0.74, 1.0, 0.56) if objective_bonus > 0 else Color(0.36, 0.34, 0.32))
	objective_row.add_child(objective_val)

	var momentum_row = HBoxContainer.new()
	momentum_row.add_theme_constant_override("separation", 8)
	momentum_row.modulate.a = 0.0
	vbox.add_child(momentum_row)

	var momentum_lbl = Label.new()
	momentum_lbl.text = "공명" if GameManager.current_locale == "ko" else "Resonance"
	momentum_lbl.add_theme_font_size_override("font_size", 13)
	momentum_lbl.add_theme_color_override("font_color", Color(0.58, 0.74, 0.92))
	momentum_lbl.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	momentum_row.add_child(momentum_lbl)

	var momentum_val = Label.new()
	momentum_val.text = "%s R%d +%d" % [momentum_label, momentum_rank, momentum_bonus] if momentum_bonus > 0 else ("냉각" if GameManager.current_locale == "ko" else "Cold")
	momentum_val.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	momentum_val.add_theme_font_size_override("font_size", 12)
	momentum_val.add_theme_color_override("font_color", Color(0.78, 0.88, 1.0) if momentum_bonus > 0 else Color(0.36, 0.34, 0.32))
	momentum_row.add_child(momentum_val)

	# --- S226: WHAT THIS FIGHT LEFT BEHIND (one line, always last word) ---
	var aftermath_text: String = rewards.get("aftermath", "")
	var aftermath_label = Label.new()
	aftermath_label.text = aftermath_text
	aftermath_label.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	aftermath_label.max_lines_visible = 2
	aftermath_label.text_overrun_behavior = TextServer.OVERRUN_TRIM_ELLIPSIS
	aftermath_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	aftermath_label.add_theme_font_size_override("font_size", 13)
	aftermath_label.add_theme_color_override("font_color", Color(0.84, 0.72, 0.96))
	aftermath_label.modulate.a = 0.0
	aftermath_label.visible = aftermath_text != ""
	vbox.add_child(aftermath_label)

	# --- HP RECOVERED ---
	var heal_row = HBoxContainer.new()
	heal_row.add_theme_constant_override("separation", 8)
	vbox.add_child(heal_row)

	var heal_lbl = Label.new()
	heal_lbl.text = _bl("HP Recovered", "HP 회복")
	heal_lbl.add_theme_font_size_override("font_size", 12)
	heal_lbl.add_theme_color_override("font_color", Color(0.4, 0.8, 0.45, 0.0))
	heal_lbl.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	heal_row.add_child(heal_lbl)

	var heal_val = Label.new()
	heal_val.text = "+%d" % heal
	heal_val.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	heal_val.add_theme_font_size_override("font_size", 13)
	heal_val.add_theme_color_override("font_color", Color(0.5, 0.9, 0.5, 0.0))
	heal_row.add_child(heal_val)

	# --- ITEM DROP ---
	var item_row = HBoxContainer.new()
	item_row.add_theme_constant_override("separation", 8)
	item_row.modulate.a = 0.0
	vbox.add_child(item_row)
	if dropped_item_id != "":
		var item_art = TextureRect.new()
		item_art.custom_minimum_size = Vector2(30, 30)
		item_art.texture = GameManager.get_item_icon(dropped_item_id)
		item_art.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
		item_art.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
		item_row.add_child(item_art)

	var item_lbl = Label.new()
	item_lbl.text = _bl("Item Found", "획득 아이템")
	item_lbl.add_theme_font_size_override("font_size", 12)
	item_lbl.add_theme_color_override("font_color", Color(0.6, 0.7, 0.9))
	item_lbl.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	item_row.add_child(item_lbl)

	var item_val = Label.new()
	item_val.text = item_name if item_name != "" else "None"
	item_val.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	item_val.add_theme_font_size_override("font_size", 13)
	item_val.add_theme_color_override("font_color", Color(0.7, 0.8, 1.0) if item_name != "" else Color(0.4, 0.38, 0.35))
	item_row.add_child(item_val)

	# --- MEMORY ACQUIRED (from _victory_rewards) ---
	var has_memory_reward = false
	for r in _victory_rewards:
		if "Memory" in r or "memory" in r:
			has_memory_reward = true
			break

	var memory_row = HBoxContainer.new()
	memory_row.add_theme_constant_override("separation", 8)
	memory_row.modulate.a = 0.0
	vbox.add_child(memory_row)

	if has_memory_reward:
		var mem_lbl = Label.new()
		mem_lbl.text = _bl("Memory Acquired", "기억 획득")
		mem_lbl.add_theme_font_size_override("font_size", 13)
		mem_lbl.add_theme_color_override("font_color", Color(1.0, 0.85, 0.3))
		mem_lbl.size_flags_horizontal = Control.SIZE_EXPAND_FILL
		memory_row.add_child(mem_lbl)

	# --- SPACER ---
	var spacer = Control.new()
	spacer.custom_minimum_size = Vector2(0, 4)
	vbox.add_child(spacer)

	# --- CONTINUE hint (hidden initially) ---
	var hint = Label.new()
	hint.text = _bl("Press any key to continue", "아무 키나 눌러 계속")
	hint.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	hint.add_theme_font_size_override("font_size", 13)
	hint.add_theme_color_override("font_color", Color(0.5, 0.45, 0.4, 0.0))
	vbox.add_child(hint)

	# Panel entrance animation
	if _victory_art:
		_victory_art.modulate.a = 0.0
		_victory_art.scale = Vector2(0.85, 0.85)
		_victory_art.pivot_offset = Vector2(260, 245)
	_victory_panel.modulate.a = 0.0
	_victory_panel.scale = Vector2(0.85, 0.85)
	_victory_panel.pivot_offset = Vector2(210, 205)
	var t_panel = create_tween().set_parallel(true)
	if _victory_art:
		t_panel.tween_property(_victory_art, "modulate:a", 0.86, 0.3)
		t_panel.tween_property(_victory_art, "scale", Vector2(1.0, 1.0), 0.35).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_BACK)
	t_panel.tween_property(_victory_panel, "modulate:a", 1.0, 0.3)
	t_panel.tween_property(_victory_panel, "scale", Vector2(1.0, 1.0), 0.35).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_BACK)

	# ====== SEQUENTIAL REVEAL ANIMATION ======
	# 0.3s: Enemy name fades in
	var tw_seq = create_tween()
	tw_seq.tween_interval(0.3)
	tw_seq.tween_property(enemy_line, "theme_override_colors/font_color", Color(0.5, 0.45, 0.4, 0.8), 0.25)

	# 0.6s: Tactical grade fills from the battle's actual performance.
	tw_seq.tween_interval(0.1)
	tw_seq.tween_property(growth_lbl, "theme_override_colors/font_color", Color(0.6, 0.55, 0.45, 1.0), 0.2)
	tw_seq.tween_property(grade_value, "theme_override_colors/font_color", Color(0.98, 0.82, 0.38, 1.0), 0.2)
	var bar_target = clampf(float(battle_score), 0.0, 100.0)
	tw_seq.tween_property(growth_bar, "value", bar_target, 0.6).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_CUBIC)

	# 1.0s: Grains count up
	tw_seq.tween_property(grains_icon, "theme_override_colors/font_color", Color(0.85, 0.7, 0.35, 1.0), 0.15)
	tw_seq.tween_property(grains_value, "theme_override_colors/font_color", Color(1.0, 0.9, 0.4, 1.0), 0.15)
	# Count-up animation for grains (step by step)
	if grains > 0:
		var grain_steps = mini(grains, 20)  # Max 20 ticks
		var grain_per_step = maxi(grains / grain_steps, 1)
		for i in range(grain_steps):
			var display_val = mini((i + 1) * grain_per_step, grains)
			tw_seq.tween_callback(func(): grains_value.text = str(display_val))
			tw_seq.tween_callback(func(): AudioManager.play_sfx("ui_select"))
			tw_seq.tween_interval(0.04)
		tw_seq.tween_callback(func(): grains_value.text = str(grains))

	tw_seq.tween_interval(0.08)
	tw_seq.tween_property(bonus_row, "modulate:a", 1.0, 0.18)
	if grade_bonus > 0 or streak_bonus > 0 or preservation_bonus > 0 or tactical_bonus > 0:
		tw_seq.tween_callback(func(): AudioManager.play_sfx("ui_select"))

	tw_seq.tween_interval(0.08)
	if objective_bonus > 0:
		tw_seq.tween_property(objective_row, "modulate:a", 1.0, 0.18)
		tw_seq.tween_callback(func(): AudioManager.play_sfx("memory_add"))
	else:
		tw_seq.tween_property(objective_row, "modulate:a", 0.45, 0.12)

	tw_seq.tween_interval(0.08)
	if momentum_bonus > 0:
		tw_seq.tween_property(momentum_row, "modulate:a", 1.0, 0.18)
		tw_seq.tween_callback(func(): AudioManager.play_sfx("ui_select"))
	else:
		tw_seq.tween_property(momentum_row, "modulate:a", 0.45, 0.12)

	# S226: The consequence line lands before the loot lines.
	if aftermath_text != "":
		tw_seq.tween_interval(0.12)
		tw_seq.tween_property(aftermath_label, "modulate:a", 1.0, 0.28)

	# Heal line
	tw_seq.tween_interval(0.15)
	tw_seq.tween_property(heal_lbl, "theme_override_colors/font_color", Color(0.4, 0.8, 0.45, 1.0), 0.2)
	tw_seq.tween_property(heal_val, "theme_override_colors/font_color", Color(0.5, 0.9, 0.5, 1.0), 0.15)

	# Item drop (slide in with pop)
	tw_seq.tween_interval(0.2)
	if item_name != "":
		tw_seq.tween_property(item_row, "modulate:a", 1.0, 0.2)
		tw_seq.tween_callback(func(): AudioManager.play_sfx("ui_select"))
		# Pop scale
		tw_seq.tween_property(item_row, "scale", Vector2(1.08, 1.08), 0.1)
		tw_seq.tween_property(item_row, "scale", Vector2(1.0, 1.0), 0.1)
	else:
		tw_seq.tween_property(item_row, "modulate:a", 0.5, 0.2)

	# Memory (golden glow + special sound)
	if has_memory_reward:
		tw_seq.tween_interval(0.3)
		tw_seq.tween_property(memory_row, "modulate:a", 1.0, 0.3)
		tw_seq.tween_callback(func(): AudioManager.play_sfx("memory_add"))
		# Gold flash behind memory row
		tw_seq.tween_property(memory_row, "modulate", Color(1.3, 1.2, 0.8, 1.0), 0.15)
		tw_seq.tween_property(memory_row, "modulate", Color(1.0, 1.0, 1.0, 1.0), 0.3)

	# Show continue hint after all reveals (1.5s minimum from start)
	tw_seq.tween_interval(0.4)
	tw_seq.tween_property(hint, "theme_override_colors/font_color", Color(0.5, 0.45, 0.4, 0.6), 0.2)
	tw_seq.tween_callback(func(): _rewards_can_dismiss = true)

	# Blink hint text
	var t_hint = create_tween().set_loops()
	t_hint.tween_interval(1.5)  # Wait for hint to appear first
	t_hint.tween_property(hint, "modulate:a", 0.3, 0.6)
	t_hint.tween_property(hint, "modulate:a", 1.0, 0.6)

## S58: Handle input to dismiss the rewards screen
## S209: Tab 배속 전환은 `_input`에서 가로챈다.
## `_unhandled_input`까지 내려오면 Godot의 포커스 이동(ui_focus_next)이 먼저
## 이벤트를 소비해 버려서, 커맨드 버튼에 포커스가 있을 때 동작하지 않는다.
func _input(event: InputEvent) -> void:
	if not (event is InputEventKey):
		return
	var key := event as InputEventKey
	if not key.pressed or key.echo or key.keycode != KEY_TAB:
		return
	if _victory_panel != null and is_instance_valid(_victory_panel):
		return
	_on_battle_speed_pressed()
	get_viewport().set_input_as_handled()

func _unhandled_input(event: InputEvent) -> void:
	if _victory_panel and _rewards_can_dismiss:
		if event is InputEventKey and event.pressed:
			_dismiss_rewards()
		elif event is InputEventMouseButton and event.pressed:
			_dismiss_rewards()
		elif event is InputEventJoypadButton and event.pressed:
			_dismiss_rewards()
		return
	if not (event is InputEventKey) or not event.pressed or event.echo:
		return
	if _handle_battle_quick_item_key(event as InputEventKey):
		return
	if action_container == null or not action_container.visible or _is_command_modal_open():
		return
	var command_index := int(event.keycode) - int(KEY_1)
	if command_index < 0 or command_index >= action_container.get_child_count():
		return
	var button := action_container.get_child(command_index) as Button
	if button == null or button.disabled:
		AudioManager.play_sfx("cancel")
		return
	button.grab_focus()
	button.pressed.emit()
	get_viewport().set_input_as_handled()

func _handle_battle_quick_item_key(event: InputEventKey) -> bool:
	if item_list_container == null:
		return false
	var item_scroll := item_list_container.get_meta("scroll_parent") as ScrollContainer
	if item_scroll == null or not item_scroll.visible:
		return false
	var slot_index := int(event.keycode) - int(KEY_1)
	if slot_index < 0 or slot_index >= _battle_quick_item_buttons.size():
		return false
	var button := _battle_quick_item_buttons[slot_index]
	if button.disabled:
		AudioManager.play_sfx("cancel")
	else:
		button.grab_focus()
		button.pressed.emit()
	get_viewport().set_input_as_handled()
	return true

func _is_command_modal_open() -> bool:
	if intro_overlay and intro_overlay.visible:
		return true
	if objective_briefing_overlay and objective_briefing_overlay.visible:
		return true
	if _burn_preview_panel and _burn_preview_panel.visible:
		return true
	if burn_list_container:
		var burn_scroll := burn_list_container.get_meta("scroll_parent") as ScrollContainer
		if burn_scroll and burn_scroll.visible:
			return true
	if item_list_container:
		var item_scroll := item_list_container.get_meta("scroll_parent") as ScrollContainer
		if item_scroll and item_scroll.visible:
			return true
	return false

func _dismiss_rewards() -> void:
	_rewards_can_dismiss = false
	AudioManager.play_sfx("ui_select")
	BattleManager.dismiss_victory()
	# Fade out victory panel
	if _victory_panel and is_instance_valid(_victory_panel):
		var tw = create_tween()
		tw.tween_property(_victory_panel, "modulate:a", 0.0, 0.25)
		tw.tween_callback(_victory_panel.queue_free)
	if _victory_art and is_instance_valid(_victory_art):
		var tw_art = create_tween()
		tw_art.tween_property(_victory_art, "modulate:a", 0.0, 0.25)
		tw_art.tween_callback(_victory_art.queue_free)

## ===================== S58: Burn Preview Popup (Risk/Reward) =====================

## Grade color map for the preview
const GRADE_COLORS: Dictionary = {
	0: Color(0.5, 0.5, 0.45),   # Grade 5, gray
	1: Color(0.4, 0.65, 0.4),   # Grade 4, green
	2: Color(0.3, 0.5, 0.85),   # Grade 3, blue
	3: Color(0.7, 0.4, 0.85),   # Grade 2, purple
	4: Color(0.95, 0.75, 0.2),  # Grade 1, gold
}

const GRADE_LABELS: Dictionary = {
	0: "Grade 5, Sensory Fragment",
	1: "Grade 4, Daily Memory",
	2: "Grade 3, Relationship",
	3: "Grade 2, Identity",
	4: "Grade 1, Core Memory",
}

## Build the burn preview popup (hidden by default, shown on memory selection).
func _build_burn_preview(root: Control) -> void:
	# Dimmer background
	_burn_preview_dimmer = ColorRect.new()
	_burn_preview_dimmer.set_anchors_preset(Control.PRESET_FULL_RECT)
	_burn_preview_dimmer.color = Color(0, 0, 0, 0.6)
	_burn_preview_dimmer.mouse_filter = Control.MOUSE_FILTER_STOP
	_burn_preview_dimmer.z_index = 90
	_burn_preview_dimmer.visible = false
	root.add_child(_burn_preview_dimmer)

	# Main panel
	# S226: The preview now carries the world consequence and the directive
	# conflict as well as the numbers, and each memory brings a different number
	# of those lines, so the frame is sized by its content instead of by a fixed
	# rectangle that either clips or leaves half the screen empty.
	_burn_preview_art = _make_interface_texture(UI_BURN_PREVIEW_PANEL_PATH, 0.86)
	_burn_preview_art.set_anchors_preset(Control.PRESET_TOP_LEFT)
	_burn_preview_art.z_index = 90
	_burn_preview_art.visible = false
	root.add_child(_burn_preview_art)

	var preview_center = CenterContainer.new()
	preview_center.name = "BurnPreviewCenter"
	preview_center.set_anchors_preset(Control.PRESET_FULL_RECT)
	preview_center.mouse_filter = Control.MOUSE_FILTER_IGNORE
	preview_center.z_index = 91
	root.add_child(preview_center)

	_burn_preview_panel = PanelContainer.new()
	_burn_preview_panel.name = "BurnPreviewPanel"
	_burn_preview_panel.custom_minimum_size = Vector2(700, 0)
	_burn_preview_panel.mouse_filter = Control.MOUSE_FILTER_STOP
	_burn_preview_panel.visible = false

	var panel_style = StyleBoxFlat.new()
	panel_style.bg_color = Color(0.040, 0.028, 0.052, 0.82)
	panel_style.border_color = Color(0.7, 0.35, 0.15, 0.85)
	panel_style.set_border_width_all(2)
	panel_style.set_corner_radius_all(6)
	panel_style.set_content_margin_all(20)
	_burn_preview_panel.add_theme_stylebox_override("panel", panel_style)

	preview_center.add_child(_burn_preview_panel)
	_burn_preview_panel.resized.connect(_sync_burn_preview_frame)

## S226: Keeps the decorative plate wrapped around whatever height the preview
## content settled on, and keeps the pop-in animation centred on it.
func _sync_burn_preview_frame() -> void:
	if _burn_preview_panel == null or not is_instance_valid(_burn_preview_panel):
		return
	_burn_preview_panel.pivot_offset = _burn_preview_panel.size / 2.0
	if _burn_preview_art == null or not is_instance_valid(_burn_preview_art):
		return
	var pad := Vector2(28, 24)
	_burn_preview_art.position = _burn_preview_panel.position - pad
	_burn_preview_art.size = _burn_preview_panel.size + pad * 2.0
	_burn_preview_art.pivot_offset = _burn_preview_art.size / 2.0

## Show the burn preview popup for a memory before confirming the burn.
func _show_burn_preview(memory: MemoryManager.Memory) -> void:
	_stop_burn_preview_motion()
	_pending_burn_id = memory.id

	# Clear previous content
	for child in _burn_preview_panel.get_children():
		child.queue_free()

	var vbox = VBoxContainer.new()
	vbox.add_theme_constant_override("separation", 6)
	_burn_preview_panel.add_child(vbox)

	# --- Header: "BURN MEMORY?" ---
	var header = Label.new()
	header.text = _bl("BURN THIS MEMORY?", "이 기억을 태울까?")
	header.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	header.add_theme_font_size_override("font_size", 18)
	header.add_theme_color_override("font_color", Color(0.9, 0.55, 0.2))
	vbox.add_child(header)

	# --- Separator ---
	var sep = HSeparator.new()
	sep.add_theme_constant_override("separation", 8)
	vbox.add_child(sep)

	# --- Memory name + grade (colored) ---
	var grade_color = GRADE_COLORS.get(memory.grade, Color(0.6, 0.6, 0.6))
	var grade_label_text = GRADE_LABELS.get(memory.grade, "Grade %d" % memory.grade)

	var name_label = Label.new()
	name_label.text = MemoryManager.localized_memory_title(memory)
	name_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	name_label.add_theme_font_size_override("font_size", 16)
	name_label.add_theme_color_override("font_color", grade_color)
	vbox.add_child(name_label)

	var grade_lbl = Label.new()
	grade_lbl.text = grade_label_text
	if memory.related_npc != "" and memory.related_npc != "Unknown":
		grade_lbl.text += _bl("  ·  Tied to %s", "  ·  %s와(과) 얽힘") % GameManager.localized_speaker(memory.related_npc)
	grade_lbl.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	grade_lbl.add_theme_font_size_override("font_size", 12)
	grade_lbl.add_theme_color_override("font_color", Color(grade_color.r * 0.7, grade_color.g * 0.7, grade_color.b * 0.7))
	vbox.add_child(grade_lbl)

	# --- DIRECTIVE CONFLICT (read before anything else that tempts you) ---
	var burn_relation := BattleManager.get_objective_action_relation("burn")
	if String(burn_relation.get("kind", "")) != "":
		var relation_label = Label.new()
		var relation_kind := String(burn_relation.get("kind", ""))
		relation_label.text = String(burn_relation.get("text", ""))
		if relation_kind == "fail":
			relation_label.text = _bl("OBJECTIVE: %s", "목표: %s") % relation_label.text
		relation_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
		relation_label.add_theme_font_size_override("font_size", 14)
		relation_label.add_theme_color_override("font_color", _objective_relation_color(relation_kind))
		vbox.add_child(relation_label)
		if relation_kind == "fail":
			var relation_pulse = create_tween().bind_node(relation_label).set_loops()
			relation_pulse.tween_property(relation_label, "modulate:a", 0.45, 0.45)
			relation_pulse.tween_property(relation_label, "modulate:a", 1.0, 0.45)

	# --- Spacer ---
	var spacer1 = Control.new()
	spacer1.custom_minimum_size.y = 6
	vbox.add_child(spacer1)

	# --- POWER GAINED ---
	var skill = BattleManager.BURN_SKILLS.get(memory.grade, BattleManager.BURN_SKILLS[0])
	var eff_power = MemoryManager.get_effective_burn_power(memory)
	var total_dmg = skill.base_damage + eff_power
	# Apply bonuses (ember_affinity passive)
	if MemoryManager.has_passive("ember_affinity"):
		total_dmg = int(total_dmg * 1.1)

	var power_label = Label.new()
	power_label.text = _bl("POWER GAINED:  +%d damage  (%s)", "얻는 힘:  피해 +%d  (%s)") % [total_dmg, skill.name]
	power_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	power_label.add_theme_font_size_override("font_size", 14)
	power_label.add_theme_color_override("font_color", Color(0.3, 0.9, 0.4))
	vbox.add_child(power_label)

	var aftershock := BattleManager.get_burn_aftershock_preview(memory.grade)
	var aftershock_turns := int(aftershock.get("turns", 0))
	if aftershock_turns > 0:
		var aftershock_label = Label.new()
		aftershock_label.text = _bl(
			"AFTERSHOCK: Enemy intent hidden for %d response(s).",
			"후유증: 적의 의도가 %d회 동안 가려집니다."
		) % aftershock_turns
		if bool(aftershock.get("elia_anchored", false)):
			aftershock_label.text += _bl(" Elia anchors one layer.", " 엘리아가 한 겹을 붙듭니다.")
		aftershock_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
		aftershock_label.add_theme_font_size_override("font_size", 13)
		aftershock_label.add_theme_color_override("font_color", Color(1.0, 0.48, 0.34))
		vbox.add_child(aftershock_label)

	# Erosion warning if applicable
	if memory.erosion > 0:
		var erosion_pct = int(MemoryManager.get_erosion_ratio(memory) * 100)
		var erosion_label = Label.new()
		erosion_label.text = _bl("Eroded: %d%%, effective power reduced", "침식: %d%%, 유효 위력 감소") % erosion_pct
		erosion_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
		erosion_label.add_theme_font_size_override("font_size", 13)
		erosion_label.add_theme_color_override("font_color", Color(0.94, 0.72, 0.46))
		vbox.add_child(erosion_label)

	# --- COST: What you lose ---
	var spacer2 = Control.new()
	spacer2.custom_minimum_size.y = 4
	vbox.add_child(spacer2)

	var cost_header = Label.new()
	cost_header.text = _bl("COST, LOST FOREVER: %s", "대가, 영구 소실: %s") % MemoryManager.localized_memory_title(memory)
	cost_header.add_theme_font_size_override("font_size", 13)
	cost_header.add_theme_color_override("font_color", Color(0.85, 0.35, 0.3))
	vbox.add_child(cost_header)

	var desc_label = Label.new()
	desc_label.text = MemoryManager.localized_memory_description(memory)
	desc_label.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	desc_label.max_lines_visible = 3
	desc_label.text_overrun_behavior = TextServer.OVERRUN_TRIM_ELLIPSIS
	desc_label.add_theme_font_size_override("font_size", 14)
	desc_label.add_theme_color_override("font_color", UITheme.TEXT_NARRATION)
	vbox.add_child(desc_label)

	# --- Where the world changes: the authored rewrite, not a generic promise ---
	var rewrite_report := WorldRewriteDirector.get_rewrite_report(memory.id)
	var rewrite_line := String(rewrite_report.get("line", ""))
	if rewrite_line != "":
		var effect_label = Label.new()
		effect_label.text = _bl("WORLD: %s", "세계: %s") % rewrite_line
		effect_label.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
		effect_label.max_lines_visible = 2
		effect_label.text_overrun_behavior = TextServer.OVERRUN_TRIM_ELLIPSIS
		effect_label.add_theme_font_size_override("font_size", 13)
		effect_label.add_theme_color_override("font_color", Color(0.84, 0.72, 0.96))
		vbox.add_child(effect_label)

	# --- Elia does not stop him. She marks the moment. ---
	if memory.related_npc == "Elia" and GameManager.player_data.get("elia_with_party", false) \
			and BattleManager.is_significant_memory(memory):
		var elia_label = Label.new()
		elia_label.text = _bl(
			"ELIA: I know you can. I am asking whether you want to.",
			"엘리아: 할 수 있다는 건 알아. 하고 싶은지를 묻는 거야."
		)
		elia_label.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
		elia_label.max_lines_visible = 2
		elia_label.add_theme_font_size_override("font_size", 13)
		elia_label.add_theme_color_override("font_color", Color(0.72, 0.86, 1.0))
		vbox.add_child(elia_label)

	# --- S233: 연쇄 예보. 무엇이 함께 상하는지 커밋 전에 이름으로 말한다. ---
	# 숨은 대가를 만들지 않는 것이 이 화면의 원칙이다 (S226).
	var cascade: Dictionary = MemoryManager.cascade_preview(memory)
	if int(cascade.get("count", 0)) > 0:
		var linked: Array = cascade.get("titles", [])
		var shown_links: Array = linked.slice(0, 3)
		var suffix := ""
		if linked.size() > shown_links.size():
			suffix = _bl(" and %d more", " 외 %d개") % (linked.size() - shown_links.size())
		var cascade_label = Label.new()
		# 한국어는 조사가 앞 글자에 따라 바뀐다. "%s이(가)" 같은 표기를 피하려고
		# 수치를 앞에 두고 이름을 뒤에 붙인다.
		var linked_names := ", ".join(shown_links) + suffix
		var erosion_amount := int(cascade.get("amount", 0))
		if GameManager.current_locale == "ko":
			cascade_label.text = "연쇄: 침식 %d · %s" % [erosion_amount, linked_names]
		else:
			cascade_label.text = "LINKED: %s take %d erosion" % [linked_names, erosion_amount]
		cascade_label.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
		cascade_label.max_lines_visible = 2
		cascade_label.text_overrun_behavior = TextServer.OVERRUN_TRIM_ELLIPSIS
		UITheme.style_meta_label(cascade_label, Color(0.96, 0.74, 0.44))
		vbox.add_child(cascade_label)
	var guarded_links: Array = cascade.get("guarded", [])
	if not guarded_links.is_empty():
		var guarded_label = Label.new()
		guarded_label.text = _bl(
			"ANCHORED: %s will hold, spending its guard.",
			"고정됨: 보호를 써서 버팁니다 · %s"
		) % ", ".join(guarded_links.slice(0, 3))
		guarded_label.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
		guarded_label.max_lines_visible = 2
		UITheme.style_meta_label(guarded_label, Color(0.66, 0.86, 0.98))
		vbox.add_child(guarded_label)

	# --- Irreversibility is stated every time, not only for high grades ---
	var restore_label = Label.new()
	restore_label.text = _bl("This cannot be restored.", "이것은 복구할 수 없습니다.")
	restore_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	restore_label.add_theme_font_size_override("font_size", 13)
	restore_label.add_theme_color_override("font_color", Color(0.86, 0.52, 0.42))
	vbox.add_child(restore_label)

	# --- Irreplaceable warning for Grade 1-2 (high value memories) ---
	if memory.grade >= MemoryManager.MemoryGrade.GRADE_2:  # Grade 2 or Grade 1
		var warn_label = Label.new()
		warn_label.text = _bl("WARNING: This memory is irreplaceable", "경고: 이 기억은 대체할 수 없습니다")
		warn_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
		warn_label.add_theme_font_size_override("font_size", 13)
		warn_label.add_theme_color_override("font_color", Color(0.95, 0.25, 0.2))
		vbox.add_child(warn_label)

		# Pulse animation on the warning
		var pulse_tween = create_tween().bind_node(warn_label).set_loops()
		pulse_tween.tween_property(warn_label, "modulate:a", 0.5, 0.6)
		pulse_tween.tween_property(warn_label, "modulate:a", 1.0, 0.6)

	# --- Spacer before buttons ---
	var spacer3 = Control.new()
	spacer3.custom_minimum_size.y = 10
	vbox.add_child(spacer3)

	# --- Buttons: Burn (red, pulsing) + Cancel (gray) ---
	var btn_row = HBoxContainer.new()
	btn_row.alignment = BoxContainer.ALIGNMENT_CENTER
	btn_row.add_theme_constant_override("separation", 30)
	vbox.add_child(btn_row)

	# Burn button
	_burn_preview_confirm_btn = Button.new()
	_burn_preview_confirm_btn.text = _bl("BURN", "연소")
	_burn_preview_confirm_btn.custom_minimum_size = Vector2(140, 44)
	_burn_preview_confirm_btn.disabled = true  # Enabled after 0.5s delay

	var burn_style = StyleBoxFlat.new()
	burn_style.bg_color = Color(0.5, 0.12, 0.08, 0.95)
	burn_style.border_color = Color(0.85, 0.3, 0.15, 0.9)
	burn_style.set_border_width_all(2)
	burn_style.set_corner_radius_all(4)
	burn_style.set_content_margin_all(8)
	_burn_preview_confirm_btn.add_theme_stylebox_override("normal", burn_style)
	var burn_hover = burn_style.duplicate()
	burn_hover.bg_color = Color(0.65, 0.15, 0.08, 1.0)
	burn_hover.border_color = Color(1.0, 0.45, 0.2, 1.0)
	_burn_preview_confirm_btn.add_theme_stylebox_override("hover", burn_hover)
	_burn_preview_confirm_btn.add_theme_stylebox_override("focus", burn_hover)
	var burn_disabled = burn_style.duplicate()
	burn_disabled.bg_color = Color(0.2, 0.1, 0.08, 0.6)
	burn_disabled.border_color = Color(0.4, 0.2, 0.15, 0.4)
	_burn_preview_confirm_btn.add_theme_stylebox_override("disabled", burn_disabled)
	_burn_preview_confirm_btn.add_theme_font_size_override("font_size", 16)
	_burn_preview_confirm_btn.add_theme_color_override("font_color", Color(0.95, 0.55, 0.25))
	_burn_preview_confirm_btn.add_theme_color_override("font_hover_color", Color(1.0, 0.75, 0.35))
	_burn_preview_confirm_btn.add_theme_color_override("font_disabled_color", Color(0.4, 0.3, 0.25))

	_burn_preview_confirm_btn.pressed.connect(_on_burn_preview_confirmed)
	btn_row.add_child(_burn_preview_confirm_btn)

	# Cancel button
	_burn_preview_cancel_btn = Button.new()
	_burn_preview_cancel_btn.text = _bl("Cancel", "취소")
	_burn_preview_cancel_btn.custom_minimum_size = Vector2(120, 44)

	var cancel_style = StyleBoxFlat.new()
	cancel_style.bg_color = Color(0.12, 0.11, 0.13, 0.9)
	cancel_style.border_color = Color(0.35, 0.33, 0.3, 0.7)
	cancel_style.set_border_width_all(1)
	cancel_style.set_corner_radius_all(4)
	cancel_style.set_content_margin_all(8)
	_burn_preview_cancel_btn.add_theme_stylebox_override("normal", cancel_style)
	var cancel_hover = cancel_style.duplicate()
	cancel_hover.bg_color = Color(0.18, 0.16, 0.2, 0.95)
	cancel_hover.border_color = Color(0.5, 0.45, 0.4, 0.9)
	_burn_preview_cancel_btn.add_theme_stylebox_override("hover", cancel_hover)
	_burn_preview_cancel_btn.add_theme_stylebox_override("focus", cancel_hover)
	_burn_preview_cancel_btn.add_theme_font_size_override("font_size", 14)
	_burn_preview_cancel_btn.add_theme_color_override("font_color", Color(0.55, 0.5, 0.45))
	_burn_preview_cancel_btn.add_theme_color_override("font_hover_color", Color(0.75, 0.7, 0.65))

	_burn_preview_cancel_btn.pressed.connect(_on_burn_preview_cancelled)
	btn_row.add_child(_burn_preview_cancel_btn)

	# Show the popup with animation
	_burn_preview_dimmer.visible = true
	if _burn_preview_art:
		_burn_preview_art.visible = true
		_burn_preview_art.modulate.a = 0.0
		_burn_preview_art.scale = Vector2(0.85, 0.85)
	_burn_preview_panel.visible = true
	_burn_preview_panel.modulate.a = 0.0
	_burn_preview_panel.scale = Vector2(0.85, 0.85)
	_sync_burn_preview_frame()

	_burn_preview_transition_tween = create_tween().bind_node(_burn_preview_panel)
	_burn_preview_transition_tween.set_parallel(true)
	_burn_preview_transition_tween.tween_property(_burn_preview_dimmer, "color:a", 0.6, 0.2)
	if _burn_preview_art:
		_burn_preview_transition_tween.tween_property(_burn_preview_art, "modulate:a", 0.86, 0.25)
		_burn_preview_transition_tween.tween_property(_burn_preview_art, "scale", Vector2(1.0, 1.0), 0.3).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_BACK)
	_burn_preview_transition_tween.tween_property(_burn_preview_panel, "modulate:a", 1.0, 0.25)
	_burn_preview_transition_tween.tween_property(_burn_preview_panel, "scale", Vector2(1.0, 1.0), 0.3).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_BACK)

	# The looping pulse belongs to the current button. Rebuilding the preview
	# kills it automatically instead of leaving a loop targeting a queued child.
	_burn_preview_pulse_tween = create_tween().bind_node(_burn_preview_confirm_btn).set_loops()
	_burn_preview_pulse_tween.tween_property(_burn_preview_confirm_btn, "modulate:a", 0.7, 0.5)
	_burn_preview_pulse_tween.tween_property(_burn_preview_confirm_btn, "modulate:a", 1.0, 0.5)

	# 0.5s delay before Burn button becomes clickable (prevent accidental burns)
	_burn_preview_confirm_btn.disabled = true
	var preview_generation := _burn_preview_generation
	var guarded_button := _burn_preview_confirm_btn
	_burn_preview_timer = BattleManager.pace_timer(0.5)
	_burn_preview_timer.timeout.connect(func():
		if preview_generation == _burn_preview_generation \
				and guarded_button == _burn_preview_confirm_btn \
				and is_instance_valid(guarded_button) \
				and _burn_preview_panel.visible:
			guarded_button.disabled = false
			AudioManager.play_sfx("ui_hover")  # subtle audio cue that button is now active
	)

## Called when player confirms the burn in the preview popup.
func _on_burn_preview_confirmed() -> void:
	AudioManager.play_sfx("ui_select")
	var mid = _pending_burn_id
	_pending_burn_id = ""
	_hide_burn_preview()

	# Find the memory to get title and grade for the dramatic sequence
	var mem = MemoryManager._get_memory(mid)
	if mem:
		action_container.visible = false
		_play_memory_burn_then_execute(mid, MemoryManager.localized_memory_title(mem), mem.grade)
	else:
		# Fallback: memory already gone somehow
		action_container.visible = false
		BattleManager.player_burn(mid)

## Called when player cancels the burn preview.
func _on_burn_preview_cancelled() -> void:
	AudioManager.play_sfx("cancel")
	_pending_burn_id = ""
	_hide_burn_preview()
	# Reopen the burn list so player can pick something else
	_toggle_burn_list()

## Hide the burn preview popup with animation.
func _hide_burn_preview() -> void:
	if _burn_preview_panel == null or _burn_preview_dimmer == null:
		return
	_stop_burn_preview_motion()
	_burn_preview_transition_tween = create_tween().bind_node(_burn_preview_panel)
	_burn_preview_transition_tween.set_parallel(true)
	_burn_preview_transition_tween.tween_property(_burn_preview_dimmer, "color:a", 0.0, 0.15)
	if _burn_preview_art:
		_burn_preview_transition_tween.tween_property(_burn_preview_art, "modulate:a", 0.0, 0.15)
	_burn_preview_transition_tween.tween_property(_burn_preview_panel, "modulate:a", 0.0, 0.15)
	var hide_generation := _burn_preview_generation
	_burn_preview_transition_tween.chain().tween_callback(func():
		if hide_generation != _burn_preview_generation:
			return
		if is_instance_valid(_burn_preview_dimmer):
			_burn_preview_dimmer.visible = false
		if _burn_preview_art and is_instance_valid(_burn_preview_art):
			_burn_preview_art.visible = false
		if is_instance_valid(_burn_preview_panel):
			_burn_preview_panel.visible = false
	)

## S227: Every dynamic preview animation has one explicit owner and lifetime.
## This also invalidates delayed unlock callbacks from an older selection.
func _stop_burn_preview_motion() -> void:
	_burn_preview_generation += 1
	if _burn_preview_transition_tween != null and _burn_preview_transition_tween.is_valid():
		_burn_preview_transition_tween.kill()
	_burn_preview_transition_tween = null
	if _burn_preview_pulse_tween != null and _burn_preview_pulse_tween.is_valid():
		_burn_preview_pulse_tween.kill()
	_burn_preview_pulse_tween = null
	_burn_preview_timer = null

## ===================== S56: Memory Burn Dramatic Sequence =====================

## Play dramatic burn sequence then execute the actual burn
func _play_memory_burn_then_execute(memory_id: String, memory_title: String, memory_grade: int) -> void:
	var cutin_path := _resolve_memory_burn_cutin(memory_id, memory_title)
	# S213: 되돌릴 수 없는 대가를 치르는 순간, 무대 자체가 반응한다.
	if _hybrid_depth_stage != null and is_instance_valid(_hybrid_depth_stage):
		_hybrid_depth_stage.play_memory_burn(memory_grade)
	if cutin_path != "":
		await _play_memory_burn_cutin(cutin_path, memory_grade)
	if battle_vfx:
		await battle_vfx.play_memory_burn_sequence(memory_title, memory_grade, player_sprite_container)
	# MemoryManager.memory_burned hands the single afterglow to
	# WorldRewriteDirector. Battle and field no longer paint competing washes.
	BattleManager.player_burn(memory_id)

## ===================== S56: Skill Element Detection Helper =====================

## Detect element type from skill name for particle effects
func _detect_skill_element(skill_name: String) -> String:
	var sn = skill_name.to_lower()
	if sn.find("void") >= 0 or sn.find("cascade") >= 0 or sn.find("zero") >= 0 or sn.find("identity") >= 0:
		return "void"
	elif sn.find("burn") >= 0 or sn.find("flame") >= 0 or sn.find("ember") >= 0 or sn.find("fire") >= 0 or sn.find("scorch") >= 0 or sn.find("incinerate") >= 0 or sn.find("pyre") >= 0:
		return "fire"
	else:
		return "physical"

## ===================== S57: Battle Juice Overhaul =====================

## Screen flash for big hits, white at 80+, red at 150+
func _play_big_hit_screen_flash(amount: int) -> void:
	var flash = ColorRect.new()
	flash.set_anchors_preset(Control.PRESET_FULL_RECT)
	flash.mouse_filter = Control.MOUSE_FILTER_IGNORE
	flash.z_index = 95
	if amount >= 150:
		# Red flash for massive damage
		flash.color = Color(0.9, 0.1, 0.05, 0.45)
	else:
		# White flash for big damage (80+)
		flash.color = Color(1.0, 1.0, 0.95, 0.35)
	canvas_root.add_child(flash)
	var t = create_tween()
	t.tween_property(flash, "color:a", 0.0, 0.1).set_ease(Tween.EASE_OUT)
	t.tween_callback(flash.queue_free)

## Turn transition dim effect, brief darkening between turns (0.2s)
func _play_turn_dim() -> void:
	if not _turn_dim_overlay:
		return
	_turn_dim_overlay.color = Color(0, 0, 0, 0)
	var t = create_tween()
	t.tween_property(_turn_dim_overlay, "color:a", 0.3, 0.1).set_ease(Tween.EASE_IN)
	t.tween_property(_turn_dim_overlay, "color:a", 0.0, 0.1).set_ease(Tween.EASE_OUT)

## Combo counter display, escalating size/color (gold at x3, red at x5+)
func _show_combo_counter(combo: int) -> void:
	if not _combo_display_label:
		return
	_combo_display_label.text = "COMBO x%d!" % combo
	# Escalating font size and color
	var font_size = 24 + combo * 3
	if combo >= 5:
		_combo_display_label.add_theme_font_size_override("font_size", mini(font_size, 42))
		_combo_display_label.add_theme_color_override("font_color", Color(1.0, 0.2, 0.15))
	elif combo >= 3:
		_combo_display_label.add_theme_font_size_override("font_size", font_size)
		_combo_display_label.add_theme_color_override("font_color", Color(1.0, 0.85, 0.2))
	else:
		_combo_display_label.add_theme_font_size_override("font_size", font_size)
		_combo_display_label.add_theme_color_override("font_color", Color(0.85, 0.75, 0.5))
	_combo_display_label.modulate.a = 0.0
	_combo_display_label.scale = Vector2(1.5, 1.5)
	_combo_display_label.pivot_offset = Vector2(120, 20)
	var t = create_tween()
	t.tween_property(_combo_display_label, "modulate:a", 1.0, 0.1)
	t.tween_property(_combo_display_label, "scale", Vector2(1.0, 1.0), 0.15).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_BACK)
	t.tween_interval(0.6)
	t.tween_property(_combo_display_label, "modulate:a", 0.0, 0.3)

## Enemy death animation, fade + shrink + particle burst before dissolve
func _play_enemy_death_animation() -> void:
	if not enemy_sprite_container:
		return
	# Particle burst from enemy position
	var center = enemy_sprite_container.position + Vector2(130, 120)
	for i in range(20):
		var particle = ColorRect.new()
		var s = randf_range(3, 8)
		particle.size = Vector2(s, s)
		particle.position = center + Vector2(randf_range(-30, 30), randf_range(-30, 30))
		particle.z_index = 65
		particle.mouse_filter = Control.MOUSE_FILTER_IGNORE
		# Mix of enemy-colored particles (purple/dark/red)
		var colors = [
			Color(0.6, 0.2, 0.8, 0.9), Color(0.8, 0.15, 0.2, 0.85),
			Color(0.3, 0.1, 0.5, 0.8), Color(0.9, 0.7, 0.2, 0.7),
		]
		particle.color = colors[randi_range(0, colors.size() - 1)]
		canvas_root.add_child(particle)
		var angle = randf() * TAU
		var dist = randf_range(60, 160)
		var target_pos = particle.position + Vector2(cos(angle), sin(angle)) * dist
		var delay = randf_range(0, 0.15)
		var pt = create_tween().set_parallel(true)
		pt.tween_property(particle, "position", target_pos, randf_range(0.4, 0.8)).set_delay(delay).set_ease(Tween.EASE_OUT)
		pt.tween_property(particle, "modulate:a", 0.0, 0.3).set_delay(delay + 0.2)
		pt.tween_property(particle, "size", Vector2(1, 1), 0.6).set_delay(delay)
		pt.chain().tween_callback(particle.queue_free)
	# Fade out + shrink the enemy sprite container
	var dt = create_tween().set_parallel(true)
	dt.tween_property(enemy_sprite_container, "modulate:a", 0.0, 0.6).set_ease(Tween.EASE_IN)
	dt.tween_property(enemy_sprite_container, "scale", Vector2(0.3, 0.3), 0.6).set_ease(Tween.EASE_IN).set_trans(Tween.TRANS_CUBIC)
	# Pivot from center
	enemy_sprite_container.pivot_offset = enemy_sprite_container.size * 0.5
