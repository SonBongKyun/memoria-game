shader_type canvas_item;

// A restrained field-character finish:
// - a dark two-source-pixel silhouette keeps pale character art readable
//   against both forest paths and charcoal interiors;
// - a very small upper-edge accent carries each character's Memory color
//   without turning the cast into neon game pieces.
uniform vec4 outline_color : source_color = vec4(0.025, 0.035, 0.060, 0.92);
uniform vec4 accent_color : source_color = vec4(0.35, 0.58, 0.90, 1.0);
uniform float outline_px : hint_range(0.0, 3.0, 0.1) = 1.8;
uniform float outline_strength : hint_range(0.0, 1.0, 0.01) = 0.72;
uniform float accent_strength : hint_range(0.0, 0.5, 0.01) = 0.11;
uniform float grade_strength : hint_range(0.0, 0.4, 0.01) = 0.10;

// S237: 바깥은 어둡게, 안쪽은 밝게.
// 외곽선 하나만 쓰면 그 색과 비슷한 배경에서 캐릭터가 사라진다.
// 어두운 지형이 대부분인 이 게임에서 검은 테두리는 밝은 숲길에서만 듣는다.
uniform vec4 rim_color : source_color = vec4(0.82, 0.88, 0.98, 1.0);
uniform float rim_strength : hint_range(0.0, 1.0, 0.01) = 0.42;

void fragment() {
	vec4 base = texture(TEXTURE, UV);
	// In a CanvasItem fragment shader COLOR already contains the sampled
	// texture.  Multiplying by it again squares dark values and crushes the
	// character palette.  Only recover the authored modulate alpha here.
	float modulate_alpha = base.a > 0.001 ? clamp(COLOR.a / base.a, 0.0, 1.0) : 1.0;
	vec2 px = TEXTURE_PIXEL_SIZE * outline_px;

	float around = 0.0;
	around = max(around, texture(TEXTURE, UV + vec2(px.x, 0.0)).a);
	around = max(around, texture(TEXTURE, UV - vec2(px.x, 0.0)).a);
	around = max(around, texture(TEXTURE, UV + vec2(0.0, px.y)).a);
	around = max(around, texture(TEXTURE, UV - vec2(0.0, px.y)).a);
	around = max(around, texture(TEXTURE, UV + vec2(px.x, px.y)).a);
	around = max(around, texture(TEXTURE, UV + vec2(-px.x, px.y)).a);
	around = max(around, texture(TEXTURE, UV + vec2(px.x, -px.y)).a);
	around = max(around, texture(TEXTURE, UV - px).a);

	float outer_edge = max(around - base.a, 0.0);

	// 안쪽 가장자리: 이웃 중 가장 투명한 쪽이 비어 있을수록 이 픽셀은 경계에 가깝다.
	float inner_neighbour = 1.0;
	inner_neighbour = min(inner_neighbour, texture(TEXTURE, UV + vec2(px.x, 0.0)).a);
	inner_neighbour = min(inner_neighbour, texture(TEXTURE, UV - vec2(px.x, 0.0)).a);
	inner_neighbour = min(inner_neighbour, texture(TEXTURE, UV + vec2(0.0, px.y)).a);
	inner_neighbour = min(inner_neighbour, texture(TEXTURE, UV - vec2(0.0, px.y)).a);
	float inner_edge = base.a * (1.0 - inner_neighbour);
	float upper_alpha = texture(TEXTURE, UV - vec2(0.0, px.y)).a;
	float lower_alpha = texture(TEXTURE, UV + vec2(0.0, px.y)).a;
	float upper_rim = clamp((base.a - upper_alpha) + (upper_alpha - lower_alpha) * 0.35, 0.0, 1.0) * base.a;

	vec3 cool_grade = base.rgb * vec3(0.91, 0.945, 1.0);
	vec3 finished_rgb = mix(base.rgb, cool_grade, grade_strength);
	finished_rgb = mix(finished_rgb, accent_color.rgb, upper_rim * accent_strength);
	finished_rgb = mix(finished_rgb, rim_color.rgb, inner_edge * rim_strength);

	float outline_alpha = outer_edge * outline_strength * outline_color.a;
	vec4 finished = vec4(finished_rgb, base.a);
	finished.rgb = mix(finished.rgb, outline_color.rgb, outline_alpha);
	finished.a = max(base.a, outline_alpha);
	finished.a *= modulate_alpha;
	COLOR = finished;
}
