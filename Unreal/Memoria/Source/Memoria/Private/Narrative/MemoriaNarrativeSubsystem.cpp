#include "Narrative/MemoriaNarrativeSubsystem.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Narrative/MemoriaMaletReaction.h"
#include "Save/MemoriaRunSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

void UMemoriaNarrativeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    Collection.InitializeDependency<UMemoriaRunSubsystem>();
    Run = GetGameInstance()->GetSubsystem<UMemoriaRunSubsystem>();
    Run->OnRunReplaced.AddUObject(this, &UMemoriaNarrativeSubsystem::Reset);
    FWorldDelegates::OnWorldCleanup.AddUObject(this, &UMemoriaNarrativeSubsystem::OnWorldCleanup);
}
void UMemoriaNarrativeSubsystem::Reset()
{
    CancelMaletDelay();
    RewardCallbackWorld.Reset(); RewardCallbackRunId.Invalidate();
    RewardCompletionCount = RewardCallbackIntentCount = RewardFieldInvocationCount = 0;
    bMaletTalkCached = bMaletFirstTalkPending = bMaletCallbackConnected = false;
    ActualDelaySeconds = ActualRewardDelaySeconds = 0;
    VN.Reset(); Field.Reset(); Context.Reset(); State = EMemoriaSliceState::Idle;
    ActiveFieldAsset = nullptr; DeferredInteraction.Reset(); MaletReactionCount = 0;
    bPaused = false; EventCursor = 0; FieldInvocationCount = 0; Trace.Reset(); ++Revision;
}
void UMemoriaNarrativeSubsystem::Deinitialize()
{
    FWorldDelegates::OnWorldCleanup.RemoveAll(this);
    if (Run) Run->OnRunReplaced.RemoveAll(this);
    Reset(); VNAsset = nullptr; FieldAsset = nullptr; MaletAsset = nullptr; EncounterAsset = nullptr; RefusedAsset = nullptr; DealAsset = nullptr; RewardAsset = nullptr; Run = nullptr;
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
    bMaletCallbackConnected = true;
    Record(TEXT("verdan:enter"));
    if (!Run->State.GetFlag(TEXT("ch2_arrived")))
    {
        const bool Seen = Run->State.GetFlag(TEXT("ch2_arrival_vn_seen"));
        Run->SetStoryFlag(TEXT("ch2_arrived"), true); Record(TEXT("flag:ch2_arrived"));
        if (!Seen)
        {
            ++FieldInvocationCount; Record(TEXT("field:start:verdan_arrival"));
            ActiveFieldAsset = FieldAsset;
            Field = MakeUnique<FMemoriaFieldInterpreter>(ActiveFieldAsset->Definition, *Context);
            State = EMemoriaSliceState::Field; Field->Start(); FlushEvents(TEXT("field")); return true;
        }
        Record(TEXT("field:skip:verdan_arrival"));
    }
    else Record(TEXT("arrival:already_arrived"));
    Explore(); return true;
}

bool UMemoriaNarrativeSubsystem::InteractWithMalet()
{
    if (State != EMemoriaSliceState::Exploration || !Context || !GetWorld() ||
        !GetWorld()->GetMapName().EndsWith(TEXT("L_VerdanHost"))) return false;
    Record(TEXT("interact:Malet"));
    MaletAsset = LoadObject<UMemoriaFieldAsset>(nullptr, TEXT("/Game/Memoria/Generated/Narrative/DA_Field_MaletTasteBurned.DA_Field_MaletTasteBurned"));
    const auto Dispatch = MemoriaMaletReaction::Resolve(*Run, false, MaletAsset, bMaletTalkCached);
    for (const auto& Event : Dispatch.Events) Record(Event);
    if (Dispatch.Group.IsEmpty()) return false;
    if (!Dispatch.bReaction && Dispatch.Group == TEXT("malet_encounter"))
    {
        EncounterAsset = LoadObject<UMemoriaFieldAsset>(nullptr, TEXT("/Game/Memoria/Generated/Narrative/DA_Field_MaletEncounter.DA_Field_MaletEncounter"));
        RefusedAsset = LoadObject<UMemoriaFieldAsset>(nullptr, TEXT("/Game/Memoria/Generated/Narrative/DA_Field_MaletRefused.DA_Field_MaletRefused"));
        DealAsset = LoadObject<UMemoriaFieldAsset>(nullptr, TEXT("/Game/Memoria/Generated/Narrative/DA_Field_MaletDeal.DA_Field_MaletDeal"));
        RewardAsset = LoadObject<UMemoriaFieldAsset>(nullptr, TEXT("/Game/Memoria/Generated/Narrative/DA_Field_MaletReward.DA_Field_MaletReward"));
        if (!EncounterAsset || !RefusedAsset || !DealAsset || !RewardAsset) { State = EMemoriaSliceState::Failed; Record(TEXT("error:missing_malet_contract")); return false; }
        // npc.gd registers its cache and first-talk callback before load_and_start.
        bMaletTalkCached = true; Record(TEXT("cache:set:malet_encounter"));
        bMaletFirstTalkPending = true; Record(TEXT("callback:npc:connect"));
        Record(TEXT("request:") + Dispatch.File + TEXT("::") + Dispatch.Group);
        StartMaletField(EncounterAsset); return true;
    }
    Record(TEXT("request:") + Dispatch.File + TEXT("::") + Dispatch.Group);
    if (!Dispatch.bReaction)
    {
        DeferredInteraction = Dispatch.Group;
        Record(TEXT("development:deferred:") + Dispatch.Group);
        ++Revision; return true;
    }
    DeferredInteraction.Reset(); ActiveFieldAsset = MaletAsset;
    ++MaletReactionCount; ++FieldInvocationCount;
    Field = MakeUnique<FMemoriaFieldInterpreter>(ActiveFieldAsset->Definition, *Context);
    State = EMemoriaSliceState::Field;
    // Observed live run value at the actual Field Start boundary.
    Record(FString::Printf(TEXT("field:start:%s:heard=%s"), *Dispatch.Group, Run->GetRunSnapshot().GetFlag(MemoriaMaletReaction::Heard)?TEXT("true"):TEXT("false")));
    Field->Start(); FlushEvents(TEXT("field")); return true;
}
void UMemoriaNarrativeSubsystem::Explore()
{
    Field.Reset(); ActiveFieldAsset = nullptr; State = EMemoriaSliceState::Exploration; bPaused = false;
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
            // Reviewed original choice intent enters the unchanged Field interpreter.
            // It owns flag-before-burn and nonblocking failed-burn semantics.
            DeferredInteraction.Reset();
            Record(TEXT("select:field:") + FString::FromInt(OriginalChoice)); Field->SelectFilteredChoice(VisibleIndex);
        }
        else Field->Advance();
        FlushEvents(TEXT("field")); if (!Field->IsActive()) FinishField();
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
        View.Header = FString::Printf(TEXT("%s   %d / %d"), ActiveFieldAsset == FieldAsset ? TEXT("VN-UNSEEN FIELD FIXTURE") : ActiveFieldAsset == EncounterAsset ? TEXT("MALET / ENCOUNTER") : ActiveFieldAsset == RefusedAsset ? TEXT("MALET / REFUSAL") : ActiveFieldAsset == DealAsset ? TEXT("MALET / DEAL") : ActiveFieldAsset == RewardAsset ? TEXT("MALET / REWARD") : TEXT("MALET / MEMORY REACTION"), Index + 1, ActiveFieldAsset->Definition.Rows.Num());
        if (ActiveFieldAsset->Definition.Rows.IsValidIndex(Index))
        {
            const auto& Row = ActiveFieldAsset->Definition.Rows[Index]; Text = &Row.Text;
            for (int32 I : Field->VisibleOriginalIndices()) View.Choices.Add({I, Context->Localized(Row.Choices[I].Text)});
        }
    }
    if (State == EMemoriaSliceState::Deferred)
    {
        View.bDevelopmentStop = true;
        View.Header = TEXT("PHASE 1I / REWARD COMPLETE / DEVELOPMENT STOP");
        View.Body = TEXT("8 / 8 reward lines completed.\n_on_reward_ended observed; effects deferred before ch2_malet_done.\nNo world memory, inventory, shop or chapter effects applied.");
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

void UMemoriaNarrativeSubsystem::CancelMaletDelay()
{
    if (auto* World = DelayWorld.Get()) World->GetTimerManager().ClearTimer(MaletDelay);
    DelayWorld.Reset(); MaletDelay.Invalidate();
    if (auto* World = RewardDelayWorld.Get()) World->GetTimerManager().ClearTimer(RewardDelay);
    RewardDelayWorld.Reset(); RewardDelay.Invalidate();
}
void UMemoriaNarrativeSubsystem::OnWorldCleanup(UWorld* World, bool, bool)
{
    if (World == RewardCallbackWorld.Get())
    {
        RewardCallbackWorld.Reset(); RewardCallbackRunId.Invalidate();
        Field.Reset(); ActiveFieldAsset = nullptr; State = EMemoriaSliceState::Idle;
        DeferredInteraction.Reset(); bPaused = false; ++Revision;
    }
    if (World == DelayWorld.Get() || World == RewardDelayWorld.Get())
    {
        CancelMaletDelay();
        bMaletTalkCached = bMaletFirstTalkPending = bMaletCallbackConnected = false;
    }
}
void UMemoriaNarrativeSubsystem::StartMaletField(UMemoriaFieldAsset* Asset)
{
    DeferredInteraction.Reset(); ActiveFieldAsset = Asset;
    ++FieldInvocationCount;
    Field = MakeUnique<FMemoriaFieldInterpreter>(Asset->Definition, *Context);
    State = EMemoriaSliceState::Field;
    Record(TEXT("field:start:") + Asset->Definition.Id);
    Field->Start(); FlushEvents(TEXT("field"));
}
void UMemoriaNarrativeSubsystem::FinishField()
{
    if (ActiveFieldAsset != EncounterAsset && ActiveFieldAsset != RefusedAsset && ActiveFieldAsset != DealAsset && ActiveFieldAsset != RewardAsset) { Explore(); return; }
    const bool bRefused = ActiveFieldAsset == RefusedAsset;
    const bool bDeal = ActiveFieldAsset == DealAsset;
    const bool bReward = ActiveFieldAsset == RewardAsset;
    Field.Reset(); ActiveFieldAsset = nullptr; State = EMemoriaSliceState::Exploration;
    Record(TEXT("state:exploration"));
    if (bReward)
    {
        ++RewardCompletionCount;
        // end_dialogue switches state before synchronous dialogue_ended. No
        // frame, timer, gameplay effect or rollback is inserted at this seam.
        DeferRewardEffects(); return;
    }
    if (bDeal)
    {
        Record(TEXT("callback:deal:enter")); Record(TEXT("delay:scheduled:500"));
        RewardDelayWorld = GetWorld(); RewardDelayStarted = GetWorld()->GetTimeSeconds();
        GetWorld()->GetTimerManager().SetTimer(RewardDelay, this, &UMemoriaNarrativeSubsystem::RewardDelayElapsed, .5f, false);
        ++Revision; return;
    }
    if (bRefused)
    {
        Record(TEXT("callback:refused:enter"));
        Run->RemoveStoryFlag(TEXT("malet_deal_refused")); Record(TEXT("erase:flag:malet_deal_refused"));
        Run->RemoveStoryFlag(TEXT("talked_Malet_malet_encounter")); Record(TEXT("erase:flag:talked_Malet_malet_encounter"));
        bMaletTalkCached = false; Record(TEXT("erase:cache:malet_encounter"));
        bMaletCallbackConnected = true; Record(TEXT("callback:normal:connect"));
        Explore(); return;
    }
    // Source map listener runs first, yields at create_timer(0.3), then the
    // NPC one-shot listener marks the first talk. The gap is exploration.
    if (bMaletCallbackConnected && (Run->GetRunSnapshot().GetFlag(TEXT("malet_deal_accepted")) || Run->GetRunSnapshot().GetFlag(TEXT("malet_deal_refused"))))
    {
        bMaletCallbackConnected = false; Record(TEXT("callback:normal:disconnect"));
        Record(TEXT("delay:scheduled:300"));
        DelayWorld = GetWorld(); DelayStarted = GetWorld()->GetTimeSeconds();
        GetWorld()->GetTimerManager().SetTimer(MaletDelay, this, &UMemoriaNarrativeSubsystem::NormalDelayElapsed, .3f, false);
    }
    if (bMaletFirstTalkPending)
    {
        bMaletFirstTalkPending = false; Record(TEXT("callback:npc:first_talk"));
        Run->SetStoryFlag(TEXT("talked_Malet_malet_encounter"), true); Record(TEXT("flag:talked_Malet_malet_encounter"));
    }
    ++Revision;
}
void UMemoriaNarrativeSubsystem::NormalDelayElapsed()
{
    auto* World = DelayWorld.Get();
    if (!World || World != GetWorld() || !Context || !Run->HasActiveRun()) { CancelMaletDelay(); return; }
    ActualDelaySeconds = World->GetTimeSeconds() - DelayStarted;
    DelayWorld.Reset(); MaletDelay.Invalidate();
    UE_LOG(LogTemp, Display, TEXT("MEMORIA_MALET_DELAY_SECONDS %.6f"), ActualDelaySeconds);
    Record(TEXT("delay:elapsed:300"));
    // Source evaluates accepted at callback resumption, including failed payments.
    const bool Accepted = Run->GetRunSnapshot().GetFlag(TEXT("malet_deal_accepted"));
    Record(Accepted ? TEXT("callback:deal:connect") : TEXT("callback:refused:connect"));
    Record(Accepted ? TEXT("request:res://data/chapter2_dialogue.json::malet_deal") : TEXT("request:res://data/chapter2_dialogue.json::malet_refused"));
    StartMaletField(Accepted ? DealAsset : RefusedAsset);
}

void UMemoriaNarrativeSubsystem::RewardDelayElapsed()
{
    auto* World = RewardDelayWorld.Get();
    if (!World || World != GetWorld() || !Context || !Run->HasActiveRun()) { CancelMaletDelay(); return; }
    ActualRewardDelaySeconds = World->GetTimeSeconds() - RewardDelayStarted;
    RewardDelayWorld.Reset(); RewardDelay.Invalidate();
    UE_LOG(LogTemp, Display, TEXT("MEMORIA_MALET_REWARD_DELAY_US %lld"), int64(FMath::RoundToDouble(ActualRewardDelaySeconds*1000000.0)));
    Record(TEXT("delay:elapsed:500"));
    RewardCallbackWorld = World; RewardCallbackRunId = Run->GetRunSnapshot().RunId;
    Record(TEXT("callback:reward:connect"));
    Record(TEXT("request:res://data/chapter2_dialogue.json::malet_reward"));
    ++RewardFieldInvocationCount; StartMaletField(RewardAsset);
}

void UMemoriaNarrativeSubsystem::DeferRewardEffects()
{
    if (!RewardCallbackWorld.IsValid() || RewardCallbackWorld.Get() != GetWorld() ||
        !Run->HasActiveRun() || RewardCallbackRunId != Run->GetRunSnapshot().RunId)
    {
        RewardCallbackWorld.Reset(); RewardCallbackRunId.Invalidate();
        State = EMemoriaSliceState::Idle; ++Revision; return;
    }
    // Only callback intent is authorized. There is no executable reward effect
    // handler or continuation to resume from this development stop.
    ++RewardCallbackIntentCount; Record(TEXT("callback:reward:enter"));
    DeferredInteraction = TEXT("_on_reward_ended:before:ch2_malet_done");
    Record(TEXT("development:deferred:") + DeferredInteraction);
    State = EMemoriaSliceState::Deferred; ++Revision;
    // Retain weak ownership of the stopped boundary for cleanup on world exit.
}
