#include "MemoriaMarketPropsCommandlet.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "StaticMeshCompiler.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"

namespace
{
const FString Base = TEXT("/Game/Memoria/Presentation/MarketPropsS355/");
const TCHAR* Names[] = {TEXT("SM_BarrelWood"), TEXT("SM_BarrelIron"), TEXT("SM_CrateWood"), TEXT("SM_CrateIron"),
    TEXT("SM_SignWood"), TEXT("SM_SignIron"), TEXT("SM_FireStone"), TEXT("SM_FireWood"), TEXT("SM_FireEmbers"), TEXT("SM_FireFlame")};

// All coordinates are authored in cm about the original prop anchor. Geometry
// contains no collision, RNG, gameplay state or dependence on source images.
struct FPropMesh
{
    FMeshDescription Description;
    FPolygonGroupID Group;
    int32 Triangles = 0;
    FPropMesh()
    {
        FStaticMeshAttributes A(Description); A.Register();
        A.GetVertexInstanceUVs().SetNumChannels(1);
        Group = Description.CreatePolygonGroup();
        A.GetPolygonGroupMaterialSlotNames()[Group] = TEXT("Surface");
    }
    void Tri(FVector P0, FVector P1, FVector P2)
    {
        const FVector Normal = FVector::CrossProduct(P1-P0, P2-P0).GetSafeNormal();
        if (Normal.IsNearlyZero()) return;
        FStaticMeshAttributes A(Description);
        const FVector Points[] = {P0, P1, P2};
        TArray<FVertexInstanceID> Instances;
        for (int32 I=0; I<3; ++I)
        {
            const auto V = Description.CreateVertex(); A.GetVertexPositions()[V] = FVector3f(Points[I]);
            const auto VI = Description.CreateVertexInstance(V); Instances.Add(VI);
            A.GetVertexInstanceNormals()[VI] = FVector3f(Normal);
            A.GetVertexInstanceTangents()[VI] = FVector3f((P1-P0).GetSafeNormal());
            A.GetVertexInstanceBinormalSigns()[VI] = 1;
            A.GetVertexInstanceColors()[VI] = FVector4f(1,1,1,1);
            A.GetVertexInstanceUVs().Set(VI, 0, FVector2f(Points[I].X*.025, Points[I].Z*.025));
        }
        Description.CreatePolygon(Group, Instances); ++Triangles;
    }
    void Quad(FVector A, FVector B, FVector C, FVector D) { Tri(A,B,C); Tri(A,C,D); }
    void Box(FVector Center, FVector Size, double Bevel=0.7, FRotator Rotation=FRotator::ZeroRotator)
    {
        const double X=Size.X*.5, Y=Size.Y*.5, Z=Size.Z*.5;
        const double B=FMath::Min(Bevel, FMath::Min3(X,Y,Z)*.45);
        const FTransform T(Rotation,Center);
        TArray<TArray<FVector>> Rings;
        for (int32 Level=0; Level<4; ++Level)
        {
            const bool bEnd=Level==0 || Level==3;
            const double Rx=X-(bEnd?B:0), Ry=Y-(bEnd?B:0);
            const double H=Level==0?-Z:Level==1?-Z+B:Level==2?Z-B:Z;
            const FVector2D Cross[]={{Rx-B,-Ry},{Rx,-Ry+B},{Rx,Ry-B},{Rx-B,Ry},
                {-Rx+B,Ry},{-Rx,Ry-B},{-Rx,-Ry+B},{-Rx+B,-Ry}};
            TArray<FVector> Ring;
            for (const auto& P:Cross) Ring.Add(T.TransformPosition(FVector(P.X,P.Y,H)));
            Rings.Add(Ring);
        }
        for(int32 L=0; L<3; ++L) for(int32 I=0; I<8; ++I)
            Quad(Rings[L][I],Rings[L][(I+1)%8],Rings[L+1][(I+1)%8],Rings[L+1][I]);
        for(int32 I=0; I<8; ++I)
        {
            Tri(T.TransformPosition(FVector(0,0,-Z)),Rings[0][(I+1)%8],Rings[0][I]);
            Tri(T.TransformPosition(FVector(0,0,Z)),Rings[3][I],Rings[3][(I+1)%8]);
        }
    }
    void Lathe(const TArray<FVector2D>& Profile, int32 Sides, const FTransform& Transform=FTransform::Identity,
        FVector2D Bend=FVector2D::ZeroVector)
    {
        TArray<TArray<FVector>> Rings;
        for(int32 L=0; L<Profile.Num(); ++L)
        {
            TArray<FVector> Ring;
            const double T=double(L)/FMath::Max(1,Profile.Num()-1);
            for(int32 I=0; I<Sides; ++I)
            {
                const double Angle=I*2*PI/Sides;
                Ring.Add(Transform.TransformPosition(FVector(FMath::Cos(Angle)*Profile[L].X+Bend.X*T*T,
                    FMath::Sin(Angle)*Profile[L].X+Bend.Y*T*T,Profile[L].Y)));
            }
            Rings.Add(Ring);
        }
        for(int32 L=0; L<Rings.Num()-1; ++L) for(int32 I=0; I<Sides; ++I)
            Quad(Rings[L][I],Rings[L][(I+1)%Sides],Rings[L+1][(I+1)%Sides],Rings[L+1][I]);
        for(int32 I=1; I<Sides-1; ++I)
        {
            Tri(Rings[0][0],Rings[0][I+1],Rings[0][I]);
            Tri(Rings.Last()[0],Rings.Last()[I],Rings.Last()[I+1]);
        }
    }
    void Hoop(double Z, double Radius, double Height)
    {
        const int32 Sides=48;
        for(int32 I=0; I<Sides; ++I)
        {
            const double A=I*2*PI/Sides, B=(I+1)*2*PI/Sides;
            auto P=[&](double R,double Angle,double H){return FVector(R*FMath::Cos(Angle),R*FMath::Sin(Angle),H);};
            Quad(P(Radius,A,Z-Height*.5),P(Radius,B,Z-Height*.5),P(Radius,B,Z+Height*.5),P(Radius,A,Z+Height*.5));
            Quad(P(Radius,A,Z+Height*.5),P(Radius,B,Z+Height*.5),P(Radius-1.0,B,Z+Height*.5),P(Radius-1.0,A,Z+Height*.5));
            Quad(P(Radius-1.0,A,Z-Height*.5),P(Radius-1.0,B,Z-Height*.5),P(Radius,B,Z-Height*.5),P(Radius,A,Z-Height*.5));
        }
    }
    bool Save(const TCHAR* Name, bool bGlow=false)
    {
        auto* Mesh=NewObject<UStaticMesh>(CreatePackage(*(Base+Name)),Name,RF_Public|RF_Standalone);
        auto* Material=LoadObject<UMaterialInterface>(nullptr,bGlow?
            TEXT("/Game/Memoria/Presentation/Depth/M_Glow.M_Glow"):
            TEXT("/Game/Memoria/Presentation/Depth2/M_FocusSurface.M_FocusSurface"));
        if(!Material) return false;
        Mesh->GetStaticMaterials().Add(FStaticMaterial(Material,TEXT("Surface")));
        UStaticMesh::FBuildMeshDescriptionsParams Params; Params.bBuildSimpleCollision=false; Params.bFastBuild=false;
        if(!Mesh->BuildFromMeshDescriptions({&Description},Params)) return false;
        FStaticMeshCompilingManager::Get().FinishAllCompilation();
        FAssetRegistryModule::AssetCreated(Mesh); Mesh->MarkPackageDirty();
        FSavePackageArgs Args; Args.TopLevelFlags=RF_Public|RF_Standalone;
        const FString File=FPackageName::LongPackageNameToFilename(Base+Name,FPackageName::GetAssetPackageExtension());
        const bool bSaved=UPackage::SavePackage(Mesh->GetOutermost(),Mesh,*File,Args);
        UE_LOG(LogTemp,Display,TEXT("MARKET_PROP %s triangles=%d bounds=%s saved=%d"),Name,Triangles,*Mesh->GetBounds().BoxExtent.ToString(),bSaved);
        return bSaved;
    }
};

bool Barrel()
{
    FPropMesh Wood, Iron;
    const double Radius[]={18.8,21.0,22.0,21.0,18.8}, Height[]={0,14,36,58,72};
    // Sixteen individual closed staves: light catches the curved body and dark
    // seams remain geometric at oblique angles, including after a prop is spent.
    for(int32 Stave=0; Stave<16; ++Stave)
    {
        const double Start=Stave*2*PI/16+.012, End=(Stave+1)*2*PI/16-.012;
        auto P=[&](int32 Level,double Angle,bool Inner){const double R=Radius[Level]-(Inner?1.6:0);return FVector(R*FMath::Cos(Angle),R*FMath::Sin(Angle),Height[Level]);};
        for(int32 L=0; L<4; ++L)
        {
            for(int32 Arc=0; Arc<3; ++Arc)
            {
                const double A=FMath::Lerp(Start,End,Arc/3.0), B=FMath::Lerp(Start,End,(Arc+1)/3.0);
                Wood.Quad(P(L,A,false),P(L,B,false),P(L+1,B,false),P(L+1,A,false));
                Wood.Quad(P(L,B,true),P(L,A,true),P(L+1,A,true),P(L+1,B,true));
            }
            Wood.Quad(P(L,Start,true),P(L,Start,false),P(L+1,Start,false),P(L+1,Start,true));
            Wood.Quad(P(L,End,false),P(L,End,true),P(L+1,End,true),P(L+1,End,false));
        }
        Wood.Quad(P(4,Start,false),P(4,End,false),P(4,End,true),P(4,Start,true));
    }
    for(int32 I=-2; I<=2; ++I)
    {
        const double X=I*6.4;
        const double HalfY=FMath::Sqrt(FMath::Max(0.0,17.0*17.0-FMath::Square(FMath::Abs(X)+3.0)));
        Wood.Box(FVector(X,0,70),FVector(6.0,HalfY*2,2.6),.35);
    }
    Iron.Hoop(14,21.65,4.8); Iron.Hoop(58,21.65,4.8); Iron.Hoop(2,19.8,2.8); Iron.Hoop(70,19.8,2.8);
    for(double Z:{14.0,58.0}) for(int32 I=0; I<8; ++I)
    {
        const double A=I*PI/4;
        Iron.Box(FVector(21.8*FMath::Cos(A),21.8*FMath::Sin(A),Z),FVector(1.5,2.0,1.8),.5,FRotator(0,FMath::RadiansToDegrees(A),0));
    }
    return Wood.Save(Names[0]) && Iron.Save(Names[1]);
}
bool Crate()
{
    FPropMesh Wood, Iron;
    for(int32 I=0; I<6; ++I)
    {
        const double At=-19.0+I*7.6;
        for(double Side:{-1.0,1.0})
        {
            Wood.Box(FVector(At,Side*21.5,23),FVector(7.05,3.0,43),.45);
            Wood.Box(FVector(Side*21.5,At,23),FVector(3.0,7.05,43),.45);
        }
        Wood.Box(FVector(At,0,44.5),FVector(7.05,43,3),.45);
    }
    for(double Y:{-23.5,23.5})
    {
        for(double Z:{5.0,41.0}) Wood.Box(FVector(0,Y,Z),FVector(46,3,5),.6);
        Wood.Box(FVector(0,Y,23),FVector(54,3,5),.6,FRotator(40,0,0));
    }
    for(double X:{-20.0,20.0}) for(double Y:{-23.6,23.6})
    {
        Iron.Box(FVector(X,Y,23),FVector(4.5,1.7,46),.4);
        for(double Z:{6.0,23.0,40.0}) Iron.Box(FVector(X,Y*1.035,Z),FVector(2.0,1.4,2.0),.6);
    }
    for(double X:{-20.0,20.0}) Iron.Box(FVector(X,0,46.0),FVector(4.5,48,1.6),.35);
    return Wood.Save(Names[2]) && Iron.Save(Names[3]);
}
bool Sign()
{
    FPropMesh Wood, Iron;
    Wood.Box(FVector(0,0,70),FVector(9,9,156),1.1);
    for(int32 I=0; I<3; ++I) Wood.Box(FVector((I-1)*.65,-8,104+I*14),FVector(86-I,5,13.2),.8,FRotator(0,0,(I-1)*.6));
    for(double X:{-35.0,35.0})
    {
        Iron.Box(FVector(X,-11.2,118),FVector(4,1.7,42),.4);
        for(double Z:{103.0,118.0,133.0}) Iron.Box(FVector(X,-12.4,Z),FVector(2.2,1.4,2.2),.6);
    }
    for(double Z:{10.0,86.0}) Iron.Box(FVector(0,0,Z),FVector(10.5,10.5,4),.55);
    // A weathered diamond medallion, without invented writing or lore.
    Iron.Box(FVector(0,-11.6,119),FVector(15,2,15),.7,FRotator(45,0,0));
    return Wood.Save(Names[4]) && Iron.Save(Names[5]);
}
bool Fire()
{
    FPropMesh Stone, Wood, Embers, Flame;
    for(int32 I=0; I<11; ++I)
    {
        const double A=I*2*PI/11, Radius=39+(I%3)*1.2;
        Stone.Box(FVector(FMath::Cos(A)*Radius,FMath::Sin(A)*Radius,6.5),FVector(18+(I%3)*2,14+(I%2)*3,13+(I%3)),3.5,
            FRotator((I%3-1)*7,FMath::RadiansToDegrees(A)+13,(I%2)*6));
    }
    for(int32 I=0; I<3; ++I)
    {
        const FTransform T(FRotator(88,I*63.0+20,0),FVector(0,0,9+I*4));
        Wood.Lathe({FVector2D(4.3,-34),FVector2D(5.8,-29),FVector2D(6.2,26),FVector2D(4.8,34)},10,T);
        for(int32 K=0; K<4; ++K)
            Embers.Box(FVector((K-1.5)*9,(I-1)*10,15+(K%2)*2),FVector(6,5,2),.8,FRotator(0,K*23,0));
    }
    for(int32 I=0; I<5; ++I)
    {
        const double A=I*2*PI/5;
        Flame.Lathe({FVector2D(3,0),FVector2D(6,8),FVector2D(3.8,18),FVector2D(1.6,29),FVector2D(.15,38+(I%2)*7)},9,
            FTransform(FRotator::ZeroRotator,FVector(FMath::Cos(A)*8,FMath::Sin(A)*8,17)),FVector2D(FMath::Sin(A)*5,FMath::Cos(A)*4));
    }
    return Stone.Save(Names[6]) && Wood.Save(Names[7]) && Embers.Save(Names[8],true) && Flame.Save(Names[9],true);
}
}
UMemoriaMarketPropsCommandlet::UMemoriaMarketPropsCommandlet()
{ IsClient=false; IsServer=false; IsEditor=true; LogToConsole=true; }
int32 UMemoriaMarketPropsCommandlet::Main(const FString& Params)
{
    for(const TCHAR* Name:Names) if(FPackageName::DoesPackageExist(Base+Name))
    { UE_LOG(LogTemp,Error,TEXT("Refusing to overwrite immutable market prop %s"),Name); return 1; }
    return Barrel() && Crate() && Sign() && Fire() ? 0 : 1;
}
