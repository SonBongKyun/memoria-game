#pragma once
#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "MemoriaCharacterGeometryCommandlet.generated.h"
// S354: imports new geometry alongside the delivered meshes. Existing rigs/clips/paint stay intact.
UCLASS()
class UMemoriaCharacterGeometryCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UMemoriaCharacterGeometryCommandlet();
    virtual int32 Main(const FString& Params) override;
};
