#include "Misc/AutomationTest.h"
#include "CoreGlobals.h"
#include "Tests/AutomationEditorCommon.h"
#include "Tests/AutomationCommon.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Slate/SceneViewport.h"
#include "Widgets/SWindow.h"
#include "Widgets/SViewport.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"
#include "Framework/MemoriaSliceHost.h"
#include "Framework/MemoriaFieldPawn.h"
#include "Combat/MemoriaFieldCombatSubsystem.h"
#include "Combat/MemoriaCombatHudWidget.h"
#include "Combat/MemoriaDefenseFeedbackS364.h"
#include "Combat/MemoriaFieldMonster.h"
#include "Narrative/MemoriaNarrativeSubsystem.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Domain/MemoriaPlayerMemoryDomain.h"
#include "Interaction/MemoriaEliaCompanion.h"
#include "Settings/MemoriaSettingsSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "UObject/UObjectIterator.h"
#include "Misc/App.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "HAL/FileManager.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "InputKeyEventArgs.h"
#include "UnrealClient.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
const TCHAR* OutcomeName(EMemoriaDefenseOutcome Outcome)
{
    switch (Outcome)
    {
    case EMemoriaDefenseOutcome::Block: return TEXT("Block");
    case EMemoriaDefenseOutcome::Parry: return TEXT("Parry");
    case EMemoriaDefenseOutcome::Ward: return TEXT("Ward");
    default: return TEXT("None");
    }
}
TArray<TSharedPtr<FJsonValue>> VectorJson(const FVector& V)
{ return {MakeShared<FJsonValueNumber>(V.X),MakeShared<FJsonValueNumber>(V.Y),MakeShared<FJsonValueNumber>(V.Z)}; }
bool Contains(const FBox2D& Outer,const FBox2D& Inner)
{ return Inner.bIsValid && Outer.IsInsideOrOn(Inner.Min) && Outer.IsInsideOrOn(Inner.Max); }

// Controlled real contacts, not fabricated popup/impact state: physical K/RMB
// enters guard; B consumes the real ink item; StrikePlayer resolves the actual
// mitigation/parry/ward path against a registered, nearby living thief. Its AI
// alone is held for repeatability; existing FieldFeel separately exercises AI.
class FDefenseS364Replay final : public IAutomationLatentCommand
{
public:
    explicit FDefenseS364Replay(FAutomationTestBase* InTest) : Test(InTest),Started(FPlatformTime::Seconds()),
        OldScreenshotX(GScreenshotResolutionX),OldScreenshotY(GScreenshotResolutionY) {}
    ~FDefenseS364Replay() override
    {
        FScreenshotRequest::OnScreenshotRequestProcessed().Remove(ScreenshotHandle);
        if (auto* PC=Controller.Get()) UGameplayStatics::SetGamePaused(PC,bOldPaused);
        if (auto* C=Combat.Get()) { C->EndBlock(); C->ResetForRestoredRun(); }
        if (Settings.IsValid())
        {
            Settings->SetReduceMotion(bOldReduceMotion);Settings->SetReduceFlash(bOldReduceFlash);
            Settings->SetTextPercent(OldText);Settings->SetShakePercent(OldShake);Settings->SetLocale(OldSettingsLocale);
        }
        if(Run.IsValid())Run->SetLocale(OldLocale);
        for(const auto& Hidden:HiddenActors)if(Hidden.Key.IsValid())Hidden.Key->SetActorHiddenInGame(Hidden.Value);
        if(Client.IsValid() && Client->Viewport && OldSize.X>0)
        {
            static_cast<FSceneViewport*>(Client->Viewport)->SetViewportSize(OldSize.X,OldSize.Y);
            if(auto Window=SavedWindow.Pin())
            {
                Window->SetWindowMode(OldMode);Window->ReshapeWindow(OldPosition,OldWindowSize);
                Window->SetViewportSizeDrivenByWindow(bOldDriven);
                if(!bOldDriven)Window->SetIndependentViewportSize(OldWindowViewport);
                if(bOldMaximized)Window->Maximize();
                const auto Extent=Window->GetViewportSize();
                FSlateApplication::Get().GetRenderer()->UpdateFullscreenState(Window.ToSharedRef(),FMath::RoundToInt(Extent.X),FMath::RoundToInt(Extent.Y));
            }
        }
        if(bFixed){FApp::SetUseFixedTimeStep(bOldFixed);FApp::SetFixedDeltaTime(OldDelta);}
        GScreenshotResolutionX=OldScreenshotX;GScreenshotResolutionY=OldScreenshotY;
    }
    bool Update() override
    {
        if(LastFrame==GFrameCounter)return false;LastFrame=GFrameCounter;
        if(FPlatformTime::Seconds()-Started>360.){Test->AddError(FString::Printf(TEXT("Defense S364 timeout case%d outcome%d phase%d"),Case,OutcomeIndex,Phase));return true;}
        UWorld* World=GEditor?GEditor->PlayWorld.Get():nullptr;
        auto* PC=World && !World->bIsTearingDown?Cast<AMemoriaSliceController>(World->GetFirstPlayerController()):nullptr;
        auto* Pawn=PC?Cast<AMemoriaFieldPawn>(PC->GetPawn()):nullptr;
        if(!Pawn || World->GetTimeSeconds()<.3)return false;
        auto* Narrative=World->GetGameInstance()->GetSubsystem<UMemoriaNarrativeSubsystem>();
        if(!bFixed)
        {
            bFixed=true;bOldFixed=FApp::UseFixedTimeStep();OldDelta=FApp::GetFixedDeltaTime();FApp::SetUseFixedTimeStep(true);FApp::SetFixedDeltaTime(1./60.);
            for(int32 I=0;I<20 && Narrative->GetState()==EMemoriaSliceState::VN;++I)Narrative->Confirm(1);
            return false;
        }
        if(!World->GetMapName().EndsWith(TEXT("L_VerdanHost")) || Narrative->GetState()!=EMemoriaSliceState::Exploration)return false;
        ++Frame;
        if(!bReady)
        {
            Combat=World->GetSubsystem<UMemoriaFieldCombatSubsystem>();Run=World->GetGameInstance()->GetSubsystem<UMemoriaRunSubsystem>();
            Settings=World->GetGameInstance()->GetSubsystem<UMemoriaSettingsSubsystem>();Client=World->GetGameViewport();Controller=PC;
            if(!Combat.IsValid() || !Run.IsValid() || !Settings.IsValid() || !Client.IsValid() || !Client->Viewport)return false;
            for(TObjectIterator<UMemoriaCombatHudWidget> It;It;++It)if(It->GetOwningPlayer()==PC && It->IsInViewport()){Hud=*It;break;}
            if(!Hud.IsValid())return false;
            auto Window=Client->GetWindow();if(!Test->TestTrue(TEXT("Real defense inspection window exists"),Window.IsValid()))return true;
            SavedWindow=Window;OldMode=Window->GetWindowMode();OldPosition=Window->GetPositionInScreen();OldWindowSize=Window->GetSizeInScreen();
            OldWindowViewport=Window->GetViewportSize();bOldDriven=Window->IsViewportSizeDrivenByWindow();bOldMaximized=Window->IsWindowMaximized();
            OldSize=Client->Viewport->GetSizeXY();OldLocale=Run->GetRunSnapshot().CurrentLocale;OldSettingsLocale=Settings->GetLocale();
            OldText=Settings->GetTextPercent();OldShake=Settings->GetShakePercent();bOldReduceMotion=Settings->IsReduceMotion();bOldReduceFlash=Settings->IsReduceFlash();bOldPaused=World->IsPaused();
            BeforeBurns=Run->GetPlayerMemory()->GetSnapshot().BurnedHistory;
            for(TActorIterator<AMemoriaEliaCompanion> It(World);It;++It){HiddenActors.Emplace(*It,It->IsHidden());It->SetActorHiddenInGame(true);}
            ScreenshotHandle=FScreenshotRequest::OnScreenshotRequestProcessed().AddRaw(this,&FDefenseS364Replay::ScreenshotProcessed);
            Output=FPaths::ProjectSavedDir()/TEXT("Validation/S364Defense")/FApp::GetInstanceId().ToString(EGuidFormats::Digits);
            IFileManager::Get().MakeDirectory(*Output,true);Report=MakeShared<FJsonObject>();Report->SetStringField(TEXT("mode"),MemoriaDefenseFeedbackS364::IsEnabled()?TEXT("After"):TEXT("Baseline"));
            Report->SetStringField(TEXT("scope"),TEXT("Physical guard/ink input plus real controlled nearby contact. Original AI strike regression remains separate. Same authored camera/site; real ages and world times recorded; simulation pauses only while asynchronous PNG is pending. No forced age, paint state or damage result."));
            bReady=true;
        }
        if(Case>=8)
        {
            Test->TestTrue(TEXT("Defense inspection never burns an authored memory"),Run->GetPlayerMemory()->GetSnapshot().BurnedHistory==BeforeBurns);
            Report->SetArrayField(TEXT("captures"),Captures);Report->SetStringField(TEXT("status"),Test->HasAnyErrors()?TEXT("FAIL"):TEXT("PASS"));
            FString Json;auto Writer=TJsonWriterFactory<>::Create(&Json);FJsonSerializer::Serialize(Report.ToSharedRef(),Writer);
            FFileHelper::SaveStringToFile(Json,*(Output/TEXT("Defense.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
            return true;
        }
        const bool bKo=(Case&1)!=0,bSmall=(Case&2)!=0,bReduced=(Case&4)!=0;
        const EMemoriaDefenseOutcome Outcome=OutcomeIndex==1?EMemoriaDefenseOutcome::Parry:OutcomeIndex==2?EMemoriaDefenseOutcome::Ward:EMemoriaDefenseOutcome::Block;
        Desired=bSmall?FIntPoint(640,360):FIntPoint(1280,720);
        auto Key=[&](FKey K,EInputEvent Event){PC->InputKey(FInputKeyEventArgs::CreateSimulated(K,Event,Event==IE_Released?0.f:1.f));};
        if(Phase==0)
        {
            Settings->SetLocale(bKo?TEXT("ko"):TEXT("en"));Run->SetLocale(bKo?TEXT("ko"):TEXT("en"));
            Settings->SetReduceMotion(bReduced);Settings->SetReduceFlash(bReduced);Settings->SetTextPercent(bReduced?125:100);Settings->SetShakePercent(0);
            auto Window=SavedWindow.Pin();Window->SetWindowMode(EWindowMode::Windowed);if(Window->IsWindowMaximized())Window->Restore();Window->Resize(FVector2D(Desired));MatchSceneTarget();
            ResizeAttempts=0;Mark=Frame;Phase=1;return false;
        }
        if(Phase==1)
        {
            if(Frame<Mark+12)return false;
            auto ViewportWidget=Client->GetGameViewportWidget();if(!ViewportWidget)return false;
            const FVector2D Absolute=ViewportWidget->GetPaintSpaceGeometry().GetAbsoluteSize();const FIntPoint Pixels(FMath::RoundToInt(FMath::Abs(Absolute.X)),FMath::RoundToInt(FMath::Abs(Absolute.Y)));
            if(Pixels!=Desired && ResizeAttempts++<3)
            {auto Window=SavedWindow.Pin();Window->Resize(FVector2D(Window->GetClientSizeInScreen())+FVector2D(Desired-Pixels));MatchSceneTarget();Mark=Frame;return false;}
            if(!Test->TestEqual(TEXT("Actual defense Slate physical dimensions"),Pixels,Desired) || !Test->TestEqual(TEXT("Actual defense scene target dimensions"),Client->Viewport->GetRenderTargetTextureSizeXY(),Desired))return true;
            Phase=2;
        }
        if(Phase==2)
        {
            Key(EKeys::K,IE_Released);Key(EKeys::RightMouseButton,IE_Released);Combat->Revive(OutcomeIndex==3?.000001f:1.f);Combat->ResetForRestoredRun();
            const auto Foes=Combat->SpawnWave(1,Pawn->GetActorLocation()+FVector(135,0,0),0.f,EMemoriaFoeKind::MarketThief);
            if(!Test->TestEqual(TEXT("One actual registered contact owner"),Foes.Num(),1))return true;
            Foe=Foes[0];Foe->SetActorTickEnabled(false);if(auto* Move=Foe->GetMovementComponent()){Move->StopMovementImmediately();Move->SetComponentTickEnabled(false);}
            HpBefore=Combat->GetPlayerHp();BlocksBefore=Combat->GetBlocks();ParriesBefore=Combat->GetParries();
            Test->TestEqual(TEXT("Explicit HP1 guard fixture uses the real Revive clamp"),HpBefore,OutcomeIndex==3?int64(1):Combat->GetPlayerMaxHp());
            if(Outcome==EMemoriaDefenseOutcome::Ward)
            {
                Run->GrantFieldItem(TEXT("witness_ink"),1);InkBefore=Run->GetItemCount(TEXT("witness_ink"));
                Key(EKeys::B,IE_Pressed);Key(EKeys::B,IE_Released);
                if(!Test->TestTrue(TEXT("Physical B arms the real witness ink"),Combat->IsWarded()))return true;
                Test->TestEqual(TEXT("Real ward consumes exactly one inventory item"),Run->GetItemCount(TEXT("witness_ink")),InkBefore-1);
            }
            else
            {
                GuardKey=(Case&1)?EKeys::RightMouseButton:EKeys::K;Key(GuardKey,IE_Pressed);
                if(!Test->TestTrue(TEXT("Physical guard input enters the actual guard state"),Combat->IsBlocking()))return true;
            }
            GuardAt=World->GetTimeSeconds();Phase=3;return false;
        }
        if(Phase==3)
        {
            if(Outcome==EMemoriaDefenseOutcome::Block && World->GetTimeSeconds()-GuardAt<MemoriaCombatTuning::ParryWindow+.06f)return false;
            const float Damage=Foe->Spec().Damage;
            const bool bHarmed=Combat->StrikePlayer(Foe.Get(),Damage);
            HpLoss=HpBefore-Combat->GetPlayerHp();
            const int64 Expected=Outcome==EMemoriaDefenseOutcome::Block?FMath::Min<int64>(FMath::RoundToInt64(Damage*MemoriaCombatTuning::BlockFactor),HpBefore-1):0;
            Test->TestEqual(TEXT("Actual mitigation produces exact expected HP loss"),HpLoss,Expected);
            Test->TestEqual(TEXT("Only held block reports a damaging contact"),bHarmed,Outcome==EMemoriaDefenseOutcome::Block);
            Test->TestFalse(TEXT("Defense blocks the thief's weakening status"),Combat->IsWeakened());Test->TestFalse(TEXT("Defense introduces no poison"),Combat->IsPoisoned());
            Test->TestEqual(TEXT("Actual block event count"),Combat->GetBlocks(),BlocksBefore+(Outcome==EMemoriaDefenseOutcome::Block?1:0));
            Test->TestEqual(TEXT("Actual parry event count"),Combat->GetParries(),ParriesBefore+(Outcome==EMemoriaDefenseOutcome::Parry?1:0));
            if(Outcome==EMemoriaDefenseOutcome::Parry)Test->TestEqual(TEXT("Parried live foe actually staggers"),Foe->GetState(),EMemoriaMonsterState::Stagger);
            if(Outcome==EMemoriaDefenseOutcome::Ward)Test->TestFalse(TEXT("The actual ward is spent"),Combat->IsWarded());
            const auto* Popup=Combat->GetPopups().FindByPredicate([&](const FMemoriaCombatPopup& P){return P.Outcome==Outcome;});
            const auto* Impact=Combat->GetImpacts().FindByPredicate([&](const FMemoriaImpact& P){return P.Outcome==Outcome;});
            if(!Test->TestTrue(TEXT("Production defense emits typed popup and impact"),Popup && Impact))return true;
            Test->TestEqual(TEXT("Typed popup retains actual HP loss rather than predicted incoming damage"),Popup->Amount,float(HpLoss));
            LegacyLabel=Popup->Label;Sample=0;Phase=4;return false;
        }
        if(Phase==4)
        {
            const float TargetAge=Case==0 && OutcomeIndex<3?(Sample==0?.02f:Sample==1?.10f:.30f):.10f;
            const auto* Popup=Combat->GetPopups().FindByPredicate([&](const FMemoriaCombatPopup& P){return P.Outcome==Outcome;});
            if(!Popup){Test->AddError(TEXT("Actual defense popup expired before inspection"));return true;}
            if(Popup->Age<TargetAge)return false;
            if(!Test->TestTrue(TEXT("Async screenshot owns the simulation pause only after reaching a real popup age"),UGameplayStatics::SetGamePaused(PC,true)))return true;
            PausedAge=Popup->Age;Mark=Frame;Phase=5;return false;
        }
        if(Phase==5)
        {
            if(Frame<Mark+2)return false;
            if(!CheckPaint(Outcome,bKo,bReduced))return true;
            if(!Test->TestFalse(TEXT("Defense screenshot never replaces an unrelated pending request"),FScreenshotRequest::IsScreenshotRequested()))return true;
            ScreenshotPath=Output/FString::Printf(TEXT("Case%02d_%s%s_T%02d.png"),Case,OutcomeName(Outcome),OutcomeIndex==3?TEXT("HP1"):TEXT(""),Case==0 && OutcomeIndex<3?(Sample==0?2:Sample==1?10:30):10);
            bScreenshotProcessed=false;FScreenshotRequest::RequestScreenshot(ScreenshotPath,true,false,false,FIntRect(),true);
            Mark=Frame;Phase=6;return false;
        }
        if(Phase==6)
        {
            if(!bScreenshotProcessed){if(Frame<Mark+300)return false;Test->AddError(TEXT("Defense screenshot processing timed out"));return true;}
            if(!CheckPaint(Outcome,bKo,bReduced))return true;
            const auto* Popup=Combat->GetPopups().FindByPredicate([&](const FMemoriaCombatPopup& P){return P.Outcome==Outcome;});
            Test->TestTrue(TEXT("Capture pause keeps its actual contact age stable"),Popup && FMath::IsNearlyEqual(Popup->Age,PausedAge,.0001f));
            if(!RecordCapture(*PC,*Pawn,Outcome,bKo,bReduced))return true;
            UGameplayStatics::SetGamePaused(PC,false);
            if(Case==0 && OutcomeIndex<3 && ++Sample<3){Phase=4;return false;}
            Key(EKeys::K,IE_Released);Key(EKeys::RightMouseButton,IE_Released);
            if(Test->HasAnyErrors())return true;
            if(++OutcomeIndex>=4){OutcomeIndex=0;++Case;Phase=0;}else Phase=2;
        }
        return false;
    }
private:
    bool CheckPaint(EMemoriaDefenseOutcome Outcome,bool bKo,bool bReduced)
    {
        const auto& Probe=Hud->GetDefensePaintProbe();const bool Enabled=MemoriaDefenseFeedbackS364::IsEnabled();
        if(!Test->TestTrue(TEXT("Defense probe comes from a fresh actual native HUD paint"),Probe.bDrawn && Probe.Frame>=LastFrame-2 && Probe.bEnabled==Enabled))return false;
        const auto* Painted=Probe.Popups.FindByPredicate([&](const FMemoriaDefensePopupPaintRecord& P){return P.Outcome==Outcome;});
        if(!Test->TestNotNull(TEXT("Actual outcome glyphs were submitted by HUD"),Painted))return false;
        const FString Expected=Enabled && Outcome==EMemoriaDefenseOutcome::Block && HpLoss>0?LegacyLabel+FString::Printf(TEXT(" · -%lld HP"),HpLoss):LegacyLabel;
        Test->TestEqual(TEXT("Actual painted result reveals exactly the committed block cost"),Painted->Text,Expected);
        Test->TestEqual(TEXT("Paint observer retains the actual gameplay amount"),Painted->Amount,float(HpLoss));
        Test->TestTrue(TEXT("Positive-alpha measured defense glyph and shadow fit the actual viewport"),Painted->Alpha>0 && Contains(FBox2D(FVector2D::ZeroVector,Probe.Viewport),Painted->Bounds));
        if(HpLoss==0)Test->TestFalse(TEXT("Zero-loss defense does not invent an HP cost"),Painted->Text.Contains(TEXT("-")));
        Test->TestEqual(TEXT("Real reduce-motion option reaches native paint"),Probe.bReduceMotion,bReduced);
        Test->TestTrue(TEXT("Real reduce-flash option reaches native paint"),FMath::IsNearlyEqual(Probe.FlashScale,bReduced?.15f:1.f));
        const auto* Shape=Probe.Shapes.FindByPredicate([&](const FMemoriaDefenseShapePaintRecord& P){return P.Outcome==Outcome;});
        const auto* Impact=Combat->GetImpacts().FindByPredicate([&](const FMemoriaImpact& P){return P.Outcome==Outcome;});
        if(Enabled && Impact)
            Test->TestTrue(TEXT("Actual defense shape is bounded, distinct typed line art"),Shape && Shape->Segments>0 && Shape->Segments<=12 && Contains(FBox2D(FVector2D::ZeroVector,Probe.Viewport),Shape->Bounds));
        else Test->TestTrue(TEXT("Baseline or expired defense submits no custom shield geometry"),Probe.Shapes.IsEmpty());
        return !Test->HasAnyErrors();
    }
    bool RecordCapture(AMemoriaSliceController& PC,AMemoriaFieldPawn& Pawn,EMemoriaDefenseOutcome Outcome,bool bKo,bool bReduced)
    {
        if(!Test->TestNotNull(TEXT("Actual gameplay camera exists for defense evidence"),PC.PlayerCameraManager.Get()))return false;
        TArray<uint8> Bytes;const uint8 Signature[]={137,80,78,71,13,10,26,10};
        if(!Test->TestTrue(TEXT("Saved defense evidence is actual PNG bytes"),FFileHelper::LoadFileToArray(Bytes,*ScreenshotPath) && Bytes.Num()>24 && FMemory::Memcmp(Bytes.GetData(),Signature,8)==0))return false;
        auto Big=[&](int32 O){return uint32(Bytes[O])<<24|uint32(Bytes[O+1])<<16|uint32(Bytes[O+2])<<8|uint32(Bytes[O+3]);};
        if(!Test->TestEqual(TEXT("Actual saved defense PNG dimensions"),FIntPoint(Big(16),Big(20)),Desired))return false;
        FSHAHash Hash;FSHA1::HashBuffer(Bytes.GetData(),Bytes.Num(),Hash.Hash);
        const auto& Probe=Hud->GetDefensePaintProbe();const auto* Popup=Probe.Popups.FindByPredicate([&](const FMemoriaDefensePopupPaintRecord& P){return P.Outcome==Outcome;});
        auto Item=MakeShared<FJsonObject>();Item->SetNumberField(TEXT("case"),Case);Item->SetStringField(TEXT("outcome"),OutcomeName(Outcome));
        Item->SetBoolField(TEXT("hp1_fixture"),OutcomeIndex==3);Item->SetStringField(TEXT("locale"),bKo?TEXT("ko"):TEXT("en"));Item->SetBoolField(TEXT("reduce_motion"),bReduced);Item->SetBoolField(TEXT("reduce_flash"),bReduced);
        Item->SetNumberField(TEXT("text_percent"),bReduced?125:100);Item->SetNumberField(TEXT("shake_percent"),0);Item->SetNumberField(TEXT("hp_before"),HpBefore);Item->SetNumberField(TEXT("hp_loss"),HpLoss);
        Item->SetStringField(TEXT("legacy_label"),LegacyLabel);Item->SetStringField(TEXT("painted_text"),Popup?Popup->Text:FString());Item->SetStringField(TEXT("glyph_bounds"),Popup?Popup->Bounds.ToString():FString());
        Item->SetNumberField(TEXT("popup_age"),PausedAge);Item->SetNumberField(TEXT("paint_frame"),double(Probe.Frame));Item->SetNumberField(TEXT("world_seconds"),Pawn.GetWorld()->GetTimeSeconds());Item->SetNumberField(TEXT("fixed_delta_seconds"),FApp::GetFixedDeltaTime());
        Item->SetArrayField(TEXT("player_world"),VectorJson(Pawn.GetActorLocation()));Item->SetArrayField(TEXT("camera_world"),VectorJson(PC.PlayerCameraManager->GetCameraLocation()));
        const auto Rotation=PC.PlayerCameraManager->GetCameraRotation();Item->SetArrayField(TEXT("camera_rotation"),VectorJson(FVector(Rotation.Pitch,Rotation.Yaw,Rotation.Roll)));Item->SetNumberField(TEXT("camera_fov"),PC.PlayerCameraManager->GetFOVAngle());
        Item->SetStringField(TEXT("png"),ScreenshotPath);Item->SetStringField(TEXT("sha1"),Hash.ToString());Item->SetNumberField(TEXT("width"),Big(16));Item->SetNumberField(TEXT("height"),Big(20));
        Item->SetNumberField(TEXT("custom_shapes"),Probe.Shapes.Num());Captures.Add(MakeShared<FJsonValueObject>(Item));
        return true;
    }
    void MatchSceneTarget()
    {
        auto Window=SavedWindow.Pin();static_cast<FSceneViewport*>(Client->Viewport)->SetViewportSize(Desired.X,Desired.Y);
        const auto Extent=Window->GetSizeInScreen();Window->SetIndependentViewportSize(Extent);
        FSlateApplication::Get().GetRenderer()->UpdateFullscreenState(Window.ToSharedRef(),FMath::RoundToInt(Extent.X),FMath::RoundToInt(Extent.Y));
    }
    void ScreenshotProcessed(){bScreenshotProcessed=true;}
    FAutomationTestBase* Test;double Started,OldDelta=0.;uint64 LastFrame=MAX_uint64;
    uint32 OldScreenshotX,OldScreenshotY;int32 Frame=0,Mark=0,Case=0,OutcomeIndex=0,Phase=0,Sample=0,ResizeAttempts=0,OldText=100,OldShake=100,BlocksBefore=0,ParriesBefore=0;
    int64 HpBefore=0,HpLoss=0,InkBefore=0;float GuardAt=0.f,PausedAge=0.f;
    bool bFixed=false,bOldFixed=false,bReady=false,bOldDriven=true,bOldMaximized=false,bOldPaused=false,bOldReduceMotion=false,bOldReduceFlash=false,bScreenshotProcessed=false;
    FKey GuardKey=EKeys::K;FString OldLocale,OldSettingsLocale,LegacyLabel,Output,ScreenshotPath;TArray<FString> BeforeBurns;
    TArray<TPair<TWeakObjectPtr<AActor>,bool>> HiddenActors;TArray<TSharedPtr<FJsonValue>> Captures;TSharedPtr<FJsonObject> Report;
    TWeakObjectPtr<UMemoriaFieldCombatSubsystem> Combat;TWeakObjectPtr<UMemoriaCombatHudWidget> Hud;TWeakObjectPtr<UMemoriaRunSubsystem> Run;TWeakObjectPtr<UMemoriaSettingsSubsystem> Settings;
    TWeakObjectPtr<UGameViewportClient> Client;TWeakObjectPtr<AMemoriaSliceController> Controller;TWeakObjectPtr<AMemoriaFieldMonster> Foe;
    TWeakPtr<SWindow> SavedWindow;FIntPoint OldSize=FIntPoint::ZeroValue,Desired=FIntPoint::ZeroValue;EWindowMode::Type OldMode=EWindowMode::Windowed;FVector2D OldPosition,OldWindowSize,OldWindowViewport;FDelegateHandle ScreenshotHandle;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDefenseS364Native,"MemoriaVisual.S364Defense.OutcomesAndAccessibility",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDefenseS364Native::RunTest(const FString&)
{
    if(!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice")))return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FDefenseS364Replay(this)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());return true;
}
#endif
