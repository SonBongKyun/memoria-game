#include "Save/MemoriaCheckpointSubsystem.h"
#include "Save/MemoriaRunSaveGame.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Chapter/MemoriaChapterMap.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UObject/StrongObjectPtr.h"
#include "Misc/Base64.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "HAL/FileManager.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#if PLATFORM_WINDOWS
#include "Windows/WindowsHWrapper.h"
#endif

namespace
{
constexpr int64 MaxFileBytes = 4 * 1024 * 1024;
const TCHAR* CatalogPath = TEXT("/Game/Memoria/Generated/Memory/DA_StartingMemoryCatalog.DA_StartingMemoryCatalog");
FString Digest(const TArray<uint8>& Bytes)
{
    FSHAHash Hash; FSHA1::HashBuffer(Bytes.GetData(), Bytes.Num(), Hash.Hash); return Hash.ToString();
}
}
void UMemoriaCheckpointSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    Collection.InitializeDependency<UMemoriaRunSubsystem>();
    Run = GetGameInstance()->GetSubsystem<UMemoriaRunSubsystem>();
    // Synthetic runs are opt-in and can only select a validation directory.
    if (!IsRunningCommandlet() && !FApp::IsUnattended() && !GIsAutomationTesting)
        StorageRoot = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("SaveGames/MemoriaSlice"));
#if WITH_DEV_AUTOMATION_TESTS
    FString TestLeaf;
    if (FParse::Value(FCommandLine::Get(),TEXT("MemoriaCheckpointTestLeaf="),TestLeaf)) ConfigureTestStorage(TestLeaf);
#endif
    StatusText = IsStorageEnabled() ? TEXT("No checkpoint saved in this session.") : TEXT("Disk saving is disabled for this test session.");
}
bool UMemoriaCheckpointSubsystem::CanAccess() const
{
    return IsStorageEnabled() && Run && !Run->GetPlayerMemory()->IsDispatchingEvent()
        && !Run->GetWorldCognition()->IsDispatchingEvent();
}
FString UMemoriaCheckpointSubsystem::GetSlotPath() const
{
    return StorageRoot.IsEmpty() ? FString() : StorageRoot/TEXT("autosave.memoria.json");
}
#if WITH_DEV_AUTOMATION_TESTS
bool UMemoriaCheckpointSubsystem::ConfigureTestStorage(const FString& Leaf)
{
    if (bBusy || !(GIsAutomationTesting || FApp::IsUnattended()) || Leaf.IsEmpty() || Leaf.Len()>80) return false;
    for (TCHAR C : Leaf) if (!FChar::IsAlnum(C) && C != TCHAR('-')) return false;
    StorageRoot = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Validation/CheckpointTests")/Leaf);
    return true;
}
#endif
bool UMemoriaCheckpointSubsystem::Fail(const TCHAR* Message)
{
    StatusText = Message; return false;
}
bool UMemoriaCheckpointSubsystem::ValidateSnapshot(const UMemoriaRunSaveGame& Save) const
{
    FString Error;
    const auto* Catalog = LoadObject<UMemoriaMemoryCatalog>(nullptr, CatalogPath);
    const auto& P = Save.FieldReturn.SourcePixelPosition;
    if (!Catalog || !Save.ValidateHeader(Error) || Save.Slot!=0 ||
        Save.ContentRevision!=Catalog->ContentRevision || Save.MemoryCatalogId!=Catalog->GetPrimaryAssetId() ||
        Save.SavedAtUtc.GetTicks()<=0 || !Save.ImportedFromVersion.IsEmpty() ||
        Save.FieldReturn.MapId!=BoundaryId || Save.FieldReturn.SourceScenePath!=SourceScene ||
        !FMath::IsFinite(P.X) || !FMath::IsFinite(P.Y) || P.X<0 || P.X>2400 || P.Y<0 || P.Y>1600 ||
        Save.Run.CurrentChapter!=3 || !Save.Run.GetFlag(TEXT("ch2_complete")) ||
        Save.SceneFlow.bActive || !Save.SceneFlow.Current.SequenceId.IsEmpty() ||
        !Save.SceneFlow.Pending.SequenceId.IsEmpty() || !Save.SceneFlow.ResumeQueue.IsEmpty() || Save.SceneFlow.LedgerBurnSnapshot!=0 ||
        Save.Diary.SchemaVersion!=0 || !Save.Diary.SourceJson.IsEmpty() ||
        Save.Hints.SchemaVersion!=0 || !Save.Hints.SourceJson.IsEmpty() ||
        Save.WorldCognition.SchemaVersion!=1 || Save.WorldCognition.SourceJson.IsEmpty()) return false;
    return ValidateDomains(Save);
}
bool UMemoriaCheckpointSubsystem::ValidateDomains(const UMemoriaRunSaveGame& Save) const
{
    // Validate into disposable domains BEFORE touching the live run.
    FMemoriaWorldSnapshot World;
    auto* CandidateWorld = NewObject<UMemoriaWorldCognition>();
    auto* CandidateMemory = NewObject<UMemoriaPlayerMemoryDomain>();
    return UMemoriaWorldCognition::Decode(Save.WorldCognition.SourceJson, World) && CandidateWorld->Restore(World) &&
        CandidateMemory->Restore(Save.MemoryDefinitions, Save.PlayerMemory)==EMemoriaMemoryResult::Success;
}
bool UMemoriaCheckpointSubsystem::ValidateChapterSnapshot(const UMemoriaRunSaveGame& Save) const
{
    FString Error;
    const auto* Catalog = LoadObject<UMemoriaMemoryCatalog>(nullptr, CatalogPath);
    const auto& Flow = Save.SceneFlow;
    // A chapter autosave resumes inside the VN route: an active cursor on a named scene.
    if (!Catalog || !Save.ValidateHeader(Error) || Save.Slot!=0 ||
        Save.ContentRevision!=Catalog->ContentRevision || Save.MemoryCatalogId!=Catalog->GetPrimaryAssetId() ||
        Save.SavedAtUtc.GetTicks()<=0 || !Save.ImportedFromVersion.IsEmpty() ||
        Save.FieldReturn.MapId!=ChapterBoundaryId || Save.FieldReturn.SourceScenePath!=ChapterSourceScene ||
        Save.Run.CurrentChapter<2 || !Flow.bActive || Flow.Current.SequenceId.IsEmpty() || Flow.Current.OriginalIndex<0 ||
        Save.WorldCognition.SchemaVersion!=1 || Save.WorldCognition.SourceJson.IsEmpty()) return false;
    return ValidateDomains(Save);
}
FString UMemoriaCheckpointSubsystem::GetChapterSlotPath() const
{
    return StorageRoot.IsEmpty() ? FString() : StorageRoot/TEXT("chapter.memoria.json");
}
bool UMemoriaCheckpointSubsystem::SaveChapterTransition(UMemoriaRunSaveGame& Save)
{
    if (bBusy || !CanAccess()) return Fail(TEXT("Autosave skipped: disk storage is unavailable in this session."));
    TGuardValue<bool> Busy(bBusy,true);
    const auto* Catalog=LoadObject<UMemoriaMemoryCatalog>(nullptr,CatalogPath);
    if (!Catalog) return Fail(TEXT("Autosave skipped: no memory catalog."));
    Save.SavedAtUtc=FDateTime::UtcNow(); Save.MemoryCatalogId=Catalog->GetPrimaryAssetId();
    Save.FieldReturn.SourceScenePath=ChapterSourceScene; Save.FieldReturn.MapId=ChapterBoundaryId; Save.FieldReturn.SourcePixelPosition=FVector2D::ZeroVector;
    if (!ValidateChapterSnapshot(Save)) return Fail(TEXT("Autosave skipped: this run is outside the supported boundary."));
    TArray<uint8> Bytes;
    if (!UGameplayStatics::SaveGameToMemory(&Save,Bytes)) return Fail(TEXT("Autosave skipped: serialization failed."));
    auto Object=MakeShared<FJsonObject>(); Object->SetNumberField(TEXT("version"),1);
    Object->SetStringField(TEXT("sha1"),Digest(Bytes)); Object->SetStringField(TEXT("payload"),FBase64::Encode(Bytes));
    FString Text; FJsonSerializer::Serialize(Object,TJsonWriterFactory<>::Create(&Text));
    if (Text.Len()>MaxFileBytes || !IFileManager::Get().MakeDirectory(*StorageRoot,true)) return Fail(TEXT("Autosave skipped: cannot create the save folder."));
    if (!CommitDiskFile(GetChapterSlotPath(),Text,ESlot::Chapter)) return Fail(TEXT("Autosave failed: disk write failed."));
    StatusText=TEXT("Chapter autosaved."); return true;
}
UMemoriaRunSaveGame* UMemoriaCheckpointSubsystem::LoadChapterTransition()
{
    if (bBusy || !CanAccess()) { Fail(TEXT("Autosave unavailable: disk storage is disabled or busy.")); return nullptr; }
    auto* Save=ReadFile(GetChapterSlotPath(),ESlot::Chapter);
    if (!Save) Fail(TEXT("No valid chapter autosave found."));
    return Save;
}
EMemoriaContinueSource UMemoriaCheckpointSubsystem::FindContinue() const
{
    if (!IsStorageEnabled()) return EMemoriaContinueSource::None;
    // The source has one slot; here the newest valid file stands for it, by the time each save records
    // (file times are too coarse to order two saves made in the same second).
    auto Written = [this](const FString& Path, ESlot Slot, FDateTime& Out)
    {
        const auto* Save = ReadFile(Path, Slot);
        if (!Save) return false;
        Out = Save->SavedAtUtc; return true;
    };
    FDateTime Chapter, Boundary, Map;
    const bool bChapter = Written(GetChapterSlotPath(), ESlot::Chapter, Chapter);
    const bool bBoundary = Written(GetSlotPath(), ESlot::Boundary, Boundary) || Written(GetSlotPath() + TEXT(".bak"), ESlot::Boundary, Boundary);
    const bool bMap = Written(GetMapSlotPath(), ESlot::Map, Map);
    auto Source = EMemoriaContinueSource::None; FDateTime Newest;
    auto Offer = [&](bool bValid, const FDateTime& When, EMemoriaContinueSource Kind)
    { if (bValid && (Source == EMemoriaContinueSource::None || When > Newest)) { Source = Kind; Newest = When; } };
    // Ties keep the earlier offer: the boundary, then the chapter autosave, then the map.
    Offer(bBoundary, Boundary, EMemoriaContinueSource::Boundary); Offer(bChapter, Chapter, EMemoriaContinueSource::Chapter); Offer(bMap, Map, EMemoriaContinueSource::Map);
    return Source;
}
FString UMemoriaCheckpointSubsystem::GetMapSlotPath() const
{
    return StorageRoot.IsEmpty() ? FString() : StorageRoot/TEXT("map.memoria.json");
}
bool UMemoriaCheckpointSubsystem::ValidateMapSnapshot(const UMemoriaRunSaveGame& Save) const
{
    FString Error;
    const auto* Catalog = LoadObject<UMemoriaMemoryCatalog>(nullptr, CatalogPath);
    const auto& Flow = Save.SceneFlow; const auto& P = Save.FieldReturn.SourcePixelPosition;
    const FString& Scene = Save.FieldReturn.SourceScenePath;
    // A map save resumes in the field of a ported chapter map: its scene, a place inside it, and no VN cursor.
    const auto* Spec = Scene.StartsWith(TEXT("res://scenes/maps/")) && Scene.EndsWith(TEXT(".tscn")) ? MemoriaChapterMaps::Find(FPaths::GetBaseFilename(Scene)) : nullptr;
    if (!Catalog || !Spec || !Save.ValidateHeader(Error) || Save.Slot!=0 ||
        Save.ContentRevision!=Catalog->ContentRevision || Save.MemoryCatalogId!=Catalog->GetPrimaryAssetId() ||
        Save.SavedAtUtc.GetTicks()<=0 || !Save.ImportedFromVersion.IsEmpty() || Save.FieldReturn.MapId!=MapBoundaryId ||
        !FMath::IsFinite(P.X) || !FMath::IsFinite(P.Y) || P.X<0 || P.X>Spec->Width*Spec->TileSize || P.Y<0 || P.Y>Spec->Height*Spec->TileSize ||
        Save.Run.CurrentChapter<Spec->Chapter ||
        Flow.bActive || !Flow.Current.SequenceId.IsEmpty() || !Flow.Pending.SequenceId.IsEmpty() || !Flow.ResumeQueue.IsEmpty() ||
        Save.WorldCognition.SchemaVersion!=1 || Save.WorldCognition.SourceJson.IsEmpty()) return false;
    return ValidateDomains(Save);
}
UMemoriaRunSaveGame* UMemoriaCheckpointSubsystem::CaptureChapterMap(const FString& Map, const FVector2D& Position) const
{
    auto* Save=Run->CaptureSave();
    const auto* Catalog=LoadObject<UMemoriaMemoryCatalog>(nullptr,CatalogPath);
    if (!Save || !Catalog) return nullptr;
    Save->SavedAtUtc=FDateTime::UtcNow(); Save->MemoryCatalogId=Catalog->GetPrimaryAssetId();
    Save->FieldReturn.SourceScenePath=FString::Printf(TEXT("res://scenes/maps/%s.tscn"),*Map); Save->FieldReturn.MapId=MapBoundaryId;
    Save->FieldReturn.SourcePixelPosition=Position;
    return ValidateMapSnapshot(*Save) ? Save : nullptr;
}
bool UMemoriaCheckpointSubsystem::CanSaveChapterMap(const FString& Map, const FVector2D& Position) const
{
    return !bBusy && CanAccess() && CaptureChapterMap(Map, Position) != nullptr;
}
bool UMemoriaCheckpointSubsystem::SaveChapterMap(const FString& Map, const FVector2D& Position)
{
    if (bBusy || !CanAccess()) return Fail(TEXT("Save skipped: disk storage is unavailable in this session."));
    TGuardValue<bool> Busy(bBusy,true);
    auto* Save=CaptureChapterMap(Map, Position);
    if (!Save) return Fail(TEXT("Save skipped: this run is outside the supported boundary."));
    TArray<uint8> Bytes;
    if (!UGameplayStatics::SaveGameToMemory(Save,Bytes)) return Fail(TEXT("Save skipped: serialization failed."));
    auto Object=MakeShared<FJsonObject>(); Object->SetNumberField(TEXT("version"),1);
    Object->SetStringField(TEXT("sha1"),Digest(Bytes)); Object->SetStringField(TEXT("payload"),FBase64::Encode(Bytes));
    FString Text; FJsonSerializer::Serialize(Object,TJsonWriterFactory<>::Create(&Text));
    if (Text.Len()>MaxFileBytes || !IFileManager::Get().MakeDirectory(*StorageRoot,true)) return Fail(TEXT("Save skipped: cannot create the save folder."));
    // The temp file is verified before the same-directory rename, so a failed write keeps the last good save.
    if (!CommitDiskFile(GetMapSlotPath(),Text,ESlot::Map)) return Fail(TEXT("Save failed: disk write failed."));
    StatusText=TEXT("Saved in the field."); return true;
}
FString UMemoriaCheckpointSubsystem::PeekChapterMap() const
{
    if (!IsStorageEnabled()) return FString();
    const auto* Save=ReadFile(GetMapSlotPath(),ESlot::Map);
    return Save ? FPaths::GetBaseFilename(Save->FieldReturn.SourceScenePath) : FString();
}
bool UMemoriaCheckpointSubsystem::RestoreChapterMap(FString& OutMap, FVector2D& OutPosition)
{
    if (bBusy || !CanAccess()) return Fail(TEXT("Save unavailable: disk storage is disabled or busy."));
    TGuardValue<bool> Busy(bBusy,true);
    auto* Save=ReadFile(GetMapSlotPath(),ESlot::Map);
    if (!Save) return Fail(TEXT("No valid field save found. The current run has not changed."));
    // Keep the decoded DTO alive across run-replaced observers.
    TStrongObjectPtr<UMemoriaRunSaveGame> RetainedSave(Save);
    if (!Run->RestoreSave(*Save)) return Fail(TEXT("The field save could not be restored. The current run has not changed."));
    OutMap=FPaths::GetBaseFilename(Save->FieldReturn.SourceScenePath); OutPosition=Save->FieldReturn.SourcePixelPosition;
    StatusText=TEXT("Field save loaded."); return true;
}
UMemoriaRunSaveGame* UMemoriaCheckpointSubsystem::ReadFile(const FString& Path, ESlot Slot) const
{
    const auto Size=IFileManager::Get().FileSize(*Path);
    if (Size<=0 || Size>MaxFileBytes) return nullptr;
    FString Text; TSharedPtr<FJsonObject> Object;
    if (!FFileHelper::LoadFileToString(Text,*Path) ||
        !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Object) || !Object) return nullptr;
    double Version=0; FString Payload, Hash;
    if (!Object->TryGetNumberField(TEXT("version"),Version) || Version!=1 ||
        !Object->TryGetStringField(TEXT("payload"),Payload) || !Object->TryGetStringField(TEXT("sha1"),Hash)) return nullptr;
    TArray<uint8> Bytes;
    if (!FBase64::Decode(Payload,Bytes) || Bytes.IsEmpty() || Digest(Bytes)!=Hash) return nullptr;
    auto* Save=Cast<UMemoriaRunSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
    return Save && (Slot==ESlot::Chapter ? ValidateChapterSnapshot(*Save) : Slot==ESlot::Map ? ValidateMapSnapshot(*Save) : ValidateSnapshot(*Save)) ? Save : nullptr;
}
bool UMemoriaCheckpointSubsystem::CommitDiskFile(const FString& Path, const FString& Text, ESlot Slot) const
{
    const FString Temp=Path+TEXT(".tmp");
    if (!FFileHelper::SaveStringToFile(Text,*Temp,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM) || !ReadFile(Temp,Slot)) return false;
    // Same-directory rename; retain the last good primary if writing/verification failed.
#if PLATFORM_WINDOWS
    return ::MoveFileExW(*Temp,*Path,MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)!=0;
#else
    // This slice currently targets Windows; fail without deleting an existing file.
    return false;
#endif
}
bool UMemoriaCheckpointSubsystem::SaveClosedBoundary(const FVector2D& Position)
{
    if (bBusy || !CanAccess()) return Fail(TEXT("Checkpoint not saved: disk storage is unavailable in this session."));
    TGuardValue<bool> Busy(bBusy,true);
    auto* Save=Run->CaptureSave();
    const auto* Catalog=LoadObject<UMemoriaMemoryCatalog>(nullptr,CatalogPath);
    if (!Save || !Catalog) return Fail(TEXT("Checkpoint not saved: no valid run."));
    Save->SavedAtUtc=FDateTime::UtcNow(); Save->MemoryCatalogId=Catalog->GetPrimaryAssetId();
    Save->FieldReturn.SourceScenePath=SourceScene; Save->FieldReturn.MapId=BoundaryId; Save->FieldReturn.SourcePixelPosition=Position;
    if (!ValidateSnapshot(*Save)) return Fail(TEXT("Checkpoint not saved: this run is outside the supported boundary."));
    TArray<uint8> Bytes;
    if (!UGameplayStatics::SaveGameToMemory(Save,Bytes)) return Fail(TEXT("Checkpoint not saved: serialization failed."));
    auto Object=MakeShared<FJsonObject>(); Object->SetNumberField(TEXT("version"),1);
    Object->SetStringField(TEXT("sha1"),Digest(Bytes)); Object->SetStringField(TEXT("payload"),FBase64::Encode(Bytes));
    FString Text; FJsonSerializer::Serialize(Object,TJsonWriterFactory<>::Create(&Text));
    if (Text.Len()>MaxFileBytes || !IFileManager::Get().MakeDirectory(*StorageRoot,true))
        return Fail(TEXT("Checkpoint not saved: cannot create the save folder."));
    const FString Path=GetSlotPath();
    // Never replace a good backup with a corrupt primary.
    if (ReadFile(Path))
    {
        FString Previous;
        if (!FFileHelper::LoadFileToString(Previous,*Path) || !CommitDiskFile(Path+TEXT(".bak"),Previous))
            return Fail(TEXT("Checkpoint not saved: backup failed. Previous checkpoint retained."));
    }
    if (!CommitDiskFile(Path,Text)) return Fail(TEXT("Checkpoint not saved: disk write failed. Retry saving."));
    StatusText=TEXT("Checkpoint saved to disk."); return true;
}
bool UMemoriaCheckpointSubsystem::CanSaveClosedBoundary() const
{
    if (bBusy || !CanAccess()) return false;
    auto* Save=Run->CaptureSave();
    const auto* Catalog=LoadObject<UMemoriaMemoryCatalog>(nullptr,CatalogPath);
    if (!Save || !Catalog) return false;
    Save->SavedAtUtc=FDateTime::UtcNow(); Save->MemoryCatalogId=Catalog->GetPrimaryAssetId();
    Save->FieldReturn.SourceScenePath=SourceScene; Save->FieldReturn.MapId=BoundaryId; Save->FieldReturn.SourcePixelPosition=FVector2D(500,340);
    return ValidateSnapshot(*Save);
}
bool UMemoriaCheckpointSubsystem::RestoreClosedBoundary(FVector2D& OutPosition)
{
    if (bBusy || !CanAccess()) return Fail(TEXT("Checkpoint unavailable: disk storage is disabled or busy."));
    TGuardValue<bool> Busy(bBusy,true);
    const FString Path=GetSlotPath(); auto* Save=ReadFile(Path);
    const bool Recovered=!Save;
    if (!Save) Save=ReadFile(Path+TEXT(".bak"));
    if (!Save) return Fail(TEXT("No valid checkpoint found. The current run has not changed."));
    // Keep the decoded DTO alive across run-replaced observers.
    TStrongObjectPtr<UMemoriaRunSaveGame> RetainedSave(Save);
    if (!Run->RestoreSave(*Save)) return Fail(TEXT("Checkpoint could not be restored. The current run has not changed."));
    OutPosition=Save->FieldReturn.SourcePixelPosition;
    StatusText=TEXT("Checkpoint loaded. Your exchange is complete.");
    if (Recovered)
    {
        FString Previous;
        const bool Repaired=FFileHelper::LoadFileToString(Previous,*(Path+TEXT(".bak"))) && CommitDiskFile(Path,Previous);
        StatusText=Repaired ? TEXT("Checkpoint recovered from backup. Primary save repaired.") :
            TEXT("Checkpoint loaded from backup. Primary repair failed; backup retained.");
    }
    return true;
}
