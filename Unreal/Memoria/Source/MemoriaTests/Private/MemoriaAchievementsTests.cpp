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
#include "Combat/MemoriaFieldCombatSubsystem.h"
#include "Combat/MemoriaFieldCombatTypes.h"
#include "Achievements/MemoriaAchievementSubsystem.h"
#include "Achievements/MemoriaAchievementWidgets.h"
#include "Presentation/MemoriaPauseWidget.h"
#include "Save/MemoriaCheckpointSubsystem.h"
#include "Interaction/MemoriaEliaCompanion.h"
#include "EngineUtils.h"
#include "Misc/App.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"
#include "UObject/StrongObjectPtr.h"
#include "UnrealClient.h"
#if WITH_DEV_AUTOMATION_TESTS
// S324: achievement_manager.gd's rules. Each achievement unlocks once; unknown ids are refused; ten won
// battles make a veteran, a win at 10 HP or less a survivor; five maps an explorer; the seventh ending
// every path; a chapter's number its chapter achievement.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMemoriaAchievementRulesTest, "Memoria.Achievements.Rules", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMemoriaAchievementRulesTest::RunTest(const FString&)
{
    // A game instance subsystem lives in a game instance; this one is never initialized, so it never touches the file.
    TStrongObjectPtr<UGameInstance> Game(NewObject<UGameInstance>());
    TStrongObjectPtr<UMemoriaAchievementSubsystem> A(NewObject<UMemoriaAchievementSubsystem>(Game.Get()));
    TestEqual(TEXT("The source's 38 achievements"), UMemoriaAchievementSubsystem::All().Num(), 38);
    TestFalse(TEXT("An unknown id is refused"), A->Unlock(TEXT("not_an_achievement")));
    A->RecordBattleWon(80);
    TestTrue(TEXT("The first win is First Blood"), A->IsUnlocked(TEXT("first_blood")));
    TestFalse(TEXT("A win at 80 HP is no survival"), A->IsUnlocked(TEXT("survivor")));
    TestFalse(TEXT("An unlock happens once"), A->Unlock(TEXT("first_blood")));
    for (int32 I = 0; I < 8; ++I) A->RecordBattleWon(50);
    TestFalse(TEXT("Nine wins are not ten"), A->IsUnlocked(TEXT("battle_veteran")));
    A->RecordBattleWon(10);
    TestTrue(TEXT("Ten wins make a veteran"), A->IsUnlocked(TEXT("battle_veteran")) && A->GetBattlesWon() == 10);
    TestTrue(TEXT("A win at 10 HP is survival"), A->IsUnlocked(TEXT("survivor")));
    for (const TCHAR* Map : {TEXT("rim_forest"), TEXT("verdan_market"), TEXT("verdan_market"), TEXT("belt_waystation"), TEXT("drift_shelter")}) A->RecordMapVisit(Map);
    TestFalse(TEXT("Four maps are not five"), A->IsUnlocked(TEXT("explorer")));
    A->RecordMapVisit(TEXT("the_seam"));
    TestTrue(TEXT("Five maps make an explorer"), A->IsUnlocked(TEXT("explorer")));
    A->RecordChapterComplete(3);
    TestTrue(TEXT("A chapter's number names its achievement"), A->IsUnlocked(TEXT("chapter_complete_3")));
    for (const TCHAR* E : {TEXT("ending_zero"), TEXT("ending_preservation"), TEXT("ending_ash"), TEXT("ending_seam"), TEXT("ending_tobias"), TEXT("ending_hollow")}) A->Unlock(E);
    TestFalse(TEXT("Six endings are not every path"), A->IsUnlocked(TEXT("all_endings")));
    A->Unlock(TEXT("ending_weave"));
    TestTrue(TEXT("The seventh ending is every path"), A->IsUnlocked(TEXT("all_endings")));
    return true;
}
namespace
{
// S324 in the field: the arrival's bribe burn is First Burn; a husk burned down is First Blood, whose popup
// slides in; the pause menu's Achievements row opens the list with the unlocked titles and "???" for the
// rest; the arrows scroll it and ESC returns to the menu.
class FAchievementsReplay final : public IAutomationLatentCommand
{
public:
    explicit FAchievementsReplay(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
    ~FAchievementsReplay() override { if (bFixed) { FApp::SetUseFixedTimeStep(bOldFixed); FApp::SetFixedDeltaTime(OldDelta); } }
    bool Update() override
    {
        if (LastFrame == GFrameCounter) return false;
        LastFrame = GFrameCounter;
        if (FPlatformTime::Seconds() - Started > 150) { Test->AddError(FString::Printf(TEXT("Achievements timeout in phase %d"), Phase)); return true; }
        UWorld* World = GEditor->PlayWorld;
        auto* PC = World ? Cast<AMemoriaSliceController>(World->GetFirstPlayerController()) : nullptr;
        auto* Pawn = PC ? Cast<AMemoriaFieldPawn>(PC->GetPawn()) : nullptr;
        if (!Pawn || World->GetTimeSeconds() < .3) return false;
        auto* Game = World->GetGameInstance();
        auto* Host = Game->GetSubsystem<UMemoriaNarrativeSubsystem>();
        auto* Achievements = Game->GetSubsystem<UMemoriaAchievementSubsystem>();
        if (!bFixed)
        {
            bFixed = true; bOldFixed = FApp::UseFixedTimeStep(); OldDelta = FApp::GetFixedDeltaTime();
            FApp::SetUseFixedTimeStep(true); FApp::SetFixedDeltaTime(1.0 / 60.0);
            Game->GetSubsystem<UMemoriaCheckpointSubsystem>()->ConfigureTestStorage(TEXT("ach-") + FGuid::NewGuid().ToString(EGuidFormats::Digits));
            for (int32 I = 0; I < 20 && Host->GetState() == EMemoriaSliceState::VN; ++I) Host->Confirm(1);
            return false;
        }
        if (!World->GetMapName().EndsWith(TEXT("L_VerdanHost"))) return false;
        auto* Combat = World->GetSubsystem<UMemoriaFieldCombatSubsystem>();
        if (!Combat || !Combat->GetPlayer() || (Host->GetState() != EMemoriaSliceState::Exploration && Phase < 3)) return false;
        ++Frame;
        auto Capture = [&](const TCHAR* Name) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Validation/Achievements") / (FString(Name) + TEXT(".png")), true, false); };
        auto Press = [&](const FKey& Key) { PC->InputKey(FInputKeyEventArgs::CreateSimulated(Key, IE_Pressed, 1.f)); PC->InputKey(FInputKeyEventArgs::CreateSimulated(Key, IE_Released, 0.f)); };
        switch (Phase)
        {
        case 0:
            if (Frame < 10) break;
            Test->TestTrue(TEXT("The arrival's bribe burn is First Burn"), Achievements->IsUnlocked(TEXT("first_burn")));
            Test->TestTrue(TEXT("Verdan is a visited map"), Achievements->GetMapsVisited().Contains(TEXT("verdan_market")));
            for (TActorIterator<AMemoriaEliaCompanion> It(World); It; ++It) It->SetActorHiddenInGame(true);
            // One husk close by, burned down with the hand memory's ring.
            Combat->SpawnWave(1, Pawn->GetActorLocation(), 240.f);
            Press(EKeys::R);
            if (const int32 Hand = Combat->GetBurnChoices().IndexOfByPredicate([](const FMemoriaBurnChoice& C) { return C.Id == TEXT("rel_hand_reaching"); }); Hand >= 0) Combat->SelectBurn(Hand);
            Press(EKeys::Enter);
            ++Phase; Mark = Frame; break;
        case 1:
            if (Achievements->IsUnlocked(TEXT("first_blood")))
            {
                Test->TestEqual(TEXT("One battle won"), Achievements->GetBattlesWon(), 1);
                ++Phase; Mark = Frame;
            }
            if (Frame > Mark + 900) { Test->AddError(TEXT("The husk never fell")); return true; }
            break;
        case 2:
            // The popups: First Burn's first, then First Blood's.
            if (PC->GetAchievementPopup() && PC->GetAchievementPopup()->GetShownTitle() == TEXT("First Blood") && Achievements->GetPopupAge() > .6f)
            {
                Capture(TEXT("AchievementPopup"));
                ++Phase; Mark = Frame;
            }
            if (Frame > Mark + 1200) { Test->AddError(TEXT("First Blood's popup never showed")); return true; }
            break;
        case 3:
            if (Frame < Mark + 6) break;
            Press(EKeys::Escape);
            if (auto* Menu = PC->GetPauseWidget())
            {
                Test->TestEqual(TEXT("Achievements is the fifth row"), Menu->ItemLabel(4), FString(TEXT("업적")));
                Menu->Select(4); Press(EKeys::Enter);
            }
            ++Phase; Mark = Frame; break;
        case 4:
            if (Frame < Mark + 10) break;
            if (auto* List = PC->GetAchievementsWidget())
            {
                Test->TestEqual(TEXT("The header counts the unlocked"), List->HeaderText(), FString::Printf(TEXT("ACHIEVEMENTS  (%d / 38)"), Achievements->NumUnlocked()));
                Test->TestEqual(TEXT("First Blood is named"), List->RowTitle(0), FString(TEXT("First Blood")));
                Test->TestEqual(TEXT("A locked one is ???"), List->RowTitle(1), FString(TEXT("???")));
                Test->TestTrue(TEXT("The completion line is Korean"), List->ProgressText().StartsWith(TEXT("기억 속에")));
                Capture(TEXT("AchievementsList"));
            }
            else { Test->AddError(TEXT("The Achievements row did not open the list")); return true; }
            ++Phase; Mark = Frame; break;
        case 5:
            if (Frame < Mark + 6) break;
            for (int32 I = 0; I < 40; ++I) Press(EKeys::Down);
            Test->TestEqual(TEXT("The arrows scroll to the end"), PC->GetAchievementsWidget()->GetFirstRow(), 38 - PC->GetAchievementsWidget()->GetVisibleRows());
            ++Phase; Mark = Frame; break;
        case 6:
            if (Frame == Mark + 8) Capture(TEXT("AchievementsEnd"));
            if (Frame < Mark + 12) break;
            Press(EKeys::Escape);
            Test->TestTrue(TEXT("ESC returns to the menu"), PC->GetAchievementsWidget() == nullptr && PC->GetPauseWidget() != nullptr);
            Press(EKeys::Escape);
            Test->TestTrue(TEXT("and ESC again resumes"), PC->GetPauseWidget() == nullptr);
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
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAchievementsVisualTest, "MemoriaVisual.Achievements", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAchievementsVisualTest::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice"))) return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FAchievementsReplay(this)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
