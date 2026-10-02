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
#include "Combat/MemoriaFieldCombatSubsystem.h"
#include "Combat/MemoriaFieldCombatTypes.h"
#include "Combat/MemoriaFieldHudWidget.h"
#include "Combat/MemoriaFieldMonster.h"
#include "Interaction/MemoriaEliaCompanion.h"
#include "EngineUtils.h"
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#if WITH_DEV_AUTOMATION_TESTS
namespace
{
// S341: the items in the field, by GameManager.ITEMS. An item that would do nothing is kept; the potion heals 40
// and the hi-potion (taken for a deep wound) 80; the antidote ends the poison; the witness ink turns one blow
// aside; the firebomb does 12 where it bursts and burns for 15 twice; the smoke bomb ends the fight unpaid.
class FFieldItemsReplay final : public IAutomationLatentCommand
{
public:
    explicit FFieldItemsReplay(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
    ~FFieldItemsReplay() override { if (bFixed) { FApp::SetUseFixedTimeStep(bOldFixed); FApp::SetFixedDeltaTime(OldDelta); } }
    bool Update() override
    {
        if (LastFrame == GFrameCounter) return false;
        LastFrame = GFrameCounter;
        if (FPlatformTime::Seconds() - Started > 180) { Test->AddError(FString::Printf(TEXT("Field items timeout in phase %d"), Phase)); return true; }
        UWorld* World = GEditor->PlayWorld;
        auto* PC = World ? Cast<AMemoriaSliceController>(World->GetFirstPlayerController()) : nullptr;
        auto* Pawn = PC ? Cast<AMemoriaFieldPawn>(PC->GetPawn()) : nullptr;
        if (!Pawn || World->GetTimeSeconds() < .3) return false;
        auto* Host = World->GetGameInstance()->GetSubsystem<UMemoriaNarrativeSubsystem>();
        auto* Run = World->GetGameInstance()->GetSubsystem<UMemoriaRunSubsystem>();
        if (!bFixed)
        {
            bFixed = true; bOldFixed = FApp::UseFixedTimeStep(); OldDelta = FApp::GetFixedDeltaTime();
            FApp::SetUseFixedTimeStep(true); FApp::SetFixedDeltaTime(1.0 / 60.0);
            for (int32 I = 0; I < 20 && Host->GetState() == EMemoriaSliceState::VN; ++I) Host->Confirm(1);
            return false;
        }
        if (!World->GetMapName().EndsWith(TEXT("L_VerdanHost")) || World->GetTimeSeconds() == LastWorldTime) return false;
        LastWorldTime = World->GetTimeSeconds(); ++Frame;
        auto* Combat = World->GetSubsystem<UMemoriaFieldCombatSubsystem>();
        if (!Combat || !Combat->GetPlayer() || !Run || Host->GetState() != EMemoriaSliceState::Exploration) return false;
        auto Capture = [&](const TCHAR* Name) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Validation/FieldItems") / (FString(Name) + TEXT(".png")), true, false); };
        auto Used = [&](const TCHAR* Id) { return Host->GetTrace().Contains(FString(TEXT("item:used:")) + Id); };
        const FVector Me = Pawn->GetActorLocation();
        AMemoriaFieldMonster* H = Husk.Get();
        const int64 Max = Combat->GetPlayerMaxHp();
        switch (Phase)
        {
        case 0:
        {
            for (TActorIterator<AMemoriaEliaCompanion> It(World); It; ++It) It->SetActorHiddenInGame(true);
            // The source's numbers.
            using namespace MemoriaCombatTuning;
            Test->TestTrue(TEXT("GameManager.ITEMS: potion 40, hi-potion 80"), FindFieldItem(TEXT("potion"))->Power == 40 && FindFieldItem(TEXT("hi_potion"))->Power == 80);
            Test->TestTrue(TEXT("Antidote restores 12; firebomb 12 then 15 a turn"), FindFieldItem(TEXT("antidote"))->Extra == 12 && FindFieldItem(TEXT("firebomb"))->Extra == 12 && FindFieldItem(TEXT("firebomb"))->Power == 15 && BombBurnTicks == 2);
            // A known kit, whatever the route gave.
            for (const TCHAR* Id : {TEXT("potion"), TEXT("hi_potion"), TEXT("antidote"), TEXT("firebomb"), TEXT("smoke_bomb"), TEXT("witness_ink")}) while (Run->ConsumeItem(Id)) {}
            Run->GrantFieldItem(TEXT("potion"), 2); Run->GrantFieldItem(TEXT("hi_potion"), 1); Run->GrantFieldItem(TEXT("antidote"), 1);
            Run->GrantFieldItem(TEXT("firebomb"), 1); Run->GrantFieldItem(TEXT("smoke_bomb"), 1); Run->GrantFieldItem(TEXT("witness_ink"), 1);
            Test->TestEqual(TEXT("Slot one counts both potions"), Combat->QuickItemCount(0), int64(3));
            Test->TestTrue(TEXT("The slots hold their items"), Combat->QuickItemId(1) == TEXT("antidote") && Combat->QuickItemId(2) == TEXT("firebomb") && Combat->QuickItemId(3) == TEXT("smoke_bomb") && Combat->QuickItemId(4) == TEXT("witness_ink"));
            Test->TestTrue(TEXT("The tray has its six icons"), PC->GetFieldHud() && PC->GetFieldHud()->GetIconCount() == 6);
            // Unhurt, a potion is kept; so is an antidote with nothing to cure, and a smoke bomb with no one to flee.
            Test->TestEqual(TEXT("Arrel is unhurt"), Combat->GetPlayerHp(), Max);
            Test->TestFalse(TEXT("A potion at full HP is refused"), Combat->UseQuickItem(0, Me));
            Test->TestFalse(TEXT("An antidote with nothing to cure is refused"), Combat->UseQuickItem(1, Me));
            Test->TestFalse(TEXT("A smoke bomb with no foe is refused"), Combat->UseQuickItem(3, Me));
            Test->TestTrue(TEXT("Nothing was spent, and the refusal says why"), Combat->GetItemsUsed() == 0 && Run->GetItemCount(TEXT("potion")) == 2 && Run->GetItemCount(TEXT("antidote")) == 1 && Run->GetItemCount(TEXT("smoke_bomb")) == 1 && !Combat->GetItemRefusal().IsEmpty());
            Husk = Combat->SpawnWave(1, Me + FVector(-90, 30, 0), 0.f, EMemoriaFoeKind::VoidHusk)[0];
            Phase = 1; Mark = Frame; break;
        }
        case 1:
            if (Frame < Mark + 3 || !H) break;
            if (Frame == Mark + 3) H->Stun(120.f);   // it stands where it is for the whole test
            if (Frame == Mark + 10) Capture(TEXT("ItemsTray"));
            if (Frame < Mark + 20) break;
            // A light wound: the potion, and only what is missing.
            Test->TestTrue(TEXT("The husk's blow lands"), Combat->StrikePlayer(H, 30.f));
            Test->TestTrue(TEXT("Arrel is hurt and poisoned"), Combat->GetPlayerHp() == Max - 30 && Combat->IsPoisoned());
            Test->TestEqual(TEXT("A light wound takes the potion"), Combat->QuickItemId(0), FString(TEXT("potion")));
            Test->TestTrue(TEXT("The potion is used"), Combat->UseQuickItem(0, Me));
            Test->TestTrue(TEXT("It restores what was missing, and one is spent"), Combat->GetPlayerHp() == Max && Run->GetItemCount(TEXT("potion")) == 1 && Used(TEXT("potion")));
            Test->TestFalse(TEXT("A second item must wait its moment"), Combat->UseQuickItem(1, Me));
            Test->TestTrue(TEXT("Nothing spent in the wait"), Run->GetItemCount(TEXT("antidote")) == 1 && Combat->GetItemCooldown() > 0.f);
            Phase = 2; Mark = Frame; break;
        case 2:
            if (Frame == Mark + 4) Capture(TEXT("ItemsHeal"));
            if (Frame < Mark + 55) break;
            // The antidote ends the poison; its entry leaves the inventory at zero.
            Test->TestTrue(TEXT("Still poisoned"), Combat->IsPoisoned());
            Test->TestTrue(TEXT("The antidote is used"), Combat->UseQuickItem(1, Me));
            Test->TestTrue(TEXT("The poison is gone and the antidote spent"), !Combat->IsPoisoned() && Run->GetItemCount(TEXT("antidote")) == 0 && Used(TEXT("antidote")));
            Test->TestFalse(TEXT("The spent item's entry is gone"), Run->GetRunSnapshot().Player.Items.ContainsByPredicate([](const FMemoriaItemCount& I) { return I.Id == TEXT("antidote"); }));
            Phase = 3; Mark = Frame; break;
        case 3:
            if (Frame < Mark + 55) break;
            // A deep wound: the hi-potion is taken before the potion.
            Test->TestTrue(TEXT("A heavy blow lands"), Combat->StrikePlayer(H, 70.f));
            Test->TestEqual(TEXT("A deep wound takes the hi-potion"), Combat->QuickItemId(0), FString(TEXT("hi_potion")));
            Test->TestTrue(TEXT("The hi-potion is used"), Combat->UseQuickItem(0, Me));
            Test->TestTrue(TEXT("It restores 70 of its 80"), Combat->GetPlayerHp() == Max && Run->GetItemCount(TEXT("hi_potion")) == 0 && Run->GetItemCount(TEXT("potion")) == 1);
            Phase = 4; Mark = Frame; break;
        case 4:
        {
            if (Frame < Mark + 55) break;
            // The witness ink turns one blow aside.
            Test->TestTrue(TEXT("The ink is used"), Combat->UseQuickItem(4, Me) && Combat->IsWarded());
            const int64 Hp = Combat->GetPlayerHp(); const int32 Taken = Combat->GetStrikesTaken();
            Test->TestFalse(TEXT("The warded blow does not land"), Combat->StrikePlayer(H, 20.f));
            Test->TestTrue(TEXT("No harm, and the ward is spent"), Combat->GetPlayerHp() == Hp && Combat->GetStrikesTaken() == Taken && !Combat->IsWarded());
            Phase = 5; Mark = Frame; break;
        }
        case 5:
            if (Frame < Mark + 55 || !H) break;
            // The firebomb, thrown at the husk.
            HuskHealth = H->GetHealth();
            Test->TestTrue(TEXT("The firebomb is thrown"), Combat->UseQuickItem(2, H->GetActorLocation()) && Combat->GetThrown().Num() == 1);
            Test->TestEqual(TEXT("Nothing burns while it flies"), H->GetHealth(), HuskHealth);
            Phase = 6; Mark = Frame; break;
        case 6:
            if (Frame == Mark + 12) Capture(TEXT("ItemsBombFlight"));
            if (Frame < Mark + int32(MemoriaCombatTuning::BombFlight * 60.f) + 4 || !H) break;
            Test->TestTrue(TEXT("It bursts for 12 and sets the husk burning"), FMath::IsNearlyEqual(H->GetHealth(), HuskHealth - 12.f) && H->IsBurning() && Combat->GetThrown().Num() == 0);
            Capture(TEXT("ItemsBombBurst"));
            Phase = 7; Mark = Frame; break;
        case 7:
            if (Frame < Mark + 150 || !H) break;
            Test->TestTrue(TEXT("The fire takes 15 twice"), FMath::IsNearlyEqual(H->GetHealth(), HuskHealth - 12.f - 30.f) && !H->IsBurning());
            // The smoke bomb: the fight ends, unpaid.
            Grains = Run->GetRunSnapshot().Player.Grains;
            Test->TestTrue(TEXT("The smoke bomb is used"), Combat->UseQuickItem(3, Me));
            Test->TestTrue(TEXT("The foes are gone"), Combat->LiveMonsterCount() == 0);
            Phase = 8; Mark = Frame; break;
        case 8:
            if (Frame == Mark + 6) Capture(TEXT("ItemsSmoke"));
            if (Frame < Mark + 60) break;
            Test->TestTrue(TEXT("An escape pays nothing"), Run->GetRunSnapshot().Player.Grains == Grains && Combat->GetLastReward().Age > 3.f);
            Test->TestEqual(TEXT("Six items were used"), Combat->GetItemsUsed(), 6);
            Test->TestFalse(TEXT("An empty slot is refused"), Combat->UseQuickItem(3, Me));
            Test->TestEqual(TEXT("And still six"), Combat->GetItemsUsed(), 6);
            return true;
        }
        return false;
    }
private:
    FAutomationTestBase* Test;
    double Started, OldDelta = 0, LastWorldTime = -1;
    uint64 LastFrame = MAX_uint64;
    int32 Phase = 0, Frame = 0, Mark = 0;
    int64 Grains = 0;
    float HuskHealth = 0.f;
    bool bFixed = false, bOldFixed = false;
    TWeakObjectPtr<AMemoriaFieldMonster> Husk;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFieldItemsTest, "MemoriaVisual.FieldItems", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFieldItemsTest::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice"))) return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FFieldItemsReplay(this)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
