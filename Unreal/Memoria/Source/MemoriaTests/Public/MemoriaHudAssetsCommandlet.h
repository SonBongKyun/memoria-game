#pragma once
#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "MemoriaHudAssetsCommandlet.generated.h"

// S339: imports the field HUD's plates (Unreal/ArtSource/Hud, cut by Unreal/Tools/export_hud_art.py from the
// source's UI paintings) to Content/Memoria/Presentation/Hud: T_HudPlate, T_HudToast, T_HudRibbon.
// Additive: existing textures are kept. -Refresh imports them again.
UCLASS()
class UMemoriaHudAssetsCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UMemoriaHudAssetsCommandlet();
    virtual int32 Main(const FString& Params) override;
};
