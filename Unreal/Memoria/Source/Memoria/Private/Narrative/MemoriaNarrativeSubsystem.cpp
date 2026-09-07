#include "Narrative/MemoriaNarrativeSubsystem.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Save/MemoriaRunSaveGame.h"
#include "Kismet/GameplayStatics.h"

void UMemoriaNarrativeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    Collection.InitializeDependency<UMemoriaRunSubsystem>();
    Run = GetGameInstance()->GetSubsystem<UMemoriaRunSubsystem>();
    Run->OnRunReplaced.AddUObject(this, &UMemoriaNarrativeSubsystem::Reset);
}
void UMemoriaNarrativeSubsystem::Reset()
{
    VN.Reset(); Field.Reset(); Context.Reset(); State = EMemoriaSliceState::Idle;
    bPaused = false; EventCursor = 0; FieldInvocationCount = 0; Trace.Reset(); ++Revision;
}
void UMemoriaNarrativeSubsystem::Deinitialize()
{
    if (Run) Run->OnRunReplaced.RemoveAll(this);
    Reset(); VNAsset = nullptr; FieldAsset = nullptr; Run = nullptr;
    Super::Deinitialize();
}
bool UMemoriaNarrativeSubsystem::LoadContracts()
{
    VNAsset = LoadObject<UMemoriaVNAsset>(nullptr, TEXT("/Game/Memoria/Generated/Narrative/DA_VN_Ch2MarketArrival.DA_VN_Ch2MarketArrival"));
    FieldAsset = LoadObject<UMemoriaFieldAsset>(nullptr, TEXT("/Game/Memoria/Generated/Narrative/DA_Field_VerdanArrival.DA_Field_VerdanArrival"));
    return VNAsset && FieldAsset && VNAsset->Definition.Id.Equals(TEXT("ch2_market_arrival"), ESearchCase::CaseSensitive)
        && FieldAsset->Definition.Id.Equals(TEXT("verdan_arrival"), ESearchCase::CaseSensitive);
}
void UMemoriaNarrativeSubsystem::Record(const FString& Event)
{
    Trace.Add(Event); UE_LOG(LogTemp, Display, TEXT("MEMORIA_SLICE %s"), *Event);
}
void UMemoriaNarrativeSubsystem::FlushEvents(const FString& Dialect)
{
    for (; EventCursor < Context->Events.Num(); ++EventCursor) Record(Dialect + TEXT(":") + Context->Events[EventCursor]);
    ++Revision;
}
bool UMemoriaNarrativeSubsystem::StartDevelopmentVN()
{
    if (!LoadContracts() || Run->BeginStartingMemoryRun() != EMemoriaMemoryResult::Success) return false;
    Context = MakeUnique<FMemoriaNarrativeContext>(Run->State, *Run->GetPlayerMemory());
    VN = MakeUnique<FMemoriaVNInterpreter>(VNAsset->Definition, *Context);
    State = EMemoriaSliceState::VN; Record(TEXT("vn:start:ch2_market_arrival"));
    VN->Play(); AfterVN(); return true;
}
bool UMemoriaNarrativeSubsystem::StartUnseenFieldFixture()
{
    if (!LoadContracts() || Run->BeginStartingMemoryRun() != EMemoriaMemoryResult::Success) return false;
    Context = MakeUnique<FMemoriaNarrativeContext>(Run->State, *Run->GetPlayerMemory());
    Record(TEXT("fixture:vn_unseen")); return EnterVerdan();
}
bool UMemoriaNarrativeSubsystem::EnterVerdan()
{
    // Source: verdan_market.gd _ready guard and the two _start_ch2_* methods.
    if (!Run->HasActiveRun() || !Context || !LoadContracts()) return false;
    Record(TEXT("verdan:enter"));
    if (!Run->State.GetFlag(TEXT("ch2_arrived")))
    {
        const bool Seen = Run->State.GetFlag(TEXT("ch2_arrival_vn_seen"));
        Run->SetStoryFlag(TEXT("ch2_arrived"), true); Record(TEXT("flag:ch2_arrived"));
        if (!Seen)
        {
            ++FieldInvocationCount; Record(TEXT("field:start:verdan_arrival"));
            Field = MakeUnique<FMemoriaFieldInterpreter>(FieldAsset->Definition, *Context);
            State = EMemoriaSliceState::Field; Field->Start(); FlushEvents(TEXT("field")); return true;
        }
        Record(TEXT("field:skip:verdan_arrival"));
    }
    else Record(TEXT("arrival:already_arrived"));
    Explore(); return true;
}
void UMemoriaNarrativeSubsystem::Explore()
{
    Field.Reset(); State = EMemoriaSliceState::Exploration; bPaused = false;
    Record(TEXT("exploration:ready")); ++Revision;
}
void UMemoriaNarrativeSubsystem::AfterVN()
{
    FlushEvents(TEXT("vn"));
    if (!Context->RequestedMap.IsEmpty())
    {
        if (Context->RequestedMap.Equals(TEXT("res://scenes/maps/verdan_market.tscn"), ESearchCase::CaseSensitive))
        {
            State = EMemoriaSliceState::Travelling;
            Record(TEXT("travel:verdan"));
            UGameplayStatics::OpenLevel(this, FName(VerdanMap));
        }
        else { State = EMemoriaSliceState::Failed; Record(TEXT("error:unsupported_map")); }
    }
    else if (!VN->ExportContinuation().bActive)
    {
        if (!VN->ConsumePendingOrQueue()) State = EMemoriaSliceState::Idle;
        else FlushEvents(TEXT("vn"));
    }
}
void UMemoriaNarrativeSubsystem::Confirm(int32 OriginalChoice)
{
    if (bPaused) { bPaused = false; ++Revision; return; }
    if (State == EMemoriaSliceState::VN && VN)
    {
        const auto Choices = VN->VisibleOriginalIndices();
        const auto Cursor = VN->ExportContinuation().Current.OriginalIndex;
        if (VNAsset->Definition.Steps.IsValidIndex(Cursor) && VNAsset->Definition.Steps[Cursor].bChoicesPresent)
        {
            if (!Choices.Contains(OriginalChoice)) return;
            Record(TEXT("select:vn:") + FString::FromInt(OriginalChoice)); VN->SelectOriginalChoice(OriginalChoice);
        }
        else VN->Advance();
        AfterVN();
    }
    else if (State == EMemoriaSliceState::Field && Field)
    {
        const auto& Choices = Field->VisibleOriginalIndices();
        if (!Choices.IsEmpty())
        {
            const int32 VisibleIndex = Choices.IndexOfByKey(OriginalChoice);
            if (VisibleIndex == INDEX_NONE) return;
            Record(TEXT("select:field:") + FString::FromInt(OriginalChoice)); Field->SelectFilteredChoice(VisibleIndex);
        }
        else Field->Advance();
        FlushEvents(TEXT("field")); if (!Field->IsActive()) Explore();
    }
}
void UMemoriaNarrativeSubsystem::Back()
{
    // PauseMenu._can_open_pause_menu permits VN; Field dialogue cannot cancel.
    if (State == EMemoriaSliceState::VN) { bPaused = !bPaused; ++Revision; }
}
FMemoriaNarrativeView UMemoriaNarrativeSubsystem::GetView() const
{
    FMemoriaNarrativeView View; View.bPaused = bPaused;
    if (!Context) return View;
    const FMemoriaNarrativeText* Text = nullptr;
    if (State == EMemoriaSliceState::VN && VN)
    {
        int32 Index = VN->ExportContinuation().Current.OriginalIndex;
        View.Header = FString::Printf(TEXT("CH2 ARRIVAL / VN   %d / %d"), Index + 1, VNAsset->Definition.Steps.Num());
        if (VNAsset->Definition.Steps.IsValidIndex(Index))
        {
            const auto& Step = VNAsset->Definition.Steps[Index]; Text = &Step.Text;
            for (int32 I : VN->VisibleOriginalIndices()) View.Choices.Add({I, Context->Localized(Step.Choices[I].Text)});
        }
    }
    else if (State == EMemoriaSliceState::Field && Field)
    {
        int32 Index = Field->OriginalIndex();
        View.Header = FString::Printf(TEXT("VN-UNSEEN FIELD FIXTURE   %d / %d"), Index + 1, FieldAsset->Definition.Rows.Num());
        if (FieldAsset->Definition.Rows.IsValidIndex(Index))
        {
            const auto& Row = FieldAsset->Definition.Rows[Index]; Text = &Row.Text;
            for (int32 I : Field->VisibleOriginalIndices()) View.Choices.Add({I, Context->Localized(Row.Choices[I].Text)});
        }
    }
    if (Text) { View.Speaker = Text->Speaker; View.Narration = Context->Localized(*Text, true); View.Body = Context->Localized(*Text); }
    return View;
}
FMemoriaVNContinuation UMemoriaNarrativeSubsystem::GetContinuation() const
{ return VN ? VN->ExportContinuation() : FMemoriaVNContinuation(); }
UMemoriaRunSaveGame* UMemoriaNarrativeSubsystem::CaptureSave() const
{
    if (!Run->HasActiveRun() || State != EMemoriaSliceState::VN || !VN) return nullptr;
    auto* Save = NewObject<UMemoriaRunSaveGame>();
    Save->Run = Run->GetRunSnapshot(); Save->ContentRevision = Save->Run.ContentRevision;
    Save->MemoryDefinitions = Run->GetPlayerMemory()->GetDefinitions(); Save->PlayerMemory = Run->GetPlayerMemory()->GetSnapshot();
    Save->SceneFlow = VN->ExportContinuation(); return Save;
}
bool UMemoriaNarrativeSubsystem::PrepareRestore(const UMemoriaRunSaveGame& Save)
{
    FString Error;
    if (!Save.ValidateHeader(Error) || !LoadContracts()) return false;
    // Validate the full bounded replacement before disturbing the active host.
    auto* Candidate = NewObject<UMemoriaPlayerMemoryDomain>(this);
    if (Candidate->Restore(Save.MemoryDefinitions, Save.PlayerMemory) != EMemoriaMemoryResult::Success) return false;
    auto Snapshot = Save.Run; FMemoriaNarrativeContext CheckContext(Snapshot, *Candidate);
    FMemoriaVNInterpreter Check(VNAsset->Definition, CheckContext);
    if (!Check.PrepareResume(Save.SceneFlow)) return false;
    if (Run->RestoreRun(Save.Run, Save.MemoryDefinitions, Save.PlayerMemory) != EMemoriaMemoryResult::Success) return false;
    Context = MakeUnique<FMemoriaNarrativeContext>(Run->State, *Run->GetPlayerMemory());
    VN = MakeUnique<FMemoriaVNInterpreter>(VNAsset->Definition, *Context);
    VN->PrepareResume(Save.SceneFlow); State = EMemoriaSliceState::VN; ++Revision; return true;
}
bool UMemoriaNarrativeSubsystem::ResumePrepared()
{
    if (!VN || !VN->ConsumePendingOrQueue()) return false;
    State = EMemoriaSliceState::VN; AfterVN(); return true;
}
