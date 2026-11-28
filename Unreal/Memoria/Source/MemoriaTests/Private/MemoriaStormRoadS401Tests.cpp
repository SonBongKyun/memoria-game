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

// S401: Chapter 19, the storm road, from 최종본 36화 「네라」 to 39화 「등록관의 손」 (Gemini's T-23 brief, Option A).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStormRoadS401Test, "Memoria.S401.StormRoad", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStormRoadS401Test::RunTest(const FString&)
{
    using namespace MemoriaManuscript;
    const auto* Road = MemoriaChapterMaps::Find(StormRoadMap);
    if (!TestNotNull(TEXT("The storm road is a chapter map"), Road)) return false;
    TestEqual(TEXT("Chapter 19"), Road->Chapter, 19);
    TestEqual(TEXT("Its level"), MemoriaChapterMaps::LevelPath(StormRoadMap), FString(TEXT("/Game/Memoria/Maps/L_StormRoad")));
    TestEqual(TEXT("In Korean"), MemoriaChapterMaps::Korean(StormRoadMap), FString(TEXT("폭풍의 길")));
    for (const TCHAR* Name : {TEXT("ROAD"), TEXT("FIELD"), TEXT("MOUND"), TEXT("STONE"), TEXT("BOULDER"), TEXT("THINTREE")})
        TestTrue(*(FString(TEXT("Tile ")) + Name), Road->TileNames.Contains(Name));
    auto TileOf = [&](const FVector2D& P) { return FIntPoint(int32(P.X) / Road->TileSize, int32(P.Y) / Road->TileSize); };
    auto Open = [&](const FVector2D& P) { const FIntPoint T = TileOf(P); return !Road->IsSolid(Road->TileAt(T.X, T.Y)); };
    TestTrue(TEXT("In from the west, on the road"), Open(Road->Spawn) && TileOf(Road->Spawn).X <= 2);
    for (const auto& T : Road->Triggers) TestTrue(*(TEXT("Open ground: ") + T.Group), Open(T.Rect.Center()));
    TestEqual(TEXT("Five places on the way"), Road->Triggers.Num(), 5);
    // Two scenes at each of the first two stops: the storm and the burn; the shell and "세우지 말자."
    TestTrue(TEXT("The storm, then the burn, at one stop"), Road->Triggers.Num() == 5 && Road->Triggers[0].Rect.Center() == Road->Triggers[1].Rect.Center() && Road->Triggers[1].Gate == Road->Triggers[0].Flag);
    TestTrue(TEXT("The shell, then the count, at the next"), Road->Triggers.Num() == 5 && Road->Triggers[2].Rect.Center() == Road->Triggers[3].Rect.Center() && Road->Triggers[3].Gate == Road->Triggers[2].Flag);
    TestTrue(TEXT("The places run west to east"), Road->Triggers.Num() == 5 && Road->Triggers[2].Rect.Center().X > Road->Triggers[0].Rect.Center().X && Road->Triggers[4].Rect.Center().X > Road->Triggers[2].Rect.Center().X);
    const FIntPoint ExitTile = TileOf(Road->Exit.Rect.Center());
    int32 TreesNear = 0;
    for (int32 Y = ExitTile.Y - 2; Y <= ExitTile.Y + 2; ++Y) for (int32 X = ExitTile.X - 2; X <= ExitTile.X + 2; ++X) TreesNear += Road->TileAt(X, Y) == Road->TileNames.IndexOfByKey(TEXT("THINTREE"));
    TestTrue(TEXT("The exit is the empty ground among thin trees"), Open(Road->Exit.Rect.Center()) && TreesNear >= 3);
    TestTrue(TEXT("No fights"), Road->Battles.IsEmpty() && Road->EncountersGate == TEXT("ms_never"));
    TestTrue(TEXT("No lamps, no splash"), Road->Decorations.IsEmpty() && Road->Splash.IsEmpty());
    TArray<FString> Groups; for (const auto& S : Road->Sequence) Groups.Add(S.Group);
    TestTrue(TEXT("Lumea first: Nera"), Groups.Num() == 2 && Groups[0] == TEXT("ms_storm_nera") && Groups[1] == TEXT("ms_storm_weather"));
    for (const auto& T : Road->Triggers) Groups.Add(T.Group);
    Groups.Add(Road->Exit.Group); Groups.Append(Road->Exit.Then);
    TestEqual(TEXT("Ten scenes"), Groups.Num(), 10);
    for (const FString& G : Groups) TestTrue(*(TEXT("A manuscript scene: ") + G), IsScene(G));
    TestTrue(TEXT("Lumea last: Veil, as the exit's chain"), Road->Exit.Group == TEXT("ms_storm_veil") && Road->Exit.Then.Num() == 2 && Road->Exit.Then.Last() == TEXT("ms_storm_rubbing"));
    const auto* East = MemoriaChapterMaps::Find(EastRoadMap);
    TestTrue(TEXT("The yard's road leads here"), East && East->Exit.NextMap == StormRoadMap && East->Exit.NextChapter == 19);
    TestTrue(TEXT("and on toward Chapter 20"), Road->Exit.NextChapter == 20 && Road->Exit.NextMap.IsEmpty() && Road->Exit.Flags.Contains(TEXT("canon_ch20_white_country_ready")));

    auto Has = [](const UMemoriaFieldAsset* A, const TCHAR* Speaker, const TCHAR* Ko)
    { return A && A->Definition.Rows.ContainsByPredicate([&](const FMemoriaFieldRow& R) { return (Speaker ? R.Text.Speaker == Speaker : !R.Text.bHasSpeaker) && (R.Text.TextKo == Ko || R.Text.NarrateKo == Ko); }); };
    auto Scene = [](const TCHAR* G) { return BuildScene(GetTransientPackage(), G, 0); };
    const auto* Nera = Scene(TEXT("ms_storm_nera"));
    TestTrue(TEXT("36화 opens on the notice"), Nera && Nera->Definition.Rows[0].Text.NarrateKo.StartsWith(TEXT("네라는 통지서에 적힌 이름을 맞춰 보았다.")));
    TestTrue(TEXT("The identification, not a name"), Has(Nera, nullptr, TEXT("여덟째 마일 마당에서 오른손을 검 자루로 가져간 남자.")));
    const auto* Weather = Scene(TEXT("ms_storm_weather"));
    TestTrue(TEXT("Send it under that identification"), Has(Weather, TEXT("Nera"), TEXT("신청 대상과 일치합니다. 그 식별로 넘기세요.")));
    TestTrue(TEXT("The weather note"), Has(Weather, nullptr, TEXT("벨트 동부 격자, 기상 시그니처 패턴 이탈.")));
    const auto* Wind = Scene(TEXT("ms_storm_wind"));
    TestTrue(TEXT("A third-district storm"), Has(Wind, TEXT("Driver"), TEXT("제3구 폭풍이다. 달아나서 될 일이 아냐.")));
    TestTrue(TEXT("It opened by itself"), Has(Wind, TEXT("Elia"), TEXT("아니. 내가 연 것도 아니야.")));
    const auto* Burn = Scene(TEXT("ms_storm_burn"));
    TestTrue(TEXT("I'll burn that time"), Has(Burn, TEXT("Arrel"), TEXT("그때를 태울게.")) && Has(Burn, TEXT("Elia"), TEXT("내 것도.")));
    TestTrue(TEXT("The two hands locked"), Has(Burn, nullptr, TEXT("두 손이 단단히 맞물렸다.")));
    TestTrue(TEXT("The cost on her face"), Has(Burn, nullptr, TEXT("엘리아의 뺨이 달라져 있었다. 광대 아래의 오목한 부분이 드러났다. 턱선도 조금 전보다 얇았다. 두 사람이 잡은 손 아래로 펼쳐진 책의 흰 면이 나와 있었다.")));
    TestTrue(TEXT("I can't call up your face"), Has(Burn, TEXT("Elia"), TEXT("나도 네 얼굴은 안 떠올라.")));
    const auto* Shell = Scene(TEXT("ms_storm_shell"));
    TestTrue(TEXT("The count was wrong"), Has(Shell, nullptr, TEXT("셈이 틀렸다.")));
    TestTrue(TEXT("End"), Has(Shell, nullptr, TEXT("끝.")));
    TestTrue(TEXT("Don't stop"), Has(Scene(TEXT("ms_storm_count")), TEXT("Arrel"), TEXT("세우지 말자. 갈 수 있는 동안은 타고 가겠어.")));
    TestTrue(TEXT("It's stopped"), Has(Scene(TEXT("ms_storm_trees")), TEXT("Arrel"), TEXT("멈췄어.")));
    const auto* Veil = Scene(TEXT("ms_storm_veil"));
    TestTrue(TEXT("39화 opens on the same sentence"), Veil && Veil->Definition.Rows[0].Text.NarrateKo == TEXT("베일은 같은 문장에서 다시 멈췄다."));
    TestTrue(TEXT("I won't leave anything out"), Has(Veil, TEXT("Veil"), TEXT("빠뜨리지 않겠습니다.")));
    const auto* Copy = Scene(TEXT("ms_storm_copy"));
    TestTrue(TEXT("It takes two"), Has(Copy, TEXT("Veil"), TEXT("원본 반출에는 둘이 필요합니다.")) && Has(Copy, TEXT("Envoy"), TEXT("원본을 가져오라는 지시입니다.")));
    const auto* Rubbing = Scene(TEXT("ms_storm_rubbing"));
    TestTrue(TEXT("Locked"), Rubbing && Rubbing->Definition.Rows.Last().Text.NarrateKo == TEXT("베일은 서랍을 닫고 잠갔다. 열쇠를 뽑아 주머니에 넣었다."));

    bool bRows = true, bCast = true, bFaces = true, bDark = true, bKairosAbsent = true, bNoDraw = true;
    int32 Rows = 0;
    for (const FString& G : SceneGroups())
        if (G.StartsWith(TEXT("ms_storm_")))
            if (const auto* A = BuildScene(GetTransientPackage(), G, 0))
            {
                bDark &= A->Definition.Rows.Num() > 0 && A->Definition.Rows[0].Presentation.bHasCg && A->Definition.Rows[0].Presentation.Cg.IsEmpty();
                for (int32 I = 0; I < A->Definition.Rows.Num(); ++I, ++Rows)
                {
                    const auto& R = A->Definition.Rows[I];
                    bRows &= R.OriginalIndex == I && (R.Text.bHasTextKo || R.Text.bHasNarrateKo);
                    const FString& Who = R.Text.Speaker;
                    bCast &= !R.Text.bHasSpeaker || Who == TEXT("Arrel") || Who == TEXT("Elia") || Who == TEXT("Tobias") || Who == TEXT("Nera") || Who == TEXT("Veil")
                        || Who == TEXT("Courier") || Who == TEXT("Envoy") || Who == TEXT("Driver");
                    bKairosAbsent &= Who != TEXT("Kairos") && Who != TEXT("Vael");
                    if (Who == TEXT("Nera")) bFaces &= R.Presentation.Portrait == TEXT("nera_neutral");
                    if (Who == TEXT("Tobias")) bFaces &= R.Presentation.Portrait == TEXT("tobias_neutral");
                    if (Who == TEXT("Veil") || Who == TEXT("Courier") || Who == TEXT("Envoy") || Who == TEXT("Driver")) bFaces &= !R.Presentation.bHasPortrait;
                    // The storm pictures are UNSAFE (drawn swords): only black.
                    if (R.Presentation.bHasCg) bDark &= R.Presentation.Cg.IsEmpty();
                    bNoDraw &= !R.Text.NarrateKo.Contains(TEXT("칼을 뽑")) && !R.Text.NarrateKo.Contains(TEXT("검을 뽑아"));
                }
            }
    TestTrue(TEXT("Rows indexed, in Korean"), bRows);
    TestTrue(TEXT("The cast: the three, Nera, Veil, the courier, the envoy, the driver"), bCast);
    TestTrue(TEXT("No Kairos speaks; Veil is never 'Vael'"), bKairosAbsent);
    TestTrue(TEXT("Faces: Nera's and Tobias's; Veil and the nameless have none"), bFaces);
    TestTrue(TEXT("Every scene on black, and only black"), bDark);
    TestTrue(TEXT("No sword drawn"), bNoDraw);
    TestEqual(TEXT("178 rows in ten scenes"), Rows, 178);
    TestEqual(TEXT("Nera's face"), MemoriaNarrativeArtwork::PortraitSource(TEXT("nera_neutral")), FString(TEXT("res://assets/portraits/nera_face_neutral.png")));
    TestNotNull(TEXT("is imported"), MemoriaNarrativeArtwork::Load(TEXT("res://assets/portraits/nera_face_neutral.png")));

    TestEqual(TEXT("The title in Korean"), MemoriaKorean::Text(TEXT("The Storm of Forgetting")), FString(TEXT("잊음의 폭풍")));
    TestEqual(TEXT("The subtitle in Korean"), MemoriaKorean::Text(TEXT("The Registrar's Hand")), FString(TEXT("등록관의 손")));
    TestEqual(TEXT("The burn in Korean"), MemoriaKorean::Text(TEXT("Burned together: after he opened his eyes, before the fourth mile")), FString(TEXT("함께 연소: 넷째 마일 전, 눈 뜬 뒤")));

    TStrongObjectPtr<UGameInstance> Game(NewObject<UGameInstance>()); Game->Init();
    auto* Run = Game->GetSubsystem<UMemoriaRunSubsystem>();
    if (!TestTrue(TEXT("A run at Chapter 19"), Run->BeginStartingMemoryRun(19) == EMemoriaMemoryResult::Success)) { Game->Shutdown(); return false; }
    Run->SetLocale(TEXT("ko"));
    const TCHAR* Steps[][2] = {{TEXT("ch19_weather"), TEXT("목표  수레로 동쪽으로")}, {TEXT("ch19_burn"), TEXT("목표  동쪽으로: 굽이를 지나")},
        {TEXT("ch19_count"), TEXT("목표  그늘을 지나 길 옆 나무들로")}, {TEXT("ch19_trees"), TEXT("목표  나무 사이 빈 땅으로")}};
    for (const auto& S : Steps) { Run->SetStoryFlag(S[0], true); TestEqual(*FString(S[0]), MemoriaNavigationGoal::Line(Run->GetRunSnapshot(), StormRoadMap, false), FString(S[1])); }
    for (const auto& S : Steps) Run->SetStoryFlag(S[0], false);
    for (const auto& M : MainSteps()) if (!FString(M.Flag).StartsWith(TEXT("ch19_")) && FString(M.Flag) != TEXT("ms_never")) Run->SetStoryFlag(M.Flag, true);
    TestTrue(TEXT("The main quest: on east"), MainRecord(Run->GetRunSnapshot(), true).Body.Contains(TEXT("→ 같은 수레로 더 동쪽으로. 책은 여전히 북서쪽을 가리킨다")));
    Run->SetStoryFlag(TEXT("ch19_burn"), true);
    TestTrue(TEXT("then where the shell points"), MainRecord(Run->GetRunSnapshot(), true).Body.Contains(TEXT("→ 조개가 가리키는 동쪽으로")));
    Run->SetStoryFlag(TEXT("ch19_trees"), true);
    TestTrue(TEXT("then the trees"), MainRecord(Run->GetRunSnapshot(), true).Body.Contains(TEXT("→ 길 옆 가느다란 나무들 사이")));
    Run->SetStoryFlag(TEXT("ch19_complete"), true);
    TestTrue(TEXT("then the night"), MainRecord(Run->GetRunSnapshot(), true).Body.Contains(TEXT("→ 나무 사이의 밤. 빵 반쪽, 얼마 남지 않은 물")));
    Game->Shutdown();
    return true;
}

namespace
{
// S401 in the renderer: Lumea; aboard; the storm and the burn at one stop; the shell and the count at the next; in
// among the trees, down; to the empty ground; Veil's three scenes; the card.
class FStormRoadReplay final : public IAutomationLatentCommand
{
public:
    explicit FStormRoadReplay(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
    ~FStormRoadReplay() override { if (bFixed) { FApp::SetUseFixedTimeStep(bOldFixed); FApp::SetFixedDeltaTime(OldDelta); } }
    bool Update() override
    {
        if (LastFrame == GFrameCounter) return false;
        LastFrame = GFrameCounter;
        if (FPlatformTime::Seconds() - Started > 1200) { Test->AddError(FString::Printf(TEXT("S401 timeout at step %d"), Step)); return true; }
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
        auto Capture = [&](const FString& Name) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Validation/S401Storm") / (Name + TEXT(".png")), true, false); };
        auto Flag = [&](const FString& Id) { return Run->GetRunSnapshot().GetFlag(Id); };
        auto Place = [&](const FVector2D& Source) { Pawn->SetActorLocation(MemoriaChapterMaps::ToWorld(Source) + FVector(0, 0, Pawn->GetActorLocation().Z)); };
        AMemoriaCart* Cart = nullptr; int32 Carts = 0; for (TActorIterator<AMemoriaCart> It(World); It; ++It) { Cart = *It; ++Carts; }
        const bool bExploring = Host->GetState() == EMemoriaSliceState::Exploration;
        if (Host->GetState() == EMemoriaSliceState::Field)
        {
            const FMemoriaNarrativeView View = Host->GetView();
            for (const auto& Shot : Shots)
                if (!Taken.Contains(Shot.Key) && View.CueKey == Shot.Key) { Taken.Add(Shot.Key); ShotAt = Frame; ShotName = Shot.Value; }
            if (View.CueKey.StartsWith(TEXT("field:ms_storm_"))) Seen.Add(View.CueKey);
            if (!ShotName.IsEmpty() && Frame == ShotAt + 20) { Capture(ShotName); ShotName.Reset(); return false; }
            if (ShotName.IsEmpty() && Frame % 6 == 0) Host->Confirm();
            return false;
        }
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
        switch (Step)
        {
        case 0:
            Test->TestEqual(TEXT("The map is the storm road"), Map->GetMap(), FString(MemoriaManuscript::StormRoadMap));
            Test->TestEqual(TEXT("A development run in Chapter 19"), Run->GetRunSnapshot().CurrentChapter, int64(19));
            { int32 Count = 0; for (TActorIterator<AMemoriaTobiasCompanion> It(World); It; ++It) ++Count; Test->TestEqual(TEXT("Tobias rides with them"), Count, 1); }
            Test->TestTrue(TEXT("The cart is there from the start"), Carts == 1);
            ++Step; Mark = Frame; break;
        case 1:
            if (Frame == Mark + 40) Capture(TEXT("S401Title"));
            if (Flag(TEXT("ch19_weather")) && bExploring && Frame > Mark + 60 && !bShot)
            { Test->TestTrue(TEXT("The absence is registered"), Flag(TEXT("ch19_absence_registered"))); Test->TestTrue(TEXT("Aboard after Lumea"), Cart && Cart->IsRiding()); }
            if (Frame > Mark + 20000 && !Flag(TEXT("ch19_weather"))) { Test->AddError(TEXT("Lumea's scenes did not finish")); return true; }
            Ride(TEXT("ch19_weather"), TEXT("S401Riding")); break;
        case 2: Ride(TEXT("ch19_wind"), TEXT("S401StormStop")); break;
        case 3:
            if (Flag(TEXT("ch19_burn")) && bExploring)
                Test->TestTrue(TEXT("Burned together"), Flag(TEXT("ch19_burned_together")) && Flag(TEXT("ch19_turns_lost")) && Host->GetTrace().Contains(TEXT("toast:Burned together: after he opened his eyes, before the fourth mile")));
            Ride(TEXT("ch19_burn"), TEXT("S401AfterTheStorm")); break;
        case 4:
            if (Flag(TEXT("ch19_shell")) && bExploring) Test->TestTrue(TEXT("The shell points east"), Flag(TEXT("ch19_shell_east")));
            Ride(TEXT("ch19_shell"), TEXT("S401Shell")); break;
        case 5: Ride(TEXT("ch19_count"), TEXT("S401Bend")); break;
        case 6:
            if (Flag(TEXT("ch19_trees")) && bExploring) Test->TestTrue(TEXT("They got down among the trees"), Cart && !Cart->IsRiding());
            if (Flag(TEXT("ch19_trees")) && bExploring && Frame > Mark + 20)
            {
                if (!bShot) { bShot = true; Mark = Frame; break; }
                if (Frame == Mark + 30) { Capture(TEXT("S401Trees")); break; }
                if (Frame < Mark + 40) break;
                bShot = false; Place(Spec->Exit.Rect.Center()); ++Step; Mark = Frame; break;
            }
            if (Frame > Mark + 30000) { Test->AddError(TEXT("The cart never reached the trees")); return true; }
            break;
        case 7:
            if (Map->IsChapterComplete())
            {
                Test->TestTrue(TEXT("Veil's night closes Chapter 19"), Flag(TEXT("ch19_complete")) && Flag(TEXT("canon_ch20_white_country_ready")) && Flag(TEXT("ch19_rubbing_hidden")));
                Test->TestTrue(TEXT("Every row of the ten scenes was shown"), Seen.Num() == 178);
                Test->TestTrue(TEXT("Its last line was shown"), Seen.Contains(TEXT("field:ms_storm_rubbing:8")));
                Test->TestTrue(TEXT("The card shows"), Map->GetCard() && Map->GetCard()->IsShowing());
                ++Step; Mark = Frame;
            }
            if (Frame > Mark + 20000) { Test->AddError(TEXT("The exit chain never closed the chapter")); return true; }
            break;
        case 8:
            if (Frame == Mark + 30) Capture(TEXT("S401Complete"));
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
        {TEXT("field:ms_storm_nera:8"), TEXT("S401Identification")}, {TEXT("field:ms_storm_weather:4"), TEXT("S401NeraSpeaks")},
        {TEXT("field:ms_storm_weather:8"), TEXT("S401WeatherNote")}, {TEXT("field:ms_storm_wind:5"), TEXT("S401ThirdDistrict")},
        {TEXT("field:ms_storm_wind:12"), TEXT("S401NotOpened")}, {TEXT("field:ms_storm_burn:7"), TEXT("S401BurnThatTime")},
        {TEXT("field:ms_storm_burn:10"), TEXT("S401HandsLocked")}, {TEXT("field:ms_storm_burn:24"), TEXT("S401YourFace")},
        {TEXT("field:ms_storm_shell:17"), TEXT("S401CountWrong")}, {TEXT("field:ms_storm_shell:20"), TEXT("S401End")},
        {TEXT("field:ms_storm_count:17"), TEXT("S401DontStop")}, {TEXT("field:ms_storm_trees:7"), TEXT("S401ShellStopped")},
        {TEXT("field:ms_storm_veil:1"), TEXT("S401NotDrawn")}, {TEXT("field:ms_storm_veil:18"), TEXT("S401WontLeaveOut")},
        {TEXT("field:ms_storm_copy:2"), TEXT("S401TakesTwo")}, {TEXT("field:ms_storm_rubbing:8"), TEXT("S401Locked")}};
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStormRoadVisualTest, "MemoriaVisual.S401StormRoad", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStormRoadVisualTest::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Memoria/Maps/L_StormRoad"))) return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FStormRoadReplay(this)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
