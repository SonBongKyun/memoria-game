#include "Presentation/MemoriaVerdanPresentation.h"
#include "Presentation/MemoriaVerdanArt.h"
#include "Framework/MemoriaFieldPawn.h"
#include "Interaction/MemoriaMaletActor.h"
#include "Engine/World.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "PaperSpriteComponent.h"
#include "PaperSprite.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#if WITH_EDITOR
#include "TextureCompiler.h"
#include "Engine/Texture2D.h"
#endif

AMemoriaVerdanPresentation::AMemoriaVerdanPresentation()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PostPhysics;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("PresentationRoot")));
    SetActorEnableCollision(false);
}
UPaperSpriteComponent* AMemoriaVerdanPresentation::Picture(const FString& Name, const FVector& Location, FVector Scale)
{
    auto* Component = NewObject<UPaperSpriteComponent>(this);
    AddInstanceComponent(Component); Component->SetupAttachment(RootComponent);
    Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Component->SetGenerateOverlapEvents(false); Component->SetCastShadow(false);
    Component->SetSprite(MemoriaVerdanArt::LoadSprite(Name));
    Component->SetRelativeRotation(FRotator(0, 0, 90));
    Component->SetRelativeLocation(Location); Component->SetRelativeScale3D(Scale);
    Component->RegisterComponent();
    return Component;
}
UStaticMeshComponent* AMemoriaVerdanPresentation::SoftQuad(const FVector& Location, const FVector& Scale, FLinearColor Tint, float Alpha)
{
    auto* Component = NewObject<UStaticMeshComponent>(this);
    AddInstanceComponent(Component); Component->SetupAttachment(RootComponent);
    Component->SetCollisionEnabled(ECollisionEnabled::NoCollision); Component->SetGenerateOverlapEvents(false);
    Component->SetCastShadow(false);
    Component->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane")));
    Component->SetRelativeLocation(Location); Component->SetRelativeScale3D(Scale);
    auto* Material = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Memoria/Presentation/Verdan/M_SoftLight.M_SoftLight"));
    auto* Instance = UMaterialInstanceDynamic::Create(Material, this);
    if (Instance)
    {
        Instance->SetVectorParameterValue(TEXT("Tint"), Tint); Instance->SetScalarParameterValue(TEXT("Alpha"), Alpha);
        Component->SetMaterial(0, Instance);
        if (Tint != FLinearColor::Black) LampLights.Add(Instance);
    }
    Component->RegisterComponent();
    return Component;
}
void AMemoriaVerdanPresentation::BeginPlay()
{
    Super::BeginPlay();
    Player = Cast<AMemoriaFieldPawn>(GetWorld()->GetFirstPlayerController()->GetPawn());
    // The complete source atlas stays intact; regions reuse its painted stone and facades.
#if WITH_EDITOR
    for (const auto& Entry : MemoriaVerdanArt::Textures())
    {
        const FString Name = FString(TEXT("T_")) + Entry.Name;
        if (auto* Texture = LoadObject<UTexture2D>(nullptr, *(MemoriaVerdanArt::Package(Name) + TEXT(".") + Name)))
            FTextureCompilingManager::Get().FinishCompilation({Texture});
    }
#endif
    for (const TCHAR* Facing : {TEXT("Down"), TEXT("Up"), TEXT("Left"), TEXT("Right")})
    {
        PlayerArt.Add(MemoriaVerdanArt::LoadSprite(FString(TEXT("Arrel")) + Facing));
        for (int32 Frame = 0; Frame < 4; ++Frame)
            PlayerArt.Add(MemoriaVerdanArt::LoadSprite(FString::Printf(TEXT("ArrelWalk%s%d"), Facing, Frame)));
    }
    // Hide only the seven known foundation surfaces. Their physical bodies are untouched.
    for (TActorIterator<AStaticMeshActor> It(GetWorld()); It; ++It)
    {
        const FVector P = It->GetActorLocation();
        if (P.Equals(FVector(0, 0, -30)) || P.Equals(FVector(-900, 0, 0)) || P.Equals(FVector(900, 0, 0)) ||
            P.Equals(FVector(0, -600, 0)) || P.Equals(FVector(0, 600, 0)) ||
            P.Equals(FVector(-400, 200, -5)) || P.Equals(FVector(400, 200, -5)))
            It->GetStaticMeshComponent()->SetHiddenInGame(true);
    }
    for (int32 X = 0; X < 12; ++X) for (int32 Y = 0; Y < 8; ++Y)
        Picture(TEXT("FloorInterior"), FVector(-825 + X * 150, -525 + Y * 150, -10), FVector(150.0/256.0, 1, 150.0/256.0));
    Picture(TEXT("North"), FVector(0, 590, 12), FVector(1800.0/1448.0, 1, 1.24));
    Picture(TEXT("South"), FVector(0, -590, 18), FVector(1800.0/1448.0, 1, 1.24));
    for (double Y : {-300.0, 300.0})
    {
        Picture(TEXT("West"), FVector(-890, Y, 15), FVector(1.2, 1, 600.0/540.0));
        Picture(TEXT("East"), FVector(890, Y, 15), FVector(1.2, 1, 600.0/540.0));
    }
    // Plinth silhouettes occupy the two existing 100 x 100 collision footprints.
    for (double X : {-400.0, 400.0})
    {
        Picture(TEXT("Plinth"), FVector(X, 200, 25), FVector(100.0/180.0, 1, 100.0/60.0));
        Picture(TEXT("Lantern"), FVector(X, 205, 42));
        SoftQuad(FVector(X, 200, -4), FVector(2.8, 2.8, 1), FLinearColor(0.5f, 0.23f, 0.045f), 0.5f);
    }
    for (TActorIterator<AMemoriaMaletActor> It(GetWorld()); It; ++It)
    {
        Malet = *It;
        TArray<UStaticMeshComponent*> Meshes; It->GetComponents(Meshes);
        for (auto* Component : Meshes) Component->SetHiddenInGame(true);
        TArray<UTextRenderComponent*> Labels; It->GetComponents(Labels);
        for (auto* Component : Labels) Component->SetHiddenInGame(true);
        const FVector P = It->GetActorLocation();
        MaletArt = Picture(TEXT("Malet"), FVector(P.X, P.Y, 50 - P.Y * 0.02));
        SoftQuad(FVector(P.X, P.Y, -3), FVector(0.72, 0.30, 1), FLinearColor::Black, 0.8f);
        break;
    }
    PlayerShadow = SoftQuad(FVector(0, 0, -2), FVector(0.80, 0.32, 1), FLinearColor::Black, 0.8f);
    if (Player.IsValid())
    {
        PreviousPosition = Player->GetActorLocation();
        Player->GetFieldSprite()->SetSprite(PlayerArt[0]);
    }
}
void AMemoriaVerdanPresentation::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    LightTime += DeltaSeconds;
    for (int32 I = 0; I < LampLights.Num(); ++I)
        LampLights[I]->SetScalarParameterValue(TEXT("Alpha"), 0.48f + 0.025f * FMath::Sin(LightTime * 1.7f + I));
    if (!Player.IsValid()) return;
    const FVector Position = Player->GetActorLocation();
    const FVector Step = Position - PreviousPosition;
    bWalking = Step.SizeSquared2D() > 0.0001 && DeltaSeconds > 0;
    if (bWalking)
    {
        Direction = FMath::Abs(Step.X) >= FMath::Abs(Step.Y) ? (Step.X > 0 ? TEXT("Right") : TEXT("Left")) : (Step.Y > 0 ? TEXT("Up") : TEXT("Down"));
        GaitTime += DeltaSeconds * FMath::Clamp(float(Step.Size2D() / DeltaSeconds / 200.0), 0.65f, 1.85f);
    }
    else GaitTime = 0;
    const int32 DirectionIndex = Direction == TEXT("Down") ? 0 : Direction == TEXT("Up") ? 1 : Direction == TEXT("Left") ? 2 : 3;
    const int32 Frame = bWalking ? 1 + (int32(GaitTime * 9) % 4) : 0;
    auto* Sprite = Player->GetFieldSprite();
    Sprite->SetSprite(PlayerArt[DirectionIndex * 5 + Frame]);
    // Camera looks down Z. Lower feet draw above higher feet; no actor/collision displacement.
    Sprite->SetRelativeLocation(FVector(0, 0, 50 - Position.Y * 0.02));
    PlayerShadow->SetWorldLocation(FVector(Position.X, Position.Y, -2));
    PreviousPosition = Position;
}
