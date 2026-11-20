#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "Tests/AutomationCommon.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "Framework/MemoriaSliceHost.h"
#include "Framework/MemoriaFieldPawn.h"
#include "Narrative/MemoriaNarrativeSubsystem.h"
#include "Combat/MemoriaFieldCombatSubsystem.h"
#include "Combat/MemoriaFieldCombatTypes.h"
#include "Combat/MemoriaBurnArtS368.h"
#include "Interaction/MemoriaEliaCompanion.h"
#include "Materials/Material.h"
#include "EngineUtils.h"
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#if WITH_DEV_AUTOMATION_TESTS
namespace
{
// S368, with real keys in the Verdan slice: R, arrows, Enter. Burning the first sword raises its illustration
// above Arrel; as the ring is released it burns from the bottom and is gone, and the ground keeps soot and a
// cooling glow. A memory without art burns as a smoke of its grade's colour. The marks fade in ten seconds.
class FBurnRelicS368Replay final : public IAutomationLatentCommand
{
public:
    explicit FBurnRelicS368Replay(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
    ~FBurnRelicS368Replay() override { if (bFixed) { FApp::SetUseFixedTimeStep(bOldFixed); FApp::SetFixedDeltaTime(OldDelta); } }
    bool Update() override
    {
        using namespace MemoriaCombatTuning;
        if (LastFrame == GFrameCounter) return false;
        LastFrame = GFrameCounter;
        if (FPlatformTime::Seconds() - Started > 200) { Test->AddError(FString::Printf(TEXT("Burn relic timeout in phase %d"), Phase)); return true; }
        UWorld* World = GEditor->PlayWorld;
        auto* PC = World ? Cast<AMemoriaSliceController>(World->GetFirstPlayerController()) : nullptr;
        auto* Pawn = PC ? Cast<AMemoriaFieldPawn>(PC->GetPawn()) : nullptr;
        if (!Pawn || World->GetTimeSeconds() < .3) return false;
        auto* Host = World->GetGameInstance()->GetSubsystem<UMemoriaNarrativeSubsystem>();
        if (!bFixed)
        {
            bFixed = true; bOldFixed = FApp::UseFixedTimeStep(); OldDelta = FApp::GetFixedDeltaTime();
            FApp::SetUseFixedTimeStep(true); FApp::SetFixedDeltaTime(1.0 / 60.0);
            for (int32 I = 0; I < 20 && Host->GetState() == EMemoriaSliceState::VN; ++I) Host->Confirm(1);
            return false;
        }
        if (!World->GetMapName().EndsWith(TEXT("L_VerdanHost"))) return false;
        ++Frame;
        auto* Combat = World->GetSubsystem<UMemoriaFieldCombatSubsystem>();
        if (!Combat || !Combat->GetPlayer() || Host->GetState() != EMemoriaSliceState::Exploration) return false;
        auto Capture = [&](const TCHAR* Name) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Validation/S368BurnRelic") / (FString(Name) + TEXT(".png")), true, false); };
        auto Press = [&](const FKey& Key)
        {
            PC->InputKey(FInputKeyEventArgs::CreateSimulated(Key, IE_Pressed, 1.f));
            PC->InputKey(FInputKeyEventArgs::CreateSimulated(Key, IE_Released, 0.f));
        };
        // R, arrows to the chosen memory, Enter (twice when the memory asks twice).
        auto BurnWithKeys = [&](auto&& Pick)
        {
            Press(EKeys::R);
            const int32 Index = Combat->GetBurnChoices().IndexOfByPredicate(Pick);
            if (Index < 0) return false;
            for (int32 I = 0; I < Combat->GetBurnChoices().Num() && Combat->GetBurnSelection() != Index; ++I) Press(EKeys::Down);
            for (int32 I = 0; I < 2 && Combat->IsPickingBurn(); ++I) Press(EKeys::Enter);
            return !Combat->IsPickingBurn();
        };
        if (Phase == 0)
        {
            // The session's first screenshot can come from the editor's viewport: take a throwaway one first.
            if (Frame == 10) Capture(TEXT("Warmup"));
            if (Frame < 20) return false;
            for (const auto& Entry : MemoriaBurnArt::Entries)
                Test->TestNotNull(*FString::Printf(TEXT("Burn art %s is imported"), Entry.Name), LoadObject<UTexture2D>(nullptr, *MemoriaBurnArt::ObjectPath(Entry.Name)));
            Test->TestNotNull(TEXT("The card material exists"), LoadObject<UMaterial>(nullptr, TEXT("/Game/Memoria/Presentation/Combat/M_MemoryCard.M_MemoryCard")));
            Test->TestEqual(TEXT("The source's keyword fallback"), FString(MemoriaBurnArt::Resolve(TEXT("ch5_old_compass"), TEXT("The Compass"))), FString(TEXT("compass")));
            Test->TestTrue(TEXT("A memory without art has none"), MemoriaBurnArt::Resolve(TEXT("sense_warm_light"), TEXT("Warm Light")) == nullptr);
            for (TActorIterator<AMemoriaEliaCompanion> It(World); It; ++It) It->SetActorHiddenInGame(true);
            const bool bBurned = BurnWithKeys([](const FMemoriaBurnChoice& C) { return C.Id == TEXT("identity_first_sword"); });
            Test->TestTrue(TEXT("The first sword burns"), bBurned && Combat->GetBurns() == 1);
            Test->TestTrue(TEXT("Its illustration rises above Arrel"), Combat->HasMemoryCard() && Combat->GetMemoryCardArt() == TEXT("first_sword"));
            Test->TestEqual(TEXT("Whole while the fire gathers"), Combat->GetMemoryCardBurn(), 0.f);
            Phase = 1; Mark = Frame; return false;
        }
        if (Phase == 1)
        {
            if (Frame == Mark + 22) Capture(TEXT("BurnCardSeen"));   // a few frames after the picker closes
            if (!bReleased && Combat->GetBurnWave().bLive)
            {
                bReleased = true;
                Test->TestEqual(TEXT("The release marks the ground"), Combat->GetScorchCount(), 1);
                Test->TestTrue(TEXT("With dark soot"), Combat->GetNewestScorchAlpha() > .5f);
            }
            if (bReleased && !bBurningShot && Combat->GetMemoryCardBurn() >= .3f) { bBurningShot = true; Capture(TEXT("BurnCardBurning")); }
            if (bReleased && !bGone && !Combat->HasMemoryCard())
            {
                bGone = true; GoneFrame = Frame;
                Test->TestTrue(TEXT("The card burned through before it vanished"), bBurningShot);
            }
            if (bGone && Frame == GoneFrame + 50)
            {
                Capture(TEXT("BurnScorch"));
                Test->TestEqual(TEXT("The scorch remains after the card"), Combat->GetScorchCount(), 1);
            }
            if (bGone && Frame == GoneFrame + 90 && !Combat->IsCasting())
            {
                // A memory with no illustration: the picture-less smoke.
                const bool bBurned = BurnWithKeys([](const FMemoriaBurnChoice& C) { return MemoriaBurnArt::Resolve(C.Id, C.Title) == nullptr; });
                Test->TestTrue(TEXT("A memory without art burns"), bBurned && Combat->GetBurns() == 2);
                Test->TestTrue(TEXT("As a smoke without a picture"), Combat->HasMemoryCard() && Combat->GetMemoryCardArt().IsEmpty());
                Phase = 2; Mark = Frame;
            }
            if (Frame > Mark + 900) { Test->AddError(TEXT("The first burn never finished")); return true; }
            return false;
        }
        if (Phase == 2)
        {
            if (Frame == Mark + 14) Capture(TEXT("BurnCardSmoke"));
            if (Frame == Mark + 120) Test->TestEqual(TEXT("Two burns, two marks"), Combat->GetScorchCount(), 2);
            // Ten world seconds after the second release, the ground is clean again.
            if (Frame > Mark + 120 && Combat->GetScorchCount() == 0)
            {
                Test->TestTrue(TEXT("The marks fade in about ten seconds"), Frame - Mark <= int32((ScorchLife + 1.5f) * 60.f));
                return true;
            }
            if (Frame > Mark + 1200) { Test->AddError(TEXT("The scorches never faded")); return true; }
            return false;
        }
        return false;
    }
private:
    FAutomationTestBase* Test;
    double Started, OldDelta = 0;
    uint64 LastFrame = MAX_uint64;
    int32 Phase = 0, Frame = 0, Mark = 0, GoneFrame = 0;
    bool bFixed = false, bOldFixed = false, bReleased = false, bBurningShot = false, bGone = false;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBurnRelicS368Test, "MemoriaVisual.S368BurnRelic", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBurnRelicS368Test::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice"))) return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FBurnRelicS368Replay(this)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
