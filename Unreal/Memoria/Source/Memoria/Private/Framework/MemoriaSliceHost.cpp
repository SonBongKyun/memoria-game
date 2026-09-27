#include "Framework/MemoriaSliceHost.h"
#include "Framework/MemoriaFieldPawn.h"
#include "Presentation/MemoriaVerdanPresentation.h"
#include "Narrative/MemoriaNarrativeSubsystem.h"
#include "Presentation/MemoriaDevelopmentNarrativeWidget.h"
#include "EnhancedInputComponent.h"
#include "Interaction/MemoriaInteractionComponent.h"
#include "InputActionValue.h"
#include "Presentation/MemoriaShopWidget.h"
#include "Presentation/MemoriaArchiveWidget.h"
#include "Presentation/MemoriaBattleEntryWidget.h"
#include "Presentation/MemoriaTitleWidget.h"
#include "Save/MemoriaCheckpointSubsystem.h"
#include "Settings/MemoriaSettingsSubsystem.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Battle/MemoriaBattleEntrySubsystem.h"
#include "Framework/MemoriaCoordinates.h"
#include "InputKeyEventArgs.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Audio/MemoriaAudioSubsystem.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Kismet/GameplayStatics.h"

AMemoriaSliceGameMode::AMemoriaSliceGameMode() { PlayerControllerClass = AMemoriaSliceController::StaticClass(); }
void AMemoriaSliceGameMode::StartPlay()
{
    Super::StartPlay();
    auto* Narrative = GetGameInstance()->GetSubsystem<UMemoriaNarrativeSubsystem>();
    const FString Map = GetWorld()->GetMapName(); bool Started = false;
    // New Game (main.gd) enters through the VN host: -MemoriaNewGame or the ?NewGame travel option.
    // main.gd is the game's main scene: the default map opens with ?Title (GameMapsSettings LocalMapOptions).
    if (Map.EndsWith(TEXT("L_Ch2VerdanSlice")) && UGameplayStatics::HasOption(OptionsString, TEXT("Title")))
    { Narrative->EnterTitle(); Started = true; }
    else if (Map.EndsWith(TEXT("L_Ch2VerdanSlice")))
        Started = UGameplayStatics::HasOption(OptionsString, TEXT("NewGame")) || FParse::Param(FCommandLine::Get(), TEXT("MemoriaNewGame"))
            ? Narrative->StartNewGame() : Narrative->StartDevelopmentVN();
    else if (Map.EndsWith(TEXT("L_VerdanUnseenFixture"))) Started = Narrative->StartUnseenFieldFixture();
    else if (Map.EndsWith(TEXT("L_VerdanHost")))
    {
        if (Narrative->HasPendingVerdanReentry()) Started=Narrative->EnterVerdanReentry();
        else if ((FParse::Param(FCommandLine::Get(),TEXT("MemoriaContinue")) || UGameplayStatics::HasOption(OptionsString,TEXT("Continue"))) && !GetGameInstance()->GetSubsystem<UMemoriaRunSubsystem>()->HasActiveRun())
        { Narrative->ContinueCheckpoint(); Started=true; } // Failure is an actionable Continue screen.
        else Started=Narrative->EnterVerdan();
    }
    if (Started && (Map.EndsWith(TEXT("L_VerdanHost")) || Map.EndsWith(TEXT("L_VerdanUnseenFixture"))))
    {
        if (auto* PC = GetWorld()->GetFirstPlayerController())
            if (auto* Pawn = Cast<AMemoriaFieldPawn>(PC->GetPawn())) Pawn->ApplyVerdanMovementProfile();
        GetWorld()->SpawnActor<AMemoriaVerdanPresentation>();
    }
    if (!Started) UE_LOG(LogTemp, Error, TEXT("MEMORIA_SLICE entry refused: %s; start from an explicit slice fixture"), *Map);
}
AMemoriaSliceController::AMemoriaSliceController()
{ Interaction = CreateDefaultSubobject<UMemoriaInteractionComponent>(TEXT("Interaction")); }
FString AMemoriaSliceController::GetInteractionPrompt() const { return Interaction->GetPrompt(); }
UMemoriaNarrativeSubsystem* AMemoriaSliceController::Host() const
{ return GetGameInstance()->GetSubsystem<UMemoriaNarrativeSubsystem>(); }
void AMemoriaSliceController::Cue(const TCHAR* Id) const
{ if (auto* Audio = GetGameInstance()->GetSubsystem<UMemoriaAudioSubsystem>()) Audio->PlaySfx(Id); }
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
    // New PlayerInput synthesizes Press from an unmatched Repeat after level travel.
    // Confirm/menu actions require a physical press; holding through flee must stay inert.
    if (Params.Event==IE_Repeat && (ConfirmKey || Params.Key==EKeys::Escape || Params.Key==EKeys::Tab ||
        Params.Key==EKeys::M || Params.Key==EKeys::Gamepad_FaceButton_Right)) return true;
    // Modal/context changes flush processed key state. Only a physical release
    // ends the gesture that crossed a narrative boundary; repeats do not.
    if(ConfirmKey)TrackConfirmGesture(Params.Key,Params.Event);
    // A Slate-consumed press must not be auto-reconciled as a fresh press in
    // the restored shop when its first repeat reaches PlayerInput.
    if(!ArchiveWidget && ArchiveConsumedKeys.Contains(Params.Key))
    {
        if(Params.Event==IE_Released)ArchiveConsumedKeys.Remove(Params.Key);
        return true;
    }
    if (BattleWidget)
    {
        if (ConfirmKey)
        {
            TrackArchiveGesture(Params.Key,Params.Event);
            if (Params.Event==IE_Pressed && !bAwaitConfirmRelease) BattleWidget->ConfirmIntent();
            return true;
        }
        if(Params.Event==IE_Pressed)BattleWidget->Navigate(Params.Key);
        return true;
    }
    const bool ArchiveToggle=Params.Key==EKeys::Tab || Params.Key==EKeys::M;
    if(ArchiveWidget)
    {
        const bool CloseKey=ArchiveToggle || Params.Key==EKeys::Escape || Params.Key==EKeys::Gamepad_FaceButton_Right;
        const bool Arrow=Params.Key==EKeys::Up || Params.Key==EKeys::Down || Params.Key==EKeys::Left || Params.Key==EKeys::Right;
        if(CloseKey || Arrow || ConfirmKey)
        {
            TrackArchiveGesture(Params.Key,Params.Event);
            if(Params.Event==IE_Pressed)
            {
                if(CloseKey)CloseArchive();
                else if(Arrow)
                {
                    if(Params.Key==EKeys::Up || Params.Key==EKeys::Down)ArchiveWidget->Navigate(Params.Key==EKeys::Up?-1:1);
                    else ArchiveWidget->CycleFilter(Params.Key==EKeys::Left?-1:1);
                }
            }
            return true;
        }
    }
    if(ArchiveToggle)
    {
        if(Params.Event==IE_Pressed)ToggleArchive();
        return true;
    }
    return Super::InputKey(Params);
}
void AMemoriaSliceController::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds); auto* Narrative = Host();
    if (!Narrative) return;
    // main.gd: the title owns the screen until New Game or Continue leaves it.
    if (Narrative->IsOnTitle())
    {
        if (!TitleWidget)
        {
            TitleWidget=CreateWidget<UMemoriaTitleWidget>(this,UMemoriaTitleWidget::StaticClass());
            const auto* Checkpoint=GetGameInstance()->GetSubsystem<UMemoriaCheckpointSubsystem>();
            TitleWidget->Configure(Checkpoint->FindContinue()!=EMemoriaContinueSource::None,GetGameInstance()->GetSubsystem<UMemoriaSettingsSubsystem>());
            TitleWidget->OnAction.BindUObject(this,&AMemoriaSliceController::TitleAction);
            TitleWidget->OnConsumedKey.BindUObject(this,&AMemoriaSliceController::TrackConfirmGesture);
            PresentModal(TitleWidget);
        }
        return;
    }
    if (TitleWidget) ClearTitle();
    auto* Battle=GetGameInstance()->GetSubsystem<UMemoriaBattleEntrySubsystem>();
    if (Battle->IsActive() || Battle->IsReturning())
    {
        Interaction->UpdateTarget(nullptr);
        if (!BattleWidget)
        {
            if (NarrativeWidget) NarrativeWidget->OnConfirm.Unbind();
            DismissModal(); NarrativeWidget=nullptr;
            if (StatusWidget) StatusWidget->SetVisibility(ESlateVisibility::Collapsed);
            BattleWidget=CreateWidget<UMemoriaBattleEntryWidget>(this,UMemoriaBattleEntryWidget::StaticClass());
            BattleWidget->OnFlee.BindUObject(this,&AMemoriaSliceController::RequestBattleFlee);
            BattleWidget->OnAction.BindUObject(this,&AMemoriaSliceController::RequestBattleAction);
            BattleWidget->OnConsumedKey.BindUObject(this,&AMemoriaSliceController::TrackArchiveGesture);
            Battle->OnReturned.AddUObject(this,&AMemoriaSliceController::BattleReturned);
            BattleWidget->BindBattle(Battle); PresentModal(BattleWidget);
            bAwaitConfirmRelease=!HeldConfirmKeys.IsEmpty();
        }
        return;
    }
    if (BattleWidget) ClearBattleWidget();
    if (Narrative->IsVerdanRevisit() && GetPawn())
    {
        if (!bEncounterInitialized) { Encounter.Reset(EncounterRng.Real(60,100)); bEncounterInitialized=true; }
        const bool Exploring=Narrative->GetState()==EMemoriaSliceState::Exploration && !IsModalOpen();
        const auto Step=Encounter.Advance(Memoria::Coordinates::ToSource(GetPawn()->GetActorLocation()),Exploring,EncounterRng);
        const bool PressureChanged=(EncounterPressure>=.5)!=(Step.Pressure>=.5);
        EncounterPressure=Step.Pressure;
        if (PressureChanged) LastRevision=INDEX_NONE;
        if (Step.bWarningStarted) { Narrative->Record(TEXT("encounter:warning")); LastRevision=INDEX_NONE; }
        if (Step.bTriggered)
        {
            if (Battle->BeginEncounter(Step.EnemyIndex,EncounterRng,GetWorld())) Narrative->Record(TEXT("encounter:battle_started"));
            return;
        }
    }
    if(ArchiveWidget)
    {
        Interaction->UpdateTarget(nullptr);
        return; // Keep the archive modal; resume the actual owner view when it closes.
    }
    const auto State = Narrative->GetState();
    Interaction->UpdateTarget(State == EMemoriaSliceState::Exploration && !IsModalOpen() ? GetPawn() : nullptr);
    const FString Prompt = Interaction->GetPrompt();
    const bool Changed = Narrative->GetRevision() != LastRevision;
    const FString NoticeNow = Narrative->GetExplorationNotice();
    if (!Changed && Prompt == LastPrompt && NoticeNow == LastNotice) return;
    LastPrompt = Prompt; LastNotice = NoticeNow; LastRevision = Narrative->GetRevision();
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
        Status.Body = TEXT("WASD / stick  Move     TAB / M  Memories");
        if (Narrative->IsVerdanRevisit() && Encounter.bWarningEmitted)
            Status.Body+=TEXT("\nMemory noise closes in...");
        if (!Prompt.IsEmpty()) Status.Body += TEXT("\n") + Prompt;
        if (const FString Quest = Narrative->GetQuestTrackerLine(); !Quest.IsEmpty()) Status.Body += TEXT("\n") + Quest;
        if (const FString Notice = Narrative->GetExplorationNotice(); !Notice.IsEmpty()) Status.Body += TEXT("\n") + Notice;
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
void AMemoriaSliceController::TitleAction(EMemoriaTitleAction Action)
{
    // main.gd _play_select_sfx on every menu press; a held confirm must not also advance the first line.
    Cue(TEXT("ui_select"));
    auto* Game=GetGameInstance();
    switch (Action)
    {
    case EMemoriaTitleAction::NewGame:
        bAwaitConfirmRelease=!HeldConfirmKeys.IsEmpty(); ClearTitle();
        Host()->StartNewGame(Game->GetSubsystem<UMemoriaSettingsSubsystem>()->GetLocale()); LastRevision=INDEX_NONE;
        break;
    case EMemoriaTitleAction::Continue:
    {
        const auto Source=Game->GetSubsystem<UMemoriaCheckpointSubsystem>()->FindContinue();
        bAwaitConfirmRelease=!HeldConfirmKeys.IsEmpty();
        if (Source==EMemoriaContinueSource::Chapter) { ClearTitle(); Host()->ResumeChapterAutosave(); LastRevision=INDEX_NONE; }
        else if (Source==EMemoriaContinueSource::Boundary) UGameplayStatics::OpenLevel(this,TEXT("/Game/Tests/Campaign/L_VerdanHost"),true,TEXT("Continue"));
        break;
    }
    case EMemoriaTitleAction::Options: break; // The widget opens its own panel.
    case EMemoriaTitleAction::Quit:
        UE_LOG(LogTemp,Display,TEXT("MEMORIA_TITLE quit"));
        if (!GIsAutomationTesting) UKismetSystemLibrary::QuitGame(this,this,EQuitPreference::Quit,false);
        break;
    }
}
void AMemoriaSliceController::ClearTitle()
{
    if(!TitleWidget)return;
    TitleWidget->OnAction.Unbind();TitleWidget->OnConsumedKey.Unbind();
    DismissModal();TitleWidget=nullptr;
}
void AMemoriaSliceController::TrackArchiveGesture(const FKey& Key,EInputEvent Event)
{
    if(Event==IE_Pressed)ArchiveConsumedKeys.Add(Key);
    else if(Event==IE_Released)ArchiveConsumedKeys.Remove(Key);
    if(Key==EKeys::E || Key==EKeys::Enter || Key==EKeys::SpaceBar || Key==EKeys::Gamepad_FaceButton_Bottom)
        TrackConfirmGesture(Key,Event);
}
void AMemoriaSliceController::TrackConfirmGesture(const FKey& Key,EInputEvent Event)
{
    if(Event==IE_Pressed)HeldConfirmKeys.Add(Key);
    else if(Event==IE_Released)
    {
        HeldConfirmKeys.Remove(Key);
        if(HeldConfirmKeys.IsEmpty())bAwaitConfirmRelease=false;
    }
}
void AMemoriaSliceController::ToggleArchive()
{
    if(ArchiveWidget){CloseArchive();return;}
    if(GetGameInstance()->GetSubsystem<UMemoriaBattleEntrySubsystem>()->IsActive())return;
    auto* Narrative=Host();auto* Run=GetGameInstance()->GetSubsystem<UMemoriaRunSubsystem>();
    if(!Narrative || !Run->HasActiveRun() || !GetWorld() || GetWorld()->bIsTearingDown ||
        (Narrative->GetState()!=EMemoriaSliceState::Exploration && Narrative->GetState()!=EMemoriaSliceState::Deferred))return;
    if(NarrativeWidget)NarrativeWidget->OnConfirm.Unbind();
    DismissModal();NarrativeWidget=nullptr;
    if(StatusWidget)StatusWidget->SetVisibility(ESlateVisibility::Collapsed);
    ArchiveWidget=CreateWidget<UMemoriaArchiveWidget>(this,UMemoriaArchiveWidget::StaticClass());
    ArchiveWidget->OnConsumedKey.BindUObject(this,&AMemoriaSliceController::TrackArchiveGesture);
    ArchiveWidget->OnClose.BindUObject(this,&AMemoriaSliceController::CloseArchive);
    ArchiveWidget->BindRun(Run);PresentModal(ArchiveWidget);
    Cue(TEXT("ui_open")); // Addition: the source archive opens silently.
}
void AMemoriaSliceController::CloseArchive()
{
    if(!ArchiveWidget)return;
    Cue(TEXT("ui_close"));
    ArchiveWidget->OnConsumedKey.Unbind();ArchiveWidget->OnClose.Unbind();ArchiveWidget->BindRun(nullptr);
    DismissModal();ArchiveWidget=nullptr;LastRevision=INDEX_NONE;
    bAwaitConfirmRelease=!HeldConfirmKeys.IsEmpty();
}
void AMemoriaSliceController::Move(const FInputActionValue& Value)
{
    if (Host()->GetState() == EMemoriaSliceState::Exploration) Super::Move(Value);
}
void AMemoriaSliceController::Navigate(const FInputActionValue& Value)
{
    if(TitleWidget)
    {
        // Keys reach the focused title through Slate; the stick arrives here.
        const auto Axis=Value.Get<FVector2D>();
        if(FMath::Abs(Axis.Y)>.5f)TitleWidget->Navigate(Axis.Y>0?-1:1);
        else if(FMath::Abs(Axis.X)>.5f)TitleWidget->Adjust(Axis.X>0?1:-1);
        return;
    }
    if(ArchiveWidget)
    {
        const auto Axis=Value.Get<FVector2D>();
        if(FMath::Abs(Axis.Y)>.5f)ArchiveWidget->Navigate(Axis.Y>0?-1:1);
        else if(FMath::Abs(Axis.X)>.5f)ArchiveWidget->CycleFilter(Axis.X>0?1:-1);
        return;
    }
    if (NarrativeWidget)
    {
        const auto View = Host()->GetView();
        const float X = Value.Get<FVector2D>().X;
        if (View.bShopPresentation && FMath::Abs(X) > .5f && NarrativeWidget->GetShopWidget())
            NarrativeWidget->GetShopWidget()->SwitchMode(X > 0 ? 1 : -1);
        const float Y = Value.Get<FVector2D>().Y;
        if (FMath::Abs(Y) > 0.5f) NarrativeWidget->Navigate(Y > 0 ? -1 : 1);
        // Source dialogue_box/memory_shop play ui_hover when a choice or row gains focus.
        if (FMath::Abs(Y) > .5f && (View.bShopPresentation || View.Choices.Num() > 1)) Cue(TEXT("ui_hover"));
    }
}
void AMemoriaSliceController::ForwardConfirm(int32 OriginalIndex)
{
    if (bAwaitConfirmRelease || ArchiveWidget || BattleWidget) return;
    // Source dialogue_box: ui_select when a choice is pressed, confirm when a line advances.
    Cue(Host()->GetView().Choices.IsEmpty() ? TEXT("confirm") : TEXT("ui_select"));
    const auto Before = Host()->GetState(); Host()->Confirm(OriginalIndex);
    if (Before != Host()->GetState()) bAwaitConfirmRelease = true;
}
void AMemoriaSliceController::Confirm()
{
    if (bAwaitConfirmRelease || ArchiveWidget || TitleWidget) return;
    if (BattleWidget) BattleWidget->ConfirmIntent();
    else if (NarrativeWidget) NarrativeWidget->ConfirmIntent();
    else if (Host()->GetState() == EMemoriaSliceState::Exploration && !IsModalOpen())
    {
        if (Interaction->Interact(GetPawn())) bAwaitConfirmRelease = true;
    }
    else Super::Confirm();
}
void AMemoriaSliceController::Back()
{
    if(TitleWidget){TitleWidget->Back();return;}
    if(BattleWidget)return;
    if(ArchiveWidget){CloseArchive();return;}
    if (NarrativeWidget) Host()->Back(); else Super::Back();
}
void AMemoriaSliceController::OpenModal()
{
    if(BattleWidget || TitleWidget)return;
    if(ArchiveWidget){CloseArchive();return;}
    if (NarrativeWidget) Host()->Back();
    else if (Host()->GetState() == EMemoriaSliceState::Exploration) Super::OpenModal();
}
void AMemoriaSliceController::RequestBattleFlee(uint64 Revision)
{
    if(bAwaitConfirmRelease)return;
    if(GetGameInstance()->GetSubsystem<UMemoriaBattleEntrySubsystem>()->Flee(Revision))
        bAwaitConfirmRelease=!HeldConfirmKeys.IsEmpty();
}
void AMemoriaSliceController::ClearBattleWidget()
{
    if(!BattleWidget)return;
    BattleWidget->OnAction.Unbind();BattleWidget->OnFlee.Unbind();BattleWidget->OnConsumedKey.Unbind();BattleWidget->BindBattle(nullptr);
    GetGameInstance()->GetSubsystem<UMemoriaBattleEntrySubsystem>()->OnReturned.RemoveAll(this);
    DismissModal();BattleWidget=nullptr;LastRevision=INDEX_NONE;
}
void AMemoriaSliceController::BattleReturned()
{
    ClearBattleWidget();Host()->ReturnFromAmbientBattle();
}
void AMemoriaSliceController::EndPlay(const EEndPlayReason::Type Reason)
{
    ClearBattleWidget(); ClearTitle();
    if(ArchiveWidget){ArchiveWidget->OnConsumedKey.Unbind();ArchiveWidget->OnClose.Unbind();ArchiveWidget->BindRun(nullptr);ArchiveWidget=nullptr;}
    if (NarrativeWidget) NarrativeWidget->OnConfirm.Unbind();
    if (StatusWidget) StatusWidget->RemoveFromParent(); StatusWidget = nullptr;
    NarrativeWidget = nullptr; Super::EndPlay(Reason);
}

void AMemoriaSliceController::RequestBattleAction(const FString& Action,const FString& Id,uint64 Revision)
{
    if(bAwaitConfirmRelease)return;
    auto* Battle=GetGameInstance()->GetSubsystem<UMemoriaBattleEntrySubsystem>();
    bool Accepted=false;
    if(Action==TEXT("continue"))Accepted=Battle->DismissVictory(Revision);
    else if(Action==TEXT("recover"))Accepted=Battle->RecoverToVerdan(Revision);
    else if(Action==TEXT("checkpoint"))Accepted=Battle->RetryCheckpoint(Revision);
    else Accepted=Battle->Submit(Action,Id,Revision);
    if(Accepted)bAwaitConfirmRelease=!HeldConfirmKeys.IsEmpty();
}
