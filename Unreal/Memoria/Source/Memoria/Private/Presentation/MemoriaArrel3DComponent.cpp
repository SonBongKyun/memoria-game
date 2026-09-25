#include "Presentation/MemoriaArrel3DComponent.h"
#include "Framework/MemoriaVerdanTuning.h"
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
        // Match stance travel to actual displacement, including analog input and swept stops.
        // The safety cap is above the supported walk speed, so ordinary walking never saturates it.
        const float CycleDistance=2.f*MemoriaVerdanTuning::FootReach*FMath::Abs(float(GetComponentScale().X))/MemoriaVerdanTuning::StanceFraction;
        Phase=FMath::Fmod(Phase+FMath::Min(float(Step.Size2D())/FMath::Max(CycleDistance,.01f),Dt*MemoriaVerdanTuning::MaxVisualCyclesPerSecond),1.f);
        SetWorldRotation(FMath::RInterpConstantTo(GetComponentRotation(),FRotator(0,Step.Rotation().Yaw,0),Dt,900.f));
    }
    BoneSpaceTransforms=Mesh->GetRefSkeleton().GetRefBonePose();
    if(BonesFor.Get()!=Mesh)CacheGaitBones(*Mesh);
    auto Rotate=[&](int32 I,float Y,float X=0.f){if(BoneSpaceTransforms.IsValidIndex(I))BoneSpaceTransforms[I].SetRotation(FQuat(FVector::YAxisVector,FMath::DegreesToRadians(Y))*FQuat(FVector::XAxisVector,FMath::DegreesToRadians(X)));};
    const float Drop=(-8.f+FMath::Cos(4*UE_PI*Phase))*WalkWeight;
    if(BoneSpaceTransforms.IsValidIndex(Bones.Pelvis))BoneSpaceTransforms[Bones.Pelvis].AddToTranslation(FVector(0,0,Drop));
    for(int32 I=0;I<2;++I)
    {
        const float P=FMath::Fmod(Phase+I*.5f,1.f);
        const float Stance=MemoriaVerdanTuning::StanceFraction;
        const bool Planted=P<Stance;
        const float Swing=Planted?0.f:(P-Stance)/(1.f-Stance);
        const float Ease=Swing*Swing*(3.f-2.f*Swing);
        const float X=MemoriaVerdanTuning::FootReach*(Planted?(1.f-2.f*P/Stance):(-1.f+2.f*Ease))*WalkWeight;
        const float Arc=FMath::Square(FMath::Sin(UE_PI*Swing));
        const float Lift=12.f*Arc*WalkWeight;
        const float Down=81.f+Drop-Lift;
        const float D=FMath::Clamp(FMath::Sqrt(X*X+Down*Down),1.f,81.f);
        const float Hip=-(FMath::Atan2(X,Down)+FMath::Acos(FMath::Clamp((39.f*39+D*D-42.f*42)/(2*39.f*D),-1.f,1.f)));
        const float Knee=UE_PI-FMath::Acos(FMath::Clamp((39.f*39+42.f*42-D*D)/(2*39.f*42),-1.f,1.f));
        Rotate(Bones.Thigh[I],FMath::RadiansToDegrees(Hip));
        Rotate(Bones.Calf[I],FMath::RadiansToDegrees(Knee));
        Rotate(Bones.Foot[I],-FMath::RadiansToDegrees(Hip+Knee)+10.f*Arc*FMath::Cos(UE_PI*Swing)*WalkWeight);
        Rotate(Bones.UpperArm[I],-FMath::Cos(P*UE_TWO_PI)*19.f*WalkWeight,I==0?-3.f:3.f);
        Rotate(Bones.Forearm[I],-8.f-10.f*WalkWeight);
    }
    Rotate(Bones.Chest,2.f*WalkWeight+FMath::Sin(IdleTime*1.6f)*.45f, FMath::Sin(Phase*UE_TWO_PI)*1.2f*WalkWeight);
    Rotate(Bones.CapeUpper,3.f+WalkWeight*8.f);
    Rotate(Bones.CapeMid,FMath::Sin(IdleTime*2.5f)*2.f+WalkWeight*5.f);
    Rotate(Bones.CapeTip,FMath::Sin(IdleTime*2.5f-.7f)*3.f+WalkWeight*6.f);
    MarkRefreshTransformDirty();RefreshBoneTransforms();
}
void UMemoriaArrel3DComponent::CacheGaitBones(USkeletalMesh& Mesh)
{
    const auto& Skeleton=Mesh.GetRefSkeleton();
    auto Find=[&](const FString& Name)
    {
        const int32 I=Skeleton.FindBoneIndex(FName(*Name));
        // Once per asset: a swapped rig reports what it cannot animate instead of crashing.
        if(I==INDEX_NONE)UE_LOG(LogTemp,Warning,TEXT("Arrel gait: %s has no bone '%s'; that part keeps its reference pose"),*Mesh.GetName(),*Name);
        return I;
    };
    Bones=FMemoriaArrelGaitBones();
    Bones.Pelvis=Find(TEXT("pelvis"));Bones.Chest=Find(TEXT("chest"));
    Bones.CapeUpper=Find(TEXT("cape_upper"));Bones.CapeMid=Find(TEXT("cape_mid"));Bones.CapeTip=Find(TEXT("cape_tip"));
    for(int32 I=0;I<2;++I)
    {
        const TCHAR* Side=I==0?TEXT("l"):TEXT("r");
        Bones.Thigh[I]=Find(FString::Printf(TEXT("thigh_%s"),Side));Bones.Calf[I]=Find(FString::Printf(TEXT("calf_%s"),Side));
        Bones.Foot[I]=Find(FString::Printf(TEXT("foot_%s"),Side));Bones.UpperArm[I]=Find(FString::Printf(TEXT("upperarm_%s"),Side));
        Bones.Forearm[I]=Find(FString::Printf(TEXT("forearm_%s"),Side));
    }
    BonesFor=&Mesh;
}
