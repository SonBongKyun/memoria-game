#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "Tests/AutomationCommon.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "UObject/StrongObjectPtr.h"
#include "Framework/MemoriaSliceHost.h"
#include "Framework/MemoriaFieldPawn.h"
#include "Narrative/MemoriaNarrativeSubsystem.h"
#include "Narrative/MemoriaNarrativeData.h"
#include "Narrative/MemoriaManuscript.h"
#include "Narrative/MemoriaNavigationGoal.h"
#include "Localization/MemoriaKorean.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Run/MemoriaRunTypes.h"
#include "Domain/MemoriaPlayerMemoryDomain.h"
#include "Domain/MemoriaChapterMemories.h"
#include "Chapter/MemoriaChapterMap.h"
#include "Chapter/MemoriaChapterPresentation.h"
#include "Chapter/MemoriaChapterCardWidget.h"
#include "Combat/MemoriaFieldCombatSubsystem.h"
#include "Combat/MemoriaFieldMonster.h"
#if WITH_DEV_AUTOMATION_TESTS

// S372: Chapter 7, the listening forest, from 최종본 8-10화: the map, its scenes, the road from the Seam.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FForestS372Test, "Memoria.S372.Forest", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FForestS372Test::RunTest(const FString&)
{
    using namespace MemoriaManuscript;
    const auto* Forest = MemoriaChapterMaps::Find(ForestMap);
    if (!TestNotNull(TEXT("The forest is a chapter map"), Forest)) return false;
    TestEqual(TEXT("Chapter 7"), Forest->Chapter, 7);
    TestEqual(TEXT("Its level"), MemoriaChapterMaps::LevelPath(ForestMap), FString(TEXT("/Game/Memoria/Maps/L_ForgottenForest")));
    TestEqual(TEXT("In Korean, the listening forest"), MemoriaChapterMaps::Korean(ForestMap), FString(TEXT("듣는 숲")));
    auto Open = [&](const FVector2D& P) { return !Forest->IsSolid(Forest->TileAt(int32(P.X) / Forest->TileSize, int32(P.Y) / Forest->TileSize)); };
    TestTrue(TEXT("They come in on the north road"), Open(Forest->Spawn) && Forest->Spawn.Y < 2 * Forest->TileSize);
    TestTrue(TEXT("and leave to the south"), Open(Forest->Exit.Rect.Center()) && Forest->Exit.Rect.Center().Y > 15 * Forest->TileSize);
    for (const auto& T : Forest->Triggers) TestTrue(*(TEXT("Open ground: ") + T.Group), Open(T.Rect.Center()));
    for (const auto& B : Forest->Battles) TestTrue(TEXT("The shape rises on open ground"), Open(B.Rect.Center()));
    TestEqual(TEXT("One shape between the trees"), Forest->Battles.Num(), 1);
    TestEqual(TEXT("after the lost hour"), Forest->BattlesGate, FString(TEXT("ch7_lost_hour")));
    TArray<FString> Groups; for (const auto& S : Forest->Sequence) Groups.Add(S.Group);
    for (const auto& T : Forest->Triggers) Groups.Add(T.Group);
    Groups.Add(Forest->Exit.Group);
    TestEqual(TEXT("Seven scenes"), Groups.Num(), 7);
    for (const FString& G : Groups) TestTrue(*(TEXT("A manuscript scene: ") + G), IsScene(G));
    TestEqual(TEXT("The arrival brings Chapter 8's forest memories"), Forest->Sequence[0].MemoriesChapter, 8);
    TestTrue(TEXT("which exist"), MemoriaChapterMemories::For(8).Num() >= 2);
    // The Seam's road now goes on into the forest.
    const auto* Seam = MemoriaChapterMaps::Find(SeamMap);
    TestTrue(TEXT("The Seam's exit leads into the forest"), Seam && Seam->Exit.NextMap == ForestMap && Seam->Exit.NextChapter == 7);

    // The scenes: the manuscript's turns.
    auto Has = [](const UMemoriaFieldAsset* A, const TCHAR* Speaker, const TCHAR* Ko)
    { return A && A->Definition.Rows.ContainsByPredicate([&](const FMemoriaFieldRow& R) { return (Speaker ? R.Text.Speaker == Speaker : !R.Text.bHasSpeaker) && (R.Text.TextKo == Ko || R.Text.NarrateKo == Ko); }); };
    TestTrue(TEXT("Sable's warning"), Has(BuildScene(GetTransientPackage(), TEXT("ms_forest_ridge"), 0), TEXT("Sable"), TEXT("덜 태우는 편이 낫겠지. 얼마나 더 갈지는 못 말해.")));
    TestTrue(TEXT("The trees move"), Has(BuildScene(GetTransientPackage(), TEXT("ms_forest_enter"), 0), TEXT("Arrel"), TEXT("엘리아. 나무가 움직여.")));
    TestTrue(TEXT("Don't hold on to it"), Has(BuildScene(GetTransientPackage(), TEXT("ms_forest_mother"), 0), TEXT("Elia"), TEXT("가지 마. 거기서 주는 거, 붙잡지 마.")));
    TestTrue(TEXT("Kairos observes, does not intervene"), Has(BuildScene(GetTransientPackage(), TEXT("ms_forest_mother"), 0), nullptr, TEXT("동쪽 오르막에 이르러 그는 붉은 노트를 다시 폈다. 한 줄을 더했다. 관찰함. 개입하지 않음.")));
    TestTrue(TEXT("I have to burn"), Has(BuildScene(GetTransientPackage(), TEXT("ms_forest_lost_hour"), 0), TEXT("Arrel"), TEXT("태워야겠어.")));
    const UMemoriaFieldAsset* Valley = BuildScene(GetTransientPackage(), TEXT("ms_forest_valley"), 0);
    TestTrue(TEXT("Elia gives the echo shell"), Has(Valley, TEXT("Elia"), TEXT("에코 셸.")) && Has(Valley, TEXT("Elia"), TEXT("오늘 밤 목소리는 내 거니까.")));
    TestTrue(TEXT("and it speaks"), Has(Valley, TEXT("Arrel"), TEXT("세는 게 틀렸다고 했어. ...그게 다야.")));
    bool bRows = true;
    for (const FString& G : SceneGroups())
        if (G.StartsWith(TEXT("ms_forest_")))
            if (const auto* A = BuildScene(GetTransientPackage(), G, 0))
                for (int32 I = 0; I < A->Definition.Rows.Num(); ++I)
                {
                    const auto& R = A->Definition.Rows[I];
                    bRows &= R.OriginalIndex == I && (R.Text.bHasTextKo || R.Text.bHasNarrateKo) && (R.Text.bHasText || R.Text.bHasNarrate);
                    // Sable is blind: no line of hers or about her has her look at anything.
                    if (R.Text.Speaker == TEXT("Sable")) bRows &= !R.Text.TextKo.Contains(TEXT("보았")) && !R.Text.TextKo.Contains(TEXT("봤"));
                }
    TestTrue(TEXT("Rows in both languages, indexed"), bRows);
    TestEqual(TEXT("Seven forest scenes"), SceneGroups().FilterByPredicate([](const FString& G) { return G.StartsWith(TEXT("ms_forest_")); }).Num(), 7);

    // Goals and the main quest.
    TStrongObjectPtr<UGameInstance> Game(NewObject<UGameInstance>()); Game->Init();
    auto* Run = Game->GetSubsystem<UMemoriaRunSubsystem>();
    if (!TestTrue(TEXT("A run at Chapter 7"), Run->BeginStartingMemoryRun(7) == EMemoriaMemoryResult::Success)) { Game->Shutdown(); return false; }
    Run->SetLocale(TEXT("ko"));
    for (const TCHAR* F : {TEXT("ch7_ridge"), TEXT("ch7_forest_enter")}) Run->SetStoryFlag(F, true);
    TestEqual(TEXT("Into the forest"), MemoriaNavigationGoal::Line(Run->GetRunSnapshot(), ForestMap, false), FString(TEXT("목표  곧은 길을 따라 숲 안쪽으로. 소리 쪽으로 고개를 돌리지 말 것")));
    Run->SetStoryFlag(TEXT("ch7_lost_hour"), true);
    TestEqual(TEXT("Out of it"), MemoriaNavigationGoal::Line(Run->GetRunSnapshot(), ForestMap, false), FString(TEXT("목표  숲을 빠져나간다: 남쪽으로")));
    Run->SetStoryFlag(TEXT("ch7_escape"), true);
    TestEqual(TEXT("Down to the ridges"), MemoriaNavigationGoal::Line(Run->GetRunSnapshot(), ForestMap, false), FString(TEXT("목표  남쪽 출구 → 두 돌 능선 사이")));
    for (const TCHAR* F : {TEXT("ch2_arrived"), TEXT("ch2_malet_done"), TEXT("ch3_waystation_night"), TEXT("ch3_class_seven_message"), TEXT("ch4_anchoring"),
        TEXT("ch4_night_watch"), TEXT("ch6_arrived"), TEXT("ch6_sable_morning"), TEXT("ch6_complete")}) Run->SetStoryFlag(F, true);
    TestTrue(TEXT("The main quest is at the slab"), MainRecord(Run->GetRunSnapshot(), true).Body.Contains(TEXT("→ 두 돌 능선 사이, 기울어진 돌판 아래에서 밤")));
    Run->SetStoryFlag(TEXT("ch7_complete"), true);
    // S373: then the coast.
    TestTrue(TEXT("then at the coast"), MainRecord(Run->GetRunSnapshot(), true).Body.Contains(TEXT("→ 붕괴의 해안, 자갈밭의 구멍")));
    Game->Shutdown();
    return true;
}

namespace
{
// S372 in the renderer: a development run enters the forest at the ridge; the arrival chain plays, then the
// echoes, the mother's voice (with Kairos at the Seam), the lost hour; the shape rises between the trees;
// the escape; and the night under the slab, where Elia gives the echo shell.
class FForestReplay final : public IAutomationLatentCommand
{
public:
    explicit FForestReplay(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
    ~FForestReplay() override { if (bFixed) { FApp::SetUseFixedTimeStep(bOldFixed); FApp::SetFixedDeltaTime(OldDelta); } }
    bool Update() override
    {
        if (LastFrame == GFrameCounter) return false;
        LastFrame = GFrameCounter;
        if (FPlatformTime::Seconds() - Started > 360) { Test->AddError(FString::Printf(TEXT("S372 timeout at step %d"), Step)); return true; }
        UWorld* World = GEditor->PlayWorld;
        auto* PC = World ? Cast<AMemoriaSliceController>(World->GetFirstPlayerController()) : nullptr;
        auto* Pawn = PC ? Cast<AMemoriaFieldPawn>(PC->GetPawn()) : nullptr;
        if (!Pawn || World->GetTimeSeconds() < .2) return false;
        if (!bFixed) { bFixed = true; bOldFixed = FApp::UseFixedTimeStep(); OldDelta = FApp::GetFixedDeltaTime(); FApp::SetUseFixedTimeStep(true); FApp::SetFixedDeltaTime(1.0 / 60.0); }
        ++Frame;
        auto* Host = World->GetGameInstance()->GetSubsystem<UMemoriaNarrativeSubsystem>();
        auto* Run = World->GetGameInstance()->GetSubsystem<UMemoriaRunSubsystem>();
        auto* Combat = World->GetSubsystem<UMemoriaFieldCombatSubsystem>();
        AMemoriaChapterPresentation* Map = nullptr; for (TActorIterator<AMemoriaChapterPresentation> It(World); It; ++It) Map = *It;
        const auto* Spec = Map ? Map->GetSpec() : nullptr;
        if (!Spec) return false;
        auto Capture = [&](const FString& Name) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Validation/S372Forest") / (Name + TEXT(".png")), true, false); };
        auto Flag = [&](const FString& Id) { return Run->GetRunSnapshot().GetFlag(Id); };
        auto Place = [&](const FVector2D& Source) { Pawn->SetActorLocation(MemoriaChapterMaps::ToWorld(Source) + FVector(0, 0, Pawn->GetActorLocation().Z)); };
        if (Host->GetState() == EMemoriaSliceState::Field)
        {
            const FMemoriaNarrativeView View = Host->GetView();
            for (const auto& Shot : Shots)
                if (!Taken.Contains(Shot.Key) && View.CueKey == Shot.Key)
                {
                    Taken.Add(Shot.Key); ShotAt = Frame; ShotName = Shot.Value;
                    Test->TestFalse(*(TEXT("A line shows: ") + Shot.Key), MemoriaKorean::Text(View.Body.IsEmpty() ? View.Narration : View.Body).IsEmpty());
                }
            if (!ShotName.IsEmpty() && Frame == ShotAt + 20) { Capture(ShotName); ShotName.Reset(); return false; }
            if (ShotName.IsEmpty() && Frame % 6 == 0) Host->Confirm();
            return false;
        }
        auto Next = [&](const TCHAR* Done, int32 TriggerIndex)
        {
            if (Flag(Done) && Host->GetState() == EMemoriaSliceState::Exploration && Frame > Mark + 20)
            {
                if (TriggerIndex >= 0) Place(Spec->Triggers[TriggerIndex].Rect.Center());
                ++Step; Mark = Frame; return true;
            }
            if (Frame > Mark + 6000) { Test->AddError(FString::Printf(TEXT("Stuck before %s"), Done)); Step = 99; }
            return false;
        };
        switch (Step)
        {
        case 0:
            Test->TestEqual(TEXT("The map is the forest"), Map->GetMap(), FString(MemoriaManuscript::ForestMap));
            Test->TestEqual(TEXT("A development run in Chapter 7"), Run->GetRunSnapshot().CurrentChapter, int64(7));
            ++Step; Mark = Frame; break;
        case 1:
            if (Frame == Mark + 40) Capture(TEXT("S372Title"));
            if (Flag(TEXT("ch7_forest_enter")) && Host->GetState() == EMemoriaSliceState::Exploration && Frame > Mark + 60)
            {
                Test->TestTrue(TEXT("The forest's memories came with the arrival"), Run->GetPlayerMemory()->GetSnapshot().Owned.ContainsByPredicate([](const auto& M) { return M.Id == TEXT("sense_hollow_trees"); }));
                Capture(TEXT("S372Forest"));
                Place(Spec->Triggers[0].Rect.Center());
                ++Step; Mark = Frame;
            }
            if (Frame > Mark + 6000) { Test->AddError(TEXT("The arrival chain did not finish")); return true; }
            break;
        case 2: Next(TEXT("ch7_echoes"), 1); break;
        case 3: Next(TEXT("ch7_mother"), 2); break;
        case 4:
            if (Next(TEXT("ch7_lost_hour"), -1)) Place(Spec->Battles[0].Rect.Center());
            break;
        case 5:
            // The shape between the trees rises where the lost hour ends.
            if (Combat && Combat->LiveMonsterCount() > 0)
            {
                Test->TestTrue(TEXT("The shape's battle area is spent"), Flag(TEXT("battle_forgotten_forest_1")));
                if (Frame > Mark + 20) { Capture(TEXT("S372Shape"));
                    for (const auto& M : Combat->GetMonsters()) if (M.IsValid()) M->Destroy();
                    Place(Spec->Triggers[3].Rect.Center()); ++Step; Mark = Frame; }
            }
            else if (Frame > Mark + 600) { Test->AddError(TEXT("The shape never rose")); return true; }
            break;
        case 6:
            if (Next(TEXT("ch7_escape"), -1)) Place(Spec->Exit.Rect.Center());
            break;
        case 7:
            if (Map->IsChapterComplete())
            {
                Test->TestTrue(TEXT("The night under the slab closes Chapter 7"), Flag(TEXT("ch7_complete")) && Flag(TEXT("canon_ch8_coast_ready")));
                Test->TestTrue(TEXT("Elia gave the echo shell"), Run->GetPlayerMemory()->GetSnapshot().Owned.ContainsByPredicate([](const auto& M) { return M.Id == MemoriaManuscript::EchoShellMemory; })
                    && Host->GetTrace().Contains(TEXT("toast:Obtained: Echo shell")));
                Test->TestTrue(TEXT("Its last line was shown"), Taken.Contains(TEXT("field:ms_forest_valley:21")));
                Test->TestTrue(TEXT("The card shows"), Map->GetCard() && Map->GetCard()->IsShowing());
                ++Step; Mark = Frame;
            }
            if (Frame > Mark + 6000) { Test->AddError(TEXT("The exit never closed the chapter")); return true; }
            break;
        case 8:
            if (Frame == Mark + 30) Capture(TEXT("S372Complete"));
            if (Frame < Mark + 40) break;
            return true;
        default:
            return true;
        }
        return false;
    }
private:
    FAutomationTestBase* Test;
    double Started, OldDelta = 0;
    uint64 LastFrame = MAX_uint64;
    int32 Step = 0, Frame = 0, Mark = 0, ShotAt = 0;
    bool bFixed = false, bOldFixed = false;
    FString ShotName;
    TSet<FString> Taken;
    const TArray<TPair<FString, FString>> Shots = {
        {TEXT("field:ms_forest_ridge:7"), TEXT("S372LessBurning")}, {TEXT("field:ms_forest_enter:6"), TEXT("S372TreesMove")},
        {TEXT("field:ms_forest_echoes:2"), TEXT("S372Oven")}, {TEXT("field:ms_forest_mother:4"), TEXT("S372DontGo")},
        {TEXT("field:ms_forest_mother:19"), TEXT("S372Kairos")}, {TEXT("field:ms_forest_lost_hour:13"), TEXT("S372IHaveToBurn")},
        {TEXT("field:ms_forest_escape:8"), TEXT("S372SomethingElse")}, {TEXT("field:ms_forest_valley:11"), TEXT("S372EchoShell")},
        {TEXT("field:ms_forest_valley:21"), TEXT("S372SayItAgain")}};
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FForestVisualTest, "MemoriaVisual.S372Forest", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FForestVisualTest::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Memoria/Maps/L_ForgottenForest"))) return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FForestReplay(this)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
