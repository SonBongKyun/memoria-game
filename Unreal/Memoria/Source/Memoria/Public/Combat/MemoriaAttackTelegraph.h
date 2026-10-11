#pragma once
#include "CoreMinimal.h"
class FSlateWindowElementList;
struct FGeometry;
class APlayerController;
class UMemoriaFieldCombatSubsystem;
namespace MemoriaAttackTelegraph
{
    void Paint(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& Geometry,
        APlayerController* PC, const UMemoriaFieldCombatSubsystem& Combat);
}
