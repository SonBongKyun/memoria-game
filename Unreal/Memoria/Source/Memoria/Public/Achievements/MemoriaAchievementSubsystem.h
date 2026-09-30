#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MemoriaAchievementSubsystem.generated.h"
class UMemoriaRunSubsystem;

struct FMemoriaAchievement { const TCHAR* Id; const TCHAR* Title; const TCHAR* Desc; const TCHAR* Icon; };

// S324: achievement_manager.gd. The source's 38 achievements, titles and descriptions as authored (the
// source shows them in English in both locales), unlocked once and kept across runs in their own file
// (user://achievements.json -> Saved/Memoria/achievements.json; never in tests or commandlets), with the
// battles-won, items-used and maps-visited counters. Each unlock queues the "ACHIEVEMENT UNLOCKED" popup.
// The unlocks the port can reach: battles won in the field (first_blood, battle_veteran, survivor), burns
// (first_burn, pyromaniac, identity_crisis, zero_burn), chapters 1-5, Malet's trade (merchant), 100 Grains
// (wealthy) and map visits (explorer, which needs five). The rest wait for their content, item_master for
// items usable in field combat.
UCLASS()
class MEMORIA_API UMemoriaAchievementSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    static const TArray<FMemoriaAchievement>& All();
    static const FMemoriaAchievement* Find(const FString& Id);
    // unlock: false when already unlocked or unknown.
    bool Unlock(const FString& Id);
    bool IsUnlocked(const FString& Id) const { return Unlocked.Contains(Id); }
    int32 NumUnlocked() const { return Unlocked.Num(); }
    // _on_battle_ended (a won field fight, with Arrel's HP as it ended), record_* and check_grains.
    void RecordBattleWon(int64 HpAtEnd);
    void RecordChapterComplete(int32 Chapter);
    void RecordMapVisit(const FString& Map);
    void RecordItemUsed();
    // Polled with the run: its burns (_on_memory_burned), Grains, Malet's trade and closed chapters.
    void Observe(const UMemoriaRunSubsystem& Run);
    int32 GetBattlesWon() const { return BattlesWon; }
    int32 GetItemsUsed() const { return ItemsUsed; }
    const TArray<FString>& GetMapsVisited() const { return MapsVisited; }
    // The popup queue: one at a time, sliding in for .4 s, holding 4 s, leaving over .3 s.
    const FString& GetPopup() const { return Popup; }
    float GetPopupAge() const { return PopupAge; }
    void Advance(float DeltaSeconds);
    static constexpr float PopupIn = .4f;
    static constexpr float PopupHold = 4.f;
    static constexpr float PopupOut = .3f;
    // Tests only: forget everything (never touches the file).
    void ResetForTest();
private:
    TSet<FString> Unlocked;
    TArray<FString> Queue, MapsVisited;
    FString Popup;
    float PopupAge = 0.f;
    int32 BattlesWon = 0, ItemsUsed = 0;
    FGuid SeenRun;
    int32 SeenBurns = 0;
    bool bPersist = false;
    FString Path() const;
    void Save() const;
};
