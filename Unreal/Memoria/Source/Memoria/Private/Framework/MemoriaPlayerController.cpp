#include "Framework/MemoriaPlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Pawn.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Framework/Application/SlateApplication.h"

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
        {
            Input->RemoveMappingContext(ExplorationContext);
            if (ModalContext) { Input->RemoveMappingContext(ModalContext); }
        }
    }
    if (Modal) { Modal->RemoveFromParent(); Modal = nullptr; }
    Super::EndPlay(Reason);
}
void AMemoriaPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    // Foundation-only assets. Later gameplay controllers can supply their own defaults.
    const FString Base = TEXT("/Game/Tests/Foundation/");
    if (!ExplorationContext) { ExplorationContext = LoadObject<UInputMappingContext>(nullptr, *(Base + TEXT("IMC_Foundation.IMC_Foundation"))); }
    if (!ModalContext) { ModalContext = LoadObject<UInputMappingContext>(nullptr, *(Base + TEXT("IMC_Modal.IMC_Modal"))); }
    if (!MoveAction) { MoveAction = LoadObject<UInputAction>(nullptr, *(Base + TEXT("IA_Move.IA_Move"))); }
    if (!ConfirmAction) { ConfirmAction = LoadObject<UInputAction>(nullptr, *(Base + TEXT("IA_Confirm.IA_Confirm"))); }
    if (!BackAction) { BackAction = LoadObject<UInputAction>(nullptr, *(Base + TEXT("IA_Back.IA_Back"))); }
    if (!MenuAction) { MenuAction = LoadObject<UInputAction>(nullptr, *(Base + TEXT("IA_Menu.IA_Menu"))); }
    if (auto* Input = Cast<UEnhancedInputComponent>(InputComponent); Input && MoveAction)
    {
        Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMemoriaPlayerController::Move);
        Input->BindAction(ConfirmAction, ETriggerEvent::Started, this, &AMemoriaPlayerController::Confirm);
        Input->BindAction(BackAction, ETriggerEvent::Started, this, &AMemoriaPlayerController::Back);
        Input->BindAction(MenuAction, ETriggerEvent::Started, this, &AMemoriaPlayerController::OpenModal);
    }
}
void AMemoriaPlayerController::Move(const FInputActionValue& Value)
{
    if (APawn* ControlledPawn = GetPawn(); ControlledPawn && !IsModalOpen())
    {
        const FVector2D Axis = Value.Get<FVector2D>();
        ControlledPawn->AddMovementInput(FVector::ForwardVector, Axis.X);
        ControlledPawn->AddMovementInput(FVector::RightVector, Axis.Y);
    }
}

void AMemoriaPlayerController::OpenModal()
{
    if (Modal) { return; }
    auto* Class = LoadClass<UUserWidget>(nullptr, TEXT("/Game/Tests/Foundation/WBP_FoundationModal.WBP_FoundationModal_C"));
    if (!Class) { return; }
    Modal = CreateWidget<UUserWidget>(this, Class);
    if (!Modal) { return; }
    Modal->SetIsFocusable(true);
    Modal->AddToViewport(100);
    SetIgnoreMoveInput(true);
    if (APawn* P = GetPawn())
    {
        P->ConsumeMovementInputVector();
        if (auto* M = P->GetMovementComponent()) { M->StopMovementImmediately(); }
    }
    if (auto* Local = GetLocalPlayer())
    {
        if (auto* Input = Local->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
        {
            // Rebuild next frame and ignore held keys until release: the opening
            // Escape must not become a Back in the newly active modal context.
            Input->AddMappingContext(ModalContext, 100, FModifyContextOptions());
        }
    }
    FInputModeGameAndUI Mode; Mode.SetWidgetToFocus(Modal->TakeWidget()); Mode.SetHideCursorDuringCapture(false);
    SetInputMode(Mode); bShowMouseCursor = true;
    ++ModalTransitionCount;
}
void AMemoriaPlayerController::Back()
{
    ++BackDispatchCount;
    if (!Modal) { return; }
    if (auto* Local = GetLocalPlayer())
    {
        if (auto* Input = Local->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
        { Input->RemoveMappingContext(ModalContext, FModifyContextOptions()); }
    }
    Modal->RemoveFromParent(); Modal = nullptr;
    SetIgnoreMoveInput(false);
    SetInputMode(FInputModeGameOnly()); bShowMouseCursor = false;
    FSlateApplication::Get().SetAllUserFocusToGameViewport();
    ++ModalTransitionCount;
}
void AMemoriaPlayerController::Confirm()
{
    ++ConfirmCount;
    if (Modal)
    {
        if (auto* Text = Cast<UTextBlock>(Modal->GetWidgetFromName(TEXT("Message"))))
        { Text->SetText(FText::FromString(TEXT("Confirm received. Escape / B closes this modal."))); }
    }
}
