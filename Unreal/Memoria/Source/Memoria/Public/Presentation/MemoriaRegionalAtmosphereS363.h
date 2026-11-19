#pragma once
#include "CoreMinimal.h"

class AActor;
class UMaterialInstanceDynamic;
struct FMemoriaChapterMapSpec;

// Additive presentation only: no map data, collision, navigation, lights or RNG.
// Returned siblings join the owner's existing camera-reveal uniform updates.
namespace MemoriaRegionalAtmosphereS363
{
    struct FResult
    {
        TArray<UMaterialInstanceDynamic*> Materials;
        int32 SolidInstances = 0;
        int32 SoftPlanes = 0;
    };
    MEMORIA_API bool IsEnabled();
    MEMORIA_API FResult BuildVerdan(AActor& Owner, UMaterialInstanceDynamic* Stone, UMaterialInstanceDynamic* Paving);
    MEMORIA_API FResult BuildChapter(AActor& Owner, const FMemoriaChapterMapSpec& Spec, const FString& Map, UMaterialInstanceDynamic* Stone);
}
