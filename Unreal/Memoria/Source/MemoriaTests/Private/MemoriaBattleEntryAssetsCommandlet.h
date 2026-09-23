#pragma once
#include "Commandlets/Commandlet.h"
#include "MemoriaBattleEntryAssetsCommandlet.generated.h"
UCLASS()
class UMemoriaBattleEntryAssetsCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UMemoriaBattleEntryAssetsCommandlet();
    virtual int32 Main(const FString& Params) override;
};
