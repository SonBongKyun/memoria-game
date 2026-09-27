#pragma once
#include "CoreMinimal.h"
// S311 field action combat tuning (Diablo-style quarter view). Distances are field units; the rigged
// Arrel stands 150 tall. First-pass numbers, tuned by feel; the turn-based source has no direct equivalent.
namespace MemoriaCombatTuning
{
// Player: a three-step combo toward the cursor.
inline constexpr float ComboDamage[3] = {12.f, 14.f, 22.f};
inline constexpr float ComboRate = 1.35f;          // clip playback rate
inline constexpr float HitWindowStart = .30f;       // fraction of the clip
inline constexpr float HitWindowEnd = .55f;
inline constexpr float QueueFrom = .40f;            // input from here chains the next step
inline constexpr float ComboReset = .45f;           // seconds after a step ends
inline constexpr float AttackRange = 175.f;
inline constexpr float AttackArcCos = .57f;         // about 55 degrees either side
// Dash: a short invulnerable burst.
inline constexpr float DashTime = .32f;
inline constexpr float DashDistance = 330.f;
inline constexpr float DashCooldown = .7f;
inline constexpr float PlayerStagger = .30f;
// Void husk (the mannequin stand-in).
inline constexpr float HuskHealth = 60.f;
inline constexpr float HuskHeight = 165.f;
inline constexpr float HuskSpeed = 95.f;
inline constexpr float HuskAggro = 750.f;
inline constexpr float HuskReach = 150.f;
inline constexpr float HuskWindup = .60f;
inline constexpr float HuskRecover = .90f;
inline constexpr float HuskStagger = .35f;
inline constexpr float HuskDamage = 9.f;
inline constexpr float HuskCorpseTime = 2.5f;
}
enum class EMemoriaMonsterState : uint8 { Idle, Chase, Windup, Recover, Stagger, Dead };
