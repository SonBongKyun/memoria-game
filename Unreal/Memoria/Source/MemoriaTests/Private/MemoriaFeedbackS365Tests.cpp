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
#include "Camera/CameraComponent.h"
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
#include "Combat/MemoriaCombatFeedbackS365.h"
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
#include "Blueprint/WidgetLayoutLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
TArray<TSharedPtr<FJsonValue>> VectorJson(const FVector& V)
{ return {MakeShared<FJsonValueNumber>(V.X),MakeShared<FJsonValueNumber>(V.Y),MakeShared<FJsonValueNumber>(V.Z)}; }
bool Contains(const FBox2D& Outer,const FBox2D& Inner)
{ return Inner.bIsValid && Outer.IsInsideOrOn(Inner.Min) && Outer.IsInsideOrOn(Inner.Max); }

// Controlled real contacts, not fabricated popup/impact state: physical K/RMB
// enters guard; B consumes the real ink item; StrikePlayer resolves the actual
// mitigation/parry/ward path against a registered, nearby living thief. Its AI
// alone is held for repeatability; existing FieldFeel separately exercises AI.
class FFeedbackS365Replay final : public IAutomationLatentCommand
{
public:
    explicit FFeedbackS365Replay(FAutomationTestBase* InTest) : Test(InTest),Started(FPlatformTime::Seconds()),
        OldScreenshotX(GScreenshotResolutionX),OldScreenshotY(GScreenshotResolutionY) {}
    ~FFeedbackS365Replay() override
    {
        FScreenshotRequest::OnScreenshotRequestProcessed().Remove(ScreenshotHandle);
        if(EdgeCamera.IsValid() && bCameraOffset)EdgeCamera->SetWorldTransform(OldCameraTransform);
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
        if(FPlatformTime::Seconds()-Started>360.){Test->AddError(FString::Printf(TEXT("Feedback S365 timeout case%d outcome%d phase%d"),Case,OutcomeIndex,Phase));return true;}
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
            ScreenshotHandle=FScreenshotRequest::OnScreenshotRequestProcessed().AddRaw(this,&FFeedbackS365Replay::ScreenshotProcessed);
            Output=FPaths::ProjectSavedDir()/TEXT("Validation/S365Feedback")/FApp::GetInstanceId().ToString(EGuidFormats::Digits);
            IFileManager::Get().MakeDirectory(*Output,true);Report=MakeShared<FJsonObject>();Report->SetStringField(TEXT("mode"),MemoriaCombatFeedbackS365::IsEnabled()?TEXT("After"):TEXT("Baseline"));
            Report->SetStringField(TEXT("scope"),TEXT("Actual held guard then fresh parry and real blade HP damage, concurrent popup fonts and game-time recent damage. Controlled registered AI, actual semantic combat execution; no forced popup values/ages. Edge-smoke uses real item, destroys foe without reward and temporarily reframes actual camera; popup data is untouched. Native1280/640 PNG, no resizing or cropping."));
            bReady=true;
        }
        if(Case>=16)
        {
            Test->TestTrue(TEXT("Defense inspection never burns an authored memory"),Run->GetPlayerMemory()->GetSnapshot().BurnedHistory==BeforeBurns);
            Report->SetArrayField(TEXT("captures"),Captures);Report->SetStringField(TEXT("status"),Test->HasAnyErrors()?TEXT("FAIL"):TEXT("PASS"));
            FString Json;auto Writer=TJsonWriterFactory<>::Create(&Json);FJsonSerializer::Serialize(Report.ToSharedRef(),Writer);
            FFileHelper::SaveStringToFile(Json,*(Output/TEXT("Feedback.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
            return true;
        }
        const bool bKo=(Case&1)!=0,bSmall=(Case&2)!=0,bReduced=(Case&4)!=0;
        const int32 TextPercent=(Case&8)?125:100;
        Desired=bSmall?FIntPoint(640,360):FIntPoint(1280,720);
        auto Key=[&](FKey K,EInputEvent Event){PC->InputKey(FInputKeyEventArgs::CreateSimulated(K,Event,Event==IE_Released?0.f:1.f));};
        if(Phase==0)
        {
            Settings->SetLocale(bKo?TEXT("ko"):TEXT("en"));Run->SetLocale(bKo?TEXT("ko"):TEXT("en"));
            Settings->SetReduceMotion(bReduced);Settings->SetReduceFlash(bReduced);Settings->SetTextPercent(TextPercent);Settings->SetShakePercent(0);
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
            Key(EKeys::K,IE_Released);Key(EKeys::RightMouseButton,IE_Released);Combat->Revive(1.f);Combat->ResetForRestoredRun();
            const auto Foes=Combat->SpawnWave(1,Pawn->GetActorLocation()+FVector(135,0,0),0.f,EMemoriaFoeKind::MarketThief);
            if(!Test->TestEqual(TEXT("One actual registered controlled foe"),Foes.Num(),1))return true;
            Foe=Foes[0];Foe->SetActorTickEnabled(false);if(auto* Move=Foe->GetMovementComponent()){Move->StopMovementImmediately();Move->SetComponentTickEnabled(false);}
            HpBefore=Combat->GetPlayerHp();FoeHpBefore=Foe->GetHealth();
            GuardKey=EKeys::RightMouseButton;Key(GuardKey,IE_Pressed);GuardAt=World->GetTimeSeconds();Phase=3;return false;
        }
        if(Phase==3)
        {
            if(World->GetTimeSeconds()-GuardAt<MemoriaCombatTuning::ParryWindow+.06f)return false;
            Test->TestTrue(TEXT("Real held block contact harms HP"),Combat->StrikePlayer(Foe.Get(),Foe->Spec().Damage));
            HpLoss=HpBefore-Combat->GetPlayerHp();
            Test->TestEqual(TEXT("Held block actual cost"),HpLoss,FMath::RoundToInt64(Foe->Spec().Damage*MemoriaCombatTuning::BlockFactor));
            Key(GuardKey,IE_Released);Key(GuardKey,IE_Pressed);
            Test->TestFalse(TEXT("Fresh physical guard contact is a zero-loss parry"),Combat->StrikePlayer(Foe.Get(),Foe->Spec().Damage));
            Test->TestEqual(TEXT("Parry does not invent more HP loss"),HpBefore-Combat->GetPlayerHp(),HpLoss);
            Key(GuardKey,IE_Released);
            if(!Test->TestTrue(TEXT("Actual semantic sword attack begins"),Combat->RequestAttack(Foe->GetActorLocation())))return true;
            Phase=4;return false;
        }
        if(Phase==4)
        {
            if(Foe->GetHealth()>=FoeHpBefore)return false;
            Test->TestEqual(TEXT("Actual first blade hit deals12"),FoeHpBefore-Foe->GetHealth(),12.f);
            const auto* Damage=Combat->GetPopups().FindByPredicate([](const FMemoriaCombatPopup& P){return P.Label.IsEmpty()&&!P.bPlayer;});
            if(!Damage || Damage->Age<.1f)return false;
            if(!Test->TestTrue(TEXT("Owned screenshot pause succeeds"),UGameplayStatics::SetGamePaused(PC,true)))return true;
            PausedAge=Damage->Age;Mark=Frame;Phase=5;return false;
        }
        if(Phase==5)
        {
            if(Frame<Mark+2)return false;
            if(!CheckPaint(bReduced,false))return true;
            if(!RequestCapture(false))return true;
            Mark=Frame;Phase=6;return false;
        }
        if(Phase==6)
        {
            if(!bScreenshotProcessed){if(Frame<Mark+300)return false;Test->AddError(TEXT("Concurrent popup PNG timed out"));return true;}
            if(!RecordCapture(*PC,*Pawn,bKo,bReduced,false))return true;
            UGameplayStatics::SetGamePaused(PC,false);
            // Healing, including a same-target increase, must immediately remove
            // the old wound instead of painting a delayed fictional health loss.
            Foe->Heal(6.f);Mark=Frame;Phase=7;return false;
        }
        if(Phase==7)
        {
            if(Frame<Mark+3)return false;
            const auto* H=FindHealth();if(!Test->TestTrue(TEXT("Healing snaps recent damage to actual health"),H && FMath::IsNearlyEqual(H->Current,Foe->GetHealth()) && FMath::IsNearlyEqual(H->Trail,H->Current)))return true;
            // Rebinding resets presentation state even when the authoritative HP
            // value is unchanged, as a restored run can reuse an HP value.
            Foe->TakeHit(3.f,Pawn->GetActorLocation(),0.f);Mark=Frame;Phase=8;return false;
        }
        if(Phase==8)
        {
            if(Frame<Mark+3)return false;
            Hud->Bind(nullptr);Hud->Bind(Combat.Get());Mark=Frame;Phase=9;return false;
        }
        if(Phase==9)
        {
            if(Frame<Mark+3)return false;
            const auto* H=FindHealth();if(!Test->TestTrue(TEXT("Rebind snaps same HP and keeps actual authoritative fill"),H && FMath::IsNearlyEqual(H->Trail,Foe->GetHealth()) && H->Current==Foe->GetHealth()))return true;
            Foe->TakeHit(4.f,Pawn->GetActorLocation(),0.f);TrailAt=World->GetTimeSeconds();Phase=10;return false;
        }
        if(Phase==10)
        {
            if(World->GetTimeSeconds()-TrailAt<.8f)return false;
            const auto* H=FindHealth();if(!Test->TestTrue(TEXT("Short gameplay-time damage trail settles to current HP"),H && FMath::IsNearlyEqual(H->Trail,H->Current)))return true;
            Run->GrantFieldItem(TEXT("smoke_bomb"),1);
            if(!Test->TestTrue(TEXT("Real smoke item emits authored long word and removes the foe"),Combat->UseQuickItem(3,Pawn->GetActorLocation())))return true;
            TrailAt=World->GetTimeSeconds();Phase=11;return false;
        }
        if(Phase==11)
        {
            if(World->GetTimeSeconds()-TrailAt<.1f)return false;
            // Reframe the real camera for an explicit viewport-edge fixture.
            // Authored popup text, world location, amount and age are untouched.
            if(!Test->TestTrue(TEXT("Edge word screenshot pause succeeds"),UGameplayStatics::SetGamePaused(PC,true)))return true;
            EdgeCamera=Pawn->GetFieldCamera();OldCameraTransform=EdgeCamera->GetComponentTransform();bCameraOffset=true;
            const auto* Word=Combat->GetPopups().FindByPredicate([&](const FMemoriaCombatPopup& P){return P.Label.Contains(bKo?TEXT("연기"):TEXT("smoke"));});
            if(!Test->TestNotNull(TEXT("Actual authored smoke popup exists"),Word))return true;
            PausedAge=Word->Age;
            const FVector2D Edge(Desired.X-8.f,Desired.Y*.35f);FVector Ray,Direction;
            if(!Test->TestTrue(TEXT("Real camera deprojects edge fixture"),PC->DeprojectScreenPositionToWorld(Edge.X,Edge.Y,Ray,Direction)))return true;
            const FVector Forward=PC->PlayerCameraManager->GetCameraRotation().Vector();
            const FVector CameraLocation=PC->PlayerCameraManager->GetCameraLocation();
            const float Depth=FVector::DotProduct(Word->Location-CameraLocation,Forward),Denom=FVector::DotProduct(Direction,Forward);
            if(!Test->TestTrue(TEXT("Edge fixture lies in front of camera"),Depth>0.f && Denom>.01f))return true;
            const FVector EdgeWorld=CameraLocation+Direction*(Depth/Denom);
            EdgeCamera->SetWorldLocation(EdgeCamera->GetComponentLocation()+Word->Location-EdgeWorld);
            PC->PlayerCameraManager->UpdateCamera(0.f);
            Mark=Frame;Phase=12;return false;
        }
        if(Phase==12)
        {
            if(Frame<Mark+2)return false;
            if(!CheckPaint(bReduced,true) || !RequestCapture(true))return true;
            Mark=Frame;Phase=13;return false;
        }
        if(Phase==13)
        {
            if(!bScreenshotProcessed){if(Frame<Mark+300)return false;Test->AddError(TEXT("Edge popup PNG timed out"));return true;}
            if(!RecordCapture(*PC,*Pawn,bKo,bReduced,true))return true;
            Test->TestTrue(TEXT("Destroyed foe leaves no stale damage band"),Hud->GetCombatFeedbackPaintProbe().Health.IsEmpty());
            EdgeCamera->SetWorldTransform(OldCameraTransform);bCameraOffset=false;
            UGameplayStatics::SetGamePaused(PC,false);
            if(Test->HasAnyErrors())return true;
            ++Case;Phase=0;
        }
        return false;
    }
private:
    const FMemoriaHealthPaintRecordS365* FindHealth() const
    { return Hud->GetCombatFeedbackPaintProbe().Health.FindByPredicate([&](const FMemoriaHealthPaintRecordS365& H){return H.Target==Foe;}); }
    bool CheckPaint(bool bReduced,bool bEdge)
    {
        const auto& Probe=Hud->GetCombatFeedbackPaintProbe();const bool Enabled=MemoriaCombatFeedbackS365::IsEnabled();
        if(!Test->TestTrue(TEXT("Fresh actual native feedback probe"),Probe.bDrawn && Probe.Frame>=LastFrame-2 && Probe.bEnabled==Enabled))return false;
        int32 Expected=0;bool HasBlock=false,HasParry=false,HasDamage=false,HasLong=false;
        const auto& Source=Combat->GetPopups();
        for(int32 I=0;I<Source.Num();++I)
        {
            const auto& P=Source[I];const float Alpha=FMath::Clamp((P.Label.IsEmpty()?1.4f:2.2f)-P.Age*1.4f,0.f,1.f);
            if(Alpha<=.001f)continue;FVector2D At;
            if(!UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(Controller.Get(),P.Location,At,false))continue;
            ++Expected;const auto* Painted=Probe.Popups.FindByPredicate([&](const FMemoriaPopupPaintRecordS365& R){return R.SourceIndex==I;});
            if(!Test->TestNotNull(TEXT("Every visible source event has a measured painted record"),Painted))return false;
            const FString Text=P.Outcome!=EMemoriaDefenseOutcome::None?MemoriaDefenseFeedbackS364::PopupText(P,Run->GetRunSnapshot().CurrentLocale==TEXT("ko")):P.Label.IsEmpty()?FString::Printf(TEXT("%d"),FMath::RoundToInt(P.Amount)):P.Label;
            Test->TestEqual(TEXT("Popup text truth and amount preserved"),Painted->Text,Text);Test->TestEqual(TEXT("Popup amount preserved"),Painted->Amount,P.Amount);
            Test->TestTrue(TEXT("Popup age alpha preserved"),FMath::IsNearlyEqual(Painted->Alpha,Alpha));
            if(Enabled){Test->TestFalse(TEXT("Controlled concurrent load does not overload layout"),Painted->bOverloaded);Test->TestTrue(TEXT("Measured glyphs plus shadow stay in viewport"),Contains(FBox2D(FVector2D::ZeroVector,Probe.Viewport),Painted->Bounds));}
            HasBlock|=P.Outcome==EMemoriaDefenseOutcome::Block;HasParry|=P.Outcome==EMemoriaDefenseOutcome::Parry;HasDamage|=P.Label.IsEmpty()&&!P.bPlayer;HasLong|=P.Label.Contains(TEXT("smoke"))||P.Label.Contains(TEXT("연기"));
        }
        Test->TestEqual(TEXT("No visible event hidden or coalesced"),Probe.Popups.Num(),Expected);
        if(!bEdge)Test->TestTrue(TEXT("Real block, parry and numeric blade damage coexist"),HasBlock&&HasParry&&HasDamage);
        else Test->TestTrue(TEXT("Authored long smoke word reaches native layout"),HasLong);
        if(Enabled)for(int32 I=0;I<Probe.Popups.Num();++I)for(int32 J=I+1;J<Probe.Popups.Num();++J)
            Test->TestFalse(TEXT("Concurrent actual glyph and shadow rectangles do not overlap"),Probe.Popups[I].Bounds.Intersect(Probe.Popups[J].Bounds));
        if(!bEdge)
        {
            const auto* H=FindHealth();if(!Test->TestNotNull(TEXT("Actual living foe has a measured native HP band"),H))return false;
            Test->TestEqual(TEXT("Authoritative HP fill never delayed"),H->Current,Foe->GetHealth());
            Test->TestEqual(TEXT("Recent damage respects reduced motion and baseline"),H->bTrailVisible,Enabled&&!bReduced);
            if(Enabled&&!bReduced)Test->TestTrue(TEXT("Recent band reveals real lost health while immediate fill is truthful"),H->Trail>H->Current && H->Trail<=FoeHpBefore);
        }
        return !Test->HasAnyErrors();
    }
    bool RequestCapture(bool bEdge)
    {
        if(!Test->TestFalse(TEXT("Own PNG never replaces unrelated capture"),FScreenshotRequest::IsScreenshotRequested()))return false;
        ScreenshotPath=Output/FString::Printf(TEXT("Case%02d_%s.png"),Case,bEdge?TEXT("EdgeWord"):TEXT("Concurrent"));
        bScreenshotProcessed=false;FScreenshotRequest::RequestScreenshot(ScreenshotPath,true,false,false,FIntRect(),true);return true;
    }
    bool RecordCapture(AMemoriaSliceController& PC,AMemoriaFieldPawn& Pawn,bool bKo,bool bReduced,bool bEdge)
    {
        TArray<uint8> Bytes;const uint8 Signature[]={137,80,78,71,13,10,26,10};
        if(!Test->TestTrue(TEXT("Native feedback evidence is actual PNG bytes"),FFileHelper::LoadFileToArray(Bytes,*ScreenshotPath) && Bytes.Num()>24 && FMemory::Memcmp(Bytes.GetData(),Signature,8)==0))return false;
        auto Big=[&](int32 O){return uint32(Bytes[O])<<24|uint32(Bytes[O+1])<<16|uint32(Bytes[O+2])<<8|uint32(Bytes[O+3]);};
        if(!Test->TestEqual(TEXT("Native PNG exact physical dimensions"),FIntPoint(Big(16),Big(20)),Desired))return false;
        FSHAHash Hash;FSHA1::HashBuffer(Bytes.GetData(),Bytes.Num(),Hash.Hash);
        const auto& Probe=Hud->GetCombatFeedbackPaintProbe();auto Item=MakeShared<FJsonObject>();
        Item->SetNumberField(TEXT("case"),Case);Item->SetBoolField(TEXT("edge_fixture"),bEdge);Item->SetStringField(TEXT("locale"),bKo?TEXT("ko"):TEXT("en"));
        Item->SetBoolField(TEXT("reduce_motion"),bReduced);Item->SetNumberField(TEXT("text_percent"),(Case&8)?125:100);
        Item->SetNumberField(TEXT("hp_before"),HpBefore);Item->SetNumberField(TEXT("hp_loss"),HpLoss);Item->SetNumberField(TEXT("enemy_hp"),Foe.IsValid()?Foe->GetHealth():0);
        Item->SetNumberField(TEXT("popup_age"),PausedAge);Item->SetNumberField(TEXT("paint_frame"),double(Probe.Frame));
        Item->SetArrayField(TEXT("player_world"),VectorJson(Pawn.GetActorLocation()));Item->SetArrayField(TEXT("camera_world"),VectorJson(PC.PlayerCameraManager->GetCameraLocation()));
        const auto Rotation=PC.PlayerCameraManager->GetCameraRotation();Item->SetArrayField(TEXT("camera_rotation"),VectorJson(FVector(Rotation.Pitch,Rotation.Yaw,Rotation.Roll)));Item->SetNumberField(TEXT("camera_fov"),PC.PlayerCameraManager->GetFOVAngle());
        TArray<TSharedPtr<FJsonValue>> Entries;
        for(const auto& P:Probe.Popups)
        {
            auto E=MakeShared<FJsonObject>();E->SetNumberField(TEXT("source_index"),P.SourceIndex);E->SetStringField(TEXT("text"),P.Text);E->SetNumberField(TEXT("amount"),P.Amount);E->SetNumberField(TEXT("alpha"),P.Alpha);
            const auto& Source=Combat->GetPopups();
            if(Source.IsValidIndex(P.SourceIndex))E->SetNumberField(TEXT("age"),Source[P.SourceIndex].Age);
            E->SetArrayField(TEXT("desired"),{MakeShared<FJsonValueNumber>(P.DesiredCenter.X),MakeShared<FJsonValueNumber>(P.DesiredCenter.Y)});E->SetArrayField(TEXT("painted"),{MakeShared<FJsonValueNumber>(P.PaintedCenter.X),MakeShared<FJsonValueNumber>(P.PaintedCenter.Y)});
            E->SetArrayField(TEXT("bounds"),{MakeShared<FJsonValueNumber>(P.Bounds.Min.X),MakeShared<FJsonValueNumber>(P.Bounds.Min.Y),MakeShared<FJsonValueNumber>(P.Bounds.Max.X),MakeShared<FJsonValueNumber>(P.Bounds.Max.Y)});E->SetBoolField(TEXT("overload"),P.bOverloaded);Entries.Add(MakeShared<FJsonValueObject>(E));
        }
        Item->SetArrayField(TEXT("popups"),Entries);
        if(const auto* H=FindHealth()){Item->SetNumberField(TEXT("health_current"),H->Current);Item->SetNumberField(TEXT("health_trail"),H->Trail);Item->SetBoolField(TEXT("trail_visible"),H->bTrailVisible);}
        Item->SetStringField(TEXT("png"),ScreenshotPath);Item->SetStringField(TEXT("sha1"),Hash.ToString());Item->SetNumberField(TEXT("width"),Big(16));Item->SetNumberField(TEXT("height"),Big(20));Captures.Add(MakeShared<FJsonValueObject>(Item));return true;
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
    int64 HpBefore=0,HpLoss=0,InkBefore=0;float GuardAt=0.f,PausedAge=0.f,FoeHpBefore=0.f,TrailAt=0.f;
    TWeakObjectPtr<UCameraComponent> EdgeCamera; FTransform OldCameraTransform; bool bCameraOffset=false;
    bool bFixed=false,bOldFixed=false,bReady=false,bOldDriven=true,bOldMaximized=false,bOldPaused=false,bOldReduceMotion=false,bOldReduceFlash=false,bScreenshotProcessed=false;
    FKey GuardKey=EKeys::K;FString OldLocale,OldSettingsLocale,LegacyLabel,Output,ScreenshotPath;TArray<FString> BeforeBurns;
    TArray<TPair<TWeakObjectPtr<AActor>,bool>> HiddenActors;TArray<TSharedPtr<FJsonValue>> Captures;TSharedPtr<FJsonObject> Report;
    TWeakObjectPtr<UMemoriaFieldCombatSubsystem> Combat;TWeakObjectPtr<UMemoriaCombatHudWidget> Hud;TWeakObjectPtr<UMemoriaRunSubsystem> Run;TWeakObjectPtr<UMemoriaSettingsSubsystem> Settings;
    TWeakObjectPtr<UGameViewportClient> Client;TWeakObjectPtr<AMemoriaSliceController> Controller;TWeakObjectPtr<AMemoriaFieldMonster> Foe;
    TWeakPtr<SWindow> SavedWindow;FIntPoint OldSize=FIntPoint::ZeroValue,Desired=FIntPoint::ZeroValue;EWindowMode::Type OldMode=EWindowMode::Windowed;FVector2D OldPosition,OldWindowSize,OldWindowViewport;FDelegateHandle ScreenshotHandle;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFeedbackS365Native,"MemoriaVisual.S365Feedback.ConcurrentPopupsAndDamageTrail",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFeedbackS365Native::RunTest(const FString&)
{
    MemoriaCombatFeedbackS365::FHealthTrail Trail;Trail.Observe(100,100,0,false);Trail.Observe(80,100,.01f,false);
    TestEqual(TEXT("Observed real value immediate while trail holds previous HP"),Trail.Last,80.f);TestEqual(TEXT("Recent band retains preceding HP"),Trail.Value,100.f);
    Trail.Observe(80,100,0,false);TestEqual(TEXT("Paused gameplay delta does not drain recent damage"),Trail.Value,100.f);
    Trail.Observe(80,100,.8f,false);TestEqual(TEXT("Bounded trail catches up"),Trail.Value,80.f);
    Trail.Observe(70,100,.01f,false);Trail.Observe(75,100,.01f,false);TestEqual(TEXT("Healing snaps"),Trail.Value,75.f);
    Trail.Observe(75,120,.01f,false);TestEqual(TEXT("Max-health change snaps"),Trail.Value,75.f);
    Trail.Observe(50,120,.01f,false);Trail.Observe(50,120,0,true);TestEqual(TEXT("Reduced motion snaps same HP"),Trail.Value,50.f);
    Trail.Observe(0,120,.01f,false);TestEqual(TEXT("Death clears trail"),Trail.Value,0.f);
    if(!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice")))return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FFeedbackS365Replay(this)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());return true;
}
#endif
