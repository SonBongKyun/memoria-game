#include "Combat/MemoriaFieldCombatSubsystem.h"
#include "Combat/MemoriaFieldCombatTypes.h"
#include "Combat/MemoriaFieldMonster.h"
#include "Presentation/MemoriaFieldCharacterComponent.h"
#include "Presentation/MemoriaCombatClips.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Audio/MemoriaAudioSubsystem.h"
#include "Presentation/MemoriaArchiveView.h"
#include "Domain/MemoriaPlayerMemoryDomain.h"
#include "Components/PointLightComponent.h"
#include "Engine/PointLight.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
using namespace MemoriaCombatTuning;
bool UMemoriaFieldCombatSubsystem::bFieldEncountersInTests = false;
namespace
{
UMemoriaRunSubsystem* RunOf(const UWorld* World)
{ return World && World->GetGameInstance() ? World->GetGameInstance()->GetSubsystem<UMemoriaRunSubsystem>() : nullptr; }
void Cue(const UWorld* World, const TCHAR* Id)
{ if (auto* Audio = World && World->GetGameInstance() ? World->GetGameInstance()->GetSubsystem<UMemoriaAudioSubsystem>() : nullptr) Audio->PlaySfx(Id); }
}
bool UMemoriaFieldCombatSubsystem::UseFieldEncounters() { return !GIsAutomationTesting || bFieldEncountersInTests; }
void UMemoriaFieldCombatSubsystem::RegisterPlayer(APawn* Pawn, UMemoriaFieldCharacterComponent* Figure)
{ Player = Pawn; PlayerFigure = Figure; ComboStep = -1; DashLeft = StaggerLeft = CastLeft = 0.f; bDefeated = false; CloseBurnPicker(); }
int64 UMemoriaFieldCombatSubsystem::GetPlayerHp() const { const auto* Run = RunOf(GetWorld()); return Run ? Run->State.Player.Hp : 0; }
int64 UMemoriaFieldCombatSubsystem::GetPlayerMaxHp() const { const auto* Run = RunOf(GetWorld()); return Run ? Run->State.Player.MaxHp : 0; }
int32 UMemoriaFieldCombatSubsystem::LiveMonsterCount() const
{
    int32 Live = 0;
    for (const auto& M : Monsters) if (M.IsValid() && !M->IsDead()) ++Live;
    return Live;
}
bool UMemoriaFieldCombatSubsystem::StartStep(int32 Step)
{
    auto* Figure = PlayerFigure.Get();
    ComboStep = Step; StepTime = 0.f; bQueued = false; HitThisSwing.Reset(); DrawSword();
    if (Figure) Figure->SetAim(AimDirection.Rotation().Yaw);
    // Without a clip (no rigged art) the combo still resolves on a fixed half-second beat.
    const float Rate = Figure && Figure->HasSword() ? SwordComboRate[FMath::Clamp(Step, 0, 2)] : ComboRate;
    StepLength = Figure && Figure->PlayAction(MemoriaCombatClips::Attack(Step), Rate) ? Figure->GetActionLength() / Rate : .5f;
    Cue(GetWorld(), TEXT("sword_slash"));
    return true;
}
bool UMemoriaFieldCombatSubsystem::RequestAttack(const FVector& AimPoint)
{
    APawn* Pawn = Player.Get();
    if (!Pawn || bDefeated || DashLeft > 0.f || StaggerLeft > 0.f) return false;
    const FVector Aim = (AimPoint - Pawn->GetActorLocation()) * FVector(1, 1, 0);
    if (!IsAttacking())
    {
        if (!Aim.IsNearlyZero()) AimDirection = Aim.GetSafeNormal();
        // A press shortly after a step ends still chains the combo; otherwise it starts over.
        return StartStep(SinceStep < ComboReset && LastStep < 2 ? LastStep + 1 : 0);
    }
    if (StepTime / FMath::Max(StepLength, .01f) >= QueueFrom && ComboStep < 2)
    {
        if (!Aim.IsNearlyZero()) AimDirection = Aim.GetSafeNormal();
        bQueued = true;
    }
    return true;
}
bool UMemoriaFieldCombatSubsystem::RequestDash(const FVector& Direction)
{
    APawn* Pawn = Player.Get(); auto* Figure = PlayerFigure.Get();
    if (!Pawn || bDefeated || DashLeft > 0.f || DashCooldownLeft > 0.f) return false;
    const FVector Flat = Direction * FVector(1, 1, 0);
    DashDirection = Flat.IsNearlyZero() ? AimDirection : Flat.GetSafeNormal();
    ComboStep = -1; bQueued = false; StaggerLeft = 0.f;
    DashLeft = DashTime; DashCooldownLeft = DashCooldown + DashTime; DrawSword();
    if (Figure)
    {
        Figure->SetAim(DashDirection.Rotation().Yaw);
        if (const auto* Clip = MemoriaCombatClips::Load(Figure->GetCharacterId(), MemoriaCombatClips::Dash()))
            Figure->PlayAction(MemoriaCombatClips::Dash(), Clip->GetPlayLength() / DashTime);
    }
    Cue(GetWorld(), TEXT("flee"));
    return true;
}
void UMemoriaFieldCombatSubsystem::ResolveSwing()
{
    APawn* Pawn = Player.Get(); if (!Pawn) return;
    const FVector Origin = Pawn->GetActorLocation();
    for (const auto& Weak : Monsters)
    {
        AMemoriaFieldMonster* Monster = Weak.Get();
        if (!Monster || Monster->IsDead() || HitThisSwing.Contains(Weak)) continue;
        const FVector To = (Monster->GetActorLocation() - Origin) * FVector(1, 1, 0);
        const float Distance = To.Size();
        if (Distance > AttackRange + 30.f) continue;
        if (Distance > 60.f && FVector::DotProduct(To / Distance, AimDirection) < AttackArcCos) continue;
        HitThisSwing.Add(Weak);
        const float Damage = ComboDamage[FMath::Clamp(ComboStep, 0, 2)];
        if (Monster->TakeHit(Damage, Origin))
        {
            ++HitsLanded; Popup(Monster->GetActorLocation() + FVector(0, 0, HuskHeight), Damage, false);
            Cue(GetWorld(), TEXT("hit"));
        }
    }
}
bool UMemoriaFieldCombatSubsystem::StrikePlayer(AMemoriaFieldMonster* Monster, float Damage)
{
    APawn* Pawn = Player.Get(); auto* Run = RunOf(GetWorld()); auto* Figure = PlayerFigure.Get();
    if (!Pawn || !Monster || !Run || IsInvulnerable()) return false;
    if (FVector::Dist2D(Pawn->GetActorLocation(), Monster->GetActorLocation()) > HuskReach + 40.f) return false;
    if (bPicking) CloseBurnPicker();
    auto& Hp = Run->State.Player.Hp;
    Hp = FMath::Max<int64>(0, Hp - FMath::RoundToInt64(Damage));
    Popup(Pawn->GetActorLocation() + FVector(0, 0, 150.f), Damage, true);
    Cue(GetWorld(), TEXT("hit"));
    ComboStep = -1; bQueued = false;
    if (Hp <= 0)
    {
        // Defeat: Arrel falls, and after a beat rises again at full strength; the husks withdraw.
        bDefeated = true; DefeatLeft = 2.5f;
        if (Figure) Figure->PlayAction(MemoriaCombatClips::Death(), 1.f, true);
        Cue(GetWorld(), TEXT("defeat"));
        return true;
    }
    StaggerLeft = PlayerStagger;
    if (Figure) Figure->PlayAction(MemoriaCombatClips::Hit(), 1.3f);
    return true;
}
TArray<AMemoriaFieldMonster*> UMemoriaFieldCombatSubsystem::SpawnWave(int32 Count, const FVector& Center, float Radius)
{
    TArray<AMemoriaFieldMonster*> Spawned;
    UWorld* World = GetWorld(); if (!World) return Spawned;
    FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    for (int32 I = 0; I < Count; ++I)
    {
        const float Angle = 2.f * PI * I / FMath::Max(1, Count) + .6f;
        const FVector Location = Center + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0) * Radius;
        if (auto* Monster = World->SpawnActor<AMemoriaFieldMonster>(Location, FRotator::ZeroRotator, Params))
        { Monsters.Add(Monster); Spawned.Add(Monster); }
    }
    return Spawned;
}
void UMemoriaFieldCombatSubsystem::NotifyMonsterDied(AMemoriaFieldMonster*) { ++Kills; Cue(GetWorld(), TEXT("enemy_die")); }
void UMemoriaFieldCombatSubsystem::Popup(const FVector& Location, float Amount, bool bPlayer)
{ FMemoriaCombatPopup P; P.Location = Location; P.Amount = Amount; P.bPlayer = bPlayer; Popups.Add(P); }
void UMemoriaFieldCombatSubsystem::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    for (auto& P : Popups) P.Age += DeltaSeconds;
    Popups.RemoveAll([](const FMemoriaCombatPopup& P) { return P.Age > 1.f; });
    Monsters.RemoveAll([](const TWeakObjectPtr<AMemoriaFieldMonster>& M) { return !M.IsValid(); });
    APawn* Pawn = Player.Get(); auto* Figure = PlayerFigure.Get();
    if (!Pawn) return;
    if (bDefeated)
    {
        TickBurn(DeltaSeconds);
        DefeatLeft -= DeltaSeconds;
        if (DefeatLeft <= 0.f)
        {
            if (auto* Run = RunOf(GetWorld())) Run->State.Player.Hp = Run->State.Player.MaxHp;
            for (const auto& M : Monsters) if (M.IsValid()) M->Destroy();
            Monsters.Reset(); bDefeated = false;
            if (Figure) { Figure->StopAction(); Figure->ClearAim(); }
        }
        return;
    }
    TickBurn(DeltaSeconds);
    if (CastLeft > 0.f) return;
    // S312: the sword stays out while husks stand, and goes back a while after the last of them falls.
    if (LiveMonsterCount() > 0) DrawSword();
    else if (Figure && Figure->IsSwordDrawn() && !IsAttacking() && DashLeft <= 0.f && (SheatheIn -= DeltaSeconds) <= 0.f) Figure->SetSwordDrawn(false);
    StaggerLeft = FMath::Max(0.f, StaggerLeft - DeltaSeconds);
    DashCooldownLeft = FMath::Max(0.f, DashCooldownLeft - DeltaSeconds);
    if (DashLeft > 0.f)
    {
        const float Move = FMath::Min(DeltaSeconds, DashLeft);
        Pawn->SetActorLocation(Pawn->GetActorLocation() + DashDirection * (DashDistance / DashTime) * Move, true);
        DashLeft -= Move;
        if (DashLeft <= 0.f && Figure) Figure->ClearAim();
    }
    if (IsAttacking())
    {
        StepTime += DeltaSeconds;
        const float Fraction = StepTime / FMath::Max(StepLength, .01f);
        if (Fraction >= HitWindowStart && Fraction <= HitWindowEnd) ResolveSwing();
        if (StepTime >= StepLength)
        {
            LastStep = ComboStep;
            if (bQueued && ComboStep < 2) StartStep(ComboStep + 1);
            else { ComboStep = -1; SinceStep = 0.f; if (Figure) Figure->ClearAim(); }
        }
    }
    else SinceStep += DeltaSeconds;
}
bool UMemoriaFieldCombatSubsystem::OpenBurnPicker()
{
    APawn* Pawn = Player.Get(); auto* Run = RunOf(GetWorld());
    if (!Pawn || !Run || !Run->GetPlayerMemory() || bDefeated || bPicking || CastLeft > 0.f || DashLeft > 0.f) return false;
    const auto* Memory = Run->GetPlayerMemory();
    Choices.Reset();
    // The archive's rows carry the localized titles and grade labels; only what can burn now is offered.
    for (const auto& Row : MemoriaArchive::Build(*Run).Rows)
    {
        if (Memory->CanBurn(Row.Id) != EMemoriaMemoryResult::Success) continue;
        FMemoriaBurnChoice Choice;
        Choice.Id = Row.Id; Choice.Title = Row.Title; Choice.GradeLabel = Row.GradeLabel; Choice.Grade = FMath::Clamp(Row.Grade, 0, 4);
        Choice.Power = Memory->GetEffectiveBurnPower(Row.Id); Choice.Damage = BurnBaseDamage[Choice.Grade] + float(Choice.Power);
        Choice.Accent = Row.Accent;
        Choices.Add(Choice);
    }
    if (Choices.IsEmpty()) return false;
    // Weakest first, so the first keys are the cheap burns.
    Choices.StableSort([](const FMemoriaBurnChoice& A, const FMemoriaBurnChoice& B) { return A.Grade < B.Grade; });
    ComboStep = -1; bQueued = false; Selection = 0; bArmed = false; bPicking = true;
    UGameplayStatics::SetGlobalTimeDilation(GetWorld(), BurnPickDilation);
    Cue(GetWorld(), TEXT("ui_open"));
    return true;
}
void UMemoriaFieldCombatSubsystem::CloseBurnPicker()
{
    if (!bPicking) return;
    bPicking = bArmed = false;
    if (GetWorld()) UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.f);
}
void UMemoriaFieldCombatSubsystem::SelectBurn(int32 Index)
{
    if (!bPicking || !Choices.IsValidIndex(Index)) return;
    if (Index != Selection) Cue(GetWorld(), TEXT("ui_hover"));
    Selection = Index; bArmed = false;
}
bool UMemoriaFieldCombatSubsystem::ConfirmBurn()
{
    auto* Run = RunOf(GetWorld()); auto* Figure = PlayerFigure.Get();
    if (!bPicking || !Run || !Choices.IsValidIndex(Selection)) return false;
    const FMemoriaBurnChoice Choice = Choices[Selection];
    // Grade 2 and 1 (identity, the core) ask twice: the first confirm only arms.
    if (Choice.Grade >= BurnAskTwiceFrom && !bArmed) { bArmed = true; Cue(GetWorld(), TEXT("heartbeat")); return false; }
    CloseBurnPicker();
    // The burn is the run's: the memory is gone for good, and its passives, erosion and drama follow.
    if (Run->BurnMemory(Choice.Id) != EMemoriaMemoryResult::Success) { Cue(GetWorld(), TEXT("cancel")); return false; }
    ++Burns; Casting = Choice;
    Casting.Power = Run->GetPlayerMemory()->GetEffectiveBurnPower(Choice.Id);
    Casting.Damage = FMath::Max(Choice.Damage, BurnBaseDamage[Choice.Grade] + float(Casting.Power));
    CastLeft = BurnCastTime; StaggerLeft = 0.f; DrawSword();
    Wave = FMemoriaBurnWave(); Wave.Grade = Choice.Grade; Wave.Title = Choice.Title;
    Wave.Skill = BurnSkillName(Choice.Grade, Run->GetRunSnapshot().CurrentLocale == TEXT("ko"));
    // The finisher (the sword's spinning cut, or the unarmed charged blow), sped so its sweep meets the ring.
    if (Figure)
    {
        const TCHAR* Swing = Figure->HasSword() ? MemoriaCombatClips::Attack(2) : MemoriaCombatClips::Charged();
        if (const auto* Clip = MemoriaCombatClips::Load(Figure->GetCharacterId(), MemoriaCombatClips::ForAction(Figure->GetCharacterId(), Swing)))
            Figure->PlayAction(Swing, Clip->GetPlayLength() / (BurnCastTime + .9f));
    }
    Cue(GetWorld(), TEXT("burn"));
    return true;
}
void UMemoriaFieldCombatSubsystem::ReleaseBurn()
{
    APawn* Pawn = Player.Get(); UWorld* World = GetWorld();
    if (!Pawn || !World) return;
    const int32 Grade = Casting.Grade;
    Wave.Center = Pawn->GetActorLocation(); Wave.Radius = BurnRadius[Grade]; Wave.Age = 0.f; Wave.bLive = true;
    Burned.Reset();
    // A flare of the grade's colour lights the field around Arrel while the ring spreads.
    FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    if (auto* Light = World->SpawnActor<APointLight>(Wave.Center + FVector(0, 0, 120.f), FRotator::ZeroRotator, Params))
    {
        auto* Component = Light->PointLightComponent.Get();
        Component->SetMobility(EComponentMobility::Movable);
        Component->SetLightColor(BurnColor(Grade)); Component->SetAttenuationRadius(Wave.Radius * 1.6f);
        Component->SetIntensityUnits(ELightUnits::Candelas); Component->SetIntensity(60.f + 40.f * Grade);
        Component->SetCastShadows(false);
        Flare = Light;
    }
    Cue(World, Grade >= 3 ? TEXT("void_pulse") : TEXT("burn_ignite"));
}
void UMemoriaFieldCombatSubsystem::TickBurn(float DeltaSeconds)
{
    if (CastLeft > 0.f)
    {
        CastLeft -= DeltaSeconds;
        if (CastLeft <= 0.f) { CastLeft = 0.f; ReleaseBurn(); }
    }
    if (!Wave.bLive) return;
    Wave.Age += DeltaSeconds;
    const float Reach = Wave.Radius * FMath::Clamp(Wave.Age / BurnRingTime, 0.f, 1.f);
    // The ring strikes each husk once as it passes, and shoves it outward.
    for (const auto& Weak : Monsters)
    {
        AMemoriaFieldMonster* Monster = Weak.Get();
        if (!Monster || Monster->IsDead() || Burned.Contains(Weak) || FVector::Dist2D(Monster->GetActorLocation(), Wave.Center) > Reach + 30.f) continue;
        Burned.Add(Weak);
        if (Monster->TakeHit(Casting.Damage, Wave.Center, BurnShove[Casting.Grade]))
        { ++HitsLanded; Popup(Monster->GetActorLocation() + FVector(0, 0, HuskHeight), Casting.Damage, false); }
    }
    if (auto* Light = Flare.Get())
    {
        const float Fade = FMath::Clamp(1.f - Wave.Age / BurnAfterglow, 0.f, 1.f);
        Light->PointLightComponent->SetIntensity((60.f + 40.f * Casting.Grade) * Fade * Fade);
        if (Fade <= 0.f) Light->Destroy();
    }
    if (Wave.Age >= BurnAfterglow)
    {
        Wave.bLive = false;
        if (auto* Figure = PlayerFigure.Get()) if (!IsAttacking()) Figure->ClearAim();
    }
}
void UMemoriaFieldCombatSubsystem::Deinitialize()
{
    if (auto* Light = Flare.Get()) Light->Destroy();
    CloseBurnPicker();
    Super::Deinitialize();
}
void UMemoriaFieldCombatSubsystem::DrawSword()
{
    SheatheIn = SheatheDelay;
    if (auto* Figure = PlayerFigure.Get()) Figure->SetSwordDrawn(true);
}
