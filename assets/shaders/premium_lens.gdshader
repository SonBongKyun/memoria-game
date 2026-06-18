shader_type canvas_item;

uniform vec4 tint_color : source_color = vec4(0.72, 0.58, 0.36, 1.0);
uniform float vignette_strength : hint_range(0.0, 1.0) = 0.42;
uniform float tint_strength : hint_range(0.0, 0.45) = 0.10;
uniform float grain_strength : hint_range(0.0, 0.12) = 0.025;
uniform float letterbox_strength : hint_range(0.0, 0.8) = 0.22;
uniform float shaft_strength : hint_range(0.0, 0.35) = 0.08;
uniform float pulse_speed : hint_range(0.0, 3.0) = 0.35;

float hash21(vec2 p) {
	p = fract(p * vec2(123.34, 456.21));
	p += dot(p, p + 45.32);
	return fract(p.x * p.y);
}

void fragment() {
	vec2 centered = UV - vec2(0.5);
	float dist = length(centered) * 1.9;
	float vignette = smoothstep(0.36, 1.0, dist) * vignette_strength;

	float top_bar = smoothstep(0.23, 0.0, UV.y);
	float bottom_bar = smoothstep(0.77, 1.0, UV.y);
	float bars = max(top_bar, bottom_bar) * letterbox_strength;

	float diagonal = smoothstep(0.04, 0.0, abs(UV.x + UV.y * 0.42 - 0.72));
	float slow_pulse = 0.75 + 0.25 * sin(TIME * pulse_speed);
	float shaft = diagonal * shaft_strength * slow_pulse;

	float n = hash21(floor(UV * vec2(420.0, 240.0)) + floor(TIME * 10.0));
	float grain = (n - 0.5) * grain_strength;

	vec3 color = tint_color.rgb;
	float alpha = clamp(vignette + bars + tint_strength + shaft + grain, 0.0, 0.85);
	COLOR = vec4(color, alpha);
}
