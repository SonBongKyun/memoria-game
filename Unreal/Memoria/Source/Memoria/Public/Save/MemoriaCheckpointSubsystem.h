#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MemoriaCheckpointSubsystem.generated.h"

class UMemoriaRunSaveGame;
class UMemoriaRunSubsystem;

// Bounded native disk adapter. Never reads/writes Godot slots or resumes Chapter 3.
UCLASS()
class MEMORIA_API UMemoriaCheckpointSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    bool SaveClosedBoundary(const FVector2D& Position);
    bool RestoreClosedBoundary(FVector2D& OutPosition);
    bool ValidateSnapshot(const UMemoriaRunSaveGame& Save) const;
    // SaveManager.autosave_on_chapter_transition: a separate slot holding an active VN cursor.
    bool SaveChapterTransition(UMemoriaRunSaveGame& Save);
    UMemoriaRunSaveGame* LoadChapterTransition();
    bool ValidateChapterSnapshot(const UMemoriaRunSaveGame& Save) const;
    FString GetChapterSlotPath() const;
    const FString& GetStatusText() const { return StatusText; }
    FString GetSlotPath() const;
    bool IsStorageEnabled() const { return !StorageRoot.IsEmpty(); }
#if WITH_DEV_AUTOMATION_TESTS
    // Only an isolated leaf beneath Saved/Validation/CheckpointTests is accepted.
    bool ConfigureTestStorage(const FString& Leaf);
#endif
    static constexpr const TCHAR* BoundaryId = TEXT("verdan_shop_closed_v1");
    static constexpr const TCHAR* SourceScene = TEXT("res://scenes/maps/verdan_market.tscn");
    static constexpr const TCHAR* ChapterBoundaryId = TEXT("chapter_transition_v1");
    static constexpr const TCHAR* ChapterSourceScene = TEXT("res://scenes/main/vn_host.tscn");
private:
    UPROPERTY(Transient) TObjectPtr<UMemoriaRunSubsystem> Run;
    FString StorageRoot, StatusText;
    bool bBusy = false;
    bool CanAccess() const;
    bool Fail(const TCHAR* Message);
    UMemoriaRunSaveGame* ReadFile(const FString& Path, bool bChapter = false) const;
    bool CommitDiskFile(const FString& Path, const FString& Text, bool bChapter = false) const;
    bool ValidateDomains(const UMemoriaRunSaveGame& Save) const;
};
