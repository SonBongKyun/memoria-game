// S368: what a memory burn leaves in the world. The memory is seen one last time: its illustration (or, without
// one, a smoke of its grade's colour) rises above Arrel as he gathers the fire, and as the ring is released it
// burns away from the bottom edge behind a glowing seam, shedding embers. The burn marks the ground where it
// was released: soot that fades over ten seconds and a glow of the grade's colour that cools in three.
#include "Combat/MemoriaFieldCombatSubsystem.h"
#include "Combat/MemoriaFieldCombatTypes.h"
#include "Combat/MemoriaBurnArtS368.h"
#include "Settings/MemoriaSettingsSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "CollisionQueryParams.h"
using namespace MemoriaCombatTuning;
namespace
{
const TCHAR* PlaneMesh = TEXT("/Engine/BasicShapes/Plane.Plane");
const TCHAR* CardMaterialPath = TEXT("/Game/Memoria/Presentation/Combat/M_MemoryCard.M_MemoryCard");
const TCHAR* SoftLightPath = TEXT("/Game/Memoria/Presentation/Verdan/M_SoftLight.M_SoftLight");
UStaticMeshComponent* Quad(APawn* Owner, UMaterialInterface* Material, UMaterialInstanceDynamic*& OutInstance)
{
    auto* Component = NewObject<UStaticMeshComponent>(Owner, NAME_None, RF_Transient);
    Component->SetUsingAbsoluteLocation(true); Component->SetUsingAbsoluteRotation(true); Component->SetUsingAbsoluteScale(true);
    Component->SetCollisionEnabled(ECollisionEnabled::NoCollision); Component->SetGenerateOverlapEvents(false); Component->SetCastShadow(false);
    Component->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, PlaneMesh));
    OutInstance = UMaterialInstanceDynamic::Create(Material, Owner);
    Component->SetMaterial(0, OutInstance);
    Component->RegisterComponent();
    return Component;
}
FRotator CameraFacing(APawn* Pawn)
{
    // The card faces the camera, upright on screen: its face (+Z) toward the camera, its +X to the right.
    const auto* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
    const FRotator View = PC && PC->PlayerCameraManager ? PC->PlayerCameraManager->GetCameraRotation() : FRotator(-55.f, 90.f, 0.f);
    const FRotationMatrix Axes(View);
    return FRotationMatrix::MakeFromZX(-Axes.GetUnitAxis(EAxis::X), Axes.GetUnitAxis(EAxis::Y)).Rotator();
}
}
float UMemoriaFieldCombatSubsystem::GetNewestScorchAlpha() const
{
    if (Scorches.IsEmpty()) return 0.f;
    return ScorchSootAlpha * FMath::Square(FMath::Clamp(1.f - Scorches.Last().Age / ScorchLife, 0.f, 1.f));
}
void UMemoriaFieldCombatSubsystem::ShowMemoryCard(const FMemoriaBurnChoice& Choice)
{
    APawn* Pawn = Player.Get();
    UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, CardMaterialPath);
    if (!Pawn || !Base) return;
    if (auto* Old = MemoryCard.Get()) Old->DestroyComponent();
    UMaterialInstanceDynamic* Material = nullptr;
    UStaticMeshComponent* Card = Quad(Pawn, Base, Material);
    const TCHAR* Art = MemoriaBurnArt::Resolve(Choice.Id, Choice.Title);
    UTexture2D* Texture = Art ? LoadObject<UTexture2D>(nullptr, *MemoriaBurnArt::ObjectPath(Art)) : nullptr;
    CardArt = Texture ? FString(Art) : FString();
    CardEdge = BurnColor(Choice.Grade);
    // Every burn illustration is 16:9 (the imported texture is padded to a square, so its size cannot say).
    CardHeight = MemoryCardWidth * 9.f / 16.f;
    if (Texture) Material->SetTextureParameterValue(TEXT("Card"), Texture);
    Material->SetScalarParameterValue(TEXT("HasArt"), Texture ? 1.f : 0.f);
    Material->SetVectorParameterValue(TEXT("Edge"), CardEdge);
    Material->SetScalarParameterValue(TEXT("Burn"), 0.f); Material->SetScalarParameterValue(TEXT("Alpha"), 0.f);
    // The engine plane is 100 units; it lies in XY with its face up. Its U runs against the camera-right axis
    // used in CameraFacing, so X is mirrored (the material is two-sided) to show the picture the right way round.
    Card->SetWorldScale3D(FVector(-MemoryCardWidth / 100.f, CardHeight / 100.f, 1.f));
    MemoryCard = Card; MemoryCardMaterial = Material;
    CardBase = Pawn->GetActorLocation() + FVector(0, 0, MemoryCardLift); CardAge = 0.f; CardBurnAge = -1.f;
    TickBurnRelics(0.f);
}
void UMemoriaFieldCombatSubsystem::IgniteMemoryCard()
{
    if (MemoryCard.IsValid() && CardBurnAge < 0.f) CardBurnAge = 0.f;
}
void UMemoriaFieldCombatSubsystem::LeaveScorch(const FVector& At, int32 Grade)
{
    APawn* Pawn = Player.Get(); UWorld* World = GetWorld();
    UMaterialInterface* Soft = LoadObject<UMaterialInterface>(nullptr, SoftLightPath);
    if (!Pawn || !World || !Soft) return;
    // Find the floor under the release, a hair above it so the soot never fights the paving.
    FVector Floor = At - FVector(0, 0, 8.5f);
    FHitResult Hit; FCollisionQueryParams Query(SCENE_QUERY_STAT(MemoriaScorch), false, Pawn);
    if (World->LineTraceSingleByChannel(Hit, At + FVector(0, 0, 40.f), At - FVector(0, 0, 400.f), ECC_Visibility, Query)) Floor = Hit.ImpactPoint;
    while (Scorches.Num() >= MaxScorches)
    {
        if (auto* Soot = Scorches[0].Soot.Get()) Soot->DestroyComponent();
        if (auto* Glow = Scorches[0].Glow.Get()) Glow->DestroyComponent();
        Scorches.RemoveAt(0);
    }
    FMemoriaScorch Mark;
    const float Size = FMath::Min(BurnRadius[FMath::Clamp(Grade, 0, 4)] * 1.1f, 640.f);
    UMaterialInstanceDynamic* SootMaterial = nullptr; UMaterialInstanceDynamic* GlowMaterial = nullptr;
    UStaticMeshComponent* Soot = Quad(Pawn, Soft, SootMaterial);
    Soot->SetWorldLocationAndRotation(Floor + FVector(0, 0, 1.2f), FRotator::ZeroRotator); Soot->SetWorldScale3D(FVector(Size / 100.f, Size / 100.f, 1.f));
    SootMaterial->SetVectorParameterValue(TEXT("Tint"), FLinearColor(.035f, .025f, .02f)); SootMaterial->SetScalarParameterValue(TEXT("Alpha"), ScorchSootAlpha);
    UStaticMeshComponent* Glow = Quad(Pawn, Soft, GlowMaterial);
    Glow->SetWorldLocationAndRotation(Floor + FVector(0, 0, 1.6f), FRotator(0, FMath::FRandRange(0.f, 360.f), 0)); Glow->SetWorldScale3D(FVector(Size * .55f / 100.f, Size * .55f / 100.f, 1.f));
    GlowMaterial->SetVectorParameterValue(TEXT("Tint"), BurnColor(Grade) * 2.2f); GlowMaterial->SetScalarParameterValue(TEXT("Alpha"), ScorchGlowAlpha);
    Mark.Soot = Soot; Mark.Glow = Glow; Mark.SootMaterial = SootMaterial; Mark.GlowMaterial = GlowMaterial;
    Scorches.Add(Mark);
}
void UMemoriaFieldCombatSubsystem::TickBurnRelics(float DeltaSeconds)
{
    APawn* Pawn = Player.Get();
    const auto* Settings = GetWorld() && GetWorld()->GetGameInstance() ? GetWorld()->GetGameInstance()->GetSubsystem<UMemoriaSettingsSubsystem>() : nullptr;
    const bool bStill = Settings && Settings->IsReduceMotion();
    if (UStaticMeshComponent* Card = MemoryCard.Get())
    {
        CardAge += DeltaSeconds;
        if (CardBurnAge >= 0.f) CardBurnAge += DeltaSeconds;
        const float Burn = GetMemoryCardBurn();
        // Rises slowly while it is seen (not under Reduce Motion), facing the camera.
        const FVector At = CardBase + FVector(0, 0, bStill ? 0.f : MemoryCardRise * FMath::Clamp(CardAge / (BurnCastTime + MemoryCardBurnTime), 0.f, 1.f));
        const FRotator Facing = CameraFacing(Pawn);
        Card->SetWorldLocationAndRotation(At, Facing);
        if (auto* Material = MemoryCardMaterial.Get())
        {
            Material->SetScalarParameterValue(TEXT("Alpha"), FMath::Clamp(CardAge / MemoryCardFadeIn, 0.f, 1.f));
            Material->SetScalarParameterValue(TEXT("Burn"), Burn);
        }
        // Embers leave the burning seam (the seam climbs from the card's lower edge to its top).
        if (CardBurnAge >= 0.f && Burn < 1.f && DeltaSeconds > 0.f)
        {
            const FRotationMatrix Axes(Facing);
            const FVector Right = Axes.GetUnitAxis(EAxis::X), Up = -Axes.GetUnitAxis(EAxis::Y);
            for (int32 I = 0; I < 2; ++I)
            {
                const FVector Seam = At + Right * FMath::FRandRange(-.5f, .5f) * MemoryCardWidth + Up * (Burn - .5f) * CardHeight;
                Burst(Seam, 1, CardEdge, 160.f);
            }
        }
        if (CardBurnAge >= MemoryCardBurnTime) { Card->DestroyComponent(); MemoryCard.Reset(); MemoryCardMaterial.Reset(); }
    }
    // The soot fades over ten seconds; the glow cools in five, evenly.
    for (int32 I = Scorches.Num() - 1; I >= 0; --I)
    {
        FMemoriaScorch& Mark = Scorches[I];
        Mark.Age += DeltaSeconds;
        const float Soot = FMath::Clamp(1.f - Mark.Age / ScorchLife, 0.f, 1.f), Glow = FMath::Clamp(1.f - Mark.Age / ScorchGlowLife, 0.f, 1.f);
        if (auto* Material = Mark.SootMaterial.Get()) Material->SetScalarParameterValue(TEXT("Alpha"), ScorchSootAlpha * Soot * Soot);
        if (auto* Material = Mark.GlowMaterial.Get()) Material->SetScalarParameterValue(TEXT("Alpha"), ScorchGlowAlpha * Glow * (bStill ? 1.f : .85f + .15f * FMath::Sin(Mark.Age * 9.f)));
        if (Glow <= 0.f) if (auto* Component = Mark.Glow.Get()) { Component->DestroyComponent(); Mark.Glow.Reset(); }
        if (Soot <= 0.f)
        {
            if (auto* Component = Mark.Soot.Get()) Component->DestroyComponent();
            Scorches.RemoveAt(I);
        }
    }
}
void UMemoriaFieldCombatSubsystem::ClearBurnRelics()
{
    if (auto* Card = MemoryCard.Get()) Card->DestroyComponent();
    MemoryCard.Reset(); MemoryCardMaterial.Reset(); CardBurnAge = -1.f; CardArt.Reset();
    for (const FMemoriaScorch& Mark : Scorches)
    {
        if (auto* Soot = Mark.Soot.Get()) Soot->DestroyComponent();
        if (auto* Glow = Mark.Glow.Get()) Glow->DestroyComponent();
    }
    Scorches.Reset();
}
