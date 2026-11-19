#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "Tests/AutomationCommon.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Blueprint/GameViewportSubsystem.h"
#include "Slate/SGameLayerManager.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SConstraintCanvas.h"
#include "Achievements/MemoriaAchievementSubsystem.h"
#include "Achievements/MemoriaAchievementWidgets.h"
#include "Slate/SceneViewport.h"
#include "Widgets/SWindow.h"
#include "Widgets/SViewport.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"
#include "Framework/MemoriaSliceHost.h"
#include "Framework/MemoriaFieldPawn.h"
#include "Narrative/MemoriaNarrativeSubsystem.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Domain/MemoriaChapterMemories.h"
#include "Combat/MemoriaFieldCombatSubsystem.h"
#include "Combat/MemoriaCombatHudWidget.h"
#include "Settings/MemoriaSettingsSubsystem.h"
#include "UObject/UObjectIterator.h"
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "UnrealClient.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
struct FActualOverlayHost
{
    const SWidget* Overlay = nullptr;
    const SWidget* Host = nullptr;
    int32 Index = INDEX_NONE, Z = 0;
};
bool ActualHost(UGameViewportClient* Client, const UWidget* Widget, FActualOverlayHost& Out)
{
    Out = {};
    if (!Client || !Widget) return false;
    auto Manager = Client->GetGameLayerManager();
    TSharedPtr<SWidget> Node = Widget->GetCachedWidget();
    if (!Manager || !Node) return false;
    TSharedPtr<SWidget> Root = Manager->AsWidget();
    for (int32 Depth = 0; Node && Depth < 64; ++Depth)
    {
        if (Node == Root) return Out.Overlay != nullptr;
        auto Parent = Node->GetParentWidget();
        if (!Parent) break;
        if (!Out.Overlay && Parent->GetWidgetClass().GetWidgetType() == SOverlay::StaticWidgetClass().GetWidgetType()
            && Node->GetWidgetClass().GetWidgetType() == SConstraintCanvas::StaticWidgetClass().GetWidgetType())
        {
            FChildren* Children = Parent->GetChildren();
            for (int32 I = 0; I < Children->Num(); ++I)
            {
                TSharedPtr<SWidget> Child = Children->GetChildAt(I);
                if (Child == Node)
                {
                    Out.Overlay = Parent.Get(); Out.Host = Node.Get(); Out.Index = I;
                    Out.Z = static_cast<const SOverlay::FOverlaySlot&>(Children->GetSlotAt(I)).GetZOrder(); break;
                }
            }
        }
        Node = Parent;
    }
    Out = {}; return false;
}
FString WithoutWhitespace(const FString& Text)
{ FString Out; for (TCHAR C : Text) if (!FChar::IsWhitespace(C)) Out.AppendChar(C); return Out; }
class FPickerReadableReplay final : public IAutomationLatentCommand
{
public:
    explicit FPickerReadableReplay(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
    ~FPickerReadableReplay() override
    {
        // Cleanup is shared by success, any assertion failure, and the timeout path. Only this PIE
        // client's viewport is resized; global renderer/cvars and the editor viewport are untouched.
        if (Combat.IsValid()) Combat->CloseBurnPicker();
        if (bReady && Settings.IsValid()) Settings->SetTextPercent(OldTextPercent);
        if (bReady && Run.IsValid()) Run->SetLocale(OldLocale);
        FScreenshotRequest::OnScreenshotRequestProcessed().Remove(ScreenshotHandle);
        if (bWindowCaptured && Client.IsValid() && Client->Viewport && OldSize.X > 0 && OldSize.Y > 0)
        {
            static_cast<FSceneViewport*>(Client->Viewport)->SetViewportSize(OldSize.X, OldSize.Y);
            if (const auto Window = SavedWindow.Pin(); Window.IsValid())
            {
                Window->SetWindowMode(OldMode);
                Window->ReshapeWindow(OldWindowPosition, OldWindowSize);
                Window->SetViewportSizeDrivenByWindow(bOldViewportDrivenByWindow);
                if (!bOldViewportDrivenByWindow) Window->SetIndependentViewportSize(OldWindowViewportSize);
                if (bOldMaximized) Window->Maximize();
                const FVector2D RestoredBuffer = Window->GetViewportSize();
                FSlateApplication::Get().GetRenderer()->UpdateFullscreenState(Window.ToSharedRef(), FMath::RoundToInt(RestoredBuffer.X), FMath::RoundToInt(RestoredBuffer.Y));
            }
        }
        if (bFixed) { FApp::SetUseFixedTimeStep(bOldFixed); FApp::SetFixedDeltaTime(OldDelta); }
    }
    bool Update() override
    {
        if (LastFrame == GFrameCounter) return false; LastFrame = GFrameCounter;
        if (FPlatformTime::Seconds() - Started > 180.) { Test->AddError(FString::Printf(TEXT("S362 picker readability timeout case %d phase %d"), CaseIndex, Phase)); return true; }
        UWorld* World = GEditor->PlayWorld;
        auto* PC = World ? Cast<AMemoriaSliceController>(World->GetFirstPlayerController()) : nullptr;
        auto* Pawn = PC ? Cast<AMemoriaFieldPawn>(PC->GetPawn()) : nullptr;
        if (!Pawn || World->GetTimeSeconds() < .3) return false;
        auto* Host = World->GetGameInstance()->GetSubsystem<UMemoriaNarrativeSubsystem>();
        if (!bFixed)
        {
            bFixed = true; bOldFixed = FApp::UseFixedTimeStep(); OldDelta = FApp::GetFixedDeltaTime();
            FApp::SetUseFixedTimeStep(true); FApp::SetFixedDeltaTime(1. / 60.);
            for (int32 I = 0; I < 20 && Host->GetState() == EMemoriaSliceState::VN; ++I) Host->Confirm(1);
            return false;
        }
        if (!World->GetMapName().EndsWith(TEXT("L_VerdanHost")) || Host->GetState() != EMemoriaSliceState::Exploration) return false;
        ++Frame;
        if (!bReady)
        {
            Combat = World->GetSubsystem<UMemoriaFieldCombatSubsystem>();
            Run = World->GetGameInstance()->GetSubsystem<UMemoriaRunSubsystem>();
            Settings = World->GetGameInstance()->GetSubsystem<UMemoriaSettingsSubsystem>();
            Client = World->GetGameViewport();
            if (!Combat.IsValid() || !Run.IsValid() || !Settings.IsValid() || !Client.IsValid() || !Client->Viewport) return false;
            OldTextPercent = Settings->GetTextPercent(); OldLocale = Run->GetRunSnapshot().CurrentLocale;
            OldSize = Client->Viewport->GetSizeXY();
            const auto Window = Client->GetWindow();
            if (!Test->TestTrue(TEXT("Responsive fixture has a real PIE client window"), Window.IsValid())) return true;
            SavedWindow = Window; OldMode = Window->GetWindowMode();
            OldWindowPosition = Window->GetPositionInScreen(); OldWindowSize = Window->GetSizeInScreen();
            OldWindowViewportSize = Window->GetViewportSize(); bOldViewportDrivenByWindow = Window->IsViewportSizeDrivenByWindow();
            bOldMaximized = Window->IsWindowMaximized(); bWindowCaptured = true;
            ScreenshotHandle = FScreenshotRequest::OnScreenshotRequestProcessed().AddRaw(this, &FPickerReadableReplay::OnScreenshotProcessed);
            for (int64 Chapter = 3; Chapter <= 5; ++Chapter)
                for (const auto& Definition : MemoriaChapterMemories::For(Chapter))
                    if (!Run->GetPlayerMemory()->GetDefinitions().ContainsByPredicate([&](const auto& D) { return D.Id == Definition.Id; }))
                        Test->TestEqual(TEXT("Use an authored overflow inventory"), Run->AcquireMemory(Definition), EMemoriaMemoryResult::Success);
            Settings->SetTextPercent(125); bReady = true;
        }
        if (CaseIndex >= 8) return true;
        const bool bKo = CaseIndex >= 4, bSmall = (CaseIndex / 2) % 2 == 1, bCore = CaseIndex % 2 == 1;
        const FIntPoint Desired = bSmall ? FIntPoint(640, 360) : FIntPoint(1280, 720);
        if (Phase == 0)
        {
            Combat->CloseBurnPicker(); Run->SetLocale(bKo ? TEXT("ko") : TEXT("en"));
            Before = Run->GetPlayerMemory()->GetSnapshot();
            // ResizeFrame is game-only and does nothing in Editor PIE. Resize this client's
            // Slate window and independent backbuffer instead; no global renderer setting changes.
            const auto Window = SavedWindow.Pin();
            Window->SetWindowMode(EWindowMode::Windowed);
            if (Window->IsWindowMaximized()) Window->Restore();
            Window->Resize(FVector2D(Desired));
            MatchSceneTargetToClient(Desired);
            ResizeAttempts = 0; Mark = Frame; Phase = 10; return false;
        }
        if (Phase == 10)
        {
            if (Frame < Mark + 12) return false;
            const auto ViewportWidget = Client->GetGameViewportWidget();
            if (!Test->TestTrue(TEXT("Responsive case retains the actual PIE viewport widget"), ViewportWidget.IsValid())) return true;
            // Paint-space render size includes render transforms; legacy GetDrawSize only
            // applies layout transforms and is not a physical-pixel measurement.
            const auto& PaintedGeometry = ViewportWidget->GetPaintSpaceGeometry();
            const FVector2D AbsoluteSize = PaintedGeometry.GetAbsoluteSize();
            const FVector2D DrawSize(FMath::Abs(AbsoluteSize.X), FMath::Abs(AbsoluteSize.Y));
            const FIntPoint DrawPixels(FMath::RoundToInt(DrawSize.X), FMath::RoundToInt(DrawSize.Y));
            const auto Window = SavedWindow.Pin();
            const FVector2D WindowClient = Window->GetClientSizeInScreen(), WindowOuter = Window->GetSizeInScreen(), WindowLayout = Window->GetViewportSize();
            const auto ScenePixels = Client->Viewport->GetSizeXY(), TargetPixels = Client->Viewport->GetRenderTargetTextureSizeXY();
            Test->AddInfo(FString::Printf(TEXT("S362_PICKER_RESIZE desired=%dx%d attempt=%d client=%.3fx%.3f outer=%.3fx%.3f layout=%.3fx%.3f local=%.3fx%.3f render=%.3fx%.3f scene=%dx%d target=%dx%d dpi=%.3f appScale=%.3f"),
                Desired.X, Desired.Y, ResizeAttempts, WindowClient.X, WindowClient.Y, WindowOuter.X, WindowOuter.Y, WindowLayout.X, WindowLayout.Y,
                PaintedGeometry.GetLocalSize().X, PaintedGeometry.GetLocalSize().Y, DrawSize.X, DrawSize.Y, ScenePixels.X, ScenePixels.Y, TargetPixels.X, TargetPixels.Y,
                Window->GetDPIScaleFactor(), FSlateApplication::Get().GetApplicationScale()));
            if (DrawPixels != Desired && ResizeAttempts++ < 3)
            {
                // Account for an editor-hosted viewport's border/tool strip using measured
                // client pixels, rather than mistaking the outer window for the game viewport.
                Window->Resize(FVector2D(Window->GetClientSizeInScreen()) + FVector2D(Desired - DrawPixels));
                MatchSceneTargetToClient(Desired);
                Mark = Frame; return false;
            }
            if (!Test->TestEqual(TEXT("Actual Slate game viewport draws the requested pixel area"), DrawPixels, Desired)) return true;
            if (!Test->TestTrue(TEXT("Native picker opens at 125 percent text"), Combat->OpenBurnPicker())) return true;
            int32 Selected = INDEX_NONE, Longest = -1;
            for (int32 I = 0; I < Combat->GetBurnChoices().Num(); ++I)
            {
                const auto& Row = Combat->GetBurnChoices()[I];
                if (bCore && Row.Id == TEXT("core_name_origin")) { Selected = I; break; }
                if (!bCore && Row.Description.Len() > Longest) { Selected = I; Longest = Row.Description.Len(); }
            }
            if (!Test->TestTrue(TEXT("Authored longest/core memory is available"), Selected >= 0)) return true;
            Combat->SelectBurn(Selected);
            if (bCore) { Test->TestFalse(TEXT("First core confirm only arms"), Combat->ConfirmBurn()); Test->TestTrue(TEXT("Core warning stays armed without consuming a memory"), Combat->IsBurnArmed()); }
            Mark = Frame; Phase = 1; return false;
        }
        UMemoriaCombatHudWidget* Hud = nullptr;
        for (TObjectIterator<UMemoriaCombatHudWidget> It; It; ++It) if (It->GetOwningPlayer() == PC && It->IsInViewport()) { Hud = *It; break; }
        if (!Hud || Frame < Mark + 24) return false;
        if (Phase == 3)
        {
            Test->TestEqual(TEXT("Closing picker restores the original field HUD viewport order"), UGameViewportSubsystem::Get()->GetWidgetSlot(Hud).ZOrder, 4);
            FActualOverlayHost ActualHud;
            Test->TestTrue(TEXT("Closed HUD is attached to the actual viewport Overlay"), ActualHost(Client.Get(), Hud, ActualHud));
            Test->TestEqual(TEXT("Closing picker restores actual outer Overlay ZOrder"), ActualHud.Z, 14);
            ++CaseIndex; Phase = 0; return false;
        }
        if (Phase == 1)
        {
            Test->TestEqual(TEXT("Actual picker paints above achievement toasts and the location/item HUD"), UGameViewportSubsystem::Get()->GetWidgetSlot(Hud).ZOrder, 100);
            FActualOverlayHost ActualHud, ActualPopup;
            const bool bHudHost = ActualHost(Client.Get(), Hud, ActualHud);
            const bool bPopupHost = ActualHost(Client.Get(), PC->GetAchievementPopup(), ActualPopup);
            Test->TestTrue(TEXT("HUD has a real outer viewport Overlay host"), bHudHost);
            Test->TestTrue(TEXT("Existing First Burn popup has its real outer viewport host"), bPopupHost);
            Test->TestEqual(TEXT("Picker outer host is actually reinserted above transient overlays"), ActualHud.Z, 110);
            Test->TestEqual(TEXT("Achievement popup keeps its original outer host order"), ActualPopup.Z, 105);
            Test->TestTrue(TEXT("Actual sibling ordering puts picker above achievement toast"), bHudHost && bPopupHost
                && ActualHud.Overlay == ActualPopup.Overlay && ActualHud.Host != ActualPopup.Host
                && ActualHud.Index > ActualPopup.Index && ActualHud.Z > ActualPopup.Z);
            Test->AddInfo(FString::Printf(TEXT("S362_PICKER_ACTUAL_STACK hud_z=%d popup_z=%d hud_index=%d popup_index=%d same_overlay=%d"),
                ActualHud.Z, ActualPopup.Z, ActualHud.Index, ActualPopup.Index, int32(ActualHud.Overlay == ActualPopup.Overlay)));
            Test->TestEqual(TEXT("Actual PIE render size matches the requested responsive case"), Client->Viewport->GetSizeXY(), Desired);
            Test->TestEqual(TEXT("Actual PIE render target matches the requested responsive case"), Client->Viewport->GetRenderTargetTextureSizeXY(), Desired);
            const auto& Probe = Hud->GetBurnPickerPaintProbe();
            const auto& Row = Combat->GetBurnChoices()[Combat->GetBurnSelection()];
            Test->TestTrue(TEXT("Probe comes from the actual owning native paint"), Probe.bDrawn && Probe.Viewport == Hud->GetCachedGeometry().GetLocalSize());
            Test->TestEqual(TEXT("Actual text scale is 125 percent"), Probe.TextScale, 1.25f);
            Test->TestEqual(TEXT("Painter receives the complete authored description"), Probe.Description, Row.Description);
            Test->TestEqual(TEXT("Painter receives the complete authored consequence"), Probe.AuthoredConsequence, Row.KnownConsequence);
            Test->TestEqual(TEXT("Wrapped description loses no authored characters"), WithoutWhitespace(FString::Join(Probe.DescriptionLines, TEXT(""))), WithoutWhitespace(Row.Description));
            Test->TestEqual(TEXT("Wrapped consequence loses no painted characters"), WithoutWhitespace(FString::Join(Probe.ConsequenceLines, TEXT(""))), WithoutWhitespace(Probe.PaintedConsequence));
            Test->TestFalse(TEXT("Full meaning, controls and irreversible confirmation fit"), Probe.bClipped || Probe.bDescriptionClipped || Probe.bConsequenceClipped);
            Test->TestTrue(TEXT("At least two navigable rows remain"), Probe.Visible >= 2);
            Test->TestTrue(TEXT("Native page follows the selected row"), Probe.Selected >= Probe.First && Probe.Selected < Probe.First + Probe.Visible && Probe.Page >= 1 && Probe.Page <= Probe.Pages);
            Test->TestTrue(TEXT("Confirmation follows the full selected information"), Probe.WarningBounds.Min.Y > Probe.ConsequenceBounds.Max.Y && Probe.ControlsBounds.Min.Y >= Probe.WarningBounds.Max.Y);
            const FString Name = FString::Printf(TEXT("%s_%s_%dx%d_125"), bKo ? TEXT("ko") : TEXT("en"), bCore ? TEXT("core_confirmation") : TEXT("longest_meaning"), Desired.X, Desired.Y);
            ScreenshotPath = FPaths::ProjectSavedDir() / TEXT("Validation/S362Picker") / (Name + TEXT(".png"));
            bScreenshotProcessed = false;
            // Restrict the UI-inclusive screenshot to the actual game viewport, excluding
            // editor window borders. The saved PNG dimensions are checked before proceeding.
            FScreenshotRequest::RequestScreenshot(ScreenshotPath, true, false, false, FIntRect(), true);
            Test->AddInfo(FString::Printf(TEXT("S362_PICKER %s pixels=%dx%d local=%.1fx%.1f rows=%d page=%d/%d descLines=%d costLines=%d infoFont=%d clipped=%d"), *Name, Desired.X, Desired.Y, Probe.Viewport.X, Probe.Viewport.Y, Probe.Visible, Probe.Page, Probe.Pages, Probe.DescriptionLines.Num(), Probe.ConsequenceLines.Num(), Probe.InfoFontSize, int32(Probe.bClipped)));
            Mark = Frame; Phase = 2; return false;
        }
        if (Phase == 2 && Frame > Mark + 8)
        {
            if (!bScreenshotProcessed)
            {
                if (Frame < Mark + 300) return false;
                Test->AddError(TEXT("UI screenshot request was not processed")); return true;
            }
            TArray<uint8> Png;
            const bool bLoaded = FFileHelper::LoadFileToArray(Png, *ScreenshotPath);
            const uint8 Signature[] = {137, 80, 78, 71, 13, 10, 26, 10};
            const bool bPng = bLoaded && Png.Num() >= 24 && FMemory::Memcmp(Png.GetData(), Signature, 8) == 0
                && Png[12] == 'I' && Png[13] == 'H' && Png[14] == 'D' && Png[15] == 'R';
            if (!Test->TestTrue(TEXT("Processed native UI capture is a saved PNG with an IHDR"), bPng)) return true;
            auto BigEndian = [&](int32 Offset) { return (uint32(Png[Offset]) << 24) | (uint32(Png[Offset + 1]) << 16) | (uint32(Png[Offset + 2]) << 8) | uint32(Png[Offset + 3]); };
            const FIntPoint ScreenshotSize(int32(BigEndian(16)), int32(BigEndian(20)));
            Test->TestEqual(TEXT("Saved native UI screenshot proves the requested responsive pixel size"), ScreenshotSize, Desired);
            Test->AddInfo(FString::Printf(TEXT("S362_PICKER_SCREENSHOT pixels=%dx%d path=%s"), ScreenshotSize.X, ScreenshotSize.Y, *ScreenshotPath));
            Combat->CloseBurnPicker();
            const auto After = Run->GetPlayerMemory()->GetSnapshot();
            Test->TestTrue(TEXT("Inspecting and cancelling every responsive case preserves burn history"), Before.BurnedHistory == After.BurnedHistory);
            Test->TestEqual(TEXT("Inspecting and cancelling consumes no field burns"), Combat->GetBurns(), 0);
            Mark = Frame; Phase = 3;
        }
        return false;
    }
private:
    void MatchSceneTargetToClient(const FIntPoint& Desired)
    {
        const auto Window = SavedWindow.Pin();
        static_cast<FSceneViewport*>(Client->Viewport)->SetViewportSize(Desired.X, Desired.Y);
        // SetViewportSize also sets SWindow's independent extent. That extent describes
        // the WHOLE Slate window, including its title/border, whereas Desired describes
        // the game client. Keep the real scene target exact and lay out Slate in the
        // actual outer window extent so its child viewport still has Desired pixels.
        const FVector2D OuterPixels = Window->GetSizeInScreen();
        Window->SetIndependentViewportSize(OuterPixels);
        FSlateApplication::Get().GetRenderer()->UpdateFullscreenState(Window.ToSharedRef(), FMath::RoundToInt(OuterPixels.X), FMath::RoundToInt(OuterPixels.Y));
    }
    void OnScreenshotProcessed() { bScreenshotProcessed = true; }
    FAutomationTestBase* Test; double Started, OldDelta = 0.; uint64 LastFrame = MAX_uint64;
    int32 Frame = 0, Mark = 0, Phase = 0, CaseIndex = 0, OldTextPercent = 100, ResizeAttempts = 0;
    bool bReady = false, bFixed = false, bOldFixed = false, bWindowCaptured = false, bOldMaximized = false, bOldViewportDrivenByWindow = true, bScreenshotProcessed = false;
    TWeakObjectPtr<UMemoriaFieldCombatSubsystem> Combat; TWeakObjectPtr<UMemoriaRunSubsystem> Run;
    TWeakObjectPtr<UMemoriaSettingsSubsystem> Settings; TWeakObjectPtr<UGameViewportClient> Client;
    FIntPoint OldSize = FIntPoint::ZeroValue; EWindowMode::Type OldMode = EWindowMode::Windowed;
    TWeakPtr<SWindow> SavedWindow; FVector2D OldWindowPosition, OldWindowSize, OldWindowViewportSize;
    FDelegateHandle ScreenshotHandle; FString ScreenshotPath, OldLocale; FMemoriaMemorySnapshot Before;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FS362PickerReadable, "MemoriaVisual.S362PickerFullMeaning", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FS362PickerReadable::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice"))) return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FPickerReadableReplay(this)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand()); return true;
}
#endif
