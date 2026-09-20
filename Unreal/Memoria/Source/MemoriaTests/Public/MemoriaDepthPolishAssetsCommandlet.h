#pragma once
#include "Commandlets/Commandlet.h"
#include "MemoriaDepthPolishAssetsCommandlet.generated.h"
UCLASS()
class UMemoriaDepthPolishAssetsCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UMemoriaDepthPolishAssetsCommandlet();
    virtual int32 Main(const FString& Params) override;
};
