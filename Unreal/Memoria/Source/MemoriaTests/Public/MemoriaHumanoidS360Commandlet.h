#pragma once
#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "MemoriaHumanoidS360Commandlet.generated.h"
UCLASS()
class UMemoriaHumanoidS360Commandlet : public UCommandlet
{
 GENERATED_BODY()
public:
 UMemoriaHumanoidS360Commandlet();
 virtual int32 Main(const FString& Params) override;
};
