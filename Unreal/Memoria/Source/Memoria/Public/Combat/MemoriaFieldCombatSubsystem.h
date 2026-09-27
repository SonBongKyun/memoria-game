#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MemoriaFieldCombatSubsystem.generated.h"
class APawn;
class AMemoriaFieldMonster;
class UMemoriaFieldCharacterComponent;

struct FMemoriaCombatPopup { FVector Location; float Amount = 0.f; float Age = 0.f; bool bPlayer = false; };

// S311: field action combat. Owns the player's combo, dash and stagger, the live monsters, damage and
// defeat recovery. Player health is the run's HP, so the archive, saves and VN see the same number.
UCLASS()
class MEMORIA_API UMemoriaFieldCombatSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual void Tick(float DeltaSeconds) override;
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UMemoriaFieldCombatSubsystem, STATGROUP_Tickables); }
    virtual bool IsTickable() const override { return Player.IsValid(); }
    // The Verdan presentation registers Arrel's pawn and figure when the field is built.
    void RegisterPlayer(APawn* Pawn, UMemoriaFieldCharacterComponent* Figure);
    bool RequestAttack(const FVector& AimPoint);
    bool RequestDash(const FVector& Direction);
    // Movement input is ignored while an attack, dash, stagger or defeat plays.
    bool CanMove() const { return !IsAttacking() && DashLeft <= 0.f && StaggerLeft <= 0.f && !bDefeated; }
    bool IsAttacking() const { return ComboStep >= 0; }
    bool IsDashing() const { return DashLeft > 0.f; }
    bool IsInvulnerable() const { return DashLeft > 0.f || bDefeated; }
    int32 GetComboStep() const { return ComboStep; }
    // Spawns void husks in a ring around a point on the floor.
    TArray<AMemoriaFieldMonster*> SpawnWave(int32 Count, const FVector& Center, float Radius = 420.f);
    const TArray<TWeakObjectPtr<AMemoriaFieldMonster>>& GetMonsters() const { return Monsters; }
    int32 LiveMonsterCount() const;
    int32 GetKills() const { return Kills; }
    int32 GetHitsLanded() const { return HitsLanded; }
    // Called by a husk at the end of its windup; false when the player dodged, left the reach, or is down.
    bool StrikePlayer(AMemoriaFieldMonster* Monster, float Damage);
    APawn* GetPlayer() const { return Player.Get(); }
    UMemoriaFieldCharacterComponent* GetPlayerFigure() const { return PlayerFigure.Get(); }
    int64 GetPlayerHp() const;
    int64 GetPlayerMaxHp() const;
    bool IsDefeated() const { return bDefeated; }
    const TArray<FMemoriaCombatPopup>& GetPopups() const { return Popups; }
    // Field encounters replace the turn-based battle in play. Automation keeps the legacy battle unless a
    // test opts in, so the stopgap turn-based suites keep their encounter route until they retire.
    static bool UseFieldEncounters();
    static void SetFieldEncountersForTests(bool bEnabled) { bFieldEncountersInTests = bEnabled; }
    void NotifyMonsterDied(AMemoriaFieldMonster* Monster);
private:
    TWeakObjectPtr<APawn> Player;
    TWeakObjectPtr<UMemoriaFieldCharacterComponent> PlayerFigure;
    TArray<TWeakObjectPtr<AMemoriaFieldMonster>> Monsters;
    TArray<FMemoriaCombatPopup> Popups;
    TSet<TWeakObjectPtr<AMemoriaFieldMonster>> HitThisSwing;
    FVector DashDirection = FVector::ZeroVector, AimDirection = FVector::ForwardVector;
    int32 ComboStep = -1, LastStep = -1, Kills = 0, HitsLanded = 0;
    float StepTime = 0.f, StepLength = 0.f, SinceStep = 99.f, DashLeft = 0.f, DashCooldownLeft = 0.f, StaggerLeft = 0.f, DefeatLeft = 0.f;
    bool bQueued = false, bDefeated = false;
    static bool bFieldEncountersInTests;
    bool StartStep(int32 Step);
    void ResolveSwing();
    void Popup(const FVector& Location, float Amount, bool bPlayer);
};
