#include "Presentation/MemoriaHeroPropMount.h"

FTransform FMemoriaHeroPropMount::FinishScabbardMount(const FTransform& OriginalReference)
{
    const FVector Mouth=OriginalReference.TransformPosition(NativeScabbardMouth());
    const FQuat Cant(FVector(-1,0,0),FMath::DegreesToRadians(OutwardCantDegrees));
    FTransform Result=OriginalReference;
    Result.SetRotation((Cant*OriginalReference.GetRotation()).GetNormalized());
    Result.SetTranslation(Mouth+FVector(0,-OutwardTranslationCm,0)
        +Cant.RotateVector(OriginalReference.GetTranslation()-Mouth));
    return Result;
}
