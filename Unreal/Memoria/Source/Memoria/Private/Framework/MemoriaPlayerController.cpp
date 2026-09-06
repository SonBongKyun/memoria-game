#include "Framework/MemoriaPlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Pawn.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"

void AMemoriaPlayerController::BeginPlay()
{
    Super::BeginPlay();
    if (const auto* Local = GetLocalPlayer())
    {
        if (auto* Input = Local->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(); Input && ExplorationContext)
        { Input->AddMappingContext(ExplorationContext, 0); }
    }
}
void AMemoriaPlayerController::EndPlay(const EEndPlayReason::Type Reason)
{
    if (const auto* Local = GetLocalPlayer())
    {
        if (auto* Input = Local->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(); Input && ExplorationContext)
        { Input->RemoveMappingContext(ExplorationContext); }
    }
    Super::EndPlay(Reason);
}
void AMemoriaPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    if (auto* Input = Cast<UEnhancedInputComponent>(InputComponent); Input && MoveAction)
    { Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMemoriaPlayerController::Move); }
}
void AMemoriaPlayerController::Move(const FInputActionValue& Value)
{
    if (auto* ControlledPawn = GetPawn())
    {
        const FVector2D Axis = Value.Get<FVector2D>();
        ControlledPawn->AddMovementInput(FVector::ForwardVector, Axis.X);
        ControlledPawn->AddMovementInput(FVector::RightVector, Axis.Y);
    }
}
