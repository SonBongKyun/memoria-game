#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MemoriaCodexSubsystem.generated.h"
class UMemoriaRunSubsystem;

struct FMemoriaCodexEnemy { FString Name; int32 Encounters = 0, Defeated = 0; bool bVoid = false, bBoss = false; int32 MaxHp = 0, Atk = 0; };
struct FMemoriaCodexMemory { FString Id, Title, Desc; int32 Grade = 0; bool bBurned = false; };   // Grade: raw, 0 = Grade 5 (sensory) .. 4 = Grade 1 (core)

// S325: codex.gd, the codex (도감): the Bestiary of foes met and the Memory Archive of memories carried and
// spent, kept across runs in their own file (user://codex.json -> Saved/Memoria/codex.json; never in tests
// or commandlets, the source's suppress_recording). A field wave is an encounter with its kind; each foe
// that falls is a defeat. Memories are recorded as the run holds them, with the title and description in
// the run's language at that moment, and marked when burned. The source roster (ENEMY_NAMES_KO) lists the
// foes still unmet; scans (Tobias' Analyze) and the enemy picture preview are not carried.
UCLASS()
class MEMORIA_API UMemoriaCodexSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    // GameManager.ENEMY_NAMES_KO: the source roster, English -> Korean.
    static const TArray<TPair<FString, FString>>& Roster();
    static FString EnemyNameKo(const FString& Name);
    // _get_star_rating by raw grade (Grade 5 = one star .. Grade 1 = five) and _get_defeat_badge (10 / 25 / 50).
    static FString Stars(int32 RawGrade);
    static FString DefeatBadge(int32 Defeated);
    void RecordEncounter(const FString& Name, bool bVoid, bool bBoss, int32 MaxHp, int32 Atk);
    void RecordDefeat(const FString& Name);
    // _on_memory_added / _on_memory_burned, from the run's archive.
    void Observe(const UMemoriaRunSubsystem& Run);
    const TArray<FMemoriaCodexEnemy>& GetEnemies() const { return Enemies; }
    const TArray<FMemoriaCodexMemory>& GetMemories() const { return Memories; }
    const FMemoriaCodexEnemy* FindEnemy(const FString& Name) const;
    // The roster names not recorded yet, and the bestiary's denominator (roster plus anything recorded).
    TArray<FString> Unmet() const;
    int32 KnownTotal() const;
private:
    TArray<FMemoriaCodexEnemy> Enemies;
    TArray<FMemoriaCodexMemory> Memories;
    FGuid SeenRun;
    int32 SeenOwned = -1, SeenBurned = -1;
    bool bPersist = false;
    FString Path() const;
    void Save() const;
};
