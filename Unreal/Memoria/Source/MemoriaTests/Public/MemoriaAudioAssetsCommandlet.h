#pragma once
#include "Commandlets/Commandlet.h"
#include "MemoriaAudioAssetsCommandlet.generated.h"
UCLASS()
class UMemoriaAudioAssetsCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UMemoriaAudioAssetsCommandlet();
    virtual int32 Main(const FString& Params) override;
};
