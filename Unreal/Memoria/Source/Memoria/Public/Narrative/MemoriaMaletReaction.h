#pragma once
#include "CoreMinimal.h"
class UMemoriaRunSubsystem;
class UMemoriaFieldAsset;
struct MEMORIA_API FMemoriaMaletDispatch
{
    FString File, Group;
    bool bReaction = false;
    TArray<FString> Events;
};
namespace MemoriaMaletReaction
{
    inline constexpr const TCHAR* Food = TEXT("daily_market_food");
    inline constexpr const TCHAR* Group = TEXT("malet_taste_burned");
    inline constexpr const TCHAR* Heard = TEXT("burn_reaction_heard_malet_taste_burned");
    // Bounded npc.gd + PerceptionFilter contract. No normal dialogue is executed.
    MEMORIA_API FMemoriaMaletDispatch Resolve(UMemoriaRunSubsystem& Run, bool bDialogueActive, const UMemoriaFieldAsset* Reaction);
}
