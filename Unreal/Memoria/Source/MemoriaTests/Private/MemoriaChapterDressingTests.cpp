#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "Tests/AutomationCommon.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Framework/MemoriaSliceHost.h"
#include "Framework/MemoriaFieldPawn.h"
#include "Narrative/MemoriaNarrativeSubsystem.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Chapter/MemoriaChapterMap.h"
#include "Chapter/MemoriaChapterPresentation.h"
#include "Chapter/MemoriaChapterCardWidget.h"
#include "Combat/MemoriaFieldCombatSubsystem.h"
#include "Combat/MemoriaFieldMonster.h"
#include "InputCoreTypes.h"
#include "Combat/MemoriaCombatHudWidget.h"
#include "Combat/MemoriaExplorationHudWidget.h"
#include "Combat/MemoriaFieldHudWidget.h"
#include "Audio/MemoriaAudioSubsystem.h"
#include "EngineUtils.h"
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#if WITH_DEV_AUTOMATION_TESTS
namespace
{
// S338: the dressed chapter maps. Each map is entered, its arrival chain is played through, and Arrel is stood
// at the places the dressing was built for (the Belt's rail line, platform and freight; Drift's tarp, walls and
// trees), with a capture at each to be read. The dressing must not change where he can walk: the blocks are
// still one per solid tile.
// S344: the set pieces are Codex's S343 models; each map counts the kinds of model that stand in it.
struct FDressingView { const TCHAR* Name; FVector2D Tile; };
class FChapterDressingReplay final : public IAutomationLatentCommand
{
public:
    FChapterDressingReplay(FAutomationTestBase* InTest, TArray<FDressingView> InViews, int32 InLamps, int32 InKinds, FName InAir)
        : Test(InTest), Views(MoveTemp(InViews)), Lamps(InLamps), Kinds(InKinds), Air(InAir), Started(FPlatformTime::Seconds()) {}
    ~FChapterDressingReplay() override { if (bFixed) { FApp::SetUseFixedTimeStep(bOldFixed); FApp::SetFixedDeltaTime(OldDelta); } }
    bool Update() override
    {
        if (LastFrame == GFrameCounter) return false;
        LastFrame = GFrameCounter;
        if (FPlatformTime::Seconds() - Started > 240) { Test->AddError(FString::Printf(TEXT("Chapter dressing timeout at step %d"), Step)); return true; }
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
        if (Host->GetState() == EMemoriaSliceState::Field) { if (Frame % 4 == 0) Host->Confirm(); return false; }
        switch (Step)
        {
        case 0:
        {
            bool bArrived = !Map->GetCard()->IsShowing();
            for (const auto& Link : Spec->Sequence) bArrived = bArrived && Run->GetRunSnapshot().GetFlag(Link.Flag);
            if (!bArrived) { if (Frame > 6000) { Test->AddError(TEXT("The arrival chain did not finish")); return true; } break; }
            int32 Solid = 0; for (int32 T : Spec->Tiles) Solid += Spec->IsSolid(T) ? 1 : 0;
            Test->TestEqual(TEXT("The dressing leaves one block per solid tile"), Map->GetBlockerCount(), Solid);
            Test->TestTrue(TEXT("The ground wears the painted material"), Map->IsGroundPainted());
            Test->TestEqual(TEXT("The map's lamps burn"), Map->GetLampCount(), Lamps);
            Test->TestTrue(TEXT("The air carries dust or rain"), Map->GetMoteCount() >= 40);
            // S344: Codex's environment kit stands where the box stand-ins stood.
            Test->TestEqual(TEXT("The map's kinds of kit model stand"), Map->GetKitKindCount(), Kinds);
            Test->TestTrue(TEXT("Many of them"), Map->GetKitPropCount() >= 40);
            // S348: the revisit's three NPCs wear models in both maps now (Drift's on Codex's S347 townsfolk).
            Test->TestEqual(TEXT("The revisit's NPCs wear rigged models"), Map->GetRiggedNpcCount(), 3);
            // S339: the field HUD stands on the source's plates, and the ribbon names the place in the run's language.
            Test->TestTrue(TEXT("The status panel is drawn on its plate"), Map->GetExplorationHud() && Map->GetExplorationHud()->HasPlate());
            Test->TestTrue(TEXT("The combat bar has its command ribbon"), Map->GetCombatHud() && Map->GetCombatHud()->HasRibbon());
            if (const auto* Hud = PC->GetFieldHud(); Test->TestNotNull(TEXT("The field HUD is up while exploring"), Hud))
            {
                const bool bKo = Run->GetRunSnapshot().CurrentLocale == TEXT("ko");
                Test->TestTrue(TEXT("The place stands on the toast frame"), Hud->HasRibbon());
                Test->TestEqual(TEXT("The ribbon names the place"), Hud->GetView().Title, bKo ? MemoriaChapterMaps::Korean(Spec->Map) : Spec->TitleName);
                Test->TestEqual(TEXT("And its subtitle"), Hud->GetView().Subtitle, bKo ? MemoriaChapterMaps::Korean(Spec->Subtitle) : Spec->Subtitle);
                Test->TestEqual(TEXT("In the run's language"), Hud->GetView().bKorean, bKo);
            }
            // S345: the map's sound: the exploration track and the map's air, crossfaded in.
            if (auto* Audio = World->GetGameInstance()->GetSubsystem<UMemoriaAudioSubsystem>(); Test->TestNotNull(TEXT("Audio"), Audio))
            {
                Test->TestEqual(TEXT("The map plays the exploration track"), Audio->GetMusic(), FName(TEXT("exploration")));
                Test->TestTrue(TEXT("And it is heard"), Audio->IsMusicPlaying());
                Test->TestEqual(TEXT("The map's air"), Audio->GetAmbient(), Air);
                Test->TestTrue(TEXT("And it is heard"), Audio->IsAmbientPlaying());
            }
            ++Step; Mark = Frame; View = 0; break;
        }
        case 1:
            if (View >= Views.Num())
            {
                // S345: Arrel walks east along the open ground south of the house; each footfall plays the step.
                auto* Audio = World->GetGameInstance()->GetSubsystem<UMemoriaAudioSubsystem>();
                Steps = Audio ? Audio->GetCueCount(TEXT("step")) : 0;
                Pawn->SetActorLocation(MemoriaChapterMaps::ToWorld(FVector2D(6.5, 14.5) * Spec->TileSize) + FVector(0, 0, Pawn->GetActorLocation().Z));
                ++Step; Mark = Frame; break;
            }
            if (Frame == Mark + 1)
                Pawn->SetActorLocation(MemoriaChapterMaps::ToWorld((Views[View].Tile + FVector2D(.5, .5)) * Spec->TileSize) + FVector(0, 0, Pawn->GetActorLocation().Z));
            if (Frame == Mark + 30)
                FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Validation/ChapterDressing") / (FString(Views[View].Name) + TEXT(".png")), true, false);
            if (Frame >= Mark + 36) { ++View; Mark = Frame; }
            break;
        case 2:
            if (Frame < Mark + 100) { Pawn->SetActorLocation(Pawn->GetActorLocation() + FVector(2.5, 0, 0)); break; }
            if (auto* Audio = World->GetGameInstance()->GetSubsystem<UMemoriaAudioSubsystem>())
            {
                const int32 Made = Audio->GetCueCount(TEXT("step")) - Steps;
                Test->AddInfo(FString::Printf(TEXT("CHAPTER_STEPS %d over 250 units"), Made));
                Test->TestTrue(TEXT("Walking plays footsteps"), Made >= 2);
            }
            // S348: capture the revisit figures before the retained S345 audio lifecycle replay.
            Run->SetStoryFlag(Spec->AmbientNpcsGate, true);
            Pawn->SetActorLocation(MemoriaChapterMaps::ToWorld(Spec->AmbientNpcs[0].Position + FVector2D(40, 60)) + FVector(0, 0, Pawn->GetActorLocation().Z));
            ++Step; Mark = Frame; break;
        case 3:
            if (Frame == Mark + 40)
            {
                Test->TestEqual(TEXT("The revisit's NPCs stand"), Map->GetVisibleNpcCount(), 3);
                FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Validation/ChapterDressing") / (Spec->Map + TEXT("_Npcs.png")), true, false);
            }
            if (Frame >= Mark + 46) { ++Step; Mark = Frame; }
            break;
        case 4:
            // Let the final displacement reach the gait before measuring the standing interval.
            if (Frame == Mark + 5)
                Steps = World->GetGameInstance()->GetSubsystem<UMemoriaAudioSubsystem>()->GetCueCount(TEXT("step"));
            if (Frame >= Mark + 35)
            {
                auto* Audio = World->GetGameInstance()->GetSubsystem<UMemoriaAudioSubsystem>();
                Test->TestEqual(TEXT("Standing makes no footsteps"), Audio->GetCueCount(TEXT("step")), Steps);
                auto* Combat = World->GetSubsystem<UMemoriaFieldCombatSubsystem>();
                if (!Test->TestNotNull(TEXT("Chapter field combat"), Combat)) return true;
                Test->TestEqual(TEXT("No fight is active before the audio replay"), Combat->LiveMonsterCount(), 0);
                const auto Foes = Combat->SpawnWave(1, Pawn->GetActorLocation() + FVector(400, 0, 0), 0.f);
                if (!Test->TestEqual(TEXT("The audio replay starts one foe"), Foes.Num(), 1)) return true;
                Target = Foes[0]; Target->Stun(10.f);
                ++Step; Mark = Frame;
            }
            break;
        case 5:
            if (Frame >= Mark + 30)
            {
                auto* Audio = World->GetGameInstance()->GetSubsystem<UMemoriaAudioSubsystem>();
                Test->TestEqual(TEXT("A chapter fight switches to battle music"), Audio->GetMusic(), FName(TEXT("battle")));
                Test->TestTrue(TEXT("The battle loop plays"), Audio->IsMusicPlaying());
                Test->TestTrue(TEXT("The map air is removed during battle"), Audio->GetAmbient().IsNone() && !Audio->IsAmbientPlaying());
                Test->TestFalse(TEXT("Battle music is not dialogue-ducked"), Audio->IsDucked());
                if (!Test->TestTrue(TEXT("The replay foe is still alive"), Target.IsValid() && !Target->IsDead())) return true;
                Target->TakeHit(Target->GetHealth() + 1.f, Pawn->GetActorLocation(), 0.f);
                ++Step; Mark = Frame;
            }
            break;
        case 6:
            if (Frame >= Mark + 45)
            {
                auto* Audio = World->GetGameInstance()->GetSubsystem<UMemoriaAudioSubsystem>();
                Test->TestEqual(TEXT("Winning restores exploration music"), Audio->GetMusic(), FName(TEXT("exploration")));
                Test->TestTrue(TEXT("The restored music loop plays"), Audio->IsMusicPlaying());
                Test->TestEqual(TEXT("Winning restores this map's air"), Audio->GetAmbient(), Air);
                Test->TestTrue(TEXT("The restored air loop plays"), Audio->IsAmbientPlaying());
                Test->TestFalse(TEXT("The exploration loop is not left ducked"), Audio->IsDucked());
                Steps = Audio->GetCueCount(TEXT("step"));
                PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::Escape, IE_Pressed, 1.f));
                PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::Escape, IE_Released, 0.f));
                Test->TestTrue(TEXT("The chapter's pause menu stops movement"), World->IsPaused());
                ++Step; Mark = Frame;
            }
            break;
        case 7:
            if (Frame >= Mark + 30)
            {
                auto* Audio = World->GetGameInstance()->GetSubsystem<UMemoriaAudioSubsystem>();
                Test->TestEqual(TEXT("Paused movement makes no footsteps"), Audio->GetCueCount(TEXT("step")), Steps);
                PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::Escape, IE_Pressed, 1.f));
                PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::Escape, IE_Released, 0.f));
                Test->TestFalse(TEXT("Closing the menu resumes the chapter"), World->IsPaused());
                Host->EnterTitle(); ++Step; Mark = Frame;
            }
            break;
        case 8:
            if (Frame >= Mark + 30)
            {
                auto* Audio = World->GetGameInstance()->GetSubsystem<UMemoriaAudioSubsystem>();
                Test->TestEqual(TEXT("Returning from the chapter restores title music"), Audio->GetMusic(), FName(TEXT("title")));
                Test->TestTrue(TEXT("The title music loop plays"), Audio->IsMusicPlaying());
                Test->TestTrue(TEXT("The chapter air is removed on the title"), Audio->GetAmbient().IsNone() && !Audio->IsAmbientPlaying());
                Test->TestFalse(TEXT("Title music is not left ducked"), Audio->IsDucked());
                Test->AddInfo(TEXT("CHAPTER_AUDIO_LIFECYCLE idle, battle, win, pause and title verified"));
                return true;
            }
            break;
        }
        return false;
    }
private:
    FAutomationTestBase* Test;
    TArray<FDressingView> Views;
    int32 Lamps, Kinds;
    FName Air;
    int32 Steps = 0;
    TWeakObjectPtr<AMemoriaFieldMonster> Target;
    double Started, OldDelta = 0;
    uint64 LastFrame = MAX_uint64;
    int32 Step = 0, Frame = 0, Mark = 0, View = 0;
    bool bFixed = false, bOldFixed = false;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBeltDressingTest, "MemoriaVisual.BeltDressing", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBeltDressingTest::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Memoria/Maps/L_BeltWaystation"))) return false;
    // Lamps: two at the door, the signal post's, the platform's two, and three lantern posts.
    // Kit: signal post, platform shelter, crates, chain fence, lantern post, rail, banner pole, ruined wall, dry grass.
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FChapterDressingReplay(this,
        {{TEXT("BeltPlatform"), FVector2D(16, 2)}, {TEXT("BeltSignal"), FVector2D(6, 2)}, {TEXT("BeltFreight"), FVector2D(6, 13)},
         {TEXT("BeltExit"), FVector2D(22, 10)}, {TEXT("BeltDoor"), FVector2D(12, 13)}, {TEXT("BeltInside"), FVector2D(12, 9)},
         {TEXT("BeltRuin"), FVector2D(5, 5)}, {TEXT("BeltCorner"), FVector2D(18, 15)}}, 8, 9, TEXT("wind_light"))));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDriftDressingTest, "MemoriaVisual.DriftDressing", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDriftDressingTest::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Memoria/Maps/L_DriftShelter"))) return false;
    // Lamps: one on each of the awnings' four front poles, two lantern posts and the camp's beyond the east wall.
    // Kit: tarp, ruined wall, gramophone, dead tree, lantern post, crates, dry grass.
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FChapterDressingReplay(this,
        {{TEXT("DriftTarp"), FVector2D(10, 3)}, {TEXT("DriftStores"), FVector2D(17, 4)}, {TEXT("DriftSouth"), FVector2D(10, 14)},
         {TEXT("DriftEast"), FVector2D(21, 8)}, {TEXT("DriftWest"), FVector2D(3, 8)}, {TEXT("DriftGramophone"), FVector2D(5, 4)},
         {TEXT("DriftCamp"), FVector2D(22, 6)}, {TEXT("DriftShelter"), FVector2D(10, 8)}}, 7, 7, TEXT("rain"))));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
