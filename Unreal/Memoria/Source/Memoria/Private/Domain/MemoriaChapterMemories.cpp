#include "Domain/MemoriaChapterMemories.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "MemoriaChapterMemorySources.inl"
namespace
{
struct FChapterMemoryData
{
    TMap<int64, TArray<FMemoriaMemoryDefinition>> Chapters;
    TMap<FString, FMemoriaMemoryLocalizedText> Korean;
};
const FChapterMemoryData& Data()
{
    static const FChapterMemoryData Loaded = []
    {
        FChapterMemoryData D; FString Json;
        for (const TCHAR* Part : MemoriaChapterMemorySourceParts) Json += Part;
        TSharedPtr<FJsonObject> O; const TSharedPtr<FJsonObject>* Chapters = nullptr;
        if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), O) || !O || !O->TryGetObjectField(TEXT("chapters"), Chapters) || !Chapters)
        { UE_LOG(LogTemp, Error, TEXT("MEMORIA_CHAPTER_MEMORIES unreadable source")); return D; }
        auto Str = [](const TSharedPtr<FJsonObject>& E, const TCHAR* Key) { FString S; E->TryGetStringField(Key, S); return S; };
        // String-key lookups: the chapters the source's match statement can name.
        for (int64 Chapter = 1; Chapter <= 30; ++Chapter)
        {
            const TArray<TSharedPtr<FJsonValue>>* Items = nullptr;
            if (!(*Chapters)->TryGetArrayField(LexToString(Chapter), Items) || !Items) continue;
            auto& Rows = D.Chapters.Add(Chapter);
            for (const auto& V : *Items)
            {
                const auto E = V->AsObject(); if (!E) continue;
                FMemoriaMemoryDefinition M;
                M.Id = Str(E, TEXT("id")); M.Title = Str(E, TEXT("title")); M.Description = Str(E, TEXT("description"));
                int32 Grade = 0; E->TryGetNumberField(TEXT("grade"), Grade); M.RawGrade = static_cast<EMemoriaMemoryGrade>(Grade);
                E->TryGetNumberField(TEXT("burn_power"), M.BurnPower);
                M.StoryEffect = Str(E, TEXT("story_effect")); M.RelatedNpc = Str(E, TEXT("related_npc"));
                M.SourcePath = TEXT("scripts/systems/memory_manager.gd::add_chapter_memories"); M.TextId = M.Id;
                Rows.Add(M);
                const TSharedPtr<FJsonObject>* Ko = nullptr;
                if (E->TryGetObjectField(TEXT("ko"), Ko) && Ko)
                {
                    FMemoriaMemoryLocalizedText T; T.MemoryId = M.Id; T.Locale = TEXT("ko");
                    T.Title = Str(*Ko, TEXT("title")); T.Description = Str(*Ko, TEXT("description"));
                    T.StoryEffect = Str(*Ko, TEXT("story_effect")); T.bHasStoryEffect = !T.StoryEffect.IsEmpty();
                    D.Korean.Add(M.Id, T);
                }
            }
        }
        return D;
    }();
    return Loaded;
}
}
const TArray<FMemoriaMemoryDefinition>& MemoriaChapterMemories::For(int64 Chapter)
{
    static const TArray<FMemoriaMemoryDefinition> None;
    const auto* Rows = Data().Chapters.Find(Chapter); return Rows ? *Rows : None;
}
bool MemoriaChapterMemories::Korean(const FString& Id, FMemoriaMemoryLocalizedText& Out)
{
    const auto* Text = Data().Korean.Find(Id);
    if (Text) Out = *Text;
    return Text != nullptr;
}
