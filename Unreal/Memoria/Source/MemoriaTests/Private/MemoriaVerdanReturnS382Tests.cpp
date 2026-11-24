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

// S382: Chapter 13, Verdan again, from 최종본 21화 「다시 베르단」 (Gemini's T-7 brief).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVerdanReturnS382Test, "Memoria.S382.VerdanReturn", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVerdanReturnS382Test::RunTest(const FString&)
{
    using namespace MemoriaManuscript;
    const auto* Town = MemoriaChapterMaps::Find(VerdanReturnMap);
    if (!TestNotNull(TEXT("Verdan again is a chapter map"), Town)) return false;
    TestEqual(TEXT("Chapter 13"), Town->Chapter, 13);
    TestEqual(TEXT("Its level"), MemoriaChapterMaps::LevelPath(VerdanReturnMap), FString(TEXT("/Game/Memoria/Maps/L_VerdanReturn")));
    TestEqual(TEXT("In Korean"), MemoriaChapterMaps::Korean(VerdanReturnMap), FString(TEXT("다시 베르단")));
    TestTrue(TEXT("The Godot Verdan market ground"), Town->Width == 30 && Town->Height == 20 && Town->TileNames.Contains(TEXT("STALL")) && Town->TileNames.Contains(TEXT("DOOR")));
    auto Open = [&](const FVector2D& P) { return !Town->IsSolid(Town->TileAt(int32(P.X) / Town->TileSize, int32(P.Y) / Town->TileSize)); };
    TestTrue(TEXT("Down the slope from the north"), Open(Town->Spawn) && Town->Spawn.Y < 2 * Town->TileSize);
    TestTrue(TEXT("and out east"), Open(Town->Exit.Rect.Center()) && Town->Exit.Rect.Center().X > (Town->Width - 2) * Town->TileSize);
    for (const auto& T : Town->Triggers) TestTrue(*(TEXT("Open ground: ") + T.Group), Open(T.Rect.Center()));
    // The Sump: its trigger stands right below the DOOR tiles.
    const int32 DoorTile = Town->TileNames.IndexOfByKey(TEXT("DOOR"));
    const auto& Sump = Town->Triggers[1];
    TestTrue(TEXT("The Sump's door is right above its trigger"), Sump.Group == TEXT("ms_verdan_sump") && Town->TileAt(int32(Sump.Rect.Origin.X) / Town->TileSize, int32(Sump.Rect.Origin.Y) / Town->TileSize - 1) == DoorTile);
    TestTrue(TEXT("The market has people, from the arrival on"), Town->AmbientNpcs.Num() >= 4 && Town->AmbientNpcsGate == TEXT("ch13_road"));
    TArray<FString> Groups; for (const auto& S : Town->Sequence) Groups.Add(S.Group);
    for (const auto& T : Town->Triggers) Groups.Add(T.Group);
    Groups.Add(Town->Exit.Group);
    TestEqual(TEXT("Seven scenes"), Groups.Num(), 7);
    for (const FString& G : Groups) TestTrue(*(TEXT("A manuscript scene: ") + G), IsScene(G));
    const auto* Basin = MemoriaChapterMaps::Find(BasinMap);
    TestTrue(TEXT("The basin's road leads to Verdan"), Basin && Basin->Exit.NextMap == VerdanReturnMap && Basin->Exit.NextChapter == 13);

    auto Has = [](const UMemoriaFieldAsset* A, const TCHAR* Speaker, const TCHAR* Ko)
    { return A && A->Definition.Rows.ContainsByPredicate([&](const FMemoriaFieldRow& R) { return (Speaker ? R.Text.Speaker == Speaker : !R.Text.bHasSpeaker) && (R.Text.TextKo == Ko || R.Text.NarrateKo == Ko); }); };
    TestTrue(TEXT("No wall"), Has(BuildScene(GetTransientPackage(), TEXT("ms_verdan_road"), 0), nullptr, TEXT("냄새가 먼저 왔다. 오래 쓴 기름에 매운 것을 넣어 볶는 냄새였다. 낮은 비탈 아래로 집들이 드러났다. 지붕 사이에서 가는 연기가 올라왔다. 성벽은 없었다. 밭을 지나던 길이 그대로 첫 노점들 사이로 들어갔다.")));
    const auto* Farewell = BuildScene(GetTransientPackage(), TEXT("ms_verdan_farewell"), 0);
    TestTrue(TEXT("This far"), Has(Farewell, TEXT("Sable"), TEXT("나는 여기까지다.")));
    TestTrue(TEXT("Catch it first"), Has(Farewell, TEXT("Sable"), TEXT("이 아이가 들던 걸 놓치면, 먼저 받아 줘라. 왜 놓쳤는지 묻느라 늦지 말고.")));
    TestTrue(TEXT("The seal"), Has(BuildScene(GetTransientPackage(), TEXT("ms_verdan_sump"), 0), nullptr, TEXT("끈이 걸려 있었다. 문틀의 고리에서 문의 고리로 이어졌고, 두 줄이 겹친 곳에는 붉은 밀랍이 붙어 있었다. 눌러 찍은 인장의 테두리가 작은 빛을 받았다. 관리국의 인장이었다.")));
    const auto* Vendor = BuildScene(GetTransientPackage(), TEXT("ms_verdan_vendor"), 0);
    TestTrue(TEXT("Filed"), Has(Vendor, TEXT("Vendor"), TEXT("파일됐어.")));
    TestTrue(TEXT("Never said a name"), Has(Vendor, TEXT("Vendor"), TEXT("에디터라고 한 적 없어. ...이름도 안 했어.")));
    TestTrue(TEXT("The editor is Arrel's word"), Has(Vendor, TEXT("Arrel"), TEXT("에디터.")));
    const auto* Alley = BuildScene(GetTransientPackage(), TEXT("ms_verdan_alley"), 0);
    TestTrue(TEXT("Not here"), Has(Alley, TEXT("Elia"), TEXT("여기서 확인하고 싶지 않아.")));
    TestTrue(TEXT("That man? (T-11)"), Has(Alley, TEXT("Arrel"), TEXT("그 사내?")) && Has(Alley, TEXT("Elia"), TEXT("모른다고 했어.")));
    const auto* Furrow = BuildScene(GetTransientPackage(), TEXT("ms_verdan_furrow"), 0);
    TestTrue(TEXT("North-west"), Has(Furrow, TEXT("Elia"), TEXT("아니. 북서쪽.")));
    TestTrue(TEXT("Something like this book"), Has(Furrow, TEXT("Elia"), TEXT("사람인지는 몰라. ...이 책 같은 게 있어.")));
    TestTrue(TEXT("The same? (T-11)"), Has(Furrow, TEXT("Arrel"), TEXT("같아?")));
    TestTrue(TEXT("Arrel tells her to sleep (T-11)"), Has(Furrow, TEXT("Arrel"), TEXT("지금은 좀 자.")) && !Has(Furrow, TEXT("Elia"), TEXT("지금은 좀 자.")));
    TestTrue(TEXT("The book stays shut"), Furrow && Furrow->Definition.Rows.Last().Text.NarrateKo.Contains(TEXT("책은 닫힌 채")));
    bool bRows = true, bCast = true, bFaces = true, bSableLeaves = true;
    for (const FString& G : SceneGroups())
        if (G.StartsWith(TEXT("ms_verdan_")))
            if (const auto* A = BuildScene(GetTransientPackage(), G, 0))
                for (int32 I = 0; I < A->Definition.Rows.Num(); ++I)
                {
                    const auto& R = A->Definition.Rows[I];
                    bRows &= R.OriginalIndex == I && (R.Text.bHasTextKo || R.Text.bHasNarrateKo);
                    // 21화: Arrel, Elia, Sable (until she turns back), the oil vendor. Malet is filed; no Tobias.
                    bCast &= !R.Text.bHasSpeaker || R.Text.Speaker == TEXT("Arrel") || R.Text.Speaker == TEXT("Elia") || R.Text.Speaker == TEXT("Sable") || R.Text.Speaker == TEXT("Vendor");
                    if (R.Text.Speaker == TEXT("Sable")) { bFaces &= R.Presentation.Portrait == TEXT("sable_calm"); bSableLeaves &= G == TEXT("ms_verdan_road") || G == TEXT("ms_verdan_farewell"); }
                    if (R.Text.Speaker == TEXT("Vendor")) bFaces &= !R.Presentation.bHasPortrait;
                }
    TestTrue(TEXT("Rows indexed, in Korean"), bRows);
    TestTrue(TEXT("Only the four speak: no Malet, no Tobias"), bCast);
    TestTrue(TEXT("Sable's face is the blind one; the vendor has none"), bFaces);
    TestTrue(TEXT("Sable speaks only until she turns back"), bSableLeaves);
    TestEqual(TEXT("Seven Verdan scenes"), SceneGroups().FilterByPredicate([](const FString& G) { return G.StartsWith(TEXT("ms_verdan_")); }).Num(), 7);
    TestEqual(TEXT("The vendor in Korean"), MemoriaKorean::Text(TEXT("Malet: filed")), FString(TEXT("말레트: 파일됨")));
    TestEqual(TEXT("The title in Korean"), MemoriaKorean::Text(TEXT("Verdan, Again")), FString(TEXT("다시 베르단")));

    TStrongObjectPtr<UGameInstance> Game(NewObject<UGameInstance>()); Game->Init();
    auto* Run = Game->GetSubsystem<UMemoriaRunSubsystem>();
    if (!TestTrue(TEXT("A run at Chapter 13"), Run->BeginStartingMemoryRun(13) == EMemoriaMemoryResult::Success)) { Game->Shutdown(); return false; }
    Run->SetLocale(TEXT("ko"));
    const TCHAR* Steps[][2] = {{TEXT("ch13_farewell"), TEXT("목표  시장으로: 공동 저수지")}, {TEXT("ch13_market"), TEXT("목표  셈터로: 광장 남쪽의 돌계단")},
        {TEXT("ch13_sump"), TEXT("목표  동쪽 모퉁이의 기름 노점")}, {TEXT("ch13_vendor"), TEXT("목표  모퉁이 안쪽 골목으로")}, {TEXT("ch13_alley"), TEXT("목표  동쪽 출구 → 묵은 밭")}};
    for (const auto& S : Steps) { Run->SetStoryFlag(S[0], true); TestEqual(*FString(S[0]), MemoriaNavigationGoal::Line(Run->GetRunSnapshot(), VerdanReturnMap, false), FString(S[1])); }
    for (const TCHAR* F : {TEXT("ch13_sump"), TEXT("ch13_vendor")}) Run->SetStoryFlag(F, false);
    for (const auto& M : MainSteps()) if (!FString(M.Flag).StartsWith(TEXT("ch13_")) && !FString(M.Flag).StartsWith(TEXT("ch14_")) && FString(M.Flag) != TEXT("ms_never")) Run->SetStoryFlag(M.Flag, true);
    TestTrue(TEXT("The main quest is at the Sump"), MainRecord(Run->GetRunSnapshot(), true).Body.Contains(TEXT("→ 다시 베르단: 셈터")));
    for (const TCHAR* F : {TEXT("ch13_sump"), TEXT("ch13_vendor"), TEXT("ch13_complete")}) Run->SetStoryFlag(F, true);
    TestTrue(TEXT("then north-west"), MainRecord(Run->GetRunSnapshot(), true).Body.Contains(TEXT("→ 북서쪽: 신호탑을 돌아 그루터기 사이로")));
    Game->Shutdown();
    return true;
}

namespace
{
// S382 in the renderer: down the slope into Verdan from the north; Sable turns back; the market, the sealed Sump,
// the oil stall (Malet filed), the alley; and the night in the old furrows.
class FVerdanReturnReplay final : public IAutomationLatentCommand
{
public:
    explicit FVerdanReturnReplay(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
    ~FVerdanReturnReplay() override { if (bFixed) { FApp::SetUseFixedTimeStep(bOldFixed); FApp::SetFixedDeltaTime(OldDelta); } }
    bool Update() override
    {
        if (LastFrame == GFrameCounter) return false;
        LastFrame = GFrameCounter;
        if (FPlatformTime::Seconds() - Started > 360) { Test->AddError(FString::Printf(TEXT("S382 timeout at step %d"), Step)); return true; }
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
        auto Capture = [&](const FString& Name) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Validation/S382Verdan") / (Name + TEXT(".png")), true, false); };
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
            Test->TestEqual(TEXT("The map is Verdan again"), Map->GetMap(), FString(MemoriaManuscript::VerdanReturnMap));
            Test->TestEqual(TEXT("A development run in Chapter 13"), Run->GetRunSnapshot().CurrentChapter, int64(13));
            ++Step; Mark = Frame; break;
        case 1:
            if (Frame == Mark + 40) Capture(TEXT("S382Title"));
            if (Flag(TEXT("ch13_farewell")) && Host->GetState() == EMemoriaSliceState::Exploration && Frame > Mark + 60)
            {
                Test->TestTrue(TEXT("Parted: Sable"), Flag(TEXT("ch13_sable_gone")) && Host->GetTrace().Contains(TEXT("toast:Parted: Sable")));
                Capture(TEXT("S382Slope")); Place(Spec->Triggers[0].Rect.Center()); ++Step; Mark = Frame;
            }
            if (Frame > Mark + 6000) { Test->AddError(TEXT("The arrival chain did not finish")); return true; }
            break;
        case 2: Next(TEXT("ch13_market"), 1); break;
        case 3:
            if (!bShot && Flag(TEXT("ch13_sump")) && Host->GetState() == EMemoriaSliceState::Exploration)
            { bShot = true; Capture(TEXT("S382Door")); Mark = Frame; break; }
            Next(TEXT("ch13_sump"), 2); break;
        case 4:
            if (!bAnger && Flag(TEXT("ch13_vendor")) && Host->GetState() == EMemoriaSliceState::Exploration)
            {
                bAnger = true;
                Test->TestTrue(TEXT("Malet: filed"), Flag(TEXT("ch13_malet_filed")) && Host->GetTrace().Contains(TEXT("toast:Malet: filed")));
                Capture(TEXT("S382Stall")); Mark = Frame; break;
            }
            Next(TEXT("ch13_vendor"), 3); break;
        case 5: Next(TEXT("ch13_alley"), -1); break;
        case 6:
            if (Map->IsChapterComplete())
            {
                Test->TestTrue(TEXT("The furrows close Chapter 13"), Flag(TEXT("ch13_complete")) && Flag(TEXT("canon_ch14_resonance_ready")) && Flag(TEXT("ch13_book_northwest")));
                Test->TestTrue(TEXT("Its last line was shown"), Taken.Contains(TEXT("field:ms_verdan_furrow:28")));
                Test->TestTrue(TEXT("The card shows"), Map->GetCard() && Map->GetCard()->IsShowing());
                ++Step; Mark = Frame;
            }
            if (Frame > Mark + 6000) { Test->AddError(TEXT("The exit never closed the chapter")); return true; }
            break;
        case 7:
            if (Frame == Mark + 30) Capture(TEXT("S382Complete"));
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
        {TEXT("field:ms_verdan_road:9"), TEXT("S382NoWall")}, {TEXT("field:ms_verdan_farewell:0"), TEXT("S382ThisFar")},
        {TEXT("field:ms_verdan_farewell:11"), TEXT("S382CatchIt")}, {TEXT("field:ms_verdan_farewell:17"), TEXT("S382Gone")},
        {TEXT("field:ms_verdan_market:0"), TEXT("S382Market")}, {TEXT("field:ms_verdan_sump:2"), TEXT("S382Seal")},
        {TEXT("field:ms_verdan_vendor:9"), TEXT("S382Filed")}, {TEXT("field:ms_verdan_vendor:16"), TEXT("S382NoName")},
        {TEXT("field:ms_verdan_alley:15"), TEXT("S382NotHere")}, {TEXT("field:ms_verdan_furrow:10"), TEXT("S382Warm")},
        {TEXT("field:ms_verdan_furrow:14"), TEXT("S382NorthWest")}, {TEXT("field:ms_verdan_furrow:28"), TEXT("S382Closed")}};
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVerdanReturnVisualTest, "MemoriaVisual.S382VerdanReturn", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVerdanReturnVisualTest::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Memoria/Maps/L_VerdanReturn"))) return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FVerdanReturnReplay(this)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
