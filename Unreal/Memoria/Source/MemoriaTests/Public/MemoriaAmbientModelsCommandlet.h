#pragma once
#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "MemoriaAmbientModelsCommandlet.generated.h"

// S335: imports Codex's S334 ambient models from Unreal/ArtSource: the three NPCs of the Belt Waystation's
// revisit (traveler, bureau agent, guard; the same 77-bone rig as Arrel) and the three props of the chapter
// maps (water tank, campfire, rubble). The NPCs get a body material in the leads' style and an idle and a
// walk retargeted from Epic's mannequin, named as the field figure looks for them (A_<Name>_Idle, _Walk), so
// UMemoriaFieldCharacterComponent::InitializeCharacter finds the model before the pixel card.
// Additive: existing assets are kept. Run after Unreal/Tools/install_mannequin.py.
UCLASS()
class UMemoriaAmbientModelsCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UMemoriaAmbientModelsCommandlet();
    virtual int32 Main(const FString& Params) override;
};
