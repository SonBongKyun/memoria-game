#include "Framework/MemoriaSliceHost.h"
#include "Narrative/MemoriaNarrativeSubsystem.h"
#include "Presentation/MemoriaDevelopmentNarrativeWidget.h"
#include "EnhancedInputComponent.h"
#include "Interaction/MemoriaInteractionComponent.h"
#include "InputActionValue.h"
#include "InputKeyEventArgs.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Framework/Application/SlateApplication.h"

AMemoriaSliceGameMode::AMemoriaSliceGameMode() { PlayerControllerClass = AMemoriaSliceController::StaticClass(); }
void AMemoriaSliceGameMode::StartPlay()
{
    Super::StartPlay();
    auto* Narrative = GetGameInstance()->GetSubsystem<UMemoriaNarrativeSubsystem>();
    const FString Map = GetWorld()->GetMapName(); bool Started = false;
    if (Map.EndsWith(TEXT("L_Ch2VerdanSlice"))) Started = Narrative->StartDevelopmentVN();
    else if (Map.EndsWith(TEXT("L_VerdanUnseenFixture"))) Started = Narrative->StartUnseenFieldFixture();
    else if (Map.EndsWith(TEXT("L_VerdanHost"))) Started = Narrative->EnterVerdan();
    if (!Started) UE_LOG(LogTemp, Error, TEXT("MEMORIA_SLICE entry refused: %s; start from an explicit slice fixture"), *Map);
}
AMemoriaSliceController::AMemoriaSliceController()
{ Interaction = CreateDefaultSubobject<UMemoriaInteractionComponent>(TEXT("Interaction")); }
FString AMemoriaSliceController::GetInteractionPrompt() const { return Interaction->GetPrompt(); }
UMemoriaNarrativeSubsystem* AMemoriaSliceController::Host() const
{ return GetGameInstance()->GetSubsystem<UMemoriaNarrativeSubsystem>(); }
void AMemoriaSliceController::SetupInputComponent()
{
    Super::SetupInputComponent();
    if (auto* Input = Cast<UEnhancedInputComponent>(InputComponent))
        Input->BindAction(MoveAction, ETriggerEvent::Started, this, &AMemoriaSliceController::Navigate);
}
bool AMemoriaSliceController::InputKey(const FInputKeyEventArgs& Params)
{
    const bool ConfirmKey = Params.Key == EKeys::E || Params.Key == EKeys::SpaceBar ||
        Params.Key == EKeys::Enter || Params.Key == EKeys::Gamepad_FaceButton_Bottom;
    // Modal/context changes flush processed key state. Only a physical release
    // ends the gesture that crossed a narrative boundary; repeats do not.
    if (ConfirmKey)
    {
        if (Params.Event == IE_Pressed) HeldConfirmKeys.Add(Params.Key);
        else if (Params.Event == IE_Released)
        {
            HeldConfirmKeys.Remove(Params.Key);
            if (HeldConfirmKeys.IsEmpty()) bAwaitConfirmRelease = false;
        }
    }
    return Super::InputKey(Params);
}
void AMemoriaSliceController::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds); auto* Narrative = Host();
    if (!Narrative) return;
    const auto State = Narrative->GetState();
    Interaction->UpdateTarget(State == EMemoriaSliceState::Exploration && !IsModalOpen() ? GetPawn() : nullptr);
    const FString Prompt = Interaction->GetPrompt();
    const bool Changed = Narrative->GetRevision() != LastRevision;
    if (!Changed && Prompt == LastPrompt) return;
    LastPrompt = Prompt; LastRevision = Narrative->GetRevision();
    if (State != EMemoriaSliceState::Exploration && StatusWidget) { StatusWidget->RemoveFromParent(); StatusWidget = nullptr; }
    if (State == EMemoriaSliceState::VN || State == EMemoriaSliceState::Field || State == EMemoriaSliceState::Deferred)
    {
        if (!NarrativeWidget)
        {
            DismissModal(); NarrativeWidget = CreateWidget<UMemoriaDevelopmentNarrativeWidget>(this, UMemoriaDevelopmentNarrativeWidget::StaticClass());
            NarrativeWidget->OnConfirm.BindUObject(this, &AMemoriaSliceController::ForwardConfirm);
            PresentModal(NarrativeWidget);
        }
        NarrativeWidget->Display(Narrative->GetView());
    }
    else if (NarrativeWidget)
    {
        NarrativeWidget->OnConfirm.Unbind(); DismissModal(); NarrativeWidget = nullptr;
    }
    if (State == EMemoriaSliceState::Exploration)
    {
        if (!StatusWidget)
        {
            StatusWidget = CreateWidget<UMemoriaDevelopmentNarrativeWidget>(this, UMemoriaDevelopmentNarrativeWidget::StaticClass());
        }
        FMemoriaNarrativeView Status; Status.bCompactStatus = true; Status.Header = TEXT("VERDAN  /  THE GRAY BELT");
        Status.Body = TEXT("WASD / stick  Move");
        if (!Prompt.IsEmpty()) Status.Body += TEXT("\n") + Prompt;
        if (!Narrative->GetDeferredInteraction().IsEmpty())
            Status.Body += TEXT("\nDevelopment boundary: resolved ") + Narrative->GetDeferredInteraction() + TEXT("; content deferred.");
        StatusWidget->Display(Status); StatusWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
        if (!StatusWidget->IsInViewport()) StatusWidget->AddToViewport(10);
        if (Changed && !IsModalOpen())
        {
            SetInputMode(FInputModeGameOnly()); bShowMouseCursor = false;
            FSlateApplication::Get().SetAllUserFocusToGameViewport();
        }
    }
}
void AMemoriaSliceController::Move(const FInputActionValue& Value)
{
    if (Host()->GetState() == EMemoriaSliceState::Exploration) Super::Move(Value);
}
void AMemoriaSliceController::Navigate(const FInputActionValue& Value)
{
    if (NarrativeWidget)
    {
        const float Y = Value.Get<FVector2D>().Y;
        if (FMath::Abs(Y) > 0.5f) NarrativeWidget->Navigate(Y > 0 ? -1 : 1);
    }
}
void AMemoriaSliceController::ForwardConfirm(int32 OriginalIndex)
{
    if (bAwaitConfirmRelease) return;
    const auto Before = Host()->GetState(); Host()->Confirm(OriginalIndex);
    if (Before != Host()->GetState()) bAwaitConfirmRelease = true;
}
void AMemoriaSliceController::Confirm()
{
    if (bAwaitConfirmRelease) return;
    if (NarrativeWidget) NarrativeWidget->ConfirmIntent();
    else if (Host()->GetState() == EMemoriaSliceState::Exploration && !IsModalOpen())
    {
        if (Interaction->Interact(GetPawn())) bAwaitConfirmRelease = true;
    }
    else Super::Confirm();
}
void AMemoriaSliceController::Back()
{
    if (NarrativeWidget) Host()->Back(); else Super::Back();
}
void AMemoriaSliceController::OpenModal()
{
    if (NarrativeWidget) Host()->Back();
    else if (Host()->GetState() == EMemoriaSliceState::Exploration) Super::OpenModal();
}
void AMemoriaSliceController::EndPlay(const EEndPlayReason::Type Reason)
{
    if (NarrativeWidget) NarrativeWidget->OnConfirm.Unbind();
    if (StatusWidget) StatusWidget->RemoveFromParent(); StatusWidget = nullptr;
    NarrativeWidget = nullptr; Super::EndPlay(Reason);
}
