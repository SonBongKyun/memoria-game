#include "Combat/MemoriaFieldMonster.h"
#include "Combat/MemoriaFieldCombatSubsystem.h"
#include "CollisionQueryParams.h"
#include "Presentation/MemoriaFieldCharacterComponent.h"
#include "Presentation/MemoriaCombatClips.h"
#include "Animation/AnimSequence.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
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
    Look.bRetint = S.bRetint; Look.ModelGlow = S.ModelGlow; Look.ModelCrack = S.ModelCrack; Look.ModelRim = S.ModelRim; Look.ModelRimStrength = S.ModelRimStrength;
    Look.SurfaceTint = S.SurfaceTint; Look.Weathering = S.Weathering;
    if (bElite) Look.Height = S.Height * EliteScale;   // S367: the elite stands taller
    Figure->InitializeFoe(Look);
    Health = MaxHealth = S.Health * (bElite ? EliteHealth : 1.f); Movement->MaxSpeed = S.Speed;
    if (bElite)
    {
        // A cold violet light breathes about it, and the void shell glows over its body.
        AuraLight = NewObject<UPointLightComponent>(this, NAME_None, RF_Transient);
        AuraLight->SetupAttachment(GetRootComponent());
        AuraLight->SetRelativeLocation(FVector(0, 0, S.Height * EliteScale * .55f));
        AuraLight->SetMobility(EComponentMobility::Movable); AuraLight->SetCastShadows(false);
        AuraLight->SetIntensityUnits(ELightUnits::Candelas); AuraLight->SetIntensity(14.f);
        AuraLight->SetLightColor(FLinearColor(.62f, .38f, 1.f)); AuraLight->SetAttenuationRadius(360.f);
        AuraLight->RegisterComponent();
        bShelled = true; SetShellLook(true);
    }
    PreviousLocation = GetActorLocation();
    if (S.bReach) DressReach();
}
void AMemoriaFieldMonster::DressReach()
{
    // The market thief keeps his pale coat and round shield. Only the warden loses the shield and wears the coat dark.
    USkeletalMeshComponent* Body = Figure ? Figure->GetSkeletalMesh() : nullptr;
    if (UStaticMeshComponent* Dagger = Figure ? Figure->GetBlade() : nullptr) { Dagger->SetHiddenInGame(true); Dagger->SetVisibility(false); }
    if (Body && Body->GetSkeletalMeshAsset())
    {
        const TArray<FSkeletalMaterial>& Slots = Body->GetSkeletalMeshAsset()->GetMaterials();
        const int32 Lods = FMath::Max(1, Body->GetNumLODs());
        // The authored coat only blends a third of the way toward SurfaceTint, so the cloth stays pale.
        // The body slot wears a flat dark cloth. The face keeps its own material, and the shield stays hidden.
        auto* Cloth = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Memoria/Presentation/Combat/M_FieldFoe.M_FieldFoe"));
        for (int32 I = 0; I < Slots.Num(); ++I)
        {
            const FString Name = Slots[I].MaterialSlotName.ToString() + Slots[I].ImportedMaterialSlotName.ToString() + GetNameSafe(Slots[I].MaterialInterface);
            if (Name.Contains(TEXT("Gear")))
                for (int32 Lod = 0; Lod < Lods; ++Lod) Body->ShowMaterialSection(I, INDEX_NONE, false, Lod);
            else if (Name.Contains(TEXT("Face")))
            {
                if (auto* Mid = Cast<UMaterialInstanceDynamic>(Body->GetMaterial(I)))
                    Mid->SetVectorParameterValue(TEXT("SurfaceTint"), FLinearColor::White);
            }
            else if (Cloth && Name.Contains(TEXT("Body")))
            {
                auto* Ink = UMaterialInstanceDynamic::Create(Cloth, Body);
                Ink->SetVectorParameterValue(TEXT("Color"), FLinearColor(.042f, .04f, .038f));
                Ink->SetVectorParameterValue(TEXT("SurfaceTint"), Spec().SurfaceTint);
                Ink->SetVectorParameterValue(TEXT("Glow"), FLinearColor(.02f, .02f, .022f));
                Ink->SetScalarParameterValue(TEXT("CrackStrength"), 0.f);
                Ink->SetScalarParameterValue(TEXT("RimStrength"), 0.f);
                Body->SetMaterial(I, Ink);
            }
        }
    }
    // The dagger is gone. A short black club sits in the striking hand, banded pale at the grip.
    USkeletalMeshComponent* Hand = Body;
    const bool bHand = Hand && Hand->GetSkeletalMeshAsset() && Hand->GetSkeletalMeshAsset()->GetRefSkeleton().FindBoneIndex(TEXT("hand_r")) != INDEX_NONE;
    auto* Mesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    auto* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    auto MakeBar = [&](const TCHAR* Name, const FVector& Scale, const FLinearColor& InkColor)
    {
        auto* Bar = NewObject<UStaticMeshComponent>(this, Name, RF_Transient);
        Bar->SetStaticMesh(Mesh);
        if (bHand) Bar->SetupAttachment(Hand, TEXT("hand_r"));
        else Bar->SetupAttachment(Figure);
        Bar->SetMobility(EComponentMobility::Movable); Bar->SetCollisionEnabled(ECollisionEnabled::NoCollision); Bar->SetCastShadow(false);
        // The idle hand rises. The club stays where PlaceClub puts it: low, and level.
        Bar->SetUsingAbsoluteLocation(true); Bar->SetUsingAbsoluteRotation(true);
        Bar->SetRelativeScale3D(Scale);
        if (Base)
        {
            auto* Ink = UMaterialInstanceDynamic::Create(Base, this);
            Ink->SetVectorParameterValue(TEXT("Color"), InkColor); Ink->SetVectorParameterValue(TEXT("BaseColor"), InkColor);
            Bar->SetMaterial(0, Ink);
        }
        Bar->RegisterComponent();
        return Bar;
    };
    if (Mesh)
    {
        // 46 cm long, a short baton rather than a crate. The pale band is only the grip.
        Club = MakeBar(TEXT("Club"), FVector(.46f, .055f, .055f), FLinearColor(.015f, .015f, .016f));
        ClubBand = MakeBar(TEXT("ClubBand"), FVector(.028f, .095f, .095f), FLinearColor(.62f, .58f, .50f));
        PlaceClub();
    }
}
void AMemoriaFieldMonster::Enter(EMemoriaMonsterState Next)
{
    State = Next; StateTime = 0.f;
    // The kind's strike, timed so its blow lands as the windup ends.
    if (Next == EMemoriaMonsterState::Windup && Spec().Strike)
        if (const UAnimSequence* Clip = MemoriaCombatClips::Load(Figure->GetCharacterId(), Spec().Strike))
            Figure->PlayAction(Spec().Strike, FMath::Max(.1f, Clip->GetPlayLength() * Spec().StrikeAt / Spec().Windup));
}
FMemoriaAttackCue AMemoriaFieldMonster::GetAttackCue() const
{
    FMemoriaAttackCue Cue;
    const auto* C = Combat();
    const APawn* Target = C ? C->GetPlayer() : nullptr;
    if (State != EMemoriaMonsterState::Windup || !Target || C->IsDefeated()) return Cue;
    Cue.bActive = true;
    Cue.Behaviour = Spec().Behaviour;
    Cue.Origin = GetActorLocation() - FVector(0, 0, 8.5f);
    Cue.Progress = FMath::Clamp(StateTime / FMath::Max(Spec().Windup, SMALL_NUMBER), 0.f, 1.f);
    if (Cue.Behaviour == EMemoriaFoeBehaviour::Charger)
    {
        // The charger commits when it enters Windup, unlike the caster's live aim.
        Cue.Direction = RushDirection;
        Cue.Length = RushDistance;
        Cue.Radius = RushHit;
        // Preview the same blocking box used by the rush, with pawn bodies ignored.
        FCollisionQueryParams Query(SCENE_QUERY_STAT(MemoriaRushPreview), false, this);
        for (TActorIterator<APawn> It(GetWorld()); It; ++It) Query.AddIgnoredActor(*It);
        const auto* Collision = CastChecked<UPrimitiveComponent>(GetRootComponent());
        FHitResult Hit;
        if (GetWorld()->SweepSingleByChannel(Hit, GetActorLocation(),
            GetActorLocation() + RushDirection * RushDistance, GetActorQuat(),
            Collision->GetCollisionObjectType(), Collision->GetCollisionShape(), Query))
        {
            Cue.bBlocked = true;
            Cue.Length = FMath::Min(Cue.Length, FVector::Dist2D(GetActorLocation(), Hit.Location));
            Cue.BlockPoint = Hit.ImpactPoint;
            Cue.BlockNormal = Hit.ImpactNormal.GetSafeNormal2D();
        }
    }
    else if (Cue.Behaviour == EMemoriaFoeBehaviour::Caster)
    {
        Cue.Direction = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
        if (Cue.Direction.IsNearlyZero()) Cue.Direction = FVector::ForwardVector;
        Cue.Length = 220.f; // a direction guide, not a promise of the orb's full travel
        Cue.Radius = OrbRadius;
    }
    else if (Spec().bReach)
    {
        // A short stroke held forward, the width of the club, not the orange disc of a swung blow.
        Cue.bReach = true;
        Cue.Direction = FRotator(0.f, Figure->GetYaw(), 0.f).Vector();
        if (Cue.Direction.IsNearlyZero()) Cue.Direction = FVector::ForwardVector;
        Cue.Length = Spec().Reach + 40.f;
        Cue.Radius = WardenReachWidth;
    }
    else Cue.Radius = Spec().Reach + 40.f; // StrikePlayer includes Arrel's contact allowance
    return Cue;
}
bool AMemoriaFieldMonster::IsReachTouch(const FVector& Point) const
{
    if (!Spec().bReach || !Figure) return false;
    const FVector Flat = (Point - GetActorLocation()) * FVector(1, 1, 0);
    const FVector Forward = FRotator(0.f, Figure->GetYaw(), 0.f).Vector();
    const float Along = FVector::DotProduct(Flat, Forward);
    const float Side = FVector::DotProduct(Flat, FVector(-Forward.Y, Forward.X, 0.f));
    return Along > 0.f && Along <= Spec().Reach + 40.f && FMath::Abs(Side) <= WardenReachWidth;
}
void AMemoriaFieldMonster::PlaceClub()
{
    if (!Club || !Figure) return;
    float Extend = 0.f;
    if (State == EMemoriaMonsterState::Windup) Extend = FMath::Clamp(StateTime / FMath::Max(Spec().Windup, SMALL_NUMBER), 0.f, 1.f);
    else if (State == EMemoriaMonsterState::Recover) Extend = 1.f - FMath::Clamp(StateTime / FMath::Max(Spec().Recover, SMALL_NUMBER), 0.f, 1.f);
    // Held low in front of the waist, then pushed straight out. The bar never rises into a swing.
    const FRotator Yaw(0.f, Figure->GetYaw(), 0.f);
    const FVector Forward = Yaw.Vector();
    const FVector Side(-Forward.Y, Forward.X, 0.f);
    const float Grip = FMath::Lerp(20.f, 74.f, Extend);
    const FVector Waist = GetActorLocation() + Forward * Grip + Side * 18.f + FVector(0, 0, Spec().Height * .42f);
    Club->SetWorldLocation(Waist + Forward * 23.f); Club->SetWorldRotation(Yaw);
    if (ClubBand) { ClubBand->SetWorldLocation(Waist + Forward * 16.f); ClubBand->SetWorldRotation(Yaw); }
}
void AMemoriaFieldMonster::SetShellLook(bool bOn)
{
    USkeletalMeshComponent* Body = Figure ? Figure->GetSkeletalMesh() : nullptr;
    if (!Body) return;
    if (!bOn) { Body->SetOverlayMaterial(nullptr); return; }
    // The ghost material of S366, as an overlay: the body again, violet and edge-lit.
    if (auto* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Memoria/Presentation/Combat/M_Afterimage.M_Afterimage")))
    {
        auto* Shell = UMaterialInstanceDynamic::Create(Base, this);
        Shell->SetVectorParameterValue(TEXT("Tint"), FLinearColor(.66f, .42f, 1.f)); Shell->SetScalarParameterValue(TEXT("Alpha"), .55f);
        Body->SetOverlayMaterial(Shell);
    }
}
bool AMemoriaFieldMonster::BreakShell(const FVector& From)
{
    if (!bShelled || IsDead()) return false;
    bShelled = false; SetShellLook(false);
    if (AuraLight) AuraLight->SetIntensity(5.f);
    Stun(EliteShellBreakStun);
    if (auto* C = Combat()) C->NotifyShellBroken(this, From);
    return true;
}
bool AMemoriaFieldMonster::TakeHit(float Damage, const FVector& From, float Shove, bool bBreaksShell)
{
    if (IsDead()) return false;
    // S367: the void shell. A plain blow does a third and does not stop the elite; a breaking one opens it.
    const bool bWasShelled = bShelled && Damage > 0.f;
    if (bWasShelled && bBreaksShell) BreakShell(From);
    else if (bWasShelled) Damage *= EliteShellFactor;
    LastHitDamage = Damage;
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
    if (bWasShelled && !bBreaksShell) return true;   // shrugged off: no stagger, no shove
    // A broken shell has already set the stun; keep it rather than the shorter stagger.
    if (bWasShelled && State == EMemoriaMonsterState::Stagger) return true;
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
    if (AuraLight && !IsDead()) { AuraClock += DeltaSeconds; AuraLight->SetIntensity((bShelled ? 14.f : 5.f) * (.75f + .25f * FMath::Sin(AuraClock * 3.1f))); }
    else if (AuraLight && IsDead()) AuraLight->SetIntensity(0.f);
    Figure->AdvanceLocomotion(Step, DeltaSeconds);
    if (IsDead())
    {
        // S340: the corpse sinks out of the world over its last moments instead of vanishing.
        if (StateTime > HuskCorpseTime - CorpseSink) AddActorWorldOffset(FVector(0, 0, -Spec().Height * DeltaSeconds / CorpseSink));
        if (StateTime > HuskCorpseTime) Destroy();
        return;
    }
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
        // Moved directly: pawn movement only applies input under a local controller, and husks have none.
        if (Spec().Behaviour == EMemoriaFoeBehaviour::Caster)
        {
            // S342: a caster closes to its range, backs off when Arrel comes at it, and casts from between.
            if (Distance < CasterRetreat) SetActorLocation(GetActorLocation() - ToTarget.GetSafeNormal() * Spec().Speed * .8f * DeltaSeconds, true);
            else if (Distance > CasterRange) SetActorLocation(GetActorLocation() + ToTarget.GetSafeNormal() * Spec().Speed * DeltaSeconds, true);
            else Enter(EMemoriaMonsterState::Windup);
        }
        else if (Spec().Behaviour == EMemoriaFoeBehaviour::Charger)
        {
            // S342: a charger closes, then commits to a lane toward where Arrel stands.
            if (Distance <= RushFrom && Distance > 1.f) { RushDirection = ToTarget / Distance; Figure->SetAim(RushDirection.Rotation().Yaw); Enter(EMemoriaMonsterState::Windup); }
            else SetActorLocation(GetActorLocation() + ToTarget.GetSafeNormal() * Spec().Speed * DeltaSeconds, true);
        }
        else if (Distance <= Spec().Reach * .85f) Enter(EMemoriaMonsterState::Windup);
        else SetActorLocation(GetActorLocation() + ToTarget.GetSafeNormal() * Spec().Speed * DeltaSeconds, true);
        break;
    case EMemoriaMonsterState::Windup:
        // The charger's lane is set; the others keep turning to Arrel.
        if (Spec().Behaviour != EMemoriaFoeBehaviour::Charger) FaceTarget();
        if (StateTime >= Spec().Windup)
        {
            ++Strikes;
            if (Spec().Behaviour == EMemoriaFoeBehaviour::Caster)
            {
                if (C && Distance > 1.f) C->LaunchOrb(this, GetActorLocation() + FVector(0, 0, Spec().Height * .6f), ToTarget / Distance);
                Enter(EMemoriaMonsterState::Recover);
            }
            else if (Spec().Behaviour == EMemoriaFoeBehaviour::Charger) { bRushHit = false; Enter(EMemoriaMonsterState::Rush); }
            else
            {
                // A reach that falls wide of the corridor recovers with the club unlanded.
                const bool bTouch = !Spec().bReach || IsReachTouch(Target->GetActorLocation());
                if (C && bTouch) C->StrikePlayer(this, Spec().Damage);
                // A parry has already sent the foe reeling (Stun); keep that instead of the recovery.
                if (State == EMemoriaMonsterState::Windup) Enter(EMemoriaMonsterState::Recover);
            }
        }
        break;
    case EMemoriaMonsterState::Rush:
        // Down the lane, striking Arrel once if he is still in it.
        {
            // Walls stop the rush; Arrel and the other foes do not (it runs through them).
            const FVector Before = GetActorLocation();
            // The final long frame must not travel beyond the lane the player was shown.
            const float TimeLeft = FMath::Max(0.f, RushDistance / RushSpeed - (StateTime - DeltaSeconds));
            const FVector Next = Before + RushDirection * RushSpeed * FMath::Min(DeltaSeconds, TimeLeft);
            // Ignore pawns for this sweep only. An unswept retry after hitting a pawn would also
            // skip a wall behind it, especially during a long frame.
            auto* Collision = CastChecked<UPrimitiveComponent>(GetRootComponent());
            const ECollisionResponse PawnResponse = Collision->GetCollisionResponseToChannel(ECC_Pawn);
            Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
            SetActorLocation(Next, true);
            Collision->SetCollisionResponseToChannel(ECC_Pawn, PawnResponse);
            // Resolve contact along the distance actually travelled, including a long frame that
            // crosses Arrel. The hit radius must not reach through a wall that stopped the sweep.
            const FVector PlayerPosition = Target->GetActorLocation();
            const FVector Contact = FMath::ClosestPointOnSegment(PlayerPosition, Before, GetActorLocation());
            if (!bRushHit && FVector::DistSquared2D(Contact, PlayerPosition) <= FMath::Square(RushHit))
            {
                FCollisionQueryParams Query(SCENE_QUERY_STAT(MemoriaRushContact), false, this);
                Query.AddIgnoredActor(Target);
                if (!GetWorld()->LineTraceTestByChannel(Contact, PlayerPosition, ECC_Visibility, Query))
                {
                    bRushHit = true;
                    if (C) C->StrikePlayer(this, Spec().Damage, true);
                }
            }
        }
        if (State == EMemoriaMonsterState::Rush && StateTime >= RushDistance / RushSpeed) Enter(EMemoriaMonsterState::Recover);
        break;
    case EMemoriaMonsterState::Recover:
        if (StateTime >= Spec().Recover) Enter(EMemoriaMonsterState::Chase);
        break;
    case EMemoriaMonsterState::Stagger:
        if (StateTime >= StaggerFor) Enter(EMemoriaMonsterState::Chase);
        break;
    default: break;
    }
    PlaceClub();
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
