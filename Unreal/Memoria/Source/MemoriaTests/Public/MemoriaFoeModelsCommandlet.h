#pragma once
#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "MemoriaFoeModelsCommandlet.generated.h"

// S326: imports Codex's S313 foe models (the void husk and the market thief with its dagger) from
// Unreal/ArtSource/FieldCharacters, builds their materials (the husk's violet cracks from its emissive
// mask; the Hit flash for both) and retargets the mannequin foe clips onto them, so the field foes stop
// being the tinted mannequin. Additive: existing assets are kept. Run after -run=MemoriaFoeAssets.
UCLASS()
class UMemoriaFoeModelsCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UMemoriaFoeModelsCommandlet();
    virtual int32 Main(const FString& Params) override;
};
