#pragma once
#include "CoreMinimal.h"
#include "Battle/MemoriaEncounterModel.h"
#include "Run/MemoriaRunTypes.h"

struct FMemoriaBattleStatus { int32 Effect=0, Turns=0; int64 Power=0; };
struct FMemoriaBattleHit { FString Target, Skill; int64 Amount=0; };
struct FMemoriaBattleBurn
{
    FString Id, Title;
    int32 Grade=0; int64 Power=0, EffectivePower=0;
    bool bEmberAffinity=false, bVoidTouch=false, bResidualWarmth=false, bMemoryCascade=false;
};
struct FMemoriaBattleReward
{
    int64 Grains=0, Heal=0, ObjectiveBonus=0, ObjectiveHeal=0, MomentumBonus=0;
    int64 TacticalBonus=0, PreservationBonus=0, GradeBonus=0, StreakBonus=0, FocusGained=0;
    int32 Score=0;
    FString Item, ItemId, ObjectiveItem, Grade=TEXT("D"), Resolution=TEXT("defeat");
};
// Synchronous value model. No UObject, timers, delegates, audio or world access.
// The host commits a copied result only while the originating run/world/revision live.
struct MEMORIA_API FMemoriaBattleModel
{
    FMemoriaRunSnapshot Run;
    FString EnemyName, Weakness, Resistance, ObjectiveId, ObjectiveTitle, ObjectiveItem;
    FString ModifierEffect, LastAction, Ability;
    int64 EnemyHp=0, EnemyMaxHp=0, EnemyAttack=0, ModifierValue=0;
    int64 ObjectiveGrains=0, ObjectiveHeal=0;
    bool bVoid=false, bDefending=false, bShielded=false, bReflecting=false, bCharged=false;
    bool bLastStand=false, bStunned=false, bPendingEnemy=false, bVictory=false, bDefeat=false;
    bool bObjectiveComplete=false, bObjectiveFailed=false, bObjectiveSupported=true;
    bool bMemoryCascade=false, bUnbrokenEdge=false, bSteadyHand=false;
    double AnchorGuard=0, Difficulty=0, Momentum=0, Limit=0, Break=0;
    int32 Rank=0, BestRank=0, BrokenTurns=0, Combo=0, MaxCombo=0, Chain=0, Aftershock=0;
    int32 Turns=0, Actions=0, Burns=0, Breaks=0, EnemyResponses=0;
    // Source WITNESS reading: a turn spent hearing the echo instead of burning.
    int32 WitnessProgress=0, WitnessRequired=2, ItemsUsed=0;
    bool bWitnessComplete=false, bResolvedByWitness=false, bScanned=false;
    FString WitnessLine;
    TArray<FString> Abilities, Logs, Sounds;
    TArray<FMemoriaBattleStatus> PlayerStatuses, EnemyStatuses;
    TArray<FMemoriaBattleHit> Hits;
    FMemoriaBattleReward Reward;
    bool Act(const FString& Action, const FString& Id, const FMemoriaBattleBurn* Burn, FMemoriaEncounterRng& Rng);
    void EnemyTurn(FMemoriaEncounterRng& Rng);
    void ApplyStatus(bool bPlayer, int32 Effect, int32 Duration, int64 Power);
    void ClearEvents();
    static bool SupportsObjective(const FString& Id);
    // Source _get_witness_requirement for non-boss encounters; read before corruption.
    static int32 WitnessRequirement(bool bVoidBeast, bool bEliaAnchor);
    static FString WitnessKey(const FString& EnemyName);
    static FString Text(const FString& Locale, const FString& Key, const TArray<FString>& Args={});
private:
    void Log(const FString& Key, const TArray<FString>& Args={});
    void AddMomentum(double Amount, const FString& Reason);
    void AddLimit(double Amount);
    void CheckObjective(bool bBurn=false, bool bItem=false);
    void Witness(int32 Power, bool bInk, FMemoriaEncounterRng& Rng);
    void Pressure(const FString& Element);
    void StatusTick(bool bPlayer);
    void EndPlayer(FMemoriaEncounterRng& Rng);
    void CheckPlayer();
    void Win(FMemoriaEncounterRng& Rng);
    bool LastStand(bool bLethal);
    int64 Damage(int64 Amount, const FString& Skill);
    double Element(const FString& Id) const;
    double Weaken(bool bPlayer) const;
    bool HasStatus(bool bPlayer,int32 Effect) const;
    FString SelectAbility(FMemoriaEncounterRng& Rng) const;
    void AddItem(const FString& Id);
};
