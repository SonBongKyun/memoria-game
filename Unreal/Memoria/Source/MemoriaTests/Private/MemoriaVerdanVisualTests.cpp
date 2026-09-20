#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "Tests/AutomationCommon.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "Framework/MemoriaSliceHost.h"
#include "Framework/MemoriaFieldPawn.h"
#include "Interaction/MemoriaMaletActor.h"
#include "Presentation/MemoriaVerdanPresentation.h"
#include "Presentation/MemoriaVerdanArt.h"
#include "Narrative/MemoriaNarrativeSubsystem.h"
#include "Run/MemoriaRunSubsystem.h"
#include "PaperSpriteComponent.h"
#include "PaperSprite.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "InputKeyEventArgs.h"
#include "JsonObjectConverter.h"
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "UnrealClient.h"
#if WITH_DEV_AUTOMATION_TESTS
namespace
{
class FVerdanVisualReplay final : public IAutomationLatentCommand
{
public:
    explicit FVerdanVisualReplay(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
    ~FVerdanVisualReplay() override
    { if (bFixed) { FApp::SetUseFixedTimeStep(bOldFixed); FApp::SetFixedDeltaTime(OldDelta); } }
    bool Update() override
    {
        if (LastFrame == GFrameCounter) return false;
        LastFrame = GFrameCounter;
        if (FPlatformTime::Seconds() - Started > 100) { Test->AddError(TEXT("Verdan visual replay timeout")); return true; }
        UWorld* World = GEditor->PlayWorld;
        auto* PC = World ? Cast<AMemoriaSliceController>(World->GetFirstPlayerController()) : nullptr;
        auto* Pawn = PC ? Cast<AMemoriaFieldPawn>(PC->GetPawn()) : nullptr;
        if (!Pawn || !PC->PlayerInput || World->GetTimeSeconds() < 0.3) return false;
        auto* Host = World->GetGameInstance()->GetSubsystem<UMemoriaNarrativeSubsystem>();
        auto* Run = World->GetGameInstance()->GetSubsystem<UMemoriaRunSubsystem>();
        if (!bFixed)
        {
            bFixed = true; bOldFixed = FApp::UseFixedTimeStep(); OldDelta = FApp::GetFixedDeltaTime();
            FApp::SetUseFixedTimeStep(true); FApp::SetFixedDeltaTime(1.0/60.0);
            // Enter the existing paid route; campaign tests separately replay its physical UI inputs.
            for (int32 I = 0; I < 20 && Host->GetState() == EMemoriaSliceState::VN; ++I) Host->Confirm(1);
            return false;
        }
        if (!World->GetMapName().EndsWith(TEXT("L_VerdanHost"))) return false;
        AMemoriaVerdanPresentation* Presentation = nullptr;
        int32 Count = 0;
        for (TActorIterator<AMemoriaVerdanPresentation> It(World); It; ++It) { Presentation = *It; ++Count; }
        if (!Test->TestEqual(TEXT("One presentation layer after travel"), Count, 1)) return true;
        auto Key = [&](FKey K, EInputEvent Event) { PC->InputKey(FInputKeyEventArgs::CreateSimulated(K, Event, Event == IE_Released ? 0.0f : 1.0f)); };
        auto Capture = [&](const TCHAR* Name)
        { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Validation/Presentation2")/(FString(Name)+TEXT(".png")), true, false); };
        if (Frame == 0)
        {
            FJsonObjectConverter::UStructToJsonObjectString(Run->GetRunSnapshot(), RunBefore);
            FJsonObjectConverter::UStructToJsonObjectString(Run->GetPlayerMemory()->GetSnapshot(), MemoryBefore);
            TraceBefore = Host->GetTrace();
            TArray<UPrimitiveComponent*> Components; Presentation->GetComponents(Components);
            for (auto* Component : Components) Test->TestTrue(TEXT("Presentation cannot block movement or visibility traces"), Component->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
            int32 Bodies = 0;
            for (TActorIterator<AStaticMeshActor> It(World); It; ++It)
            {
                Test->TestTrue(TEXT("Existing geometry remains collidable"), It->GetStaticMeshComponent()->GetCollisionEnabled() != ECollisionEnabled::NoCollision);
                ++Bodies;
            }
            Test->TestEqual(TEXT("All seven original physical surfaces retained"), Bodies, 7);
            Test->TestTrue(TEXT("Pawn collider remains 16 x 16"), Cast<UBoxComponent>(Pawn->GetRootComponent())->GetUnscaledBoxExtent().Equals(FVector(8, 8, 8)));
            Test->TestEqual(TEXT("Camera width unchanged"), Pawn->GetFieldCamera()->OrthoWidth, 1280.0f);
            for (TActorIterator<AMemoriaMaletActor> It(World); It; ++It)
                Test->TestTrue(TEXT("Original Malet position"), It->GetActorLocation().Equals(AMemoriaMaletActor::DevelopmentLocation()));
        }
        const FKey Keys[] = {EKeys::W, EKeys::D, EKeys::S, EKeys::A};
        const TCHAR* Directions[] = {TEXT("Up"), TEXT("Right"), TEXT("Down"), TEXT("Left")};
        for (int32 I = 0; I < 4; ++I)
        {
            const int32 Start = 10 + I * 36;
            if (Frame == Start) Key(Keys[I], IE_Pressed);
            if (Frame > Start + 3 && Frame < Start + 20)
            {
                Test->TestEqual(TEXT("Direction follows real displacement"), Presentation->Facing(), FString(Directions[I]));
                Test->TestTrue(TEXT("Real movement activates source gait"), Presentation->IsWalking());
                SeenWalkSprites.Add(Pawn->GetFieldSprite()->GetSprite()->GetName());
                Test->TestTrue(TEXT("Foot pivot follows physical pawn"), FVector2D(Pawn->GetFieldSprite()->GetComponentLocation()).Equals(FVector2D(Pawn->GetActorLocation()), 0.001));
            }
            if (Frame == Start + 18) Capture(Directions[I]);
            if (Frame == Start + 20) Key(Keys[I], IE_Released);
        }
        if (Frame == 156)
        {
            Test->TestTrue(TEXT("Source walk changes frames in every direction"), SeenWalkSprites.Num() >= 8);
            Capture(TEXT("Market"));
        }
        if (Frame == 158)
        {
            Pawn->SetActorLocation(FVector(850, 0, 0)); Key(EKeys::D, IE_Pressed);
        }
        if (Frame == 196)
        {
            Test->TestTrue(TEXT("Original wall still blocks sweep"), Pawn->GetActorLocation().X <= 882.1 && Pawn->GetActorLocation().X > 850);
            Test->TestFalse(TEXT("Blocked input does not slide animated feet"), Presentation->IsWalking());
            WallX = Pawn->GetActorLocation().X;
            Capture(TEXT("Boundary")); Key(EKeys::D, IE_Released);
        }
        if (Frame == 204) Pawn->SetActorLocation(AMemoriaMaletActor::DevelopmentLocation() + FVector(-65, 0, 0));
        if (Frame == 208)
        {
            Test->TestTrue(TEXT("Original interaction prompt remains reachable"), PC->GetInteractionPrompt().Contains(TEXT("Malet")));
            Capture(TEXT("NearMalet"));
        }
        if (Frame == 214)
        {
            FString After, MemoryAfter;
            FJsonObjectConverter::UStructToJsonObjectString(Run->GetRunSnapshot(), After);
            FJsonObjectConverter::UStructToJsonObjectString(Run->GetPlayerMemory()->GetSnapshot(), MemoryAfter);
            Test->TestEqual(TEXT("Art, walking and lighting do not mutate run"), After, RunBefore);
            Test->TestEqual(TEXT("Memory state untouched by visuals"), MemoryAfter, MemoryBefore);
            Test->TestTrue(TEXT("Narrative trace untouched by visuals"), Host->GetTrace() == TraceBefore);
            const FString Dir = FPaths::ProjectSavedDir()/TEXT("Validation/Presentation2");
            IFileManager::Get().MakeDirectory(*Dir, true);
            FFileHelper::SaveStringToFile(FString::Printf(TEXT("{\"status\":\"%s\",\"distinct_walk_sprites\":%d,\"physical_surfaces\":7,\"wall_x\":%.3f,\"run_unchanged\":%s,\"memory_unchanged\":%s}\n"), Test->HasAnyErrors() ? TEXT("FAIL") : TEXT("PASS"), SeenWalkSprites.Num(), WallX, After == RunBefore ? TEXT("true") : TEXT("false"), MemoryAfter == MemoryBefore ? TEXT("true") : TEXT("false")), *(Dir/TEXT("exploration.json")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
            return true;
        }
        ++Frame; return false;
    }
private:
    FAutomationTestBase* Test;
    double Started, OldDelta = 0, WallX = 0;
    uint64 LastFrame = MAX_uint64;
    int32 Frame = 0;
    bool bFixed = false, bOldFixed = false;
    FString RunBefore, MemoryBefore;
    TArray<FString> TraceBefore;
    TSet<FString> SeenWalkSprites;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVerdanVisualTest, "MemoriaVisual.VerdanExploration", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVerdanVisualTest::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice"))) return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FVerdanVisualReplay(this)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
