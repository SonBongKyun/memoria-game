#include "Battle/MemoriaBattleEntrySubsystem.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
#include "MemoriaBattleSource.inl"
using Obj = TSharedPtr<FJsonObject>;
using Val = TSharedPtr<FJsonValue>;
const Obj& Source()
{
    static const Obj Catalog = [] {
        Obj Result;
        FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(FString(MemoriaBattleSourceJson)), Result);
        return Result;
    }();
    return Catalog;
}
FString Message(const FMemoriaBattleEntryView& V, const TCHAR* SourceKey)
{
    // The key identifies an extracted _bl call. Authored text comes from the generated catalog.
    const auto Messages = Source()->GetObjectField(TEXT("messages"))->GetObjectField(V.bKo ? TEXT("ko") : TEXT("en"));
    FString Result;
    Messages->TryGetStringField(SourceKey, Result);
    return Result;
}
FString Format(FString Text, std::initializer_list<FString> Values)
{
    for (const FString& Value : Values)
    {
        const int32 StringAt = Text.Find(TEXT("%s"), ESearchCase::CaseSensitive);
        const int32 NumberAt = Text.Find(TEXT("%d"), ESearchCase::CaseSensitive);
        const int32 At = StringAt == INDEX_NONE ? NumberAt :
            NumberAt == INDEX_NONE ? StringAt : FMath::Min(StringAt, NumberAt);
        if (At != INDEX_NONE) Text = Text.Left(At) + Value + Text.Mid(At + 2);
    }
    return Text;
}
Obj Event(const TCHAR* Kind)
{
    auto E = MakeShared<FJsonObject>(); E->SetStringField(TEXT("event"), Kind); return E;
}
void AddEvent(TArray<Val>& Events, const Obj& E) { Events.Add(MakeShared<FJsonValueObject>(E)); }
FString Encode(const TArray<Val>& Events)
{
    FString Text; FJsonSerializer::Serialize(Events, TJsonWriterFactory<>::Create(&Text)); return Text;
}
TArray<Val> Decode(const FString& Text)
{
    TArray<Val> Events;
    FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Events);
    return Events;
}
void NumberEvent(TArray<Val>& Events, const TCHAR* Kind, double Value)
{
    auto E = Event(Kind); E->SetNumberField(TEXT("value"), Value); AddEvent(Events, E);
}
void MomentumEvent(TArray<Val>& Events, const FMemoriaBattleEntryView& V)
{
    auto E = Event(TEXT("momentum")); E->SetNumberField(TEXT("value"), V.Momentum);
    E->SetNumberField(TEXT("rank"), V.MomentumRank); E->SetStringField(TEXT("label"), V.MomentumLabel); AddEvent(Events, E);
}
void SetFlag(FMemoriaRunSnapshot& State, const TCHAR* Id)
{
    if (auto* F = State.StoryFlags.FindByPredicate([&](const auto& X) { return X.Id.Equals(Id, ESearchCase::CaseSensitive); }))
        F->bValue = true;
    else State.StoryFlags.Add({FString(Id), true});
}
uint32 SourceStringHash(const FString& Text)
{
    // Godot String.hash(): unsigned 32-bit DJB2 addition. All seed identifiers are ASCII.
    uint32 Hash = 5381;
    for (const TCHAR C : Text) Hash = Hash * 33u + static_cast<uint32>(C);
    return Hash;
}
FString LocalizedObjective(const TCHAR* Table, const FString& Raw, bool bKo)
{
    FString Result;
    if (bKo && Source()->GetObjectField(Table)->TryGetStringField(Raw, Result)) return Result;
    return Raw;
}
bool SetupObjective(FMemoriaBattleEntryView& V, const FMemoriaRunSnapshot& State, TArray<Val>& Events)
{
    const FString Seed = V.EnemyName + TEXT(":") + Source()->GetStringField(TEXT("return_scene")) +
        TEXT(":") + FString::Printf(TEXT("%lld"), State.TotalBattles);
    Obj Best; uint32 BestHash = MAX_uint32;
    for (const auto& Row : Source()->GetArrayField(TEXT("objectives")))
    {
        const Obj Candidate = Row->AsObject();
        const FString Id = Candidate->GetStringField(TEXT("id"));
        if ((Id == TEXT("force_break") || Id == TEXT("limit_release")) && V.EnemyMaxHp < 160) continue;
        if (Id == TEXT("stance_shift") && State.CurrentChapter < 4) continue;
        if (Id == TEXT("ally_coordination") && !V.bSableInParty && !V.bTobiasInParty) continue;
        const uint32 Rank = SourceStringHash(Seed + TEXT(":") + Id);
        if (!Best || Rank < BestHash) { Best = Candidate; BestHash = Rank; }
    }
    if (!Best) return false;
    V.ObjectiveId = Best->GetStringField(TEXT("id"));
    const FString RawTitle = Best->GetStringField(TEXT("title")), RawDesc = Best->GetStringField(TEXT("desc"));
    V.ObjectiveTitle = LocalizedObjective(TEXT("objective_titles_ko"), RawTitle, V.bKo);
    V.ObjectiveDescription = LocalizedObjective(TEXT("objective_descriptions_ko"), RawDesc, V.bKo);
    V.ObjectiveRewardGrains = static_cast<int64>(Best->GetNumberField(TEXT("reward_grains"))) + (V.bFieldFocusOpening ? 2 : 0);
    V.ObjectiveRewardHeal = static_cast<int64>(Best->GetNumberField(TEXT("reward_heal")));
    V.ObjectiveRewardItem = Best->GetStringField(TEXT("reward_item"));
    V.bObjectiveFocusBoosted = V.bFieldFocusOpening;
    if (V.ObjectiveId == TEXT("witness_echo")) V.ObjectiveProgressTarget = V.WitnessRequired;
    else if (V.ObjectiveId == TEXT("combo_three")) V.ObjectiveProgressTarget = 3;
    else if (V.ObjectiveId == TEXT("swift_finish")) V.ObjectiveProgressTarget = 4;
    else if (V.ObjectiveId == TEXT("kindle_momentum"))
    { V.ObjectiveProgressCurrent = FMath::Min(static_cast<int32>(V.Momentum), 75); V.ObjectiveProgressTarget = 75; }
    else if (V.ObjectiveId == TEXT("stance_shift") || V.ObjectiveId == TEXT("ally_coordination")) V.ObjectiveProgressTarget = 2;
    if (V.ObjectiveId == TEXT("keep_memory")) V.ObjectiveProgress = Format(Message(V, TEXT("Memories burned: %d")), {TEXT("0")});
    else if (V.ObjectiveId == TEXT("no_items")) V.ObjectiveProgress = Format(Message(V, TEXT("Items used: %d")), {TEXT("0")});
    else if (V.ObjectiveId == TEXT("swift_finish")) V.ObjectiveProgress = Format(Message(V, TEXT("Actions: %d / 4 max")), {TEXT("0")});
    else V.ObjectiveProgress = FString::Printf(TEXT("%d / %d"), V.ObjectiveProgressCurrent, V.ObjectiveProgressTarget);
    V.Logs.Add(Format(Message(V, TEXT("[OBJECTIVE] %s - %s")), {RawTitle, RawDesc}));
    auto Objective = MakeShared<FJsonObject>(*Best);
    Objective->SetNumberField(TEXT("reward_grains"), V.ObjectiveRewardGrains);
    if (V.bFieldFocusOpening) Objective->SetBoolField(TEXT("focus_boosted"), true);
    Objective->SetStringField(TEXT("status"), TEXT("active"));
    Objective->SetBoolField(TEXT("complete"), false); Objective->SetBoolField(TEXT("failed"), false);
    Objective->SetNumberField(TEXT("progress_current"), V.ObjectiveProgressCurrent);
    Objective->SetNumberField(TEXT("progress_target"), V.ObjectiveProgressTarget);
    Objective->SetStringField(TEXT("progress_text"), V.ObjectiveProgress);
    auto E = Event(TEXT("objective")); E->SetObjectField(TEXT("value"), Objective); AddEvent(Events, E);
    return true;
}
bool ApplyModifier(FMemoriaBattleEntryView& V, int32 BurnCount, FMemoriaEncounterRng& Rng)
{
    if (BurnCount <= 2) return true;
    if (!Rng.Real || !Rng.Integer) return false;
    const double Roll = Rng.Real(0., 1.);
    if (!FMath::IsFinite(Roll) || Roll < 0. || Roll > 1.) return false;
    const double Chance = BurnCount <= 5 ? .4 : BurnCount <= 8 ? .6 : .8;
    if (Roll > Chance) return true;
    const Obj Modifiers = Source()->GetObjectField(TEXT("modifiers"));
    TArray<Val> Pool;
    if (BurnCount <= 8) Pool.Append(Modifiers->GetArrayField(TEXT("mid")));
    if (BurnCount >= 6) Pool.Append(Modifiers->GetArrayField(TEXT("high")));
    if (BurnCount >= 9) Pool.Append(Modifiers->GetArrayField(TEXT("extreme")));
    const int32 Index = Rng.Integer(0, Pool.Num() - 1);
    if (!Pool.IsValidIndex(Index)) return false;
    const Obj M = Pool[Index]->AsObject();
    V.ModifierId = M->GetStringField(TEXT("id")); V.ModifierName = M->GetStringField(TEXT("name"));
    V.ModifierDescription = M->GetStringField(TEXT("desc")); V.ModifierEffect = M->GetStringField(TEXT("effect"));
    V.ModifierValue = static_cast<int64>(M->GetNumberField(TEXT("value")));
    if (V.ModifierEffect == TEXT("enemy_atk_up")) V.EnemyAttack += V.ModifierValue;
    else if (V.ModifierEffect == TEXT("enemy_ability_add"))
    {
        TArray<FString> Available;
        for (const auto& A : Source()->GetArrayField(TEXT("extra_abilities")))
            if (!V.EnemyAbilities.Contains(A->AsString())) Available.Add(A->AsString());
        if (!Available.IsEmpty())
        {
            const int32 Ability = Rng.Integer(0, Available.Num() - 1);
            if (!Available.IsValidIndex(Ability)) return false;
            V.EnemyAbilities.Add(Available[Ability]);
        }
    }
    else if (V.ModifierEffect == TEXT("void_convert") && !V.bEnemyVoid)
    { V.bEnemyVoid = true; V.Weakness = TEXT("void"); V.EnemyName = TEXT("Void ") + V.EnemyName; }
    V.Logs.Add(Format(Message(V, TEXT("[VOID CORRUPTION] %s")), {V.ModifierName}));
    V.Logs.Add(V.ModifierDescription);
    return true;
}
}
void UMemoriaBattleEntrySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    Collection.InitializeDependency<UMemoriaRunSubsystem>();
    Run = GetGameInstance()->GetSubsystem<UMemoriaRunSubsystem>();
    Run->OnRunReplaced.AddUObject(this, &UMemoriaBattleEntrySubsystem::Cancel);
    FWorldDelegates::OnWorldCleanup.AddUObject(this, &UMemoriaBattleEntrySubsystem::OnWorldCleanup);
}
void UMemoriaBattleEntrySubsystem::Deinitialize()
{
    FWorldDelegates::OnWorldCleanup.RemoveAll(this);
    if (Run) Run->OnRunReplaced.RemoveAll(this);
    Cancel(); Run = nullptr;
    Super::Deinitialize();
}
bool UMemoriaBattleEntrySubsystem::HasLiveOwner() const
{
    if (!Run || !Run->HasActiveRun() || Run->GetRunSnapshot().RunId != OwnerRun || !OwnerWorld.IsValid()) return false;
    const UWorld* World = OwnerWorld.Get();
    if (World->bIsTearingDown || World->GetGameInstance() != GetGameInstance()) return false;
    const FWorldContext* Context = GEngine ? GEngine->GetWorldContextFromWorld(World) : nullptr;
    return !Context || Context->TravelURL.IsEmpty();
}
bool UMemoriaBattleEntrySubsystem::IsActive() const { return View.bActive && HasLiveOwner(); }
bool UMemoriaBattleEntrySubsystem::IsReturning() const { return IsActive() && View.bReturning; }
FMemoriaBattleEntryView UMemoriaBattleEntrySubsystem::GetView() const
{
    auto Result = View;
    Result.Revision = Revision; Result.bActive = IsActive();
    Result.bReturning = Result.bActive && View.bReturning;
    Result.bCanFlee = Result.bActive && !Result.bReturning;
    return Result;
}
void UMemoriaBattleEntrySubsystem::Cancel()
{
    if (OwnerWorld.IsValid()) OwnerWorld->GetTimerManager().ClearTimer(ReturnTimer);
    ReturnTimer.Invalidate(); OwnerWorld.Reset(); OwnerRun.Invalidate();
    View = FMemoriaBattleEntryView{}; View.Revision = ++Revision;
    OnChanged.Broadcast();
}
void UMemoriaBattleEntrySubsystem::OnWorldCleanup(UWorld* World, bool, bool)
{
    if (OwnerWorld.Get() == World) Cancel();
}
bool UMemoriaBattleEntrySubsystem::BeginEncounter(int32 EnemyIndex, FMemoriaEncounterRng& Rng, UWorld* Owner)
{
    if (bBusy || IsActive() || !Run || !Run->HasActiveRun() || !Run->GetPlayerMemory() || !Source()) return false;
    if (!Owner) Owner = GetWorld();
    if (!Owner || Owner->bIsTearingDown || Owner->GetGameInstance() != GetGameInstance()) return false;
    if (const auto* Context = GEngine ? GEngine->GetWorldContextFromWorld(Owner) : nullptr;
        Context && !Context->TravelURL.IsEmpty()) return false;
    const auto& Enemies = Source()->GetArrayField(TEXT("enemy_pool"));
    if (!Enemies.IsValidIndex(EnemyIndex)) return false;
    auto State = Run->GetRunSnapshot();
    if (State.TotalBattles == MAX_int64 || State.CurrentChapter < 1 || State.CurrentChapter > 10) return false;
    TGuardValue<bool> Busy(bBusy, true);
    const uint64 EntryRevision = Revision;
    const FGuid EntryRun = State.RunId;
    const TWeakObjectPtr<UWorld> EntryWorld(Owner);
    FMemoriaBattleEntryView Next; TArray<Val> Events;
    const Obj Enemy = Enemies[EnemyIndex]->AsObject();
    Next.EnemyIndex = EnemyIndex; Next.EnemyName = Enemy->GetStringField(TEXT("name"));
    Next.EnemyHp = Next.EnemyMaxHp = static_cast<int64>(Enemy->GetNumberField(TEXT("hp")));
    Next.EnemyAttack = static_cast<int64>(Enemy->GetNumberField(TEXT("atk")));
    Next.bEnemyVoid = Enemy->GetBoolField(TEXT("is_void")); Next.Weakness = Next.bEnemyVoid ? TEXT("void") : TEXT("fire");
    for (const auto& A : Enemy->GetArrayField(TEXT("abilities"))) Next.EnemyAbilities.Add(A->AsString());
    Next.CurrentLocale = State.CurrentLocale; Next.bKo = State.CurrentLocale == TEXT("ko");
    Next.bEliaInParty = State.Player.bEliaWithParty;
    Next.bEliaCooldownsReset = Next.bEliaInParty;
    Next.bSableInParty = State.GetFlag(TEXT("sable_joined")) && State.CurrentChapter >= 4;
    Next.bTobiasInParty = State.GetFlag(TEXT("tobias_joined")) && State.CurrentChapter >= 3 && State.CurrentChapter < 7;
    const auto& Labels = Source()->GetArrayField(Next.bKo ? TEXT("momentum_labels_ko") : TEXT("momentum_labels"));
    Next.MomentumLabel = Labels[0]->AsString(); MomentumEvent(Events, Next);
    NumberEvent(Events, TEXT("limit"), 0.);
    const double LimitMultiplier = Run->GetPlayerMemory()->HasPassive(TEXT("memory_cascade")) ? 1.2 : 1.;
    const auto AddLimit = [&](double Amount) {
        Next.LimitGauge = FMath::Min(100., Next.LimitGauge + Amount * LimitMultiplier);
        NumberEvent(Events, TEXT("limit"), Next.LimitGauge);
    };
    const int64 Focus = FMath::Clamp<int64>(State.Player.FieldFocus, 0, 3);
    if (Focus > 0)
    {
        State.Player.FieldFocus = Focus - 1;
        auto E = Event(TEXT("focus")); E->SetNumberField(TEXT("value"), Focus - 1); E->SetNumberField(TEXT("cap"), 3); AddEvent(Events, E);
        Next.bFieldFocusOpening = true; Next.Momentum = 25.; Next.MomentumRank = 1; Next.MomentumLabel = Labels[1]->AsString();
        MomentumEvent(Events, Next);
        Next.Logs.Add(Format(Message(Next, TEXT("[RESONANCE] %s state reached. Damage momentum rises.")), {Next.MomentumLabel}));
        State.HighestMomentumRank = FMath::Max<int64>(State.HighestMomentumRank, 1); AddLimit(20.);
    }
    const int64 ChapterHp = 100 + (State.CurrentChapter - 1) * 15;
    if (State.Player.MaxHp < ChapterHp)
    { State.Player.MaxHp = ChapterHp; State.Player.Hp = FMath::Min(State.Player.Hp + 15, ChapterHp); }
    Next.PlayerHp = State.Player.Hp; Next.PlayerMaxHp = State.Player.MaxHp;
    Next.DifficultyBonus = State.CurrentChapter >= 7 ? .15 : 0.;
    ++State.TotalBattles;
    auto Started = Event(TEXT("started")); Started->SetStringField(TEXT("enemy"), Next.EnemyName); AddEvent(Events, Started);
    Next.Logs.Add(Format(Message(Next, TEXT("A %s appears!")), {Next.EnemyName}));
    const FString Hint = Format(Message(Next, TEXT("Exploit %s to build BREAK pressure.")), {Next.Weakness.ToUpper()});
    Next.Logs.Add(Format(Message(Next, TEXT("[TACTIC] %s%s")), {Hint, TEXT("")}));
    if (!SetupObjective(Next, State, Events)) return false;
    Next.Requests.Add(TEXT("tutorial:first_battle"));
    const Obj Environment = Source()->GetObjectField(TEXT("environment"));
    Next.EnvironmentName = Environment->GetStringField(TEXT("name"));
    Next.EnvironmentDescription = Environment->GetStringField(TEXT("desc"));
    Next.Logs.Add(Format(Message(Next, TEXT("[TERRAIN] %s, %s")), {Next.EnvironmentName, Next.EnvironmentDescription}));
    if (!State.GetFlag(TEXT("ch1_opening_trait_spent")))
    {
        if (State.GetFlag(TEXT("burned_for_passage")))
        {
            AddLimit(18.); Next.BreakGauge = 22.;
            auto E = Event(TEXT("break")); E->SetNumberField(TEXT("value"), 22.); E->SetNumberField(TEXT("max"), 100.); AddEvent(Events, E);
            Next.Logs.Add(Message(Next, TEXT("[CHOICE ECHO] The burned song scatters through the ash. Limit +18, BREAK pressure +22.")));
            SetFlag(State, TEXT("ch1_opening_trait_spent"));
            Next.Requests.Add(TEXT("flag:ch1_opening_trait_spent"));
        }
        else if (State.GetFlag(TEXT("refused_to_burn")))
        {
            Next.bPlayerDefending = true; AddLimit(8.);
            Next.Logs.Add(Message(Next, TEXT("[CHOICE ECHO] Arrel keeps the song intact. The first enemy blow is guarded.")));
            SetFlag(State, TEXT("ch1_opening_trait_spent"));
            Next.Requests.Add(TEXT("flag:ch1_opening_trait_spent"));
        }
    }
    if (State.GetFlag(TEXT("listened_to_humming")) && !State.GetFlag(TEXT("ch1_humming_focus_spent")))
    {
        AddLimit(10.); Next.Logs.Add(Message(Next, TEXT("[ANCHOR] Elia's melody steadies Arrel. Limit +10.")));
        SetFlag(State, TEXT("ch1_humming_focus_spent"));
        Next.Requests.Add(TEXT("flag:ch1_humming_focus_spent"));
    }
    // Resolve art, objective, witness requirement, and hints before corruption changes the enemy.
    Next.BackgroundSource = Source()->GetStringField(TEXT("background"));
    const TArray<Val>* Art = nullptr;
    if (Source()->TryGetArrayField(TEXT("enemy_art"), Art) && Art->IsValidIndex(EnemyIndex))
        Next.EnemyImageSource = (*Art)[EnemyIndex]->AsString();
    if (!ApplyModifier(Next, Run->GetPlayerMemory()->GetSnapshot().BurnedHistory.Num(), Rng)) return false;
    // A recorded RNG provider can synchronously restore/cancel. Invalidate on
    // every replacement epoch, including a same-RunId restore, before committing.
    if (Revision != EntryRevision || !Run || !Run->HasActiveRun() ||
        Run->GetRunSnapshot().RunId != EntryRun || !EntryWorld.IsValid() ||
        EntryWorld->bIsTearingDown || EntryWorld->GetGameInstance() != GetGameInstance()) return false;
    if (const auto* Context = GEngine ? GEngine->GetWorldContextFromWorld(EntryWorld.Get()) : nullptr;
        Context && !Context->TravelURL.IsEmpty()) return false;
    AddEvent(Events, Event(TEXT("player_turn")));
    Next.SourceEventsJson = Encode(Events); Next.BattleState = TEXT("PLAYER_TURN");
    Next.bActive = Next.bCanFlee = true;
    // Commit only after every catalog/RNG prerequisite succeeds. Never writes a checkpoint.
    if (OwnerWorld.IsValid()) OwnerWorld->GetTimerManager().ClearTimer(ReturnTimer);
    OwnerWorld = Owner; OwnerRun = State.RunId; Next.Revision = ++Revision;
    Run->State = MoveTemp(State); View = MoveTemp(Next);
    OnChanged.Broadcast();
    return true;
}
bool UMemoriaBattleEntrySubsystem::Flee(uint64 ExpectedRevision)
{
    if (bBusy || !IsActive() || View.bReturning || Revision != ExpectedRevision) return false;
    TGuardValue<bool> Busy(bBusy, true);
    View.bReturning = true; View.bCanFlee = false; View.BattleState = TEXT("FLED");
    View.Logs.Add(Message(View, TEXT("Arrel withdraws before the memory closes around him.")));
    auto Events = Decode(View.SourceEventsJson);
    auto Ended = Event(TEXT("ended")); Ended->SetNumberField(TEXT("state"), 5); AddEvent(Events, Ended);
    View.SourceEventsJson = Encode(Events); View.Revision = ++Revision;
    // Ambient Flee is guaranteed, including corruption-converted Void enemies.
    OwnerWorld->GetTimerManager().SetTimer(ReturnTimer, this, &UMemoriaBattleEntrySubsystem::FinishReturn, .3f, false);
    OnChanged.Broadcast();
    return true;
}
void UMemoriaBattleEntrySubsystem::FinishReturn()
{
    if (!HasLiveOwner() || !View.bReturning) { Cancel(); return; }
    TGuardValue<bool> Busy(bBusy, true);
    ReturnTimer.Invalidate();
    View.bActive = View.bReturning = View.bCanFlee = false;
    const FString ChapterKey = FString::Printf(TEXT("%lld"), Run->GetRunSnapshot().CurrentChapter);
    View.Requests.Add(TEXT("request:presence:") + Source()->GetObjectField(TEXT("exploration_presence"))->GetStringField(ChapterKey));
    View.Requests.Add(TEXT("map:") + Source()->GetStringField(TEXT("return_scene")));
    View.Revision = ++Revision;
    // Source cleanup keeps state FLED, removes the enemy, and requests scene re-entry.
    OnChanged.Broadcast();
    OnReturned.Broadcast();
}
