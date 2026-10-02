#pragma once
#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "MemoriaEnvironmentAssetsCommandlet.generated.h"

// S344: imports Codex's S343 environment kit from Unreal/ArtSource/Environment: the twelve static models the
// Belt Waystation's and Drift Shelter's canvases paint (signal post, platform shelter, crates, chain fence,
// lantern post, rail, tarp, ruined wall, gramophone, dead tree, banner pole, dry grass). The models share four
// colour atlases and two glow masks, which are imported once each, and wear one material, M_EnvProp: the atlas,
// a warm glow through the mask, the grass's alpha, and the opening round Arrel that M_FocusSurface has.
// Additive: existing meshes and textures are kept. -Rebuild writes the material again.
UCLASS()
class UMemoriaEnvironmentAssetsCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UMemoriaEnvironmentAssetsCommandlet();
    virtual int32 Main(const FString& Params) override;
};
