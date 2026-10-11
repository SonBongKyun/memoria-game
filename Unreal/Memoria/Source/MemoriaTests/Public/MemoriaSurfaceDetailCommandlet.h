#pragma once
#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "MemoriaSurfaceDetailCommandlet.generated.h"
// Rebuilds only the fourteen existing field body materials and eight surface-data textures.
// Does not import or save meshes, skeletons, animations, original colour or emissive textures.
UCLASS()
class UMemoriaSurfaceDetailCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UMemoriaSurfaceDetailCommandlet();
    virtual int32 Main(const FString& Params) override;
};
