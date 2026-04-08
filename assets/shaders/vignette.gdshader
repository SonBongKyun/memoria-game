shader_type canvas_item;

// Radial vignette shader — 부드러운 원형 비네트 (알파 출력)
uniform float outer_radius : hint_range(0.3, 1.5) = 0.85;
uniform float inner_radius : hint_range(0.0, 1.0) = 0.35;
uniform float intensity : hint_range(0.0, 1.0) = 0.55;
uniform vec4 vignette_color : source_color = vec4(0.0, 0.0, 0.0, 1.0);

void fragment() {
	vec2 center = UV - vec2(0.5);
	float dist = length(center) * 2.0;

	// 부드러운 감쇠 — 중앙 투명, 가장자리 어둡게
	float vignette = smoothstep(inner_radius, outer_radius, dist);
	vignette *= intensity;

	COLOR = vec4(vignette_color.rgb, vignette);
}
