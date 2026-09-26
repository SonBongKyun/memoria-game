#include "Narrative/MemoriaNarrativeSubsystem.h"
#include "Presentation/MemoriaNarrativeArtwork.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Narrative/MemoriaMaletReaction.h"
#include "Save/MemoriaRunSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Save/MemoriaCheckpointSubsystem.h"
#include "Framework/MemoriaCoordinates.h"
#include "Battle/MemoriaBattleEntrySubsystem.h"
#include "Narrative/MemoriaVerdanStory.h"
#include "Interaction/MemoriaStoryPointActor.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

void UMemoriaNarrativeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    Collection.InitializeDependency<UMemoriaRunSubsystem>();
    Collection.InitializeDependency<UMemoriaShopSubsystem>();
    GetGameInstance()->GetSubsystem<UMemoriaShopSubsystem>()->OnChanged.AddUObject(this,&UMemoriaNarrativeSubsystem::ShopChanged);
    Run = GetGameInstance()->GetSubsystem<UMemoriaRunSubsystem>();
    Run->OnRunReplaced.AddUObject(this, &UMemoriaNarrativeSubsystem::Reset);
    FWorldDelegates::OnWorldCleanup.AddUObject(this, &UMemoriaNarrativeSubsystem::OnWorldCleanup);
}
void UMemoriaNarrativeSubsystem::ShopChanged()
{
    if(State != EMemoriaSliceState::Deferred) return;
    const auto V=GetGameInstance()->GetSubsystem<UMemoriaShopSubsystem>()->GetView();
    if(V.bClosed) { DeferredInteraction=TEXT("before:chapter_transition_delay"); Record(TEXT("shop:closed")); Record(TEXT("development:deferred:before:chapter_transition_delay")); }
    ++Revision;
}
void UMemoriaNarrativeSubsystem::Reset()
{
    bPendingVerdanReentry=false; RevisitRunId.Invalidate(); RevisitWorld.Reset();
    bCheckpointScreen = bCheckpointLoadFailed = false; CheckpointWorld.Reset();
    SeedObservations.Reset(); PresentedSeedObservation = INDEX_NONE;
    FirebombObservations.Reset(); PresentedFirebombObservation=INDEX_NONE;
    PotionObservations.Reset(); PresentedPotionObservation = INDEX_NONE; PotionToast.Reset(); RewardToasts.Reset(); AntidoteObservations.Reset(); PresentedAntidoteObservation=INDEX_NONE;
    CancelMaletDelay();
    RewardCallbackWorld.Reset(); RewardCallbackRunId.Invalidate();
    RewardCompletionCount = RewardCallbackIntentCount = RewardFieldInvocationCount = 0;
    bMaletTalkCached = bMaletFirstTalkPending = bMaletCallbackConnected = false;
    ActualDelaySeconds = ActualRewardDelaySeconds = 0;
    VN.Reset(); Field.Reset(); Context.Reset(); State = EMemoriaSliceState::Idle;
    ActiveFieldAsset = nullptr; DeferredInteraction.Reset(); MaletReactionCount = 0;
    ArmedStoryBeats.Reset(); StoryWorld.Reset(); StoryAsset = nullptr;
    bPaused = false; EventCursor = 0; FieldInvocationCount = 0; Trace.Reset(); ++Revision;
}
void UMemoriaNarrativeSubsystem::Deinitialize()
{
    if(auto* Shop=GetGameInstance()->GetSubsystem<UMemoriaShopSubsystem>())Shop->OnChanged.RemoveAll(this);
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
namespace
{
// The source reaches Verdan after ch1_after_forest's set_chapter 2. The slice skips
// Chapter 1, so its run is born in the chapter its imported entry sequence declares.
int64 EntryChapter(const FMemoriaNarrativeMetadata& Metadata)
{ return Metadata.bHasChapter && Metadata.Chapter >= 1 ? Metadata.Chapter : 0; }
}
bool UMemoriaNarrativeSubsystem::StartDevelopmentVN()
{
    if (!LoadContracts() || !EntryChapter(VNAsset->Definition.Metadata) ||
        Run->BeginStartingMemoryRun(EntryChapter(VNAsset->Definition.Metadata)) != EMemoriaMemoryResult::Success) return false;
    Context = MakeUnique<FMemoriaNarrativeContext>(Run->State, *Run->GetPlayerMemory());
    VN = MakeUnique<FMemoriaVNInterpreter>(VNAsset->Definition, *Context);
    State = EMemoriaSliceState::VN; Record(TEXT("vn:start:ch2_market_arrival"));
    VN->Play(); AfterVN(); return true;
}
bool UMemoriaNarrativeSubsystem::StartUnseenFieldFixture()
{
    if (!LoadContracts() || !EntryChapter(FieldAsset->Definition.Metadata) ||
        Run->BeginStartingMemoryRun(EntryChapter(FieldAsset->Definition.Metadata)) != EMemoriaMemoryResult::Success) return false;
    Context = MakeUnique<FMemoriaNarrativeContext>(Run->State, *Run->GetPlayerMemory());
    Record(TEXT("fixture:vn_unseen")); return EnterVerdan();
}
bool UMemoriaNarrativeSubsystem::EnterVerdan()
{
    // Source: verdan_market.gd _ready guard and the two _start_ch2_* methods.
    if (!Run->HasActiveRun() || !Context || !LoadContracts()) return false;
    bMaletCallbackConnected = true;
    Record(TEXT("verdan:enter"));
    ArmStoryBeats();
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

bool UMemoriaNarrativeSubsystem::ContinueCheckpoint()
{
    auto* World=GetWorld();
    if (!World || World->bIsTearingDown || !World->GetMapName().EndsWith(TEXT("L_VerdanHost"))) return false;
    FVector2D Position;
    auto* Checkpoint=GetGameInstance()->GetSubsystem<UMemoriaCheckpointSubsystem>();
    if (!Checkpoint->RestoreClosedBoundary(Position))
    {
        // A failed load leaves an existing run and its owner unchanged.
        if (!Context) { bCheckpointLoadFailed=true; State=EMemoriaSliceState::Deferred; CheckpointWorld=World; }
        ++Revision; return false;
    }
    Context=MakeUnique<FMemoriaNarrativeContext>(Run->State,*Run->GetPlayerMemory());
    bCheckpointScreen=true; bCheckpointLoadFailed=false; CheckpointWorld=World;
    State=EMemoriaSliceState::Deferred; DeferredInteraction=TEXT("before:chapter_transition_delay");
    if (auto* PC=World->GetFirstPlayerController())
        if (APawn* Pawn=PC->GetPawn()) Pawn->SetActorLocation(Memoria::Coordinates::FromSource(Position));
    Record(TEXT("checkpoint:restored:before:chapter_transition_delay")); ++Revision;
    return true;
}

bool UMemoriaNarrativeSubsystem::IsVerdanRevisit() const
{
    return Run && Run->HasActiveRun() && Run->GetRunSnapshot().RunId==RevisitRunId &&
        RevisitWorld.IsValid() && RevisitWorld.Get()==GetWorld() && !RevisitWorld->bIsTearingDown;
}
bool UMemoriaNarrativeSubsystem::RequestCheckpointRevisit()
{
    if (!bCheckpointScreen || bCheckpointLoadFailed || State!=EMemoriaSliceState::Deferred ||
        !Run->HasActiveRun() || !Run->GetRunSnapshot().GetFlag(TEXT("ch2_complete")) ||
        !CheckpointWorld.IsValid() || CheckpointWorld->bIsTearingDown || CheckpointWorld.Get()!=GetWorld()) return false;
    ReentryPosition=FVector2D(128,288);
    if (auto* PC=GetWorld()->GetFirstPlayerController())
        if (APawn* Pawn=PC->GetPawn()) ReentryPosition=Memoria::Coordinates::ToSource(Pawn->GetActorLocation());
    RevisitRunId=Run->GetRunSnapshot().RunId; bPendingVerdanReentry=true;
    // Intentional map replacement must survive cleanup; run replacement still clears it.
    CheckpointWorld.Reset(); bCheckpointScreen=false; CancelMaletDelay(); RewardCallbackWorld.Reset();
    State=EMemoriaSliceState::Travelling; Record(TEXT("checkpoint:revisit:travel")); ++Revision;
    UGameplayStatics::OpenLevel(this,FName(VerdanMap)); return true;
}
bool UMemoriaNarrativeSubsystem::EnterVerdanReentry()
{
    if (!bPendingVerdanReentry || !GetWorld() || GetWorld()->bIsTearingDown || !Run->HasActiveRun() ||
        Run->GetRunSnapshot().RunId!=RevisitRunId || !Run->GetRunSnapshot().GetFlag(TEXT("ch2_complete")) ||
        !GetWorld()->GetMapName().EndsWith(TEXT("L_VerdanHost"))) return false;
    bPendingVerdanReentry=false; RevisitWorld=GetWorld();
    Context=MakeUnique<FMemoriaNarrativeContext>(Run->State,*Run->GetPlayerMemory());
    DeferredInteraction.Reset(); bMaletTalkCached=false;
    if (!EnterVerdan()) { RevisitWorld.Reset(); return false; }
    if (auto* PC=GetWorld()->GetFirstPlayerController())
        if (APawn* Pawn=PC->GetPawn()) Pawn->SetActorLocation(Memoria::Coordinates::FromSource(ReentryPosition));
    Record(TEXT("verdan:revisit:encounters_enabled")); ++Revision; return true;
}
bool UMemoriaNarrativeSubsystem::ReturnFromAmbientBattle()
{
    if (!IsVerdanRevisit() || State!=EMemoriaSliceState::Exploration ||
        GetGameInstance()->GetSubsystem<UMemoriaBattleEntrySubsystem>()->IsActive()) return false;
    // Source _position_player has no loaded_pos after flee, so uses the authored spawn.
    ReentryPosition=FVector2D(128,288); bPendingVerdanReentry=true; RevisitWorld.Reset();
    CancelMaletDelay(); RewardCallbackWorld.Reset(); State=EMemoriaSliceState::Travelling;
    Record(TEXT("battle:fled:travel:verdan")); ++Revision;
    UGameplayStatics::OpenLevel(this,FName(VerdanMap)); return true;
}

bool UMemoriaNarrativeSubsystem::InteractWithMalet()
{
    if (GetGameInstance()->GetSubsystem<UMemoriaBattleEntrySubsystem>()->IsActive()) return false;
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
void UMemoriaNarrativeSubsystem::ArmStoryBeats()
{
    // Source _add_story_trigger at map _ready: skip seen flags, and gate the backstory on
    // malet_deal_accepted as it stood on entry. Arming is not part of the source trace.
    UWorld* World = GetWorld();
    if (!World || StoryWorld.Get() == World) return;
    StoryWorld = World; ArmedStoryBeats.Reset();
    const auto& S = Run->GetRunSnapshot();
    for (const auto& Beat : MemoriaVerdanStory::Beats())
    {
        if (S.GetFlag(Beat.Flag) || (Beat.RequiresFlag && !S.GetFlag(Beat.RequiresFlag))) continue;
        ArmedStoryBeats.Add(Beat.Group);
        if (World->bIsTearingDown || !World->GetMapName().EndsWith(TEXT("L_VerdanHost"))) continue;
        FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        if (auto* Point = World->SpawnActor<AMemoriaStoryPointActor>(Beat.Location, FRotator::ZeroRotator, Params)) Point->Configure(Beat.Group, Beat.Prompt);
    }
}
bool UMemoriaNarrativeSubsystem::IsStoryBeatAvailable(const FString& Group) const
{
    const auto* Beat = MemoriaVerdanStory::Find(Group);
    return Beat && Run && Run->HasActiveRun() && StoryWorld.IsValid() && StoryWorld.Get() == GetWorld() &&
        ArmedStoryBeats.Contains(Group) && !Run->GetRunSnapshot().GetFlag(Beat->Flag);
}
bool UMemoriaNarrativeSubsystem::StartStoryBeat(const FString& Group)
{
    if (GetGameInstance()->GetSubsystem<UMemoriaBattleEntrySubsystem>()->IsActive()) return false;
    if (State != EMemoriaSliceState::Exploration || !Context || !IsStoryBeatAvailable(Group)) return false;
    const auto* Beat = MemoriaVerdanStory::Find(Group);
    const FString Path = FString(TEXT("/Game/Memoria/Generated/Narrative/")) + Beat->Asset + TEXT(".") + Beat->Asset;
    StoryAsset = LoadObject<UMemoriaFieldAsset>(nullptr, *Path);
    if (!StoryAsset) { Record(TEXT("error:missing_story_contract:") + Group); return false; }
    // Source body_entered: set the one-time flag, then load_and_start the group.
    Run->SetStoryFlag(Beat->Flag, true); Record(FString(TEXT("flag:")) + Beat->Flag);
    ArmedStoryBeats.Remove(Group);
    Record(TEXT("request:res://data/chapter2_dialogue.json::") + Group);
    DeferredInteraction.Reset(); ActiveFieldAsset = StoryAsset; ++FieldInvocationCount;
    Field = MakeUnique<FMemoriaFieldInterpreter>(StoryAsset->Definition, *Context);
    State = EMemoriaSliceState::Field; Record(TEXT("field:start:") + Group);
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
    if (State==EMemoriaSliceState::Deferred && (bCheckpointScreen || bCheckpointLoadFailed ||
        GetGameInstance()->GetSubsystem<UMemoriaShopSubsystem>()->GetView().bClosed))
    {
        if (OriginalChoice==3) RequestCheckpointRevisit();
        else if (OriginalChoice==1) ContinueCheckpoint();
        else if (OriginalChoice==0 && !bCheckpointLoadFailed && GetWorld() && !GetWorld()->bIsTearingDown)
        {
            FVector2D Position(500,340);
            if (auto* PC=GetWorld()->GetFirstPlayerController())
                if (APawn* Pawn=PC->GetPawn()) Position=Memoria::Coordinates::ToSource(Pawn->GetActorLocation());
            GetGameInstance()->GetSubsystem<UMemoriaCheckpointSubsystem>()->SaveClosedBoundary(Position); ++Revision;
        }
        else if (OriginalChoice==2 && bCheckpointLoadFailed)
            UGameplayStatics::OpenLevel(this,TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice"));
        return;
    }
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
    if (GetView().bShopPresentation) { auto* Shop=GetGameInstance()->GetSubsystem<UMemoriaShopSubsystem>(); Shop->Close(Shop->GetView().Revision); return; }
    // PauseMenu._can_open_pause_menu permits VN; Field dialogue cannot cancel.
    if (State == EMemoriaSliceState::VN) { bPaused = !bPaused; ++Revision; }
}
FMemoriaNarrativeView UMemoriaNarrativeSubsystem::GetView() const
{
    if (State == EMemoriaSliceState::Deferred && PresentedSeedObservation == INDEX_NONE && PresentedPotionObservation == INDEX_NONE && PresentedAntidoteObservation == INDEX_NONE && PresentedFirebombObservation == INDEX_NONE)
    {
        const auto Shop = GetGameInstance()->GetSubsystem<UMemoriaShopSubsystem>()->GetView();
        if (Shop.bOpen) { FMemoriaNarrativeView V; V.bShopPresentation = true; V.Shop = Shop; return V; }
        if (Shop.bClosed || bCheckpointScreen || bCheckpointLoadFailed)
        {
            FMemoriaNarrativeView V; V.bDevelopmentStop=true;V.Header=TEXT("VERDAN EXCHANGE COMPLETE / DEVELOPMENT BOUNDARY");
            V.Header=bCheckpointLoadFailed ? TEXT("MEMORIA / CONTINUE") : TEXT("VERDAN / EXCHANGE COMPLETE");
            V.Body=GetGameInstance()->GetSubsystem<UMemoriaCheckpointSubsystem>()->GetStatusText();
            if (!bCheckpointLoadFailed)
                V.Body+=TEXT("\n\nThis checkpoint includes memories, Grains and items at the end of the exchange.\nLoad this checkpoint to revisit Verdan and enter ambient encounters.\nAttack and burn turns, Chapter 3 travel and persistent achievements are still in development.");
            if (bCheckpointScreen && !bCheckpointLoadFailed) V.Choices.Add({3,TEXT("Return to Verdan")});
            V.Choices.Add({1,TEXT("Load checkpoint")});
            V.Choices.Add(bCheckpointLoadFailed ? FMemoriaPresentedChoice{2,TEXT("Start a new slice")} : FMemoriaPresentedChoice{0,TEXT("Save checkpoint again")});
            return V;
        }
    }
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
            View.LocationTitle = TEXT("CHAPTER II  /  VERDAN");
            // Imported arrival is a linear sequence. Reconstruct its last authored CG,
            // including when resuming directly at a later cursor; no gameplay writes.
            for (int32 I = 0; I <= Index; ++I)
                if (VNAsset->Definition.Steps[I].Presentation.bHasCg) View.BackdropSource = VNAsset->Definition.Steps[I].Presentation.Cg;
            View.PortraitSource = MemoriaNarrativeArtwork::PortraitSource(Step.Presentation.Portrait);
            View.PortraitSide = Step.Presentation.Side;
            for (int32 I : VN->VisibleOriginalIndices()) View.Choices.Add({I, Context->Localized(Step.Choices[I].Text)});
        }
    }
    else if (State == EMemoriaSliceState::Field && Field)
    {
        int32 Index = Field->OriginalIndex();
        View.Header = FString::Printf(TEXT("%s   %d / %d"), ActiveFieldAsset == FieldAsset ? TEXT("VN-UNSEEN FIELD FIXTURE") : ActiveFieldAsset == EncounterAsset ? TEXT("MALET / ENCOUNTER") : ActiveFieldAsset == RefusedAsset ? TEXT("MALET / REFUSAL") : ActiveFieldAsset == DealAsset ? TEXT("MALET / DEAL") : ActiveFieldAsset == RewardAsset ? TEXT("MALET / REWARD") : ActiveFieldAsset == StoryAsset ? TEXT("VERDAN / STORY") : TEXT("MALET / MEMORY REACTION"), Index + 1, ActiveFieldAsset->Definition.Rows.Num());
        if (ActiveFieldAsset->Definition.Rows.IsValidIndex(Index))
        {
            const auto& Row = ActiveFieldAsset->Definition.Rows[Index]; Text = &Row.Text;
            View.LocationTitle = ActiveFieldAsset == FieldAsset ? TEXT("CHAPTER II  /  VERDAN") : TEXT("THE SUMP  /  MALET");
            if (const auto* Beat = ActiveFieldAsset && ActiveFieldAsset == StoryAsset ? MemoriaVerdanStory::Find(ActiveFieldAsset->Definition.Id) : nullptr) View.LocationTitle = Beat->Title;
            // Sequences without a CG use the already-authored encounter location.
            View.BackdropSource = ActiveFieldAsset == FieldAsset ? FString() : TEXT("res://assets/cg/generated/story_ch2_malet_cellar.png");
            for (int32 I = 0; I <= Index; ++I)
                if (ActiveFieldAsset->Definition.Rows[I].Presentation.bHasCg) View.BackdropSource = ActiveFieldAsset->Definition.Rows[I].Presentation.Cg;
            View.PortraitSource = MemoriaNarrativeArtwork::PortraitSource(Row.Presentation.Portrait);
            View.PortraitSide = Row.Text.Speaker == TEXT("Elia") ? TEXT("right") : TEXT("left");
            for (int32 I : Field->VisibleOriginalIndices()) View.Choices.Add({I, Context->Localized(Row.Choices[I].Text)});
        }
    }
    if (State == EMemoriaSliceState::Deferred)
    {
        View.bDevelopmentStop = true;
        const auto Snapshot = SeedObservations.IsValidIndex(PresentedSeedObservation) ? SeedObservations[PresentedSeedObservation] : Run->GetWorldCognition()->GetSnapshot();
        const auto* Actor = Snapshot.Actors.FindByPredicate([](const auto& A){return A.ActorId.Equals(MemoriaWorldIds::Malet,ESearchCase::CaseSensitive);});
        const auto* Knowledge = Actor ? Actor->Knowledge.FindByPredicate([](const auto& K){return K.FactId.Equals(MemoriaWorldIds::RouteFact,ESearchCase::CaseSensitive);}) : nullptr;
        const auto* Memory = Actor ? Actor->Memories.FindByPredicate([](const auto& M){return M.Id.Equals(MemoriaWorldIds::RouteMemory,ESearchCase::CaseSensitive);}) : nullptr;
        const TCHAR* Labels[] = {TEXT("MaletDone_Set"),TEXT("WorldKnowledge_Seeded"),TEXT("WorldMemory_Seeded")};
        View.Header = TEXT("PHASE 1N / FIREBOMB GRANT / PRE-SHOP STOP");
        View.Body = FString::Printf(TEXT("%s\n%s\nch2_malet_done=%s | revision=%lld | event_sequence=%lld\nnpc.malet / route fact=%s / source memory=%s\nFirebomb x1 deferred."),
            SeedObservations.IsValidIndex(PresentedSeedObservation) ? Labels[PresentedSeedObservation] : TEXT("Firebomb_Deferred"),
            SeedObservations.IsValidIndex(PresentedSeedObservation) ? TEXT("RECORDED SYNCHRONOUS SNAPSHOT / READ ONLY") : TEXT("LIVE AUTHORITATIVE WORLD STATE"),
            Run->GetRunSnapshot().GetFlag(TEXT("ch2_malet_done")) ? TEXT("true") : TEXT("false"),Snapshot.Revision,Snapshot.EventSequence,
            Knowledge ? (Knowledge->bValue ? TEXT("true") : TEXT("forgotten")) : TEXT("absent"),Memory ? *Memory->Status : TEXT("absent"));
    }
    if(State==EMemoriaSliceState::Deferred && PresentedSeedObservation==INDEX_NONE)
    {
        const bool PotionRecorded=PotionObservations.IsValidIndex(PresentedPotionObservation);
        const bool AntidoteRecorded=AntidoteObservations.IsValidIndex(PresentedAntidoteObservation);
        const bool FirebombRecorded=FirebombObservations.IsValidIndex(PresentedFirebombObservation);
        const bool Recorded=PotionRecorded || AntidoteRecorded || FirebombRecorded;
        const auto S=PotionRecorded?PotionObservations[PresentedPotionObservation]:(AntidoteRecorded?AntidoteObservations[PresentedAntidoteObservation]:(FirebombRecorded?FirebombObservations[PresentedFirebombObservation]:Run->GetRunSnapshot()));
        const auto* Item=S.Player.Items.FindByPredicate([](const auto& I){return I.Id.Equals(TEXT("potion"),ESearchCase::CaseSensitive);});
        const TCHAR* Labels[]={TEXT("Potion_Before"),TEXT("Potion_Granted"),TEXT("Recent_Items"),TEXT("Inventory_Changed"),TEXT("Potion_Toast")};
        const auto* Antidote=S.Player.Items.FindByPredicate([](const auto& I){return I.Id==TEXT("antidote");});
        const TCHAR* AntidoteLabels[]={TEXT("Antidote_Before"),TEXT("Antidote_Granted"),TEXT("Antidote_Recent"),TEXT("Antidote_Signal"),TEXT("Antidote_Toast")};
        const auto* Firebomb=S.Player.Items.FindByPredicate([](const auto& I){return I.Id==TEXT("firebomb");});
        const TCHAR* FirebombLabels[]={TEXT("Firebomb_Before"),TEXT("Firebomb_Granted"),TEXT("Firebomb_Recent"),TEXT("Firebomb_Signal (post-broadcast observation)"),TEXT("Reward_Toasts")};
        const int32 VisibleRequests=PotionRecorded?(PresentedPotionObservation==4?1:0):(AntidoteRecorded?(PresentedAntidoteObservation==4?2:1):(FirebombRecorded?(PresentedFirebombObservation==4?3:2):RewardToasts.Num()));
        TArray<FString> VisibleToasts;for(int32 I=0;I<FMath::Min(VisibleRequests,RewardToasts.Num());++I)VisibleToasts.Add(RewardToasts[I]);
        const FString Toasts=VisibleToasts.IsEmpty()?TEXT("Toast not yet requested at this recorded boundary"):FString(TEXT("[+] "))+FString::Join(VisibleToasts,TEXT("\n[+] "));
        View.Body=FString::Printf(TEXT("%s\n%s\nPotion: %lld | Antidote: %lld | Firebomb: %lld\nrecent: [%s]\nch2_malet_done=%s | world revision=%lld / sequence=%lld\n%s\nSTOP before _open_malet_shop() entry"),
            PotionRecorded?Labels[PresentedPotionObservation]:(AntidoteRecorded?AntidoteLabels[PresentedAntidoteObservation]:(FirebombRecorded?FirebombLabels[PresentedFirebombObservation]:TEXT("Shop_Deferred"))),Recorded?TEXT("RECORDED SYNCHRONOUS SNAPSHOT / READ ONLY"):TEXT("LIVE AUTHORITATIVE RUN STATE"),Item?Item->Count:0,Antidote?Antidote->Count:0,Firebomb?Firebomb->Count:0,*FString::Join(S.Player.RecentItems,TEXT(", ")),S.GetFlag(TEXT("ch2_malet_done"))?TEXT("true"):TEXT("false"),Run->GetWorldCognition()->GetSnapshot().Revision,Run->GetWorldCognition()->GetSnapshot().EventSequence,*Toasts);
    }
    if (Text) { View.Speaker = Text->Speaker; View.Narration = Context->Localized(*Text, true); View.Body = Context->Localized(*Text); }
    return View;
}
FMemoriaVNContinuation UMemoriaNarrativeSubsystem::GetContinuation() const
{ return VN ? VN->ExportContinuation() : FMemoriaVNContinuation(); }
UMemoriaRunSaveGame* UMemoriaNarrativeSubsystem::CaptureSave() const
{
    if (!Run->HasActiveRun() || State != EMemoriaSliceState::VN || !VN) return nullptr;
    auto* Save = Run->CaptureSave();
    if (!Save) return nullptr;
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
    if (!Run->RestoreSave(Save)) return false;
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
    if (World == RevisitWorld.Get()) RevisitWorld.Reset();
    if (World == CheckpointWorld.Get()) { Reset(); return; }
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
        // frame or timer is inserted at this seam; the first flag commits below.
#if WITH_DEV_AUTOMATION_TESTS
        OnRewardBoundaryObserved.Broadcast(TEXT("before_callback"));
#endif
        CommitRewardFlagAndDeferSeed(); return;
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

bool UMemoriaNarrativeSubsystem::HasLiveRewardOwner() const
{
    if (!RewardCallbackWorld.IsValid() || RewardCallbackWorld.Get() != GetWorld()) return false;
    const FWorldContext* WorldContext = GEngine ? GEngine->GetWorldContextFromWorld(RewardCallbackWorld.Get()) : nullptr;
    // OpenLevel queues native teardown for the engine's safe travel point. Do not
    // commit into an owner with an already accepted outgoing travel request.
    return WorldContext && WorldContext->TravelURL.IsEmpty() &&
        !RewardCallbackWorld->bIsTearingDown && Run->HasActiveRun() &&
        RewardCallbackRunId == Run->GetRunSnapshot().RunId;
}
void UMemoriaNarrativeSubsystem::CommitRewardFlagAndDeferSeed()
{
    if (!HasLiveRewardOwner()) return;
    ++RewardCallbackIntentCount; Record(TEXT("callback:reward:enter"));
#if WITH_DEV_AUTOMATION_TESTS
    OnRewardBoundaryObserved.Broadcast(TEXT("before_flag"));
#endif
    if (!HasLiveRewardOwner()) return;
    // Exact first source effect, through the existing case-sensitive run authority.
    if (!Run->SetStoryFlag(TEXT("ch2_malet_done"), true))
    {
        State = EMemoriaSliceState::Failed; ++Revision; return;
    }
    Record(TEXT("flag:ch2_malet_done"));
#if WITH_DEV_AUTOMATION_TESTS
    OnRewardBoundaryObserved.Broadcast(TEXT("after_flag"));
#endif
    // A committed old-run flag is never rolled back if a lifecycle observer replaces it.
    if (!HasLiveRewardOwner()) return;
    auto* World = Run->GetWorldCognition();
    SeedObservations.Reset(); SeedObservations.Add(World->GetSnapshot());
    Record(TEXT("worldseed:enter"));
#if WITH_DEV_AUTOMATION_TESTS
    OnRewardBoundaryObserved.Broadcast(TEXT("before_world_seed"));
#endif
    if (!HasLiveRewardOwner()) return;
    const auto Handle = World->OnCommitted.AddLambda([this](const FMemoriaWorldEvent& E,const FMemoriaWorldSnapshot& S)
    {
        Record(FString(E.bKnowledge ? TEXT("world:knowledge:") : TEXT("world:memory:"))+E.ActorId+TEXT(":")+E.TargetId);
        Record(FString::Printf(TEXT("world:revision:%lld"),E.Revision));
        SeedObservations.Add(S);
#if WITH_DEV_AUTOMATION_TESTS
        OnRewardBoundaryObserved.Broadcast(E.bKnowledge ? TEXT("after_knowledge") : TEXT("after_memory"));
#endif
    });
    World->SeedMaletRoute(Run->GetRunSnapshot().GetFlag(TEXT("ch2_malet_done")));
    World->OnCommitted.Remove(Handle);
    Record(TEXT("worldseed:end"));
#if WITH_DEV_AUTOMATION_TESTS
    OnRewardBoundaryObserved.Broadcast(TEXT("world_seed_complete"));
#endif
    if (!HasLiveRewardOwner()) return;
    PotionObservations.Reset(); PotionObservations.Add(Run->GetRunSnapshot());
#if WITH_DEV_AUTOMATION_TESTS
    OnRewardBoundaryObserved.Broadcast(TEXT("before_potion"));
#endif
    if (!HasLiveRewardOwner()) return;
    const int64 PotionBefore=Run->GetItemCount(TEXT("potion"));
    Record(TEXT("item:add:begin:potion:2"));
    const auto Observation=Run->OnPotionObserved.AddLambda([this,PotionBefore](const FString& Point,const FMemoriaRunSnapshot& Snapshot)
    {
        if(!HasLiveRewardOwner())return;
        if(Point==TEXT("after_inventory_mutation"))Record(FString::Printf(TEXT("inventory:potion:%lld->%lld"),PotionBefore,Run->GetItemCount(TEXT("potion"))));
        if(Point==TEXT("after_recent_items"))Record(TEXT("recent_items:")+FString::Join(Snapshot.Player.RecentItems,TEXT(",")));
        PotionObservations.Add(Snapshot);
#if WITH_DEV_AUTOMATION_TESTS
        OnRewardBoundaryObserved.Broadcast(Point);
#endif
    });
    const auto Changed=Run->OnInventoryChanged.AddLambda([this](const FString& Id){if(HasLiveRewardOwner())Record(TEXT("inventory_changed:")+Id);});
    const auto Toast=Run->OnItemToastRequested.AddLambda([this](const FString& Text,int32 Type)
    {if(HasLiveRewardOwner()){PotionToast=Text;RewardToasts.Add(Text);Record(TEXT("toast:")+Text+FString::Printf(TEXT(":%d"),Type));}});
    const bool Complete=Run->AddRewardPotion(TEXT("potion"),2);
    Run->OnPotionObserved.Remove(Observation);Run->OnInventoryChanged.Remove(Changed);Run->OnItemToastRequested.Remove(Toast);
    if(!Complete || !HasLiveRewardOwner())return;
    Record(TEXT("item:add:end:potion:2"));
#if WITH_DEV_AUTOMATION_TESTS
    OnRewardBoundaryObserved.Broadcast(TEXT("potion_contract_complete"));
#endif
    if(!HasLiveRewardOwner())return;
    CommitAntidoteAndDeferFirebomb();
}

void UMemoriaNarrativeSubsystem::CommitAntidoteAndDeferFirebomb()
{
    if(!HasLiveRewardOwner())return;
    AntidoteObservations.Add(Run->GetRunSnapshot());
#if WITH_DEV_AUTOMATION_TESTS
    OnRewardBoundaryObserved.Broadcast(TEXT("before_antidote"));
#endif
    if(!HasLiveRewardOwner())return;
    const int64 Before=Run->GetItemCount(TEXT("antidote"));
    Record(TEXT("item:add:begin:antidote:1"));
    const auto Observation=Run->OnRewardItemObserved.AddLambda([this,Before](const FString& Id,const FString& Point,const FMemoriaRunSnapshot& Snapshot)
    {
        if(!HasLiveRewardOwner() || Id!=TEXT("antidote"))return;
        if(Point==TEXT("after_inventory_mutation"))Record(FString::Printf(TEXT("inventory:antidote:%lld->%lld"),Before,Run->GetItemCount(Id)));
        if(Point==TEXT("after_recent_items"))Record(TEXT("recent_items:")+FString::Join(Snapshot.Player.RecentItems,TEXT(",")));
        AntidoteObservations.Add(Snapshot);
#if WITH_DEV_AUTOMATION_TESTS
        OnRewardBoundaryObserved.Broadcast(TEXT("antidote_")+Point);
#endif
    });
    const auto Changed=Run->OnInventoryChanged.AddLambda([this](const FString& Id){if(HasLiveRewardOwner())Record(TEXT("inventory_changed:")+Id);});
    const auto Toast=Run->OnItemToastRequested.AddLambda([this](const FString& Text,int32 Type)
    {if(HasLiveRewardOwner()){RewardToasts.Add(Text);Record(TEXT("toast:")+Text+FString::Printf(TEXT(":%d"),Type));}});
    const bool Complete=Run->AddRewardAntidote(TEXT("antidote"),1);
    Run->OnRewardItemObserved.Remove(Observation);Run->OnInventoryChanged.Remove(Changed);Run->OnItemToastRequested.Remove(Toast);
    if(!Complete || !HasLiveRewardOwner())return;
    Record(TEXT("item:add:end:antidote:1"));
#if WITH_DEV_AUTOMATION_TESTS
    OnRewardBoundaryObserved.Broadcast(TEXT("antidote_contract_complete"));
#endif
    if(!HasLiveRewardOwner())return;
    CommitFirebombAndDeferShop();
}

void UMemoriaNarrativeSubsystem::CommitFirebombAndDeferShop()
{
    if(!HasLiveRewardOwner())return;
    FirebombObservations.Add(Run->GetRunSnapshot());
#if WITH_DEV_AUTOMATION_TESTS
    OnRewardBoundaryObserved.Broadcast(TEXT("before_firebomb"));
#endif
    if(!HasLiveRewardOwner())return;
    const int64 Before=Run->GetItemCount(TEXT("firebomb"));
    Record(TEXT("item:add:begin:firebomb:1"));
    const auto Observation=Run->OnRewardItemObserved.AddLambda([this,Before](const FString& Id,const FString& Point,const FMemoriaRunSnapshot& Snapshot)
    {
        if(!HasLiveRewardOwner() || Id!=TEXT("firebomb"))return;
        if(Point==TEXT("after_inventory_mutation"))Record(FString::Printf(TEXT("inventory:firebomb:%lld->%lld"),Before,Run->GetItemCount(Id)));
        if(Point==TEXT("after_recent_items"))Record(TEXT("recent_items:")+FString::Join(Snapshot.Player.RecentItems,TEXT(",")));
        FirebombObservations.Add(Snapshot);
#if WITH_DEV_AUTOMATION_TESTS
        OnRewardBoundaryObserved.Broadcast(TEXT("firebomb_")+Point);
#endif
    });
    const auto Changed=Run->OnInventoryChanged.AddLambda([this](const FString& Id){if(HasLiveRewardOwner())Record(TEXT("inventory_changed:")+Id);});
    const auto Toast=Run->OnItemToastRequested.AddLambda([this](const FString& Text,int32 Type)
    {if(HasLiveRewardOwner()){RewardToasts.Add(Text);Record(TEXT("toast:")+Text+FString::Printf(TEXT(":%d"),Type));}});
    const bool Complete=Run->AddRewardFirebomb(TEXT("firebomb"),1);
    Run->OnRewardItemObserved.Remove(Observation);Run->OnInventoryChanged.Remove(Changed);Run->OnItemToastRequested.Remove(Toast);
    if(!Complete || !HasLiveRewardOwner())return;
    Record(TEXT("item:add:end:firebomb:1"));
#if WITH_DEV_AUTOMATION_TESTS
    OnRewardBoundaryObserved.Broadcast(TEXT("firebomb_contract_complete"));
#endif
    if(!HasLiveRewardOwner())return;
    // Retain the exact Phase 1N seam before any shop entry work.
    Record(TEXT("development:deferred:before:shop_open"));
    auto* Shop = GetGameInstance()->GetSubsystem<UMemoriaShopSubsystem>();
    if (!Shop->OpenMalet(GetWorld())) { State = EMemoriaSliceState::Failed; Record(TEXT("error:shop_open")); ++Revision; return; }
    Record(TEXT("shop:open:Malet:sell"));
    for (const auto& Request : Shop->GetView().Requests) Record(Request);
    DeferredInteraction = TEXT("before:shop_actions");
    Record(TEXT("development:deferred:") + DeferredInteraction);
    State = EMemoriaSliceState::Deferred; ++Revision;
}

void UMemoriaNarrativeSubsystem::PresentSeedObservation(int32 Index)
{
    if(State!=EMemoriaSliceState::Deferred)return;
    PresentedSeedObservation=SeedObservations.IsValidIndex(Index) ? Index : INDEX_NONE;
    ++Revision;
}

void UMemoriaNarrativeSubsystem::PresentPotionObservation(int32 Index)
{
    if(State!=EMemoriaSliceState::Deferred)return;
    PresentedSeedObservation=INDEX_NONE;PresentedFirebombObservation=INDEX_NONE;
    PresentedAntidoteObservation=INDEX_NONE;
    PresentedPotionObservation=PotionObservations.IsValidIndex(Index)?Index:INDEX_NONE;
    ++Revision;
}

void UMemoriaNarrativeSubsystem::PresentAntidoteObservation(int32 Index)
{
    if(State!=EMemoriaSliceState::Deferred)return;
    PresentedSeedObservation=INDEX_NONE;PresentedFirebombObservation=INDEX_NONE;PresentedPotionObservation=INDEX_NONE;
    PresentedAntidoteObservation=AntidoteObservations.IsValidIndex(Index)?Index:INDEX_NONE;
    ++Revision;
}

void UMemoriaNarrativeSubsystem::PresentFirebombObservation(int32 Index)
{
    if(State!=EMemoriaSliceState::Deferred)return;
    PresentedSeedObservation=INDEX_NONE;PresentedPotionObservation=INDEX_NONE;PresentedAntidoteObservation=INDEX_NONE;
    PresentedFirebombObservation=FirebombObservations.IsValidIndex(Index)?Index:INDEX_NONE;
    ++Revision;
}
