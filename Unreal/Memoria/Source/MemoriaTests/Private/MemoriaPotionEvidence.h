#pragma once
#include "Misc/AutomationTest.h"
#include "MemoriaPlayerObservation.h"
#include "Save/MemoriaRunSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/GameInstance.h"
#include "UObject/StrongObjectPtr.h"
#include "JsonObjectConverter.h"
#include "Serialization/JsonSerializer.h"
#include "Import/MemoriaStartingCatalogImport.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
namespace MemoriaPotionEvidence
{
using Obj=TSharedPtr<FJsonObject>;
inline FString Canon(const Obj& O){return MemoriaCatalogImport::Canonical(MakeShared<FJsonValueObject>(O));}
inline Obj Parse(const FString& S){Obj O;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(S),O);return O;}
inline Obj Inventory(const FMemoriaRunSnapshot& S)
{
    auto O=MakeShared<FJsonObject>(),Items=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Recent;
    for(const auto& I:S.Player.Items)Items->SetNumberField(I.Id,I.Count);
    for(const auto& R:S.Player.RecentItems)Recent.Add(MakeShared<FJsonValueString>(R));
    O->SetObjectField(TEXT("items"),Items);O->SetArrayField(TEXT("recent"),Recent);return O;
}
inline void Write(const FString& Name,const Obj& O)
{
    const auto Dir=FPaths::ProjectSavedDir()/TEXT("Validation/Phase1N");IFileManager::Get().MakeDirectory(*Dir,true);
    FFileHelper::SaveStringToFile(Canon(O)+TEXT("\n"),*(Dir/Name),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}
inline Obj Full(UMemoriaRunSubsystem& Run)
{
    auto O=MakeShared<FJsonObject>();O->SetObjectField(TEXT("run"),FJsonObjectConverter::UStructToJsonObject(Run.GetRunSnapshot()));
    O->SetObjectField(TEXT("inventory"),Inventory(Run.GetRunSnapshot()));
    O->SetObjectField(TEXT("player"),FJsonObjectConverter::UStructToJsonObject(Run.GetPlayerMemory()->GetSnapshot()));
    O->SetObjectField(TEXT("player_observables"),MemoriaPlayerObservation(Run));
    O->SetObjectField(TEXT("world"),Parse(Run.GetWorldCognition()->ExportJson()));return O;
}
inline bool SaveRoundTrip(FAutomationTestBase& Test,UMemoriaRunSubsystem& Run,const FString& Name)
{
    auto Evidence=MakeShared<FJsonObject>();const auto Before=Full(Run);
    TStrongObjectPtr<UMemoriaRunSaveGame> Save(Run.CaptureSave());TArray<uint8> Bytes;
    if(!Test.TestTrue(TEXT("Actual binary SaveGame write"),Save.IsValid() && UGameplayStatics::SaveGameToMemory(Save.Get(),Bytes)))return false;
    TStrongObjectPtr<UMemoriaRunSaveGame> Loaded(Cast<UMemoriaRunSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes)));
    if(!Test.TestTrue(TEXT("Actual binary SaveGame read"),Loaded.IsValid()))return false;
    TStrongObjectPtr<UGameInstance> Game(NewObject<UGameInstance>());Game->Init();auto* Restored=Game->GetSubsystem<UMemoriaRunSubsystem>();
    int32 RestoreSignals=0,RestoreToasts=0,RestoreGrants=0;
    Restored->OnInventoryChanged.AddLambda([&](const FString&){++RestoreSignals;});
    Restored->OnItemToastRequested.AddLambda([&](const FString&,int32){++RestoreToasts;});
    Restored->OnRewardItemObserved.AddLambda([&](const FString&,const FString&,const FMemoriaRunSnapshot&){++RestoreGrants;});
    Test.TestTrue(TEXT("Restore binary save into independent authority"),Restored->RestoreSave(*Loaded));
    const auto After=Full(*Restored);
    TArray<TSharedPtr<FJsonValue>> Query;
    for(const auto& R:Restored->GetRecentItems())Query.Add(MakeShared<FJsonValueString>(R));
    Evidence->SetArrayField(TEXT("restored_recent_query"),Query);
    Test.TestTrue(TEXT("Restored normalized query agrees with original"),Restored->GetRecentItems()==Run.GetRecentItems());
    Test.TestEqual(TEXT("Query after binary restore does not mutate raw recent"),Canon(Full(*Restored)),Canon(After));
    for(const TCHAR* Section:{TEXT("run"),TEXT("inventory"),TEXT("player"),TEXT("player_observables"),TEXT("world")})
        Test.TestEqual(FString(TEXT("Binary save exact "))+Section,Canon(Before->GetObjectField(Section)),Canon(After->GetObjectField(Section)));
    Test.TestEqual(TEXT("Canonical original authority unchanged by independent restore"),Canon(Full(Run)),Canon(Before));
    Evidence->SetObjectField(TEXT("before"),Before);Evidence->SetObjectField(TEXT("after"),After);Evidence->SetNumberField(TEXT("binary_bytes"),Bytes.Num());Evidence->SetNumberField(TEXT("schema"),Loaded->SchemaVersion);
    Test.TestEqual(TEXT("Restore emits no inventory signal"),RestoreSignals,0);Test.TestEqual(TEXT("Restore emits no toast"),RestoreToasts,0);Test.TestEqual(TEXT("Restore does not replay grants"),RestoreGrants,0);
    Evidence->SetNumberField(TEXT("restore_signals"),RestoreSignals);Evidence->SetNumberField(TEXT("restore_toasts"),RestoreToasts);Evidence->SetNumberField(TEXT("restore_grants"),RestoreGrants);
    Write(Name,Evidence);Game->Shutdown();return !Test.HasAnyErrors();
}
}
