#pragma once
#include "Commandlets/Commandlet.h"
#include "MemoriaFoeAssetsCommandlet.generated.h"
// S313: the field foes' stand-in assets until Codex's monster models arrive.
// - /Game/Memoria/Presentation/Combat/M_FieldFoe: a lit skeletal-mesh material (the engine's BasicShapeMaterial
//   lacks the skeletal usage flag, so the tinted mannequin rendered with the default material). Parameters:
//   Color, Glow, CrackStrength, RimStrength (violet void cracks fixed to the body, a rim light) and Hit/HitColor
//   (the flash when struck).
// - /Game/Memoria/Presentation/Combat/Foes/A_Mannequin_<Clip>: UAL2's zombie and one-handed cuts retargeted onto
//   Epic's mannequin (Manny and Quinn share its skeleton). Needs install_mannequin.py and the UAL2 import from
//   -run=MemoriaSwordRetarget. Additive; regenerate from deleted outputs.
UCLASS()
class UMemoriaFoeAssetsCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UMemoriaFoeAssetsCommandlet();
    virtual int32 Main(const FString& Params) override;
};
