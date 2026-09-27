#pragma once
#include "Commandlets/Commandlet.h"
#include "MemoriaBattleStageAssetsCommandlet.generated.h"
// Authors M_BattlePlate: battle_stage_blend.gdshader as a UI material for the battle stage plates.
UCLASS()
class UMemoriaBattleStageAssetsCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UMemoriaBattleStageAssetsCommandlet();
    virtual int32 Main(const FString& Params) override;
};
