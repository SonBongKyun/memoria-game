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
#include "Interaction/MemoriaHannaCompanion.h"
#include "Interaction/MemoriaCart.h"
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

// S398: Chapter 18, the east road, from 최종본 31화 「늙은 편집자」 to 35화 「모르는 얼굴」 (Gemini's T-17 and T-20 briefs, Option A).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEastRoadS398Test, "Memoria.S398.EastRoad", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FEastRoadS398Test::RunTest(const FString&)
{
    using namespace MemoriaManuscript;
    const auto* Road = MemoriaChapterMaps::Find(EastRoadMap);
    if (!TestNotNull(TEXT("The east road is a chapter map"), Road)) return false;
    TestEqual(TEXT("Chapter 18"), Road->Chapter, 18);
    TestEqual(TEXT("Its level"), MemoriaChapterMaps::LevelPath(EastRoadMap), FString(TEXT("/Game/Memoria/Maps/L_EastRoad")));
    TestEqual(TEXT("In Korean"), MemoriaChapterMaps::Korean(EastRoadMap), FString(TEXT("동쪽 길")));
    // A ground of its own: mile stones, low walls, trees, the post, the yard's shed, rail, trough, bench and carts.
    for (const TCHAR* Name : {TEXT("ROAD"), TEXT("FIELD"), TEXT("MILESTONE"), TEXT("LOWWALL"), TEXT("TREE"), TEXT("POST"), TEXT("SHED"), TEXT("RAIL"), TEXT("TROUGH"), TEXT("BENCH"), TEXT("CART")})
        TestTrue(*(FString(TEXT("Tile ")) + Name), Road->TileNames.Contains(Name));
    auto TileOf = [&](const FVector2D& P) { return FIntPoint(int32(P.X) / Road->TileSize, int32(P.Y) / Road->TileSize); };
    auto TypeAt = [&](const FVector2D& P) { const FIntPoint T = TileOf(P); return Road->TileAt(T.X, T.Y); };
    auto Open = [&](const FVector2D& P) { return !Road->IsSolid(TypeAt(P)); };
    int32 Miles = 0, Carts = 0;
    const int32 Mile = Road->TileNames.IndexOfByKey(TEXT("MILESTONE")), Cart = Road->TileNames.IndexOfByKey(TEXT("CART"));
    for (int32 Y = 0; Y < Road->Height; ++Y) for (int32 X = 0; X < Road->Width; ++X) { Miles += Road->TileAt(X, Y) == Mile; Carts += Road->TileAt(X, Y) == Cart; }
    TestEqual(TEXT("Four mile stones: first, second, fourth, seventh"), Miles, 4);
    TestEqual(TEXT("Three other carts in the yard"), Carts, 3);
    TestTrue(TEXT("In from the west, on the road"), Open(Road->Spawn) && TileOf(Road->Spawn).X <= 2 && TypeAt(Road->Spawn) == Road->TileNames.IndexOfByKey(TEXT("ROAD")));
    const FVector2D ExitAt = Road->Exit.Rect.Center();
    TestTrue(TEXT("Out east of the yard"), Open(ExitAt) && TileOf(ExitAt).X >= Road->Width - 3);
    for (const auto& T : Road->Triggers) TestTrue(*(TEXT("Open ground: ") + T.Group), Open(T.Rect.Center()));
    bool bEast = true;
    for (int32 I = 1; I < Road->Triggers.Num(); ++I) bEast &= Road->Triggers[I].Rect.Center().X >= Road->Triggers[I - 1].Rect.Center().X;
    TestTrue(TEXT("The places run west to east"), bEast && ExitAt.X > Road->Triggers.Last().Rect.Center().X);
    TestTrue(TEXT("No fights on the road"), Road->Battles.IsEmpty() && Road->EncountersGate == TEXT("ms_never"));
    TestTrue(TEXT("No lamps lit"), Road->Decorations.IsEmpty());
    TestTrue(TEXT("No splash: the scenes play on black"), Road->Splash.IsEmpty());
    TestEqual(TEXT("Four in the yard, from the post on"), Road->AmbientNpcs.Num(), 4);
    TestEqual(TEXT("shown once the post is passed"), Road->AmbientNpcsGate, FString(TEXT("ch18_post")));
    TArray<FString> Groups; for (const auto& S : Road->Sequence) Groups.Add(S.Group);
    TestTrue(TEXT("Two cutaways open it: Telos, then Kairos's report"), Groups.Num() == 2 && Groups[0] == TEXT("ms_east_telos") && Groups[1] == TEXT("ms_east_report"));
    for (const auto& T : Road->Triggers) Groups.Add(T.Group);
    Groups.Add(Road->Exit.Group);
    TestEqual(TEXT("Ten scenes"), Groups.Num(), 10);
    for (const FString& G : Groups) TestTrue(*(TEXT("A manuscript scene: ") + G), IsScene(G));
    TestTrue(TEXT("33화 plays as the cart leaves the bend"), Road->Triggers.Num() == 7 && Road->Triggers[3].Group == TEXT("ms_east_rest") && Road->Triggers[4].Group == TEXT("ms_east_panel"));
    const auto* Leave = MemoriaChapterMaps::Find(ArcaneLeaveMap);
    TestTrue(TEXT("The south gate leads here"), Leave && Leave->Exit.NextMap == EastRoadMap && Leave->Exit.NextChapter == 18);
    TestTrue(TEXT("and the yard on toward Chapter 19"), Road->Exit.NextChapter == 19 && Road->Exit.NextMap.IsEmpty() && Road->Exit.Flags.Contains(TEXT("canon_ch19_east_onward_ready")));

    auto Has = [](const UMemoriaFieldAsset* A, const TCHAR* Speaker, const TCHAR* Ko)
    { return A && A->Definition.Rows.ContainsByPredicate([&](const FMemoriaFieldRow& R) { return (Speaker ? R.Text.Speaker == Speaker : !R.Text.bHasSpeaker) && (R.Text.TextKo == Ko || R.Text.NarrateKo == Ko); }); };
    auto Scene = [](const TCHAR* G) { return BuildScene(GetTransientPackage(), G, 0); };
    const auto* Telos = Scene(TEXT("ms_east_telos"));
    TestTrue(TEXT("31화 opens at the drawer"), Telos && Telos->Definition.Rows[0].Text.NarrateKo.StartsWith(TEXT("텔로스는 서랍 밑을 받쳐 들었다.")));
    TestTrue(TEXT("Twenty-five years"), Has(Telos, nullptr, TEXT("스물다섯 해가 지났다.")));
    const auto* Report = Scene(TEXT("ms_east_report"));
    TestTrue(TEXT("0.3 degrees, in the report"), Has(Report, nullptr, TEXT("0.3도였다.")));
    TestTrue(TEXT("He does not pick up the set square"), Has(Report, nullptr, TEXT("카이로스는 직각자를 집지 않았다.")));
    TestTrue(TEXT("Nothing to go with it"), Has(Report, TEXT("Kairos"), TEXT("없네.")) && Has(Report, TEXT("Messenger"), TEXT("함께 보낼 것은요?")));
    TestTrue(TEXT("A stone in it?"), Has(Scene(TEXT("ms_east_first_mile")), TEXT("Elia"), TEXT("돌 들어갔어?")));
    const auto* AtCart = Scene(TEXT("ms_east_cart"));
    TestTrue(TEXT("Can't say"), Has(AtCart, TEXT("Driver"), TEXT("말 못 해.")));
    TestTrue(TEXT("The eighth-mile yard"), Has(AtCart, TEXT("Driver"), TEXT("여덟째 마일 마당.")));
    TestTrue(TEXT("Knock"), Has(AtCart, TEXT("Driver"), TEXT("두드려.")));
    TestTrue(TEXT("As far as the yard"), Has(AtCart, TEXT("Arrel"), TEXT("마당까지.")));
    TestTrue(TEXT("How do you know it's ordinary"), Has(Scene(TEXT("ms_east_third_mile")), TEXT("Arrel"), TEXT("평범한지 어떻게 아는데.")));
    TestTrue(TEXT("While you eat, I'll watch"), Has(Scene(TEXT("ms_east_rest")), TEXT("Arrel"), TEXT("먹을 때는 내가 볼게.")));
    const auto* Panel = Scene(TEXT("ms_east_panel"));
    TestTrue(TEXT("0.3 degrees, at the panel"), Has(Panel, nullptr, TEXT("0.3도였다.")));
    TestTrue(TEXT("He does not take the handle"), Has(Panel, nullptr, TEXT("그는 손잡이를 잡지 않았다.")));
    const auto* Post = Scene(TEXT("ms_east_post"));
    TestTrue(TEXT("Nobody came out"), Has(Post, nullptr, TEXT("아무도 나오지 않았다.")));
    TestTrue(TEXT("Saw it at the second mile"), Has(Post, TEXT("Driver"), TEXT("둘째 마일에서 봤어. 아직 동쪽으로 가고 있어.")));
    const auto* Yard = Scene(TEXT("ms_east_yard"));
    TestTrue(TEXT("Dry to the bottom"), Has(Yard, TEXT("Elia"), TEXT("말랐어. 바닥까지.")));
    TestTrue(TEXT("The scar"), Has(Yard, nullptr, TEXT("왼쪽 눈썹에서 비스듬히 내려온 흉터가 눈가와 콧등을 지나 광대뼈 위에서 끝났다. 오래되어 색이 옅어진 자국이었다.")));
    TestTrue(TEXT("Not that I remember"), Has(Yard, TEXT("Arrel"), TEXT("기억에 없어.")));
    TestTrue(TEXT("He saw your hand"), Has(Yard, TEXT("Tobias"), TEXT("당신 오른손을요. 손을 보고 나서 얼굴을 다시 봤습니다.")));
    const auto* Onward = Scene(TEXT("ms_east_onward"));
    TestTrue(TEXT("Get in"), Has(Onward, TEXT("Driver"), TEXT("타.")));
    TestTrue(TEXT("Onto the east road"), Onward && Onward->Definition.Rows.Last().Text.NarrateKo == TEXT("수레는 동쪽 길로 올라섰다."));

    const FString Unsent(TEXT("res://assets/cg/generated/ch14_unsent_incident_report.png"));
    bool bRows = true, bCast = true, bFaces = true, bArt = true, bKairosApart = true, bOpensDark = true;
    int32 Rows = 0, DriverLines = 0;
    for (const FString& G : SceneGroups())
        if (G.StartsWith(TEXT("ms_east_")))
            if (const auto* A = BuildScene(GetTransientPackage(), G, 0))
            {
                const bool bKairos = G == TEXT("ms_east_report") || G == TEXT("ms_east_panel") || G == TEXT("ms_east_telos");
                bOpensDark &= A->Definition.Rows.Num() > 0 && A->Definition.Rows[0].Presentation.bHasCg;
                for (int32 I = 0; I < A->Definition.Rows.Num(); ++I, ++Rows)
                {
                    const auto& R = A->Definition.Rows[I];
                    bRows &= R.OriginalIndex == I && (R.Text.bHasTextKo || R.Text.bHasNarrateKo);
                    const FString& Who = R.Text.Speaker;
                    // The man in the grey coat never speaks; Telos never speaks; nobody is named for him.
                    bCast &= !R.Text.bHasSpeaker || Who == TEXT("Arrel") || Who == TEXT("Elia") || Who == TEXT("Tobias") || Who == TEXT("Kairos") || Who == TEXT("Messenger") || Who == TEXT("Driver");
                    DriverLines += Who == TEXT("Driver");
                    if (Who == TEXT("Tobias")) bFaces &= R.Presentation.Portrait == TEXT("tobias_neutral");
                    if (Who == TEXT("Kairos")) bFaces &= R.Presentation.Portrait == TEXT("kairos_neutral");
                    if (Who == TEXT("Driver") || Who == TEXT("Messenger")) bFaces &= !R.Presentation.bHasPortrait;
                    // Kairos and the three are never in one scene.
                    bKairosApart &= bKairos ? !(Who == TEXT("Arrel") || Who == TEXT("Elia") || Who == TEXT("Tobias")) : Who != TEXT("Kairos");
                    // Black, except Kairos's own room over his unsent-report desk.
                    if (R.Presentation.bHasCg) bArt &= R.Presentation.Cg.IsEmpty() || (bKairos && R.Presentation.Cg == Unsent);
                }
            }
    TestTrue(TEXT("Rows indexed, in Korean"), bRows);
    TestTrue(TEXT("The cast: the three, Kairos, the messenger, the driver; nobody else speaks"), bCast);
    TestEqual(TEXT("The driver has his six lines"), DriverLines, 6);
    TestTrue(TEXT("Faces: Tobias's and Kairos's; the driver and the messenger have none"), bFaces);
    TestTrue(TEXT("Kairos and the three are never in one scene"), bKairosApart);
    TestTrue(TEXT("Every scene opens with its picture set (black or Kairos's desk)"), bOpensDark);
    TestTrue(TEXT("Black, except Kairos's room"), bArt);
    TestEqual(TEXT("255 rows in ten scenes"), Rows, 255);
    TestNotNull(TEXT("His desk is imported"), MemoriaNarrativeArtwork::Load(Unsent));
    TestEqual(TEXT("The title in Korean"), MemoriaKorean::Text(TEXT("The East Road")), FString(TEXT("동쪽 길")));
    TestEqual(TEXT("The subtitle in Korean"), MemoriaKorean::Text(TEXT("An Unknown Face")), FString(TEXT("모르는 얼굴")));
    TestEqual(TEXT("The toast in Korean"), MemoriaKorean::Text(TEXT("A face he does not know")), FString(TEXT("모르는 얼굴")));
    TestEqual(TEXT("The yard in Korean"), MemoriaKorean::Text(TEXT("The Eighth-Mile Yard")), FString(TEXT("여덟째 마일 마당")));

    TStrongObjectPtr<UGameInstance> Game(NewObject<UGameInstance>()); Game->Init();
    auto* Run = Game->GetSubsystem<UMemoriaRunSubsystem>();
    if (!TestTrue(TEXT("A run at Chapter 18"), Run->BeginStartingMemoryRun(18) == EMemoriaMemoryResult::Success)) { Game->Shutdown(); return false; }
    Run->SetLocale(TEXT("ko"));
    const TCHAR* Steps[][2] = {{TEXT("ch18_report"), TEXT("목표  동쪽으로: 첫째 마일 돌")}, {TEXT("ch18_first_mile"), TEXT("목표  둘째 마일: 마차")},
        {TEXT("ch18_cart"), TEXT("목표  셋째 마일로")}, {TEXT("ch18_third_mile"), TEXT("목표  넷째 마일 굽이: 나무 밑")},
        {TEXT("ch18_rest"), TEXT("목표  굽이를 지나 동쪽으로")}, {TEXT("ch18_panel"), TEXT("목표  일곱째 마일 초소")},
        {TEXT("ch18_post"), TEXT("목표  비탈 위로: 여덟째 마일 마당")}, {TEXT("ch18_yard"), TEXT("목표  마당 동쪽으로: 같은 수레")}};
    for (const auto& S : Steps) { Run->SetStoryFlag(S[0], true); TestEqual(*FString(S[0]), MemoriaNavigationGoal::Line(Run->GetRunSnapshot(), EastRoadMap, false), FString(S[1])); }
    for (const auto& S : Steps) Run->SetStoryFlag(S[0], false);
    for (const auto& M : MainSteps()) if (!FString(M.Flag).StartsWith(TEXT("ch18_")) && FString(M.Flag) != TEXT("ms_never")) Run->SetStoryFlag(M.Flag, true);
    TestTrue(TEXT("The main quest is the cart"), MainRecord(Run->GetRunSnapshot(), true).Body.Contains(TEXT("→ 동쪽으로 가는 마차까지 두 마일. 아직 정한 건 없다")));
    Run->SetStoryFlag(TEXT("ch18_cart"), true);
    TestTrue(TEXT("then east with the driver"), MainRecord(Run->GetRunSnapshot(), true).Body.Contains(TEXT("→ 후드 쓴 마부와 동쪽으로, 마당까지")));
    Run->SetStoryFlag(TEXT("ch18_post"), true);
    TestTrue(TEXT("then the yard"), MainRecord(Run->GetRunSnapshot(), true).Body.Contains(TEXT("→ 여덟째 마일 마당: 내릴지, 더 탈지")));
    Run->SetStoryFlag(TEXT("ch18_complete"), true);
    TestTrue(TEXT("then on east"), MainRecord(Run->GetRunSnapshot(), true).Body.Contains(TEXT("→ 같은 수레로 더 동쪽으로. 책은 여전히 북서쪽을 가리킨다")));
    Game->Shutdown();
    return true;
}

namespace
{
// S398 in the renderer: the two cutaways; the road from the first mile to the yard, one place after another; out east.
class FEastRoadReplay final : public IAutomationLatentCommand
{
public:
    explicit FEastRoadReplay(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
    ~FEastRoadReplay() override { if (bFixed) { FApp::SetUseFixedTimeStep(bOldFixed); FApp::SetFixedDeltaTime(OldDelta); } }
    bool Update() override
    {
        if (LastFrame == GFrameCounter) return false;
        LastFrame = GFrameCounter;
        if (FPlatformTime::Seconds() - Started > 1200) { Test->AddError(FString::Printf(TEXT("S398 timeout at step %d"), Step)); return true; }
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
        auto Capture = [&](const FString& Name) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Validation/S398East") / (Name + TEXT(".png")), true, false); };
        auto Flag = [&](const FString& Id) { return Run->GetRunSnapshot().GetFlag(Id); };
        AMemoriaCart* Cart = nullptr; int32 Carts = 0; for (TActorIterator<AMemoriaCart> It(World); It; ++It) { Cart = *It; ++Carts; }
        const bool bExploring = Host->GetState() == EMemoriaSliceState::Exploration;
        // S399: once aboard, the cart carries Arrel from place to place; the replay only waits and looks.
        auto Ride = [&](const TCHAR* Done, const TCHAR* Shot)
        {
            if (Flag(Done) && bExploring && Frame > Mark + 20)
            {
                if (!bShot) { bShot = true; Mark = Frame; return; }
                if (Frame == Mark + 30) { Capture(Shot); return; }
                if (Frame < Mark + 40) return;
                bShot = false; ++Step; Mark = Frame; return;
            }
            if (Frame > Mark + 30000) { Test->AddError(FString::Printf(TEXT("The cart never brought them to %s"), Done)); Step = 99; }
        };
        auto Place = [&](const FVector2D& Source) { Pawn->SetActorLocation(MemoriaChapterMaps::ToWorld(Source) + FVector(0, 0, Pawn->GetActorLocation().Z)); };
        if (Host->GetState() == EMemoriaSliceState::Field)
        {
            const FMemoriaNarrativeView View = Host->GetView();
            for (const auto& Shot : Shots)
                if (!Taken.Contains(Shot.Key) && View.CueKey == Shot.Key) { Taken.Add(Shot.Key); ShotAt = Frame; ShotName = Shot.Value; }
            if (View.CueKey.StartsWith(TEXT("field:ms_east_"))) Seen.Add(View.CueKey);
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
            Test->TestEqual(TEXT("The map is the east road"), Map->GetMap(), FString(MemoriaManuscript::EastRoadMap));
            Test->TestEqual(TEXT("A development run in Chapter 18"), Run->GetRunSnapshot().CurrentChapter, int64(18));
            { int32 Count = 0; for (TActorIterator<AMemoriaTobiasCompanion> It(World); It; ++It) ++Count; Test->TestEqual(TEXT("Tobias rides with them"), Count, 1); }
            { int32 Count = 0; for (TActorIterator<AMemoriaHannaCompanion> It(World); It; ++It) ++Count; Test->TestEqual(TEXT("Hanna stayed in Arcane"), Count, 0); }
            ++Step; Mark = Frame; break;
        case 1:
            if (Frame == Mark + 40) Capture(TEXT("S398Title"));
            if (Flag(TEXT("ch18_report")) && Host->GetState() == EMemoriaSliceState::Exploration && Frame > Mark + 60)
            {
                Test->TestTrue(TEXT("The report went 0.3 degrees off true"), Flag(TEXT("ch18_kairos_tilted")));
                Capture(TEXT("S398GateRoad")); Place(Spec->Triggers[0].Rect.Center()); ++Step; Mark = Frame;
            }
            if (Frame > Mark + 20000) { Test->AddError(TEXT("The cutaways did not finish")); return true; }
            break;
        case 2: Next(TEXT("ch18_first_mile"), 1, TEXT("S398FirstMile")); break;
        case 3:
            // The second mile: the three board and the cart drives off east.
            if (Flag(TEXT("ch18_cart")) && bExploring && Frame > Mark + 20 && !bShot)
            {
                Test->TestTrue(TEXT("They board the cart"), Flag(TEXT("ch18_cart_boarded")));
                Test->TestTrue(TEXT("One cart, and the three ride it"), Carts == 1 && Cart && Cart->IsRiding());
            }
            Ride(TEXT("ch18_cart"), TEXT("S398Riding")); break;
        case 4: Ride(TEXT("ch18_third_mile"), TEXT("S398LowWalls")); break;
        case 5: Ride(TEXT("ch18_rest"), TEXT("S398Bend")); break;
        case 6:
            if (Flag(TEXT("ch18_panel")) && bExploring) Test->TestTrue(TEXT("Kairos moves nothing"), Flag(TEXT("ch18_panel_unmoved")));
            Ride(TEXT("ch18_panel"), TEXT("S398PastTheBend")); break;
        case 7:
            if (Flag(TEXT("ch18_post")) && bExploring)
                Test->TestTrue(TEXT("The driver turned round, and back"), Flag(TEXT("ch18_driver_turned")) && Cart && !Cart->IsDriverTurnedTowardBed());
            Ride(TEXT("ch18_post"), TEXT("S398Post")); break;
        case 8:
            if (Flag(TEXT("ch18_yard")) && bExploring)
            {
                Test->TestTrue(TEXT("A face he does not know"), Flag(TEXT("ch18_unknown_face")) && Host->GetTrace().Contains(TEXT("toast:A face he does not know")));
                Test->TestTrue(TEXT("They got down in the yard"), Cart && !Cart->IsRiding());
            }
            // Down in the yard: walk east to the same cart.
            if (Flag(TEXT("ch18_yard")) && bExploring && Frame > Mark + 20)
            {
                if (!bShot) { bShot = true; Mark = Frame; break; }
                if (Frame == Mark + 30) { Capture(TEXT("S398Yard")); break; }
                if (Frame < Mark + 40) break;
                bShot = false; Place(Spec->Exit.Rect.Center()); ++Step; Mark = Frame; break;
            }
            if (Frame > Mark + 30000) { Test->AddError(TEXT("The cart never reached the yard")); return true; }
            break;
        case 9:
            if (Map->IsChapterComplete())
            {
                Test->TestTrue(TEXT("On east closes Chapter 18"), Flag(TEXT("ch18_complete")) && Flag(TEXT("canon_ch19_east_onward_ready")));
                Test->TestTrue(TEXT("Every row of the ten scenes was shown"), Seen.Num() == 255);
                Test->TestTrue(TEXT("Its last line was shown"), Seen.Contains(TEXT("field:ms_east_onward:23")));
                Test->TestTrue(TEXT("The card shows"), Map->GetCard() && Map->GetCard()->IsShowing());
                Test->TestTrue(TEXT("Aboard again, on east"), Cart && Cart->IsRiding());
                ++Step; Mark = Frame;
            }
            if (Frame > Mark + 12000) { Test->AddError(TEXT("The yard never closed the chapter")); return true; }
            break;
        case 10:
            if (Frame == Mark + 30) Capture(TEXT("S398Complete"));
            if (Frame == Mark + 150) Capture(TEXT("S398OnEast"));
            if (Frame < Mark + 160) break;
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
        {TEXT("field:ms_east_telos:0"), TEXT("S398Drawer")}, {TEXT("field:ms_east_telos:9"), TEXT("S398TwentyFive")},
        {TEXT("field:ms_east_report:0"), TEXT("S398KairosRoom")}, {TEXT("field:ms_east_report:9"), TEXT("S398PointThree")},
        {TEXT("field:ms_east_report:23"), TEXT("S398Nothing")}, {TEXT("field:ms_east_first_mile:2"), TEXT("S398AStone")},
        {TEXT("field:ms_east_cart:9"), TEXT("S398CantSay")}, {TEXT("field:ms_east_cart:22"), TEXT("S398Knock")},
        {TEXT("field:ms_east_third_mile:6"), TEXT("S398Ordinary")}, {TEXT("field:ms_east_rest:34"), TEXT("S398IllWatch")},
        {TEXT("field:ms_east_panel:7"), TEXT("S398SilenceUnderBreath")}, {TEXT("field:ms_east_panel:16"), TEXT("S398NotTheHandle")},
        {TEXT("field:ms_east_panel:19"), TEXT("S398KairosBack")}, {TEXT("field:ms_east_post:13"), TEXT("S398DriverTurns")},
        {TEXT("field:ms_east_yard:9"), TEXT("S398DryTrough")}, {TEXT("field:ms_east_yard:11"), TEXT("S398Scar")},
        {TEXT("field:ms_east_yard:19"), TEXT("S398NotRemember")}, {TEXT("field:ms_east_onward:6"), TEXT("S398GetIn")},
        {TEXT("field:ms_east_onward:23"), TEXT("S398OntoTheRoad")}};
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEastRoadVisualTest, "MemoriaVisual.S398EastRoad", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FEastRoadVisualTest::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Memoria/Maps/L_EastRoad"))) return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FEastRoadReplay(this)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
