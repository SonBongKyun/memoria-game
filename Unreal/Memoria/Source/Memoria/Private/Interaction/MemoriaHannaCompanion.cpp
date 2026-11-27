#include "Interaction/MemoriaHannaCompanion.h"
#include "Interaction/MemoriaTobiasCompanion.h"
#include "Presentation/MemoriaVerdanPresentation.h"
#include "Presentation/MemoriaFieldCharacterComponent.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "PaperSpriteComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
bool bLeavingPoint = false;
FVector LeavingPoint = FVector::ZeroVector;
}

void AMemoriaHannaCompanion::SetLeavingPoint(const FVector& WorldPoint)
{
    bLeavingPoint = true;
    LeavingPoint = WorldPoint;
}
bool AMemoriaHannaCompanion::HasLeavingPoint() { return bLeavingPoint; }
FVector AMemoriaHannaCompanion::GetLeavingPoint() { return LeavingPoint; }
void AMemoriaHannaCompanion::ClearLeavingPoint()
{
    bLeavingPoint = false;
    LeavingPoint = FVector::ZeroVector;
}
AMemoriaHannaCompanion::AMemoriaHannaCompanion()
{
    PrimaryActorTick.bCanEverTick = true; PrimaryActorTick.TickGroup = TG_PostPhysics;
    Body = CreateDefaultSubobject<USphereComponent>(TEXT("CompanionBody")); SetRootComponent(Body);
    Body->SetSphereRadius(26); Body->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Body->SetCollisionObjectType(ECC_WorldDynamic); Body->SetCollisionResponseToAllChannels(ECR_Ignore);
    Body->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap); Body->SetGenerateOverlapEvents(false);
    Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("CompanionSprite")); Sprite->SetupAttachment(Body);
    Sprite->SetCollisionEnabled(ECollisionEnabled::NoCollision); Sprite->SetGenerateOverlapEvents(false);
    Sprite->SetRelativeRotation(FRotator(0, 0, 42)); Sprite->SetRelativeLocation(FVector(0, 0, -8));
    Shadow = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CompanionShadow")); Shadow->SetupAttachment(Body);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Plane(TEXT("/Engine/BasicShapes/Plane.Plane"));
    Shadow->SetStaticMesh(Plane.Object); Shadow->SetCollisionEnabled(ECollisionEnabled::NoCollision); Shadow->SetCastShadow(false);
    Shadow->SetRelativeLocation(FVector(0, 0, -8.8)); Shadow->SetRelativeScale3D(FVector(.62, .26, 1));
}
AMemoriaHannaCompanion* AMemoriaHannaCompanion::Join(UWorld* World, APawn* Leader)
{
    if (!World || !Leader) return nullptr;
    for (TActorIterator<AMemoriaHannaCompanion> It(World); It; ++It) return *It;
    if (UGameInstance* Game = World->GetGameInstance())
        if (UMemoriaRunSubsystem* Run = Game->GetSubsystem<UMemoriaRunSubsystem>())
            if (Run->HasActiveRun() && Run->GetRunSnapshot().GetFlag(TEXT("hanna_gone"))) return nullptr;
    FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    // Tobias stands near (-36, -210). This is further down the same side, clear of him on the first frame.
    auto* Hanna = World->SpawnActor<AMemoriaHannaCompanion>(Leader->GetActorLocation() + FVector(-42, -340, 0), FRotator::ZeroRotator, Params);
    if (Hanna) Hanna->Follow(Leader);
    return Hanna;
}
void AMemoriaHannaCompanion::Follow(APawn* InTarget)
{
    Target = InTarget; Trail.Reset();
    if (InTarget) { Trail.Add(GetActorLocation()); Trail.Add(InTarget->GetActorLocation()); LastTargetPosition = InTarget->GetActorLocation(); }
    if (!Figure)
    {
        Figure = NewObject<UMemoriaFieldCharacterComponent>(this); AddInstanceComponent(Figure);
        Figure->SetupAttachment(Body); Figure->RegisterComponent();
    }
    const float Height = UMemoriaFieldCharacterComponent::AmbientHeight(TEXT("villagerf"), AMemoriaVerdanPresentation::ArrelHeight);
    if (Figure->InitializeCharacter(TEXT("hanna"), Height)) Sprite->SetHiddenInGame(true);
    if (auto* Material = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Memoria/Presentation/Verdan/M_SoftLight.M_SoftLight")))
        if (auto* Instance = UMaterialInstanceDynamic::Create(Material, this))
        { Instance->SetVectorParameterValue(TEXT("Tint"), FLinearColor::Black); Instance->SetScalarParameterValue(TEXT("Alpha"), .6f); Shadow->SetMaterial(0, Instance); }
    Face(FVector(0, -1, 0));
}
void AMemoriaHannaCompanion::Face(const FVector& Move)
{
    const FString Direction = FMath::Abs(Move.X) >= FMath::Abs(Move.Y) ? (Move.X > 0 ? TEXT("Right") : TEXT("Left")) : (Move.Y > 0 ? TEXT("Up") : TEXT("Down"));
    if (Figure) Figure->Face(Direction);
}
FVector AMemoriaHannaCompanion::FollowPoint() const
{
    if (!Target.IsValid()) return GetActorLocation();
    if (Trail.Num() < 2) return Target->GetActorLocation();
    double Accumulated = 0;
    for (int32 Index = Trail.Num() - 1; Index > 0; --Index)
    {
        const double Segment = FVector::Dist2D(Trail[Index], Trail[Index - 1]);
        if (Segment <= .001) continue;
        if (Accumulated + Segment >= FormationDistance) return FMath::Lerp(Trail[Index], Trail[Index - 1], (FormationDistance - Accumulated) / Segment);
        Accumulated += Segment;
    }
    return Trail[0];
}
void AMemoriaHannaCompanion::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds); Age += DeltaSeconds;
    if (DeltaSeconds <= 0) return;
    if (!bLeaving)
        if (UGameInstance* Game = GetGameInstance())
            if (UMemoriaRunSubsystem* Run = Game->GetSubsystem<UMemoriaRunSubsystem>())
                if (Run->HasActiveRun() && Run->GetRunSnapshot().GetFlag(TEXT("hanna_gone")))
                {
                    bLeaving = true;
                    LeaveTarget = bLeavingPoint ? LeavingPoint : GetActorLocation() + FVector(0, -DefaultLeavingDistance, 0);
                }
    if (bLeaving)
    {
        if (bLeavingPoint) LeaveTarget = LeavingPoint;
        const FVector Flat(LeaveTarget.X - GetActorLocation().X, LeaveTarget.Y - GetActorLocation().Y, 0);
        if (Flat.Size() <= LeaveArrival) { Destroy(); return; }
        const FVector Step = Flat.GetSafeNormal() * FMath::Min(LeaveSpeed * DeltaSeconds, Flat.Size());
        SetActorLocation(GetActorLocation() + Step);
        if (Step.SizeSquared2D() > .0001) Face(Step);
        if (Figure) Figure->AdvanceLocomotion(Step, DeltaSeconds);
        Sprite->SetRelativeLocation(FVector(0, 0, -8 + .6 * FMath::Sin(Age * 1.8)));
        return;
    }
    if (!Target.IsValid()) return;
    const FVector TargetPosition = Target->GetActorLocation();
    if (Trail.IsEmpty() || FVector::Dist2D(Trail.Last(), TargetPosition) >= TrailSample)
    { Trail.Add(TargetPosition); if (Trail.Num() > MaxTrailPoints) Trail.RemoveAt(0, Trail.Num() - MaxTrailPoints); }
    FVector Point = FollowPoint();
    AMemoriaTobiasCompanion* Tobias = nullptr;
    for (TActorIterator<AMemoriaTobiasCompanion> It(GetWorld()); It; ++It) { Tobias = *It; break; }
    if (Tobias)
    {
        const FVector TobiasAt = Tobias->GetActorLocation();
        const FVector Back = FVector(TobiasAt.X - TargetPosition.X, TobiasAt.Y - TargetPosition.Y, 0).GetSafeNormal();
        if (!Back.IsNearlyZero() && FVector::Dist2D(Point, TobiasAt) < ClearGap) Point = TobiasAt + Back * ClearGap;
    }
    const FVector ToPoint = FVector(Point.X - GetActorLocation().X, Point.Y - GetActorLocation().Y, 0);
    const double Distance = ToPoint.Size();
    if (Distance > WarpDistance)
    {
        SetActorLocation(FVector(Point.X, Point.Y, GetActorLocation().Z)); Velocity = FVector::ZeroVector;
        Trail.Reset(); Trail.Add(GetActorLocation()); Trail.Add(TargetPosition); return;
    }
    const double TargetSpeed = FVector::Dist2D(TargetPosition, LastTargetPosition) / DeltaSeconds;
    const double Speed = FollowSpeed * (TargetSpeed > FollowSpeed * 1.05 ? SprintCatchup : 1.0);
    const FVector Desired = Distance > ArrivalRadius ? ToPoint.GetSafeNormal() * Speed * FMath::Clamp((Distance - ArrivalRadius) / 72.0, 0.0, 1.0) : FVector::ZeroVector;
    Velocity = FMath::VInterpConstantTo(Velocity, Desired, DeltaSeconds, FollowAccel);
    const FVector Step = Velocity * DeltaSeconds;
    if (Step.SizeSquared2D() > .0001) { SetActorLocation(GetActorLocation() + Step); if (Velocity.Size2D() > 12) Face(Velocity); }
    if (Figure) Figure->AdvanceLocomotion(Velocity.Size2D() > 12 ? Step : FVector::ZeroVector, DeltaSeconds);
    LastTargetPosition = TargetPosition;
    Sprite->SetRelativeLocation(FVector(0, 0, -8 + .6 * FMath::Sin(Age * 1.8)));
}
