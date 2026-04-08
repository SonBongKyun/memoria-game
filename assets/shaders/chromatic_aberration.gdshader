shader_type canvas_item;

// Chromatic aberration — Limit Break / 보이드 충격 연출
uniform float strength : hint_range(0.0, 0.02) = 0.005;
uniform float pulse_speed : hint_range(0.0, 10.0) = 3.0;
uniform bool use_pulse = true;

void fragment() {
	float s = strength;
	if (use_pulse) {
		s *= 0.5 + 0.5 * sin(TIME * pulse_speed);
	}

	vec2 dir = UV - vec2(0.5);
	float dist = length(dir);
	vec2 offset = dir * s * dist;

	float r = texture(TEXTURE, UV + offset).r;
	float g = texture(TEXTURE, UV).g;
	float b = texture(TEXTURE, UV - offset).b;
	float a = texture(TEXTURE, UV).a;

	COLOR = vec4(r, g, b, a);
}
