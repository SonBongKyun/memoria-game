#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TimerManager.h"
#include "World/MemoriaWorldCognition.h"
#include "Run/MemoriaRunTypes.h"
#include "Narrative/MemoriaNarrativeRuntime.h"
#include "Shop/MemoriaShopSubsystem.h"
#include "MemoriaNarrativeSubsystem.generated.h"

class UMemoriaRunSubsystem;
class UMemoriaRunSaveGame;
DECLARE_MULTICAST_DELEGATE_OneParam(FMemoriaRewardBoundaryObserved, const FString&);
enum class EMemoriaSliceState : uint8 { Idle, VN, Travelling, Field, Exploration, Deferred, Failed };
struct FMemoriaPresentedChoice { int32 OriginalIndex; FString Text; FString Effect; };
// Presentation receives values only. No conditions, effect data or mutable run.
struct MEMORIA_API FMemoriaNarrativeView
{
    FString Header, Speaker, Narration, Body;
    FString BackdropSource, PortraitSource, PortraitSide, LocationTitle;
    TArray<FMemoriaPresentedChoice> Choices;
    bool bShopPresentation = false;
    FMemoriaShopView Shop;
    bool bPaused = false;
    bool bCompactStatus = false;
    bool bDevelopmentStop = false;
    // vn_scene.gd step cues. CueKey changes once per shown step; one-shot cues (impact
    // flash and nudge, CG motion restart, distortion glitch) fire only when it changes.
    FString CueKey, Impact, CgMotion;
    float CgFadeSeconds = .8f;
    bool bStepHasCg = false, bDistorted = false, bSystemLog = false;
    FString ChoiceTitle, ChoiceHint;
    // SceneFlow._show_chapter_ledger, shown once per completion (a new LedgerSerial).
    int32 LedgerSerial = 0;
    FString LedgerTitle;
    TArray<FString> LedgerLines;
    bool bLedgerThreadHolds = false;
    // vn_scene.gd _on_memory_burned: a new BurnSerial plays the burn glitch once.
    int32 BurnSerial = 0;
};

UCLASS()
class MEMORIA_API UMemoriaNarrativeSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
#if WITH_DEV_AUTOMATION_TESTS
    // Synchronous observation only. Test subscribers may exercise real owner lifetimes;
    // the normal route has no subscriber, injected state or alternate flag authority.
    FMemoriaRewardBoundaryObserved OnRewardBoundaryObserved;
#endif
    // Read-only presentation of recorded synchronous seed boundaries, after stop.
    void PresentSeedObservation(int32 Index);
    void PresentFirebombObservation(int32 Index);
    void PresentAntidoteObservation(int32 Index);
    void PresentPotionObservation(int32 Index);
    bool StartDevelopmentVN();
    // main.gd _on_new_game_pressed: a fresh chapter 1 run plays ch1_cold_open through the
    // imported Chapter 1 route to the Verdan map request. The locale is a setting, not run data:
    // it carries over from the previous run unless one is given.
    bool StartNewGame(const FString& Locale = FString());
    bool IsNewGameRoute() const { return bNewGameRoute; }
    // SaveManager.autosave_on_chapter_transition: resume the latest chapter autosave.
    bool ResumeChapterAutosave();
    int32 GetAutosaveCount() const { return AutosaveCount; }
    // Track id of the BGM the current VN scene declared (SceneFlow.play), or None.
    FName GetSceneMusic() const { return SceneMusic; }
    bool StartUnseenFieldFixture();
    bool EnterVerdan();
    bool ContinueCheckpoint();
    bool RequestCheckpointRevisit();
    bool HasPendingVerdanReentry() const { return bPendingVerdanReentry; }
    bool EnterVerdanReentry();
    bool IsVerdanRevisit() const;
    bool ReturnFromAmbientBattle();
    bool InteractWithMalet();
    // Source companion.gd interact() for Elia in Verdan: burn reaction, first talk, repeat line.
    bool InteractWithElia();
    // Source verdan_market.gd exploration beats: armed on Verdan entry, one-time by flag.
    bool StartStoryBeat(const FString& Group);
    bool IsStoryBeatAvailable(const FString& Group) const;
    const TArray<FString>& GetArmedStoryBeats() const { return ArmedStoryBeats; }
    // Source toast text shown in the exploration status for a few seconds, and the quest tracker line.
    FString GetExplorationNotice() const;
    FString GetQuestTrackerLine() const;
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
    UPROPERTY(Transient) TObjectPtr<UMemoriaFieldAsset> StoryAsset;
    UPROPERTY(Transient) TArray<TObjectPtr<UMemoriaVNAsset>> RouteAssets;
    const UMemoriaVNAsset* ResolveVN(const FString& Id);
    FMemoriaVNInterpreter::FResolver Resolver();
    bool bNewGameRoute = false;
    FString SceneCg, ShownScene, ShownCue;
    FName SceneMusic;
    int32 LedgerSerial = 0, AutosaveCount = 0, BurnSerial = 0;
    FString LedgerTitle;
    TArray<FString> LedgerLines;
    bool bLedgerThreadHolds = false;
    void ShowChapterLedger(const FString& Event);
    void Autosave();
    void PresentVNStep();
    TArray<FString> ArmedStoryBeats;
    TWeakObjectPtr<UWorld> StoryWorld;
    void ArmStoryBeats();
    bool StartStoryField(const FString& Group, const TCHAR* Asset, const TCHAR* File = TEXT("data/chapter2_dialogue.json"));
    bool HandleSumpLedger(const FString& Point);
    void Notice(const FString& Text);
    bool bTraderArmed = false, bLedgerArmed = false;
    bool bEliaTalkCached = false; // companion.gd _talked_keys: per loaded Verdan, not saved.
    UPROPERTY(Transient) TObjectPtr<UMemoriaFieldAsset> EliaTalkAsset;
    UPROPERTY(Transient) TObjectPtr<UMemoriaFieldAsset> EliaRepeatAsset;
    TArray<FString> Notices;
    double NoticeTime = -1000;
    TWeakObjectPtr<UWorld> NoticeWorld;
    bool bNoticeHeld = false; // Toasts raised before a Field start counting when exploration resumes.
    // Source reward listener is one-shot and synchronous. This owner binds only
    // the flag, world seed and three item grants, then opens the bounded shop presentation.
    TWeakObjectPtr<UWorld> RewardCallbackWorld;
    FGuid RewardCallbackRunId;
    int32 RewardCompletionCount = 0, RewardCallbackIntentCount = 0, RewardFieldInvocationCount = 0;
    void CommitRewardFlagAndDeferSeed();
    TArray<FMemoriaWorldSnapshot> SeedObservations;
    TArray<FMemoriaRunSnapshot> PotionObservations;
    int32 PresentedPotionObservation = INDEX_NONE;
    FString PotionToast;
    TArray<FString> RewardToasts;
    TArray<FMemoriaRunSnapshot> AntidoteObservations;
    int32 PresentedAntidoteObservation = INDEX_NONE;
    void CommitAntidoteAndDeferFirebomb(); // Historical method name; preserves the antidote contract seam.
    void CommitFirebombAndDeferShop();
    TArray<FMemoriaRunSnapshot> FirebombObservations;
    int32 PresentedFirebombObservation = INDEX_NONE;
    int32 PresentedSeedObservation = INDEX_NONE;
    bool HasLiveRewardOwner() const;
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
    bool bCheckpointScreen = false, bCheckpointLoadFailed = false;
    bool bPendingVerdanReentry = false;
    FGuid RevisitRunId;
    TWeakObjectPtr<UWorld> RevisitWorld;
    FVector2D ReentryPosition = FVector2D(128,288);
    TWeakObjectPtr<UWorld> CheckpointWorld;
    bool bPaused = false;
    int32 Revision = 0, EventCursor = 0, FieldInvocationCount = 0;
    TArray<FString> Trace;
    void Reset();
    void ShopChanged();
    bool LoadContracts();
    void FlushEvents(const FString& Dialect);
    void AfterVN();
    void Explore();
};
