#pragma once
#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "MemoriaHumanoidS357Commandlet.generated.h"

UCLASS()
class UMemoriaHumanoidS357Commandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UMemoriaHumanoidS357Commandlet();
    virtual int32 Main(const FString& Params) override;
};
