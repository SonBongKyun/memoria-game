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
    static const TArray<FMemoriaBattleEntryArtSource> Values={
        {TEXT("res://assets/cg/generated/chapter_splash_verdan_market.png"),TEXT("assets/cg/generated/chapter_splash_verdan_market.png"),TEXT("/Game/Memoria/Presentation/BattleEntry/T_VerdanMarket"),false},
        {TEXT("res://assets/portraits/character_shots/arrel_battle_v3.png"),TEXT("assets/portraits/character_shots/arrel_battle_v3.png"),TEXT("/Game/Memoria/Presentation/BattleEntry/T_ArrelBattle"),false},
        {TEXT("res://assets/portraits/character_shots/elia_anchor_v3.png"),TEXT("assets/portraits/character_shots/elia_anchor_v3.png"),TEXT("/Game/Memoria/Presentation/BattleEntry/T_EliaAnchor"),false},
        // This is the existing legacy Alley Rat mapping, not a newly established creature design.
        {TEXT("res://assets/cg/generated/battle_stage_v2/enemy_ash_hound_stage_v1.png"),TEXT("assets/cg/generated/battle_stage_v2/enemy_ash_hound_stage_v1.png"),TEXT("/Game/Memoria/Presentation/BattleEntry/T_AlleyRatLegacy"),false},
        {TEXT("source://PixelSprite/create_battle_enemy/Market Thief"),TEXT("docs/unreal-migration/fixtures/battle_entry_art/market_thief.png"),TEXT("/Game/Memoria/Presentation/BattleEntry/T_MarketThiefSource"),true},
        // S291 presentation study only; not source-authored creature or story canon.
        {TEXT("draft://S291/MarketThiefStudy"),TEXT("Unreal/ArtSource/BattleEntry/market_thief_study_v1.png"),TEXT("/Game/Memoria/Presentation/BattleEntry/T_MarketThiefStudy"),false},
        // S292 presentation study; the legacy source image remains available as fallback.
        {TEXT("draft://S292/AlleyRatStudy"),TEXT("Unreal/ArtSource/BattleEntry/alley_rat_study_v2.png"),TEXT("/Game/Memoria/Presentation/BattleEntry/T_AlleyRatStudy"),false},
    };
    return Values;
}
FString MarketThiefSource() { return TEXT("source://PixelSprite/create_battle_enemy/Market Thief"); }
FString MarketThiefStudySource() { return TEXT("draft://S291/MarketThiefStudy"); }
FString AlleyRatStudySource() { return TEXT("draft://S292/AlleyRatStudy"); }
UTexture2D* Load(const FString& Source)
{
    const auto* Entry=Sources().FindByPredicate([&](const auto& E){return Source.Equals(E.Source,ESearchCase::CaseSensitive);});
    if(!Entry)return nullptr;
    const FString Package(Entry->Package);
    if((Source==MarketThiefStudySource() || Source==AlleyRatStudySource()) && !FPackageName::DoesPackageExist(Package))return nullptr;
    auto* Texture=LoadObject<UTexture2D>(nullptr,*(Package+TEXT(".")+FPaths::GetBaseFilename(Package)));
#if WITH_EDITOR
    if(Texture)FTextureCompilingManager::Get().FinishCompilation({Texture});
#endif
    return Texture;
}
}
