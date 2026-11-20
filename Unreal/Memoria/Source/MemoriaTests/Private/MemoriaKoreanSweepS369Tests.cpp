#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "Tests/AutomationCommon.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Framework/MemoriaSliceHost.h"
#include "Framework/MemoriaFieldPawn.h"
#include "Narrative/MemoriaNarrativeSubsystem.h"
#include "Combat/MemoriaFieldCombatSubsystem.h"
#include "Combat/MemoriaFieldCombatTypes.h"
#include "Combat/MemoriaFieldMonster.h"
#include "Presentation/MemoriaPauseWidget.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Settings/MemoriaSettingsSubsystem.h"
#include "Interaction/MemoriaEliaCompanion.h"
#include "EngineUtils.h"
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#if WITH_DEV_AUTOMATION_TESTS
namespace
{
// S369 Korean sweep: the Verdan slice in Korean, every screen a player reaches captured once, so the Korean
// patch can be checked by eye (and later kept from regressing). Settings and the run are both Korean.
class FKoreanSweepReplay final : public IAutomationLatentCommand
{
public:
    explicit FKoreanSweepReplay(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
    ~FKoreanSweepReplay() override { if (bFixed) { FApp::SetUseFixedTimeStep(bOldFixed); FApp::SetFixedDeltaTime(OldDelta); } }
    bool Update() override
    {
        if (LastFrame == GFrameCounter) return false;
        LastFrame = GFrameCounter;
        if (FPlatformTime::Seconds() - Started > 400) { Test->AddError(FString::Printf(TEXT("Korean sweep timeout at step %d"), Step)); return true; }
        UWorld* World = GEditor->PlayWorld;
        auto* PC = World ? Cast<AMemoriaSliceController>(World->GetFirstPlayerController()) : nullptr;
        auto* Pawn = PC ? Cast<AMemoriaFieldPawn>(PC->GetPawn()) : nullptr;
        if (!Pawn || World->GetTimeSeconds() < .3) return false;
        auto* Host = World->GetGameInstance()->GetSubsystem<UMemoriaNarrativeSubsystem>();
        auto* Run = World->GetGameInstance()->GetSubsystem<UMemoriaRunSubsystem>();
        auto* Settings = World->GetGameInstance()->GetSubsystem<UMemoriaSettingsSubsystem>();
        if (!bFixed)
        {
            bFixed = true; bOldFixed = FApp::UseFixedTimeStep(); OldDelta = FApp::GetFixedDeltaTime();
            FApp::SetUseFixedTimeStep(true); FApp::SetFixedDeltaTime(1.0 / 60.0);
            if (Settings) Settings->SetLocale(TEXT("ko"));
            if (Run) Run->SetLocale(TEXT("ko"));
            return false;
        }
        // The arrival scene first, in Korean: two of its pages are captured on the way.
        if (Host->GetState() == EMemoriaSliceState::VN)
        {
            ++VnFrames;
            if (VnFrames == 30) Capture(TEXT("KoVnArrival"));
            if (VnFrames % 20 == 0) Host->Confirm(1);
            return false;
        }
        if (!World->GetMapName().EndsWith(TEXT("L_VerdanHost"))) return false;
        auto* Combat = World->GetSubsystem<UMemoriaFieldCombatSubsystem>();
        if (!Combat || !Combat->GetPlayer()) return false;
        ++Frame;
        auto Key = [&](const FKey& K) { PC->InputKey(FInputKeyEventArgs::CreateSimulated(K, IE_Pressed, 1.f)); PC->InputKey(FInputKeyEventArgs::CreateSimulated(K, IE_Released, 0.f)); };
        auto Pause = [&](int32 Row) { if (auto* Menu = PC->GetPauseWidget()) { Menu->Select(Row); Key(EKeys::Enter); } };
        // One action every 16 frames; a capture 12 frames after it.
        if (Frame < 30 || (Frame - 30) % 16 != 0) return false;
        switch (Step++)
        {
        case 0: Capture(TEXT("KoHud")); break;
        case 1: Key(EKeys::Escape); break;
        case 2: Capture(TEXT("KoPause")); Pause(1); break;
        case 3: Capture(TEXT("KoOptions")); Key(EKeys::Escape); break;
        case 5: Pause(4); break;
        case 7: Capture(TEXT("KoJournal")); Key(EKeys::Right); break;
        case 9: Capture(TEXT("KoJournal2")); Key(EKeys::Right); break;
        case 11: Capture(TEXT("KoJournal3")); Key(EKeys::Right); break;
        case 13: Capture(TEXT("KoJournal4")); Key(EKeys::Right); break;
        case 15: Capture(TEXT("KoJournal5")); Key(EKeys::Right); break;
        case 17: Capture(TEXT("KoJournal6")); Key(EKeys::Escape); break;
        case 19: Pause(5); break;
        case 21: Capture(TEXT("KoCodex")); Key(EKeys::Right); break;
        case 23: Capture(TEXT("KoCodex2")); Key(EKeys::Escape); break;
        case 25: Pause(6); break;
        case 27: Capture(TEXT("KoAchievements")); Key(EKeys::Escape); break;
        case 29: Pause(8); break;
        case 31: Capture(TEXT("KoQuitAsk")); Key(EKeys::Escape); break;
        case 33: Key(EKeys::Escape); break;
        case 35: Key(EKeys::Escape); break;
        case 37: Key(EKeys::Tab); break;
        case 39: Capture(TEXT("KoArchive")); Key(EKeys::Escape); break;
        case 41:
            for (TActorIterator<AMemoriaEliaCompanion> It(World); It; ++It) It->SetActorHiddenInGame(true);
            Combat->SpawnWave(1, Pawn->GetActorLocation() + FVector(260, 0, 0), 0.f, EMemoriaFoeKind::MarketThief, 1);
            break;
        case 42: Capture(TEXT("KoElite")); break;
        case 43: Key(EKeys::R); break;
        case 45: Capture(TEXT("KoBurnPicker")); Key(EKeys::Escape); break;
        case 47:
            for (const auto& M : Combat->GetMonsters()) if (M.IsValid() && !M->IsDead()) M->TakeHit(9999.f, Pawn->GetActorLocation(), 0.f, true);
            break;
        case 48: Capture(TEXT("KoReward")); break;
        case 50:
            // Defeat: the game over screen.
            if (auto* Husk = Combat->SpawnWave(1, Pawn->GetActorLocation() + FVector(120, 0, 0), 0.f, EMemoriaFoeKind::VoidHusk)[0]) Combat->StrikePlayer(Husk, 9999.f, true);
            break;
        default:
            if (Step > 52 && (Combat->IsAwaitingGameOver() || Step > 80))
            {
                if (!bOverShot) { bOverShot = true; Capture(TEXT("KoGameOver")); return false; }
                return true;
            }
            break;
        }
        return false;
    }
private:
    void Capture(const TCHAR* Name) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Validation/S369Korean") / (FString(Name) + TEXT(".png")), true, false); }
    FAutomationTestBase* Test;
    double Started, OldDelta = 0;
    uint64 LastFrame = MAX_uint64;
    int32 Frame = 0, Step = 0, VnFrames = 0;
    bool bFixed = false, bOldFixed = false, bOverShot = false;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKoreanSweepS369Test, "MemoriaVisual.S369KoreanSweep", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FKoreanSweepS369Test::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice"))) return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FKoreanSweepReplay(this)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
