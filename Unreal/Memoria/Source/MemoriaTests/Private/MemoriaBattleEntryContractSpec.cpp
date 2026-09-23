#include "MemoriaPotionEvidence.h"
#include "Battle/MemoriaBattleEntrySubsystem.h"
#include "Engine/World.h"
#include "TimerManager.h"
#if WITH_DEV_AUTOMATION_TESTS
namespace
{
using namespace MemoriaPotionEvidence;
using Val = TSharedPtr<FJsonValue>;
TArray<Val> Cases(const TCHAR* Name)
{
    FString Text; Val Value;
    FFileHelper::LoadFileToString(Text, *(FPaths::ProjectDir() /
        TEXT("../../docs/unreal-migration/fixtures/battle_entry") / Name));
    FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Value);
    return Value ? Value->AsArray() : TArray<Val>();
}
Obj FindCase(const TArray<Val>& Rows, const FString& Id)
{
    for (const auto& Row : Rows)
        if (Row->AsObject()->GetStringField(TEXT("id")) == Id) return Row->AsObject();
    return nullptr;
}
TArray<Val> Strings(const TArray<FString>& Items)
{
    TArray<Val> Rows; for (const auto& Item : Items) Rows.Add(MakeShared<FJsonValueString>(Item)); return Rows;
}
Obj Player(const FMemoriaRunSnapshot& S)
{
    auto P = MakeShared<FJsonObject>(), Items = MakeShared<FJsonObject>();
    P->SetStringField(TEXT("name"), S.Player.Name);
    P->SetNumberField(TEXT("hp"), S.Player.Hp); P->SetNumberField(TEXT("max_hp"), S.Player.MaxHp);
    P->SetNumberField(TEXT("grains"), S.Player.Grains); P->SetNumberField(TEXT("field_focus"), S.Player.FieldFocus);
    P->SetNumberField(TEXT("directive_streak"), S.Player.DirectiveStreak);
    P->SetBoolField(TEXT("elia_with_party"), S.Player.bEliaWithParty);
    for (const auto& I : S.Player.Items) Items->SetNumberField(I.Id, I.Count);
    P->SetObjectField(TEXT("items"), Items); P->SetArrayField(TEXT("item_quick_slots"), Strings(S.Player.QuickSlots));
    P->SetArrayField(TEXT("recent_items"), Strings(S.Player.RecentItems)); return P;
}
Obj Project(const UMemoriaRunSubsystem& Run, const FMemoriaBattleEntryView& V)
{
    auto O = MakeShared<FJsonObject>(), Flags = MakeShared<FJsonObject>(), Stats = MakeShared<FJsonObject>();
    const auto S = Run.GetRunSnapshot();
    O->SetObjectField(TEXT("player"), Player(S));
    for (const auto& F : S.StoryFlags) Flags->SetBoolField(F.Id, F.bValue);
    O->SetObjectField(TEXT("flags"), Flags); O->SetNumberField(TEXT("total_battles"), S.TotalBattles);
    Stats->SetNumberField(TEXT("total_battles"), S.TotalBattles);
    Stats->SetNumberField(TEXT("highest_momentum_rank"), S.HighestMomentumRank);
    O->SetObjectField(TEXT("play_stats"), Stats);
    O->SetNumberField(TEXT("state"), V.BattleState == TEXT("FLED") ? 5 : V.BattleState == TEXT("PLAYER_TURN") ? 1 : 0);
    O->SetNumberField(TEXT("game_state"), V.bActive ? 2 : 0);
    O->SetNumberField(TEXT("burn_count"), Run.GetPlayerMemory()->GetSnapshot().BurnedHistory.Num());
    if (V.bActive)
    {
        auto E = MakeShared<FJsonObject>();
        E->SetStringField(TEXT("name"), V.EnemyName); E->SetNumberField(TEXT("hp"), V.EnemyHp);
        E->SetNumberField(TEXT("max_hp"), V.EnemyMaxHp); E->SetNumberField(TEXT("attack"), V.EnemyAttack);
        E->SetBoolField(TEXT("is_void"), V.bEnemyVoid); E->SetBoolField(TEXT("is_ambient"), true);
        E->SetBoolField(TEXT("is_boss"), false); E->SetNumberField(TEXT("phase"), 1);
        E->SetBoolField(TEXT("phase_changed"), false);
        E->SetStringField(TEXT("weakness"), V.Weakness); E->SetStringField(TEXT("resistance"), V.Resistance);
        E->SetArrayField(TEXT("abilities"), Strings(V.EnemyAbilities)); O->SetObjectField(TEXT("enemy"), E);
    }
    else O->SetField(TEXT("enemy"), MakeShared<FJsonValueNull>());
    auto M = MakeShared<FJsonObject>();
    if (!V.ModifierId.IsEmpty())
    {
        M->SetStringField(TEXT("id"), V.ModifierId); M->SetStringField(TEXT("name"), V.ModifierName);
        M->SetStringField(TEXT("desc"), V.ModifierDescription); M->SetStringField(TEXT("effect"), V.ModifierEffect);
        M->SetNumberField(TEXT("value"), V.ModifierValue);
    }
    O->SetObjectField(TEXT("modifier"), M);
    TArray<Val> Events;
    FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(V.SourceEventsJson), Events);
    O->SetArrayField(TEXT("events"), Events);
    auto Objective = MakeShared<FJsonObject>();
    for (const auto& E : Events)
        if (E->AsObject()->GetStringField(TEXT("event")) == TEXT("objective"))
            Objective = MakeShared<FJsonObject>(*E->AsObject()->GetObjectField(TEXT("value")));
    Objective->RemoveField(TEXT("progress_current")); Objective->RemoveField(TEXT("progress_target"));
    Objective->RemoveField(TEXT("progress_text")); O->SetObjectField(TEXT("objective"), Objective);
    O->SetArrayField(TEXT("objective_options"), {});
    auto Ui = MakeShared<FJsonObject>(), Progress = MakeShared<FJsonObject>();
    Ui->SetStringField(TEXT("title"), V.ObjectiveTitle); Ui->SetStringField(TEXT("description"), V.ObjectiveDescription);
    Progress->SetNumberField(TEXT("current"), V.ObjectiveProgressCurrent);
    Progress->SetNumberField(TEXT("target"), V.ObjectiveProgressTarget);
    Progress->SetStringField(TEXT("text"), V.ObjectiveProgress); Ui->SetObjectField(TEXT("progress"), Progress);
    O->SetObjectField(TEXT("objective_view"), Ui);
    O->SetArrayField(TEXT("logs"), Strings(V.Logs)); O->SetArrayField(TEXT("external"), Strings(V.Requests));
    auto Art = MakeShared<FJsonObject>(); Art->SetStringField(TEXT("background"), V.BackgroundSource);
    Art->SetStringField(TEXT("enemy"), V.EnemyImageSource); O->SetObjectField(TEXT("art"), Art);
    auto T = MakeShared<FJsonObject>();
    T->SetBoolField(TEXT("player_defending"), V.bPlayerDefending);
    T->SetBoolField(TEXT("field_focus_opening"), V.bFieldFocusOpening);
    T->SetBoolField(TEXT("sable_in_party"), V.bSableInParty); T->SetBoolField(TEXT("tobias_in_party"), V.bTobiasInParty);
    T->SetNumberField(TEXT("enemy_break_gauge"), V.BreakGauge);
    T->SetNumberField(TEXT("momentum"), V.Momentum); T->SetNumberField(TEXT("momentum_rank"), V.MomentumRank);
    T->SetNumberField(TEXT("limit_gauge"), V.LimitGauge); T->SetNumberField(TEXT("difficulty_bonus"), V.DifficultyBonus);
    T->SetNumberField(TEXT("_witness_progress"), V.WitnessProgress); T->SetNumberField(TEXT("_witness_required"), V.WitnessRequired);
    O->SetObjectField(TEXT("transient"), T);
    O->SetBoolField(TEXT("elia_cooldown_reset_observed"), V.bEliaCooldownsReset);
    return O;
}
Obj Expected(const Obj& Gold, const Obj& Shape)
{
    auto E = MakeShared<FJsonObject>();
    for (const auto& Pair : Shape->Values)
    {
        if (Pair.Key == TEXT("elia_cooldown_reset_observed"))
        {
            E->SetBoolField(Pair.Key, Gold->GetObjectField(TEXT("player"))->GetBoolField(TEXT("elia_with_party")));
        }
        else if (Pair.Key == TEXT("play_stats") || Pair.Key == TEXT("transient"))
        {
            auto Sub = MakeShared<FJsonObject>();
            for (const auto& F : Pair.Value->AsObject()->Values)
                Sub->SetField(F.Key, Gold->GetObjectField(Pair.Key)->Values[F.Key]);
            E->SetObjectField(Pair.Key, Sub);
        }
        else E->SetField(Pair.Key, Gold->Values[Pair.Key]);
    }
    return E;
}
struct FCaseWorld
{
    TStrongObjectPtr<UGameInstance> Game{NewObject<UGameInstance>()};
    UWorld* World = nullptr;
    UMemoriaRunSubsystem* Run = nullptr;
    UMemoriaBattleEntrySubsystem* Battle = nullptr;
    int32 Returned = 0;
    FCaseWorld()
    {
        Game->Init(); Run = Game->GetSubsystem<UMemoriaRunSubsystem>();
        Battle = Game->GetSubsystem<UMemoriaBattleEntrySubsystem>();
        World = UWorld::CreateWorld(EWorldType::Game, false); World->SetGameInstance(Game.Get());
        Battle->OnReturned.AddLambda([this] { ++Returned; });
    }
    ~FCaseWorld()
    {
        Battle->OnReturned.Clear(); World->DestroyWorld(false); Game->Shutdown();
    }
};
bool Setup(FAutomationTestBase& Test, FCaseWorld& C, const Obj& Input, const Obj& Before)
{
    if (!Test.TestTrue(TEXT("Source catalog starts native authority"), C.Run->BeginStartingMemoryRun() == EMemoriaMemoryResult::Success)) return false;
    auto S = C.Run->GetRunSnapshot(); const auto P = Before->GetObjectField(TEXT("player"));
    S.CurrentChapter = Input->GetIntegerField(TEXT("chapter")); S.CurrentLocale = Input->GetStringField(TEXT("locale"));
    S.Player.Hp = P->GetIntegerField(TEXT("hp")); S.Player.MaxHp = P->GetIntegerField(TEXT("max_hp"));
    S.Player.Grains = P->GetIntegerField(TEXT("grains")); S.Player.FieldFocus = P->GetIntegerField(TEXT("field_focus"));
    S.Player.DirectiveStreak = P->GetIntegerField(TEXT("directive_streak"));
    S.Player.bEliaWithParty = P->GetBoolField(TEXT("elia_with_party"));
    S.Player.Items.Reset(); S.Player.RecentItems.Reset(); S.Player.QuickSlots.Reset();
    for (const auto& I : P->GetObjectField(TEXT("items"))->Values) S.Player.Items.Add({FString(I.Key.ToView()), static_cast<int64>(I.Value->AsNumber())});
    for (const auto& I : P->GetArrayField(TEXT("recent_items"))) S.Player.RecentItems.Add(I->AsString());
    for (const auto& I : P->GetArrayField(TEXT("item_quick_slots"))) S.Player.QuickSlots.Add(I->AsString());
    S.StoryFlags.Reset();
    for (const auto& F : Before->GetObjectField(TEXT("flags"))->Values) S.StoryFlags.Add({FString(F.Key.ToView()), F.Value->AsBool()});
    S.TotalBattles = static_cast<int64>(Before->GetNumberField(TEXT("total_battles")));
    S.HighestMomentumRank = Before->GetObjectField(TEXT("play_stats"))->GetIntegerField(TEXT("highest_momentum_rank"));
    auto Memory = C.Run->GetPlayerMemory()->GetSnapshot(); auto Definitions = C.Run->GetPlayerMemory()->GetDefinitions();
    // Oracle labels these histories synthetic. Native Restore requires matching,
    // unique burned owned definitions; fixture-only IDs preserve that invariant.
    for (int32 N = 0; N < Input->GetIntegerField(TEXT("burn_count")); ++N)
    {
        FMemoriaMemoryDefinition D; D.Id = FString::Printf(TEXT("test_battle_history_%d"), N);
        Definitions.Add(D); FMemoriaMemoryState M; M.Id = D.Id; M.bBurned = true;
        Memory.Owned.Add(M); Memory.BurnedHistory.Add(D.Id);
    }
    return Test.TestTrue(TEXT("Explicit source state restored without executing extra burns"),
        C.Run->RestoreRun(S, Definitions, Memory, C.Run->GetWorldCognition()->GetSnapshot()) == EMemoriaMemoryResult::Success);
}
void Evidence(const FString& Name, const Obj& O)
{
    const FString Dir = FPaths::ProjectSavedDir() / TEXT("Validation/Battle1");
    IFileManager::Get().MakeDirectory(*Dir, true);
    // RNG tapes contain fractions; the catalog canonicalizer intentionally accepts integers only.
    FString Json; FJsonSerializer::Serialize(O.ToSharedRef(), TJsonWriterFactory<>::Create(&Json));
    FFileHelper::SaveStringToFile(Json + TEXT("\n"), *(Dir / Name), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}
class FFleeCheck final : public IAutomationLatentCommand
{
    FAutomationTestBase* Test;
    TSharedPtr<FCaseWorld> Case;
    Obj Gold, Report;
    FString Id, MemoryBefore, WorldBefore;
    uint64 LastFrame = MAX_uint64;
    double Elapsed = 0.;
public:
    FFleeCheck(FAutomationTestBase* InTest, TSharedPtr<FCaseWorld> InCase, Obj InGold,
        Obj InReport, FString InId, FString InMemory, FString InWorld)
        : Test(InTest), Case(MoveTemp(InCase)), Gold(MoveTemp(InGold)), Report(MoveTemp(InReport)),
          Id(MoveTemp(InId)), MemoryBefore(MoveTemp(InMemory)), WorldBefore(MoveTemp(InWorld)) {}
    bool Update() override
    {
        if (LastFrame == GFrameCounter) return false;
        LastFrame = GFrameCounter; Case->World->GetTimerManager().Tick(.05f); Elapsed += .05;
        if (Elapsed < .2) Test->TestEqual(TEXT("Flee retains owner during source delay"), Case->Returned, 0);
        if (Elapsed < .5) return false;
        Test->TestEqual(TEXT("One source return request after .3 second timer"), Case->Returned, 1);
        const auto Actual = Project(*Case->Run, Case->Battle->GetView());
        const auto ExpectedAfter = Expected(Gold->GetObjectField(TEXT("after_flee")), Actual);
        Test->TestEqual(TEXT("Source exact projected state after ambient Flee"), Canon(Actual), Canon(ExpectedAfter));
        Test->TestFalse(TEXT("Cleanup releases battle modal"), Case->Battle->IsActive());
        Test->TestFalse(TEXT("Old revision cannot flee again"), Case->Battle->Flee(Case->Battle->GetRevision()));
        Test->TestEqual(TEXT("Flee never burns or mutates memory"), Canon(FJsonObjectConverter::UStructToJsonObject(Case->Run->GetPlayerMemory()->GetSnapshot())), MemoryBefore);
        Test->TestEqual(TEXT("Flee never mutates world cognition"), Case->Run->GetWorldCognition()->ExportJson(), WorldBefore);
        Report->SetObjectField(TEXT("after_flee"), Actual); Evidence(TEXT("source_") + Id + TEXT(".json"), Report);
        return true;
    }
};
}
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FBattleEntrySource, "Memoria.BattleEntry.Source",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
void FBattleEntrySource::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{
    for (const auto& Row : Cases(TEXT("contract_inputs.v1.json")))
    {
        const Obj Input = Row->AsObject();
        // These three source-only field-flow approaches are outside neutral ambient entry.
        if (Input->GetStringField(TEXT("entry_mode")) != TEXT("neutral")) continue;
        const FString Id = Input->GetStringField(TEXT("id")); Names.Add(Id); Commands.Add(Id);
    }
}
bool FBattleEntrySource::RunTest(const FString& Id)
{
    const Obj Input = FindCase(Cases(TEXT("contract_inputs.v1.json")), Id);
    const Obj Gold = FindCase(Cases(TEXT("contract_expected.v1.json")), Id);
    if (!TestTrue(TEXT("Executed source fixtures present"), Input.IsValid() && Gold.IsValid())) return false;
    auto C = MakeShared<FCaseWorld>();
    if (!Setup(*this, *C, Input, Gold->GetObjectField(TEXT("before")))) return false;
    const FString MemoryBefore = Canon(FJsonObjectConverter::UStructToJsonObject(C->Run->GetPlayerMemory()->GetSnapshot()));
    const FString WorldBefore = C->Run->GetWorldCognition()->ExportJson();
    const auto Tape = Input->GetArrayField(TEXT("rng")); TArray<Val> Trace; int32 Cursor = 0;
    const auto Draw = [&](const TCHAR* Kind, double Min, double Max) {
        if (!TestTrue(TEXT("No unrecorded random draw"), Tape.IsValidIndex(Cursor))) return Min;
        const Obj Row = Tape[Cursor++]->AsObject(); TestEqual(TEXT("Random draw kind"), Row->GetStringField(TEXT("kind")), FString(Kind));
        const double Value = Row->GetNumberField(TEXT("value"));
        auto E = MakeShared<FJsonObject>(); E->SetStringField(TEXT("kind"), Kind);
        E->SetNumberField(TEXT("min"), Min); E->SetNumberField(TEXT("max"), Max); E->SetNumberField(TEXT("value"), Value);
        Trace.Add(MakeShared<FJsonValueObject>(E)); return Value;
    };
    FMemoriaEncounterRng Rng{
        [&](double Min, double Max) { return Draw(TEXT("float"), Min, Max); },
        [&](int32 Min, int32 Max) { return static_cast<int32>(Draw(TEXT("int"), Min, Max)); }
    };
    if (!TestTrue(TEXT("Begin exact ambient enemy"), C->Battle->BeginEncounter(Input->GetIntegerField(TEXT("enemy_index")), Rng, C->World))) return false;
    TestEqual(TEXT("Consumes exactly source random tape"), Cursor, Tape.Num());
    const auto GoldTrace = Gold->GetArrayField(TEXT("rng"));
    TestEqual(TEXT("Exact source RNG trace length"), Trace.Num(), GoldTrace.Num());
    for (int32 N = 0; N < FMath::Min(Trace.Num(), GoldTrace.Num()); ++N)
    {
        const Obj A = Trace[N]->AsObject(), E = GoldTrace[N]->AsObject();
        TestEqual(TEXT("RNG trace kind"), A->GetStringField(TEXT("kind")), E->GetStringField(TEXT("kind")));
        for (const TCHAR* Key : {TEXT("min"), TEXT("max"), TEXT("value")})
            TestEqual(FString(TEXT("Exact RNG ")) + Key, A->GetNumberField(Key), E->GetNumberField(Key));
    }
    const auto V = C->Battle->GetView(); const Obj Actual = Project(*C->Run, V);
    TestEqual(TEXT("Source exact first turn projection"), Canon(Actual), Canon(Expected(Gold->GetObjectField(TEXT("initial")), Actual)));
    TestFalse(TEXT("Duplicate entry cannot increment statistics or heal"), C->Battle->BeginEncounter(V.EnemyIndex, Rng, C->World));
    TestEqual(TEXT("Read and duplicate begin preserve entire authority"), Canon(Actual), Canon(Project(*C->Run, C->Battle->GetView())));
    TestEqual(TEXT("Entry does not burn memory"), Canon(FJsonObjectConverter::UStructToJsonObject(C->Run->GetPlayerMemory()->GetSnapshot())), MemoryBefore);
    TestEqual(TEXT("Entry preserves world cognition"), C->Run->GetWorldCognition()->ExportJson(), WorldBefore);
    TestFalse(TEXT("Stale revision cannot flee"), C->Battle->Flee(V.Revision + 1));
    TestTrue(TEXT("Ambient Flee always succeeds including Void conversion"), C->Battle->Flee(V.Revision));
    TestFalse(TEXT("Duplicate Flee is ignored"), C->Battle->Flee(C->Battle->GetRevision()));
    TestTrue(TEXT("Flee waits with modal retained"), C->Battle->IsReturning());
    TestEqual(TEXT("No early scene return"), C->Returned, 0);
    auto Report = MakeShared<FJsonObject>(); Report->SetObjectField(TEXT("initial"), Actual);
    Report->SetArrayField(TEXT("rng"), Trace);
    ADD_LATENT_AUTOMATION_COMMAND(FFleeCheck(this, C, Gold, Report, Id, MemoryBefore, WorldBefore));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBattleEncounterDistance, "Memoria.BattleEntry.EncounterDistance",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBattleEncounterDistance::RunTest(const FString&)
{
    FMemoriaEncounterModel Model; Model.Reset(60.);
    TArray<FString> Calls;
    FMemoriaEncounterRng Rng{
        [&](double Min, double Max) { TestEqual(TEXT("Reset threshold min"), Min, 60.); TestEqual(TEXT("Reset threshold max"), Max, 100.); Calls.Add(TEXT("threshold")); return 100.; },
        [&](int32 Min, int32 Max) { TestEqual(TEXT("Enemy pool min"), Min, 0); TestEqual(TEXT("Enemy pool max"), Max, 1); Calls.Add(TEXT("enemy")); return 1; }
    };
    TestFalse(TEXT("First sample establishes position"), Model.Advance({128.,288.}, true, Rng).bTriggered);
    Model.Advance({128. + 32.*40.,288.}, true, Rng);
    TestEqual(TEXT("Distance counts tiles independent of time"), Model.StepCount, 40.);
    const auto Warning = Model.Advance({128. + 32.*43.2,288.}, true, Rng);
    TestTrue(TEXT("Exactly 72 percent emits warning"), Warning.bWarningStarted);
    TestTrue(TEXT("Threat pressure follows source normalized ramp"), FMath::IsNearlyEqual(Warning.Pressure, (.72-.42)/.58, 1.e-10));
    TestFalse(TEXT("Stationary sample never retriggers warning"), Model.Advance(Model.LastPosition, true, Rng).bWarningStarted);
    const double BeforePause = Model.StepCount;
    Model.Advance({7000.,288.}, false, Rng);
    TestEqual(TEXT("Modal rebase does not add movement"), Model.StepCount, BeforePause);
    Model.Advance({7000.+32.,288.}, true, Rng);
    TestTrue(TEXT("Resume counts from rebased position"), FMath::IsNearlyEqual(Model.StepCount, BeforePause+1., 1.e-10));
    const auto Trigger = Model.Advance({7000.+32.*17.8,288.}, true, Rng);
    TestTrue(TEXT("Threshold crossing triggers once"), Trigger.bTriggered);
    TestEqual(TEXT("Selected second source enemy"), Trigger.EnemyIndex, 1);
    TestEqual(TEXT("Source consumes new threshold before enemy draw"), FString::Join(Calls,TEXT(",")), FString(TEXT("threshold,enemy")));
    TestEqual(TEXT("Overshoot discarded"), Model.StepCount, 0.); TestEqual(TEXT("Injected new threshold"), Model.Threshold, 100.);
    TestEqual(TEXT("Trigger clears pressure"), Trigger.Pressure, 0.);
    Model.Reset(60.); Model.Advance({128.,288.}, true, Rng);
    Model.Advance({128.+32.*44.,288.}, true, Rng);
    const auto Broken = Model.Advance({128.+32.*48.,288.}, true, Rng, true);
    TestTrue(TEXT("Source dash can break an already warned trail"), Broken.bTrailBroken);
    TestTrue(TEXT("Source dash subtracts 4.4 times moved distance after accumulation"), FMath::IsNearlyEqual(Model.StepCount, 30.4, 1.e-10));
    Model.bEnabled=false; const FVector2D Last = Model.LastPosition; Model.Advance({0.,0.},false,Rng);
    TestTrue(TEXT("Disabled model does not rebase"), Model.LastPosition == Last);
    return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBattleEntryOwner, "Memoria.BattleEntry.OwnerLifetime",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBattleEntryOwner::RunTest(const FString&)
{
    auto C = MakeShared<FCaseWorld>(); auto Rng = FMemoriaEncounterRng::Random();
    TestFalse(TEXT("Inactive run cannot begin"), C->Battle->BeginEncounter(0,Rng,C->World));
    C->Run->BeginStartingMemoryRun();
    TestFalse(TEXT("Invalid enemy rejected"), C->Battle->BeginEncounter(2,Rng,C->World));
    TestTrue(TEXT("Valid owner begins"), C->Battle->BeginEncounter(0,Rng,C->World));
    const auto Begun=Full(*C->Run); const auto OldRevision=C->Battle->GetRevision();
    TestTrue(TEXT("Flee schedules return"),C->Battle->Flee(OldRevision));
    TStrongObjectPtr<UMemoriaRunSaveGame> Save(C->Run->CaptureSave());
    TestTrue(TEXT("Same-ID restore succeeds"), C->Run->RestoreSave(*Save));
    TestFalse(TEXT("Same-ID replacement cancels pending timer and modal"), C->Battle->IsActive());
    TestEqual(TEXT("Replacement does not emit old return"), C->Returned, 0);
    TestFalse(TEXT("Old revision cannot affect replacement"), C->Battle->Flee(OldRevision));
    TestEqual(TEXT("Cancellation preserves restored authoritative data"),Canon(Full(*C->Run)),Canon(Begun));
    TestTrue(TEXT("Independent new entry begins"),C->Battle->BeginEncounter(1,Rng,C->World));
    const auto Second=Full(*C->Run);
    FWorldDelegates::OnWorldCleanup.Broadcast(C->World,false,true);
    TestFalse(TEXT("Owner world cleanup cancels battle"),C->Battle->IsActive());
    TestEqual(TEXT("World cleanup never rolls back or saves entry effects"),Canon(Full(*C->Run)),Canon(Second));
    TestEqual(TEXT("Cleanup never returns to stale map"),C->Returned,0);
    TestEqual(TEXT("Cleanup clears source requests"),C->Battle->GetView().Requests.Num(),0);
    // Injected RNG is public and may synchronously replace even the same RunId.
    auto State=C->Run->GetRunSnapshot();
    auto Memory=C->Run->GetPlayerMemory()->GetSnapshot();
    auto Definitions=C->Run->GetPlayerMemory()->GetDefinitions();
    for(int32 N=0;N<3;++N)
    {
        FMemoriaMemoryDefinition D;D.Id=FString::Printf(TEXT("test_rng_owner_%d"),N);
        Definitions.Add(D);FMemoriaMemoryState M;M.Id=D.Id;M.bBurned=true;
        Memory.Owned.Add(M);Memory.BurnedHistory.Add(D.Id);
    }
    TestTrue(TEXT("Synthetic modifier band setup"),C->Run->RestoreRun(State,Definitions,Memory,C->Run->GetWorldCognition()->GetSnapshot())==EMemoriaMemoryResult::Success);
    TStrongObjectPtr<UMemoriaRunSaveGame> Replacement(C->Run->CaptureSave());
    Replacement->Run.Player.Hp=37;
    int32 Draws=0;
    FMemoriaEncounterRng Replacing{
        [&](double,double){++Draws;C->Run->RestoreSave(*Replacement);return .99;},
        [](int32 Min,int32){return Min;}
    };
    TestFalse(TEXT("Run replaced during injected RNG cannot commit stale entry"),C->Battle->BeginEncounter(0,Replacing,C->World));
    TestEqual(TEXT("Injected replacement branch was reached"),Draws,1);
    TestEqual(TEXT("Replacement HP survives rejected entry"),C->Run->GetRunSnapshot().Player.Hp,37ll);
    TestEqual(TEXT("Replacement statistics survive rejected entry"),C->Run->GetRunSnapshot().TotalBattles,Replacement->Run.TotalBattles);
    TestFalse(TEXT("Replacement leaves no stale battle modal"),C->Battle->IsActive());
    return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBattleEntrySavedStats, "Memoria.BattleEntry.SavedStats",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBattleEntrySavedStats::RunTest(const FString&)
{
    auto C=MakeShared<FCaseWorld>(); C->Run->BeginStartingMemoryRun(); auto S=C->Run->GetRunSnapshot();
    S.TotalBattles=2147483647ll; S.HighestMomentumRank=3; S.CurrentChapter=3; S.Player.FieldFocus=1;
    TestTrue(TEXT("State with persisted source stats restores"),C->Run->RestoreRun(S,C->Run->GetPlayerMemory()->GetDefinitions(),C->Run->GetPlayerMemory()->GetSnapshot(),C->Run->GetWorldCognition()->GetSnapshot())==EMemoriaMemoryResult::Success);
    auto Rng=FMemoriaEncounterRng::Random();TestTrue(TEXT("Entry beyond int32 total"),C->Battle->BeginEncounter(0,Rng,C->World));
    TestEqual(TEXT("Total battle counter is int64"),C->Run->GetRunSnapshot().TotalBattles,2147483648ll);
    TestEqual(TEXT("Opening focus never lowers peak"),C->Run->GetRunSnapshot().HighestMomentumRank,3ll);
    SaveRoundTrip(*this,*C->Run,TEXT("battle_entry_saved_stats.json"));
    auto OldJson=FJsonObjectConverter::UStructToJsonObject(S);
    OldJson->RemoveField(TEXT("totalBattles"));OldJson->RemoveField(TEXT("highestMomentumRank"));
    FMemoriaRunSnapshot Old;
    TestTrue(TEXT("Older DTO without additive stats parses"),FJsonObjectConverter::JsonObjectToUStruct(OldJson.ToSharedRef(),&Old));
    TestEqual(TEXT("Missing total defaults zero"),Old.TotalBattles,0ll);
    TestEqual(TEXT("Missing peak defaults zero"),Old.HighestMomentumRank,0ll);
    TStrongObjectPtr<UMemoriaRunSaveGame> Captured(C->Run->CaptureSave());
    TStrongObjectPtr<UGameInstance> RestoredGame(NewObject<UGameInstance>()); RestoredGame->Init();
    auto* RestoredRun=RestoredGame->GetSubsystem<UMemoriaRunSubsystem>();
    auto* RestoredBattle=RestoredGame->GetSubsystem<UMemoriaBattleEntrySubsystem>();
    TestTrue(TEXT("Independent game restores captured run"),RestoredRun->RestoreSave(*Captured));
    TestEqual(TEXT("Independent restore retains persistent battle count"),RestoredRun->GetRunSnapshot().TotalBattles,C->Run->GetRunSnapshot().TotalBattles);
    TestFalse(TEXT("Independent save restore has no active battle transient"),RestoredBattle->IsActive());
    TestFalse(TEXT("Independent save restore has no pending return"),RestoredBattle->IsReturning());
    TestEqual(TEXT("Independent save restore has no battle requests"),RestoredBattle->GetView().Requests.Num(),0);
    TestTrue(TEXT("Reading and restoring elsewhere does not cancel source battle"),C->Battle->IsActive());
    RestoredGame->Shutdown();
    return !HasAnyErrors();
}
#endif
