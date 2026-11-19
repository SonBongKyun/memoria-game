#pragma once
#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "MemoriaHeroS362Commandlet.generated.h"

UCLASS()
class UMemoriaHeroS362Commandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UMemoriaHeroS362Commandlet();
    virtual int32 Main(const FString& Params) override;
};
