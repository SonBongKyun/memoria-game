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

// S375: Chapter 10, the Seam, leaving, from 최종본 15화 「손이 기억하는 것」 and 16화 「출발」.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReturnS375Test, "Memoria.S375.Return", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FReturnS375Test::RunTest(const FString&)
{
    using namespace MemoriaManuscript;
    const auto* Return = MemoriaChapterMaps::Find(ReturnMap);
    const auto* Seam = MemoriaChapterMaps::Find(SeamMap);
    if (!TestNotNull(TEXT("The Seam, leaving, is a chapter map"), Return) || !TestNotNull(TEXT("and the Seam"), Seam)) return false;
    TestEqual(TEXT("Chapter 10"), Return->Chapter, 10);
    TestEqual(TEXT("Its level"), MemoriaChapterMaps::LevelPath(ReturnMap), FString(TEXT("/Game/Memoria/Maps/L_SeamReturn")));
    TestEqual(TEXT("In Korean"), MemoriaChapterMaps::Korean(ReturnMap), FString(TEXT("떠나는 더 심")));
    bool bSame = Return->Width == Seam->Width && Return->Height == Seam->Height && Return->TileNames == Seam->TileNames;
    for (int32 Y = 0; bSame && Y < Seam->Height; ++Y) for (int32 X = 0; X < Seam->Width; ++X) bSame &= Return->TileAt(X, Y) == Seam->TileAt(X, Y);
    TestTrue(TEXT("The same village: the Seam's own ground"), bSame);
    auto Open = [&](const FVector2D& P) { return !Return->IsSolid(Return->TileAt(int32(P.X) / Return->TileSize, int32(P.Y) / Return->TileSize)); };
    TestTrue(TEXT("Haren brings them down the north-east path"), Open(Return->Spawn) && Return->Spawn.Y < 2 * Return->TileSize && Return->Spawn.X > Return->Width * Return->TileSize / 2);
    TestTrue(TEXT("and they leave west"), Open(Return->Exit.Rect.Center()) && Return->Exit.Rect.Center().X < 2 * Return->TileSize);
    for (const auto& T : Return->Triggers) TestTrue(*(TEXT("Open ground: ") + T.Group), Open(T.Rect.Center()));
    TArray<FString> Groups; for (const auto& S : Return->Sequence) Groups.Add(S.Group);
    for (const auto& T : Return->Triggers) Groups.Add(T.Group);
    Groups.Add(Return->Exit.Group);
    TestEqual(TEXT("Six scenes"), Groups.Num(), 6);
    for (const FString& G : Groups) TestTrue(*(TEXT("A manuscript scene: ") + G), IsScene(G));
    const auto* Clearing = MemoriaChapterMaps::Find(ClearingMap);
    TestTrue(TEXT("The dry channel's road leads back to the Seam"), Clearing && Clearing->Exit.NextMap == ReturnMap && Clearing->Exit.NextChapter == 10);
    TestTrue(TEXT("and the Seam's road west leads to the farmhouses"), Return->Exit.NextMap == FarmsMap && Return->Exit.NextChapter == 11);

    auto Has = [](const UMemoriaFieldAsset* A, const TCHAR* Speaker, const TCHAR* Ko)
    { return A && A->Definition.Rows.ContainsByPredicate([&](const FMemoriaFieldRow& R) { return (Speaker ? R.Text.Speaker == Speaker : !R.Text.bHasSpeaker) && (R.Text.TextKo == Ko || R.Text.NarrateKo == Ko); }); };
    TestTrue(TEXT("Coming back down"), Has(BuildScene(GetTransientPackage(), TEXT("ms_return_road"), 0), TEXT("Haren"), TEXT("다시 내려오는군.")));
    TestTrue(TEXT("Something else is coming"), Has(BuildScene(GetTransientPackage(), TEXT("ms_return_bridge"), 0), TEXT("Haren"), TEXT("너희 기다리는 게 아냐. ...다른 게 오고 있어.")));
    const auto* Step = BuildScene(GetTransientPackage(), TEXT("ms_return_step"), 0);
    TestTrue(TEXT("Burn the tree"), Has(Step, TEXT("Sable"), TEXT("공터로 가. 어제 벤 나무를 쪼개서 태워. 글씨 남은 데가 없도록. 재는 묻고. 어디에 묻었는지 말하지 마.")));
    TestTrue(TEXT("A cradle song"), Has(Step, TEXT("Sable"), TEXT("요람가야.")));
    TestTrue(TEXT("No name here"), Has(Step, TEXT("Sable"), TEXT("이름은 여기서 말하지 않을 거야.")));
    TestTrue(TEXT("The two halves fit"), Has(Step, TEXT("Sable"), TEXT("두 반쪽이 맞아.")));
    TestTrue(TEXT("Don't call it charity"), Has(Step, TEXT("Elia"), TEXT("자선이라고 하지 마.")));
    const auto* Horn = BuildScene(GetTransientPackage(), TEXT("ms_return_horn"), 0);
    TestTrue(TEXT("I can't feel Arrel"), Has(Horn, TEXT("Elia"), TEXT("아렐이 안 느껴져.")));
    TestTrue(TEXT("The boy counts nine"), Has(Horn, TEXT("Boy"), TEXT("여덟이에요. 조정관까지 아홉. 조정관은 가운데 있고. 마을로 내려오기 전 마지막 능선에 있어요. 천천히 와요.")));
    TestTrue(TEXT("For you"), Has(Horn, TEXT("Sable"), TEXT("그래. 너에게.")));
    TestTrue(TEXT("West"), Has(Horn, TEXT("Arrel"), TEXT("서쪽.")));
    TestTrue(TEXT("The anger you just got"), Has(BuildScene(GetTransientPackage(), TEXT("ms_return_square"), 0), TEXT("Sable"), TEXT("멀어지고 나서 얘기하자. 네가 방금 얻은 분노를 어디에 쓸지.")));
    const auto* Cross = BuildScene(GetTransientPackage(), TEXT("ms_return_crossroads"), 0);
    TestTrue(TEXT("Not here"), Has(Cross, TEXT("Sable"), TEXT("여긴 아니야.")));
    TestTrue(TEXT("Three more"), Has(Cross, TEXT("Sable"), TEXT("아렐. 벨트 도로까지 저런 곳이 셋 더 있다.")));
    bool bRows = true, bCast = true, bSableFace = true;
    for (const FString& G : SceneGroups())
        if (G.StartsWith(TEXT("ms_return_")))
            if (const auto* A = BuildScene(GetTransientPackage(), G, 0))
                for (int32 I = 0; I < A->Definition.Rows.Num(); ++I)
                {
                    const auto& R = A->Definition.Rows[I];
                    bRows &= R.OriginalIndex == I && (R.Text.bHasTextKo || R.Text.bHasNarrateKo);
                    // 15-16화: Arrel, Elia, Haren, Sable and the boy speak. No Kairos, no Tobias, no Feren by name.
                    bCast &= !R.Text.bHasSpeaker || R.Text.Speaker == TEXT("Arrel") || R.Text.Speaker == TEXT("Elia") || R.Text.Speaker == TEXT("Haren")
                        || R.Text.Speaker == TEXT("Sable") || R.Text.Speaker == TEXT("Boy");
                    // Sable's face is the blind one.
                    if (R.Text.Speaker == TEXT("Sable")) bSableFace &= R.Presentation.Portrait == TEXT("sable_calm");
                }
    TestTrue(TEXT("Rows indexed, in Korean"), bRows);
    TestTrue(TEXT("Only the five speak"), bCast);
    TestTrue(TEXT("Sable's portrait is the blind one"), bSableFace);
    TestEqual(TEXT("Six return scenes"), SceneGroups().FilterByPredicate([](const FString& G) { return G.StartsWith(TEXT("ms_return_")); }).Num(), 6);
    TestEqual(TEXT("The toast in Korean"), MemoriaKorean::Text(TEXT("Gained: anger")), FString(TEXT("얻음: 분노")));
    TestEqual(TEXT("The cradle toast in Korean"), MemoriaKorean::Text(TEXT("Two halves of one cradle song")), FString(TEXT("한 요람가의 두 반쪽")));
    TestEqual(TEXT("The title in Korean"), MemoriaKorean::Text(TEXT("What the hand remembers")), FString(TEXT("손이 기억하는 것")));

    TStrongObjectPtr<UGameInstance> Game(NewObject<UGameInstance>()); Game->Init();
    auto* Run = Game->GetSubsystem<UMemoriaRunSubsystem>();
    if (!TestTrue(TEXT("A run at Chapter 10"), Run->BeginStartingMemoryRun(10) == EMemoriaMemoryResult::Success)) { Game->Shutdown(); return false; }
    Run->SetLocale(TEXT("ko"));
    const TCHAR* Steps[][2] = {{TEXT("ch10_bridge"), TEXT("목표  마을 가운데, 돌턱의 세이블에게")}, {TEXT("ch10_step"), TEXT("목표  동쪽 길 쪽으로")},
        {TEXT("ch10_horn"), TEXT("목표  광장 서쪽 끝으로")}, {TEXT("ch10_square"), TEXT("목표  서쪽 출구 → 벨트 도로 쪽")}};
    for (const auto& S : Steps) { Run->SetStoryFlag(S[0], true); TestEqual(*FString(S[0]), MemoriaNavigationGoal::Line(Run->GetRunSnapshot(), ReturnMap, false), FString(S[1])); }
    // The main quest: after the dry channel, Sable; after the crossroads, the Belt road.
    Run->SetStoryFlag(TEXT("ch10_step"), false); Run->SetStoryFlag(TEXT("ch10_horn"), false);
    for (const auto& M : MainSteps()) if (!FString(M.Flag).StartsWith(TEXT("ch10_")) && !FString(M.Flag).StartsWith(TEXT("ch11_")) && !FString(M.Flag).StartsWith(TEXT("ch12_")) && FString(M.Flag) != TEXT("ms_never")) Run->SetStoryFlag(M.Flag, true);
    TestTrue(TEXT("The main quest is at Sable's step"), MainRecord(Run->GetRunSnapshot(), true).Body.Contains(TEXT("→ 더 심에서 세이블에게, 그 앞에서 묻는다")));
    for (const TCHAR* F : {TEXT("ch10_step"), TEXT("ch10_horn"), TEXT("ch10_complete")}) Run->SetStoryFlag(F, true);
    TestTrue(TEXT("then the farmhouses"), MainRecord(Run->GetRunSnapshot(), true).Body.Contains(TEXT("→ 지붕 없는 농가에서 밤")));
    Game->Shutdown();
    return true;
}

namespace
{
// S375 in the renderer: a development run comes down into the Seam from the north-east; the road and the bridge play;
// Sable's step (the two halves), the east road (the horn), the square (anger); and the crossroads below the ridge.
class FReturnReplay final : public IAutomationLatentCommand
{
public:
    explicit FReturnReplay(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
    ~FReturnReplay() override { if (bFixed) { FApp::SetUseFixedTimeStep(bOldFixed); FApp::SetFixedDeltaTime(OldDelta); } }
    bool Update() override
    {
        if (LastFrame == GFrameCounter) return false;
        LastFrame = GFrameCounter;
        if (FPlatformTime::Seconds() - Started > 360) { Test->AddError(FString::Printf(TEXT("S375 timeout at step %d"), Step)); return true; }
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
        auto Capture = [&](const FString& Name) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Validation/S375Return") / (Name + TEXT(".png")), true, false); };
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
            Test->TestEqual(TEXT("The map is the Seam, leaving"), Map->GetMap(), FString(MemoriaManuscript::ReturnMap));
            Test->TestEqual(TEXT("A development run in Chapter 10"), Run->GetRunSnapshot().CurrentChapter, int64(10));
            ++Step; Mark = Frame; break;
        case 1:
            if (Frame == Mark + 40) Capture(TEXT("S375Title"));
            if (Flag(TEXT("ch10_bridge")) && Host->GetState() == EMemoriaSliceState::Exploration && Frame > Mark + 60)
            { Capture(TEXT("S375Village")); Place(Spec->Triggers[0].Rect.Center()); ++Step; Mark = Frame; }
            if (Frame > Mark + 6000) { Test->AddError(TEXT("The arrival chain did not finish")); return true; }
            break;
        case 2:
            if (!bShot && Flag(TEXT("ch10_step")) && Host->GetState() == EMemoriaSliceState::Exploration)
            {
                bShot = true;
                Test->TestTrue(TEXT("Two halves of one cradle song"), Flag(TEXT("ch10_cradle_halves")) && Host->GetTrace().Contains(TEXT("toast:Two halves of one cradle song")));
                Mark = Frame; break;
            }
            Next(TEXT("ch10_step"), 1); break;
        case 3: Next(TEXT("ch10_horn"), 2); break;
        case 4:
            if (!bAnger && Flag(TEXT("ch10_square")) && Host->GetState() == EMemoriaSliceState::Exploration)
            {
                bAnger = true;
                Test->TestTrue(TEXT("Gained: anger"), Flag(TEXT("ch10_anger")) && Host->GetTrace().Contains(TEXT("toast:Gained: anger")));
                Capture(TEXT("S375Square")); Mark = Frame; break;
            }
            Next(TEXT("ch10_square"), -1); break;
        case 5:
            if (Map->IsChapterComplete())
            {
                Test->TestTrue(TEXT("The crossroads closes Chapter 10"), Flag(TEXT("ch10_complete")) && Flag(TEXT("canon_ch11_belt_road_ready")) && Flag(TEXT("ch10_not_here")));
                Test->TestTrue(TEXT("Its last line was shown"), Taken.Contains(TEXT("field:ms_return_crossroads:34")));
                Test->TestTrue(TEXT("The card shows"), Map->GetCard() && Map->GetCard()->IsShowing());
                ++Step; Mark = Frame;
            }
            if (Frame > Mark + 6000) { Test->AddError(TEXT("The exit never closed the chapter")); return true; }
            break;
        case 6:
            if (Frame == Mark + 30) Capture(TEXT("S375Complete"));
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
        {TEXT("field:ms_return_road:5"), TEXT("S375Haren")}, {TEXT("field:ms_return_bridge:5"), TEXT("S375Packing")},
        {TEXT("field:ms_return_step:0"), TEXT("S375SableStep")}, {TEXT("field:ms_return_step:21"), TEXT("S375Cradle")},
        {TEXT("field:ms_return_step:29"), TEXT("S375Halves")}, {TEXT("field:ms_return_horn:14"), TEXT("S375Horn")},
        {TEXT("field:ms_return_horn:16"), TEXT("S375Boy")}, {TEXT("field:ms_return_horn:36"), TEXT("S375West")},
        {TEXT("field:ms_return_square:4"), TEXT("S375Anger")}, {TEXT("field:ms_return_crossroads:9"), TEXT("S375Crossroads")},
        {TEXT("field:ms_return_crossroads:15"), TEXT("S375NotHere")}, {TEXT("field:ms_return_crossroads:34"), TEXT("S375OverTheRidge")}};
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReturnVisualTest, "MemoriaVisual.S375Return", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FReturnVisualTest::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Memoria/Maps/L_SeamReturn"))) return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FReturnReplay(this)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
