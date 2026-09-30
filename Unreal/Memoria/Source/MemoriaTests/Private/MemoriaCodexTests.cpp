#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "Tests/AutomationCommon.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Framework/MemoriaSliceHost.h"
#include "Framework/MemoriaFieldPawn.h"
#include "Narrative/MemoriaNarrativeSubsystem.h"
#include "Combat/MemoriaFieldCombatSubsystem.h"
#include "Combat/MemoriaFieldCombatTypes.h"
#include "Codex/MemoriaCodexSubsystem.h"
#include "Codex/MemoriaCodexWidget.h"
#include "Presentation/MemoriaPauseWidget.h"
#include "Interaction/MemoriaEliaCompanion.h"
#include "EngineUtils.h"
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "UObject/StrongObjectPtr.h"
#include "UnrealClient.h"
#if WITH_DEV_AUTOMATION_TESTS
// S325: codex.gd's rules. Encounters and defeats count per name; the roster names the unmet, and the
// bestiary's denominator is the roster plus anything recorded; the stars follow the grade (one for Grade 5,
// five for Grade 1) and the defeat badges come at 10, 25 and 50.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMemoriaCodexRulesTest, "Memoria.Codex.Rules", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMemoriaCodexRulesTest::RunTest(const FString&)
{
    TStrongObjectPtr<UGameInstance> Game(NewObject<UGameInstance>());
    TStrongObjectPtr<UMemoriaCodexSubsystem> C(NewObject<UMemoriaCodexSubsystem>(Game.Get()));
    const int32 Roster = UMemoriaCodexSubsystem::Roster().Num() - 1; // less the field's stand-in void husk
    TestEqual(TEXT("Nothing recorded: every roster foe is unmet"), C->Unmet().Num(), Roster);
    C->RecordEncounter(TEXT("Market Thief"), false, false, 45, 7);
    C->RecordEncounter(TEXT("Market Thief"), false, false, 45, 7);
    C->RecordDefeat(TEXT("Market Thief"));
    C->RecordDefeat(TEXT("Nobody"));
    const auto* Thief = C->FindEnemy(TEXT("Market Thief"));
    TestTrue(TEXT("Two encounters, one defeat"), Thief && Thief->Encounters == 2 && Thief->Defeated == 1 && Thief->MaxHp == 45);
    TestNull(TEXT("A defeat never recorded as met adds nothing"), C->FindEnemy(TEXT("Nobody")));
    TestEqual(TEXT("A met roster foe leaves the unmet"), C->Unmet().Num(), Roster - 1);
    C->RecordEncounter(TEXT("Void Husk"), true, false, 60, 9);
    TestEqual(TEXT("The denominator is the roster plus what was recorded"), C->KnownTotal(), Roster - 1 + 2);
    TestEqual(TEXT("Korean names from the roster"), UMemoriaCodexSubsystem::EnemyNameKo(TEXT("Market Thief")), FString(TEXT("시장 도적")));
    TestEqual(TEXT("Grade 5 is one star"), UMemoriaCodexSubsystem::Stars(0), FString(TEXT("★☆☆☆☆")));
    TestEqual(TEXT("Grade 1 is five"), UMemoriaCodexSubsystem::Stars(4), FString(TEXT("★★★★★")));
    TestEqual(TEXT("No badge under ten"), UMemoriaCodexSubsystem::DefeatBadge(9), FString());
    TestEqual(TEXT("Bronze at ten"), UMemoriaCodexSubsystem::DefeatBadge(10), FString(TEXT(" ◦")));
    TestEqual(TEXT("Silver at 25"), UMemoriaCodexSubsystem::DefeatBadge(25), FString(TEXT(" ○")));
    TestEqual(TEXT("Gold at 50"), UMemoriaCodexSubsystem::DefeatBadge(50), FString(TEXT(" ●")));
    return true;
}
namespace
{
// S325 in the field: the arrival's memories are in the Memory Archive (the bribe burn marked burned); a husk
// met and burned down is a Bestiary entry (one encounter, one defeat); the pause menu's Codex row opens the
// screen, TAB switches to the archive, and ESC returns to the menu.
class FCodexReplay final : public IAutomationLatentCommand
{
public:
    explicit FCodexReplay(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
    ~FCodexReplay() override { if (bFixed) { FApp::SetUseFixedTimeStep(bOldFixed); FApp::SetFixedDeltaTime(OldDelta); } }
    bool Update() override
    {
        if (LastFrame == GFrameCounter) return false;
        LastFrame = GFrameCounter;
        if (FPlatformTime::Seconds() - Started > 150) { Test->AddError(FString::Printf(TEXT("Codex timeout in phase %d"), Phase)); return true; }
        UWorld* World = GEditor->PlayWorld;
        auto* PC = World ? Cast<AMemoriaSliceController>(World->GetFirstPlayerController()) : nullptr;
        auto* Pawn = PC ? Cast<AMemoriaFieldPawn>(PC->GetPawn()) : nullptr;
        if (!Pawn || World->GetTimeSeconds() < .3) return false;
        auto* Game = World->GetGameInstance();
        auto* Host = Game->GetSubsystem<UMemoriaNarrativeSubsystem>();
        auto* Codex = Game->GetSubsystem<UMemoriaCodexSubsystem>();
        if (!bFixed)
        {
            bFixed = true; bOldFixed = FApp::UseFixedTimeStep(); OldDelta = FApp::GetFixedDeltaTime();
            FApp::SetUseFixedTimeStep(true); FApp::SetFixedDeltaTime(1.0 / 60.0);
            for (int32 I = 0; I < 20 && Host->GetState() == EMemoriaSliceState::VN; ++I) Host->Confirm(1);
            return false;
        }
        if (!World->GetMapName().EndsWith(TEXT("L_VerdanHost"))) return false;
        auto* Combat = World->GetSubsystem<UMemoriaFieldCombatSubsystem>();
        if (!Combat || !Combat->GetPlayer() || (Host->GetState() != EMemoriaSliceState::Exploration && Phase < 2)) return false;
        ++Frame;
        auto Capture = [&](const TCHAR* Name) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Validation/Codex") / (FString(Name) + TEXT(".png")), true, false); };
        auto Press = [&](const FKey& Key) { PC->InputKey(FInputKeyEventArgs::CreateSimulated(Key, IE_Pressed, 1.f)); PC->InputKey(FInputKeyEventArgs::CreateSimulated(Key, IE_Released, 0.f)); };
        switch (Phase)
        {
        case 0:
            if (Frame < 10) break;
            {
                const auto* Bribe = Codex->GetMemories().FindByPredicate([](const FMemoriaCodexMemory& M) { return M.Id == TEXT("daily_market_food"); });
                Test->TestTrue(TEXT("The arrival's memories are archived"), Codex->GetMemories().Num() >= 5);
                Test->TestTrue(TEXT("The bribe's memory is marked burned"), Bribe && Bribe->bBurned);
            }
            for (TActorIterator<AMemoriaEliaCompanion> It(World); It; ++It) It->SetActorHiddenInGame(true);
            Combat->SpawnWave(1, Pawn->GetActorLocation(), 240.f);
            Press(EKeys::R);
            if (const int32 Hand = Combat->GetBurnChoices().IndexOfByPredicate([](const FMemoriaBurnChoice& C) { return C.Id == TEXT("rel_hand_reaching"); }); Hand >= 0) Combat->SelectBurn(Hand);
            Press(EKeys::Enter);
            ++Phase; Mark = Frame; break;
        case 1:
            if (const auto* Husk = Codex->FindEnemy(TEXT("Void Husk")); Husk && Husk->Defeated == 1)
            {
                Test->TestEqual(TEXT("One encounter"), Husk->Encounters, 1);
                Test->TestTrue(TEXT("It is a void foe"), Husk->bVoid);
                ++Phase; Mark = Frame;
            }
            if (Frame > Mark + 900) { Test->AddError(TEXT("The husk never fell")); return true; }
            break;
        case 2:
            if (Frame < Mark + 30 || Host->GetState() != EMemoriaSliceState::Exploration) break;
            Press(EKeys::Escape);
            if (auto* Menu = PC->GetPauseWidget())
            {
                int32 Row = -1; for (int32 I = 0; I < UMemoriaPauseWidget::ItemCount; ++I) if (Menu->ItemLabel(I) == TEXT("도감")) Row = I;
                Test->TestTrue(TEXT("The menu lists the Codex before Achievements"), Row >= 0 && Menu->ItemLabel(Row + 1) == TEXT("업적"));
                Menu->Select(Row); Press(EKeys::Enter);
            }
            ++Phase; Mark = Frame; break;
        case 3:
            if (Frame < Mark + 10) break;
            if (auto* Screen = PC->GetCodexWidget())
            {
                const auto Lines = Screen->ListLines();
                Test->TestTrue(TEXT("The bestiary counts what was recorded"), Lines.Num() > 2 && Lines[0].Label.StartsWith(TEXT("기록 1 /")));
                Test->TestTrue(TEXT("The husk leads the list"), Lines.Num() > 1 && Lines[1].Label.StartsWith(TEXT("보이드 허스크")));
                Test->TestTrue(TEXT("Its detail counts one defeat"), Screen->DetailBody().Contains(TEXT("처치: 1")));
                Capture(TEXT("CodexBestiary"));
            }
            else { Test->AddError(TEXT("The Codex row did not open the screen")); return true; }
            ++Phase; Mark = Frame; break;
        case 4:
            if (Frame < Mark + 6) break;
            Press(EKeys::Tab);
            Test->TestEqual(TEXT("TAB turns to the archive"), PC->GetCodexWidget()->GetTab(), 1);
            Press(EKeys::Down);
            Test->TestTrue(TEXT("A memory's detail shows its grade"), PC->GetCodexWidget()->DetailBody().Contains(TEXT("등급")));
            ++Phase; Mark = Frame; break;
        case 5:
            if (Frame == Mark + 8) Capture(TEXT("CodexArchive"));
            if (Frame < Mark + 12) break;
            Press(EKeys::Escape);
            Test->TestTrue(TEXT("ESC returns to the menu"), PC->GetCodexWidget() == nullptr && PC->GetPauseWidget() != nullptr);
            Press(EKeys::Escape);
            return true;
        }
        return false;
    }
private:
    FAutomationTestBase* Test;
    double Started, OldDelta = 0;
    uint64 LastFrame = MAX_uint64;
    int32 Phase = 0, Frame = 0, Mark = 0;
    bool bFixed = false, bOldFixed = false;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCodexVisualTest, "MemoriaVisual.Codex", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCodexVisualTest::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice"))) return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FCodexReplay(this)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
