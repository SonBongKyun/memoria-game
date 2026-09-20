#include "Presentation/MemoriaArrel3DComponent.h"
#include "Engine/SkeletalMesh.h"
#if WITH_EDITOR
#include "SkinnedAssetCompiler.h"
#endif
bool UMemoriaArrel3DComponent::InitializePrototype()
{
    auto* Mesh=LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/Memoria/Presentation/Character1/SK_ArrelPrototype.SK_ArrelPrototype"));
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
    WalkWeight=FMath::FInterpTo(WalkWeight,Moving?1.f:0.f,Dt,15.f);
    if(Moving)
    {
        Phase=FMath::Fmod(Phase+float(Step.Size2D())*UE_TWO_PI/140.f,UE_TWO_PI);
        SetWorldRotation(FMath::RInterpConstantTo(GetComponentRotation(),FRotator(0,Step.Rotation().Yaw,0),Dt,900.f));
    }
    BoneSpaceTransforms=Mesh->GetRefSkeleton().GetRefBonePose();
    auto Index=[&](const TCHAR* Name){return Mesh->GetRefSkeleton().FindBoneIndex(Name);};
    auto Rotate=[&](const TCHAR* Name,float Y,float X=0.f){const int32 I=Index(Name);if(I!=INDEX_NONE)BoneSpaceTransforms[I].SetRotation(FQuat(FVector::YAxisVector,FMath::DegreesToRadians(Y))*FQuat(FVector::XAxisVector,FMath::DegreesToRadians(X)));};
    const float Drop=(-5.f+1.5f*FMath::Cos(2*Phase))*WalkWeight;
    BoneSpaceTransforms[Index(TEXT("pelvis"))].AddToTranslation(FVector(0,0,Drop));
    for(int32 I=0;I<2;++I)
    {
        const float P=Phase+I*UE_PI;
        const float X=23.f*FMath::Cos(P)*WalkWeight;
        const float Lift=FMath::Max(0.f,-FMath::Sin(P))*14.f*WalkWeight;
        const float Down=81.f+Drop-Lift;
        const float D=FMath::Clamp(FMath::Sqrt(X*X+Down*Down),1.f,80.99f);
        const float Hip=-(FMath::Atan2(X,Down)+FMath::Acos(FMath::Clamp((39.f*39+D*D-42.f*42)/(2*39.f*D),-1.f,1.f)));
        const float Knee=UE_PI-FMath::Acos(FMath::Clamp((39.f*39+42.f*42-D*D)/(2*39.f*42),-1.f,1.f));
        const TCHAR* Side=I==0?TEXT("l"):TEXT("r");
        Rotate(*FString::Printf(TEXT("thigh_%s"),Side),FMath::RadiansToDegrees(Hip));
        Rotate(*FString::Printf(TEXT("calf_%s"),Side),FMath::RadiansToDegrees(Knee));
        Rotate(*FString::Printf(TEXT("foot_%s"),Side),-FMath::RadiansToDegrees(Hip+Knee));
        Rotate(*FString::Printf(TEXT("upperarm_%s"),Side),-FMath::Cos(P)*24.f*WalkWeight,I==0?-3.f:3.f);
        Rotate(*FString::Printf(TEXT("forearm_%s"),Side),-8.f-10.f*WalkWeight);
    }
    Rotate(TEXT("chest"),FMath::Sin(IdleTime*1.6f)*.6f, FMath::Sin(Phase)*2.f*WalkWeight);
    Rotate(TEXT("cape_upper"),3.f+WalkWeight*8.f);
    Rotate(TEXT("cape_mid"),FMath::Sin(IdleTime*2.5f)*2.f+WalkWeight*5.f);
    Rotate(TEXT("cape_tip"),FMath::Sin(IdleTime*2.5f-.7f)*3.f+WalkWeight*6.f);
    MarkRefreshTransformDirty();RefreshBoneTransforms();
}
