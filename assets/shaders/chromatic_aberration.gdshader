shader_type canvas_item;

// Chromatic aberration — Limit Break / 보이드 충격 연출
// hint_screen_texture로 실제 화면을 샘플링
uniform sampler2D screen_texture : hint_screen_texture, filter_linear_mipmap;
uniform float strength : hint_range(0.0, 0.02) = 0.005;
uniform float pulse_speed : hint_range(0.0, 10.0) = 3.0;
uniform bool use_pulse = true;

void fragment() {
	float s = strength;
	if (use_pulse) {
		s *= 0.5 + 0.5 * sin(TIME * pulse_speed);
	}

	vec2 dir = SCREEN_UV - vec2(0.5);
	float dist = length(dir);
	vec2 offset = dir * s * dist;

	float r = texture(screen_texture, SCREEN_UV + offset).r;
	float g = texture(screen_texture, SCREEN_UV).g;
	float b = texture(screen_texture, SCREEN_UV - offset).b;

	COLOR = vec4(r, g, b, 1.0);
}
