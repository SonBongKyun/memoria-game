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
#include "Chapter/MemoriaChapterMap.h"
#include "Chapter/MemoriaChapterPresentation.h"
#include "Chapter/MemoriaChapterCardWidget.h"
#if WITH_DEV_AUTOMATION_TESTS

// S373: Chapter 8, the Crumbling Coast and Blank 7, from 최종본 11-12화.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoastS373Test, "Memoria.S373.Coast", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCoastS373Test::RunTest(const FString&)
{
    using namespace MemoriaManuscript;
    const auto* Coast = MemoriaChapterMaps::Find(CoastMap);
    if (!TestNotNull(TEXT("The coast is a chapter map"), Coast)) return false;
    TestEqual(TEXT("Chapter 8"), Coast->Chapter, 8);
    TestEqual(TEXT("Its level"), MemoriaChapterMaps::LevelPath(CoastMap), FString(TEXT("/Game/Memoria/Maps/L_CrumblingCoast")));
    TestEqual(TEXT("In Korean"), MemoriaChapterMaps::Korean(CoastMap), FString(TEXT("붕괴의 해안")));
    TestTrue(TEXT("Its colours drain"), Coast->Saturation < .5f);
    auto Open = [&](const FVector2D& P) { return !Coast->IsSolid(Coast->TileAt(int32(P.X) / Coast->TileSize, int32(P.Y) / Coast->TileSize)); };
    TestTrue(TEXT("They come in from the west"), Open(Coast->Spawn) && Coast->Spawn.X < 2 * Coast->TileSize);
    TestTrue(TEXT("and leave north"), Open(Coast->Exit.Rect.Center()) && Coast->Exit.Rect.Center().Y < 2 * Coast->TileSize);
    for (const auto& T : Coast->Triggers) TestTrue(*(TEXT("Open ground: ") + T.Group), Open(T.Rect.Center()));
    // The hole stands where the pool was: the city trigger is at its edge.
    const int32 Water = Coast->TileNames.IndexOfByKey(TEXT("WATER"));
    const auto& City = Coast->Triggers[1];
    TestTrue(TEXT("The city is entered at the hole's edge"), City.Group == TEXT("ms_coast_city") && Coast->TileAt(int32(City.Rect.Center().X) / Coast->TileSize, int32(City.Rect.Center().Y) / Coast->TileSize - 2) == Water);
    TArray<FString> Groups; for (const auto& S : Coast->Sequence) Groups.Add(S.Group);
    for (const auto& T : Coast->Triggers) Groups.Add(T.Group);
    Groups.Add(Coast->Exit.Group);
    TestEqual(TEXT("Six scenes"), Groups.Num(), 6);
    for (const FString& G : Groups) TestTrue(*(TEXT("A manuscript scene: ") + G), IsScene(G));
    const auto* Forest = MemoriaChapterMaps::Find(ForestMap);
    TestTrue(TEXT("The forest's road leads to the coast"), Forest && Forest->Exit.NextMap == CoastMap && Forest->Exit.NextChapter == 8);

    auto Has = [](const UMemoriaFieldAsset* A, const TCHAR* Speaker, const TCHAR* Ko)
    { return A && A->Definition.Rows.ContainsByPredicate([&](const FMemoriaFieldRow& R) { return (Speaker ? R.Text.Speaker == Speaker : !R.Text.bHasSpeaker) && (R.Text.TextKo == Ko || R.Text.NarrateKo == Ko); }); };
    TestTrue(TEXT("Nine years"), Has(BuildScene(GetTransientPackage(), TEXT("ms_coast_shore"), 0), TEXT("Elia"), TEXT("아홉 해가 지났는데.")));
    TestTrue(TEXT("Blank 7"), Has(BuildScene(GetTransientPackage(), TEXT("ms_coast_hole"), 0), TEXT("Arrel"), TEXT("공백 7호?")));
    const auto* CityScene = BuildScene(GetTransientPackage(), TEXT("ms_coast_city"), 0);
    TestTrue(TEXT("No loneliness"), Has(CityScene, TEXT("Elia"), TEXT("전부가 전부였어. 동시에. ...외로움이 없었어.")));
    TestTrue(TEXT("I can't feel you"), Has(CityScene, TEXT("Elia"), TEXT("조용해. 소리 말고. ...네가 안 느껴져. 도시는 와. 네 것은 안 와.")));
    TestTrue(TEXT("I will not stop you yet"), Has(BuildScene(GetTransientPackage(), TEXT("ms_coast_tree"), 0), nullptr, TEXT("표식 아래에 작은 글씨가 있었다. 아직 너를 막지 않겠다.")));
    const auto* Chair = BuildScene(GetTransientPackage(), TEXT("ms_coast_chair"), 0);
    TestTrue(TEXT("The man in the brown coat, unnamed"), Has(Chair, TEXT("Brown Coat"), TEXT("강 전까지 길은 두 번 더 갈라진다. 북쪽에서는 비가 그쳤다.")) && !Chair->Definition.Rows.ContainsByPredicate([](const FMemoriaFieldRow& R) { return R.Text.Speaker == TEXT("Kairos"); }));
    bool bRows = true;
    for (const FString& G : SceneGroups())
        if (G.StartsWith(TEXT("ms_coast_")))
            if (const auto* A = BuildScene(GetTransientPackage(), G, 0))
                for (int32 I = 0; I < A->Definition.Rows.Num(); ++I)
                    bRows &= A->Definition.Rows[I].OriginalIndex == I && (A->Definition.Rows[I].Text.bHasTextKo || A->Definition.Rows[I].Text.bHasNarrateKo);
    TestTrue(TEXT("Rows indexed, in Korean"), bRows);
    TestEqual(TEXT("Six coast scenes"), SceneGroups().FilterByPredicate([](const FString& G) { return G.StartsWith(TEXT("ms_coast_")); }).Num(), 6);
    TestEqual(TEXT("The speaker's Korean name"), MemoriaKorean::Text(TEXT("Lost: fear")), FString(TEXT("잃음: 공포")));

    TStrongObjectPtr<UGameInstance> Game(NewObject<UGameInstance>()); Game->Init();
    auto* Run = Game->GetSubsystem<UMemoriaRunSubsystem>();
    if (!TestTrue(TEXT("A run at Chapter 8"), Run->BeginStartingMemoryRun(8) == EMemoriaMemoryResult::Success)) { Game->Shutdown(); return false; }
    Run->SetLocale(TEXT("ko"));
    const TCHAR* Steps[][2] = {{TEXT("ch8_shore"), TEXT("목표  자갈밭 가운데의 구멍으로")}, {TEXT("ch8_hole"), TEXT("목표  구멍 안으로: 같이")},
        {TEXT("ch8_city"), TEXT("목표  북쪽 길로")}, {TEXT("ch8_tree"), TEXT("목표  북쪽 출구 → 갈림길")}};
    for (const auto& S : Steps) { Run->SetStoryFlag(S[0], true); TestEqual(*FString(S[0]), MemoriaNavigationGoal::Line(Run->GetRunSnapshot(), CoastMap, false), FString(S[1])); }
    Game->Shutdown();
    return true;
}

namespace
{
// S373 in the renderer: a development run comes onto the coast from the west; the morning and the shore play;
// the hole, then Blank 7 (fear lost, a line of the lullaby back); the young tree; and the empty chair.
class FCoastReplay final : public IAutomationLatentCommand
{
public:
    explicit FCoastReplay(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
    ~FCoastReplay() override { if (bFixed) { FApp::SetUseFixedTimeStep(bOldFixed); FApp::SetFixedDeltaTime(OldDelta); } }
    bool Update() override
    {
        if (LastFrame == GFrameCounter) return false;
        LastFrame = GFrameCounter;
        if (FPlatformTime::Seconds() - Started > 360) { Test->AddError(FString::Printf(TEXT("S373 timeout at step %d"), Step)); return true; }
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
        auto Capture = [&](const FString& Name) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Validation/S373Coast") / (Name + TEXT(".png")), true, false); };
        auto Flag = [&](const FString& Id) { return Run->GetRunSnapshot().GetFlag(Id); };
        auto Place = [&](const FVector2D& Source) { Pawn->SetActorLocation(MemoriaChapterMaps::ToWorld(Source) + FVector(0, 0, Pawn->GetActorLocation().Z)); };
        if (Host->GetState() == EMemoriaSliceState::Field)
        {
            const FMemoriaNarrativeView View = Host->GetView();
            for (const auto& Shot : Shots)
                if (!Taken.Contains(Shot.Key) && View.CueKey == Shot.Key) { Taken.Add(Shot.Key); ShotAt = Frame; ShotName = Shot.Value; }
            if (!ShotName.IsEmpty() && Frame == ShotAt + 20) { Capture(ShotName); ShotName.Reset(); return false; }
            if (ShotName.IsEmpty() && Frame % 6 == 0) Host->Confirm();
            return false;
        }
        auto Next = [&](const TCHAR* Done, int32 TriggerIndex)
        {
            if (Flag(Done) && Host->GetState() == EMemoriaSliceState::Exploration && Frame > Mark + 20)
            {
                if (TriggerIndex >= 0) Place(Spec->Triggers[TriggerIndex].Rect.Center()); else Place(Spec->Exit.Rect.Center());
                ++Step; Mark = Frame; return true;
            }
            if (Frame > Mark + 6000) { Test->AddError(FString::Printf(TEXT("Stuck before %s"), Done)); Step = 99; }
            return false;
        };
        switch (Step)
        {
        case 0:
            Test->TestEqual(TEXT("The map is the coast"), Map->GetMap(), FString(MemoriaManuscript::CoastMap));
            Test->TestEqual(TEXT("A development run in Chapter 8"), Run->GetRunSnapshot().CurrentChapter, int64(8));
            ++Step; Mark = Frame; break;
        case 1:
            if (Frame == Mark + 40) Capture(TEXT("S373Title"));
            if (Flag(TEXT("ch8_shore")) && Host->GetState() == EMemoriaSliceState::Exploration && Frame > Mark + 60)
            { Capture(TEXT("S373Shore")); Place(Spec->Triggers[0].Rect.Center()); ++Step; Mark = Frame; }
            if (Frame > Mark + 6000) { Test->AddError(TEXT("The arrival chain did not finish")); return true; }
            break;
        case 2: Next(TEXT("ch8_hole"), 1); break;
        case 3:
            if (Flag(TEXT("ch8_city")) && Host->GetState() == EMemoriaSliceState::Exploration && Frame > Mark + 20)
            {
                Test->TestTrue(TEXT("The fear is gone, a line came back"), Flag(TEXT("ch8_fear_gone")) && Flag(TEXT("ch8_lullaby_line")));
                Test->TestTrue(TEXT("with their toasts"), Host->GetTrace().Contains(TEXT("toast:Lost: fear")) && Host->GetTrace().Contains(TEXT("toast:Returned: a line of the lullaby")));
                Capture(TEXT("S373AfterCity"));
            }
            Next(TEXT("ch8_city"), 2); break;
        case 4: Next(TEXT("ch8_tree"), -1); break;
        case 5:
            if (Map->IsChapterComplete())
            {
                Test->TestTrue(TEXT("The empty chair closes Chapter 8"), Flag(TEXT("ch8_complete")) && Flag(TEXT("canon_ch9_clearing_ready")));
                Test->TestTrue(TEXT("Its last line was shown"), Taken.Contains(TEXT("field:ms_coast_chair:21")));
                Test->TestTrue(TEXT("The card shows"), Map->GetCard() && Map->GetCard()->IsShowing());
                ++Step; Mark = Frame;
            }
            if (Frame > Mark + 6000) { Test->AddError(TEXT("The exit never closed the chapter")); return true; }
            break;
        case 6:
            if (Frame == Mark + 30) Capture(TEXT("S373Complete"));
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
        {TEXT("field:ms_coast_shore:3"), TEXT("S373Fading")}, {TEXT("field:ms_coast_hole:3"), TEXT("S373Blank7")},
        {TEXT("field:ms_coast_city:5"), TEXT("S373NoLoneliness")}, {TEXT("field:ms_coast_city:8"), TEXT("S373Lullaby")},
        {TEXT("field:ms_coast_city:17"), TEXT("S373BookMark")}, {TEXT("field:ms_coast_tree:3"), TEXT("S373NotYet")},
        {TEXT("field:ms_coast_chair:8"), TEXT("S373FourLosses")}, {TEXT("field:ms_coast_chair:21"), TEXT("S373Hilt")}};
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoastVisualTest, "MemoriaVisual.S373Coast", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCoastVisualTest::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Memoria/Maps/L_CrumblingCoast"))) return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FCoastReplay(this)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
