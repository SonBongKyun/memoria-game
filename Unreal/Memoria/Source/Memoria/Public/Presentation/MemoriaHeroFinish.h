#pragma once
#include "CoreMinimal.h"

class USkeletalMesh;
class USkeletalMeshComponent;

// Presentation only: no timers, random global state, save fields or eye-bone
// manipulation. Elapsed time is supplied by the existing character component.
// Morphs deform dedicated lid skin; original eye spheres remain unchanged.
struct MEMORIA_API FMemoriaHeroFinish
{
    static const FName BlinkLeft;
    static const FName BlinkRight;
    static bool HasDedicatedLids(const USkeletalMesh* Mesh);
    static float BlinkWeight(double ElapsedSeconds, uint32 StableSeed, bool bSuppressed = false);
    // Returns false for older/fallback meshes. Suppression explicitly clears
    // both weights, including when a death pose starts during a blink.
    static bool ApplyBlink(USkeletalMeshComponent* Mesh, float Weight, bool bSuppressed = false);
    static void ClearBlink(USkeletalMeshComponent* Mesh);
};
