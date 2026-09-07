#include "Framework/MemoriaSliceHost.h"
#include "Narrative/MemoriaNarrativeSubsystem.h"
#include "Presentation/MemoriaDevelopmentNarrativeWidget.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
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
UMemoriaNarrativeSubsystem* AMemoriaSliceController::Host() const
{ return GetGameInstance()->GetSubsystem<UMemoriaNarrativeSubsystem>(); }
void AMemoriaSliceController::SetupInputComponent()
{
    Super::SetupInputComponent();
    if (auto* Input = Cast<UEnhancedInputComponent>(InputComponent))
        Input->BindAction(MoveAction, ETriggerEvent::Started, this, &AMemoriaSliceController::Navigate);
}
void AMemoriaSliceController::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds); auto* Narrative = Host();
    if (!Narrative || Narrative->GetRevision() == LastRevision) return;
    LastRevision = Narrative->GetRevision();
    const auto State = Narrative->GetState();
    if (State != EMemoriaSliceState::Exploration && StatusWidget) { StatusWidget->RemoveFromParent(); StatusWidget = nullptr; }
    if (State == EMemoriaSliceState::VN || State == EMemoriaSliceState::Field)
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
            FMemoriaNarrativeView Status; Status.bCompactStatus = true; Status.Header = TEXT("VERDAN DEVELOPMENT HOST / EXPLORATION READY");
            const bool Seen = GetGameInstance()->GetSubsystem<UMemoriaRunSubsystem>()->GetRunSnapshot().GetFlag(TEXT("ch2_arrival_vn_seen"));
            Status.Body = FString::Printf(TEXT("VN seen: %s    Field starts: %d    |    Move: WASD / stick"), Seen ? TEXT("true") : TEXT("false"), Narrative->GetFieldInvocationCount());
            StatusWidget->Display(Status); StatusWidget->SetVisibility(ESlateVisibility::HitTestInvisible); StatusWidget->AddToViewport(10);
        }
        SetInputMode(FInputModeGameOnly()); bShowMouseCursor = false;
        FSlateApplication::Get().SetAllUserFocusToGameViewport();
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
void AMemoriaSliceController::ForwardConfirm(int32 OriginalIndex) { Host()->Confirm(OriginalIndex); }
void AMemoriaSliceController::Confirm()
{
    if (NarrativeWidget) NarrativeWidget->ConfirmIntent(); else Super::Confirm();
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
