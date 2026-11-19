#include "Misc/AutomationTest.h"
#include "CoreGlobals.h"
#include "Tests/AutomationEditorCommon.h"
#include "Tests/AutomationCommon.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Slate/SceneViewport.h"
#include "Widgets/SWindow.h"
#include "Widgets/SViewport.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture.h"
#include "EngineUtils.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Presentation/MemoriaVerdanPresentation.h"
#include "Presentation/MemoriaRegionalAtmosphereS363.h"
#include "Presentation/MemoriaFieldCharacterComponent.h"
#include "Framework/MemoriaFieldPawn.h"
#include "Framework/MemoriaSliceHost.h"
#include "Narrative/MemoriaNarrativeSubsystem.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Chapter/MemoriaChapterPresentation.h"
#include "Chapter/MemoriaChapterCardWidget.h"
#include "Chapter/MemoriaChapterMap.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "HAL/FileManager.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "UnrealClient.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
TArray<TSharedPtr<FJsonValue>> VectorJson(const FVector& V)
{return {MakeShared<FJsonValueNumber>(V.X),MakeShared<FJsonValueNumber>(V.Y),MakeShared<FJsonValueNumber>(V.Z)};}
class FRegionalS363Replay final : public IAutomationLatentCommand
{
public:
    FRegionalS363Replay(FAutomationTestBase* InTest,const FString& InMap)
        : Test(InTest),Map(InMap),Started(FPlatformTime::Seconds()),bBaseline(FParse::Param(FCommandLine::Get(),TEXT("MemoriaGraphicsS363Baseline"))) {}
    ~FRegionalS363Replay() override
    {
        if(Owner.IsValid())Owner->SetActorTickEnabled(bOldOwnerTick);
        if(Run.IsValid())Run->SetLocale(OldLocale);
        RestoreScreenshotExtent();
        FScreenshotRequest::OnScreenshotRequestProcessed().Remove(ScreenshotHandle);
        // Same per-client restoration as the accepted S362 responsive fixture.
        // No renderer cvar or unrelated editor viewport is changed.
        if(bWindowCaptured && Client.IsValid() && Client->Viewport && OldSize.X>0 && OldSize.Y>0)
        {
            static_cast<FSceneViewport*>(Client->Viewport)->SetViewportSize(OldSize.X,OldSize.Y);
            if(const auto Window=SavedWindow.Pin();Window.IsValid())
            {
                Window->SetWindowMode(OldMode);
                Window->ReshapeWindow(OldWindowPosition,OldWindowSize);
                Window->SetViewportSizeDrivenByWindow(bOldViewportDrivenByWindow);
                if(!bOldViewportDrivenByWindow)Window->SetIndependentViewportSize(OldWindowViewportSize);
                if(bOldMaximized)Window->Maximize();
                const FVector2D RestoredBuffer=Window->GetViewportSize();
                FSlateApplication::Get().GetRenderer()->UpdateFullscreenState(Window.ToSharedRef(),FMath::RoundToInt(RestoredBuffer.X),FMath::RoundToInt(RestoredBuffer.Y));
            }
        }
        if(bFixed){FApp::SetUseFixedTimeStep(bOldFixed);FApp::SetFixedDeltaTime(OldDelta);}
    }
    bool Update() override
    {
        if(LastFrame==GFrameCounter)return false;LastFrame=GFrameCounter;
        if(FPlatformTime::Seconds()-Started>180.){Test->AddError(FString::Printf(TEXT("S363 regional replay timeout %s phase%d view%d"),*Map,Phase,View));return true;}
        UWorld* World=GEditor?GEditor->PlayWorld.Get():nullptr;
        auto* PC=World && !World->bIsTearingDown?Cast<AMemoriaSliceController>(World->GetFirstPlayerController()):nullptr;
        auto* Pawn=PC?Cast<AMemoriaFieldPawn>(PC->GetPawn()):nullptr;
        if(!Pawn || World->GetTimeSeconds()<.2f || !World->GetGameViewport() || !World->GetGameViewport()->Viewport)return false;
        auto* Host=World->GetGameInstance()->GetSubsystem<UMemoriaNarrativeSubsystem>();
        if(!Host)return false;
        if(!bFixed)
        {
            bFixed=true;bOldFixed=FApp::UseFixedTimeStep();OldDelta=FApp::GetFixedDeltaTime();
            FApp::SetUseFixedTimeStep(true);FApp::SetFixedDeltaTime(1./60.);
            Run=World->GetGameInstance()->GetSubsystem<UMemoriaRunSubsystem>();OldLocale=Run->GetRunSnapshot().CurrentLocale;Run->SetLocale(TEXT("en"));
            ScreenshotHandle=FScreenshotRequest::OnScreenshotRequestProcessed().AddRaw(this,&FRegionalS363Replay::ScreenshotProcessed);
        }
        ++Frame;
        if(Host->GetState()==EMemoriaSliceState::VN || Host->GetState()==EMemoriaSliceState::Field)
        {if(Frame%4==0)Host->Confirm(Map==TEXT("verdan_market")?1:INDEX_NONE);return false;}
        if(Host->GetState()!=EMemoriaSliceState::Exploration)return false;
        if(Phase>0 && (!Owner.IsValid() || !Figure.IsValid()))return false;
        if(Phase==0)
        {
            if(Map==TEXT("verdan_market"))
            {
                if(!World->GetMapName().EndsWith(TEXT("L_VerdanHost")))return false;
                for(TActorIterator<AMemoriaVerdanPresentation> It(World);It;++It)
                {
                    Owner=*It;Figure=It->CharacterFigure();
                    Test->TestEqual(TEXT("S363 preserves the ten retained stalls"),It->GetMarketStallCount(),10);
                    Test->TestEqual(TEXT("S363 adds no market point lights"),It->GetMarketLightCount(),17);
                }
            }
            else
            {
                for(TActorIterator<AMemoriaChapterPresentation> It(World);It;++It)
                {
                    if(It->GetMap()!=Map)continue;
                    if(It->GetCard() && It->GetCard()->IsShowing())return false;
                    Spec=It->GetSpec();if(!Spec)return false;
                    for(const auto& Link:Spec->Sequence)if(!Link.Flag.IsEmpty() && !Run->GetRunSnapshot().GetFlag(Link.Flag))return false;
                    Owner=*It;Figure=It->GetArrelFigure();int32 Solids=0;for(int32 Type:Spec->Tiles)Solids+=Spec->IsSolid(Type)?1:0;
                    Test->TestEqual(TEXT("S363 keeps one blocker per original solid tile"),It->GetBlockerCount(),Solids);
                    Test->TestEqual(TEXT("S363 adds no regional lamps"),It->GetLampCount(),Map==TEXT("belt_waystation")?8:7);
                    Test->TestEqual(TEXT("S363 preserves original environment kit kinds"),It->GetKitKindCount(),Map==TEXT("belt_waystation")?9:7);
                    Test->TestTrue(TEXT("The original painted map ground remains"),It->IsGroundPainted());
                }
            }
            if(!Owner.IsValid() || !Figure.IsValid() || World->GetTimeSeconds()<.8)return false;
            bOldOwnerTick=Owner->IsActorTickEnabled();
            if(!Test->TestTrue(TEXT("Diagnostic feature intent matches requested command line"),MemoriaRegionalAtmosphereS363::IsEnabled()!=bBaseline))return true;
            if(!CheckActualComponents(*Pawn))return true;
            Output=FPaths::ProjectSavedDir()/TEXT("Validation/S363Regional")/FApp::GetInstanceId().ToString(EGuidFormats::Digits);
            IFileManager::Get().MakeDirectory(*Output,true);
            Report=MakeShared<FJsonObject>();Report->SetStringField(TEXT("map"),Map);Report->SetStringField(TEXT("mode"),bBaseline?TEXT("Baseline"):TEXT("After"));
            Report->SetBoolField(TEXT("feature_enabled"),MemoriaRegionalAtmosphereS363::IsEnabled());
            Report->SetNumberField(TEXT("new_solid_instances"),SolidInstances);Report->SetNumberField(TEXT("new_soft_planes"),SoftPlanes);
            Report->SetNumberField(TEXT("new_solid_batches"),SolidBatches);
            Report->SetStringField(TEXT("capture_scope"),TEXT("Actual game render without Slate UI; fixed timestep and same authored player sites; owner tick frozen after24settlingframes. Existing scene time is recorded, not fabricated."));
            Client=World->GetGameViewport();OldSize=Client->Viewport->GetSizeXY();
            const auto Window=Client->GetWindow();
            SavedWindow=Window;LogViewportState(TEXT("BeforeResize"));
            if(!Test->TestTrue(TEXT("Regional fixture has a real PIE clientwindow"),Window.IsValid()))return true;
            OldMode=Window->GetWindowMode();
            OldWindowPosition=Window->GetPositionInScreen();OldWindowSize=Window->GetSizeInScreen();OldWindowViewportSize=Window->GetViewportSize();
            bOldViewportDrivenByWindow=Window->IsViewportSizeDrivenByWindow();bOldMaximized=Window->IsWindowMaximized();bWindowCaptured=true;
            Window->SetWindowMode(EWindowMode::Windowed);if(Window->IsWindowMaximized())Window->Restore();
            Window->Resize(FVector2D(1280,720));MatchSceneTargetToClient(FIntPoint(1280,720));
            ResizeAttempts=0;Phase=10;Mark=Frame;return false;
        }
        if(Phase==10)
        {
            if(Frame<Mark+12)return false;
            const auto ViewportWidget=Client->GetGameViewportWidget();
            LogViewportState(TEXT("AfterResize"));
            if(!Test->TestTrue(TEXT("Regional fixture retains its actual PIE viewportwidget"),ViewportWidget.IsValid()))return true;
            const FVector2D AbsoluteSize=ViewportWidget->GetPaintSpaceGeometry().GetAbsoluteSize();
            const FIntPoint DrawPixels(FMath::RoundToInt(FMath::Abs(AbsoluteSize.X)),FMath::RoundToInt(FMath::Abs(AbsoluteSize.Y)));
            const FIntPoint Desired(1280,720),ScenePixels=Client->Viewport->GetSizeXY(),TargetPixels=Client->Viewport->GetRenderTargetTextureSizeXY();
            const auto Window=SavedWindow.Pin();
            if(!Test->TestTrue(TEXT("Regional resize retains its original PIE window"),Window.IsValid()))return true;
            if((DrawPixels!=Desired || ScenePixels!=Desired || TargetPixels!=Desired) && ResizeAttempts++<3)
            {
                // Correct measured CLIENT pixels, not an assumed outer-window
                // border. Reassert the independent scene target after resizing.
                if(DrawPixels!=Desired)Window->Resize(FVector2D(Window->GetClientSizeInScreen())+FVector2D(Desired-DrawPixels));
                MatchSceneTargetToClient(Desired);Mark=Frame;return false;
            }
            if(!Test->TestEqual(TEXT("Actual Slate regional gameviewport draws1280x720pixels"),DrawPixels,Desired) ||
                !Test->TestEqual(TEXT("Resized actual regional scene is1280x720"),ScenePixels,Desired) ||
                !Test->TestEqual(TEXT("Resized actual regional target is1280x720"),TargetPixels,Desired))return true;
            Phase=1;Mark=Frame;return false;
        }
        if(Phase==1)
        {
            Owner->SetActorTickEnabled(true);
            FVector At;
            if(Map==TEXT("verdan_market"))At=View==0?FVector(0,-180,0):FVector(-390,20,0);
            else
            {
                const FVector2D Tile=View==0?Spec->Spawn/Spec->TileSize-FVector2D(.5,.5):Map==TEXT("belt_waystation")?FVector2D(12,13):FVector2D(10,3);
                At=MemoriaChapterMaps::ToWorld((Tile+FVector2D(.5,.5))*Spec->TileSize);
            }
            At.Z=Pawn->GetActorLocation().Z;Pawn->SetActorLocation(At,false,nullptr,ETeleportType::TeleportPhysics);
            Mark=Frame;Phase=2;return false;
        }
        if(Phase==2 && Frame>=Mark+24)
        {
            Owner->SetActorTickEnabled(false);
            auto* Camera=Pawn->GetFieldCamera();if(!Test->TestNotNull(TEXT("Actual gameplay camera exists"),Camera))return true;
            const FIntPoint Size=World->GetGameViewport()->Viewport->GetSizeXY();
            const FIntPoint Target=World->GetGameViewport()->Viewport->GetRenderTargetTextureSizeXY();
            LogViewportState(TEXT("BeforeCapture"));
            if(!Test->TestEqual(TEXT("Actual game viewport is1280x720"),Size,FIntPoint(1280,720)) || !Test->TestEqual(TEXT("Actual scene target is1280x720"),Target,FIntPoint(1280,720)))return true;
            auto Item=MakeShared<FJsonObject>();Item->SetStringField(TEXT("view"),View==0?TEXT("Wide"):TEXT("Detail"));
            Item->SetArrayField(TEXT("player_world"),VectorJson(Pawn->GetActorLocation()));Item->SetArrayField(TEXT("camera_world"),VectorJson(Camera->GetComponentLocation()));
            Item->SetArrayField(TEXT("camera_rotation"),VectorJson(FVector(Camera->GetComponentRotation().Pitch,Camera->GetComponentRotation().Yaw,Camera->GetComponentRotation().Roll)));
            Item->SetNumberField(TEXT("camera_fov"),Camera->FieldOfView);Item->SetNumberField(TEXT("world_seconds"),World->GetTimeSeconds());
            Item->SetNumberField(TEXT("frame_since_replay"),Frame);Item->SetNumberField(TEXT("fixed_delta_seconds"),FApp::GetFixedDeltaTime());
            const FString Name=Map+TEXT("_")+(bBaseline?TEXT("Baseline"):TEXT("After"))+TEXT("_")+(View==0?TEXT("Wide"):TEXT("Detail"));
            ScreenshotPath=Output/(Name+TEXT(".png"));Item->SetStringField(TEXT("png"),ScreenshotPath);Views.Add(MakeShared<FJsonValueObject>(Item));
            if(!Test->TestFalse(TEXT("A regional render never replaces another pending screenshot"),FScreenshotRequest::IsScreenshotRequested()) ||
                !Test->TestFalse(TEXT("A regional render never changes an active highrescapture"),GIsHighResScreenshot))return true;
            // UI captures publish their size to these globals, but UE5.8's
            // static non-UI RequestScreenshot does not. ProcessScreenshotData
            // later uses them as a crop rect. Mirror the engine's ordinary
            // HandleScreenshotCommand using the VERIFIED real target extent,
            // then restore the caller's exact state after processing.
            PreviousScreenshotX=GScreenshotResolutionX;PreviousScreenshotY=GScreenshotResolutionY;bScreenshotExtentOwned=true;
            GScreenshotResolutionX=uint32(Target.X);GScreenshotResolutionY=uint32(Target.Y);
            Item->SetNumberField(TEXT("previous_global_screenshot_width"),PreviousScreenshotX);Item->SetNumberField(TEXT("previous_global_screenshot_height"),PreviousScreenshotY);
            Item->SetNumberField(TEXT("owned_screenshot_width"),Target.X);Item->SetNumberField(TEXT("owned_screenshot_height"),Target.Y);
            Test->AddInfo(FString::Printf(TEXT("S363_REGION_SCREENSHOT_EXTENT previous=%ux%u measured_target=%dx%d scope=until_processed"),PreviousScreenshotX,PreviousScreenshotY,Target.X,Target.Y));
            bProcessed=false;
            FScreenshotRequest::RequestScreenshot(ScreenshotPath,false,false);
            Test->AddInfo(FString::Printf(TEXT("S363_REGION_CAPTURE mode=%s map=%s view=%s player=%s camera=%s rotation=%s world_time=%.6f fixed_delta=%.9f"),
                bBaseline?TEXT("Baseline"):TEXT("After"),*Map,View==0?TEXT("Wide"):TEXT("Detail"),*Pawn->GetActorLocation().ToString(),*Camera->GetComponentLocation().ToString(),*Camera->GetComponentRotation().ToString(),World->GetTimeSeconds(),FApp::GetFixedDeltaTime()));
            Mark=Frame;Phase=3;return false;
        }
        if(Phase==3)
        {
            if(!bProcessed){if(Frame<Mark+300)return false;Test->AddError(TEXT("Regional native screenshot did not complete"));return true;}
            Test->TestTrue(TEXT("Processed regional capture restored its caller's exact screenshotextent"),!bScreenshotExtentOwned &&
                GScreenshotResolutionX==PreviousScreenshotX && GScreenshotResolutionY==PreviousScreenshotY);
            TArray<uint8> Png;const bool Loaded=FFileHelper::LoadFileToArray(Png,*ScreenshotPath);
            const uint8 Signature[]={137,80,78,71,13,10,26,10};
            if(!Test->TestTrue(TEXT("Actual saved regional render is a PNG"),Loaded && Png.Num()>24 && FMemory::Memcmp(Png.GetData(),Signature,8)==0))return true;
            auto Big=[&](int32 O){return int32(uint32(Png[O])<<24|uint32(Png[O+1])<<16|uint32(Png[O+2])<<8|uint32(Png[O+3]));};
            if(!Test->TestEqual(TEXT("Saved native regional pixels are1280x720"),FIntPoint(Big(16),Big(20)),FIntPoint(1280,720)))return true;
            FSHAHash Hash;FSHA1::HashBuffer(Png.GetData(),Png.Num(),Hash.Hash);
            auto ViewObject=Views.Last()->AsObject();ViewObject->SetStringField(TEXT("sha1"),Hash.ToString());ViewObject->SetNumberField(TEXT("width"),Big(16));ViewObject->SetNumberField(TEXT("height"),Big(20));
            if(++View<2){Phase=1;return false;}
            Report->SetArrayField(TEXT("captures"),Views);Report->SetStringField(TEXT("status"),Test->HasAnyErrors()?TEXT("FAIL"):TEXT("PASS"));
            FString Text;FJsonSerializer::Serialize(Report.ToSharedRef(),TJsonWriterFactory<>::Create(&Text));
            const FString Path=Output/(Map+TEXT("_")+(bBaseline?TEXT("Baseline"):TEXT("After"))+TEXT(".json"));
            Test->TestTrue(TEXT("Recorded actual regional evidence was saved"),FFileHelper::SaveStringToFile(Text,*Path,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM));
            Test->AddInfo(FString::Printf(TEXT("S363_REGION_EVIDENCE path=%s solids=%d soft=%d"),*Path,SolidInstances,SoftPlanes));return true;
        }
        return false;
    }
private:
    void MatchSceneTargetToClient(const FIntPoint& Desired)
    {
        const auto Window=SavedWindow.Pin();
        static_cast<FSceneViewport*>(Client->Viewport)->SetViewportSize(Desired.X,Desired.Y);
        // Independent extent describes the WHOLE Slate window. Keep the scene
        // exact, then retain the measured outer layout for its game child.
        const FVector2D OuterPixels=Window->GetSizeInScreen();
        Window->SetIndependentViewportSize(OuterPixels);
        FSlateApplication::Get().GetRenderer()->UpdateFullscreenState(Window.ToSharedRef(),FMath::RoundToInt(OuterPixels.X),FMath::RoundToInt(OuterPixels.Y));
    }
    void LogViewportState(const TCHAR* Stage)
    {
        const FIntPoint Scene=Client->Viewport->GetSizeXY(),Target=Client->Viewport->GetRenderTargetTextureSizeXY();
        const auto Window=SavedWindow.Pin();const auto Widget=Client->GetGameViewportWidget();
        const FVector2D Paint=Widget.IsValid()?Widget->GetPaintSpaceGeometry().GetAbsoluteSize():FVector2D(-1,-1);
        const FVector2D Local=Widget.IsValid()?FVector2D(Widget->GetPaintSpaceGeometry().GetLocalSize()):FVector2D(-1,-1);
        const FVector2D WindowClient=Window.IsValid()?FVector2D(Window->GetClientSizeInScreen()):FVector2D(-1,-1);
        const FVector2D Outer=Window.IsValid()?FVector2D(Window->GetSizeInScreen()):FVector2D(-1,-1);
        const FVector2D Layout=Window.IsValid()?FVector2D(Window->GetViewportSize()):FVector2D(-1,-1);
        Test->AddInfo(FString::Printf(TEXT("S363_REGION_RESIZE stage=%s map=%s mode=%s attempt=%d client=%.3fx%.3f outer=%.3fx%.3f layout=%.3fx%.3f local=%.3fx%.3f paint=%.3fx%.3f scene=%dx%d target=%dx%d dpi=%.3f appScale=%.3f"),
            Stage,*Map,bBaseline?TEXT("Baseline"):TEXT("After"),ResizeAttempts,WindowClient.X,WindowClient.Y,Outer.X,Outer.Y,Layout.X,Layout.Y,Local.X,Local.Y,
            FMath::Abs(Paint.X),FMath::Abs(Paint.Y),Scene.X,Scene.Y,Target.X,Target.Y,Window.IsValid()?Window->GetDPIScaleFactor():0.f,FSlateApplication::Get().GetApplicationScale()));
        if(Report.IsValid())
        {
            auto Actual=MakeShared<FJsonObject>();Actual->SetNumberField(TEXT("attempt"),ResizeAttempts);
            Actual->SetNumberField(TEXT("paint_width"),FMath::Abs(Paint.X));Actual->SetNumberField(TEXT("paint_height"),FMath::Abs(Paint.Y));
            Actual->SetNumberField(TEXT("scene_width"),Scene.X);Actual->SetNumberField(TEXT("scene_height"),Scene.Y);
            Actual->SetNumberField(TEXT("target_width"),Target.X);Actual->SetNumberField(TEXT("target_height"),Target.Y);Report->SetObjectField(Stage,Actual);
        }
    }
    bool CheckActualComponents(AMemoriaFieldPawn& Pawn)
    {
        TArray<UStaticMeshComponent*> Components;Owner->GetComponents(Components);
        TSet<UMaterialInstanceDynamic*> Materials;int32 Atlas=0,CounterFootings=0,CurbCaps=0;bool bOriginalStone=false,bOriginalGround=false;
        for(auto* Mesh:Components)
        {
            if(auto* Original=Cast<UMaterialInstanceDynamic>(Mesh->GetMaterial(0)))
            {
                const FName StoneName=Map==TEXT("verdan_market")?TEXT("Stone"):TEXT("ChapterRuinStone");
                const FName GroundName=Map==TEXT("verdan_market")?TEXT("CourtyardPaving"):TEXT("ChapterGround");
                if(Original->GetFName()==StoneName)
                {
                    UTexture* Paint=nullptr;
                    bOriginalStone=Mesh->GetStaticMesh() && Original->Parent &&
                        Original->Parent->GetPathName()==TEXT("/Game/Memoria/Presentation/EnvironmentS356/M_ArchitectureSurface.M_ArchitectureSurface") &&
                        Original->GetTextureParameterValue(FMaterialParameterInfo(TEXT("ArchitectureAtlas")),Paint) && Paint &&
                        Paint->GetPathName()==TEXT("/Game/Memoria/Presentation/EnvironmentS356/T_ArchitectureAtlas.T_ArchitectureAtlas");
                }
                if(Original->GetFName()==GroundName)
                    bOriginalGround=Mesh->GetStaticMesh() && Original->Parent &&
                        Original->Parent->GetPathName()==(Map==TEXT("verdan_market")?TEXT("/Game/Memoria/Presentation/Depth/M_Paving.M_Paving"):TEXT("/Game/Memoria/Presentation/Chapter/M_ChapterGround.M_ChapterGround"));
            }
            if(!Mesh->ComponentHasTag(TEXT("MemoriaRegionalS363")))continue;
            Test->TestNotNull(TEXT("New regional detail actually binds a mesh"),Mesh->GetStaticMesh().Get());
            Test->TestEqual(TEXT("New regional detail has no collision"),Mesh->GetCollisionEnabled(),ECollisionEnabled::NoCollision);
            Test->TestFalse(TEXT("New regional detail generates no overlaps"),Mesh->GetGenerateOverlapEvents());
            Test->TestFalse(TEXT("New regional detail changes no navigation"),Mesh->CanEverAffectNavigation());
            auto* M=Cast<UMaterialInstanceDynamic>(Mesh->GetMaterial(0));
            if(!Test->TestNotNull(TEXT("New detail uses an actual dynamic material"),M))return false;
            Test->TestTrue(TEXT("Only newS363named siblings are assigned"),M->GetName().StartsWith(TEXT("S363")));
            if(!Test->TestNotNull(TEXT("New material has a valid parent"),M->Parent.Get()))return false;
            Test->TestFalse(TEXT("A MID is never another MID's parent"),M->Parent && M->Parent->IsA<UMaterialInstanceDynamic>());
            Materials.Add(M);
            if(auto* Batch=Cast<UInstancedStaticMeshComponent>(Mesh))
            {
                Test->TestTrue(TEXT("New instanced mesh has real pieces"),Batch->GetInstanceCount()>0);SolidInstances+=Batch->GetInstanceCount();++SolidBatches;
                if(Mesh->GetName().Contains(TEXT("Chips")))
                    for(int32 I=0;I<Batch->GetInstanceCount();++I)
                    {
                        FTransform T;Batch->GetInstanceTransform(I,T,true);
                        const FBox Bounds=Batch->GetStaticMesh()->GetBoundingBox().TransformBy(T);
                        Test->TestTrue(TEXT("Every loose chip remains under2cm above the unchangedfootplane"),Bounds.Max.Z<=-6.f);
                    }
                if(Spec && Mesh->GetName().Contains(TEXT("Footings")))
                    for(int32 I=0;I<Batch->GetInstanceCount();++I)
                    {
                        FTransform T;Batch->GetInstanceTransform(I,T,true);
                        const FBox Local=Batch->GetStaticMesh()->GetBoundingBox();
                        for(double X:{Local.Min.X,Local.Max.X})for(double Y:{Local.Min.Y,Local.Max.Y})
                        {
                            const auto Source=MemoriaChapterMaps::ToSource(T.TransformPosition(FVector(X,Y,0)));
                            const int32 TX=FMath::FloorToInt(Source.X/Spec->TileSize),TY=FMath::FloorToInt(Source.Y/Spec->TileSize);
                            Test->TestTrue(TEXT("Every new footingcorner remains within source-solidtiles"),Spec->IsSolid(Spec->TileAt(TX,TY)));
                        }
                    }
                if(Map==TEXT("verdan_market") && Mesh->GetName().Contains(TEXT("StoneTrim")))
                    for(int32 I=0;I<Batch->GetInstanceCount();++I)
                    {
                        FTransform T;Batch->GetInstanceTransform(I,T,true);
                        const FVector Centre=T.GetLocation();
                        constexpr double E=.001;
                        const bool bCounter=FMath::IsNearlyEqual(Centre.Y,157.,E) && FMath::IsNearlyEqual(Centre.Z,-3.,E);
                        const bool bCap=FMath::IsNearlyEqual(Centre.Y,600.,E) && FMath::IsNearlyEqual(Centre.Z,31.5,E);
                        // One shared mesh/material/shadow batch, but geometry is
                        // classified by its real native transform, not its name.
                        if(!Test->TestTrue(TEXT("Every sharedtrim instance is exactly one boundedcounterfootingorcurbcap"),bCounter!=bCap))continue;
                        if(bCounter)++CounterFootings;else ++CurbCaps;
                        const double CounterX=Centre.X<0 ? -400. : 400.;
                        const FBox Local=Batch->GetStaticMesh()->GetBoundingBox();
                        for(double X:{Local.Min.X,Local.Max.X})for(double Y:{Local.Min.Y,Local.Max.Y})for(double Z:{Local.Min.Z,Local.Max.Z})
                        {
                            const FVector Corner=T.TransformPosition(FVector(X,Y,Z));
                            // One-thousandth of a centimetre only allows native
                            // float instance-matrix quantization at these bounds.
                            if(bCounter)
                                Test->TestTrue(TEXT("All8rotatedcounterfootingcornersremaininsideoriginal100x100obstacleandzminus8to2cm"),
                                    Corner.X>=CounterX-50. && Corner.X<=CounterX+50. && Corner.Y>=150. && Corner.Y<=250. && Corner.Z>=-8.-E && Corner.Z<=2.+E);
                            else
                                Test->TestTrue(TEXT("All8copingcornersremainonretainednorthcurbfootprintandtop30to33cm"),
                                    Corner.X>=-900.-E && Corner.X<=900.+E && Corner.Y>=590.-E && Corner.Y<=610.+E && Corner.Z>=30.-E && Corner.Z<=33.+E);
                        }
                    }
            }
            else{++SoftPlanes;Test->TestFalse(TEXT("Soft spill never casts an obstacle shadow"),Mesh->CastShadow);}
        }
        for(auto* M:Materials)
        {
            if(!M->Parent->GetPathName().Contains(TEXT("M_ArchitectureSurface")))continue;
            UTexture* Painted=nullptr;
            Test->TestTrue(TEXT("New stone retains the original painted architectureatlas"),M->GetTextureParameterValue(FMaterialParameterInfo(TEXT("ArchitectureAtlas")),Painted) && Painted &&
                Painted->GetPathName()==TEXT("/Game/Memoria/Presentation/EnvironmentS356/T_ArchitectureAtlas.T_ArchitectureAtlas"));
            Test->TestEqual(TEXT("New stone keeps the masked sourcecamera-revealgraph"),M->GetBlendMode(),BLEND_Masked);++Atlas;
            const auto* Camera=Pawn.GetFieldCamera();
            if(!Test->TestNotNull(TEXT("Camera reveal has its actual gameplaycamera"),Camera))return false;
            for(const auto& Pair:TArray<TPair<FName,FVector>>{{TEXT("OcclusionEye"),Camera->GetComponentLocation()},{TEXT("OcclusionFocus"),Figure->FocusPosition()},{TEXT("OcclusionUp"),Camera->GetUpVector()}})
            {
                FLinearColor Actual;
                Test->TestTrue(TEXT("New siblings are registered in live camera-reveal updates"),M->GetVectorParameterValue(FMaterialParameterInfo(Pair.Key),Actual) &&
                    FVector(Actual.R,Actual.G,Actual.B).Equals(Pair.Value,.1));
            }
        }
        Test->TestTrue(TEXT("Baseline or after scene retains the real original paintedstonebinding"),bOriginalStone);
        Test->TestTrue(TEXT("Baseline or after scene retains the real original groundshaderbinding"),bOriginalGround);
        if(bBaseline)
        {
            Test->TestEqual(TEXT("Intentional baseline has no newS363components"),SolidInstances+SoftPlanes,0);
            Test->TestEqual(TEXT("Intentional baseline has no newS363materials"),Materials.Num(),0);
            Test->TestEqual(TEXT("Intentional baseline has no newS363solidbatches"),SolidBatches,0);
        }
        else
        {
            Test->TestTrue(TEXT("Default live scene actually uses bounded new instanced detail"),SolidInstances>=30 && SolidInstances<=280);
            Test->TestEqual(TEXT("Softspill uses a boundednumber of ordinaryplanes"),SoftPlanes,Map==TEXT("verdan_market")?4:8);
            Test->TestEqual(TEXT("Two actual paintedstone siblingfamilies are used"),Atlas,2);
            Test->TestEqual(TEXT("Expected newmaterial families arebound"),Materials.Num(),Map==TEXT("verdan_market")?5:4);
            Test->TestEqual(TEXT("New detail respects its exactsolidbatchbudget"),SolidBatches,Map==TEXT("verdan_market")?3:2);
            if(Map==TEXT("verdan_market"))
            {
                Test->TestEqual(TEXT("Both original counters have four containedfootingpieces"),CounterFootings,8);
                Test->TestEqual(TEXT("The retainednorthcurb has sixteen boundedtopcopings"),CurbCaps,16);
            }
        }
        return !Test->HasAnyErrors();
    }
    void RestoreScreenshotExtent()
    {
        if(!bScreenshotExtentOwned)return;
        GScreenshotResolutionX=PreviousScreenshotX;GScreenshotResolutionY=PreviousScreenshotY;bScreenshotExtentOwned=false;
    }
    void ScreenshotProcessed(){RestoreScreenshotExtent();bProcessed=true;}
    FAutomationTestBase* Test;FString Map,Output,ScreenshotPath,OldLocale;
    double Started,OldDelta=0.;uint64 LastFrame=MAX_uint64;int32 Frame=0,Mark=0,Phase=0,View=0,SolidInstances=0,SolidBatches=0,SoftPlanes=0,ResizeAttempts=0;
    bool bBaseline=false,bFixed=false,bOldFixed=false,bOldOwnerTick=true,bProcessed=false,bWindowCaptured=false,bOldMaximized=false,bOldViewportDrivenByWindow=true,bScreenshotExtentOwned=false;
    uint32 PreviousScreenshotX=0,PreviousScreenshotY=0;
    TWeakObjectPtr<AActor> Owner;TWeakObjectPtr<UMemoriaFieldCharacterComponent> Figure;TWeakObjectPtr<UMemoriaRunSubsystem> Run;
    TWeakObjectPtr<UGameViewportClient> Client;TWeakPtr<SWindow> SavedWindow;
    FIntPoint OldSize=FIntPoint::ZeroValue;EWindowMode::Type OldMode=EWindowMode::Windowed;
    FVector2D OldWindowPosition,OldWindowSize,OldWindowViewportSize;
    const FMemoriaChapterMapSpec* Spec=nullptr;FDelegateHandle ScreenshotHandle;
    TSharedPtr<FJsonObject> Report;TArray<TSharedPtr<FJsonValue>> Views;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FS363RegionVerdan,"MemoriaVisual.S363Region.Verdan",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FS363RegionVerdan::RunTest(const FString&)
{
    if(!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice")))return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FRegionalS363Replay(this,TEXT("verdan_market"))));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FS363RegionBelt,"MemoriaVisual.S363Region.Belt",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FS363RegionBelt::RunTest(const FString&)
{
    if(!AutomationOpenMap(TEXT("/Game/Memoria/Maps/L_BeltWaystation")))return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FRegionalS363Replay(this,TEXT("belt_waystation"))));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FS363RegionDrift,"MemoriaVisual.S363Region.Drift",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FS363RegionDrift::RunTest(const FString&)
{
    if(!AutomationOpenMap(TEXT("/Game/Memoria/Maps/L_DriftShelter")))return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FRegionalS363Replay(this,TEXT("drift_shelter"))));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());return true;
}
#endif
