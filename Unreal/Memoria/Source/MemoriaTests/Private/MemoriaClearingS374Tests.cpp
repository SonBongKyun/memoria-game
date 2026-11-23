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

// S374: Chapter 9, Haren's clearing, from 최종본 13화 「산수」 and 14화 「자장가가 돌아오다」.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FClearingS374Test, "Memoria.S374.Clearing", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FClearingS374Test::RunTest(const FString&)
{
    using namespace MemoriaManuscript;
    const auto* Clearing = MemoriaChapterMaps::Find(ClearingMap);
    if (!TestNotNull(TEXT("The clearing is a chapter map"), Clearing)) return false;
    TestEqual(TEXT("Chapter 9"), Clearing->Chapter, 9);
    TestEqual(TEXT("Its level"), MemoriaChapterMaps::LevelPath(ClearingMap), FString(TEXT("/Game/Memoria/Maps/L_HarenClearing")));
    TestEqual(TEXT("In Korean"), MemoriaChapterMaps::Korean(ClearingMap), FString(TEXT("하렌의 공터")));
    TestTrue(TEXT("Greener than the coast"), Clearing->Saturation > .6f && Clearing->Saturation < 1.f);
    TestEqual(TEXT("The rim forest's grass is read as grass"), Clearing->TileDetails.IsValidIndex(0) ? Clearing->TileDetails[0] : FString(), FString(TEXT("grass")));
    auto Open = [&](const FVector2D& P) { return !Clearing->IsSolid(Clearing->TileAt(int32(P.X) / Clearing->TileSize, int32(P.Y) / Clearing->TileSize)); };
    TestTrue(TEXT("They come up from the south"), Open(Clearing->Spawn) && Clearing->Spawn.Y > (Clearing->Height - 3) * Clearing->TileSize);
    TestTrue(TEXT("and leave north"), Open(Clearing->Exit.Rect.Center()) && Clearing->Exit.Rect.Center().Y < 2 * Clearing->TileSize);
    for (const auto& T : Clearing->Triggers) TestTrue(*(TEXT("Open ground: ") + T.Group), Open(T.Rect.Center()));
    TArray<FString> Groups; for (const auto& S : Clearing->Sequence) Groups.Add(S.Group);
    for (const auto& T : Clearing->Triggers) Groups.Add(T.Group);
    Groups.Add(Clearing->Exit.Group);
    TestEqual(TEXT("Six scenes"), Groups.Num(), 6);
    for (const FString& G : Groups) TestTrue(*(TEXT("A manuscript scene: ") + G), IsScene(G));
    const auto* Coast = MemoriaChapterMaps::Find(CoastMap);
    TestTrue(TEXT("The coast's road leads to the clearing"), Coast && Coast->Exit.NextMap == ClearingMap && Coast->Exit.NextChapter == 9);
    TestTrue(TEXT("and the clearing's back to the Seam"), Clearing->Exit.NextMap == ReturnMap && Clearing->Exit.NextChapter == 10);

    auto Has = [](const UMemoriaFieldAsset* A, const TCHAR* Speaker, const TCHAR* Ko)
    { return A && A->Definition.Rows.ContainsByPredicate([&](const FMemoriaFieldRow& R) { return (Speaker ? R.Text.Speaker == Speaker : !R.Text.bHasSpeaker) && (R.Text.TextKo == Ko || R.Text.NarrateKo == Ko); }); };
    TestTrue(TEXT("Tell me if it happens again"), Has(BuildScene(GetTransientPackage(), TEXT("ms_clearing_hand"), 0), TEXT("Elia"), TEXT("말할게.")));
    TestTrue(TEXT("That isn't real"), Has(BuildScene(GetTransientPackage(), TEXT("ms_clearing_trees"), 0), TEXT("Arrel"), TEXT("저건 진짜가 아니야.")));
    TestTrue(TEXT("So it's you two"), Has(BuildScene(GetTransientPackage(), TEXT("ms_clearing_haren"), 0), TEXT("Haren"), TEXT("너희였군.")));
    const auto* Rings = BuildScene(GetTransientPackage(), TEXT("ms_clearing_rings"), 0);
    TestTrue(TEXT("The same hand"), Has(Rings, TEXT("Arrel"), TEXT("같은 글씨야.")));
    TestTrue(TEXT("The tower is a copy"), Has(Rings, TEXT("Elia"), TEXT("탑은 더 오래된 것의 복제품이야. 더 오래된 것은 공유를 위한 거였어. 복제품은 보관을 위한 거야.")));
    TestTrue(TEXT("It's arithmetic"), Has(Rings, TEXT("Elia"), TEXT("본 곳을 찾는 게 아니야. 이건 기억이 아니야. 이건 산수야.")));
    TestTrue(TEXT("This time I put it in"), Has(Rings, TEXT("Elia"), TEXT("이번에는 내가 넣었어.")));
    TestTrue(TEXT("Something white"), Has(BuildScene(GetTransientPackage(), TEXT("ms_clearing_rock"), 0), TEXT("Arrel"), TEXT("흰 게 지나갔어.")));
    const auto* Channel = BuildScene(GetTransientPackage(), TEXT("ms_clearing_channel"), 0);
    TestTrue(TEXT("I didn't cut it"), Has(Channel, TEXT("Arrel"), TEXT("내가 안 팠어.")));
    TestTrue(TEXT("Different. They join"), Has(Channel, TEXT("Arrel"), TEXT("다른데. ...이어져.")));
    TestTrue(TEXT("At least to her face"), Has(Channel, TEXT("Arrel"), TEXT("적어도 그 사람 앞에서 물어야지.")));
    TestTrue(TEXT("The brown coat closes his notebook, unnamed"), Channel && Channel->Definition.Rows.Last().Text.NarrateKo.Contains(TEXT("갈색 코트의 사내")));
    bool bRows = true, bCast = true;
    for (const FString& G : SceneGroups())
        if (G.StartsWith(TEXT("ms_clearing_")))
            if (const auto* A = BuildScene(GetTransientPackage(), G, 0))
                for (int32 I = 0; I < A->Definition.Rows.Num(); ++I)
                {
                    const auto& R = A->Definition.Rows[I];
                    bRows &= R.OriginalIndex == I && (R.Text.bHasTextKo || R.Text.bHasNarrateKo);
                    // 13-14화: only Arrel, Elia and Haren speak. Kairos, Tobias and Sable are not here.
                    bCast &= !R.Text.bHasSpeaker || R.Text.Speaker == TEXT("Arrel") || R.Text.Speaker == TEXT("Elia") || R.Text.Speaker == TEXT("Haren");
                }
    TestTrue(TEXT("Rows indexed, in Korean"), bRows);
    TestTrue(TEXT("Only Arrel, Elia and Haren speak"), bCast);
    TestEqual(TEXT("Six clearing scenes"), SceneGroups().FilterByPredicate([](const FString& G) { return G.StartsWith(TEXT("ms_clearing_")); }).Num(), 6);
    TestEqual(TEXT("The toast in Korean"), MemoriaKorean::Text(TEXT("Returned: six notes")), FString(TEXT("돌아옴: 여섯 음")));
    TestEqual(TEXT("The title in Korean"), MemoriaKorean::Text(TEXT("This isn't memory. It's arithmetic.")), FString(TEXT("이건 기억이 아니야. 이건 산수야.")));

    TStrongObjectPtr<UGameInstance> Game(NewObject<UGameInstance>()); Game->Init();
    auto* Run = Game->GetSubsystem<UMemoriaRunSubsystem>();
    if (!TestTrue(TEXT("A run at Chapter 9"), Run->BeginStartingMemoryRun(9) == EMemoriaMemoryResult::Success)) { Game->Shutdown(); return false; }
    Run->SetLocale(TEXT("ko"));
    const TCHAR* Steps[][2] = {{TEXT("ch9_trees"), TEXT("목표  도끼 소리가 나는 공터로")}, {TEXT("ch9_haren"), TEXT("목표  쓰러진 나무의 잘린 면을 본다")},
        {TEXT("ch9_rings"), TEXT("목표  길 건너편 바위의 물로: 북쪽 길")}, {TEXT("ch9_rock"), TEXT("목표  북쪽 출구 → 부서진 암반 너머 마른 수로")}};
    for (const auto& S : Steps) { Run->SetStoryFlag(S[0], true); TestEqual(*FString(S[0]), MemoriaNavigationGoal::Line(Run->GetRunSnapshot(), ClearingMap, false), FString(S[1])); }
    // The main quest: after the coast, the clearing; after the channel, back to the Seam.
    Run->SetStoryFlag(TEXT("ch9_haren"), false); Run->SetStoryFlag(TEXT("ch9_rings"), false);
    for (const auto& Step : MainSteps()) if (!FString(Step.Flag).StartsWith(TEXT("ch9_")) && !FString(Step.Flag).StartsWith(TEXT("ch10_")) && !FString(Step.Flag).StartsWith(TEXT("ch11_")) && !FString(Step.Flag).StartsWith(TEXT("ch12_")) && !FString(Step.Flag).StartsWith(TEXT("ch13_")) && FString(Step.Flag) != TEXT("ms_never")) Run->SetStoryFlag(Step.Flag, true);
    TestTrue(TEXT("The main quest is at the clearing"), MainRecord(Run->GetRunSnapshot(), true).Body.Contains(TEXT("→ 하렌의 공터, 쓰러진 나무")));
    for (const TCHAR* F : {TEXT("ch9_haren"), TEXT("ch9_rings"), TEXT("ch9_complete")}) Run->SetStoryFlag(F, true);
    TestTrue(TEXT("then back to the Seam"), MainRecord(Run->GetRunSnapshot(), true).Body.Contains(TEXT("→ 더 심에서 세이블에게, 그 앞에서 묻는다")));
    Game->Shutdown();
    return true;
}

namespace
{
// S374 in the renderer: a development run comes up into the clearing from the south; the hand and the trees play;
// Haren, the rings (a new line in the blank book), the rock and the white thing; and the dry channel's night.
class FClearingReplay final : public IAutomationLatentCommand
{
public:
    explicit FClearingReplay(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
    ~FClearingReplay() override { if (bFixed) { FApp::SetUseFixedTimeStep(bOldFixed); FApp::SetFixedDeltaTime(OldDelta); } }
    bool Update() override
    {
        if (LastFrame == GFrameCounter) return false;
        LastFrame = GFrameCounter;
        if (FPlatformTime::Seconds() - Started > 360) { Test->AddError(FString::Printf(TEXT("S374 timeout at step %d"), Step)); return true; }
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
        auto Capture = [&](const FString& Name) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Validation/S374Clearing") / (Name + TEXT(".png")), true, false); };
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
            Test->TestEqual(TEXT("The map is the clearing"), Map->GetMap(), FString(MemoriaManuscript::ClearingMap));
            Test->TestEqual(TEXT("A development run in Chapter 9"), Run->GetRunSnapshot().CurrentChapter, int64(9));
            ++Step; Mark = Frame; break;
        case 1:
            if (Frame == Mark + 40) Capture(TEXT("S374Title"));
            if (Flag(TEXT("ch9_trees")) && Host->GetState() == EMemoriaSliceState::Exploration && Frame > Mark + 60)
            { Capture(TEXT("S374Road")); Place(Spec->Triggers[0].Rect.Center()); ++Step; Mark = Frame; }
            if (Frame > Mark + 6000) { Test->AddError(TEXT("The arrival chain did not finish")); return true; }
            break;
        case 2: Next(TEXT("ch9_haren"), 1); break;
        case 3:
            if (!bRingsShot && Flag(TEXT("ch9_rings")) && Host->GetState() == EMemoriaSliceState::Exploration)
            {
                bRingsShot = true;
                Test->TestTrue(TEXT("A new line in the blank book"), Flag(TEXT("ch9_tower_copy")) && Host->GetTrace().Contains(TEXT("toast:A new line in the blank book")));
                Capture(TEXT("S374Clearing")); Mark = Frame; break;
            }
            Next(TEXT("ch9_rings"), 2); break;
        case 4: Next(TEXT("ch9_rock"), -1); break;
        case 5:
            if (Map->IsChapterComplete())
            {
                Test->TestTrue(TEXT("The dry channel closes Chapter 9"), Flag(TEXT("ch9_complete")) && Flag(TEXT("canon_ch10_seam_return_ready")));
                Test->TestTrue(TEXT("Six notes, joined to her seven"), Flag(TEXT("ch9_six_notes")) && Flag(TEXT("ch9_lullaby_joined")) && Host->GetTrace().Contains(TEXT("toast:Returned: six notes")));
                Test->TestTrue(TEXT("Its last line was shown"), Taken.Contains(TEXT("field:ms_clearing_channel:40")));
                Test->TestTrue(TEXT("The card shows"), Map->GetCard() && Map->GetCard()->IsShowing());
                ++Step; Mark = Frame;
            }
            if (Frame > Mark + 6000) { Test->AddError(TEXT("The exit never closed the chapter")); return true; }
            break;
        case 6:
            if (Frame == Mark + 30) Capture(TEXT("S374Complete"));
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
    bool bFixed = false, bOldFixed = false, bRingsShot = false;
    FString ShotName;
    TSet<FString> Taken;
    const TArray<TPair<FString, FString>> Shots = {
        {TEXT("field:ms_clearing_trees:2"), TEXT("S374NotReal")}, {TEXT("field:ms_clearing_haren:2"), TEXT("S374Haren")},
        {TEXT("field:ms_clearing_rings:7"), TEXT("S374SameHand")}, {TEXT("field:ms_clearing_rings:24"), TEXT("S374Arithmetic")},
        {TEXT("field:ms_clearing_rings:29"), TEXT("S374Notch")}, {TEXT("field:ms_clearing_rock:1"), TEXT("S374White")},
        {TEXT("field:ms_clearing_channel:9"), TEXT("S374FourthNotch")}, {TEXT("field:ms_clearing_channel:18"), TEXT("S374Humming")},
        {TEXT("field:ms_clearing_channel:21"), TEXT("S374Join")}, {TEXT("field:ms_clearing_channel:40"), TEXT("S374BrownCoat")}};
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FClearingVisualTest, "MemoriaVisual.S374Clearing", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FClearingVisualTest::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Memoria/Maps/L_HarenClearing"))) return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FClearingReplay(this)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
