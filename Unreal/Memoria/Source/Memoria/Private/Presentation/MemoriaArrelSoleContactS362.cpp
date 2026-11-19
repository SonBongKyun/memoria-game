#include "Presentation/MemoriaArrelSoleContactS362.h"
#include "BoneContainer.h"

namespace
{
double Smooth(double T){T=FMath::Clamp(T,0.,1.);return T*T*(3.-2.*T);}
float Curve(float Phase,double ToeOff)
{
    double D=FMath::Fmod(double(Phase)-ToeOff+.5+2.,1.)-.5;
    if(D<=-.02 || D>=.05)return 0.f;
    if(D<0.)return float(.24*Smooth((D+.02)/.02));
    if(D<=.015)return .24f;
    return float(.24*(1.-Smooth((D-.015)/.035)));
}
// Begin settling the heel before the authored ankle reaches touchdown. The
// small vertical lift never changes phase/XY motion or the contact criterion.
float LandingCurve(float Phase,double Touchdown)
{
    const double D=FMath::Fmod(double(Phase)-Touchdown+.5+2.,1.)-.5;
    if(D<=-.045 || D>=.008)return 0.f;
    if(D<-.025)return float(.24*Smooth((D+.045)/.02));
    if(D<=-.012)return .24f;
    return float(.24*(1.-Smooth((D+.012)/.02)));
}
}
bool MemoriaArrelSoleContactS362::SupportsMeshPath(const FString& Path)
{return Path==TEXT("/Game/Memoria/Presentation/Field3D/HeroFinishS362/R4/Arrel/SK_Arrel.SK_Arrel");}
FMemoriaSoleLiftS362 MemoriaArrelSoleContactS362::Sample(float Phase,float WalkWeight,bool bR4,bool bAction)
{
    FMemoriaSoleLiftS362 P;
    if(!bR4 || bAction || !FMath::IsFinite(Phase) || !FMath::IsFinite(WalkWeight) || WalkWeight<=0.f)return P;
    const float Weight=FMath::Clamp(WalkWeight,0.f,1.f);
    P.LeftReferenceCm=FMath::Max(Curve(Phase,.6),LandingCurve(Phase,0.))*Weight;
    P.RightReferenceCm=FMath::Max(Curve(Phase,.1),LandingCurve(Phase,.5))*Weight;return P;
}
bool MemoriaArrelSoleContactS362::SolveLeg(const FTransform& Thigh,const FTransform& Calf,const FTransform& Foot,float Lift,
    FTransform& OutThigh,FTransform& OutCalf,FTransform& OutFoot)
{
    OutThigh=Thigh;OutCalf=Calf;OutFoot=Foot;
    if(!FMath::IsFinite(Lift) || Lift<0.f || Lift>.240001f || Thigh.ContainsNaN() || Calf.ContainsNaN() || Foot.ContainsNaN())return false;
    if(Lift==0.f)return true;
    const FVector A=Thigh.GetLocation(),B=Calf.GetLocation(),C=Foot.GetLocation(),Target=C+FVector(0,0,Lift);
    const double L1=(B-A).Length(),L2=(C-B).Length(),Distance=(Target-A).Length();
    if(L1<1e-6 || L2<1e-6 || Distance<=FMath::Abs(L1-L2)+1e-6 || Distance>=L1+L2-1e-6)return false;
    const FVector Axis=(Target-A)/Distance;
    const double Along=(L1*L1-L2*L2+Distance*Distance)/(2.*Distance),Height=FMath::Sqrt(FMath::Max(0.,L1*L1-Along*Along));
    const FVector Bend=(B-A)-Axis*FVector::DotProduct(B-A,Axis);if(Bend.SizeSquared()<1e-12)return false;
    const FVector Knee=A+Axis*Along+Bend.GetSafeNormal()*Height;
    OutThigh.SetRotation((FQuat::FindBetweenNormals((B-A).GetSafeNormal(),(Knee-A).GetSafeNormal())*Thigh.GetRotation()).GetNormalized());
    OutCalf.SetLocation(Knee);OutCalf.SetRotation((FQuat::FindBetweenNormals((C-B).GetSafeNormal(),(Target-Knee).GetSafeNormal())*Calf.GetRotation()).GetNormalized());
    OutFoot.SetLocation(Target); // orientation/scales remain their exact values
    return !OutThigh.ContainsNaN() && !OutCalf.ContainsNaN() && !OutFoot.ContainsNaN();
}
void MemoriaArrelSoleContactS362::Apply(FCompactPose& Pose,const FMemoriaSoleLiftS362& Lift)
{
    if(Lift.IsZero() || !Pose.IsValid())return;
    const auto& Bones=Pose.GetBoneContainer();const auto& Ref=Bones.GetReferenceSkeleton();
    TArray<FTransform,TInlineAllocator<128>> Original;Original.Reserve(Pose.GetNumBones());
    for(FCompactPoseBoneIndex I:Pose.ForEachBoneIndex())Original.Add(Pose[I]);
    TArray<int32,TInlineAllocator<6>> Edited;FCSPose<FCompactPose> Component;Component.InitPose(Pose);
    auto Leg=[&](const TCHAR* Side,float Amount)
    {
        if(Amount<=0.f)return;
        FCompactPoseBoneIndex Ids[3];const TCHAR* Stems[]={TEXT("thigh_"),TEXT("calf_"),TEXT("foot_")};
        for(int32 I=0;I<3;++I)
        {const int32 MeshIndex=Ref.FindBoneIndex(FName(FString(Stems[I])+Side));if(MeshIndex==INDEX_NONE)return;Ids[I]=Bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(MeshIndex));if(Ids[I].GetInt()==INDEX_NONE)return;}
        FTransform A,B,C;if(!SolveLeg(Component.GetComponentSpaceTransform(Ids[0]),Component.GetComponentSpaceTransform(Ids[1]),Component.GetComponentSpaceTransform(Ids[2]),Amount,A,B,C))return;
        TArray<FBoneTransform,TInlineAllocator<3>> Edits;Edits.Emplace(Ids[0],A);Edits.Emplace(Ids[1],B);Edits.Emplace(Ids[2],C);
        Component.LocalBlendCSBoneTransforms(MakeArrayView(Edits),1.f);for(auto I:Ids)Edited.AddUnique(I.GetInt());
    };
    Leg(TEXT("l"),Lift.LeftReferenceCm);Leg(TEXT("r"),Lift.RightReferenceCm);
    if(Edited.IsEmpty())return;
    FCSPose<FCompactPose>::ConvertComponentPosesToLocalPosesSafe(Component,Pose);
    // CS conversion can introduce tiny roundoff in cached ancestors. Restore
    // every unowned LOCAL transform verbatim, not through a loose comparison.
    for(FCompactPoseBoneIndex I:Pose.ForEachBoneIndex())
    {if(!Edited.Contains(I.GetInt()))Pose[I]=Original[I.GetInt()];else Pose[I].SetScale3D(Original[I.GetInt()].GetScale3D());}
}