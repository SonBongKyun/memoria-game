#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MemoriaCheckpointSubsystem.generated.h"

class UMemoriaRunSaveGame;
// Which save the title's Continue resumes: the chapter autosave (VN host), the Verdan checkpoint, or a
// chapter map's field (S330).
enum class EMemoriaContinueSource : uint8 { None, Chapter, Boundary, Map };
class UMemoriaRunSubsystem;

// Bounded native disk adapter. Never reads/writes Godot slots or resumes Chapter 3.
UCLASS()
class MEMORIA_API UMemoriaCheckpointSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    bool SaveClosedBoundary(const FVector2D& Position);
    // S317: whether SaveClosedBoundary would be accepted now (the ported save point is the closed Chapter 2
    // boundary: chapter 3, ch2_complete, no scene flow). Validates a capture; writes nothing.
    bool CanSaveClosedBoundary() const;
    bool RestoreClosedBoundary(FVector2D& OutPosition);
    bool ValidateSnapshot(const UMemoriaRunSaveGame& Save) const;
    // SaveManager.autosave_on_chapter_transition: a separate slot holding an active VN cursor.
    bool SaveChapterTransition(UMemoriaRunSaveGame& Save);
    UMemoriaRunSaveGame* LoadChapterTransition();
    bool ValidateChapterSnapshot(const UMemoriaRunSaveGame& Save) const;
    FString GetChapterSlotPath() const;
    // S330: SaveManager.save_game in a ported chapter map (belt_waystation, drift_shelter): the run, the map's
    // scene and Arrel's place in it. Continue opens that map, whose _ready_sequence takes the story up again
    // from the run's flags. A third slot, map.memoria.json.
    bool SaveChapterMap(const FString& Map, const FVector2D& Position);
    bool CanSaveChapterMap(const FString& Map, const FVector2D& Position) const;
    bool RestoreChapterMap(FString& OutMap, FVector2D& OutPosition);
    // The map a valid map slot names, without touching the run; empty when there is none.
    FString PeekChapterMap() const;
    bool ValidateMapSnapshot(const UMemoriaRunSaveGame& Save) const;
    FString GetMapSlotPath() const;
    // SaveManager.has_save / load_game for the title: the most recently written valid slot.
    EMemoriaContinueSource FindContinue() const;
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
    static constexpr const TCHAR* MapBoundaryId = TEXT("chapter_map_v1");
private:
    enum class ESlot : uint8 { Boundary, Chapter, Map };
    UPROPERTY(Transient) TObjectPtr<UMemoriaRunSubsystem> Run;
    FString StorageRoot, StatusText;
    bool bBusy = false;
    bool CanAccess() const;
    bool Fail(const TCHAR* Message);
    UMemoriaRunSaveGame* ReadFile(const FString& Path, ESlot Slot = ESlot::Boundary) const;
    bool CommitDiskFile(const FString& Path, const FString& Text, ESlot Slot = ESlot::Boundary) const;
    UMemoriaRunSaveGame* CaptureChapterMap(const FString& Map, const FVector2D& Position) const;
    bool ValidateDomains(const UMemoriaRunSaveGame& Save) const;
};
