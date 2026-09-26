#pragma once
#include "Commandlets/Commandlet.h"
#include "MemoriaFieldCharacterAssetsCommandlet.generated.h"
// Imports the illustrated field sets delivered per docs/unreal-migration/FIELD_SPRITE_ART_SPEC.md.
UCLASS()
class UMemoriaFieldCharacterAssetsCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UMemoriaFieldCharacterAssetsCommandlet();
    virtual int32 Main(const FString& Params) override;
};
