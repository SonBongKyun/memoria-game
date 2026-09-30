#pragma once
#include "CoreMinimal.h"

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
namespace MemoriaJournal
{
    MEMORIA_API const TArray<FMemoriaJournalEntry>& Events();
    MEMORIA_API const TArray<FMemoriaJournalEntry>& People();
    MEMORIA_API const TArray<FMemoriaJournalEntry>& World();
    MEMORIA_API const TArray<FMemoriaJournalEntry>& Choices();
    // GameManager.localized_chapter_name: CHAPTER_NAMES_KO, else RICH_PRESENCE_CHAPTERS / CHAPTER_NAMES.
    MEMORIA_API FString ChapterName(int32 Chapter, bool bKo);
}
