#include "Misc/AutomationTest.h"
#include "Presentation/MemoriaArrelSoleContactS362.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimSequence.h"
#include "BoneContainer.h"
#include "Misc/MemStack.h"
#include <limits>

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoleS362Values,"MemoriaGraphics.S362.SoleR4.Values",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FSoleS362Values::RunTest(const FString&)
{
    using namespace MemoriaArrelSoleContactS362;
    TestTrue(TEXT("Only exact immutable R4 Arrel is eligible"),SupportsMeshPath(TEXT("/Game/Memoria/Presentation/Field3D/HeroFinishS362/R4/Arrel/SK_Arrel.SK_Arrel")));
    for(const TCHAR* P:{TEXT("/Game/Memoria/Presentation/Field3D/HeroFinishS362/R2/Arrel/SK_Arrel.SK_Arrel"),TEXT("/Game/Memoria/Presentation/Field3D/HeroFinishS362/R3/Arrel/SK_Arrel.SK_Arrel"),TEXT("/Game/Memoria/Presentation/Field3D/HeroFinishS362/R4/Elia/SK_Elia.SK_Elia"),TEXT("Arrel")})TestFalse(TEXT("Other bodies and default Arrel remain excluded"),SupportsMeshPath(P));
    TestTrue(TEXT("Idle has no offset"),Sample(.6f,0.f,true,false).IsZero());TestTrue(TEXT("Committed action has no offset even at zero action blend"),Sample(.6f,1.f,true,true).IsZero());
    TestTrue(TEXT("Old/fallback body has no offset"),Sample(.6f,1.f,false,false).IsZero());TestTrue(TEXT("NaN phase neutral"),Sample(std::numeric_limits<float>::quiet_NaN(),1.f,true,false).IsZero());
    TestTrue(TEXT("Infinite walk weight neutral"),Sample(.6f,std::numeric_limits<float>::infinity(),true,false).IsZero());
    TestTrue(TEXT("Left peak matches .24referencecm/.20worldcm at150"),FMath::IsNearlyEqual(Sample(.6f,1.f,true,false).LeftReferenceCm,.24f,1e-6f));
    TestTrue(TEXT("Right peak matches same curve half-cycle offset"),FMath::IsNearlyEqual(Sample(.1f,1.f,true,false).RightReferenceCm,.24f,1e-6f));
    TestTrue(TEXT("Left touchdown lift is .08448referencecm at phase0"),FMath::IsNearlyEqual(Sample(0.f,1.f,true,false).LeftReferenceCm,.08448f,1e-6f));
    TestTrue(TEXT("Right touchdown has same lift at phase.5"),FMath::IsNearlyEqual(Sample(.5f,1.f,true,false).RightReferenceCm,.08448f,1e-6f));
    TestTrue(TEXT("Left/right prelanding plateaus remain bounded at .24referencecm"),FMath::IsNearlyEqual(Sample(.98f,1.f,true,false).LeftReferenceCm,.24f,1e-6f) && FMath::IsNearlyEqual(Sample(.48f,1.f,true,false).RightReferenceCm,.24f,1e-6f));
    TestTrue(TEXT("Landing lift blends by actual walk weight"),FMath::IsNearlyEqual(Sample(0.f,.5f,true,false).LeftReferenceCm,.04224f,1e-6f));
    const auto BeforeWrap=Sample(1.f-1e-6f,1.f,true,false),AfterWrap=Sample(1e-6f,1.f,true,false);
    TestTrue(TEXT("Landing curve is continuous across cycle wrap"),FMath::Abs(BeforeWrap.LeftReferenceCm-AfterWrap.LeftReferenceCm)<.0001f && FMath::Abs(BeforeWrap.RightReferenceCm-AfterWrap.RightReferenceCm)<.0001f);
    for(float Phase:{0.f,.5f,.98f,.48f})
    {
        TestTrue(TEXT("Landing remains disabled for other bodies"),Sample(Phase,1.f,false,false).IsZero());
        TestTrue(TEXT("Landing remains disabled for every committed action"),Sample(Phase,1.f,true,true).IsZero());
        TestTrue(TEXT("Landing remains disabled while idle"),Sample(Phase,0.f,true,false).IsZero());
    }
    float MaxStep=0.f;auto Prev=Sample(0.f,1.f,true,false);
    for(int32 I=1;I<=10000;++I){const auto P=Sample(I/10000.f,1.f,true,false);TestTrue(TEXT("Curve finite and bounded"),FMath::IsFinite(P.LeftReferenceCm) && P.LeftReferenceCm>=0.f && P.LeftReferenceCm<=.240001f && P.RightReferenceCm>=0.f && P.RightReferenceCm<=.240001f);MaxStep=FMath::Max(MaxStep,FMath::Max(FMath::Abs(P.LeftReferenceCm-Prev.LeftReferenceCm),FMath::Abs(P.RightReferenceCm-Prev.RightReferenceCm)));Prev=P;}
    TestTrue(TEXT("Toeoff and landing curves remain continuous"),MaxStep<.002f);
    const FTransform A(FQuat::Identity,FVector(0,0,90)),B(FQuat::Identity,FVector(0,-10,50)),C(FQuat::Identity,FVector(0,0,10));FTransform NA,NB,NC;
    TestTrue(TEXT("Connected non-stretching leg solve succeeds"),SolveLeg(A,B,C,.24f,NA,NB,NC));
    TestTrue(TEXT("Both link lengths retained"),FMath::Abs((NB.GetLocation()-NA.GetLocation()).Length()-(B.GetLocation()-A.GetLocation()).Length())<1e-8 && FMath::Abs((NC.GetLocation()-NB.GetLocation()).Length()-(C.GetLocation()-B.GetLocation()).Length())<1e-8);
    TestTrue(TEXT("Hip location and all scales retained"),NA.GetLocation()==A.GetLocation() && NA.GetScale3D()==A.GetScale3D() && NB.GetScale3D()==B.GetScale3D() && NC.GetScale3D()==C.GetScale3D());
    TestTrue(TEXT("Authored foot component orientation retained"),NC.GetRotation()==C.GetRotation());
    TestTrue(TEXT("Only requested vertical ankle lift"),(NC.GetLocation()-C.GetLocation()).Equals(FVector(0,0,.24f),1e-8));
    TestFalse(TEXT("Oversized/unbounded lift is rejected"),SolveLeg(A,B,C,1.f,NA,NB,NC));return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoleS362Pose,"MemoriaGraphics.S362.SoleR4.PoseScope",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FSoleS362Pose::RunTest(const FString&)
{
    auto* Mesh=LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/Memoria/Presentation/Field3D/HeroFinishS362/R4/Arrel/SK_Arrel.SK_Arrel"));
    auto* Walk=LoadObject<UAnimSequence>(nullptr,TEXT("/Game/Memoria/Presentation/Field3D/Arrel/A_Arrel_Walk.A_Arrel_Walk"));
    if(!TestNotNull(TEXT("Actual R4 mesh loads"),Mesh) || !TestNotNull(TEXT("Original authored walk remains active"),Walk))return false;
    TestEqual(TEXT("Walk/skeleton shared with original, no new clips"),Walk->GetSkeleton(),Mesh->GetSkeleton());
    FMemMark Mark(FMemStack::Get());TArray<FBoneIndexType> Required;for(int32 I=0;I<Mesh->GetRefSkeleton().GetNum();++I)Required.Add(I);
    FBoneContainer Bones;Bones.InitializeTo(Required,UE::Anim::FCurveFilterSettings(),*Mesh);const auto& Ref=Mesh->GetRefSkeleton();
    int32 LegChanges=0,UnownedChanges=0,SuppressedChanges=0;double MaximumLengthError=0.;
    for(float Phase:{.08f,.10f,.112f,.13f,.58f,.60f,.612f,.63f,.975f,.991f,0.f,.004f,.475f,.491f,.5f,.504f})
    {
        FCompactPose Pose;Pose.SetBoneContainer(&Bones);Pose.ResetToRefPose();TArray<FTransform> Old;
        for(FCompactPoseBoneIndex I:Pose.ForEachBoneIndex()){Walk->GetBoneTransform(Pose[I],FSkeletonPoseBoneIndex(Bones.GetSkeletonIndex(I)),FAnimExtractContext(Phase*Walk->GetPlayLength(),false),false);Old.Add(Pose[I]);}
        for(int32 Guard=0;Guard<4;++Guard)
        {
            // Old ArrelR2, Elia and all committed actions are explicit negative
            // cases; the zero sample must leave EVERY pose value unchanged.
            FCompactPose Inactive;Inactive.SetBoneContainer(&Bones);for(FCompactPoseBoneIndex I:Inactive.ForEachBoneIndex())Inactive[I]=Old[I.GetInt()];
            MemoriaArrelSoleContactS362::Apply(Inactive,MemoriaArrelSoleContactS362::Sample(Phase,Guard==3?0.f:1.f,Guard>=2,Guard==2));
            for(FCompactPoseBoneIndex I:Inactive.ForEachBoneIndex())if(Inactive[I].GetLocation()!=Old[I.GetInt()].GetLocation() || Inactive[I].GetRotation()!=Old[I.GetInt()].GetRotation() || Inactive[I].GetScale3D()!=Old[I.GetInt()].GetScale3D())++SuppressedChanges;
        }
        FCSPose<FCompactPose> Before;Before.InitPose(Pose);FTransform OriginalFoot[2];double Length[2][2];
        auto Index=[&](const FString& N){return Bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(Ref.FindBoneIndex(FName(N))));};
        for(int32 S=0;S<2;++S){const FString Side=S==0?TEXT("l"):TEXT("r");const auto A=Before.GetComponentSpaceTransform(Index(TEXT("thigh_")+Side)),B=Before.GetComponentSpaceTransform(Index(TEXT("calf_")+Side)),C=Before.GetComponentSpaceTransform(Index(TEXT("foot_")+Side));Length[S][0]=(B.GetLocation()-A.GetLocation()).Length();Length[S][1]=(C.GetLocation()-B.GetLocation()).Length();OriginalFoot[S]=C;}
        MemoriaArrelSoleContactS362::Apply(Pose,MemoriaArrelSoleContactS362::Sample(Phase,1.f,true,false));
        for(FCompactPoseBoneIndex I:Pose.ForEachBoneIndex())
        {
            const FString N=Ref.GetBoneName(Required[I.GetInt()]).ToString();const bool Owned=N==TEXT("thigh_l") || N==TEXT("thigh_r") || N==TEXT("calf_l") || N==TEXT("calf_r") || N==TEXT("foot_l") || N==TEXT("foot_r");
            const bool Same=Pose[I].GetLocation()==Old[I.GetInt()].GetLocation() && Pose[I].GetRotation()==Old[I.GetInt()].GetRotation() && Pose[I].GetScale3D()==Old[I.GetInt()].GetScale3D();if(!Same){if(Owned)++LegChanges;else ++UnownedChanges;}
            TestTrue(TEXT("No bone scales changed"),Pose[I].GetScale3D().Equals(Old[I.GetInt()].GetScale3D(),1e-10));
        }
        FCSPose<FCompactPose> After;After.InitPose(Pose);
        for(int32 S=0;S<2;++S){const FString Side=S==0?TEXT("l"):TEXT("r");const auto A=After.GetComponentSpaceTransform(Index(TEXT("thigh_")+Side)),B=After.GetComponentSpaceTransform(Index(TEXT("calf_")+Side)),C=After.GetComponentSpaceTransform(Index(TEXT("foot_")+Side));MaximumLengthError=FMath::Max(MaximumLengthError,FMath::Max(FMath::Abs((B.GetLocation()-A.GetLocation()).Length()-Length[S][0]),FMath::Abs((C.GetLocation()-B.GetLocation()).Length()-Length[S][1])));TestTrue(TEXT("Foot component orientation preserved"),C.GetRotation().AngularDistance(OriginalFoot[S].GetRotation())<1e-6);}
    }
    TestEqual(TEXT("OldArrelR2/Elia/committed-action/idle negative poses remain exact"),SuppressedChanges,0);TestTrue(TEXT("Actual walk gets a real leg correction"),LegChanges>0);TestEqual(TEXT("Every non-leg local bone including root/hands/ball remains exact"),UnownedChanges,0);TestTrue(TEXT("Native two-link lengths retained without stretching"),MaximumLengthError<.0001);
    AddInfo(FString::Printf(TEXT("S362_SOLE_R4_POSE_SCOPE leg_edits=%d unowned=%d max_link_error_cm=%.9f"),LegChanges,UnownedChanges,MaximumLengthError));return true;
}
#endif