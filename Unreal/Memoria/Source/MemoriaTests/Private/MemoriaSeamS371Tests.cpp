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
#if WITH_DEV_AUTOMATION_TESTS

// S371: Chapter 6, the Seam, from 최종본 6-7화: the map, its scenes, the road from Drift and the goals.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSeamS371Test, "Memoria.S371.Seam", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSeamS371Test::RunTest(const FString&)
{
    using namespace MemoriaManuscript;
    const auto* Seam = MemoriaChapterMaps::Find(SeamMap);
    if (!TestNotNull(TEXT("The Seam is a chapter map"), Seam)) return false;
    TestEqual(TEXT("Chapter 6"), Seam->Chapter, 6);
    TestEqual(TEXT("Its level"), MemoriaChapterMaps::LevelPath(SeamMap), FString(TEXT("/Game/Memoria/Maps/L_TheSeam")));
    auto Open = [&](const FVector2D& P) { return !Seam->IsSolid(Seam->TileAt(int32(P.X) / Seam->TileSize, int32(P.Y) / Seam->TileSize)); };
    TestTrue(TEXT("They arrive on open ground"), Open(Seam->Spawn));
    TestTrue(TEXT("The exit is on open ground"), Open(Seam->Exit.Rect.Center()));
    for (const auto& Trigger : Seam->Triggers) TestTrue(TEXT("Each story place is on open ground"), Open(Trigger.Rect.Center()));
    // Every group the map plays is a manuscript scene.
    TArray<FString> Groups; for (const auto& S : Seam->Sequence) Groups.Add(S.Group);
    for (const auto& T : Seam->Triggers) Groups.Add(T.Group);
    Groups.Add(Seam->Exit.Group);
    TestEqual(TEXT("Six scenes"), Groups.Num(), 6);
    for (const FString& G : Groups) TestTrue(*(TEXT("A manuscript scene: ") + G), IsScene(G));
    TestEqual(TEXT("and no other Seam scenes"), SceneGroups().FilterByPredicate([](const FString& G) { return G.StartsWith(TEXT("ms_seam_")); }).Num(), 6);
    TestEqual(TEXT("The arrival brings Chapter 6's memories"), Seam->Sequence[0].MemoriesChapter, 6);
    TestTrue(TEXT("Chapter 6 has its memories"), MemoriaChapterMemories::For(6).Num() == 3);
    TestTrue(TEXT("and Chapter 7 the echo shell"), MemoriaChapterMemories::For(7).ContainsByPredicate([](const FMemoriaMemoryDefinition& M) { return M.Id == EchoShellMemory; }));
    TestEqual(TEXT("The place in Korean"), MemoriaChapterMaps::Korean(SeamMap), FString(TEXT("더 심")));

    // The scenes: the manuscript's lines, and Sable's answer to how much he has burned.
    auto Ko = [](const UMemoriaFieldAsset* A, int32 I) { return A && A->Definition.Rows.IsValidIndex(I) ? (A->Definition.Rows[I].Text.bHasTextKo ? A->Definition.Rows[I].Text.TextKo : A->Definition.Rows[I].Text.NarrateKo) : FString(); };
    const UMemoriaFieldAsset* Name = BuildScene(GetTransientPackage(), TEXT("ms_seam_name"), 0);
    TestTrue(TEXT("Arrel loses Elia's name on the bridge"), Name && Name->Definition.Rows.ContainsByPredicate([](const FMemoriaFieldRow& R) { return R.Text.TextKo == TEXT("네 이름이 안 나와."); }));
    const UMemoriaFieldAsset* Night = BuildScene(GetTransientPackage(), TEXT("ms_seam_night"), 0);
    const UMemoriaFieldAsset* HeavyNight = BuildScene(GetTransientPackage(), TEXT("ms_seam_night"), SableHeavyBurns);
    TestEqual(TEXT("Months: three"), Ko(Night, 10), FString(TEXT("몇 달. 셋. 지금 정도로 버티면 넷까지 갈 수 있겠지. 더 크게 태우면 그보다 짧고.")));
    TestTrue(TEXT("Burned more, two"), Ko(HeavyNight, 10).StartsWith(TEXT("몇 달. 둘.")));
    TestEqual(TEXT("Sable says it"), Night ? Night->Definition.Rows[10].Text.Speaker : FString(), FString(TEXT("Sable")));
    const UMemoriaFieldAsset* Leave = BuildScene(GetTransientPackage(), TEXT("ms_seam_departure"), 0);
    TestEqual(TEXT("The chapter ends on her line"), Leave ? Ko(Leave, Leave->Definition.Rows.Num() - 1) : FString(), FString(TEXT("열일곱은 아니야.")));
    bool bIndexed = true, bArt = true;
    for (const FString& G : SceneGroups())
        if (const auto* A = BuildScene(GetTransientPackage(), G, 0))
            for (int32 I = 0; I < A->Definition.Rows.Num(); ++I)
            {
                const auto& R = A->Definition.Rows[I];
                bIndexed &= R.OriginalIndex == I && R.Id == FString::Printf(TEXT("field/%s/%d"), *G, I) && (R.Text.bHasTextKo || R.Text.bHasNarrateKo) && (R.Text.bHasText || R.Text.bHasNarrate);
                // S385/S390: an empty picture plays on black (Chapter 14's return from Tobias's desk, the sheep pen,
                // the old furrows): only where that is the choice.
                bArt &= !R.Presentation.bHasCg || !R.Presentation.Cg.IsEmpty() || G.StartsWith(TEXT("ms_ridge_")) || G.StartsWith(TEXT("ms_leave_")) || G.StartsWith(TEXT("ms_east_")) || G.StartsWith(TEXT("ms_storm_")) || G == TEXT("ms_arcane_room") || G == TEXT("ms_basin_pen") || G == TEXT("ms_verdan_furrow") || G == TEXT("ms_kairos_relay");
            }
    TestTrue(TEXT("Rows carry their indices and both languages"), bIndexed && bArt);
    TestNull(TEXT("Unknown group"), BuildScene(GetTransientPackage(), TEXT("ms_nowhere"), 0));

    // The road from Drift, and the goals in the village.
    TStrongObjectPtr<UGameInstance> Game(NewObject<UGameInstance>()); Game->Init();
    auto* Run = Game->GetSubsystem<UMemoriaRunSubsystem>();
    if (!TestTrue(TEXT("A run at Chapter 5"), Run->BeginStartingMemoryRun(5) == EMemoriaMemoryResult::Success)) { Game->Shutdown(); return false; }
    Run->SetLocale(TEXT("ko"));
    TestTrue(TEXT("No road on before the classifier is done"), RoadOnward(TEXT("drift_shelter"), Run->GetRunSnapshot()).IsEmpty());
    Run->SetStoryFlag(TEXT("ch5_classifier_started"), true); Run->SetStoryFlag(TEXT("canon_ch6_seam_ready"), true);
    TestEqual(TEXT("After it, Drift's east road goes to the Seam"), RoadOnward(TEXT("drift_shelter"), Run->GetRunSnapshot()), FString(SeamMap));
    TestEqual(TEXT("The goal says so"), MemoriaNavigationGoal::Line(Run->GetRunSnapshot(), TEXT("drift_shelter"), false), FString(TEXT("목표  동쪽 출구 → 더 심")));
    TestTrue(TEXT("Only from Drift"), RoadOnward(TEXT("belt_waystation"), Run->GetRunSnapshot()).IsEmpty());
    Run->SetCurrentChapter(6);
    for (const TCHAR* F : {TEXT("ch6_arrived"), TEXT("ch6_name_slip"), TEXT("ch6_feren")}) Run->SetStoryFlag(F, true);
    TestEqual(TEXT("Rest in the inner room"), MemoriaNavigationGoal::Line(Run->GetRunSnapshot(), SeamMap, false), FString(TEXT("목표  마을 가운데 집의 안쪽 방에서 쉰다")));
    Run->SetStoryFlag(TEXT("ch6_sable_night"), true);
    TestEqual(TEXT("Then the morning"), MemoriaNavigationGoal::Line(Run->GetRunSnapshot(), SeamMap, false), FString(TEXT("목표  아침: 개울 북쪽 집에서 세이블에게 길을 묻는다")));
    Run->SetStoryFlag(TEXT("ch6_sable_morning"), true);
    TestEqual(TEXT("Then the ridge road"), MemoriaNavigationGoal::Line(Run->GetRunSnapshot(), SeamMap, false), FString(TEXT("목표  북동쪽 능선길 → 하렌을 따라")));
    for (const TCHAR* F : {TEXT("ch2_arrived"), TEXT("ch2_malet_done"), TEXT("ch3_waystation_night"), TEXT("ch3_class_seven_message"), TEXT("ch4_anchoring"), TEXT("ch4_night_watch")}) Run->SetStoryFlag(F, true);
    TestEqual(TEXT("The main quest is at the ridge road"), MainStepIndex(Run->GetRunSnapshot()), 8);
    Run->SetStoryFlag(TEXT("ch6_complete"), true);
    const auto Main = MainRecord(Run->GetRunSnapshot(), true);
    // S372: after the ridge road comes the listening forest.
    TestTrue(TEXT("and then into the listening forest"), Main.Body.Contains(TEXT("→ 듣는 숲을 지난다")) && Main.Body.Contains(TEXT("세이블에게 길을 묻는다  (완료)")));
    Game->Shutdown();
    return true;
}

namespace
{
// S371 in the renderer: a development run enters the Seam; the arrival chain plays (the name lost on the
// bridge), the night in the inner room gives the spiralwave shell, the morning gives the bark map, and
// Haren's road closes the chapter on Sable's "열일곱은 아니야."
class FSeamReplay final : public IAutomationLatentCommand
{
public:
    explicit FSeamReplay(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
    ~FSeamReplay() override { if (bFixed) { FApp::SetUseFixedTimeStep(bOldFixed); FApp::SetFixedDeltaTime(OldDelta); } }
    bool Update() override
    {
        if (LastFrame == GFrameCounter) return false;
        LastFrame = GFrameCounter;
        if (FPlatformTime::Seconds() - Started > 300) { Test->AddError(FString::Printf(TEXT("S371 timeout at step %d"), Step)); return true; }
        UWorld* World = GEditor->PlayWorld;
        auto* PC = World ? Cast<AMemoriaSliceController>(World->GetFirstPlayerController()) : nullptr;
        auto* Pawn = PC ? Cast<AMemoriaFieldPawn>(PC->GetPawn()) : nullptr;
        if (!Pawn || World->GetTimeSeconds() < .2) return false;
        if (!bFixed) { bFixed = true; bOldFixed = FApp::UseFixedTimeStep(); OldDelta = FApp::GetFixedDeltaTime(); FApp::SetUseFixedTimeStep(true); FApp::SetFixedDeltaTime(1.0 / 60.0); }
        ++Frame;
        auto* Host = World->GetGameInstance()->GetSubsystem<UMemoriaNarrativeSubsystem>();
        auto* Run = World->GetGameInstance()->GetSubsystem<UMemoriaRunSubsystem>();
        AMemoriaChapterPresentation* Map = nullptr; for (TActorIterator<AMemoriaChapterPresentation> It(World); It; ++It) Map = *It;
        const auto* Spec = Map ? Map->GetSpec() : nullptr;
        if (!Spec) return false;
        auto Capture = [&](const FString& Name) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Validation/S371Seam") / (Name + TEXT(".png")), true, false); };
        auto Flag = [&](const FString& Id) { return Run->GetRunSnapshot().GetFlag(Id); };
        auto Place = [&](const FVector2D& Source) { Pawn->SetActorLocation(MemoriaChapterMaps::ToWorld(Source) + FVector(0, 0, Pawn->GetActorLocation().Z)); };
        if (Host->GetState() == EMemoriaSliceState::Field)
        {
            const FMemoriaNarrativeView View = Host->GetView();
            // One frame of each scene on screen: the line the manuscript gives it.
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
        switch (Step)
        {
        case 0:
            Test->TestEqual(TEXT("The map is the Seam"), Map->GetMap(), FString(MemoriaManuscript::SeamMap));
            Test->TestEqual(TEXT("A development run in Chapter 6"), Run->GetRunSnapshot().CurrentChapter, int64(6));
            ++Step; Mark = Frame; break;
        case 1:
            if (Frame == Mark + 40) Capture(TEXT("S371Title"));
            if (Flag(TEXT("ch6_feren")) && Host->GetState() == EMemoriaSliceState::Exploration && Frame > Mark + 60)
            {
                Test->TestTrue(TEXT("Arrival, the name and Feren"), Flag(TEXT("ch6_arrived")) && Flag(TEXT("ch6_name_slip")));
                Test->TestTrue(TEXT("Chapter 6's memories came with the arrival"), Run->GetPlayerMemory()->GetSnapshot().Owned.ContainsByPredicate([](const auto& M) { return M.Id == TEXT("rel_sable_trust"); }));
                Test->TestTrue(TEXT("The bridge scene was shown"), Taken.Contains(TEXT("field:ms_seam_name:5")));
                Test->TestEqual(TEXT("The goal points to the inner room"), MemoriaKorean::Text(Host->GetQuestTrackerLine()), FString(TEXT("목표  마을 가운데 집의 안쪽 방에서 쉰다")));
                Capture(TEXT("S371Village"));
                ++Step; Mark = Frame;
            }
            if (Frame > Mark + 6000) { Test->AddError(TEXT("The arrival chain did not finish")); return true; }
            break;
        case 2:
            if (Frame < Mark + 20) break;
            Place(Spec->Triggers[0].Rect.Center());
            ++Step; Mark = Frame; break;
        case 3:
            if (Flag(TEXT("ch6_sable_night")) && Host->GetState() == EMemoriaSliceState::Exploration)
            {
                // S372: Sable gives the small spiralwave; the echo shell is Elia's to give, in Chapter 7 (10화).
                Test->TestTrue(TEXT("Sable gave him the spiralwave"), Flag(TEXT("has_spiralwave")));
                Test->TestFalse(TEXT("not the echo shell"), Run->GetPlayerMemory()->GetSnapshot().Owned.ContainsByPredicate([](const auto& M) { return M.Id == MemoriaManuscript::EchoShellMemory; }));
                Test->TestTrue(TEXT("Its toast"), Host->GetTrace().Contains(TEXT("toast:Obtained: Spiralwave shell")));
                Capture(TEXT("S371ShellToast"));
                Place(Spec->Triggers[1].Rect.Center());
                ++Step; Mark = Frame;
            }
            if (Frame > Mark + 6000) { Test->AddError(TEXT("The night never played")); return true; }
            break;
        case 4:
            if (Flag(TEXT("ch6_sable_morning")) && Host->GetState() == EMemoriaSliceState::Exploration)
            {
                Test->TestTrue(TEXT("The bark map"), Flag(TEXT("has_bark_map")) && Host->GetTrace().Contains(TEXT("toast:Obtained: Sable's bark map")));
                Place(Spec->Exit.Rect.Center());
                ++Step; Mark = Frame;
            }
            if (Frame > Mark + 6000) { Test->AddError(TEXT("The morning never played")); return true; }
            break;
        case 5:
            if (Map->IsChapterComplete())
            {
                Test->TestTrue(TEXT("Haren's road closes Chapter 6"), Flag(TEXT("ch6_complete")) && Flag(TEXT("canon_ch7_ridge_ready")));
                Test->TestTrue(TEXT("Sable's last line was shown"), Taken.Contains(TEXT("field:ms_seam_departure:7")));
                Test->TestTrue(TEXT("The card shows"), Map->GetCard() && Map->GetCard()->IsShowing());
                ++Step; Mark = Frame;
            }
            if (Frame > Mark + 6000) { Test->AddError(TEXT("The exit never closed the chapter")); return true; }
            break;
        case 6:
            if (Frame == Mark + 30) Capture(TEXT("S371Complete"));
            if (Frame < Mark + 40) break;
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
        {TEXT("field:ms_seam_arrival:6"), TEXT("S371Fern")}, {TEXT("field:ms_seam_name:5"), TEXT("S371Name")},
        {TEXT("field:ms_seam_feren:4"), TEXT("S371Feren")}, {TEXT("field:ms_seam_night:10"), TEXT("S371Months")},
        {TEXT("field:ms_seam_night:15"), TEXT("S371Shell")}, {TEXT("field:ms_seam_morning:5"), TEXT("S371Daughter")},
        {TEXT("field:ms_seam_departure:7"), TEXT("S371NotSeventeen")}};
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSeamVisualTest, "MemoriaVisual.S371Seam", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSeamVisualTest::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Memoria/Maps/L_TheSeam"))) return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FSeamReplay(this)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
