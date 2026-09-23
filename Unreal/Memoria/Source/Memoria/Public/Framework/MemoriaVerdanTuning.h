#pragma once
#include "CoreMinimal.h"

// Provisional play-feel values for the bounded Verdan prototype, not story canon.
// The foundation pawn retains its defaults until the slice host opts in.
namespace MemoriaVerdanTuning
{
inline constexpr float WalkSpeed = 120.f;
inline constexpr float Acceleration = 1200.f;
inline constexpr float Deceleration = 1200.f;
inline constexpr float TurningBoost = 8.f;
// Source skeleton centimetres; world stride also includes the mesh scale.
inline constexpr float FootReach = 32.f;
inline constexpr float StanceFraction = .55f;
inline constexpr float MaxVisualCyclesPerSecond = 3.f;
inline constexpr float CameraFOV = 55.f;
inline constexpr float CameraPitch = -48.f;
inline const FVector CameraOffset(0, -820, 1000);
}
