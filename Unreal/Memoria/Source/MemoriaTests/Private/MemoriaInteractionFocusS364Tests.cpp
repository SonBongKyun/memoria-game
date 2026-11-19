#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "Tests/AutomationCommon.h"
#include "Editor.h"
#include "CoreGlobals.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "EngineUtils.h"
#include "Framework/MemoriaSliceHost.h"
#include "Framework/MemoriaFieldPawn.h"
#include "Framework/MemoriaCombatInputRouter.h"
#include "Combat/MemoriaFieldHudWidget.h"
#include "Combat/MemoriaFieldCombatSubsystem.h"
#include "Interaction/MemoriaInteractionComponent.h"
#include "Interaction/MemoriaInteractionFocusS364.h"
#include "Interaction/MemoriaEliaCompanion.h"
#include "Interaction/MemoriaMaletActor.h"
#include "Interaction/MemoriaStoryPointActor.h"
#include "Narrative/MemoriaNarrativeSubsystem.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Chapter/MemoriaChapterPresentation.h"
#include "Chapter/MemoriaChapterCardWidget.h"
#include "Chapter/MemoriaChapterMap.h"
#include "Components/BoxComponent.h"
#include "Camera/CameraComponent.h"
#include "InputKeyEventArgs.h"
#include "Misc/App.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "HAL/FileManager.h"
#include "JsonObjectConverter.h"
#include "Serialization/JsonSerializer.h"
#include "Slate/SceneViewport.h"
#include "Widgets/SWindow.h"
#include "Widgets/SViewport.h"
#include "Rendering/SlateRenderer.h"
#include "Framework/Application/SlateApplication.h"
#include "UnrealClient.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
FString RunJson(const UMemoriaRunSubsystem& Run)
{
    FString Json; FJsonObjectConverter::UStructToJsonObjectString(Run.GetRunSnapshot(), Json); return Json;
}
void Key(AMemoriaSliceController& PC, FKey K, EInputEvent Event)
{ PC.InputKey(FInputKeyEventArgs::CreateSimulated(K, Event, Event == IE_Released ? 0.f : 1.f)); }
void Tap(AMemoriaSliceController& PC, FKey K) { Key(PC,K,IE_Pressed); Key(PC,K,IE_Released); }
bool CueHidden(const AMemoriaSliceController& PC, uint64 Since)
{
    const auto* Hud=PC.GetFieldHud();
    return !Hud || !Hud->GetInteractionFocusPaintProbe().bDrawn || Hud->GetInteractionFocusPaintProbe().Frame<=Since;
}
class FFocusS364Replay final : public IAutomationLatentCommand
{
public:
    explicit FFocusS364Replay(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()), OldScreenshotX(GScreenshotResolutionX), OldScreenshotY(GScreenshotResolutionY) {}
    ~FFocusS364Replay() override
    {
        FScreenshotRequest::OnScreenshotRequestProcessed().Remove(ScreenshotHandle);
        if(Controller.IsValid() && Controller->GetWorld() && !Controller->GetWorld()->bIsTearingDown)
        {
            Key(*Controller.Get(),EKeys::D,IE_Released);Key(*Controller.Get(),EKeys::E,IE_Released);Key(*Controller.Get(),EKeys::Gamepad_FaceButton_Bottom,IE_Released);
            if(Controller->GetPauseWidget())Controller->ClosePause();
            if(Controller->GetArchiveWidget())Controller->ToggleArchive();
        }
        if(bCameraChanged && ObservedPawn.IsValid())ObservedPawn->GetFieldCamera()->SetWorldRotation(OldCamera);
        if(Blocker.IsValid())Blocker->Destroy();if(OwnedPoint.IsValid())OwnedPoint->Destroy();
        if(Elia.IsValid())Elia->SetActorTickEnabled(bOldEliaTick);
        if(Client.IsValid() && Client->Viewport && OldSize.X>0)
        {
            static_cast<FSceneViewport*>(Client->Viewport)->SetViewportSize(OldSize.X,OldSize.Y);
            if(auto Window=SavedWindow.Pin())
            {
                Window->SetWindowMode(OldMode);Window->ReshapeWindow(OldPosition,OldWindowSize);
                Window->SetViewportSizeDrivenByWindow(bOldDriven);if(!bOldDriven)Window->SetIndependentViewportSize(OldWindowViewport);
                if(bOldMaximized)Window->Maximize();const auto Extent=Window->GetViewportSize();
                FSlateApplication::Get().GetRenderer()->UpdateFullscreenState(Window.ToSharedRef(),FMath::RoundToInt(Extent.X),FMath::RoundToInt(Extent.Y));
            }
        }
        if(bFixed){FApp::SetUseFixedTimeStep(bOldFixed);FApp::SetFixedDeltaTime(OldDelta);}
        GScreenshotResolutionX=OldScreenshotX;GScreenshotResolutionY=OldScreenshotY;
    }
    bool Update() override
    {
        if(LastFrame==GFrameCounter)return false;LastFrame=GFrameCounter;
        if(FPlatformTime::Seconds()-Started>300.){Test->AddError(FString::Printf(TEXT("S364 focus timeout phase%d case%d"),Phase,CaseIndex));return true;}
        UWorld* World=GEditor?GEditor->PlayWorld.Get():nullptr;
        auto* PC=World && !World->bIsTearingDown?Cast<AMemoriaSliceController>(World->GetFirstPlayerController()):nullptr;
        auto* Pawn=PC?Cast<AMemoriaFieldPawn>(PC->GetPawn()):nullptr;
        if(!Pawn || World->GetTimeSeconds()<.3f)return false;
        auto* Host=World->GetGameInstance()->GetSubsystem<UMemoriaNarrativeSubsystem>();
        if(!bFixed)
        {bFixed=true;bOldFixed=FApp::UseFixedTimeStep();OldDelta=FApp::GetFixedDeltaTime();FApp::SetUseFixedTimeStep(true);FApp::SetFixedDeltaTime(1./60.);return false;}
        ++Frame;
        if(!bReady)
        {
            if(Host->GetState()==EMemoriaSliceState::VN){if(Frame%4==0)Host->Confirm(1);return false;}
            if(!World->GetMapName().EndsWith(TEXT("L_VerdanHost")) || Host->GetState()!=EMemoriaSliceState::Exploration)return false;
            for(TActorIterator<AMemoriaEliaCompanion> It(World);It;++It)Elia=*It;
            for(TActorIterator<AMemoriaMaletActor> It(World);It;++It)Malet=*It;
            for(TActorIterator<AMemoriaStoryPointActor> It(World);It;++It)if(Host->IsStoryBeatAvailable(It->GetGroup())){Story=*It;break;}
            Run=World->GetGameInstance()->GetSubsystem<UMemoriaRunSubsystem>();Client=World->GetGameViewport();Controller=PC;ObservedPawn=Pawn;
            if(!Elia.IsValid() || !Malet.IsValid() || !Story.IsValid() || !Run.IsValid() || !Client.IsValid() || !Client->Viewport || !PC->GetFieldHud())return false;
            Test->TestTrue(TEXT("Original Verdan dialogue entry is eligible"),Host->CanInteractWithElia());
            StoryGroup=Story->GetGroup();bOldEliaTick=Elia->IsActorTickEnabled();Elia->SetActorTickEnabled(false);
            OldSize=Client->Viewport->GetSizeXY();const auto Window=Client->GetWindow();if(!Test->TestTrue(TEXT("Focus fixture owns a real client window"),Window.IsValid()))return true;
            SavedWindow=Window;OldMode=Window->GetWindowMode();OldPosition=Window->GetPositionInScreen();OldWindowSize=Window->GetSizeInScreen();
            OldWindowViewport=Window->GetViewportSize();bOldDriven=Window->IsViewportSizeDrivenByWindow();bOldMaximized=Window->IsWindowMaximized();
            ScreenshotHandle=FScreenshotRequest::OnScreenshotRequestProcessed().AddRaw(this,&FFocusS364Replay::ScreenshotProcessed);
            Output=FPaths::ProjectSavedDir()/TEXT("Validation/S364Focus")/FApp::GetInstanceId().ToString(EGuidFormats::Digits);
            IFileManager::Get().MakeDirectory(*Output,true);bReady=true;
        }
        auto* Combat=World->GetSubsystem<UMemoriaFieldCombatSubsystem>();
        if(CaseIndex<12)
        {
            const int32 Kind=CaseIndex/4;AActor* Target=Kind==0?static_cast<AActor*>(Elia.Get()):Kind==1?static_cast<AActor*>(Story.Get()):static_cast<AActor*>(Malet.Get());
            const bool bPad=(CaseIndex&2)!=0;Desired=(CaseIndex&1)!=0?FIntPoint(640,360):FIntPoint(1280,720);
            if(Phase==0)
            {
                Combat->ResetForRestoredRun();PC->ToggleArchive();
                if(!Test->TestNotNull(TEXT("Safe fixture relocation opens the real archive"),PC->GetArchiveWidget()))return true;
                auto Window=SavedWindow.Pin();Window->SetWindowMode(EWindowMode::Windowed);if(Window->IsWindowMaximized())Window->Restore();Window->Resize(FVector2D(Desired));MatchTarget();ResizeAttempts=0;
                Mark=Frame;Phase=1;return false;
            }
            if(Phase==1 && Frame>Mark+1){Pawn->SetActorLocation(Target->GetActorLocation()+FVector(-40,0,0));Mark=Frame;Phase=2;return false;}
            if(Phase==2 && Frame>Mark+1){PC->ToggleArchive();Key(*PC,EKeys::D,IE_Pressed);Mark=Frame;Phase=3;return false;}
            if(Phase==3 && Frame>Mark+2){Key(*PC,EKeys::D,IE_Released);Tap(*PC,bPad?EKeys::Gamepad_RightThumbstick:EKeys::MiddleMouseButton);Mark=Frame;Phase=4;return false;}
            if(Phase==4)
            {
                if(Frame<Mark+12)return false;const auto Widget=Client->GetGameViewportWidget();if(!Widget.IsValid())return false;
                const FVector2D Paint=Widget->GetPaintSpaceGeometry().GetAbsoluteSize();const FIntPoint Pixels(FMath::RoundToInt(FMath::Abs(Paint.X)),FMath::RoundToInt(FMath::Abs(Paint.Y)));
                if(Pixels!=Desired && ResizeAttempts++<3){SavedWindow.Pin()->Resize(FVector2D(SavedWindow.Pin()->GetClientSizeInScreen())+FVector2D(Desired-Pixels));MatchTarget();Mark=Frame;return false;}
                if(!Test->TestEqual(TEXT("Focus actual Slate paint has requested pixels"),Pixels,Desired) || !Test->TestEqual(TEXT("Focus actual scene target has requested pixels"),Client->Viewport->GetRenderTargetTextureSizeXY(),Desired))return true;
                if(!CheckFocus(*PC,Target,bPad))return true;
                BeforeObservation=RunJson(*Run.Get());Capture(FString::Printf(TEXT("Case%02d_%s"),CaseIndex,Kind==0?TEXT("Elia"):Kind==1?TEXT("Story"):TEXT("Malet")),*PC,*Pawn);
                Phase=5;return false;
            }
            if(Phase==5)
            {
                if(!CaptureDone())return false;
                Test->TestEqual(TEXT("Drawing/observing focus changes no run data, memory, rewards or flags"),RunJson(*Run.Get()),BeforeObservation);
                if(Test->HasAnyErrors())return true;++CaseIndex;Phase=0;return false;
            }
            return false;
        }
        // All checks use real input and original resolver. The blocker and query
        // actor below are owned by this isolated test world, never saved data.
        if(Phase==0){BeforeObservation=RunJson(*Run.Get());HiddenSince=GFrameCounter;PC->OpenPause();Mark=Frame;Phase=10;return false;}
        if(Phase==10 && Frame>Mark+4){Test->TestTrue(TEXT("Pause hides focus before a resolver refresh"),CueHidden(*PC,HiddenSince));PC->ClosePause();Mark=Frame;Phase=11;return false;}
        if(Phase==11 && Frame>Mark+4){HiddenSince=GFrameCounter;Tap(*PC,EKeys::R);Test->TestTrue(TEXT("Real burn input opens picker"),Combat->IsPickingBurn());Mark=Frame;Phase=12;return false;}
        if(Phase==12 && Frame>Mark+4){Test->TestTrue(TEXT("Burn picker hides focus"),CueHidden(*PC,HiddenSince));Tap(*PC,EKeys::Escape);Mark=Frame;Phase=13;return false;}
        if(Phase==13 && Frame>Mark+4)
        {
            const FVector Mid=(Pawn->GetActorLocation()+Malet->GetActorLocation())*.5;AActor* Actor=World->SpawnActor<AActor>();Blocker=Actor;
            auto* Box=NewObject<UBoxComponent>(Actor);Actor->AddInstanceComponent(Box);Actor->SetRootComponent(Box);Box->SetBoxExtent(FVector(5,55,90));
            Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);Box->SetCollisionResponseToAllChannels(ECR_Ignore);Box->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);Box->RegisterComponent();Actor->SetActorLocation(Mid);
            Mark=Frame;Phase=14;return false;
        }
        if(Phase==14 && Frame>Mark+4)
        {
            Test->TestTrue(TEXT("Real visibility blocker removes Malet from the original resolver"),PC->GetInteraction()->GetTarget()!=Malet.Get());
            Test->TestTrue(TEXT("LOS-hidden target is never painted"),!PC->GetFieldHud()->GetInteractionFocusPaintProbe().bDrawn || PC->GetFieldHud()->GetInteractionFocusPaintProbe().Target!=Malet);
            Blocker->Destroy();Blocker.Reset();OldCamera=Pawn->GetFieldCamera()->GetComponentRotation();bCameraChanged=true;Pawn->GetFieldCamera()->SetWorldRotation(FRotator(0,-90,0));Mark=Frame;Phase=15;return false;
        }
        if(Phase==15 && Frame>Mark+4)
        {
            Test->TestFalse(TEXT("Offscreen target has no partially clipped cue"),PC->GetFieldHud()->GetInteractionFocusPaintProbe().bDrawn);
            Pawn->GetFieldCamera()->SetWorldRotation(OldCamera);bCameraChanged=false;FActorSpawnParameters Params;Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            OwnedPoint=World->SpawnActor<AMemoriaStoryPointActor>(AMemoriaStoryPointActor::StaticClass(),Pawn->GetActorLocation()+FVector(18,0,0),FRotator::ZeroRotator,Params);
            OwnedPoint->Configure(StoryGroup,TEXT("Native lifetime probe  |  E / A: listen"));Mark=Frame;Phase=16;return false;
        }
        if(Phase==16 && Frame>Mark+4)
        {
            Test->TestTrue(TEXT("Owned lifetime actor is selected by actual overlap resolver"),PC->GetInteraction()->GetTarget()==OwnedPoint.Get());
            Test->TestTrue(TEXT("Owned valid actor cue follows the resolver"),!MemoriaInteractionFocusS364::IsEnabled() || PC->GetFieldHud()->GetInteractionFocusPaintProbe().Target==OwnedPoint);
            DestroyedPoint=OwnedPoint;OwnedPoint->Destroy();OwnedPoint.Reset();Mark=Frame;Phase=17;return false;
        }
        if(Phase==17 && Frame>Mark+4)
        {
            const auto& Probe=PC->GetFieldHud()->GetInteractionFocusPaintProbe();
            Test->TestTrue(TEXT("Destroyed actor cannot survive as a rendered weak target"),!Probe.bDrawn || Probe.Target!=DestroyedPoint);
            Test->TestEqual(TEXT("Modal, LOS, offscreen and lifetime observations do not mutate run"),RunJson(*Run.Get()),BeforeObservation);
            PC->ToggleArchive();Mark=Frame;Phase=18;return false;
        }
        if(Phase==18 && Frame>Mark+1){Pawn->SetActorLocation(Malet->GetActorLocation()+FVector(500,0,0));Mark=Frame;Phase=19;return false;}
        if(Phase==19 && Frame>Mark+1){PC->ToggleArchive();Mark=Frame;Phase=27;return false;}
        if(Phase==27 && Frame>Mark+4)
        {
            Test->TestTrue(TEXT("Leaving range removes the old target in original resolver"),PC->GetInteraction()->GetTarget()!=Malet.Get());
            const auto& Probe=PC->GetFieldHud()->GetInteractionFocusPaintProbe();
            Test->TestTrue(TEXT("Leaving range cannot leave the old cue visible"),!Probe.bDrawn || Probe.Target.Get()!=Malet.Get());
            ExecuteIndex=0;Phase=20;return false;
        }
        if(Phase==20)
        {PC->ToggleArchive();Mark=Frame;Phase=21;return false;}
        if(Phase==21 && Frame>Mark+1)
        {
            AActor* Target=ExecuteIndex==0?static_cast<AActor*>(Elia.Get()):ExecuteIndex==1?static_cast<AActor*>(Story.Get()):static_cast<AActor*>(Malet.Get());
            Pawn->SetActorLocation(Target->GetActorLocation()+FVector(-40,0,0));Mark=Frame;Phase=22;return false;
        }
        if(Phase==22 && Frame>Mark+1){PC->ToggleArchive();Key(*PC,EKeys::D,IE_Pressed);Mark=Frame;Phase=23;return false;}
        if(Phase==23 && Frame>Mark+2){Key(*PC,EKeys::D,IE_Released);Mark=Frame;Phase=24;return false;}
        if(Phase==24 && Frame>Mark+8)
        {
            AActor* Target=ExecuteIndex==0?static_cast<AActor*>(Elia.Get()):ExecuteIndex==1?static_cast<AActor*>(Story.Get()):static_cast<AActor*>(Malet.Get());
            if(!CheckFocus(*PC,Target,false))return true;
            InvocationBefore=Host->GetFieldInvocationCount();HiddenSince=GFrameCounter;Key(*PC,ExecuteIndex==1?EKeys::Gamepad_FaceButton_Bottom:EKeys::E,IE_Pressed);Mark=Frame;Phase=25;return false;
        }
        if(Phase==25 && Frame>Mark+3)
        {
            Key(*PC,ExecuteIndex==1?EKeys::Gamepad_FaceButton_Bottom:EKeys::E,IE_Released);
            Test->TestTrue(TEXT("Actual E/A executes the same focused target through the original resolver"),Host->GetState()==EMemoriaSliceState::Field && Host->GetFieldInvocationCount()==InvocationBefore+1);
            Test->TestTrue(TEXT("Dialogue ownership hides target cue"),CueHidden(*PC,HiddenSince));
            if(Test->HasAnyErrors() || ExecuteIndex==2)return true;
            Mark=Frame;Phase=26;return false;
        }
        if(Phase==26)
        {
            if(Host->GetState()==EMemoriaSliceState::Field){if(Frame%6==0)Tap(*PC,EKeys::Enter);return false;}
            if(Host->GetState()!=EMemoriaSliceState::Exploration || PC->IsModalOpen())return false;
            if(ExecuteIndex==1)
            {
                Test->TestFalse(TEXT("Consumed original story point no longer offers interaction"),Story->CanInteract(*Pawn));
                Test->TestFalse(TEXT("Consumed original story cannot execute again"),Host->StartStoryBeat(StoryGroup));
            }
            ++ExecuteIndex;Phase=20;return false;
        }
        return false;
    }
private:
    bool CheckFocus(const AMemoriaSliceController& PC,AActor* Expected,bool bPad)
    {
        Test->TestTrue(TEXT("Actual resolver chooses the intended original actor"),PC.GetInteraction()->GetTarget()==Expected);
        const auto* Hud=PC.GetFieldHud();if(!Test->TestNotNull(TEXT("Field owns the actual painted HUD"),Hud))return false;
        const auto& Probe=Hud->GetInteractionFocusPaintProbe();
        if(!MemoriaInteractionFocusS364::IsEnabled())return Test->TestFalse(TEXT("Baseline disables only the new cue"),Probe.bDrawn);
        Test->TestTrue(TEXT("Fresh painted focus belongs to resolver, not a forced target"),Probe.bDrawn && Probe.Frame>=GFrameCounter-2 && Probe.Target.Get()==Expected);
        Test->TestEqual(TEXT("Focus preserves resolver prompt"),Probe.Prompt,PC.GetInteraction()->GetPrompt());
        Test->TestEqual(TEXT("Actual focus records current input preference"),Probe.bUsingGamepad,bPad);
        Test->TestEqual(TEXT("Bounded cue uses twelve thin strokes"),Probe.Segments,12);
        Test->TestTrue(TEXT("All cue strokes lie inside actual HUD viewport"),Probe.Bounds.bIsValid && Probe.Bounds.Min.X>=0 && Probe.Bounds.Min.Y>=0 && Probe.Bounds.Max.X<=Probe.Viewport.X && Probe.Bounds.Max.Y<=Probe.Viewport.Y);
        return !Test->HasAnyErrors();
    }
    void MatchTarget()
    {auto Window=SavedWindow.Pin();static_cast<FSceneViewport*>(Client->Viewport)->SetViewportSize(Desired.X,Desired.Y);const auto Extent=Window->GetSizeInScreen();Window->SetIndependentViewportSize(Extent);FSlateApplication::Get().GetRenderer()->UpdateFullscreenState(Window.ToSharedRef(),FMath::RoundToInt(Extent.X),FMath::RoundToInt(Extent.Y));}
    void Capture(const FString& Name,const AMemoriaSliceController& PC,const AMemoriaFieldPawn& Pawn)
    {
        ScreenshotPath=Output/(Name+(MemoriaInteractionFocusS364::IsEnabled()?TEXT("_After.png"):TEXT("_Baseline.png")));bProcessed=false;
        const auto& Probe=PC.GetFieldHud()->GetInteractionFocusPaintProbe();
        CaptureReport=MakeShared<FJsonObject>();CaptureReport->SetStringField(TEXT("target"),PC.GetInteraction()->GetTarget()->GetPathName());CaptureReport->SetStringField(TEXT("prompt"),PC.GetInteraction()->GetPrompt());
        CaptureReport->SetStringField(TEXT("target_world"),PC.GetInteraction()->GetTarget()->GetActorLocation().ToString());CaptureReport->SetStringField(TEXT("cue_world_anchor"),Probe.WorldAnchor.ToString());
        CaptureReport->SetBoolField(TEXT("feature_enabled"),MemoriaInteractionFocusS364::IsEnabled());CaptureReport->SetNumberField(TEXT("fixed_delta_seconds"),FApp::GetFixedDeltaTime());
        CaptureReport->SetStringField(TEXT("fixture_scope"),TEXT("Actual original target/resolver and game camera; Elia actor tick is temporarily frozen/restored for stable focus sites. World time/weather are recorded, not pixel-identical claims."));
        CaptureReport->SetStringField(TEXT("pawn_world"),Pawn.GetActorLocation().ToString());CaptureReport->SetStringField(TEXT("camera_world"),Pawn.GetFieldCamera()->GetComponentLocation().ToString());CaptureReport->SetStringField(TEXT("camera_rotation"),Pawn.GetFieldCamera()->GetComponentRotation().ToString());CaptureReport->SetNumberField(TEXT("camera_fov"),Pawn.GetFieldCamera()->FieldOfView);
        CaptureReport->SetNumberField(TEXT("world_seconds"),Pawn.GetWorld()->GetTimeSeconds());CaptureReport->SetNumberField(TEXT("frame"),GFrameCounter);CaptureReport->SetBoolField(TEXT("drawn"),Probe.bDrawn);CaptureReport->SetStringField(TEXT("bounds"),Probe.Bounds.ToString());CaptureReport->SetBoolField(TEXT("pad"),PC.GetCombatInputRouter()->IsUsingGamepad());
        if(!Test->TestFalse(TEXT("Focus never replaces another screenshot"),FScreenshotRequest::IsScreenshotRequested()) ||
            !Test->TestFalse(TEXT("Focus does not replace active highres capture"),GIsHighResScreenshot)){bProcessed=true;ScreenshotPath.Reset();return;}
        FScreenshotRequest::RequestScreenshot(ScreenshotPath,true,false,false,FIntRect(),true);
    }
    bool CaptureDone()
    {
        if(!bProcessed)return false;TArray<uint8> Bytes;
        if(!Test->TestTrue(TEXT("Focus native PNG is saved"),FFileHelper::LoadFileToArray(Bytes,*ScreenshotPath) && Bytes.Num()>24))return true;
        auto Big=[&](int32 I){return uint32(Bytes[I])<<24|uint32(Bytes[I+1])<<16|uint32(Bytes[I+2])<<8|uint32(Bytes[I+3]);};
        const uint8 Signature[]={137,80,78,71,13,10,26,10};
        Test->TestTrue(TEXT("Focus PNG keeps exact physical requested dimensions"),FMemory::Memcmp(Bytes.GetData(),Signature,8)==0 && Big(16)==uint32(Desired.X) && Big(20)==uint32(Desired.Y));
        FSHAHash Hash;FSHA1::HashBuffer(Bytes.GetData(),Bytes.Num(),Hash.Hash);CaptureReport->SetStringField(TEXT("sha1"),Hash.ToString());CaptureReport->SetNumberField(TEXT("width"),Big(16));CaptureReport->SetNumberField(TEXT("height"),Big(20));
        FString Json;FJsonSerializer::Serialize(CaptureReport.ToSharedRef(),TJsonWriterFactory<>::Create(&Json));FFileHelper::SaveStringToFile(Json,*FPaths::ChangeExtension(ScreenshotPath,TEXT("json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);return true;
    }
    void ScreenshotProcessed(){bProcessed=true;}
    FAutomationTestBase* Test;double Started,OldDelta=0.;uint64 LastFrame=MAX_uint64,HiddenSince=0;
    uint32 OldScreenshotX,OldScreenshotY;int32 Frame=0,Mark=0,Phase=0,CaseIndex=0,ResizeAttempts=0,ExecuteIndex=0,InvocationBefore=0;
    bool bReady=false,bFixed=false,bOldFixed=false,bOldEliaTick=true,bOldDriven=true,bOldMaximized=false,bProcessed=false,bCameraChanged=false;
    TWeakObjectPtr<UMemoriaRunSubsystem> Run;TWeakObjectPtr<UGameViewportClient> Client;
    TWeakObjectPtr<AMemoriaSliceController> Controller;TWeakObjectPtr<AMemoriaFieldPawn> ObservedPawn;
    TWeakObjectPtr<AMemoriaEliaCompanion> Elia;TWeakObjectPtr<AMemoriaMaletActor> Malet;TWeakObjectPtr<AMemoriaStoryPointActor> Story,OwnedPoint,DestroyedPoint;
    TWeakObjectPtr<AActor> Blocker;TWeakPtr<SWindow> SavedWindow;FIntPoint OldSize,Desired;FVector2D OldPosition,OldWindowSize,OldWindowViewport;
    EWindowMode::Type OldMode=EWindowMode::Windowed;FRotator OldCamera;FString Output,ScreenshotPath,StoryGroup,BeforeObservation;FDelegateHandle ScreenshotHandle;TSharedPtr<FJsonObject> CaptureReport;
};
class FChapterEliaS364Replay final : public IAutomationLatentCommand
{
public:
    FChapterEliaS364Replay(FAutomationTestBase* InTest,const FString& InMap):Test(InTest),Map(InMap),Started(FPlatformTime::Seconds()){}
    ~FChapterEliaS364Replay() override {if(Elia.IsValid())Elia->SetActorTickEnabled(bOldTick);}
    bool Update() override
    {
        if(LastFrame==GFrameCounter)return false;LastFrame=GFrameCounter;
        if(FPlatformTime::Seconds()-Started>180.){Test->AddError(TEXT("Chapter Elia eligibility timeout"));return true;}
        auto* World=GEditor?GEditor->PlayWorld.Get():nullptr;auto* PC=World?Cast<AMemoriaSliceController>(World->GetFirstPlayerController()):nullptr;APawn* Pawn=PC?PC->GetPawn().Get():nullptr;
        if(!Pawn)return false;auto* Host=World->GetGameInstance()->GetSubsystem<UMemoriaNarrativeSubsystem>();auto* Run=World->GetGameInstance()->GetSubsystem<UMemoriaRunSubsystem>();++Frame;
        if(Host->GetState()==EMemoriaSliceState::VN || Host->GetState()==EMemoriaSliceState::Field){if(Frame%4==0)Host->Confirm();return false;}
        if(Host->GetState()!=EMemoriaSliceState::Exploration || Host->GetChapterMap()!=Map)return false;
        if(Phase==0)
        {
            AMemoriaChapterPresentation* Presentation=nullptr;for(TActorIterator<AMemoriaChapterPresentation> It(World);It;++It)if(It->GetMap()==Map)Presentation=*It;
            if(!Presentation || (Presentation->GetCard() && Presentation->GetCard()->IsShowing()))return false;
            for(const auto& Link:Presentation->GetSpec()->Sequence)if(!Link.Flag.IsEmpty() && !Run->GetRunSnapshot().GetFlag(Link.Flag))return false;
            for(TActorIterator<AMemoriaEliaCompanion> It(World);It;++It)Elia=*It;if(!Test->TestTrue(TEXT("Current chapter retains its actual companion"),Elia.IsValid()))return true;
            bOldTick=Elia->IsActorTickEnabled();Elia->SetActorTickEnabled(false);PC->ToggleArchive();Mark=Frame;Phase=1;return false;
        }
        if(Phase==1 && Frame>Mark+1){Pawn->SetActorLocation(Elia->GetActorLocation()+FVector(0,40,0));Mark=Frame;Phase=2;return false;}
        if(Phase==2 && Frame>Mark+1){PC->ToggleArchive();Mark=Frame;Phase=3;return false;}
        if(Phase==3 && Frame>Mark+8)
        {
            const FString Before=RunJson(*Run);const int32 Invocations=Host->GetFieldInvocationCount();
            Test->TestFalse(TEXT("Chapter never advertises unsupported Verdan companion dialogue"),Host->CanInteractWithElia());
            Test->TestFalse(TEXT("Companion eligibility shares actual narrative refusal"),Elia->CanInteract(*Pawn));
            Test->TestTrue(TEXT("Original resolver skips unsupported companion"),PC->GetInteraction()->GetTarget()!=Elia.Get());
            Test->TestFalse(TEXT("Chapter prompt has no false Elia talk affordance"),PC->GetInteractionPrompt().StartsWith(TEXT("Elia")));
            Test->TestFalse(TEXT("Actual companion interface refuses without inventing chapter dialogue"),Elia->Interact(*Pawn));
            Test->TestEqual(TEXT("Refused chapter talk changes no run, rewards, memories or flags"),RunJson(*Run),Before);
            Test->TestEqual(TEXT("Refused chapter talk starts no field"),Host->GetFieldInvocationCount(),Invocations);
            return true;
        }
        return false;
    }
private:
    FAutomationTestBase* Test;FString Map;double Started;uint64 LastFrame=MAX_uint64;int32 Frame=0,Mark=0,Phase=0;bool bOldTick=true;TWeakObjectPtr<AMemoriaEliaCompanion> Elia;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFocusS364Native,"MemoriaVisual.S364Focus.NativeTargetAndExecution",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFocusS364Native::RunTest(const FString&)
{if(!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice")))return false;FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FFocusS364Replay(this)));ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());return true;}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBeltEliaS364Native,"Memoria.S364Focus.BeltEliaEligibility",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FBeltEliaS364Native::RunTest(const FString&)
{if(!AutomationOpenMap(TEXT("/Game/Memoria/Maps/L_BeltWaystation")))return false;FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FChapterEliaS364Replay(this,TEXT("belt_waystation"))));ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());return true;}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDriftEliaS364Native,"Memoria.S364Focus.DriftEliaEligibility",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDriftEliaS364Native::RunTest(const FString&)
{if(!AutomationOpenMap(TEXT("/Game/Memoria/Maps/L_DriftShelter")))return false;FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FChapterEliaS364Replay(this,TEXT("drift_shelter"))));ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());return true;}
#endif
