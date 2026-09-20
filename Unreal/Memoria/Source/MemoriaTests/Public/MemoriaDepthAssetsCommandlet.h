#pragma once
#include "Commandlets/Commandlet.h"
#include "MemoriaDepthAssetsCommandlet.generated.h"
UCLASS()
class UMemoriaDepthAssetsCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UMemoriaDepthAssetsCommandlet();
    virtual int32 Main(const FString& Params) override;
};
