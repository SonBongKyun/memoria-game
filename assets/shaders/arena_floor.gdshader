shader_type spatial;
render_mode blend_mix, cull_disabled, unshaded, depth_draw_never;

// S211: 전투 무대의 3D 바닥.
//
// 예전 하이브리드 무대는 점선 타원 몇 개만 그려서, 캐릭터가 서 있는 "공간"이
// 전혀 느껴지지 않았다. 그렇다고 불투명한 원판을 깔면 손으로 그린 배경
// 일러스트를 덮어 버린다 (S206에서 한 번 겪은 문제).
//
// 그래서 바닥을 "가까울수록 진하고 멀수록 사라지는" 면으로 만든다.
// 화면 아래쪽(카메라에 가까운 지면)에만 실체가 있고, 지평선 쪽은 배경 그림에
// 자리를 내준다. 격자선은 원근을 읽히게 해 주는 최소한의 단서다.

uniform vec3 floor_color : source_color = vec3(0.10, 0.09, 0.13);
uniform vec3 grid_color : source_color = vec3(0.55, 0.45, 0.30);
uniform float near_alpha : hint_range(0.0, 1.0) = 0.85;
uniform float fade_start : hint_range(0.0, 60.0) = 4.0;
uniform float fade_end : hint_range(0.0, 60.0) = 26.0;
uniform float grid_spacing : hint_range(0.25, 8.0) = 2.0;
uniform float grid_width : hint_range(0.002, 0.2) = 0.03;
uniform float grid_strength : hint_range(0.0, 2.0) = 0.55;
uniform float side_fade : hint_range(0.0, 40.0) = 15.0;

varying vec3 world_position;

void vertex() {
	world_position = (MODEL_MATRIX * vec4(VERTEX, 1.0)).xyz;
}

// 한 축에 대한 격자선 세기. 셀 경계에 가까울수록 1에 가깝다.
float grid_line(float coordinate) {
	float cell = abs(fract(coordinate / grid_spacing) - 0.5) * grid_spacing;
	return 1.0 - smoothstep(0.0, grid_width, cell);
}

void fragment() {
	float distance_from_camera = length(world_position - CAMERA_POSITION_WORLD);
	// 깊이 감쇠: 가까운 지면은 남고 먼 지면은 배경으로 사라진다.
	float depth_fade = 1.0 - smoothstep(fade_start, fade_end, distance_from_camera);
	// 좌우 감쇠: 무대 밖으로 나가면서 가장자리가 잘려 보이지 않게 한다.
	float lateral_fade = 1.0 - smoothstep(side_fade * 0.45, side_fade, abs(world_position.x));

	float grid = max(grid_line(world_position.x), grid_line(world_position.z));
	// 격자도 멀어지면 함께 흐려져야 모아레가 생기지 않는다.
	grid *= depth_fade * grid_strength;

	vec3 color = mix(floor_color, grid_color, clamp(grid, 0.0, 1.0));
	ALBEDO = color;
	ALPHA = clamp(near_alpha * depth_fade * lateral_fade + grid * 0.35, 0.0, 1.0);
}
