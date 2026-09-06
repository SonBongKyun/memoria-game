#include "Misc/AutomationTest.h"
#include "Domain/MemoriaPlayerMemoryDomain.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Framework/MemoriaCoordinates.h"
#include "Save/MemoriaRunSaveGame.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "Engine/GameInstance.h"
#include <type_traits>

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMemoriaAdapterTest, "Memoria.Foundation.MemoryAdapter",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMemoriaAdapterTest::RunTest(const FString& Parameters)
{
    auto* Domain = NewObject<UMemoriaPlayerMemoryDomain>();
    FMemoriaMemoryDefinition Definition;
    Definition.Id = TEXT("ch1_test"); Definition.RawGrade = EMemoriaMemoryGrade::Grade3; Definition.BurnPower = 100;
    Definition.SourcePath = TEXT("fixture://adapter");
    FMemoriaMemoryContext Context;
    int32 Events = 0;
    Domain->OnObserved.AddLambda([&](const FMemoriaMemoryEvent& Event)
    {
        ++Events;
        if (Event.Kind == EMemoriaMemoryEventKind::Added)
        {
            TestEqual(TEXT("Definition visible during acquisition"), Domain->GetDefinitions().Num(), 1);
            TestEqual(TEXT("State visible during acquisition"), Domain->GetSnapshot().Owned.Num(), 1);
        }
        if (Event.Kind == EMemoriaMemoryEventKind::ResidueCreated)
        {
            TestTrue(TEXT("Residue visible before history append"), Domain->HasResidue(Definition.Id));
            TestEqual(TEXT("History still excludes burn"), Domain->GetSnapshot().BurnedHistory.Num(), 0);
            TestTrue(TEXT("Reentrant burn rejected"), Domain->Burn(Definition.Id, EMemoriaBurnMode::Silent, true, Context) == EMemoriaMemoryResult::Busy);
            TestTrue(TEXT("Reentrant restore rejected"), Domain->Restore({}, {}) == EMemoriaMemoryResult::Busy);
        }
    });
    TestTrue(TEXT("Acquire"), Domain->Add(Definition, Context) == EMemoriaMemoryResult::Success);
    TestTrue(TEXT("Burn"), Domain->Burn(Definition.Id, EMemoriaBurnMode::Normal, false, Context) == EMemoriaMemoryResult::Success);
    TestEqual(TEXT("Acquisition and normal burn events"), Events, 5);
    TestEqual(TEXT("History after burn"), Domain->GetSnapshot().BurnedHistory.Num(), 1);
    auto* Restored = NewObject<UMemoriaPlayerMemoryDomain>();
    TestTrue(TEXT("Round trip memory DTO"), Restored->Restore(Domain->GetDefinitions(), Domain->GetSnapshot()) == EMemoriaMemoryResult::Success);
    TestTrue(TEXT("Round trip residue"), Restored->HasResidue(Definition.Id));
    TestEqual(TEXT("Provenance retained"), Restored->GetDefinitions()[0].SourcePath, Definition.SourcePath);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMemoriaSaveBoundaryTest, "Memoria.Foundation.SaveAndDialectBoundaries",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMemoriaSaveBoundaryTest::RunTest(const FString& Parameters)
{
    static_assert(!std::is_same_v<FFieldDialogueLine, FVNStoryStep>);
    static_assert(!std::is_same_v<FMemoriaWorldCognitionSaveSection, FMemoriaMemorySnapshot>);
    auto* Save = NewObject<UMemoriaRunSaveGame>();
    Save->Run.RunId = FGuid::NewGuid(); Save->Run.ContentRevision = TEXT("test-revision"); Save->ContentRevision = TEXT("test-revision");
    Save->Run.StoryFlags.AddDefaulted_GetRef().Id = TEXT("absent_vs_false");
    FString Error;
    for (int32 Slot = 0; Slot <= 3; ++Slot) { Save->Slot = Slot; TestTrue(TEXT("Supported slot"), Save->ValidateHeader(Error)); }
    Save->Slot = 4; TestFalse(TEXT("Reject slot 4"), Save->ValidateHeader(Error)); Save->Slot = 1;
    Save->SchemaVersion = 2; TestFalse(TEXT("Reject future schema"), Save->ValidateHeader(Error)); Save->SchemaVersion = 1;
    Save->SceneFlow.Current.SequenceId = TEXT("test_sequence"); Save->SceneFlow.Current.OriginalIndex = 7;
    Save->SceneFlow.ResumeQueue.Add(Save->SceneFlow.Current); Save->SceneFlow.LedgerBurnSnapshot = 3;
    FMemoriaMemoryDefinition Definition; Definition.Id = TEXT("dynamic_memory"); Definition.RawGrade = EMemoriaMemoryGrade::Grade1;
    Definition.SourceHash = TEXT("test-hash"); Save->MemoryDefinitions.Add(Definition);
    Save->PlayerMemory.Owned.AddDefaulted_GetRef().Id = Definition.Id;
    Save->WorldCognition.SourceJson = TEXT("{\"actors\":{}}");
    TArray<uint8> Bytes;
    FMemoryWriter Writer(Bytes);
    FObjectAndNameAsStringProxyArchive WriteArchive(Writer, false); WriteArchive.ArIsSaveGame = true;
    Save->Serialize(WriteArchive);
    auto* Copy = NewObject<UMemoriaRunSaveGame>();
    FMemoryReader Reader(Bytes);
    FObjectAndNameAsStringProxyArchive ReadArchive(Reader, true); ReadArchive.ArIsSaveGame = true;
    Copy->Serialize(ReadArchive);
    TestTrue(TEXT("Serialized header"), Copy->ValidateHeader(Error));
    TestTrue(TEXT("Run identity persists"), Copy->Run.RunId == Save->Run.RunId);
    TestTrue(TEXT("Stored false remains present"), Copy->Run.HasFlag(TEXT("absent_vs_false")));
    TestFalse(TEXT("Stored false value"), Copy->Run.GetFlag(TEXT("absent_vs_false")));
    TestFalse(TEXT("Flags case sensitive"), Copy->Run.HasFlag(TEXT("ABSENT_VS_FALSE")));
    TestEqual(TEXT("Dynamic definition count"), Copy->MemoryDefinitions.Num(), 1);
    if (Copy->MemoryDefinitions.Num() == 1)
    {
        TestEqual(TEXT("Dynamic definition provenance"), Copy->MemoryDefinitions[0].SourceHash, Definition.SourceHash);
        TestTrue(TEXT("Raw grade serialized"), Copy->MemoryDefinitions[0].RawGrade == EMemoriaMemoryGrade::Grade1);
    }
    TestEqual(TEXT("VN original index"), Copy->SceneFlow.Current.OriginalIndex, 7);
    TestEqual(TEXT("VN FIFO queue"), Copy->SceneFlow.ResumeQueue.Num(), 1);
    TestEqual(TEXT("World data separate"), Copy->WorldCognition.SourceJson, Save->WorldCognition.SourceJson);
    TestTrue(TEXT("Source coordinate round trip"), Memoria::Coordinates::ToSource(Memoria::Coordinates::FromSource(FVector2D(42.0, -7.0))) == FVector2D(42.0, -7.0));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMemoriaRunOwnershipTest, "Memoria.Foundation.RunOwnership",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMemoriaRunOwnershipTest::RunTest(const FString& Parameters)
{
    auto* Game = NewObject<UGameInstance>();
    Game->Init();
    auto* Run = Game->GetSubsystem<UMemoriaRunSubsystem>();
    if (!TestNotNull(TEXT("GameInstance creates run subsystem"), Run)) { Game->Shutdown(); return false; }
    TestTrue(TEXT("Domain owned by run"), Run->GetPlayerMemory()->GetOuter() == Run);
    TestFalse(TEXT("No implicit campaign start"), Run->HasActiveRun());
    auto* Catalog = NewObject<UMemoriaMemoryCatalog>(); Catalog->ContentRevision = TEXT("ownership-test");
    TestTrue(TEXT("Explicit begin"), Run->BeginRun(*Catalog, {}) == EMemoriaMemoryResult::Success);
    const FGuid FirstId = Run->GetRunSnapshot().RunId;
    Run->SetStoryFlag(TEXT("canon_test"), true);
    TestTrue(TEXT("Explicit reset"), Run->BeginRun(*Catalog, {}) == EMemoriaMemoryResult::Success);
    TestTrue(TEXT("New identity per run"), FirstId != Run->GetRunSnapshot().RunId);
    TestFalse(TEXT("No old canon flags"), Run->GetRunSnapshot().HasFlag(TEXT("canon_test")));
    Game->Shutdown();
    return true;
}
#endif
