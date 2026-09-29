#include "Presentation/MemoriaNarrativeArtwork.h"
#include "Engine/Texture2D.h"
#include "Misc/Paths.h"
#if WITH_EDITOR
#include "TextureCompiler.h"
#endif
namespace MemoriaNarrativeArtwork
{
const TArray<FMemoriaArtworkSource>& Sources()
{
    static const TArray<FMemoriaArtworkSource> Values = {
        {TEXT("res://assets/cg/generated/archive_ch2_information_price_v1.png"), TEXT("/Game/Memoria/Presentation/Dialogue/T_archive_ch2_information_price_v1"), false},
        {TEXT("res://assets/cg/generated/chapter_splash_verdan_market.png"), TEXT("/Game/Memoria/Presentation/BattleEntry/T_VerdanMarket"), true},
        {TEXT("res://assets/cg/generated/chapter_expansion/ch02_bottled_memories_market_v1.png"), TEXT("/Game/Memoria/Presentation/Dialogue/T_ch02_bottled_memories_market_v1"), false},
        {TEXT("res://assets/cg/generated/chapter_expansion/ch02_first_sword_empty_space_v1.png"), TEXT("/Game/Memoria/Presentation/Dialogue/T_ch02_first_sword_empty_space_v1"), false},
        {TEXT("res://assets/cg/generated/chapter_expansion/ch02_malet_from_gray_v1.png"), TEXT("/Game/Memoria/Presentation/Dialogue/T_ch02_malet_from_gray_v1"), false},
        {TEXT("res://assets/cg/generated/chapter_expansion/ch02_nameless_burner_v1.png"), TEXT("/Game/Memoria/Presentation/Dialogue/T_ch02_nameless_burner_v1"), false},
        {TEXT("res://assets/cg/generated/chapter_expansion/ch02_sump_waiting_alcoves_v1.png"), TEXT("/Game/Memoria/Presentation/Dialogue/T_ch02_sump_waiting_alcoves_v1"), false},
        {TEXT("res://assets/cg/generated/story_ch2_first_sword_extraction.png"), TEXT("/Game/Memoria/Presentation/Dialogue/T_story_ch2_first_sword_extraction"), false},
        {TEXT("res://assets/cg/generated/story_ch2_ledger_found.png"), TEXT("/Game/Memoria/Presentation/Dialogue/T_story_ch2_ledger_found"), false},
        {TEXT("res://assets/cg/generated/story_ch2_ledger_return.png"), TEXT("/Game/Memoria/Presentation/Dialogue/T_story_ch2_ledger_return"), false},
        {TEXT("res://assets/cg/generated/story_ch2_lost_instructor_grip.png"), TEXT("/Game/Memoria/Presentation/Dialogue/T_story_ch2_lost_instructor_grip"), false},
        {TEXT("res://assets/cg/generated/story_ch2_malet_cellar.png"), TEXT("/Game/Memoria/Presentation/Dialogue/T_story_ch2_malet_cellar"), false},
        {TEXT("res://assets/cg/generated/story_ch2_malet_seventeen_eyes.png"), TEXT("/Game/Memoria/Presentation/Dialogue/T_story_ch2_malet_seventeen_eyes"), false},
        {TEXT("res://assets/cg/generated/story_ch2_memory_market.png"), TEXT("/Game/Memoria/Presentation/Dialogue/T_story_ch2_memory_market"), false},
        {TEXT("res://assets/cg/generated/story_ch2_nervous_trader_ledger.png"), TEXT("/Game/Memoria/Presentation/Dialogue/T_story_ch2_nervous_trader_ledger"), false},
        {TEXT("res://assets/cg/generated/story_ch2_old_burner.png"), TEXT("/Game/Memoria/Presentation/Dialogue/T_story_ch2_old_burner"), false},
        {TEXT("res://assets/cg/generated/story_ch2_sump_breathing_walls.png"), TEXT("/Game/Memoria/Presentation/Dialogue/T_story_ch2_sump_breathing_walls"), false},
        {TEXT("res://assets/cg/generated/story_ch2_verdan_gate.png"), TEXT("/Game/Memoria/Presentation/Dialogue/T_story_ch2_verdan_gate"), false},
        {TEXT("res://assets/portraits/arrel_face_sad.png"), TEXT("/Game/Memoria/Presentation/Dialogue/T_arrel_face_sad"), false},
        {TEXT("res://assets/portraits/character_shots/arrel_story_v2.png"), TEXT("/Game/Memoria/Presentation/Dialogue/T_arrel_story_v2"), false},
        {TEXT("res://assets/portraits/character_shots/elia_story_v2.png"), TEXT("/Game/Memoria/Presentation/Dialogue/T_elia_story_v2"), false},
        {TEXT("res://assets/portraits/elia_face_gentle_smile.png"), TEXT("/Game/Memoria/Presentation/Dialogue/T_elia_face_gentle_smile"), false},
        {TEXT("res://assets/portraits/elia_face_sad.png"), TEXT("/Game/Memoria/Presentation/Dialogue/T_elia_face_sad"), false},
        {TEXT("res://assets/portraits/elia_face_worried.png"), TEXT("/Game/Memoria/Presentation/Dialogue/T_elia_face_worried"), false},
        {TEXT("res://assets/portraits/malet_face_amused.png"), TEXT("/Game/Memoria/Presentation/Dialogue/T_malet_face_amused"), false},
        {TEXT("res://assets/portraits/malet_face_calculating.png"), TEXT("/Game/Memoria/Presentation/Dialogue/T_malet_face_calculating"), false},
        {TEXT("res://assets/portraits/malet_face_deal_accepted.png"), TEXT("/Game/Memoria/Presentation/Dialogue/T_malet_face_deal_accepted"), false},
        {TEXT("res://assets/portraits/malet_face_disappointed.png"), TEXT("/Game/Memoria/Presentation/Dialogue/T_malet_face_disappointed"), false},
        {TEXT("res://assets/portraits/malet_face_neutral.png"), TEXT("/Game/Memoria/Presentation/Shop/T_MaletPortrait"), true},
        {TEXT("res://assets/portraits/malet_face_price_revealed.png"), TEXT("/Game/Memoria/Presentation/Dialogue/T_malet_face_price_revealed"), false},
        {TEXT("res://assets/portraits/malet_face_warning.png"), TEXT("/Game/Memoria/Presentation/Dialogue/T_malet_face_warning"), false},
        // vn_scene.gd DIALOGUE_OVERLAY_PATH / CHOICE_OVERLAY_PATH frame art (S305).
        {TEXT("res://assets/cg/generated/ui_vn_memory_frame_overlay.png"), TEXT("/Game/Memoria/Presentation/Dialogue/T_ui_vn_memory_frame_overlay"), false},
        {TEXT("res://assets/cg/generated/ui_vn_choice_archive_overlay.png"), TEXT("/Game/Memoria/Presentation/Dialogue/T_ui_vn_choice_archive_overlay"), false},
        // main.gd TITLE_BG_PATH (S308).
        {TEXT("res://assets/cg/generated/ui_title_memoria_premium.png"), TEXT("/Game/Memoria/Presentation/Dialogue/T_ui_title_memoria_premium"), false},
        // Chapter 1 route (S304).
        {TEXT("res://assets/cg/generated/archive_ch1_camp_humming_v2.png"), TEXT("/Game/Memoria/Presentation/Dialogue/T_archive_ch1_camp_humming_v2"), false},
        {TEXT("res://assets/cg/generated/chapter_splash_rim_forest.png"), TEXT("/Game/Memoria/Presentation/Dialogue/T_chapter_splash_rim_forest"), false},
        {TEXT("res://assets/cg/generated/cinematic_void_beast_memory_devour.png"), TEXT("/Game/Memoria/Presentation/Dialogue/T_cinematic_void_beast_memory_devour"), false},
        {TEXT("res://assets/cg/generated/illustration_expansion_v2/world_rewrite_campfire_song_v3.png"), TEXT("/Game/Memoria/Presentation/Dialogue/T_world_rewrite_campfire_song_v3"), false},
        {TEXT("res://assets/cg/generated/illustration_expansion_v2/world_rewrite_first_sword_v3.png"), TEXT("/Game/Memoria/Presentation/Dialogue/T_world_rewrite_first_sword_v3"), false},
        {TEXT("res://assets/cg/generated/illustration_expansion_v2/world_rewrite_reaching_hand_v3.png"), TEXT("/Game/Memoria/Presentation/Dialogue/T_world_rewrite_reaching_hand_v3"), false},
        {TEXT("res://assets/cg/generated/story_ch1_ash_rain_touch.png"), TEXT("/Game/Memoria/Presentation/Dialogue/T_story_ch1_ash_rain_touch"), false},
        {TEXT("res://assets/cg/generated/story_ch1_elia_reunion.png"), TEXT("/Game/Memoria/Presentation/Dialogue/T_story_ch1_elia_reunion"), false},
        {TEXT("res://assets/cg/generated/story_ch1_first_burn_strike.png"), TEXT("/Game/Memoria/Presentation/Dialogue/T_story_ch1_first_burn_strike"), false},
        {TEXT("res://assets/cg/generated/story_ch1_green_tree_dawn.png"), TEXT("/Game/Memoria/Presentation/Dialogue/T_story_ch1_green_tree_dawn"), false},
        {TEXT("res://assets/cg/generated/story_ch1_memory_shrine.png"), TEXT("/Game/Memoria/Presentation/Dialogue/T_story_ch1_memory_shrine"), false},
        {TEXT("res://assets/cg/generated/story_ch1_opening_aftermath.png"), TEXT("/Game/Memoria/Presentation/Dialogue/T_story_ch1_opening_aftermath"), false},
        {TEXT("res://assets/cg/generated/story_ch1_rim_omen.png"), TEXT("/Game/Memoria/Presentation/Dialogue/T_story_ch1_rim_omen"), false},
        {TEXT("res://assets/cg/generated/story_ch1_twisted_forest_path.png"), TEXT("/Game/Memoria/Presentation/Dialogue/T_story_ch1_twisted_forest_path"), false},
        {TEXT("res://assets/cg/generated/story_ch1_void_beast_emergence.png"), TEXT("/Game/Memoria/Presentation/Dialogue/T_story_ch1_void_beast_emergence"), false},
        {TEXT("res://assets/cg/game_image/sealed_city_ruins.png"), TEXT("/Game/Memoria/Presentation/Dialogue/T_sealed_city_ruins"), false},
        {TEXT("res://assets/portraits/arrel_face_determined.png"), TEXT("/Game/Memoria/Presentation/Dialogue/T_arrel_face_determined"), false},
        {TEXT("res://assets/portraits/arrel_face_memory_fading.png"), TEXT("/Game/Memoria/Presentation/Dialogue/T_arrel_face_memory_fading"), false},
        {TEXT("res://assets/portraits/arrel_face_shocked.png"), TEXT("/Game/Memoria/Presentation/Dialogue/T_arrel_face_shocked"), false},
        {TEXT("res://assets/portraits/character_shots/elia_anchor_v3.png"), TEXT("/Game/Memoria/Presentation/BattleEntry/T_EliaAnchor"), true},
        // S320 Chapter 3, Belt Waystation: the splash and the four CGs chapter3_dialogue.json names.
        {TEXT("res://assets/cg/generated/chapter_splash_belt_waystation.png"), TEXT("/Game/Memoria/Presentation/Dialogue/T_chapter_splash_belt_waystation"), false},
        {TEXT("res://assets/cg/generated/chapter_expansion/ch03_waystation_haze_v1.png"), TEXT("/Game/Memoria/Presentation/Dialogue/T_ch03_waystation_haze_v1"), false},
        {TEXT("res://assets/cg/generated/chapter_expansion/ch03_class_seven_wall_v1.png"), TEXT("/Game/Memoria/Presentation/Dialogue/T_ch03_class_seven_wall_v1"), false},
        {TEXT("res://assets/cg/generated/story_ch3_blank_book_warmth.png"), TEXT("/Game/Memoria/Presentation/Dialogue/T_story_ch3_blank_book_warmth"), false},
        {TEXT("res://assets/cg/generated/archive_ch3_kairos_marks_v1.png"), TEXT("/Game/Memoria/Presentation/Dialogue/T_archive_ch3_kairos_marks_v1"), false},
        {TEXT("res://assets/cg/generated/chapter_splash_drift_shelter.png"), TEXT("/Game/Memoria/Presentation/Dialogue/T_chapter_splash_drift_shelter"), false},
        {TEXT("res://assets/cg/generated/chapter_expansion/ch04_words_move_v1.png"), TEXT("/Game/Memoria/Presentation/Dialogue/T_ch04_words_move_v1"), false},
        {TEXT("res://assets/cg/generated/archive_drift_anchoring_v1.png"), TEXT("/Game/Memoria/Presentation/Dialogue/T_archive_drift_anchoring_v1"), false},
        {TEXT("res://assets/cg/generated/chapter_expansion/ch04_anchor_tea_thread_v1.png"), TEXT("/Game/Memoria/Presentation/Dialogue/T_ch04_anchor_tea_thread_v1"), false},
        {TEXT("res://assets/cg/generated/resonance_drift_shelter_echo.png"), TEXT("/Game/Memoria/Presentation/Dialogue/T_resonance_drift_shelter_echo"), false},
        {TEXT("res://assets/cg/generated/cinematic_kairos_authority_edit.png"), TEXT("/Game/Memoria/Presentation/Dialogue/T_cinematic_kairos_authority_edit"), false},
        {TEXT("res://assets/portraits/character_shots/kairos_story_v2.png"), TEXT("/Game/Memoria/Presentation/Dialogue/T_kairos_story_v2"), false},
        {TEXT("res://assets/portraits/character_shots/kairos_edit_v3.png"), TEXT("/Game/Memoria/Presentation/Dialogue/T_kairos_edit_v3"), false},
    };
    return Values;
}
FString PortraitSource(const FString& Key)
{
    static const TMap<FString, FString> Values = {
        {TEXT("arrel_burn"), TEXT("res://assets/portraits/arrel_face_memory_fading.png")},
        {TEXT("arrel_cold"), TEXT("res://assets/portraits/arrel_face_memory_fading.png")},
        {TEXT("arrel_default2"), TEXT("res://assets/portraits/character_shots/arrel_story_v2.png")},
        {TEXT("arrel_determined"), TEXT("res://assets/portraits/arrel_face_determined.png")},
        {TEXT("arrel_exhausted"), TEXT("res://assets/portraits/arrel_face_sad.png")},
        {TEXT("arrel_neutral"), TEXT("res://assets/portraits/character_shots/arrel_story_v2.png")},
        {TEXT("arrel_pain"), TEXT("res://assets/portraits/arrel_face_shocked.png")},
        {TEXT("arrel_pensive"), TEXT("res://assets/portraits/character_shots/arrel_story_v2.png")},
        {TEXT("arrel_shocked"), TEXT("res://assets/portraits/arrel_face_shocked.png")},
        {TEXT("elia_calm"), TEXT("res://assets/portraits/character_shots/elia_story_v2.png")},
        {TEXT("elia_concern"), TEXT("res://assets/portraits/elia_face_worried.png")},
        {TEXT("elia_determined"), TEXT("res://assets/portraits/character_shots/elia_anchor_v3.png")},
        {TEXT("elia_gentle_smile"), TEXT("res://assets/portraits/elia_face_gentle_smile.png")},
        {TEXT("elia_hopeful"), TEXT("res://assets/portraits/elia_face_gentle_smile.png")},
        {TEXT("elia_neutral"), TEXT("res://assets/portraits/character_shots/elia_story_v2.png")},
        {TEXT("elia_sad"), TEXT("res://assets/portraits/elia_face_sad.png")},
        {TEXT("elia_worried"), TEXT("res://assets/portraits/elia_face_worried.png")},
        {TEXT("kairos_cold"), TEXT("res://assets/portraits/character_shots/kairos_edit_v3.png")},
        {TEXT("kairos_neutral"), TEXT("res://assets/portraits/character_shots/kairos_story_v2.png")},
        {TEXT("malet_amused"), TEXT("res://assets/portraits/malet_face_amused.png")},
        {TEXT("malet_calculating"), TEXT("res://assets/portraits/malet_face_calculating.png")},
        {TEXT("malet_deal_accepted"), TEXT("res://assets/portraits/malet_face_deal_accepted.png")},
        {TEXT("malet_disappointed"), TEXT("res://assets/portraits/malet_face_disappointed.png")},
        {TEXT("malet_neutral"), TEXT("res://assets/portraits/malet_face_neutral.png")},
        {TEXT("malet_price_revealed"), TEXT("res://assets/portraits/malet_face_price_revealed.png")},
        {TEXT("malet_warning"), TEXT("res://assets/portraits/malet_face_warning.png")},
    };
    const FString* Value = Values.Find(Key); return Value ? *Value : FString();
}
FString CgSource(const FString& Ref)
{
    // vn_scene.gd _resolve_cg_path: res:// paths as authored, CG_ALIAS_FALLBACKS, then
    // assets/cg/game_image/<ref>.png among the imported pictures, else DEFAULT_CG_FALLBACK.
    if (Ref.IsEmpty()) return FString();
    if (Ref.StartsWith(TEXT("res://"))) return Ref;
    static const TMap<FString, FString> Aliases = {
        {TEXT("ch1_twisted_forest"), TEXT("res://assets/cg/generated/story_ch1_twisted_forest_path.png")},
        {TEXT("ch1_stump2"), TEXT("res://assets/cg/generated/story_ch1_memory_shrine.png")},
        {TEXT("ch1_ash_forest"), TEXT("res://assets/cg/generated/story_ch1_memory_shrine.png")},
        {TEXT("ch1_green_tree"), TEXT("res://assets/cg/generated/story_ch1_green_tree_dawn.png")},
    };
    if (const FString* Alias = Aliases.Find(Ref)) return *Alias;
    const FString GameImage = TEXT("res://assets/cg/game_image/") + Ref + TEXT(".png");
    if (Sources().ContainsByPredicate([&](const auto& E){ return GameImage.Equals(E.Source, ESearchCase::CaseSensitive); })) return GameImage;
    return TEXT("res://assets/cg/generated/chapter_splash_rim_forest.png");
}
UTexture2D* Load(const FString& Source)
{
    const auto* Entry = Sources().FindByPredicate([&](const auto& E){ return Source.Equals(E.Source, ESearchCase::CaseSensitive); });
    if (!Entry) return nullptr;
    const FString Package(Entry->Package);
    auto* Texture = LoadObject<UTexture2D>(nullptr, *(Package + TEXT(".") + FPaths::GetBaseFilename(Package)));
#if WITH_EDITOR
    // UMG captures brush dimensions now. Pending editor texture compilation
    // returns placeholder dimensions, which otherwise persist in the brush.
    if (Texture) FTextureCompilingManager::Get().FinishCompilation({Texture});
#endif
    return Texture;
}
}
