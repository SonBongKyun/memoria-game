#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Run/MemoriaRunTypes.h"
#include "Domain/MemoriaPlayerMemoryDomain.h"
#include "World/MemoriaWorldCognition.h"
#include "MemoriaRunSubsystem.generated.h"

class UMemoriaRunSaveGame;

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
    EMemoriaMemoryResult BeginRun(const UMemoriaMemoryCatalog& Catalog, const TArray<FString>& InitialIds);
    // Explicit New Game memory bootstrap from the offline-imported typed asset.
    // Does not travel, start narrative, or require editor/source tooling.
    EMemoriaMemoryResult BeginStartingMemoryRun();
    EMemoriaMemoryResult RestoreRun(const FMemoriaRunSnapshot& Run, const TArray<FMemoriaMemoryDefinition>& Definitions, const FMemoriaMemorySnapshot& Memory, const FMemoriaWorldSnapshot& World = UMemoriaWorldCognition::Defaults());
    UMemoriaWorldCognition* GetWorldCognition() const { return WorldCognition; }
    UMemoriaRunSaveGame* CaptureSave() const;
    bool RestoreSave(const UMemoriaRunSaveGame& Save);
    EMemoriaMemoryResult BurnMemory(const FString& Id, EMemoriaBurnMode Mode = EMemoriaBurnMode::Normal, bool bAllowFaded = false);
    EMemoriaMemoryResult AcquireMemory(const FMemoriaMemoryDefinition& Definition);
    EMemoriaMemoryResult ErodeMemories(int64 ChapterArgument);
    // Bounded source add_item contract. Only potion grants are authorized in Phase1L.
    bool AddRewardPotion(const FString& ItemId, int64 Count);
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
    // The interpreter borrows this stable aggregate; the host is a GI subsystem
    // and cancels its cursors synchronously on OnRunReplaced.
    friend class UMemoriaNarrativeSubsystem;
    UPROPERTY(Transient) TObjectPtr<UMemoriaPlayerMemoryDomain> PlayerMemory;
    UPROPERTY(Transient) TObjectPtr<UMemoriaWorldCognition> WorldCognition;
    UPROPERTY(Transient) FMemoriaRunSnapshot State;
};
