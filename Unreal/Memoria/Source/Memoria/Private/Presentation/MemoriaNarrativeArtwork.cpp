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
    };
    return Values;
}
FString PortraitSource(const FString& Key)
{
    static const TMap<FString, FString> Values = {
        {TEXT("arrel_exhausted"), TEXT("res://assets/portraits/arrel_face_sad.png")},
        {TEXT("arrel_neutral"), TEXT("res://assets/portraits/character_shots/arrel_story_v2.png")},
        {TEXT("arrel_pensive"), TEXT("res://assets/portraits/character_shots/arrel_story_v2.png")},
        {TEXT("elia_calm"), TEXT("res://assets/portraits/character_shots/elia_story_v2.png")},
        {TEXT("elia_concern"), TEXT("res://assets/portraits/elia_face_worried.png")},
        {TEXT("elia_hopeful"), TEXT("res://assets/portraits/elia_face_gentle_smile.png")},
        {TEXT("elia_neutral"), TEXT("res://assets/portraits/character_shots/elia_story_v2.png")},
        {TEXT("elia_sad"), TEXT("res://assets/portraits/elia_face_sad.png")},
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
