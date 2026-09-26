#include "Interaction/MemoriaEliaCompanion.h"
#include "Narrative/MemoriaNarrativeSubsystem.h"
#include "Presentation/MemoriaVerdanArt.h"
#include "Presentation/MemoriaVerdanPresentation.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "PaperSpriteComponent.h"
#include "PaperSprite.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Engine/StaticMesh.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "UObject/ConstructorHelpers.h"

AMemoriaEliaCompanion::AMemoriaEliaCompanion()
{
    PrimaryActorTick.bCanEverTick = true; PrimaryActorTick.TickGroup = TG_PostPhysics;
    Body = CreateDefaultSubobject<USphereComponent>(TEXT("CompanionBody")); SetRootComponent(Body);
    Body->SetSphereRadius(20); Body->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Body->SetCollisionObjectType(ECC_WorldDynamic); Body->SetCollisionResponseToAllChannels(ECR_Ignore);
    Body->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap); Body->SetGenerateOverlapEvents(false);
    Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("CompanionSprite")); Sprite->SetupAttachment(Body);
    Sprite->SetCollisionEnabled(ECollisionEnabled::NoCollision); Sprite->SetGenerateOverlapEvents(false);
    // Same billboard tilt as Malet's field picture, facing the Verdan camera.
    Sprite->SetRelativeRotation(FRotator(0, 0, 42)); Sprite->SetRelativeLocation(FVector(0, 0, -8));
    Shadow = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CompanionShadow")); Shadow->SetupAttachment(Body);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Plane(TEXT("/Engine/BasicShapes/Plane.Plane"));
    Shadow->SetStaticMesh(Plane.Object); Shadow->SetCollisionEnabled(ECollisionEnabled::NoCollision); Shadow->SetCastShadow(false);
    Shadow->SetRelativeLocation(FVector(0, 0, -8.8)); Shadow->SetRelativeScale3D(FVector(.62, .26, 1));
}
void AMemoriaEliaCompanion::Follow(APawn* InTarget)
{
    Target = InTarget; Trail.Reset();
    if (InTarget) { Trail.Add(GetActorLocation()); Trail.Add(InTarget->GetActorLocation()); LastTargetPosition = InTarget->GetActorLocation(); }
    Sprite->SetMaterial(0, LoadObject<UMaterialInterface>(nullptr, TEXT("/Paper2D/MaskedLitSpriteMaterial.MaskedLitSpriteMaterial")));
    Sprite->SetCastShadow(true);
    if (auto* Material = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Memoria/Presentation/Verdan/M_SoftLight.M_SoftLight")))
        if (auto* Instance = UMaterialInstanceDynamic::Create(Material, this))
        { Instance->SetVectorParameterValue(TEXT("Tint"), FLinearColor::Black); Instance->SetScalarParameterValue(TEXT("Alpha"), .6f); Shadow->SetMaterial(0, Instance); }
    Face(FVector(0, -1, 0));
}
void AMemoriaEliaCompanion::Face(const FVector& Move)
{
    // Unreal +Y is the source's up; the four authored field sprites.
    Direction = FMath::Abs(Move.X) >= FMath::Abs(Move.Y) ? (Move.X > 0 ? TEXT("Right") : TEXT("Left")) : (Move.Y > 0 ? TEXT("Up") : TEXT("Down"));
    if (UPaperSprite* Art = MemoriaVerdanArt::LoadSprite(TEXT("Elia") + Direction)) Sprite->SetSprite(Art);
}
FVector AMemoriaEliaCompanion::FollowPoint() const
{
    // _get_trail_follow_point: walk back along the recorded trail by FORMATION_DISTANCE.
    if (!Target.IsValid()) return GetActorLocation();
    if (Trail.Num() < 2) return Target->GetActorLocation();
    double Accumulated = 0;
    for (int32 I = Trail.Num() - 1; I > 0; --I)
    {
        const double Segment = FVector::Dist2D(Trail[I], Trail[I - 1]);
        if (Segment <= .001) continue;
        if (Accumulated + Segment >= FormationDistance) return FMath::Lerp(Trail[I], Trail[I - 1], (FormationDistance - Accumulated) / Segment);
        Accumulated += Segment;
    }
    return Trail[0];
}
void AMemoriaEliaCompanion::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds); Age += DeltaSeconds;
    if (!Target.IsValid() || DeltaSeconds <= 0) return;
    const FVector TargetPosition = Target->GetActorLocation();
    if (Trail.IsEmpty() || FVector::Dist2D(Trail.Last(), TargetPosition) >= TrailSample)
    { Trail.Add(TargetPosition); if (Trail.Num() > MaxTrailPoints) Trail.RemoveAt(0, Trail.Num() - MaxTrailPoints); }
    const FVector Point = FollowPoint(), ToPoint = FVector(Point.X - GetActorLocation().X, Point.Y - GetActorLocation().Y, 0);
    const double Distance = ToPoint.Size();
    if (Distance > WarpDistance)
    {
        // _warp_to_trail: snap to the follow point and restart the trail.
        SetActorLocation(FVector(Point.X, Point.Y, GetActorLocation().Z)); Velocity = FVector::ZeroVector;
        Trail.Reset(); Trail.Add(GetActorLocation()); Trail.Add(TargetPosition); return;
    }
    // Ease toward the follow point; hurry when the player sprints ahead.
    const double TargetSpeed = FVector::Dist2D(TargetPosition, LastTargetPosition) / DeltaSeconds;
    const double Speed = FollowSpeed * (TargetSpeed > FollowSpeed * 1.05 ? SprintCatchup : 1.0);
    const FVector Desired = Distance > ArrivalRadius ? ToPoint.GetSafeNormal() * Speed * FMath::Clamp((Distance - ArrivalRadius) / 72.0, 0.0, 1.0) : FVector::ZeroVector;
    Velocity = FMath::VInterpConstantTo(Velocity, Desired, DeltaSeconds, FollowAccel);
    const FVector Step = Velocity * DeltaSeconds;
    if (Step.SizeSquared2D() > .0001) { SetActorLocation(GetActorLocation() + Step); if (Velocity.Size2D() > 12) Face(Velocity); }
    LastTargetPosition = TargetPosition;
    // Breathing bob, as companion.gd keeps her alive while standing.
    Sprite->SetRelativeLocation(FVector(0, 0, -8 + .6 * FMath::Sin(Age * 1.8)));
}
bool AMemoriaEliaCompanion::CanInteract(const APawn& Pawn) const
{
    // companion.gd is reached by the player's InteractionRay: the player has to face her.
    if (Pawn.GetWorld() != GetWorld()) return false;
    const FVector To = FVector(GetActorLocation().X - Pawn.GetActorLocation().X, GetActorLocation().Y - Pawn.GetActorLocation().Y, 0);
    if (To.Size() > InteractionRange || To.IsNearlyZero()) return false;
    FString Facing = TEXT("Down");
    for (TActorIterator<AMemoriaVerdanPresentation> It(GetWorld()); It; ++It) { Facing = It->Facing(); break; }
    const FVector Forward = Facing == TEXT("Up") ? FVector(0, 1, 0) : Facing == TEXT("Down") ? FVector(0, -1, 0) : Facing == TEXT("Right") ? FVector(1, 0, 0) : FVector(-1, 0, 0);
    return FVector::DotProduct(To.GetSafeNormal(), Forward) >= FacingDot;
}
bool AMemoriaEliaCompanion::Interact(APawn& Pawn)
{
    if (!CanInteract(Pawn)) return false;
    auto* Host = GetGameInstance()->GetSubsystem<UMemoriaNarrativeSubsystem>();
    return Host && Host->InteractWithElia();
}
