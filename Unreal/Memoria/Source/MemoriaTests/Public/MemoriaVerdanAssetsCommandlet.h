#pragma once
#include "Commandlets/Commandlet.h"
#include "MemoriaVerdanAssetsCommandlet.generated.h"
UCLASS()
class UMemoriaVerdanAssetsCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UMemoriaVerdanAssetsCommandlet();
    virtual int32 Main(const FString& Params) override;
};
