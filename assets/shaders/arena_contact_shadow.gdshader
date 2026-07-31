shader_type spatial;
render_mode blend_mix, cull_disabled, unshaded, depth_draw_never;

// S212: 전투원이 3D 바닥에 닿는 지점의 그림자.
// 2D 타원(Polygon2D)은 화면 좌표에 고정돼 있어서, 카메라가 패닝하면 캐릭터와
// 그림자가 통째로 미끄러졌다. 바닥면에 눕힌 이 그림자는 원근을 따라 눌리고
// 무대와 함께 움직이므로 "저 자리에 서 있다"가 읽힌다.

uniform float shadow_strength : hint_range(0.0, 1.0) = 0.42;
uniform float core_ratio : hint_range(0.0, 1.0) = 0.35;

void fragment() {
	// UV 중심에서의 거리로 원형 감쇠를 만든다.
	float d = length(UV - vec2(0.5)) * 2.0;
	// 안쪽은 고르게 진하고 바깥쪽만 부드럽게 풀린다.
	float falloff = 1.0 - smoothstep(core_ratio, 1.0, d);
	ALBEDO = vec3(0.0);
	ALPHA = clamp(falloff * falloff * shadow_strength, 0.0, 1.0);
}
