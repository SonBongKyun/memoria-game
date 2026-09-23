#pragma once

#include "CoreMinimal.h"
#include "MemoriaRunTypes.generated.h"

// Source get_flag/set_flag use bool; absence is distinct from a stored false.
USTRUCT(BlueprintType)
struct MEMORIA_API FMemoriaStoryFlag
{
    GENERATED_BODY()
    UPROPERTY(SaveGame, BlueprintReadOnly) FString Id;
    UPROPERTY(SaveGame, BlueprintReadOnly) bool bValue = false;
};

USTRUCT(BlueprintType)
struct MEMORIA_API FMemoriaItemCount
{
    GENERATED_BODY()
    UPROPERTY(SaveGame, BlueprintReadOnly) FString Id;
    UPROPERTY(SaveGame, BlueprintReadOnly) int64 Count = 0;
};

USTRUCT(BlueprintType)
struct MEMORIA_API FMemoriaPlayerState
{
    GENERATED_BODY()
    UPROPERTY(SaveGame, BlueprintReadOnly) FString Name = TEXT("Arrel");
    UPROPERTY(SaveGame, BlueprintReadOnly) int64 Hp = 100;
    UPROPERTY(SaveGame, BlueprintReadOnly) int64 MaxHp = 100;
    UPROPERTY(SaveGame, BlueprintReadOnly) int64 Grains = 0;
    UPROPERTY(SaveGame, BlueprintReadOnly) bool bEliaWithParty = true;
    UPROPERTY(SaveGame, BlueprintReadOnly) int64 FieldFocus = 0;
    UPROPERTY(SaveGame, BlueprintReadOnly) int64 DirectiveStreak = 0;
    UPROPERTY(SaveGame, BlueprintReadOnly) TArray<FMemoriaItemCount> Items;
    UPROPERTY(SaveGame, BlueprintReadOnly) TArray<FString> QuickSlots = {TEXT("potion"), TEXT("antidote"), TEXT("witness_ink")};
    UPROPERTY(SaveGame, BlueprintReadOnly) TArray<FString> RecentItems;
};

USTRUCT(BlueprintType)
struct MEMORIA_API FMemoriaRunSnapshot
{
    GENERATED_BODY()
    UPROPERTY(SaveGame, BlueprintReadOnly) FGuid RunId;
    UPROPERTY(SaveGame, BlueprintReadOnly) FString ContentRevision;
    UPROPERTY(SaveGame, BlueprintReadOnly) int64 CurrentChapter = 1;
    UPROPERTY(SaveGame, BlueprintReadOnly) FString CurrentLocale = TEXT("en");
    // Source play_stats; additive SaveGame fields default to zero for older saves.
    UPROPERTY(SaveGame, BlueprintReadOnly) int64 TotalBattles = 0;
    UPROPERTY(SaveGame, BlueprintReadOnly) int64 HighestMomentumRank = 0;
    UPROPERTY(SaveGame, BlueprintReadOnly) FMemoriaPlayerState Player;
    // Includes canon_* progression flags verbatim; no second chapter authority.
    UPROPERTY(SaveGame, BlueprintReadOnly) TArray<FMemoriaStoryFlag> StoryFlags;
    bool HasFlag(const FString& Id) const;
    bool GetFlag(const FString& Id) const;
    bool IsValid() const;
};
