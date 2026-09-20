#include "MemoriaDepthAssetsCommandlet.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionWorldPosition.h"
#include "Materials/MaterialExpressionVertexNormalWS.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "UObject/SavePackage.h"
namespace
{
const FString Base = TEXT("/Game/Memoria/Presentation/Depth/");
template<class T> T* Asset(const TCHAR* Name)
{ return NewObject<T>(CreatePackage(*(Base + Name)), Name, RF_Public | RF_Standalone); }
template<class T> T* Node(UMaterial* M)
{ auto* N = NewObject<T>(M); M->GetExpressionCollection().AddExpression(N); return N; }
bool Save(UObject* Object)
{
    FAssetRegistryModule::AssetCreated(Object); Object->MarkPackageDirty();
    FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone;
    const auto File = FPackageName::LongPackageNameToFilename(Object->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension());
    const bool OK = UPackage::SavePackage(Object->GetOutermost(), Object, *File, Args);
    UE_LOG(LogTemp, Display, TEXT("DEPTH_ASSET %s %s"), *Object->GetPathName(), OK ? TEXT("SAVED") : TEXT("FAILED"));
    return OK;
}
UMaterialExpressionScalarParameter* Scalar(UMaterial* M, const TCHAR* Name, float Value)
{ auto* N = Node<UMaterialExpressionScalarParameter>(M); N->ParameterName = Name; N->DefaultValue = Value; return N; }
UMaterialExpressionVectorParameter* Color(UMaterial* M, FLinearColor Value)
{ auto* N = Node<UMaterialExpressionVectorParameter>(M); N->ParameterName = TEXT("Tint"); N->DefaultValue = Value; return N; }
void Input(UMaterialExpressionCustom* N, const TCHAR* Name, UMaterialExpression* Value)
{ FCustomInput I; I.InputName = Name; I.Input.Connect(0, Value); N->Inputs.Add(I); }
}
UMemoriaDepthAssetsCommandlet::UMemoriaDepthAssetsCommandlet()
{ IsClient = false; IsServer = false; IsEditor = true; LogToConsole = true; }
int32 UMemoriaDepthAssetsCommandlet::Main(const FString& ParamsText)
{
    // Scoped repair for the three materials authored by this commandlet only.
    if (FParse::Param(*ParamsText, TEXT("EnableInstancing")))
    {
        TArray<UMaterial*> Materials;
        for (const TCHAR* Name : {TEXT("M_Surface"), TEXT("M_Paving"), TEXT("M_Glow")})
        {
            auto* M = LoadObject<UMaterial>(nullptr, *(Base+Name+TEXT(".")+Name));
            if (!M) return 1;
            Materials.Add(M);
        }
        for (auto* M : Materials) { M->bUsedWithInstancedStaticMeshes = true; M->PostEditChange(); if (!Save(M)) return 1; }
        return 0;
    }
    for (const TCHAR* Name : {TEXT("M_Surface"), TEXT("M_Paving"), TEXT("M_Glow"), TEXT("SM_PitchedRoof")})
        if (FPackageName::DoesPackageExist(Base + Name))
        { UE_LOG(LogTemp, Error, TEXT("Refusing existing depth asset %s"), Name); return 1; }
    auto* Source = LoadObject<UTexture2D>(nullptr, TEXT("/Game/Memoria/Presentation/Verdan/T_Market.T_Market"));
    if (!Source) return 1;
    auto* Surface = Asset<UMaterial>(TEXT("M_Surface"));
    Surface->bUsedWithInstancedStaticMeshes = true; Surface->SetShadingModel(MSM_DefaultLit); Surface->TwoSided = true;
    auto* Pattern = Node<UMaterialExpressionCustom>(Surface);
    Pattern->Inputs.Reset(); Pattern->OutputType = CMOT_Float3;
    Pattern->Code = TEXT(R"(
float3 a = abs(N);
float2 q = a.z > 0.6 ? P.xy : (a.x > a.y ? P.yz : P.xz);
float shade = 1;
if (Mode < 0.5) {
    float2 g = q / float2(58, 28);
    g.x += fmod(abs(floor(g.y)), 2) * 0.5;
    float2 f = frac(g);
    float edge = min(min(f.x, 1-f.x), min(f.y, 1-f.y));
    float mortar = smoothstep(0.01, 0.055, edge);
    float noise = frac(sin(dot(floor(g), float2(127.1,311.7))) * 43758.5453);
    shade = lerp(0.32, 0.78 + noise*0.3, mortar);
} else if (Mode < 1.5) {
    shade = 0.87 + 0.075*sin(q.x*1.2 + sin(q.y*0.035)*3) + 0.025*sin(q.x*3.6);
} else if (Mode < 2.5) {
    shade = 0.94 + 0.06*sin(q.x*1.1)*sin(q.y*1.1);
}
return Tint.rgb * shade;
)");
    Input(Pattern, TEXT("P"), Node<UMaterialExpressionWorldPosition>(Surface));
    Input(Pattern, TEXT("N"), Node<UMaterialExpressionVertexNormalWS>(Surface));
    Input(Pattern, TEXT("Tint"), Color(Surface, FLinearColor(0.16f,0.18f,0.20f)));
    Input(Pattern, TEXT("Mode"), Scalar(Surface, TEXT("Mode"), 0));
    Surface->GetEditorOnlyData()->BaseColor.Connect(0, Pattern);
    Surface->GetEditorOnlyData()->Roughness.Connect(0, Scalar(Surface, TEXT("Roughness"), 0.82f));
    Surface->GetEditorOnlyData()->Metallic.Connect(0, Scalar(Surface, TEXT("Metallic"), 0));
    Surface->PostEditChange(); if (!Save(Surface)) return 1;

    auto* Paving = Asset<UMaterial>(TEXT("M_Paving"));
    Paving->bUsedWithInstancedStaticMeshes = true; Paving->SetShadingModel(MSM_DefaultLit); Paving->TwoSided = true;
    auto* UV = Node<UMaterialExpressionCustom>(Paving); UV->Inputs.Reset(); UV->OutputType = CMOT_Float2;
    UV->Code = TEXT("return (float2(250,205) + frac(P.xy / 220.0) * float2(255,255) + 0.5) / float2(1448,1086);");
    Input(UV, TEXT("P"), Node<UMaterialExpressionWorldPosition>(Paving));
    auto* Texture = Node<UMaterialExpressionTextureSampleParameter2D>(Paving);
    Texture->Texture = Source; Texture->ParameterName = TEXT("OriginalMarketAtlas"); Texture->Coordinates.Connect(0, UV);
    auto* Tone = Node<UMaterialExpressionMultiply>(Paving); Tone->A.Connect(0, Texture); Tone->B.Connect(0, Color(Paving, FLinearColor(1.35f,1.4f,1.45f)));
    Paving->GetEditorOnlyData()->BaseColor.Connect(0, Tone);
    Paving->GetEditorOnlyData()->Roughness.Connect(0, Scalar(Paving, TEXT("Roughness"), 0.72f));
    Paving->PostEditChange(); if (!Save(Paving)) return 1;

    auto* Glow = Asset<UMaterial>(TEXT("M_Glow")); Glow->bUsedWithInstancedStaticMeshes = true; Glow->SetShadingModel(MSM_Unlit); Glow->TwoSided = true;
    Glow->GetEditorOnlyData()->EmissiveColor.Connect(0, Color(Glow, FLinearColor(2.5f,0.85f,0.2f)));
    Glow->PostEditChange(); if (!Save(Glow)) return 1;

    FMeshDescription Description; FStaticMeshAttributes Attributes(Description); Attributes.Register();
    auto Positions = Attributes.GetVertexPositions(); auto Normals = Attributes.GetVertexInstanceNormals();
    auto Tangents = Attributes.GetVertexInstanceTangents(); auto Signs = Attributes.GetVertexInstanceBinormalSigns();
    auto UVs = Attributes.GetVertexInstanceUVs(); UVs.SetNumChannels(1);
    const FVector3f Points[] = {{-50,-50,0},{50,-50,0},{-50,50,0},{50,50,0},{-50,0,40},{50,0,40}};
    TArray<FVertexID> Vertices;
    for (const auto& P : Points) { const auto V = Description.CreateVertex(); Vertices.Add(V); Positions[V] = P; }
    const auto Group = Description.CreatePolygonGroup(); Attributes.GetPolygonGroupMaterialSlotNames()[Group] = TEXT("Surface");
    const int32 Triangles[][3] = {{0,1,5},{0,5,4},{2,4,5},{2,5,3},{0,4,2},{1,3,5},{0,2,3},{0,3,1}};
    for (const auto& Triangle : Triangles)
    {
        const FVector3f Normal = FVector3f::CrossProduct(Points[Triangle[1]]-Points[Triangle[0]], Points[Triangle[2]]-Points[Triangle[0]]).GetSafeNormal();
        const FVector3f Tangent = (Points[Triangle[1]]-Points[Triangle[0]]).GetSafeNormal();
        TArray<FVertexInstanceID> Instances;
        for (int32 I = 0; I < 3; ++I)
        {
            const auto VI = Description.CreateVertexInstance(Vertices[Triangle[I]]); Instances.Add(VI);
            Normals[VI] = Normal; Tangents[VI] = Tangent; Signs[VI] = 1;
            UVs.Set(VI, 0, FVector2f(I == 1 ? 1 : 0, I == 2 ? 1 : 0));
        }
        Description.CreatePolygon(Group, Instances);
    }
    auto* Roof = Asset<UStaticMesh>(TEXT("SM_PitchedRoof"));
    Roof->GetStaticMaterials().Add(FStaticMaterial(Surface, TEXT("Surface")));
    UStaticMesh::FBuildMeshDescriptionsParams Params; Params.bBuildSimpleCollision = false; Params.bFastBuild = false;
    if (!Roof->BuildFromMeshDescriptions({&Description}, Params)) return 1;
    return Save(Roof) ? 0 : 1;
}
