#include "Presentation/MemoriaVerdanPresentation.h"
#include "Combat/MemoriaFieldCombatSubsystem.h"
#include "Combat/MemoriaCombatHudWidget.h"
#include "Combat/MemoriaExplorationHudWidget.h"
#include "Presentation/MemoriaFieldCharacterComponent.h"
#include "Audio/MemoriaAudioCatalog.h"
#include "Audio/MemoriaAudioSubsystem.h"
#include "Engine/GameInstance.h"
#include "Presentation/MemoriaVerdanArt.h"
#include "Framework/MemoriaFieldPawn.h"
#include "Framework/MemoriaVerdanTuning.h"
#include "Interaction/MemoriaMaletActor.h"
#include "Engine/World.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
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
#include "Engine/Texture2D.h"
#if WITH_EDITOR
#include "TextureCompiler.h"
#include "Engine/Texture2D.h"
#include "Achievements/MemoriaAchievementSubsystem.h"
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
bool AMemoriaVerdanPresentation::IsPlaceholderGeometry(const AStaticMeshActor& Actor)
{
    // Identify by intent rather than exact coordinates, so moving a retained body in
    // the level cannot reveal it and new art placed at an old coordinate is not hidden.
    if (Actor.ActorHasTag(PlaceholderTag)) return true;
    const UStaticMesh* Mesh = Actor.GetStaticMeshComponent() ? Actor.GetStaticMeshComponent()->GetStaticMesh().Get() : nullptr;
    return Mesh && Mesh->GetPathName().StartsWith(TEXT("/Engine/BasicShapes/"));
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
        const FString Path = MeshName[0] == TEXT('/') ? FString(MeshName) : FString(MeshName) == TEXT("Roof") ? FString(RoofPath) : FString::Printf(TEXT("/Engine/BasicShapes/%s.%s"), MeshName, MeshName);
        Batch->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, *Path)); Batch->SetMaterial(0, Material);
        // Flush courtyard inlays must not create false raised-obstacle shadows.
        Batch->SetCastShadow(Material->GetName() != TEXT("M_Glow") && !Material->GetName().StartsWith(TEXT("Courtyard")));
        Batch->RegisterComponent();
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
    Light->SetLightColor(FLinearColor(1.0f,0.58f,0.28f)); Light->SetIntensity(3.6f); Light->SetAttenuationRadius(450);
    Light->SetCastShadows(bShadow); Light->SetSourceRadius(18); Light->SetSoftSourceRadius(28); Light->RegisterComponent(); LampLights.Add(Light);
    // Static reflected warmth connects the existing lamp to the paving without adding lights.
    SoftQuad(FVector(P.X,P.Y-30,-9.25),FVector(2.8,2.5,1),FLinearColor(.19f,.095f,.035f),.19f);
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
void AMemoriaVerdanPresentation::Stall(FVector P, UMaterialInterface* Timber, UMaterialInterface* Cloth, UMaterialInterface* Iron, UMaterialInterface* Glow, bool bLantern)
{
    Box(Timber,P+FVector(0,0,66),FVector(98,92,12));
    Box(Cloth,P+FVector(0,-44,30),FVector(95,4,58));
    for (double X : {-42.0,42.0}) for (double Y : {-38.0,38.0}) Box(Timber,P+FVector(X,Y,74),FVector(7,7,148));
    Solid(TEXT("Roof"),Cloth,P+FVector(0,0,145),FVector(1.4,1.28,0.75));
    // The canopy and its seams keep the original roof silhouette and obstacle footprint.
    for (double X : {-48.0,-16.0,16.0,48.0})
    {
        Beam(Timber,P+FVector(X,-63,146),P+FVector(X,0,175),1.3f);
        Box(Cloth,P+FVector(X,-63,138),FVector(29,2,14));
    }
    Beam(Iron,P+FVector(-65,0,176),P+FVector(65,0,176),1.8f);
    for (double X : {-42.0,42.0})
        Beam(Timber,P+FVector(X,-38,104),P+FVector(X,-13,145),4);
    Box(Timber,P+FVector(0,-47,64),FVector(99,5,8));
    Box(Timber,P+FVector(0,-47,4),FVector(99,5,8));
    for (double X : {-46.0,46.0}) Box(Timber,P+FVector(X,-47,34),FVector(5,5,56));
    Box(Timber,P+FVector(0,31,104),FVector(85,21,5));
    // Three glass colours shared by every stall (one material each keeps the batches few).
    static const FLinearColor GlassTints[] = {FLinearColor(.10f,.23f,.20f), FLinearColor(.26f,.12f,.07f), FLinearColor(.09f,.12f,.24f)};
    const int32 Shade=FMath::Abs(int32(P.X/7+P.Y/3))%3;
    const FName GlassName(*FString::Printf(TEXT("Bottles%d"),Shade));
    UMaterialInstanceDynamic* Glass=nullptr;
    for (const auto& M:SurfaceMaterials) if (M && M->GetFName()==GlassName) Glass=M;
    if (!Glass) Glass=Surface(GlassName,GlassTints[Shade],3,.28f,.32f);
    for (int32 I=0; I<8; ++I)
    {
        const bool bShelf=I>=5;
        const double Height=I%3==0?23:18;
        const FVector V=P+(bShelf?FVector(-27+(I-5)*24,30,107+Height*.5):FVector(-32+I*15,-8,72+Height*.5));
        Solid(TEXT("Sphere"),Glass,V,FVector(.105,.105,Height/100));
        Solid(TEXT("Cylinder"),Iron,V+FVector(0,0,Height*.5+1),FVector(.033,.033,.055));
    }
    // Bound ledgers, a shallow tray and stored parcels make both existing counters legible.
    Box(Cloth,P+FVector(24,14,76),FVector(22,23,5));
    Box(Timber,P+FVector(20,14,81),FVector(24,23,4),FRotator(0,12,0));
    Box(Iron,P+FVector(-25,-29,75),FVector(27,18,2));
    for (double X : {-37.0,-13.0}) Box(Timber,P+FVector(X,-29,78),FVector(2,18,6));
    for (double Y : {-37.0,-21.0}) Box(Timber,P+FVector(-25,Y,78),FVector(27,2,6));
    Box(Cloth,P+FVector(-22,18,24),FVector(27,27,37));
    Box(Timber,P+FVector(18,20,18),FVector(39,32,25));
    for (double X : {4.0,32.0}) Box(Iron,P+FVector(X,20,31),FVector(3,33,2));
    if (bLantern) Lantern(P+FVector(-35,-27,119),Iron,Glow,true);
}
int32 AMemoriaVerdanPresentation::GetRiggedTownsfolkCount() const
{
    int32 Count = 0;
    for (const auto& Figure : Townsfolk) Count += Figure && Figure->IsRigged() ? 1 : 0;
    return Count;
}
void AMemoriaVerdanPresentation::MarketLight(const FVector& P, float Intensity, float Radius)
{
    // A stall's or a lantern's warm light: no shadow, so a ring of them stays cheap.
    auto* Light = NewObject<UPointLightComponent>(this, FName(*FString::Printf(TEXT("MarketLight%d"), MarketLights.Num())));
    AddInstanceComponent(Light); Light->SetupAttachment(RootComponent); Light->SetRelativeLocation(P);
    Light->bUseInverseSquaredFalloff = false; Light->LightFalloffExponent = 2.2f;
    Light->SetLightColor(FLinearColor(1.f,.6f,.3f)); Light->SetIntensity(Intensity); Light->SetAttenuationRadius(Radius);
    Light->SetCastShadows(false); Light->SetSourceRadius(12); Light->RegisterComponent();
    MarketLights.Add(Light); MarketBase.Add(Intensity);
}
void AMemoriaVerdanPresentation::BuildMarketRing(UMaterialInterface* Timber, UMaterialInterface* Iron, UMaterialInterface* Glow)
{
    // S346: verdan_market.gd sets its stalls round the square (STALL tiles, each with a warm PointLight2D), and the
    // market canvas paints a ring of lantern-lit stalls under wine, slate and moss cloth. The port's square keeps its
    // two story stalls; ten more stand along its edges, clear of the story's places (the memory stalls, the old
    // man at the west edge, Malet's table, Elia and the sump stairs) and of the edge spots the tests stand Arrel
    // on. The side ones stand against the curb, leaving no alley behind them. Like the two, they are visual only:
    // the presentation owns no collision.
    auto* Wine=Surface(TEXT("RingWine"),FLinearColor(.22f,.055f,.06f),2);
    auto* Moss=Surface(TEXT("RingMoss"),FLinearColor(.065f,.15f,.14f),2);
    auto* SlateCloth=Surface(TEXT("RingSlate"),FLinearColor(.07f,.075f,.09f),2);
    UMaterialInterface* Cloths[] = {Wine, SlateCloth, Moss};
    const FVector Stalls[] = {
        FVector(-690,505,-8), FVector(-400,505,-8), FVector(400,505,-8), FVector(690,505,-8),
        FVector(-840,330,-8), FVector(-840,-170,-8), FVector(-840,-430,-8),
        FVector(840,330,-8), FVector(840,160,-8), FVector(840,-160,-8)};
    for (int32 I = 0; I < UE_ARRAY_COUNT(Stalls); ++I)
    {
        Stall(Stalls[I], Timber, Cloths[I % 3], Iron, Glow, false);
        // The lamp under the canopy: a glowing pane and its light on the counter.
        Box(Glow, Stalls[I] + FVector(-30, -30, 110), FVector(11, 11, 15));
        MarketLight(Stalls[I] + FVector(-30, -70, 118), 5.f, 420.f);
        // The lamp's warmth on the paving in front of the counter, as the story stalls' lanterns have.
        SoftQuad(FVector(Stalls[I].X - 10, Stalls[I].Y - 95, -9.25), FVector(2.4, 1.8, 1), FLinearColor(.19f, .095f, .035f), .2f);
        ++MarketStalls;
    }
    // Codex's S343 kit, where it is imported: lantern posts on the curb by the pillars, and freight between the
    // north stalls. Their material is the kit's, with the same opening round Arrel as the walls.
    auto* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Memoria/Presentation/Environment/M_EnvProp.M_EnvProp"), nullptr, LOAD_NoWarn | LOAD_Quiet);
    auto Atlas = [](const TCHAR* Name) { return LoadObject<UTexture2D>(nullptr, *FString::Printf(TEXT("/Game/Memoria/Presentation/Environment/%s.%s"), Name, Name), nullptr, LOAD_NoWarn | LOAD_Quiet); };
    UTexture2D* Small = Atlas(TEXT("T_EnvSmall")); UTexture2D* SmallGlow = Atlas(TEXT("T_EnvSmallGlow")); UTexture2D* Belt = Atlas(TEXT("T_EnvBelt"));
    if (!Base || !Small || !Belt) { UE_LOG(LogTemp, Warning, TEXT("Verdan market: environment kit not imported (-run=MemoriaEnvironmentAssets)")); return; }
    auto Kit = [&](const TCHAR* Name, UTexture2D* Color, UTexture2D* Mask, const FLinearColor& Tint)
    {
        auto* M = UMaterialInstanceDynamic::Create(Base, this, Name);
        M->SetTextureParameterValue(TEXT("Color"), Color); M->SetVectorParameterValue(TEXT("Tint"), Tint);
        if (Mask) { M->SetTextureParameterValue(TEXT("Glow"), Mask); M->SetVectorParameterValue(TEXT("GlowColor"), FLinearColor(1.f, .58f, .28f, 14.f)); }
        SurfaceMaterials.Add(M); return M;
    };
    auto* Post = Kit(TEXT("MarketLanternPost"), Small, SmallGlow, FLinearColor(.95f, .97f, 1.05f));
    auto* Freight = Kit(TEXT("MarketFreight"), Belt, nullptr, FLinearColor(1.05f, 1.08f, 1.18f));
    for (double X : {-900.0, 900.0})
        for (double Y : {-250.0, 350.0})
        {
            // On the curb (its top at 30) beside the pillar; the lantern hangs 17 cm towards the square.
            const float Yaw = X < 0 ? 0.f : 180.f;
            Solid(TEXT("/Game/Memoria/Presentation/Environment/SM_EnvLanternPost.SM_EnvLanternPost"), Post, FVector(X, Y + 46, 30), FVector::OneVector, FRotator(0, Yaw, 0));
            MarketLight(FVector(X, Y + 46, 30) + FRotator(0, Yaw, 0).RotateVector(FVector(17, -6, 150)), 3.f, 430.f);
            ++MarketProps;
        }
    Solid(TEXT("/Game/Memoria/Presentation/Environment/SM_EnvCrateStack.SM_EnvCrateStack"), Freight, FVector(-210, 525, -8), FVector(.7), FRotator(0, 8, 0));
    Solid(TEXT("/Game/Memoria/Presentation/Environment/SM_EnvCrateStack.SM_EnvCrateStack"), Freight, FVector(210, 528, -8), FVector(.66), FRotator(0, -6, 0));
    MarketProps += 2;
    // Lanterns hung from the north rope, where the cloth ends.
    for (double X : {-540.0, 0.0, 540.0})
    {
        const double Z = 255 + 75 * FMath::Square(X / 800) - 52;
        Beam(Iron, FVector(X, 575, Z + 50), FVector(X, 575, Z + 10), 1.5f);
        Box(Glow, FVector(X, 572, Z), FVector(10, 10, 16));
        MarketLight(FVector(X, 560, Z - 6), 2.4f, 360.f);
    }
}
void AMemoriaVerdanPresentation::BuildCourtyard(UMaterialInterface* Iron)
{
    // All details are flush with the retained floor (visual top below the -8 foot anchor).
    // A worn central passage and short cross-course break up the repeating atlas at play scale.
    // Reuse the original market paving texture; plain lit colors read as new concrete plates.
    auto PavingSurface=[this](const TCHAR* Name,FLinearColor Tint,float Roughness)
    {
        auto* Material=UMaterialInstanceDynamic::Create(LoadObject<UMaterialInterface>(nullptr,
            TEXT("/Game/Memoria/Presentation/Depth/M_Paving.M_Paving")),this,Name);
        Material->SetVectorParameterValue(TEXT("Tint"),Tint);
        Material->SetScalarParameterValue(TEXT("Roughness"),Roughness);
        return Material;
    };
    UMaterialInterface* Slabs[] = {
        PavingSurface(TEXT("CourtyardWorn0"),FLinearColor(1.43f,1.44f,1.42f),.86f),
        PavingSurface(TEXT("CourtyardWorn1"),FLinearColor(1.29f,1.35f,1.40f),.91f),
        PavingSurface(TEXT("CourtyardWorn2"),FLinearColor(1.36f,1.39f,1.40f),.88f)};
    for (int32 Row=0; Row<17; ++Row)
        for (int32 Column=0; Column<4; ++Column)
        {
            const int32 Pattern=(Row*7+Column*3)%11;
            // Broken outer courses blend into the older cobbles instead of outlining a cross.
            const bool bOuter=Column==0 || Column==3;
            if(bOuter && Pattern%4==0)continue;
            const double X=-72+Column*48+((Row%2)?4:-4);
            const double Y=-539+Row*64;
            Box(Slabs[bOuter?1:Pattern%3],FVector(X,Y,-9.55),FVector(44+(Pattern%3),59-(Pattern%4),.5),FRotator(0,(Pattern-5)*.22,0));
        }
    for (int32 Column=-9; Column<=9; ++Column)
    {
        if (FMath::Abs(Column)<2) continue;
        for (int32 Row=0; Row<2; ++Row)
        {
            const int32 Pattern=FMath::Abs(Column*5+Row*3);
            if(FMath::Abs(Column)>6 && (Pattern%3==0 || Row==1))continue;
            Box(Slabs[(Row==1 || FMath::Abs(Column)>6)?1:Pattern%3],FVector(Column*48,-163+Row*48,-9.55),FVector(43+(Pattern%3),44,.5),FRotator(0,(Pattern%5-2)*.3,0));
        }
    }
    auto* Edge=PavingSurface(TEXT("CourtyardDrainStone"),FLinearColor(1.04f,1.13f,1.18f),.93f);
    for (double X : {-760.0,760.0})
    {
        Box(Edge,FVector(X,0,-9.6),FVector(29,1120,.4));
        for (int32 Row=0; Row<15; ++Row)
        {
            const double Y=-520+Row*74;
            for (double Side : {-1.0,1.0})
                Box(Slabs[1],FVector(X+Side*24,Y,-9.55),FVector(16,69,.5));
            // A few recessed grates read as drainage, never as raised collision obstacles.
            if (Row%4==1)
                for (int32 Bar=0; Bar<4; ++Bar)
                    Box(Iron,FVector(X,Y-12+Bar*8,-9.3),FVector(26,2,.25));
        }
    }
    // Accumulated damp and dirt stay at architectural edges; the interaction area stays clear.
    for (double X : {-680.0,-220.0,230.0,670.0})
        SoftQuad(FVector(X,520,-9.2),FVector(4.4,1.5,1),FLinearColor(.022f,.03f,.029f),.38f);
    for (double X : {-400.0,400.0})
        SoftQuad(FVector(X,210,-9.15),FVector(2.1,2.0,1),FLinearColor(.035f,.025f,.018f),.25f);
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
    auto* Paving=UMaterialInstanceDynamic::Create(LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Memoria/Presentation/Depth/M_Paving.M_Paving")),this,TEXT("CourtyardPaving"));
    Paving->SetScalarParameterValue(TEXT("Roughness"),.86f);
    auto* OuterGround=Surface(TEXT("OuterGround"),FLinearColor(.028f,.038f,.049f),3,1.0f);
    Box(OuterGround,FVector(0,0,-58),FVector(5200,4200,90));
    Solid(TEXT("Plane"),Paving,FVector(0,0,-10),FVector(18,12,1));
    BuildCourtyard(Iron);
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
    BuildMarketRing(Timber,Iron,Glow);
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
        Light->SetForwardShadingPriority(1); Light->SetIntensity(2.0f); Light->SetLightColor(FLinearColor(.72f,.79f,.91f)); Light->SetCastShadows(true);
        Light->SetLightSourceAngle(3.0f); Light->SetShadowAmount(.78f);
        Light->DynamicShadowDistanceMovableLight=5000; Light->ShadowBias=.35f;
    }
    auto* Fill=NewObject<UDirectionalLightComponent>(this); AddInstanceComponent(Fill); Fill->SetupAttachment(RootComponent);
    Fill->SetRelativeRotation(FRotator(-38,145,0)); Fill->SetIntensity(1.15f); Fill->SetLightColor(FLinearColor(.49f,.58f,.69f)); Fill->SetCastShadows(false); Fill->RegisterComponent();
    auto* Fog=NewObject<UExponentialHeightFogComponent>(this); AddInstanceComponent(Fog); Fog->SetupAttachment(RootComponent);
    Fog->SetRelativeLocation(FVector(0,0,-100)); Fog->SetFogDensity(.017f); Fog->SetFogHeightFalloff(.1f);
    Fog->SetFogInscatteringColor(FLinearColor(.035f,.055f,.08f)); Fog->SetStartDistance(800); Fog->SetFogMaxOpacity(.55f); Fog->RegisterComponent();
}
namespace
{
constexpr int32 AshCount=56,EmberCount=18,MistCount=5;
// Stable per-index scatter so captures are reproducible frame to frame.
double Scatter(int32 I,double Salt){return FMath::Frac(FMath::Sin(I*12.9898+Salt*78.233)*43758.5453);}
}
void AMemoriaVerdanPresentation::MoteGroup(TArray<TObjectPtr<UStaticMeshComponent>>& Out, int32 Count, FLinearColor Tint, float Alpha)
{
    auto* Mesh=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Plane.Plane"));
    auto* Instance=UMaterialInstanceDynamic::Create(LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Memoria/Presentation/Verdan/M_SoftLight.M_SoftLight")),this);
    if(Instance){Instance->SetVectorParameterValue(TEXT("Tint"),Tint);Instance->SetScalarParameterValue(TEXT("Alpha"),Alpha);}
    for(int32 I=0;I<Count;++I)
    {
        auto* Mote=NewObject<UStaticMeshComponent>(this);
        AddInstanceComponent(Mote);Mote->SetupAttachment(RootComponent);Mote->SetMobility(EComponentMobility::Movable);
        Mote->SetCollisionEnabled(ECollisionEnabled::NoCollision);Mote->SetGenerateOverlapEvents(false);Mote->SetCanEverAffectNavigation(false);Mote->SetCastShadow(false);
        Mote->SetStaticMesh(Mesh);if(Instance)Mote->SetMaterial(0,Instance);Mote->RegisterComponent();Out.Add(Mote);
    }
}
void AMemoriaVerdanPresentation::BuildFieldLife()
{
    // Pale ash drifts through the whole courtyard; embers rise only from the two hanging lanterns.
    MoteGroup(AshMotes,AshCount,FLinearColor(.70f,.74f,.80f),.75f);
    MoteGroup(EmberMotes,EmberCount,FLinearColor(1.f,.52f,.20f),.95f);
    for(int32 I=0;I<MistCount;++I)
    {
        auto* Patch=SoftQuad(FVector(-700+I*350,-380+Scatter(I,3)*700,-8.6),FVector(7.5,3.4,1),FLinearColor(.15f,.19f,.25f),.16f);
        MistPatches.Add(Patch);MistMaterials.Add(Cast<UMaterialInstanceDynamic>(Patch->GetMaterial(0)));
    }
    TickFieldLife();
}
void AMemoriaVerdanPresentation::TickFieldLife()
{
    const double T=LightTime;
    for(int32 I=0;I<AshCount;++I)
    {
        const double Fall=FMath::Fmod(T*(14+10*Scatter(I,1))+Scatter(I,2)*360,360.0);
        const double X=FMath::Fmod(-950+Scatter(I,4)*1900+T*9+950,1900.0)-950+26*FMath::Sin(T*.4+I);
        const FVector P(X,-620+Scatter(I,5)*1240,340-Fall);
        AshMotes[I]->SetRelativeTransform(FTransform(FRotator(0,T*20+I*37,0),P,FVector(.09+.05*Scatter(I,6))));
    }
    for(int32 I=0;I<EmberCount;++I)
    {
        const double Life=FMath::Frac(T*(.22+.12*Scatter(I,7))+Scatter(I,8));
        const double Side=I%2?830:-830;
        const FVector P(Side+22*FMath::Sin(T*1.3+I*2.1)+14*Scatter(I,9),552+10*Scatter(I,10),150+Life*170);
        EmberMotes[I]->SetRelativeTransform(FTransform(FRotator::ZeroRotator,P,FVector(.075*(1-Life)+.01)));
    }
    for(int32 I=0;I<MistPatches.Num();++I)
    {
        MistPatches[I]->SetRelativeLocation(FVector(-700+I*350+60*FMath::Sin(T*.07+I*1.7),-380+Scatter(I,3)*700,-8.6));
        if(MistMaterials.IsValidIndex(I)&&MistMaterials[I])MistMaterials[I]->SetScalarParameterValue(TEXT("Alpha"),.15f+.06f*FMath::Sin(T*.23+I));
    }
}
void AMemoriaVerdanPresentation::BeginPlay()
{
    Super::BeginPlay();
    // verdan_market.gd record_map_visit.
    if (auto* Achievements = GetGameInstance()->GetSubsystem<UMemoriaAchievementSubsystem>()) Achievements->RecordMapVisit(TEXT("verdan_market"));
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
    // Only the visuals are hidden; the retained bodies keep their collision.
    for (TActorIterator<AStaticMeshActor> It(GetWorld()); It; ++It)
        if (IsPlaceholderGeometry(**It)) It->GetStaticMeshComponent()->SetHiddenInGame(true);
    BuildDepthEnvironment();
    auto* Camera=Player->GetFieldCamera(); Camera->ProjectionMode=ECameraProjectionMode::Perspective;
    Camera->SetFieldOfView(MemoriaVerdanTuning::CameraFOV); Camera->SetRelativeLocation(MemoriaVerdanTuning::CameraOffset); Camera->SetRelativeRotation(FRotator(MemoriaVerdanTuning::CameraPitch,90,0));
    Camera->bAutoCalculateOrthoPlanes=false;
    auto* Sprite=Player->GetFieldSprite(); Sprite->SetRelativeRotation(FRotator(0,0,42)); Sprite->SetRelativeLocation(FVector(0,0,-8));
    Sprite->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Paper2D/MaskedLitSpriteMaterial.MaskedLitSpriteMaterial"))); Sprite->SetCastShadow(true);
    for (TActorIterator<AMemoriaMaletActor> It(GetWorld()); It; ++It)
    {
        Malet=*It; TArray<UStaticMeshComponent*> Meshes; It->GetComponents(Meshes);
        for (auto* Component:Meshes) Component->SetHiddenInGame(true);
        TArray<UTextRenderComponent*> Labels; It->GetComponents(Labels); for (auto* Component:Labels) Component->SetHiddenInGame(true);
        const FVector P=It->GetActorLocation();
        MaletCard=NewObject<UMemoriaFieldCharacterComponent>(this);AddInstanceComponent(MaletCard);MaletCard->SetupAttachment(GetRootComponent());
        MaletCard->RegisterComponent();MaletCard->SetWorldLocation(FVector(P.X,P.Y,0));MaletCard->InitializeCharacter(TEXT("malet"),ArrelHeight*1.0f);
        SoftQuad(FVector(P.X,P.Y,-9),FVector(.72,.30,1),FLinearColor::Black,.7f); break;
    }
    PlayerShadow=SoftQuad(FVector(0,0,-9),FVector(.80,.32,1),FLinearColor::Black,.7f);
    // S306: Arrel is drawn like every other field character (the user rejected the 3D prototype beside the sprites).
    ArrelFigure=NewObject<UMemoriaFieldCharacterComponent>(this);
    AddInstanceComponent(ArrelFigure);ArrelFigure->SetupAttachment(Player->GetRootComponent());ArrelFigure->RegisterComponent();
    if(!ArrelFigure->InitializeCharacter(TEXT("arrel"),ArrelHeight))
    { UE_LOG(LogTemp,Error,TEXT("Arrel field art missing"));SetActorTickEnabled(false);return; }
    Sprite->SetHiddenInGame(true);Sprite->SetCastShadow(false);
    // A soft character-only fill preserves the courtyard's existing night lighting. Every figure gets the same one,
    // so painted art reads alike wherever it stands; 3 keeps silver armour and white robes off the clip.
    auto AddFill=[this](USceneComponent* Parent,const TCHAR* Name)
    {
        auto* Fill=NewObject<UPointLightComponent>(this,Name);AddInstanceComponent(Fill);
        Fill->SetupAttachment(Parent);Fill->SetRelativeLocation(FVector(40,-140,160));
        Fill->bUseInverseSquaredFalloff=false;Fill->LightFalloffExponent=2;
        Fill->SetIntensity(3.f);Fill->SetAttenuationRadius(500);Fill->SetLightColor(FLinearColor(.75f,.83f,1.f));
        Fill->SetLightingChannels(false,true,false);Fill->SetCastShadows(false);Fill->RegisterComponent();
    };
    AddFill(Player->GetRootComponent(),TEXT("ArrelFillLight"));
    if(MaletCard) AddFill(MaletCard,TEXT("MaletFillLight"));
    // S348: verdan_market.gd's S55 market townsfolk, on Codex's S347 models (the pixel presets have no card art,
    // so before them none stood). The source tiles (7,6), (12,5), (16,7), (4,9), (19,6) are read on the port's
    // smaller square and moved off its stalls and story places: the woman shops at the west story stall, the
    // elder keeps the west edge where the old man's talk is, the child stands by the east stall. They idle where
    // they stand, with the same character fill as Arrel and Malet; no collision, as with every figure here.
    struct FTownsperson { const TCHAR* Id; FVector At; const TCHAR* Facing; };
    const FTownsperson People[] = {
        {TEXT("villagerf"), FVector(-470,105,0), TEXT("Up")}, {TEXT("fisherman"), FVector(-160,300,0), TEXT("Down")},
        {TEXT("villagerm"), FVector(110,140,0), TEXT("Left")}, {TEXT("elder"), FVector(-640,30,0), TEXT("Right")},
        {TEXT("child"), FVector(290,215,0), TEXT("Down")}};
    for (const FTownsperson& Person : People)
    {
        auto* Figure=NewObject<UMemoriaFieldCharacterComponent>(this);AddInstanceComponent(Figure);Figure->SetupAttachment(GetRootComponent());
        Figure->RegisterComponent();Figure->SetWorldLocation(Person.At);
        if(!Figure->InitializeCharacter(Person.Id,UMemoriaFieldCharacterComponent::AmbientHeight(Person.Id,ArrelHeight))){Figure->DestroyComponent();continue;}
        Figure->Face(Person.Facing);
        SoftQuad(FVector(Person.At.X,Person.At.Y,-9),FVector(.6,.26,1),FLinearColor::Black,.6f);
        AddFill(Figure,*FString::Printf(TEXT("Town%sFillLight"),Person.Id));
        Townsfolk.Add(Figure);
    }
    // S311: Arrel fights in the field; the combat HUD paints only while a fight or a wound shows.
    if (auto* Combat = GetWorld()->GetSubsystem<UMemoriaFieldCombatSubsystem>())
    {
        Combat->RegisterPlayer(Player.Get(), ArrelFigure);
        if (APlayerController* HudOwner = GetWorld()->GetFirstPlayerController())
        {
            CombatHud = CreateWidget<UMemoriaCombatHudWidget>(HudOwner, UMemoriaCombatHudWidget::StaticClass());
            CombatHud->Bind(Combat); CombatHud->SetVisibility(ESlateVisibility::HitTestInvisible); CombatHud->AddToViewport(4);
            // S316: the source exploration panel (HP, chapter, memories, grains, items) at the top right.
            ExplorationHud = CreateWidget<UMemoriaExplorationHudWidget>(HudOwner, UMemoriaExplorationHudWidget::StaticClass());
            ExplorationHud->SetVisibility(ESlateVisibility::HitTestInvisible); ExplorationHud->AddToViewport(3);
        }
    }
    BuildFieldLife();
    PreviousPosition=Player->GetActorLocation();
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
    // S315: the combat shake rides on the follow position.
    const FVector Shake=GetWorld()->GetSubsystem<UMemoriaFieldCombatSubsystem>()?GetWorld()->GetSubsystem<UMemoriaFieldCombatSubsystem>()->GetShakeOffset():FVector::ZeroVector;
    Camera->SetWorldLocation(Anchor+MemoriaVerdanTuning::CameraOffset+Shake);
    auto Color=[](const FVector& V){ return FLinearColor(V.X,V.Y,V.Z,1); };
    const FLinearColor Eye=Color(Camera->GetComponentLocation());
    const FLinearColor Focus=Color(ArrelFigure->FocusPosition());
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
        LampLights[I]->SetIntensity(3.6f + 0.12f * FMath::Sin(LightTime * 1.7f + I));
    for (int32 I = 0; I < MarketLights.Num(); ++I)
        MarketLights[I]->SetIntensity(MarketBase[I] * (1.f + .05f * FMath::Sin(LightTime * 2.1f + I * 1.7f)));
    if (AshMotes.Num()==AshCount && EmberMotes.Num()==EmberCount) TickFieldLife();
    if (!Player.IsValid()) return;
    const FVector Position = Player->GetActorLocation();
    const FVector Step = Position - PreviousPosition;
    bWalking = Step.SizeSquared2D() > 0.0001 && DeltaSeconds > 0;
    if (bWalking)
    {
        Direction = FMath::Abs(Step.X) >= FMath::Abs(Step.Y) ? (Step.X > 0 ? TEXT("Right") : TEXT("Left")) : (Step.Y > 0 ? TEXT("Up") : TEXT("Down"));
    }
    const float PhaseBefore = ArrelFigure->GaitPhase();
    ArrelFigure->AdvanceLocomotion(Step,DeltaSeconds);
    // Source player.gd plays play_step on Verdan's stone paving; each footfall follows the stride.
    if (ArrelFigure->LocomotionWeight() > .5f && MemoriaAudio::CrossedFootContact(PhaseBefore, ArrelFigure->GaitPhase()))
        if (auto* Audio = GetGameInstance() ? GetGameInstance()->GetSubsystem<UMemoriaAudioSubsystem>() : nullptr) Audio->PlaySfx(TEXT("step_stone"));
    PlayerShadow->SetWorldLocation(FVector(Position.X, Position.Y, -9));
    UpdateCameraAndVisibility();
    PreviousPosition = Position;
}
void AMemoriaVerdanPresentation::EndPlay(const EEndPlayReason::Type Reason)
{
    if (CombatHud) { CombatHud->RemoveFromParent(); CombatHud = nullptr; }
    if (ExplorationHud) { ExplorationHud->RemoveFromParent(); ExplorationHud = nullptr; }
    Super::EndPlay(Reason);
}
