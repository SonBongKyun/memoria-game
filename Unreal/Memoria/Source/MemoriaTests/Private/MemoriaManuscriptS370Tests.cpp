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
#include "Interaction/MemoriaStoryPointActor.h"
#include "Localization/MemoriaKorean.h"
#include "Journal/MemoriaJournal.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Domain/MemoriaPlayerMemoryDomain.h"
#include "Chapter/MemoriaChapterMap.h"
#include "Chapter/MemoriaChapterPresentation.h"
#if WITH_DEV_AUTOMATION_TESTS
namespace
{
// Burn the first small owned memory the domain lets go of (never the name, the core or an identity).
bool BurnAny(UMemoriaRunSubsystem& Run)
{
    const auto Snapshot = Run.GetPlayerMemory()->GetSnapshot();
    for (const auto& State : Snapshot.Owned)
        if (!State.bBurned && !State.Id.Contains(TEXT("core")) && !State.Id.Contains(TEXT("name")) && !State.Id.Contains(TEXT("identity"))
            && Run.BurnMemory(State.Id) == EMemoriaMemoryResult::Success) return true;
    return false;
}
}

// S370: the final manuscript's Korean, its main quest and chapter 4's signposts, without a world.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FManuscriptS370Test, "Memoria.S370.Manuscript", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FManuscriptS370Test::RunTest(const FString&)
{
    using namespace MemoriaManuscript;
    // Lines the manuscript rewrote, and its names.
    TestEqual(TEXT("128 rewritten lines"), LineCount(), 128);
    TestEqual(TEXT("Malet asks for the first sword in his own words"), Canon(TEXT("제 값은, 당신이 처음 검을 잡은 날입니다.")),
        FString(TEXT("첫 입맞춤 같은 건 가져오지 마. 네가 지금의 네가 된 데 쓰인 기억이 필요해. 검을 처음 쥔 날.")));
    TestEqual(TEXT("The wall reads as the manuscript has it"), Canon(TEXT("대상은 7급 연소 효율을 시현함.")), FString(TEXT("대상은 7급 연소 효율을 보인다.")));
    TestEqual(TEXT("Miles, not kilometres"), Canon(TEXT("남쪽은 림 경계. 다음 경유지까지 14킬로미터.")), FString(TEXT("다음 경유지까지 아홉 마일.")));
    TestEqual(TEXT("Chapter 1's burn is a small one (2026-09-27)"), Canon(TEXT("한 번 태웠어. 3등급쯤. 없어져도 아깝지 않은 거야.")), FString(TEXT("하나. 작은 거. 없어도 되는 걸로.")));
    TestEqual(TEXT("Kairos files the class"), Canon(TEXT("7등급. 확인.")), FString(TEXT("연소자 분류, 7급. 확인.")));
    TestEqual(TEXT("Malet is 말레트"), Canon(TEXT("말렛의 탁자")), FString(TEXT("말레트의 탁자")));
    TestEqual(TEXT("BL-07 is 공백 7호"), Canon(TEXT("BL-07을 생각한다.")), FString(TEXT("공백 7호를 생각한다.")));
    TestEqual(TEXT("The bar is 셈터"), Canon(TEXT("웅덩이  /  말렛")), FString(TEXT("셈터  /  말레트")));
    TestEqual(TEXT("A whole 웅덩이 title is 셈터"), Canon(TEXT("웅덩이")), FString(TEXT("셈터")));
    TestEqual(TEXT("A puddle inside a sentence stays a puddle"), Canon(TEXT("입구 옆 웅덩이를 살폈다.")), FString(TEXT("입구 옆 웅덩이를 살폈다.")));
    TestEqual(TEXT("The village is 더 심"), Canon(TEXT("심")), FString(TEXT("더 심")));
    TestEqual(TEXT("The forest is 아헨 숲"), Canon(TEXT("[위치 추정: 림 외곽, 아켄 숲]")), FString(TEXT("[위치 추정: 림 외곽, 아헨 숲]")));
    TestEqual(TEXT("Place names in the map table"), MemoriaChapterMaps::Korean(TEXT("drift_shelter")), FString(TEXT("무너진 돌벽")));
    TestEqual(TEXT("Through the display path"), MemoriaKorean::Text(TEXT("Malet's table  |  E / A: sit")), FString(TEXT("말레트의 탁자  |  E / A: 앉기")));
    TestEqual(TEXT("The sign counter"), MemoriaKorean::Text(TEXT("Signposts 2 / 3")), FString(TEXT("표지판 2 / 3")));
    TestEqual(TEXT("English is not touched by the Korean layer"), MemoriaKorean::Text(TEXT("")), FString());
    for (const FString& Sample : {FString(TEXT("말렛, BL-07, 더 섬프, 14킬로미터")), FString(TEXT("분류자 보고: 아렐 식별")), FString(TEXT("획득: 백서"))})
        TestEqual(TEXT("Canon is idempotent"), Canon(Canon(Sample)), Canon(Sample));

    TStrongObjectPtr<UGameInstance> Game(NewObject<UGameInstance>()); Game->Init();
    auto* Run = Game->GetSubsystem<UMemoriaRunSubsystem>();
    if (!TestTrue(TEXT("A run at Chapter 2"), Run->BeginStartingMemoryRun(2) == EMemoriaMemoryResult::Success)) { Game->Shutdown(); return false; }
    // The main quest follows the run's own flags.
    TestEqual(TEXT("It starts in the forest"), MainStepIndex(Run->GetRunSnapshot()), 0);
    Run->SetStoryFlag(TEXT("ch2_arrived"), true);
    TestEqual(TEXT("Verdan: Malet"), MainStepIndex(Run->GetRunSnapshot()), 1);
    auto Main = MainRecord(Run->GetRunSnapshot(), true);
    TestTrue(TEXT("The current step is marked"), Main.Body.Contains(TEXT("→ 셈터에서 말레트에게 공백 7호를 묻는다")) && Main.Body.Contains(TEXT("아헨 숲을 빠져나가 베르단으로  (완료)")));
    TestFalse(TEXT("Later steps stay hidden"), Main.Body.Contains(TEXT("역참")));
    for (const TCHAR* F : {TEXT("ch2_malet_done"), TEXT("ch3_waystation_night"), TEXT("ch3_class_seven_message")}) Run->SetStoryFlag(F, true);
    Main = MainRecord(Run->GetRunSnapshot(), true);
    TestTrue(TEXT("Malet's three things and the wall are noted"), Main.Body.Contains(TEXT("경고(에디터 카이로스)")) && Main.Body.Contains(TEXT("대상은 7급 연소 효율을 보인다.")));
    TestEqual(TEXT("On to the east road"), MainStepIndex(Run->GetRunSnapshot()), 4);
    TestTrue(TEXT("In English too"), MainRecord(Run->GetRunSnapshot(), false).Label == TEXT("[MAIN] The Road to BL-07"));
    TestTrue(TEXT("The journal lists it first"), MemoriaJournal::QuestRecords(*Run, true).Num() >= 1 && MemoriaJournal::QuestRecords(*Run, true)[0].Title == TEXT("공백 7호를 찾아서"));

    // The signposts.
    const auto* Drift = MemoriaChapterMaps::Find(TEXT("drift_shelter"));
    if (TestNotNull(TEXT("The drift map"), Drift))
        for (int32 I = 0; I < SignCount(); ++I)
        {
            const FVector2D O = SignOrigin(I);
            TestFalse(TEXT("Each sign stands on open ground"), Drift->IsSolid(Drift->TileAt(int32(O.X) / Drift->TileSize, int32(O.Y) / Drift->TileSize)));
            TestEqual(TEXT("Its group names it"), SignIndex(SignGroup(I)), I);
        }
    TestEqual(TEXT("Not a sign"), SignIndex(TEXT("malet_backstory")), int32(INDEX_NONE));
    TestFalse(TEXT("Closed before the anchoring"), SignsOpen(Run->GetRunSnapshot()));
    TestFalse(TEXT("No signs in the journal yet"), MemoriaJournal::QuestRecords(*Run, true).ContainsByPredicate([](const FMemoriaJournalRecord& R) { return R.Title == TEXT("표지판"); }));
    Run->SetStoryFlag(AnchorFlag, true);
    TestTrue(TEXT("Open after it"), SignsOpen(Run->GetRunSnapshot()));
    const int32 Burned = Run->GetPlayerMemory()->GetSnapshot().BurnedHistory.Num();
    EnsureAnchorBaseline(*Run); EnsureAnchorBaseline(*Run);
    TestEqual(TEXT("The anchoring notes what had burned, once"), AnchorBaseline(Run->GetRunSnapshot()), Burned);
    TestFalse(TEXT("The letters hold"), Scrambled(Run->GetRunSnapshot(), Burned));
    TestTrue(TEXT("A burn since then loosens them"), Scrambled(Run->GetRunSnapshot(), Burned + 1));
    TestTrue(TEXT("A real burn happens"), BurnAny(*Run));
    TestTrue(TEXT("and the run's own count loosens them"), Scrambled(Run->GetRunSnapshot(), Run->GetPlayerMemory()->GetSnapshot().BurnedHistory.Num()));
    // The rows: Arrel reads a holding sign himself; Elia reads a loose one for him.
    UMemoriaFieldAsset* Clear = BuildSign(GetTransientPackage(), 1, false);
    UMemoriaFieldAsset* Loose = BuildSign(GetTransientPackage(), 1, true);
    if (TestNotNull(TEXT("Rows"), Clear) && TestNotNull(TEXT("Loose rows"), Loose))
    {
        TestTrue(TEXT("Arrel reads the waymark"), Clear->Definition.Rows.ContainsByPredicate([](const FMemoriaFieldRow& R)
            { return R.Text.Speaker == TEXT("Arrel") && R.Text.TextKo == TEXT("다음 경유지까지 아홉 마일."); }));
        TestTrue(TEXT("Elia reads it when it is loose"), Loose->Definition.Rows.ContainsByPredicate([](const FMemoriaFieldRow& R)
            { return R.Text.Speaker == TEXT("Elia") && R.Text.TextKo == TEXT("다음 경유지까지 아홉 마일."); }));
        bool bIndexed = true;
        for (int32 I = 0; I < Clear->Definition.Rows.Num(); ++I) bIndexed &= Clear->Definition.Rows[I].OriginalIndex == I && Clear->Definition.Rows[I].Id == FString::Printf(TEXT("field/ms_drift_sign_2/%d"), I);
        TestTrue(TEXT("Rows carry their own indices"), bIndexed);
    }
    TestNull(TEXT("No fourth sign"), BuildSign(GetTransientPackage(), 3, false));
    auto Signs = SignsRecord(Run->GetRunSnapshot(), true);
    TestTrue(TEXT("The journal counts the signs"), Signs.Body.EndsWith(TEXT("(0 / 3)")) && !Signs.Label.StartsWith(TEXT("[완료]")));
    for (int32 I = 0; I < SignCount(); ++I) Run->SetStoryFlag(SignGroup(I), true);
    Run->SetStoryFlag(SignsDoneFlag, true);
    Signs = SignsRecord(Run->GetRunSnapshot(), true);
    TestTrue(TEXT("and closes them"), SignsRead(Run->GetRunSnapshot()) == 3 && Signs.Label == TEXT("[완료] 표지판"));
    Game->Shutdown();
    return true;
}

namespace
{
// S370 in the renderer: the drift chain in the manuscript's Korean, then the three signposts on the east road.
// The first is read with the anchoring holding; a burn in between loosens the second and Elia reads it; the
// third closes the quest with its Grains.
class FSignsReplay final : public IAutomationLatentCommand
{
public:
    explicit FSignsReplay(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
    ~FSignsReplay() override { if (bFixed) { FApp::SetUseFixedTimeStep(bOldFixed); FApp::SetFixedDeltaTime(OldDelta); } }
    bool Update() override
    {
        if (LastFrame == GFrameCounter) return false;
        LastFrame = GFrameCounter;
        if (FPlatformTime::Seconds() - Started > 240) { Test->AddError(FString::Printf(TEXT("S370 timeout at step %d"), Step)); return true; }
        UWorld* World = GEditor->PlayWorld;
        auto* PC = World ? Cast<AMemoriaSliceController>(World->GetFirstPlayerController()) : nullptr;
        auto* Pawn = PC ? Cast<AMemoriaFieldPawn>(PC->GetPawn()) : nullptr;
        if (!Pawn || World->GetTimeSeconds() < .2) return false;
        if (!bFixed) { bFixed = true; bOldFixed = FApp::UseFixedTimeStep(); OldDelta = FApp::GetFixedDeltaTime(); FApp::SetUseFixedTimeStep(true); FApp::SetFixedDeltaTime(1.0 / 60.0); }
        ++Frame;
        auto* Host = World->GetGameInstance()->GetSubsystem<UMemoriaNarrativeSubsystem>();
        auto* Run = World->GetGameInstance()->GetSubsystem<UMemoriaRunSubsystem>();
        auto Capture = [&](const FString& Name) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Validation/S370Manuscript") / (Name + TEXT(".png")), true, false); };
        auto Flag = [&](const FString& Id) { return Run->GetRunSnapshot().GetFlag(Id); };
        auto Place = [&](const FVector2D& Source) { Pawn->SetActorLocation(MemoriaChapterMaps::ToWorld(Source) + FVector(0, 0, Pawn->GetActorLocation().Z)); };
        auto Point = [&](int32 I) -> AMemoriaStoryPointActor*
        {
            for (TActorIterator<AMemoriaStoryPointActor> It(World); It; ++It) if (It->GetGroup() == MemoriaManuscript::SignGroup(I)) return *It;
            return nullptr;
        };
        if (Host->GetState() == EMemoriaSliceState::Field)
        {
            const FMemoriaNarrativeView View = Host->GetView();
            // The anchoring in the manuscript's words, captured once on screen.
            if (!bShotAnchor && View.CueKey == TEXT("field:anchoring_session:3"))
            {
                Test->TestEqual(TEXT("Elia's anchoring line is the manuscript's"), MemoriaKorean::Text(View.Body), FString(TEXT("나랑 있어. ...이름.")));
                bShotAnchor = true; ShotAt = Frame;
            }
            if (bShotAnchor && Frame == ShotAt + 20) { Capture(TEXT("S370Anchoring")); return false; }
            if (Step >= 2 && !bShotSign[Reading] && Frame > SignAt + 30) { bShotSign[Reading] = true; Capture(FString::Printf(TEXT("S370Sign%d"), Reading + 1)); return false; }
            if (Frame % 6 == 0 && (!bShotAnchor || Frame > ShotAt + 30) && (Step < 2 || bShotSign[Reading])) Host->Confirm();
            return false;
        }
        switch (Step)
        {
        case 0:
            if (Flag(TEXT("ch4_night_watch")) && Host->GetState() == EMemoriaSliceState::Exploration && Frame > 80)
            {
                Test->TestTrue(TEXT("The anchoring was shown"), bShotAnchor);
                Test->TestTrue(TEXT("The signs are open"), MemoriaManuscript::SignsOpen(Run->GetRunSnapshot()));
                Test->TestEqual(TEXT("The anchoring's burn count is noted"), MemoriaManuscript::AnchorBaseline(Run->GetRunSnapshot()), Run->GetPlayerMemory()->GetSnapshot().BurnedHistory.Num());
                for (int32 I = 0; I < MemoriaManuscript::SignCount(); ++I) Test->TestNotNull(TEXT("Each sign stands on the map"), Point(I));
                ++Step; Mark = Frame;
            }
            if (Frame > 6000) { Test->AddError(TEXT("The drift chain did not finish")); return true; }
            break;
        case 1:
        {
            if (Frame < Mark + 10) break;
            // Read the next sign: walk to it, check it offers itself, and read it.
            if (Reading == 1 && !bBurned) { bBurned = true; Test->TestTrue(TEXT("A burn on the road"), BurnAny(*Run)); }
            const FVector2D At = MemoriaManuscript::SignOrigin(Reading);
            Place(At + FVector2D(0, 20));
            ++Step; Mark = Frame; break;
        }
        case 2:
        {
            if (Frame < Mark + 12) break;
            auto* Sign = Point(Reading);
            if (Frame == Mark + 12)
            {
                Test->TestTrue(TEXT("The sign can be read where Arrel stands"), Sign && Sign->CanInteract(*Pawn));
                Test->TestEqual(TEXT("Its prompt in Korean"), MemoriaKorean::Text(Sign ? Sign->InteractionPrompt() : FString()),
                    Reading == 0 ? FString(TEXT("기울어진 표지판  |  E / A: 읽기")) : Reading == 1 ? FString(TEXT("이정표  |  E / A: 읽기")) : FString(TEXT("돌벽의 돌  |  E / A: 읽기")));
                if (Frame == Mark + 12 && Reading == 0) Capture(TEXT("S370SignPrompt"));
                SignAt = Frame;
                Test->TestTrue(TEXT("Reading starts the sign's rows"), Sign && Sign->Interact(*Pawn));
            }
            ++Step; Mark = Frame; break;
        }
        case 3:
            if (Host->GetState() != EMemoriaSliceState::Exploration || Frame < Mark + 5) break;
            {
                const FString Group = MemoriaManuscript::SignGroup(Reading);
                Test->TestTrue(TEXT("The sign is read"), Flag(Group));
                // One burn after the anchoring loosens every sign read after it.
                Test->TestEqual(TEXT("Signs read after the burn read loose"), Host->GetTrace().Contains(TEXT("manuscript:sign_loose:") + Group), Reading >= 1);
                if (++Reading < MemoriaManuscript::SignCount()) { GrainsBefore = Run->GetRunSnapshot().Player.Grains; Step = 1; Mark = Frame; break; }
                Test->TestTrue(TEXT("The quest closes"), Flag(MemoriaManuscript::SignsDoneFlag) && Host->GetTrace().Contains(TEXT("quest:complete:drift_signs")));
                Test->TestEqual(TEXT("with its Grains"), Run->GetRunSnapshot().Player.Grains, GrainsBefore + MemoriaManuscript::SignsRewardGrains);
                Test->TestTrue(TEXT("and its toast"), Host->GetTrace().Contains(TEXT("toast:Quest Complete: Signposts")));
                ++Step; Mark = Frame;
            }
            break;
        case 4:
            if (Frame == Mark + 10) Capture(TEXT("S370SignsDone"));
            if (Frame < Mark + 20) break;
            return true;
        }
        return false;
    }
private:
    FAutomationTestBase* Test;
    double Started, OldDelta = 0;
    uint64 LastFrame = MAX_uint64;
    int32 Step = 0, Frame = 0, Mark = 0, ShotAt = 0, SignAt = 0, Reading = 0;
    int64 GrainsBefore = 0;
    bool bFixed = false, bOldFixed = false, bShotAnchor = false, bBurned = false;
    bool bShotSign[3] = {false, false, false};
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FManuscriptSignsVisualTest, "MemoriaVisual.S370Manuscript", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FManuscriptSignsVisualTest::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Memoria/Maps/L_DriftShelter"))) return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FSignsReplay(this)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
