#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/MemoriaInteractable.h"
#include "MemoriaMaletActor.generated.h"
class UBoxComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
UCLASS()
class MEMORIA_API AMemoriaMaletActor : public AActor, public IMemoriaInteractable
{
    GENERATED_BODY()
public:
    AMemoriaMaletActor();
    virtual bool CanInteract(const APawn& Pawn) const override;
    virtual FString InteractionPrompt() const override;
    virtual bool Interact(APawn& Pawn) override;
    static FVector DevelopmentLocation() { return FVector(240, -160, 0); }
    static constexpr double InteractionRange = 80.0;
private:
    UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> Collision;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Placeholder;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> Label;
};
