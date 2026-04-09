shader_type canvas_item;

// Subtle heat haze / atmospheric distortion for outdoor maps
uniform sampler2D screen_texture : hint_screen_texture, repeat_disable, filter_linear;
uniform float distortion_strength : hint_range(0.0, 0.02) = 0.003;
uniform float wave_speed : hint_range(0.0, 5.0) = 1.5;
uniform float wave_frequency : hint_range(1.0, 20.0) = 8.0;

void fragment() {
	vec2 uv = SCREEN_UV;
	float wave = sin(uv.y * wave_frequency + TIME * wave_speed) * distortion_strength;
	wave += sin(uv.y * wave_frequency * 0.7 + TIME * wave_speed * 1.3) * distortion_strength * 0.5;
	vec4 tex = texture(screen_texture, uv + vec2(wave, 0.0));
	COLOR = tex;
}
