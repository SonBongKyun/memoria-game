shader_type canvas_item;

// Water distortion shader — 물 타일 위 웨이브 + 반짝임 (알파 오버레이)
uniform float wave_speed : hint_range(0.5, 5.0) = 1.5;
uniform float wave_strength : hint_range(0.001, 0.05) = 0.015;
uniform float wave_frequency : hint_range(5.0, 40.0) = 18.0;
uniform float shimmer_intensity : hint_range(0.0, 0.4) = 0.15;
uniform vec4 tint_color : source_color = vec4(0.3, 0.5, 0.8, 0.25);

void fragment() {
	float t = TIME * wave_speed;

	// 수면 반짝임 패턴
	float wave1 = sin(UV.x * wave_frequency + t) * cos(UV.y * wave_frequency * 0.7 + t * 0.8);
	float wave2 = sin(UV.x * wave_frequency * 1.3 + t * 1.5) * cos(UV.y * wave_frequency * 0.5 + t * 1.2);
	float shimmer = max(0.0, wave1 * 0.6 + wave2 * 0.4);
	shimmer = pow(shimmer, 2.5) * shimmer_intensity;

	// 물결 명암 패턴
	float dark_wave = sin(UV.y * wave_frequency * 0.5 + t * 0.6) * wave_strength;

	// 반투명 오버레이 출력 — 중심: 물 색상 틴트 + 반짝임
	float alpha = tint_color.a + shimmer + abs(dark_wave);
	vec3 col = tint_color.rgb + vec3(shimmer);
	COLOR = vec4(col, alpha);
}
