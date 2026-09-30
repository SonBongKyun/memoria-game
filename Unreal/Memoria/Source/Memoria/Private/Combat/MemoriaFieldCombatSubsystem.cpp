#include "Combat/MemoriaFieldCombatSubsystem.h"
#include "Combat/MemoriaFieldCombatTypes.h"
#include "Combat/MemoriaFieldMonster.h"
#include "Presentation/MemoriaFieldCharacterComponent.h"
#include "Presentation/MemoriaCombatClips.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Run/MemoriaRunTypes.h"
#include "Audio/MemoriaAudioSubsystem.h"
#include "Presentation/MemoriaArchiveView.h"
#include "Domain/MemoriaPlayerMemoryDomain.h"
#include "Components/PointLightComponent.h"
#include "Engine/PointLight.h"
#include "Kismet/GameplayStatics.h"
#include "Components/StaticMeshComponent.h"
#include "Misc/App.h"
#include "Interaction/MemoriaEliaCompanion.h"
#include "EngineUtils.h"
#include "Engine/GameInstance.h"
#include "Achievements/MemoriaAchievementSubsystem.h"
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
    const float Rate = Step == 3 ? HeavyRate : Figure && Figure->HasSword() ? SwordComboRate[FMath::Clamp(Step, 0, 2)] : ComboRate;
    StepLength = Figure && Figure->PlayAction(MemoriaCombatClips::Attack(FMath::Min(Step, 2)), Rate) ? Figure->GetActionLength() / Rate : .5f;
    Cue(GetWorld(), TEXT("sword_slash"));
    return true;
}
bool UMemoriaFieldCombatSubsystem::RequestAttack(const FVector& AimPoint)
{
    APawn* Pawn = Player.Get();
    if (!Pawn || bDefeated || DashLeft > 0.f || StaggerLeft > 0.f || bBlocking) return false;
    const FVector Aim = (AimPoint - Pawn->GetActorLocation()) * FVector(1, 1, 0);
    if (!IsAttacking())
    {
        if (!Aim.IsNearlyZero()) AimDirection = Aim.GetSafeNormal();
        // A press shortly after a step ends still chains the combo; otherwise it starts over.
        return StartStep(SinceStep < ComboReset && LastStep >= 0 && LastStep < 2 ? LastStep + 1 : 0);
    }
    if (!IsHeavy() && StepTime / FMath::Max(StepLength, .01f) >= QueueFrom && ComboStep < 2)
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
        if (Distance > (IsHeavy() ? HeavyRange : AttackRange) + 30.f) continue;
        if (!IsHeavy() && Distance > 60.f && FVector::DotProduct(To / Distance, AimDirection) < AttackArcCos) continue;
        HitThisSwing.Add(Weak);
        const float Damage = (IsHeavy() ? HeavyDamage : ComboDamage[FMath::Clamp(ComboStep, 0, 2)]) * (IsWeakened() ? WeakenFactor : 1.f);
        if (Monster->TakeHit(Damage, Origin, IsHeavy() ? HeavyShove : 14.f))
        {
            ++HitsLanded; Popup(Monster->GetActorLocation() + FVector(0, 0, Monster->Spec().Height), Damage, false);
            Cue(GetWorld(), TEXT("hit"));
            const bool bBig = IsHeavy() || ComboStep == 2;
            HitStop(bBig ? HitStopHeavy : HitStopLight, bBig ? ShakeHeavy : ShakeLight);
            Burst(Monster->GetActorLocation() + FVector(0, 0, Monster->Spec().Height * .55f), bBig ? 14 : 8, FLinearColor(1.f, .7f, .35f), bBig ? 420.f : 300.f);
        }
    }
}
bool UMemoriaFieldCombatSubsystem::StrikePlayer(AMemoriaFieldMonster* Monster, float Damage)
{
    APawn* Pawn = Player.Get(); auto* Run = RunOf(GetWorld()); auto* Figure = PlayerFigure.Get();
    if (!Pawn || !Monster || !Run || IsInvulnerable()) return false;
    if (FVector::Dist2D(Pawn->GetActorLocation(), Monster->GetActorLocation()) > Monster->Spec().Reach + 40.f) return false;
    if (bPicking) CloseBurnPicker();
    const bool Ko = Run->GetRunSnapshot().CurrentLocale == TEXT("ko");
    if (bBlocking)
    {
        const FVector Spark = (Pawn->GetActorLocation() + Monster->GetActorLocation()) * .5f + FVector(0, 0, 110.f);
        if (BlockHeld <= ParryWindow)
        {
            // Parry: the guard met the blow as it came. No harm, and the foe reels.
            ++Parries; Monster->Stun(ParryStun);
            Popup(Pawn->GetActorLocation() + FVector(0, 0, 175.f), 0.f, true, Ko ? TEXT("패링!") : TEXT("Parry!"), FLinearColor(1.f, .9f, .55f));
            HitStop(HitStopHeavy, ShakeHeavy); Burst(Spark, 18, FLinearColor(1.f, .92f, .6f), 480.f);
            Cue(GetWorld(), TEXT("shield"));
            return false;
        }
        // A held guard only softens the blow, and it spares Arrel the blow's poison or curse.
        ++Blocks;
        auto& Hp = Run->State.Player.Hp;
        const int64 Loss = FMath::Min<int64>(FMath::RoundToInt64(Damage * BlockFactor), Hp - 1);
        if (Loss > 0) Hp -= Loss;
        Popup(Pawn->GetActorLocation() + FVector(0, 0, 150.f), float(Loss), true, Ko ? TEXT("막음") : TEXT("Blocked"), FLinearColor(.75f, .82f, 1.f));
        HitStop(HitStopLight, ShakeLight); Burst(Spark, 8, FLinearColor(.8f, .85f, 1.f), 300.f);
        Cue(GetWorld(), TEXT("shield"));
        return true;
    }
    if (ShieldLeft > 0.f) Damage *= HummingShieldFactor; // Elia's humming shield (S319)
    auto& Hp = Run->State.Player.Hp;
    Hp = FMath::Max<int64>(0, Hp - FMath::RoundToInt64(Damage)); ++StrikesTaken;
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
    StaggerLeft = PlayerStagger; bCharging = false;
    if (Figure) Figure->PlayAction(MemoriaCombatClips::Hit(), 1.3f);
    HitStop(HitStopLight, ShakeHeavy);
    Afflict(Monster->Spec().Ability, Damage);
    return true;
}
TArray<AMemoriaFieldMonster*> UMemoriaFieldCombatSubsystem::SpawnWave(int32 Count, const FVector& Center, float Radius, EMemoriaFoeKind Kind)
{
    TArray<AMemoriaFieldMonster*> Spawned;
    UWorld* World = GetWorld(); if (!World) return Spawned;
    FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    for (int32 I = 0; I < Count; ++I)
    {
        const float Angle = 2.f * PI * I / FMath::Max(1, Count) + .6f;
        const FVector Location = Center + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0) * Radius;
        // Deferred, so the kind is known when the foe builds its figure in BeginPlay.
        if (auto* Monster = World->SpawnActorDeferred<AMemoriaFieldMonster>(AMemoriaFieldMonster::StaticClass(), FTransform(Location), nullptr, nullptr, Params.SpawnCollisionHandlingOverride))
        { Monster->SetKind(Kind); Monster->FinishSpawning(FTransform(Location)); Monsters.Add(Monster); Spawned.Add(Monster); }
    }
    return Spawned;
}
void UMemoriaFieldCombatSubsystem::NotifyMonsterDied(AMemoriaFieldMonster* Monster)
{
    ++Kills; Cue(GetWorld(), TEXT("enemy_die"));
    auto* Run = RunOf(GetWorld());
    if (!Monster || !Run) return;
    // Source Win grains for the foe: (void ? 8 : 3) + max HP / 20, paid as each one falls.
    const FMemoriaFoeSpec& S = Monster->Spec();
    const int64 Grains = (S.bVoid ? 8 : 3) + int64(S.Health) / 20;
    Run->State.Player.Grains += Grains; WaveGrains += Grains; ++WaveKills; bWaveVoid |= S.bVoid;
    Popup(Monster->GetActorLocation() + FVector(0, 0, S.Height * .6f), float(Grains), false, FString::Printf(TEXT("+%lld Grains"), Grains), FLinearColor(1.f, .86f, .35f));
    if (LiveMonsterCount() == 0) WinWave();
}
void UMemoriaFieldCombatSubsystem::WinWave()
{
    auto* Run = RunOf(GetWorld()); APawn* Pawn = Player.Get();
    if (!Run) return;
    // The rest of the source Win: 20% HP back and a 30% drop from the potion table (richer after a void foe).
    auto& P = Run->State.Player;
    if (auto* Achievements = GetWorld()->GetGameInstance()->GetSubsystem<UMemoriaAchievementSubsystem>()) Achievements->RecordBattleWon(P.Hp);
    Reward = FMemoriaFieldReward(); Reward.Grains = WaveGrains; Reward.Kills = WaveKills; Reward.Age = 0.f;
    Reward.Heal = FMath::Min<int64>(int64(P.MaxHp * WaveHealShare), P.MaxHp - P.Hp); P.Hp += Reward.Heal;
    if (Drops.FRand() <= ItemDropChance)
    {
        TArray<const TCHAR*> Table = {TEXT("potion"), TEXT("potion"), TEXT("potion"), TEXT("antidote"), TEXT("antidote"), TEXT("firebomb")};
        if (bWaveVoid) Table.Append({TEXT("firebomb"), TEXT("hi_potion"), TEXT("witness_ink")});
        Reward.ItemId = Table[Drops.RandRange(0, Table.Num() - 1)];
        auto* Item = P.Items.FindByPredicate([&](const FMemoriaItemCount& I) { return I.Id == Reward.ItemId; });
        if (Item) ++Item->Count; else { FMemoriaItemCount New; New.Id = Reward.ItemId; New.Count = 1; P.Items.Add(New); }
        Run->State.RecordRecentItem(Reward.ItemId);
        Reward.ItemName = MemoriaCombatTuning::ItemName(Reward.ItemId, Run->GetRunSnapshot().CurrentLocale == TEXT("ko"));
    }
    if (Pawn && Reward.Heal > 0) Popup(Pawn->GetActorLocation() + FVector(0, 0, 150.f), float(Reward.Heal), true, FString::Printf(TEXT("+%lld HP"), Reward.Heal), FLinearColor(.5f, 1.f, .6f));
    // A won fight ends the statuses and the chain, like the source battle's end.
    WeakenLeft = 0.f; PoisonLeft = 0; BurnChain = 0; WaveKills = 0; WaveGrains = 0; bWaveVoid = false;
    Cue(GetWorld(), TEXT("heal"));
}
void UMemoriaFieldCombatSubsystem::Afflict(EMemoriaFoeAbility Ability, float Damage)
{
    APawn* Pawn = Player.Get(); if (!Pawn || bDefeated) return;
    const bool Ko = [&] { const auto* Run = RunOf(GetWorld()); return !Run || Run->GetRunSnapshot().CurrentLocale == TEXT("ko"); }();
    if (Ability == EMemoriaFoeAbility::Weaken)
    {
        WeakenLeft = WeakenTime;
        Popup(Pawn->GetActorLocation() + FVector(0, 0, 175.f), 0.f, true, Ko ? TEXT("약화") : TEXT("Weakened"), FLinearColor(.75f, .6f, 1.f));
    }
    else if (Ability == EMemoriaFoeAbility::Poison && PoisonLeft == 0)
    {
        // Source poison: attack * 0.3 + 2..5 each turn; the field takes the middle of the range.
        PoisonLeft = PoisonTicks; PoisonClock = PoisonInterval; PoisonDamage = int64(Damage * .3f) + 3;
        Popup(Pawn->GetActorLocation() + FVector(0, 0, 175.f), 0.f, true, Ko ? TEXT("중독") : TEXT("Poisoned"), FLinearColor(.55f, 1.f, .35f));
    }
}
void UMemoriaFieldCombatSubsystem::TickStatuses(float DeltaSeconds)
{
    WeakenLeft = FMath::Max(0.f, WeakenLeft - DeltaSeconds);
    auto* Run = RunOf(GetWorld()); APawn* Pawn = Player.Get();
    if (PoisonLeft > 0 && Run && Pawn && (PoisonClock -= DeltaSeconds) <= 0.f)
    {
        // Poison wears Arrel down but never fells him; only a blow does.
        PoisonClock = PoisonInterval; --PoisonLeft;
        auto& Hp = Run->State.Player.Hp;
        const int64 Loss = FMath::Min(PoisonDamage, Hp - 1);
        if (Loss > 0) { Hp -= Loss; Popup(Pawn->GetActorLocation() + FVector(0, 0, 150.f), float(Loss), true, FString(), FLinearColor(.55f, 1.f, .35f)); }
    }
}
void UMemoriaFieldCombatSubsystem::NotifyBurnTick(AMemoriaFieldMonster* Monster, float Damage)
{ if (Monster) Popup(Monster->GetActorLocation() + FVector(0, 0, Monster->Spec().Height), Damage, false, FString(), FLinearColor(1.f, .45f, .15f)); }
void UMemoriaFieldCombatSubsystem::Popup(const FVector& Location, float Amount, bool bPlayer, const FString& Label, const FLinearColor& Tint)
{
    FMemoriaCombatPopup P; P.Location = Location; P.Amount = Amount; P.bPlayer = bPlayer; P.Label = Label; P.Tint = Tint;
    // Words raised at the same moment and place stack upward instead of printing over each other.
    if (!Label.IsEmpty())
        for (const FMemoriaCombatPopup& Other : Popups)
            if (!Other.Label.IsEmpty() && Other.Age < .5f && FVector::Dist2D(Other.Location, P.Location) < 60.f && FMath::Abs(Other.Location.Z - P.Location.Z) < 24.f)
                P.Location.Z = Other.Location.Z + 28.f;
    Popups.Add(P);
}
void UMemoriaFieldCombatSubsystem::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    TickFeel(DeltaSeconds);
    for (auto& P : Popups) P.Age += DeltaSeconds;
    Popups.RemoveAll([](const FMemoriaCombatPopup& P) { return P.Age > (P.Label.IsEmpty() ? 1.f : 1.6f); });
    Reward.Age += DeltaSeconds;
    Monsters.RemoveAll([](const TWeakObjectPtr<AMemoriaFieldMonster>& M) { return !M.IsValid(); });
    APawn* Pawn = Player.Get(); auto* Figure = PlayerFigure.Get();
    if (!Pawn) return;
    if (bDefeated)
    {
        // The fall plays out; then the game over screen owns the choice (see Revive).
        TickBurn(DeltaSeconds);
        DefeatLeft = FMath::Max(0.f, DefeatLeft - DeltaSeconds);
        return;
    }
    TickBurn(DeltaSeconds);
    TickStatuses(DeltaSeconds);
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
    NoteDiary(Choice.Id);
    const auto* Memory = Run->GetPlayerMemory();
    Casting.Power = Memory->GetEffectiveBurnPower(Choice.Id);
    // Source burn damage: base + effective power, then Ember Affinity, the chain and (void grades) Void Touch.
    float Damage = FMath::Max(Choice.Damage, BurnBaseDamage[Choice.Grade] + float(Casting.Power));
    if (Memory->HasPassive(TEXT("ember_affinity"))) Damage *= EmberAffinity;
    BurnChain = Choice.Grade >= 2 ? BurnChain + 1 : 0;
    if (BurnChain >= 2) Damage *= 1.f + (BurnChain - 1) * BurnChainBonus;
    if (Choice.Grade >= 3 && Memory->HasPassive(TEXT("void_touch"))) Damage *= VoidTouch;
    Casting.Damage = Damage;
    if (Memory->HasPassive(TEXT("residual_warmth")))
    {
        auto& P = Run->State.Player; const int64 Warmth = FMath::Min(ResidualWarmth, P.MaxHp - P.Hp); P.Hp += Warmth;
        if (Warmth > 0 && Player.IsValid()) Popup(Player->GetActorLocation() + FVector(0, 0, 150.f), float(Warmth), true, FString::Printf(TEXT("+%lld HP"), Warmth), FLinearColor(.5f, 1.f, .6f));
    }
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
    HitStop(HitStopHeavy, ShakeHeavy + 3.f * Grade);
    Burst(Wave.Center + FVector(0, 0, 60.f), 24 + 8 * Grade, BurnColor(Grade), 520.f);
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
        {
            ++HitsLanded; Popup(Monster->GetActorLocation() + FVector(0, 0, Monster->Spec().Height), Casting.Damage, false);
            // Source: a burn of grade 2 or 1 leaves the foe burning (power * 0.3 + 5 for two turns).
            if (Casting.Grade >= 3) Monster->Ignite(float(Casting.Power) * .3f + 5.f, IgniteTicks);
        }
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
float UMemoriaFieldCombatSubsystem::ChargeTimeValue() { return ChargeTime; }
bool UMemoriaFieldCombatSubsystem::BeginBlock()
{
    auto* Figure = PlayerFigure.Get();
    if (!Player.IsValid() || bDefeated || bPicking || CastLeft > 0.f || DashLeft > 0.f || bBlocking) return false;
    // The guard cancels a swing; the sword comes out for it.
    ComboStep = -1; bQueued = false; bCharging = false; bBlocking = true; BlockHeld = 0.f; DrawSword();
    if (Figure) Figure->PlayAction(MemoriaCombatClips::Block(), 1.6f, true, BlockHoldAt);
    return true;
}
void UMemoriaFieldCombatSubsystem::EndBlock()
{
    if (!bBlocking) return;
    bBlocking = false;
    if (auto* Figure = PlayerFigure.Get()) { Figure->StopAction(); Figure->ClearAim(); }
}
void UMemoriaFieldCombatSubsystem::HitStop(float Seconds, float Shake)
{
    // The burn picker owns the world's time while it is open.
    if (!bPicking && GetWorld())
    {
        HitStopLeft = FMath::Max(HitStopLeft, Seconds);
        UGameplayStatics::SetGlobalTimeDilation(GetWorld(), HitStopDilation);
    }
    if (Shake >= ShakeStrength * ShakeLeft / ShakeTime) { ShakeStrength = Shake; ShakeLeft = ShakeTime; }
}
FVector UMemoriaFieldCombatSubsystem::GetShakeOffset() const
{
    if (ShakeLeft <= 0.f) return FVector::ZeroVector;
    const float A = ShakeStrength * ShakeLeft / ShakeTime;
    return FVector(FMath::Sin(ShakeClock * 71.f), FMath::Sin(ShakeClock * 53.f + 1.3f), .5f * FMath::Sin(ShakeClock * 37.f + .7f)) * A;
}
void UMemoriaFieldCombatSubsystem::Burst(const FVector& Location, int32 Count, const FLinearColor& Color, float Speed)
{
    for (int32 I = 0; I < Count; ++I)
    {
        FMemoriaSpark S; S.Location = Location; S.Color = Color;
        S.Velocity = FVector(FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(.2f, 1.f)).GetSafeNormal() * Speed * FMath::FRandRange(.4f, 1.f);
        S.Life = FMath::FRandRange(.3f, .6f); S.Size = FMath::FRandRange(3.5f, 6.5f);
        Sparks.Add(S);
    }
}
void UMemoriaFieldCombatSubsystem::TickFeel(float DeltaSeconds)
{
    // Hit stop and shake run on real time; sparks and the trail on world time, so they hang in the stop.
    const float Real = FApp::GetDeltaTime();
    if (HitStopLeft > 0.f && (HitStopLeft -= Real) <= 0.f && !bPicking && GetWorld()) UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.f);
    ShakeClock += Real; ShakeLeft = FMath::Max(0.f, ShakeLeft - Real);
    for (FMemoriaSpark& S : Sparks) { S.Age += DeltaSeconds; S.Velocity.Z -= 900.f * DeltaSeconds; S.Location += S.Velocity * DeltaSeconds; }
    Sparks.RemoveAll([](const FMemoriaSpark& S) { return S.Age >= S.Life; });
    for (FMemoriaTrailSample& T : Trail) T.Age += DeltaSeconds;
    Trail.RemoveAll([](const FMemoriaTrailSample& T) { return T.Age > TrailLife; });
    auto* Figure = PlayerFigure.Get();
    const UStaticMeshComponent* Blade = Figure ? Figure->GetBlade() : nullptr;
    if (Blade && Figure->IsSwordDrawn() && (IsAttacking() || CastLeft > 0.f) && DeltaSeconds > 0.f)
    {
        // The prop's blade runs from the guard (about -13 cm) to the tip (-91 cm) along its -Z.
        FMemoriaTrailSample T; T.Base = Blade->GetComponentTransform().TransformPosition(FVector(0, 0, -30));
        T.Tip = Blade->GetComponentTransform().TransformPosition(FVector(0, 0, -88)); Trail.Add(T);
    }
    if (bBlocking) BlockHeld += DeltaSeconds;
    for (float& Cooldown : EliaCooldown) Cooldown = FMath::Max(0.f, Cooldown - DeltaSeconds);
    ShieldLeft = FMath::Max(0.f, ShieldLeft - DeltaSeconds); EliaNoticeAge += DeltaSeconds;
    if (bCharging && !IsHeavy() && !bBlocking)
    {
        ChargeHeld += DeltaSeconds;
        // Held long enough: the heavy cut, a full spin, breaking off whatever swing was playing.
        if (ChargeHeld >= ChargeTime && Player.IsValid() && !bDefeated && DashLeft <= 0.f && StaggerLeft <= 0.f && CastLeft <= 0.f)
        { bCharging = false; StartStep(3); }
    }
}
void UMemoriaFieldCombatSubsystem::Revive(float HpShare)
{
    if (auto* Run = RunOf(GetWorld()))
    {
        auto& P = Run->State.Player;
        P.Hp = FMath::Clamp<int64>(int64(P.MaxHp * HpShare), 1, P.MaxHp);
    }
    for (const auto& M : Monsters) if (M.IsValid()) M->Destroy();
    Monsters.Reset(); bDefeated = false; DefeatLeft = 0.f; StaggerLeft = CastLeft = DashLeft = 0.f; ComboStep = -1;
    WeakenLeft = 0.f; PoisonLeft = 0; BurnChain = 0; WaveKills = 0; WaveGrains = 0; bWaveVoid = false;
    EndBlock(); EndCharge();
    if (auto* Figure = PlayerFigure.Get()) { Figure->StopAction(); Figure->ClearAim(); }
}
AMemoriaEliaCompanion* UMemoriaFieldCombatSubsystem::FindElia() const
{
    for (TActorIterator<AMemoriaEliaCompanion> It(GetWorld()); It; ++It) return *It;
    return nullptr;
}
bool UMemoriaFieldCombatSubsystem::IsEliaSkillUnlocked(int32 Slot) const
{
    // The diary's unlock, derived from the run so no new save data is needed: her memory is burned and she is
    // with the party. (The source also requires her company at the moment of the burn; in Chapters 1-2 she
    // never leaves.)
    const auto* Run = RunOf(GetWorld());
    if (Slot < 0 || Slot >= 4 || !Run || !Run->GetPlayerMemory() || !Run->GetRunSnapshot().Player.bEliaWithParty) return false;
    return Run->GetPlayerMemory()->GetSnapshot().BurnedHistory.Contains(FString(EliaSkills[Slot].Memory));
}
void UMemoriaFieldCombatSubsystem::NoteDiary(const FString& MemoryId)
{
    const auto* Run = RunOf(GetWorld());
    if (!Run || !Run->GetRunSnapshot().Player.bEliaWithParty) return;
    const bool Ko = Run->GetRunSnapshot().CurrentLocale == TEXT("ko");
    static const TCHAR* Diary[] = {TEXT("sense_forest_smell"), TEXT("daily_campfire_song"), TEXT("daily_market_food"), TEXT("rel_hand_reaching"),
        TEXT("identity_first_sword"), TEXT("rel_sable_trust"), TEXT("daily_elia_hands"), TEXT("identity_compass")};
    bool bEntry = false; for (const TCHAR* Id : Diary) bEntry |= MemoryId == Id;
    if (!bEntry) return;
    EliaNotice = Ko ? TEXT("엘리아가 일지를 썼다...") : TEXT("Elia wrote in her diary...");
    for (const auto& Skill : EliaSkills)
        if (MemoryId == Skill.Memory)
            EliaNotice += Ko ? FString::Printf(TEXT("\n엘리아 기술 해금: %s"), Skill.NameKo) : FString::Printf(TEXT("\nElia Technique unlocked: %s"), Skill.Name);
    EliaNoticeAge = 0.f;
}
bool UMemoriaFieldCombatSubsystem::UseEliaSkill(int32 Slot)
{
    APawn* Pawn = Player.Get(); auto* Run = RunOf(GetWorld());
    if (!Pawn || !Run || bDefeated || bPicking || !IsEliaSkillUnlocked(Slot) || EliaCooldown[Slot] > 0.f) return false;
    AMemoriaEliaCompanion* Elia = FindElia();
    const bool Ko = Run->GetRunSnapshot().CurrentLocale == TEXT("ko");
    const FVector Me = Pawn->GetActorLocation();
    const FVector From = Elia ? Elia->GetActorLocation() : Me;
    auto Nearest = [&](float Range)
    {
        AMemoriaFieldMonster* Best = nullptr; float BestDistance = Range;
        for (const auto& Weak : Monsters)
            if (AMemoriaFieldMonster* M = Weak.Get(); M && !M->IsDead())
                if (const float D = FVector::Dist2D(M->GetActorLocation(), Me); D < BestDistance) { Best = M; BestDistance = D; }
        return Best;
    };
    const FLinearColor Pale(.62f, .82f, 1.f);
    switch (Slot)
    {
    case 0:
        // Humming Shield: the melody halves the next blows.
        ShieldLeft = HummingShieldTime;
        Popup(Me + FVector(0, 0, 175.f), 0.f, true, Ko ? TEXT("흥얼거림의 방패") : TEXT("Humming Shield"), Pale);
        Cue(GetWorld(), TEXT("shield"));
        break;
    case 1:
    {
        // Desperate Reach: every foe close by freezes.
        int32 Held = 0;
        for (const auto& Weak : Monsters)
            if (AMemoriaFieldMonster* M = Weak.Get(); M && !M->IsDead() && FVector::Dist2D(M->GetActorLocation(), Me) <= DesperateReachRange)
            { M->Stun(DesperateReachStun); Burst(M->GetActorLocation() + FVector(0, 0, 100.f), 8, Pale, 260.f); ++Held; }
        if (Held == 0) return false;
        Popup(Me + FVector(0, 0, 175.f), 0.f, true, Ko ? TEXT("절박한 손길") : TEXT("Desperate Reach"), Pale);
        Cue(GetWorld(), TEXT("void_pulse"));
        break;
    }
    case 2:
    {
        // Remembered Strike: 10 + 8 per burned memory, on the nearest foe; Elia swings at it.
        AMemoriaFieldMonster* Target = Nearest(RememberedStrikeRange);
        if (!Target) return false;
        const float Damage = 10.f + 8.f * Run->GetPlayerMemory()->GetSnapshot().BurnedHistory.Num();
        if (Target->TakeHit(Damage, From, 30.f))
        {
            ++HitsLanded;
            Popup(Target->GetActorLocation() + FVector(0, 0, Target->Spec().Height), Damage, false, FString(), Pale);
            Burst(Target->GetActorLocation() + FVector(0, 0, Target->Spec().Height * .55f), 12, Pale, 380.f);
            HitStop(HitStopLight, ShakeLight);
        }
        if (Elia && Elia->GetFigure())
        {
            Elia->GetFigure()->SetAim((Target->GetActorLocation() - From).Rotation().Yaw);
            Elia->GetFigure()->PlayAction(MemoriaCombatClips::Attack(0), 1.2f);
        }
        Cue(GetWorld(), TEXT("sword_slash"));
        break;
    }
    case 3:
    {
        // Anchor Pulse: 15% of max HP and every status cured.
        auto& P = Run->State.Player;
        const int64 Heal = FMath::Min<int64>(int64(P.MaxHp * AnchorPulseShare), P.MaxHp - P.Hp); P.Hp += Heal;
        WeakenLeft = 0.f; PoisonLeft = 0;
        Popup(Me + FVector(0, 0, 150.f), float(Heal), true, FString::Printf(TEXT("+%lld HP"), Heal), FLinearColor(.5f, 1.f, .6f));
        Burst(Me + FVector(0, 0, 60.f), 14, FLinearColor(.55f, 1.f, .7f), 240.f);
        Cue(GetWorld(), TEXT("heal"));
        break;
    }
    default: return false;
    }
    EliaCooldown[Slot] = EliaSkills[Slot].Cooldown; ++EliaSkillsUsed;
    return true;
}
