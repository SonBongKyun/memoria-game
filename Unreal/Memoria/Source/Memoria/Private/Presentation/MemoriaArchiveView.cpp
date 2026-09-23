#include "Presentation/MemoriaArchiveView.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Domain/MemoriaPlayerMemoryDomain.h"

namespace
{
// BEGIN EXECUTED ARCHIVE TEXT
// Generated from executed source archive UI; do not hand-edit.
static const TCHAR* SourceTitle[2] = {TEXT("ARREL'S ARCHIVE"), TEXT("\uc544\ub810\uc758 \uc11c\uace0")};
static const TCHAR* SourceEmpty[2] = {TEXT("Select a memory..."), TEXT("\uae30\uc5b5\uc744 \uace0\ub974\uc138\uc694...")};
static const TCHAR* SourceFilters[2][6] = {
    {TEXT("All Memories"), TEXT("Grade 5, Sensory"), TEXT("Grade 4, Daily"), TEXT("Grade 3, Relational"), TEXT("Grade 2, Identity"), TEXT("Grade 1, Core")},
    {TEXT("\uc804\uccb4 \uae30\uc5b5"), TEXT("5\ub4f1\uae09 \u00b7 \uac10\uac01"), TEXT("4\ub4f1\uae09 \u00b7 \uc77c\uc0c1"), TEXT("3\ub4f1\uae09 \u00b7 \uad00\uacc4"), TEXT("2\ub4f1\uae09 \u00b7 \uc815\uccb4\uc131"), TEXT("1\ub4f1\uae09 \u00b7 \ud575\uc2ec")},
};
static const TCHAR* SourceStates[2][4] = {
    {TEXT("INTACT"), TEXT("BURNED"), TEXT("RESIDUE"), TEXT("FADED")},
    {TEXT("\uc628\uc804\ud568"), TEXT("\uc5f0\uc18c\ub428"), TEXT("\uc794\uc874"), TEXT("\ud750\ub824\uc9d0")},
};
static const TCHAR* SourceEroding[2] = {TEXT("ERODING %d%%"), TEXT("\uce68\uc2dd %d%%")};
static const FLinearColor SourceAccents[5] = {
    FLinearColor(0.50000000f, 0.50000000f, 0.44999999f, 1.00000000f),
    FLinearColor(0.55000001f, 0.50000000f, 0.34999999f, 1.00000000f),
    FLinearColor(0.40000001f, 0.50000000f, 0.60000002f, 1.00000000f),
    FLinearColor(0.60000002f, 0.44999999f, 0.55000001f, 1.00000000f),
    FLinearColor(0.69999999f, 0.55000001f, 0.30000001f, 1.00000000f),
};
// END EXECUTED ARCHIVE TEXT
}
FMemoriaArchiveView MemoriaArchive::Build(const UMemoriaRunSubsystem& Run, int32 FilterGrade)
{
    FMemoriaArchiveView View;
    const auto RunState = Run.GetRunSnapshot();
    View.bKo = RunState.CurrentLocale == TEXT("ko");
    const int32 Locale = View.bKo ? 1 : 0;
    View.Title = SourceTitle[Locale];
    View.EmptyText = SourceEmpty[Locale];
    for (int32 I = 0; I < 6; ++I) View.FilterLabels.Add(SourceFilters[Locale][I]);
    if (!Run.HasActiveRun() || !Run.GetPlayerMemory()) return View;
    const auto* Memory = Run.GetPlayerMemory();
    const auto Snapshot = Memory->GetSnapshot();
    const auto Definitions = Memory->GetDefinitions();
    const auto* Catalog = LoadObject<UMemoriaMemoryCatalog>(nullptr,
        TEXT("/Game/Memoria/Generated/Memory/DA_StartingMemoryCatalog.DA_StartingMemoryCatalog"));
    View.OwnedCount = Snapshot.Owned.Num();
    View.BurnedCount = Snapshot.BurnedHistory.Num();
    for (const auto& State : Snapshot.Owned)
    {
        if (!State.bBurned && !State.bFaded) ++View.RemainingCount;
        const auto* Definition = Definitions.FindByPredicate([&](const auto& D){ return D.Id == State.Id; });
        if (!Definition) continue;
        const int32 Grade = static_cast<int32>(Definition->RawGrade);
        if (Grade < 0 || Grade >= 5 || (FilterGrade >= 0 && Grade != FilterGrade)) continue;
        FMemoriaArchiveRow Row;
        Row.Id = State.Id; Row.Title = Definition->Title; Row.Description = Definition->Description;
        Row.StoryEffect = Definition->StoryEffect; Row.Grade = Grade;
        Row.GradeLabel = SourceFilters[Locale][Grade + 1]; Row.Accent = SourceAccents[Grade];
        Row.bBurned = State.bBurned; Row.bResidue = State.bResidue; Row.bFaded = State.bFaded;
        Row.BurnPower = Definition->BurnPower;
        Row.Erosion = Row.BurnPower > 0 ? FMath::Clamp(double(State.Erosion) / double(Row.BurnPower), 0.0, 1.0) : 0.0;
        if (Catalog)
            if (const auto* Text = Catalog->LocalizedText.FindByPredicate([&](const auto& T)
                { return T.MemoryId == Row.Id && T.Locale == RunState.CurrentLocale; }))
            {
                Row.Title = Text->Title; Row.Description = Text->Description;
                if (Text->bHasStoryEffect) Row.StoryEffect = Text->StoryEffect;
            }
        // Use the source card's state priority. Its legacy detail panel labels
        // all unburned rows INTACT, including faded rows; the card is authoritative here.
        if (Row.bBurned) Row.StateLabel = SourceStates[Locale][Row.bResidue ? 2 : 1];
        else if (Row.bFaded) Row.StateLabel = SourceStates[Locale][3];
        else if (State.Erosion > 0)
            Row.StateLabel = FString(SourceEroding[Locale]).Replace(TEXT("%d"), *FString::FromInt(static_cast<int32>(Row.Erosion * 100.0))).Replace(TEXT("%%"), TEXT("%"));
        else Row.StateLabel = SourceStates[Locale][0];
        View.Rows.Add(MoveTemp(Row));
    }
    return View;
}
