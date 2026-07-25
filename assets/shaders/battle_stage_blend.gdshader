shader_type canvas_item;

uniform float edge_softness : hint_range(0.02, 0.4) = 0.16;
uniform float lower_fade : hint_range(0.0, 1.0) = 0.76;
// S209: 전투원 판(plate)용 타원 마스크.
// 적/동행자 아트의 상당수가 16:9 장면 일러스트라서, 사각 테두리가 그대로 보이면
// 배경 위에 카드를 붙인 것처럼 읽힌다. 0보다 크면 중심에서 멀어질수록 알파를 깎아
// 테두리를 무대 속으로 녹인다.
uniform float oval_mask : hint_range(0.0, 1.0) = 0.0;

void fragment() {
	vec4 tex = texture(TEXTURE, UV);
	float left = smoothstep(0.0, edge_softness, UV.x);
	float right = smoothstep(0.0, edge_softness, 1.0 - UV.x);
	float top = smoothstep(0.0, edge_softness * 0.7, UV.y);
	float bottom = smoothstep(0.0, edge_softness, 1.0 - UV.y);
	float edge_mask = left * right * top * bottom;
	float floor_blend = 1.0 - smoothstep(lower_fade, 1.0, UV.y);

	vec2 centered = (UV - vec2(0.5)) * vec2(1.0, 0.92);
	float radius = length(centered) * 2.0;
	float oval = 1.0 - smoothstep(0.78, 1.16, radius);
	float mask = mix(1.0, oval, oval_mask);

	COLOR = tex * COLOR;
	COLOR.a *= edge_mask * mask * mix(0.45, 1.0, floor_blend);
}
