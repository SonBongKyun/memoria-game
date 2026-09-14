#pragma once
#include "Commandlets/Commandlet.h"
#include "MemoriaShopAssetsCommandlet.generated.h"
UCLASS()
class UMemoriaShopAssetsCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UMemoriaShopAssetsCommandlet();
    virtual int32 Main(const FString& Params) override;
};
