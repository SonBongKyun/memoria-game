#pragma once
#include "Commandlets/Commandlet.h"
#include "MemoriaFoundationAssetsCommandlet.generated.h"

// Explicit editor-only authoring operation. Never imports campaign content.
UCLASS()
class UMemoriaFoundationAssetsCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UMemoriaFoundationAssetsCommandlet();
    virtual int32 Main(const FString& Params) override;
};
