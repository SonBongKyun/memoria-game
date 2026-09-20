#pragma once
#include "Commandlets/Commandlet.h"
#include "MemoriaArrelRefinedAssetsCommandlet.generated.h"
UCLASS()
class UMemoriaArrelRefinedAssetsCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UMemoriaArrelRefinedAssetsCommandlet();
    virtual int32 Main(const FString& Params) override;
};
