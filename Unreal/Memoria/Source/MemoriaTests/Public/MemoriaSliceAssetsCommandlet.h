#pragma once
#include "Commandlets/Commandlet.h"
#include "MemoriaSliceAssetsCommandlet.generated.h"
UCLASS()
class UMemoriaSliceAssetsCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UMemoriaSliceAssetsCommandlet();
    virtual int32 Main(const FString& Params) override;
};
