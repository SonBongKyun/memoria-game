#pragma once
#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "MemoriaArrelSoleS362Commandlet.generated.h"
UCLASS()
class UMemoriaArrelSoleS362Commandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UMemoriaArrelSoleS362Commandlet();
    virtual int32 Main(const FString& Params) override;
};