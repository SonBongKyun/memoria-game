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
#include "Narrative/MemoriaVerdanStory.h"
#include "Narrative/MemoriaSumpLedger.h"
#include "Chapter/MemoriaChapterMap.h"
#include "Narrative/MemoriaClassifier.h"
#include "Achievements/MemoriaAchievementSubsystem.h"
#include "World/MemoriaWorldCognition.h"
#include "Settings/MemoriaSettingsSubsystem.h"
#include "Audio/MemoriaAudioSubsystem.h"
#include "Interaction/MemoriaStoryPointActor.h"
#include "Interaction/MemoriaEliaCompanion.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Misc/Paths.h"
#include "UObject/StrongObjectPtr.h"

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
    ChapterMap.Reset();
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
    ArmedStoryBeats.Reset(); StoryWorld.Reset(); StoryAsset = nullptr; bEliaTalkCached = false; EliaTalkAsset = nullptr; bTraderArmed = bLedgerArmed = false; Notices.Reset(); NoticeTime = -1000; bNoticeHeld = false;
    bNewGameRoute = false; SceneCg.Reset(); ShownScene.Reset(); ShownCue.Reset(); SceneMusic = NAME_None;
    LedgerTitle.Reset(); LedgerLines.Reset(); bLedgerThreadHolds = false; AutosaveCount = 0; BurnSerial = 0;
    bPaused = false; EventCursor = 0; FieldInvocationCount = 0; Trace.Reset(); ++Revision;
}
void UMemoriaNarrativeSubsystem::Deinitialize()
{
    if(auto* Shop=GetGameInstance()->GetSubsystem<UMemoriaShopSubsystem>())Shop->OnChanged.RemoveAll(this);
    FWorldDelegates::OnWorldCleanup.RemoveAll(this);
    if (Run) Run->OnRunReplaced.RemoveAll(this);
    Reset(); RouteAssets.Reset(); VNAsset = nullptr; FieldAsset = nullptr; MaletAsset = nullptr; EncounterAsset = nullptr; RefusedAsset = nullptr; DealAsset = nullptr; RewardAsset = nullptr; Run = nullptr;
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
    for (; EventCursor < Context->Events.Num(); ++EventCursor)
    {
        const FString& Event = Context->Events[EventCursor];
        if (Event.StartsWith(TEXT("burn:")) && Event.EndsWith(TEXT(":ok"))) ++BurnSerial;
        if (Event.StartsWith(TEXT("chapter_complete:")))
            if (auto* Achievements = GetGameInstance()->GetSubsystem<UMemoriaAchievementSubsystem>()) Achievements->RecordChapterComplete(FCString::Atoi(*Event.RightChop(17)));
        Record(Dialect + TEXT(":") + Event);
    }
    ++Revision;
}
namespace
{
// The source reaches Verdan after ch1_after_forest's set_chapter 2. The slice skips
// Chapter 1, so its run is born in the chapter its imported entry sequence declares.
// game_manager.gd SPEAKER_NAMES_KO / localized_speaker.
FString SpeakerKo(const FString& Speaker)
{
    static const TMap<FString, FString> Names = {
        {TEXT("system_log"), TEXT("시스템")}, {TEXT("System"), TEXT("시스템")}, {TEXT("Narration"), TEXT("나레이션")},
        {TEXT("Arrel"), TEXT("아렐")}, {TEXT("Elia"), TEXT("엘리아")}, {TEXT("Malet"), TEXT("말렛")}, {TEXT("Mallet"), TEXT("말렛")},
        {TEXT("Kairos"), TEXT("카이로스")}, {TEXT("Sable"), TEXT("세이블")}, {TEXT("Nera"), TEXT("네라")}, {TEXT("Seric"), TEXT("세릭")},
        {TEXT("Tobias"), TEXT("토비아스")}, {TEXT("Veil"), TEXT("베일")}, {TEXT("Ashen Figure"), TEXT("잿빛 형상")}, {TEXT("Old Man"), TEXT("노인")},
        {TEXT("Nervous Trader"), TEXT("불안한 상인")}, {TEXT("Gardener"), TEXT("정원사")}, {TEXT("Handler"), TEXT("관리관")},
        {TEXT("Prisoner"), TEXT("수감자")}, {TEXT("Guard"), TEXT("경비병")}, {TEXT("Han"), TEXT("한")}, {TEXT("Mira"), TEXT("미라")},
        {TEXT("Vael"), TEXT("바엘")}, {TEXT("Belor"), TEXT("벨로르")}, {TEXT("Chief Archivist"), TEXT("수석 기록관")}};
    const FString* Found = Names.Find(Speaker); return Found ? *Found : Speaker;
}
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
const UMemoriaVNAsset* UMemoriaNarrativeSubsystem::ResolveVN(const FString& Id)
{
    for (const auto& Asset : RouteAssets) if (Asset && Asset->Definition.Id.Equals(Id, ESearchCase::CaseSensitive)) return Asset;
    // Imported VN packages are named from the scene id: ch1_cold_open -> DA_VN_Ch1ColdOpen.
    TArray<FString> Parts; Id.ParseIntoArray(Parts, TEXT("_")); FString Name;
    for (const auto& Part : Parts) Name += Part.Left(1).ToUpper() + Part.Mid(1);
    auto* Asset = Name.IsEmpty() ? nullptr : LoadObject<UMemoriaVNAsset>(nullptr, *FString::Printf(TEXT("/Game/Memoria/Generated/Narrative/DA_VN_%s.DA_VN_%s"), *Name, *Name));
    if (!Asset || !Asset->Definition.Id.Equals(Id, ESearchCase::CaseSensitive)) return nullptr;
    RouteAssets.Add(Asset); return Asset;
}
FMemoriaVNInterpreter::FResolver UMemoriaNarrativeSubsystem::Resolver()
{
    return [this](const FString& Id) -> const FMemoriaVNDefinition* { const auto* Asset = ResolveVN(Id); return Asset ? &Asset->Definition : nullptr; };
}
bool UMemoriaNarrativeSubsystem::StartNewGame(const FString& Locale)
{
    const auto* Cold = ResolveVN(TEXT("ch1_cold_open"));
    const FString KeptLocale = Locale.IsEmpty() ? Run->State.CurrentLocale : Locale;
    if (!Cold || !LoadContracts() || Run->BeginStartingMemoryRun(1) != EMemoriaMemoryResult::Success) return false;
    bTitle = false;
    Run->State.CurrentLocale = KeptLocale.IsEmpty() ? TEXT("en") : KeptLocale;
    // main.gd _on_new_game_pressed player_data; flags start empty and memories are the starting set.
    auto& Player = Run->State.Player; Player = FMemoriaPlayerState();
    Player.Hp = Player.MaxHp = 100; Player.Grains = 0; Player.FieldFocus = Player.DirectiveStreak = 0; Player.bEliaWithParty = true;
    FMemoriaItemCount Ink; Ink.Id = TEXT("witness_ink"); Ink.Count = 1; Player.Items = {Ink};
    Player.QuickSlots = {TEXT("witness_ink"), TEXT("potion"), TEXT("antidote")};
    Context = MakeUnique<FMemoriaNarrativeContext>(Run->State, *Run->GetPlayerMemory());
    VN = MakeUnique<FMemoriaVNInterpreter>(Cold->Definition, *Context, Resolver(), true);
    bNewGameRoute = true; State = EMemoriaSliceState::VN; Record(TEXT("vn:start:ch1_cold_open"));
    VN->Play(); AfterVN(); return true;
}
void UMemoriaNarrativeSubsystem::ShowChapterLedger(const FString& Event)
{
    // Event is ledger:<chapter>:<ids burned since set_chapter>; lines follow _show_chapter_ledger.
    TArray<FString> Parts; Event.ParseIntoArray(Parts, TEXT(":"), false);
    if (Parts.Num() < 2) return;
    const bool Ko = Run->State.CurrentLocale == TEXT("ko");
    TArray<FString> Burned; if (Parts.Num() > 2) Parts[2].ParseIntoArray(Burned, TEXT(","), true);
    auto* Memory = Run->GetPlayerMemory(); const auto Snapshot = Memory->GetSnapshot();
    int32 Held = 0; for (const auto& M : Snapshot.Owned) if (!M.bBurned && !M.bFaded) ++Held;
    int32 Anchors = 0; for (const TCHAR* Id : {TEXT("identity_first_sword"), TEXT("rel_hand_reaching"), TEXT("daily_elia_hands"), TEXT("rel_sable_trust")}) if (Memory->IsIntact(Id)) ++Anchors;
    const bool Name = Memory->IsIntact(TEXT("core_name_origin"));
    LedgerTitle = Ko ? FString::Printf(TEXT("장부, 제%s장"), *Parts[1]) : FString::Printf(TEXT("THE LEDGER, CHAPTER %s"), *Parts[1]);
    LedgerLines.Reset();
    if (Burned.IsEmpty()) LedgerLines.Add(Ko ? TEXT("이번 장에서 태운 기억: 없음") : TEXT("Burned this chapter: nothing"));
    else
    {
        // Unreal has no Korean memory titles yet; the source title is shown in both locales.
        TArray<FString> Names;
        for (const auto& Id : Burned)
        {
            const auto* D = Memory->GetDefinitions().FindByPredicate([&](const auto& V){ return V.Id.Equals(Id, ESearchCase::CaseSensitive); });
            Names.Add(D ? D->Title : Id);
        }
        FString Joined = FString::Join(TArray<FString>(Names.GetData(), FMath::Min(3, Names.Num())), TEXT(", "));
        if (Names.Num() > 3) Joined += Ko ? FString::Printf(TEXT(" 외 %d"), Names.Num() - 3) : FString::Printf(TEXT(" +%d more"), Names.Num() - 3);
        LedgerLines.Add(Ko ? FString::Printf(TEXT("이번 장에서 태운 기억 %d, %s"), Burned.Num(), *Joined) : FString::Printf(TEXT("Burned this chapter: %d, %s"), Burned.Num(), *Joined));
    }
    LedgerLines.Add(Ko ? FString::Printf(TEXT("아직 온전한 기억: %d"), Held) : FString::Printf(TEXT("Still held intact: %d"), Held));
    LedgerLines.Add(Ko ? FString::Printf(TEXT("닻: %d/4 · 이름: %s"), Anchors, Name ? TEXT("온전") : TEXT("소실")) : FString::Printf(TEXT("Anchors: %d/4 · The name: %s"), Anchors, Name ? TEXT("intact") : TEXT("gone")));
    // weave_unlocked: the name intact, fewer than WEAVE_MAX_BURNS (4) burns, 3+ anchors.
    bLedgerThreadHolds = Name && Snapshot.BurnedHistory.Num() < 4 && Anchors >= 3;
    LedgerLines.Add(bLedgerThreadHolds ? (Ko ? TEXT("실은 아직 이어져 있다.") : TEXT("The thread still holds.")) : (Ko ? TEXT("실이 닳아 가고 있다.") : TEXT("The thread is fraying.")));
    ++LedgerSerial; Record(FString::Printf(TEXT("ledger:shown:%s:burned=%d:held=%d:anchors=%d"), *Parts[1], Burned.Num(), Held, Anchors));
}
bool UMemoriaNarrativeSubsystem::AutosaveChapterMap(const FString& Map, const FVector2D& Position)
{
    const bool Saved = Run->HasActiveRun() && GetGameInstance()->GetSubsystem<UMemoriaCheckpointSubsystem>()->SaveChapterMap(Map, Position);
    if (Saved)
    {
        ++AutosaveCount;
        // NotificationToast._on_save_completed, slot 0.
        Notice(Run->GetRunSnapshot().CurrentLocale == TEXT("ko") ? TEXT("자동 저장 완료") : TEXT("Autosaved"));
    }
    Record(Saved ? TEXT("autosave:map_saved:") + Map : FString(TEXT("autosave:skipped")));
    return Saved;
}
bool UMemoriaNarrativeSubsystem::ContinueChapterMap(const FString& Map)
{
    auto* Checkpoint = GetGameInstance()->GetSubsystem<UMemoriaCheckpointSubsystem>();
    ContinuedMapPosition.Reset();
    // The slot must name this map; a refused load leaves a live run as it was.
    FString Saved; FVector2D Position;
    if (Checkpoint->PeekChapterMap() != Map || !Checkpoint->RestoreChapterMap(Saved, Position)) { Record(TEXT("autosave:map_resume_failed")); return false; }
    ContinuedMapPosition = Position; Record(TEXT("autosave:map_resumed:") + Map);
    return true;
}
bool UMemoriaNarrativeSubsystem::ConsumeMapPosition(FVector2D& Out)
{
    if (!ContinuedMapPosition.IsSet()) return false;
    Out = ContinuedMapPosition.GetValue(); ContinuedMapPosition.Reset(); return true;
}
void UMemoriaNarrativeSubsystem::Autosave()
{
    // S330: a step that also leaves for a chapter map (ch5_classifier's last step: complete_chapter, autosave,
    // goto_map drift_shelter) has no VN cursor left to resume; the save is that map's field, at its spawn.
    if (const FString Map = FPaths::GetBaseFilename(Context->RequestedMap); Context->RequestedMap.StartsWith(TEXT("res://scenes/maps/")))
        if (const auto* Spec = MemoriaChapterMaps::Find(Map)) { AutosaveChapterMap(Map, Spec->Spawn); return; }
    // The autosave step also jumps to the next scene, so the save resumes at that scene's start.
    auto* Save = CaptureSave();
    const bool Saved = Save && GetGameInstance()->GetSubsystem<UMemoriaCheckpointSubsystem>()->SaveChapterTransition(*Save);
    if (Saved) ++AutosaveCount;
    Record(Saved ? TEXT("autosave:saved") : TEXT("autosave:skipped"));
}
void UMemoriaNarrativeSubsystem::PresentVNStep()
{
    if (!VN || !VN->ExportContinuation().bActive) return;
    const auto& Definition = VN->GetDefinition(); const int32 Index = VN->ExportContinuation().Current.OriginalIndex;
    if (!Definition.Id.Equals(ShownScene, ESearchCase::CaseSensitive))
    {
        // SceneFlow.play: a scene that declares bgm switches the music; others keep it.
        ShownScene = Definition.Id;
        if (Definition.Metadata.bHasBgm) SceneMusic = FName(*FPaths::GetBaseFilename(Definition.Metadata.Bgm));
    }
    const FString Cue = Definition.Id + TEXT(":") + FString::FromInt(Index);
    if (Cue == ShownCue || !Definition.Steps.IsValidIndex(Index)) return;
    ShownCue = Cue;
    const auto& Step = Definition.Steps[Index]; const auto Display = Context->VNDisplay(Step);
    // vn_scene.gd _change_cg keeps the current CG until a step names another one.
    if (!Display.Cg.IsEmpty()) SceneCg = MemoriaNarrativeArtwork::CgSource(Display.Cg);
    if (Step.Presentation.bHasSfx && !Step.Presentation.Sfx.IsEmpty())
        if (auto* Audio = GetGameInstance()->GetSubsystem<UMemoriaAudioSubsystem>()) Audio->PlaySfx(FName(*Step.Presentation.Sfx));
}
void UMemoriaNarrativeSubsystem::EnterTitle()
{
    bTitle = true; State = EMemoriaSliceState::Idle; Record(TEXT("title:enter")); ++Revision;
}
bool UMemoriaNarrativeSubsystem::ResumeChapterAutosave()
{
    auto* Save = GetGameInstance()->GetSubsystem<UMemoriaCheckpointSubsystem>()->LoadChapterTransition();
    TStrongObjectPtr<UMemoriaRunSaveGame> Retained(Save);
    if (!Save || !PrepareRestore(*Save)) { Record(TEXT("autosave:resume_failed")); return false; }
    bTitle = false;
    Record(TEXT("autosave:resumed"));
    return ResumePrepared();
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
    ChapterMap.Reset();
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
    // S329: the turn-based flee's return is retired; this remains the revisit's re-entry at the authored spawn.
    if (!IsVerdanRevisit() || State!=EMemoriaSliceState::Exploration) return false;
    // Source _position_player has no loaded_pos after flee, so uses the authored spawn.
    ReentryPosition=FVector2D(128,288); bPendingVerdanReentry=true; RevisitWorld.Reset();
    CancelMaletDelay(); RewardCallbackWorld.Reset(); State=EMemoriaSliceState::Travelling;
    Record(TEXT("battle:fled:travel:verdan")); ++Revision;
    UGameplayStatics::OpenLevel(this,FName(VerdanMap)); return true;
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
void UMemoriaNarrativeSubsystem::ArmStoryBeats()
{
    // Source _add_story_trigger at map _ready: skip seen flags, and gate the backstory on
    // malet_deal_accepted as it stood on entry. Arming is not part of the source trace.
    UWorld* World = GetWorld();
    if (!World || StoryWorld.Get() == World) return;
    StoryWorld = World; ArmedStoryBeats.Reset(); bEliaTalkCached = false;
    const auto& S = Run->GetRunSnapshot();
    for (const auto& Beat : MemoriaVerdanStory::Beats())
    {
        if (S.GetFlag(Beat.Flag) || (Beat.RequiresFlag && !S.GetFlag(Beat.RequiresFlag))) continue;
        ArmedStoryBeats.Add(Beat.Group);
        if (World->bIsTearingDown || !World->GetMapName().EndsWith(TEXT("L_VerdanHost"))) continue;
        FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        if (auto* Point = World->SpawnActor<AMemoriaStoryPointActor>(Beat.Location, FRotator::ZeroRotator, Params)) Point->Configure(Beat.Group, Beat.Prompt);
    }
    // Source _setup_side_quests at _ready: the trader exists while the quest is available or
    // active; the ledger only if the quest was already active when Verdan was entered.
    bTraderArmed = MemoriaSumpLedger::IsAvailable(S) || MemoriaSumpLedger::IsActive(S);
    bLedgerArmed = MemoriaSumpLedger::IsActive(S) && !S.GetFlag(MemoriaSumpLedger::Steps()[1].Flag);
    if (World->bIsTearingDown || !World->GetMapName().EndsWith(TEXT("L_VerdanHost"))) return;
    FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    if (bTraderArmed)
        if (auto* Point = World->SpawnActor<AMemoriaStoryPointActor>(MemoriaSumpLedger::TraderLocation, FRotator::ZeroRotator, Params))
            Point->Configure(MemoriaSumpLedger::TraderPoint, TEXT("Nervous Trader  |  E / A: talk"));
    if (bLedgerArmed)
        if (auto* Point = World->SpawnActor<AMemoriaStoryPointActor>(MemoriaSumpLedger::LedgerLocation, FRotator::ZeroRotator, Params))
            Point->Configure(MemoriaSumpLedger::LedgerPoint, TEXT("Loose stone  |  E / A: search"));
    // The companion node sits beside the player (verdan_market.gd places her at the player's side).
    APawn* Pawn = World->GetFirstPlayerController() ? World->GetFirstPlayerController()->GetPawn() : nullptr;
    if (Pawn && S.Player.bEliaWithParty)
        if (auto* Elia = World->SpawnActor<AMemoriaEliaCompanion>(Pawn->GetActorLocation() + FVector(-30, -20, 0), FRotator::ZeroRotator, Params))
            Elia->Follow(Pawn);
}
bool UMemoriaNarrativeSubsystem::IsStoryBeatAvailable(const FString& Group) const
{
    const bool bLive = Run && Run->HasActiveRun() && StoryWorld.IsValid() && StoryWorld.Get() == GetWorld();
    if (Group == MemoriaSumpLedger::TraderPoint) return bLive && bTraderArmed && !MemoriaSumpLedger::IsComplete(Run->GetRunSnapshot());
    if (Group == MemoriaSumpLedger::LedgerPoint) return bLive && bLedgerArmed && !Run->GetRunSnapshot().GetFlag(MemoriaSumpLedger::Steps()[1].Flag);
    const auto* Beat = MemoriaVerdanStory::Find(Group);
    return Beat && Run && Run->HasActiveRun() && StoryWorld.IsValid() && StoryWorld.Get() == GetWorld() &&
        ArmedStoryBeats.Contains(Group) && !Run->GetRunSnapshot().GetFlag(Beat->Flag);
}
bool UMemoriaNarrativeSubsystem::StartStoryBeat(const FString& Group)
{
    if (State != EMemoriaSliceState::Exploration || !Context || !IsStoryBeatAvailable(Group)) return false;
    if (Group == MemoriaSumpLedger::TraderPoint || Group == MemoriaSumpLedger::LedgerPoint) return HandleSumpLedger(Group);
    const auto* Beat = MemoriaVerdanStory::Find(Group);
    const FString Path = FString(TEXT("/Game/Memoria/Generated/Narrative/")) + Beat->Asset + TEXT(".") + Beat->Asset;
    if (!LoadObject<UMemoriaFieldAsset>(nullptr, *Path)) { Record(TEXT("error:missing_story_contract:") + Group); return false; }
    // Source body_entered: set the one-time flag, then load_and_start the group.
    Run->SetStoryFlag(Beat->Flag, true); Record(FString(TEXT("flag:")) + Beat->Flag);
    ArmedStoryBeats.Remove(Group);
    return StartStoryField(Group, Beat->Asset);
}
bool UMemoriaNarrativeSubsystem::StartStoryField(const FString& Group, const TCHAR* Asset, const TCHAR* File)
{
    const FString Path = FString(TEXT("/Game/Memoria/Generated/Narrative/")) + Asset + TEXT(".") + Asset;
    StoryAsset = LoadObject<UMemoriaFieldAsset>(nullptr, *Path);
    if (!StoryAsset) { Record(TEXT("error:missing_story_contract:") + Group); return false; }
    // The trace names the authored file the source loads (Elia's reactions come from Chapter 1).
    Record(FString(TEXT("request:res://")) + File + TEXT("::") + Group);
    DeferredInteraction.Reset(); ActiveFieldAsset = StoryAsset; ++FieldInvocationCount;
    Field = MakeUnique<FMemoriaFieldInterpreter>(StoryAsset->Definition, *Context);
    State = EMemoriaSliceState::Field; Record(TEXT("field:start:") + Group);
    Field->Start(); FlushEvents(TEXT("field")); return true;
}
bool UMemoriaNarrativeSubsystem::HandleSumpLedger(const FString& Point)
{
    using namespace MemoriaSumpLedger;
    const auto& Flags = Steps();
    if (Point == LedgerPoint)
    {
        // Ledger body_entered: advance to found, ui_select, then the found group.
        Run->SetStoryFlag(Flags[1].Flag, true); Record(FString(TEXT("flag:")) + Flags[1].Flag); bLedgerArmed = false;
        if (auto* Audio = GetGameInstance()->GetSubsystem<UMemoriaAudioSubsystem>()) Audio->PlaySfx(TEXT("ui_select"));
        return StartStoryField(TEXT("sq_sump_ledger_found"), TEXT("DA_Field_SumpLedgerFound"));
    }
    switch (TraderAction(Run->GetRunSnapshot()))
    {
    case ETraderAction::Start:
        Run->SetStoryFlag(Flags[0].Flag, true); Record(FString(TEXT("flag:")) + Flags[0].Flag);
        return StartStoryField(TEXT("sq_sump_ledger_start"), TEXT("DA_Field_SumpLedgerStart"));
    case ETraderAction::Return:
    {
        // advance_step on the last flag grants rewards before the return group starts:
        // Grains, then items (add_item toast), then the memory, then the completion toast.
        Run->SetStoryFlag(Flags.Last().Flag, true); Record(FString(TEXT("flag:")) + Flags.Last().Flag);
        Run->State.Player.Grains += RewardGrains; Notice(FString::Printf(TEXT("+%lld Grains"), RewardGrains));
        auto* Item = Run->State.Player.Items.FindByPredicate([](const auto& I){ return I.Id.Equals(RewardItem, ESearchCase::CaseSensitive); });
        if (Item) Item->Count += RewardItemCount; else { FMemoriaItemCount New; New.Id = RewardItem; New.Count = RewardItemCount; Run->State.Player.Items.Add(New); }
        Run->State.RecordRecentItem(RewardItem); Notice(FString::Printf(TEXT("+%lld Hi-Potion"), RewardItemCount));
        if (Run->AcquireMemory(RewardMemory()) != EMemoriaMemoryResult::Success) Record(TEXT("error:quest_memory_rejected"));
        Notice(TEXT("Quest Complete: ") + Title(false)); Record(TEXT("quest:complete:sump_ledger")); bNoticeHeld = true;
        return StartStoryField(TEXT("sq_sump_ledger_return"), TEXT("DA_Field_SumpLedgerReturn"));
    }
    case ETraderAction::Remind:
        Notice(TEXT("Find the ledger in the Sump.")); ++Revision; return true;
    default: return false;
    }
}
void UMemoriaNarrativeSubsystem::Notice(const FString& Text)
{
    // World time restarts on every Verdan load, so a notice belongs to the world that raised it.
    if (!GetWorld() || NoticeWorld.Get() != GetWorld() || GetWorld()->GetTimeSeconds() - NoticeTime > 4.0) Notices.Reset();
    Notices.Add(Text); NoticeWorld = GetWorld(); NoticeTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0; Record(TEXT("toast:") + Text);
}
FString UMemoriaNarrativeSubsystem::GetExplorationNotice() const
{
    return GetWorld() && NoticeWorld.Get() == GetWorld() && GetWorld()->GetTimeSeconds() - NoticeTime <= 4.0 ? FString::Join(Notices, TEXT("\n")) : FString();
}
FString UMemoriaNarrativeSubsystem::GetQuestTrackerLine() const
{
    if (!Run || !Run->HasActiveRun() || !MemoriaSumpLedger::IsActive(Run->GetRunSnapshot())) return FString();
    const bool bKo = Run->GetRunSnapshot().CurrentLocale == TEXT("ko");
    return (bKo ? TEXT("퀘스트  ") : TEXT("QUEST  ")) + MemoriaSumpLedger::Title(bKo) + TEXT("  -  ") + MemoriaSumpLedger::CurrentStepText(Run->GetRunSnapshot(), bKo);
}
bool UMemoriaNarrativeSubsystem::InteractWithElia()
{
    if (State != EMemoriaSliceState::Exploration || !Context || !StoryWorld.IsValid() || StoryWorld.Get() != GetWorld()) return false;
    const auto& S = Run->GetRunSnapshot();
    // PerceptionFilter.take_burn_reaction, in the order verdan_market.gd sets the metadata.
    for (const auto& R : MemoriaVerdanStory::EliaReactions())
    {
        const FString Heard = FString(TEXT("burn_reaction_heard_")) + R.Group;
        if (!Run->GetPlayerMemory()->GetSnapshot().BurnedHistory.Contains(R.Memory) || S.GetFlag(Heard)) continue;
        Run->SetStoryFlag(Heard, true); Record(TEXT("flag:") + Heard);
        return StartStoryField(R.Group, R.Asset, R.File);
    }
    const FString Talked = MemoriaVerdanStory::EliaTalkFlag;
    if (bEliaTalkCached || S.GetFlag(Talked))
    {
        // start_dialogue([{speaker: npc_name, text: repeat_line, portrait: ""}]).
        if (!EliaRepeatAsset)
        {
            EliaRepeatAsset = NewObject<UMemoriaFieldAsset>(this);
            EliaRepeatAsset->Definition.Id = TEXT("elia_repeat"); EliaRepeatAsset->Definition.IndexMappingVersion = 1;
            FMemoriaFieldRow Row; Row.Id = TEXT("field/elia_repeat/0"); Row.EffectPhase = TEXT("gate_then_effects");
            Row.Text.bHasSpeaker = true; Row.Text.Speaker = TEXT("Elia"); Row.Text.bHasText = true; Row.Text.Text = MemoriaVerdanStory::EliaRepeatLine;
            EliaRepeatAsset->Definition.Rows.Add(Row);
        }
        Record(TEXT("request:elia:repeat_line")); DeferredInteraction.Reset(); StoryAsset = EliaRepeatAsset; ActiveFieldAsset = EliaRepeatAsset; ++FieldInvocationCount;
        Field = MakeUnique<FMemoriaFieldInterpreter>(EliaRepeatAsset->Definition, *Context);
        State = EMemoriaSliceState::Field; Record(TEXT("field:start:elia_repeat"));
        Field->Start(); FlushEvents(TEXT("field")); return true;
    }
    // First talk: cache now, set the talked flag on dialogue_ended.
    bEliaTalkCached = true;
    if (!StartStoryField(MemoriaVerdanStory::EliaDialogueKey, TEXT("DA_Field_EliaCh2Talk"))) { bEliaTalkCached = false; return false; }
    EliaTalkAsset = StoryAsset; return true;
}
void UMemoriaNarrativeSubsystem::Explore()
{
    Field.Reset(); ActiveFieldAsset = nullptr; State = EMemoriaSliceState::Exploration; bPaused = false;
    if (bNoticeHeld && GetWorld()) { NoticeTime = GetWorld()->GetTimeSeconds(); bNoticeHeld = false; }
    Record(TEXT("exploration:ready")); ++Revision;
}
void UMemoriaNarrativeSubsystem::AfterVN()
{
    const int32 From = EventCursor;
    FlushEvents(TEXT("vn"));
    for (int32 I = From; I < Context->Events.Num(); ++I)
    {
        const FString Event = Context->Events[I];
        if (Event.StartsWith(TEXT("ledger:"))) ShowChapterLedger(Event);
        else if (Event.StartsWith(TEXT("autosave:"))) Autosave();
    }
    PresentVNStep();
    if (!Context->RequestedMap.IsEmpty())
    {
        if (Context->RequestedMap.Equals(TEXT("res://scenes/maps/verdan_market.tscn"), ESearchCase::CaseSensitive))
        {
            State = EMemoriaSliceState::Travelling;
            Record(TEXT("travel:verdan"));
            UGameplayStatics::OpenLevel(this, FName(VerdanMap));
        }
        else if (const FString Map = FPaths::GetBaseFilename(Context->RequestedMap); Context->RequestedMap.StartsWith(TEXT("res://scenes/maps/")) && MemoriaChapterMaps::Find(Map))
        {
            // ch5_classifier ends goto_map drift_shelter: the chapter map takes the run up again.
            State = EMemoriaSliceState::Travelling;
            Record(TEXT("chapter:travel:") + Map);
            UGameplayStatics::OpenLevel(this, FName(*MemoriaChapterMaps::LevelPath(Map)));
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
        else if (OriginalChoice==4 && !bCheckpointLoadFailed && Run->GetRunSnapshot().GetFlag(TEXT("ch2_complete")))
            TravelToChapterMap(TEXT("belt_waystation"));
        return;
    }
    if (State == EMemoriaSliceState::VN && VN)
    {
        const auto Choices = VN->VisibleOriginalIndices();
        const auto Cursor = VN->ExportContinuation().Current.OriginalIndex;
        if (VN->GetDefinition().Steps.IsValidIndex(Cursor) && VN->GetDefinition().Steps[Cursor].bChoicesPresent)
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
                V.Body+=TEXT("\n\nThis checkpoint includes memories, Grains and items at the end of the exchange.\nLoad this checkpoint to revisit Verdan and enter ambient encounters, or take the Belt road on to Chapter 3.");
            if (bCheckpointScreen && !bCheckpointLoadFailed) V.Choices.Add({3,TEXT("Return to Verdan")});
            V.Choices.Add({1,TEXT("Load checkpoint")});
            V.Choices.Add(bCheckpointLoadFailed ? FMemoriaPresentedChoice{2,TEXT("Start a new slice")} : FMemoriaPresentedChoice{0,TEXT("Save checkpoint again")});
            // S320: verdan_market.gd travels on to the Belt Waystation once Chapter 2 closes.
            if (!bCheckpointLoadFailed && Run->GetRunSnapshot().GetFlag(TEXT("ch2_complete"))) V.Choices.Add({4,TEXT("Travel on: Chapter 3, The Belt")});
            return V;
        }
    }
    FMemoriaNarrativeView View; View.bPaused = bPaused;
    if (!Context) return View;
    const FMemoriaNarrativeText* Text = nullptr;
    if (State == EMemoriaSliceState::VN && VN)
    {
        const auto& Definition = VN->GetDefinition();
        int32 Index = VN->ExportContinuation().Current.OriginalIndex;
        const bool Ko = Context->Run.CurrentLocale == TEXT("ko");
        const auto& Meta = Definition.Metadata;
        const FString Title = Ko && Meta.bHasTitleKo ? Meta.TitleKo : Meta.Title;
        View.Header = bNewGameRoute ? FString::Printf(TEXT("%s / VN   %d / %d"), *Title.ToUpper(), Index + 1, Definition.Steps.Num())
            : FString::Printf(TEXT("CH2 ARRIVAL / VN   %d / %d"), Index + 1, Definition.Steps.Num());
        if (Definition.Steps.IsValidIndex(Index))
        {
            const auto& Step = Definition.Steps[Index]; const auto Display = Context->VNDisplay(Step);
            View.LocationTitle = bNewGameRoute ? (Ko ? Title : Title.ToUpper()).Replace(TEXT(", "), TEXT("  /  ")) : TEXT("CHAPTER II  /  VERDAN");
            View.BackdropSource = SceneCg;
            // A resumed cursor has no shown history: reconstruct the scene's last authored CG.
            if (View.BackdropSource.IsEmpty())
                for (int32 I = 0; I <= Index; ++I)
                    if (Definition.Steps[I].Presentation.bHasCg) View.BackdropSource = MemoriaNarrativeArtwork::CgSource(Definition.Steps[I].Presentation.Cg);
            View.PortraitSource = MemoriaNarrativeArtwork::PortraitSource(Display.Portrait);
            View.PortraitSide = Step.Presentation.Side;
            View.Speaker = Display.Speaker; View.Narration = Display.Narrate; View.Body = Display.Text; View.bDistorted = Display.bDistorted;
            const auto& T = Step.Text;
            if (T.bHasSystemLog)
            {
                // _show_system_log: the System speaker in the dialogue box, instead of the line.
                View.bSystemLog = true; View.Speaker = Ko ? TEXT("시스템") : TEXT("System"); View.Narration.Reset(); View.PortraitSource.Reset();
                View.Body = Ko && T.bHasSystemLogKo ? T.SystemLogKo : T.SystemLog;
            }
            const auto& P = Step.Presentation;
            View.CueKey = Definition.Id + TEXT(":") + FString::FromInt(Index);
            View.Impact = P.bHasImpact ? P.Impact : FString(); View.CgMotion = P.bHasCgMotion ? P.CgMotion : TEXT("ambient");
            View.bStepHasCg = !Display.Cg.IsEmpty(); View.CgFadeSeconds = P.bHasFadeMs ? P.FadeMs / 1000.f : .8f;
            if (Step.bChoicesPresent)
            {
                View.ChoiceTitle = Ko && T.bHasChoiceTitleKo ? T.ChoiceTitleKo : T.bHasChoiceTitle ? T.ChoiceTitle : (Ko ? TEXT("결정") : TEXT("DECISION"));
                View.ChoiceHint = Ko && T.bHasChoiceHintKo ? T.ChoiceHintKo : T.bHasChoiceHint ? T.ChoiceHint
                    : (Ko ? TEXT("어떤 선택은 아렐이 지킬 것, 잃을 것, 살아남는 방식을 바꿉니다.") : TEXT("Some choices change what Arrel can keep, spend, or survive."));
            }
            for (int32 I : VN->VisibleOriginalIndices())
            {
                const auto& C = Step.Choices[I].Text;
                View.Choices.Add({I, Context->Localized(C), Ko && C.bHasEffectKo ? C.EffectKo : C.bHasEffect ? C.Effect : FString()});
            }
        }
        View.LedgerSerial = LedgerSerial; View.LedgerTitle = LedgerTitle; View.LedgerLines = LedgerLines; View.bLedgerThreadHolds = bLedgerThreadHolds;
    }
    else if (State == EMemoriaSliceState::Field && Field)
    {
        int32 Index = Field->OriginalIndex();
        const auto* ChapterSpec = ChapterMap.IsEmpty() ? nullptr : MemoriaChapterMaps::Find(ChapterMap);
        View.Header = ChapterSpec ? FString::Printf(TEXT("%s / STORY   %d / %d"), *ChapterSpec->TitleName.ToUpper(), Index + 1, ActiveFieldAsset->Definition.Rows.Num()) :
            FString::Printf(TEXT("%s   %d / %d"), ActiveFieldAsset == FieldAsset ? TEXT("VN-UNSEEN FIELD FIXTURE") : ActiveFieldAsset == EncounterAsset ? TEXT("MALET / ENCOUNTER") : ActiveFieldAsset == RefusedAsset ? TEXT("MALET / REFUSAL") : ActiveFieldAsset == DealAsset ? TEXT("MALET / DEAL") : ActiveFieldAsset == RewardAsset ? TEXT("MALET / REWARD") : ActiveFieldAsset == StoryAsset ? TEXT("VERDAN / STORY") : TEXT("MALET / MEMORY REACTION"), Index + 1, ActiveFieldAsset->Definition.Rows.Num());
        if (ActiveFieldAsset->Definition.Rows.IsValidIndex(Index))
        {
            const auto& Row = ActiveFieldAsset->Definition.Rows[Index]; Text = &Row.Text;
            View.LocationTitle = ActiveFieldAsset == FieldAsset ? TEXT("CHAPTER II  /  VERDAN") : TEXT("THE SUMP  /  MALET");
            if (const auto* Beat = ActiveFieldAsset && ActiveFieldAsset == StoryAsset ? MemoriaVerdanStory::Find(ActiveFieldAsset->Definition.Id) : nullptr) View.LocationTitle = Beat->Title;
            else if (ActiveFieldAsset && ActiveFieldAsset == StoryAsset)
                View.LocationTitle = ActiveFieldAsset->Definition.Id.StartsWith(TEXT("elia")) ? TEXT("VERDAN  /  ELIA") : TEXT("VERDAN  /  THE SUMP LEDGER");
            // Sequences without a CG use the already-authored encounter location.
            View.BackdropSource = ActiveFieldAsset == FieldAsset ? FString() : TEXT("res://assets/cg/generated/story_ch2_malet_cellar.png");
            // Elia talks happen in the open market, not Malet's cellar.
            if (ActiveFieldAsset == StoryAsset && ActiveFieldAsset->Definition.Id.StartsWith(TEXT("elia"))) View.BackdropSource = TEXT("res://assets/cg/generated/chapter_splash_verdan_market.png");
            // S320: a chapter map's story plays over its splash, under "CHAPTER N / PLACE".
            if (ChapterSpec)
            {
                const bool Ko = Run->GetRunSnapshot().CurrentLocale == TEXT("ko");
                View.LocationTitle = Ko ? FString::Printf(TEXT("%d장  /  %s"), ChapterSpec->Chapter, *MemoriaChapterMaps::Korean(ChapterSpec->TitleName)) : FString::Printf(TEXT("CHAPTER %d  /  %s"), ChapterSpec->Chapter, *ChapterSpec->TitleName.ToUpper());
                View.BackdropSource = ChapterSpec->Splash;
            }
            for (int32 I = 0; I <= Index; ++I)
                if (ActiveFieldAsset->Definition.Rows[I].Presentation.bHasCg) View.BackdropSource = ActiveFieldAsset->Definition.Rows[I].Presentation.Cg;
            View.PortraitSource = MemoriaNarrativeArtwork::PortraitSource(Row.Presentation.bHasBurnedPortrait && Context->UsesBurnedText(Row.Text) ? Row.Presentation.BurnedPortrait : Row.Presentation.Portrait);
            View.PortraitSide = Row.Text.Speaker == TEXT("Elia") ? TEXT("right") : TEXT("left");
            View.CueKey = TEXT("field:") + ActiveFieldAsset->Definition.Id + TEXT(":") + FString::FromInt(Index);
            View.bStepHasCg = Row.Presentation.bHasCg && !Row.Presentation.Cg.IsEmpty();
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
    View.SpeakerLabel = Run->State.CurrentLocale == TEXT("ko") ? SpeakerKo(View.Speaker) : View.Speaker;
    View.BurnSerial = BurnSerial;
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
    // The cursor names its scene: the Ch2 arrival checkpoint or a Chapter 1 route autosave.
    const FString& Scene = Save.SceneFlow.bActive && !Save.SceneFlow.Current.SequenceId.IsEmpty() ? Save.SceneFlow.Current.SequenceId : Save.SceneFlow.Pending.SequenceId;
    const auto* Asset = Scene.IsEmpty() ? VNAsset.Get() : ResolveVN(Scene);
    if (!Asset) return false;
    // A run that already crossed a chapter transition came through the Chapter 1 route.
    const bool Route = !Asset->Definition.Id.Equals(VNAsset->Definition.Id, ESearchCase::CaseSensitive) || !Save.PlayerMemory.VigilChapters.IsEmpty();
    auto Snapshot = Save.Run; FMemoriaNarrativeContext CheckContext(Snapshot, *Candidate);
    FMemoriaVNInterpreter Check(Asset->Definition, CheckContext, Resolver(), Route);
    if (!Check.PrepareResume(Save.SceneFlow)) return false;
    if (!Run->RestoreSave(Save)) return false;
    Context = MakeUnique<FMemoriaNarrativeContext>(Run->State, *Run->GetPlayerMemory());
    VN = MakeUnique<FMemoriaVNInterpreter>(Asset->Definition, *Context, Resolver(), Route);
    bNewGameRoute = Route;
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
    if (EliaTalkAsset && ActiveFieldAsset == EliaTalkAsset)
    { Run->SetStoryFlag(MemoriaVerdanStory::EliaTalkFlag, true); Record(FString(TEXT("flag:")) + MemoriaVerdanStory::EliaTalkFlag); EliaTalkAsset = nullptr; }
    if (ActiveFieldAsset != EncounterAsset && ActiveFieldAsset != RefusedAsset && ActiveFieldAsset != DealAsset && ActiveFieldAsset != RewardAsset)
    {
        const FString Finished = ActiveFieldAsset ? ActiveFieldAsset->Definition.Id : FString();
        Explore(); OnFieldFinished.Broadcast(Finished); return;
    }
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
bool UMemoriaNarrativeSubsystem::EnterChapterMap(const FString& Map)
{
    const auto* Spec = MemoriaChapterMaps::Find(Map);
    if (!Spec) { Record(TEXT("error:missing_chapter_map:") + Map); return false; }
    if (!Run->HasActiveRun())
    {
        // A development entry straight into the map: New Game's player at the map's chapter.
        if (Run->BeginStartingMemoryRun(Spec->Chapter) != EMemoriaMemoryResult::Success) return false;
        auto& Player = Run->State.Player; Player = FMemoriaPlayerState();
        Player.Hp = Player.MaxHp = 100; Player.bEliaWithParty = true;
        FMemoriaItemCount Ink; Ink.Id = TEXT("witness_ink"); Ink.Count = 1; Player.Items = {Ink};
        Player.QuickSlots = {TEXT("witness_ink"), TEXT("potion"), TEXT("antidote")};
        Run->State.CurrentLocale = GetGameInstance()->GetSubsystem<UMemoriaSettingsSubsystem>()->GetLocale();
        Record(TEXT("chapter:development_run"));
    }
    // The chapter a map belongs to becomes the run's (belt_waystation.gd arrives in Chapter 3).
    if (Run->State.CurrentChapter < Spec->Chapter) Run->State.CurrentChapter = Spec->Chapter;
    VN.Reset(); Field.Reset(); ActiveFieldAsset = nullptr; bTitle = false; bPaused = false; DeferredInteraction.Reset();
    bCheckpointScreen = bCheckpointLoadFailed = false; CheckpointWorld.Reset();
    // A fresh context starts a fresh event list; the cursor follows it.
    Context = MakeUnique<FMemoriaNarrativeContext>(Run->State, *Run->GetPlayerMemory()); EventCursor = 0;
    ChapterMap = Map; State = EMemoriaSliceState::Exploration;
    Record(TEXT("chapter:enter:") + Map); ++Revision;
    return true;
}
bool UMemoriaNarrativeSubsystem::StartChapterField(const FString& Group, const FString& Asset, const FString& File)
{
    if (ChapterMap.IsEmpty() || State != EMemoriaSliceState::Exploration) return false;
    return StartStoryField(Group, *Asset, *File);
}
bool UMemoriaNarrativeSubsystem::EnterStoryScene(const FString& Scene)
{
    if (Scene.EndsWith(TEXT("ch5_classifier_entry.tscn"))) return EnterClassifier();
    Record(TEXT("error:unported_story_scene:") + Scene); return false;
}
bool UMemoriaNarrativeSubsystem::EnterClassifier()
{
    // prepare_classifier_entry: only from the Chapter 4 boundary (or a Chapter 5 entry already begun), and
    // never once the story has reached Chapter 6.
    if (!Run->HasActiveRun() || !Context || Run->State.GetFlag(TEXT("canon_ch6_seam_ready"))) return false;
    const bool bBoundary = Run->State.GetFlag(TEXT("canon_ch5_classifier_ready"));
    const bool bResuming = Run->State.GetFlag(TEXT("ch5_classifier_started")) && Run->State.CurrentChapter == 5;
    if (!bBoundary && !bResuming) return false;
    const auto* Scene = ResolveVN(MemoriaClassifier::Scene);
    if (!Scene) { Record(TEXT("error:missing_vn:ch5_classifier")); return false; }
    if (bBoundary) Run->SetStoryFlag(TEXT("canon_ch5_classifier_ready"), false);
    Run->SetStoryFlag(TEXT("ch5_classifier_started"), true); Run->SetStoryFlag(TEXT("ch5_kairos_seen"), true);
    Run->SetCurrentChapter(5);
    const FString Outcome = MemoriaClassifier::ResolveReport(*Run->GetWorldCognition());
    if (Outcome.IsEmpty()) { Record(TEXT("error:classifier_report")); return false; }
    // _project_report_flags: the flags only select the VN's authored lines.
    Run->SetStoryFlag(TEXT("ch5_malet_report_identified_arrel"), Outcome == MemoriaClassifier::Identified);
    Run->SetStoryFlag(TEXT("ch5_malet_report_requester_unknown"), Outcome == MemoriaClassifier::Unknown);
    Record(TEXT("classifier:report:") + Outcome);
    Field.Reset(); ActiveFieldAsset = nullptr; DeferredInteraction.Reset();
    VN = MakeUnique<FMemoriaVNInterpreter>(Scene->Definition, *Context, Resolver(), true); bNewGameRoute = true;
    State = EMemoriaSliceState::VN; Record(TEXT("vn:start:ch5_classifier")); ++Revision;
    VN->Play(); AfterVN(); return true;
}
bool UMemoriaNarrativeSubsystem::TravelToChapterMap(const FString& Map)
{
    if (!MemoriaChapterMaps::Find(Map) || !Run->HasActiveRun()) return false;
    // The run travels with the game instance; the map's presentation takes it up on arrival.
    CheckpointWorld.Reset(); bCheckpointScreen = false; CancelMaletDelay(); RewardCallbackWorld.Reset();
    VN.Reset(); Field.Reset(); ActiveFieldAsset = nullptr;
    State = EMemoriaSliceState::Travelling; Record(TEXT("chapter:travel:") + Map); ++Revision;
    UGameplayStatics::OpenLevel(this, FName(*MemoriaChapterMaps::LevelPath(Map)));
    return true;
}
