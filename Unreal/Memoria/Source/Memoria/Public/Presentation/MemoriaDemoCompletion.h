#pragma once
#include "CoreMinimal.h"
#include "Run/MemoriaRunTypes.h"
class UMemoriaRunSubsystem;

// Presentation derived from the existing Chapter 5 boundary. No new canon, save flag or chapter authority.
struct MEMORIA_API FMemoriaDemoCompletionView
{
    FString Title, Summary, Review;
    int32 Held = 0, Burned = 0;
};
namespace MemoriaDemoCompletion
{
    MEMORIA_API bool IsComplete(const FMemoriaRunSnapshot& Run);
    MEMORIA_API bool CanPresent(const FMemoriaRunSnapshot& Run, const FString& Map, bool bExploring, bool bCompletionCardShowing);
    MEMORIA_API FMemoriaDemoCompletionView Build(const UMemoriaRunSubsystem& Run, bool bKo);
}
