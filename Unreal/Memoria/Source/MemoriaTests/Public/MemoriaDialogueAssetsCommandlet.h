#pragma once
#include "Commandlets/Commandlet.h"
#include "MemoriaDialogueAssetsCommandlet.generated.h"
UCLASS()
class UMemoriaDialogueAssetsCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UMemoriaDialogueAssetsCommandlet();
    virtual int32 Main(const FString& Params) override;
};
