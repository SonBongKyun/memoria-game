#pragma once
#include "Commandlets/Commandlet.h"
#include "MemoriaSwordRetargetCommandlet.generated.h"
// S312: Arrel's sword set. Imports Quaternius' Universal Animation Library 2 (CC0, the free Standard glb)
// into the git-ignored /Game/Memoria/Imported/UAL2, then retargets its sword clips onto Arrel as
// /Game/Memoria/Presentation/Field3D/Arrel/Combat/A_Arrel_<Clip>. -Source=<UAL2_Standard.glb> overrides the
// shared-folder copy; -ImportOnly stops after listing what the import produced. It also imports the drawn
// sword and empty scabbard split from the sheathed prop (models/_raw/s312_sword, see sword_split.py there). Like MemoriaCombatRetarget,
// regenerate from deleted outputs, never in place.
UCLASS()
class UMemoriaSwordRetargetCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UMemoriaSwordRetargetCommandlet();
    virtual int32 Main(const FString& Params) override;
};
