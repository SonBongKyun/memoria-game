#include "Journal/MemoriaJournal.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "MemoriaJournalSources.inl"
namespace
{
struct FJournalData
{
    TArray<FMemoriaJournalEntry> Events, People, World, Choices;
    TMap<int32, FString> Chapters, ChaptersKo;
};
// _npc_name's fallback: GameManager.localized_speaker for the names without a name_ko.
FString SpeakerKo(const FString& Name)
{
    static const TMap<FString, FString> Names = {{TEXT("Elia"), TEXT("엘리아")}, {TEXT("Malet"), TEXT("말렛")}, {TEXT("Sable"), TEXT("세이블")},
        {TEXT("Kairos"), TEXT("카이로스")}, {TEXT("Arrel"), TEXT("아렐")}, {TEXT("Tobias"), TEXT("토비아스")}};
    const FString* Found = Names.Find(Name); return Found ? *Found : FString();
}
const FJournalData& Data()
{
    static const FJournalData Loaded = []
    {
        FJournalData D; FString Json;
        for (const TCHAR* Part : MemoriaJournalSourceParts) Json += Part;
        TSharedPtr<FJsonObject> O;
        if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), O) || !O) { UE_LOG(LogTemp, Error, TEXT("MEMORIA_JOURNAL unreadable source")); return D; }
        auto Str = [](const TSharedPtr<FJsonObject>& E, const TCHAR* Key) { FString S; E->TryGetStringField(Key, S); return S; };
        auto List = [&](const TCHAR* Key, TArray<FMemoriaJournalEntry>& Out, bool bPeople)
        {
            const TArray<TSharedPtr<FJsonValue>>* Items;
            if (!O->TryGetArrayField(Key, Items)) return;
            for (const auto& V : *Items)
            {
                const auto E = V->AsObject(); if (!E) continue;
                FMemoriaJournalEntry R;
                R.Flag = Str(E, TEXT("flag")); R.Art = Str(E, TEXT("art")); R.Desc = Str(E, TEXT("desc")); R.DescKo = Str(E, TEXT("desc_ko"));
                E->TryGetNumberField(TEXT("chapter"), R.Chapter);
                if (bPeople)
                {
                    R.Title = Str(E, TEXT("name")); R.TitleKo = Str(E, TEXT("name_ko"));
                    if (R.TitleKo.IsEmpty()) R.TitleKo = SpeakerKo(R.Title);
                    R.Role = Str(E, TEXT("role")); R.RoleKo = Str(E, TEXT("role_ko"));
                }
                else { R.Title = Str(E, TEXT("title")); R.TitleKo = Str(E, TEXT("title_ko")); }
                Out.Add(R);
            }
        };
        List(TEXT("events"), D.Events, false); List(TEXT("npcs"), D.People, true); List(TEXT("world"), D.World, false); List(TEXT("choices"), D.Choices, false);
        for (const TCHAR* Key : {TEXT("chapters"), TEXT("chapters_ko")})
            if (const TSharedPtr<FJsonObject>* Names; O->TryGetObjectField(Key, Names))
                for (int32 Chapter = 1; Chapter <= 40; ++Chapter)
                    if (FString Name; (*Names)->TryGetStringField(FString::FromInt(Chapter), Name))
                        (FCString::Strcmp(Key, TEXT("chapters")) == 0 ? D.Chapters : D.ChaptersKo).Add(Chapter, Name);
        return D;
    }();
    return Loaded;
}
}
FString FMemoriaJournalEntry::TitleIn(bool bKo) const { return bKo && !TitleKo.IsEmpty() ? TitleKo : Title; }
FString FMemoriaJournalEntry::DescIn(bool bKo) const { return bKo && !DescKo.IsEmpty() ? DescKo : Desc; }
namespace MemoriaJournal
{
const TArray<FMemoriaJournalEntry>& Events() { return Data().Events; }
const TArray<FMemoriaJournalEntry>& People() { return Data().People; }
const TArray<FMemoriaJournalEntry>& World() { return Data().World; }
const TArray<FMemoriaJournalEntry>& Choices() { return Data().Choices; }
FString ChapterName(int32 Chapter, bool bKo)
{
    if (bKo) if (const FString* Ko = Data().ChaptersKo.Find(Chapter)) return *Ko;
    const FString* En = Data().Chapters.Find(Chapter); return En ? *En : FString();
}
}
