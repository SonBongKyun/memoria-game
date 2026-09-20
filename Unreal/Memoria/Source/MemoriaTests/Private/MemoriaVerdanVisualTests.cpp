#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "Tests/AutomationCommon.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "Framework/MemoriaSliceHost.h"
#include "Framework/MemoriaFieldPawn.h"
#include "Interaction/MemoriaMaletActor.h"
#include "Presentation/MemoriaVerdanPresentation.h"
#include "Presentation/MemoriaArrel3DComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Presentation/MemoriaVerdanArt.h"
#include "Narrative/MemoriaNarrativeSubsystem.h"
#include "Run/MemoriaRunSubsystem.h"
#include "PaperSpriteComponent.h"
#include "PaperSprite.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "Camera/CameraActor.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"
#include "InputKeyEventArgs.h"
#include "JsonObjectConverter.h"
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "UnrealClient.h"
#if WITH_DEV_AUTOMATION_TESTS
namespace
{
class FVerdanVisualReplay final : public IAutomationLatentCommand
{
public:
    explicit FVerdanVisualReplay(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
    ~FVerdanVisualReplay() override
    { if (bFixed) { FApp::SetUseFixedTimeStep(bOldFixed); FApp::SetFixedDeltaTime(OldDelta); } }
    bool Update() override
    {
        if (LastFrame == GFrameCounter) return false;
        LastFrame = GFrameCounter;
        if (FPlatformTime::Seconds() - Started > 100) { Test->AddError(TEXT("Verdan visual replay timeout")); return true; }
        UWorld* World = GEditor->PlayWorld;
        auto* PC = World ? Cast<AMemoriaSliceController>(World->GetFirstPlayerController()) : nullptr;
        auto* Pawn = PC ? Cast<AMemoriaFieldPawn>(PC->GetPawn()) : nullptr;
        if (!Pawn || !PC->PlayerInput || World->GetTimeSeconds() < 0.3) return false;
        auto* Host = World->GetGameInstance()->GetSubsystem<UMemoriaNarrativeSubsystem>();
        auto* Run = World->GetGameInstance()->GetSubsystem<UMemoriaRunSubsystem>();
        if (!bFixed)
        {
            bFixed = true; bOldFixed = FApp::UseFixedTimeStep(); OldDelta = FApp::GetFixedDeltaTime();
            FApp::SetUseFixedTimeStep(true); FApp::SetFixedDeltaTime(1.0/60.0);
            // Enter the existing paid route; campaign tests separately replay its physical UI inputs.
            for (int32 I = 0; I < 20 && Host->GetState() == EMemoriaSliceState::VN; ++I) Host->Confirm(1);
            return false;
        }
        if (!World->GetMapName().EndsWith(TEXT("L_VerdanHost"))) return false;
        // Shader compilation can pump engine frames without ticking the PIE world.
        if (World->GetTimeSeconds() == LastWorldTime) return false;
        LastWorldTime = World->GetTimeSeconds();
        AMemoriaVerdanPresentation* Presentation = nullptr;
        int32 Count = 0;
        for (TActorIterator<AMemoriaVerdanPresentation> It(World); It; ++It) { Presentation = *It; ++Count; }
        if (!Test->TestEqual(TEXT("One presentation layer after travel"), Count, 1)) return true;
        auto* Character=Presentation->CharacterMesh();
        if(!Test->TestNotNull(TEXT("Actual skeletal character in field"),Character))return true;
        auto Key = [&](FKey K, EInputEvent Event) { PC->InputKey(FInputKeyEventArgs::CreateSimulated(K, Event, Event == IE_Released ? 0.0f : 1.0f)); };
        auto Capture = [&](const TCHAR* Name)
        { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Validation/Character1")/(FString(Name)+TEXT(".png")), true, false); };
        if (Frame == 0)
        {
            FJsonObjectConverter::UStructToJsonObjectString(Run->GetRunSnapshot(), RunBefore);
            FJsonObjectConverter::UStructToJsonObjectString(Run->GetPlayerMemory()->GetSnapshot(), MemoryBefore);
            TraceBefore = Host->GetTrace();
            TArray<UPrimitiveComponent*> Components; Presentation->GetComponents(Components);
            for (auto* Component : Components) Test->TestTrue(TEXT("Presentation cannot block movement or visibility traces"), Component->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
            int32 Bodies = 0;
            for (TActorIterator<AStaticMeshActor> It(World); It; ++It)
            {
                Test->TestTrue(TEXT("Existing geometry remains collidable"), It->GetStaticMeshComponent()->GetCollisionEnabled() != ECollisionEnabled::NoCollision);
                ++Bodies;
            }
            Test->TestEqual(TEXT("All seven original physical surfaces retained"), Bodies, 7);
            Test->TestTrue(TEXT("Pawn collider remains 16 x 16"), Cast<UBoxComponent>(Pawn->GetRootComponent())->GetUnscaledBoxExtent().Equals(FVector(8, 8, 8)));
            const auto* Camera = Pawn->GetFieldCamera();
            Test->TestTrue(TEXT("Verdan uses perspective"), Camera->ProjectionMode == ECameraProjectionMode::Perspective);
            Test->TestEqual(TEXT("Bounded field of view"), Camera->FieldOfView, 65.0f);
            Test->TestTrue(TEXT("Oblique camera elevation"), FMath::IsNearlyEqual(Camera->GetComponentRotation().Pitch, -42.0, 0.1));
            Test->TestTrue(TEXT("Player actually receives perspective view"), PC->PlayerCameraManager->GetCameraCacheView().ProjectionMode == ECameraProjectionMode::Perspective);
            const FVector P = Pawn->GetActorLocation();
            FVector2D Origin, Right, Back, Up, NearLow, NearHigh, FarLow, FarHigh;
            Test->TestTrue(TEXT("Origin projects"), PC->ProjectWorldLocationToScreen(P, Origin));
            PC->ProjectWorldLocationToScreen(P+FVector(100,0,0), Right);
            PC->ProjectWorldLocationToScreen(P+FVector(0,100,0), Back);
            PC->ProjectWorldLocationToScreen(P+FVector(0,0,100), Up);
            Test->TestTrue(TEXT("World X remains screen right"), Right.X > Origin.X);
            Test->TestTrue(TEXT("World Y remains screen up"), Back.Y < Origin.Y);
            Test->TestTrue(TEXT("Geometry height projects upward"), Up.Y < Origin.Y);
            PC->ProjectWorldLocationToScreen(P+FVector(0,-300,0), NearLow);
            PC->ProjectWorldLocationToScreen(P+FVector(100,-300,0), NearHigh);
            PC->ProjectWorldLocationToScreen(P+FVector(0,300,0), FarLow);
            PC->ProjectWorldLocationToScreen(P+FVector(100,300,0), FarHigh);
            Test->TestTrue(TEXT("Perspective foreshortens distant geometry"), (NearHigh-NearLow).Size() > (FarHigh-FarLow).Size());
            Test->TestTrue(TEXT("Original field sprite hidden behind 3D presentation"), Pawn->GetFieldSprite()->bHiddenInGame);
            auto* SkeletalAsset=Cast<USkeletalMesh>(Character->GetSkinnedAsset());
            if(!Test->TestNotNull(TEXT("Real skeletal mesh asset loaded"),SkeletalAsset))return true;
            Test->TestEqual(TEXT("Editable full-body rig has 23 bones"),SkeletalAsset->GetRefSkeleton().GetNum(),23);
            Test->TestTrue(TEXT("Character uses actual skinning and two lit surface materials"),SkeletalAsset->GetSkeleton()!=nullptr && SkeletalAsset->GetMaterials().Num()==2 && SkeletalAsset->GetHasVertexColors());
            Test->AddInfo(FString::Printf(TEXT("ARREL_BOUNDS %s asset=%s scale=%s"),*Character->Bounds.BoxExtent.ToString(),*SkeletalAsset->GetBounds().BoxExtent.ToString(),*Character->GetComponentScale().ToString()));
            Test->TestTrue(TEXT("Character is volumetric, with readable height"),Character->Bounds.BoxExtent.X>10 && Character->Bounds.BoxExtent.Y>10 && Character->Bounds.BoxExtent.Z>50);
            Test->TestEqual(TEXT("Skeletal feet use the existing visual ground anchor"),Character->GetRelativeLocation().Z,-8.0);
            TArray<UInstancedStaticMeshComponent*> Batches; Presentation->GetComponents(Batches);
            int32 Instances = 0, Roofs = 0;
            for (auto* Batch : Batches)
            {
                Instances += Batch->GetInstanceCount();
                Test->TestTrue(TEXT("Every geometry batch has a mesh"), Batch->GetStaticMesh() != nullptr);
                Test->TestNotNull(TEXT("Every geometry batch has a material"), Batch->GetMaterial(0));
                if (Batch->GetStaticMesh() && Batch->GetStaticMesh()->GetName() == TEXT("SM_PitchedRoof")) Roofs += Batch->GetInstanceCount();
            }
            Test->TestTrue(TEXT("Architecture is instanced real geometry"), Instances > 200 && Batches.Num() < 30);
            Test->TestEqual(TEXT("Six buildings and two physical stall roofs"), Roofs, 8);
            Test->TestTrue(TEXT("Architecture has substantial height"), Presentation->GetComponentsBoundingBox(true).Max.Z > 580);
            auto* Lit = LoadObject<UMaterial>(nullptr,TEXT("/Game/Memoria/Presentation/Depth2/M_FocusSurface.M_FocusSurface"));
            Test->TestTrue(TEXT("Walls respond to real lighting"), Lit && Lit->GetUsageByFlag(MATUSAGE_InstancedStaticMeshes) && Lit->GetShadingModels().HasShadingModel(MSM_DefaultLit));
            Test->TestTrue(TEXT("Occlusion material supports masked reveal"), Lit && Lit->BlendMode == BLEND_Masked);
            int32 ArtLanterns=0;
            TArray<UPaperSpriteComponent*> Pictures; Presentation->GetComponents(Pictures);
            for (auto* Picture:Pictures) if (Picture->GetSprite() && Picture->GetSprite()->GetName()==TEXT("SPR_MemoryLantern")) ++ArtLanterns;
            Test->TestEqual(TEXT("Four original-art lanterns in actual scene"), ArtLanterns, 4);
            TArray<UPointLightComponent*> Lights; Presentation->GetComponents(Lights);
            auto* Fill=Lights.FindByPredicate([](const UPointLightComponent* L){return L->GetFName()==TEXT("ArrelFillLight");});
            Test->TestTrue(TEXT("Character fill is isolated from world lighting"),Fill && !(*Fill)->LightingChannels.bChannel0 && (*Fill)->LightingChannels.bChannel1);
            Lights.RemoveAll([](const UPointLightComponent* L){return L->GetFName()==TEXT("ArrelFillLight");});
            Test->TestEqual(TEXT("Four real lantern lights"), Lights.Num(), 4);
            int32 ShadowLights = 0;
            for (auto* Light : Lights) { Test->TestTrue(TEXT("Lanterns illuminate scene"), Light->Intensity > 0); ShadowLights += Light->CastShadows ? 1 : 0; }
            Test->TestEqual(TEXT("Two bounded shadow casting lanterns"), ShadowLights, 2);
            for (TActorIterator<AMemoriaMaletActor> It(World); It; ++It)
                Test->TestTrue(TEXT("Original Malet position"), It->GetActorLocation().Equals(AMemoriaMaletActor::DevelopmentLocation()));
        }
        const FKey Keys[] = {EKeys::W, EKeys::D, EKeys::S, EKeys::A};
        const TCHAR* Directions[] = {TEXT("Up"), TEXT("Right"), TEXT("Down"), TEXT("Left")};
        for (int32 I = 0; I < 4; ++I)
        {
            const int32 Start = 10 + I * 36;
            if (Frame == Start) Key(Keys[I], IE_Pressed);
            if (Frame > Start + 3 && Frame < Start + 20)
            {
                Test->TestEqual(TEXT("Direction follows real displacement"), Presentation->Facing(), FString(Directions[I]));
                Test->TestTrue(TEXT("Real movement activates skeletal gait"), Presentation->IsWalking());
                const auto& Pose=Character->GetBoneSpaceTransforms();
                SeenGaitPoses.Add(Pose[Character->GetBoneIndex(TEXT("calf_l"))].GetRotation().ToString());
                Test->TestTrue(TEXT("Skeletal root follows physical pawn"),FVector2D(Character->GetComponentLocation()).Equals(FVector2D(Pawn->GetActorLocation()),.001));
                if(Frame>Start+14)
                {
                    const FVector Vectors[]={FVector(0,1,0),FVector(1,0,0),FVector(0,-1,0),FVector(-1,0,0)};
                    Test->TestTrue(TEXT("3D body turns toward real travel"),FVector::DotProduct(Character->GetForwardVector(),Vectors[I])>.98);
                    Test->TestTrue(TEXT("Actual joint gait blends in"),Character->LocomotionWeight()>.6f);
                }
            }
            if (Frame == Start + 18) Capture(Directions[I]);
            if (Frame == Start + 20) Key(Keys[I], IE_Released);
        }
        if (Frame == 156)
        {
            Test->TestTrue(TEXT("Skinned joint pose changes through movement"),SeenGaitPoses.Num()>=8);
            Capture(TEXT("Market"));
        }
        if (Frame == 158)
        {
            Pawn->SetActorLocation(FVector(850, 0, 0)); Key(EKeys::D, IE_Pressed);
        }
        auto Lens=[&](float Strength)
        {
            TArray<UInstancedStaticMeshComponent*> Batches; Presentation->GetComponents(Batches);
            for (auto* Batch:Batches) if (auto* M=Cast<UMaterialInstanceDynamic>(Batch->GetMaterial(0))) M->SetScalarParameterValue(TEXT("FocusStrength"),Strength);
        };
        if (Frame == 188) Lens(0);
        if (Frame == 196)
        {
            Test->TestTrue(TEXT("Original wall still blocks sweep"), Pawn->GetActorLocation().X <= 882.1 && Pawn->GetActorLocation().X > 850);
            Test->TestFalse(TEXT("Blocked input does not slide animated feet"), Presentation->IsWalking());
            Test->TestTrue(TEXT("Blocked movement settles skeletal gait"),Character->LocomotionWeight()<.02f);
            WallX = Pawn->GetActorLocation().X;
            Capture(TEXT("BoundaryOpaque")); Key(EKeys::D, IE_Released);
        }
        if (Frame == 198) Lens(1);
        if (Frame == 208) Capture(TEXT("BoundaryClear"));
        if (Frame == 216) Pawn->SetActorLocation(AMemoriaMaletActor::DevelopmentLocation() + FVector(-65, 0, 0));
        if (Frame == 222)
        {
            Test->TestTrue(TEXT("Original interaction prompt remains reachable"), PC->GetInteractionPrompt().Contains(TEXT("Malet")));
            Capture(TEXT("NearMalet"));
        }
        const FVector Edges[]={FVector(-870,0,0),FVector(870,0,0),FVector(0,570,0),FVector(0,-570,0),FVector(-870,570,0),FVector(870,570,0),FVector(-870,-570,0),FVector(870,-570,0)};
        const TCHAR* Names[]={TEXT("West"),TEXT("East"),TEXT("North"),TEXT("South"),TEXT("NorthWest"),TEXT("NorthEast"),TEXT("SouthWest"),TEXT("SouthEast")};
        for (int32 I=0; I<8; ++I)
        {
            const int32 Start=228+I*18;
            if (Frame==Start) Pawn->SetActorLocation(Edges[I]);
            if (Frame==Start+14)
            {
                auto* Camera=Pawn->GetFieldCamera();
                const FVector Eye=Camera->GetComponentLocation();
                Test->TestTrue(TEXT("Camera stays inside courtyard tracking limits"), FMath::Abs(Eye.X)<550.1 && Eye.Y>-1510.1 && Eye.Y<-869.9);
                int32 Width,Height; PC->GetViewportSize(Width,Height);
                FVector2D Foot,Head;
                Test->TestTrue(TEXT("Boundary feet project to screen"), PC->ProjectWorldLocationToScreen(Character->GetComponentLocation(),Foot));
                Test->TestTrue(TEXT("Boundary head projects to screen"), PC->ProjectWorldLocationToScreen(Character->GetComponentLocation()+FVector(0,0,124),Head));
                Test->TestTrue(TEXT("Entire character stays inside view at edges and corners"), Foot.X>30 && Foot.X<Width-30 && Foot.Y>20 && Foot.Y<Height-20 && Head.X>30 && Head.X<Width-30 && Head.Y>20 && Head.Y<Height-20);
                TArray<UInstancedStaticMeshComponent*> Batches; Presentation->GetComponents(Batches);
                bool FocusTracked=false;
                for (auto* Batch:Batches) if (auto* M=Cast<UMaterialInstanceDynamic>(Batch->GetMaterial(0)))
                {
                    FLinearColor Focus,View;
                    if (M->GetVectorParameterValue(FMaterialParameterInfo(TEXT("OcclusionFocus")),Focus) && M->GetVectorParameterValue(FMaterialParameterInfo(TEXT("OcclusionEye")),View))
                        FocusTracked=FVector(Focus.R,Focus.G,Focus.B).Equals(Character->FocusPosition(),.01) && FVector(View.R,View.G,View.B).Equals(Eye,.01);
                }
                Test->TestTrue(TEXT("Reveal follows 3D character and bounded camera"), FocusTracked);
                if (I>0) CameraRecords+=TEXT(",");
                CameraRecords+=FString::Printf(TEXT("{\"edge\":\"%s\",\"camera\":[%.3f,%.3f,%.3f],\"foot\":[%.3f,%.3f],\"head\":[%.3f,%.3f],\"viewport\":[%d,%d]}"),Names[I],Eye.X,Eye.Y,Eye.Z,Foot.X,Foot.Y,Head.X,Head.Y,Width,Height);
                Capture(Names[I]);
            }
        }
        if(Frame==378)Pawn->SetActorLocation(FVector::ZeroVector);
        if(Frame==390){Character->SetRelativeRotation(FRotator(0,-90,0));Capture(TEXT("MarketFront"));}
        if(Frame==400)
        {
            // A temporary review camera shows the real field mesh at a readable angle.
            PreviewCamera=World->SpawnActor<ACameraActor>();
            PreviewCamera->SetActorLocation(FVector(0,-340,110));
            PreviewCamera->SetActorRotation((Character->FocusPosition()-PreviewCamera->GetActorLocation()).Rotation());
            PreviewCamera->GetCameraComponent()->SetFieldOfView(40);
            PC->SetViewTarget(PreviewCamera.Get());
        }
        if(Frame==406)Capture(TEXT("CharacterFront"));
        if(Frame==410)Character->SetRelativeRotation(FRotator::ZeroRotator);
        if(Frame==420)Capture(TEXT("CharacterSide"));
        if(Frame==424)Character->SetRelativeRotation(FRotator(0,90,0));
        if(Frame==434)Capture(TEXT("CharacterBack"));
        if(Frame==440)
        {
            PC->SetViewTarget(Pawn);if(PreviewCamera.IsValid())PreviewCamera->Destroy();
            Character->SetRelativeRotation(FRotator(0,-90,0));
        }
        if (Frame == 455)
        {
            FString After, MemoryAfter;
            FJsonObjectConverter::UStructToJsonObjectString(Run->GetRunSnapshot(), After);
            FJsonObjectConverter::UStructToJsonObjectString(Run->GetPlayerMemory()->GetSnapshot(), MemoryAfter);
            Test->TestEqual(TEXT("Art, walking and lighting do not mutate run"), After, RunBefore);
            Test->TestEqual(TEXT("Memory state untouched by visuals"), MemoryAfter, MemoryBefore);
            Test->TestTrue(TEXT("Narrative trace untouched by visuals"), Host->GetTrace() == TraceBefore);
            const FString Dir = FPaths::ProjectSavedDir()/TEXT("Validation/Character1");
            IFileManager::Get().MakeDirectory(*Dir, true);
            FFileHelper::SaveStringToFile(FString::Printf(TEXT("{\"status\":\"%s\",\"distinct_joint_poses\":%d,\"physical_surfaces\":7,\"wall_x\":%.3f,\"run_unchanged\":%s,\"memory_unchanged\":%s,\"camera_cases\":[%s]}\n"), Test->HasAnyErrors() ? TEXT("FAIL") : TEXT("PASS"), SeenGaitPoses.Num(), WallX, After == RunBefore ? TEXT("true") : TEXT("false"), MemoryAfter == MemoryBefore ? TEXT("true") : TEXT("false"), *CameraRecords), *(Dir/TEXT("exploration.json")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
            return true;
        }
        ++Frame; return false;
    }
private:
    FAutomationTestBase* Test;
    double Started, OldDelta = 0, WallX = 0, LastWorldTime = -1;
    uint64 LastFrame = MAX_uint64;
    int32 Frame = 0;
    bool bFixed = false, bOldFixed = false;
    FString RunBefore, MemoryBefore, CameraRecords;
    TArray<FString> TraceBefore;
    TSet<FString> SeenGaitPoses;
    TWeakObjectPtr<ACameraActor> PreviewCamera;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVerdanVisualTest, "MemoriaVisual.VerdanExploration", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVerdanVisualTest::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice"))) return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FVerdanVisualReplay(this)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
