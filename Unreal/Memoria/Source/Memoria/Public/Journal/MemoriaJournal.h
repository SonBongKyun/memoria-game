#pragma once
#include "CoreMinimal.h"
#include "Presentation/MemoriaNarrativeArtwork.h"
class UMemoriaRunSubsystem;
class UTexture2D;
struct FMemoriaRunSnapshot;

// S327: story_journal.gd's tables, read from the IR Unreal/Tools/export_journal.py extracts (compiled in).
// The journal is derived from the run's story flags: an entry shows once its flag is set.
struct MEMORIA_API FMemoriaJournalEntry
{
    FString Flag, Title, TitleKo, Desc, DescKo, Art, Role, RoleKo;   // NPCs: Title is the name, Role the role
    int32 Chapter = 0;
    // _field: the Korean text when the locale asks for it and it exists, else the English.
    FString TitleIn(bool bKo) const;
    FString DescIn(bool bKo) const;
};
// S332: SideQuest.QUESTS as the journal reads it.
struct MEMORIA_API FMemoriaJournalQuestStep { FString Flag, Desc, DescKo; };
struct MEMORIA_API FMemoriaJournalQuest
{
    FString Id, Title, TitleKo, Desc, DescKo, Art, Map, Npc, PrereqFlag;
    int32 ChapterReq = 0;
    TArray<FMemoriaJournalQuestStep> Steps;
};
// SideQuest.get_all_quests' status.
enum class EMemoriaQuestStatus : uint8 { Locked, Available, Active, Complete };
// S332: a row of the Quests or Losses tab as it is shown: its list label, and the detail's title, body and art.
struct MEMORIA_API FMemoriaJournalRecord { FString Label, Title, Body, Art; FLinearColor Color = FLinearColor::White; };
namespace MemoriaJournal
{
    MEMORIA_API const TArray<FMemoriaJournalEntry>& Events();
    MEMORIA_API const TArray<FMemoriaJournalEntry>& People();
    MEMORIA_API const TArray<FMemoriaJournalEntry>& World();
    MEMORIA_API const TArray<FMemoriaJournalEntry>& Choices();
    // GameManager.localized_chapter_name: CHAPTER_NAMES_KO, else RICH_PRESENCE_CHAPTERS / CHAPTER_NAMES.
    MEMORIA_API FString ChapterName(int32 Chapter, bool bKo);
    // S332 Quests: the source's six side quests; a quest's status from the run's chapter and flags; and
    // _populate_quests' rows. The rows leave out quests on maps the port does not have, so the journal never
    // sends the player to a place that cannot be reached.
    MEMORIA_API const TArray<FMemoriaJournalQuest>& Quests();
    MEMORIA_API EMemoriaQuestStatus QuestStatus(const FMemoriaJournalQuest& Quest, const FMemoriaRunSnapshot& Run);
    MEMORIA_API TArray<FMemoriaJournalRecord> QuestRecords(const UMemoriaRunSubsystem& Run, bool bKo);
    // S332 Losses: WorldRewriteDirector.get_loss_records, one record per burned memory and per faded one, in
    // the archive's order (MEMORY_REWRITE_RULES, else the grade's default line, colour and art).
    MEMORIA_API TArray<FMemoriaJournalRecord> LossRecords(const UMemoriaRunSubsystem& Run, bool bKo);
    // The journal's own illustrations (the pictures no dialogue shows), imported by -run=MemoriaDialogueAssets,
    // and a picture from any of the port's art tables.
    MEMORIA_API const TArray<FMemoriaArtworkSource>& ArtSources();
    MEMORIA_API UTexture2D* LoadArt(const FString& Source);
}
