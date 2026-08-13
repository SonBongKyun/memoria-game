extends Node

## S243: VN CG 위의 `_cg_detail_top`이 만드는 가로 단차를 잰다.
##
## 콘택트 시트 여러 칸에서 CG 위에 네 변이 뚜렷한 밝은 직사각형 띠가 보였다.
## 런타임 사각형을 찍어 보니 `_cg_detail_top`이 (0,0)-(1280,287), 알파 0.145,
## 색 (0.88, 0.80, 0.68)이었다. 코드를 보면 이 노드는 **CG 자신의 텍스처를**
## 상단 287px 사각형에 KEEP_ASPECT_COVERED로 한 번 더 그린다. 아래 CG와
## 크롭·배율이 다르므로 어긋난 겹상이 되고, y=287에서 뚝 끊긴다.
##
## 눈으로 본 것을 숫자로 바꾼다. 띠 안쪽(y 200~280)과 바로 아래(y 300~380)의
## 행 평균 밝기 차이를 잰다. 레이어를 끈 화면과 짝지어 비교하고, 같은 조건
## 두 장으로 잡음 바닥도 함께 잰다.
##
## 알파가 5초 주기로 0.10↔0.18 맥동하므로 먼저 트윈을 죽이고 값을 고정한다.
## 그러지 않으면 두 캡처의 차이가 효과인지 맥동인지 가려지지 않는다.

const SCENES: Array[String] = [
	"ch13_third_person", "ch12_reader", "ch17_forgetting_storm",
	"ch20_monolith", "ch22_core", "ch2_market_arrival",
]
const BAND_TOP := 200
const BAND_BOTTOM := 280
const BELOW_TOP := 300
const BELOW_BOTTOM := 380
const FROZEN_ALPHA := 0.145

func _ready() -> void:
	Codex.suppress_recording = true
	OptionsMenu.settings["reduce_motion"] = true
	GameManager.current_locale = "ko"
	await get_tree().process_frame

	for scene_id in SCENES:
		await _measure(scene_id)
	print("VN_DETAIL_SEAM_DONE")
	get_tree().quit(0)

func _measure(scene_id: String) -> void:
	SceneFlow.play(scene_id, 0)
	await get_tree().create_timer(0.5).timeout
	for i in range(2):
		SceneFlow.advance()
		await get_tree().create_timer(0.2).timeout
	await get_tree().create_timer(1.6).timeout

	var ui: Node = SceneFlow.get("_vn_ui")
	var detail := ui.get("_cg_detail_top") as TextureRect if ui != null else null
	if detail == null:
		print("VN_DETAIL_SEAM %-22s <no detail layer>" % scene_id)
		return
	# 맥동 트윈을 죽여야 두 캡처가 같은 조건이 된다.
	if detail.has_meta("detail_tween"):
		var prev = detail.get_meta("detail_tween")
		if prev != null and prev is Tween and prev.is_valid():
			prev.kill()

	detail.modulate.a = FROZEN_ALPHA
	await get_tree().process_frame
	var on_a := _seam(await _grab())
	detail.modulate.a = 0.0
	await get_tree().process_frame
	var off := _seam(await _grab())
	detail.modulate.a = FROZEN_ALPHA
	await get_tree().process_frame
	var on_b := _seam(await _grab())

	# 대조군: 켠 화면 두 장 사이의 편차. 이 장면에서 계측기가 갖는 잡음 바닥이다.
	var on_mean := (on_a + on_b) * 0.5
	print("VN_DETAIL_SEAM %-22s on=%.4f off=%.4f delta=%.4f null=%.4f" % [
		scene_id, on_mean, off, on_mean - off, absf(on_a - on_b)])

	SceneFlow.call("_end_scene")
	await get_tree().create_timer(0.4).timeout

func _grab() -> Image:
	await RenderingServer.frame_post_draw
	return get_viewport().get_texture().get_image()

## 띠 안쪽과 바로 아래의 행 평균 밝기 차이.
func _seam(image: Image) -> float:
	return _band_luma(image, BAND_TOP, BAND_BOTTOM) - _band_luma(image, BELOW_TOP, BELOW_BOTTOM)

func _band_luma(image: Image, top: int, bottom: int) -> float:
	var total := 0.0
	var count := 0
	for y in range(top, bottom, 2):
		for x in range(0, image.get_width(), 4):
			var c := image.get_pixel(x, y)
			total += 0.2126 * c.r + 0.7152 * c.g + 0.0722 * c.b
			count += 1
	return total / maxf(float(count), 1.0)
