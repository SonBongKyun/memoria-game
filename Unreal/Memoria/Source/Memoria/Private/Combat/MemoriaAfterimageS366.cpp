// S366 Afterimage: the perfect dodge, its Echo Counter, and the ghosts of Arrel's pose that a dash leaves behind.
// A dash begun just before a blow lands leaves an afterimage where Arrel stood; the blow falls on what he was a
// moment ago. The world slows and drains of colour for an instant, the foe loses him, and his next swing lands
// harder. Every dash also leaves a faint ghost, so the dodge reads before it is perfect.
#include "Combat/MemoriaFieldCombatSubsystem.h"
#include "Combat/MemoriaFieldCombatTypes.h"
#include "Combat/MemoriaFieldMonster.h"
#include "Presentation/MemoriaFieldCharacterComponent.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Audio/MemoriaAudioSubsystem.h"
#include "Settings/MemoriaSettingsSubsystem.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PostProcessComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
using namespace MemoriaCombatTuning;
namespace
{
const TCHAR* AfterimageMaterialPath = TEXT("/Game/Memoria/Presentation/Combat/M_Afterimage.M_Afterimage");
constexpr int32 MaxGhosts = 6;
const FLinearColor DashTint(.62f, .74f, 1.f);
const FLinearColor DodgeTint(.72f, .56f, 1.f);
const UMemoriaSettingsSubsystem* SettingsOf(const UWorld* World)
{ return World && World->GetGameInstance() ? World->GetGameInstance()->GetSubsystem<UMemoriaSettingsSubsystem>() : nullptr; }
}
float UMemoriaFieldCombatSubsystem::GetAfterimageWeight() const
{
    if (SlowLeft <= 0.f || SlowPeak <= 0.f) return 0.f;
    // A sharp fall into the drained world, then an easing return to colour.
    const float T = 1.f - SlowLeft / SlowPeak;
    return T < .15f ? T / .15f : 1.f - FMath::Square((T - .15f) / .85f);
}
float UMemoriaFieldCombatSubsystem::GetBrightestGhostAlpha() const
{
    float Best = 0.f;
    for (const FMemoriaAfterimageGhost& G : Ghosts) Best = FMath::Max(Best, G.Alpha * FMath::Pow(FMath::Clamp(1.f - G.Age / G.Life, 0.f, 1.f), 1.5f));
    return Best;
}
UPoseableMeshComponent* UMemoriaFieldCombatSubsystem::SpawnGhost(float Alpha, float Life, const FLinearColor& Tint)
{
    APawn* Pawn = Player.Get(); auto* Figure = PlayerFigure.Get();
    USkeletalMeshComponent* Source = Figure ? Figure->GetSkeletalMesh() : nullptr;
    if (!Pawn || !Source || !Source->GetSkeletalMeshAsset() || !Source->IsVisible()) return nullptr;
    UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, AfterimageMaterialPath);
    if (!Base) return nullptr;
    while (Ghosts.Num() >= MaxGhosts)
    {
        if (auto* Old = Ghosts[0].Body.Get()) Old->DestroyComponent();
        if (auto* OldBlade = Ghosts[0].Blade.Get()) OldBlade->DestroyComponent();
        Ghosts.RemoveAt(0);
    }
    auto* Material = UMaterialInstanceDynamic::Create(Base, Pawn);
    Material->SetVectorParameterValue(TEXT("Tint"), Tint); Material->SetScalarParameterValue(TEXT("Alpha"), Alpha);
    // Arrel's own mesh, frozen in this instant's pose where he stands; it does not follow him.
    auto* Body = NewObject<UPoseableMeshComponent>(Pawn, NAME_None, RF_Transient);
    Body->SetUsingAbsoluteLocation(true); Body->SetUsingAbsoluteRotation(true); Body->SetUsingAbsoluteScale(true);
    Body->SetCollisionEnabled(ECollisionEnabled::NoCollision); Body->SetGenerateOverlapEvents(false);
    Body->SetCastShadow(false); Body->bReceivesDecals = false;
    Body->SetSkinnedAssetAndUpdate(Source->GetSkeletalMeshAsset());
    Body->SetWorldTransform(Source->GetComponentTransform());
    Body->RegisterComponent();
    Body->CopyPoseFromSkeletalComponent(Source);
    for (int32 I = 0; I < Body->GetNumMaterials(); ++I) Body->SetMaterial(I, Material);
    FMemoriaAfterimageGhost Ghost; Ghost.Body = Body; Ghost.Material = Material; Ghost.Life = Life; Ghost.Alpha = Alpha; Ghost.Tint = Tint;
    // The drawn blade goes with him.
    UStaticMeshComponent* Blade = Figure->IsSwordDrawn() ? Figure->GetBlade() : nullptr;
    if (Blade && Blade->GetStaticMesh() && Blade->IsVisible())
    {
        auto* Copy = NewObject<UStaticMeshComponent>(Pawn, NAME_None, RF_Transient);
        Copy->SetUsingAbsoluteLocation(true); Copy->SetUsingAbsoluteRotation(true); Copy->SetUsingAbsoluteScale(true);
        Copy->SetCollisionEnabled(ECollisionEnabled::NoCollision); Copy->SetGenerateOverlapEvents(false); Copy->SetCastShadow(false);
        Copy->SetStaticMesh(Blade->GetStaticMesh()); Copy->SetWorldTransform(Blade->GetComponentTransform());
        Copy->RegisterComponent();
        for (int32 I = 0; I < Copy->GetNumMaterials(); ++I) Copy->SetMaterial(I, Material);
        Ghost.Blade = Copy;
    }
    Ghosts.Add(Ghost);
    return Body;
}
bool UMemoriaFieldCombatSubsystem::TryPerfectDodge(AMemoriaFieldMonster* Monster, bool bContactResolved)
{
    APawn* Pawn = Player.Get(); UWorld* World = GetWorld();
    if (!Pawn || !Monster || !World || bDefeated || bDodgeSpent || DashLeft <= 0.f || DashTime - DashLeft > PerfectDodgeWindow) return false;
    // The blow must have been meant for where Arrel stood (or where he is): a swing at empty air is no dodge.
    const float Reach = Monster->Spec().Reach + 40.f;
    if (!bContactResolved && FVector::Dist2D(DashOrigin, Monster->GetActorLocation()) > Reach
        && FVector::Dist2D(Pawn->GetActorLocation(), Monster->GetActorLocation()) > Reach) return false;
    bDodgeSpent = true; ++PerfectDodges; EchoLeft = EchoCounterTime;
    Monster->Stun(AfterimageStun);
    // The world slows for a breath (real time), unless a hit stop or the burn picker holds it already.
    SlowLeft = SlowPeak = AfterimageSlowTime;
    if (!bPicking && HitStopLeft <= 0.f) UGameplayStatics::SetGlobalTimeDilation(World, AfterimageSlowDilation);
    // The faint ghost the dash left where it began takes the blow: it flares violet and lingers.
    FMemoriaAfterimageGhost* Struck = Ghosts.FindByPredicate([&](const FMemoriaAfterimageGhost& G) { return G.Body.IsValid() && G.Body == DashGhostBody; });
    if (!Struck && SpawnGhost(GhostDodgeAlpha, GhostDodgeLife, DodgeTint)) Struck = &Ghosts.Last();
    if (Struck)
    {
        Struck->Age = 0.f; Struck->Life = GhostDodgeLife; Struck->Alpha = GhostDodgeAlpha; Struck->Tint = DodgeTint;
        if (auto* Material = Struck->Material.Get()) Material->SetVectorParameterValue(TEXT("Tint"), DodgeTint);
    }
    const auto* Run = World->GetGameInstance() ? World->GetGameInstance()->GetSubsystem<UMemoriaRunSubsystem>() : nullptr;
    const bool Ko = Run && Run->GetRunSnapshot().CurrentLocale == TEXT("ko");
    const FVector At = DashOrigin + FVector(0, 0, 100.f);
    Popup(At + FVector(0, 0, 80.f), 0.f, true, Ko ? TEXT("잔상!") : TEXT("Afterimage!"), FLinearColor(.84f, .76f, 1.f), true);
    Burst(At, 16, DodgeTint, 380.f);
    Impact(At, DodgeTint, 130.f, true);
    if (auto* Audio = World->GetGameInstance() ? World->GetGameInstance()->GetSubsystem<UMemoriaAudioSubsystem>() : nullptr) Audio->PlaySfx(TEXT("shield"));
    return true;
}
void UMemoriaFieldCombatSubsystem::TickAfterimage(float DeltaSeconds, float Real)
{
    UWorld* World = GetWorld();
    EchoLeft = FMath::Max(0.f, EchoLeft - DeltaSeconds);
    if (SlowLeft > 0.f && (SlowLeft -= Real) <= 0.f)
    {
        SlowLeft = 0.f;
        if (!bPicking && HitStopLeft <= 0.f && World) UGameplayStatics::SetGlobalTimeDilation(World, 1.f);
    }
    const auto* Settings = SettingsOf(World);
    // A second, fainter trail ghost partway through the dash (not under Reduce Motion).
    if (DashLeft > 0.f && !bDashTrailSecond && DashTime - DashLeft >= GhostDashInterval)
    {
        bDashTrailSecond = true;
        if (!Settings || !Settings->IsReduceMotion()) SpawnGhost(GhostDashAlpha * .7f, GhostDashLife * .85f, DashTint);
    }
    // The ghosts fade on world time, so an Afterimage lingers through its own slow.
    for (int32 I = Ghosts.Num() - 1; I >= 0; --I)
    {
        FMemoriaAfterimageGhost& G = Ghosts[I];
        G.Age += DeltaSeconds;
        const float Left = FMath::Clamp(1.f - G.Age / G.Life, 0.f, 1.f);
        if (Left <= 0.f || !G.Body.IsValid())
        {
            if (auto* Body = G.Body.Get()) Body->DestroyComponent();
            if (auto* Blade = G.Blade.Get()) Blade->DestroyComponent();
            Ghosts.RemoveAt(I);
            continue;
        }
        if (auto* Material = G.Material.Get()) Material->SetScalarParameterValue(TEXT("Alpha"), G.Alpha * FMath::Pow(Left, 1.5f));
    }
    // The drained, violet-edged world of the slow.
    const float Weight = GetAfterimageWeight() * (Settings && Settings->IsReduceFlash() ? .5f : 1.f);
    UPostProcessComponent* Post = AfterimagePost.Get();
    if (!Post && Weight > 0.f)
        if (APawn* Pawn = Player.Get())
        {
            Post = NewObject<UPostProcessComponent>(Pawn, NAME_None, RF_Transient);
            Post->bUnbound = true; Post->Priority = 10.f; Post->BlendWeight = 0.f;
            Post->Settings.bOverride_ColorSaturation = true; Post->Settings.ColorSaturation = FVector4(1, 1, 1, .35f);
            Post->Settings.bOverride_ColorGain = true; Post->Settings.ColorGain = FVector4(.94f, .92f, 1.08f, 1.f);
            Post->Settings.bOverride_VignetteIntensity = true; Post->Settings.VignetteIntensity = .75f;
            Post->Settings.bOverride_SceneFringeIntensity = true; Post->Settings.SceneFringeIntensity = 1.6f;
            Post->RegisterComponent();
            AfterimagePost = Post;
        }
    if (Post) Post->BlendWeight = Weight;
}
void UMemoriaFieldCombatSubsystem::ClearAfterimage()
{
    for (const FMemoriaAfterimageGhost& G : Ghosts)
    {
        if (auto* Body = G.Body.Get()) Body->DestroyComponent();
        if (auto* Blade = G.Blade.Get()) Blade->DestroyComponent();
    }
    Ghosts.Reset(); DashGhostBody.Reset();
    const bool bWasSlow = SlowLeft > 0.f;
    EchoLeft = SlowLeft = SlowPeak = 0.f; bDodgeSpent = bEchoSwing = bDashTrailSecond = false;
    if (auto* Post = AfterimagePost.Get()) Post->BlendWeight = 0.f;
    if (bWasSlow && !bPicking && HitStopLeft <= 0.f && GetWorld()) UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.f);
}
