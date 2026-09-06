#pragma once

#include "CoreMinimal.h"
#include "MemoriaNarrativeContracts.generated.h"

// These are distinct import/runtime contracts, not a unified interpreter.
UENUM()
enum class EMemoriaFieldEffectPhase : uint8 { AfterLineGate, BeforeChoiceCost, AfterChoiceCost };
UENUM()
enum class EMemoriaVNEffectPhase : uint8 { BeforeStepGate, AfterStepGate, AfterChoicePayment };

USTRUCT(BlueprintType)
struct MEMORIA_API FMemoriaNarrativeSource
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, SaveGame, BlueprintReadOnly) FString SourcePath;
    UPROPERTY(EditAnywhere, SaveGame, BlueprintReadOnly) FString SourceHash;
    UPROPERTY(EditAnywhere, SaveGame, BlueprintReadOnly) FString SequenceId;
    UPROPERTY(EditAnywhere, SaveGame, BlueprintReadOnly) int32 OriginalIndex = 0;
    UPROPERTY(EditAnywhere, SaveGame, BlueprintReadOnly) FString StableStepId;
    UPROPERTY(EditAnywhere, SaveGame, BlueprintReadOnly) FString TextId;
    UPROPERTY(EditAnywhere, SaveGame, BlueprintReadOnly) int32 IndexMappingVersion = 1;
};

USTRUCT()
struct MEMORIA_API FFieldDialogueLine
{
    GENERATED_BODY()
    UPROPERTY() FMemoriaNarrativeSource Source;
    // Filtered choice ordinal, never the original VN choice index.
    UPROPERTY() int32 FilteredChoiceIndex = INDEX_NONE;
    UPROPERTY() EMemoriaFieldEffectPhase EffectPhase = EMemoriaFieldEffectPhase::AfterLineGate;
};

USTRUCT()
struct MEMORIA_API FVNStoryStep
{
    GENERATED_BODY()
    UPROPERTY() FMemoriaNarrativeSource Source;
    UPROPERTY() int32 OriginalChoiceIndex = INDEX_NONE;
    UPROPERTY() EMemoriaVNEffectPhase EffectPhase = EMemoriaVNEffectPhase::BeforeStepGate;
};

USTRUCT(BlueprintType)
struct MEMORIA_API FMemoriaVNCursor
{
    GENERATED_BODY()
    UPROPERTY(SaveGame, BlueprintReadOnly) FString SequenceId;
    UPROPERTY(SaveGame, BlueprintReadOnly) int32 OriginalIndex = 0;
};

USTRUCT(BlueprintType)
struct MEMORIA_API FMemoriaVNContinuation
{
    GENERATED_BODY()
    UPROPERTY(SaveGame, BlueprintReadOnly) int32 SchemaVersion = 1;
    UPROPERTY(SaveGame, BlueprintReadOnly) int32 IndexMappingVersion = 1;
    UPROPERTY(SaveGame, BlueprintReadOnly) FMemoriaVNCursor Current;
    UPROPERTY(SaveGame, BlueprintReadOnly) FMemoriaVNCursor Pending;
    UPROPERTY(SaveGame, BlueprintReadOnly) TArray<FMemoriaVNCursor> ResumeQueue;
    UPROPERTY(SaveGame, BlueprintReadOnly) bool bActive = false;
    UPROPERTY(SaveGame, BlueprintReadOnly) int64 LedgerBurnSnapshot = 0;
};

// Source-index validation/effect interpretation belongs to Phase 1B+ importers.
// Separate signatures prevent field filtered choices from entering the VN API.
class MEMORIA_API IMemoriaFieldDialogueRuntime
{
public:
    virtual ~IMemoriaFieldDialogueRuntime() = default;
    virtual void AdvanceFieldLine(const FFieldDialogueLine& Line) = 0;
    virtual void SelectFilteredChoice(int32 FilteredIndex) = 0;
};
class MEMORIA_API IMemoriaVNRuntime
{
public:
    virtual ~IMemoriaVNRuntime() = default;
    virtual void ExecuteVNStep(const FVNStoryStep& Step) = 0;
    virtual void SelectOriginalChoice(int32 OriginalIndex) = 0;
    virtual FMemoriaVNContinuation ExportContinuation() const = 0;
};
