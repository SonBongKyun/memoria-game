#pragma once
#include "Commandlets/Commandlet.h"
#include "MemoriaNarrativeCommandlet.generated.h"
UCLASS()
class UMemoriaNarrativeCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UMemoriaNarrativeCommandlet();
    virtual int32 Main(const FString& Params) override;
};
