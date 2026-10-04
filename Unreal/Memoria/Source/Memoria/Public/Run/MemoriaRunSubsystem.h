#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Run/MemoriaRunTypes.h"
#include "Domain/MemoriaPlayerMemoryDomain.h"
#include "World/MemoriaWorldCognition.h"
#include "MemoriaRunSubsystem.generated.h"

class UMemoriaRunSaveGame;

enum class EMemoriaRewardItemScope { InvalidSourceId, DeferredByPhase, Supported };
DECLARE_MULTICAST_DELEGATE_ThreeParams(FMemoriaRewardItemObserved, const FString&, const FString&, const FMemoriaRunSnapshot&);
DECLARE_MULTICAST_DELEGATE(FMemoriaRunReplaced);
// Source signal payload is item_id only; quantity is read from committed run state.
DECLARE_MULTICAST_DELEGATE_OneParam(FMemoriaInventoryChanged, const FString&);
DECLARE_MULTICAST_DELEGATE_TwoParams(FMemoriaItemToastRequested, const FString&, int32);
// Value-only development observations, never a second inventory authority.
DECLARE_MULTICAST_DELEGATE_TwoParams(FMemoriaPotionObserved, const FString&, const FMemoriaRunSnapshot&);

// Owns independent player and world cognition domains across map travel.
UCLASS()
class MEMORIA_API UMemoriaRunSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    // Explicit catalog/starting IDs: no hidden content import or chapter advance.
    // StartChapter is the run's chapter at birth (New Game is 1); no chapter effects run.
    EMemoriaMemoryResult BeginRun(const UMemoriaMemoryCatalog& Catalog, const TArray<FString>& InitialIds, int64 StartChapter = 1);
    // Explicit New Game memory bootstrap from the offline-imported typed asset.
    // Does not travel, start narrative, or require editor/source tooling.
    EMemoriaMemoryResult BeginStartingMemoryRun(int64 StartChapter = 1);
    EMemoriaMemoryResult RestoreRun(const FMemoriaRunSnapshot& Run, const TArray<FMemoriaMemoryDefinition>& Definitions, const FMemoriaMemorySnapshot& Memory, const FMemoriaWorldSnapshot& World = UMemoriaWorldCognition::Defaults());
    UMemoriaWorldCognition* GetWorldCognition() const { return WorldCognition; }
    UMemoriaRunSaveGame* CaptureSave() const;
    bool RestoreSave(const UMemoriaRunSaveGame& Save);
    // S320 chapter maps: the chapter a departure moves the run to, and the rewards of a chest.
    void SetCurrentChapter(int64 Chapter) { if (HasActiveRun()) State.CurrentChapter = Chapter; }
    void AddGrains(int64 Amount) { if (HasActiveRun()) State.Player.Grains += Amount; }
    // S350: a rest (Verdan's campfire, MapEffects.add_interactive_prop): HP back by the amount, up to max.
    void RestoreHp(int64 Amount) { if (HasActiveRun()) State.Player.Hp = FMath::Min(State.Player.MaxHp, State.Player.Hp + FMath::Max<int64>(0, Amount)); }
    // S328: the source's battle_started statistic, counted as a field encounter begins.
    void RecordBattleStarted() { if (HasActiveRun() && State.TotalBattles < MAX_int64) ++State.TotalBattles; }
    void GrantFieldItem(const FString& ItemId, int64 Count);
    // S341: GameManager.remove_item. Takes one from the count and drops the entry at zero; false when there is none.
    bool ConsumeItem(const FString& ItemId);
    // S317: the options' language applies to the live run too.
    void SetLocale(const FString& Locale) { if (HasActiveRun()) State.CurrentLocale = Locale == TEXT("en") ? TEXT("en") : TEXT("ko"); }
    EMemoriaMemoryResult BurnMemory(const FString& Id, EMemoriaBurnMode Mode = EMemoriaBurnMode::Normal, bool bAllowFaded = false);
    EMemoriaMemoryResult AcquireMemory(const FMemoriaMemoryDefinition& Definition);
    // S330: MemoryManager.add_chapter_memories for the live run.
    EMemoriaMemoryResult AddChapterMemories(int64 Chapter);
    EMemoriaMemoryResult ErodeMemories(int64 ChapterArgument);
    // Bounded source add_item contract. Potion and antidote entry points retain distinct exact-ID contracts.
    bool AddRewardPotion(const FString& ItemId, int64 Count);
    bool AddRewardAntidote(const FString& ItemId, int64 Count);
    bool AddRewardFirebomb(const FString& ItemId, int64 Count);
    static EMemoriaRewardItemScope RewardItemScope(const FString& ItemId);
    TArray<FString> GetRecentItems() const;
    FMemoriaRewardItemObserved OnRewardItemObserved;
    int64 GetItemCount(const FString& ItemId) const;
    FMemoriaInventoryChanged OnInventoryChanged;
    FMemoriaItemToastRequested OnItemToastRequested;
    FMemoriaPotionObserved OnPotionObserved;
    bool RemoveStoryFlag(const FString& Id);
    bool SetStoryFlag(const FString& Id, bool bValue);
    UFUNCTION(BlueprintPure, Category="Memoria|Run") FMemoriaRunSnapshot GetRunSnapshot() const { return State; }
    UFUNCTION(BlueprintPure, Category="Memoria|Run") UMemoriaPlayerMemoryDomain* GetPlayerMemory() const { return PlayerMemory; }
    UFUNCTION(BlueprintPure, Category="Memoria|Run") bool HasActiveRun() const { return State.RunId.IsValid(); }
    FMemoriaMemoryContext GetMemoryContext() const;
    // After atomic replacement; restore emits no acquisition/burn rewards.
    FMemoriaRunReplaced OnRunReplaced;
private:
    bool GrantRewardItem(const FString& ItemId, const TCHAR* DisplayName, int64 Count);
    // The interpreter borrows this stable aggregate; the host is a GI subsystem
    // and cancels its cursors synchronously on OnRunReplaced.
    friend class UMemoriaNarrativeSubsystem;
    friend class UMemoriaShopSubsystem;
    friend class UMemoriaFieldCombatSubsystem;
    UPROPERTY(Transient) TObjectPtr<UMemoriaPlayerMemoryDomain> PlayerMemory;
    UPROPERTY(Transient) TObjectPtr<UMemoriaWorldCognition> WorldCognition;
    UPROPERTY(Transient) FMemoriaRunSnapshot State;
};
