#pragma once
#include "Commandlets/Commandlet.h"
#include "MemoriaStartingCatalogCommandlet.generated.h"
UCLASS()
class UMemoriaStartingCatalogCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UMemoriaStartingCatalogCommandlet();
    virtual int32 Main(const FString& Params) override;
};
