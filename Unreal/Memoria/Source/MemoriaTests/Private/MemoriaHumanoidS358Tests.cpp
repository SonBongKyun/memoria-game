#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "Tests/AutomationCommon.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Engine/SkeletalMesh.h"
#include "Narrative/MemoriaNarrativeSubsystem.h"
#include "Presentation/MemoriaFieldCharacterComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "SkeletalRenderPublic.h"
#include "Rendering/SkinWeightVertexBuffer.h"
#include "HighResScreenshot.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#if WITH_DEV_AUTOMATION_TESTS
namespace {
const TCHAR* Names[]={TEXT("Traveler"),TEXT("Bureauagent"),TEXT("Guard"),TEXT("Villagerf"),TEXT("Villagerm"),TEXT("Fisherman"),TEXT("Elder"),TEXT("Child"),TEXT("Scholar"),TEXT("Elia"),TEXT("Malet")};
class FS358Replay final:public IAutomationLatentCommand
{
 FAutomationTestBase* Test;double Started=FPlatformTime::Seconds();uint64 Last=0;int32 Index=0,Frame=0;
 bool Fixed=false,OldFixed=false;double OldDelta=0;TWeakObjectPtr<AActor> Actor;TWeakObjectPtr<ACameraActor> Camera;TWeakObjectPtr<UMemoriaFieldCharacterComponent> Figure;float Height=180;FTransform Leg;
 void Capture(const TCHAR* View)
 {if(FParse::Param(FCommandLine::Get(),TEXT("MemoriaCapture")))FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Validation/HumanoidS358")/(FString(Names[Index])+View+TEXT(".png")),true,false);}
 void Look(bool Close=false)
 {
  auto* Skin=Figure->GetSkeletalMesh();const FVector Base=Actor->GetActorLocation();
  const FVector Target=Close?Skin->GetSocketLocation(TEXT("head"))+FVector(0,0,Height*.045f):Base+FVector(0,0,Height*.42f);
  const FVector Forward=Skin->GetForwardVector();const FVector Right=Skin->GetRightVector();
  const FVector Eye=Target+Forward*Height*(Close?.75f:3.6f)+Right*Height*(Close?.10f:.6f)+FVector(0,0,Height*(Close?.025f:.18f));
  Camera->SetActorLocation(Eye);Camera->SetActorRotation((Target-Eye).Rotation());Camera->GetCameraComponent()->SetFieldOfView(Close?35.f:40.f);
  GEditor->PlayWorld->GetFirstPlayerController()->SetViewTarget(Camera.Get());
 }
 void CheckPose(const TCHAR* Pose)
 {
  auto* Skin=Figure->GetSkeletalMesh();TArray<FFinalSkinVertex> V;Skin->GetCPUSkinnedVertices(V,0);int32 Finite=0,Above=0,Near=0;FBox Body(ForceInit);
  for(const auto& Vertex:V)
  {const FVector P=Skin->GetComponentTransform().TransformPosition(FVector(Vertex.Position));if(P.ContainsNaN())continue;++Finite;Body+=P;Above+=P.Z>=Actor->GetActorLocation().Z-8.f-Height*.03f;Near+=FVector::Dist(P,Actor->GetActorLocation()+FVector(0,0,Height*.5f))<Height;}
  const float N=FMath::Max(1,V.Num());
  Test->TestTrue(TEXT("S358 actual posed NPC geometry remains finite, grounded and near its actor"),V.Num()>1000 && Finite==V.Num() && Above/N>.98 && Near/N>.98);
  Test->TestTrue(TEXT("S358 clothes retain a human body volume"),Body.IsValid && Body.GetSize().Z>Height*.65f && Body.GetSize().Z<Height*1.35f && Body.GetSize().GetMin()>Height*.05f);
  if(Index<9)
  {
   const auto& LOD=Skin->GetSkeletalMeshRenderData()->LODRenderData[0];const auto* Weights=LOD.GetSkinWeightVertexBuffer();const auto& Ref=Skin->GetSkeletalMeshAsset()->GetRefSkeleton();
   int32 Mixed=0,Attached=0;FBox NeckBox(ForceInit);const FVector A=Skin->GetSocketLocation(TEXT("neck_01")),B=Skin->GetSocketLocation(TEXT("head")),AB=B-A;
   for(const auto& Section:LOD.RenderSections)
   {
    const TConstArrayView<FBoneIndexType> BoneMap=Section.BoneMap.IsEmpty()?LOD.GetUnifiedBoneMap():TConstArrayView<FBoneIndexType>(Section.BoneMap);
    for(uint32 I=Section.BaseVertexIndex;I<FMath::Min(uint32(V.Num()),Section.BaseVertexIndex+Section.NumVertices);++I)
    {
     bool Neck=false,Head=false;for(uint32 J=0;J<Weights->GetMaxBoneInfluences();++J)if(Weights->GetBoneWeight(I,J)>0){const uint32 Local=Weights->GetBoneIndex(I,J);if(Local>=uint32(BoneMap.Num()))continue;const FName Bone=Ref.GetBoneName(BoneMap[Local]);Neck|=Bone==TEXT("neck_01");Head|=Bone==TEXT("head");}
     if(!Neck || !Head)continue;++Mixed;const FVector P=Skin->GetComponentTransform().TransformPosition(FVector(V[I].Position));NeckBox+=P;
     const double T=FMath::Clamp(FVector::DotProduct(P-A,AB)/FMath::Max(.0001,AB.SizeSquared()),0.,1.);Attached+=FVector::Dist(P,A+AB*T)<Height*(Index==7?.10:.08);
    }
   }
   Test->TestTrue(TEXT("S358 actual blended neck has visible volume and follows both neck and head"),Mixed>=64 && Mixed==Attached && NeckBox.IsValid && NeckBox.GetSize().GetMin()>Height*.015 && NeckBox.GetSize().GetMax()<Height*.25);
   Test->AddInfo(FString::Printf(TEXT("S358_NECK %s %s mixed=%d attached=%d bounds=%s"),Names[Index],Pose,Mixed,Attached,*NeckBox.GetSize().ToString()));
  }
  Test->AddInfo(FString::Printf(TEXT("S358_NPC_POSE %s %s vertices=%d finite=%d above=%.3f near=%.3f bounds=%s"),Names[Index],Pose,V.Num(),Finite,Above/N,Near/N,*Body.GetSize().ToString()));
 }
public:
 explicit FS358Replay(FAutomationTestBase* In):Test(In){}
 ~FS358Replay(){if(Fixed){FApp::SetUseFixedTimeStep(OldFixed);FApp::SetFixedDeltaTime(OldDelta);}}
 bool Update() override
 {
  if(Last==GFrameCounter)return false;Last=GFrameCounter;
  if(FPlatformTime::Seconds()-Started>300){Test->AddError(TEXT("S358 NPC inspection timed out"));return true;}
  UWorld* World=GEditor->PlayWorld;auto* PC=World?World->GetFirstPlayerController():nullptr;
  if(!PC || !PC->GetPawn() || World->GetTimeSeconds()<.3)return false;
  if(!Fixed){Fixed=true;OldFixed=FApp::UseFixedTimeStep();OldDelta=FApp::GetFixedDeltaTime();FApp::SetUseFixedTimeStep(true);FApp::SetFixedDeltaTime(1.0/60.0);auto* Story=World->GetGameInstance()->GetSubsystem<UMemoriaNarrativeSubsystem>();for(int32 I=0;I<20 && Story->GetState()==EMemoriaSliceState::VN;++I)Story->Confirm(1);return false;}
  auto* Story=World->GetGameInstance()->GetSubsystem<UMemoriaNarrativeSubsystem>();
  if(!World->GetMapName().EndsWith(TEXT("L_VerdanHost")) || !Story || Story->GetState()!=EMemoriaSliceState::Exploration)return false;
  if(!Camera.IsValid())Camera=World->SpawnActor<ACameraActor>();
  if(Index>=UE_ARRAY_COUNT(Names))return true;
  if(Frame>0 && (!Actor.IsValid() || !Figure.IsValid() || !Figure->GetSkeletalMesh())){Test->AddError(TEXT("S358 inspection actor was lost after creation"));return true;}
  if(Frame==0)
  {
   Actor=World->SpawnActor<AActor>();auto* Root=NewObject<USceneComponent>(Actor.Get());Actor->SetRootComponent(Root);Root->RegisterComponent();FVector At=PC->GetPawn()->GetActorLocation()+FVector(-140,-220,0);At.Z=0;Actor->SetActorLocation(At);
   auto* F=NewObject<UMemoriaFieldCharacterComponent>(Actor.Get());F->SetupAttachment(Root);F->RegisterComponent();Figure=F;
   const FString Name=Names[Index],Asset=TEXT("SK_")+Name+(Index<9?TEXT("_Detail"):TEXT(""));const FString Path=TEXT("/Game/Memoria/Presentation/Field3D/HumanoidAnatomyS360/R7/")+Name+TEXT("/")+Asset+TEXT(".")+Asset;
   const FString ActivePath=Name==TEXT("Elia")?TEXT("/Game/Memoria/Presentation/Field3D/HeroPolishS361/R4/Elia/SK_Elia.SK_Elia"):Path;auto* Mesh=LoadObject<USkeletalMesh>(nullptr,*ActivePath);if(!Test->TestNotNull(TEXT("S358 delivered NPC mesh loads"),Mesh))return true;
   Height=Mesh->GetBounds().BoxExtent.Z*2;
   if(!Test->TestTrue(TEXT("S358 NPC initializes through the real presentation component"),F->InitializeCharacter(Name.ToLower(),Height)))return true;
   auto* Skin=F->GetSkeletalMesh();Test->TestEqual(TEXT("S358 live NPC uses the delivered revision"),Skin->GetSkeletalMeshAsset(),Mesh);
   const FString OldPath=Index<9?TEXT("/Game/Memoria/Presentation/Field3D/")+Name+TEXT("/SK_")+Name+TEXT("_Detail.SK_")+Name+TEXT("_Detail"):TEXT("/Game/Memoria/Presentation/Field3D/CharacterUpgradeS356/Accepted/")+Name+TEXT("/SK_")+Name+TEXT(".SK_")+Name;
   auto* Old=LoadObject<USkeletalMesh>(nullptr,*OldPath);if(!Test->TestNotNull(TEXT("S358 original NPC remains for comparison"),Old))return true;
   Test->TestTrue(TEXT("S360 NPC has a new anatomy rig with all compatible bone names"),Mesh->GetSkeleton()!=Old->GetSkeleton() && Mesh->GetRefSkeleton().GetNum()==77);Test->TestEqual(TEXT("NPC current slot count is natural surface or original garments plus face"),Skin->GetNumMaterials(),Index<9?1:(Index==9?4:3));
   auto* Surface=LoadObject<UMaterialInterface>(nullptr,Index==9?TEXT("/Game/Memoria/Presentation/Field3D/HeroPolishS361/R4/M_EliaFace.M_EliaFace"):TEXT("/Game/Memoria/Presentation/Field3D/HumanoidAnatomyS360/R7/M_NaturalHuman.M_NaturalHuman"));
   if(Index<9)Test->TestEqual(TEXT("Ambient NPC uses the delivered natural skin/cloth material"),Skin->GetMaterial(0),Surface);
   else {for(int32 I=0;I<2;++I)Test->TestEqual(TEXT("Named NPC preserves actual painted garment materials"),Skin->GetMaterial(I),Old->GetMaterials()[I].MaterialInterface.Get());Test->TestEqual(TEXT("Named NPC face uses the delivered natural surface"),Skin->GetMaterial(2),Surface);}
   Test->TestEqual(TEXT("S358 NPC adds no gameplay collision"),Skin->GetCollisionEnabled(),ECollisionEnabled::NoCollision);
   Test->TestFalse(TEXT("S358 NPC adds no overlap events"),Skin->GetGenerateOverlapEvents());
   Test->TestTrue(TEXT("S360 source hair stays within half a centimeter; runtime normalizes requested height"),FMath::IsNearlyEqual(Height,float(Old->GetBounds().BoxExtent.Z*2),.5f));Look();
  }
  if(Frame==20){CheckPose(TEXT("idle"));Capture(TEXT("Front"));}
  if(Frame==23)Look(true);
  if(Frame==35)Capture(TEXT("Detail"));
  if(Frame>=42 && Frame<75)Figure->AdvanceLocomotion(FVector(4,0,0),1.f/60.f);
  if(Frame==42)Leg=Figure->GetSkeletalMesh()->GetSocketTransform(TEXT("calf_l"));
  if(Frame==52)Look();
  if(Frame==68){CheckPose(TEXT("walk"));Test->TestFalse(TEXT("S358 original walk clip moves its live leg"),Figure->GetSkeletalMesh()->GetSocketTransform(TEXT("calf_l")).Equals(Leg,.05f));Capture(TEXT("Walk"));}
  if(Frame==83){Actor->Destroy();Actor.Reset();Figure.Reset();++Index;Frame=0;return false;}
  ++Frame;return false;
 }
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHumanoidS358Test,"MemoriaGraphics.S358",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FHumanoidS358Test::RunTest(const FString&)
{if(!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice")))return false;FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FS358Replay(this)));ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());return true;}
#endif
