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

// S379: Chapter 12, the basin waystation, from 최종본 19화 「미라」 and 20화 「회색 우리」.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBasinS379Test, "Memoria.S379.Basin", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBasinS379Test::RunTest(const FString&)
{
    using namespace MemoriaManuscript;
    const auto* Basin = MemoriaChapterMaps::Find(BasinMap);
    const auto* Belt = MemoriaChapterMaps::Find(TEXT("belt_waystation"));
    if (!TestNotNull(TEXT("The basin waystation is a chapter map"), Basin) || !TestNotNull(TEXT("and the Belt yard"), Belt)) return false;
    TestEqual(TEXT("Chapter 12"), Basin->Chapter, 12);
    TestEqual(TEXT("Its level"), MemoriaChapterMaps::LevelPath(BasinMap), FString(TEXT("/Game/Memoria/Maps/L_BasinStation")));
    TestEqual(TEXT("In Korean"), MemoriaChapterMaps::Korean(BasinMap), FString(TEXT("분지의 역참")));
    bool bSame = Basin->Width == Belt->Width && Basin->Height == Belt->Height;
    for (int32 Y = 0; bSame && Y < Belt->Height; ++Y) for (int32 X = 0; X < Belt->Width; ++X) bSame &= Basin->TileAt(X, Y) == Belt->TileAt(X, Y);
    TestTrue(TEXT("A waystation at a crossroads: the Belt yard's ground"), bSame);
    auto Open = [&](const FVector2D& P) { return !Basin->IsSolid(Basin->TileAt(int32(P.X) / Basin->TileSize, int32(P.Y) / Basin->TileSize)); };
    TestTrue(TEXT("Down from the north"), Open(Basin->Spawn) && Basin->Spawn.Y < 2 * Basin->TileSize);
    TestTrue(TEXT("and out by the east road"), Open(Basin->Exit.Rect.Center()) && Basin->Exit.Rect.Center().X > (Basin->Width - 2) * Basin->TileSize);
    for (const auto& T : Basin->Triggers) TestTrue(*(TEXT("Open ground: ") + T.Group), Open(T.Rect.Center()));
    // The queue: a clerk at the window and people along the south wall, all on open ground, nowhere near the walk.
    TestEqual(TEXT("A clerk and eleven in line"), Basin->AmbientNpcs.Num(), 12);
    TestTrue(TEXT("The clerk at the window"), Basin->AmbientNpcs.Num() && Basin->AmbientNpcs[0].Preset == TEXT("bureau_agent"));
    bool bQueueOpen = true; for (const auto& N : Basin->AmbientNpcs) bQueueOpen &= Open(N.Position);
    TestTrue(TEXT("The queue stands on open ground"), bQueueOpen);
    // S383 (T-8): the queue holds its places; the eleven look at the window, the clerk out at the line.
    bool bHeld = true, bToWindow = true;
    for (int32 I = 0; I < Basin->AmbientNpcs.Num(); ++I)
    {
        const auto& N = Basin->AmbientNpcs[I]; bHeld &= N.bStill && N.bHasFace;
        if (I > 0) bToWindow &= N.Face.Equals(FVector2D(374.4f, 368.f), .1f);
    }
    TestTrue(TEXT("Everyone in the queue holds a place and faces a point"), bHeld);
    TestTrue(TEXT("The line faces the ration window"), bToWindow);
    TestTrue(TEXT("The clerk looks out at the line"), Basin->AmbientNpcs.Num() && Basin->AmbientNpcs[0].Face.Y > Basin->AmbientNpcs[0].Position.Y);
    TestEqual(TEXT("The queue is there from the arrival on"), Basin->AmbientNpcsGate, FString(TEXT("ch12_road")));
    TArray<FString> Groups; for (const auto& S : Basin->Sequence) Groups.Add(S.Group);
    for (const auto& T : Basin->Triggers) Groups.Add(T.Group);
    Groups.Add(Basin->Exit.Group);
    TestEqual(TEXT("Six scenes"), Groups.Num(), 6);
    for (const FString& G : Groups) TestTrue(*(TEXT("A manuscript scene: ") + G), IsScene(G));
    const auto* Farms = MemoriaChapterMaps::Find(FarmsMap);
    TestTrue(TEXT("The farmhouses' road leads to the basin"), Farms && Farms->Exit.NextMap == BasinMap && Farms->Exit.NextChapter == 12);
    TestTrue(TEXT("and the basin's road to Verdan"), Basin->Exit.NextMap == VerdanReturnMap && Basin->Exit.NextChapter == 13);

    auto Has = [](const UMemoriaFieldAsset* A, const TCHAR* Speaker, const TCHAR* Ko)
    { return A && A->Definition.Rows.ContainsByPredicate([&](const FMemoriaFieldRow& R) { return (Speaker ? R.Text.Speaker == Speaker : !R.Text.bHasSpeaker) && (R.Text.TextKo == Ko || R.Text.NarrateKo == Ko); }); };
    const auto* Mira = BuildScene(GetTransientPackage(), TEXT("ms_basin_mira"), 0);
    TestTrue(TEXT("0.3 degrees"), Has(Mira, nullptr, TEXT("종이 위쪽에 K-7이라고 적혀 있었다. 그 관리관의 어긋남은 0.03도를 넘지 않았다. 직각자를 대어 각도를 재었다. 0.3도였다.")));
    TestTrue(TEXT("Anything?"), Has(Mira, TEXT("Belor"), TEXT("뭔가 있었나.")));
    TestTrue(TEXT("Nothing"), Has(Mira, TEXT("Mira"), TEXT("없었습니다.")));
    TestTrue(TEXT("The cutaway is Mira's alone"), Mira && !Mira->Definition.Rows.ContainsByPredicate([](const FMemoriaFieldRow& R) { return R.Text.Speaker == TEXT("Arrel") || R.Text.Speaker == TEXT("Elia") || R.Text.Speaker == TEXT("Sable"); }));
    TestTrue(TEXT("South, then"), Has(BuildScene(GetTransientPackage(), TEXT("ms_basin_road"), 0), TEXT("Sable"), TEXT("남쪽으로 가자.")));
    TestTrue(TEXT("Half-mast is not mourning"), Has(BuildScene(GetTransientPackage(), TEXT("ms_basin_tower"), 0), nullptr, TEXT("반쯤 내린 것은 애도를 알리는 신호가 아니었다. 그것도 알았다.")));
    const auto* Queue = BuildScene(GetTransientPackage(), TEXT("ms_basin_queue"), 0);
    TestTrue(TEXT("Blankers"), Has(Queue, TEXT("Arrel"), TEXT("블랭커예요. ...삼키고 가요.")));
    TestTrue(TEXT("Into that line too?"), Has(Queue, TEXT("Sable"), TEXT("너도 그 안으로 들어가겠다는 거니?")));
    TestTrue(TEXT("Sable's hand"), Has(BuildScene(GetTransientPackage(), TEXT("ms_basin_walk"), 0), nullptr, TEXT("세이블의 왼손이 그의 소매에 닿았다. 노파의 손끝이 천을 따라 내려와 손등에 얹혔다. 세이블은 그쪽으로 얼굴을 돌리지 않았다. 마른 손바닥이었다. 누르지도, 끌어당기지도 않았다.")));
    const auto* Pen = BuildScene(GetTransientPackage(), TEXT("ms_basin_pen"), 0);
    TestTrue(TEXT("The book would have known"), Has(Pen, TEXT("Elia"), TEXT("책이 우리가 어디 있는지 알았을 테니까.")));
    TestTrue(TEXT("The next window"), Pen && Pen->Definition.Rows.Last().Text.NarrateKo.Contains(TEXT("다음 창구 앞에서")));
    bool bRows = true, bCast = true, bFaces = true;
    for (const FString& G : SceneGroups())
        if (G.StartsWith(TEXT("ms_basin_")))
            if (const auto* A = BuildScene(GetTransientPackage(), G, 0))
                for (int32 I = 0; I < A->Definition.Rows.Num(); ++I)
                {
                    const auto& R = A->Definition.Rows[I];
                    bRows &= R.OriginalIndex == I && (R.Text.bHasTextKo || R.Text.bHasNarrateKo);
                    // 19-20화: Arrel, Elia, Sable; in Lumea, Mira and Belor. No Tobias, no Haren, no Kairos.
                    bCast &= !R.Text.bHasSpeaker || R.Text.Speaker == TEXT("Arrel") || R.Text.Speaker == TEXT("Elia") || R.Text.Speaker == TEXT("Sable")
                        || R.Text.Speaker == TEXT("Mira") || R.Text.Speaker == TEXT("Belor");
                    if (R.Text.Speaker == TEXT("Sable")) bFaces &= R.Presentation.Portrait == TEXT("sable_calm");
                    if (R.Text.Speaker == TEXT("Mira") || R.Text.Speaker == TEXT("Belor")) bFaces &= !R.Presentation.bHasPortrait;
                }
    TestTrue(TEXT("Rows indexed, in Korean"), bRows);
    TestTrue(TEXT("Only the five speak"), bCast);
    TestTrue(TEXT("Sable's face is the blind one; Mira and Belor have none"), bFaces);
    TestEqual(TEXT("Six basin scenes"), SceneGroups().FilterByPredicate([](const FString& G) { return G.StartsWith(TEXT("ms_basin_")); }).Num(), 6);
    TestEqual(TEXT("The toast in Korean"), MemoriaKorean::Text(TEXT("The ration window: blankers")), FString(TEXT("배급 창구: 블랭커")));
    TestEqual(TEXT("The title in Korean"), MemoriaKorean::Text(TEXT("The grey pen")), FString(TEXT("회색 우리")));

    TStrongObjectPtr<UGameInstance> Game(NewObject<UGameInstance>()); Game->Init();
    auto* Run = Game->GetSubsystem<UMemoriaRunSubsystem>();
    if (!TestTrue(TEXT("A run at Chapter 12"), Run->BeginStartingMemoryRun(12) == EMemoriaMemoryResult::Success)) { Game->Shutdown(); return false; }
    Run->SetLocale(TEXT("ko"));
    const TCHAR* Steps[][2] = {{TEXT("ch12_road"), TEXT("목표  남쪽 길로: 표주를 지나")}, {TEXT("ch12_tower"), TEXT("목표  서쪽 높은 땅에서 창구를 본다")},
        {TEXT("ch12_queue"), TEXT("목표  분지를 돌아 동쪽으로: 창구에서 육십 걸음")}, {TEXT("ch12_walk"), TEXT("목표  동쪽 출구 → 마른 시내 위의 양 우리")}};
    for (const auto& S : Steps) { Run->SetStoryFlag(S[0], true); TestEqual(*FString(S[0]), MemoriaNavigationGoal::Line(Run->GetRunSnapshot(), BasinMap, false), FString(S[1])); }
    Run->SetStoryFlag(TEXT("ch12_queue"), false);
    for (const auto& M : MainSteps()) if (!FString(M.Flag).StartsWith(TEXT("ch12_")) && !FString(M.Flag).StartsWith(TEXT("ch13_")) && !FString(M.Flag).StartsWith(TEXT("ch14_")) && FString(M.Flag) != TEXT("ms_never")) Run->SetStoryFlag(M.Flag, true);
    TestTrue(TEXT("The main quest is at the window"), MainRecord(Run->GetRunSnapshot(), true).Body.Contains(TEXT("→ 분지 역참의 배급 창구")));
    for (const TCHAR* F : {TEXT("ch12_queue"), TEXT("ch12_complete")}) Run->SetStoryFlag(F, true);
    TestTrue(TEXT("then Verdan"), MainRecord(Run->GetRunSnapshot(), true).Body.Contains(TEXT("→ 다시 베르단: 셈터")));
    Game->Shutdown();
    return true;
}

namespace
{
// S379 in the renderer: Mira's registry in Lumea, then the run comes down into the basin from the north; the tower,
// the ration window and its queue from the west, the walk round to the east; and the sheep pen.
class FBasinReplay final : public IAutomationLatentCommand
{
public:
    explicit FBasinReplay(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
    ~FBasinReplay() override { if (bFixed) { FApp::SetUseFixedTimeStep(bOldFixed); FApp::SetFixedDeltaTime(OldDelta); } }
    bool Update() override
    {
        if (LastFrame == GFrameCounter) return false;
        LastFrame = GFrameCounter;
        if (FPlatformTime::Seconds() - Started > 360) { Test->AddError(FString::Printf(TEXT("S379 timeout at step %d"), Step)); return true; }
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
        auto Capture = [&](const FString& Name) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Validation/S379Basin") / (Name + TEXT(".png")), true, false); };
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
            Test->TestEqual(TEXT("The map is the basin"), Map->GetMap(), FString(MemoriaManuscript::BasinMap));
            Test->TestEqual(TEXT("A development run in Chapter 12"), Run->GetRunSnapshot().CurrentChapter, int64(12));
            ++Step; Mark = Frame; break;
        case 1:
            if (Frame == Mark + 40) Capture(TEXT("S379Title"));
            if (Flag(TEXT("ch12_road")) && Host->GetState() == EMemoriaSliceState::Exploration && Frame > Mark + 60)
            { Capture(TEXT("S379Road")); Place(Spec->Triggers[0].Rect.Center()); ++Step; Mark = Frame; }
            if (Frame > Mark + 6000) { Test->AddError(TEXT("The arrival chain did not finish")); return true; }
            break;
        case 2: Next(TEXT("ch12_tower"), 1); break;
        case 3:
            if (!bShot && Flag(TEXT("ch12_queue")) && Host->GetState() == EMemoriaSliceState::Exploration)
            {
                bShot = true;
                Test->TestTrue(TEXT("The ration window: blankers"), Flag(TEXT("ch12_blankers")) && Host->GetTrace().Contains(TEXT("toast:The ration window: blankers")));
                Capture(TEXT("S379Queue")); Mark = Frame; break;
            }
            Next(TEXT("ch12_queue"), 2); break;
        case 4:
            if (!bAnger && Flag(TEXT("ch12_walk")) && Host->GetState() == EMemoriaSliceState::Exploration)
            { bAnger = true; Test->TestTrue(TEXT("Sable's hand on his sleeve"), Flag(TEXT("ch12_sable_hand"))); Capture(TEXT("S379East")); Mark = Frame; break; }
            Next(TEXT("ch12_walk"), -1); break;
        case 5:
            if (Map->IsChapterComplete())
            {
                Test->TestTrue(TEXT("The sheep pen closes Chapter 12"), Flag(TEXT("ch12_complete")) && Flag(TEXT("canon_ch13_verdan_ready")) && Flag(TEXT("ch12_mira_kept")));
                Test->TestTrue(TEXT("Its last line was shown"), Taken.Contains(TEXT("field:ms_basin_pen:26")));
                Test->TestTrue(TEXT("The card shows"), Map->GetCard() && Map->GetCard()->IsShowing());
                ++Step; Mark = Frame;
            }
            if (Frame > Mark + 6000) { Test->AddError(TEXT("The exit never closed the chapter")); return true; }
            break;
        case 6:
            if (Frame == Mark + 30) Capture(TEXT("S379Complete"));
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
        {TEXT("field:ms_basin_mira:0"), TEXT("S379Registry")}, {TEXT("field:ms_basin_mira:2"), TEXT("S379Angle")},
        {TEXT("field:ms_basin_mira:15"), TEXT("S379Nothing")}, {TEXT("field:ms_basin_road:2"), TEXT("S379Fel")},
        {TEXT("field:ms_basin_tower:2"), TEXT("S379Flag")}, {TEXT("field:ms_basin_queue:3"), TEXT("S379Pill")},
        {TEXT("field:ms_basin_queue:5"), TEXT("S379Blankers")}, {TEXT("field:ms_basin_walk:1"), TEXT("S379Hand")},
        {TEXT("field:ms_basin_pen:0"), TEXT("S379Pen")}, {TEXT("field:ms_basin_pen:15"), TEXT("S379BookKnows")},
        {TEXT("field:ms_basin_pen:26"), TEXT("S379NextWindow")}};
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBasinVisualTest, "MemoriaVisual.S379Basin", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBasinVisualTest::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Memoria/Maps/L_BasinStation"))) return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FBasinReplay(this)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
