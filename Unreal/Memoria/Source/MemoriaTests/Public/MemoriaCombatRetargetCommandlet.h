#pragma once
#include "Commandlets/Commandlet.h"
#include "MemoriaCombatRetargetCommandlet.generated.h"
// S311: retargets the UE 5.8 mannequin's melee, dash, hit and death clips onto the rigged field characters.
// Requires Unreal/Tools/install_mannequin.py. Writes /Game/Memoria/Presentation/Combat (IK rigs, retargeters)
// and /Game/Memoria/Presentation/Field3D/<Name>/Combat/A_<Name>_<Clip>. Additive: a character whose clips exist is kept.
// To regenerate, delete Content/Memoria/Presentation/Combat and Field3D/*/Combat first. Replacing loaded assets in
// place crashed the batch retarget, and a retargeter without its default op stack writes only the reference pose.
UCLASS()
class UMemoriaCombatRetargetCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UMemoriaCombatRetargetCommandlet();
    virtual int32 Main(const FString& Params) override;
};
