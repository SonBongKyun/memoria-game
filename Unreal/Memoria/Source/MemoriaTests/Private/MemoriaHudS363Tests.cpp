#include "Misc/AutomationTest.h"
#include "CoreGlobals.h"
#include "Tests/AutomationEditorCommon.h"
#include "Tests/AutomationCommon.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Blueprint/GameViewportSubsystem.h"
#include "Slate/SGameLayerManager.h"
#include "Slate/SceneViewport.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SWindow.h"
#include "Widgets/SViewport.h"
#include "Widgets/Layout/SConstraintCanvas.h"
#include "Rendering/SlateRenderer.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/MemoriaSliceHost.h"
#include "Framework/MemoriaFieldPawn.h"
#include "Framework/MemoriaCombatInputRouter.h"
#include "Combat/MemoriaFieldCombatSubsystem.h"
#include "Combat/MemoriaCombatHudWidget.h"
#include "Combat/MemoriaFieldHudWidget.h"
#include "Narrative/MemoriaNarrativeSubsystem.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Domain/MemoriaPlayerMemoryDomain.h"
#include "Settings/MemoriaSettingsSubsystem.h"
#include "Achievements/MemoriaAchievementSubsystem.h"
#include "Achievements/MemoriaAchievementWidgets.h"
#include "Presentation/MemoriaPauseWidget.h"
#include "InputKeyEventArgs.h"
#include "UObject/UObjectIterator.h"
#include "Misc/App.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
struct FHudOuterHost { const SWidget* Overlay = nullptr; int32 Z = 0; };
bool HudOuterHost(UGameViewportClient* Client, const UWidget* Widget, FHudOuterHost& Out)
{
    Out = {};
    if (!Client || !Widget) return false;
    auto Manager = Client->GetGameLayerManager();
    TSharedPtr<SWidget> Node = Widget->GetCachedWidget();
    if (!Manager || !Node) return false;
    auto Root = Manager->AsWidget();
    for (int32 Depth = 0; Node && Depth < 64; ++Depth)
    {
        if (Node == Root) return Out.Overlay != nullptr;
        auto Parent = Node->GetParentWidget(); if (!Parent) break;
        if (!Out.Overlay && Parent->GetWidgetClass().GetWidgetType() == SOverlay::StaticWidgetClass().GetWidgetType() &&
            Node->GetWidgetClass().GetWidgetType() == SConstraintCanvas::StaticWidgetClass().GetWidgetType())
        {
            auto* Children = Parent->GetChildren();
            for (int32 I = 0; I < Children->Num(); ++I) if (Children->GetChildAt(I) == Node)
            {
                Out.Overlay = Parent.Get(); Out.Z = static_cast<const SOverlay::FOverlaySlot&>(Children->GetSlotAt(I)).GetZOrder(); break;
            }
        }
        Node = Parent;
    }
    Out = {}; return false;
}
bool ContainsRect(const FBox2D& Outer, const FBox2D& Inner)
{
    return Outer.bIsValid && Inner.bIsValid && Inner.Min.X >= Outer.Min.X - .05 && Inner.Min.Y >= Outer.Min.Y - .05 &&
        Inner.Max.X <= Outer.Max.X + .05 && Inner.Max.Y <= Outer.Max.Y + .05;
}
class FHudS363Replay final : public IAutomationLatentCommand
{
public:
    explicit FHudS363Replay(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()),
        OldScreenshotResolutionX(GScreenshotResolutionX), OldScreenshotResolutionY(GScreenshotResolutionY) {}
    ~FHudS363Replay() override
    {
        if (Combat.IsValid()) { Combat->OnBurnPickerVisibilityChanged.Remove(PickerHandle); Combat->CloseBurnPicker(); }
        FScreenshotRequest::OnScreenshotRequestProcessed().Remove(ScreenshotHandle);
        if (Settings.IsValid()) { Settings->SetTextPercent(OldText); Settings->SetLocale(OldSettingsLocale); }
        if (Run.IsValid()) Run->SetLocale(OldLocale);
        if (Client.IsValid() && Client->Viewport && OldSize.X > 0)
        {
            static_cast<FSceneViewport*>(Client->Viewport)->SetViewportSize(OldSize.X, OldSize.Y);
            if (auto Window = SavedWindow.Pin())
            {
                Window->SetWindowMode(OldMode); Window->ReshapeWindow(OldPosition, OldWindowSize);
                Window->SetViewportSizeDrivenByWindow(bOldDriven);
                if (!bOldDriven) Window->SetIndependentViewportSize(OldWindowViewport);
                if (bOldMaximized) Window->Maximize();
                auto Extent = Window->GetViewportSize();
                FSlateApplication::Get().GetRenderer()->UpdateFullscreenState(Window.ToSharedRef(), FMath::RoundToInt(Extent.X), FMath::RoundToInt(Extent.Y));
            }
        }
        if (bFixed) { FApp::SetUseFixedTimeStep(bOldFixed); FApp::SetFixedDeltaTime(OldDelta); }
        // UI screenshots update these globals; Reset does not restore their prior dimensions.
        GScreenshotResolutionX = OldScreenshotResolutionX; GScreenshotResolutionY = OldScreenshotResolutionY;
    }
    bool Update() override
    {
        if (LastFrame == GFrameCounter) return false; LastFrame = GFrameCounter;
        if (FPlatformTime::Seconds() - Started > 240.) { Test->AddError(FString::Printf(TEXT("S363 HUD timeout at case%d phase%d"), CaseIndex, Phase)); return true; }
        UWorld* World = GEditor ? GEditor->PlayWorld.Get() : nullptr;
        auto* PC = World && !World->bIsTearingDown ? Cast<AMemoriaSliceController>(World->GetFirstPlayerController()) : nullptr;
        auto* Pawn = PC ? Cast<AMemoriaFieldPawn>(PC->GetPawn()) : nullptr;
        if (!Pawn || World->GetTimeSeconds() < .3) return false;
        auto* Narrative = World->GetGameInstance()->GetSubsystem<UMemoriaNarrativeSubsystem>();
        if (!bFixed)
        {
            bFixed = true; bOldFixed = FApp::UseFixedTimeStep(); OldDelta = FApp::GetFixedDeltaTime();
            FApp::SetUseFixedTimeStep(true); FApp::SetFixedDeltaTime(1. / 60.);
            for (int32 I = 0; I < 20 && Narrative->GetState() == EMemoriaSliceState::VN; ++I) Narrative->Confirm(1);
            return false;
        }
        if (!World->GetMapName().EndsWith(TEXT("L_VerdanHost")) || Narrative->GetState() != EMemoriaSliceState::Exploration) return false;
        ++Frame;
        if (!bReady)
        {
            Combat = World->GetSubsystem<UMemoriaFieldCombatSubsystem>(); Run = World->GetGameInstance()->GetSubsystem<UMemoriaRunSubsystem>();
            Settings = World->GetGameInstance()->GetSubsystem<UMemoriaSettingsSubsystem>(); Client = World->GetGameViewport(); Controller = PC;
            if (!Combat.IsValid() || !Run.IsValid() || !Settings.IsValid() || !Client.IsValid() || !Client->Viewport) return false;
            for (TObjectIterator<UMemoriaCombatHudWidget> It; It; ++It) if (It->GetOwningPlayer() == PC && It->IsInViewport()) { Hud = *It; break; }
            if (!Hud.IsValid() || !PC->GetFieldHud()) return false;
            OldText = Settings->GetTextPercent(); OldLocale = Run->GetRunSnapshot().CurrentLocale; OldSettingsLocale = Settings->GetLocale();
            OldSize = Client->Viewport->GetSizeXY(); auto Window = Client->GetWindow();
            if (!Test->TestTrue(TEXT("Real HUD resize window exists"), Window.IsValid())) return true;
            SavedWindow = Window; OldMode = Window->GetWindowMode(); OldPosition = Window->GetPositionInScreen(); OldWindowSize = Window->GetSizeInScreen();
            OldWindowViewport = Window->GetViewportSize(); bOldDriven = Window->IsViewportSizeDrivenByWindow(); bOldMaximized = Window->IsWindowMaximized();
            ScreenshotHandle = FScreenshotRequest::OnScreenshotRequestProcessed().AddRaw(this, &FHudS363Replay::ScreenshotProcessed);
            // Record the authoritative event and request its first paint. Delegate listener order
            // is unspecified: layer assertions happen after the real input/broadcast returns.
            // This observer never calls Bind/Sync or changes a widget's placement itself.
            PickerHandle = Combat->OnBurnPickerVisibilityChanged.AddRaw(this, &FHudS363Replay::PickerChanged);
            BeforeBurns = Run->GetPlayerMemory()->GetSnapshot().BurnedHistory;
            for (int32 I = 0; I < 5; ++I) BeforeItems.Add(Combat->QuickItemCount(I));
            bReady = true;
        }
        if (CaseIndex >= 16)
        {
            Test->TestEqual(TEXT("Every real opening notified the first-paint observer once"), OpenEvents, 16);
            Test->TestEqual(TEXT("Every actual cancellation restored the viewport once"), CloseEvents, 16);
            Test->TestTrue(TEXT("Presentation loops never burned an authored memory"), Run->GetPlayerMemory()->GetSnapshot().BurnedHistory == BeforeBurns);
            for (int32 I = 0; I < 5; ++I) Test->TestEqual(TEXT("Presentation loops never spent a quick item"), Combat->QuickItemCount(I), BeforeItems[I]);
            return true;
        }
        const bool bKo = (CaseIndex & 1) != 0, bSmall = (CaseIndex & 2) != 0, bPad = (CaseIndex & 4) != 0;
        Desired = bSmall ? FIntPoint(640, 360) : FIntPoint(1280, 720);
        const int32 TextPercent = (CaseIndex & 8) != 0 ? 125 : 100;
        auto Key = [&](FKey K, EInputEvent E) { PC->InputKey(FInputKeyEventArgs::CreateSimulated(K, E, E == IE_Released ? 0.f : 1.f)); };
        auto Tap = [&](FKey K) { Key(K, IE_Pressed); Key(K, IE_Released); };
        if (Phase == 0)
        {
            Combat->ResetForRestoredRun(); Combat->Revive(1.f);
            Settings->SetTextPercent(TextPercent);
            Settings->SetLocale(bKo ? TEXT("en") : TEXT("ko")); Run->SetLocale(bKo ? TEXT("en") : TEXT("ko"));
            // The real language option updates the run and invalidates the status presentation.
            // Direct test Display/Bind calls would miss that actual locale lifecycle.
            Tap(EKeys::Escape);
            if (!Test->TestNotNull(TEXT("Real Pause exists for the locale option"), PC->GetPauseWidget())) return true;
            Tap(EKeys::Down); Tap(EKeys::Enter);
            if (!Test->TestTrue(TEXT("Real Options opens before language navigation"), PC->GetPauseWidget()->IsOptionsOpen())) return true;
            for (int32 I = 0; I < 4; ++I) Tap(EKeys::Down);
            Tap(EKeys::Enter); Tap(EKeys::Escape); Tap(EKeys::Escape);
            if (!Test->TestTrue(TEXT("Language option reaches the selected run locale and resumes field ownership"),
                Run->GetRunSnapshot().CurrentLocale == (bKo ? TEXT("ko") : TEXT("en")) && !PC->GetPauseWidget() && !World->IsPaused())) return true;
            auto Window = SavedWindow.Pin(); Window->SetWindowMode(EWindowMode::Windowed);
            if (Window->IsWindowMaximized()) Window->Restore(); Window->Resize(FVector2D(Desired)); MatchSceneTarget();
            ResizeAttempts = 0; Mark = Frame; Phase = 10; return false;
        }
        if (Phase == 10)
        {
            if (Frame < Mark + 12) return false;
            auto ViewportWidget = Client->GetGameViewportWidget(); if (!ViewportWidget.IsValid()) return false;
            const FVector2D Absolute = ViewportWidget->GetPaintSpaceGeometry().GetAbsoluteSize();
            const FIntPoint Pixels(FMath::RoundToInt(FMath::Abs(Absolute.X)), FMath::RoundToInt(FMath::Abs(Absolute.Y)));
            if (Pixels != Desired && ResizeAttempts++ < 3)
            {
                auto Window = SavedWindow.Pin(); Window->Resize(FVector2D(Window->GetClientSizeInScreen()) + FVector2D(Desired - Pixels));
                MatchSceneTarget(); Mark = Frame; return false;
            }
            if (!Test->TestEqual(TEXT("Actual Slate HUD client has requested physical pixels"), Pixels, Desired) ||
                !Test->TestEqual(TEXT("Actual HUD scene target has requested physical pixels"), Client->Viewport->GetRenderTargetTextureSizeXY(), Desired)) return true;
            Tap(bPad ? EKeys::Gamepad_RightThumbstick : EKeys::MiddleMouseButton);
            Mark = Frame; Phase = 1; return false;
        }
        if (Phase == 1)
        {
            if (Frame < Mark + 8) return false;
            const auto& Tray = PC->GetFieldHud()->GetQuickTrayPaintProbe(); const auto& Ribbon = Hud->GetRibbonPaintProbe();
            Test->TestTrue(TEXT("Idle quick tray has a fresh real native paint"), Tray.bDrawn && Tray.Frame >= LastFrame - 2 && Tray.Cells.Num() == 5);
            Test->TestFalse(TEXT("Unwounded idle field does not paint the combat ribbon"), Ribbon.bDrawn);
            Test->TestTrue(TEXT("Idle field retains the original bottom-right tray position"), !Tray.bLifted && Tray.Bounds.Min.Equals(Tray.OriginalBounds.Min, .01) && Tray.Bounds.Max.Equals(Tray.OriginalBounds.Max, .01));
            if (!CheckTray(Tray, bKo, bPad)) return true;
            Capture(TEXT("Idle")); Phase = 2; return false;
        }
        if (Phase == 2)
        {
            if (!CaptureDone()) return false;
            const auto Foes = Combat->SpawnWave(1, Pawn->GetActorLocation() + FVector(2000, 0, 0), 0.f, EMemoriaFoeKind::VoidHusk);
            if (!Test->TestEqual(TEXT("Actual distant field combat activates the live command ribbon"), Foes.Num(), 1)) return true;
            Mark = Frame; Phase = 3; return false;
        }
        if (Phase == 3)
        {
            if (Frame < Mark + 8) return false;
            const auto& Tray = PC->GetFieldHud()->GetQuickTrayPaintProbe(); const auto& Ribbon = Hud->GetRibbonPaintProbe();
            if (!Test->TestTrue(TEXT("Both actual native HUD owners painted live combat"), Tray.bDrawn && Ribbon.bDrawn && Tray.Frame >= LastFrame - 2 && Ribbon.Frame >= LastFrame - 2)) return true;
            Test->TestEqual(TEXT("Seven action cells remain painted"), Ribbon.ActionCells.Num(), 7);
            Test->TestEqual(TEXT("Both owners follow the same real input preference"), Ribbon.bUsingGamepad, bPad);
            Test->TestFalse(TEXT("Tray does not cover the memory orb"), Tray.Bounds.Intersect(Ribbon.MemoryOrbBounds));
            for (const auto& Cell : Ribbon.ActionCells) Test->TestFalse(TEXT("Tray leaves every action cell unobscured"), Tray.Bounds.Intersect(Cell));
            if (Tray.OriginalBounds.Intersect(Ribbon.OccupiedBounds))
                Test->TestTrue(TEXT("Intersecting tray rises with at least twelve Slate units clearance"), Tray.bLifted && Tray.Bounds.Max.Y + 12.f <= Ribbon.OccupiedBounds.Min.Y + .05);
            else Test->TestFalse(TEXT("A tray without overlap is not moved"), Tray.bLifted);
            Test->TestTrue(TEXT("Tray preserves width, height and horizontal position"), Tray.Bounds.GetSize().Equals(Tray.OriginalBounds.GetSize(), .01) && FMath::IsNearlyEqual(Tray.Bounds.Min.X, Tray.OriginalBounds.Min.X, .01));
            if (!CheckTray(Tray, bKo, bPad)) return true;
            Test->AddInfo(FString::Printf(TEXT("S363_HUD_CASE case=%d ko=%d pad=%d text=%d px=%dx%d local=%.2fx%.2f tray=%s ribbon=%s lifted=%d"),
                CaseIndex, bKo, bPad, TextPercent, Desired.X, Desired.Y, Tray.Viewport.X, Tray.Viewport.Y, *Tray.Bounds.ToString(), *Ribbon.OccupiedBounds.ToString(), int32(Tray.bLifted)));
            Capture(TEXT("Combat")); Phase = 4; return false;
        }
        if (Phase == 4)
        {
            if (!CaptureDone()) return false;
            bCaptureOpening = true; bOpenObserved = false; bCloseObserved = false;
            Key(bPad ? EKeys::Gamepad_FaceButton_Top : EKeys::R, IE_Pressed);
            // Keyboard opens synchronously. Enhanced gamepad callbacks run in the actual next input tick.
            if (!bPad)
            {
                Test->TestTrue(TEXT("Keyboard opening returned only after the authoritative opening event"), bOpenObserved);
                CheckOuterOrder(true);
                Test->AddInfo(FString::Printf(TEXT("S363_PICKER_SYNC_BEFORE_INPUT_RETURNS case=%d frame=%llu"), CaseIndex, GFrameCounter));
            }
            Phase = 5; return false;
        }
        if (Phase == 5)
        {
            if (!bOpenObserved || !Combat->IsPickingBurn()) return false;
            CheckOuterOrder(true);
            if (!CaptureDone()) return false;
            Test->TestFalse(TEXT("Field quick tray hides while the picker owns the screen"), PC->GetFieldHud()->GetQuickTrayPaintProbe().bDrawn);
            Key(bPad ? EKeys::Gamepad_FaceButton_Top : EKeys::R, IE_Released);
            Tap(EKeys::Escape);
            Test->TestTrue(TEXT("Physical cancellation restores actual normal layer before returning"), bCloseObserved && !Combat->IsPickingBurn());
            CheckOuterOrder(false);
            Mark = Frame; Phase = 6; return false;
        }
        if (Phase == 6 && Frame >= Mark + 8)
        {
            FHudOuterHost Host;
            Test->TestTrue(TEXT("Normal HUD remains at outer order14 after repeated open/close/reset"), HudOuterHost(Client.Get(), Hud.Get(), Host) && Host.Z == 14);
            if (Test->HasAnyErrors()) return true;
            ++CaseIndex; Phase = 0;
        }
        return false;
    }
private:
    bool CheckTray(const FMemoriaQuickTrayPaintProbe& Tray, bool bKo, bool bPad)
    {
        static const TCHAR* Keys[] = {TEXT("Z"), TEXT("X"), TEXT("C"), TEXT("V"), TEXT("B")};
        static const TCHAR* PadKeys[] = {TEXT("RT+↑"), TEXT("RT+→"), TEXT("RT+↓"), TEXT("RT+←"), TEXT("RT+R3")};
        Test->TestTrue(TEXT("All five native item cells and labels are present"), Tray.Cells.Num() == 5 && Tray.Keys.Num() == 5 && Tray.Names.Num() == 5 && Tray.KeyBounds.Num() == 5 && Tray.NameBounds.Num() == 5);
        if (Tray.Cells.Num() != 5 || Tray.Keys.Num() != 5 || Tray.KeyBounds.Num() != 5 || Tray.NameBounds.Num() != 5) return false;
        Test->TestEqual(TEXT("Quick tray reflects the physical device, not a forced test flag"), Tray.bUsingGamepad, bPad);
        Test->TestTrue(TEXT("Quick tray stays inside the actual native viewport"), ContainsRect(FBox2D(FVector2D::ZeroVector, Tray.Viewport), Tray.Bounds));
        for (int32 I = 0; I < 5; ++I)
        {
            Test->TestEqual(TEXT("Actual item cap retains its binding"), Tray.Keys[I], FString(bPad ? PadKeys[I] : Keys[I]));
            Test->TestEqual(TEXT("Actual item name uses the selected locale"), Tray.Names[I], MemoriaCombatTuning::ItemName(Combat->QuickItemId(I), bKo));
            Test->TestTrue(TEXT("Measured native key text remains inside its item cell"), ContainsRect(Tray.Cells[I], Tray.KeyBounds[I]));
            Test->TestTrue(TEXT("Measured native localized item label remains inside its item cell"), ContainsRect(Tray.Cells[I], Tray.NameBounds[I]));
        }
        return !Test->HasAnyErrors();
    }
    void PickerChanged()
    {
        if (!Combat.IsValid() || !Hud.IsValid() || !Client.IsValid()) return;
        if (Combat->IsPickingBurn())
        {
            ++OpenEvents; bOpenObserved = true;
            Test->AddInfo(FString::Printf(TEXT("S363_PICKER_OPEN_EVENT eventFrame=%llu previousHudPaint=%llu"), GFrameCounter, Hud->GetRibbonPaintProbe().Frame));
            if (bCaptureOpening) { Capture(TEXT("PickerFirstPaint")); bCaptureOpening = false; }
        }
        else
        {
            ++CloseEvents; bCloseObserved = true;
        }
    }
    void CheckOuterOrder(bool bPicker)
    {
        FHudOuterHost Host, Popup;
        const bool bHost = HudOuterHost(Client.Get(), Hud.Get(), Host);
        Test->TestTrue(TEXT("Actual HUD outer Overlay order matches the completed state transition"), bHost && Host.Z == (bPicker ? 110 : 14));
        Test->TestEqual(TEXT("Actual viewport slot matches the completed state transition"), UGameViewportSubsystem::Get()->GetWidgetSlot(Hud.Get()).ZOrder, bPicker ? 100 : 4);
        if (bPicker)
        {
            auto* PC = Controller.Get();
            Test->TestTrue(TEXT("Existing real achievement popup stays below the picker"), PC && PC->GetAchievementPopup() && HudOuterHost(Client.Get(), PC->GetAchievementPopup(), Popup) && Popup.Overlay == Host.Overlay && Popup.Z == 105 && Host.Z > Popup.Z);
        }
    }
    void MatchSceneTarget()
    {
        auto Window = SavedWindow.Pin(); static_cast<FSceneViewport*>(Client->Viewport)->SetViewportSize(Desired.X, Desired.Y);
        auto Extent = Window->GetSizeInScreen(); Window->SetIndependentViewportSize(Extent);
        FSlateApplication::Get().GetRenderer()->UpdateFullscreenState(Window.ToSharedRef(), FMath::RoundToInt(Extent.X), FMath::RoundToInt(Extent.Y));
    }
    void Capture(const TCHAR* State)
    {
        ScreenshotPath = FPaths::ProjectSavedDir() / TEXT("Validation/S363Hud") / FString::Printf(TEXT("Case%02d_%s.png"), CaseIndex, State);
        bScreenshotProcessed = false;
        FScreenshotRequest::RequestScreenshot(ScreenshotPath, true, false, false, FIntRect(), true);
    }
    bool CaptureDone()
    {
        if (!bScreenshotProcessed) return false;
        TArray<uint8> Bytes; if (!Test->TestTrue(TEXT("Requested native HUD PNG is readable"), FFileHelper::LoadFileToArray(Bytes, *ScreenshotPath) && Bytes.Num() >= 24)) return true;
        auto BigEndian = [&](int32 At) { return (uint32(Bytes[At]) << 24) | (uint32(Bytes[At + 1]) << 16) | (uint32(Bytes[At + 2]) << 8) | uint32(Bytes[At + 3]); };
        Test->TestTrue(TEXT("Saved HUD capture is a PNG with the requested physical dimensions"), Bytes[0] == 137 && Bytes[1] == 80 && BigEndian(16) == uint32(Desired.X) && BigEndian(20) == uint32(Desired.Y));
        return true;
    }
    void ScreenshotProcessed() { bScreenshotProcessed = true; }
    FAutomationTestBase* Test;
    double Started, OldDelta = 0.; uint64 LastFrame = MAX_uint64;
    uint32 OldScreenshotResolutionX, OldScreenshotResolutionY;
    int32 Frame = 0, Mark = 0, Phase = 0, CaseIndex = 0, ResizeAttempts = 0, OldText = 100, OpenEvents = 0, CloseEvents = 0;
    bool bFixed = false, bOldFixed = false, bReady = false, bOldDriven = true, bOldMaximized = false;
    bool bScreenshotProcessed = false, bCaptureOpening = false, bOpenObserved = false, bCloseObserved = false;
    FString OldLocale, OldSettingsLocale, ScreenshotPath;
    TArray<FString> BeforeBurns; TArray<int64> BeforeItems;
    TWeakObjectPtr<UMemoriaFieldCombatSubsystem> Combat; TWeakObjectPtr<UMemoriaCombatHudWidget> Hud;
    TWeakObjectPtr<UMemoriaRunSubsystem> Run; TWeakObjectPtr<UMemoriaSettingsSubsystem> Settings;
    TWeakObjectPtr<UGameViewportClient> Client; TWeakObjectPtr<AMemoriaSliceController> Controller;
    TWeakPtr<SWindow> SavedWindow; FIntPoint OldSize = FIntPoint::ZeroValue, Desired = FIntPoint::ZeroValue;
    EWindowMode::Type OldMode = EWindowMode::Windowed;
    FVector2D OldPosition, OldWindowSize, OldWindowViewport;
    FDelegateHandle PickerHandle, ScreenshotHandle;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHudS363Native, "MemoriaVisual.S363.HudLayoutAndFirstPaint", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHudS363Native::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice"))) return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FHudS363Replay(this)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand()); return true;
}
#endif
