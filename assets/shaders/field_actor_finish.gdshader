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
	float upper_alpha = texture(TEXTURE, UV - vec2(0.0, px.y)).a;
	float lower_alpha = texture(TEXTURE, UV + vec2(0.0, px.y)).a;
	float upper_rim = clamp((base.a - upper_alpha) + (upper_alpha - lower_alpha) * 0.35, 0.0, 1.0) * base.a;

	vec3 cool_grade = base.rgb * vec3(0.91, 0.945, 1.0);
	vec3 finished_rgb = mix(base.rgb, cool_grade, grade_strength);
	finished_rgb = mix(finished_rgb, accent_color.rgb, upper_rim * accent_strength);

	float outline_alpha = outer_edge * outline_strength * outline_color.a;
	vec4 finished = vec4(finished_rgb, base.a);
	finished.rgb = mix(finished.rgb, outline_color.rgb, outline_alpha);
	finished.a = max(base.a, outline_alpha);
	finished.a *= modulate_alpha;
	COLOR = finished;
}
