#pragma once
#include "Commandlets/Commandlet.h"
#include "MemoriaEnvironmentS356Commandlet.generated.h"

// Immutable architecture revision. Run explicitly after the source atlas is delivered.
UCLASS()
class UMemoriaEnvironmentS356Commandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UMemoriaEnvironmentS356Commandlet();
    virtual int32 Main(const FString& Params) override;
};
