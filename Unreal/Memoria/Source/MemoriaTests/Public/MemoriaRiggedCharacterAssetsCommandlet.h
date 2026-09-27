#pragma once
#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "MemoriaRiggedCharacterAssetsCommandlet.generated.h"
UCLASS()
class UMemoriaRiggedCharacterAssetsCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UMemoriaRiggedCharacterAssetsCommandlet();
    virtual int32 Main(const FString& Params) override;
};
