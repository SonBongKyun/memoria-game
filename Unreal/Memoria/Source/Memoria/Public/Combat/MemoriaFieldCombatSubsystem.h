#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MemoriaFieldCombatSubsystem.generated.h"
class APawn;
class AMemoriaFieldMonster;
class UMemoriaFieldCharacterComponent;

struct FMemoriaCombatPopup { FVector Location; float Amount = 0.f; float Age = 0.f; bool bPlayer = false; };
// S312: a memory that can be burned, as the picker lists it (localized title, source grade and skill).
struct FMemoriaBurnChoice { FString Id, Title, GradeLabel; int32 Grade = 0; int64 Power = 0; float Damage = 0.f; FLinearColor Accent = FLinearColor::White; };
// The released burn: its ring on the floor, and what burned, for the HUD.
struct FMemoriaBurnWave { FVector Center = FVector::ZeroVector; float Radius = 0.f, Age = 0.f; int32 Grade = 0; FString Title, Skill; bool bLive = false; };
class APointLight;

// S311: field action combat. Owns the player's combo, dash and stagger, the live monsters, damage and
// defeat recovery. Player health is the run's HP, so the archive, saves and VN see the same number.
UCLASS()
class MEMORIA_API UMemoriaFieldCombatSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual void Tick(float DeltaSeconds) override;
    virtual void Deinitialize() override;
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UMemoriaFieldCombatSubsystem, STATGROUP_Tickables); }
    virtual bool IsTickable() const override { return Player.IsValid(); }
    // The Verdan presentation registers Arrel's pawn and figure when the field is built.
    void RegisterPlayer(APawn* Pawn, UMemoriaFieldCharacterComponent* Figure);
    bool RequestAttack(const FVector& AimPoint);
    bool RequestDash(const FVector& Direction);
    // Movement input is ignored while an attack, dash, stagger or defeat plays.
    bool CanMove() const { return !IsAttacking() && DashLeft <= 0.f && StaggerLeft <= 0.f && !bDefeated && !bPicking && CastLeft <= 0.f; }
    bool IsAttacking() const { return ComboStep >= 0; }
    bool IsDashing() const { return DashLeft > 0.f; }
    bool IsInvulnerable() const { return DashLeft > 0.f || bDefeated || CastLeft > 0.f; }
    // S312 memory burn. R opens the picker and the world slows; a choice is confirmed (twice from grade 2
    // up) and the memory is burned for good through the run, then Arrel releases the grade's skill: a ring
    // of fire (void for the top grades) that strikes every husk within its reach.
    bool OpenBurnPicker();
    void CloseBurnPicker();
    bool IsPickingBurn() const { return bPicking; }
    const TArray<FMemoriaBurnChoice>& GetBurnChoices() const { return Choices; }
    int32 GetBurnSelection() const { return Selection; }
    void SelectBurn(int32 Index);
    void MoveBurnSelection(int32 Delta) { if (Choices.Num()) SelectBurn((Selection + Delta + Choices.Num()) % Choices.Num()); }
    // True when the burn went through; false while it only armed the second ask, or when nothing burned.
    bool ConfirmBurn();
    bool IsBurnArmed() const { return bArmed; }
    bool IsCasting() const { return CastLeft > 0.f; }
    const FMemoriaBurnWave& GetBurnWave() const { return Wave; }
    int32 GetBurns() const { return Burns; }
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
    TArray<FMemoriaBurnChoice> Choices;
    FMemoriaBurnChoice Casting;
    FMemoriaBurnWave Wave;
    TSet<TWeakObjectPtr<AMemoriaFieldMonster>> Burned;
    TWeakObjectPtr<APointLight> Flare;
    int32 Selection = 0, Burns = 0;
    float CastLeft = 0.f, SheatheIn = 0.f;
    bool bPicking = false, bArmed = false;
    static bool bFieldEncountersInTests;
    void TickBurn(float DeltaSeconds);
    void DrawSword();
    void ReleaseBurn();
    bool StartStep(int32 Step);
    void ResolveSwing();
    void Popup(const FVector& Location, float Amount, bool bPlayer);
};
