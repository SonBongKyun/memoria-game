#pragma once
#include "CoreMinimal.h"
#include "Domain/MemoriaMemoryTypes.h"

// S330: memory_manager.gd add_chapter_memories, the memories a chapter brings (the connected canon's Chapters
// 3 to 5), from the source table exported by Unreal/Tools/export_chapter_memories.py.
namespace MemoriaChapterMemories
{
    // The chapter's memories in the source's order; empty for a chapter that brings none.
    MEMORIA_API const TArray<FMemoriaMemoryDefinition>& For(int64 Chapter);
    // MEMORY_TEXT_KO for a chapter memory; false for any other id.
    MEMORIA_API bool Korean(const FString& Id, FMemoriaMemoryLocalizedText& Out);
}
