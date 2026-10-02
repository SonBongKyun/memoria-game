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
        const auto* Memory = Run->GetPlayerMemory();
        if (Phase == 0)
        {
            for (TActorIterator<AMemoriaEliaCompanion> It(World); It; ++It) It->SetActorHiddenInGame(true);
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
            Press(EKeys::R);
            Test->TestTrue(TEXT("R opens the burn picker"), Combat->IsPickingBurn());
            Test->TestTrue(TEXT("The picker offers the starting memories"), Combat->GetBurnChoices().Num() >= 5);
            Test->TestTrue(TEXT("Weakest first"), Combat->GetBurnChoices().Num() > 1 && Combat->GetBurnChoices()[0].Grade <= Combat->GetBurnChoices().Last().Grade);
            Test->TestTrue(TEXT("The world slows while choosing"), FMath::IsNearlyEqual(Dilation(), MemoriaCombatTuning::BurnPickDilation, .001f));
            Test->TestFalse(TEXT("Arrel holds still while choosing"), Combat->CanMove());
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
                static const FKey Digits[9] = {EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five, EKeys::Six, EKeys::Seven, EKeys::Eight, EKeys::Nine};
                if (Sword >= 0 && Sword < 9) Press(Digits[Sword]);
                Press(EKeys::Enter);
                Test->TestTrue(TEXT("An identity memory asks twice"), Combat->IsBurnArmed() && Combat->IsPickingBurn());
                Test->TestEqual(TEXT("One confirm does not burn it"), Memory->CanBurn(TEXT("identity_first_sword")), EMemoriaMemoryResult::Success);
                Hand = IndexOf(TEXT("rel_hand_reaching"));
                if (Hand >= 0 && Hand < 9) Press(Digits[Hand]);
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
