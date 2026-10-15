#pragma once
#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "MemoriaHumanoidS359Commandlet.generated.h"
UCLASS()
class UMemoriaHumanoidS359Commandlet : public UCommandlet
{
 GENERATED_BODY()
public:
 UMemoriaHumanoidS359Commandlet();
 virtual int32 Main(const FString& Params) override;
};
