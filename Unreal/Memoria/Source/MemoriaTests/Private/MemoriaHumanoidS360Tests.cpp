#include "Misc/AutomationTest.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/Skeleton.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAnatomyS360Test,"MemoriaGraphics.S360",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FAnatomyS360Test::RunTest(const FString&)
{
 for(const TCHAR* N:{TEXT("Traveler"),TEXT("Bureauagent"),TEXT("Guard"),TEXT("Villagerf"),TEXT("Villagerm"),TEXT("Fisherman"),TEXT("Elder"),TEXT("Child"),TEXT("Scholar"),TEXT("Elia"),TEXT("Malet"),TEXT("AshWalker"),TEXT("BeltScavenger")})
 {
  const FString Name=N;const bool Ambient=Name!=TEXT("Elia") && Name!=TEXT("Malet") && Name!=TEXT("AshWalker") && Name!=TEXT("BeltScavenger");const FString A=TEXT("SK_")+Name+(Ambient?TEXT("_Detail"):TEXT(""));
  const bool Foe=Name==TEXT("AshWalker") || Name==TEXT("BeltScavenger");
  auto* Mesh=LoadObject<USkeletalMesh>(nullptr,*((Foe?FString(TEXT("/Game/Memoria/Presentation/Field3D/HumanoidAnatomyS360/R10/")):FString(TEXT("/Game/Memoria/Presentation/Field3D/HumanoidAnatomyS360/R7/")))+Name+TEXT("/")+A+TEXT(".")+A));if(!TestNotNull(TEXT("Anatomical mesh loads"),Mesh))continue;
  const auto& Ref=Mesh->GetRefSkeleton();TestEqual(TEXT("Anatomy retains 77 role bones"),Ref.GetNum(),77);TArray<FTransform> Global=Ref.GetRefBonePose();
  for(int32 I=0;I<Global.Num();++I)if(Ref.GetParentIndex(I)!=INDEX_NONE)Global[I]=Global[I]*Global[Ref.GetParentIndex(I)];
  auto P=[&](const TCHAR* Bone){return Global[Ref.FindBoneIndex(Bone)].GetLocation();};const double H=Mesh->GetBounds().BoxExtent.Z*2;
  TestTrue(TEXT("Human shoulder joint height follows natural upper torso"),P(TEXT("upperarm_l")).Z/H>.70 && P(TEXT("upperarm_l")).Z/H<.87);
  TestTrue(TEXT("Human hip joints have bounded natural separation"),FVector::Dist(P(TEXT("thigh_l")),P(TEXT("thigh_r")))/H>.08 && FVector::Dist(P(TEXT("thigh_l")),P(TEXT("thigh_r")))/H<.19);
  TestTrue(TEXT("Elbow and knee lie between their anatomical endpoints"),P(TEXT("upperarm_l")).Z>P(TEXT("lowerarm_l")).Z && P(TEXT("lowerarm_l")).Z>P(TEXT("hand_l")).Z && P(TEXT("thigh_l")).Z>P(TEXT("calf_l")).Z && P(TEXT("calf_l")).Z>P(TEXT("foot_l")).Z);
  AddInfo(FString::Printf(TEXT("S360_ANATOMY %s height=%.3f shoulder=%.3f hip_width=%.3f"),N,H,P(TEXT("upperarm_l")).Z,FVector::Dist(P(TEXT("thigh_l")),P(TEXT("thigh_r")))));
 }
 return true;
}
#endif
