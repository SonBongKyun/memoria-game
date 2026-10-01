#include "Journal/MemoriaJournal.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Domain/MemoriaPlayerMemoryDomain.h"
#include "Domain/MemoriaChapterMemories.h"
#include "Chapter/MemoriaChapterMap.h"
#include "Presentation/MemoriaBattleEntryArt.h"
#include "Engine/Texture2D.h"
#include "Misc/Paths.h"
#if WITH_EDITOR
#include "TextureCompiler.h"
#endif
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "MemoriaJournalSources.inl"
namespace
{
// MEMORY_REWRITE_RULES: what burning one memory rewrites.
struct FLossRule { FString Flag, Title, TitleKo, Line, LineKo, Compass, Art; FLinearColor Color = FLinearColor::White; };
struct FJournalData
{
    TArray<FMemoriaJournalEntry> Events, People, World, Choices;
    TMap<int32, FString> Chapters, ChaptersKo;
    TArray<FMemoriaJournalQuest> Quests;
    TMap<FString, FLossRule> LossRules;
    FString LossLines[5], LossLinesKo[5];   // DEFAULT_LINES by grade (0: Grade 5 ... 4: Grade 1)
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
        if (const TArray<TSharedPtr<FJsonValue>>* Items = nullptr; O->TryGetArrayField(TEXT("quests"), Items) && Items)
            for (const auto& V : *Items)
            {
                const auto E = V->AsObject(); if (!E) continue;
                FMemoriaJournalQuest Q;
                Q.Id = Str(E, TEXT("id")); Q.Title = Str(E, TEXT("title")); Q.TitleKo = Str(E, TEXT("title_ko")); Q.Desc = Str(E, TEXT("desc")); Q.DescKo = Str(E, TEXT("desc_ko"));
                Q.Art = Str(E, TEXT("art")); Q.Map = Str(E, TEXT("map")); Q.Npc = Str(E, TEXT("npc")); Q.PrereqFlag = Str(E, TEXT("prereq_flag"));
                E->TryGetNumberField(TEXT("chapter_req"), Q.ChapterReq);
                if (const TArray<TSharedPtr<FJsonValue>>* Steps = nullptr; E->TryGetArrayField(TEXT("steps"), Steps) && Steps)
                    for (const auto& S : *Steps) if (const auto Step = S->AsObject()) Q.Steps.Add({Str(Step, TEXT("flag")), Str(Step, TEXT("desc")), Str(Step, TEXT("desc_ko"))});
                D.Quests.Add(Q);
            }
        if (const TArray<TSharedPtr<FJsonValue>>* Items = nullptr; O->TryGetArrayField(TEXT("loss_rules"), Items) && Items)
            for (const auto& V : *Items)
            {
                const auto E = V->AsObject(); if (!E) continue;
                FLossRule R;
                R.Flag = Str(E, TEXT("flag")); R.Title = Str(E, TEXT("title")); R.TitleKo = Str(E, TEXT("title_ko")); R.Line = Str(E, TEXT("line")); R.LineKo = Str(E, TEXT("line_ko"));
                R.Compass = Str(E, TEXT("compass")); R.Art = Str(E, TEXT("art"));
                if (const TArray<TSharedPtr<FJsonValue>>* Rgb = nullptr; E->TryGetArrayField(TEXT("color"), Rgb) && Rgb && Rgb->Num() >= 3)
                    R.Color = FLinearColor((*Rgb)[0]->AsNumber(), (*Rgb)[1]->AsNumber(), (*Rgb)[2]->AsNumber());
                D.LossRules.Add(Str(E, TEXT("memory")), R);
            }
        for (int32 Grade = 0; Grade < 5; ++Grade)
        {
            if (const TSharedPtr<FJsonObject>* Lines = nullptr; O->TryGetObjectField(TEXT("loss_default_lines"), Lines) && Lines) (*Lines)->TryGetStringField(FString::FromInt(Grade), D.LossLines[Grade]);
            if (const TSharedPtr<FJsonObject>* Lines = nullptr; O->TryGetObjectField(TEXT("loss_default_lines_ko"), Lines) && Lines) (*Lines)->TryGetStringField(FString::FromInt(Grade), D.LossLinesKo[Grade]);
        }
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
const TArray<FMemoriaJournalQuest>& Quests() { return Data().Quests; }
EMemoriaQuestStatus QuestStatus(const FMemoriaJournalQuest& Quest, const FMemoriaRunSnapshot& Run)
{
    // get_all_quests: complete (the last step's flag), active (started, not complete), available (the chapter
    // and the prerequisite flag, not started), else locked.
    if (!Quest.Steps.IsEmpty() && Run.GetFlag(Quest.Steps.Last().Flag)) return EMemoriaQuestStatus::Complete;
    const bool bStarted = Run.GetFlag(FString::Printf(TEXT("sq_%s_started"), *Quest.Id));
    if (bStarted) return EMemoriaQuestStatus::Active;
    if (Run.CurrentChapter < Quest.ChapterReq || (!Quest.PrereqFlag.IsEmpty() && !Run.GetFlag(Quest.PrereqFlag))) return EMemoriaQuestStatus::Locked;
    return EMemoriaQuestStatus::Available;
}
TArray<FMemoriaJournalRecord> QuestRecords(const UMemoriaRunSubsystem& Run, bool bKo)
{
    TArray<FMemoriaJournalRecord> Out;
    const auto& State = Run.GetRunSnapshot();
    for (const auto& Q : Quests())
    {
        // A quest's map must be one the port has: Verdan, or a ported chapter map.
        if (Q.Map != TEXT("verdan_market") && !MemoriaChapterMaps::Find(Q.Map)) continue;
        const auto Status = QuestStatus(Q, State);
        if (Status == EMemoriaQuestStatus::Locked) continue;
        const FString Title = bKo && !Q.TitleKo.IsEmpty() ? Q.TitleKo : Q.Title, Desc = bKo && !Q.DescKo.IsEmpty() ? Q.DescKo : Q.Desc;
        FMemoriaJournalRecord R; R.Title = Title; R.Art = Q.Art;
        if (Status == EMemoriaQuestStatus::Complete)
        { R.Color = FLinearColor(.4f, .7f, .4f); R.Label = (bKo ? TEXT("[완료] ") : TEXT("[DONE] ")) + Title; R.Body = Desc; }
        else if (Status == EMemoriaQuestStatus::Active)
        {
            // get_current_step_text: the first step whose flag is not set.
            FString Step;
            for (const auto& S : Q.Steps) if (!State.GetFlag(S.Flag)) { Step = bKo && !S.DescKo.IsEmpty() ? S.DescKo : S.Desc; break; }
            R.Color = FLinearColor(.85f, .7f, .4f); R.Label = Title; R.Body = Desc + (bKo ? TEXT("\n\n현재: ") : TEXT("\n\nCurrent: ")) + Step;
        }
        else
        {
            // "Talk to <npc> at <map, title-cased>."
            FString Where; TArray<FString> Parts; Q.Map.ParseIntoArray(Parts, TEXT("_"));
            for (const FString& P : Parts) Where += (Where.IsEmpty() ? TEXT("") : TEXT(" ")) + P.Left(1).ToUpper() + P.Mid(1);
            R.Color = FLinearColor(.55f, .5f, .45f); R.Label = (bKo ? TEXT("[신규] ") : TEXT("[NEW] ")) + Title;
            // In Korean the first step's own text says the same thing in the source's words.
            R.Body = Desc + (bKo && !Q.Steps.IsEmpty() && !Q.Steps[0].DescKo.IsEmpty() ? TEXT("\n\n") + Q.Steps[0].DescKo
                                 : FString::Printf(TEXT("\n\nTalk to %s at %s."), *Q.Npc, *Where));
        }
        Out.Add(R);
    }
    return Out;
}
TArray<FMemoriaJournalRecord> LossRecords(const UMemoriaRunSubsystem& Run, bool bKo)
{
    TArray<FMemoriaJournalRecord> Out;
    const auto* Memory = Run.GetPlayerMemory();
    if (!Run.HasActiveRun() || !Memory) return Out;
    const auto Snapshot = Memory->GetSnapshot();
    const auto* Catalog = LoadObject<UMemoriaMemoryCatalog>(nullptr, TEXT("/Game/Memoria/Generated/Memory/DA_StartingMemoryCatalog.DA_StartingMemoryCatalog"));
    // _grade_color, _grade_name and _fallback_art_for_grade, by grade index (0: Grade 5 ... 4: Grade 1).
    static const FLinearColor GradeColors[5] = {FLinearColor(.70f, .76f, .62f), FLinearColor(.88f, .70f, .42f), FLinearColor(.72f, .58f, .9f), FLinearColor(.9f, .46f, .52f), FLinearColor(1.f, .36f, .22f)};
    static const TCHAR* GradeNames[5] = {TEXT("Grade 5 / Sensation"), TEXT("Grade 4 / Daily Life"), TEXT("Grade 3 / Relationship"), TEXT("Grade 2 / Identity"), TEXT("Grade 1 / Core")};
    static const TCHAR* GradeNamesKo[5] = {TEXT("5등급 / 감각"), TEXT("4등급 / 일상"), TEXT("3등급 / 관계"), TEXT("2등급 / 정체성"), TEXT("1등급 / 핵심")};
    for (const auto& State : Snapshot.Owned)
    {
        const bool bFaded = !State.bBurned && State.bFaded;
        if (!State.bBurned && !bFaded) continue;
        const auto* Def = Memory->GetDefinitions().FindByPredicate([&](const auto& D) { return D.Id.Equals(State.Id, ESearchCase::CaseSensitive); });
        if (!Def) continue;
        const int32 Grade = FMath::Clamp(int32(Def->RawGrade), 0, 4);
        const FLossRule* Rule = Data().LossRules.Find(State.Id);
        // _build_report.
        FString Title = Rule ? (bKo && !Rule->TitleKo.IsEmpty() ? Rule->TitleKo : Rule->Title) : FString(bKo ? TEXT("기록되지 않은 공백") : TEXT("Uncatalogued Absence"));
        FString Line = Rule ? (bKo && !Rule->LineKo.IsEmpty() ? Rule->LineKo : Rule->Line) : (bKo ? Data().LossLinesKo[Grade] : Data().LossLines[Grade]);
        // _default_compass_line names the memory by its source title.
        FString Compass = Rule && !Rule->Compass.IsEmpty() ? Rule->Compass : (bFaded ? TEXT("Eroding: ") : TEXT("Lost: ")) + Def->Title;
        if (bFaded)
        {
            Title = (bKo ? TEXT("바래는 중: ") : TEXT("Fading: ")) + Title;
            Line = (bKo ? TEXT("타기도 전에 얇아진다: ") : TEXT("Before it burns, it thins: ")) + Line;
            Compass = TEXT("Erosion warning: ") + Compass;
        }
        // MemoryManager.localized_memory_title.
        FString MemoryTitle = Def->Title;
        if (bKo)
        {
            FMemoriaMemoryLocalizedText Text;
            if (MemoriaChapterMemories::Korean(State.Id, Text)) MemoryTitle = Text.Title;
            else if (Catalog)
                if (const auto* Found = Catalog->LocalizedText.FindByPredicate([&](const auto& T) { return T.MemoryId == State.Id && T.Locale == TEXT("ko"); })) MemoryTitle = Found->Title;
        }
        // _build_loss_record. The source's body labels are English in both locales; the port translates them.
        const FString Status = bFaded ? (bKo ? TEXT("바래는 중") : TEXT("FADING")) : (bKo ? TEXT("연소") : TEXT("BURNED"));
        FMemoriaJournalRecord R;
        R.Title = R.Label = Status + TEXT(" - ") + MemoryTitle;
        R.Body = bKo ? FString::Printf(TEXT("%s\n\n기억: %s\n등급: %s\n\n세계에 남은 결과:\n%s\n\n나침반 판독:\n%s\n\n이야기의 실마리: %s"), *Status, *MemoryTitle, GradeNamesKo[Grade], *Line, *Compass, *(Rule ? Rule->Flag : TEXT("world_rewrite_") + State.Id))
                     : FString::Printf(TEXT("%s\n\nMemory: %s\nGrade: %s\n\nWorld consequence:\n%s\n\nCompass reading:\n%s\n\nStory hook: %s"), *Status, *MemoryTitle, GradeNames[Grade], *Line, *Compass, *(Rule ? Rule->Flag : TEXT("world_rewrite_") + State.Id));
        R.Color = Rule ? Rule->Color : GradeColors[Grade];
        // _fallback_art_for_grade: Grade 2 and Grade 1 share the name's picture; the rest the blank book.
        R.Art = Rule && !Rule->Art.IsEmpty() ? Rule->Art : FString(Grade >= 3 ? TEXT("res://assets/cg/generated/illustration_expansion_v2/world_rewrite_name_origin_v3.png") : TEXT("res://assets/cg/generated/ui_loss_record_blank_book_v2.png"));
        Out.Add(R);
    }
    return Out;
}
const TArray<FMemoriaArtworkSource>& ArtSources()
{
    // The pictures the journal can show on the port's route (Chapters 1 to 5) that no dialogue carries: the
    // archive plates of its events and world notes, the Sump Ledger quest, and the world rewrite's loss records
    // for the memories Arrel can hold there, with the two fallbacks.
    static const TArray<FMemoriaArtworkSource> Values = {
        {TEXT("res://assets/cg/generated/archive_verdan_memory_price_v1.png"), TEXT("/Game/Memoria/Presentation/Journal/T_archive_verdan_memory_price_v1"), false},
        {TEXT("res://assets/cg/generated/archive_belt_blank_book_v1.png"), TEXT("/Game/Memoria/Presentation/Journal/T_archive_belt_blank_book_v1"), false},
        {TEXT("res://assets/cg/generated/archive_ch4_reading_loss_v1.png"), TEXT("/Game/Memoria/Presentation/Journal/T_archive_ch4_reading_loss_v1"), false},
        {TEXT("res://assets/cg/generated/story_ch3_waystation_blank_book.png"), TEXT("/Game/Memoria/Presentation/Journal/T_story_ch3_waystation_blank_book"), false},
        {TEXT("res://assets/environment/map_canvases/map_verdan_market_canvas_v1.png"), TEXT("/Game/Memoria/Presentation/Verdan/T_Market"), true},
        {TEXT("res://assets/environment/map_canvases/map_belt_signal_yard_canvas_v1.png"), TEXT("/Game/Memoria/Presentation/Journal/T_map_belt_signal_yard_canvas_v1"), false},
        {TEXT("res://assets/environment/map_canvases/map_drift_shelter_canvas_v2.png"), TEXT("/Game/Memoria/Presentation/Journal/T_map_drift_shelter_canvas_v2"), false},
        {TEXT("res://assets/cg/generated/quest_sump_ledger_v1.png"), TEXT("/Game/Memoria/Presentation/Journal/T_quest_sump_ledger_v1"), false},
        {TEXT("res://assets/cg/generated/memory_rewrite_forest_scent_v2.png"), TEXT("/Game/Memoria/Presentation/Journal/T_memory_rewrite_forest_scent_v2"), false},
        {TEXT("res://assets/cg/generated/illustration_expansion_v3/world_rewrite_verdan_taste_v3.png"), TEXT("/Game/Memoria/Presentation/Journal/T_world_rewrite_verdan_taste_v3"), false},
        {TEXT("res://assets/cg/generated/illustration_expansion_v2/world_rewrite_campfire_song_v3.png"), TEXT("/Game/Memoria/Presentation/Journal/T_world_rewrite_campfire_song_v3"), false},
        {TEXT("res://assets/cg/generated/illustration_expansion_v2/world_rewrite_reaching_hand_v3.png"), TEXT("/Game/Memoria/Presentation/Journal/T_world_rewrite_reaching_hand_v3"), false},
        {TEXT("res://assets/cg/generated/illustration_expansion_v2/world_rewrite_first_sword_v3.png"), TEXT("/Game/Memoria/Presentation/Journal/T_world_rewrite_first_sword_v3"), false},
        {TEXT("res://assets/cg/generated/illustration_expansion_v2/world_rewrite_name_origin_v3.png"), TEXT("/Game/Memoria/Presentation/Journal/T_world_rewrite_name_origin_v3"), false},
        {TEXT("res://assets/cg/generated/illustration_expansion_v3/world_rewrite_tobias_ink_v3.png"), TEXT("/Game/Memoria/Presentation/Journal/T_world_rewrite_tobias_ink_v3"), false},
        {TEXT("res://assets/cg/generated/illustration_expansion_v2/world_rewrite_elia_anchor_v3.png"), TEXT("/Game/Memoria/Presentation/Journal/T_world_rewrite_elia_anchor_v3"), false},
        {TEXT("res://assets/cg/generated/ui_loss_record_blank_book_v2.png"), TEXT("/Game/Memoria/Presentation/Journal/T_ui_loss_record_blank_book_v2"), false},
    };
    return Values;
}
UTexture2D* LoadArt(const FString& Source)
{
    if (Source.IsEmpty()) return nullptr;
    if (UTexture2D* Texture = MemoriaNarrativeArtwork::Load(Source)) return Texture;
    if (UTexture2D* Texture = MemoriaBattleEntryArt::Load(Source)) return Texture;
    const auto* Entry = ArtSources().FindByPredicate([&](const auto& E) { return Source.Equals(E.Source, ESearchCase::CaseSensitive); });
    if (!Entry) return nullptr;
    const FString Package(Entry->Package);
    auto* Texture = LoadObject<UTexture2D>(nullptr, *(Package + TEXT(".") + FPaths::GetBaseFilename(Package)));
#if WITH_EDITOR
    // As MemoriaNarrativeArtwork::Load: a brush must not capture a compiling texture's placeholder size.
    if (Texture) FTextureCompilingManager::Get().FinishCompilation({Texture});
#endif
    return Texture;
}
}
