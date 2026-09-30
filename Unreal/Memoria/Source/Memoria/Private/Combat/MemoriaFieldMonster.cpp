#include "Combat/MemoriaFieldMonster.h"
#include "Combat/MemoriaFieldCombatSubsystem.h"
#include "Presentation/MemoriaFieldCharacterComponent.h"
#include "Presentation/MemoriaCombatClips.h"
#include "Animation/AnimSequence.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "Materials/MaterialInstanceDynamic.h"
using namespace MemoriaCombatTuning;
AMemoriaFieldMonster::AMemoriaFieldMonster()
{
    PrimaryActorTick.bCanEverTick = true;
    auto* Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Collision"));
    Box->SetBoxExtent(FVector(30, 30, 8)); Box->SetCollisionProfileName(TEXT("Pawn"));
    SetRootComponent(Box);
    Movement = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("Movement"));
    Movement->SetUpdatedComponent(Box); Movement->SetPlaneConstraintNormal(FVector::UpVector); Movement->SetPlaneConstraintEnabled(true);
    Movement->MaxSpeed = HuskSpeed; Movement->Acceleration = 900.f; Movement->Deceleration = 1400.f; // the kind's speed is set in BeginPlay
    Figure = CreateDefaultSubobject<UMemoriaFieldCharacterComponent>(TEXT("Figure"));
    Figure->SetupAttachment(Box);
    // The telegraph: a thin red disc under the husk while it winds up, the reach of its strike.
    Telegraph = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Telegraph"));
    Telegraph->SetupAttachment(Box); Telegraph->SetCollisionEnabled(ECollisionEnabled::NoCollision); Telegraph->SetCastShadow(false);
    Telegraph->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));
    Telegraph->SetRelativeLocation(FVector(0, 0, -8.5)); Telegraph->SetRelativeScale3D(FVector(HuskReach / 50.f, HuskReach / 50.f, .004f));
    Telegraph->SetHiddenInGame(true);
    AutoPossessAI = EAutoPossessAI::Disabled;
}
UPawnMovementComponent* AMemoriaFieldMonster::GetMovementComponent() const { return Movement; }
UMemoriaFieldCombatSubsystem* AMemoriaFieldMonster::Combat() const { return GetWorld() ? GetWorld()->GetSubsystem<UMemoriaFieldCombatSubsystem>() : nullptr; }
void AMemoriaFieldMonster::BeginPlay()
{
    Super::BeginPlay();
    const FMemoriaFoeSpec& S = Spec();
    FMemoriaFoeLook Look;
    Look.Model = S.Model; Look.Height = S.Height; Look.bQuinn = S.bQuinn; Look.Idle = S.Idle; Look.Walk = S.Walk;
    Look.Color = S.Color; Look.Glow = S.Glow; Look.Crack = S.Crack; Look.Rim = S.Rim; Look.BladeScale = S.BladeScale;
    Figure->InitializeFoe(Look);
    Health = MaxHealth = S.Health; Movement->MaxSpeed = S.Speed;
    Telegraph->SetRelativeScale3D(FVector(S.Reach / 50.f, S.Reach / 50.f, .004f));
    if (auto* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
    {
        TelegraphMaterial = UMaterialInstanceDynamic::Create(Base, this);
        TelegraphMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(.45f, .02f, .03f));
        Telegraph->SetMaterial(0, TelegraphMaterial);
    }
    PreviousLocation = GetActorLocation();
}
void AMemoriaFieldMonster::Enter(EMemoriaMonsterState Next)
{
    State = Next; StateTime = 0.f;
    Telegraph->SetHiddenInGame(Next != EMemoriaMonsterState::Windup);
    // The kind's strike, timed so its blow lands as the windup ends.
    if (Next == EMemoriaMonsterState::Windup)
        if (const UAnimSequence* Clip = MemoriaCombatClips::Load(Figure->GetCharacterId(), Spec().Strike))
            Figure->PlayAction(Spec().Strike, FMath::Max(.1f, Clip->GetPlayLength() * Spec().StrikeAt / Spec().Windup));
}
bool AMemoriaFieldMonster::TakeHit(float Damage, const FVector& From, float Shove)
{
    if (IsDead()) return false;
    Health = FMath::Max(0.f, Health - Damage);
    if (Damage > 0.f) { HitFlash = 1.f; Figure->SetHitFlash(1.f); }
    const FVector Away = (GetActorLocation() - From).GetSafeNormal2D();
    if (Health <= 0.f)
    {
        Enter(EMemoriaMonsterState::Dead);
        Figure->SetAim((-Away).Rotation().Yaw);
        Figure->PlayAction(MemoriaCombatClips::Death(), 1.f, true);
        SetActorEnableCollision(false);
        SetActorLocation(GetActorLocation() + Away * Shove * .5f, false);
        if (auto* C = Combat()) C->NotifyMonsterDied(this);
        return true;
    }
    Enter(EMemoriaMonsterState::Stagger); StaggerFor = Spec().Stagger;
    Figure->PlayAction(MemoriaCombatClips::Hit(), 1.2f);
    // A shove away from the blow: short for a sword, far for a burn.
    SetActorLocation(GetActorLocation() + Away * Shove, true);
    return true;
}
void AMemoriaFieldMonster::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    StateTime += DeltaSeconds;
    const FVector Step = GetActorLocation() - PreviousLocation; PreviousLocation = GetActorLocation();
    if (HitFlash > 0.f) { HitFlash = FMath::Max(0.f, HitFlash - DeltaSeconds * 6.f); Figure->SetHitFlash(HitFlash); }
    Figure->AdvanceLocomotion(Step, DeltaSeconds);
    if (IsDead()) { if (StateTime > HuskCorpseTime) Destroy(); return; }
    if (IgniteLeft > 0 && (IgniteClock -= DeltaSeconds) <= 0.f)
    {
        IgniteClock = IgniteInterval; --IgniteLeft;
        Health = FMath::Max(0.f, Health - IgniteDamage);
        if (auto* C = Combat()) C->NotifyBurnTick(this, IgniteDamage);
        if (Health <= 0.f) { TakeHit(0.f, GetActorLocation()); return; }
    }
    auto* C = Combat(); APawn* Target = C ? C->GetPlayer() : nullptr;
    if (!Target || (C && C->IsDefeated())) { if (State != EMemoriaMonsterState::Idle) Enter(EMemoriaMonsterState::Idle); return; }
    const FVector ToTarget = (Target->GetActorLocation() - GetActorLocation()) * FVector(1, 1, 0);
    const float Distance = ToTarget.Size();
    auto FaceTarget = [&] { if (Distance > 1.f) Figure->SetAim(ToTarget.Rotation().Yaw); };
    switch (State)
    {
    case EMemoriaMonsterState::Idle:
        if (Distance < Spec().Aggro) Enter(EMemoriaMonsterState::Chase);
        break;
    case EMemoriaMonsterState::Chase:
        FaceTarget();
        if (Distance <= Spec().Reach * .85f) Enter(EMemoriaMonsterState::Windup);
        // Moved directly: pawn movement only applies input under a local controller, and husks have none.
        else SetActorLocation(GetActorLocation() + ToTarget.GetSafeNormal() * Spec().Speed * DeltaSeconds, true);
        break;
    case EMemoriaMonsterState::Windup:
        FaceTarget();
        if (TelegraphMaterial) TelegraphMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(.45f + .5f * StateTime / Spec().Windup, .02f, .03f));
        if (StateTime >= Spec().Windup)
        {
            ++Strikes;
            if (C) C->StrikePlayer(this, Spec().Damage);
            // A parry has already sent the foe reeling (Stun); keep that instead of the recovery.
            if (State == EMemoriaMonsterState::Windup) Enter(EMemoriaMonsterState::Recover);
        }
        break;
    case EMemoriaMonsterState::Recover:
        if (StateTime >= Spec().Recover) Enter(EMemoriaMonsterState::Chase);
        break;
    case EMemoriaMonsterState::Stagger:
        if (StateTime >= StaggerFor) Enter(EMemoriaMonsterState::Chase);
        break;
    default: break;
    }
}
void AMemoriaFieldMonster::Ignite(float TickDamage, int32 Ticks)
{
    if (IsDead()) return;
    IgniteDamage = FMath::Max(IgniteDamage * (IgniteLeft > 0), TickDamage); IgniteLeft = FMath::Max(IgniteLeft, Ticks); IgniteClock = IgniteInterval;
}
void AMemoriaFieldMonster::Stun(float Seconds)
{
    if (IsDead()) return;
    Enter(EMemoriaMonsterState::Stagger); StaggerFor = Seconds;
    HitFlash = 1.f; Figure->SetHitFlash(1.f);
    Figure->PlayAction(MemoriaCombatClips::Hit(), .8f);
}
