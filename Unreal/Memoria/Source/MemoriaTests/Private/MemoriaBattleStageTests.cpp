#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInterface.h"
#include "Presentation/MemoriaBattleEntryWidget.h"
#include "Presentation/MemoriaBattleEntryArt.h"
#include "UObject/StrongObjectPtr.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBattleStagePresentation, "Memoria.BattleCore.StagePresentation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBattleStagePresentation::RunTest(const FString&)
{
    // battle_scene.gd composition (S309): the stage blend material and the interface art all resolve,
    // and the stage shows Arrel and Elia before any encounter view arrives.
    TestNotNull(TEXT("Stage blend material"), LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Memoria/Presentation/BattleEntry/M_BattlePlate.M_BattlePlate")));
    for (const TCHAR* Art : {TEXT("res://assets/cg/generated/ui_battle_command_deck_v4.png"), TEXT("res://assets/cg/generated/ui_battle_field_readout_v4.png"),
        TEXT("res://assets/cg/generated/ui_battle_tactical_plate.png"), TEXT("res://assets/cg/generated/ui_battle_victory_reward_panel.png")})
        TestNotNull(*(FString(TEXT("Interface art ")) + Art), MemoriaBattleEntryArt::Load(Art));
    TStrongObjectPtr<UGameInstance> Game(NewObject<UGameInstance>()); Game->Init();
    auto* Widget = CreateWidget<UMemoriaBattleEntryWidget>(Game.Get()); Widget->TakeWidget();
    TestEqual(TEXT("Arrel stands on the stage"), Widget->DisplayedPlayerArtwork(), MemoriaBattleEntryArt::Load(TEXT("res://assets/portraits/character_shots/arrel_battle_v3.png")));
    FMemoriaBattleEntryView View; View.bActive = true; View.bEliaInParty = true; View.bKo = true; View.EnemyIndex = 0;
    View.EnemyImageSource = TEXT("res://assets/cg/generated/battle_stage_v2/enemy_ash_hound_stage_v1.png");
    Widget->Display(View);
    TestNotNull(TEXT("Elia stands behind him"), Widget->DisplayedAllyArtwork());
    TestNotNull(TEXT("The enemy is on the stage"), Widget->DisplayedEnemyArtwork());
    const FString Text = Widget->VisibleText();
    for (const TCHAR* Line : {TEXT("1 · 공격"), TEXT("피해 + 브레이크"), TEXT("3 · 증언 "), TEXT("6 · 도주"), TEXT("전투 이탈")})
        TestTrue(*(FString(TEXT("Command deck shows ")) + Line), Text.Contains(Line));
    Game->Shutdown();
    return !HasAnyErrors();
}
#endif
