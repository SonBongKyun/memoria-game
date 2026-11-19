#include "Framework/MemoriaCombatInputRouter.h"
#include "Framework/MemoriaSliceHost.h"
#include "Combat/MemoriaFieldCombatSubsystem.h"
#include "Narrative/MemoriaNarrativeSubsystem.h"
#include "Presentation/MemoriaFieldCharacterComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "Engine/LocalPlayer.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

UMemoriaCombatInputRouter::UMemoriaCombatInputRouter()
{
    PrimaryComponentTick.bCanEverTick=true;
    PrimaryComponentTick.bTickEvenWhenPaused=true;
}
FKey UMemoriaCombatInputRouter::KeyFor(ECommand Command)
{
    static const FKey Keys[]={EKeys::Gamepad_RightShoulder,EKeys::Gamepad_LeftShoulder,
        EKeys::Gamepad_FaceButton_Right,EKeys::Gamepad_FaceButton_Top,
        EKeys::Gamepad_LeftTrigger,EKeys::Gamepad_RightTrigger,EKeys::Gamepad_DPad_Up,
        EKeys::Gamepad_DPad_Right,EKeys::Gamepad_DPad_Down,EKeys::Gamepad_DPad_Left,EKeys::Gamepad_RightThumbstick};
    return Command>=Attack && Command<=Ink ? Keys[Command] : FKey();
}
bool UMemoriaCombatInputRouter::Initialize(AMemoriaSliceController* InController,UEnhancedInputComponent* Input)
{
    if (Context) return true;
    if (!InController || !Input || !InController->GetLocalPlayer()) return false;
    Controller=InController; Enhanced=Input;
    Context=NewObject<UInputMappingContext>(this,TEXT("IMC_RuntimeFieldCombat"));
    const TCHAR* Names[]={TEXT("Attack"),TEXT("Guard"),TEXT("Dodge"),TEXT("Burn"),TEXT("Skills"),TEXT("Items"),TEXT("SlotUp"),TEXT("SlotRight"),TEXT("SlotDown"),TEXT("SlotLeft"),TEXT("Ink")};
    for (int32 I=Attack;I<=Ink;++I) AddButton(Input,Names[I],static_cast<ECommand>(I));
    auto* Action=NewObject<UInputAction>(this,TEXT("IA_RuntimeCombatAim"));
    Action->ValueType=EInputActionValueType::Axis2D; Action->bConsumeInput=true; Actions.Add(Action);
    Action->AccumulationBehavior=EInputActionAccumulationBehavior::Cumulative;
    Context->MapKey(Action,EKeys::Gamepad_RightX);
    auto& MappingY=Context->MapKey(Action,EKeys::Gamepad_RightY);
    auto* Swizzle=NewObject<UInputModifierSwizzleAxis>(this);
    Swizzle->Order=EInputAxisSwizzle::YXZ; MappingY.Modifiers.Add(Swizzle);
    auto* DeadZone=NewObject<UInputModifierDeadZone>(this);
    DeadZone->Type=EDeadZoneType::Radial; DeadZone->LowerThreshold=.23f; DeadZone->UpperThreshold=1.f;
    Action->Modifiers.Add(DeadZone); // radial dead zone after the X/Y mappings combine
    BindingHandles.Add(Input->BindAction(Action,ETriggerEvent::Triggered,this,&UMemoriaCombatInputRouter::Aim).GetHandle());
    RefreshContext();
    return true;
}
void UMemoriaCombatInputRouter::AddButton(UEnhancedInputComponent* Input,const TCHAR* Name,ECommand Command)
{
    auto* Action=NewObject<UInputAction>(this,FName(*(FString(TEXT("IA_RuntimeCombat"))+Name)));
    Action->ValueType=EInputActionValueType::Boolean; Action->bConsumeInput=true; Actions.Add(Action);
    Context->MapKey(Action,KeyFor(Command));
    BindingHandles.Add(Input->BindAction(Action,ETriggerEvent::Started,this,&UMemoriaCombatInputRouter::Started,int32(Command)).GetHandle());
    BindingHandles.Add(Input->BindAction(Action,ETriggerEvent::Completed,this,&UMemoriaCombatInputRouter::Finished,int32(Command)).GetHandle());
    BindingHandles.Add(Input->BindAction(Action,ETriggerEvent::Canceled,this,&UMemoriaCombatInputRouter::Finished,int32(Command)).GetHandle());
}
UMemoriaFieldCombatSubsystem* UMemoriaCombatInputRouter::Combat() const
{ return GetWorld() ? GetWorld()->GetSubsystem<UMemoriaFieldCombatSubsystem>() : nullptr; }
bool UMemoriaCombatInputRouter::IsHigherModal() const
{
    const auto* C=Controller.Get();
    return !C || C->IsModalOpen() || C->GetTitleWidget() || C->GetPauseWidget() || C->GetArchiveWidget() ||
        C->GetNarrativeWidget() || C->GetGameOverWidget() || C->GetJournalWidget() || C->GetCodexWidget() || C->GetAchievementsWidget();
}
bool UMemoriaCombatInputRouter::IsFieldAllowed() const
{
    const auto* C=Controller.Get(); const auto* B=Combat();
    auto* Narrative=C && C->GetGameInstance() ? C->GetGameInstance()->GetSubsystem<UMemoriaNarrativeSubsystem>() : nullptr;
    return C && C->GetPawn() && B && B->GetPlayer() && !IsHigherModal() && !GetWorld()->IsPaused() &&
        Narrative && Narrative->GetState()==EMemoriaSliceState::Exploration && !B->IsPickingBurn() && !B->IsDefeated();
}
bool UMemoriaCombatInputRouter::ClaimsRawKey(FKey Key) const
{
    for (int32 I=Attack;I<=Ink;++I) if (Key==KeyFor(static_cast<ECommand>(I))) return true;
    return Key==EKeys::Gamepad_Right2D || Key==EKeys::Gamepad_RightX || Key==EKeys::Gamepad_RightY;
}
bool UMemoriaCombatInputRouter::ShouldFeedEnhanced() const
{ return IsFieldAllowed() || (!IsHigherModal() && Combat() && Combat()->IsPickingBurn()); }
void UMemoriaCombatInputRouter::ObservePhysicalKey(FKey Key,EInputEvent Event)
{
    // Releases, repeats and pointer motion cannot steal a device preference.
    if (Key.IsValid() && (Event==IE_Pressed || Event==IE_DoubleClick)) bUsingGamepad=Key.IsGamepadKey();
    // Remove the combat context before a menu's first raw event reaches its
    // own Enhanced Input bindings (not merely after the next world tick).
    RefreshContext();
    if (Event==IE_Released) { PhysicalDown.Remove(Key); BlockedUntilRelease.Remove(Key); }
    else if (Event==IE_Pressed && ClaimsRawKey(Key))
    {
        PhysicalDown.Add(Key); bGamepadAim=true;
        if (!ShouldFeedEnhanced()) BlockedUntilRelease.Add(Key);
    }
    else if (Event==IE_Pressed && !Key.IsGamepadKey()) bGamepadAim=false;
}
void UMemoriaCombatInputRouter::TrackGamepadAxes()
{
    const auto* C=Controller.Get(); if (!C) return;
    static const FKey Axes[]={EKeys::Gamepad_LeftX,EKeys::Gamepad_LeftY,EKeys::Gamepad_RightX,
        EKeys::Gamepad_RightY,EKeys::Gamepad_LeftTriggerAxis,EKeys::Gamepad_RightTriggerAxis};
    for (int32 I=0;I<UE_ARRAY_COUNT(Axes);++I)
    {
        const float Value=C->GetInputAnalogKeyState(Axes[I]);
        if (FMath::Abs(Value)<=.23f) { LastDeviceAxes[I]=0.f; continue; }
        // Count a meaningful physical change, not a resting/held axis poll.
        // Small gradual motion accumulates relative to the last noted value.
        if (FMath::Abs(Value-LastDeviceAxes[I])>.08f) { LastDeviceAxes[I]=Value; bUsingGamepad=true; }
    }
}
void UMemoriaCombatInputRouter::RefreshContext()
{
    const bool bWant=Context && ShouldFeedEnhanced();
    if (bWant==bContextActive) return;
    auto* C=Controller.Get();
    auto* Local=C && C->GetLocalPlayer() ? C->GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
    if (!Local) return;
    FModifyContextOptions Options; Options.bIgnoreAllPressedKeysUntilRelease=true; Options.bForceImmediately=true;
    if (bWant) Local->AddMappingContext(Context,20,Options); // below modal priority100
    else Local->RemoveMappingContext(Context,Options);
    bContextActive=bWant;
}
bool UMemoriaCombatInputRouter::CanStart(ECommand Command) const
{ return IsFieldAllowed() && !BlockedUntilRelease.Contains(KeyFor(Command)); }
int32 UMemoriaCombatInputRouter::LayeredSlot(int32 Direction,bool SkillLayer,bool ItemLayer)
{ return Direction<0 || Direction>3 ? INDEX_NONE : SkillLayer ? Direction : ItemLayer ? Direction+4 : INDEX_NONE; }
void UMemoriaCombatInputRouter::Started(const FInputActionValue& Value,int32 Raw)
{
    const ECommand Command=static_cast<ECommand>(Raw); auto* B=Combat();
    if (!B || BlockedUntilRelease.Contains(KeyFor(Command)) || IsHigherModal()) return;
    bGamepadAim=true;
    if (B->IsPickingBurn())
    {
        if (Command==Burn) B->ConfirmBurn();
        else if (Command==Dodge) B->CloseBurnPicker();
        else if (Command==Up || Command==Left) B->MoveBurnSelection(-1);
        else if (Command==Down || Command==Right) B->MoveBurnSelection(1);
        return;
    }
    if (!CanStart(Command)) return;
    bFieldWasAllowed=true;
    // Read reconciled physical trigger state too: pressing a trigger and a
    // direction in the same input frame must not depend on callback order.
    const bool SkillHeld=!BlockedUntilRelease.Contains(KeyFor(Skills)) &&
        (bSkills || Controller->IsInputKeyDown(KeyFor(Skills)));
    const bool ItemHeld=!BlockedUntilRelease.Contains(KeyFor(Items)) &&
        (bItems || Controller->IsInputKeyDown(KeyFor(Items)));
    switch (Command)
    {
    case Attack: bAttack=true; B->RequestAttack(AimPoint()); B->BeginCharge(); break;
    case Guard: bGuard=true; B->BeginBlock(); break;
    case Dodge:
    {
        const APawn* Pawn=Controller->GetPawn();
        const FVector Move=Pawn->GetLastMovementInputVector();
        B->RequestDash(Move.IsNearlyZero() ? AimPoint()-Pawn->GetActorLocation() : Move); break;
    }
    case Burn: B->OpenBurnPicker(); CancelHeld(); break;
    case Skills: bSkills=true; break;
    case Items: bItems=true; break;
    case Ink: if (ItemHeld && !SkillHeld) B->UseQuickItem(4,AimPoint()); break;
    default:
    {
        const int32 Slot=LayeredSlot(Command-Up,SkillHeld,ItemHeld);
        if (Slot>=0 && Slot<4) B->UseEliaSkill(Slot);
        else if (Slot>=4) B->UseQuickItem(Slot-4,AimPoint());
        break;
    }
    }
}
void UMemoriaCombatInputRouter::Finished(const FInputActionValue& Value,int32 Raw)
{
    const ECommand Command=static_cast<ECommand>(Raw); auto* B=Combat();
    if (Command==Attack && bAttack) { if (B) { if (IsFieldAllowed()) B->EndCharge(); else B->CancelHeldInput(); } bAttack=false; }
    if (Command==Guard && bGuard) { if (B) B->EndBlock(); bGuard=false; }
    if (Command==Skills) bSkills=false;
    if (Command==Items) bItems=false;
}
void UMemoriaCombatInputRouter::Aim(const FInputActionValue& Value)
{
    if (!IsFieldAllowed()) return;
    const FVector2D Axis=Value.Get<FVector2D>();
    if (Axis.SizeSquared()>.0025) { LastAim=Axis.GetSafeNormal(); bGamepadAim=true; }
}
FVector UMemoriaCombatInputRouter::AimPoint() const
{
    const auto* C=Controller.Get(); const APawn* Pawn=C ? C->GetPawn().Get() : nullptr;
    if (!Pawn) return FVector::ZeroVector;
    FVector Direction(LastAim.X,LastAim.Y,0);
    if (Direction.IsNearlyZero())
    {
        const auto* B=Combat(); const auto* Figure=B ? B->GetPlayerFigure() : nullptr;
        Direction=FRotator(0,Figure ? Figure->GetYaw() : Pawn->GetActorRotation().Yaw,0).Vector();
    }
    return Pawn->GetActorLocation()+Direction*1000.f;
}
void UMemoriaCombatInputRouter::CancelHeld()
{
    if (bAttack || bGuard) if (auto* B=Combat()) B->CancelHeldInput();
    bAttack=bGuard=bSkills=bItems=false;
    for (const auto& Key : PhysicalDown) BlockedUntilRelease.Add(Key);
}
void UMemoriaCombatInputRouter::TickComponent(float Delta,ELevelTick TickType,FActorComponentTickFunction* Tick)
{
    Super::TickComponent(Delta,TickType,Tick);
    TrackGamepadAxes();
    RefreshContext();
    const bool bAllowed=IsFieldAllowed();
    if (!bAllowed && bFieldWasAllowed) CancelHeld();
    bFieldWasAllowed=bAllowed;
}
int32 UMemoriaCombatInputRouter::GetMappingCount() const { return Context ? Context->GetMappings().Num() : 0; }
void UMemoriaCombatInputRouter::Shutdown()
{
    CancelHeld();
    if (auto* C=Controller.Get(); C && C->GetLocalPlayer())
        if (auto* Local=C->GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(); Local && Context) Local->RemoveMappingContext(Context);
    if (auto* Input=Enhanced.Get()) for (uint32 Handle : BindingHandles) Input->RemoveBindingByHandle(Handle);
    BindingHandles.Reset(); Actions.Reset(); Context=nullptr; Enhanced.Reset(); Controller.Reset(); bContextActive=false;
}
void UMemoriaCombatInputRouter::EndPlay(const EEndPlayReason::Type Reason) { Shutdown(); Super::EndPlay(Reason); }
