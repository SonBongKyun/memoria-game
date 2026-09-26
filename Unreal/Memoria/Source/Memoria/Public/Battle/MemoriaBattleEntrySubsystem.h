#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Battle/MemoriaEncounterModel.h"
#include "Battle/MemoriaBattleModel.h"
#include "MemoriaBattleEntrySubsystem.generated.h"

class UMemoriaRunSubsystem;
struct FMemoriaBattleChoice { FString Id, Label; int32 Grade=0; bool bAvailable=false; FLinearColor Accent=FLinearColor::White; };
struct MEMORIA_API FMemoriaBattleEntryView
{
    bool bActive = false, bReturning = false, bKo = false, bCanFlee = false;
    uint64 Revision = 0;
    int32 EnemyIndex = INDEX_NONE;
    FString EnemyName, Weakness, Resistance, CurrentLocale;
    int64 EnemyHp = 0, EnemyMaxHp = 0, EnemyAttack = 0, PlayerHp = 0, PlayerMaxHp = 0;
    bool bEnemyVoid = false, bPlayerDefending = false, bEliaInParty = false;
    bool bFieldFocusOpening = false, bSableInParty = false, bTobiasInParty = false;
    bool bEliaCooldownsReset = false;
    // Development observation of source signal order; never rendered as player UI.
    FString SourceEventsJson;
    TArray<FString> EnemyAbilities, Logs, Requests;
    FString ModifierId, ModifierName, ModifierDescription, ModifierEffect;
    int64 ModifierValue = 0;
    FString ObjectiveId, ObjectiveTitle, ObjectiveDescription, ObjectiveProgress;
    int64 ObjectiveRewardGrains = 0, ObjectiveRewardHeal = 0;
    FString ObjectiveRewardItem;
    bool bObjectiveFocusBoosted = false;
    int32 ObjectiveProgressCurrent = 0, ObjectiveProgressTarget = 1;
    double Momentum = 0., LimitGauge = 0., BreakGauge = 0., DifficultyBonus = 0.;
    int32 MomentumRank = 0, WitnessProgress = 0, WitnessRequired = 2;
    bool bWitnessComplete = false, bResolvedByWitness = false;
    FString WitnessLine;
    FString MomentumLabel, EnvironmentName, EnvironmentDescription;
    FString BackgroundSource, EnemyImageSource;
    bool bResolving=false, bVictory=false, bDefeat=false, bObjectiveSupported=true, bObjectiveComplete=false, bObjectiveFailed=false;
    int32 Combo=0, BrokenTurns=0;
    uint64 ImpactSerial=0;
    FString Telegraph, LastBurnTitle;
    TArray<FMemoriaBattleHit> Hits;
    TArray<FMemoriaBattleStatus> PlayerStatuses, EnemyStatuses;
    TArray<FMemoriaBattleChoice> Memories, Items;
    FMemoriaBattleReward Reward;
    FString BattleState = TEXT("IDLE");
};
DECLARE_MULTICAST_DELEGATE(FMemoriaBattleEntryChanged);
DECLARE_MULTICAST_DELEGATE(FMemoriaBattleEntryReturned);

// Bounded combat session. A live world and run epoch own all delayed commands.
UCLASS()
class MEMORIA_API UMemoriaBattleEntrySubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    bool BeginEncounter(int32 EnemyIndex, FMemoriaEncounterRng& Rng, UWorld* Owner = nullptr);
    FMemoriaBattleEntryView GetView() const;
    bool Flee(uint64 ExpectedRevision);
    bool Submit(const FString& Action, const FString& Id, uint64 ExpectedRevision);
    bool DismissVictory(uint64 ExpectedRevision);
    bool RecoverToVerdan(uint64 ExpectedRevision);
    bool RetryCheckpoint(uint64 ExpectedRevision);
#if WITH_DEV_AUTOMATION_TESTS
    void SetCombatRng(FMemoriaEncounterRng InRng) { CombatRng=MoveTemp(InRng); }
#endif
    bool IsActive() const;
    bool IsReturning() const;
    uint64 GetRevision() const { return Revision; }
    void Cancel();
    FMemoriaBattleEntryChanged OnChanged;
    FMemoriaBattleEntryReturned OnReturned;
private:
    UPROPERTY(Transient) TObjectPtr<UMemoriaRunSubsystem> Run;
    TWeakObjectPtr<UWorld> OwnerWorld;
    FGuid OwnerRun;
    FTimerHandle ReturnTimer, ActionTimer;
    FMemoriaBattleModel Combat;
    TOptional<FMemoriaBattleModel> PendingEnemyResult;
    uint64 PendingEnemyRevision=0;
    FMemoriaEncounterRng CombatRng=FMemoriaEncounterRng::Random();
    FString PendingAction, PendingId;
    void StartCombat();
    void ResolveAction();
    void ResolveEnemy();
    void ResolveEnemyImpact();
    void CommitCombat(FMemoriaBattleModel&& Next);
    void UpdateCombatView();
    FMemoriaBattleEntryView View;
    uint64 Revision = 0;
    bool bBusy = false;
    bool HasLiveOwner() const;
    void FinishReturn();
    void OnWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);
};
