#pragma once
#include "Commandlets/Commandlet.h"
#include "MemoriaAfterimageAssetsCommandlet.generated.h"
// S366: authors /Game/Memoria/Presentation/Combat/M_Afterimage, the translucent unlit material of the ghosts a
// dash leaves (Fresnel-lit rim, Tint and Alpha parameters, usable on skeletal meshes). Additive: refuses to
// rewrite an existing package.
UCLASS()
class UMemoriaAfterimageAssetsCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UMemoriaAfterimageAssetsCommandlet();
    virtual int32 Main(const FString& Params) override;
};
