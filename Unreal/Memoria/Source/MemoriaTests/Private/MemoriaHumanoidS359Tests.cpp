#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "Tests/AutomationCommon.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Engine/SkeletalMesh.h"
#include "Narrative/MemoriaNarrativeSubsystem.h"
#include "Presentation/MemoriaFieldCharacterComponent.h"
#include "Presentation/MemoriaFieldAnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Animation/AnimSequence.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "SkeletalRenderPublic.h"
#include "Rendering/SkinWeightVertexBuffer.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "HighResScreenshot.h"
#if WITH_DEV_AUTOMATION_TESTS
namespace {
const TCHAR* Names[]={TEXT("Traveler"),TEXT("Bureauagent"),TEXT("Guard"),TEXT("Villagerf"),TEXT("Villagerm"),TEXT("Fisherman"),TEXT("Elder"),TEXT("Child"),TEXT("Scholar"),TEXT("Elia"),TEXT("Malet"),TEXT("AshWalker"),TEXT("BeltScavenger")};
class FS359Replay final:public IAutomationLatentCommand
{
 FAutomationTestBase* Test;double Started=FPlatformTime::Seconds();uint64 Last=0;int32 Index=0,Frame=0;
 bool Fixed=false,OldFixed=false;double OldDelta=0;TWeakObjectPtr<AActor> Actor;TWeakObjectPtr<ACameraActor> Camera;TWeakObjectPtr<UMemoriaFieldCharacterComponent> Figure;
 float Height=180,PreviousPhase=0,MaxFootDrift=0;int32 FootSamples=0;FVector PreviousFeet[2];bool PreviousSupport[2]={false,false};bool FootReady=false;
 void Capture(const FString& View)
 {if(FParse::Param(FCommandLine::Get(),TEXT("MemoriaCapture")))FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Validation/HumanoidS359")/(FString(Names[Index])+View+TEXT(".png")),true,false);}
 void Look(bool Profile=false,bool Close=false)
 {
  auto* Skin=Figure->GetSkeletalMesh();const FVector Base=Actor->GetActorLocation();
  const FVector Target=Close?Skin->GetSocketLocation(TEXT("head"))+FVector(0,0,Height*.060f):Base+FVector(0,0,Height*.43f);
  const FVector Direction=Profile?Skin->GetRightVector():Skin->GetForwardVector();
  const FVector Eye=Target+Direction*Height*(Close?.85f:3.1f)+(Profile?FVector::ZeroVector:Skin->GetRightVector()*Height*.05f)+FVector(0,0,Height*(Close?.025f:.10f));
  Camera->SetActorLocation(Eye);Camera->SetActorRotation((Target-Eye).Rotation());Camera->GetCameraComponent()->SetFieldOfView(Close?35.f:42.f);GEditor->PlayWorld->GetFirstPlayerController()->SetViewTarget(Camera.Get());
 }
 void CheckBody(const TCHAR* Pose)
 {
  auto* Skin=Figure->GetSkeletalMesh();TArray<FFinalSkinVertex> Vertices;Skin->GetCPUSkinnedVertices(Vertices,0);FBox Body(ForceInit),Head(ForceInit);int32 Finite=0,Near=0;
  const auto& LOD=Skin->GetSkeletalMeshRenderData()->LODRenderData[0];const auto* Weights=LOD.GetSkinWeightVertexBuffer();const auto& Ref=Skin->GetSkeletalMeshAsset()->GetRefSkeleton();
  for(const auto& V:Vertices)
  {const FVector P=Skin->GetComponentTransform().TransformPosition(FVector(V.Position));if(P.ContainsNaN())continue;++Finite;Body+=P;Near+=FVector::Dist(P,Actor->GetActorLocation()+FVector(0,0,Height*.5f))<Height;}
  Test->TestTrue(TEXT("S359 live skinning is finite and remains around the actor"),Vertices.Num()>1000 && Finite==Vertices.Num() && Near>Vertices.Num()*.98f);
  if(Index<11)
  {
   for(const auto& Section:LOD.RenderSections)
   {
    const TConstArrayView<FBoneIndexType> Map=Section.BoneMap.IsEmpty()?LOD.GetUnifiedBoneMap():TConstArrayView<FBoneIndexType>(Section.BoneMap);
    for(uint32 I=Section.BaseVertexIndex;I<FMath::Min(uint32(Vertices.Num()),Section.BaseVertexIndex+Section.NumVertices);++I)
     for(uint32 J=0;J<Weights->GetMaxBoneInfluences();++J)
     {const uint32 Bone=Weights->GetBoneIndex(I,J);if(Bone<uint32(Map.Num()) && Weights->GetBoneWeight(I,J)>52000 && Ref.GetBoneName(Map[Bone])==TEXT("head")){Head+=FVector(Vertices[I].Position);break;}}
   }
   const FVector Size=Head.GetSize();
   Test->TestTrue(TEXT("S359 NPC head has natural bounded proportions instead of the old oversized face"),Head.IsValid && FMath::Max(Size.X,Size.Y)<Height*(Index==7?.18f:.16f) && Size.Z>Height*.08f && Size.Z<Height*.25f);
  }
  if(Index>=11)
  {
   FBox FaceBounds(ForceInit),OutfitBounds(ForceInit);
   for(const auto& Section:LOD.RenderSections)for(uint32 I=Section.BaseVertexIndex;I<FMath::Min(uint32(Vertices.Num()),Section.BaseVertexIndex+Section.NumVertices);++I)
   {const FVector P=Skin->GetComponentTransform().TransformPosition(FVector(Vertices[I].Position));if(Section.MaterialIndex==2)FaceBounds+=P;else OutfitBounds+=P;}
   const double Gap=FaceBounds.Min.Z-OutfitBounds.Max.Z;
   Test->TestTrue(TEXT("S360 humanoid face/neck geometry reaches its torso instead of floating above it"),FaceBounds.IsValid && OutfitBounds.IsValid && Gap<Height*.015);
   Test->AddInfo(FString::Printf(TEXT("S360_NECK_ATTACHMENT %s pose=%s vertical_gap_cm=%.6f"),Names[Index],Pose,Gap));
  }
  Test->TestTrue(TEXT("S359 body stands with coherent human height and ground support"),Body.IsValid && Body.GetSize().Z>Height*.78f && Body.GetSize().Z<Height*1.10f && FMath::Abs(Body.Min.Z-(Actor->GetActorLocation().Z-8.f))<Height*.04f);
  Test->AddInfo(FString::Printf(TEXT("S359_NATIVE_BODY %s pose=%s vertices=%d body=%s head=%s min_ground=%.4f"),Names[Index],Pose,Vertices.Num(),*Body.GetSize().ToString(),*Head.GetSize().ToString(),Body.Min.Z-Actor->GetActorLocation().Z+8.f));
 }
 void Feet()
 {
  auto* Skin=Figure->GetSkeletalMesh();auto* Anim=Cast<UMemoriaFieldAnimInstance>(Skin->GetAnimInstance());const float Phase=Figure->GaitPhase();
  for(int32 I=0;I<2;++I)
  {
   const FVector P=Skin->GetSocketLocation(I==0?TEXT("foot_l"):TEXT("foot_r"));const float Local=FMath::Fmod(Phase+(I==0?0.f:.5f),1.f);const bool Support=Local<.5f;
   if(FootReady && Anim && Anim->Weight>.98f && Support && PreviousSupport[I] && Phase>PreviousPhase && Phase-PreviousPhase<.1f)
   {MaxFootDrift=FMath::Max(MaxFootDrift,FVector::Dist2D(P,PreviousFeet[I]));++FootSamples;}
   PreviousFeet[I]=P;PreviousSupport[I]=Support;
  }
  PreviousPhase=Phase;FootReady=true;
 }
public:
 explicit FS359Replay(FAutomationTestBase* T):Test(T){}
 ~FS359Replay(){if(Actor.IsValid())Actor->Destroy();if(Camera.IsValid())Camera->Destroy();if(Fixed){FApp::SetUseFixedTimeStep(OldFixed);FApp::SetFixedDeltaTime(OldDelta);}}
 bool Update() override
 {
  if(Last==GFrameCounter)return false;Last=GFrameCounter;
  if(FPlatformTime::Seconds()-Started>480){Test->AddError(TEXT("S359 motion replay timed out"));return true;}
  UWorld* World=GEditor->PlayWorld;auto* PC=World?World->GetFirstPlayerController():nullptr;if(!PC || !PC->GetPawn() || World->GetTimeSeconds()<.3)return false;
  auto* Story=World->GetGameInstance()->GetSubsystem<UMemoriaNarrativeSubsystem>();
  if(!Fixed){Fixed=true;OldFixed=FApp::UseFixedTimeStep();OldDelta=FApp::GetFixedDeltaTime();FApp::SetUseFixedTimeStep(true);FApp::SetFixedDeltaTime(1./60.);for(int32 I=0;I<20 && Story->GetState()==EMemoriaSliceState::VN;++I)Story->Confirm(1);return false;}
  if(!World->GetMapName().EndsWith(TEXT("L_VerdanHost")) || !Story || Story->GetState()!=EMemoriaSliceState::Exploration)return false;
  if(Index>=UE_ARRAY_COUNT(Names))return true;if(!Camera.IsValid())Camera=World->SpawnActor<ACameraActor>();
  if(Frame>0 && (!Actor.IsValid() || !Figure.IsValid() || !Figure->GetSkeletalMesh())){Test->AddError(TEXT("S359 replay figure lost"));return true;}
  if(Frame==0)
  {
   Actor=World->SpawnActor<AActor>();auto* Root=NewObject<USceneComponent>(Actor.Get());Actor->SetRootComponent(Root);Root->RegisterComponent();FVector At=PC->GetPawn()->GetActorLocation()+FVector(-200,-240,0);At.Z=0;Actor->SetActorLocation(At);
   auto* F=NewObject<UMemoriaFieldCharacterComponent>(Actor.Get());F->SetupAttachment(Root);F->RegisterComponent();Figure=F;
   const FString Name=Names[Index],A=TEXT("SK_")+Name+(Index<9?TEXT("_Detail"):TEXT(""));
   const FString Path=(Index<11?TEXT("/Game/Memoria/Presentation/Field3D/HumanoidAnatomyS360/R7/"):TEXT("/Game/Memoria/Presentation/Field3D/HumanoidAnatomyS360/R10/"))+Name+TEXT("/")+A+TEXT(".")+A;
   const FString ActivePath=Name==TEXT("Elia")?TEXT("/Game/Memoria/Presentation/Field3D/HeroPolishS361/R4/Elia/SK_Elia.SK_Elia"):Path;auto* Mesh=LoadObject<USkeletalMesh>(nullptr,*ActivePath);if(!Test->TestNotNull(TEXT("S359 delivered mesh loads"),Mesh))return true;Height=Mesh->GetBounds().BoxExtent.Z*2;
   if(Index<11){if(!F->InitializeCharacter(Name.ToLower(),Height))return true;}
   else{FMemoriaFoeLook L;L.Model=Names[Index];L.Height=Height;L.Idle=Index==11?TEXT("Zombie_Idle_Loop"):TEXT("Idle");L.Walk=Index==11?TEXT("Zombie_Walk_Fwd_Loop"):TEXT("Walk");if(!F->InitializeFoe(L))return true;}
   F->Face(TEXT("Right"));auto* Skin=F->GetSkeletalMesh();Test->TestEqual(TEXT("S359 real figure binds delivered mesh"),Skin->GetSkeletalMeshAsset(),Mesh);
   auto* Anim=Cast<UMemoriaFieldAnimInstance>(Skin->GetAnimInstance());
   const auto& ColorLOD=Skin->GetSkeletalMeshRenderData()->LODRenderData[0];const auto& Colors=ColorLOD.StaticVertexBuffers.ColorVertexBuffer;int32 Pigmented=0,PaintCount=0;
   if(Index<11 && Colors.GetAllowCPUAccess())for(const auto& Section:ColorLOD.RenderSections)if(Section.MaterialIndex==(Index<9?0:2))
    for(uint32 I=Section.BaseVertexIndex;I<Section.BaseVertexIndex+Section.NumVertices;++I){const FColor C=Colors.VertexColor(I);++PaintCount;Pigmented+=C.R<250 || C.G<250 || C.B<250;}
   if(Index<11)Test->TestTrue(TEXT("S359 actual face/cloth section color buffer retains authored pigment"),Colors.GetNumVertices()==ColorLOD.GetNumVertices() && PaintCount>1000 && Pigmented>PaintCount*.99f);
   Test->TestTrue(TEXT("S359 idle and walk actually use the new compatible motion"),Anim && Anim->Idle && Anim->Walk && Anim->Idle->GetPathName().Contains(Index<11?TEXT("HumanoidAnatomyS360/R7/"):TEXT("HumanoidAnatomyS360/R8/")) && Anim->Walk->GetPathName().Contains(Index<11?TEXT("HumanoidAnatomyS360/R7/"):TEXT("HumanoidAnatomyS360/R8/")) && Anim->Idle->GetSkeleton()==Mesh->GetSkeleton() && Anim->Walk->GetSkeleton()==Mesh->GetSkeleton());
   FootReady=false;MaxFootDrift=0;FootSamples=0;Look();
  }
  if(Frame==20){CheckBody(TEXT("idle"));Capture(TEXT("Front"));}
  if(Frame==25)Look(true);if(Frame==35)Capture(TEXT("Profile"));
  if(Frame==40)Look(false,true);if(Frame==50)Capture(TEXT("Face"));
  if(Frame>=70 && Frame<230)
  {
   Feet();if(Frame%20==0)CheckBody(TEXT("walk"));if(Frame==105 || Frame==125 || Frame==145 || Frame==165)Capture(FString::Printf(TEXT("Walk%03d"),Frame));
   if((Index==5 || Index>=11) && Frame>=90 && Frame<=210 && Frame%3==0)Capture(FString::Printf(TEXT("Motion%03d"),Frame));
   const FVector Step(.5f,0,0);Figure->AdvanceLocomotion(Step,1.f/60.f);Actor->AddActorWorldOffset(Step);Look();
  }
  if(Frame>=230)Figure->AdvanceLocomotion(FVector::ZeroVector,1.f/60.f);
  if(Frame==270)
  {
   CheckBody(TEXT("settled"));Capture(TEXT("Settled"));Test->TestTrue(TEXT("S359 stopping settles smoothly back to idle"),Figure->LocomotionWeight()<.02f);
   Test->TestTrue(TEXT("S359 planted foot stays in place during actual actor travel"),FootSamples>80 && MaxFootDrift<.25f);
   Test->AddInfo(FString::Printf(TEXT("S359_FOOT_CONTACT %s grounded_samples=%d max_world_drift_per_half_cm_step=%.6f"),Names[Index],FootSamples,MaxFootDrift));
  }
  if(Frame==280){Actor->Destroy();Actor.Reset();Figure.Reset();++Index;Frame=0;return false;}
  ++Frame;return false;
 }
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHumanoidS359Test,"MemoriaGraphics.S359",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FHumanoidS359Test::RunTest(const FString&)
{if(!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice")))return false;FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FS359Replay(this)));ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());return true;}
#endif
