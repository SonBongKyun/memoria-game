#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "Tests/AutomationCommon.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "GameFramework/WorldSettings.h"
#include "Framework/MemoriaSliceHost.h"
#include "Framework/MemoriaFieldPawn.h"
#include "Narrative/MemoriaNarrativeSubsystem.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Combat/MemoriaFieldCombatSubsystem.h"
#include "Combat/MemoriaFieldCombatTypes.h"
#include "Combat/MemoriaFieldMonster.h"
#include "Combat/MemoriaBurnPickerLayout.h"
#include "Combat/MemoriaCombatHudWidget.h"
#include "UObject/UObjectIterator.h"
#include "Domain/MemoriaChapterMemories.h"
#include "Tutorial/MemoriaTutorialSubsystem.h"
#include "Achievements/MemoriaAchievementSubsystem.h"
#include "Interaction/MemoriaEliaCompanion.h"
#include "Presentation/MemoriaFieldCharacterComponent.h"
#include "EngineUtils.h"
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#if WITH_DEV_AUTOMATION_TESTS
namespace
{
// S312: Arrel burns a memory in the Verdan field. R opens the picker and slows the world, Esc lets it go,
// an identity memory asks twice, and a relationship memory becomes Incinerate: its ring fells the husks
// within reach, spares the one beyond it, and the memory is gone from the run for good.
class FFieldBurnReplay final : public IAutomationLatentCommand
{
public:
    explicit FFieldBurnReplay(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
    ~FFieldBurnReplay() override { if (bFixed) { FApp::SetUseFixedTimeStep(bOldFixed); FApp::SetFixedDeltaTime(OldDelta); } }
    bool Update() override
    {
        if (LastFrame == GFrameCounter) return false;
        LastFrame = GFrameCounter;
        if (FPlatformTime::Seconds() - Started > 180) { Test->AddError(FString::Printf(TEXT("Field burn timeout in phase %d"), Phase)); return true; }
        UWorld* World = GEditor->PlayWorld;
        auto* PC = World ? Cast<AMemoriaSliceController>(World->GetFirstPlayerController()) : nullptr;
        auto* Pawn = PC ? Cast<AMemoriaFieldPawn>(PC->GetPawn()) : nullptr;
        if (!Pawn || World->GetTimeSeconds() < .3) return false;
        auto* Host = World->GetGameInstance()->GetSubsystem<UMemoriaNarrativeSubsystem>();
        auto* Run = World->GetGameInstance()->GetSubsystem<UMemoriaRunSubsystem>();
        if (!bFixed)
        {
            bFixed = true; bOldFixed = FApp::UseFixedTimeStep(); OldDelta = FApp::GetFixedDeltaTime();
            FApp::SetUseFixedTimeStep(true); FApp::SetFixedDeltaTime(1.0 / 60.0);
            for (int32 I = 0; I < 20 && Host->GetState() == EMemoriaSliceState::VN; ++I) Host->Confirm(1);
            return false;
        }
        if (!World->GetMapName().EndsWith(TEXT("L_VerdanHost")) || World->GetTimeSeconds() == LastWorldTime) return false;
        LastWorldTime = World->GetTimeSeconds(); ++Frame;
        auto* Combat = World->GetSubsystem<UMemoriaFieldCombatSubsystem>();
        if (!Combat || !Combat->GetPlayer() || !Run || Host->GetState() != EMemoriaSliceState::Exploration) return false;
        auto Capture = [&](const TCHAR* Name) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Validation/FieldBurn") / (FString(Name) + TEXT(".png")), true, false); };
        auto Press = [&](const FKey& Key)
        {
            PC->InputKey(FInputKeyEventArgs::CreateSimulated(Key, IE_Pressed, 1.f));
            PC->InputKey(FInputKeyEventArgs::CreateSimulated(Key, IE_Released, 0.f));
        };
        auto IndexOf = [&](const TCHAR* Id) { return Combat->GetBurnChoices().IndexOfByPredicate([&](const FMemoriaBurnChoice& C) { return C.Id == Id; }); };
        auto Dilation = [&] { return World->GetWorldSettings()->TimeDilation; };
        auto* Memory = Run->GetPlayerMemory();
        auto SelectWithKeys = [&](int32 Index)
        {
            for (int32 I = 0; I < Combat->GetBurnChoices().Num() && Combat->GetBurnSelection() != Index; ++I)
                Press(EKeys::Down);
        };
        auto CheckUnchanged = [&]
        {
            const auto After = Memory->GetSnapshot();
            Test->TestEqual(TEXT("Cancelling a long list burns nothing"), Combat->GetBurns(), 0);
            Test->TestEqual(TEXT("Cancelling keeps every owned memory"), After.Owned.Num(), BeforePicker.Owned.Num());
            Test->TestTrue(TEXT("Cancelling keeps burn history"), After.BurnedHistory == BeforePicker.BurnedHistory);
            for (int32 I = 0; I < BeforePicker.Owned.Num() && I < After.Owned.Num(); ++I)
            {
                const auto& A = BeforePicker.Owned[I]; const auto& B = After.Owned[I];
                Test->TestTrue(*FString::Printf(TEXT("Cancel leaves %s intact and in order"), *A.Id),
                    A.Id == B.Id && A.bBurned == B.bBurned && A.bFaded == B.bFaded && A.bResidue == B.bResidue
                    && A.Erosion == B.Erosion && A.Connections == B.Connections);
            }
        };
        auto CheckLayout = [&](const FVector2D& Viewport, int32 Selection)
        {
            const auto L = MemoriaBurnPicker::Build(Viewport, Combat->GetBurnChoices().Num(), Selection);
            Test->TestTrue(TEXT("Picker panel remains inside the viewport"),
                L.Panel.X >= 2.f && L.Panel.Y >= 2.f && L.Panel.X + L.Extent.X <= Viewport.X - 2.f
                && L.Panel.Y + L.Extent.Y <= Viewport.Y - 2.f);
            Test->TestTrue(TEXT("Selected memory remains visible"), L.Visible > 0 && L.Shows(Selection));
            Test->TestTrue(TEXT("Rows stop above the reserved confirmation and controls"),
                L.RowAt(L.First + L.Visible - 1).Y + L.RowHeight <= L.FooterTop() + .01f
                && L.WarningAt().Y > L.FooterTop() && L.ControlsAt().Y > L.WarningAt().Y
                && L.ControlsAt().Y + 12.f * L.Scale < L.Panel.Y + L.Extent.Y);
        };
        if (Phase == 0)
        {
            for (TActorIterator<AMemoriaEliaCompanion> It(World); It; ++It) It->SetActorHiddenInGame(true);
            // A late-run inventory made entirely from the real imported chapter definitions. This fixture
            // does not advance the chapter or create synthetic memories merely to exercise the UI.
            OriginalLocale = Run->GetRunSnapshot().CurrentLocale;
            Run->SetLocale(TEXT("ko"));
            for (int64 Chapter = 3; Chapter <= 5; ++Chapter)
                for (const auto& Definition : MemoriaChapterMemories::For(Chapter))
                    if (!Memory->GetDefinitions().ContainsByPredicate([&](const auto& D) { return D.Id == Definition.Id; }))
                        Test->TestEqual(TEXT("Acquire an authored long-list fixture memory"), Run->AcquireMemory(Definition), EMemoriaMemoryResult::Success);
            BeforePicker = Memory->GetSnapshot();
            const FVector Me = Pawn->GetActorLocation();
            // Three husks inside Incinerate's reach, one far beyond it (and beyond its aggro, so it stays put).
            for (AMemoriaFieldMonster* Husk : Combat->SpawnWave(3, Me, 300.f)) Near.Add(Husk);
            Far = Combat->SpawnWave(1, Me + FVector(0, -1150, 0), 0.f)[0];
            Test->TestEqual(TEXT("Three husks rise close by"), Near.Num(), 3);
            Test->TestEqual(TEXT("The hand memory can burn"), Memory->CanBurn(TEXT("rel_hand_reaching")), EMemoriaMemoryResult::Success);
            Phase = 1; Mark = Frame; return false;
        }
        if (Phase == 1)
        {
            if (Frame < Mark + 12) return false;
            UMemoriaCombatHudWidget* LiveHud = nullptr;
            for (TObjectIterator<UMemoriaCombatHudWidget> It; It; ++It)
                if (It->GetOwningPlayer() == PC && It->IsInViewport()) { LiveHud = *It; break; }
            // Native UMG local units can differ from physical screenshot pixels under DPI scaling.
            // Exercise the geometry the actual owning picker paints, then separately check smaller limits.
            if (!LiveHud || LiveHud->GetCachedGeometry().GetLocalSize().IsNearlyZero()) return false;
            NativeViewport = LiveHud->GetCachedGeometry().GetLocalSize();
            Press(EKeys::R);
            Test->TestTrue(TEXT("R opens the burn picker"), Combat->IsPickingBurn());
            Test->TestTrue(TEXT("The picker offers the starting memories"), Combat->GetBurnChoices().Num() >= 5);
            Test->TestTrue(TEXT("Weakest first"), Combat->GetBurnChoices().Num() > 1 && Combat->GetBurnChoices()[0].Grade <= Combat->GetBurnChoices().Last().Grade);
            Test->TestTrue(TEXT("The world slows while choosing"), FMath::IsNearlyEqual(Dilation(), MemoriaCombatTuning::BurnPickDilation, .001f));
            Test->TestFalse(TEXT("Arrel holds still while choosing"), Combat->CanMove());
            Test->TestTrue(TEXT("The native fixture has more than nine real memories"), Combat->GetBurnChoices().Num() > 9);
            const auto NativeLayout = MemoriaBurnPicker::Build(NativeViewport, Combat->GetBurnChoices().Num(), 0);
            Test->TestTrue(TEXT("The real owning HUD requires scrolling"), Combat->GetBurnChoices().Num() > NativeLayout.Visible);
            MiddleIndex = FMath::Min(Combat->GetBurnChoices().Num() - 1, NativeLayout.Visible + 2);
            Test->AddInfo(FString::Printf(TEXT("S352 picker: owned=%d available=%d native_local=%.1fx%.1f visible=%d middle=%d"),
                Memory->GetSnapshot().Owned.Num(), Combat->GetBurnChoices().Num(), NativeViewport.X, NativeViewport.Y, NativeLayout.Visible, MiddleIndex));
            for (const FVector2D Viewport : {NativeViewport, FVector2D(1280, 720), FVector2D(960, 540), FVector2D(640, 360)})
                for (int32 I = 0; I < Combat->GetBurnChoices().Num(); ++I) CheckLayout(Viewport, I);
            Phase = 7; Mark = Frame; return false;
        }
        if (Phase == 7)
        {
            // Let ordinary first-battle hints and queued achievement toasts finish before the UI captures.
            const auto* Tutorial = World->GetGameInstance()->GetSubsystem<UMemoriaTutorialSubsystem>();
            const auto* Achievements = World->GetGameInstance()->GetSubsystem<UMemoriaAchievementSubsystem>();
            if ((Tutorial && Tutorial->IsShowing()) || (Achievements && !Achievements->GetPopup().IsEmpty())) { QuietFrames = 0; return false; }
            if (++QuietFrames < 32) return false;
            Phase = 10; Mark = Frame; return false;
        }
        if (Phase == 10)
        {
            if (Frame == Mark + 1)
            {
                SelectWithKeys(MiddleIndex);
                Test->TestEqual(TEXT("Keyboard reaches beyond the native first visible page"), Combat->GetBurnSelection(), MiddleIndex);
                const auto L = MemoriaBurnPicker::Build(NativeViewport, Combat->GetBurnChoices().Num(), MiddleIndex);
                Test->TestTrue(TEXT("The native visible window follows keyboard selection"), L.First > 0 && L.Shows(MiddleIndex));
                CheckLayout(NativeViewport, Combat->GetBurnSelection());
            }
            if (Frame == Mark + 6) Capture(TEXT("BurnPickerMiddle"));
            if (Frame == Mark + 10)
            {
                for (int32 I = Combat->GetBurnSelection(); I < Combat->GetBurnChoices().Num() - 1; ++I) Press(EKeys::MouseScrollDown);
                Test->TestEqual(TEXT("Wheel reaches the final memory"), Combat->GetBurnSelection(), Combat->GetBurnChoices().Num() - 1);
                CheckLayout(NativeViewport, Combat->GetBurnSelection());
            }
            if (Frame == Mark + 16) Capture(TEXT("BurnPickerEnd"));
            if (Frame == Mark + 20)
            {
                Press(EKeys::One);
                Test->TestEqual(TEXT("Digit one returns from the last page to the first memory"), Combat->GetBurnSelection(), 0);
                Press(EKeys::Up);
                Test->TestEqual(TEXT("Up wraps from the first memory to the last"), Combat->GetBurnSelection(), Combat->GetBurnChoices().Num() - 1);
                const auto& Last = Combat->GetBurnChoices().Last();
                Test->TestTrue(TEXT("The last authored memory needs irreversible confirmation"), Last.Grade >= MemoriaCombatTuning::BurnAskTwiceFrom);
                Press(EKeys::Enter);
                Test->TestTrue(TEXT("The last page keeps its second confirmation armed"), Combat->IsPickingBurn() && Combat->IsBurnArmed());
                CheckLayout(NativeViewport, Combat->GetBurnSelection());
                Test->TestEqual(TEXT("Arming the last memory does not consume it"), Memory->CanBurn(Last.Id), EMemoriaMemoryResult::Success);
            }
            if (Frame == Mark + 26) Capture(TEXT("BurnPickerConfirmation"));
            if (Frame < Mark + 32) return false;
            Press(EKeys::Escape);
            Test->TestFalse(TEXT("Esc cancels even from a last-page irreversible confirmation"), Combat->IsPickingBurn());
            CheckUnchanged();
            Run->SetLocale(TEXT("en"));
            Press(EKeys::R);
            Test->TestTrue(TEXT("The English picker opens the same long inventory"), Combat->IsPickingBurn());
            for (const auto& Choice : Combat->GetBurnChoices())
            {
                const auto* Definition = Memory->GetDefinitions().FindByPredicate([&](const auto& D) { return D.Id == Choice.Id; });
                Test->TestTrue(TEXT("English rows keep their authored title"), Definition && Choice.Title == Definition->Title);
            }
            Phase = 11; Mark = Frame; return false;
        }
        if (Phase == 11)
        {
            if (Frame == Mark + 1)
            {
                SelectWithKeys(MiddleIndex);
                Test->TestEqual(TEXT("English keyboard selection reaches the native overflow window"), Combat->GetBurnSelection(), MiddleIndex);
                CheckLayout(NativeViewport, Combat->GetBurnSelection());
            }
            if (Frame == Mark + 6) Capture(TEXT("BurnPickerMiddleEnglish"));
            if (Frame == Mark + 10)
            {
                SelectWithKeys(Combat->GetBurnChoices().Num() - 1);
                Press(EKeys::Enter);
                Test->TestTrue(TEXT("English last-page confirmation arms without burning"), Combat->IsPickingBurn() && Combat->IsBurnArmed());
                CheckLayout(NativeViewport, Combat->GetBurnSelection());
            }
            if (Frame == Mark + 16) Capture(TEXT("BurnPickerConfirmationEnglish"));
            if (Frame < Mark + 22) return false;
            Press(EKeys::RightMouseButton);
            Test->TestFalse(TEXT("Right click cancels the English confirmation"), Combat->IsPickingBurn());
            CheckUnchanged();
            Run->SetLocale(OriginalLocale);
            Press(EKeys::R);
            Phase = 2; Mark = Frame; return false;
        }
        if (Phase == 2)
        {
            if (Frame == Mark + 8)
            {
                // Esc lets the fire go out: nothing burns and time returns.
                Press(EKeys::Escape);
                Test->TestFalse(TEXT("Esc closes the picker"), Combat->IsPickingBurn());
                Test->TestTrue(TEXT("Time returns"), FMath::IsNearlyEqual(Dilation(), 1.f, .001f));
                Test->TestEqual(TEXT("Cancelling burns nothing"), Combat->GetBurns(), 0);
                Press(EKeys::R);
                const int32 Sword = IndexOf(TEXT("identity_first_sword"));
                Test->TestTrue(TEXT("The first sword is offered"), Sword >= 0);
                SelectWithKeys(Sword);
                Press(EKeys::Enter);
                Test->TestTrue(TEXT("An identity memory asks twice"), Combat->IsBurnArmed() && Combat->IsPickingBurn());
                Test->TestEqual(TEXT("One confirm does not burn it"), Memory->CanBurn(TEXT("identity_first_sword")), EMemoriaMemoryResult::Success);
                Hand = IndexOf(TEXT("rel_hand_reaching"));
                SelectWithKeys(Hand);
                Test->TestFalse(TEXT("Choosing again disarms"), Combat->IsBurnArmed());
                Test->TestEqual(TEXT("The hand is selected"), Combat->GetBurnSelection(), Hand);
            }
            if (Frame == Mark + 14) Capture(TEXT("BurnPicker"));
            if (Frame < Mark + 18) return false;
            Press(EKeys::Enter);
            Test->TestFalse(TEXT("A relationship memory burns on one confirm"), Combat->IsPickingBurn());
            Test->TestTrue(TEXT("Arrel gathers the fire"), Combat->IsCasting() && Combat->IsInvulnerable());
            Test->TestEqual(TEXT("The hand is gone for good"), Memory->CanBurn(TEXT("rel_hand_reaching")), EMemoriaMemoryResult::AlreadyBurned);
            Test->TestTrue(TEXT("The run records the burn"), Memory->GetSnapshot().BurnedHistory.Contains(TEXT("rel_hand_reaching")));
            Test->TestEqual(TEXT("The first sword is untouched"), Memory->CanBurn(TEXT("identity_first_sword")), EMemoriaMemoryResult::Success);
            Test->TestTrue(TEXT("Time returns for the release"), FMath::IsNearlyEqual(Dilation(), 1.f, .001f));
            Test->TestEqual(TEXT("A relationship burn starts the chain (S314)"), Combat->GetBurnChain(), 1);
            Phase = 3; Mark = Frame; return false;
        }
        if (Phase == 3)
        {
            const FMemoriaBurnWave& Wave = Combat->GetBurnWave();
            if (Wave.bLive && !bReleased)
            {
                bReleased = true; ReleaseFrame = Frame;
                Test->TestEqual(TEXT("Incinerate is the relationship grade's skill"), Wave.Grade, 2);
                Test->TestEqual(TEXT("It reaches its grade's radius"), Wave.Radius, MemoriaCombatTuning::BurnRadius[2]);
            }
            if (bReleased && Frame == ReleaseFrame + 10) Capture(TEXT("BurnRelease"));
            if (bReleased && Frame == ReleaseFrame + 24) Capture(TEXT("BurnRing"));
            if (bReleased && Frame == ReleaseFrame + 70)
            {
                int32 Fallen = 0;
                for (const auto& Husk : Near) if (!Husk.IsValid() || Husk->IsDead()) ++Fallen;
                Test->TestEqual(TEXT("Every husk within reach falls"), Fallen, 3);
                Test->TestTrue(TEXT("The husk beyond the reach is spared"), Far.IsValid() && !Far->IsDead() && Far->GetHealth() == Far->GetMaxHealth());
                Test->TestEqual(TEXT("Three kills"), Combat->GetKills(), 3);
                Test->TestTrue(TEXT("Arrel is free again"), Combat->CanMove());
                Capture(TEXT("BurnAfter"));
                Test->TestTrue(TEXT("The sword is out after the burn"), Combat->GetPlayerFigure()->IsSwordDrawn());
                // S343: the surviving husk keeps this fight open; smoke must end its burn chain as victory does.
                Run->GrantFieldItem(TEXT("smoke_bomb"), 1);
                Phase = 4; Mark = Frame;
            }
            if (Frame > Mark + 400) { Test->AddError(TEXT("The burn never released")); return true; }
            return false;
        }
        if (Phase == 4)
        {
            if (Combat->IsCasting() || Combat->GetItemCooldown() > 0.f) return false;
            Test->TestEqual(TEXT("The surviving husk preserves the first burn chain"), Combat->GetBurnChain(), 1);
            Test->TestTrue(TEXT("Smoke escapes from the surviving husk"), Combat->UseQuickItem(3, Pawn->GetActorLocation()));
            Test->TestEqual(TEXT("Smoke leaves no live foes"), Combat->LiveMonsterCount(), 0);
            Test->TestEqual(TEXT("Escaping ends the burn chain"), Combat->GetBurnChain(), 0);
            Phase = 5; Mark = Frame; return false;
        }
        if (Phase == 5)
        {
            if (Frame < Mark + int32(MemoriaCombatTuning::SheatheDelay * 60.f) + 30) return false;
            Test->TestFalse(TEXT("Arrel sheathes once the fight is over"), Combat->GetPlayerFigure()->IsSwordDrawn());
            // Keep the next foe beyond the identity burn, so winning cannot conceal a carried-over chain.
            Far = Combat->SpawnWave(1, Pawn->GetActorLocation() + FVector(0, -1150, 0), 0.f)[0];
            Test->TestTrue(TEXT("The next encounter opens its burn picker"), Combat->OpenBurnPicker());
            const int32 Sword = IndexOf(TEXT("identity_first_sword"));
            if (!Test->TestTrue(TEXT("The untouched identity memory is offered again"), Sword >= 0)) return true;
            Combat->SelectBurn(Sword);
            Test->TestFalse(TEXT("The identity burn still asks twice"), Combat->ConfirmBurn());
            Test->TestTrue(TEXT("The next encounter burns the identity memory"), Combat->ConfirmBurn());
            Test->TestEqual(TEXT("The first burn after escape starts a fresh chain"), Combat->GetBurnChain(), 1);
            Phase = 6; Mark = Frame; return false;
        }
        if (Phase == 6 && !Combat->IsCasting() && !Combat->GetBurnWave().bLive)
        {
            Test->TestTrue(TEXT("The next foe survives beyond the burn"), Far.IsValid() && !Far->IsDead() && Far->GetHealth() == Far->GetMaxHealth());
            Test->TestEqual(TEXT("The fresh encounter still has only one burn in its chain"), Combat->GetBurnChain(), 1);
            if (Far.IsValid()) Far->Destroy();
            return true;
        }
        return false;
    }
private:
    FAutomationTestBase* Test;
    double Started, OldDelta = 0, LastWorldTime = -1;
    uint64 LastFrame = MAX_uint64;
    int32 Phase = 0, Frame = 0, Mark = 0, Hand = -1, ReleaseFrame = 0;
    int32 QuietFrames = 0, MiddleIndex = 0;
    FVector2D NativeViewport = FVector2D::ZeroVector;
    FString OriginalLocale;
    FMemoriaMemorySnapshot BeforePicker;
    bool bFixed = false, bOldFixed = false, bReleased = false;
    TArray<TWeakObjectPtr<AMemoriaFieldMonster>> Near;
    TWeakObjectPtr<AMemoriaFieldMonster> Far;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFieldBurnTest, "MemoriaVisual.FieldBurn", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFieldBurnTest::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice"))) return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FFieldBurnReplay(this)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
