#include "Presentation/MemoriaBattleEntryArt.h"
#include "Engine/Texture2D.h"
#include "Misc/Paths.h"
#include "Misc/PackageName.h"
#if WITH_EDITOR
#include "TextureCompiler.h"
#endif
namespace MemoriaBattleEntryArt
{
const TArray<FMemoriaBattleEntryArtSource>& Sources()
{
    // S333: the turn-based battle's own art (the command deck, readout, plates, the battle portraits and the
    // enemy studies) went with the battle. What remains is the UI art the menus load, and the two pictures
    // the dialogue's table reuses from this folder.
    static const TArray<FMemoriaBattleEntryArtSource> Values={
        {TEXT("res://assets/cg/generated/chapter_splash_verdan_market.png"),TEXT("assets/cg/generated/chapter_splash_verdan_market.png"),TEXT("/Game/Memoria/Presentation/BattleEntry/T_VerdanMarket"),false},
        {TEXT("res://assets/portraits/character_shots/elia_anchor_v3.png"),TEXT("assets/portraits/character_shots/elia_anchor_v3.png"),TEXT("/Game/Memoria/Presentation/BattleEntry/T_EliaAnchor"),false},
        // game_over.gd and pause_menu.gd backdrops (S317-S318).
        {TEXT("res://assets/cg/generated/ui_game_over_void_backdrop.png"),TEXT("assets/cg/generated/ui_game_over_void_backdrop.png"),TEXT("/Game/Memoria/Presentation/BattleEntry/T_UiGameOverBackdrop"),false},
        {TEXT("res://assets/cg/generated/ui_pause_archive_backdrop_v2.png"),TEXT("assets/cg/generated/ui_pause_archive_backdrop_v2.png"),TEXT("/Game/Memoria/Presentation/BattleEntry/T_UiPauseBackdrop"),false},
        {TEXT("res://assets/cg/generated/ui_pause_control_slab.png"),TEXT("assets/cg/generated/ui_pause_control_slab.png"),TEXT("/Game/Memoria/Presentation/BattleEntry/T_UiPauseSlab"),false},
        // tutorial_hints.gd banner frame (S323).
        {TEXT("res://assets/cg/generated/ui_tutorial_hint_banner.png"),TEXT("assets/cg/generated/ui_tutorial_hint_banner.png"),TEXT("/Game/Memoria/Presentation/BattleEntry/T_UiHintBanner"),false},
        // story_journal.gd backdrop (S327).
        {TEXT("res://assets/cg/generated/ui_story_journal_backdrop_v3.png"),TEXT("assets/cg/generated/ui_story_journal_backdrop_v3.png"),TEXT("/Game/Memoria/Presentation/BattleEntry/T_UiJournalBackdrop"),false},
        // codex.gd backdrop (S325).
        {TEXT("res://assets/cg/generated/ui_codex_archive_backdrop_v2.png"),TEXT("assets/cg/generated/ui_codex_archive_backdrop_v2.png"),TEXT("/Game/Memoria/Presentation/BattleEntry/T_UiCodexBackdrop"),false},
        // pause_menu.gd achievements backdrop (S324).
        {TEXT("res://assets/cg/generated/ui_achievements_chronicle_backdrop.png"),TEXT("assets/cg/generated/ui_achievements_chronicle_backdrop.png"),TEXT("/Game/Memoria/Presentation/BattleEntry/T_UiAchievementsBackdrop"),false},
    };
    return Values;
}
UTexture2D* Load(const FString& Source)
{
    const auto* Entry=Sources().FindByPredicate([&](const auto& E){return Source.Equals(E.Source,ESearchCase::CaseSensitive);});
    if(!Entry)return nullptr;
    const FString Package(Entry->Package);
    auto* Texture=LoadObject<UTexture2D>(nullptr,*(Package+TEXT(".")+FPaths::GetBaseFilename(Package)));
#if WITH_EDITOR
    if(Texture)FTextureCompilingManager::Get().FinishCompilation({Texture});
#endif
    return Texture;
}
}
