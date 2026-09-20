#include "Presentation/MemoriaArrel3DComponent.h"
#include "Engine/SkeletalMesh.h"
#if WITH_EDITOR
#include "SkinnedAssetCompiler.h"
#endif
bool UMemoriaArrel3DComponent::InitializePrototype()
{
    auto* Mesh=LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/Memoria/Presentation/Character2/SK_ArrelRefined.SK_ArrelRefined"));
    if(!Mesh)return false;
#if WITH_EDITOR
    FSkinnedAssetCompilingManager::Get().FinishCompilation({Mesh});
#endif
    SetSkinnedAssetAndUpdate(Mesh);SetCollisionEnabled(ECollisionEnabled::NoCollision);SetGenerateOverlapEvents(false);
    SetCastShadow(true);SetRelativeScale3D(FVector(.68));SetRelativeLocation(FVector(0,0,-8));SetRelativeRotation(FRotator(0,-90,0));
    bComponentUseFixedSkelBounds=true;SetLightingChannels(true,true,false);
    InvalidateCachedBounds();UpdateBounds();AdvanceLocomotion(FVector::ZeroVector,.001f);return true;
}
void UMemoriaArrel3DComponent::AdvanceLocomotion(const FVector& Step,float Dt)
{
    auto* Mesh=Cast<USkeletalMesh>(GetSkinnedAsset());if(!Mesh || Dt<=0)return;
    IdleTime+=Dt;
    // Ignore discontinuous test/travel teleports; animation is driven by ordinary displacement.
    const bool Moving=Step.SizeSquared2D()>.0001 && Step.Size2D()<150;
    WalkWeight=FMath::Lerp(WalkWeight,Moving?1.f:0.f,1.f-FMath::Exp(-15.f*Dt));
    if(Moving)
    {
        // A planted foot traverses 56 source cm during 60% of the stride.
        // Preserve the existing fast pawn movement; cap visual cadence above walk speeds.
        const float CycleDistance=56.f*FMath::Abs(float(GetComponentScale().X))/.6f;
        Phase=FMath::Fmod(Phase+FMath::Min(float(Step.Size2D())/FMath::Max(CycleDistance,.01f),Dt*3.f),1.f);
        SetWorldRotation(FMath::RInterpConstantTo(GetComponentRotation(),FRotator(0,Step.Rotation().Yaw,0),Dt,900.f));
    }
    BoneSpaceTransforms=Mesh->GetRefSkeleton().GetRefBonePose();
    auto Index=[&](const TCHAR* Name){return Mesh->GetRefSkeleton().FindBoneIndex(Name);};
    auto Rotate=[&](const TCHAR* Name,float Y,float X=0.f){const int32 I=Index(Name);if(I!=INDEX_NONE)BoneSpaceTransforms[I].SetRotation(FQuat(FVector::YAxisVector,FMath::DegreesToRadians(Y))*FQuat(FVector::XAxisVector,FMath::DegreesToRadians(X)));};
    const float Drop=(-8.f+FMath::Cos(4*UE_PI*Phase))*WalkWeight;
    BoneSpaceTransforms[Index(TEXT("pelvis"))].AddToTranslation(FVector(0,0,Drop));
    for(int32 I=0;I<2;++I)
    {
        const float P=FMath::Fmod(Phase+I*.5f,1.f);
        const bool Planted=P<.6f;
        const float Swing=Planted?0.f:(P-.6f)/.4f;
        const float Ease=Swing*Swing*(3.f-2.f*Swing);
        const float X=28.f*(Planted?(1.f-2.f*P/.6f):(-1.f+2.f*Ease))*WalkWeight;
        const float Arc=FMath::Square(FMath::Sin(UE_PI*Swing));
        const float Lift=12.f*Arc*WalkWeight;
        const float Down=81.f+Drop-Lift;
        const float D=FMath::Clamp(FMath::Sqrt(X*X+Down*Down),1.f,81.f);
        const float Hip=-(FMath::Atan2(X,Down)+FMath::Acos(FMath::Clamp((39.f*39+D*D-42.f*42)/(2*39.f*D),-1.f,1.f)));
        const float Knee=UE_PI-FMath::Acos(FMath::Clamp((39.f*39+42.f*42-D*D)/(2*39.f*42),-1.f,1.f));
        const TCHAR* Side=I==0?TEXT("l"):TEXT("r");
        Rotate(*FString::Printf(TEXT("thigh_%s"),Side),FMath::RadiansToDegrees(Hip));
        Rotate(*FString::Printf(TEXT("calf_%s"),Side),FMath::RadiansToDegrees(Knee));
        Rotate(*FString::Printf(TEXT("foot_%s"),Side),-FMath::RadiansToDegrees(Hip+Knee)+10.f*Arc*FMath::Cos(UE_PI*Swing)*WalkWeight);
        Rotate(*FString::Printf(TEXT("upperarm_%s"),Side),-FMath::Cos(P*UE_TWO_PI)*19.f*WalkWeight,I==0?-3.f:3.f);
        Rotate(*FString::Printf(TEXT("forearm_%s"),Side),-8.f-10.f*WalkWeight);
    }
    Rotate(TEXT("chest"),2.f*WalkWeight+FMath::Sin(IdleTime*1.6f)*.45f, FMath::Sin(Phase*UE_TWO_PI)*1.2f*WalkWeight);
    Rotate(TEXT("cape_upper"),3.f+WalkWeight*8.f);
    Rotate(TEXT("cape_mid"),FMath::Sin(IdleTime*2.5f)*2.f+WalkWeight*5.f);
    Rotate(TEXT("cape_tip"),FMath::Sin(IdleTime*2.5f-.7f)*3.f+WalkWeight*6.f);
    MarkRefreshTransformDirty();RefreshBoneTransforms();
}
