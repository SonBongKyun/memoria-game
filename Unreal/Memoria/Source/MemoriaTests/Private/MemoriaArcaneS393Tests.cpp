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
#include "Interaction/MemoriaTobiasCompanion.h"
#include "Narrative/MemoriaNarrativeSubsystem.h"
#include "Narrative/MemoriaNarrativeData.h"
#include "Narrative/MemoriaManuscript.h"
#include "Narrative/MemoriaNavigationGoal.h"
#include "Localization/MemoriaKorean.h"
#include "Presentation/MemoriaNarrativeArtwork.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Run/MemoriaRunTypes.h"
#include "Chapter/MemoriaChapterMap.h"
#include "Chapter/MemoriaChapterPresentation.h"
#include "Chapter/MemoriaChapterCardWidget.h"
#if WITH_DEV_AUTOMATION_TESTS

// S393: Chapter 16, Arcane, from 최종본 25화 「고해소」 and 26화 「카이로스 주저하다」 (Gemini's T-11 and T-14 briefs).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArcaneS393Test, "Memoria.S393.Arcane", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArcaneS393Test::RunTest(const FString&)
{
    using namespace MemoriaManuscript;
    const auto* City = MemoriaChapterMaps::Find(ArcaneMap);
    if (!TestNotNull(TEXT("Arcane is a chapter map"), City)) return false;
    TestEqual(TEXT("Chapter 16"), City->Chapter, 16);
    TestEqual(TEXT("Its level"), MemoriaChapterMaps::LevelPath(ArcaneMap), FString(TEXT("/Game/Memoria/Maps/L_ArcaneCity")));
    TestEqual(TEXT("In Korean"), MemoriaChapterMaps::Korean(ArcaneMap), FString(TEXT("아르케인")));
    // A ground of its own (Gemini T-14): booths, bench, bread window, cistern, the door with the brass ring.
    const int32 Booth = City->TileNames.IndexOfByKey(TEXT("BOOTH")), Bench = City->TileNames.IndexOfByKey(TEXT("BENCH"));
    const int32 Window = City->TileNames.IndexOfByKey(TEXT("WINDOW")), Cistern = City->TileNames.IndexOfByKey(TEXT("CISTERN")), Door = City->TileNames.IndexOfByKey(TEXT("DOOR"));
    TestTrue(TEXT("Its own tiles"), Booth != INDEX_NONE && Bench != INDEX_NONE && Window != INDEX_NONE && Cistern != INDEX_NONE && Door != INDEX_NONE && !City->TileNames.Contains(TEXT("STALL")));
    int32 Booths = 0; FIntPoint Third(-1, -1);
    for (int32 Y = 0; Y < City->Height; ++Y)
        for (int32 X = 0; X < City->Width; ++X)
            if (City->TileAt(X, Y) == Booth) { if (++Booths == 3) Third = FIntPoint(X, Y); }
    TestEqual(TEXT("Five confession booths"), Booths, 5);
    TestTrue(TEXT("all solid, on the second square's south side"), City->IsSolid(Booth) && Third.Y == 9);
    auto Tile = [&](const FVector2D& P) { return City->TileAt(int32(P.X) / City->TileSize, int32(P.Y) / City->TileSize); };
    auto Open = [&](const FVector2D& P) { return !City->IsSolid(Tile(P)); };
    TestTrue(TEXT("In by the south gate"), Open(City->Spawn) && City->Spawn.Y > (City->Height - 3) * City->TileSize && !City->IsSolid(City->TileAt(12, City->Height - 1)));
    const FVector2D ExitAt = City->Exit.Rect.Center();
    TestTrue(TEXT("The exit stands before the door with the brass ring"), Open(ExitAt) && City->TileAt(int32(ExitAt.X) / City->TileSize, int32(ExitAt.Y) / City->TileSize - 1) == Door);
    for (const auto& T : City->Triggers) TestTrue(*(TEXT("Open ground: ") + T.Group), Open(T.Rect.Center()));
    const auto& AtBooth = City->Triggers[1];
    TestTrue(TEXT("The old man's scene plays before the third booth"), AtBooth.Group == TEXT("ms_arcane_booth") && int32(AtBooth.Rect.Origin.Y) / City->TileSize == Third.Y - 1);
    const auto& AtBench = City->Triggers[2];
    TestTrue(TEXT("The burn plays by the bench"), AtBench.Group == TEXT("ms_arcane_burn") && City->TileAt(int32(AtBench.Rect.Center().X) / City->TileSize, int32(AtBench.Rect.Center().Y) / City->TileSize + 1) == Bench);
    TestTrue(TEXT("No fights inside the walls"), City->Battles.IsEmpty() && City->EncountersGate == TEXT("ms_never"));
    TestTrue(TEXT("No lamps by day"), City->Decorations.IsEmpty());
    TestEqual(TEXT("The city from above"), City->Splash, FString(TEXT("res://assets/cg/generated/env_gray_belt_panorama.png")));
    // The queue at the third booth holds and faces its door (T-8).
    int32 Queue = 0; for (const auto& N : City->AmbientNpcs) Queue += N.bStill && N.bHasFace && FMath::IsNearlyEqual(N.Face.X, 208.f, 1.f) ? 1 : 0;
    TestEqual(TEXT("Five wait at the third booth, facing its door"), Queue, 5);
    TArray<FString> Groups; for (const auto& S : City->Sequence) Groups.Add(S.Group);
    for (const auto& T : City->Triggers) Groups.Add(T.Group);
    Groups.Add(City->Exit.Group); Groups.Append(City->Exit.Then);
    TestEqual(TEXT("Twelve scenes"), Groups.Num(), 12);
    for (const FString& G : Groups) TestTrue(*(TEXT("A manuscript scene: ") + G), IsScene(G));
    TestTrue(TEXT("26화 plays after the room, Kairos alone"), City->Exit.Group == TEXT("ms_arcane_room") && City->Exit.Then.Num() == 5 && City->Exit.Then[0] == TEXT("ms_kairos_report") && City->Exit.Then.Last() == TEXT("ms_kairos_unsent"));
    const auto* Wells = MemoriaChapterMaps::Find(DryWellsMap);
    TestTrue(TEXT("The wells' road leads to Arcane"), Wells && Wells->Exit.NextMap == ArcaneMap && Wells->Exit.NextChapter == 16);
    TestTrue(TEXT("and Arcane's on to Chapter 17"), City->Exit.NextChapter == 17 && City->Exit.Flags.Contains(TEXT("canon_ch17_hanna_ready")));

    auto Has = [](const UMemoriaFieldAsset* A, const TCHAR* Speaker, const TCHAR* Ko)
    { return A && A->Definition.Rows.ContainsByPredicate([&](const FMemoriaFieldRow& R) { return (Speaker ? R.Text.Speaker == Speaker : !R.Text.bHasSpeaker) && (R.Text.TextKo == Ko || R.Text.NarrateKo == Ko); }); };
    auto Scene = [](const TCHAR* G) { return BuildScene(GetTransientPackage(), G, 0); };
    const auto* Gate = Scene(TEXT("ms_arcane_gate"));
    TestTrue(TEXT("25화 opens at the gate"), Gate && Gate->Definition.Rows[0].Text.NarrateKo.StartsWith(TEXT("성문 서기는 토비아스만 안으로 불렀다.")));
    TestTrue(TEXT("Under his name"), Has(Gate, TEXT("Tobias"), TEXT("제 이름 밑에 적으세요. 같이 머물 겁니다.")));
    TestTrue(TEXT("Look at the ground"), Has(Gate, TEXT("Tobias"), TEXT("여기서는 사람을 오래 보지 마세요. 서기가 부르면 그때 보고. 다른 때는 바닥을 보는 게 낫습니다.")));
    TestTrue(TEXT("Five booths, a bell on each"), Has(Scene(TEXT("ms_arcane_squares")), nullptr, TEXT("둘째 광장 남쪽에는 나무 부스 다섯이 서 있었다. 사람 하나가 들어가 앉을 크기였다. 앞문이 광장을 향했고, 부스 사이로 뒤쪽의 좁은 통로가 보였다. 문설주마다 작은 종이 달려 있었다.")));
    const auto* AtThird = Scene(TEXT("ms_arcane_booth"));
    TestTrue(TEXT("Whose child"), Has(AtThird, TEXT("Old Man"), TEXT("누구 애냐.")));
    TestTrue(TEXT("Exit assistance"), Has(AtThird, TEXT("Confessor"), TEXT("셋째. 퇴실 보조.")));
    TestTrue(TEXT("Before that"), AtThird && AtThird->Definition.Rows.Last().Text.TextKo == TEXT("그 전에."));
    const auto* Burn = Scene(TEXT("ms_arcane_burn"));
    TestTrue(TEXT("He sets fire to the warmth"), Has(Burn, nullptr, TEXT("아렐은 남아 있던 따뜻함에 불을 붙였다.")));
    TestTrue(TEXT("Three steps"), Has(Burn, TEXT("Arrel"), TEXT("세 걸음. 벤치까지.")));
    TestTrue(TEXT("What he burned"), Has(Scene(TEXT("ms_arcane_alley")), TEXT("Arrel"), TEXT("어릴 때 머리를 받쳐 주던 손.")));
    TestTrue(TEXT("It stays in the tower"), Has(Scene(TEXT("ms_arcane_cistern")), TEXT("Tobias"), TEXT("여기서 태우면 탑에 남습니다.")));
    TestTrue(TEXT("Sit. You first."), Has(Scene(TEXT("ms_arcane_room")), TEXT("Arrel"), TEXT("앉아. 먼저.")));
    TestTrue(TEXT("Observed. Did not intervene."), Has(Scene(TEXT("ms_kairos_report")), nullptr, TEXT("관찰함. 개입하지 않음.")));
    TestTrue(TEXT("Only what you saw"), Has(Scene(TEXT("ms_kairos_booth")), TEXT("Kairos"), TEXT("직접 본 것만 적게.")));
    TestTrue(TEXT("Two with him"), Has(Scene(TEXT("ms_kairos_gate")), nullptr, TEXT("토비아스.")));
    TestTrue(TEXT("He keeps the witness check"), Has(Scene(TEXT("ms_kairos_relay")), TEXT("Kairos"), TEXT("목격자 추가 확인은 내가 하겠네. 이 회신에는 넣지 말게.")));
    const auto* Unsent = Scene(TEXT("ms_kairos_unsent"));
    TestTrue(TEXT("The stopper stays shut"), Has(Unsent, nullptr, TEXT("마개는 닫힌 채였다.")));
    TestTrue(TEXT("Unsubmitted"), Has(Unsent, TEXT("Messenger"), TEXT("미제출로 남깁니까?")) && Has(Unsent, TEXT("Kairos"), TEXT("그렇게 적게.")));
    TestTrue(TEXT("Did I intervene?"), Has(Unsent, nullptr, TEXT("개입했나?")));
    TestTrue(TEXT("Toward the cistern"), Unsent && Unsent->Definition.Rows.Last().Text.NarrateKo == TEXT("그는 저수조 쪽으로 걸음을 빨리했다."));

    const FString Unsent2(TEXT("res://assets/cg/generated/ch14_unsent_incident_report.png"));
    const TSet<FString> Nameless = {TEXT("Gate Clerk"), TEXT("Confessor"), TEXT("Clerk"), TEXT("Child"), TEXT("Old Man"), TEXT("Man in Line"), TEXT("Messenger"), TEXT("Relay Clerk")};
    bool bRows = true, bCast = true, bFaces = true, bKairosApart = true, bArt = true;
    int32 Rows = 0;
    for (const FString& G : SceneGroups())
        if (G.StartsWith(TEXT("ms_arcane_")) || G.StartsWith(TEXT("ms_kairos_")))
            if (const auto* A = BuildScene(GetTransientPackage(), G, 0))
            {
                const bool bKairos = G.StartsWith(TEXT("ms_kairos_"));
                for (int32 I = 0; I < A->Definition.Rows.Num(); ++I, ++Rows)
                {
                    const auto& R = A->Definition.Rows[I];
                    bRows &= R.OriginalIndex == I && (R.Text.bHasTextKo || R.Text.bHasNarrateKo);
                    const FString& Who = R.Text.Speaker;
                    bCast &= !R.Text.bHasSpeaker || Who == TEXT("Arrel") || Who == TEXT("Elia") || Who == TEXT("Tobias") || Who == TEXT("Kairos") || Nameless.Contains(Who);
                    if (Who == TEXT("Tobias")) bFaces &= R.Presentation.Portrait == TEXT("tobias_neutral");
                    if (Who == TEXT("Kairos")) bFaces &= R.Presentation.Portrait == TEXT("kairos_neutral");
                    if (Nameless.Contains(Who)) bFaces &= !R.Presentation.bHasPortrait;
                    // 26화: Kairos never meets the three; in 25화 Kairos is not there at all.
                    bKairosApart &= bKairos ? !(Who == TEXT("Arrel") || Who == TEXT("Elia") || Who == TEXT("Tobias")) : Who != TEXT("Kairos");
                    if (R.Presentation.bHasCg) bArt &= bKairos && (R.Presentation.Cg == Unsent2 || R.Presentation.Cg.IsEmpty());
                }
            }
    TestTrue(TEXT("Rows indexed, in Korean"), bRows);
    TestTrue(TEXT("The cast: the three, Kairos, and Arcane's nameless"), bCast);
    TestTrue(TEXT("Faces: Tobias's and Kairos's; the nameless have none"), bFaces);
    TestTrue(TEXT("Kairos and the three are never in one scene"), bKairosApart);
    TestTrue(TEXT("Pictures only in Kairos's room (and black in the relay office)"), bArt);
    TestEqual(TEXT("312 rows in twelve scenes"), Rows, 312);
    TestEqual(TEXT("Kairos's plain face"), MemoriaNarrativeArtwork::PortraitSource(TEXT("kairos_neutral")), FString(TEXT("res://assets/portraits/kairos_face_neutral.png")));
    TestNotNull(TEXT("is imported"), MemoriaNarrativeArtwork::Load(TEXT("res://assets/portraits/kairos_face_neutral.png")));
    TestNotNull(TEXT("and so is the city"), MemoriaNarrativeArtwork::Load(City->Splash));
    TestNotNull(TEXT("and his unsent report"), MemoriaNarrativeArtwork::Load(Unsent2));
    TestEqual(TEXT("The burn in Korean"), MemoriaKorean::Text(TEXT("Burned: the hand that held his head")), FString(TEXT("연소: 머리를 받쳐 주던 손")));
    TestEqual(TEXT("Unsubmitted in Korean"), MemoriaKorean::Text(TEXT("Kairos: unsubmitted")), FString(TEXT("카이로스: 미제출")));
    TestEqual(TEXT("The title in Korean"), MemoriaKorean::Text(TEXT("The Confessional")), FString(TEXT("고해소")));

    TStrongObjectPtr<UGameInstance> Game(NewObject<UGameInstance>()); Game->Init();
    auto* Run = Game->GetSubsystem<UMemoriaRunSubsystem>();
    if (!TestTrue(TEXT("A run at Chapter 16"), Run->BeginStartingMemoryRun(16) == EMemoriaMemoryResult::Success)) { Game->Shutdown(); return false; }
    Run->SetLocale(TEXT("ko"));
    const TCHAR* Steps[][2] = {{TEXT("ch16_gate"), TEXT("목표  첫 광장: 빵 창구")}, {TEXT("ch16_squares"), TEXT("목표  둘째 광장: 셋째 부스")},
        {TEXT("ch16_booth"), TEXT("목표  셋째 부스 옆 낮은 벤치")}, {TEXT("ch16_burn"), TEXT("목표  북서쪽 골목으로")},
        {TEXT("ch16_alley"), TEXT("목표  저수조의 돌벽으로")}, {TEXT("ch16_cistern"), TEXT("목표  낮은 아치 골목 끝: 놋쇠 고리 문")}};
    for (const auto& S : Steps) { Run->SetStoryFlag(S[0], true); TestEqual(*FString(S[0]), MemoriaNavigationGoal::Line(Run->GetRunSnapshot(), ArcaneMap, false), FString(S[1])); }
    for (const auto& S : Steps) Run->SetStoryFlag(S[0], false);
    for (const auto& M : MainSteps()) if (!FString(M.Flag).StartsWith(TEXT("ch16_")) && FString(M.Flag) != TEXT("ms_never")) Run->SetStoryFlag(M.Flag, true);
    TestTrue(TEXT("The main quest is the gate"), MainRecord(Run->GetRunSnapshot(), true).Body.Contains(TEXT("→ 아르케인: 성문, 배지는 하나")));
    Run->SetStoryFlag(TEXT("ch16_gate"), true);
    TestTrue(TEXT("then the third booth"), MainRecord(Run->GetRunSnapshot(), true).Body.Contains(TEXT("→ 셋째 부스: 노인과 아이")));
    for (const TCHAR* F : {TEXT("ch16_burn"), TEXT("ch16_complete")}) Run->SetStoryFlag(F, true);
    TestTrue(TEXT("then the room, and Kairos's silence"), MainRecord(Run->GetRunSnapshot(), true).Body.Contains(TEXT("→ 토비아스의 방. 어딘가에서 카이로스는 회신하지 않았다")));
    Game->Shutdown();
    return true;
}

namespace
{
// S393 in the renderer: in by the gate, the squares, the third booth, the burn and the bench, the alley, the cistern,
// the room; then 26화's five cutaways play one after another from the exit, and the card closes Chapter 16.
class FArcaneReplay final : public IAutomationLatentCommand
{
public:
    explicit FArcaneReplay(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
    ~FArcaneReplay() override { if (bFixed) { FApp::SetUseFixedTimeStep(bOldFixed); FApp::SetFixedDeltaTime(OldDelta); } }
    bool Update() override
    {
        if (LastFrame == GFrameCounter) return false;
        LastFrame = GFrameCounter;
        if (FPlatformTime::Seconds() - Started > 900) { Test->AddError(FString::Printf(TEXT("S393 timeout at step %d"), Step)); return true; }
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
        auto Capture = [&](const FString& Name) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Validation/S393Arcane") / (Name + TEXT(".png")), true, false); };
        auto Flag = [&](const FString& Id) { return Run->GetRunSnapshot().GetFlag(Id); };
        auto Place = [&](const FVector2D& Source) { Pawn->SetActorLocation(MemoriaChapterMaps::ToWorld(Source) + FVector(0, 0, Pawn->GetActorLocation().Z)); };
        if (Host->GetState() == EMemoriaSliceState::Field)
        {
            const FMemoriaNarrativeView View = Host->GetView();
            for (const auto& Shot : Shots)
                if (!Taken.Contains(Shot.Key) && View.CueKey == Shot.Key) { Taken.Add(Shot.Key); ShotAt = Frame; ShotName = Shot.Value; }
            if (View.CueKey.StartsWith(TEXT("field:ms_arcane_")) || View.CueKey.StartsWith(TEXT("field:ms_kairos_"))) Seen.Add(View.CueKey);
            if (!ShotName.IsEmpty() && Frame == ShotAt + 20) { Capture(ShotName); ShotName.Reset(); return false; }
            if (ShotName.IsEmpty() && Frame % 6 == 0) Host->Confirm();
            return false;
        }
        auto Next = [&](const TCHAR* Done, int32 TriggerIndex, const TCHAR* Shot = nullptr)
        {
            if (Flag(Done) && Host->GetState() == EMemoriaSliceState::Exploration && Frame > Mark + 20)
            {
                if (Shot && !bShot) { bShot = true; Capture(Shot); Mark = Frame; return false; }
                bShot = false;
                if (TriggerIndex >= 0) Place(Spec->Triggers[TriggerIndex].Rect.Center()); else Place(Spec->Exit.Rect.Center());
                ++Step; Mark = Frame; return true;
            }
            if (Frame > Mark + 12000) { Test->AddError(FString::Printf(TEXT("Stuck before %s"), Done)); Step = 99; }
            return false;
        };
        switch (Step)
        {
        case 0:
            Test->TestEqual(TEXT("The map is Arcane"), Map->GetMap(), FString(MemoriaManuscript::ArcaneMap));
            Test->TestEqual(TEXT("A development run in Chapter 16"), Run->GetRunSnapshot().CurrentChapter, int64(16));
            { int32 Count = 0; for (TActorIterator<AMemoriaTobiasCompanion> It(World); It; ++It) ++Count; Test->TestEqual(TEXT("Tobias walks with them"), Count, 1); }
            ++Step; Mark = Frame; break;
        case 1:
            if (Frame == Mark + 40) Capture(TEXT("S393Title"));
            if (Flag(TEXT("ch16_gate")) && Host->GetState() == EMemoriaSliceState::Exploration && Frame > Mark + 60)
            { Capture(TEXT("S393GateStreet")); Place(Spec->Triggers[0].Rect.Center()); ++Step; Mark = Frame; }
            if (Frame > Mark + 12000) { Test->AddError(TEXT("The arrival did not finish")); return true; }
            break;
        case 2: Next(TEXT("ch16_squares"), 1, TEXT("S393Squares")); break;
        case 3: Next(TEXT("ch16_booth"), 2, TEXT("S393Booths")); break;
        case 4:
            if (Flag(TEXT("ch16_burn"))) Test->TestTrue(TEXT("Burned: the hand that held his head"), Flag(TEXT("ch16_burned_hand")) && Host->GetTrace().Contains(TEXT("toast:Burned: the hand that held his head")));
            Next(TEXT("ch16_burn"), 3, TEXT("S393Bench")); break;
        case 5: Next(TEXT("ch16_alley"), 4); break;
        case 6: Next(TEXT("ch16_cistern"), -1, TEXT("S393Cistern")); break;
        case 7:
            if (Map->IsChapterComplete())
            {
                Test->TestTrue(TEXT("Kairos leaves it unsubmitted"), Flag(TEXT("ch16_unsubmitted")) && Host->GetTrace().Contains(TEXT("toast:Kairos: unsubmitted")));
                Test->TestTrue(TEXT("The room and Kairos's evening close Chapter 16"), Flag(TEXT("ch16_complete")) && Flag(TEXT("canon_ch17_hanna_ready")));
                Test->TestTrue(TEXT("Every row of the twelve scenes was shown"), Seen.Num() == 312);
                Test->TestTrue(TEXT("Its last line was shown"), Seen.Contains(TEXT("field:ms_kairos_unsent:37")));
                Test->TestTrue(TEXT("The card shows"), Map->GetCard() && Map->GetCard()->IsShowing());
                ++Step; Mark = Frame;
            }
            if (Frame > Mark + 20000) { Test->AddError(TEXT("The exit chain never closed the chapter")); return true; }
            break;
        case 8:
            if (Frame == Mark + 30) Capture(TEXT("S393Complete"));
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
    bool bFixed = false, bOldFixed = false, bShot = false;
    FString ShotName;
    TSet<FString> Taken, Seen;
    const TArray<TPair<FString, FString>> Shots = {
        {TEXT("field:ms_arcane_gate:13"), TEXT("S393UnderMyName")}, {TEXT("field:ms_arcane_squares:7"), TEXT("S393FiveBooths")},
        {TEXT("field:ms_arcane_booth:5"), TEXT("S393WhoseChild")}, {TEXT("field:ms_arcane_booth:9"), TEXT("S393ExitAssistance")},
        {TEXT("field:ms_arcane_burn:4"), TEXT("S393SetFire")}, {TEXT("field:ms_arcane_burn:11"), TEXT("S393ThreeSteps")},
        {TEXT("field:ms_arcane_alley:15"), TEXT("S393TheHand")}, {TEXT("field:ms_arcane_cistern:5"), TEXT("S393Tower")},
        {TEXT("field:ms_arcane_room:11"), TEXT("S393SitFirst")}, {TEXT("field:ms_kairos_report:0"), TEXT("S393Teacup")},
        {TEXT("field:ms_kairos_booth:20"), TEXT("S393OnlyWhatYouSaw")}, {TEXT("field:ms_kairos_gate:3"), TEXT("S393Tobias")},
        {TEXT("field:ms_kairos_relay:0"), TEXT("S393Relay")}, {TEXT("field:ms_kairos_unsent:19"), TEXT("S393Stopper")},
        {TEXT("field:ms_kairos_unsent:25"), TEXT("S393Unsubmitted")}, {TEXT("field:ms_kairos_unsent:32"), TEXT("S393DidIIntervene")}};
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArcaneVisualTest, "MemoriaVisual.S393Arcane", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArcaneVisualTest::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Memoria/Maps/L_ArcaneCity"))) return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FArcaneReplay(this)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
