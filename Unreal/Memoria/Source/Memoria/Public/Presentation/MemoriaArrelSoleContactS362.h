#pragma once
#include "CoreMinimal.h"
#include "BonePose.h"

struct MEMORIA_API FMemoriaSoleLiftS362
{
    float LeftReferenceCm=0.f,RightReferenceCm=0.f;
    bool IsZero() const {return LeftReferenceCm==0.f && RightReferenceCm==0.f;}
};
namespace MemoriaArrelSoleContactS362
{
    // Pure values; the proxy resolves this exact mesh path on the game thread.
    MEMORIA_API bool SupportsMeshPath(const FString& Path);
    // .24referencecm = .20worldcm at the150cm Arrel scale. No clock/phase edits.
    MEMORIA_API FMemoriaSoleLiftS362 Sample(float Phase,float WalkWeight,bool bExactR4Arrel,bool bCommittedAction);
    // Connected, non-stretching two-link solve. Values/outputs only, no World.
    MEMORIA_API bool SolveLeg(const FTransform& Thigh,const FTransform& Calf,const FTransform& Foot,float LiftReferenceCm,
        FTransform& OutThigh,FTransform& OutCalf,FTransform& OutFoot);
    // AnyThread. Only six thigh/calf/foot LOCAL transforms may change. Original
    // foot component orientation, child-local ball, root/hands and other local
    // transforms remain exact; no saved state or UObject access.
    MEMORIA_API void Apply(FCompactPose& Pose,const FMemoriaSoleLiftS362& Lift);
}