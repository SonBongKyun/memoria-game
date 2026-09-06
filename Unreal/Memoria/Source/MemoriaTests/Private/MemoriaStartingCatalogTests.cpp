#include "Misc/AutomationTest.h"
#include "Import/MemoriaStartingCatalogImport.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Engine/GameInstance.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
using Obj = TSharedPtr<FJsonObject>;
FString IrPath() { return FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()/TEXT("../../docs/unreal-migration/ir/starting_memory_catalog.v1.json")); }
Obj ReadJson(const FString& Path)
{
    FString Text; Obj O;
    if (FFileHelper::LoadFileToString(Text,*Path)) { FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),O); }
    return O;
}
void Eq(FAutomationTestBase& T, const TCHAR* Label, const FString& A, const FString& B)
{ T.TestTrue(Label,A.Equals(B,ESearchCase::CaseSensitive)); }
void Strings(FAutomationTestBase& T, const TCHAR* Label, const TArray<FString>& Actual, const TArray<TSharedPtr<FJsonValue>>& Expected)
{
    T.TestEqual(Label,Actual.Num(),Expected.Num());
    for (int32 I=0; I<FMath::Min(Actual.Num(),Expected.Num()); ++I) { Eq(T,Label,Actual[I],Expected[I]->AsString()); }
}
void Snapshot(FAutomationTestBase& T, UMemoriaPlayerMemoryDomain& D, const Obj& E)
{
    auto S=D.GetSnapshot(); const auto& Owned=E->GetArrayField(TEXT("owned"));
    T.TestEqual(TEXT("Owned count"),S.Owned.Num(),Owned.Num());
    for (int32 I=0; I<FMath::Min(S.Owned.Num(),Owned.Num()); ++I)
    {
        const auto& A=S.Owned[I]; auto R=Owned[I]->AsObject();
        Eq(T,TEXT("Exact ordered ID"),A.Id,R->GetStringField(TEXT("id")));
        T.TestEqual(TEXT("Burned"),A.bBurned,R->GetBoolField(TEXT("burned")));
        T.TestEqual(TEXT("Residue"),A.bResidue,R->GetBoolField(TEXT("residue")));
        T.TestEqual(TEXT("Faded"),A.bFaded,R->GetBoolField(TEXT("faded")));
        T.TestEqual(TEXT("Erosion"),A.Erosion,int64(R->GetNumberField(TEXT("erosion"))));
        Strings(T,TEXT("Derived connection order"),A.Connections,R->GetArrayField(TEXT("connections")));
        T.TestEqual(TEXT("Effective power"),D.GetEffectiveBurnPower(A.Id),int64(E->GetArrayField(TEXT("effective_powers"))[I]->AsNumber()));
        T.TestEqual(TEXT("Intact"),D.IsIntact(A.Id),E->GetArrayField(TEXT("intact"))[I]->AsBool());
    }
    Strings(T,TEXT("Burn history"),S.BurnedHistory,E->GetArrayField(TEXT("history")));
    Strings(T,TEXT("Available order"),D.GetAvailable(),E->GetArrayField(TEXT("available")));
    Strings(T,TEXT("Available including faded"),D.GetAvailable(EMemoriaMemoryGrade::Grade5,true),E->GetArrayField(TEXT("available_allow_faded")));
    Strings(T,TEXT("Guards"),S.ErosionGuarded,E->GetArrayField(TEXT("guards")));
    Strings(T,TEXT("Extracted"),S.Extracted,E->GetArrayField(TEXT("extracted")));
    auto Passives=E->GetObjectField(TEXT("passives"));
    T.TestEqual(TEXT("Passive count"),S.BurnPassives.Num(),Passives->Values.Num());
    for (const auto& P:S.BurnPassives) { T.TestTrue(TEXT("Passive ID/value"),Passives->HasField(P.Id) && P.bValue==Passives->GetBoolField(P.Id)); }
    T.TestEqual(TEXT("Carry weight"),D.GetCarryWeight(),int64(E->GetNumberField(TEXT("carry_weight"))));
    T.TestEqual(TEXT("Carry capacity"),D.GetCarryCapacity(1),int64(E->GetNumberField(TEXT("carry_capacity"))));
    FMemoriaMemoryContext Context;
    T.TestTrue(TEXT("Carry overload"),FMath::Abs(D.GetCarryOverload(Context)-E->GetNumberField(TEXT("overload")))<1e-12);
    T.TestFalse(TEXT("No baked collateral"),S.ActiveLoan.bActive);
    T.TestEqual(TEXT("No baked preservation"),S.AnchorVigil,int64(0));
    T.TestEqual(TEXT("No baked anchor passives"),S.AnchorPassives.Num(),0);
    T.TestEqual(TEXT("No baked vigil chapters"),S.VigilChapters.Num(),0);
    T.TestEqual(TEXT("No baked guard usage"),S.GuardSlotsUsed,int64(0));
}
void RefreshHash(const Obj& O)
{
    auto Payload=MakeShared<FJsonObject>(); Payload->SetNumberField(TEXT("schema_version"),1);
    Payload->SetStringField(TEXT("content_kind"),TEXT("player_memory.starting_catalog"));
    Payload->SetArrayField(TEXT("entries"),O->GetArrayField(TEXT("entries")));
    O->SetStringField(TEXT("semantic_sha256"),MemoriaCatalogImport::Sha256(MemoriaCatalogImport::Canonical(MakeShared<FJsonValueObject>(Payload))+TEXT("\n")));
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMemoriaStartingCatalogParity,"Memoria.Content.StartingCatalogParity",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMemoriaStartingCatalogParity::RunTest(const FString& Parameters)
{
    auto* Asset=LoadObject<UMemoriaMemoryCatalog>(nullptr,MemoriaCatalogImport::ObjectPath);
    if (!TestNotNull(TEXT("Saved typed production catalog loads in Automation process"),Asset)) { return false; }
    TStrongObjectPtr<UMemoriaMemoryCatalog> Expected(NewObject<UMemoriaMemoryCatalog>()); FString Error;
    if (!TestTrue(TEXT("Canonical IR and source hashes validate"),MemoriaCatalogImport::ReadIr(IrPath(),*Expected,Error))) { AddError(Error); return false; }
    for (TFieldIterator<FProperty> P(UMemoriaMemoryCatalog::StaticClass(),EFieldIterationFlags::None); P; ++P)
    { TestTrue(*P->GetName(),P->Identical_InContainer(Asset,Expected.Get())); }
    Eq(*this,TEXT("Independently source-derived canonical semantic SHA256"),MemoriaCatalogImport::Fingerprint(*Asset),Expected->SemanticSha256);
    Obj Ir=ReadJson(IrPath()); const auto& Rows=Ir->GetArrayField(TEXT("entries"));
    TestEqual(TEXT("Actual source entry count"),Asset->Definitions.Num(),Rows.Num());
    for (int32 I=0; I<Rows.Num(); ++I)
    {
        const auto& D=Asset->Definitions[I]; const auto& R=Rows[I]->AsObject();
        Eq(*this,TEXT("Case-sensitive ID and order"),D.Id,R->GetStringField(TEXT("id")));
        Eq(*this,TEXT("Title"),D.Title,R->GetStringField(TEXT("title"))); Eq(*this,TEXT("Description"),D.Description,R->GetStringField(TEXT("description")));
        Eq(*this,TEXT("Story effect"),D.StoryEffect,R->GetStringField(TEXT("story_effect"))); Eq(*this,TEXT("Related NPC"),D.RelatedNpc,R->GetStringField(TEXT("related_npc")));
        TestEqual(TEXT("Raw grade"),int32(D.RawGrade),int32(R->GetNumberField(TEXT("grade"))));
        TestEqual(TEXT("Burn power"),D.BurnPower,int64(R->GetNumberField(TEXT("burn_power"))));
        Eq(*this,TEXT("Definition source hash"),D.SourceHash,Asset->Sources[0].Sha256Utf8Lf);
    }
    AddInfo(FString::Printf(TEXT("Catalog entries=%d semantic=%s"),Rows.Num(),*Asset->SemanticSha256));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMemoriaStartingCatalogRuntime,"Memoria.Content.StartingCatalogRuntime",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMemoriaStartingCatalogRuntime::RunTest(const FString& Parameters)
{
    Obj Oracle=ReadJson(FPaths::GetPath(IrPath())/TEXT("starting_memory_oracle.v1.json"));
    if (!TestTrue(TEXT("Source-executed command oracle exists"),Oracle.IsValid())) { return false; }
    FString IrText; FFileHelper::LoadFileToString(IrText,*IrPath());
    Eq(*this,TEXT("Oracle is tied to this IR"),Oracle->GetStringField(TEXT("catalog_ir_sha256")),MemoriaCatalogImport::Sha256(IrText));
    TStrongObjectPtr<UGameInstance> Game(NewObject<UGameInstance>()); Game->Init();
    auto* Run=Game->GetSubsystem<UMemoriaRunSubsystem>();
    if (!TestTrue(TEXT("Production bootstrap loads typed catalog without campaign"),Run && Run->BeginStartingMemoryRun()==EMemoriaMemoryResult::Success)) { Game->Shutdown(); return false; }
    auto* Domain=Run->GetPlayerMemory(); Snapshot(*this,*Domain,Oracle->GetObjectField(TEXT("initial")));
    auto* Asset=LoadObject<UMemoriaMemoryCatalog>(nullptr,MemoriaCatalogImport::ObjectPath); const FString Before=MemoriaCatalogImport::Fingerprint(*Asset);
    const auto& Commands=Oracle->GetObjectField(TEXT("behavior_input"))->GetArrayField(TEXT("commands"));
    const auto& Steps=Oracle->GetObjectField(TEXT("behavior"))->GetArrayField(TEXT("steps"));
    TestEqual(TEXT("Source command/step count"),Commands.Num(),Steps.Num());
    const TArray<FString> Kinds={TEXT("Added"),TEXT("ResidueCreated"),TEXT("Faded"),TEXT("Cascaded"),TEXT("Burned"),TEXT("CarryChanged"),TEXT("PassiveUnlocked"),TEXT("MemoriesEroded")};
    for (int32 I=0; I<Commands.Num(); ++I)
    {
        auto C=Commands[I]->AsObject(); auto Step=Steps[I]->AsObject(); const auto& Events=Step->GetArrayField(TEXT("events")); int32 Seen=0;
        auto Handle=Domain->OnObserved.AddLambda([&](const FMemoriaMemoryEvent& E)
        {
            if (!TestTrue(TEXT("No extra domain event"),Events.IsValidIndex(Seen))) { ++Seen; return; }
            auto Expected=Events[Seen++]->AsObject();
            Eq(*this,TEXT("Event order/kind"),Kinds[int32(E.Kind)],Expected->GetStringField(TEXT("kind")));
            Eq(*this,TEXT("Event ID"),E.MemoryId,Expected->GetStringField(TEXT("memory_id")));
            TestEqual(TEXT("Event amount"),E.Amount,int64(Expected->GetNumberField(TEXT("amount"))));
            Strings(*this,TEXT("Event affected order"),E.AffectedIds,Expected->GetArrayField(TEXT("affected_ids")));
            TestEqual(TEXT("Event weight"),E.CarryWeight,int64(Expected->GetNumberField(TEXT("weight"))));
            TestEqual(TEXT("Event capacity"),E.CarryCapacity,int64(Expected->GetNumberField(TEXT("capacity"))));
            Eq(*this,TEXT("Event passive name"),E.PassiveName,Expected->GetStringField(TEXT("passive_name")));
            Snapshot(*this,*Domain,Expected->GetObjectField(TEXT("observation")));
        });
        EMemoriaMemoryResult Result=EMemoriaMemoryResult::InvalidSnapshot; const FString Op=C->GetStringField(TEXT("op"));
        if (Op==TEXT("erode")) { Result=Run->ErodeMemories(int64(C->GetNumberField(TEXT("chapter")))); }
        else if (Op==TEXT("passives")) { Result=Domain->EvaluatePassives(); }
        else if (Op==TEXT("burn") || Op==TEXT("silent")) { Result=Run->BurnMemory(C->GetStringField(TEXT("id")),Op==TEXT("silent") ? EMemoriaBurnMode::Silent : EMemoriaBurnMode::Normal,C->GetBoolField(TEXT("allow_faded"))); }
        Domain->OnObserved.Remove(Handle);
        TestEqual(TEXT("Command source success"),Result==EMemoriaMemoryResult::Success,Step->GetBoolField(TEXT("success")));
        TestEqual(TEXT("Exact event count"),Seen,Events.Num()); Snapshot(*this,*Domain,Step->GetObjectField(TEXT("state")));
    }
    TestTrue(TEXT("Imported memories reached first passive threshold"),Domain->HasPassive(TEXT("ember_affinity")));
    Eq(*this,TEXT("Commands never mutate immutable asset"),Before,MemoriaCatalogImport::Fingerprint(*Asset));
    TestTrue(TEXT("New memory run resets mutable state"),Run->BeginStartingMemoryRun()==EMemoriaMemoryResult::Success);
    Snapshot(*this,*Domain,Oracle->GetObjectField(TEXT("initial")));
    Game->Shutdown(); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMemoriaStartingCatalogValidation,"Memoria.Content.StartingCatalogValidation",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMemoriaStartingCatalogValidation::RunTest(const FString& Parameters)
{
    const FString Temp=FPaths::ProjectSavedDir()/TEXT("Validation/starting-catalog-mutation.json");
    TStrongObjectPtr<UMemoriaMemoryCatalog> C(NewObject<UMemoriaMemoryCatalog>()); FString Error;
    auto Attempt=[&](const Obj& O,bool Expected,const TCHAR* Label)
    {
        const FString Text=MemoriaCatalogImport::Canonical(MakeShared<FJsonValueObject>(O))+TEXT("\n");
        FFileHelper::SaveStringToFile(Text,*Temp,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
        TestEqual(Label,MemoriaCatalogImport::ReadIr(Temp,*C,Error),Expected);
    };
    for (const FString Kind : {TEXT("schema"),TEXT("kind"),TEXT("grade"),TEXT("fractional"),TEXT("power"),TEXT("empty_id"),TEXT("duplicate"),TEXT("mutable"),TEXT("npc"),TEXT("source"),TEXT("hash")})
    {
        auto O=ReadJson(IrPath()); auto R=O->GetArrayField(TEXT("entries"))[0]->AsObject();
        if (Kind==TEXT("schema")) { O->SetNumberField(TEXT("schema_version"),2); }
        if (Kind==TEXT("kind")) { O->SetStringField(TEXT("content_kind"),TEXT("world_memory")); }
        if (Kind==TEXT("grade")) { R->SetNumberField(TEXT("grade"),5); }
        if (Kind==TEXT("fractional")) { R->SetNumberField(TEXT("grade"),0.5); }
        if (Kind==TEXT("power")) { R->SetNumberField(TEXT("burn_power"),0); }
        if (Kind==TEXT("empty_id")) { R->SetStringField(TEXT("id"),TEXT("")); }
        if (Kind==TEXT("duplicate")) { R->SetStringField(TEXT("id"),O->GetArrayField(TEXT("entries"))[1]->AsObject()->GetStringField(TEXT("id"))); }
        if (Kind==TEXT("mutable")) { R->SetBoolField(TEXT("burned"),false); }
        if (Kind==TEXT("npc")) { R->SetNumberField(TEXT("related_npc"),9); }
        if (Kind==TEXT("source")) { O->GetArrayField(TEXT("sources"))[0]->AsObject()->SetStringField(TEXT("sha256_utf8_lf"),FString::ChrN(64,'0')); }
        if (Kind==TEXT("hash")) { R->SetStringField(TEXT("title"),TEXT("Temporary difference")); }
        if (Kind!=TEXT("hash") && Kind!=TEXT("fractional")) { RefreshHash(O); }
        Attempt(O,false,*Kind);
    }
    auto Changed=ReadJson(IrPath()); const FString OriginalHash=Changed->GetStringField(TEXT("semantic_sha256"));
    Changed->GetArrayField(TEXT("entries"))[0]->AsObject()->SetStringField(TEXT("title"),TEXT("Temporary semantic difference")); RefreshHash(Changed);
    Attempt(Changed,true,TEXT("Valid temporary semantic change loads only into transient object"));
    TestFalse(TEXT("Fingerprint detects changed field"),MemoriaCatalogImport::Fingerprint(*C).Equals(OriginalHash,ESearchCase::CaseSensitive));
    auto Case=ReadJson(IrPath()); auto& Rows=Case->GetArrayField(TEXT("entries"));
    Rows[0]->AsObject()->SetStringField(TEXT("id"),Rows[1]->AsObject()->GetStringField(TEXT("id")).ToUpper()); RefreshHash(Case);
    Attempt(Case,true,TEXT("IDs differing only in case stay distinct"));
    FString Text; FFileHelper::LoadFileToString(Text,*IrPath());
    FFileHelper::SaveStringToFile(TEXT(" ")+Text,*Temp,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    TestFalse(TEXT("Noncanonical IR rejected"),MemoriaCatalogImport::ReadIr(Temp,*C,Error));
    IFileManager::Get().Delete(*Temp);
    AddInfo(TEXT("Temporary IR semantic change detected; no temporary package saved"));
    return true;
}
#endif
