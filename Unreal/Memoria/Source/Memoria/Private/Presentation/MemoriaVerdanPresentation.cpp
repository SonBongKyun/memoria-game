#include "Presentation/MemoriaVerdanPresentation.h"
#include "Presentation/MemoriaVerdanArt.h"
#include "Framework/MemoriaFieldPawn.h"
#include "Interaction/MemoriaMaletActor.h"
#include "Engine/World.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/DirectionalLight.h"
#include "EngineUtils.h"
#include "PaperSpriteComponent.h"
#include "PaperSprite.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#if WITH_EDITOR
#include "TextureCompiler.h"
#include "Engine/Texture2D.h"
#endif
namespace
{
const TCHAR* SurfacePath = TEXT("/Game/Memoria/Presentation/Depth2/M_FocusSurface.M_FocusSurface");
const TCHAR* RoofPath = TEXT("/Game/Memoria/Presentation/Depth/SM_PitchedRoof.SM_PitchedRoof");
}
AMemoriaVerdanPresentation::AMemoriaVerdanPresentation()
{
    PrimaryActorTick.bCanEverTick = true; PrimaryActorTick.TickGroup = TG_PostPhysics;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("PresentationRoot")));
    SetActorEnableCollision(false);
}
UPaperSpriteComponent* AMemoriaVerdanPresentation::Picture(const FString& Name, const FVector& Location, FVector Scale)
{
    auto* Component = NewObject<UPaperSpriteComponent>(this);
    AddInstanceComponent(Component); Component->SetupAttachment(RootComponent);
    Component->SetCollisionEnabled(ECollisionEnabled::NoCollision); Component->SetGenerateOverlapEvents(false);
    const bool bLantern=Name==TEXT("MemoryLantern");
    Component->SetCastShadow(!bLantern);
    Component->SetSprite(bLantern ? LoadObject<UPaperSprite>(nullptr,TEXT("/Game/Memoria/Presentation/Depth2/SPR_MemoryLantern.SPR_MemoryLantern")) : MemoriaVerdanArt::LoadSprite(Name));
    Component->SetRelativeRotation(FRotator(0, 0, 42));
    Component->SetRelativeLocation(Location); Component->SetRelativeScale3D(Scale);
    Component->SetMaterial(0, LoadObject<UMaterialInterface>(nullptr, TEXT("/Paper2D/MaskedLitSpriteMaterial.MaskedLitSpriteMaterial")));
    Component->RegisterComponent(); return Component;
}
UStaticMeshComponent* AMemoriaVerdanPresentation::SoftQuad(const FVector& Location, const FVector& Scale, FLinearColor Tint, float Alpha)
{
    auto* Component = NewObject<UStaticMeshComponent>(this);
    AddInstanceComponent(Component); Component->SetupAttachment(RootComponent);
    Component->SetCollisionEnabled(ECollisionEnabled::NoCollision); Component->SetGenerateOverlapEvents(false); Component->SetCastShadow(false);
    Component->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane")));
    Component->SetRelativeLocation(Location); Component->SetRelativeScale3D(Scale);
    auto* Material = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Memoria/Presentation/Verdan/M_SoftLight.M_SoftLight"));
    auto* Instance = UMaterialInstanceDynamic::Create(Material, this);
    if (Instance)
    {
        Instance->SetVectorParameterValue(TEXT("Tint"), Tint); Instance->SetScalarParameterValue(TEXT("Alpha"), Alpha);
        Component->SetMaterial(0, Instance);
    }
    Component->RegisterComponent(); return Component;
}
UMaterialInstanceDynamic* AMemoriaVerdanPresentation::Surface(FName Name, FLinearColor Tint, float Mode, float Roughness, float Metallic)
{
    auto* Result = UMaterialInstanceDynamic::Create(LoadObject<UMaterialInterface>(nullptr, SurfacePath), this, Name);
    Result->SetVectorParameterValue(TEXT("Tint"), Tint); Result->SetScalarParameterValue(TEXT("Mode"), Mode);
    Result->SetScalarParameterValue(TEXT("Roughness"), Roughness); Result->SetScalarParameterValue(TEXT("Metallic"), Metallic);
    SurfaceMaterials.Add(Result); return Result;
}
void AMemoriaVerdanPresentation::Solid(const TCHAR* MeshName, UMaterialInterface* Material, FVector Position, FVector Scale, FRotator Rotation)
{
    const FString Key = FString(MeshName) + TEXT("|") + Material->GetName();
    auto& Batch = MeshBatches.FindOrAdd(Key);
    if (!Batch)
    {
        Batch = NewObject<UInstancedStaticMeshComponent>(this);
        AddInstanceComponent(Batch); Batch->SetupAttachment(RootComponent); Batch->SetMobility(EComponentMobility::Movable);
        Batch->SetCollisionEnabled(ECollisionEnabled::NoCollision); Batch->SetGenerateOverlapEvents(false); Batch->SetCanEverAffectNavigation(false);
        const FString Path = FString(MeshName) == TEXT("Roof") ? FString(RoofPath) : FString::Printf(TEXT("/Engine/BasicShapes/%s.%s"), MeshName, MeshName);
        Batch->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, *Path)); Batch->SetMaterial(0, Material);
        Batch->SetCastShadow(Material->GetName() != TEXT("M_Glow")); Batch->RegisterComponent();
    }
    Batch->AddInstance(FTransform(Rotation, Position, Scale));
}
void AMemoriaVerdanPresentation::Box(UMaterialInterface* Material, FVector Position, FVector Size, FRotator Rotation)
{ Solid(TEXT("Cube"), Material, Position, Size / 100.0, Rotation); }
void AMemoriaVerdanPresentation::Beam(UMaterialInterface* Material, FVector A, FVector B, float Width)
{ Box(Material, (A+B)*0.5, FVector(Width,Width,(B-A).Size()), FRotationMatrix::MakeFromZ(B-A).Rotator()); }
void AMemoriaVerdanPresentation::Lantern(FVector P, UMaterialInterface* Iron, UMaterialInterface* Glow, bool bShadow)
{
    // The intact original lantern illustration hangs from a real beam and light.
    Picture(TEXT("MemoryLantern"),P-FVector(0,0,28));
    Beam(Iron,P+FVector(0,0,34),P+FVector(0,0,49),3);
    auto* Light = NewObject<UPointLightComponent>(this); AddInstanceComponent(Light); Light->SetupAttachment(RootComponent);
    Light->SetRelativeLocation(P); Light->bUseInverseSquaredFalloff = false; Light->LightFalloffExponent = 2;
    Light->SetLightColor(FLinearColor(1.0f,0.45f,0.16f)); Light->SetIntensity(3.0f); Light->SetAttenuationRadius(420);
    Light->SetCastShadows(bShadow); Light->SetSourceRadius(8); Light->RegisterComponent(); LampLights.Add(Light);
}
void AMemoriaVerdanPresentation::Building(FVector P, FVector Size, UMaterialInterface* Wall, UMaterialInterface* Timber, UMaterialInterface* Roof, UMaterialInterface* Glow)
{
    const double W=Size.X,D=Size.Y,H=Size.Z;
    Box(Wall,P+FVector(0,0,H*.5),Size);
    Box(Timber,P+FVector(0,0,16),FVector(W+12,D+12,14));
    Box(Timber,P+FVector(0,0,H*.6),FVector(W+14,D+14,10));
    Box(Timber,P+FVector(0,0,H-5),FVector(W+20,D+20,12));
    for (double X : {-W*.5+5,W*.5-5})
        for (double Y : {-D*.5-3,D*.5+3}) Box(Timber,P+FVector(X,Y,H*.5),FVector(13,13,H));
    const double Front = -D*.5-5;
    Box(Timber,P+FVector(0,Front,66),FVector(62,7,126));
    for (double X : {-W*.28,W*.28})
    {
        Box(Timber,P+FVector(X,Front,151),FVector(62,9,93));
        Box(Glow,P+FVector(X,Front-5,151),FVector(48,3,76));
        Box(Timber,P+FVector(X,Front-8,151),FVector(5,4,78));
        Box(Timber,P+FVector(X,Front-8,151),FVector(50,4,5));
        for (double Side : {-1.0,1.0})
        {
            Box(Timber,P+FVector(X+Side*42,Front-8,151),FVector(19,8,86));
            Box(Roof,P+FVector(X+Side*42,Front-13,128),FVector(21,3,4));
            Box(Roof,P+FVector(X+Side*42,Front-13,175),FVector(21,3,4));
        }
        Beam(Timber,P+FVector(X-40,Front,H*.64),P+FVector(X+40,Front,H-10),7);
    }
    Solid(TEXT("Roof"),Roof,P+FVector(0,0,H),FVector((W+46)/100,(D+55)/100,2.3));
    Box(Timber,P+FVector(0,0,H+96),FVector(W+50,12,10));
    // Recessed masonry chimney and cap provide a readable rooftop silhouette.
    Box(Wall,P+FVector(W*.27,D*.1,H+80),FVector(43,45,150));
    Box(Timber,P+FVector(W*.27,D*.1,H+159),FVector(55,57,10));
}
void AMemoriaVerdanPresentation::Stall(FVector P, UMaterialInterface* Timber, UMaterialInterface* Cloth, UMaterialInterface* Iron, UMaterialInterface* Glow)
{
    Box(Timber,P+FVector(0,0,66),FVector(98,92,12));
    Box(Cloth,P+FVector(0,-44,30),FVector(95,4,58));
    for (double X : {-42.0,42.0}) for (double Y : {-38.0,38.0}) Box(Timber,P+FVector(X,Y,74),FVector(7,7,148));
    Solid(TEXT("Roof"),Cloth,P+FVector(0,0,145),FVector(1.4,1.28,0.75));
    auto* Glass=Surface(FName(*FString::Printf(TEXT("Bottles%.0f"),P.X)),FLinearColor(.08f,.24f,.23f),3,.23f,.35f);
    for (int32 I=0; I<5; ++I)
    {
        const FVector V=P+FVector(-30+I*15,-8,84);
        Solid(TEXT("Sphere"),Glass,V,FVector(.12,.12,.25));
        Solid(TEXT("Cylinder"),Iron,V+FVector(0,0,15),FVector(.035,.035,.075));
    }
    // Small bound ledgers share the existing stall footprint.
    Box(Cloth,P+FVector(24,22,76),FVector(22,25,5));
    Box(Timber,P+FVector(20,22,81),FVector(24,24,4),FRotator(0,12,0));
    Lantern(P+FVector(-35,-27,119),Iron,Glow,true);
}
void AMemoriaVerdanPresentation::BuildDepthEnvironment()
{
    auto* Stone=Surface(TEXT("Stone"),FLinearColor(.19f,.21f,.23f),0);
    auto* Plaster=Surface(TEXT("Plaster"),FLinearColor(.32f,.30f,.26f),0);
    auto* Timber=Surface(TEXT("Timber"),FLinearColor(.115f,.067f,.037f),1);
    auto* Slate=Surface(TEXT("Slate"),FLinearColor(.065f,.115f,.15f),0,.64f);
    auto* Iron=Surface(TEXT("Iron"),FLinearColor(.075f,.085f,.10f),3,.5f,.65f);
    auto* Wine=Surface(TEXT("WineCanvas"),FLinearColor(.25f,.065f,.075f),2);
    auto* Moss=Surface(TEXT("MossCanvas"),FLinearColor(.075f,.17f,.16f),2);
    auto* Glow=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Memoria/Presentation/Depth/M_Glow.M_Glow"));
    auto* Paving=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Memoria/Presentation/Depth/M_Paving.M_Paving"));
    auto* OuterGround=Surface(TEXT("OuterGround"),FLinearColor(.028f,.038f,.049f),3,1.0f);
    Box(OuterGround,FVector(0,0,-58),FVector(5200,4200,90));
    Solid(TEXT("Plane"),Paving,FVector(0,0,-10),FVector(18,12,1));
    // The curb marks the same four barriers as the retained collision bodies.
    for (double X : {-900.0,900.0}) Box(Stone,FVector(X,0,10),FVector(20,1200,40));
    for (double Y : {-600.0,600.0}) Box(Stone,FVector(0,Y,10),FVector(1800,20,40));
    for (double X : {-900.0,900.0})
        for (double Y : {-550.0,-250.0,50.0,350.0,570.0}) Box(Stone,FVector(X,Y,50),FVector(36,40,120));
    Building(FVector(-615,770,-10),FVector(390,310,330),Plaster,Timber,Slate,Glow);
    Building(FVector(-170,780,-10),FVector(390,320,455),Stone,Timber,Slate,Glow);
    Building(FVector(265,770,-10),FVector(365,310,370),Plaster,Timber,Slate,Glow);
    Building(FVector(660,750,-10),FVector(320,270,290),Stone,Timber,Slate,Glow);
    Building(FVector(-1090,200,-10),FVector(330,310,280),Plaster,Timber,Slate,Glow);
    Building(FVector(1090,220,-10),FVector(330,310,295),Plaster,Timber,Slate,Glow);
    // Both stalls occupy the existing 100 x 100 obstacles, leaving the canonical route clear.
    Stall(FVector(-400,200,-8),Timber,Wine,Iron,Glow);
    Stall(FVector(400,200,-8),Timber,Moss,Iron,Glow);
    Lantern(FVector(-830,560,155),Iron,Glow,false);
    Lantern(FVector(830,560,155),Iron,Glow,false);
    Beam(Timber,FVector(-830,560,0),FVector(-830,560,205),10);
    Beam(Timber,FVector(830,560,0),FVector(830,560,205),10);
    // Crates and barrels stay beyond the walkable boundary, so no invisible new obstacle appears.
    for (int32 I=0; I<7; ++I)
    {
        const double X=-720+I*235;
        Box(Timber,FVector(X,660,22),FVector(55,55,65));
        Box(Iron,FVector(X,660,22),FVector(58,4,68));
        Solid(TEXT("Cylinder"),Timber,FVector(X+63,652,20),FVector(.45,.45,.6));
    }
    // Sagging ropes and wine-colored cloth echo the original market illustrations.
    FVector Last(-800,575,330);
    for (int32 I=1; I<=16; ++I)
    {
        const double X=-800+I*100;
        const FVector Next(X,575,255+75*FMath::Square(X/800));
        Beam(Iron,Last,Next,2.5); Last=Next;
    }
    for (int32 I=0; I<7; ++I)
    {
        const double X=-650+I*215;
        const double Z=255+75*FMath::Square(X/800);
        auto* Cloth=(I%3==1)?Moss:Wine;
        Box(Cloth,FVector(X,575,Z-39),FVector(82,3,78),FRotator((I%2==0)?-4:4,0,0));
        Box(Timber,FVector(X,575,Z),FVector(88,6,5));
        Box(Iron,FVector(X,572,Z-75),FVector(74,2,3));
    }
    for (TActorIterator<ADirectionalLight> It(GetWorld()); It; ++It)
    {
        It->SetActorRotation(FRotator(-52,-28,0)); auto* Light=Cast<UDirectionalLightComponent>(It->GetLightComponent());
        Light->SetForwardShadingPriority(1); Light->SetIntensity(2.6f); Light->SetLightColor(FLinearColor(.62f,.75f,1.0f)); Light->SetCastShadows(true);
        Light->DynamicShadowDistanceMovableLight=5000; Light->ShadowBias=.35f;
    }
    auto* Fill=NewObject<UDirectionalLightComponent>(this); AddInstanceComponent(Fill); Fill->SetupAttachment(RootComponent);
    Fill->SetRelativeRotation(FRotator(-38,145,0)); Fill->SetIntensity(.75f); Fill->SetLightColor(FLinearColor(.42f,.5f,.65f)); Fill->SetCastShadows(false); Fill->RegisterComponent();
    auto* Fog=NewObject<UExponentialHeightFogComponent>(this); AddInstanceComponent(Fog); Fog->SetupAttachment(RootComponent);
    Fog->SetRelativeLocation(FVector(0,0,-100)); Fog->SetFogDensity(.017f); Fog->SetFogHeightFalloff(.1f);
    Fog->SetFogInscatteringColor(FLinearColor(.035f,.055f,.08f)); Fog->SetStartDistance(800); Fog->SetFogMaxOpacity(.55f); Fog->RegisterComponent();
}
void AMemoriaVerdanPresentation::BeginPlay()
{
    Super::BeginPlay();
    auto* PC=GetWorld()->GetFirstPlayerController(); Player=PC?Cast<AMemoriaFieldPawn>(PC->GetPawn()):nullptr;
    if (!Player.IsValid() || !LoadObject<UMaterialInterface>(nullptr,SurfacePath) || !LoadObject<UStaticMesh>(nullptr,RoofPath)
        || !LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Memoria/Presentation/Depth/M_Paving.M_Paving"))
        || !LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Memoria/Presentation/Depth/M_Glow.M_Glow"))
        || !LoadObject<UPaperSprite>(nullptr,TEXT("/Game/Memoria/Presentation/Depth2/SPR_MemoryLantern.SPR_MemoryLantern")))
    { UE_LOG(LogTemp,Error,TEXT("Verdan depth stage assets or pawn missing")); SetActorTickEnabled(false); return; }
#if WITH_EDITOR
    if (auto* Texture=LoadObject<UTexture2D>(nullptr,TEXT("/Game/Memoria/Presentation/Depth2/T_MemoryLantern.T_MemoryLantern"))) FTextureCompilingManager::Get().FinishCompilation({Texture});
    for (const auto& Entry : MemoriaVerdanArt::Textures())
    {
        const FString Name=FString(TEXT("T_"))+Entry.Name;
        if (auto* Texture=LoadObject<UTexture2D>(nullptr,*(MemoriaVerdanArt::Package(Name)+TEXT(".")+Name))) FTextureCompilingManager::Get().FinishCompilation({Texture});
    }
#endif
    for (const TCHAR* Facing : {TEXT("Down"),TEXT("Up"),TEXT("Left"),TEXT("Right")})
    {
        PlayerArt.Add(MemoriaVerdanArt::LoadSprite(FString(TEXT("Arrel"))+Facing));
        for (int32 Frame=0; Frame<4; ++Frame) PlayerArt.Add(MemoriaVerdanArt::LoadSprite(FString::Printf(TEXT("ArrelWalk%s%d"),Facing,Frame)));
    }
    for (TActorIterator<AStaticMeshActor> It(GetWorld()); It; ++It)
    {
        const FVector P=It->GetActorLocation();
        if (P.Equals(FVector(0,0,-30)) || P.Equals(FVector(-900,0,0)) || P.Equals(FVector(900,0,0)) || P.Equals(FVector(0,-600,0)) || P.Equals(FVector(0,600,0)) || P.Equals(FVector(-400,200,-5)) || P.Equals(FVector(400,200,-5)))
            It->GetStaticMeshComponent()->SetHiddenInGame(true);
    }
    BuildDepthEnvironment();
    auto* Camera=Player->GetFieldCamera(); Camera->ProjectionMode=ECameraProjectionMode::Perspective;
    Camera->SetFieldOfView(65); Camera->SetRelativeLocation(FVector(0,-1150,1450)); Camera->SetRelativeRotation(FRotator(-42,90,0));
    Camera->bAutoCalculateOrthoPlanes=false;
    auto* Sprite=Player->GetFieldSprite(); Sprite->SetRelativeRotation(FRotator(0,0,42)); Sprite->SetRelativeLocation(FVector(0,0,-8));
    Sprite->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Paper2D/MaskedLitSpriteMaterial.MaskedLitSpriteMaterial"))); Sprite->SetCastShadow(true);
    for (TActorIterator<AMemoriaMaletActor> It(GetWorld()); It; ++It)
    {
        Malet=*It; TArray<UStaticMeshComponent*> Meshes; It->GetComponents(Meshes);
        for (auto* Component:Meshes) Component->SetHiddenInGame(true);
        TArray<UTextRenderComponent*> Labels; It->GetComponents(Labels); for (auto* Component:Labels) Component->SetHiddenInGame(true);
        const FVector P=It->GetActorLocation(); MaletArt=Picture(TEXT("Malet"),FVector(P.X,P.Y,-8));
        SoftQuad(FVector(P.X,P.Y,-9),FVector(.72,.30,1),FLinearColor::Black,.7f); break;
    }
    PlayerShadow=SoftQuad(FVector(0,0,-9),FVector(.80,.32,1),FLinearColor::Black,.7f);
    PreviousPosition=Player->GetActorLocation(); Sprite->SetSprite(PlayerArt[0]);
    UpdateCameraAndVisibility();
}
void AMemoriaVerdanPresentation::UpdateCameraAndVisibility()
{
    const FVector P=Player->GetActorLocation();
    // Follow exactly in the interaction area, then ease toward a finite edge limit.
    // This avoids a velocity jump at a hard clamp and does not modify the pawn.
    auto Limit=[](double V,double Min,double Max,double Reach)
    {
        if (V>Max) return Max+Reach*(1-FMath::Exp(-(V-Max)/Reach));
        if (V<Min) return Min-Reach*(1-FMath::Exp(-(Min-V)/Reach));
        return V;
    };
    const FVector Anchor(Limit(P.X,-450,450,100),Limit(P.Y,-180,100,180),P.Z);
    auto* Camera=Player->GetFieldCamera();
    Camera->SetWorldLocation(Anchor+FVector(0,-1150,1450));
    auto Color=[](const FVector& V){ return FLinearColor(V.X,V.Y,V.Z,1); };
    const FLinearColor Eye=Color(Camera->GetComponentLocation());
    const FLinearColor Focus=Color(Player->GetFieldSprite()->Bounds.Origin);
    const FLinearColor Up=Color(Camera->GetUpVector());
    for (const auto& M:SurfaceMaterials)
    {
        M->SetVectorParameterValue(TEXT("OcclusionEye"),Eye);
        M->SetVectorParameterValue(TEXT("OcclusionFocus"),Focus);
        M->SetVectorParameterValue(TEXT("OcclusionUp"),Up);
    }
}
void AMemoriaVerdanPresentation::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    LightTime += DeltaSeconds;
    for (int32 I = 0; I < LampLights.Num(); ++I)
        LampLights[I]->SetIntensity(3.0f + 0.12f * FMath::Sin(LightTime * 1.7f + I));
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
    // The ground anchor remains at the physical foot; depth testing handles occlusion.
    Sprite->SetRelativeLocation(FVector(0, 0, -8));
    PlayerShadow->SetWorldLocation(FVector(Position.X, Position.Y, -9));
    UpdateCameraAndVisibility();
    PreviousPosition = Position;
}
