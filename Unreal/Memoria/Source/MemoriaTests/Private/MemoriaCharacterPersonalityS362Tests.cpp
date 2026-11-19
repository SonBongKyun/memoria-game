#include "Misc/AutomationTest.h"
#include "Presentation/MemoriaCharacterPersonalityS362.h"
#include "Engine/SkeletalMesh.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPersonalityS362,"MemoriaUpgrade.S362.IdlePersonality",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FPersonalityS362::RunTest(const FString&)
{
    using namespace MemoriaCharacterPersonalityS362;
    const TCHAR* Roles[]={TEXT("Malet"),TEXT("Scholar"),TEXT("Guard"),TEXT("Elder"),TEXT("bureau_agent"),TEXT("Traveler"),TEXT("Fisherman"),TEXT("Villagerf"),TEXT("Villagerm"),TEXT("Child")};
    for (const TCHAR* Role : Roles)
    {
        TestTrue(TEXT("Actual named/ambient role is supported"),Supports(Role));
        for (int32 I=0;I<500;++I)
        {
            const double Age=I*.137;
            const auto A=Sample(Role,Age,0.f,false,false,false),B=Sample(Role,Age,0.f,false,false,false);
            TestTrue(TEXT("Stable id/age has deterministic pose without RNG"),A.Head.Equals(B.Head,0.f) && A.Chest.Equals(B.Chest,0.f));
            TestTrue(TEXT("Head inclination stays below eight degrees"),FMath::Abs(A.Head.Pitch)<8.f && FMath::Abs(A.Head.Yaw)<8.f && FMath::Abs(A.Head.Roll)<2.f);
            TestTrue(TEXT("Chest inclination stays below three degrees"),FMath::Abs(A.Chest.Pitch)<3.f && FMath::Abs(A.Chest.Roll)<2.f);
            TestTrue(TEXT("Walking keeps the original authored motion"),Sample(Role,Age,1.f,false,false,false).IsIdentity());
            TestTrue(TEXT("Action clips have full priority including blend-in"),Sample(Role,Age,0.f,true,false,false).IsIdentity());
            TestTrue(TEXT("Combat attention keeps authored cues"),Sample(Role,Age,0.f,false,true,false).IsIdentity());
            TestTrue(TEXT("Reduce Motion removes additive personality"),Sample(Role,Age,0.f,false,false,true).IsIdentity());
        }
    }
    for (const TCHAR* Id : {TEXT("Arrel"),TEXT("Elia"),TEXT("Husk"),TEXT("Thief"),TEXT("VoidWisp"),TEXT("DustCrawler"),TEXT("MemoryLeech"),TEXT("RubbleRat"),TEXT("AshWalker"),TEXT("Unknown")})
        TestTrue(TEXT("Heroes/creatures/unknown ids retain their original poses"),Sample(Id,4.,0.f,false,false,false).IsIdentity());
    const auto Merchant=Sample(TEXT("Malet"),4.,0.f,false,false,false);
    TestTrue(TEXT("Merchant chest and shoulders cannot disturb the ledger hand chain"),Merchant.Chest.IsNearlyZero() && Merchant.LeftShoulder.IsNearlyZero() && Merchant.RightShoulder.IsNearlyZero());
    TestTrue(TEXT("Scholar reads lower than the watchful guard"),Sample(TEXT("Scholar"),0.,0.f,false,false,false).Head.Pitch < Sample(TEXT("Guard"),0.,0.f,false,false,false).Head.Pitch-4.f);
    for (const TCHAR* Role : {TEXT("Malet"),TEXT("Scholar"),TEXT("Guard"),TEXT("Elder")})
    {
        const FString Name=Role,Asset=TEXT("SK_")+Name+(Name==TEXT("Malet") ? TEXT("") : TEXT("_Detail"));
        auto* Mesh=LoadObject<USkeletalMesh>(nullptr,*(TEXT("/Game/Memoria/Presentation/Field3D/HumanoidAnatomyS360/R7/")+Name+TEXT("/")+Asset+TEXT(".")+Asset));
        if (!TestNotNull(TEXT("Current anatomical role mesh loads"),Mesh)) continue;
        const auto& Ref=Mesh->GetRefSkeleton();
        for (const TCHAR* Bone : {TEXT("spine_03"),TEXT("clavicle_l"),TEXT("clavicle_r"),TEXT("neck_01"),TEXT("head")})
            TestTrue(TEXT("Existing rig supplies each additive upper-body bone"),Ref.FindBoneIndex(Bone)!=INDEX_NONE);
    }
    return true;
}
#endif
