#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Run/MemoriaRunTypes.h"
#include "Domain/MemoriaMemoryTypes.h"
#include "Narrative/MemoriaNarrativeContracts.h"
#include "MemoriaRunSaveGame.generated.h"

// Opaque deferred sections keep future import data distinct. Phase 1A does not
// parse/apply these or claim legacy save compatibility. Empty means absent.
USTRUCT()
struct MEMORIA_API FMemoriaWorldCognitionSaveSection
{
    GENERATED_BODY()
    UPROPERTY(SaveGame) int32 SchemaVersion = 1;
    UPROPERTY(SaveGame) FString SourceJson;
};
USTRUCT()
struct MEMORIA_API FMemoriaDiarySaveSection
{
    GENERATED_BODY()
    UPROPERTY(SaveGame) int32 SchemaVersion = 0;
    UPROPERTY(SaveGame) FString SourceJson;
};
USTRUCT()
struct MEMORIA_API FMemoriaHintsSaveSection
{
    GENERATED_BODY()
    UPROPERTY(SaveGame) int32 SchemaVersion = 0;
    UPROPERTY(SaveGame) FString SourceJson;
};

USTRUCT(BlueprintType)
struct MEMORIA_API FMemoriaFieldReturnLocation
{
    GENERATED_BODY()
    UPROPERTY(SaveGame, BlueprintReadOnly) FString SourceScenePath;
    UPROPERTY(SaveGame, BlueprintReadOnly) FString MapId;
    UPROPERTY(SaveGame, BlueprintReadOnly) FVector2D SourcePixelPosition = FVector2D::ZeroVector;
    UPROPERTY(SaveGame, BlueprintReadOnly) int32 CoordinateVersion = 1;
};

// DTO only. No disk writer, legacy JSON importer, actor, battle session or widget.
UCLASS()
class MEMORIA_API UMemoriaRunSaveGame : public USaveGame
{
    GENERATED_BODY()
public:
    static constexpr int32 CurrentSchemaVersion = 1;
    static constexpr const TCHAR* LegacyGodotVersion = TEXT("0.4.0");
    UPROPERTY(SaveGame) int32 SchemaVersion = CurrentSchemaVersion;
    UPROPERTY(SaveGame) FString ImportedFromVersion;
    UPROPERTY(SaveGame) int32 Slot = 0;
    UPROPERTY(SaveGame) FDateTime SavedAtUtc;
    UPROPERTY(SaveGame) FString ContentRevision;
    UPROPERTY(SaveGame) FPrimaryAssetId MemoryCatalogId;
    UPROPERTY(SaveGame) FMemoriaRunSnapshot Run;
    // Definitions in owned order retain synthesized/acquired instance definitions.
    // Matching catalog revision is required by the future save coordinator.
    UPROPERTY(SaveGame) TArray<FMemoriaMemoryDefinition> MemoryDefinitions;
    UPROPERTY(SaveGame) FMemoriaMemorySnapshot PlayerMemory;
    UPROPERTY(SaveGame) FMemoriaWorldCognitionSaveSection WorldCognition;
    UPROPERTY(SaveGame) FMemoriaVNContinuation SceneFlow;
    UPROPERTY(SaveGame) FMemoriaDiarySaveSection Diary;
    UPROPERTY(SaveGame) FMemoriaHintsSaveSection Hints;
    UPROPERTY(SaveGame) FMemoriaFieldReturnLocation FieldReturn;

    static bool IsSupportedSlot(int32 Value) { return Value >= 0 && Value <= 3; }
    // Header validation only; not permission to restore arbitrary DTO sections.
    bool ValidateHeader(FString& OutError) const;
};
