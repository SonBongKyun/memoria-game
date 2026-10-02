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
struct FDressingView { const TCHAR* Name; FVector2D Tile; };
class FChapterDressingReplay final : public IAutomationLatentCommand
{
public:
    FChapterDressingReplay(FAutomationTestBase* InTest, TArray<FDressingView> InViews, int32 InLamps)
        : Test(InTest), Views(MoveTemp(InViews)), Lamps(InLamps), Started(FPlatformTime::Seconds()) {}
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
            ++Step; Mark = Frame; View = 0; break;
        }
        case 1:
            if (View >= Views.Num()) return true;
            if (Frame == Mark + 1)
                Pawn->SetActorLocation(MemoriaChapterMaps::ToWorld((Views[View].Tile + FVector2D(.5, .5)) * Spec->TileSize) + FVector(0, 0, Pawn->GetActorLocation().Z));
            if (Frame == Mark + 30)
                FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Validation/ChapterDressing") / (FString(Views[View].Name) + TEXT(".png")), true, false);
            if (Frame >= Mark + 36) { ++View; Mark = Frame; }
            break;
        }
        return false;
    }
private:
    FAutomationTestBase* Test;
    TArray<FDressingView> Views;
    int32 Lamps;
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
    // Lamps: two at the door, the signal post's, the platform's, and three posts where the road leaves the yard.
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FChapterDressingReplay(this,
        {{TEXT("BeltPlatform"), FVector2D(18, 2)}, {TEXT("BeltSignal"), FVector2D(6, 2)}, {TEXT("BeltFreight"), FVector2D(6, 13)},
         {TEXT("BeltExit"), FVector2D(21, 9)}, {TEXT("BeltDoor"), FVector2D(12, 13)}, {TEXT("BeltInside"), FVector2D(12, 9)}}, 7)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDriftDressingTest, "MemoriaVisual.DriftDressing", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDriftDressingTest::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Memoria/Maps/L_DriftShelter"))) return false;
    // Lamps: one on each of the shelter's four poles and two posts on the road.
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FChapterDressingReplay(this,
        {{TEXT("DriftTarp"), FVector2D(10, 3)}, {TEXT("DriftStores"), FVector2D(17, 4)}, {TEXT("DriftSouth"), FVector2D(10, 14)},
         {TEXT("DriftEast"), FVector2D(21, 8)}, {TEXT("DriftWest"), FVector2D(3, 8)}}, 6)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
