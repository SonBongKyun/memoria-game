shader_type canvas_item;

// Memory burn desaturation — world loses color as memories are burned
uniform sampler2D screen_texture : hint_screen_texture, repeat_disable, filter_linear;
uniform float desaturation : hint_range(0.0, 1.0) = 0.0;
uniform vec4 tint_color : source_color = vec4(0.15, 0.12, 0.18, 1.0);
uniform float tint_strength : hint_range(0.0, 1.0) = 0.0;

void fragment() {
	vec4 tex = texture(screen_texture, SCREEN_UV);
	// Luminance-preserving grayscale
	float gray = dot(tex.rgb, vec3(0.299, 0.587, 0.114));
	vec3 desaturated = mix(tex.rgb, vec3(gray), desaturation);
	// Optional void tint at high burn counts
	vec3 tinted = mix(desaturated, tint_color.rgb, tint_strength);
	COLOR = vec4(tinted, tex.a);
}
