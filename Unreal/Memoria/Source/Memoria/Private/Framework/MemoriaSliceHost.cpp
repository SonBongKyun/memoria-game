#include "Framework/MemoriaSliceHost.h"
#include "Tutorial/MemoriaTutorialSubsystem.h"
#include "Tutorial/MemoriaHintWidget.h"
#include "Achievements/MemoriaAchievementSubsystem.h"
#include "Achievements/MemoriaAchievementWidgets.h"
#include "Codex/MemoriaCodexSubsystem.h"
#include "Codex/MemoriaCodexWidget.h"
#include "Journal/MemoriaJournalWidget.h"
#include "Framework/MemoriaFieldPawn.h"
#include "Presentation/MemoriaVerdanPresentation.h"
#include "Narrative/MemoriaNarrativeSubsystem.h"
#include "Presentation/MemoriaDevelopmentNarrativeWidget.h"
#include "EnhancedInputComponent.h"
#include "Interaction/MemoriaInteractionComponent.h"
#include "InputActionValue.h"
#include "Presentation/MemoriaShopWidget.h"
#include "Presentation/MemoriaArchiveWidget.h"
#include "Presentation/MemoriaTitleWidget.h"
#include "Presentation/MemoriaPauseWidget.h"
#include "Presentation/MemoriaGameOverWidget.h"
#include "Chapter/MemoriaChapterMap.h"
#include "Chapter/MemoriaChapterPresentation.h"
#include "Domain/MemoriaPlayerMemoryDomain.h"
#include "Save/MemoriaCheckpointSubsystem.h"
#include "Settings/MemoriaSettingsSubsystem.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Combat/MemoriaFieldCombatSubsystem.h"
#include "Presentation/MemoriaFieldCharacterComponent.h"
#include "Framework/MemoriaCoordinates.h"
#include "InputKeyEventArgs.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Audio/MemoriaAudioSubsystem.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/CommandLine.h"
#include "Misc/App.h"
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
        // An explicit ?Continue travel always restores, even over a live run (Title from the pause menu, then
        // Continue); the -MemoriaContinue switch outlives travel, so it only applies before a run exists.
        else if (UGameplayStatics::HasOption(OptionsString,TEXT("Continue")) || (FParse::Param(FCommandLine::Get(),TEXT("MemoriaContinue")) && !GetGameInstance()->GetSubsystem<UMemoriaRunSubsystem>()->HasActiveRun()))
        { Narrative->ContinueCheckpoint(); Started=true; } // Failure is an actionable Continue screen.
        else Started=Narrative->EnterVerdan();
    }
    if (Started && (Map.EndsWith(TEXT("L_VerdanHost")) || Map.EndsWith(TEXT("L_VerdanUnseenFixture"))))
    {
        if (auto* PC = GetWorld()->GetFirstPlayerController())
            if (auto* Pawn = Cast<AMemoriaFieldPawn>(PC->GetPawn())) Pawn->ApplyVerdanMovementProfile();
        GetWorld()->SpawnActor<AMemoriaVerdanPresentation>();
    }
    // S320: a content-first chapter map; its presentation builds the field from the map's IR.
    if (const FString Chapter = MemoriaChapterMaps::MapFromLevel(Map); !Started && !Chapter.IsEmpty() && Narrative->EnterChapterMap(Chapter))
    {
        Started = true;
        FActorSpawnParameters Params; Params.bDeferConstruction = true;
        if (auto* Presentation = GetWorld()->SpawnActor<AMemoriaChapterPresentation>(AMemoriaChapterPresentation::StaticClass(), FTransform::Identity, Params))
        { Presentation->Configure(Chapter); Presentation->FinishSpawning(FTransform::Identity); }
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
    // tutorial_hints.gd: any key or click lets a hint go. It is not swallowed: in the field it is also a strike.
    if(Params.Event==IE_Pressed)
        if(auto* Tutorial=GetGameInstance()->GetSubsystem<UMemoriaTutorialSubsystem>(); Tutorial && Tutorial->IsShowing() && Tutorial->GetAge()>=UMemoriaTutorialSubsystem::SlideSeconds)
            Tutorial->Dismiss();
    // A Slate-consumed press must not be auto-reconciled as a fresh press in
    // the restored shop when its first repeat reaches PlayerInput.
    if(!ArchiveWidget && ArchiveConsumedKeys.Contains(Params.Key))
    {
        if(Params.Event==IE_Released)ArchiveConsumedKeys.Remove(Params.Key);
        return true;
    }
    if(JournalWidget)
    {
        if(Params.Event!=IE_Pressed && Params.Event!=IE_Repeat)return true;
        const FKey K=Params.Key;
        if(K==EKeys::Up || K==EKeys::W || K==EKeys::Gamepad_DPad_Up){JournalWidget->Move(-1);Cue(TEXT("ui_hover"));}
        else if(K==EKeys::Down || K==EKeys::S || K==EKeys::Gamepad_DPad_Down){JournalWidget->Move(1);Cue(TEXT("ui_hover"));}
        else if(Params.Event==IE_Pressed && (K==EKeys::Tab || K==EKeys::Right || K==EKeys::D || K==EKeys::Gamepad_RightShoulder)){JournalWidget->CycleTab(1);Cue(TEXT("ui_select"));}
        else if(Params.Event==IE_Pressed && (K==EKeys::Left || K==EKeys::A || K==EKeys::Gamepad_LeftShoulder)){JournalWidget->CycleTab(-1);Cue(TEXT("ui_select"));}
        else if(Params.Event==IE_Pressed && (K==EKeys::Escape || K==EKeys::BackSpace || K==EKeys::Gamepad_FaceButton_Right || K==EKeys::Gamepad_Special_Right))CloseJournal();
        return true;
    }
    if(CodexWidget)
    {
        if(Params.Event!=IE_Pressed && Params.Event!=IE_Repeat)return true;
        const FKey K=Params.Key;
        if(K==EKeys::Up || K==EKeys::W || K==EKeys::Gamepad_DPad_Up){CodexWidget->Move(-1);Cue(TEXT("ui_hover"));}
        else if(K==EKeys::Down || K==EKeys::S || K==EKeys::Gamepad_DPad_Down){CodexWidget->Move(1);Cue(TEXT("ui_hover"));}
        else if(Params.Event==IE_Pressed && (K==EKeys::Tab || K==EKeys::Left || K==EKeys::Right || K==EKeys::A || K==EKeys::D || K==EKeys::Gamepad_LeftShoulder || K==EKeys::Gamepad_RightShoulder))
        {CodexWidget->SetTab(1-CodexWidget->GetTab());Cue(TEXT("ui_select"));}
        else if(Params.Event==IE_Pressed && (K==EKeys::Escape || K==EKeys::BackSpace || K==EKeys::Gamepad_FaceButton_Right || K==EKeys::Gamepad_Special_Right))CloseCodex();
        return true;
    }
    if(AchievementsWidget)
    {
        if(Params.Event!=IE_Pressed && Params.Event!=IE_Repeat)return true;
        const FKey K=Params.Key;
        if(K==EKeys::Up || K==EKeys::W || K==EKeys::Gamepad_DPad_Up)AchievementsWidget->Scroll(-1);
        else if(K==EKeys::Down || K==EKeys::S || K==EKeys::Gamepad_DPad_Down)AchievementsWidget->Scroll(1);
        else if(K==EKeys::PageUp)AchievementsWidget->Scroll(-AchievementsWidget->GetVisibleRows());
        else if(K==EKeys::PageDown)AchievementsWidget->Scroll(AchievementsWidget->GetVisibleRows());
        else if(Params.Event==IE_Pressed && (K==EKeys::Escape || K==EKeys::BackSpace || K==EKeys::Gamepad_FaceButton_Right || K==EKeys::Gamepad_Special_Right))CloseAchievements();
        return true;
    }
    // S318 game over: it holds every key; there is no escape from it, as in the source.
    if(GameOverWidget)
    {
        if(Params.Event!=IE_Pressed)return true;
        const FKey K=Params.Key;
        if(K==EKeys::Up || K==EKeys::W || K==EKeys::Gamepad_DPad_Up){GameOverWidget->Navigate(-1);Cue(TEXT("ui_hover"));}
        else if(K==EKeys::Down || K==EKeys::S || K==EKeys::Gamepad_DPad_Down){GameOverWidget->Navigate(1);Cue(TEXT("ui_hover"));}
        else if(ConfirmKey || K==EKeys::J)GameOverWidget->Confirm();
        return true;
    }
    // S317 pause menu: it holds every key while open (the world is paused behind it).
    if(PauseWidget)
    {
        if(Params.Event!=IE_Pressed)return true;
        const FKey K=Params.Key;
        if(K==EKeys::Up || K==EKeys::W || K==EKeys::Gamepad_DPad_Up){PauseWidget->Navigate(-1);Cue(TEXT("ui_hover"));}
        else if(K==EKeys::Down || K==EKeys::S || K==EKeys::Gamepad_DPad_Down){PauseWidget->Navigate(1);Cue(TEXT("ui_hover"));}
        else if(K==EKeys::Left || K==EKeys::A || K==EKeys::Gamepad_DPad_Left)PauseWidget->Adjust(-1);
        else if(K==EKeys::Right || K==EKeys::D || K==EKeys::Gamepad_DPad_Right)PauseWidget->Adjust(1);
        else if(ConfirmKey || K==EKeys::J){Cue(TEXT("ui_select"));PauseWidget->Confirm();}
        else if(K==EKeys::Escape || K==EKeys::BackSpace || K==EKeys::Gamepad_FaceButton_Right || K==EKeys::Gamepad_Special_Right){if(!PauseWidget->Back())ClosePause();}
        return true;
    }
    // S312 memory burn picker: it holds every key while open. 1-9 or Up/Down/wheel choose, R, Enter or a
    // click burns (grade 2 and 1 ask twice), Esc or a right click lets the fire go out.
    if(auto* Combat=GetWorld()?GetWorld()->GetSubsystem<UMemoriaFieldCombatSubsystem>():nullptr; Combat && Combat->IsPickingBurn())
    {
        if(Params.Event!=IE_Pressed)return true;
        static const FKey Digits[9]={EKeys::One,EKeys::Two,EKeys::Three,EKeys::Four,EKeys::Five,EKeys::Six,EKeys::Seven,EKeys::Eight,EKeys::Nine};
        for(int32 I=0;I<9;++I)if(Params.Key==Digits[I]){Combat->SelectBurn(I);return true;}
        if(Params.Key==EKeys::Up || Params.Key==EKeys::W || Params.Key==EKeys::MouseScrollUp)Combat->MoveBurnSelection(-1);
        else if(Params.Key==EKeys::Down || Params.Key==EKeys::S || Params.Key==EKeys::MouseScrollDown)Combat->MoveBurnSelection(1);
        else if(Params.Key==EKeys::R || Params.Key==EKeys::LeftMouseButton || ConfirmKey)Combat->ConfirmBurn();
        else if(Params.Key==EKeys::Escape || Params.Key==EKeys::RightMouseButton || Params.Key==EKeys::Gamepad_FaceButton_Right){Combat->CloseBurnPicker();Cue(TEXT("cancel"));}
        return true;
    }
    const bool ArchiveToggle=Params.Key==EKeys::Tab || Params.Key==EKeys::M;
    // S315: releasing the attack ends a charge; releasing the guard lowers it (in any state, so it never sticks).
    if(Params.Event==IE_Released)
        if(auto* Combat=GetWorld()?GetWorld()->GetSubsystem<UMemoriaFieldCombatSubsystem>():nullptr)
        {
            if(Params.Key==EKeys::LeftMouseButton || Params.Key==EKeys::J)Combat->EndCharge();
            if(Params.Key==EKeys::RightMouseButton || Params.Key==EKeys::K)Combat->EndBlock();
        }
    // S311 field combat: left click or J attacks toward the cursor (held: the heavy cut, S315), right click or K guards (S315),
    // Shift dodges, R burns a memory (S312), F9 calls a husk, F10 a thief (development).
    if(!ArchiveWidget && Params.Event==IE_Pressed && Host()->GetState()==EMemoriaSliceState::Exploration && !IsModalOpen())
    {
        auto* Combat=GetWorld()?GetWorld()->GetSubsystem<UMemoriaFieldCombatSubsystem>():nullptr;
        if(Combat && Combat->GetPlayer() && GetPawn())
        {
            if(Params.Key==EKeys::LeftMouseButton || Params.Key==EKeys::J){Combat->RequestAttack(CursorOnFloor());Combat->BeginCharge();return true;}
            if(Params.Key==EKeys::RightMouseButton || Params.Key==EKeys::K){Combat->BeginBlock();return true;}
            if(Params.Key==EKeys::LeftShift || Params.Key==EKeys::RightShift){Combat->RequestDash(GetPawn()->GetLastMovementInputVector().IsNearlyZero()?GetPawn()->GetVelocity():GetPawn()->GetLastMovementInputVector());return true;}
            if(Params.Key==EKeys::F9){Combat->SpawnWave(1,GetPawn()->GetActorLocation(),380.f);return true;}
            if(Params.Key==EKeys::F10){Combat->SpawnWave(1,GetPawn()->GetActorLocation(),380.f,EMemoriaFoeKind::MarketThief);return true;}
            if(Params.Key==EKeys::R){Combat->OpenBurnPicker();return true;}
            // S319: Elia's techniques on 1-4.
            static const FKey Techniques[4]={EKeys::One,EKeys::Two,EKeys::Three,EKeys::Four};
            for(int32 I=0;I<4;++I)if(Params.Key==Techniques[I]){if(!Combat->UseEliaSkill(I))Cue(TEXT("cancel"));return true;}
        }
    }
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
    if(Params.Key==EKeys::Escape && Params.Event==IE_Pressed && OpenPause())return true;
    return Super::InputKey(Params);
}
void AMemoriaSliceController::UpdateHints(UMemoriaFieldCombatSubsystem* Combat)
{
    auto* Tutorial=GetGameInstance()->GetSubsystem<UMemoriaTutorialSubsystem>();
    if(!Tutorial)return;
    // battle_manager.gd's moments in the action field: the first foes, the first burn, the first status on Arrel.
    if(Combat && Combat->GetPlayer() && !Combat->IsDefeated())
    {
        auto* Narrative=Host();
        auto Show=[&](const TCHAR* Id){ if(Tutorial->ShowHint(Id) && Narrative)Narrative->Record(FString(TEXT("hint:"))+Id); };
        if(Combat->LiveMonsterCount()>0)Show(TEXT("first_battle"));
        if(Combat->GetBurns()>0)Show(TEXT("first_burn"));
        if(Combat->IsPoisoned() || Combat->IsWeakened())Show(TEXT("first_status_effect"));
    }
    Tutorial->Advance(FApp::GetDeltaTime());
    if(Tutorial->IsShowing() && !HintWidget)
    {
        HintWidget=CreateWidget<UMemoriaHintWidget>(this,UMemoriaHintWidget::StaticClass());
        HintWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
        HintWidget->AddToViewport(65); // over the pause menu (60), under the game over screen (70)
    }
    if(HintWidget)HintWidget->Bind(Tutorial,GetGameInstance()->GetSubsystem<UMemoriaSettingsSubsystem>()->GetLocale()!=TEXT("en"));
}
void AMemoriaSliceController::UpdateAchievements()
{
    auto* Achievements=GetGameInstance()->GetSubsystem<UMemoriaAchievementSubsystem>();
    if(!Achievements)return;
    Achievements->Observe(*GetGameInstance()->GetSubsystem<UMemoriaRunSubsystem>());
    if(auto* Codex=GetGameInstance()->GetSubsystem<UMemoriaCodexSubsystem>())Codex->Observe(*GetGameInstance()->GetSubsystem<UMemoriaRunSubsystem>());
    Achievements->Advance(FApp::GetDeltaTime());
    if(!Achievements->GetPopup().IsEmpty() && !AchievementPopup)
    {
        // Layer 95 in the source: over everything.
        AchievementPopup=CreateWidget<UMemoriaAchievementPopupWidget>(this,UMemoriaAchievementPopupWidget::StaticClass());
        AchievementPopup->SetVisibility(ESlateVisibility::HitTestInvisible);
        AchievementPopup->Bind(Achievements);
        AchievementPopup->AddToViewport(95);
    }
}
void AMemoriaSliceController::CloseJournal()
{
    if(!JournalWidget)return;
    JournalWidget->RemoveFromParent(); JournalWidget=nullptr;
    Cue(TEXT("cancel"));
}
void AMemoriaSliceController::CloseCodex()
{
    if(!CodexWidget)return;
    CodexWidget->RemoveFromParent(); CodexWidget=nullptr;
    Cue(TEXT("cancel"));
}
void AMemoriaSliceController::CloseAchievements()
{
    if(!AchievementsWidget)return;
    AchievementsWidget->RemoveFromParent(); AchievementsWidget=nullptr;
    Cue(TEXT("cancel"));
}
void AMemoriaSliceController::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds); auto* Narrative = Host();
    if (!Narrative) return;
    // S318: once Arrel's fall has played out, game_over.gd's choice.
    if(auto* Combat=GetWorld()?GetWorld()->GetSubsystem<UMemoriaFieldCombatSubsystem>():nullptr; Combat && Combat->IsAwaitingGameOver() && !GameOverWidget)
    {
        ClosePause();
        auto* Game=GetGameInstance();
        GameOverWidget=CreateWidget<UMemoriaGameOverWidget>(this,UMemoriaGameOverWidget::StaticClass());
        GameOverWidget->Configure(Game->GetSubsystem<UMemoriaCheckpointSubsystem>()->FindContinue()!=EMemoriaContinueSource::None,
            Game->GetSubsystem<UMemoriaSettingsSubsystem>()->GetLocale()!=TEXT("en"));
        GameOverWidget->OnAction.BindUObject(this,&AMemoriaSliceController::GameOverAction);
        GameOverWidget->AddToViewport(70);
        FInputModeGameAndUI Mode; Mode.SetHideCursorDuringCapture(false); SetInputMode(Mode); bShowMouseCursor=true;
        Narrative->Record(TEXT("combat:game_over"));
    }
    UpdateHints(GetWorld()?GetWorld()->GetSubsystem<UMemoriaFieldCombatSubsystem>():nullptr);
    UpdateAchievements();
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
    if (Narrative->IsVerdanRevisit() && GetPawn())
    {
        if (!bEncounterInitialized) { Encounter.Reset(EncounterRng.Real(60,100)); bEncounterInitialized=true; }
        // S328: a fight in the field holds the encounter distance, as the source's separate battle scene did.
        const auto* FieldFight=GetWorld()->GetSubsystem<UMemoriaFieldCombatSubsystem>();
        const bool Exploring=Narrative->GetState()==EMemoriaSliceState::Exploration && !IsModalOpen() && !(FieldFight && FieldFight->LiveMonsterCount()>0);
        const auto Step=Encounter.Advance(Memoria::Coordinates::ToSource(GetPawn()->GetActorLocation()),Exploring,EncounterRng);
        const bool PressureChanged=(EncounterPressure>=.5)!=(Step.Pressure>=.5);
        EncounterPressure=Step.Pressure;
        if (PressureChanged) LastRevision=INDEX_NONE;
        if (Step.bWarningStarted) { Narrative->Record(TEXT("encounter:warning")); LastRevision=INDEX_NONE; }
        if (Step.bTriggered)
        {
            // S311: foes rise in the field around Arrel (S329: the turn-based battle is retired).
            // S313: the source pool's Market Thief (index 1) comes as two thieves; the Alley Rat has no model
            // yet, so void husks stand in for it.
            auto* Combat=GetWorld()->GetSubsystem<UMemoriaFieldCombatSubsystem>();
            if (Combat && Combat->GetPlayer())
            {
                const bool bThief=Step.EnemyIndex%2==1;
                Combat->SpawnWave(bThief?2:3,GetPawn()->GetActorLocation(),420.f,bThief?EMemoriaFoeKind::MarketThief:EMemoriaFoeKind::VoidHusk);
                GetGameInstance()->GetSubsystem<UMemoriaRunSubsystem>()->RecordBattleStarted();
                Narrative->Record(TEXT("encounter:field_started"));
            }
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
        if (const auto* Chapter = Narrative->GetChapterMap().IsEmpty() ? nullptr : MemoriaChapterMaps::Find(Narrative->GetChapterMap()))
            Status.Header = FString::Printf(TEXT("%s  /  %s"), *Chapter->TitleName.ToUpper(), *Chapter->Subtitle.ToUpper());
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
            // The cursor aims attacks in the field (Diablo-style).
            SetInputMode(FInputModeGameOnly()); bShowMouseCursor = true;
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
    auto* Combat=GetWorld()?GetWorld()->GetSubsystem<UMemoriaFieldCombatSubsystem>():nullptr;
    if (Combat && Combat->GetPlayer() && !Combat->CanMove()) return;
    if (Host()->GetState() == EMemoriaSliceState::Exploration) Super::Move(Value);
}
FVector AMemoriaSliceController::CursorOnFloor() const
{
    // The cursor ray meets the floor plane at Arrel's height; without a pointer, aim where he faces.
    const APawn* Self=GetPawn(); if(!Self) return FVector::ZeroVector;
    FVector Origin,Direction;
    if(DeprojectMousePositionToWorld(Origin,Direction) && FMath::Abs(Direction.Z)>1e-3)
    {
        const double T=(Self->GetActorLocation().Z-Origin.Z)/Direction.Z;
        if(T>0) return Origin+Direction*T;
    }
    const auto* Combat=GetWorld()?GetWorld()->GetSubsystem<UMemoriaFieldCombatSubsystem>():nullptr;
    const auto* Figure=Combat?Combat->GetPlayerFigure():nullptr;
    const float Yaw=Figure?Figure->GetYaw():0.f;
    return Self->GetActorLocation()+FRotator(0,Yaw,0).Vector()*100.f;
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
    if (bAwaitConfirmRelease || ArchiveWidget) return;
    // Source dialogue_box: ui_select when a choice is pressed, confirm when a line advances.
    Cue(Host()->GetView().Choices.IsEmpty() ? TEXT("confirm") : TEXT("ui_select"));
    const auto Before = Host()->GetState(); Host()->Confirm(OriginalIndex);
    if (Before != Host()->GetState()) bAwaitConfirmRelease = true;
}
void AMemoriaSliceController::Confirm()
{
    if (bAwaitConfirmRelease || ArchiveWidget || TitleWidget) return;
    if (NarrativeWidget) NarrativeWidget->ConfirmIntent();
    else if (Host()->GetState() == EMemoriaSliceState::Exploration && !IsModalOpen())
    {
        if (Interaction->Interact(GetPawn())) bAwaitConfirmRelease = true;
    }
    else Super::Confirm();
}
void AMemoriaSliceController::Back()
{
    if(PauseWidget){if(!PauseWidget->Back())ClosePause();return;}
    if(TitleWidget){TitleWidget->Back();return;}
    if(ArchiveWidget){CloseArchive();return;}
    if (NarrativeWidget) Host()->Back(); else Super::Back();
}
void AMemoriaSliceController::OpenModal()
{
    if(TitleWidget)return;
    if(ArchiveWidget){CloseArchive();return;}
    if (NarrativeWidget) Host()->Back();
    else if (Host()->GetState() == EMemoriaSliceState::Exploration && !OpenPause() && !PauseWidget) Super::OpenModal();
}
void AMemoriaSliceController::EndPlay(const EEndPlayReason::Type Reason)
{
    ClearTitle();
    if(ArchiveWidget){ArchiveWidget->OnConsumedKey.Unbind();ArchiveWidget->OnClose.Unbind();ArchiveWidget->BindRun(nullptr);ArchiveWidget=nullptr;}
    if (NarrativeWidget) NarrativeWidget->OnConfirm.Unbind();
    if (StatusWidget) StatusWidget->RemoveFromParent(); StatusWidget = nullptr;
    NarrativeWidget = nullptr; Super::EndPlay(Reason);
}

bool AMemoriaSliceController::OpenPause()
{
    auto* Narrative=Host();
    if(PauseWidget || TitleWidget || ArchiveWidget || NarrativeWidget || !Narrative || Narrative->GetState()!=EMemoriaSliceState::Exploration || IsModalOpen())return false;
    if(auto* Combat=GetWorld()?GetWorld()->GetSubsystem<UMemoriaFieldCombatSubsystem>():nullptr; Combat && (Combat->IsPickingBurn() || Combat->IsDefeated()))return false;
    auto* Game=GetGameInstance();
    PauseWidget=CreateWidget<UMemoriaPauseWidget>(this,UMemoriaPauseWidget::StaticClass());
    PauseWidget->Configure(Game->GetSubsystem<UMemoriaSettingsSubsystem>(),Game->GetSubsystem<UMemoriaCheckpointSubsystem>()->CanSaveClosedBoundary(),
        Game->GetSubsystem<UMemoriaCheckpointSubsystem>()->FindContinue()!=EMemoriaContinueSource::None,PauseInfo());
    PauseWidget->OnAction.BindUObject(this,&AMemoriaSliceController::PauseAction);
    PauseWidget->AddToViewport(60);
    // get_tree().paused: the world stops behind the menu; the mouse works on its rows.
    UGameplayStatics::SetGamePaused(this,true);
    FInputModeGameAndUI Mode; Mode.SetHideCursorDuringCapture(false); SetInputMode(Mode); bShowMouseCursor=true;
    Cue(TEXT("ui_open"));
    return true;
}
void AMemoriaSliceController::ClosePause()
{
    if(!PauseWidget)return;
    PauseWidget->OnAction.Unbind(); PauseWidget->RemoveFromParent(); PauseWidget=nullptr;
    UGameplayStatics::SetGamePaused(this,false);
    SetInputMode(FInputModeGameOnly()); bShowMouseCursor=true;
    FSlateApplication::Get().SetAllUserFocusToGameViewport();
    Cue(TEXT("ui_close"));
}
FString AMemoriaSliceController::PauseInfo() const
{
    // _update_save_info: the chapter and place, then the run's numbers, in the menu's language.
    auto* Game=GetGameInstance();
    const auto* Run=Game->GetSubsystem<UMemoriaRunSubsystem>();
    const bool Ko=Game->GetSubsystem<UMemoriaSettingsSubsystem>()->GetLocale()!=TEXT("en");
    if(!Run || !Run->HasActiveRun())return FString();
    const auto State=Run->GetRunSnapshot();
    int64 Held=0,Burned=0;
    if(const auto* Memory=Run->GetPlayerMemory())
    {
        const auto Snapshot=Memory->GetSnapshot();
        for(const auto& M:Snapshot.Owned)if(!M.bBurned && !M.bFaded)++Held;
        Burned=Snapshot.BurnedHistory.Num();
    }
    return Ko?FString::Printf(TEXT("%lld장 — 베르단 시장\n기억  보유 %lld · 연소 %lld\nHP %lld / %lld    Grains %lld"),State.CurrentChapter,Held,Burned,State.Player.Hp,State.Player.MaxHp,State.Player.Grains)
        :FString::Printf(TEXT("Chapter %lld — Verdan Market\nMemories: %lld held, %lld burned\nHP %lld / %lld    Grains %lld"),State.CurrentChapter,Held,Burned,State.Player.Hp,State.Player.MaxHp,State.Player.Grains);
}
void AMemoriaSliceController::PauseAction(EMemoriaPauseAction Action)
{
    auto* Game=GetGameInstance();
    auto* Checkpoint=Game->GetSubsystem<UMemoriaCheckpointSubsystem>();
    const bool Ko=Game->GetSubsystem<UMemoriaSettingsSubsystem>()->GetLocale()!=TEXT("en");
    switch(Action)
    {
    case EMemoriaPauseAction::Resume: ClosePause(); break;
    case EMemoriaPauseAction::Save:
    {
        // The field checkpoint (a closed boundary) at Arrel's place, as the market's save point writes it.
        const bool bSaved=GetPawn() && Checkpoint->SaveClosedBoundary(Memoria::Coordinates::ToSource(GetPawn()->GetActorLocation()));
        PauseWidget->SetNotice(bSaved?(Ko?TEXT("저장했습니다."):TEXT("Saved.")):(Ko?TEXT("저장할 수 없습니다."):TEXT("Could not save.")));
        PauseWidget->SetCanLoad(Checkpoint->FindContinue()!=EMemoriaContinueSource::None);
        Cue(bSaved?TEXT("confirm"):TEXT("cancel"));
        break;
    }
    case EMemoriaPauseAction::Load:
    {
        // The title's Continue, in place: the newest valid slot replaces the live run.
        ClosePause(); LoadNewest();
        break;
    }
    case EMemoriaPauseAction::Journal:
        // pause_menu.gd _on_journal: over the menu, which stays open beneath it.
        JournalWidget=CreateWidget<UMemoriaJournalWidget>(this,UMemoriaJournalWidget::StaticClass());
        JournalWidget->Configure(Game->GetSubsystem<UMemoriaRunSubsystem>(),Ko);
        JournalWidget->AddToViewport(62);
        FSlateApplication::Get().SetAllUserFocusToGameViewport();
        Cue(TEXT("ui_select"));
        break;
    case EMemoriaPauseAction::Codex:
        // pause_menu.gd _on_codex: over the menu, which stays open beneath it.
        CodexWidget=CreateWidget<UMemoriaCodexWidget>(this,UMemoriaCodexWidget::StaticClass());
        CodexWidget->Configure(Game->GetSubsystem<UMemoriaCodexSubsystem>(),Ko);
        CodexWidget->AddToViewport(62);
        FSlateApplication::Get().SetAllUserFocusToGameViewport();
        Cue(TEXT("ui_select"));
        break;
    case EMemoriaPauseAction::Achievements:
        // pause_menu.gd _show_achievements_panel: over the menu, which stays open beneath it.
        AchievementsWidget=CreateWidget<UMemoriaAchievementsWidget>(this,UMemoriaAchievementsWidget::StaticClass());
        AchievementsWidget->Configure(Game->GetSubsystem<UMemoriaAchievementSubsystem>(),Ko);
        AchievementsWidget->AddToViewport(62);
        FSlateApplication::Get().SetAllUserFocusToGameViewport();
        Cue(TEXT("ui_select"));
        break;
    case EMemoriaPauseAction::Title:
        ClosePause();
        UGameplayStatics::OpenLevel(this,TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice"),true,TEXT("Title"));
        break;
    case EMemoriaPauseAction::Quit:
        UE_LOG(LogTemp,Display,TEXT("MEMORIA_PAUSE quit"));
        if(!GIsAutomationTesting)UKismetSystemLibrary::QuitGame(this,this,EQuitPreference::Quit,false);
        break;
    case EMemoriaPauseAction::Language:
        // The run speaks the menu's language from here on, as the source's single language setting does.
        Game->GetSubsystem<UMemoriaRunSubsystem>()->SetLocale(Game->GetSubsystem<UMemoriaSettingsSubsystem>()->GetLocale());
        PauseWidget->SetInfo(PauseInfo()); LastRevision=INDEX_NONE;
        break;
    default: break;
    }
}
void AMemoriaSliceController::LoadNewest()
{
    // The title's Continue, in place: the newest valid slot replaces the live run.
    const auto Source=GetGameInstance()->GetSubsystem<UMemoriaCheckpointSubsystem>()->FindContinue();
    if(auto* Combat=GetWorld()?GetWorld()->GetSubsystem<UMemoriaFieldCombatSubsystem>():nullptr)Combat->Revive(1.f);
    if(Source==EMemoriaContinueSource::Boundary)Host()->ContinueCheckpoint();
    else if(Source==EMemoriaContinueSource::Chapter)Host()->ResumeChapterAutosave();
    LastRevision=INDEX_NONE;
}
void AMemoriaSliceController::CloseGameOver()
{
    if(!GameOverWidget)return;
    GameOverWidget->OnAction.Unbind(); GameOverWidget->RemoveFromParent(); GameOverWidget=nullptr;
    SetInputMode(FInputModeGameOnly()); bShowMouseCursor=true;
    FSlateApplication::Get().SetAllUserFocusToGameViewport();
}
void AMemoriaSliceController::GameOverAction(EMemoriaGameOverAction Action)
{
    auto* Combat=GetWorld()?GetWorld()->GetSubsystem<UMemoriaFieldCombatSubsystem>():nullptr;
    switch(Action)
    {
    case EMemoriaGameOverAction::StaggerOn:
        // _on_retry: HP to 30% and back to the field; the husks are gone.
        Cue(TEXT("ui_select")); CloseGameOver();
        if(Combat)Combat->Revive(.3f);
        LastRevision=INDEX_NONE;
        break;
    case EMemoriaGameOverAction::Load:
        // _on_load: without a save, only the cancel sound.
        if(GetGameInstance()->GetSubsystem<UMemoriaCheckpointSubsystem>()->FindContinue()==EMemoriaContinueSource::None){Cue(TEXT("cancel"));break;}
        Cue(TEXT("ui_select")); CloseGameOver(); LoadNewest();
        break;
    case EMemoriaGameOverAction::Title:
        Cue(TEXT("ui_select")); CloseGameOver();
        UGameplayStatics::OpenLevel(this,TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice"),true,TEXT("Title"));
        break;
    }
}
