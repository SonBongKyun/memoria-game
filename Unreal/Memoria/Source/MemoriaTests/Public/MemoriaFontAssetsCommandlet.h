#pragma once
#include "Commandlets/Commandlet.h"
#include "MemoriaFontAssetsCommandlet.generated.h"
// Imports the static font instances written by Unreal/Tools/generate_font_sources.py.
UCLASS()
class UMemoriaFontAssetsCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UMemoriaFontAssetsCommandlet();
    virtual int32 Main(const FString& Params) override;
};
