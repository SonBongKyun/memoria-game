#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Run/MemoriaRunTypes.h"
#include "Domain/MemoriaPlayerMemoryDomain.h"
#include "MemoriaRunSubsystem.generated.h"

DECLARE_MULTICAST_DELEGATE(FMemoriaRunReplaced);

// Owns persistent player state. ActorRegistry/WorldState cognition belongs to a
// different future domain: no player burn -> NPC memory deletion bridge exists.
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
    EMemoriaMemoryResult RestoreRun(const FMemoriaRunSnapshot& Run, const TArray<FMemoriaMemoryDefinition>& Definitions, const FMemoriaMemorySnapshot& Memory);
    EMemoriaMemoryResult BurnMemory(const FString& Id, EMemoriaBurnMode Mode = EMemoriaBurnMode::Normal, bool bAllowFaded = false);
    EMemoriaMemoryResult AcquireMemory(const FMemoriaMemoryDefinition& Definition);
    EMemoriaMemoryResult ErodeMemories(int64 ChapterArgument);
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
    UPROPERTY(Transient) FMemoriaRunSnapshot State;
};
