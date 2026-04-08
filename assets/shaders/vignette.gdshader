shader_type canvas_item;

// Radial vignette shader — 부드러운 원형 비네트
uniform float outer_radius : hint_range(0.3, 1.5) = 0.85;
uniform float inner_radius : hint_range(0.0, 1.0) = 0.35;
uniform float intensity : hint_range(0.0, 1.0) = 0.55;
uniform vec4 vignette_color : source_color = vec4(0.0, 0.0, 0.0, 1.0);

void fragment() {
	vec2 center = UV - vec2(0.5);
	float dist = length(center) * 2.0;

	// 부드러운 감쇠
	float vignette = smoothstep(inner_radius, outer_radius, dist);
	vignette *= intensity;

	vec4 col = texture(TEXTURE, UV);
	col.rgb = mix(col.rgb, vignette_color.rgb, vignette);
	COLOR = col;
}
