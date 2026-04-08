shader_type canvas_item;

// Dissolve shader — 적 사망 / 기억 연소 디졸브
uniform float progress : hint_range(0.0, 1.0) = 0.0;
uniform float edge_width : hint_range(0.01, 0.15) = 0.06;
uniform vec4 edge_color : source_color = vec4(1.0, 0.4, 0.1, 1.0);
uniform float noise_scale : hint_range(1.0, 20.0) = 8.0;

// 간단한 해시 노이즈
float hash(vec2 p) {
	return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float noise(vec2 p) {
	vec2 i = floor(p);
	vec2 f = fract(p);
	f = f * f * (3.0 - 2.0 * f);
	float a = hash(i);
	float b = hash(i + vec2(1.0, 0.0));
	float c = hash(i + vec2(0.0, 1.0));
	float d = hash(i + vec2(1.0, 1.0));
	return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

void fragment() {
	vec4 col = texture(TEXTURE, UV);
	float n = noise(UV * noise_scale);

	// 디졸브 진행
	float threshold = progress;
	if (n < threshold) {
		// 엣지 글로우
		if (n > threshold - edge_width) {
			float edge = 1.0 - (threshold - n) / edge_width;
			col.rgb = mix(col.rgb, edge_color.rgb, edge);
			col.a *= edge;
		} else {
			col.a = 0.0;
		}
	}

	COLOR = col;
}
