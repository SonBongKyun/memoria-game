shader_type canvas_item;

// Glow pulse shader — 랜턴, 크리스탈, 기억 파편 등의 맥동 글로우 (알파 출력)
uniform vec4 glow_color : source_color = vec4(0.9, 0.7, 0.3, 0.6);
uniform float pulse_speed : hint_range(0.5, 5.0) = 2.0;
uniform float min_intensity : hint_range(0.0, 1.0) = 0.3;
uniform float max_intensity : hint_range(0.0, 1.0) = 0.8;
uniform float glow_radius : hint_range(0.0, 0.5) = 0.3;

void fragment() {
	vec2 center = UV - vec2(0.5);
	float dist = length(center) * 2.0;

	// 원형 글로우 감쇠
	float glow = 1.0 - smoothstep(0.0, glow_radius * 2.0, dist);

	// 맥동
	float pulse = mix(min_intensity, max_intensity, 0.5 + 0.5 * sin(TIME * pulse_speed));

	// 알파 기반 투명 글로우 — 중앙 밝고 가장자리 투명
	float alpha = glow * pulse * glow_color.a;
	COLOR = vec4(glow_color.rgb, alpha);
}
