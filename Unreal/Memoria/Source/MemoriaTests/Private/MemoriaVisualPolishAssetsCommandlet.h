#pragma once
#include "Commandlets/Commandlet.h"
#include "MemoriaVisualPolishAssetsCommandlet.generated.h"

/** Reproducible S337 surfaces; -Refresh is required before replacing any owned package. */
UCLASS()
class UMemoriaVisualPolishAssetsCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UMemoriaVisualPolishAssetsCommandlet();
    virtual int32 Main(const FString& Params) override;
};
