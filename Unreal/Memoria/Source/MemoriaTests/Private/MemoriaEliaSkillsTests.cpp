#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "Tests/AutomationCommon.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Framework/MemoriaSliceHost.h"
#include "Framework/MemoriaFieldPawn.h"
#include "Narrative/MemoriaNarrativeSubsystem.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Domain/MemoriaPlayerMemoryDomain.h"
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
// S319: Elia's techniques after elia_diary.gd, with real keys. All four start locked. Burning the campfire song,
// the reaching hand and the first sword through the burn picker writes her diary and unlocks Humming Shield,
// Desperate Reach and Remembered Strike (Anchor Pulse needs a memory the starting set lacks). Then 1 halves a
// blow, 2 freezes the foes nearby, 3 strikes for 10 + 8 per burned memory, and a used technique waits out
// its cooldown.
class FEliaSkillsReplay final : public IAutomationLatentCommand
{
public:
    explicit FEliaSkillsReplay(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
    ~FEliaSkillsReplay() override { if (bFixed) { FApp::SetUseFixedTimeStep(bOldFixed); FApp::SetFixedDeltaTime(OldDelta); } }
    bool Update() override
    {
        if (LastFrame == GFrameCounter) return false;
        LastFrame = GFrameCounter;
        if (FPlatformTime::Seconds() - Started > 180) { Test->AddError(FString::Printf(TEXT("Elia skills timeout at step %d"), Step)); return true; }
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
        if (!World->GetMapName().EndsWith(TEXT("L_VerdanHost")) || Host->GetState() != EMemoriaSliceState::Exploration) return false;
        ++Frame;
        auto* Combat = World->GetSubsystem<UMemoriaFieldCombatSubsystem>();
        if (!Combat || !Combat->GetPlayer() || Frame % 8 != 0) return false;
        auto Key = [&](const FKey& K) { PC->InputKey(FInputKeyEventArgs::CreateSimulated(K, IE_Pressed, 1.f)); PC->InputKey(FInputKeyEventArgs::CreateSimulated(K, IE_Released, 0.f)); };
        auto Capture = [&](const TCHAR* Name) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Validation/EliaSkills") / (FString(Name) + TEXT(".png")), true, false); };
        static const FKey Digits[9] = {EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five, EKeys::Six, EKeys::Seven, EKeys::Eight, EKeys::Nine};
        auto BurnById = [&](const TCHAR* Id, bool bTwice)
        {
            Key(EKeys::R);
            const int32 Index = Combat->GetBurnChoices().IndexOfByPredicate([&](const FMemoriaBurnChoice& C) { return C.Id == Id; });
            if (Index < 0 || Index > 8) { Test->AddError(FString::Printf(TEXT("%s is not offered"), Id)); return false; }
            Key(Digits[Index]); Key(EKeys::Enter); if (bTwice) Key(EKeys::Enter);
            return Run->GetPlayerMemory()->CanBurn(Id) == EMemoriaMemoryResult::AlreadyBurned;
        };
        // Each burn plays out, ring and all, before the next step (the ring would burn foes spawned into it).
        // The diary capture (step 2) is taken while the first ring still burns, so the notice is fully up.
        if (Combat->IsCasting() || Combat->IsPickingBurn() || (Step != 2 && Combat->GetBurnWave().bLive)) return false;
        switch (Step++)
        {
        case 0:
            for (int32 I = 0; I < 4; ++I) Test->TestFalse(*FString::Printf(TEXT("Technique %d starts locked"), I + 1), Combat->IsEliaSkillUnlocked(I));
            Key(EKeys::One);
            Test->TestEqual(TEXT("A locked technique does nothing"), Combat->GetEliaSkillsUsed(), 0);
            break;
        case 1:
            Test->TestTrue(TEXT("The campfire song burns"), BurnById(TEXT("daily_campfire_song"), false));
            Test->TestTrue(TEXT("Elia writes her diary and Humming Shield opens"), Combat->IsEliaSkillUnlocked(0) && Combat->GetEliaNotice().Contains(TEXT("\n")));
            break;
        case 2: Capture(TEXT("EliaDiary")); break;
        case 3: Test->TestTrue(TEXT("The reaching hand burns"), BurnById(TEXT("rel_hand_reaching"), false)); break;
        case 4:
            Test->TestTrue(TEXT("The first sword burns (asked twice)"), BurnById(TEXT("identity_first_sword"), true));
            Test->TestTrue(TEXT("Desperate Reach and Remembered Strike open"), Combat->IsEliaSkillUnlocked(1) && Combat->IsEliaSkillUnlocked(2));
            Test->TestFalse(TEXT("Anchor Pulse waits for a memory the starting set lacks"), Combat->IsEliaSkillUnlocked(3));
            break;
        case 5:
        {
            // A thief and a husk close by, both beyond aggro reach of a first blow.
            const FVector Me = Pawn->GetActorLocation();
            Thief = Combat->SpawnWave(1, Me + FVector(110, 0, 0), 0.f, EMemoriaFoeKind::MarketThief)[0];
            Husk = Combat->SpawnWave(1, Me + FVector(-150, 60, 0), 0.f)[0];
            break;
        }
        case 6:
        {
            // 1: Humming Shield halves the thief's blow.
            Key(EKeys::One);
            Test->TestTrue(TEXT("1 raises Humming Shield"), Combat->IsShielded());
            const int64 Before = Combat->GetPlayerHp();
            if (Thief.IsValid()) Combat->StrikePlayer(Thief.Get(), 8.f);
            Test->TestEqual(TEXT("The shield halves the blow"), Before - Combat->GetPlayerHp(), int64(4));
            Key(EKeys::One);
            Test->TestEqual(TEXT("A used technique waits out its cooldown"), Combat->GetEliaSkillsUsed(), 1);
            break;
        }
        case 7: Capture(TEXT("EliaShield")); break;
        case 8:
            // 2: Desperate Reach freezes the foes close by.
            Key(EKeys::Two);
            Test->TestTrue(TEXT("2 freezes both foes"), Thief.IsValid() && Husk.IsValid() && Thief->GetState() == EMemoriaMonsterState::Stagger && Husk->GetState() == EMemoriaMonsterState::Stagger);
            break;
        case 9:
        {
            // 3: Remembered Strike, 10 + 8 x 3 burned memories, and Elia swings.
            const float ThiefBefore = Thief.IsValid() ? Thief->GetHealth() : 0.f, HuskBefore = Husk.IsValid() ? Husk->GetHealth() : 0.f;
            Key(EKeys::Three);
            const float Dealt = (ThiefBefore - (Thief.IsValid() ? Thief->GetHealth() : 0.f)) + (HuskBefore - (Husk.IsValid() ? Husk->GetHealth() : 0.f));
            Test->TestEqual(TEXT("3 strikes for 10 + 8 per burned memory"), Dealt, 10.f + 8.f * Run->GetPlayerMemory()->GetSnapshot().BurnedHistory.Num(), .01f);
            AMemoriaEliaCompanion* Elia = nullptr; for (TActorIterator<AMemoriaEliaCompanion> It(World); It; ++It) Elia = *It;
            Test->TestTrue(TEXT("Elia swings"), Elia && Elia->GetFigure() && Elia->GetFigure()->IsActing());
            break;
        }
        case 10: Capture(TEXT("EliaStrike")); break;
        default: return true;
        }
        return false;
    }
private:
    FAutomationTestBase* Test;
    double Started, OldDelta = 0;
    uint64 LastFrame = MAX_uint64;
    int32 Step = 0, Frame = 0;
    bool bFixed = false, bOldFixed = false;
    TWeakObjectPtr<AMemoriaFieldMonster> Thief, Husk;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEliaSkillsTest, "MemoriaVisual.EliaSkills", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FEliaSkillsTest::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice"))) return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FEliaSkillsReplay(this)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
