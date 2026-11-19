#pragma once

#include "CoreMinimal.h"

/** Presentation-only value transform measured against the original S312 mount. */
struct MEMORIA_API FMemoriaHeroPropMount
{
    static constexpr double OutwardTranslationCm=4.;
    static constexpr double OutwardCantDegrees=15.;
    static FVector NativeScabbardMouth(){return FVector(0,0,-13.5);}

    // Input/output use the imported reference-component frame (Xforward,
    // Yright,Zup). Apply to BOTH mounted sword/scabbard before converting the
    // original desired transform to pelvis-relative. Never apply to drawn grip.
    static FTransform FinishScabbardMount(const FTransform& OriginalReference);
};
