#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "Tests/AutomationCommon.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
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
#include "Presentation/MemoriaNarrativeArtwork.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Run/MemoriaRunTypes.h"
#include "Chapter/MemoriaChapterMap.h"
#include "Chapter/MemoriaChapterPresentation.h"
#include "Chapter/MemoriaChapterCardWidget.h"
#if WITH_DEV_AUTOMATION_TESTS

// S385: Chapter 14, the farmhouse under the ridge, from 최종본 22화 「공명」 and 23화 「세 번째 사람」 (Gemini's T-7 brief).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRidgeFarmhouseS385Test, "Memoria.S385.RidgeFarmhouse", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRidgeFarmhouseS385Test::RunTest(const FString&)
{
    using namespace MemoriaManuscript;
    const auto* Ridge = MemoriaChapterMaps::Find(RidgeMap);
    if (!TestNotNull(TEXT("The farmhouse under the ridge is a chapter map"), Ridge)) return false;
    TestEqual(TEXT("Chapter 14"), Ridge->Chapter, 14);
    TestEqual(TEXT("Its level"), MemoriaChapterMaps::LevelPath(RidgeMap), FString(TEXT("/Game/Memoria/Maps/L_RidgeFarmhouse")));
    TestEqual(TEXT("In Korean"), MemoriaChapterMaps::Korean(RidgeMap), FString(TEXT("능선 아래 농가")));
    const int32 Wall = Ridge->TileNames.IndexOfByKey(TEXT("WALL")), Floor = Ridge->TileNames.IndexOfByKey(TEXT("SHELTER"));
    TestTrue(TEXT("The Godot drift_shelter ground, its rim open field"), Ridge->Width == 25 && Ridge->Height == 18 && Wall != INDEX_NONE && Floor != INDEX_NONE
        && Ridge->TileNames.Contains(TEXT("EDGE")) && !Ridge->TileNames.Contains(TEXT("CONCRETE")));
    // 22화: "높은 벽 셋이 서로 이어졌고, 나머지 한쪽은 모서리만 남아 있었다."
    bool bBack = true, bSides = true; for (int32 X = 6; X <= 14; ++X) bBack &= Ridge->TileAt(X, 5) == Wall;
    for (int32 Y = 6; Y <= 9; ++Y) bSides &= Ridge->TileAt(5, Y) == Wall && Ridge->TileAt(15, Y) == Wall;
    TestTrue(TEXT("Three high walls"), bBack && bSides);
    TestTrue(TEXT("and of the south side only the corners"), Ridge->TileAt(6, 10) == Wall && Ridge->TileAt(13, 10) == Wall && !Ridge->IsSolid(Ridge->TileAt(9, 10)) && !Ridge->IsSolid(Ridge->TileAt(10, 10)));
    auto Tile = [&](const FVector2D& P) { return Ridge->TileAt(int32(P.X) / Ridge->TileSize, int32(P.Y) / Ridge->TileSize); };
    auto Open = [&](const FVector2D& P) { return !Ridge->IsSolid(Tile(P)); };
    TestTrue(TEXT("In from the fields to the south"), Open(Ridge->Spawn) && Ridge->Spawn.Y > (Ridge->Height - 3) * Ridge->TileSize);
    TestTrue(TEXT("It ends inside the walls, at first light"), Tile(Ridge->Exit.Rect.Center()) == Floor && Ridge->Exit.Group == TEXT("ms_ridge_third"));
    for (const auto& T : Ridge->Triggers) TestTrue(*(TEXT("Open ground: ") + T.Group), Open(T.Rect.Center()));
    const auto& Corner = Ridge->Triggers[5];
    TestTrue(TEXT("Arrel keeps watch inside the broken corner"), Corner.Group == TEXT("ms_ridge_slope") && Tile(Corner.Rect.Center()) == Floor
        && Ridge->TileAt(int32(Corner.Rect.Origin.X) / Ridge->TileSize, int32(Corner.Rect.Origin.Y) / Ridge->TileSize + 1) == Wall);
    TestTrue(TEXT("No fire, no lamp"), Ridge->Decorations.IsEmpty());
    TestTrue(TEXT("Nobody stands about"), Ridge->AmbientNpcs.IsEmpty());
    TArray<FString> Groups; for (const auto& S : Ridge->Sequence) Groups.Add(S.Group);
    for (const auto& T : Ridge->Triggers) Groups.Add(T.Group);
    Groups.Add(Ridge->Exit.Group);
    TestEqual(TEXT("Nine scenes"), Groups.Num(), 9);
    for (const FString& G : Groups) TestTrue(*(TEXT("A manuscript scene: ") + G), IsScene(G));
    const auto* Verdan = MemoriaChapterMaps::Find(VerdanReturnMap);
    TestTrue(TEXT("Verdan's road leads to the farmhouse"), Verdan && Verdan->Exit.NextMap == RidgeMap && Verdan->Exit.NextChapter == 14);
    TestTrue(TEXT("and the farmhouse's on to Chapter 15"), Ridge->Exit.NextChapter == 15 && Ridge->Exit.Flags.Contains(TEXT("canon_ch15_two_notes_ready")));

    auto Has = [](const UMemoriaFieldAsset* A, const TCHAR* Speaker, const TCHAR* Ko)
    { return A && A->Definition.Rows.ContainsByPredicate([&](const FMemoriaFieldRow& R) { return (Speaker ? R.Text.Speaker == Speaker : !R.Text.bHasSpeaker) && (R.Text.TextKo == Ko || R.Text.NarrateKo == Ko); }); };
    auto Scene = [](const TCHAR* G) { return BuildScene(GetTransientPackage(), G, 0); };
    TestTrue(TEXT("North-west"), Has(Scene(TEXT("ms_ridge_morning")), TEXT("Elia"), TEXT("북서쪽.")));
    const auto* House = Scene(TEXT("ms_ridge_house"));
    TestTrue(TEXT("Three walls, a corner"), Has(House, nullptr, TEXT("능선 아래에서 농가를 발견했다. 남아 있는 벽 위로 지붕이 없었다. 높은 벽 셋이 서로 이어졌고, 나머지 한쪽은 모서리만 남아 있었다. 뒤로는 비탈이 솟아 있었으며, 열린 쪽 앞을 좁은 길이 지났다.")));
    TestTrue(TEXT("Your hand then too"), Has(House, TEXT("Arrel"), TEXT("그때도 네 손이었어.")));
    TestTrue(TEXT("Not now"), Has(House, TEXT("Elia"), TEXT("안 열 거야.")) && Has(House, TEXT("Elia"), TEXT("지금은.")));
    const auto* Night = Scene(TEXT("ms_ridge_night"));
    TestTrue(TEXT("No fire"), Night && Night->Definition.Rows[0].Text.NarrateKo.StartsWith(TEXT("불은 피우지 않았다.")));
    TestTrue(TEXT("Is it coming"), Has(Night, TEXT("Arrel"), TEXT("그쪽이 오는 걸까.")));
    TestTrue(TEXT("No light leaks out"), Has(Night, nullptr, TEXT("아렐은 코트 앞섶을 보았다. 밖으로 새어 나오는 빛도, 종이가 움직이는 소리도 없었다. 엘리아의 말이 끝난 뒤에도 천은 움직이지 않았다.")));
    TestTrue(TEXT("The sword by his left hand"), Night && Night->Definition.Rows[Night->Definition.Rows.Num() - 2].Text.NarrateKo.EndsWith(TEXT("검을 왼손 가까이 당겨 놓았다.")));
    const auto* Slope = Scene(TEXT("ms_ridge_slope"));
    TestTrue(TEXT("23화 opens with Tobias"), Slope && Slope->Definition.Rows[0].Text.NarrateKo == TEXT("토비아스는 비탈 중간에서 멈췄다."));
    TestTrue(TEXT("Stop there"), Has(Slope, TEXT("Arrel"), TEXT("거기서.")) && Has(Slope, TEXT("Tobias"), TEXT("서 있습니다.")));
    TestTrue(TEXT("He draws left-handed"), Has(Slope, nullptr, TEXT("서른 걸음쯤 남았을 때 남자가 일어섰다. 검을 든 채였다. 칼집을 옆구리에 끼우고 왼손으로 자루를 당겼다. 날이 다 나오기 전에 토비아스는 멈춰 섰다. 남자는 칼을 끝까지 뽑았다.")));
    TestTrue(TEXT("Her cover is not lifted"), Has(Slope, nullptr, TEXT("남자는 앞을 향한 채 턱을 조금 움직였다. 여자가 가방에서 책을 꺼냈다. 가죽 장정의 모서리를 받쳐 무릎 위에 놓았다. 표지는 들리지 않았다.")));
    const auto* Years = Scene(TEXT("ms_ridge_ten_years"));
    TestTrue(TEXT("Ami"), Has(Years, nullptr, TEXT("셀렌에게 딸 아미가 있었는데 셋에 하얘졌다.")));
    TestTrue(TEXT("It eats deletions"), Has(Years, TEXT("Tobias"), TEXT("삭제를 먹습니다.")));
    TestTrue(TEXT("Never given back"), Has(Years, TEXT("Tobias"), TEXT("돌려준 적도 없습니다.")));
    const auto* Third = Scene(TEXT("ms_ridge_third"));
    TestTrue(TEXT("His name"), Has(Third, TEXT("Tobias"), TEXT("토비아스입니다. 동부국 삼급 등록 서기.")));
    TestTrue(TEXT("Come with you"), Has(Third, TEXT("Tobias"), TEXT("같이 가고 싶습니다.")));
    TestTrue(TEXT("The knife stays in the bag"), Third && Third->Definition.Rows.Last().Text.NarrateKo.EndsWith(TEXT("그는 칼부터 꺼내려다가 아렐을 보고, 천만 집어 들었다.")));

    const FString Ink(TEXT("res://assets/cg/generated/illustration_expansion_v3/world_rewrite_tobias_ink_v3.png"));
    bool bRows = true, bCast = true, bFaces = true, bTobiasLate = true, bNamedLate = true, bArt = true, bInkAlone = true, bShut = true;
    for (const FString& G : SceneGroups())
        if (G.StartsWith(TEXT("ms_ridge_")))
            if (const auto* A = BuildScene(GetTransientPackage(), G, 0))
            {
                const bool b23 = G == TEXT("ms_ridge_slope") || G == TEXT("ms_ridge_ten_years") || G == TEXT("ms_ridge_third");
                FString Shown;
                for (int32 I = 0; I < A->Definition.Rows.Num(); ++I)
                {
                    const auto& R = A->Definition.Rows[I];
                    bRows &= R.OriginalIndex == I && (R.Text.bHasTextKo || R.Text.bHasNarrateKo);
                    // 22-23화: Arrel, Elia, and from 23화 Tobias. No Sable, no Malet, no one else speaks.
                    bCast &= !R.Text.bHasSpeaker || R.Text.Speaker == TEXT("Arrel") || R.Text.Speaker == TEXT("Elia") || R.Text.Speaker == TEXT("Tobias");
                    if (R.Text.Speaker == TEXT("Tobias")) { bFaces &= R.Presentation.Portrait == TEXT("tobias_neutral"); bTobiasLate &= b23; }
                    const FString Ko = R.Text.bHasTextKo ? R.Text.TextKo : R.Text.NarrateKo;
                    bNamedLate &= b23 || !Ko.Contains(TEXT("토비아스"));
                    // The books stay shut: no line opens one.
                    bShut &= !Ko.Contains(TEXT("표지를 열었")) && !Ko.Contains(TEXT("책을 펼쳤"));
                    if (R.Presentation.bHasCg) { bArt &= R.Presentation.Cg.IsEmpty() || (R.Presentation.Cg == Ink && b23); Shown = R.Presentation.Cg; }
                    // His desk is the past only: nobody speaks over it.
                    bInkAlone &= !(Shown == Ink && R.Text.bHasSpeaker);
                }
                bArt &= Shown != Ink;
            }
    TestTrue(TEXT("Rows indexed, in Korean"), bRows);
    TestTrue(TEXT("Only Arrel, Elia and Tobias speak"), bCast);
    TestTrue(TEXT("Tobias's own face"), bFaces);
    TestTrue(TEXT("Tobias speaks only from 23화"), bTobiasLate);
    TestTrue(TEXT("and is not even named in 22화"), bNamedLate);
    TestTrue(TEXT("No picture with a lamp: dark, and his desk in 23화's past"), bArt && Ridge->Splash.IsEmpty());
    TestTrue(TEXT("Nobody speaks over his desk"), bInkAlone);
    TestTrue(TEXT("The books stay shut"), bShut);
    TestEqual(TEXT("Nine ridge scenes"), SceneGroups().FilterByPredicate([](const FString& G) { return G.StartsWith(TEXT("ms_ridge_")); }).Num(), 9);
    TestEqual(TEXT("Tobias's portrait"), MemoriaNarrativeArtwork::PortraitSource(TEXT("tobias_neutral")), FString(TEXT("res://assets/portraits/tobias_face_neutral.png")));
    TestNotNull(TEXT("is imported"), MemoriaNarrativeArtwork::Load(TEXT("res://assets/portraits/tobias_face_neutral.png")));
    TestNotNull(TEXT("and so is his desk"), MemoriaNarrativeArtwork::Load(Ink));
    // S386: Elia as 23화 has her, "은색 머리카락": every expression key shows the silver-haired face.
    for (const TCHAR* Key : {TEXT("elia_neutral"), TEXT("elia_concern"), TEXT("elia_sad"), TEXT("elia_determined"), TEXT("elia_calm"), TEXT("elia_worried"), TEXT("elia_gentle_smile"), TEXT("elia_hopeful")})
        TestEqual(*FString::Printf(TEXT("Elia's face: %s"), Key), MemoriaNarrativeArtwork::PortraitSource(Key), FString(TEXT("res://assets/portraits/elia_face_silver_v1.png")));
    TestNotNull(TEXT("and it is imported"), MemoriaNarrativeArtwork::Load(TEXT("res://assets/portraits/elia_face_silver_v1.png")));
    TestEqual(TEXT("The toast in Korean"), MemoriaKorean::Text(TEXT("Joined: Tobias")), FString(TEXT("합류: 토비아스")));
    TestEqual(TEXT("The title in Korean"), MemoriaKorean::Text(TEXT("Resonance")), FString(TEXT("공명")));
    TestEqual(TEXT("The subtitle in Korean"), MemoriaKorean::Text(TEXT("The Third Person")), FString(TEXT("세 번째 사람")));

    TStrongObjectPtr<UGameInstance> Game(NewObject<UGameInstance>()); Game->Init();
    auto* Run = Game->GetSubsystem<UMemoriaRunSubsystem>();
    if (!TestTrue(TEXT("A run at Chapter 14"), Run->BeginStartingMemoryRun(14) == EMemoriaMemoryResult::Success)) { Game->Shutdown(); return false; }
    Run->SetLocale(TEXT("ko"));
    const TCHAR* Steps[][2] = {{TEXT("ch14_morning"), TEXT("목표  밭 가장자리 길로")}, {TEXT("ch14_fields"), TEXT("목표  서쪽: 무너진 돌담 너머")},
        {TEXT("ch14_tower"), TEXT("목표  북쪽 그루터기 사이로")}, {TEXT("ch14_stumps"), TEXT("목표  능선 아래 농가: 열린 쪽 앞의 길")},
        {TEXT("ch14_house"), TEXT("목표  높은 벽 아래: 불 없는 밤")}, {TEXT("ch14_night"), TEXT("목표  무너진 모서리: 내려오는 길을 지킨다")},
        {TEXT("ch14_slope"), TEXT("목표  길 가장자리: 그가 앉은 돌")}, {TEXT("ch14_ten_years"), TEXT("목표  벽 안쪽으로: 동이 튼다")}};
    for (const auto& S : Steps) { Run->SetStoryFlag(S[0], true); TestEqual(*FString(S[0]), MemoriaNavigationGoal::Line(Run->GetRunSnapshot(), RidgeMap, false), FString(S[1])); }
    for (const auto& S : Steps) Run->SetStoryFlag(S[0], false);
    for (const auto& M : MainSteps()) if (!FString(M.Flag).StartsWith(TEXT("ch14_")) && FString(M.Flag) != TEXT("ms_never")) Run->SetStoryFlag(M.Flag, true);
    TestTrue(TEXT("The main quest is round the tower"), MainRecord(Run->GetRunSnapshot(), true).Body.Contains(TEXT("→ 북서쪽: 신호탑을 돌아 그루터기 사이로")));
    for (const TCHAR* F : {TEXT("ch14_stumps"), TEXT("ch14_night")}) Run->SetStoryFlag(F, true);
    TestTrue(TEXT("then someone from the ridge"), MainRecord(Run->GetRunSnapshot(), true).Body.Contains(TEXT("→ 능선에서 내려온 사람")));
    Run->SetStoryFlag(TEXT("ch14_complete"), true);
    TestTrue(TEXT("then the three of them"), MainRecord(Run->GetRunSnapshot(), true).Body.Contains(TEXT("→ 셋이서 북서쪽으로")));
    Game->Shutdown();
    return true;
}

namespace
{
// S385 in the renderer: in from the fields, round the signal tower through the stumps, into the roofless farmhouse,
// the night with no fire; Tobias down the slope at first light, the ten years, and the third person sitting down.
class FRidgeFarmhouseReplay final : public IAutomationLatentCommand
{
public:
    explicit FRidgeFarmhouseReplay(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
    ~FRidgeFarmhouseReplay() override { if (bFixed) { FApp::SetUseFixedTimeStep(bOldFixed); FApp::SetFixedDeltaTime(OldDelta); } }
    bool Update() override
    {
        if (LastFrame == GFrameCounter) return false;
        LastFrame = GFrameCounter;
        if (FPlatformTime::Seconds() - Started > 720) { Test->AddError(FString::Printf(TEXT("S385 timeout at step %d"), Step)); return true; }
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
        auto Capture = [&](const FString& Name) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Validation/S385Ridge") / (Name + TEXT(".png")), true, false); };
        auto Flag = [&](const FString& Id) { return Run->GetRunSnapshot().GetFlag(Id); };
        auto Place = [&](const FVector2D& Source) { Pawn->SetActorLocation(MemoriaChapterMaps::ToWorld(Source) + FVector(0, 0, Pawn->GetActorLocation().Z)); };
        if (Host->GetState() == EMemoriaSliceState::Field)
        {
            const FMemoriaNarrativeView View = Host->GetView();
            for (const auto& Shot : Shots)
                if (!Taken.Contains(Shot.Key) && View.CueKey == Shot.Key) { Taken.Add(Shot.Key); ShotAt = Frame; ShotName = Shot.Value; }
            if (View.CueKey.StartsWith(TEXT("field:ms_ridge_"))) Seen.Add(View.CueKey);
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
            if (Frame > Mark + 9000) { Test->AddError(FString::Printf(TEXT("Stuck before %s"), Done)); Step = 99; }
            return false;
        };
        switch (Step)
        {
        case 0:
            Test->TestEqual(TEXT("The map is the farmhouse under the ridge"), Map->GetMap(), FString(MemoriaManuscript::RidgeMap));
            Test->TestEqual(TEXT("A development run in Chapter 14"), Run->GetRunSnapshot().CurrentChapter, int64(14));
            ++Step; Mark = Frame; break;
        case 1:
            if (Frame == Mark + 40) Capture(TEXT("S385Title"));
            if (Flag(TEXT("ch14_morning")) && Host->GetState() == EMemoriaSliceState::Exploration && Frame > Mark + 60)
            { Capture(TEXT("S385Fields")); Place(Spec->Triggers[0].Rect.Center()); ++Step; Mark = Frame; }
            if (Frame > Mark + 9000) { Test->AddError(TEXT("The arrival did not finish")); return true; }
            break;
        case 2: Next(TEXT("ch14_fields"), 1); break;
        case 3: Next(TEXT("ch14_tower"), 2); break;
        case 4: Next(TEXT("ch14_stumps"), 3, TEXT("S385Stumps")); break;
        case 5: Next(TEXT("ch14_house"), 4, TEXT("S385Inside")); break;
        case 6:
            if (Flag(TEXT("ch14_night"))) Test->TestTrue(TEXT("The book woke with nobody touching it"), Flag(TEXT("ch14_book_awake")));
            Next(TEXT("ch14_night"), 5, TEXT("S385NoFire")); break;
        case 7: Next(TEXT("ch14_slope"), 6, TEXT("S385Corner")); break;
        case 8: Next(TEXT("ch14_ten_years"), -1); break;
        case 9:
            if (Map->IsChapterComplete())
            {
                Test->TestTrue(TEXT("First light closes Chapter 14"), Flag(TEXT("ch14_complete")) && Flag(TEXT("canon_ch15_two_notes_ready")));
                Test->TestTrue(TEXT("Joined: Tobias"), Flag(TEXT("party_tobias")) && Host->GetTrace().Contains(TEXT("toast:Joined: Tobias")));
                Test->TestTrue(TEXT("Its last line was shown"), Seen.Contains(TEXT("field:ms_ridge_third:38")));
                Test->TestTrue(TEXT("Every row of the nine scenes was shown"), Seen.Num() == 14 + 12 + 12 + 12 + 33 + 37 + 44 + 53 + 39);
                Test->TestTrue(TEXT("The card shows"), Map->GetCard() && Map->GetCard()->IsShowing());
                ++Step; Mark = Frame;
            }
            if (Frame > Mark + 9000) { Test->AddError(TEXT("The exit never closed the chapter")); return true; }
            break;
        case 10:
            if (Frame == Mark + 30) Capture(TEXT("S385Complete"));
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
        {TEXT("field:ms_ridge_morning:4"), TEXT("S385NorthWest")}, {TEXT("field:ms_ridge_tower:0"), TEXT("S385Tower")},
        {TEXT("field:ms_ridge_house:1"), TEXT("S385Farmhouse")}, {TEXT("field:ms_ridge_house:29"), TEXT("S385NotOpening")},
        {TEXT("field:ms_ridge_night:0"), TEXT("S385NightNoFire")}, {TEXT("field:ms_ridge_night:33"), TEXT("S385LightNorthWest")},
        {TEXT("field:ms_ridge_slope:0"), TEXT("S385Halfway")}, {TEXT("field:ms_ridge_slope:4"), TEXT("S385Arcane")},
        {TEXT("field:ms_ridge_slope:10"), TEXT("S385StopThere")}, {TEXT("field:ms_ridge_slope:11"), TEXT("S385Standing")},
        {TEXT("field:ms_ridge_ten_years:4"), TEXT("S385Ami")}, {TEXT("field:ms_ridge_ten_years:11"), TEXT("S385Deletions")},
        {TEXT("field:ms_ridge_third:2"), TEXT("S385Name")}, {TEXT("field:ms_ridge_third:23"), TEXT("S385ComeWithYou")},
        {TEXT("field:ms_ridge_third:38"), TEXT("S385OnlyTheCloth")}};
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRidgeFarmhouseVisualTest, "MemoriaVisual.S385RidgeFarmhouse", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRidgeFarmhouseVisualTest::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Memoria/Maps/L_RidgeFarmhouse"))) return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FRidgeFarmhouseReplay(this)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
