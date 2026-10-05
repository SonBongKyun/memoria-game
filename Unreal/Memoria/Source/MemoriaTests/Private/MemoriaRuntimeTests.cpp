#include "Misc/AutomationTest.h"
#include "Domain/MemoriaPlayerMemoryDomain.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Save/MemoriaRunSaveGame.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
FMemoriaMemoryDefinition TestDefinition(const TCHAR* Id)
{
    FMemoriaMemoryDefinition D;
    D.Id = Id; D.Title = TEXT("Validation memory"); D.Description = TEXT("Controlled test definition");
    D.RawGrade = EMemoriaMemoryGrade::Grade3; D.BurnPower = 91;
    D.StoryEffect = TEXT("test_effect"); D.RelatedNpc = TEXT("test_npc");
    D.SourcePath = TEXT("fixture://ue-runtime"); D.SourceHash = TEXT("test-only-hash"); D.TextId = TEXT("validation.title");
    return D;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMemoriaRuntimeLifetimeTest, "Memoria.Foundation.RuntimeLifetime",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMemoriaRuntimeLifetimeTest::RunTest(const FString& Parameters)
{
    TStrongObjectPtr<UGameInstance> Game(NewObject<UGameInstance>());
    Game->Init();
    auto* Run = Game->GetSubsystem<UMemoriaRunSubsystem>();
    if (!TestNotNull(TEXT("Subsystem created by GameInstance"), Run)) { Game->Shutdown(); return false; }
    TWeakObjectPtr<UMemoriaRunSubsystem> WeakRun(Run);
    TWeakObjectPtr<UMemoriaPlayerMemoryDomain> WeakDomain(Run->GetPlayerMemory());
    TStrongObjectPtr<UMemoriaMemoryCatalog> Catalog(NewObject<UMemoriaMemoryCatalog>());
    Catalog->ContentRevision = TEXT("runtime-validation-v1");
    Catalog->Definitions = {TestDefinition(TEXT("MemoryA")), TestDefinition(TEXT("memorya"))};
    int32 Replacements = 0;
    Run->OnRunReplaced.AddLambda([&] { ++Replacements; });
    TestTrue(TEXT("Case-sensitive ordered catalog initialization"), Run->BeginRun(*Catalog, {TEXT("memorya"), TEXT("MemoryA")}) == EMemoriaMemoryResult::Success);
    const FGuid FirstId = Run->GetRunSnapshot().RunId;
    CollectGarbage(RF_NoFlags, true);
    if (!TestTrue(TEXT("GameInstance keeps subsystem and domain alive across full GC"), WeakRun.IsValid() && WeakDomain.IsValid())) { Game->Shutdown(); return false; }
    TestTrue(TEXT("Domain has run outer"), WeakDomain->GetOuter() == Run);
    TestEqual(TEXT("Requested owned order preserved"), WeakDomain->GetSnapshot().Owned[0].Id, FString(TEXT("memorya")));
    Run->SetStoryFlag(TEXT("CaseFlag"), true); Run->SetStoryFlag(TEXT("caseflag"), false);
    auto Snapshot = Run->GetRunSnapshot();
    Snapshot.CurrentChapter = 6; Snapshot.Player.Hp = 37; Snapshot.Player.bEliaWithParty = false;
    TestTrue(TEXT("Nondefault snapshot reconstruction"), Run->RestoreRun(Snapshot, WeakDomain->GetDefinitions(), WeakDomain->GetSnapshot()) == EMemoriaMemoryResult::Success);
    TestEqual(TEXT("Context uses restored chapter"), Run->GetMemoryContext().CurrentChapter, int64(6));
    TestFalse(TEXT("Context uses restored party state"), Run->GetMemoryContext().bEliaWithParty);
    TestTrue(TEXT("Flag spelling retained"), Run->GetRunSnapshot().GetFlag(TEXT("CaseFlag")));
    TestTrue(TEXT("Stored lower-case false present"), Run->GetRunSnapshot().HasFlag(TEXT("caseflag")));
    TestFalse(TEXT("Lower-case false independent"), Run->GetRunSnapshot().GetFlag(TEXT("caseflag")));
    auto Invalid = Snapshot; const auto Duplicate = Invalid.StoryFlags[0]; Invalid.StoryFlags.Add(Duplicate);
    TestTrue(TEXT("Exact duplicate flag rejected without replacing run"), Run->RestoreRun(Invalid, WeakDomain->GetDefinitions(), WeakDomain->GetSnapshot()) == EMemoriaMemoryResult::InvalidSnapshot);
    TArray<EMemoriaMemoryEventKind> Events;
    WeakDomain->OnObserved.AddLambda([&](const FMemoriaMemoryEvent& E)
    {
        Events.Add(E.Kind);
        if (E.Kind == EMemoriaMemoryEventKind::Cascaded)
        {
            TestEqual(TEXT("Cascade observes appended history"), WeakDomain->GetSnapshot().BurnedHistory.Num(), 1);
            TestFalse(TEXT("Restored absent Elia produces no new residue"), WeakDomain->HasResidue(TEXT("MemoryA")));
            TestFalse(TEXT("Reentrant run flag mutation rejected"), Run->SetStoryFlag(TEXT("illegal_reentry"), true));
        }
    });
    TestTrue(TEXT("Burn inside retained lifecycle"), Run->BurnMemory(TEXT("MemoryA")) == EMemoriaMemoryResult::Success);
    TestTrue(TEXT("Ordered connected burn without Elia (Godot burn_memory -> chain -> burned -> carry)"), Events == TArray<EMemoriaMemoryEventKind>{EMemoriaMemoryEventKind::Cascaded, EMemoriaMemoryEventKind::Burned, EMemoriaMemoryEventKind::CarryChanged});
    TestEqual(TEXT("Restore emits run replacement only"), Replacements, 2);
    WeakDomain->OnObserved.Clear();
    const int32 ReplacementsBeforeHealing = Replacements;
    Run->RestoreHp(25);
    TestEqual(TEXT("Rest heals a wounded active run by the requested amount"), Run->GetRunSnapshot().Player.Hp, int64(62));
    Run->RestoreHp(0);
    TestEqual(TEXT("Zero rest amount keeps wounded HP"), Run->GetRunSnapshot().Player.Hp, int64(62));
    Run->RestoreHp(-25);
    TestEqual(TEXT("Negative rest amount does not damage wounded HP"), Run->GetRunSnapshot().Player.Hp, int64(62));
    Run->RestoreHp(MIN_int64);
    TestEqual(TEXT("Minimum signed rest amount does not damage wounded HP"), Run->GetRunSnapshot().Player.Hp, int64(62));
    TestEqual(TEXT("Healing does not replace the run"), Replacements, ReplacementsBeforeHealing);
    const TCHAR* RestPropFlags[] = {TEXT("prop_barrel_160_128"), TEXT("prop_crate_352_288"), TEXT("prop_sign_256_64"), TEXT("prop_campfire_416_352")};
    for (const TCHAR* Flag : RestPropFlags) Run->SetStoryFlag(Flag, true);
    TStrongObjectPtr<UMemoriaRunSaveGame> HealedSave(Run->CaptureSave());
    if (TestNotNull(TEXT("Capture healed run"), HealedSave.Get()))
    {
        TestEqual(TEXT("Capture preserves healed HP"), HealedSave->Run.Player.Hp, int64(62));
        TestEqual(TEXT("Capture preserves the HP cap"), HealedSave->Run.Player.MaxHp, int64(100));
        Run->RestoreHp(35);
        TestEqual(TEXT("Rest can leave HP just below its cap"), Run->GetRunSnapshot().Player.Hp, int64(97));
        Run->RestoreHp(25);
        TestEqual(TEXT("Near-cap rest heals only to maximum HP"), Run->GetRunSnapshot().Player.Hp, int64(100));
        for (const TCHAR* Flag : RestPropFlags)
        {
            TestTrue(*FString::Printf(TEXT("Capture preserves spent prop flag: %s"), Flag), HealedSave->Run.GetFlag(Flag));
            Run->RemoveStoryFlag(Flag);
            TestFalse(*FString::Printf(TEXT("Live run spent prop flag cleared before restore: %s"), Flag), Run->GetRunSnapshot().HasFlag(Flag));
        }
        TestTrue(TEXT("Restore the captured healed run"), Run->RestoreSave(*HealedSave));
        for (const TCHAR* Flag : RestPropFlags)
            TestTrue(*FString::Printf(TEXT("Restored save retains spent prop flag: %s"), Flag), Run->GetRunSnapshot().GetFlag(Flag));
        TestEqual(TEXT("Restored save retains healed HP"), Run->GetRunSnapshot().Player.Hp, int64(62));
        TestEqual(TEXT("Restored save retains the HP cap"), Run->GetRunSnapshot().Player.MaxHp, int64(100));
    }
    // RestoreRun accepts signed HP/max HP; healing must retain the same mathematical cap.
    const struct { const TCHAR* Label; int64 Hp; int64 MaxHp; int64 Amount; int64 Expected; } HealingCases[] =
    {
        {TEXT("Maximum restored HP heals without overflow"), MAX_int64, MAX_int64, 25, MAX_int64},
        {TEXT("Near-maximum restored HP saturates at its cap"), MAX_int64 - 2, MAX_int64, 25, MAX_int64},
        {TEXT("Maximum heal amount is capped without overflow"), 37, 100, MAX_int64, 100},
        {TEXT("Minimum restored HP plus maximum heal remains negative"), MIN_int64, MAX_int64, MAX_int64, -1},
        {TEXT("Minimum restored cap is preserved"), MIN_int64, MIN_int64, MAX_int64, MIN_int64},
        {TEXT("Negative restored cap bounds a positive heal"), -25, -10, 25, -10},
        {TEXT("Zero amount still applies the existing maximum HP cap"), MAX_int64, 100, 0, 100},
    };
    for (const auto& Case : HealingCases)
    {
        auto HealingSnapshot = Run->GetRunSnapshot();
        HealingSnapshot.Player.Hp = Case.Hp; HealingSnapshot.Player.MaxHp = Case.MaxHp;
        if (TestTrue(*FString::Printf(TEXT("Restore signed HP fixture: %s"), Case.Label),
            Run->RestoreRun(HealingSnapshot, WeakDomain->GetDefinitions(), WeakDomain->GetSnapshot()) == EMemoriaMemoryResult::Success))
        {
            Run->RestoreHp(Case.Amount);
            TestEqual(Case.Label, Run->GetRunSnapshot().Player.Hp, Case.Expected);
        }
    }
    TestTrue(TEXT("Reset run explicitly"), Run->BeginRun(*Catalog, {}) == EMemoriaMemoryResult::Success);
    TestTrue(TEXT("New run ID"), FirstId != Run->GetRunSnapshot().RunId);
    TestFalse(TEXT("Old flags cleared"), Run->GetRunSnapshot().HasFlag(TEXT("CaseFlag")));
    Game->Shutdown();
    TestFalse(TEXT("Run delegate released on shutdown"), Run->OnRunReplaced.IsBound());
    TestNull(TEXT("Domain reference cleared on shutdown"), Run->GetPlayerMemory());
    Game.Reset(); Catalog.Reset();
    CollectGarbage(RF_NoFlags, true);
    TestFalse(TEXT("Domain released after GameInstance lifetime"), WeakDomain.IsValid());
    TestFalse(TEXT("Subsystem released after GameInstance lifetime"), WeakRun.IsValid());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMemoriaNondefaultSaveTest, "Memoria.Foundation.NondefaultSaveRoundTrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMemoriaNondefaultSaveTest::RunTest(const FString& Parameters)
{
    TStrongObjectPtr<UMemoriaRunSaveGame> Save(NewObject<UMemoriaRunSaveGame>());
    Save->SavedAtUtc = FDateTime(2026, 9, 6, 12, 34, 56);
    Save->ContentRevision = TEXT("save-validation-v1");
    Save->MemoryCatalogId = FPrimaryAssetId(TEXT("MemoriaMemoryCatalog"), TEXT("ValidationOnly"));
    auto& R = Save->Run;
    R.RunId = FGuid::NewGuid(); R.ContentRevision = Save->ContentRevision; R.CurrentChapter = 6; R.CurrentLocale = TEXT("ko");
    R.Player.Name = TEXT("Validation Arrel"); R.Player.Hp = 37; R.Player.MaxHp = 173; R.Player.Grains = 409;
    R.Player.bEliaWithParty = false; R.Player.FieldFocus = 8; R.Player.DirectiveStreak = 3;
    FMemoriaItemCount Item; Item.Id = TEXT("witness_ink"); Item.Count = 7; R.Player.Items.Add(Item);
    Item.Id = TEXT("Witness_ink"); Item.Count = 2; R.Player.Items.Add(Item);
    R.Player.QuickSlots = {TEXT("witness_ink"), TEXT(""), TEXT("potion")}; R.Player.RecentItems = {TEXT("antidote"), TEXT("witness_ink")};
    FMemoriaStoryFlag Flag; Flag.Id = TEXT("CaseFlag"); Flag.bValue = true; R.StoryFlags.Add(Flag);
    Flag.Id = TEXT("caseflag"); Flag.bValue = false; R.StoryFlags.Add(Flag);
    Save->MemoryDefinitions = {TestDefinition(TEXT("MemoryB")), TestDefinition(TEXT("MemoryA"))};
    auto& M = Save->PlayerMemory;
    FMemoriaMemoryState State; State.Id = TEXT("MemoryB"); State.bFaded = true; State.Erosion = 47; M.Owned.Add(State);
    State.Id = TEXT("MemoryA"); State.bBurned = true; State.bResidue = true; State.bFaded = false; State.Erosion = 26; M.Owned.Add(State);
    M.BurnedHistory = {TEXT("MemoryA")}; M.AnchorVigil = 12; M.VigilChapters = {2, 4, 6};
    FMemoriaMemoryFlag Passive; Passive.Id = TEXT("ember_sight"); Passive.bValue = false; M.BurnPassives.Add(Passive);
    Passive.Id = TEXT("validation_anchor"); Passive.bValue = true; M.AnchorPassives.Add(Passive);
    M.ErosionGuarded = {TEXT("MemoryB")}; M.GuardSlotsUsed = 1; M.Extracted = {TEXT("historic_extraction")};
    M.ActiveLoan.bActive = true; M.ActiveLoan.MemoryId = TEXT("MemoryB"); M.ActiveLoan.Principal = 19; M.ActiveLoan.Repay = 27; M.ActiveLoan.DueChapter = 8;
    Save->FieldReturn.SourceScenePath = TEXT("res://scenes/maps/validation.tscn"); Save->FieldReturn.MapId = TEXT("validation_only");
    Save->FieldReturn.SourcePixelPosition = FVector2D(137.25, -83.5);
    Save->SceneFlow.bActive = true; Save->SceneFlow.Current.SequenceId = TEXT("validation_current"); Save->SceneFlow.Current.OriginalIndex = 17;
    Save->SceneFlow.Pending.SequenceId = TEXT("validation_pending"); Save->SceneFlow.Pending.OriginalIndex = 4;
    Save->SceneFlow.ResumeQueue = {Save->SceneFlow.Pending, Save->SceneFlow.Current}; Save->SceneFlow.LedgerBurnSnapshot = 9;
    Save->WorldCognition.SchemaVersion = 3; Save->WorldCognition.SourceJson = TEXT("{\"actors\":{\"validation\":{\"seen\":true}}}");
    Save->Diary.SchemaVersion = 2; Save->Diary.SourceJson = TEXT("{\"entries\":[\"validation_diary\"]}");
    Save->Hints.SchemaVersion = 4; Save->Hints.SourceJson = TEXT("{\"shown\":[\"validation_hint\"]}");
    for (int32 Slot = 0; Slot <= 3; ++Slot)
    {
        Save->Slot = Slot;
        TArray<uint8> Bytes;
        if (!TestTrue(TEXT("Engine SaveGameToMemory"), UGameplayStatics::SaveGameToMemory(Save.Get(), Bytes))) { return false; }
        TStrongObjectPtr<UMemoriaRunSaveGame> Copy(Cast<UMemoriaRunSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes)));
        if (!TestNotNull(TEXT("Engine LoadGameFromMemory"), Copy.Get())) { return false; }
        FString Error;
        TestTrue(TEXT("Schema 1 header"), Copy->ValidateHeader(Error));
        TestEqual(TEXT("Autosave 0 / manual 1-3 preserved"), Copy->Slot, Slot);
        TestTrue(TEXT("Serialized upper-case true remains independent"), Copy->Run.GetFlag(TEXT("CaseFlag")));
        TestTrue(TEXT("Serialized lower-case false remains present"), Copy->Run.HasFlag(TEXT("caseflag")));
        TestFalse(TEXT("Serialized lower-case false value"), Copy->Run.GetFlag(TEXT("caseflag")));
        // Reflection compares every SaveGame field, including every nested nondefault section.
        for (TFieldIterator<FProperty> It(UMemoriaRunSaveGame::StaticClass()); It; ++It)
        {
            if (It->HasAnyPropertyFlags(CPF_SaveGame))
            { TestTrue(*FString::Printf(TEXT("Serialized field: %s (slot %d)"), *It->GetName(), Slot), It->Identical_InContainer(Save.Get(), Copy.Get())); }
        }
        TStrongObjectPtr<UMemoriaPlayerMemoryDomain> Domain(NewObject<UMemoriaPlayerMemoryDomain>());
        TestTrue(TEXT("Reconstruct mutable domain from serialized data"), Domain->Restore(Copy->MemoryDefinitions, Copy->PlayerMemory) == EMemoriaMemoryResult::Success);
        TestEqual(TEXT("Owned order survives reconstruction"), Domain->GetSnapshot().Owned[0].Id, FString(TEXT("MemoryB")));
        TestTrue(TEXT("Residue survives reconstruction"), Domain->HasResidue(TEXT("MemoryA")));
        TestEqual(TEXT("Erosion survives reconstruction"), Domain->GetSnapshot().Owned[0].Erosion, int64(47));
    }
    Save->Slot = -1; FString Error; TestFalse(TEXT("Negative slot rejected"), Save->ValidateHeader(Error));
    Save->Slot = 4; TestFalse(TEXT("Slot four rejected"), Save->ValidateHeader(Error));
    return true;
}
#endif
