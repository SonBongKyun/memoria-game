#pragma once
#include "CoreMinimal.h"
struct FMemoriaRunSnapshot;

// Read-only guidance for the current field and already-revealed progression.
// Map is an actual region ID, not a future destination inferred from the chapter number.
namespace MemoriaNavigationGoal
{
    MEMORIA_API FString Line(const FMemoriaRunSnapshot& Run, const FString& Map, bool bVerdanRevisit = false);
}
