#include "Presentation/MemoriaCharacterPersonalityS362.h"
#include "BoneContainer.h"

bool FMemoriaIdlePoseS362::IsIdentity() const
{ return Chest.IsNearlyZero() && Neck.IsNearlyZero() && Head.IsNearlyZero() && LeftShoulder.IsNearlyZero() && RightShoulder.IsNearlyZero(); }
namespace
{
FString Key(const FString& Id) { return Id.Replace(TEXT("_"),TEXT("")).ToLower(); }
double Wave(double Age,double Period,double Phase=0.) { return FMath::Sin((Age/Period+Phase)*2.*PI); }
// A slow look-away-and-back gesture with zero velocity at both endpoints.
double Glance(double Age,double Period,double Phase=0.)
{ const double S=Wave(Age,Period,Phase); return S*S*S; }
}
bool MemoriaCharacterPersonalityS362::Supports(const FString& Id)
{
    const FString K=Key(Id);
    return K==TEXT("malet") || K==TEXT("scholar") || K==TEXT("guard") || K==TEXT("elder") ||
        K==TEXT("bureauagent") || K==TEXT("traveler") || K==TEXT("fisherman") ||
        K==TEXT("villagerf") || K==TEXT("villagerm") || K==TEXT("child");
}
FMemoriaIdlePoseS362 MemoriaCharacterPersonalityS362::Sample(const FString& Id,double Age,float WalkWeight,bool bActing,bool bCombatAttention,bool bReduceMotion)
{
    FMemoriaIdlePoseS362 P;
    if (!FMath::IsFinite(Age) || !FMath::IsFinite(WalkWeight) || WalkWeight>.02f || bActing || bCombatAttention || bReduceMotion) return P;
    const FString K=Key(Id);
    if (K==TEXT("malet"))
    {
        // The ledger is in the right hand: a deliberate right/down glance,
        // leaving chest, arms and the ledger grip completely untouched.
        P.Head=FRotator(-3.8f+1.4f*Wave(Age,9.5),3.5f+3.2f*Glance(Age,14.,.12),.5f);
        P.Neck=FRotator(-.6f,0,0);
    }
    else if (K==TEXT("scholar"))
    {
        P.Chest=FRotator(-1.6f,0,0);
        P.Neck=FRotator(-1.1f,0,0);
        P.Head=FRotator(-6.5f+1.2f*Wave(Age,11.,.2),-2.5f+2.f*Glance(Age,17.),-.8f);
    }
    else if (K==TEXT("guard"))
    {
        // Square shoulders and a restrained patrol scan; no weapon-ready
        // gesture that could be mistaken for the actual combat telegraph.
        P.Chest=FRotator(.8f,0,0);
        P.Head=FRotator(.5f,6.f*Glance(Age,16.,.22),0);
        const float Set=1.1f+.25f*Wave(Age,6.8);
        P.LeftShoulder=FRotator(0,0,-Set); P.RightShoulder=FRotator(0,0,Set);
    }
    else if (K==TEXT("elder"))
    {
        P.Chest=FRotator(-2.4f+.3f*Wave(Age,8.8),0,.5f);
        P.Head=FRotator(-2.8f+.8f*Wave(Age,13.,.3),2.f*Glance(Age,21.),.8f);
    }
    else if (K==TEXT("bureauagent"))
    {
        P.Chest=FRotator(1.1f,0,0); P.Head=FRotator(-.8f,4.f*Glance(Age,12.5,.35),-.5f);
    }
    else if (K==TEXT("traveler"))
    {
        P.Chest=FRotator(-.6f,0,.9f); P.Head=FRotator(-.4f,-4.2f*Glance(Age,15.,.4),.7f);
    }
    else if (K==TEXT("fisherman"))
    {
        P.Chest=FRotator(-.7f,0,-.8f); P.Head=FRotator(-1.2f,5.f*Glance(Age,18.,.1),-1.2f);
    }
    else if (K==TEXT("villagerf") || K==TEXT("villagerm"))
    {
        P.Head=FRotator(1.3f*Glance(Age,10.5,.18),2.6f*Wave(Age,19.),.8f);
        P.Chest=FRotator(0,0,.45f*Wave(Age,8.5));
    }
    else if (K==TEXT("child"))
    {
        // Curiosity stays in the upper body; no root bounce or foot sliding.
        P.Head=FRotator(1.8f+1.1f*Wave(Age,7.8),4.f*Glance(Age,11.5,.3),1.3f);
        P.Chest=FRotator(.3f,0,.7f*Wave(Age,9.));
    }
    return P;
}
void MemoriaCharacterPersonalityS362::Apply(FCompactPose& Pose,const FMemoriaIdlePoseS362& P)
{
    if (P.IsIdentity() || !Pose.IsValid()) return;
    FCSPose<FCompactPose> ComponentPose; ComponentPose.InitPose(Pose);
    const auto& Bones=Pose.GetBoneContainer(); const auto& Ref=Bones.GetReferenceSkeleton();
    auto Rotate=[&](const TCHAR* Name,const FRotator& Rotation)
    {
        if (Rotation.IsNearlyZero()) return;
        const int32 MeshIndex=Ref.FindBoneIndex(Name); if (MeshIndex==INDEX_NONE) return;
        const FCompactPoseBoneIndex Index=Bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(MeshIndex));
        if (Index.GetInt()==INDEX_NONE) return; // reduced bone sets / LOD are explicit fallbacks
        FTransform Transform=ComponentPose.GetComponentSpaceTransform(Index);
        Transform.SetRotation((Rotation.Quaternion()*Transform.GetRotation()).GetNormalized());
        const FBoneTransform Edit(Index,Transform);
        // One parent-first edit at a time refreshes child component transforms;
        // the original local hierarchy is retained by the safe conversion.
        ComponentPose.LocalBlendCSBoneTransforms(MakeArrayView(&Edit,1),1.f);
    };
    Rotate(TEXT("spine_03"),P.Chest);
    Rotate(TEXT("clavicle_l"),P.LeftShoulder); Rotate(TEXT("clavicle_r"),P.RightShoulder);
    Rotate(TEXT("neck_01"),P.Neck); Rotate(TEXT("head"),P.Head);
    FCSPose<FCompactPose>::ConvertComponentPosesToLocalPosesSafe(ComponentPose,Pose);
}
