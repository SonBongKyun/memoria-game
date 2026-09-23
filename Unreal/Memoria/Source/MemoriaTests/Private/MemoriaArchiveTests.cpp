#include "MemoriaPotionEvidence.h"
#include "Presentation/MemoriaArchiveView.h"
#include "Presentation/MemoriaArchiveWidget.h"
#include "Blueprint/WidgetTree.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
using namespace MemoriaPotionEvidence;
using Val = TSharedPtr<FJsonValue>;
TArray<Val> ArchiveCases(const TCHAR* Name)
{
    FString Text; Val Parsed;
    FFileHelper::LoadFileToString(Text, *(FPaths::ProjectDir() /
        TEXT("../../docs/unreal-migration/fixtures/archive") / Name));
    FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Parsed);
    return Parsed ? Parsed->AsArray() : TArray<Val>();
}
Obj ArchiveCase(const TArray<Val>& Cases, const FString& Id)
{
    for (const auto& Case : Cases)
        if (Case->AsObject()->GetStringField(TEXT("id")) == Id) return Case->AsObject();
    return nullptr;
}
bool SetupArchive(FAutomationTestBase& Test, UMemoriaRunSubsystem& Run, const Obj& Input)
{
    if (!Test.TestTrue(TEXT("Source starting catalog"), Run.BeginStartingMemoryRun() == EMemoriaMemoryResult::Success)) return false;
    auto State = Run.GetRunSnapshot();
    State.CurrentLocale = Input->GetStringField(TEXT("locale"));
    State.CurrentChapter = 1; State.Player.bEliaWithParty = true;
    if (!Test.TestTrue(TEXT("Fixture locale restored before source burns"),
        Run.RestoreRun(State, Run.GetPlayerMemory()->GetDefinitions(), Run.GetPlayerMemory()->GetSnapshot(),
            Run.GetWorldCognition()->GetSnapshot()) == EMemoriaMemoryResult::Success)) return false;
    if (Input->GetBoolField(TEXT("states")))
    {
        if (!Test.TestTrue(TEXT("Source sensory burn"), Run.BurnMemory(TEXT("sense_forest_smell")) == EMemoriaMemoryResult::Success)) return false;
        if (!Test.TestTrue(TEXT("Source relational burn"), Run.BurnMemory(TEXT("rel_hand_reaching")) == EMemoriaMemoryResult::Success)) return false;
    }
    auto Memory = Run.GetPlayerMemory()->GetSnapshot();
    auto Definitions = Run.GetPlayerMemory()->GetDefinitions();
    if (Input->GetBoolField(TEXT("states")))
    {
        auto* Faded = Memory.Owned.FindByPredicate([](const auto& M){return M.Id == TEXT("daily_campfire_song");});
        auto* Eroding = Memory.Owned.FindByPredicate([](const auto& M){return M.Id == TEXT("identity_first_sword");});
        if (!Test.TestNotNull(TEXT("Synthetic faded UI fixture exists"), Faded) ||
            !Test.TestNotNull(TEXT("Synthetic erosion UI fixture exists"), Eroding)) return false;
        Faded->bFaded = true; Eroding->Erosion = 3;
    }
    if (Input->GetBoolField(TEXT("empty")))
    {
        Memory.Owned.Reset(); Definitions.Reset();
    }
    return Test.TestTrue(TEXT("Exact source UI input restored through existing DTO contract"),
        Run.RestoreRun(State, Definitions, Memory, Run.GetWorldCognition()->GetSnapshot()) == EMemoriaMemoryResult::Success);
}
void WriteArchiveEvidence(const FString& Name, const Obj& Object)
{
    const FString Dir = FPaths::ProjectSavedDir() / TEXT("Validation/Archive1");
    IFileManager::Get().MakeDirectory(*Dir, true);
    FFileHelper::SaveStringToFile(Canon(Object) + TEXT("\n"), *(Dir / Name),
        FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}
Obj ProjectArchive(const FMemoriaArchiveView& View)
{
    auto Object = MakeShared<FJsonObject>();
    Object->SetStringField(TEXT("title"), View.Title); Object->SetStringField(TEXT("empty"), View.EmptyText);
    Object->SetNumberField(TEXT("owned"), View.OwnedCount);
    Object->SetNumberField(TEXT("remaining"), View.RemainingCount);
    Object->SetNumberField(TEXT("burned"), View.BurnedCount);
    TArray<Val> Filters, Rows;
    for (const auto& Filter : View.FilterLabels) Filters.Add(MakeShared<FJsonValueString>(Filter));
    for (const auto& R : View.Rows)
    {
        auto Row = MakeShared<FJsonObject>();
        Row->SetStringField(TEXT("id"), R.Id); Row->SetStringField(TEXT("title"), R.Title);
        Row->SetStringField(TEXT("description"), R.Description); Row->SetStringField(TEXT("effect"), R.StoryEffect);
        Row->SetStringField(TEXT("state_label"), R.StateLabel); Row->SetStringField(TEXT("grade_label"), R.GradeLabel);
        Row->SetNumberField(TEXT("grade"), R.Grade); Row->SetNumberField(TEXT("burn_power"), R.BurnPower);
        Row->SetNumberField(TEXT("erosion_ratio"), R.Erosion);
        Row->SetBoolField(TEXT("burned"), R.bBurned); Row->SetBoolField(TEXT("residue"), R.bResidue);
        Row->SetBoolField(TEXT("faded"), R.bFaded);
        Rows.Add(MakeShared<FJsonValueObject>(Row));
    }
    Object->SetArrayField(TEXT("filters"), Filters); Object->SetArrayField(TEXT("rows"), Rows);
    return Object;
}
bool ClickArchiveButton(UMemoriaArchiveWidget& Widget, int32 Role, int32 Index)
{
    TArray<UWidget*> Children; Widget.WidgetTree->GetAllWidgets(Children);
    for (auto* Child : Children)
        if (auto* Button = Cast<UMemoriaArchiveButton>(Child); Button && Button->Role == Role && Button->Index == Index)
        {
            Button->OnClicked.Broadcast();
            return true;
        }
    return false;
}
}
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FMemoriaArchiveSource, "Memoria.Archive.Source",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
void FMemoriaArchiveSource::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{
    for (const auto& Case : ArchiveCases(TEXT("contract_inputs.v1.json")))
    {
        const FString Id = Case->AsObject()->GetStringField(TEXT("id"));
        Names.Add(Id); Commands.Add(Id);
    }
}
bool FMemoriaArchiveSource::RunTest(const FString& Id)
{
    const auto Input = ArchiveCase(ArchiveCases(TEXT("contract_inputs.v1.json")), Id);
    const auto Gold = ArchiveCase(ArchiveCases(TEXT("contract_expected.v1.json")), Id);
    if (!TestTrue(TEXT("Executed source input and output exist"), Input.IsValid() && Gold.IsValid())) return false;
    TStrongObjectPtr<UGameInstance> Game(NewObject<UGameInstance>()); Game->Init();
    auto* Run = Game->GetSubsystem<UMemoriaRunSubsystem>();
    if (!SetupArchive(*this, *Run, Input)) { Game->Shutdown(); return false; }
    const auto Before = Full(*Run);
    const int32 Grade = Input->GetIntegerField(TEXT("grade"));
    const auto View = MemoriaArchive::Build(*Run, Grade);
    TestEqual(TEXT("Source title"), View.Title, Gold->GetStringField(TEXT("title")));
    TestEqual(TEXT("Source empty-detail prompt"), View.EmptyText, Gold->GetStringField(TEXT("empty")));
    TestEqual(TEXT("Locale derives from authoritative run"), View.bKo, Input->GetStringField(TEXT("locale")) == TEXT("ko"));
    const auto& Counts = Gold->GetArrayField(TEXT("counts"));
    TestEqual(TEXT("Source all-owned count"), View.OwnedCount, static_cast<int32>(Counts[0]->AsNumber()));
    TestEqual(TEXT("Source held count excludes burned and faded"), View.RemainingCount, static_cast<int32>(Counts[1]->AsNumber()));
    TestEqual(TEXT("Source burned-history count"), View.BurnedCount, static_cast<int32>(Counts[2]->AsNumber()));
    const auto& Filters = Gold->GetArrayField(TEXT("filters"));
    TestEqual(TEXT("All and five source grade filters"), View.FilterLabels.Num(), Filters.Num());
    for (int32 I = 0; I < FMath::Min(View.FilterLabels.Num(), Filters.Num()); ++I)
        TestEqual(TEXT("Source filter label and rank direction"), View.FilterLabels[I], Filters[I]->AsString());
    const auto& Rows = Gold->GetArrayField(TEXT("rows"));
    TestEqual(TEXT("Owned-order grade projection includes burned and faded"), View.Rows.Num(), Rows.Num());
    for (int32 I = 0; I < FMath::Min(View.Rows.Num(), Rows.Num()); ++I)
    {
        const auto& Row = View.Rows[I]; const auto Expected = Rows[I]->AsObject();
        const FString Prefix = Row.Id + TEXT(" / ");
        TestEqual(Prefix + TEXT("source order"), Row.Id, Expected->GetStringField(TEXT("id")));
        TestEqual(Prefix + TEXT("title"), Row.Title, Expected->GetStringField(TEXT("title")));
        TestEqual(Prefix + TEXT("description"), Row.Description, Expected->GetStringField(TEXT("description")));
        TestEqual(Prefix + TEXT("effect"), Row.StoryEffect, Expected->GetStringField(TEXT("effect")));
        TestEqual(Prefix + TEXT("card state precedence"), Row.StateLabel, Expected->GetStringField(TEXT("state_label")));
        TestEqual(Prefix + TEXT("grade label"), Row.GradeLabel, Expected->GetStringField(TEXT("grade_label")));
        TestEqual(Prefix + TEXT("raw grade"), Row.Grade, Expected->GetIntegerField(TEXT("grade")));
        TestEqual(Prefix + TEXT("burn power"), Row.BurnPower, static_cast<int64>(Expected->GetNumberField(TEXT("burn_power"))));
        TestEqual(Prefix + TEXT("burned"), Row.bBurned, Expected->GetBoolField(TEXT("burned")));
        TestEqual(Prefix + TEXT("residue"), Row.bResidue, Expected->GetBoolField(TEXT("residue")));
        TestEqual(Prefix + TEXT("faded"), Row.bFaded, Expected->GetBoolField(TEXT("faded")));
        TestTrue(Prefix + TEXT("source erosion ratio"), FMath::IsNearlyEqual(Row.Erosion, Expected->GetNumberField(TEXT("erosion_ratio")), 1e-12));
        const auto& Accent = Expected->GetArrayField(TEXT("accent"));
        TestTrue(Prefix + TEXT("source grade color"), Row.Accent.Equals(FLinearColor(
            Accent[0]->AsNumber(), Accent[1]->AsNumber(), Accent[2]->AsNumber(), Accent[3]->AsNumber()), 1e-6f));
    }
    for (int32 I = -1; I < 5; ++I) MemoriaArchive::Build(*Run, I);
    TestEqual(TEXT("Read and filtering preserve run, memory, derived state and world"), Canon(Full(*Run)), Canon(Before));
    auto Evidence = MakeShared<FJsonObject>(); Evidence->SetObjectField(TEXT("actual"), ProjectArchive(View));
    Evidence->SetObjectField(TEXT("source"), Gold); Evidence->SetObjectField(TEXT("before"), Before);
    Evidence->SetObjectField(TEXT("after"), Full(*Run)); WriteArchiveEvidence(TEXT("source_") + Id + TEXT(".json"), Evidence);
    Game->Shutdown(); return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMemoriaArchiveWidgetReadOnly, "Memoria.Archive.WidgetReadOnly",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMemoriaArchiveWidgetReadOnly::RunTest(const FString&)
{
    const auto Input = ArchiveCase(ArchiveCases(TEXT("contract_inputs.v1.json")), TEXT("states_en"));
    if (!TestTrue(TEXT("Source states fixture exists"), Input.IsValid())) return false;
    TStrongObjectPtr<UGameInstance> Game(NewObject<UGameInstance>()); Game->Init();
    auto* Run = Game->GetSubsystem<UMemoriaRunSubsystem>();
    if (!SetupArchive(*this, *Run, Input)) { Game->Shutdown(); return false; }
    const auto Before = Full(*Run);
    TStrongObjectPtr<UMemoriaArchiveWidget> Widget(CreateWidget<UMemoriaArchiveWidget>(Game.Get()));
    if (!TestNotNull(TEXT("Real native archive widget"), Widget.Get())) { Game->Shutdown(); return false; }
    Widget->TakeWidget(); Widget->BindRun(Run);
    TestEqual(TEXT("Initial selection is owned source order"), Widget->GetSelectedId(), FString(TEXT("sense_forest_smell")));
    TestTrue(TEXT("Burned memory remains visible"), Widget->VisibleText().Contains(TEXT("BURNED")));
    TestTrue(TEXT("Actual card click"), ClickArchiveButton(*Widget, 0, 5));
    TestEqual(TEXT("Card selection uses stable memory identity"), Widget->GetSelectedId(), FString(TEXT("identity_first_sword")));
    TestTrue(TEXT("Selected detail appears"), Widget->VisibleText().Contains(
        Run->GetPlayerMemory()->GetDefinitions()[5].Description));
    TestTrue(TEXT("Actual grade filter click"), ClickArchiveButton(*Widget, 1, 2));
    TestEqual(TEXT("Filtered raw grade retained"), Widget->GetFilter(), 2);
    TestEqual(TEXT("Relational source rows"), Widget->GetView().Rows.Num(), 1);
    TestEqual(TEXT("Filter replaces unavailable selection"), Widget->GetSelectedId(), FString(TEXT("rel_hand_reaching")));
    Widget->Navigate(1);
    TestEqual(TEXT("One-item list navigation remains valid"), Widget->GetSelectedId(), FString(TEXT("rel_hand_reaching")));
    Widget->CycleFilter(1);
    TestEqual(TEXT("Next grade selects identity shelf"), Widget->GetFilter(), 3);
    TestEqual(TEXT("Next grade selection"), Widget->GetSelectedId(), FString(TEXT("identity_first_sword")));
    Widget->SetFilter(-1); Widget->Select(6); Widget->BindRun(Run);
    TestEqual(TEXT("Redraw preserves still-valid selected identity"), Widget->GetSelectedId(), FString(TEXT("core_name_origin")));
    Widget->Select(-1); Widget->SetFilter(99);
    TestEqual(TEXT("Invalid selection and filter are inert"), Widget->GetSelectedId(), FString(TEXT("core_name_origin")));
    TestEqual(TEXT("Invalid filter preserves all view"), Widget->GetFilter(), -1);
    int32 CloseCount = 0; Widget->OnClose.BindLambda([&](){ ++CloseCount; });
    TestTrue(TEXT("Actual close button"), ClickArchiveButton(*Widget, 2, 0));
    TestEqual(TEXT("Close is a presentation request"), CloseCount, 1);
    TestEqual(TEXT("Selection, filtering, redraw and close never mutate gameplay"), Canon(Full(*Run)), Canon(Before));
    auto Evidence = MakeShared<FJsonObject>(); Evidence->SetObjectField(TEXT("before"), Before);
    Evidence->SetObjectField(TEXT("after"), Full(*Run)); Evidence->SetStringField(TEXT("visible"), Widget->VisibleText());
    Evidence->SetStringField(TEXT("selected"), Widget->GetSelectedId()); Evidence->SetNumberField(TEXT("close_requests"), CloseCount);
    WriteArchiveEvidence(TEXT("widget_read_only.json"), Evidence);
    Widget->OnClose.Unbind(); Widget->BindRun(nullptr); Game->Shutdown(); return !HasAnyErrors();
}
#endif
