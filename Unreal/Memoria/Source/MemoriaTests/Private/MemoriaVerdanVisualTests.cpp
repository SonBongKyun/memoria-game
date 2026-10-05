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
#include "Audio/MemoriaAudioSubsystem.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "MemoriaPlayFeelChecks.h"
#include "Interaction/MemoriaMaletActor.h"
#include "Presentation/MemoriaVerdanPresentation.h"
#include "Presentation/MemoriaArrel3DComponent.h"
#include "Presentation/MemoriaFieldCharacterComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSequence.h"
#include "Presentation/MemoriaFieldAnimInstance.h"
#include "Presentation/MemoriaVerdanArt.h"
#include "Narrative/MemoriaNarrativeSubsystem.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Save/MemoriaRunSaveGame.h"
#include "Combat/MemoriaFieldCombatSubsystem.h"
#include "Combat/MemoriaFieldMonster.h"
#include "PaperSpriteComponent.h"
#include "PaperSprite.h"
#include "Engine/Texture2D.h"
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
#include "Interaction/MemoriaEliaCompanion.h"
#include "EngineUtils.h"
#if WITH_DEV_AUTOMATION_TESTS
namespace
{
// Probe the real poseable component, measuring world-space feet rather than repeating the IK formula.
FString CheckFootContact(FAutomationTestBase* Test, AActor* Owner)
{
    FString Records;
    for (const int32 Hz : {30,60,120}) for (const float Speed : {60.f,120.f,180.f,1200.f})
    {
        auto* Probe=NewObject<UMemoriaArrel3DComponent>(Owner);
        Probe->RegisterComponent();
        if(!Test->TestTrue(TEXT("Isolated locomotion mesh loads"),Probe->InitializePrototype())){Probe->DestroyComponent();return TEXT("[]");}
        Probe->SetVisibility(false);Probe->SetWorldRotation(FRotator::ZeroRotator);
        const float Dt=1.f/Hz;
        const double GroundAnkle=Probe->GetComponentLocation().Z+8*Probe->GetComponentScale().Z;
        FVector Previous[2];bool WasGround[2]={false,false};int32 Contacts=0;double MaxSlip=0,MinSole=1e9,MaxLift=0;
        for(int32 Frame=0;Frame<Hz*3;++Frame)
        {
            const FVector Step(Speed*Dt,0,0);Probe->AddWorldOffset(Step);Probe->AdvanceLocomotion(Step,Dt);
            for(int32 Side=0;Side<2;++Side)
            {
                const FName Foot=Side==0?TEXT("foot_l"):TEXT("foot_r");
                const FTransform Transform=Probe->GetBoneTransform(Probe->GetBoneIndex(Foot));
                const FVector Position=Transform.GetLocation();
                // Distinguish a flat planted sole from the near-ground start/end of swing.
                const bool Ground=FMath::Abs(Position.Z-GroundAnkle)<.00005 && Transform.GetRotation().AngularDistance(FQuat::Identity)<.00001;
                if(Frame>Hz && Ground && WasGround[Side]){++Contacts;MaxSlip=FMath::Max(MaxSlip,FVector::Dist2D(Position,Previous[Side]));}
                if(Frame>Hz)
                {
                    MaxLift=FMath::Max(MaxLift,Position.Z-GroundAnkle);
                    for(const double ToeX:{-7.,13.})
                        MinSole=FMath::Min(MinSole,Transform.TransformPosition(FVector(ToeX,0,-8)).Z-Probe->GetComponentLocation().Z);
                }
                Previous[Side]=Position;WasGround[Side]=Ground;
                Test->TestFalse(TEXT("Pose stays finite at walk and existing fast pawn speeds"),Position.ContainsNaN());
            }
        }
        Test->TestTrue(TEXT("Both feet lift through swing"),MaxLift>5);
        Test->TestTrue(TEXT("Flat sole clears the ground during walk"),MinSole>-.08);
        if(Speed<=180)
        {
            Test->TestTrue(TEXT("Steady walking has measured planted contacts"),Contacts>10);
            Test->TestTrue(TEXT("Planted world-space feet slip less than 0.04 units per sample"),MaxSlip<.04);
        }
        for(int32 Frame=0;Frame<Hz;++Frame)Probe->AdvanceLocomotion(FVector::ZeroVector,Dt);
        Test->TestTrue(TEXT("Stopped gait settles at all tested frame rates"),Probe->LocomotionWeight()<.001);
        if(!Records.IsEmpty())Records+=TEXT(",");
        Records+=FString::Printf(TEXT("{\"hz\":%d,\"speed\":%.0f,\"plant_samples\":%d,\"max_slip\":%.6f,\"min_sole\":%.6f,\"max_lift\":%.6f,\"cadence_limited\":%s}"),Hz,Speed,Contacts,MaxSlip,MinSole,MaxLift,Speed>180?TEXT("true"):TEXT("false"));
        Probe->DestroyComponent();
    }
    return TEXT("[")+Records+TEXT("]");
}
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
        auto* Character=Presentation->CharacterFigure();
        if(!Test->TestNotNull(TEXT("Arrel field figure in field"),Character))return true;
        auto Key = [&](FKey K, EInputEvent Event) { PC->InputKey(FInputKeyEventArgs::CreateSimulated(K, Event, Event == IE_Released ? 0.0f : 1.0f)); };
        auto Capture = [&](const TCHAR* Name)
        { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Validation/PlayFeel1")/(FString(Name)+TEXT(".png")), true, false); };
        // Observe every native tick rather than only the final destination.
        for (int32 I = 0; I < Presentation->GetTownsfolkCount(); ++I)
        {
            const FVector At = Presentation->GetTownsfolk(I)->GetComponentLocation();
            bTownGroundClear &= Presentation->CanTownsfolkStand(At);
            if (const auto* Shadow = Presentation->GetTownsShadow(I))
                bTownShadowsFollow &= FVector::Dist2D(At, Shadow->GetComponentLocation()) < .01;
            else bTownShadowsFollow = false;
            for (int32 J = I + 1; J < Presentation->GetTownsfolkCount(); ++J)
                MinTownSeparation = FMath::Min(MinTownSeparation, FVector::Dist2D(At, Presentation->GetTownsfolk(J)->GetComponentLocation()));
        }
        if (Frame == 0)
        {
            const FVector CornerFrom(280,245,0), CornerTo(331,277,0);
            Test->TestTrue(TEXT("Corner regression endpoints are allowed"), Presentation->CanTownsfolkStand(CornerFrom) && Presentation->CanTownsfolkStand(CornerTo));
            Test->TestFalse(TEXT("A path through a stall corner is rejected"), Presentation->CanTownsfolkTravel(CornerFrom, CornerTo));
            Test->TestTrue(TEXT("A clear courtyard path remains allowed"), Presentation->CanTownsfolkTravel(FVector(100,100,0), FVector(130,100,0)));
            Test->TestFalse(TEXT("A path past a waiting elder keeps 70 cm clearance"), AMemoriaVerdanPresentation::TownPathsStayApart(FVector(-511,205,0), FVector(-509,15,0), FVector(-549,92,0), FVector(-549,92,0)));
            Test->TestFalse(TEXT("Crossing reserved paths are rejected"), AMemoriaVerdanPresentation::TownPathsStayApart(FVector(0,0,0), FVector(100,0,0), FVector(50,-100,0), FVector(50,100,0)));
            Test->TestTrue(TEXT("Separate parallel paths remain allowed"), AMemoriaVerdanPresentation::TownPathsStayApart(FVector(0,0,0), FVector(100,0,0), FVector(0,80,0), FVector(100,80,0)));
            FJsonObjectConverter::UStructToJsonObjectString(Run->GetRunSnapshot(), RunBefore);
            FJsonObjectConverter::UStructToJsonObjectString(Run->GetPlayerMemory()->GetSnapshot(), MemoryBefore);
            TraceBefore = Host->GetTrace();
            TArray<UPrimitiveComponent*> Components; Presentation->GetComponents(Components);
            for (auto* Component : Components) Test->TestTrue(TEXT("Presentation cannot block movement or visibility traces"), Component->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
            int32 Bodies = 0;
            for (TActorIterator<AStaticMeshActor> It(World); It; ++It)
            {
                Test->TestTrue(TEXT("Existing geometry remains collidable"), It->GetStaticMeshComponent()->GetCollisionEnabled() != ECollisionEnabled::NoCollision);
                Test->TestTrue(TEXT("Retained placeholder visuals are hidden by the 3D stage"), It->GetStaticMeshComponent()->bHiddenInGame);
                ++Bodies;
            }
            Test->TestEqual(TEXT("All seven original physical surfaces retained"), Bodies, 7);
            Test->TestTrue(TEXT("Pawn collider remains 16 x 16"), Cast<UBoxComponent>(Pawn->GetRootComponent())->GetUnscaledBoxExtent().Equals(FVector(8, 8, 8)));
            const auto* Camera = Pawn->GetFieldCamera();
            Test->TestTrue(TEXT("Verdan uses perspective"), Camera->ProjectionMode == ECameraProjectionMode::Perspective);
            Test->TestEqual(TEXT("Bounded field of view"), Camera->FieldOfView, 55.0f);
            Test->TestTrue(TEXT("Oblique camera elevation"), FMath::IsNearlyEqual(Camera->GetComponentRotation().Pitch, -48.0, 0.1));
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
            Test->TestTrue(TEXT("Pawn's own field sprite stays hidden behind the figure"), Pawn->GetFieldSprite()->bHiddenInGame);
            // S306: Arrel, Elia and Malet share one field figure; the 3D prototype is retained only as a component.
            FootRecords=CheckFootContact(Test,Pawn);
            MovementRecords=CheckVerdanMovement(Test,Pawn);
            if (Character->IsRigged())
            {
                auto* Skin = Character->GetSkeletalMesh();
                Test->TestEqual(TEXT("Arrel has the delivered skeleton"), Skin->GetNumBones(), 77);
                Test->TestTrue(TEXT("Skinned figure has readable height"), Skin->Bounds.BoxExtent.Z > 45);
                Test->TestTrue(TEXT("Rigged feet use the visual ground anchor"), FMath::IsNearlyEqual(Skin->GetRelativeLocation().Z, -8.0, .5));
                Test->TestNotNull(TEXT("Native idle/walk blend instance"), Cast<UMemoriaFieldAnimInstance>(Skin->GetAnimInstance()));
                Test->TestEqual(TEXT("Arrel sword is attached"), Character->GetPropCount(), 1);
                Test->TestTrue(TEXT("Malet is rigged with two props"), Presentation->MaletFigure() && Presentation->MaletFigure()->IsRigged() && Presentation->MaletFigure()->GetPropCount() == 2);
            }
            else
            {
            auto* Card=Character->GetCard();
            if(!Test->TestTrue(TEXT("Figure draws an actual sprite card"),Card && Card->GetSprite()))return true;
            Test->AddInfo(FString::Printf(TEXT("ARREL_FIGURE %s hd=%d bounds=%s"),*Character->GetFrameName(),Character->IsHighResolution()?1:0,*Card->Bounds.BoxExtent.ToString()));
            Test->TestTrue(TEXT("Figure has a readable standing height"),Card->Bounds.BoxExtent.Z>45 && FMath::IsNearlyEqual(Character->GetWorldHeight(),AMemoriaVerdanPresentation::ArrelHeight));
            Test->TestTrue(TEXT("Card faces the field camera"),FMath::IsNearlyEqual(Card->GetRelativeRotation().Roll,42.0,.01));
            Test->TestTrue(TEXT("Feet stand on the existing visual ground anchor"),FMath::IsNearlyEqual(Card->GetRelativeLocation().Z,-8.0,.5));
            if(!Character->IsHighResolution()) Test->TestTrue(TEXT("Pixel art is point sampled"),Card->GetSprite()->GetBakedTexture() && Card->GetSprite()->GetBakedTexture()->Filter==TF_Nearest);
            Test->TestNotNull(TEXT("Malet drawn by the same figure"),Presentation->MaletFigure() ? Presentation->MaletFigure()->GetCard() : nullptr);
            }
            TArray<UInstancedStaticMeshComponent*> Batches; Presentation->GetComponents(Batches);
            int32 Instances = 0, Roofs = 0;
            for (auto* Batch : Batches)
            {
                Instances += Batch->GetInstanceCount();
                Test->TestTrue(TEXT("Every geometry batch has a mesh"), Batch->GetStaticMesh() != nullptr);
                Test->TestNotNull(TEXT("Every geometry batch has a material"), Batch->GetMaterial(0));
                if (Batch->GetStaticMesh() && Batch->GetStaticMesh()->GetName() == TEXT("SM_PitchedRoof")) Roofs += Batch->GetInstanceCount();
            }
            Test->AddInfo(FString::Printf(TEXT("VERDAN_BATCHES %d instances %d"), Batches.Num(), Instances));
            Test->TestTrue(TEXT("Architecture is instanced real geometry"), Instances > 200 && Batches.Num() < 40);
            // S346: six buildings, the two story stalls and the market ring's ten.
            Test->TestEqual(TEXT("Six buildings and twelve stall roofs"), Roofs, 18);
            Test->TestEqual(TEXT("The market ring stands round the square"), Presentation->GetMarketStallCount(), 10);
            Test->TestEqual(TEXT("With the kit's lantern posts and freight"), Presentation->GetMarketPropCount(), 6);
            Test->TestEqual(TEXT("Each stall, post and rope lantern burns warm"), Presentation->GetMarketLightCount(), 17);
            // S348: the source's five market townsfolk, each on its own model, the child a child's height.
            Test->TestEqual(TEXT("Five townsfolk stand in the market"), Presentation->GetTownsfolkCount(), 5);
            Test->TestEqual(TEXT("Each wears Codex's rigged model"), Presentation->GetRiggedTownsfolkCount(), 5);
            if (auto* Child = Presentation->GetTownsfolk(4))
                Test->TestTrue(TEXT("The child stands a child's height beside Arrel"), FMath::IsNearlyEqual(Child->GetWorldHeight(), AMemoriaVerdanPresentation::ArrelHeight * 120.f / 180.f, .5f));
            Test->TestTrue(TEXT("Architecture has substantial height"), Presentation->GetComponentsBoundingBox(true).Max.Z > 580);
            auto* Lit = LoadObject<UMaterial>(nullptr,TEXT("/Game/Memoria/Presentation/Depth2/M_FocusSurface.M_FocusSurface"));
            Test->TestTrue(TEXT("Walls respond to real lighting"), Lit && Lit->GetUsageByFlag(MATUSAGE_InstancedStaticMeshes) && Lit->GetShadingModels().HasShadingModel(MSM_DefaultLit));
            Test->TestTrue(TEXT("Occlusion material supports masked reveal"), Lit && Lit->BlendMode == BLEND_Masked);
            int32 ArtLanterns=0;
            TArray<UPaperSpriteComponent*> Pictures; Presentation->GetComponents(Pictures);
            for (auto* Picture:Pictures) if (Picture->GetSprite() && Picture->GetSprite()->GetName()==TEXT("SPR_MemoryLantern")) ++ArtLanterns;
            Test->TestEqual(TEXT("Four original-art lanterns in actual scene"), ArtLanterns, 4);
            TArray<UPointLightComponent*> Lights; Presentation->GetComponents(Lights);
            // Arrel and Malet carry the same character-only fill (Elia shares Arrel's as she follows).
            for (const TCHAR* Name : {TEXT("ArrelFillLight"), TEXT("MaletFillLight")})
            {
                auto* Fill=Lights.FindByPredicate([Name](const UPointLightComponent* L){return L->GetFName()==Name;});
                Test->TestTrue(TEXT("Character fill is isolated from world lighting"),Fill && !(*Fill)->LightingChannels.bChannel0 && (*Fill)->LightingChannels.bChannel1);
            }
            Lights.RemoveAll([](const UPointLightComponent* L){return L->GetFName().ToString().EndsWith(TEXT("FillLight")) || L->GetFName().ToString().StartsWith(TEXT("MarketLight")) || L->GetFName().ToString().StartsWith(TEXT("Campfire"));});
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
                Test->TestTrue(TEXT("Real movement activates the walk"), Presentation->IsWalking());
                SeenGaitPoses.Add(Character->GetFrameName());
                if (auto* Skin = Character->GetSkeletalMesh())
                {
                    const FVector Foot = Skin->GetBoneTransform(Skin->GetBoneIndex(TEXT("foot_l"))).GetLocation();
                    Test->TestFalse(TEXT("Animated foot stays finite"), Foot.ContainsNaN());
                    const FVector LocalFoot = Skin->GetComponentTransform().InverseTransformPosition(Foot);
                    MinFootX = FMath::Min(MinFootX, LocalFoot.X); MaxFootX = FMath::Max(MaxFootX, LocalFoot.X);
                }
                if (const UPaperSpriteComponent* Card = Character->GetCard()) MaxBounce = FMath::Max(MaxBounce, float(Card->GetRelativeLocation().Z) + 8.f);
                Test->TestTrue(TEXT("Figure follows physical pawn"),FVector2D(Character->GetComponentLocation()).Equals(FVector2D(Pawn->GetActorLocation()),.001));
                if(Frame>Start+14)
                {
                    Test->TestEqual(TEXT("Figure faces real travel"),Character->GetFacing(),FString(Directions[I]));
                    Test->TestTrue(TEXT("Walk cycle blends in"),Character->LocomotionWeight()>.6f);
                }
            }
            if (Frame == Start + 18) Capture(Directions[I]);
            if (Frame == Start + 20) Key(Keys[I], IE_Released);
        }
        if (Frame == 156)
        {
            // Four facings, times the walk frames when the art has them; without walk contacts the stride is a visible bounce.
            const int32 WalkFrames = Character->GetWalkFrameCount();
            Test->TestTrue(TEXT("Walk frames and facings change through movement"),SeenGaitPoses.Num()>=(WalkFrames>0 ? 8 : 4));
            Test->AddInfo(FString::Printf(TEXT("FIELD_GAIT poses=%d walk_frames=%d bounce=%.2f"),SeenGaitPoses.Num(),WalkFrames,MaxBounce));
            if (Character->IsRigged()) Test->TestTrue(TEXT("Real skinned foot moves through walking poses"), MaxFootX - MinFootX > 10);
            if (WalkFrames == 0) Test->TestTrue(TEXT("Walking without contact frames still bounces"), MaxBounce > Character->GetWorldHeight() * .01f);
            if (auto* Audio = World->GetGameInstance()->GetSubsystem<UMemoriaAudioSubsystem>(); Test->TestNotNull(TEXT("Audio subsystem exists"), Audio))
                Test->TestTrue(TEXT("Planted feet play stone footsteps"), Audio->GetCueCount(TEXT("step_stone")) > 0);
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
            Test->TestTrue(TEXT("Blocked movement settles the walk"),Character->LocomotionWeight()<.02f);
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
                Test->TestTrue(TEXT("Camera stays inside courtyard tracking limits"), FMath::Abs(Eye.X)<550.1 && Eye.Y>-1180.1 && Eye.Y<-539.9);
                int32 Width,Height; PC->GetViewportSize(Width,Height);
                FVector2D Foot,Head;
                Test->TestTrue(TEXT("Boundary feet project to screen"), PC->ProjectWorldLocationToScreen(Character->GetComponentLocation(),Foot));
                Test->TestTrue(TEXT("Boundary head projects to screen"), PC->ProjectWorldLocationToScreen(Character->GetComponentLocation()+FVector(0,0,Character->GetWorldHeight()),Head));
                Test->TestTrue(TEXT("Entire character stays inside view at edges and corners"), Foot.X>30 && Foot.X<Width-30 && Foot.Y>20 && Foot.Y<Height-20 && Head.X>30 && Head.X<Width-30 && Head.Y>20 && Head.Y<Height-20);
                TArray<UInstancedStaticMeshComponent*> Batches; Presentation->GetComponents(Batches);
                bool FocusTracked=false;
                for (auto* Batch:Batches) if (auto* M=Cast<UMaterialInstanceDynamic>(Batch->GetMaterial(0)))
                {
                    FLinearColor Focus,View;
                    if (M->GetVectorParameterValue(FMaterialParameterInfo(TEXT("OcclusionFocus")),Focus) && M->GetVectorParameterValue(FMaterialParameterInfo(TEXT("OcclusionEye")),View))
                        FocusTracked=FVector(Focus.R,Focus.G,Focus.B).Equals(Character->FocusPosition(),.01) && FVector(View.R,View.G,View.B).Equals(Eye,.01);
                }
                Test->TestTrue(TEXT("Reveal follows the figure and bounded camera"), FocusTracked);
                if (I>0) CameraRecords+=TEXT(",");
                CameraRecords+=FString::Printf(TEXT("{\"edge\":\"%s\",\"camera\":[%.3f,%.3f,%.3f],\"foot\":[%.3f,%.3f],\"head\":[%.3f,%.3f],\"viewport\":[%d,%d]}"),Names[I],Eye.X,Eye.Y,Eye.Z,Foot.X,Foot.Y,Head.X,Head.Y,Width,Height);
                Capture(Names[I]);
            }
        }
        if(Frame==378)Pawn->SetActorLocation(FVector::ZeroVector);
        if(Frame==390)
        {
            Character->Face(TEXT("Down"));
            FVector2D Foot,Head; int32 W,H; PC->GetViewportSize(W,H);
            PC->ProjectWorldLocationToScreen(Character->GetComponentLocation(),Foot);
            PC->ProjectWorldLocationToScreen(Character->GetComponentLocation()+FVector(0,0,Character->GetWorldHeight()),Head);
            const double HeightRatio=(Foot-Head).Size()/H;
            Test->TestTrue(TEXT("Playable camera gives the character 10-18 percent of viewport height"),HeightRatio>.10 && HeightRatio<.18);
            Test->AddInfo(FString::Printf(TEXT("PLAYFEEL_CHARACTER_HEIGHT ratio=%.6f"),HeightRatio));
            Capture(TEXT("MarketFront"));
        }
        if(Frame==400)
        {
            // A temporary review camera shows the real field figure at a readable angle.
            PreviewCamera=World->SpawnActor<ACameraActor>();
            PreviewCamera->SetActorLocation(FVector(0,-470,110));
            PreviewCamera->SetActorRotation((Character->FocusPosition()-PreviewCamera->GetActorLocation()).Rotation());
            PreviewCamera->GetCameraComponent()->SetFieldOfView(40);
            PC->SetViewTarget(PreviewCamera.Get());
            // The review camera inspects Arrel alone; the following companion is play presentation.
            int32 Companions=0;for(TActorIterator<AMemoriaEliaCompanion> It(World);It;++It){++Companions;It->SetActorHiddenInGame(true);}
            Test->TestEqual(TEXT("Elia follows in the Verdan field"),Companions,1);
        }
        if(Frame==406)
        {
            // The close review needs the illustrated art at full resolution, not a streamed-down mip.
            const UTexture2D* Texture=Character->GetCard()&&Character->GetCard()->GetSprite()?Character->GetCard()->GetSprite()->GetBakedTexture():nullptr;
            if(Texture)
            {
                Test->AddInfo(FString::Printf(TEXT("FIELD_TEXTURE_RESIDENT %s resident=%d mips=%d stream=%d"),*Texture->GetName(),Texture->GetNumResidentMips(),Texture->GetNumMips(),Texture->IsStreamable()?1:0));
                if(Character->IsHighResolution()) Test->TestEqual(TEXT("Illustrated field art is fully resident"),Texture->GetNumResidentMips(),Texture->GetNumMips());
            }
            Capture(TEXT("CharacterFront"));
        }
        if(Frame==410)Character->Face(TEXT("Right"));
        if(Frame==420)Capture(TEXT("CharacterSide"));
        if(Frame==424)Character->Face(TEXT("Up"));
        if(Frame==434)Capture(TEXT("CharacterBack"));
        if(Frame==440)Key(EKeys::D,IE_Pressed);
        if(Frame>=440 && Frame<484 && PreviewCamera.IsValid())
        {
            PreviewCamera->SetActorLocation(Pawn->GetActorLocation()+FVector(0,-470,110));
            PreviewCamera->SetActorRotation((Character->FocusPosition()-PreviewCamera->GetActorLocation()).Rotation());
        }
        if(Frame==448)Capture(TEXT("WalkA"));
        if(Frame==451)Capture(TEXT("WalkB"));
        if(Frame==454)Capture(TEXT("WalkC"));
        if(Frame==457)Capture(TEXT("WalkD"));
        if(Frame==460)Key(EKeys::D,IE_Released);
        if(Frame==480)Capture(TEXT("WalkSettled"));
        if(Frame==484)
        {
            PC->SetViewTarget(Pawn);if(PreviewCamera.IsValid())PreviewCamera->Destroy();
            Character->Face(TEXT("Down"));
        }
        if (Frame == 500)
        {
            auto* Malet = Presentation->MaletFigure(); Malet->Face(TEXT("Down"));
            PreviewCamera = World->SpawnActor<ACameraActor>();
            PreviewCamera->SetActorLocation(Malet->GetComponentLocation() + FVector(0, -470, 110));
            PreviewCamera->SetActorRotation((Malet->FocusPosition() - PreviewCamera->GetActorLocation()).Rotation());
            PreviewCamera->GetCameraComponent()->SetFieldOfView(40);
            PC->SetViewTarget(PreviewCamera.Get());
        }
        if (Frame == 512) Capture(TEXT("MaletRigged"));
        if (Frame == 524)
        {
            Character->SetVisibility(false, true);
            for (TActorIterator<AMemoriaEliaCompanion> It(World); It; ++It)
            {
                It->SetActorHiddenInGame(false);
                auto* Elia = It->FindComponentByClass<UMemoriaFieldCharacterComponent>();
                Test->TestTrue(TEXT("Elia rig and staff are active"), Elia && Elia->IsRigged() && Elia->GetPropCount() == 1);
                if (Elia)
                {
                    Elia->Face(TEXT("Down"));
                    PreviewCamera->SetActorLocation(Elia->GetComponentLocation() + FVector(0, -450, 100));
                    PreviewCamera->SetActorRotation((Elia->FocusPosition() - PreviewCamera->GetActorLocation()).Rotation());
                }
            }
        }
        if (Frame == 536) Capture(TEXT("EliaRigged"));
        if (Frame == 544)
        {
            Character->SetVisibility(true, true);
            PC->SetViewTarget(Pawn); if (PreviewCamera.IsValid()) PreviewCamera->Destroy();
        }
        if (Frame == 554)
        {
            // S349: by now the townsfolk have strolled, each near its place and on allowed ground.
            Test->AddInfo(FString::Printf(TEXT("VERDAN_TOWNS_TRAVEL %.1f"), Presentation->GetTownsTravel()));
            Test->TestTrue(TEXT("The townsfolk have strolled"), Presentation->GetTownsTravel() > 60.f);
            for (int32 I = 0; I < Presentation->GetTownsfolkCount(); ++I)
            {
                const FVector At = Presentation->GetTownsfolk(I)->GetComponentLocation();
                Test->TestTrue(TEXT("A townsperson stays near its place"), FVector::Dist2D(At, Presentation->GetTownsHome(I)) <= AMemoriaVerdanPresentation::TownRoam + 5.f);
                Test->TestTrue(TEXT("Off the stalls and the story's places"), Presentation->CanTownsfolkStand(At) || At.Equals(Presentation->GetTownsHome(I), 1.));
            }
            // Arrel comes up beside the man in the square, to his south-west: he stops and turns to face him.
            if (auto* Man = Presentation->GetTownsfolk(2)) Pawn->SetActorLocation(FVector(Man->GetComponentLocation().X - 70, Man->GetComponentLocation().Y - 60, 0));
        }
        if (Frame == 600) TownStop = Presentation->GetTownsfolk(2)->GetComponentLocation();
        if (Frame == 674)
        {
            Test->TestTrue(TEXT("Townsfolk remain on clear ground throughout native movement"), bTownGroundClear);
            Test->TestTrue(TEXT("Native movement keeps townsfolk separated"), MinTownSeparation >= 69.99);
            Test->TestTrue(TEXT("Every townsfolk shadow follows its figure"), bTownShadowsFollow);
            Test->TestTrue(TEXT("Nearby man stays stopped"), Presentation->GetTownsfolk(2)->GetComponentLocation().Equals(TownStop, .01));
            Test->TestTrue(TEXT("Nearby man's gait settles"), Presentation->GetTownsfolk(2)->LocomotionWeight() < .001);
            Test->AddInfo(FString::Printf(TEXT("VERDAN_TOWNS_CLEARANCE min_cm=%.2f"), MinTownSeparation));
            if (auto* Man = Presentation->GetTownsfolk(2))
            {
                const float Want = ((Pawn->GetActorLocation() - Man->GetComponentLocation()) * FVector(1, 1, 0)).Rotation().Yaw;
                Test->TestTrue(TEXT("The man has turned to Arrel"), FMath::Abs(FMath::FindDeltaAngleDegrees(Presentation->GetTownsYaw(2), Want)) < 8.f);
                Test->TestTrue(TEXT("And his rigged figure faces him"), FMath::Abs(FMath::FindDeltaAngleDegrees(Man->GetYaw(), Want)) < 8.f);
            }
            Capture(TEXT("TownsfolkTurn"));
        }
        if (Frame == 680)
        {
            FString After, MemoryAfter;
            FJsonObjectConverter::UStructToJsonObjectString(Run->GetRunSnapshot(), After);
            FJsonObjectConverter::UStructToJsonObjectString(Run->GetPlayerMemory()->GetSnapshot(), MemoryAfter);
            Test->TestEqual(TEXT("Art, walking and lighting do not mutate run"), After, RunBefore);
            Test->TestEqual(TEXT("Memory state untouched by visuals"), MemoryAfter, MemoryBefore);
            Test->TestTrue(TEXT("Narrative trace untouched by visuals"), Host->GetTrace() == TraceBefore);
            const FString Dir = FPaths::ProjectSavedDir()/TEXT("Validation/PlayFeel1");
            IFileManager::Get().MakeDirectory(*Dir, true);
            FFileHelper::SaveStringToFile(MovementRecords,*(Dir/TEXT("movement_response.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
            FFileHelper::SaveStringToFile(FootRecords,*(Dir/TEXT("foot_contact.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
            FFileHelper::SaveStringToFile(FString::Printf(TEXT("{\"status\":\"%s\",\"distinct_joint_poses\":%d,\"physical_surfaces\":7,\"wall_x\":%.3f,\"run_unchanged\":%s,\"memory_unchanged\":%s,\"camera_cases\":[%s]}\n"), Test->HasAnyErrors() ? TEXT("FAIL") : TEXT("PASS"), SeenGaitPoses.Num(), WallX, After == RunBefore ? TEXT("true") : TEXT("false"), MemoryAfter == MemoryBefore ? TEXT("true") : TEXT("false"), *CameraRecords), *(Dir/TEXT("exploration.json")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
        }
        // S350: the source's interactive props, each stepped up to in turn after the untouched-run checks above.
        const auto& Props = AMemoriaVerdanPresentation::Interactives();
        auto Grains = [&] { return Run->GetRunSnapshot().Player.Grains; };
        auto Used = [&](int32 I) { return Run->GetRunSnapshot().GetFlag(Props[I].Flag); };
        if (Frame == 690)
        {
            Test->TestEqual(TEXT("Four interactive props"), Props.Num(), 4);
            Test->TestEqual(TEXT("None was touched by the walks above"), Presentation->GetInteractiveUses(), 0);
            Run->SetLocale(TEXT("ko"));
            GrainsBefore = Grains(); PotionsBefore = Run->GetItemCount(TEXT("potion"));
            Pawn->SetActorLocation(Props[0].At + FVector(0, -20, 0));
        }
        if (Frame == 700)
        {
            const int64 Got = Grains() - GrainsBefore;
            Test->TestTrue(TEXT("The barrel gives 1 to 3 grains"), Used(0) && Got >= 1 && Got <= 3);
            Test->TestTrue(TEXT("Barrel notice uses Korean"), Host->GetExplorationNotice().Contains(TEXT("그레인")));
            GrainsBefore = Grains(); Pawn->SetActorLocation(Props[1].At + FVector(0, -20, 0));
            // Make the real potion branch deterministic, before another world tick can consume random draws.
            int32 Seed = 0;
            for (; Seed < 1000; ++Seed) { FMath::RandInit(Seed); if (FMath::RandRange(0,99) < 40) break; }
            Test->TestTrue(TEXT("Potion reward seed found"), Seed < 1000);
            TArray<FString> Events;
            const auto Changed = Run->OnInventoryChanged.AddLambda([&](const FString& Id) { Events.Add(TEXT("inventory:") + Id); });
            const auto Toast = Run->OnItemToastRequested.AddLambda([&](const FString& Text, int32 Type) { Events.Add(Text + FString::Printf(TEXT(":%d"), Type)); });
            FMath::RandInit(Seed); Presentation->Tick(0);
            Run->OnInventoryChanged.Remove(Changed); Run->OnItemToastRequested.Remove(Toast);
            Test->TestTrue(TEXT("Crate emits inventory then potion toast"), Events == TArray<FString>{TEXT("inventory:potion"), TEXT("+1 Potion:1")});
            Test->TestTrue(TEXT("Crate records the potion as a recent item"), Run->GetRecentItems().Contains(TEXT("potion")));
            const FString Notice = Host->GetExplorationNotice();
            Test->TestTrue(TEXT("Potion grant notice precedes localized found notice"), Notice.Contains(TEXT("+1 Potion\n포션을 발견했다!")));
        }
        if (Frame == 710)
        {
            const int64 Got = Grains() - GrainsBefore, Potions = Run->GetItemCount(TEXT("potion")) - PotionsBefore;
            Test->AddInfo(FString::Printf(TEXT("VERDAN_CRATE grains=%lld potions=%lld"), Got, Potions));
            Test->TestTrue(TEXT("The crate gives a potion, 2 to 5 grains, or nothing"), Used(1) && ((Potions == 1 && Got == 0) || (Potions == 0 && Got >= 2 && Got <= 5) || (Potions == 0 && Got == 0)));
            Test->TestEqual(TEXT("The deterministic crate actually gives one potion"), Potions, int64(1));
            Run->SetLocale(TEXT("en"));
            Pawn->SetActorLocation(Props[2].At + FVector(0, -20, 0));
        }
        if (Frame == 720)
        {
            Test->TestTrue(TEXT("The sign is read"), Used(2));
            Test->TestTrue(TEXT("Sign notice uses English"), Host->GetExplorationNotice().Contains(TEXT("Verdan Market — Trade at your own risk.")));
            auto* Combat = World->GetSubsystem<UMemoriaFieldCombatSubsystem>();
            auto* Foe = World->SpawnActorDeferred<AMemoriaFieldMonster>(AMemoriaFieldMonster::StaticClass(), FTransform(Pawn->GetActorLocation() + FVector(0,-90,0)));
            if (!Test->TestNotNull(TEXT("Temporary foe for a real wound"), Foe)) return true;
            Foe->SetKind(EMemoriaFoeKind::MarketThief); Foe->FinishSpawning(Foe->GetActorTransform()); Foe->Stun(100);
            Test->TestTrue(TEXT("A real strike wounds Arrel before campfire use"), Combat && Combat->StrikePlayer(Foe, 7.f, true));
            Foe->Destroy();
            HpBefore = Run->GetRunSnapshot().Player.Hp;
            Test->TestTrue(TEXT("Campfire starts with missing HP"), HpBefore < Run->GetRunSnapshot().Player.MaxHp);
            Pawn->SetActorLocation(Props[3].At + FVector(-55, -15, 0));
        }
        if (Frame == 730)
        {
            const FMemoriaRunSnapshot After = Run->GetRunSnapshot();
            Test->TestTrue(TEXT("Resting by the campfire"), Used(3) && After.Player.Hp == FMath::Min(After.Player.MaxHp, HpBefore + 5));
            Test->TestTrue(TEXT("Campfire notice uses English"), Host->GetExplorationNotice().Contains(TEXT("Rested by the fire. +5 HP")));
            GrainsBefore = Grains(); PotionsBefore = Run->GetItemCount(TEXT("potion")); HpBefore = After.Player.Hp;
            Pawn->SetActorLocation(FVector::ZeroVector);
        }
        // Each capture a little before Arrel moves on, so it shows him at the prop.
        if (Frame == 698) Capture(TEXT("PropBarrel"));
        if (Frame == 708) Capture(TEXT("PropCrate"));
        if (Frame == 718) Capture(TEXT("PropSign"));
        if (Frame == 728) Capture(TEXT("PropCampfire"));
        if (Frame == 740) Pawn->SetActorLocation(Props[0].At + FVector(0, -20, 0));
        if (Frame == 750) Pawn->SetActorLocation(Props[1].At + FVector(0,-20,0));
        if (Frame == 760) Pawn->SetActorLocation(Props[2].At + FVector(0,-20,0));
        if (Frame == 770) Pawn->SetActorLocation(Props[3].At + FVector(-55,-15,0));
        if (Frame == 780)
        {
            Test->TestEqual(TEXT("All four props act only once"), Presentation->GetInteractiveUses(), 4);
            Test->TestEqual(TEXT("Repeated props give no grains"), Grains(), GrainsBefore);
            Test->TestEqual(TEXT("Repeated crate gives no potion"), Run->GetItemCount(TEXT("potion")), PotionsBefore);
            Test->TestEqual(TEXT("Repeated fire gives no healing"), Run->GetRunSnapshot().Player.Hp, HpBefore);
            auto* Save = Run->CaptureSave();
            if (Test->TestNotNull(TEXT("Used props are captured in a save"), Save))
                for (const auto& Prop : Props) Test->TestTrue(TEXT("Save contains each spent prop flag"), Save->Run.GetFlag(Prop.Flag));
            return true;
        }
        ++Frame; return false;
    }
private:
    FAutomationTestBase* Test;
    double Started, OldDelta = 0, WallX = 0, LastWorldTime = -1;
    int64 GrainsBefore = 0, PotionsBefore = 0, HpBefore = 0;
    uint64 LastFrame = MAX_uint64;
    int32 Frame = 0;
    bool bFixed = false, bOldFixed = false;
    FString RunBefore, MemoryBefore, CameraRecords, FootRecords, MovementRecords;
    TArray<FString> TraceBefore;
    TSet<FString> SeenGaitPoses;
    float MaxBounce = 0.f;
    double MinTownSeparation = 1e9;
    bool bTownGroundClear = true, bTownShadowsFollow = true;
    FVector TownStop = FVector::ZeroVector;
    double MinFootX = 1e9, MaxFootX = -1e9;
    TWeakObjectPtr<ACameraActor> PreviewCamera;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFieldCharactersTest, "MemoriaVisual.FieldCharacters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFieldCharactersTest::RunTest(const FString&)
{
    // FIELD_SPRITE_ART_SPEC.md acceptance: every Verdan field character resolves art; the report says which.
    for (const TCHAR* Id : {TEXT("arrel"), TEXT("elia"), TEXT("malet")})
    {
        const FString Art = UMemoriaFieldCharacterComponent::DescribeArt(Id);
        AddInfo(FString::Printf(TEXT("FIELD_CHARACTER %s %s"), Id, *Art));
        TestTrue(*(FString(TEXT("Field art resolves for ")) + Id), Art != TEXT("missing"));
        if (Art == TEXT("rigged"))
        {
            const FString N = FString(Id).Left(1).ToUpper() + FString(Id).Mid(1);
            const FString Base = TEXT("/Game/Memoria/Presentation/Field3D/") + N + TEXT("/");
            auto* Skin = LoadObject<USkeletalMesh>(nullptr, *(Base + TEXT("SK_") + N + TEXT(".SK_") + N));
            if (!TestNotNull(TEXT("Delivered skeletal asset loads"), Skin)) continue;
            TestEqual(TEXT("77 deform bones, including fingers and cloth"), Skin->GetRefSkeleton().GetNum(), 77);
            TestEqual(TEXT("One body material"), Skin->GetMaterials().Num(), 1);
            TestEqual(TEXT("Feet-centred root"), Skin->GetRefSkeleton().GetBoneName(0), FName(TEXT("root")));
            TestTrue(TEXT("Physical import height matches cm source"), FMath::IsNearlyEqual(float(Skin->GetBounds().BoxExtent.Z * 2), N == TEXT("Elia") ? 166.f : 180.f, 1.f));
            for (const FString Clip : {TEXT("Idle"), TEXT("Walk")})
            {
                auto* Anim = LoadObject<UAnimSequence>(nullptr, *(Base + TEXT("A_") + N + TEXT("_") + Clip + TEXT(".A_") + N + TEXT("_") + Clip));
                if (!TestNotNull(TEXT("Delivered animation loads"), Anim)) continue;
                TestEqual(TEXT("Clip uses the matching skeleton"), Anim->GetSkeleton(), Skin->GetSkeleton());
                TestTrue(TEXT("Clip has its authored duration"), FMath::IsNearlyEqual(Anim->GetPlayLength(), Clip == TEXT("Idle") ? 2.f : 1.f, .04f));
            }
            continue;
        }
        if (Art != TEXT("hd")) continue;
        // The illustrated canvas is drawn at a fraction of its size: filtered, with a mip chain once a real RHI builds it.
        const FString Name = FString(Id).Left(1).ToUpper() + FString(Id).Mid(1);
        const FString TexturePath = FString::Printf(TEXT("/Game/Memoria/Presentation/FieldHD/T_%s_Down.T_%s_Down"), *Name, *Name);
        const UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *TexturePath);
        if (!TestNotNull(*(FString(TEXT("HD texture loads for ")) + Id), Texture)) continue;
        TestTrue(*(FString(TEXT("HD art is filtered for ")) + Id), Texture->Filter != TF_Nearest);
        AddInfo(FString::Printf(TEXT("FIELD_HD_TEXTURE %s mips=%d"), Id, Texture->GetNumMips()));
        if (FApp::CanEverRender()) TestTrue(*(FString(TEXT("HD art has mips for ")) + Id), Texture->GetNumMips() > 4);
    }
    for (const TCHAR* Id : {TEXT("sable"), TEXT("tobias"), TEXT("nera"), TEXT("kairos"), TEXT("veil")})
        AddInfo(FString::Printf(TEXT("FIELD_CHARACTER %s %s"), Id, *UMemoriaFieldCharacterComponent::DescribeArt(Id)));
    return !HasAnyErrors();
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
