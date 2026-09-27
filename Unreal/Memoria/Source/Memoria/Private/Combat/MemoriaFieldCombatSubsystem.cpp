#include "Combat/MemoriaFieldCombatSubsystem.h"
#include "Combat/MemoriaFieldCombatTypes.h"
#include "Combat/MemoriaFieldMonster.h"
#include "Presentation/MemoriaFieldCharacterComponent.h"
#include "Presentation/MemoriaCombatClips.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Audio/MemoriaAudioSubsystem.h"
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
{ Player = Pawn; PlayerFigure = Figure; ComboStep = -1; DashLeft = StaggerLeft = 0.f; bDefeated = false; }
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
    ComboStep = Step; StepTime = 0.f; bQueued = false; HitThisSwing.Reset();
    if (Figure) Figure->SetAim(AimDirection.Rotation().Yaw);
    // Without a clip (no rigged art) the combo still resolves on a fixed half-second beat.
    StepLength = Figure && Figure->PlayAction(MemoriaCombatClips::Attack(Step), ComboRate) ? Figure->GetActionLength() / ComboRate : .5f;
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
    DashLeft = DashTime; DashCooldownLeft = DashCooldown + DashTime;
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
