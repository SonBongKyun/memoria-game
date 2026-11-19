#pragma once
#include "CoreMinimal.h"
class UWorld;
namespace MemoriaPerformanceInventoryS362
{
    // Call in the actual rendered PIE world after assets/components are loaded.
    // Writes inspection data only; changes no asset, LOD, streaming or cvar.
    bool WriteWorld(UWorld* World,const FString& File);
}
