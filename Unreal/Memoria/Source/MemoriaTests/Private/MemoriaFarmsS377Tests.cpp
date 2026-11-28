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

// S377: Chapter 11, the roofless farmhouses, from 최종본 17화 「읽는 자」 and 18화 「펠의 손」.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFarmsS377Test, "Memoria.S377.Farms", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFarmsS377Test::RunTest(const FString&)
{
    using namespace MemoriaManuscript;
    const auto* Farms = MemoriaChapterMaps::Find(FarmsMap);
    if (!TestNotNull(TEXT("The farmhouses are a chapter map"), Farms)) return false;
    TestEqual(TEXT("Chapter 11"), Farms->Chapter, 11);
    TestEqual(TEXT("Its level"), MemoriaChapterMaps::LevelPath(FarmsMap), FString(TEXT("/Game/Memoria/Maps/L_RooflessFarms")));
    TestEqual(TEXT("In Korean"), MemoriaChapterMaps::Korean(FarmsMap), FString(TEXT("지붕 없는 농가")));
    TestTrue(TEXT("Open fields: the rim is not a wall"), !Farms->TileNames.Contains(TEXT("WALL")) && Farms->TileNames.Contains(TEXT("EDGE")) && Farms->TileNames.Contains(TEXT("PILLAR")));
    TestTrue(TEXT("Paler than the Seam, warmer than the waste"), Farms->Saturation > .4f && Farms->Saturation < .7f);
    auto Open = [&](const FVector2D& P) { return !Farms->IsSolid(Farms->TileAt(int32(P.X) / Farms->TileSize, int32(P.Y) / Farms->TileSize)); };
    TestTrue(TEXT("In from the south, over the ridge"), Open(Farms->Spawn) && Farms->Spawn.Y > (Farms->Height - 3) * Farms->TileSize);
    TestTrue(TEXT("and out north"), Open(Farms->Exit.Rect.Center()) && Farms->Exit.Rect.Center().Y < 2 * Farms->TileSize);
    for (const auto& T : Farms->Triggers) TestTrue(*(TEXT("Open ground: ") + T.Group), Open(T.Rect.Center()));
    // The book opens inside the farmhouse's standing corner: a PILLAR tile right beside the trigger.
    const int32 Pillar = Farms->TileNames.IndexOfByKey(TEXT("PILLAR"));
    const auto& House = Farms->Triggers[0];
    const int32 HX = int32(House.Rect.Origin.X) / Farms->TileSize, HY = int32(House.Rect.Origin.Y) / Farms->TileSize;
    TestTrue(TEXT("The first farmhouse is a standing corner"), House.Group == TEXT("ms_farm_house") && (Farms->TileAt(HX - 1, HY) == Pillar || Farms->TileAt(HX, HY - 1) == Pillar));
    TArray<FString> Groups; for (const auto& S : Farms->Sequence) Groups.Add(S.Group);
    for (const auto& T : Farms->Triggers) Groups.Add(T.Group);
    Groups.Add(Farms->Exit.Group);
    TestEqual(TEXT("Six scenes"), Groups.Num(), 6);
    for (const FString& G : Groups) TestTrue(*(TEXT("A manuscript scene: ") + G), IsScene(G));
    const auto* Return = MemoriaChapterMaps::Find(ReturnMap);
    TestTrue(TEXT("The Seam's road west leads to the farmhouses"), Return && Return->Exit.NextMap == FarmsMap && Return->Exit.NextChapter == 11);
    TestTrue(TEXT("and the farmhouses' road to the basin"), Farms->Exit.NextMap == BasinMap && Farms->Exit.NextChapter == 12);

    auto Has = [](const UMemoriaFieldAsset* A, const TCHAR* Speaker, const TCHAR* Ko)
    { return A && A->Definition.Rows.ContainsByPredicate([&](const FMemoriaFieldRow& R) { return (Speaker ? R.Text.Speaker == Speaker : !R.Text.bHasSpeaker) && (R.Text.TextKo == Ko || R.Text.NarrateKo == Ko); }); };
    TestTrue(TEXT("The edge crumbles"), Has(BuildScene(GetTransientPackage(), TEXT("ms_farm_road"), 0), TEXT("Arrel"), TEXT("가장자리 밟지 마. 부서져.")));
    const auto* House2 = BuildScene(GetTransientPackage(), TEXT("ms_farm_house"), 0);
    TestTrue(TEXT("The one who opens is the relay"), Has(House2, nullptr, TEXT("여는 자가 곧 중계다.")));
    TestTrue(TEXT("The body is the relay's cost"), Has(House2, nullptr, TEXT("중계는 몸이 아니다. 몸은 중계의 비용이다.")));
    TestTrue(TEXT("Child"), Has(House2, TEXT("Sable"), TEXT("아이.")));
    TestTrue(TEXT("No, too fast"), Has(House2, TEXT("Elia"), TEXT("아니야.")));
    TestTrue(TEXT("It's nothing"), Has(BuildScene(GetTransientPackage(), TEXT("ms_farm_night"), 0), TEXT("Elia"), TEXT("아무것도 아니야.")));
    TestTrue(TEXT("Fel"), Has(BuildScene(GetTransientPackage(), TEXT("ms_farm_fel"), 0), nullptr, TEXT("펠.")));
    TestTrue(TEXT("The grey glove"), Has(BuildScene(GetTransientPackage(), TEXT("ms_farm_glove"), 0), TEXT("Arrel"), TEXT("장갑 하나요. 가죽이에요. 회색. 오래 쓴 것 같지는 않고.")));
    const auto* Second = BuildScene(GetTransientPackage(), TEXT("ms_farm_second"), 0);
    TestTrue(TEXT("Not now"), Has(Second, TEXT("Elia"), TEXT("지금은 안 할래.")));
    TestTrue(TEXT("He would tell her he had heard"), Second && Second->Definition.Rows.Last().Text.NarrateKo.Contains(TEXT("자기가 들었다는 말부터")));
    bool bRows = true, bCast = true, bSableFace = true;
    for (const FString& G : SceneGroups())
        if (G.StartsWith(TEXT("ms_farm_")))
            if (const auto* A = BuildScene(GetTransientPackage(), G, 0))
                for (int32 I = 0; I < A->Definition.Rows.Num(); ++I)
                {
                    const auto& R = A->Definition.Rows[I];
                    bRows &= R.OriginalIndex == I && (R.Text.bHasTextKo || R.Text.bHasNarrateKo);
                    // 17-18화: only Arrel, Elia and Sable. Haren has not come back; no Tobias; Fel is a name, not a speaker.
                    bCast &= !R.Text.bHasSpeaker || R.Text.Speaker == TEXT("Arrel") || R.Text.Speaker == TEXT("Elia") || R.Text.Speaker == TEXT("Sable");
                    if (R.Text.Speaker == TEXT("Sable")) bSableFace &= R.Presentation.Portrait == TEXT("sable_calm");
                }
    TestTrue(TEXT("Rows indexed, in Korean"), bRows);
    TestTrue(TEXT("Only the three speak"), bCast);
    TestTrue(TEXT("Sable's portrait is the blind one"), bSableFace);
    TestEqual(TEXT("Six farm scenes"), SceneGroups().FilterByPredicate([](const FString& G) { return G.StartsWith(TEXT("ms_farm_")); }).Num(), 6);
    TestEqual(TEXT("The relay toast in Korean"), MemoriaKorean::Text(TEXT("The blank book: the relay")), FString(TEXT("백 노트: 중계")));
    TestEqual(TEXT("The name toast in Korean"), MemoriaKorean::Text(TEXT("A name: Fel")), FString(TEXT("이름 하나: 펠")));
    TestEqual(TEXT("The title in Korean"), MemoriaKorean::Text(TEXT("The reader")), FString(TEXT("읽는 자")));

    TStrongObjectPtr<UGameInstance> Game(NewObject<UGameInstance>()); Game->Init();
    auto* Run = Game->GetSubsystem<UMemoriaRunSubsystem>();
    if (!TestTrue(TEXT("A run at Chapter 11"), Run->BeginStartingMemoryRun(11) == EMemoriaMemoryResult::Success)) { Game->Shutdown(); return false; }
    Run->SetLocale(TEXT("ko"));
    const TCHAR* Steps[][2] = {{TEXT("ch11_road"), TEXT("목표  둔덕 뒤, 지붕 없는 농가 안으로")}, {TEXT("ch11_house"), TEXT("목표  밤: 같은 벽을 따라")},
        {TEXT("ch11_night"), TEXT("목표  아침: 농가 문턱으로")}, {TEXT("ch11_fel"), TEXT("목표  북쪽 길로: 능선을 돌아 내려가는 곳")},
        {TEXT("ch11_glove"), TEXT("목표  북쪽 출구 → 마른 수로, 두 번째 농가")}};
    for (const auto& S : Steps) { Run->SetStoryFlag(S[0], true); TestEqual(*FString(S[0]), MemoriaNavigationGoal::Line(Run->GetRunSnapshot(), FarmsMap, false), FString(S[1])); }
    // The main quest: after the crossroads, the farmhouse; after the second farmhouse, the Belt road off the cleaners' roads.
    for (const TCHAR* F : {TEXT("ch11_house"), TEXT("ch11_glove")}) Run->SetStoryFlag(F, false);
    for (const auto& M : MainSteps()) if (!FString(M.Flag).StartsWith(TEXT("ch11_")) && !FString(M.Flag).StartsWith(TEXT("ch12_")) && !FString(M.Flag).StartsWith(TEXT("ch13_")) && !FString(M.Flag).StartsWith(TEXT("ch14_")) && !FString(M.Flag).StartsWith(TEXT("ch15_")) && !FString(M.Flag).StartsWith(TEXT("ch16_")) && !FString(M.Flag).StartsWith(TEXT("ch17_")) && !FString(M.Flag).StartsWith(TEXT("ch18_")) && FString(M.Flag) != TEXT("ms_never")) Run->SetStoryFlag(M.Flag, true);
    TestTrue(TEXT("The main quest is at the farmhouse"), MainRecord(Run->GetRunSnapshot(), true).Body.Contains(TEXT("→ 지붕 없는 농가에서 밤")));
    for (const TCHAR* F : {TEXT("ch11_house"), TEXT("ch11_glove"), TEXT("ch11_complete")}) Run->SetStoryFlag(F, true);
    TestTrue(TEXT("then the basin"), MainRecord(Run->GetRunSnapshot(), true).Body.Contains(TEXT("→ 분지 역참의 배급 창구")));
    Game->Shutdown();
    return true;
}

namespace
{
// S377 in the renderer: a development run comes over the ridge into the fields; the farmhouse (the relay), the night,
// the morning (a name), the glove on the road; and the second farmhouse.
class FFarmsReplay final : public IAutomationLatentCommand
{
public:
    explicit FFarmsReplay(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
    ~FFarmsReplay() override { if (bFixed) { FApp::SetUseFixedTimeStep(bOldFixed); FApp::SetFixedDeltaTime(OldDelta); } }
    bool Update() override
    {
        if (LastFrame == GFrameCounter) return false;
        LastFrame = GFrameCounter;
        if (FPlatformTime::Seconds() - Started > 360) { Test->AddError(FString::Printf(TEXT("S377 timeout at step %d"), Step)); return true; }
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
        auto Capture = [&](const FString& Name) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Validation/S377Farms") / (Name + TEXT(".png")), true, false); };
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
            Test->TestEqual(TEXT("The map is the farmhouses"), Map->GetMap(), FString(MemoriaManuscript::FarmsMap));
            Test->TestEqual(TEXT("A development run in Chapter 11"), Run->GetRunSnapshot().CurrentChapter, int64(11));
            ++Step; Mark = Frame; break;
        case 1:
            if (Frame == Mark + 40) Capture(TEXT("S377Title"));
            if (Flag(TEXT("ch11_road")) && Host->GetState() == EMemoriaSliceState::Exploration && Frame > Mark + 60)
            { Capture(TEXT("S377Fields")); Place(Spec->Triggers[0].Rect.Center()); ++Step; Mark = Frame; }
            if (Frame > Mark + 6000) { Test->AddError(TEXT("The arrival chain did not finish")); return true; }
            break;
        case 2:
            if (!bShot && Flag(TEXT("ch11_house")) && Host->GetState() == EMemoriaSliceState::Exploration)
            {
                bShot = true;
                Test->TestTrue(TEXT("The blank book: the relay"), Flag(TEXT("ch11_relay")) && Host->GetTrace().Contains(TEXT("toast:The blank book: the relay")));
                Capture(TEXT("S377Farmhouse")); Mark = Frame; break;
            }
            Next(TEXT("ch11_house"), 1); break;
        case 3: Next(TEXT("ch11_night"), 2); break;
        case 4:
            if (!bAnger && Flag(TEXT("ch11_fel")) && Host->GetState() == EMemoriaSliceState::Exploration)
            {
                bAnger = true;
                Test->TestTrue(TEXT("A name: Fel"), Flag(TEXT("ch11_fel_name")) && Host->GetTrace().Contains(TEXT("toast:A name: Fel")));
                Mark = Frame; break;
            }
            Next(TEXT("ch11_fel"), 3); break;
        case 5: Next(TEXT("ch11_glove"), -1); break;
        case 6:
            if (Map->IsChapterComplete())
            {
                Test->TestTrue(TEXT("The second farmhouse closes Chapter 11"), Flag(TEXT("ch11_complete")) && Flag(TEXT("canon_ch12_belt_road_ready")) && Flag(TEXT("ch11_fel_heard")));
                Test->TestTrue(TEXT("Its last line was shown"), Taken.Contains(TEXT("field:ms_farm_second:20")));
                Test->TestTrue(TEXT("The card shows"), Map->GetCard() && Map->GetCard()->IsShowing());
                ++Step; Mark = Frame;
            }
            if (Frame > Mark + 6000) { Test->AddError(TEXT("The exit never closed the chapter")); return true; }
            break;
        case 7:
            if (Frame == Mark + 30) Capture(TEXT("S377Complete"));
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
    bool bFixed = false, bOldFixed = false, bShot = false, bAnger = false;
    FString ShotName;
    TSet<FString> Taken;
    const TArray<TPair<FString, FString>> Shots = {
        {TEXT("field:ms_farm_road:4"), TEXT("S377Warm")}, {TEXT("field:ms_farm_house:7"), TEXT("S377Book")},
        {TEXT("field:ms_farm_house:11"), TEXT("S377Relay")}, {TEXT("field:ms_farm_house:16"), TEXT("S377Cost")},
        {TEXT("field:ms_farm_house:21"), TEXT("S377No")}, {TEXT("field:ms_farm_night:9"), TEXT("S377Nose")},
        {TEXT("field:ms_farm_fel:2"), TEXT("S377Fel")}, {TEXT("field:ms_farm_glove:0"), TEXT("S377Glove")},
        {TEXT("field:ms_farm_second:8"), TEXT("S377NotNow")}, {TEXT("field:ms_farm_second:11"), TEXT("S377SecondFarm")},
        {TEXT("field:ms_farm_second:17"), TEXT("S377FelAsleep")}, {TEXT("field:ms_farm_second:20"), TEXT("S377Heard")}};
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFarmsVisualTest, "MemoriaVisual.S377Farms", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFarmsVisualTest::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Memoria/Maps/L_RooflessFarms"))) return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FFarmsReplay(this)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
