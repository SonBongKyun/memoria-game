#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MemoriaMemoryTypes.generated.h"

// Storage order is a save contract. Display rank is RawGrade + 1.
UENUM(BlueprintType)
enum class EMemoriaMemoryGrade : uint8
{
    Grade5 = 0, Grade4 = 1, Grade3 = 2, Grade2 = 3, Grade1 = 4
};
UENUM(BlueprintType)
enum class EMemoriaBurnMode : uint8 { Normal, Silent };
UENUM(BlueprintType)
enum class EMemoriaMemoryResult : uint8 { Success, Missing, AlreadyBurned, Faded, Collateral, Busy, InvalidSnapshot };
UENUM(BlueprintType)
enum class EMemoriaMemoryEventKind : uint8 { Added, ResidueCreated, Faded, Cascaded, Burned, CarryChanged, PassiveUnlocked, MemoriesEroded };

USTRUCT(BlueprintType)
struct MEMORIA_API FMemoriaMemoryDefinition
{
    GENERATED_BODY()
    // FString is intentional: legacy IDs are case-sensitive; FName is not.
    UPROPERTY(EditAnywhere, SaveGame, BlueprintReadOnly) FString Id;
    UPROPERTY(EditAnywhere, SaveGame, BlueprintReadOnly) FString Title;
    UPROPERTY(EditAnywhere, SaveGame, BlueprintReadOnly) FString Description;
    UPROPERTY(EditAnywhere, SaveGame, BlueprintReadOnly) EMemoriaMemoryGrade RawGrade = EMemoriaMemoryGrade::Grade5;
    UPROPERTY(EditAnywhere, SaveGame, BlueprintReadOnly) int64 BurnPower = 0;
    UPROPERTY(EditAnywhere, SaveGame, BlueprintReadOnly) FString StoryEffect;
    UPROPERTY(EditAnywhere, SaveGame, BlueprintReadOnly) FString RelatedNpc;
    // Provenance/localization keys; no manually copied campaign rows in C++.
    UPROPERTY(EditAnywhere, SaveGame, BlueprintReadOnly) FString SourcePath;
    UPROPERTY(EditAnywhere, SaveGame, BlueprintReadOnly) FString SourceHash;
    UPROPERTY(EditAnywhere, SaveGame, BlueprintReadOnly) FString TextId;
};

USTRUCT(BlueprintType)
struct MEMORIA_API FMemoriaMemoryState
{
    GENERATED_BODY()
    UPROPERTY(SaveGame, BlueprintReadOnly) FString Id;
    UPROPERTY(SaveGame, BlueprintReadOnly) bool bBurned = false;
    UPROPERTY(SaveGame, BlueprintReadOnly) bool bResidue = false;
    UPROPERTY(SaveGame, BlueprintReadOnly) bool bFaded = false;
    UPROPERTY(SaveGame, BlueprintReadOnly) int64 Erosion = 0;
    // Derived in catalog order. Recomputed on restore, not authoritative save data.
    UPROPERTY(Transient, BlueprintReadOnly) TArray<FString> Connections;
};

USTRUCT(BlueprintType)
struct MEMORIA_API FMemoriaMemoryFlag
{
    GENERATED_BODY()
    UPROPERTY(SaveGame, BlueprintReadOnly) FString Id;
    UPROPERTY(SaveGame, BlueprintReadOnly) bool bValue = true;
};

USTRUCT(BlueprintType)
struct MEMORIA_API FMemoriaMemoryLoan
{
    GENERATED_BODY()
    UPROPERTY(SaveGame, BlueprintReadOnly) bool bActive = false;
    UPROPERTY(SaveGame, BlueprintReadOnly) FString MemoryId;
    UPROPERTY(SaveGame, BlueprintReadOnly) int64 Principal = 0;
    UPROPERTY(SaveGame, BlueprintReadOnly) int64 Repay = 0;
    UPROPERTY(SaveGame, BlueprintReadOnly) int64 DueChapter = 0;
};

USTRUCT(BlueprintType)
struct MEMORIA_API FMemoriaMemorySnapshot
{
    GENERATED_BODY()
    UPROPERTY(SaveGame, BlueprintReadOnly) TArray<FMemoriaMemoryState> Owned;
    UPROPERTY(SaveGame, BlueprintReadOnly) TArray<FString> BurnedHistory;
    UPROPERTY(SaveGame, BlueprintReadOnly) TArray<FMemoriaMemoryFlag> BurnPassives;
    UPROPERTY(SaveGame, BlueprintReadOnly) int64 AnchorVigil = 0;
    UPROPERTY(SaveGame, BlueprintReadOnly) TArray<FMemoriaMemoryFlag> AnchorPassives;
    UPROPERTY(SaveGame, BlueprintReadOnly) TArray<int64> VigilChapters;
    UPROPERTY(SaveGame, BlueprintReadOnly) TArray<FString> ErosionGuarded;
    UPROPERTY(SaveGame, BlueprintReadOnly) int64 GuardSlotsUsed = 0;
    UPROPERTY(SaveGame, BlueprintReadOnly) FMemoriaMemoryLoan ActiveLoan;
    UPROPERTY(SaveGame, BlueprintReadOnly) TArray<FString> Extracted;
};

USTRUCT(BlueprintType)
struct MEMORIA_API FMemoriaMemoryContext
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) int64 CurrentChapter = 1;
    UPROPERTY(BlueprintReadOnly) bool bEliaWithParty = true;
    UPROPERTY(BlueprintReadOnly) bool bStillHandsActive = false;
};

USTRUCT(BlueprintType)
struct MEMORIA_API FMemoriaMemoryEvent
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) EMemoriaMemoryEventKind Kind = EMemoriaMemoryEventKind::Added;
    UPROPERTY(BlueprintReadOnly) FString MemoryId;
    UPROPERTY(BlueprintReadOnly) TArray<FString> AffectedIds;
    UPROPERTY(BlueprintReadOnly) int64 Amount = 0;
    UPROPERTY(BlueprintReadOnly) int64 CarryWeight = 0;
    UPROPERTY(BlueprintReadOnly) int64 CarryCapacity = 0;
    UPROPERTY(BlueprintReadOnly) FString PassiveId;
    UPROPERTY(BlueprintReadOnly) FString PassiveName;
};

USTRUCT(BlueprintType)
struct MEMORIA_API FMemoriaCatalogSource
{
    GENERATED_BODY()
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FString Path;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FString Sha256Utf8Lf;
};

// Authored localization is catalog content, not mutable run/save state.
USTRUCT(BlueprintType)
struct MEMORIA_API FMemoriaMemoryLocalizedText
{
    GENERATED_BODY()
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FString MemoryId;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FString Locale;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FString Title;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FString Description;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bHasStoryEffect = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FString StoryEffect;
};

UCLASS(BlueprintType)
class MEMORIA_API UMemoriaMemoryCatalog : public UPrimaryDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, SaveGame, BlueprintReadOnly) FString ContentRevision;
    UPROPERTY(EditAnywhere, SaveGame, BlueprintReadOnly) TArray<FMemoriaMemoryDefinition> Definitions;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 CatalogSchemaVersion = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FString ContentKind;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FString ExtractorVersion;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FString SourceRepositoryRevision;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TArray<FMemoriaCatalogSource> Sources;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FString SourceIrSha256;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FString SemanticSha256;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TArray<FMemoriaMemoryLocalizedText> LocalizedText;

    virtual FPrimaryAssetId GetPrimaryAssetId() const override
    {
        return FPrimaryAssetId(TEXT("MemoriaMemoryCatalog"), GetFName());
    }
};
