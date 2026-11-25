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
#include "Run/MemoriaRunSubsystem.h"
#include "Run/MemoriaRunTypes.h"
#include "Chapter/MemoriaChapterMap.h"
#include "Chapter/MemoriaChapterPresentation.h"
#include "Chapter/MemoriaChapterCardWidget.h"
#if WITH_DEV_AUTOMATION_TESTS

// S389: Chapter 15, the dry wells, from 최종본 24화 「두 노트」 (Gemini's T-11 brief).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDryWellsS389Test, "Memoria.S389.DryWells", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDryWellsS389Test::RunTest(const FString&)
{
    using namespace MemoriaManuscript;
    const auto* Wells = MemoriaChapterMaps::Find(DryWellsMap);
    if (!TestNotNull(TEXT("The dry wells are a chapter map"), Wells)) return false;
    TestEqual(TEXT("Chapter 15"), Wells->Chapter, 15);
    TestEqual(TEXT("Its level"), MemoriaChapterMaps::LevelPath(DryWellsMap), FString(TEXT("/Game/Memoria/Maps/L_DryWells")));
    TestEqual(TEXT("In Korean"), MemoriaChapterMaps::Korean(DryWellsMap), FString(TEXT("마른 우물")));
    const int32 Well = Wells->TileNames.IndexOfByKey(TEXT("WELL")), Tree = Wells->TileNames.IndexOfByKey(TEXT("TREE"));
    TestTrue(TEXT("The Godot seam_outskirts ground, as an open pasture with poplars"), Wells->Width == 25 && Wells->Height == 18 && Well != INDEX_NONE && Tree != INDEX_NONE
        && Wells->TileNames.Contains(TEXT("EDGE")) && !Wells->TileNames.Contains(TEXT("VOID_ROCK")));
    // 24화: "우물은 움푹한 땅 안에 둘 있었다... 서른 걸음쯤 떨어져".
    TestTrue(TEXT("Two wells, either side of the path"), Wells->TileAt(6, 6) == Well && Wells->TileAt(12, 6) == Well && Wells->IsSolid(Well) && Wells->TileAt(9, 6) != Well);
    auto Tile = [&](const FVector2D& P) { return Wells->TileAt(int32(P.X) / Wells->TileSize, int32(P.Y) / Wells->TileSize); };
    auto Open = [&](const FVector2D& P) { return !Wells->IsSolid(Tile(P)); };
    TestTrue(TEXT("In from the farmhouse to the south"), Open(Wells->Spawn) && Wells->Spawn.Y > (Wells->Height - 3) * Wells->TileSize);
    TestTrue(TEXT("Out north, to the fork"), Open(Wells->Exit.Rect.Center()) && Wells->Exit.Rect.Center().Y < 2 * Wells->TileSize && Wells->Exit.Group == TEXT("ms_wells_arcane"));
    for (const auto& T : Wells->Triggers) TestTrue(*(TEXT("Open ground: ") + T.Group), Open(T.Rect.Center()));
    const auto& Between = Wells->Triggers[1];
    TestTrue(TEXT("The books open between the two wells"), Between.Group == TEXT("ms_wells_dry") && Between.Rect.Origin.X > 7 * Wells->TileSize && Between.Rect.Origin.X + Between.Rect.Size.X < 12 * Wells->TileSize);
    TestTrue(TEXT("No fire, no lamp"), Wells->Decorations.IsEmpty());
    TestTrue(TEXT("Nobody else in the pasture"), Wells->AmbientNpcs.IsEmpty());
    TestTrue(TEXT("No picture: the scenes play on black"), Wells->Splash.IsEmpty());
    TArray<FString> Groups; for (const auto& S : Wells->Sequence) Groups.Add(S.Group);
    for (const auto& T : Wells->Triggers) Groups.Add(T.Group);
    Groups.Add(Wells->Exit.Group);
    TestEqual(TEXT("Seven scenes"), Groups.Num(), 7);
    for (const FString& G : Groups) TestTrue(*(TEXT("A manuscript scene: ") + G), IsScene(G));
    const auto* Ridge = MemoriaChapterMaps::Find(RidgeMap);
    TestTrue(TEXT("The farmhouse's road leads to the wells"), Ridge && Ridge->Exit.NextMap == DryWellsMap && Ridge->Exit.NextChapter == 15);
    TestTrue(TEXT("and the wells' on to Chapter 16"), Wells->Exit.NextChapter == 16 && Wells->Exit.NextMap == ArcaneMap && Wells->Exit.Flags.Contains(TEXT("canon_ch16_arcane_ready")));

    auto Has = [](const UMemoriaFieldAsset* A, const TCHAR* Speaker, const TCHAR* Ko)
    { return A && A->Definition.Rows.ContainsByPredicate([&](const FMemoriaFieldRow& R) { return (Speaker ? R.Text.Speaker == Speaker : !R.Text.bHasSpeaker) && (R.Text.TextKo == Ko || R.Text.NarrateKo == Ko); }); };
    auto Scene = [](const TCHAR* G) { return BuildScene(GetTransientPackage(), G, 0); };
    const auto* Bread = Scene(TEXT("ms_wells_bread"));
    TestTrue(TEXT("24화 opens on Tobias and the bread"), Bread && Bread->Definition.Rows[0].Text.NarrateKo.StartsWith(TEXT("토비아스는 빵을 손으로 찢어 보았다.")));
    TestTrue(TEXT("Water first"), Has(Bread, TEXT("Arrel"), TEXT("물부터 찾아야겠어.")));
    const auto* Dry = Scene(TEXT("ms_wells_dry"));
    TestTrue(TEXT("No sound of water"), Has(Dry, nullptr, TEXT("물소리는 없었다.")));
    TestTrue(TEXT("The cover lifted"), Has(Dry, nullptr, TEXT("표지가 들렸다.")));
    TestTrue(TEXT("By itself, for the first time"), Has(Dry, TEXT("Tobias"), TEXT("혼자 펴진 건 처음입니다.")));
    const auto* Six = Scene(TEXT("ms_wells_six"));
    TestTrue(TEXT("No spreading ink, no pen"), Six && Six->Definition.Rows[1].Text.NarrateKo.Contains(TEXT("잉크가 번지는 테두리는 없었다. 펜도 없었다.")));
    TestTrue(TEXT("The first line"), Has(Six, TEXT("Tobias"), TEXT("베르단 도로 꺾임에서 남쪽 사 마일, 셋째 목자 제단 갈라진 돌 밑.")));
    TestTrue(TEXT("Six places"), Has(Six, TEXT("Tobias"), TEXT("여섯 곳입니다.")));
    const auto* Angles = Scene(TEXT("ms_wells_angles"));
    TestTrue(TEXT("The originals"), Has(Angles, TEXT("Elia"), TEXT("여기서는 이 여섯을 원본이라고 불러. 지금 서 있는 탑들보다 오래된 것.")));
    TestTrue(TEXT("Asleep, but listening"), Has(Angles, TEXT("Elia"), TEXT("잠들어 있어.")) && Has(Angles, TEXT("Elia"), TEXT("그런데 듣고 있어.")));
    const auto* Blood = Scene(TEXT("ms_wells_blood"));
    TestTrue(TEXT("The blood under her nose"), Blood && Blood->Definition.Rows[0].Text.NarrateKo.StartsWith(TEXT("그녀가 코 밑을 닦았다. 손가락에 피가 묻었다.")));
    TestTrue(TEXT("Not now"), Has(Blood, TEXT("Elia"), TEXT("지금은 안 할 거야.")) && Has(Blood, nullptr, TEXT("그는 덮었다.")));
    const auto* Arcane = Scene(TEXT("ms_wells_arcane"));
    TestTrue(TEXT("Arcane"), Has(Arcane, TEXT("Tobias"), TEXT("아르케인입니다.")));
    TestTrue(TEXT("One badge"), Has(Arcane, TEXT("Tobias"), TEXT("셋 다 통과할 수 있다고 약속할 수는 없습니다.")));
    TestTrue(TEXT("Two sets of footsteps behind him"), Arcane && Arcane->Definition.Rows.Last().Text.NarrateKo.EndsWith(TEXT("두 사람의 발소리가 뒤에서 따라왔다.")));

    bool bRows = true, bCast = true, bFaces = true, bNoArt = true, bNoGlow = true, bNoKairos = true;
    int32 Rows = 0;
    for (const FString& G : SceneGroups())
        if (G.StartsWith(TEXT("ms_wells_")))
            if (const auto* A = BuildScene(GetTransientPackage(), G, 0))
                for (int32 I = 0; I < A->Definition.Rows.Num(); ++I, ++Rows)
                {
                    const auto& R = A->Definition.Rows[I];
                    bRows &= R.OriginalIndex == I && (R.Text.bHasTextKo || R.Text.bHasNarrateKo);
                    // 24화: only the three speak. Kairos does not appear before 26화.
                    bCast &= !R.Text.bHasSpeaker || R.Text.Speaker == TEXT("Arrel") || R.Text.Speaker == TEXT("Elia") || R.Text.Speaker == TEXT("Tobias");
                    if (R.Text.Speaker == TEXT("Tobias")) bFaces &= R.Presentation.Portrait == TEXT("tobias_neutral");
                    bNoArt &= !R.Presentation.bHasCg;
                    const FString Ko = R.Text.bHasTextKo ? R.Text.TextKo : R.Text.NarrateKo;
                    // The books open quietly: nothing glows, no light comes off a page.
                    bNoGlow &= !Ko.Contains(TEXT("빛"));
                    bNoKairos &= !Ko.Contains(TEXT("카이로스"));
                }
    TestTrue(TEXT("Rows indexed, in Korean"), bRows);
    TestTrue(TEXT("Only Arrel, Elia and Tobias speak"), bCast);
    TestTrue(TEXT("Tobias's own face"), bFaces);
    TestTrue(TEXT("No picture anywhere"), bNoArt);
    TestTrue(TEXT("Nothing glows"), bNoGlow);
    TestTrue(TEXT("No Kairos"), bNoKairos);
    TestEqual(TEXT("175 rows in seven scenes"), Rows, 175);
    TestEqual(TEXT("The toast in Korean"), MemoriaKorean::Text(TEXT("The six originals")), FString(TEXT("여섯 원본")));
    TestEqual(TEXT("The title in Korean"), MemoriaKorean::Text(TEXT("Two Notes")), FString(TEXT("두 노트")));

    TStrongObjectPtr<UGameInstance> Game(NewObject<UGameInstance>()); Game->Init();
    auto* Run = Game->GetSubsystem<UMemoriaRunSubsystem>();
    if (!TestTrue(TEXT("A run at Chapter 15"), Run->BeginStartingMemoryRun(15) == EMemoriaMemoryResult::Success)) { Game->Shutdown(); return false; }
    Run->SetLocale(TEXT("ko"));
    const TCHAR* Steps[][2] = {{TEXT("ch15_bread"), TEXT("목표  목초지 아래로: 물을 찾아")}, {TEXT("ch15_pasture"), TEXT("목표  움푹한 땅: 우물 둘")},
        {TEXT("ch15_dry"), TEXT("목표  토비아스의 돌: 새 글씨")}, {TEXT("ch15_six"), TEXT("목표  우물 옆 그늘: 엘리아의 각도")},
        {TEXT("ch15_angles"), TEXT("목표  엘리아 곁으로")}, {TEXT("ch15_blood"), TEXT("목표  북쪽 갈림 → 능선 너머")}};
    for (const auto& S : Steps) { Run->SetStoryFlag(S[0], true); TestEqual(*FString(S[0]), MemoriaNavigationGoal::Line(Run->GetRunSnapshot(), DryWellsMap, false), FString(S[1])); }
    for (const auto& S : Steps) Run->SetStoryFlag(S[0], false);
    for (const auto& M : MainSteps()) if (!FString(M.Flag).StartsWith(TEXT("ch15_")) && !FString(M.Flag).StartsWith(TEXT("ch16_")) && FString(M.Flag) != TEXT("ms_never")) Run->SetStoryFlag(M.Flag, true);
    TestTrue(TEXT("The main quest is water first"), MainRecord(Run->GetRunSnapshot(), true).Body.Contains(TEXT("→ 물부터: 움푹한 땅의 우물")));
    for (const TCHAR* F : {TEXT("ch15_dry"), TEXT("ch15_angles")}) Run->SetStoryFlag(F, true);
    TestTrue(TEXT("then over the ridge"), MainRecord(Run->GetRunSnapshot(), true).Body.Contains(TEXT("→ 능선 너머 아르케인 쪽으로")));
    Run->SetStoryFlag(TEXT("ch15_complete"), true);
    TestTrue(TEXT("then the gate"), MainRecord(Run->GetRunSnapshot(), true).Body.Contains(TEXT("→ 아르케인: 성문, 배지는 하나")));
    Game->Shutdown();
    return true;
}

namespace
{
// S389 in the renderer: down from the farmhouse through the pasture, the two dry wells, the two notes open, the six
// lines and the six angles, the blood, and up to the fork toward Arcane. Three walk: Tobias follows from the start.
class FDryWellsReplay final : public IAutomationLatentCommand
{
public:
    explicit FDryWellsReplay(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
    ~FDryWellsReplay() override { if (bFixed) { FApp::SetUseFixedTimeStep(bOldFixed); FApp::SetFixedDeltaTime(OldDelta); } }
    bool Update() override
    {
        if (LastFrame == GFrameCounter) return false;
        LastFrame = GFrameCounter;
        if (FPlatformTime::Seconds() - Started > 600) { Test->AddError(FString::Printf(TEXT("S389 timeout at step %d"), Step)); return true; }
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
        auto Capture = [&](const FString& Name) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Validation/S389Wells") / (Name + TEXT(".png")), true, false); };
        auto Flag = [&](const FString& Id) { return Run->GetRunSnapshot().GetFlag(Id); };
        auto Place = [&](const FVector2D& Source) { Pawn->SetActorLocation(MemoriaChapterMaps::ToWorld(Source) + FVector(0, 0, Pawn->GetActorLocation().Z)); };
        if (Host->GetState() == EMemoriaSliceState::Field)
        {
            const FMemoriaNarrativeView View = Host->GetView();
            for (const auto& Shot : Shots)
                if (!Taken.Contains(Shot.Key) && View.CueKey == Shot.Key) { Taken.Add(Shot.Key); ShotAt = Frame; ShotName = Shot.Value; }
            if (View.CueKey.StartsWith(TEXT("field:ms_wells_"))) Seen.Add(View.CueKey);
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
        {
            Test->TestEqual(TEXT("The map is the dry wells"), Map->GetMap(), FString(MemoriaManuscript::DryWellsMap));
            Test->TestEqual(TEXT("A development run in Chapter 15"), Run->GetRunSnapshot().CurrentChapter, int64(15));
            Test->TestTrue(TEXT("Tobias is in the party"), Flag(TEXT("party_tobias")));
            int32 Count = 0; for (TActorIterator<AMemoriaTobiasCompanion> It(World); It; ++It) ++Count;
            Test->TestEqual(TEXT("and walks with them"), Count, 1);
            ++Step; Mark = Frame; break;
        }
        case 1:
            if (Frame == Mark + 40) Capture(TEXT("S389Title"));
            if (Flag(TEXT("ch15_bread")) && Host->GetState() == EMemoriaSliceState::Exploration && Frame > Mark + 60)
            { Capture(TEXT("S389Pasture")); Place(Spec->Triggers[0].Rect.Center()); ++Step; Mark = Frame; }
            if (Frame > Mark + 9000) { Test->AddError(TEXT("The arrival did not finish")); return true; }
            break;
        case 2: Next(TEXT("ch15_pasture"), 1, TEXT("S389Poplars")); break;
        case 3:
            if (Flag(TEXT("ch15_dry"))) Test->TestTrue(TEXT("The books opened"), Flag(TEXT("ch15_books_opened")));
            Next(TEXT("ch15_dry"), 2, TEXT("S389Wells")); break;
        case 4: Next(TEXT("ch15_six"), 3); break;
        case 5:
            if (Flag(TEXT("ch15_angles"))) Test->TestTrue(TEXT("The six originals"), Flag(TEXT("ch15_six_originals")) && Host->GetTrace().Contains(TEXT("toast:The six originals")));
            Next(TEXT("ch15_angles"), 4); break;
        case 6: Next(TEXT("ch15_blood"), -1, TEXT("S389Three")); break;
        case 7:
            if (Map->IsChapterComplete())
            {
                Test->TestTrue(TEXT("The fork closes Chapter 15"), Flag(TEXT("ch15_complete")) && Flag(TEXT("canon_ch16_arcane_ready")) && Flag(TEXT("ch15_toward_arcane")));
                Test->TestTrue(TEXT("Every row of the seven scenes was shown"), Seen.Num() == 175);
                Test->TestTrue(TEXT("Its last line was shown"), Seen.Contains(TEXT("field:ms_wells_arcane:33")));
                Test->TestTrue(TEXT("The card shows"), Map->GetCard() && Map->GetCard()->IsShowing());
                ++Step; Mark = Frame;
            }
            if (Frame > Mark + 9000) { Test->AddError(TEXT("The exit never closed the chapter")); return true; }
            break;
        case 8:
            if (Frame == Mark + 30) Capture(TEXT("S389Complete"));
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
        {TEXT("field:ms_wells_bread:3"), TEXT("S389CutIt")}, {TEXT("field:ms_wells_pasture:3"), TEXT("S389NotOpen")},
        {TEXT("field:ms_wells_dry:2"), TEXT("S389NoWater")}, {TEXT("field:ms_wells_dry:19"), TEXT("S389CoverLifted")},
        {TEXT("field:ms_wells_six:4"), TEXT("S389FirstLine")}, {TEXT("field:ms_wells_six:15"), TEXT("S389SixPlaces")},
        {TEXT("field:ms_wells_angles:26"), TEXT("S389Originals")}, {TEXT("field:ms_wells_angles:29"), TEXT("S389Listening")},
        {TEXT("field:ms_wells_blood:0"), TEXT("S389Blood")}, {TEXT("field:ms_wells_arcane:13"), TEXT("S389Arcane")},
        {TEXT("field:ms_wells_arcane:33"), TEXT("S389Footsteps")}};
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDryWellsVisualTest, "MemoriaVisual.S389DryWells", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDryWellsVisualTest::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Memoria/Maps/L_DryWells"))) return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FDryWellsReplay(this)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
