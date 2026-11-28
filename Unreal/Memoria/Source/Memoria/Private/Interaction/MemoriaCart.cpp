#include "Interaction/MemoriaCart.h"
#include "Interaction/MemoriaEliaCompanion.h"
#include "Interaction/MemoriaTobiasCompanion.h"
#include "Presentation/MemoriaFieldCharacterComponent.h"
#include "Framework/MemoriaFieldPawn.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PawnMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"
#include "Misc/App.h"
#include "UObject/ConstructorHelpers.h"
#if WITH_EDITOR
#include "Materials/Material.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "MaterialShared.h"
#include "ShaderCompiler.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#endif

namespace
{
const TCHAR* WoodPackage = TEXT("/Game/Memoria/Presentation/FieldLooks/M_CartWood");
const TCHAR* WoodObject = TEXT("/Game/Memoria/Presentation/FieldLooks/M_CartWood.M_CartWood");
const TCHAR* HorsePackage = TEXT("/Game/Memoria/Presentation/FieldLooks/M_CartHorse");
const TCHAR* HorseObject = TEXT("/Game/Memoria/Presentation/FieldLooks/M_CartHorse.M_CartHorse");
const TCHAR* HoodPackage = TEXT("/Game/Memoria/Presentation/FieldLooks/M_CartHood");
const TCHAR* HoodObject = TEXT("/Game/Memoria/Presentation/FieldLooks/M_CartHood.M_CartHood");

#if WITH_EDITOR
bool SaveCartLook(const TCHAR* PackageName, const FLinearColor& Color, float Roughness)
{
    UPackage* Package = CreatePackage(PackageName);
    if (!Package) return false;
    const FString AssetName = FPaths::GetBaseFilename(PackageName);
    UMaterial* Surface = NewObject<UMaterial>(Package, *AssetName, RF_Public | RF_Standalone);
    if (!Surface) return false;
    Surface->SetShadingModel(MSM_DefaultLit);
    Surface->TwoSided = false;
    auto* Base = NewObject<UMaterialExpressionConstant3Vector>(Surface);
    auto* Rough = NewObject<UMaterialExpressionConstant>(Surface);
    Surface->GetExpressionCollection().AddExpression(Base);
    Surface->GetExpressionCollection().AddExpression(Rough);
    Base->Constant = Color;
    Rough->R = Roughness;
    if (UMaterialEditorOnlyData* Editor = Surface->GetEditorOnlyData())
    {
        Editor->BaseColor.Connect(0, Base);
        Editor->Roughness.Connect(0, Rough);
    }
    Surface->PostEditChange();
    if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
    const FMaterialResource* Resource = Surface->GetMaterialResource(GMaxRHIShaderPlatform);
    if (!Resource || Resource->GetCompileErrors().Num())
    {
        if (Resource)
        {
            for (const FString& CompileError : Resource->GetCompileErrors())
                UE_LOG(LogTemp, Error, TEXT("Cart look %s: %s"), *AssetName, *CompileError);
        }
        return false;
    }
    const FString File = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(File), true);
    FSavePackageArgs Args;
    Args.TopLevelFlags = RF_Public | RF_Standalone;
    Args.SaveFlags = SAVE_NoError;
    return UPackage::SavePackage(Package, Surface, *File, Args);
}
#endif

UMaterialInterface* CartLook(const TCHAR* PackageName, const TCHAR* ObjectPath, const FLinearColor& Color, float Roughness)
{
    if (UMaterialInterface* Loaded = LoadObject<UMaterialInterface>(nullptr, ObjectPath, nullptr, LOAD_NoWarn | LOAD_Quiet))
        return Loaded;
#if WITH_EDITOR
    if (!IsRunningCommandlet() && !FPackageName::DoesPackageExist(PackageName))
        SaveCartLook(PackageName, Color, Roughness);
#endif
    return LoadObject<UMaterialInterface>(nullptr, ObjectPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
}

UMemoriaFieldCharacterComponent* FindArrel(const APawn* Pawn)
{
    if (!Pawn || !Pawn->GetRootComponent()) return nullptr;
    TArray<USceneComponent*> Children;
    Pawn->GetRootComponent()->GetChildrenComponents(true, Children);
    for (USceneComponent* Child : Children)
        if (auto* Figure = Cast<UMemoriaFieldCharacterComponent>(Child))
            if (Figure->GetCharacterId().Equals(TEXT("Arrel"), ESearchCase::IgnoreCase))
                return Figure;
    return nullptr;
}

FString FacingOf(const FVector& Forward)
{
    return FMath::Abs(Forward.X) >= FMath::Abs(Forward.Y)
        ? (Forward.X >= 0.f ? TEXT("Right") : TEXT("Left"))
        : (Forward.Y >= 0.f ? TEXT("Up") : TEXT("Down"));
}
}

AMemoriaCart::AMemoriaCart()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PostPhysics;
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);
    Root->SetMobility(EComponentMobility::Movable);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    UStaticMesh* CubeAsset = CubeFinder.Object;
    UStaticMesh* WheelAsset = CylinderFinder.Object;

    auto Box = [&](const TCHAR* Name, USceneComponent* Parent, const FVector& Loc, const FVector& Scale, const FRotator& Rot = FRotator::ZeroRotator)
    {
        UStaticMeshComponent* Mesh = CreateDefaultSubobject<UStaticMeshComponent>(Name);
        Mesh->SetupAttachment(Parent);
        Mesh->SetStaticMesh(CubeAsset);
        Mesh->SetRelativeLocation(Loc);
        Mesh->SetRelativeRotation(Rot);
        Mesh->SetRelativeScale3D(Scale);
        Mesh->SetMobility(EComponentMobility::Movable);
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Mesh->SetGenerateOverlapEvents(false);
        Mesh->SetCastShadow(true);
        return Mesh;
    };

    // Uncovered bed. The cube is 100cm, so this is 240 x 130 x 8 with its top at Z = 74.
    Box(TEXT("CartBed"), Root, FVector(-70.f, 0.f, 70.f), FVector(2.4f, 1.3f, 0.08f));
    Box(TEXT("CartSideL"), Root, FVector(-70.f, -70.f, 85.f), FVector(2.4f, 0.08f, 0.22f));
    Box(TEXT("CartSideR"), Root, FVector(-70.f, 70.f, 85.f), FVector(2.4f, 0.08f, 0.22f));
    Box(TEXT("CartSideF"), Root, FVector(50.f, 0.f, 85.f), FVector(0.08f, 1.3f, 0.22f));
    Box(TEXT("CartSideB"), Root, FVector(-190.f, 0.f, 85.f), FVector(0.08f, 1.3f, 0.22f));
    Box(TEXT("CartShaftL"), Root, FVector(115.f, -28.f, 78.f), FVector(1.3f, 0.06f, 0.06f));
    Box(TEXT("CartShaftR"), Root, FVector(115.f, 28.f, 78.f), FVector(1.3f, 0.06f, 0.06f));

    auto Wheel = [&](const TCHAR* Name, float Side)
    {
        UStaticMeshComponent* Mesh = CreateDefaultSubobject<UStaticMeshComponent>(Name);
        Mesh->SetupAttachment(Root);
        Mesh->SetStaticMesh(WheelAsset);
        Mesh->SetRelativeLocation(FVector(-70.f, Side, 36.f));
        Mesh->SetRelativeRotation(FRotationMatrix::MakeFromZ(FVector::YAxisVector).Rotator());
        Mesh->SetRelativeScale3D(FVector(0.72f, 0.72f, 0.10f));
        Mesh->SetMobility(EComponentMobility::Movable);
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Mesh->SetGenerateOverlapEvents(false);
        Mesh->SetCastShadow(true);
        return Mesh;
    };
    Wheel(TEXT("CartWheelL"), -82.f);
    Wheel(TEXT("CartWheelR"), 82.f);

    Box(TEXT("CartHorseBody"), Root, FVector(260.f, 0.f, 108.f), FVector(1.55f, 0.42f, 0.55f));
    Box(TEXT("CartHorseNeck"), Root, FVector(335.f, 0.f, 142.f), FVector(0.28f, 0.18f, 0.48f), FRotator(-42.f, 0.f, 0.f));
    Box(TEXT("CartHorseHead"), Root, FVector(368.f, 0.f, 168.f), FVector(0.38f, 0.16f, 0.20f));
    Box(TEXT("CartHorseLeg0"), Root, FVector(205.f, -14.f, 42.f), FVector(0.10f, 0.10f, 0.84f));
    Box(TEXT("CartHorseLeg1"), Root, FVector(205.f, 14.f, 42.f), FVector(0.10f, 0.10f, 0.84f));
    Box(TEXT("CartHorseLeg2"), Root, FVector(310.f, -14.f, 42.f), FVector(0.10f, 0.10f, 0.84f));
    Box(TEXT("CartHorseLeg3"), Root, FVector(310.f, 14.f, 42.f), FVector(0.10f, 0.10f, 0.84f));
    Box(TEXT("CartHorseTail"), Root, FVector(178.f, 0.f, 118.f), FVector(0.32f, 0.06f, 0.06f));

    // Hips stay facing the horse. Head and shoulders live under DriverUpper so one yaw turns both.
    Box(TEXT("DriverHips"), Root, FVector(28.f, 0.f, 90.f), FVector(0.32f, 0.38f, 0.22f));
    DriverUpper = CreateDefaultSubobject<USceneComponent>(TEXT("DriverUpper"));
    DriverUpper->SetupAttachment(Root);
    DriverUpper->SetRelativeLocation(FVector(28.f, 0.f, 118.f));
    DriverUpper->SetMobility(EComponentMobility::Movable);
    Box(TEXT("DriverTorso"), DriverUpper, FVector(0.f, 0.f, 8.f), FVector(0.34f, 0.44f, 0.40f));
    Box(TEXT("DriverShoulderL"), DriverUpper, FVector(0.f, -20.f, 16.f), FVector(0.16f, 0.18f, 0.14f));
    Box(TEXT("DriverShoulderR"), DriverUpper, FVector(0.f, 20.f, 16.f), FVector(0.16f, 0.18f, 0.14f));
    // A closed hood. There is no face mesh under it.
    Box(TEXT("DriverHood"), DriverUpper, FVector(2.f, 0.f, 32.f), FVector(0.30f, 0.28f, 0.32f));

    auto Seat = [&](const TCHAR* Name, const FVector& Loc)
    {
        USceneComponent* Point = CreateDefaultSubobject<USceneComponent>(Name);
        Point->SetupAttachment(Root);
        Point->SetRelativeLocation(Loc);
        Point->SetMobility(EComponentMobility::Movable);
        return Point;
    };
    SeatArrel = Seat(TEXT("SeatArrel"), FVector(-80.f, 0.f, 74.f));
    SeatElia = Seat(TEXT("SeatElia"), FVector(-80.f, -36.f, 74.f));
    SeatTobias = Seat(TEXT("SeatTobias"), FVector(-80.f, 36.f, 74.f));

    // Hard references so a later cook packs the saved looks. Quiet until the editor has written them.
    ConstructorHelpers::FObjectFinderOptional<UMaterialInterface> Wood(WoodObject, LOAD_Quiet | LOAD_NoWarn);
    ConstructorHelpers::FObjectFinderOptional<UMaterialInterface> Horse(HorseObject, LOAD_Quiet | LOAD_NoWarn);
    ConstructorHelpers::FObjectFinderOptional<UMaterialInterface> Hood(HoodObject, LOAD_Quiet | LOAD_NoWarn);
    WoodMaterial = Wood.Get();
    HorseMaterial = Horse.Get();
    HoodMaterial = Hood.Get();
}

void AMemoriaCart::EnsureLooks()
{
    const FLinearColor WoodColor(0.45f, 0.32f, 0.18f);
    const FLinearColor HorseColor(0.18f, 0.13f, 0.09f);
    const FLinearColor HoodColor(0.22f, 0.21f, 0.20f);
    if (!WoodMaterial) WoodMaterial = CartLook(WoodPackage, WoodObject, WoodColor, 0.82f);
    if (!HorseMaterial) HorseMaterial = CartLook(HorsePackage, HorseObject, HorseColor, 0.72f);
    if (!HoodMaterial) HoodMaterial = CartLook(HoodPackage, HoodObject, HoodColor, 0.88f);
}

void AMemoriaCart::ApplyLooks()
{
    EnsureLooks();
    TArray<UStaticMeshComponent*> Meshes;
    GetComponents<UStaticMeshComponent>(Meshes);
    for (UStaticMeshComponent* Mesh : Meshes)
    {
        if (!Mesh) continue;
        const FString Name = Mesh->GetName();
        const bool bDriver = Name.StartsWith(TEXT("Driver"));
        const bool bHorse = Name.StartsWith(TEXT("CartHorse")) || Name.StartsWith(TEXT("CartWheel"));
        UMaterialInterface* Look = bDriver ? HoodMaterial.Get() : (bHorse ? HorseMaterial.Get() : WoodMaterial.Get());
        if (!Look)
        {
            if (UMaterialInterface* Basic = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"), nullptr, LOAD_NoWarn | LOAD_Quiet))
                if (UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(Basic, Mesh))
                {
                    const FLinearColor Fallback = bDriver ? FLinearColor(0.22f, 0.21f, 0.20f) : (bHorse ? FLinearColor(0.18f, 0.13f, 0.09f) : FLinearColor(0.45f, 0.32f, 0.18f));
                    Mid->SetVectorParameterValue(TEXT("Color"), Fallback);
                    Look = Mid;
                }
        }
        if (Look) Mesh->SetMaterial(0, Look);
    }
}

void AMemoriaCart::BeginPlay()
{
    Super::BeginPlay();
    if (Root) Root->SetMobility(EComponentMobility::Movable);
    SetActorTickEnabled(true);
    ApplyLooks();
    // Attaching the pawn on Board can leave this actor off the world tick list.
    // The core ticker still runs once a frame, and Advance refuses a second step.
    if (!DriveTicker.IsValid())
    {
        DriveTicker = FTSTicker::GetCoreTicker().AddTicker(TEXT("MemoriaCart"), 0.f,
            [Weak = TWeakObjectPtr<AMemoriaCart>(this)](float DeltaSeconds)
            {
                if (AMemoriaCart* Cart = Weak.Get()) Cart->Advance(DeltaSeconds);
                return Weak.IsValid();
            });
    }
}

void AMemoriaCart::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (DriveTicker.IsValid())
    {
        FTSTicker::GetCoreTicker().RemoveTicker(DriveTicker);
        DriveTicker.Reset();
    }
    Super::EndPlay(EndPlayReason);
}

void AMemoriaCart::ConsumeRouteProgress()
{
    RouteIndex = 0;
    while (RouteIndex < Route.Num() && FVector::Dist(GetActorLocation(), Route[RouteIndex]) <= ArrivalRadius)
        ++RouteIndex;
    bArrived = false;
    bDriving = bRiding && RouteIndex < Route.Num();
    if (bRiding && RouteIndex >= Route.Num())
    {
        bDriving = false;
        bArrived = true;
    }
}

void AMemoriaCart::SetRoute(const TArray<FVector>& WorldPoints)
{
    Route = WorldPoints;
    ConsumeRouteProgress();
}

AMemoriaCart* AMemoriaCart::SpawnOnRoute(UWorld* World, const TArray<FVector>& WorldPoints)
{
    if (!World || WorldPoints.Num() == 0) return nullptr;
    const FVector Start = WorldPoints[0];
    FRotator Yaw = FRotator::ZeroRotator;
    if (WorldPoints.Num() > 1)
    {
        const FVector Flat(WorldPoints[1].X - Start.X, WorldPoints[1].Y - Start.Y, 0.f);
        if (!Flat.IsNearlyZero()) Yaw = FRotator(0.f, Flat.Rotation().Yaw, 0.f);
    }
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AMemoriaCart* Cart = World->SpawnActor<AMemoriaCart>(Start, Yaw, Params);
    if (Cart) Cart->SetRoute(WorldPoints);
    return Cart;
}

void AMemoriaCart::FaceRiders()
{
    const FString Direction = FacingOf(GetActorForwardVector());
    if (UMemoriaFieldCharacterComponent* Figure = Arrel.Get()) Figure->Face(Direction);
    if (AMemoriaEliaCompanion* Companion = Elia.Get())
        if (UMemoriaFieldCharacterComponent* Figure = Companion->GetFigure()) Figure->Face(Direction);
    if (AMemoriaTobiasCompanion* Companion = Tobias.Get())
        if (UMemoriaFieldCharacterComponent* Figure = Companion->GetFigure()) Figure->Face(Direction);
}

bool AMemoriaCart::Board()
{
    if (bRiding) return true;
    UWorld* World = GetWorld();
    if (!World || !SeatArrel || !SeatElia || !SeatTobias) return false;
    APawn* Pawn = nullptr;
    if (APlayerController* Controller = World->GetFirstPlayerController()) Pawn = Controller->GetPawn();
    if (!Pawn)
        for (TActorIterator<AMemoriaFieldPawn> It(World); It; ++It) { Pawn = *It; break; }
    AMemoriaEliaCompanion* EliaActor = nullptr;
    for (TActorIterator<AMemoriaEliaCompanion> It(World); It; ++It) { EliaActor = *It; break; }
    AMemoriaTobiasCompanion* TobiasActor = nullptr;
    for (TActorIterator<AMemoriaTobiasCompanion> It(World); It; ++It) { TobiasActor = *It; break; }
    UMemoriaFieldCharacterComponent* ArrelFigure = FindArrel(Pawn);
    if (!Pawn || !ArrelFigure || !EliaActor || !EliaActor->GetFigure() || !TobiasActor || !TobiasActor->GetFigure())
        return false;

    if (AController* Controller = Pawn->GetController())
    {
        Controller->SetIgnoreMoveInput(true);
        bHeldInput = true;
    }
    if (UPawnMovementComponent* Move = Pawn->GetMovementComponent())
    {
        bRestoreMovementTick = true;
        bMovementTickWasEnabled = Move->IsComponentTickEnabled();
        Pawn->ConsumeMovementInputVector();
        Move->StopMovementImmediately();
        Move->SetComponentTickEnabled(false);
    }
    bRestoreEliaTick = true;
    bEliaTickWasEnabled = EliaActor->IsActorTickEnabled();
    EliaActor->SetActorTickEnabled(false);
    bRestoreTobiasTick = true;
    bTobiasTickWasEnabled = TobiasActor->IsActorTickEnabled();
    TobiasActor->SetActorTickEnabled(false);

    const FAttachmentTransformRules Snap = FAttachmentTransformRules::SnapToTargetNotIncludingScale;
    Pawn->AttachToComponent(SeatArrel, Snap);
    EliaActor->AttachToComponent(SeatElia, Snap);
    TobiasActor->AttachToComponent(SeatTobias, Snap);
    ArrelFigure->SetSeated(true);
    EliaActor->GetFigure()->SetSeated(true);
    TobiasActor->GetFigure()->SetSeated(true);

    Rider = Pawn;
    Arrel = ArrelFigure;
    Elia = EliaActor;
    Tobias = TobiasActor;
    bRiding = true;
    if (RouteIndex >= Route.Num())
    {
        bDriving = false;
        bArrived = true;
    }
    else
    {
        bDriving = true;
        bArrived = false;
    }
    FaceRiders();
    SetActorTickEnabled(true);
    return true;
}

void AMemoriaCart::GetDown()
{
    if (!bRiding) return;
    APawn* Pawn = Rider.Get();
    AMemoriaEliaCompanion* EliaActor = Elia.Get();
    AMemoriaTobiasCompanion* TobiasActor = Tobias.Get();
    if (UMemoriaFieldCharacterComponent* Figure = Arrel.Get()) Figure->SetSeated(false);
    if (EliaActor && EliaActor->GetFigure()) EliaActor->GetFigure()->SetSeated(false);
    if (TobiasActor && TobiasActor->GetFigure()) TobiasActor->GetFigure()->SetSeated(false);

    const FVector Ground = GetActorLocation();
    const FVector Right = GetActorRightVector();
    const FVector Back = -GetActorForwardVector();
    auto Place = [&](AActor* Actor, float RightCm, float BackCm)
    {
        if (!Actor) return;
        Actor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
        const FVector At = Ground + Right * RightCm + Back * BackCm;
        Actor->SetActorLocation(FVector(At.X, At.Y, Ground.Z));
    };
    Place(Pawn, 150.f, 20.f);
    Place(EliaActor, 150.f, 90.f);
    Place(TobiasActor, 210.f, 50.f);

    if (Pawn)
    {
        if (UPawnMovementComponent* Move = Pawn->GetMovementComponent())
        {
            Pawn->ConsumeMovementInputVector();
            Move->StopMovementImmediately();
            if (bRestoreMovementTick) Move->SetComponentTickEnabled(bMovementTickWasEnabled);
        }
        if (bHeldInput)
        {
            if (AController* Controller = Pawn->GetController()) Controller->SetIgnoreMoveInput(false);
            bHeldInput = false;
        }
    }
    if (EliaActor && bRestoreEliaTick) EliaActor->SetActorTickEnabled(bEliaTickWasEnabled);
    if (TobiasActor && bRestoreTobiasTick) TobiasActor->SetActorTickEnabled(bTobiasTickWasEnabled);
    bRestoreMovementTick = false;
    bRestoreEliaTick = false;
    bRestoreTobiasTick = false;
    bRiding = false;
    bDriving = false;
    Rider = nullptr;
    Arrel = nullptr;
    Elia = nullptr;
    Tobias = nullptr;
}

void AMemoriaCart::TurnDriverTowardBed()
{
    if (DriverUpper) DriverUpper->SetRelativeRotation(FRotator(0.f, 180.f, 0.f));
}

void AMemoriaCart::TurnDriverAhead()
{
    if (DriverUpper) DriverUpper->SetRelativeRotation(FRotator::ZeroRotator);
}

float AMemoriaCart::GetDriverYaw() const
{
    return DriverUpper ? DriverUpper->GetRelativeRotation().Yaw : 0.f;
}

bool AMemoriaCart::IsDriverTurnedTowardBed() const
{
    return FMath::Abs(FMath::FindDeltaAngleDegrees(GetDriverYaw(), 180.f)) < 8.f;
}

void AMemoriaCart::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    Advance(DeltaSeconds);
}

void AMemoriaCart::Advance(float DeltaSeconds)
{
    if (!bRiding) return;
    UWorld* World = GetWorld();
    if (World && World->IsPaused()) return;
    if (DrivenFrame == GFrameCounter) return;
    DrivenFrame = GFrameCounter;

    float StepTime = DeltaSeconds;
    if (FApp::UseFixedTimeStep()) StepTime = FApp::GetFixedDeltaTime();
    if (StepTime <= KINDA_SMALL_NUMBER)
    {
        const float WorldDelta = World ? World->GetDeltaSeconds() : 0.f;
        StepTime = WorldDelta > KINDA_SMALL_NUMBER ? WorldDelta : (1.f / 60.f);
    }

    if (APawn* Pawn = Rider.Get()) Pawn->ConsumeMovementInputVector();
    FaceRiders();
    if (!bDriving || !Route.IsValidIndex(RouteIndex))
    {
        if (bDriving)
        {
            bDriving = false;
            bArrived = true;
        }
        return;
    }
    const FVector Target = Route[RouteIndex];
    const FVector Here = GetActorLocation();
    const FVector Delta = Target - Here;
    const float Distance = Delta.Size();
    const FVector Flat(Delta.X, Delta.Y, 0.f);
    if (!Flat.IsNearlyZero()) SetActorRotation(FRotator(0.f, Flat.Rotation().Yaw, 0.f));
    const float Step = DriveSpeed * FMath::Max(StepTime, 0.f);
    const FVector Next = (Distance <= ArrivalRadius || Step >= Distance)
        ? Target
        : Here + Delta.GetSafeNormal() * Step;
    SetActorLocation(Next, false, nullptr, ETeleportType::TeleportPhysics);
    if (Next.Equals(Target, 0.1f) || Distance <= ArrivalRadius || Step >= Distance)
    {
        ++RouteIndex;
        if (RouteIndex >= Route.Num())
        {
            bDriving = false;
            bArrived = true;
        }
    }
}
