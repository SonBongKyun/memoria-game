#include "Presentation/MemoriaRegionalAtmosphereS363.h"
#include "Chapter/MemoriaChapterMap.h"
#include "GameFramework/Actor.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace
{
constexpr float Feet = -8.f;
constexpr int32 MaxSolidInstances = 280;
constexpr const TCHAR* FeatureTag = TEXT("MemoriaRegionalS363");
constexpr const TCHAR* ArchitectureMesh = TEXT("/Game/Memoria/Presentation/VisualPolish/SM_BeveledBlock.SM_BeveledBlock");

// Integer-only scatter is local to these pieces; it never consumes either of the
// gameplay or NPC random streams, and is stable in editor and cooked builds.
uint32 Pattern(int32 X, int32 Y, int32 Side)
{
    uint32 V = uint32(X + 31) * 73856093u ^ uint32(Y + 47) * 19349663u ^ uint32(Side + 11) * 83492791u;
    V ^= V >> 13; V *= 1274126177u; return V ^ (V >> 16);
}
bool HasFeature(const AActor& Owner)
{
    TArray<UStaticMeshComponent*> Meshes; Owner.GetComponents(Meshes);
    for (const auto* Mesh : Meshes) if (Mesh->ComponentHasTag(FeatureTag)) return true;
    return false;
}
UMaterialInstanceDynamic* Sibling(AActor& Owner, const FString& Name, UMaterialInstanceDynamic* Source,
    float Brightness, const FLinearColor& Shift, float Roughness)
{
    if (!Source || !Source->Parent || Source->Parent->IsA<UMaterialInstanceDynamic>()) return nullptr;
    auto* M = UMaterialInstanceDynamic::Create(Source->Parent.Get(), &Owner, FName(Name));
    if (!M) return nullptr;
    M->CopyInterpParameters(Source);
    // Both the architecture atlas and paving retain their existing graph/UVs.
    const bool bAtlas = Source->Parent->GetPathName().Contains(TEXT("M_ArchitectureSurface"));
    const FName TintName = bAtlas ? TEXT("ArchitectureTint") : TEXT("Tint");
    FLinearColor Tint = FLinearColor::White;
    Source->GetVectorParameterValue(FMaterialParameterInfo(TintName), Tint);
    M->SetVectorParameterValue(TintName, Tint * Brightness * Shift);
    M->SetScalarParameterValue(TEXT("Roughness"), Roughness);
    return M;
}
struct FBuilder
{
    AActor& Owner;
    MemoriaRegionalAtmosphereS363::FResult Result;
    TMap<FString, UInstancedStaticMeshComponent*> Batches;
    explicit FBuilder(AActor& InOwner) : Owner(InOwner) {}
    void Keep(UMaterialInstanceDynamic* M) { if (M) Result.Materials.Add(M); }
    void Box(const TCHAR* Family, UMaterialInstanceDynamic* Material, const FVector& At, const FVector& Size, float Yaw = 0.f, bool bShadow = true)
    {
        if (!Material || Result.SolidInstances >= MaxSolidInstances) return;
        auto*& Batch = Batches.FindOrAdd(Family);
        if (!Batch)
        {
            auto* Mesh = LoadObject<UStaticMesh>(nullptr, ArchitectureMesh, nullptr, LOAD_NoWarn | LOAD_Quiet);
            if (!Mesh) return;
            // UObjects of different classes still share their owner's name
            // namespace. Keep this component distinct from its sibling MID.
            Batch = NewObject<UInstancedStaticMeshComponent>(&Owner, FName(*(FString(Family)+TEXT("_Instances"))));
            Owner.AddInstanceComponent(Batch); Batch->SetupAttachment(Owner.GetRootComponent());
            Batch->SetMobility(EComponentMobility::Movable);
            Batch->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Batch->SetGenerateOverlapEvents(false); Batch->SetCanEverAffectNavigation(false);
            Batch->SetStaticMesh(Mesh); Batch->SetMaterial(0, Material); Batch->SetCastShadow(bShadow);
            Batch->ComponentTags.Add(FeatureTag); Batch->ComponentTags.Add(TEXT("S363Solid")); Batch->RegisterComponent();
        }
        Batch->AddInstance(FTransform(FRotator(0,Yaw,0), At, Size / 100.f), true);
        ++Result.SolidInstances;
    }
    void Soft(const FString& Name, UMaterialInstanceDynamic* Material, const FVector& At, const FVector2D& Size, float Yaw = 0.f)
    {
        if (!Material) return;
        auto* Mesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
        if (!Mesh) return;
        // The retained soft-light parent has no ISM usage. Ordinary planes keep
        // its actual shader, as the existing regional mist already does.
        auto* Quad = NewObject<UStaticMeshComponent>(&Owner, FName(Name));
        Owner.AddInstanceComponent(Quad); Quad->SetupAttachment(Owner.GetRootComponent());
        Quad->SetMobility(EComponentMobility::Movable);
        Quad->SetCollisionEnabled(ECollisionEnabled::NoCollision); Quad->SetGenerateOverlapEvents(false);
        Quad->SetCanEverAffectNavigation(false); Quad->SetCastShadow(false);
        Quad->SetStaticMesh(Mesh); Quad->SetMaterial(0, Material);
        Quad->SetWorldTransform(FTransform(FRotator(0,Yaw,0), At, FVector(Size.X/100.f,Size.Y/100.f,1)));
        Quad->ComponentTags.Add(FeatureTag); Quad->ComponentTags.Add(TEXT("S363Soft")); Quad->RegisterComponent();
        ++Result.SoftPlanes;
    }
    UMaterialInstanceDynamic* SoftMaterial(const FString& Name, const FLinearColor& Tint, float Alpha)
    {
        auto* Parent = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Memoria/Presentation/Verdan/M_SoftLight.M_SoftLight"));
        if (!Parent) return nullptr;
        auto* M = UMaterialInstanceDynamic::Create(Parent, &Owner, FName(Name));
        M->SetVectorParameterValue(TEXT("Tint"), Tint); M->SetScalarParameterValue(TEXT("Alpha"), Alpha);
        Keep(M); return M;
    }
};
}

bool MemoriaRegionalAtmosphereS363::IsEnabled()
{
    // Reproducible same-camera diagnostic A/B. No game UI or saved preference.
    return !FParse::Param(FCommandLine::Get(), TEXT("MemoriaGraphicsS363Baseline"));
}
MemoriaRegionalAtmosphereS363::FResult MemoriaRegionalAtmosphereS363::BuildVerdan(AActor& Owner,
    UMaterialInstanceDynamic* Stone, UMaterialInstanceDynamic* Paving)
{
    if (!IsEnabled() || HasFeature(Owner) || !Stone || !Paving) return {};
    FBuilder B(Owner);
    auto* Edge = Sibling(Owner,TEXT("S363VerdanFooting"),Stone,1.10f,FLinearColor(1.05f,1.02f,.94f),.82f);
    auto* Apron = Sibling(Owner,TEXT("S363VerdanApron"),Paving,1.10f,FLinearColor(1.01f,1.02f,1.02f),.64f);
    auto* Chip = Sibling(Owner,TEXT("S363VerdanChips"),Stone,.88f,FLinearColor(1.02f,1.f,.94f),.96f);
    B.Keep(Edge); B.Keep(Apron); B.Keep(Chip);
    auto* Bounce = B.SoftMaterial(TEXT("S363VerdanBounce"),FLinearColor(.32f,.145f,.04f),.055f);
    auto* Contact = B.SoftMaterial(TEXT("S363VerdanContact"),FLinearColor(.012f,.017f,.02f),.17f);
    for (int32 Side=0;Side<2;++Side)
    {
        const double X = Side==0 ? -400. : 400.;
        // The footing stays inside each retained 100x100 counter obstacle;
        // aprons stay below the foot plane and loose chips stay under2cm high.
        // The original route/collision never moves.
        for (int32 Row=0;Row<3;++Row)
            for (int32 Col=0;Col<4;++Col)
            {
                const uint32 P=Pattern(Col,Row,Side);
                B.Box(TEXT("S363VerdanAprons"),Apron,FVector(X-48+Col*32,135+Row*30,-9.32),FVector(29,27,.5),float(int32(P%5)-2)*.4f,false);
            }
        for (int32 Col=0;Col<4;++Col)
        {
            B.Box(TEXT("S363VerdanStoneTrim"),Edge,FVector(X-36+Col*24,157,Feet+5),FVector(22,8,10),float(Col%2)*.5f);
            B.Box(TEXT("S363VerdanChips"),Chip,FVector(X-50+Col*31,121+(Col%2)*3,-8.1),FVector(14+Col*2,9,1.4),float(Col*19-26),false);
        }
        B.Soft(FString::Printf(TEXT("S363VerdanCounterContact%d"),Side),Contact,FVector(X,190,-8.95),FVector2D(178,118));
        B.Soft(FString::Printf(TEXT("S363VerdanCounterBounce%d"),Side),Bounce,FVector(X-25,104,-8.86),FVector2D(155,104),Side==0 ? -16.f : 16.f);
    }
    // Sparse coping sits on the retained north curb's top (z30), wholly within
    // its original x[-900,900]/y[590,610] blocking footprint. It adds no low
    // obstacle in the walkable strip in front of that curb.
    for (int32 I=0;I<18;++I)
    {
        if (I==8 || I==9) continue;
        const double X=-714+I*84;
        B.Box(TEXT("S363VerdanStoneTrim"),Edge,FVector(X,600,31.5),FVector(76,12,3),0.f);
        if (I%3==1) B.Box(TEXT("S363VerdanChips"),Chip,FVector(X+14,562,-8.1),FVector(19,12,1.4),float(I*11),false);
    }
    UE_LOG(LogTemp,Display,TEXT("S363_REGION_BUILD map=verdan_market solids=%d soft=%d"),B.Result.SolidInstances,B.Result.SoftPlanes);
    return MoveTemp(B.Result);
}
MemoriaRegionalAtmosphereS363::FResult MemoriaRegionalAtmosphereS363::BuildChapter(AActor& Owner,
    const FMemoriaChapterMapSpec& Spec, const FString& Map, UMaterialInstanceDynamic* Stone)
{
    if (!IsEnabled() || HasFeature(Owner) || !Stone || (Map!=TEXT("belt_waystation") && Map!=TEXT("drift_shelter"))) return {};
    FBuilder B(Owner); const bool bWet=Map==TEXT("drift_shelter");
    const FString Prefix=bWet ? TEXT("S363Drift") : TEXT("S363Belt");
    const FLinearColor Shift=bWet ? FLinearColor(.93f,1.01f,1.12f) : FLinearColor(1.08f,1.f,.90f);
    auto* Edge=Sibling(Owner,Prefix+TEXT("Footing"),Stone,1.16f,Shift,bWet ? .48f : .9f);
    auto* Chip=Sibling(Owner,Prefix+TEXT("Chips"),Stone,.90f,Shift,bWet ? .46f : .97f);
    B.Keep(Edge); B.Keep(Chip);
    auto* Bounce=B.SoftMaterial(Prefix+TEXT("Bounce"),bWet ? FLinearColor(.15f,.19f,.26f) : FLinearColor(.28f,.125f,.035f),bWet ? .035f : .05f);
    auto* Contact=B.SoftMaterial(Prefix+TEXT("Contact"),bWet ? FLinearColor(.008f,.016f,.028f) : FLinearColor(.02f,.014f,.008f),.14f);
    const int32 Wall=Spec.TileNames.IndexOfByKey(TEXT("WALL"));
    const float T=Spec.TileSize*MemoriaChapterMaps::Scale;
    const FIntPoint Dirs[]={{1,0},{0,1},{-1,0},{0,-1}};
    for(int32 Y=0;Y<Spec.Height;++Y)for(int32 X=0;X<Spec.Width;++X)
    {
        if(Spec.TileAt(X,Y)!=Wall)continue;
        const FVector C=MemoriaChapterMaps::ToWorld(FVector2D((X+.5f)*Spec.TileSize,(Y+.5f)*Spec.TileSize));
        for(int32 Side=0;Side<4;++Side)
        {
            const int32 NX=X+Dirs[Side].X, NY=Y+Dirs[Side].Y;
            if(NX<0 || NY<0 || NX>=Spec.Width || NY>=Spec.Height || Spec.IsSolid(Spec.TileAt(NX,NY)))continue;
            const FVector Normal(Dirs[Side].X,-Dirs[Side].Y,0), Along(-Normal.Y,Normal.X,0);
            const uint32 P=Pattern(X,Y,Side);const float Yaw=FMath::RadiansToDegrees(FMath::Atan2(Along.Y,Along.X));
            // Both staggered stones sit wholly INSIDE the original solid tile.
            // The outward chips are under2cm high and never become blockers.
            for(int32 StoneIndex=0;StoneIndex<2;++StoneIndex)
            {
                const FVector At=C+Normal*(T*.5f-6.f)+Along*((StoneIndex==0 ? -.24f : .24f)*T)+FVector(0,0,Feet+5.5f);
                B.Box(*(Prefix+TEXT("Footings")),Edge,At,FVector(T*.43f,9.f,11.f),Yaw+float(int32((P>>(StoneIndex*3))%3)-1)*.4f);
            }
            if(P%3==0 && B.Result.SolidInstances<MaxSolidInstances)
            {
                const FVector At=C+Normal*(T*.5f+10.f)+Along*((float((P>>4)%15)/14.f-.5f)*T*.5f)+FVector(0,0,Feet+.45f);
                B.Box(*(Prefix+TEXT("Chips")),Chip,At,FVector(15.f+float(P%9),10.f+float((P>>5)%6),1.5f),float(P%180),false);
            }
        }
    }
    // Derive four pools from lights which already exist, never invent an
    // isolated bright patch. Spatial sorting avoids component-allocation order.
    TArray<UPointLightComponent*> Lights; Owner.GetComponents(Lights);
    Lights.Sort([](const UPointLightComponent& A,const UPointLightComponent& Other)
    {
        const FVector P=A.GetComponentLocation(),Q=Other.GetComponentLocation();
        if(P.X!=Q.X)return P.X<Q.X;if(P.Y!=Q.Y)return P.Y<Q.Y;return P.Z<Q.Z;
    });
    for(int32 I=0;I<FMath::Min(4,Lights.Num());++I)
    {
        FVector At=Lights[I]->GetComponentLocation()+FVector(0,-28,0);At.Z=Feet;
        B.Soft(Prefix+FString::Printf(TEXT("ThresholdContact%d"),I),Contact,At+FVector(0,0,.045f),FVector2D(185,82),I%2 ? -10.f : 10.f);
        B.Soft(Prefix+FString::Printf(TEXT("ThresholdBounce%d"),I),Bounce,At+FVector(0,-26,.07f),FVector2D(145,116),I%2 ? 14.f : -14.f);
    }
    UE_LOG(LogTemp,Display,TEXT("S363_REGION_BUILD map=%s solids=%d soft=%d"),*Map,B.Result.SolidInstances,B.Result.SoftPlanes);
    return MoveTemp(B.Result);
}
