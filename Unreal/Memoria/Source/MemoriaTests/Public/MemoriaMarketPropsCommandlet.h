#pragma once
#include "Commandlets/Commandlet.h"
#include "MemoriaMarketPropsCommandlet.generated.h"

/** Additive, deterministic authoring of the four existing market interactives. */
UCLASS()
class UMemoriaMarketPropsCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UMemoriaMarketPropsCommandlet();
    virtual int32 Main(const FString& Params) override;
};
