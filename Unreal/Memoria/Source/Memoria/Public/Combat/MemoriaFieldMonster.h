#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Combat/MemoriaFieldCombatTypes.h"
#include "MemoriaFieldMonster.generated.h"
class UFloatingPawnMovement;
class UMemoriaFieldCharacterComponent;
class UMemoriaFieldCombatSubsystem;

// Game-space attack warning, shared by the rendered cue and contact-boundary tests.
// It previews the existing rules; it does not alter attack power or timing.
struct FMemoriaAttackCue
{
    bool bActive = false, bBlocked = false;
    EMemoriaFoeBehaviour Behaviour = EMemoriaFoeBehaviour::Brawler;
    FVector Origin = FVector::ZeroVector, Direction = FVector::ForwardVector;
    float Length = 0.f, Radius = 0.f, Progress = 0.f;
    bool bReach = false;
    FVector BlockPoint = FVector::ZeroVector, BlockNormal = FVector::ZeroVector;
};

// A field foe (S313: a void husk or a market thief, FoeSpec) on Epic's mannequin, chasing the player and
// striking after a telegraph. Set the kind before BeginPlay (SpawnWave spawns deferred).
UCLASS()
class MEMORIA_API AMemoriaFieldMonster : public APawn
{
    GENERATED_BODY()
public:
    AMemoriaFieldMonster();
    void SetKind(EMemoriaFoeKind InKind) { Kind = InKind; }
    EMemoriaFoeKind GetKind() const { return Kind; }
    const FMemoriaFoeSpec& Spec() const { return FoeSpec(Kind); }
    virtual void Tick(float DeltaSeconds) override;
    virtual UPawnMovementComponent* GetMovementComponent() const override;
    // Returns true when the hit landed (not dead). Staggers and cancels a windup.
    // S367: on a shelled elite only a shell-breaking blow does full harm and staggers; others do a third.
    bool TakeHit(float Damage, const FVector& From, float Shove = 14.f, bool bBreaksShell = false);
    float GetLastHitDamage() const { return LastHitDamage; }
    // S367: a void-touched elite (set before BeginPlay, as the kind).
    void SetElite(bool bInElite) { bElite = bInElite; }
    bool IsElite() const { return bElite; }
    bool IsShelled() const { return bShelled; }
    bool BreakShell(const FVector& From);
    // S314: the source burn status on a foe, ticking once a second.
    void Ignite(float TickDamage, int32 Ticks);
    bool IsBurning() const { return IgniteLeft > 0; }
    // S315: a parried foe reels for longer than a stagger.
    void Stun(float Seconds);
    // S342: a drain gives the foe back part of the harm it did.
    void Heal(float Amount) { if (!IsDead()) Health = FMath::Min(MaxHealth, Health + Amount); }
    const FVector& GetRushDirection() const { return RushDirection; }
    FMemoriaAttackCue GetAttackCue() const;
    float GetHitFlash() const { return HitFlash; }
    float GetStateTime() const { return StateTime; }   // S366: seconds in the current state (tests time a dodge by it)
    bool IsDead() const { return State == EMemoriaMonsterState::Dead; }
    EMemoriaMonsterState GetState() const { return State; }
    float GetHealth() const { return Health; }
    float GetMaxHealth() const { return MaxHealth; }
    UMemoriaFieldCharacterComponent* GetFigure() const { return Figure; }
    int32 GetStrikeCount() const { return Strikes; }
    // S376: the club's corridor. Ahead of the reach, and no wider than the short bar held at the waist.
    bool IsReachTouch(const FVector& Point) const;
protected:
    virtual void BeginPlay() override;
private:
    UPROPERTY(VisibleAnywhere) TObjectPtr<UFloatingPawnMovement> Movement;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UMemoriaFieldCharacterComponent> Figure;
    EMemoriaMonsterState State = EMemoriaMonsterState::Idle;
    EMemoriaFoeKind Kind = EMemoriaFoeKind::VoidHusk;
    float Health = MemoriaCombatTuning::HuskHealth, MaxHealth = MemoriaCombatTuning::HuskHealth, StateTime = 0.f;
    int32 Strikes = 0, IgniteLeft = 0;
    float IgniteDamage = 0.f, IgniteClock = 0.f, HitFlash = 0.f, StaggerFor = 0.f;
    FVector PreviousLocation = FVector::ZeroVector, RushDirection = FVector::ForwardVector;
    bool bRushHit = false;
    bool bElite = false, bShelled = false;
    float LastHitDamage = 0.f, AuraClock = 0.f;
    UPROPERTY(Transient) TObjectPtr<class UPointLightComponent> AuraLight;
    UPROPERTY(Transient) TObjectPtr<class UStaticMeshComponent> Club;
    UPROPERTY(Transient) TObjectPtr<class UStaticMeshComponent> ClubBand;
    void SetShellLook(bool bOn);
    void PlaceClub();
    void Enter(EMemoriaMonsterState Next);
    UMemoriaFieldCombatSubsystem* Combat() const;
};
