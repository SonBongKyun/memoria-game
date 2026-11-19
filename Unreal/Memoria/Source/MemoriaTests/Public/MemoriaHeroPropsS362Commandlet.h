#pragma once
#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "MemoriaHeroPropsS362Commandlet.generated.h"
UCLASS()
class UMemoriaHeroPropsS362Commandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UMemoriaHeroPropsS362Commandlet();
    virtual int32 Main(const FString& Params) override;
};
