#pragma once
#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "MemoriaHeroS361Commandlet.generated.h"
UCLASS()
class UMemoriaHeroS361Commandlet : public UCommandlet
{
 GENERATED_BODY()
public:
 UMemoriaHeroS361Commandlet();
 virtual int32 Main(const FString& Params) override;
};
