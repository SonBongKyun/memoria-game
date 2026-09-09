#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TimerManager.h"
#include "Narrative/MemoriaNarrativeRuntime.h"
#include "MemoriaNarrativeSubsystem.generated.h"

class UMemoriaRunSubsystem;
class UMemoriaRunSaveGame;
enum class EMemoriaSliceState : uint8 { Idle, VN, Travelling, Field, Exploration, Deferred, Failed };
struct FMemoriaPresentedChoice { int32 OriginalIndex; FString Text; };
// Presentation receives values only. No conditions, effect data or mutable run.
struct MEMORIA_API FMemoriaNarrativeView
{
    FString Header, Speaker, Narration, Body;
    TArray<FMemoriaPresentedChoice> Choices;
    bool bPaused = false;
    bool bCompactStatus = false;
    bool bDevelopmentStop = false;
};

UCLASS()
class MEMORIA_API UMemoriaNarrativeSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    bool StartDevelopmentVN();
    bool StartUnseenFieldFixture();
    bool EnterVerdan();
    bool InteractWithMalet();
    const FString& GetDeferredInteraction() const { return DeferredInteraction; }
    bool IsMaletTalkCached() const { return bMaletTalkCached; }
    bool IsMaletCallbackConnected() const { return bMaletCallbackConnected; }
    bool IsMaletDelayPending() const { return DelayWorld.IsValid(); }
    double GetMaletDelaySeconds() const { return ActualDelaySeconds; }
    bool IsMaletRewardDelayPending() const { return RewardDelayWorld.IsValid(); }
    double GetMaletRewardDelaySeconds() const { return ActualRewardDelaySeconds; }
    bool IsRewardCallbackPending() const { return RewardCallbackWorld.IsValid() && RewardCompletionCount == 0; }
    int32 GetRewardCompletionCount() const { return RewardCompletionCount; }
    int32 GetRewardCallbackIntentCount() const { return RewardCallbackIntentCount; }
    int32 GetRewardFieldInvocationCount() const { return RewardFieldInvocationCount; }
    int32 GetMaletReactionCount() const { return MaletReactionCount; }
    void Confirm(int32 OriginalChoice = INDEX_NONE);
    void Back();
    FMemoriaNarrativeView GetView() const;
    EMemoriaSliceState GetState() const { return State; }
    bool IsPaused() const { return bPaused; }
    int32 GetRevision() const { return Revision; }
    int32 GetFieldInvocationCount() const { return FieldInvocationCount; }
    const TArray<FString>& GetTrace() const { return Trace; }
    FMemoriaVNContinuation GetContinuation() const;
    UMemoriaRunSaveGame* CaptureSave() const;
    bool PrepareRestore(const UMemoriaRunSaveGame& Save);
    bool ResumePrepared();
    void Record(const FString& Event);
    static constexpr const TCHAR* VerdanMap = TEXT("/Game/Tests/Campaign/L_VerdanHost");
private:
    UPROPERTY(Transient) TObjectPtr<UMemoriaRunSubsystem> Run;
    UPROPERTY(Transient) TObjectPtr<UMemoriaVNAsset> VNAsset;
    UPROPERTY(Transient) TObjectPtr<UMemoriaFieldAsset> FieldAsset;
    UPROPERTY(Transient) TObjectPtr<UMemoriaFieldAsset> ActiveFieldAsset;
    UPROPERTY(Transient) TObjectPtr<UMemoriaFieldAsset> MaletAsset;
    UPROPERTY(Transient) TObjectPtr<UMemoriaFieldAsset> EncounterAsset;
    UPROPERTY(Transient) TObjectPtr<UMemoriaFieldAsset> RefusedAsset;
    UPROPERTY(Transient) TObjectPtr<UMemoriaFieldAsset> DealAsset;
    UPROPERTY(Transient) TObjectPtr<UMemoriaFieldAsset> RewardAsset;
    // Source reward listener is one-shot and synchronous. This owner binds only
    // a pre-effect development boundary, never the downstream gameplay handler.
    TWeakObjectPtr<UWorld> RewardCallbackWorld;
    FGuid RewardCallbackRunId;
    int32 RewardCompletionCount = 0, RewardCallbackIntentCount = 0, RewardFieldInvocationCount = 0;
    void DeferRewardEffects();
    FString DeferredInteraction;
    bool bMaletTalkCached = false, bMaletFirstTalkPending = false, bMaletCallbackConnected = false;
    FTimerHandle MaletDelay, RewardDelay;
    TWeakObjectPtr<UWorld> DelayWorld, RewardDelayWorld;
    double DelayStarted = 0, ActualDelaySeconds = 0;
    double RewardDelayStarted = 0, ActualRewardDelaySeconds = 0;
    void CancelMaletDelay();
    void OnWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);
    void StartMaletField(UMemoriaFieldAsset* Asset);
    void FinishField();
    void NormalDelayElapsed();
    void RewardDelayElapsed();
    int32 MaletReactionCount = 0;
    TUniquePtr<FMemoriaNarrativeContext> Context;
    TUniquePtr<FMemoriaVNInterpreter> VN;
    TUniquePtr<FMemoriaFieldInterpreter> Field;
    EMemoriaSliceState State = EMemoriaSliceState::Idle;
    bool bPaused = false;
    int32 Revision = 0, EventCursor = 0, FieldInvocationCount = 0;
    TArray<FString> Trace;
    void Reset();
    bool LoadContracts();
    void FlushEvents(const FString& Dialect);
    void AfterVN();
    void Explore();
};
