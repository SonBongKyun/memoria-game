#pragma once
#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "MemoriaItemArtAssetsCommandlet.generated.h"

// S356: imports the six original transparent paintings from ArtSource/ItemsS356.
// Immutable authoring: refuses to replace any existing target package.
UCLASS()
class UMemoriaItemArtAssetsCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UMemoriaItemArtAssetsCommandlet();
    virtual int32 Main(const FString& Params) override;
};
