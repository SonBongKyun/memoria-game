#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "Tests/AutomationCommon.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Framework/MemoriaSliceHost.h"
#include "Framework/MemoriaFieldPawn.h"
#include "Framework/MemoriaCombatInputRouter.h"
#include "Narrative/MemoriaNarrativeSubsystem.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Domain/MemoriaPlayerMemoryDomain.h"
#include "Combat/MemoriaFieldCombatSubsystem.h"
#include "Combat/MemoriaFieldMonster.h"
#include "Presentation/MemoriaFieldCharacterComponent.h"
#include "Presentation/MemoriaCombatClips.h"
#include "Animation/AnimSequence.h"
#include "InputKeyEventArgs.h"
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "Kismet/GameplayStatics.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
// The running PIE owns the real player, imported clips, HP and input router.
// Manual gameplay-delta steps make boundary cases deterministic; they do not
// mutate combo internals or replace a pose/attack with a test implementation.
class FComboS364Replay final : public IAutomationLatentCommand
{
public:
    explicit FComboS364Replay(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
    ~FComboS364Replay() override
    {
        if (Controller.IsValid())
        {
            Key(EKeys::J, IE_Released); Key(EKeys::Gamepad_RightShoulder, IE_Released);
            if (Controller->GetPauseWidget()) Controller->ClosePause();
        }
        if (Combat.IsValid()) { Combat->CancelHeldInput(); Combat->CloseBurnPicker(); Combat->ResetForRestoredRun(); }
        if (bFixed) { FApp::SetUseFixedTimeStep(bOldFixed); FApp::SetFixedDeltaTime(OldDelta); }
    }
    bool Update() override
    {
        if (LastFrame == GFrameCounter) return false; LastFrame = GFrameCounter; ++Frame;
        if (FPlatformTime::Seconds() - Started > 150.)
        { Test->AddError(FString::Printf(TEXT("S364 combo timeout phase%d"), Phase)); Write(); return true; }
        UWorld* World = GEditor ? GEditor->PlayWorld.Get() : nullptr;
        auto* PC = World ? Cast<AMemoriaSliceController>(World->GetFirstPlayerController()) : nullptr;
        auto* Pawn = PC ? Cast<AMemoriaFieldPawn>(PC->GetPawn()) : nullptr;
        if (!Pawn || World->GetTimeSeconds() < .3) return false;
        auto* Host = World->GetGameInstance()->GetSubsystem<UMemoriaNarrativeSubsystem>();
        if (!bFixed)
        {
            bFixed = true; bOldFixed = FApp::UseFixedTimeStep(); OldDelta = FApp::GetFixedDeltaTime();
            FApp::SetUseFixedTimeStep(true); FApp::SetFixedDeltaTime(1. / 120.);
            for (int32 I = 0; I < 20 && Host->GetState() == EMemoriaSliceState::VN; ++I) Host->Confirm(1);
            return false;
        }
        if (!World->GetMapName().EndsWith(TEXT("L_VerdanHost")) || Host->GetState() != EMemoriaSliceState::Exploration) return false;
        if (!Combat.IsValid())
        {
            Controller = PC; Combat = World->GetSubsystem<UMemoriaFieldCombatSubsystem>();
            Player = Pawn;
            if (!Test->TestTrue(TEXT("S364 uses a registered real field player"), Combat.IsValid() && Combat->GetPlayer() == Pawn)) { Write(); return true; }
            Figure = Combat->GetPlayerFigure();
            if (!Test->TestTrue(TEXT("S364 loads the actual rigged Arrel sword figure"), Figure.IsValid() && Figure->IsRigged() && Figure->HasSword())) { Write(); return true; }
            auto* Memory = World->GetGameInstance()->GetSubsystem<UMemoriaRunSubsystem>()->GetPlayerMemory();
            BeforeBurned = Memory->GetSnapshot().BurnedHistory;
            Pawn->SetActorLocation(FVector(0, -180, 0), false);
            if (!ApiCases()) { Write(); return true; }
            Phase = 1;
        }
        if (Test->HasAnyErrors()) { Write(); return true; }
        if (Phase == 1)
        {
            // A real J tap is synchronous in the existing keyboard route.
            Reset(); Tap(EKeys::J);
            if (!CheckClip(0, TEXT("Keyboard"))) { Write(); return true; }
            auto P = Combat->GetComboInputProbe(); Combat->Tick(P.StepLength * .24f);
            Tap(EKeys::J); P = Combat->GetComboInputProbe();
            Test->TestTrue(TEXT("A fresh physical J tap enters the early slot"), P.EarlySeconds > 0.f && !P.bQueued);
            Combat->Tick(.02f); const float BeforeRepeat = Combat->GetComboInputProbe().EarlySeconds;
            Key(EKeys::J, IE_Repeat);
            Test->TestEqual(TEXT("An OS repeat does not refresh a physical combo press"), Combat->GetComboInputProbe().EarlySeconds, BeforeRepeat);
            Combat->Tick(P.StepLength * .20f);
            Test->TestTrue(TEXT("Physical keyboard early input promotes at the original window"), Combat->GetComboInputProbe().bQueued);
            const auto BeforeModal = Combat->GetComboInputProbe(); const UAnimSequence* BeforeClip = Figure->GetActionClip();
            Tap(EKeys::Escape);
            Test->TestTrue(TEXT("Actual Escape opens Pause"), World->IsPaused() && PC->GetPauseWidget());
            Test->TestTrue(TEXT("Actual modal cancellation preserves the committed clip/step"), Combat->IsAttacking() && Combat->GetComboStep() == BeforeModal.Step && Figure->GetActionClip() == BeforeClip);
            CheckCleared(TEXT("Physical keyboard Pause"));
            Tap(EKeys::Escape);
            Test->TestFalse(TEXT("Actual Escape returns field ownership"), World->IsPaused());
            Combat->Tick(BeforeModal.StepLength + .01f);
            Test->TestFalse(TEXT("Returning from Pause cannot replay the old queued press"), Combat->IsAttacking());
            bKeyboard = true; Record(TEXT("PhysicalKeyboardAndModal"));
            Reset(); Key(EKeys::Gamepad_RightShoulder, IE_Pressed); Mark = Frame; Phase = 2; return false;
        }
        if (Phase == 2)
        {
            if (!Combat->IsAttacking())
            { if (Frame > Mark + 30) { Test->AddError(TEXT("Actual Enhanced Input RB never started a swing")); Write(); return true; } return false; }
            if (!CheckClip(0, TEXT("Gamepad"))) { Write(); return true; }
            Test->TestTrue(TEXT("Physical RB used the actual Enhanced Input router"), PC->GetCombatInputRouter() && PC->GetCombatInputRouter()->IsHoldingAttack() && PC->GetCombatInputRouter()->IsUsingGamepad());
            Key(EKeys::Gamepad_RightShoulder, IE_Released); Mark = Frame; Phase = 3; return false;
        }
        if (Phase == 3 && Frame >= Mark + 2)
        {
            Test->TestFalse(TEXT("Physical RB release clears its held latch"), PC->GetCombatInputRouter()->IsHoldingAttack());
            const auto P = Combat->GetComboInputProbe();
            if (!Test->TestTrue(TEXT("Pad fixture remains before the original queue window"), P.StepTime < P.StepLength * .30f)) { Write(); return true; }
            Combat->Tick(FMath::Max(0.f, P.StepLength * .25f - P.StepTime));
            Key(EKeys::Gamepad_RightShoulder, IE_Pressed); Mark = Frame; Phase = 4; return false;
        }
        if (Phase == 4)
        {
            const auto P = Combat->GetComboInputProbe();
            if (P.EarlySeconds <= 0.f)
            { if (Frame > Mark + 15) { Test->AddError(TEXT("Second fresh physical RB never reached the early slot")); Write(); return true; } return false; }
            Test->TestFalse(TEXT("Pad early press is not prematurely a committed next step"), P.bQueued);
            Key(EKeys::Gamepad_RightShoulder, IE_Released); Mark = Frame; Phase = 5; return false;
        }
        if (Phase == 5 && Frame >= Mark + 2)
        {
            auto P = Combat->GetComboInputProbe();
            Combat->Tick(FMath::Max(0.f, P.StepLength * MemoriaCombatTuning::QueueFrom - P.StepTime) + .001f);
            Test->TestTrue(TEXT("Physical RB early press promotes at the unchanged queue window"), Combat->GetComboInputProbe().bQueued);
            P = Combat->GetComboInputProbe(); Combat->Tick(P.StepLength - P.StepTime + .001f);
            CheckClip(1, TEXT("Physical pad chained step"));
            P = Combat->GetComboInputProbe();
            Test->TestTrue(TEXT("The one physical pad slot was consumed once"), P.EarlySeconds == 0.f && !P.bQueued);
            Combat->Tick(P.StepLength + .01f);
            Test->TestFalse(TEXT("No held auto-repeat manufactures the third combo step"), Combat->IsAttacking());
            bGamepad = true; Record(TEXT("PhysicalGamepadChain"));
            auto* Memory = World->GetGameInstance()->GetSubsystem<UMemoriaRunSubsystem>()->GetPlayerMemory();
            Test->TestTrue(TEXT("Every combo/preview/cancellation case preserves irreversible memory history"), Memory->GetSnapshot().BurnedHistory == BeforeBurned);
            Write(); return true;
        }
        return false;
    }
private:
    void Key(FKey K, EInputEvent E)
    { if (Controller.IsValid()) Controller->InputKey(FInputKeyEventArgs::CreateSimulated(K, E, E == IE_Released ? 0.f : 1.f)); }
    void Tap(FKey K) { Key(K, IE_Pressed); Key(K, IE_Released); }
    void Reset()
    { Combat->ResetForRestoredRun(); Combat->Revive(1.f); }
    bool CheckClip(int32 Step, const TCHAR* Case)
    {
        const UAnimSequence* Expected = MemoriaCombatClips::Load(Figure->GetCharacterId(),
            MemoriaCombatClips::ForAction(Figure->GetCharacterId(), MemoriaCombatClips::Attack(Step)));
        const auto P = Combat->GetComboInputProbe();
        return Test->TestTrue(*FString::Printf(TEXT("%s uses the actual imported combo clip and original rate"), Case),
            Expected && Figure->GetActionClip() == Expected && P.Step == Step &&
            FMath::IsNearlyEqual(P.StepLength, float(Expected->GetPlayLength()) / MemoriaCombatTuning::SwordComboRate[Step], .0001f));
    }
    bool CheckCleared(const TCHAR* Case)
    {
        const auto P = Combat->GetComboInputProbe();
        return Test->TestTrue(*FString::Printf(TEXT("%s clears all future combo intent"), Case), !P.bQueued && P.EarlySeconds == 0.f);
    }
    bool Prime(bool bEarly)
    {
        Reset();
        if (!Combat->RequestAttack(Player->GetActorLocation() + FVector(1000, 0, 0)) || !CheckClip(0, TEXT("Cancellation fixture"))) return false;
        const auto P = Combat->GetComboInputProbe(); Combat->Tick(P.StepLength * (bEarly ? .20f : .42f));
        Combat->RequestAttack(Player->GetActorLocation() + FVector(-1000, 0, 0));
        const auto Queued = Combat->GetComboInputProbe();
        return Test->TestTrue(TEXT("Cancellation starts with actual future input"), bEarly ? Queued.EarlySeconds > 0.f && !Queued.bQueued : Queued.bQueued && Queued.EarlySeconds == 0.f);
    }
    bool ApiCases()
    {
        const FVector Me = Player->GetActorLocation();
        Reset();
        auto Front = Combat->SpawnWave(1, Me + FVector(130, 0, 0), 0.f);
        auto Back = Combat->SpawnWave(1, Me - FVector(130, 0, 0), 0.f);
        if (!Test->TestTrue(TEXT("Actual front and rear HP targets spawned"), Front.Num() == 1 && Back.Num() == 1)) return false;
        Front[0]->SetActorTickEnabled(false); Back[0]->SetActorTickEnabled(false);
        const float FrontHp = Front[0]->GetHealth(), BackHp = Back[0]->GetHealth();
        Combat->RequestAttack(Me + FVector(1000, 0, 0)); if (!CheckClip(0, TEXT("Committed front swing"))) return false;
        const float FirstLength = Combat->GetComboInputProbe().StepLength;
        Combat->Tick(FirstLength * .41f);
        Combat->RequestAttack(Me + FVector(-1000, 0, 0));
        Test->TestTrue(TEXT("Queued reverse aim does not rotate the committed visible figure"), FMath::Abs(FMath::FindDeltaAngleDegrees(Figure->GetYaw(), 0.f)) < .01f);
        Combat->Tick(FirstLength * .04f);
        Test->TestEqual(TEXT("Current front swing hits front once for unchanged12 damage"), FrontHp - Front[0]->GetHealth(), MemoriaCombatTuning::ComboDamage[0], .01f);
        Test->TestEqual(TEXT("Queued rear aim cannot redirect current front damage arc"), Back[0]->GetHealth(), BackHp, .01f);
        Combat->Tick(FirstLength * .56f);
        if (!CheckClip(1, TEXT("Next rear swing"))) return false;
        Test->TestTrue(TEXT("Next step turns the figure toward the queued rear aim"), FMath::Abs(FMath::FindDeltaAngleDegrees(Figure->GetYaw(), 180.f)) < .01f);
        Combat->Tick(Combat->GetComboInputProbe().StepLength * .35f);
        Test->TestEqual(TEXT("Next rear step hits rear for unchanged14 damage"), BackHp - Back[0]->GetHealth(), MemoriaCombatTuning::ComboDamage[1], .01f);
        Test->TestEqual(TEXT("Rear step cannot retroactively damage the front target"), FrontHp - Front[0]->GetHealth(), MemoriaCombatTuning::ComboDamage[0], .01f);
        Report->SetNumberField(TEXT("front_actual_loss"), FrontHp - Front[0]->GetHealth());
        Report->SetNumberField(TEXT("rear_actual_loss"), BackHp - Back[0]->GetHealth());
        Record(TEXT("OppositeTargetsCommittedAim"));
        // The earliest full press expires; a later press within120ms of the
        // original window is promoted. A later replacement owns only one slot.
        Reset(); Combat->RequestAttack(Me + FVector(1000, 0, 0));
        Combat->RequestAttack(Me + FVector(-1000, 0, 0));
        Combat->Tick(UMemoriaFieldCombatSubsystem::EarlyComboSeconds + .005f);
        CheckCleared(TEXT("Too-early press expiry"));
        Combat->Tick(Combat->GetComboInputProbe().StepLength + .01f);
        Test->TestFalse(TEXT("Expired input never starts a future combo"), Combat->IsAttacking()); Record(TEXT("EarlyExpiry"));
        Reset(); Combat->RequestAttack(Me + FVector(1000, 0, 0));
        float Length = Combat->GetComboInputProbe().StepLength; Combat->Tick(Length * .30f);
        Combat->RequestAttack(Me + FVector(-1000, 0, 0));
        Combat->RequestAttack(Me + FVector(0, 1000, 0));
        Test->TestTrue(TEXT("Latest early press stays pending before the window"), Combat->GetComboInputProbe().EarlySeconds > 0.f && !Combat->GetComboInputProbe().bQueued);
        Combat->Tick(Length * .11f);
        Test->TestTrue(TEXT("Valid early press promotes at QueueFrom without starting early"), Combat->GetComboInputProbe().bQueued && Combat->GetComboStep() == 0);
        Combat->Tick(Length * .60f);
        Test->TestTrue(TEXT("The latest early aim wins exactly one slot"), Combat->GetComboStep() == 1 && FMath::Abs(FMath::FindDeltaAngleDegrees(Figure->GetYaw(), 90.f)) < .01f);
        Combat->Tick(Combat->GetComboInputProbe().StepLength + .01f);
        Test->TestFalse(TEXT("One early press cannot create a third step"), Combat->IsAttacking()); Record(TEXT("LatestEarlyPressSingleConsumption"));
        // A crossing in a long frame is accepted only when the window was
        // reached before the120ms expiry, even if that frame ends later.
        for (const bool bWithin : {false, true})
        {
            Reset(); Combat->RequestAttack(Me + FVector(1000, 0, 0)); Length = Combat->GetComboInputProbe().StepLength;
            const float Lead = UMemoriaFieldCombatSubsystem::EarlyComboSeconds + (bWithin ? -.005f : .005f);
            Combat->Tick(Length * MemoriaCombatTuning::QueueFrom - Lead);
            Combat->RequestAttack(Me + FVector(-1000, 0, 0));
            Combat->Tick(Lead + .02f);
            Test->TestEqual(TEXT("A crossing frame respects actual120ms gameplay-time expiry"), Combat->GetComboInputProbe().bQueued, bWithin);
            Record(bWithin ? TEXT("LongFrameValid") : TEXT("LongFrameExpired"));
        }
        const TCHAR* Names[] = {TEXT("Modal"), TEXT("Guard"), TEXT("Dash"), TEXT("Hit"), TEXT("Death"), TEXT("BurnPicker"), TEXT("Register"), TEXT("Restore")};
        for (const bool bEarly : {true, false}) for (int32 I = 0; I < UE_ARRAY_COUNT(Names); ++I)
        {
            if (!Prime(bEarly)) return false;
            const auto Before = Combat->GetComboInputProbe(); const UAnimSequence* Clip = Figure->GetActionClip();
            switch (I)
            {
            case 0: Combat->CancelHeldInput(); Test->TestTrue(TEXT("Modal preserves the already committed swing/clip"), Combat->IsAttacking() && Combat->GetComboStep() == Before.Step && Figure->GetActionClip() == Clip); break;
            case 1: Test->TestTrue(TEXT("Actual guard cancels a swing"), Combat->BeginBlock()); break;
            case 2: Test->TestTrue(TEXT("Actual dash cancels a swing"), Combat->RequestDash(FVector(0, 1, 0))); break;
            case 3: case 4:
            {
                auto Foes = Combat->SpawnWave(1, Player->GetActorLocation() + FVector(120, 0, 0), 0.f);
                if (!Test->TestEqual(TEXT("Actual striking foe spawned"), Foes.Num(), 1)) return false;
                Foes[0]->SetActorTickEnabled(false);
                Test->TestTrue(TEXT("An actual unguarded strike cancels future input"), Combat->StrikePlayer(Foes[0], I == 4 ? 99999.f : 10.f));
                Test->TestEqual(TEXT("Actual lethal case reaches defeat"), Combat->IsDefeated(), I == 4); break;
            }
            case 5: Test->TestTrue(TEXT("Authored burn picker opens without consuming a memory"), Combat->OpenBurnPicker()); Test->TestFalse(TEXT("The semantic API cannot start an attack inside the picker"), Combat->RequestAttack(Me)); Combat->CloseBurnPicker(); break;
            case 6: Combat->RegisterPlayer(Player.Get(), Figure.Get()); break;
            case 7: Combat->ResetForRestoredRun(); break;
            }
            CheckCleared(Names[I]);
            // A cleared future gesture cannot reappear merely because time advances.
            Combat->Tick(Before.StepLength + .05f);
            Test->TestFalse(TEXT("Cancellation cannot replay an abandoned combo step"), Combat->IsAttacking());
            Record(*FString::Printf(TEXT("%s_%s"), Names[I], bEarly ? TEXT("Early") : TEXT("Queued")));
        }
        return !Test->HasAnyErrors();
    }
    void Record(const TCHAR* Name)
    {
        auto Row = MakeShared<FJsonObject>(); const auto P = Combat->GetComboInputProbe();
        Row->SetStringField(TEXT("case"), Name); Row->SetNumberField(TEXT("step"), P.Step);
        Row->SetNumberField(TEXT("step_time"), P.StepTime); Row->SetNumberField(TEXT("step_length"), P.StepLength);
        Row->SetNumberField(TEXT("early_seconds"), P.EarlySeconds); Row->SetBoolField(TEXT("queued"), P.bQueued);
        Row->SetStringField(TEXT("future_aim"), P.Aim.ToString()); Row->SetStringField(TEXT("committed_aim"), P.SwingAim.ToString());
        Row->SetNumberField(TEXT("figure_yaw"), Figure->GetYaw()); Rows.Add(MakeShared<FJsonValueObject>(Row));
    }
    void Write()
    {
        if (bWritten) return; bWritten = true;
        Report->SetStringField(TEXT("status"), Test->HasAnyErrors() ? TEXT("FAIL") : TEXT("PASS"));
        Report->SetStringField(TEXT("scope"), TEXT("Real PIE player/imported clips/actual two-foe HP; manually advanced gameplay-delta boundary cases plus real keyboard and Enhanced Input gamepad gestures. No physical hardware or rendered contact certification."));
        Report->SetBoolField(TEXT("physical_keyboard_path"), bKeyboard); Report->SetBoolField(TEXT("enhanced_gamepad_path"), bGamepad);
        Report->SetNumberField(TEXT("early_window_seconds"), UMemoriaFieldCombatSubsystem::EarlyComboSeconds);
        Report->SetArrayField(TEXT("cases"), Rows);
        FString Json; const auto Writer = TJsonWriterFactory<>::Create(&Json); FJsonSerializer::Serialize(Report, Writer);
        const FString Folder = FPaths::ProjectSavedDir() / TEXT("Validation/S364Combo") / FApp::GetInstanceId().ToString(EGuidFormats::Digits);
        IFileManager::Get().MakeDirectory(*Folder, true);
        Test->TestTrue(TEXT("Native S364 combat evidence saved"), FFileHelper::SaveStringToFile(Json, *(Folder / TEXT("combo.json"))));
    }
    FAutomationTestBase* Test;
    double Started, OldDelta = 0.; uint64 LastFrame = MAX_uint64;
    int32 Phase = 0, Frame = 0, Mark = 0;
    bool bFixed = false, bOldFixed = false, bKeyboard = false, bGamepad = false, bWritten = false;
    TWeakObjectPtr<AMemoriaSliceController> Controller;
    TWeakObjectPtr<AMemoriaFieldPawn> Player;
    TWeakObjectPtr<UMemoriaFieldCombatSubsystem> Combat;
    TWeakObjectPtr<UMemoriaFieldCharacterComponent> Figure;
    TArray<FString> BeforeBurned;
    TSharedRef<FJsonObject> Report = MakeShared<FJsonObject>();
    TArray<TSharedPtr<FJsonValue>> Rows;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FComboS364, "MemoriaUpgrade.S364Combo.CommittedAimAndBufferedInput", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FComboS364::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice"))) return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FComboS364Replay(this)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand()); return true;
}
#endif
