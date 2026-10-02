#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "Tests/AutomationCommon.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Framework/MemoriaSliceHost.h"
#include "Framework/MemoriaFieldPawn.h"
#include "Presentation/MemoriaFieldCharacterComponent.h"
#include "Narrative/MemoriaNarrativeSubsystem.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Combat/MemoriaFieldCombatSubsystem.h"
#include "Chapter/MemoriaChapterMap.h"
#include "Chapter/MemoriaChapterPresentation.h"
#include "Chapter/MemoriaChapterCardWidget.h"
#include "EngineUtils.h"
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#if WITH_DEV_AUTOMATION_TESTS
namespace
{
// S331: the Belt Waystation on its revisit (Chapter 3 closed). The props of _setup_map_decorations stand from
// the first visit; the ambient NPCs appear only once the chapter is closed; and walking then fills the source's
// random encounter model with the map's own range and pool: the warning first, then foes in the field, one
// battle counted, and no second encounter while they live.
class FChapterRevisitReplay final : public IAutomationLatentCommand
{
public:
    explicit FChapterRevisitReplay(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
    ~FChapterRevisitReplay() override { if (bFixed) { FApp::SetUseFixedTimeStep(bOldFixed); FApp::SetFixedDeltaTime(OldDelta); } }
    bool Update() override
    {
        if (LastFrame == GFrameCounter) return false;
        LastFrame = GFrameCounter;
        if (FPlatformTime::Seconds() - Started > 200) { Test->AddError(FString::Printf(TEXT("Chapter revisit timeout at step %d"), Step)); return true; }
        UWorld* World = GEditor->PlayWorld;
        auto* PC = World ? Cast<AMemoriaSliceController>(World->GetFirstPlayerController()) : nullptr;
        auto* Pawn = PC ? Cast<AMemoriaFieldPawn>(PC->GetPawn()) : nullptr;
        if (!Pawn || World->GetTimeSeconds() < .2) return false;
        if (!bFixed) { bFixed = true; bOldFixed = FApp::UseFixedTimeStep(); OldDelta = FApp::GetFixedDeltaTime(); FApp::SetUseFixedTimeStep(true); FApp::SetFixedDeltaTime(1.0 / 60.0); }
        ++Frame;
        auto* Host = World->GetGameInstance()->GetSubsystem<UMemoriaNarrativeSubsystem>();
        auto* Run = World->GetGameInstance()->GetSubsystem<UMemoriaRunSubsystem>();
        auto* Combat = World->GetSubsystem<UMemoriaFieldCombatSubsystem>();
        AMemoriaChapterPresentation* Map = nullptr; for (TActorIterator<AMemoriaChapterPresentation> It(World); It; ++It) Map = *It;
        const auto* Spec = Map ? Map->GetSpec() : nullptr;
        if (!Spec || !Combat) return false;
        auto Capture = [&](const TCHAR* Name) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Validation/ChapterRevisit") / (FString(Name) + TEXT(".png")), true, false); };
        auto Place = [&](const FVector2D& Source) { Pawn->SetActorLocation(MemoriaChapterMaps::ToWorld(Source) + FVector(0, 0, Pawn->GetActorLocation().Z)); };
        auto Count = [&](const TCHAR* Prefix) { int32 N = 0; for (const FString& E : Host->GetTrace()) N += E.StartsWith(Prefix) ? 1 : 0; return N; };
        switch (Step)
        {
        case 0:
        {
            // The IR: both maps' props, NPCs and encounter pools, as the scripts set them up.
            const auto* Drift = MemoriaChapterMaps::Find(TEXT("drift_shelter"));
            Test->TestTrue(TEXT("The Belt: a tank and three cracks"), Spec->Decorations.Num() == 4 && Spec->Decorations[0].Kind == TEXT("tank") && Spec->Decorations[3].Kind == TEXT("crack"));
            Test->TestTrue(TEXT("The tank leans over two by three tiles"), Spec->Decorations[0].Size.Equals(FVector2D(64, 96)) && Spec->Decorations[0].Origin.Equals(FVector2D(640, 224)) && FMath::IsNearlyEqual(Spec->Decorations[0].Rotation, .1f));
            Test->TestTrue(TEXT("The cracks follow the road"), Spec->Decorations[1].Origin.Equals(FVector2D(366, 168)) && Spec->Decorations[3].Origin.Equals(FVector2D(430, 168)));
            Test->TestTrue(TEXT("The Belt's revisit NPCs"), Spec->AmbientNpcs.Num() == 3 && Spec->AmbientNpcsGate == TEXT("ch3_complete") && Spec->AmbientNpcs[1].Preset == TEXT("bureau_agent") && Spec->AmbientNpcs[1].Position.Equals(FVector2D(464, 272)));
            Test->TestTrue(TEXT("The Belt's encounter pool and range"), Spec->Encounters.Num() == 3 && Spec->Encounters[1].Name == TEXT("Void Wisp") && Spec->Encounters[1].bVoid && Spec->EncounterMin == 50. && Spec->EncounterMax == 90.);
            if (Test->TestNotNull(TEXT("Drift Shelter"), Drift))
            {
                int32 Rubble = 0, Lights = 0; for (const auto& D : Drift->Decorations) { Rubble += D.Kind == TEXT("rubble") ? 1 : 0; Lights += D.bLight ? 1 : 0; }
                Test->TestTrue(TEXT("Drift Shelter: the campfire, its light and eight rubble heaps"), Drift->Decorations.Num() == 10 && Drift->Decorations[0].Kind == TEXT("fire") && Rubble == 8 && Lights == 1);
                Test->TestTrue(TEXT("Its light sits on the fire"), Drift->Decorations[1].bLight && Drift->Decorations[1].Origin.Equals(Drift->Decorations[0].Origin + FVector2D(4, 4)) && FMath::IsNearlyEqual(Drift->Decorations[1].Energy, .8f));
                Test->TestTrue(TEXT("Its NPCs and encounters wait on the closed chapter"), Drift->AmbientNpcs.Num() == 3 && Drift->AmbientNpcsGate == TEXT("ch4_complete") && Drift->Encounters.Num() == 3 && Drift->EncounterMin == 50.);
            }
            // The first visit: the props stand, the road is empty, nothing stalks it.
            Test->TestEqual(TEXT("Every prop is built"), Map->GetDecorationCount(), Spec->Decorations.Num());
            // S335: Codex's models stand in for the cylinder and the pixel cards.
            Test->TestEqual(TEXT("The water tank is the model"), Map->GetModelPropCount(), 1);
            Test->TestEqual(TEXT("The three NPCs wear rigged models"), Map->GetRiggedNpcCount(), 3);
            Test->TestEqual(TEXT("No ambient NPCs on the first visit"), Map->GetVisibleNpcCount(), 0);
            Test->TestFalse(TEXT("No random encounters on the first visit"), Map->AreEncountersOpen());
            // Chapter 3 closed, as a save loaded in the completed map finds it.
            for (const auto& S : Spec->Sequence) { Run->SetStoryFlag(S.Flag, true); for (const FString& F : S.Flags) Run->SetStoryFlag(F, true); }
            Run->SetStoryFlag(Spec->Exit.Requires, true); Run->SetStoryFlag(Spec->Exit.Completes, true); Run->SetCurrentChapter(4);
            // Every one-time object of the revisit is spent, so only the encounters act while Arrel paces.
            for (const auto& C : Spec->Chests) Run->SetStoryFlag(C.Flag, true);
            for (const auto& C : Spec->Clues) Run->SetStoryFlag(C.Flag, true);
            for (int32 I = 0; I < Spec->Battles.Num(); ++I) Run->SetStoryFlag(FString::Printf(TEXT("battle_%s_%d"), *Spec->Map, I + 1), true);
            for (const auto& T : Spec->Triggers) Run->SetStoryFlag(T.Flag, true);
            Run->SetLocale(TEXT("ko"));
            Battles = Run->GetRunSnapshot().TotalBattles;
            Place(FVector2D(520, 300));
            ++Step; Mark = Frame; break;
        }
        case 1:
            // Past the chapter title card (it runs on the screen's clock, not the fixed step).
            if (Frame < Mark + 30 || (Map->GetCard() && Map->GetCard()->IsShowing())) break;
            Test->TestEqual(TEXT("The ambient NPCs stand on the revisit"), Map->GetVisibleNpcCount(), 3);
            Test->TestTrue(TEXT("The encounters are open"), Map->AreEncountersOpen());
            Capture(TEXT("RevisitBelt"));
            ++Step; Mark = Frame; break;
        case 2:
            // S340: left alone for a while, the NPCs stroll: each stays on open ground near its place.
            if (Frame < Mark + 660) break;
            Test->TestTrue(TEXT("The NPCs have walked"), Map->GetNpcTravel() > 60.f);
            for (int32 I = 0; I < 3; ++I)
            {
                const FVector At = Map->GetAmbientNpc(I)->GetComponentLocation();
                const FVector2D P = MemoriaChapterMaps::ToSource(At);
                Test->TestTrue(TEXT("An NPC stays near its place"), FVector::Dist2D(At, Map->GetAmbientHome(I)) <= AMemoriaChapterPresentation::NpcRoam + 5.f);
                Test->TestFalse(TEXT("And on open ground"), Spec->IsSolid(Spec->TileAt(FMath::FloorToInt32(P.X / Spec->TileSize), FMath::FloorToInt32(P.Y / Spec->TileSize))));
            }
            // Arrel comes up beside the guard, on the open ground to its south-west: it stops and turns to face him.
            Place(MemoriaChapterMaps::ToSource(Map->GetAmbientNpc(2)->GetComponentLocation()) + FVector2D(-30, 22));
            Step = 20; Mark = Frame; break;
        case 20:
        {
            if (Frame < Mark + 90) break;
            const FVector Npc = Map->GetAmbientNpc(2)->GetComponentLocation();
            const float Want = ((Pawn->GetActorLocation() - Npc) * FVector(1, 1, 0)).Rotation().Yaw;
            Test->TestTrue(TEXT("The guard has turned to Arrel"), FMath::Abs(FMath::FindDeltaAngleDegrees(Map->GetAmbientYaw(2), Want)) < 8.f);
            Test->TestTrue(TEXT("And faces him as a rigged figure"), FMath::Abs(FMath::FindDeltaAngleDegrees(Map->GetAmbientNpc(2)->GetYaw(), Want)) < 8.f);
            Capture(TEXT("RevisitNpcs"));
            Step = 21; Mark = Frame; break;
        }
        case 21:
            if (Frame < Mark + 20) break;
            Test->TestTrue(TEXT("The model takes the map's range and pool"), Map->GetEncounterModel().MinSteps == 50. && Map->GetEncounterModel().MaxSteps == 90. && Map->GetEncounterModel().PoolSize == 3
                && Map->GetEncounterModel().Threshold >= 50. && Map->GetEncounterModel().Threshold <= 90.);
            Step = 3; Mark = Frame; break;
        case 3:
            // Pacing ten tiles at a time fills the distance.
            if ((Frame - Mark) % 3 == 0) Place(((Frame - Mark) / 3) % 2 ? FVector2D(300, 400) : FVector2D(620, 400));
            if (Count(TEXT("encounter:warning")) == 1 && !bWarned)
            {
                bWarned = true;
                Test->TestEqual(TEXT("The warning comes before the foes"), Count(TEXT("encounter:field_started")), 0);
                Test->TestTrue(TEXT("It is raised in Korean"), Host->GetTrace().Contains(TEXT("toast:기억 소음이 닫힌다")));
            }
            if (Count(TEXT("encounter:field_started")) == 1)
            {
                Test->TestTrue(TEXT("The warning was raised"), bWarned);
                const int32 Foes = Combat->LiveMonsterCount();
                Test->TestTrue(TEXT("Three husks or two thieves rise"), Foes == 2 || Foes == 3);
                Test->TestEqual(TEXT("One battle is counted"), Run->GetRunSnapshot().TotalBattles, Battles + 1);
                ++Step; Mark = Frame;
            }
            if (Frame > Mark + 600) { Test->AddError(TEXT("Pacing never raised an encounter")); return true; }
            break;
        case 4:
            if (Frame == Mark + 20) Capture(TEXT("RevisitEncounter"));
            // Walking on through the fight starts no second encounter.
            if ((Frame - Mark) % 3 == 0) Place(((Frame - Mark) / 3) % 2 ? FVector2D(300, 400) : FVector2D(620, 400));
            if (Frame > Mark + 90)
            {
                Test->TestTrue(TEXT("The foes still stand"), Combat->LiveMonsterCount() > 0);
                Test->TestEqual(TEXT("No second encounter during the fight"), Count(TEXT("encounter:field_started")), 1);
                Test->TestEqual(TEXT("Still one battle"), Run->GetRunSnapshot().TotalBattles, Battles + 1);
                return true;
            }
            break;
        }
        return false;
    }
private:
    FAutomationTestBase* Test;
    double Started, OldDelta = 0;
    uint64 LastFrame = MAX_uint64;
    int32 Step = 0, Frame = 0, Mark = 0;
    int64 Battles = 0;
    bool bFixed = false, bOldFixed = false, bWarned = false;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChapterRevisitTest, "MemoriaVisual.ChapterRevisit", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChapterRevisitTest::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Memoria/Maps/L_BeltWaystation"))) return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FChapterRevisitReplay(this)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
