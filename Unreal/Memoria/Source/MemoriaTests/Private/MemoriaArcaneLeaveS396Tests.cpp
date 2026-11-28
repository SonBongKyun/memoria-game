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

// S396: Chapter 17, leaving Arcane, from 최종본 27화 「조용함의 무게」 to 30화 「어머니의 자장가」 (Gemini's T-14 and
// T-17 briefs). The room behind the brass ring plays as the arrival; Hanna, who never speaks, walks them to the south gate.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArcaneLeaveS396Test, "Memoria.S396.ArcaneLeave", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArcaneLeaveS396Test::RunTest(const FString&)
{
    using namespace MemoriaManuscript;
    const auto* Leave = MemoriaChapterMaps::Find(ArcaneLeaveMap);
    const auto* City = MemoriaChapterMaps::Find(ArcaneMap);
    if (!TestNotNull(TEXT("Leaving Arcane is a chapter map"), Leave) || !TestNotNull(TEXT("and so is Arcane"), City)) return false;
    TestEqual(TEXT("Chapter 17"), Leave->Chapter, 17);
    TestEqual(TEXT("Its level"), MemoriaChapterMaps::LevelPath(ArcaneLeaveMap), FString(TEXT("/Game/Memoria/Maps/L_ArcaneLeave")));
    TestEqual(TEXT("In Korean"), MemoriaChapterMaps::Korean(ArcaneLeaveMap), FString(TEXT("아르케인")));
    // The same streets as Chapter 16, tile for tile.
    TestTrue(TEXT("Arcane's own ground again"), Leave->Width == City->Width && Leave->Height == City->Height && Leave->TileNames == City->TileNames);
    bool bSame = true;
    for (int32 Y = 0; Y < Leave->Height; ++Y) for (int32 X = 0; X < Leave->Width; ++X) bSame &= Leave->TileAt(X, Y) == City->TileAt(X, Y);
    TestTrue(TEXT("tile for tile"), bSame);
    const int32 Door = Leave->TileNames.IndexOfByKey(TEXT("DOOR"));
    auto TileOf = [&](const FVector2D& P) { return FIntPoint(int32(P.X) / Leave->TileSize, int32(P.Y) / Leave->TileSize); };
    auto Open = [&](const FVector2D& P) { const FIntPoint T = TileOf(P); return !Leave->IsSolid(Leave->TileAt(T.X, T.Y)); };
    const FIntPoint SpawnTile = TileOf(Leave->Spawn);
    TestTrue(TEXT("Out of the door with the brass ring"), Open(Leave->Spawn) && Leave->TileAt(SpawnTile.X, SpawnTile.Y - 1) == Door);
    // Chapter 16 ended at that same door.
    TestTrue(TEXT("where Chapter 16 went in"), TileOf(City->Exit.Rect.Center()) == SpawnTile);
    const FVector2D ExitAt = Leave->Exit.Rect.Center();
    TestTrue(TEXT("The exit is the south gate"), Open(ExitAt) && ExitAt.Y > (Leave->Height - 3) * Leave->TileSize);
    for (const auto& T : Leave->Triggers) TestTrue(*(TEXT("Open ground: ") + T.Group), Open(T.Rect.Center()));
    TestEqual(TEXT("Two places on the way"), Leave->Triggers.Num(), 2);
    // S397 (T-21): Hanna turns off on open road, east of the gate street, clear of the bread-window queue.
    TestTrue(TEXT("Hanna's leaving point is open road"), Open(HannaLeavingPoint) && TileOf(HannaLeavingPoint).X > 13 && TileOf(HannaLeavingPoint).Y < 12);
    TestTrue(TEXT("Past the cistern, then the low stone inside the gate"), Leave->Triggers.Num() == 2 && Leave->Triggers[0].Group == TEXT("ms_leave_cistern")
        && Leave->Triggers[1].Group == TEXT("ms_leave_gate") && Leave->Triggers[1].Rect.Center().Y > Leave->Triggers[0].Rect.Center().Y);
    TestTrue(TEXT("No fights inside the walls"), Leave->Battles.IsEmpty() && Leave->EncountersGate == TEXT("ms_never"));
    TestTrue(TEXT("No lamps by day"), Leave->Decorations.IsEmpty());
    TestEqual(TEXT("The city from above"), Leave->Splash, City->Splash);
    TestTrue(TEXT("Nobody queues at the booths now"), !Leave->AmbientNpcs.ContainsByPredicate([](const FMemoriaChapterNpc& N) { return N.bHasFace && FMath::IsNearlyEqual(N.Face.X, 208.f, 1.f); }));
    TestTrue(TEXT("A clue at the low stone"), Leave->Clues.Num() == 1 && Open(Leave->Clues[0].Origin + FVector2D(16, 16)));
    TArray<FString> Groups; for (const auto& S : Leave->Sequence) Groups.Add(S.Group);
    TestEqual(TEXT("Ten scenes in the room"), Groups.Num(), 10);
    TestTrue(TEXT("27화 to 30화, in order"), Groups.Num() == 10 && Groups[0] == TEXT("ms_leave_quiet") && Groups[3] == TEXT("ms_leave_hanna") && Groups[6] == TEXT("ms_leave_printer") && Groups[8] == TEXT("ms_leave_lullaby") && Groups.Last() == TEXT("ms_leave_door"));
    TestTrue(TEXT("Hanna walks with them from the door"), Leave->Sequence.Last().Flags.Contains(TEXT("party_hanna")));
    for (const auto& T : Leave->Triggers) Groups.Add(T.Group);
    Groups.Add(Leave->Exit.Group);
    TestEqual(TEXT("Thirteen scenes"), Groups.Num(), 13);
    for (const FString& G : Groups) TestTrue(*(TEXT("A manuscript scene: ") + G), IsScene(G));
    TestTrue(TEXT("Arcane's room leads here"), City->Exit.NextMap == ArcaneLeaveMap && City->Exit.NextChapter == 17);
    TestTrue(TEXT("and the gate on toward Chapter 18"), Leave->Exit.NextChapter == 18 && Leave->Exit.NextMap == EastRoadMap && Leave->Exit.Flags.Contains(TEXT("canon_ch18_east_road_ready")));

    auto Has = [](const UMemoriaFieldAsset* A, const TCHAR* Speaker, const TCHAR* Ko)
    { return A && A->Definition.Rows.ContainsByPredicate([&](const FMemoriaFieldRow& R) { return (Speaker ? R.Text.Speaker == Speaker : !R.Text.bHasSpeaker) && (R.Text.TextKo == Ko || R.Text.NarrateKo == Ko); }); };
    auto Scene = [](const TCHAR* G) { return BuildScene(GetTransientPackage(), G, 0); };
    const auto* Quiet = Scene(TEXT("ms_leave_quiet"));
    TestTrue(TEXT("27화 opens by the short candle"), Quiet && Quiet->Definition.Rows[0].Text.NarrateKo.StartsWith(TEXT("토비아스는 앉았다가 곧 일어났다.")));
    TestTrue(TEXT("Reading makes you bleed too"), Has(Quiet, TEXT("Arrel"), TEXT("읽는 것도 피 나.")));
    const auto* Relay = Scene(TEXT("ms_leave_relay"));
    TestTrue(TEXT("Not by the hand"), Has(Relay, nullptr, TEXT("중계는 손으로 열리지 않는다.")));
    TestTrue(TEXT("By a sound heard"), Has(Relay, nullptr, TEXT("중계는 들려진 소리로 열린다.")));
    TestTrue(TEXT("The keeping place"), Has(Relay, nullptr, TEXT("여는 몸은 보관장소였던 몸이다.")));
    TestTrue(TEXT("The seat will know"), Has(Relay, nullptr, TEXT("좌석은 알 것이다.")));
    TestTrue(TEXT("Opened all the way, it kills"), Has(Relay, nullptr, TEXT("끝까지 열면 죽는다.")));
    const auto* Cost = Scene(TEXT("ms_leave_cost"));
    TestTrue(TEXT("Reading and opening"), Has(Cost, TEXT("Elia"), TEXT("읽는 것하고 여는 건 달라. 오늘은 읽기만 했어. 열지는 않았고.")));
    TestTrue(TEXT("Not him yet"), Has(Cost, TEXT("Elia"), TEXT("내가 먼저 알아야 해.")));
    TestTrue(TEXT("Someone I know"), Has(Cost, TEXT("Tobias"), TEXT("아는 사람입니다.")));
    const auto* Hanna = Scene(TEXT("ms_leave_hanna"));
    TestTrue(TEXT("This is Hanna"), Has(Hanna, TEXT("Tobias"), TEXT("한나 씨입니다. 오래전부터 아는 분이에요.")));
    TestTrue(TEXT("Her board: keep the book closed"), Has(Hanna, nullptr, TEXT("책은 닫아 둬.")));
    TestTrue(TEXT("leave today"), Has(Hanna, nullptr, TEXT("오늘 안에 나가.")));
    TestTrue(TEXT("You decide"), Has(Scene(TEXT("ms_leave_east")), nullptr, TEXT("네가 정해.")));
    const auto* Wait = Scene(TEXT("ms_leave_wait"));
    TestTrue(TEXT("Somewhere to go first"), Has(Wait, nullptr, TEXT("먼저 다녀올 곳이 있다.")));
    TestTrue(TEXT("Ten years"), Has(Wait, TEXT("Tobias"), TEXT("십 년입니다.")));
    const auto* Printer = Scene(TEXT("ms_leave_printer"));
    TestTrue(TEXT("Lumea, ten years ago: sit"), Has(Printer, nullptr, TEXT("앉으시오.")));
    TestTrue(TEXT("What we reach"), Has(Printer, nullptr, TEXT("우리가 다다르는 것들을.")));
    TestTrue(TEXT("The middle's alike"), Has(Printer, TEXT("Elia"), TEXT("가운데가 닮았어. 앞뒤는 달라.")));
    const auto* Songs = Scene(TEXT("ms_leave_songs"));
    TestTrue(TEXT("Past five notes"), Songs && Songs->Definition.Rows.Last().Text.NarrateKo == TEXT("다섯 음을 지났는데, 한나는 멈추지 않았다."));
    const auto* Lullaby = Scene(TEXT("ms_leave_lullaby"));
    TestTrue(TEXT("It's a lullaby"), Has(Lullaby, TEXT("Arrel"), TEXT("자장가야.")));
    TestTrue(TEXT("Little one. Slowly. Then, the river."), Has(Lullaby, TEXT("Elia"), TEXT("작은 아이야. 천천히. 그다음에 강.")));
    TestTrue(TEXT("Not today"), Has(Lullaby, TEXT("Elia"), TEXT("오늘은 안 불러요.")));
    TestTrue(TEXT("As far as the south gate"), Has(Scene(TEXT("ms_leave_door")), nullptr, TEXT("나는 남문까지.")));
    const auto* Out = Scene(TEXT("ms_leave_out"));
    TestTrue(TEXT("Companions?"), Has(Out, TEXT("Gate Clerk"), TEXT("동행은요?")) && Has(Out, TEXT("Tobias"), TEXT("둘입니다.")));
    TestTrue(TEXT("His shortened stride"), Out && Out->Definition.Rows.Last().Text.NarrateKo.EndsWith(TEXT("그의 짧아진 보폭을 따라갔다.")));

    const FString Panorama(TEXT("res://assets/cg/generated/env_gray_belt_panorama.png"));
    bool bRows = true, bCast = true, bFaces = true, bArt = true, bStreetArt = false;
    int32 Rows = 0;
    for (const FString& G : SceneGroups())
        if (G.StartsWith(TEXT("ms_leave_")))
            if (const auto* A = BuildScene(GetTransientPackage(), G, 0))
            {
                const bool bStreet = G == TEXT("ms_leave_cistern") || G == TEXT("ms_leave_gate");
                for (int32 I = 0; I < A->Definition.Rows.Num(); ++I, ++Rows)
                {
                    const auto& R = A->Definition.Rows[I];
                    bRows &= R.OriginalIndex == I && (R.Text.bHasTextKo || R.Text.bHasNarrateKo);
                    const FString& Who = R.Text.Speaker;
                    // Hanna never speaks: her board's words are narration. Kairos is not in these episodes.
                    bCast &= !R.Text.bHasSpeaker || Who == TEXT("Arrel") || Who == TEXT("Elia") || Who == TEXT("Tobias") || Who == TEXT("Gate Clerk");
                    if (Who == TEXT("Tobias")) bFaces &= R.Presentation.Portrait == TEXT("tobias_neutral");
                    if (Who == TEXT("Gate Clerk")) bFaces &= !R.Presentation.bHasPortrait;
                    if (Who == TEXT("Elia")) bFaces &= R.Presentation.Portrait.StartsWith(TEXT("elia_"));
                    // The room on black (each room scene opens on Dark); the streets over the city from above.
                    if (R.Presentation.bHasCg) { bArt &= bStreet ? R.Presentation.Cg == Panorama : (I == 0 && R.Presentation.Cg.IsEmpty() && G != TEXT("ms_leave_out")); bStreetArt |= bStreet; }
                    if (I == 0 && !bStreet && G != TEXT("ms_leave_out")) bArt &= R.Presentation.bHasCg && R.Presentation.Cg.IsEmpty();
                }
            }
    TestTrue(TEXT("Rows indexed, in Korean"), bRows);
    TestTrue(TEXT("The cast: the three and the gate clerk; Hanna never speaks"), bCast);
    TestTrue(TEXT("Faces: Tobias's and Elia's; the clerk has none"), bFaces);
    TestTrue(TEXT("The room on black, pictures only in the streets"), bArt && bStreetArt);
    TestEqual(TEXT("414 rows in thirteen scenes"), Rows, 414);
    TestNotNull(TEXT("The city is imported"), MemoriaNarrativeArtwork::Load(Panorama));
    TestEqual(TEXT("The title in Korean"), MemoriaKorean::Text(TEXT("The Singer")), FString(TEXT("노래하는 자")));
    TestEqual(TEXT("The subtitle in Korean"), MemoriaKorean::Text(TEXT("Mother's Lullaby")), FString(TEXT("어머니의 자장가")));
    TestEqual(TEXT("The lullaby's words in Korean"), MemoriaKorean::Text(TEXT("Remembered: little one, slowly, the river")), FString(TEXT("떠오름: 작은 아이야, 천천히, 강")));
    TestEqual(TEXT("The gate in Korean"), MemoriaKorean::Text(TEXT("The South Gate")), FString(TEXT("남문")));

    TStrongObjectPtr<UGameInstance> Game(NewObject<UGameInstance>()); Game->Init();
    auto* Run = Game->GetSubsystem<UMemoriaRunSubsystem>();
    if (!TestTrue(TEXT("A run at Chapter 17"), Run->BeginStartingMemoryRun(17) == EMemoriaMemoryResult::Success)) { Game->Shutdown(); return false; }
    Run->SetLocale(TEXT("ko"));
    const TCHAR* Steps[][2] = {{TEXT("ch17_door"), TEXT("목표  한나를 따라 저수조 쪽으로")}, {TEXT("ch17_cistern"), TEXT("목표  거리를 지나 남문 안쪽의 낮은 돌로")},
        {TEXT("ch17_stone"), TEXT("목표  남문 아래로: 성문 서기")}};
    for (const auto& S : Steps) { Run->SetStoryFlag(S[0], true); TestEqual(*FString(S[0]), MemoriaNavigationGoal::Line(Run->GetRunSnapshot(), ArcaneLeaveMap, false), FString(S[1])); }
    for (const auto& S : Steps) Run->SetStoryFlag(S[0], false);
    for (const auto& M : MainSteps()) if (!FString(M.Flag).StartsWith(TEXT("ch17_")) && !FString(M.Flag).StartsWith(TEXT("ch18_")) && FString(M.Flag) != TEXT("ms_never")) Run->SetStoryFlag(M.Flag, true);
    TestTrue(TEXT("The main quest is the room"), MainRecord(Run->GetRunSnapshot(), true).Body.Contains(TEXT("→ 토비아스의 방: 책, 그리고 노래하는 사람")));
    Run->SetStoryFlag(TEXT("ch17_lullaby"), true);
    TestTrue(TEXT("then the gate with Hanna"), MainRecord(Run->GetRunSnapshot(), true).Body.Contains(TEXT("→ 한나와 함께, 셋째 종 전에 남문으로")));
    Run->SetStoryFlag(TEXT("ch17_stone"), true);
    TestTrue(TEXT("then out under it"), MainRecord(Run->GetRunSnapshot(), true).Body.Contains(TEXT("→ 남문 밖으로: 동행 둘")));
    Run->SetStoryFlag(TEXT("ch17_complete"), true);
    // S398: Chapter 18's first main step keeps this line.
    TestTrue(TEXT("then two miles to the cart"), MainRecord(Run->GetRunSnapshot(), true).Body.Contains(TEXT("→ 동쪽으로 가는 마차까지 두 마일. 아직 정한 건 없다")));
    Game->Shutdown();
    return true;
}

namespace
{
// S396 in the renderer: the ten room scenes play from the arrival; Hanna joins at the door; past the cistern; the low
// stone, where she leaves (hanna_gone); out under the gate, and the card closes Chapter 17.
class FArcaneLeaveReplay final : public IAutomationLatentCommand
{
public:
    explicit FArcaneLeaveReplay(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
    ~FArcaneLeaveReplay() override { if (bFixed) { FApp::SetUseFixedTimeStep(bOldFixed); FApp::SetFixedDeltaTime(OldDelta); } }
    bool Update() override
    {
        if (LastFrame == GFrameCounter) return false;
        LastFrame = GFrameCounter;
        if (FPlatformTime::Seconds() - Started > 1200) { Test->AddError(FString::Printf(TEXT("S396 timeout at step %d"), Step)); return true; }
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
        auto Capture = [&](const FString& Name) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Validation/S396Leave") / (Name + TEXT(".png")), true, false); };
        auto Flag = [&](const FString& Id) { return Run->GetRunSnapshot().GetFlag(Id); };
        auto Hannas = [&] { int32 Count = 0; for (TActorIterator<AMemoriaHannaCompanion> It(World); It; ++It) ++Count; return Count; };
        auto Place = [&](const FVector2D& Source) { Pawn->SetActorLocation(MemoriaChapterMaps::ToWorld(Source) + FVector(0, 0, Pawn->GetActorLocation().Z)); };
        if (Host->GetState() == EMemoriaSliceState::Field)
        {
            const FMemoriaNarrativeView View = Host->GetView();
            for (const auto& Shot : Shots)
                if (!Taken.Contains(Shot.Key) && View.CueKey == Shot.Key) { Taken.Add(Shot.Key); ShotAt = Frame; ShotName = Shot.Value; }
            if (View.CueKey.StartsWith(TEXT("field:ms_leave_"))) Seen.Add(View.CueKey);
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
            Test->TestEqual(TEXT("The map is leaving Arcane"), Map->GetMap(), FString(MemoriaManuscript::ArcaneLeaveMap));
            Test->TestEqual(TEXT("A development run in Chapter 17"), Run->GetRunSnapshot().CurrentChapter, int64(17));
            { int32 Count = 0; for (TActorIterator<AMemoriaTobiasCompanion> It(World); It; ++It) ++Count; Test->TestEqual(TEXT("Tobias walks with them"), Count, 1); }
            Test->TestEqual(TEXT("Hanna is not here before the door"), Hannas(), 0);
            ++Step; Mark = Frame; break;
        case 1:
            if (Frame == Mark + 40) Capture(TEXT("S396Title"));
            if (Flag(TEXT("ch17_door")) && Host->GetState() == EMemoriaSliceState::Exploration && Frame > Mark + 60)
            {
                Test->TestTrue(TEXT("The book read what opening costs"), Flag(TEXT("ch17_relay_read")));
                Test->TestTrue(TEXT("The lullaby's first words came back"), Flag(TEXT("ch17_lullaby_words")) && Host->GetTrace().Contains(TEXT("toast:Remembered: little one, slowly, the river")));
                Test->TestTrue(TEXT("Hanna walks with them from the door"), Flag(TEXT("party_hanna")) && !Flag(TEXT("hanna_gone")));
                // S397 (T-21): she joins mid-map, once, behind Tobias.
                Test->TestEqual(TEXT("Hanna is in the street"), Hannas(), 1);
                // Walk Arrel west along the street from the door for four seconds, so the companions come out behind him.
                if (!bShot) { bShot = true; Mark = Frame; break; }
                if (Frame <= Mark + 240) { Place(FMath::Lerp(Spec->Spawn, FVector2D(9.6 * 32.0, Spec->Spawn.Y), double(Frame - Mark) / 240.0)); break; }
                if (Frame == Mark + 330) { Capture(TEXT("S396HannaFollows")); break; }
                if (Frame < Mark + 350) break;
                bShot = false;
                Place(Spec->Triggers[0].Rect.Center()); ++Step; Mark = Frame;
            }
            if (Frame > Mark + 40000) { Test->AddError(TEXT("The room's scenes did not finish")); return true; }
            break;
        case 2:
            if (Flag(TEXT("ch17_cistern")) && Host->GetState() == EMemoriaSliceState::Exploration) Test->TestEqual(TEXT("Hanna still walks with them"), Hannas(), 1);
            Next(TEXT("ch17_cistern"), 1, TEXT("S396Cistern")); break;
        case 3:
            // The low stone: Hanna walks off toward the leaving point and is removed; then the gate.
            if (Flag(TEXT("ch17_stone")) && Host->GetState() == EMemoriaSliceState::Exploration && Frame > Mark + 20)
            {
                if (!bShot) { bShot = true; LeaveAt = Frame; Test->TestTrue(TEXT("Hanna turns into another alley"), Flag(TEXT("hanna_gone"))); break; }
                if (Frame == LeaveAt + 45) Capture(TEXT("S396HannaLeaving"));
                if (Hannas() == 0 && Frame > LeaveAt + 60)
                {
                    bShot = false; Capture(TEXT("S396LowStone"));
                    Place(Spec->Exit.Rect.Center()); ++Step; Mark = Frame;
                }
                else if (Frame > LeaveAt + 2400) { Test->AddError(TEXT("Hanna never reached her alley")); Step = 99; }
                break;
            }
            if (Frame > Mark + 12000) { Test->AddError(TEXT("Stuck before ch17_stone")); Step = 99; }
            break;
        case 4:
            if (Map->IsChapterComplete())
            {
                Test->TestTrue(TEXT("Out under the south gate closes Chapter 17"), Flag(TEXT("ch17_complete")) && Flag(TEXT("canon_ch18_east_road_ready")));
                Test->TestEqual(TEXT("Two companions: Hanna is gone"), Hannas(), 0);
                Test->TestTrue(TEXT("Every row of the thirteen scenes was shown"), Seen.Num() == 414);
                Test->TestTrue(TEXT("Its last line was shown"), Seen.Contains(TEXT("field:ms_leave_out:4")));
                Test->TestTrue(TEXT("The card shows"), Map->GetCard() && Map->GetCard()->IsShowing());
                ++Step; Mark = Frame;
            }
            if (Frame > Mark + 12000) { Test->AddError(TEXT("The gate never closed the chapter")); return true; }
            break;
        case 5:
            if (Frame == Mark + 30) Capture(TEXT("S396Complete"));
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
    int32 Step = 0, Frame = 0, Mark = 0, ShotAt = 0, LeaveAt = 0;
    bool bFixed = false, bOldFixed = false, bShot = false;
    FString ShotName;
    TSet<FString> Taken, Seen;
    const TArray<TPair<FString, FString>> Shots = {
        {TEXT("field:ms_leave_quiet:0"), TEXT("S396Candle")}, {TEXT("field:ms_leave_relay:10"), TEXT("S396SoundHeard")},
        {TEXT("field:ms_leave_relay:23"), TEXT("S396ItKills")}, {TEXT("field:ms_leave_cost:23"), TEXT("S396KnowFirst")},
        {TEXT("field:ms_leave_cost:43"), TEXT("S396SomeoneIKnow")}, {TEXT("field:ms_leave_hanna:11"), TEXT("S396ThisIsHanna")},
        {TEXT("field:ms_leave_hanna:20"), TEXT("S396BookClosed")}, {TEXT("field:ms_leave_east:23"), TEXT("S396YouDecide")},
        {TEXT("field:ms_leave_wait:19"), TEXT("S396TenYears")}, {TEXT("field:ms_leave_printer:14"), TEXT("S396Sit")},
        {TEXT("field:ms_leave_printer:22"), TEXT("S396WhatWeReach")}, {TEXT("field:ms_leave_songs:40"), TEXT("S396PastFive")},
        {TEXT("field:ms_leave_lullaby:47"), TEXT("S396Lullaby")}, {TEXT("field:ms_leave_lullaby:58"), TEXT("S396TheRiver")},
        {TEXT("field:ms_leave_door:3"), TEXT("S396AsFarAsTheGate")}, {TEXT("field:ms_leave_cistern:1"), TEXT("S396BootDrags")},
        {TEXT("field:ms_leave_gate:4"), TEXT("S396SecondMile")}, {TEXT("field:ms_leave_out:2"), TEXT("S396Two")}};
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArcaneLeaveVisualTest, "MemoriaVisual.S396ArcaneLeave", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArcaneLeaveVisualTest::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Memoria/Maps/L_ArcaneLeave"))) return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FArcaneLeaveReplay(this)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
