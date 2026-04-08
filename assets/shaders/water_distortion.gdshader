shader_type canvas_item;

// Water distortion shader — 물 타일 위 웨이브 왜곡 + 반짝임
uniform float wave_speed : hint_range(0.5, 5.0) = 1.5;
uniform float wave_strength : hint_range(0.001, 0.02) = 0.006;
uniform float wave_frequency : hint_range(5.0, 40.0) = 18.0;
uniform float shimmer_intensity : hint_range(0.0, 0.4) = 0.15;
uniform vec4 tint_color : source_color = vec4(0.3, 0.5, 0.8, 0.25);

void fragment() {
	vec2 uv = UV;

	// 이중 사인파 왜곡
	float t = TIME * wave_speed;
	uv.x += sin(uv.y * wave_frequency + t) * wave_strength;
	uv.y += cos(uv.x * wave_frequency * 0.8 + t * 1.3) * wave_strength * 0.7;

	vec4 col = texture(TEXTURE, uv);

	// 수면 반짝임 (하이라이트)
	float shimmer = sin(uv.x * 30.0 + t * 2.0) * cos(uv.y * 25.0 + t * 1.7);
	shimmer = max(0.0, shimmer);
	shimmer = pow(shimmer, 3.0) * shimmer_intensity;

	col.rgb += vec3(shimmer);
	col = mix(col, tint_color, tint_color.a);
	COLOR = col;
}
