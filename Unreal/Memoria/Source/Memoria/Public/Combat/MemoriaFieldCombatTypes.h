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
// S314: the source's battle rules in the field (battle_core). A turn is read as about two seconds.
inline constexpr float WaveHealShare = .20f;       // source Win: 20% of max HP back when the fight is won
inline constexpr float ItemDropChance = .30f;      // source Win: a 30% drop roll, a richer table after a void foe
inline constexpr float WeakenFactor = .70f;        // source weaken: -30% attack for 3 turns
inline constexpr float WeakenTime = 6.f;
inline constexpr int32 PoisonTicks = 3;            // source poison: 3 turns of attack*0.3 + 2..5
inline constexpr float PoisonInterval = 1.5f;
inline constexpr int32 IgniteTicks = 2;            // source burn grade >= 3: 2 turns of power*0.3 + 5 on the foe
inline constexpr float IgniteInterval = 1.f;
inline constexpr float BurnChainBonus = .20f;      // source chain: +20% per consecutive relationship-or-higher burn
inline constexpr float EmberAffinity = 1.10f;      // source passives
inline constexpr float VoidTouch = 1.15f;
inline constexpr int64 ResidualWarmth = 5;
inline const TCHAR* BurnSkillName(int32 Grade, bool bKo)
{
    static const TCHAR* Ko[5] = {TEXT("잿불"), TEXT("푸른 불꽃 베기"), TEXT("소각"), TEXT("자아의 장작더미"), TEXT("제로 번")};
    static const TCHAR* En[5] = {TEXT("Ember"), TEXT("Blue Flame Slash"), TEXT("Incinerate"), TEXT("Identity Pyre"), TEXT("Zero Burn")};
    return (bKo ? Ko : En)[FMath::Clamp(Grade, 0, 4)];
}
// The drop table's items (battle_core items), for the reward toast.
inline FString ItemName(const FString& Id, bool bKo)
{
    static const TMap<FString, TPair<const TCHAR*, const TCHAR*>> Names = {
        {TEXT("potion"), {TEXT("Potion"), TEXT("포션")}}, {TEXT("antidote"), {TEXT("Antidote"), TEXT("해독제")}},
        {TEXT("firebomb"), {TEXT("Firebomb"), TEXT("화염탄")}}, {TEXT("hi_potion"), {TEXT("Hi-Potion"), TEXT("하이포션")}},
        {TEXT("witness_ink"), {TEXT("Witness Ink"), TEXT("목격의 잉크")}}};
    const auto* Name = Names.Find(Id);
    return Name ? FString(bKo ? Name->Value : Name->Key) : Id;
}
// Fire for the lower grades, void violet for Identity Pyre and Zero Burn (the source's elements).
inline FLinearColor BurnColor(int32 Grade)
{ return Grade >= 3 ? FLinearColor(.62f, .30f, 1.f) : Grade == 1 ? FLinearColor(.35f, .62f, 1.f) : FLinearColor(1.f, .52f, .16f); }
}
enum class EMemoriaMonsterState : uint8 { Idle, Chase, Windup, Recover, Stagger, Dead };
// S313: the field foes. The void husk is the stand-in void creature; the market thief is the Verdan source
// enemy (source: 50 HP, "weaken"). Both are Epic's mannequin until Codex's models arrive.
enum class EMemoriaFoeKind : uint8 { VoidHusk, MarketThief };
// The source enemy abilities carried into the field (S314): a blow that lands also poisons or weakens.
enum class EMemoriaFoeAbility : uint8 { None, Poison, Weaken };
struct FMemoriaFoeSpec
{
    const TCHAR* Name; const TCHAR* NameKo;
    float Health, Height, Speed, Aggro, Reach, Windup, Recover, Stagger, Damage;
    const TCHAR* Idle; const TCHAR* Walk;   // mannequin foe clips (null: Epic's idle/walk)
    const TCHAR* Strike;                    // played across the windup, its blow at the windup's end
    float StrikeAt;                         // fraction of the strike clip where the blow lands
    bool bQuinn; FLinearColor Color, Glow; float Crack, Rim, BladeScale;
    bool bVoid; EMemoriaFoeAbility Ability;   // S314: source is_void (rewards) and the blow's status
};
inline const FMemoriaFoeSpec& FoeSpec(EMemoriaFoeKind Kind)
{
    using namespace MemoriaCombatTuning;
    static const FMemoriaFoeSpec Husk = {TEXT("Void Husk"), TEXT("보이드 허스크"),
        HuskHealth, HuskHeight, 80.f, HuskAggro, HuskReach, .70f, HuskRecover, HuskStagger, HuskDamage,
        TEXT("Zombie_Idle_Loop"), TEXT("Zombie_Walk_Fwd_Loop"), TEXT("Zombie_Scratch"), .55f,
        false, FLinearColor(.035f, .025f, .05f), FLinearColor(.55f, .18f, 1.f), 6.f, .8f, 0.f,
        // It stands in for the source pool's Alley Rat, so it carries the rat's poison.
        true, EMemoriaFoeAbility::Poison};
    // Quick and light: it darts in with a short blade and gets away; its blow is weaker and harder to see coming.
    static const FMemoriaFoeSpec Thief = {TEXT("Market Thief"), TEXT("시장 도둑"),
        45.f, 160.f, 150.f, 700.f, 125.f, .42f, .70f, .30f, 7.f,
        nullptr, nullptr, TEXT("Sword_Regular_A"), .55f,
        true, FLinearColor(.07f, .05f, .038f), FLinearColor(1.f, .62f, .30f), 0.f, .05f, .42f,
        false, EMemoriaFoeAbility::Weaken};
    return Kind == EMemoriaFoeKind::MarketThief ? Thief : Husk;
}
