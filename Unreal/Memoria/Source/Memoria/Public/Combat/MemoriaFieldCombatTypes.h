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
// S315 feel: hit stop (real seconds at a near-frozen world), camera shake, sparks, the blade's trail.
inline constexpr float HitStopDilation = .06f;
inline constexpr float HitStopLight = .055f, HitStopHeavy = .11f;
inline constexpr float ShakeLight = 5.f, ShakeHeavy = 12.f, ShakeTime = .22f;
inline constexpr float TrailLife = .14f;
// Heavy cut: hold the attack; a 360-degree spinning cut.
inline constexpr float ChargeTime = .45f;
inline constexpr float HeavyDamage = 30.f, HeavyRange = 215.f, HeavyShove = 60.f, HeavyRate = 1.1f;
// Guard: hold right click or K. Raised just before a blow, it parries: no harm and the foe reels.
inline constexpr float ParryWindow = .22f;
inline constexpr float BlockFactor = .30f;
inline constexpr float ParryStun = 1.2f;
inline constexpr float BlockHoldAt = .38f;          // the guard pose's point in Sword_Block
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
inline constexpr float CorpseSink = .9f;   // S340: the corpse's last seconds, sinking
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
// S319: Elia's techniques (elia_diary.gd SKILL_DEFS). Burning the memory her diary entry follows, while she is
// with the party, unlocks it. Cooldowns are the source turns at about two seconds each.
struct FMemoriaEliaSkillSpec { const TCHAR* Id; const TCHAR* Memory; const TCHAR* Name; const TCHAR* NameKo; float Cooldown; };
inline const FMemoriaEliaSkillSpec EliaSkills[4] = {
    {TEXT("humming_shield"), TEXT("daily_campfire_song"), TEXT("Humming Shield"), TEXT("흥얼거림의 방패"), 6.f},
    {TEXT("desperate_reach"), TEXT("rel_hand_reaching"), TEXT("Desperate Reach"), TEXT("절박한 손길"), 8.f},
    {TEXT("remembered_strike"), TEXT("identity_first_sword"), TEXT("Remembered Strike"), TEXT("기억된 일격"), 6.f},
    {TEXT("anchor_pulse"), TEXT("daily_elia_hands"), TEXT("Anchor Pulse"), TEXT("닻의 맥동"), 8.f}};
inline constexpr float HummingShieldTime = 2.f, HummingShieldFactor = .5f;   // "damage halved for 1 turn"
inline constexpr float DesperateReachStun = 2.f, DesperateReachRange = 320.f; // "stuns the enemy for 1 turn"
inline constexpr float RememberedStrikeRange = 420.f;                         // 10 + burned memories * 8
inline constexpr float AnchorPulseShare = .15f;                               // 15% of max HP, statuses cured
// The drop table's items (battle_core items), for the reward toast.
inline FString ItemName(const FString& Id, bool bKo)
{
    static const TMap<FString, TPair<const TCHAR*, const TCHAR*>> Names = {
        {TEXT("potion"), {TEXT("Potion"), TEXT("포션")}}, {TEXT("antidote"), {TEXT("Antidote"), TEXT("해독제")}},
        {TEXT("firebomb"), {TEXT("Firebomb"), TEXT("화염탄")}}, {TEXT("hi_potion"), {TEXT("Hi-Potion"), TEXT("하이포션")}},
        {TEXT("witness_ink"), {TEXT("Witness Ink"), TEXT("목격의 잉크")}}, {TEXT("smoke_bomb"), {TEXT("Smoke Bomb"), TEXT("연막탄")}}};
    const auto* Name = Names.Find(Id);
    return Name ? FString(bKo ? Name->Value : Name->Key) : Id;
}
// S341: the items Arrel uses in the field, by what they do. The numbers are GameManager.ITEMS':
// potion 40 HP, hi_potion 80 HP, antidote cures and restores 12, firebomb 12 on impact then 15 a turn for two
// turns, smoke_bomb a sure escape, witness_ink (here) the next blow guarded.
enum class EMemoriaItemEffect : uint8 { Heal, Cure, Bomb, Flee, Ward };
struct FMemoriaFieldItem { const TCHAR* Id; EMemoriaItemEffect Effect; int32 Power; int32 Extra; };
inline const FMemoriaFieldItem FieldItems[6] = {
    {TEXT("potion"), EMemoriaItemEffect::Heal, 40, 0}, {TEXT("hi_potion"), EMemoriaItemEffect::Heal, 80, 0},
    {TEXT("antidote"), EMemoriaItemEffect::Cure, 0, 12}, {TEXT("firebomb"), EMemoriaItemEffect::Bomb, 15, 12},
    {TEXT("smoke_bomb"), EMemoriaItemEffect::Flee, 0, 0}, {TEXT("witness_ink"), EMemoriaItemEffect::Ward, 1, 0}};
inline const FMemoriaFieldItem* FindFieldItem(const FString& Id)
{ for (const auto& Item : FieldItems) if (Id.Equals(Item.Id, ESearchCase::CaseSensitive)) return &Item; return nullptr; }
// The quick slots, on Z X C V B: healing (the potion, or the hi-potion for a deep wound), antidote, firebomb,
// smoke bomb, witness ink.
inline constexpr int32 QuickSlots = 5;
inline constexpr float ItemCooldown = .8f;          // seconds between two items
inline constexpr float BombRange = 620.f;           // how far Arrel throws
inline constexpr float BombRadius = 190.f;
inline constexpr float BombFlight = .38f;
inline constexpr int32 BombBurnTicks = 2;           // "burns the enemy for 2 turns"
inline constexpr int32 HiPotionFrom = 60;           // the hi-potion is taken first once this much HP is missing
// S342: the foes of the chapter maps' encounter pools. A caster keeps its distance and throws a slow orb; a
// charger marks a lane and runs it; a drain heals the foe by half the harm it does (battle_manager "drain");
// a scorch burns Arrel for two turns (battle_manager "burn_attack": attack * 0.2 + 3).
inline constexpr float CasterRange = 430.f, CasterRetreat = 230.f;
inline constexpr float OrbSpeed = 480.f, OrbRadius = 52.f, OrbLife = 2.4f;
inline constexpr float RushFrom = 380.f, RushSpeed = 640.f, RushDistance = 520.f, RushHit = 78.f, RushWidth = 70.f;
inline constexpr float DrainShare = .5f;
inline constexpr int32 ScorchTicks = 2;
// Fire for the lower grades, void violet for Identity Pyre and Zero Burn (the source's elements).
inline FLinearColor BurnColor(int32 Grade)
{ return Grade >= 3 ? FLinearColor(.62f, .30f, 1.f) : Grade == 1 ? FLinearColor(.35f, .62f, 1.f) : FLinearColor(1.f, .52f, .16f); }
}
enum class EMemoriaMonsterState : uint8 { Idle, Chase, Windup, Recover, Stagger, Dead, Rush };
// S313: the field foes. The void husk is the stand-in void creature; the market thief is the Verdan source
// enemy (source: 50 HP, "weaken"). Both are Epic's mannequin until Codex's models arrive.
// S342: and the six foes of the Belt Waystation's and Drift Shelter's pools, each on one of the two models.
enum class EMemoriaFoeKind : uint8 { VoidHusk, MarketThief, BeltScavenger, VoidWisp, DustCrawler, MemoryLeech, RubbleRat, AshWalker };
enum class EMemoriaFoeBehaviour : uint8 { Brawler, Caster, Charger };
// The source enemy abilities carried into the field (S314): a blow that lands also poisons or weakens.
enum class EMemoriaFoeAbility : uint8 { None, Poison, Weaken, Drain, Burn };
struct FMemoriaFoeSpec
{
    const TCHAR* Name; const TCHAR* NameKo;
    float Health, Height, Speed, Aggro, Reach, Windup, Recover, Stagger, Damage;
    const TCHAR* Idle; const TCHAR* Walk;   // mannequin foe clips (null: Epic's idle/walk)
    const TCHAR* Strike;                    // played across the windup, its blow at the windup's end
    float StrikeAt;                         // fraction of the strike clip where the blow lands
    bool bQuinn; FLinearColor Color, Glow; float Crack, Rim, BladeScale;
    bool bVoid; EMemoriaFoeAbility Ability;   // S314: source is_void (rewards) and the blow's status
    const TCHAR* Model;                       // S326: Codex's model (Field3D/<Model>), when imported
    // S342: how it fights, a second ability, the source's own numbers, and the tint that tells it from its model's kind.
    EMemoriaFoeBehaviour Behaviour = EMemoriaFoeBehaviour::Brawler;
    EMemoriaFoeAbility Second = EMemoriaFoeAbility::None;
    int32 SourceHp = 0, SourceAtk = 0;
    bool bRetint = false;
    FLinearColor ModelGlow = FLinearColor(.32f, .008f, .72f), ModelRim = FLinearColor::White;
    float ModelCrack = 2.5f, ModelRimStrength = .12f;
};
inline const FMemoriaFoeSpec& FoeSpec(EMemoriaFoeKind Kind)
{
    using namespace MemoriaCombatTuning;
    static const FMemoriaFoeSpec Husk = {TEXT("Void Husk"), TEXT("보이드 허스크"),
        HuskHealth, HuskHeight, 80.f, HuskAggro, HuskReach, .70f, HuskRecover, HuskStagger, HuskDamage,
        TEXT("Zombie_Idle_Loop"), TEXT("Zombie_Walk_Fwd_Loop"), TEXT("Zombie_Scratch"), .55f,
        false, FLinearColor(.035f, .025f, .05f), FLinearColor(.55f, .18f, 1.f), 6.f, .8f, 0.f,
        // It stands in for the source pool's Alley Rat, so it carries the rat's poison.
        true, EMemoriaFoeAbility::Poison, TEXT("Husk")};
    // Quick and light: it darts in with a short blade and gets away; its blow is weaker and harder to see coming.
    static const FMemoriaFoeSpec Thief = {TEXT("Market Thief"), TEXT("시장 도둑"),
        45.f, 160.f, 150.f, 700.f, 125.f, .42f, .70f, .30f, 7.f,
        nullptr, nullptr, TEXT("Sword_Regular_A"), .55f,
        true, FLinearColor(.07f, .05f, .038f), FLinearColor(1.f, .62f, .30f), 0.f, .05f, .42f,
        false, EMemoriaFoeAbility::Weaken, TEXT("Thief")};
    // S342: the pools' foes. Health is near the source's HP and the blow about 0.6 of its attack, as the thief's
    // (50 and 12 in the source) became 45 and 7 here.
    struct FVariant
    {
        static FMemoriaFoeSpec Of(const FMemoriaFoeSpec& Base, const TCHAR* Name, const TCHAR* NameKo, int32 Hp, int32 Atk, float Health, float Height, float Speed, float Damage,
            float Windup, float Recover, bool bVoid, EMemoriaFoeAbility Ability, EMemoriaFoeAbility Second, EMemoriaFoeBehaviour Behaviour,
            const FLinearColor& Glow, float Crack, const FLinearColor& Rim, float RimStrength)
        {
            FMemoriaFoeSpec S = Base;
            S.Name = Name; S.NameKo = NameKo; S.SourceHp = Hp; S.SourceAtk = Atk; S.Health = Health; S.Height = Height; S.Speed = Speed; S.Damage = Damage;
            S.Windup = Windup; S.Recover = Recover; S.bVoid = bVoid; S.Ability = Ability; S.Second = Second; S.Behaviour = Behaviour;
            S.bRetint = true; S.ModelGlow = Glow; S.ModelCrack = Crack; S.ModelRim = Rim; S.ModelRimStrength = RimStrength;
            // The mannequin stand-in (when the models are not imported) wears the same glow.
            S.Glow = Rim;
            return S;
        }
    };
    using A = EMemoriaFoeAbility; using B = EMemoriaFoeBehaviour;
    static const FMemoriaFoeSpec Scavenger = FVariant::Of(Thief, TEXT("Belt Scavenger"), TEXT("벨트 약탈자"), 55, 12, 50.f, 165.f, 140.f, 7.f, .45f, .75f,
        false, A::Weaken, A::None, B::Brawler, FLinearColor::Black, 0.f, FLinearColor(.95f, .62f, .22f), .24f);
    static const FMemoriaFoeSpec Wisp = FVariant::Of(Husk, TEXT("Void Wisp"), TEXT("보이드 도깨비불"), 45, 14, 40.f, 135.f, 110.f, 8.f, .90f, 1.40f,
        true, A::Drain, A::None, B::Caster, FLinearColor(.25f, .7f, 1.f), 5.f, FLinearColor(.4f, .8f, 1.f), .70f);
    static const FMemoriaFoeSpec Crawler = FVariant::Of(Thief, TEXT("Dust Crawler"), TEXT("먼지 크롤러"), 40, 10, 36.f, 118.f, 170.f, 6.f, .65f, 1.10f,
        false, A::Poison, A::None, B::Charger, FLinearColor::Black, 0.f, FLinearColor(.85f, .74f, .46f), .22f);
    static const FMemoriaFoeSpec Leech = FVariant::Of(Husk, TEXT("Memory Leech"), TEXT("기억 거머리"), 50, 13, 45.f, 150.f, 100.f, 8.f, .60f, .90f,
        true, A::Drain, A::None, B::Brawler, FLinearColor(1.f, .1f, .3f), 4.f, FLinearColor(1.f, .2f, .4f), .45f);
    static const FMemoriaFoeSpec Rat = FVariant::Of(Thief, TEXT("Rubble Rat"), TEXT("잔해쥐"), 35, 9, 32.f, 105.f, 185.f, 5.f, .60f, 1.10f,
        false, A::Poison, A::None, B::Charger, FLinearColor::Black, 0.f, FLinearColor(.62f, .64f, .70f), .20f);
    static const FMemoriaFoeSpec Walker = FVariant::Of(Husk, TEXT("Ash Walker"), TEXT("재의 방랑자"), 60, 11, 55.f, 185.f, 70.f, 8.f, .85f, 1.00f,
        false, A::Burn, A::Weaken, B::Brawler, FLinearColor(1.f, .42f, .08f), 5.f, FLinearColor(1.f, .5f, .15f), .40f);
    switch (Kind)
    {
    case EMemoriaFoeKind::MarketThief: return Thief;
    case EMemoriaFoeKind::BeltScavenger: return Scavenger;
    case EMemoriaFoeKind::VoidWisp: return Wisp;
    case EMemoriaFoeKind::DustCrawler: return Crawler;
    case EMemoriaFoeKind::MemoryLeech: return Leech;
    case EMemoriaFoeKind::RubbleRat: return Rat;
    case EMemoriaFoeKind::AshWalker: return Walker;
    default: return Husk;
    }
}
// S342: a map's encounter by its source name; one the port has no foe for falls back to the husk (void) or the thief.
inline EMemoriaFoeKind FoeKindByName(const FString& SourceName, bool bVoid)
{
    for (const EMemoriaFoeKind Kind : {EMemoriaFoeKind::BeltScavenger, EMemoriaFoeKind::VoidWisp, EMemoriaFoeKind::DustCrawler, EMemoriaFoeKind::MemoryLeech,
        EMemoriaFoeKind::RubbleRat, EMemoriaFoeKind::AshWalker, EMemoriaFoeKind::MarketThief, EMemoriaFoeKind::VoidHusk})
        if (SourceName.Equals(FoeSpec(Kind).Name, ESearchCase::CaseSensitive)) return Kind;
    return bVoid ? EMemoriaFoeKind::VoidHusk : EMemoriaFoeKind::MarketThief;
}
// How many rise together: the small chargers in a pack of three, the rest in twos (the husk keeps its three).
inline int32 FoePackSize(EMemoriaFoeKind Kind)
{ return Kind == EMemoriaFoeKind::VoidHusk || FoeSpec(Kind).Behaviour == EMemoriaFoeBehaviour::Charger ? 3 : 2; }
