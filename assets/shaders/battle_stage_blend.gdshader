shader_type canvas_item;

uniform float edge_softness : hint_range(0.02, 0.4) = 0.16;
uniform float lower_fade : hint_range(0.0, 1.0) = 0.76;
// S209: 전투원 판(plate)용 타원 마스크.
// 적/동행자 아트의 상당수가 16:9 장면 일러스트라서, 사각 테두리가 그대로 보이면
// 배경 위에 카드를 붙인 것처럼 읽힌다. 0보다 크면 중심에서 멀어질수록 알파를 깎아
// 테두리를 무대 속으로 녹인다.
uniform float oval_mask : hint_range(0.0, 1.0) = 0.0;

// S214: 무대와 같은 빛을 받게 한다.
//
// 지금까지 2D 전투원은 자기 원화의 밝기 그대로 그려졌다. 3D 무대에는 따뜻한 키라이트와
// 바이옴 색 보조광이 있는데 캐릭터만 그 빛을 안 받으니, 아무리 무대를 잘 만들어도
// "배경 위에 붙인 그림"으로 읽혔다.
//
// 두 가지만 한다. 실루엣 가장자리에 키라이트 방향의 림라이트를 얹고, 전체를 바이옴
// 색으로 아주 살짝 물들인다. 원화의 명암은 그대로 두므로 그림이 망가지지 않는다.
uniform vec2 key_direction = vec2(-0.55, -0.83);
uniform vec3 key_color : source_color = vec3(1.0, 0.82, 0.58);
uniform float rim_strength : hint_range(0.0, 3.0) = 0.0;
uniform vec3 ambient_tint : source_color = vec3(1.0, 1.0, 1.0);
uniform float ambient_strength : hint_range(0.0, 1.0) = 0.0;

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

	if (ambient_strength > 0.001) {
		COLOR.rgb = mix(COLOR.rgb, COLOR.rgb * ambient_tint, ambient_strength);
	}

	if (rim_strength > 0.001) {
		// 알파 기울기로 실루엣 경계를 찾는다. 경계가 키라이트를 향한 쪽만 밝아진다.
		//
		// 주의: 원화 포트레이트는 알파 가장자리가 넓게 흐려져 있다. 1px 기울기를 그대로
		// 쓰면 그 반투명 띠 전체가 반짝여서 캐릭터가 지직거리는 것처럼 보였다.
		// 그래서 (1) 표본 간격을 넓혀 미세 노이즈를 흘려보내고,
		//        (2) 충분히 불투명한 안쪽 경계에만 빛을 얹는다.
		vec2 px = TEXTURE_PIXEL_SIZE * 1.8;
		float ax = texture(TEXTURE, UV + vec2(px.x, 0.0)).a - texture(TEXTURE, UV - vec2(px.x, 0.0)).a;
		float ay = texture(TEXTURE, UV + vec2(0.0, px.y)).a - texture(TEXTURE, UV - vec2(0.0, px.y)).a;
		vec2 gradient = vec2(ax, ay);
		float slope = length(gradient);
		float solid = smoothstep(0.55, 0.9, tex.a);
		float edge = clamp(slope * 1.4, 0.0, 1.0) * solid;
		if (edge > 0.002) {
			// 기울기는 알파가 커지는 방향(안쪽)을 가리키므로 뒤집어 바깥 법선을 얻는다.
			vec2 outward = normalize(-gradient);
			float facing = max(dot(outward, normalize(key_direction)), 0.0);
			COLOR.rgb += key_color * (edge * facing * rim_strength);
		}
	}

	COLOR.a *= edge_mask * mask * mix(0.45, 1.0, floor_blend);
}
