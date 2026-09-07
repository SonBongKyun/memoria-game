#include "Interaction/MemoriaInteractionComponent.h"
#include "Interaction/MemoriaInteractable.h"
#include "GameFramework/Pawn.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/World.h"

void UMemoriaInteractionComponent::UpdateTarget(APawn* Pawn)
{
    Target.Reset(); if (!Pawn) return;
    TArray<AActor*> Candidates;
    TArray<TEnumAsByte<EObjectTypeQuery>> Types{UEngineTypes::ConvertToObjectType(ECC_WorldDynamic)};
    UKismetSystemLibrary::SphereOverlapActors(this, Pawn->GetActorLocation(), 100.0f, Types, AActor::StaticClass(), {Pawn}, Candidates);
    double BestDistance = TNumericLimits<double>::Max();
    for (AActor* Candidate : Candidates)
    {
        auto* Boundary = Cast<IMemoriaInteractable>(Candidate);
        if (!Boundary || !Boundary->CanInteract(*Pawn)) continue;
        FHitResult Hit; FCollisionQueryParams Query(SCENE_QUERY_STAT(MemoriaInteraction), false, Pawn);
        if (GetWorld()->LineTraceSingleByChannel(Hit, Pawn->GetActorLocation(), Candidate->GetActorLocation(), ECC_Visibility, Query) && Hit.GetActor() != Candidate) continue;
        const double Distance = FVector::DistSquared(Pawn->GetActorLocation(), Candidate->GetActorLocation());
        if (Distance < BestDistance || (Distance == BestDistance && Target.IsValid() && Candidate->GetPathName() < Target->GetPathName()))
        { BestDistance = Distance; Target = Candidate; }
    }
}
bool UMemoriaInteractionComponent::Interact(APawn* Pawn)
{
    UpdateTarget(Pawn);
    auto* Boundary = Cast<IMemoriaInteractable>(Target.Get());
    return Pawn && Boundary && Boundary->CanInteract(*Pawn) && Boundary->Interact(*Pawn);
}
FString UMemoriaInteractionComponent::GetPrompt() const
{
    auto* Boundary = Cast<IMemoriaInteractable>(Target.Get());
    return Boundary ? Boundary->InteractionPrompt() : FString();
}
