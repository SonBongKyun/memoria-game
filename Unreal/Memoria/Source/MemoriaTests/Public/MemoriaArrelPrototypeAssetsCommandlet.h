#pragma once
#include "Commandlets/Commandlet.h"
#include "MemoriaArrelPrototypeAssetsCommandlet.generated.h"
UCLASS()
class UMemoriaArrelPrototypeAssetsCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UMemoriaArrelPrototypeAssetsCommandlet();
    virtual int32 Main(const FString& Params) override;
};
