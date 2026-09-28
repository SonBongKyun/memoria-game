#pragma once
#include "CoreMinimal.h"
// S311 field action combat tuning (Diablo-style quarter view). Distances are field units; the rigged
// Arrel stands 150 tall. First-pass numbers, tuned by feel; the turn-based source has no direct equivalent.
namespace MemoriaCombatTuning
{
// Player: a three-step combo toward the cursor.
inline constexpr float ComboDamage[3] = {12.f, 14.f, 22.f};
inline constexpr float ComboRate = 1.35f;          // clip playback rate (the unarmed melee set)
inline constexpr float SwordComboRate[3] = {1.f, 1.f, 1.3f}; // S312: UAL2's cuts (0.43 s, 0.53 s, a 2 s spinning finisher)
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
inline constexpr float SheatheDelay = 4.f;          // S312: seconds after the last fight before Arrel sheathes
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
// S312 memory burn: the source's five grade skills (battle_core burn_skills, by raw grade), in the field.
// Damage is the skill's base plus the memory's effective burn power. The ring reaches further and shoves
// harder with the grade.
inline constexpr float BurnBaseDamage[5] = {12.f, 30.f, 120.f, 250.f, 999.f};
inline constexpr float BurnRadius[5] = {260.f, 340.f, 460.f, 620.f, 1100.f};
inline constexpr float BurnShove[5] = {50.f, 80.f, 130.f, 190.f, 280.f};
inline constexpr float BurnPickDilation = .12f;    // world time while a memory is being chosen
inline constexpr float BurnCastTime = .45f;        // Arrel gathers the fire before the release
inline constexpr float BurnRingTime = .45f;        // the ring spreads to its full reach
inline constexpr float BurnAfterglow = 2.2f;       // the banner and the light linger
inline constexpr int32 BurnAskTwiceFrom = 3;       // raw grade from which a burn asks twice
inline const TCHAR* BurnSkillName(int32 Grade, bool bKo)
{
    static const TCHAR* Ko[5] = {TEXT("잿불"), TEXT("푸른 불꽃 베기"), TEXT("소각"), TEXT("자아의 장작더미"), TEXT("제로 번")};
    static const TCHAR* En[5] = {TEXT("Ember"), TEXT("Blue Flame Slash"), TEXT("Incinerate"), TEXT("Identity Pyre"), TEXT("Zero Burn")};
    return (bKo ? Ko : En)[FMath::Clamp(Grade, 0, 4)];
}
// Fire for the lower grades, void violet for Identity Pyre and Zero Burn (the source's elements).
inline FLinearColor BurnColor(int32 Grade)
{ return Grade >= 3 ? FLinearColor(.62f, .30f, 1.f) : Grade == 1 ? FLinearColor(.35f, .62f, 1.f) : FLinearColor(1.f, .52f, .16f); }
}
enum class EMemoriaMonsterState : uint8 { Idle, Chase, Windup, Recover, Stagger, Dead };
