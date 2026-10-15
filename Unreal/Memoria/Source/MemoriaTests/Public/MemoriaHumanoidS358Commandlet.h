#pragma once
#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "MemoriaHumanoidS358Commandlet.generated.h"

UCLASS()
class UMemoriaHumanoidS358Commandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UMemoriaHumanoidS358Commandlet();
    virtual int32 Main(const FString& Params) override;
};
