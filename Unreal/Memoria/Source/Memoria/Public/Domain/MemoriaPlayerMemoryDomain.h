#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Domain/MemoriaMemoryTypes.h"
#include "Domain/MemoriaMemoryModel.h"
#include "MemoriaPlayerMemoryDomain.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FMemoriaMemoryObserved, const FMemoriaMemoryEvent&);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMemoriaMemoryPresentationEvent, const FMemoriaMemoryEvent&, Event);

// Run-owned domain object, not another global singleton and not world cognition.
UCLASS(BlueprintType)
class MEMORIA_API UMemoriaPlayerMemoryDomain : public UObject
{
    GENERATED_BODY()
public:
    UMemoriaPlayerMemoryDomain();
    virtual ~UMemoriaPlayerMemoryDomain() override;

    EMemoriaMemoryResult Restore(const TArray<FMemoriaMemoryDefinition>& Definitions, const FMemoriaMemorySnapshot& Snapshot);
    EMemoriaMemoryResult Add(const FMemoriaMemoryDefinition& Definition, const FMemoriaMemoryContext& Context);
    EMemoriaMemoryResult Burn(const FString& Id, EMemoriaBurnMode Mode, bool bAllowFaded, const FMemoriaMemoryContext& Context);
    EMemoriaMemoryResult ApplyErosion(int64 Chapter, const FMemoriaMemoryContext& Context);
    EMemoriaMemoryResult EvaluatePassives();

    UFUNCTION(BlueprintPure, Category="Memoria|Memory") FMemoriaMemorySnapshot GetSnapshot() const;
    UFUNCTION(BlueprintPure, Category="Memoria|Memory") EMemoriaMemoryResult CanBurn(const FString& Id, bool bAllowFaded = false) const;
    UFUNCTION(BlueprintPure, Category="Memoria|Memory") int64 GetEffectiveBurnPower(const FString& Id) const;
    UFUNCTION(BlueprintPure, Category="Memoria|Memory") int64 GetCarryWeight() const;
    UFUNCTION(BlueprintPure, Category="Memoria|Memory") int64 GetCarryCapacity(int64 CurrentChapter) const;
    UFUNCTION(BlueprintPure, Category="Memoria|Memory") double GetCarryOverload(const FMemoriaMemoryContext& Context) const;
    UFUNCTION(BlueprintPure, Category="Memoria|Memory") TArray<FString> GetAvailable(EMemoriaMemoryGrade MinimumGrade = EMemoriaMemoryGrade::Grade5, bool bAllowFaded = false) const;
    UFUNCTION(BlueprintPure, Category="Memoria|Memory") bool HasPassive(const FString& Id) const;
    UFUNCTION(BlueprintPure, Category="Memoria|Memory") bool HasResidue(const FString& Id) const;
    UFUNCTION(BlueprintPure, Category="Memoria|Memory") bool IsIntact(const FString& Id) const;

    const TArray<FMemoriaMemoryDefinition>& GetDefinitions() const { return DefinitionCatalog; }
    bool IsDispatchingEvent() const { return bDispatchingEvent; }
    // Callbacks may inspect intermediate snapshots; nested mutations return Busy.
    FMemoriaMemoryObserved OnObserved;
    UPROPERTY(BlueprintAssignable, Category="Memoria|Memory") FMemoriaMemoryPresentationEvent OnPresentationEvent;

private:
    TUniquePtr<Memoria::Memory::Model> Model;
    UPROPERTY() TArray<FMemoriaMemoryDefinition> DefinitionCatalog;
    bool bDispatchingEvent = false;
    void Publish(const FMemoriaMemoryEvent& Event);
};
