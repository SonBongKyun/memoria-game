#include "MemoriaVisualPolishAssetsCommandlet.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "MaterialShared.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionVertexNormalWS.h"
#include "Materials/MaterialExpressionWorldPosition.h"
#include "MeshDescription.h"
#include "Misc/App.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "RHI.h"
#include "ShaderCompiler.h"
#include "StaticMeshAttributes.h"
#include "StaticMeshCompiler.h"
#include "StaticMeshResources.h"
#include "UObject/SavePackage.h"
#include "UnrealEngine.h"

namespace
{
const FString Base = TEXT("/Game/Memoria/Presentation/VisualPolish/");
struct FPaintedAsset { const TCHAR* Folder; const TCHAR* Name; int32 Kind; };
// Kind 0: NPC, 1: tank, 2: fire ring, 3: concrete rubble. No other source packages are writable here.
const FPaintedAsset Painted[] = {
    {TEXT("Traveler"), TEXT("Traveler"), 0}, {TEXT("Bureauagent"), TEXT("Bureauagent"), 0},
    {TEXT("Guard"), TEXT("Guard"), 0}, {TEXT("Props"), TEXT("WaterTank"), 1},
    {TEXT("Props"), TEXT("Campfire"), 2}, {TEXT("Props"), TEXT("Rubble"), 3}};
FString PaintedPath(const FPaintedAsset& A, const TCHAR* Prefix)
{ return FString::Printf(TEXT("/Game/Memoria/Presentation/Field3D/%s/%s%s"), A.Folder, Prefix, A.Name); }
template<class T> T* Load(const FString& Path)
{ return LoadObject<T>(nullptr, *(Path + TEXT(".") + FPackageName::GetShortName(Path))); }
template<class T> T* Asset(const FString& Path)
{
    if (T* Existing = Load<T>(Path)) return Existing;
    return NewObject<T>(CreatePackage(*Path), *FPackageName::GetShortName(Path), RF_Public | RF_Standalone);
}
template<class T> T* Node(UMaterial* M)
{ auto* N = NewObject<T>(M); M->GetExpressionCollection().AddExpression(N); return N; }
UMaterialExpressionScalarParameter* Scalar(UMaterial* M, const TCHAR* Name, float Value)
{ auto* N = Node<UMaterialExpressionScalarParameter>(M); N->ParameterName = Name; N->DefaultValue = Value; return N; }
UMaterialExpressionConstant* Constant(UMaterial* M, float Value)
{ auto* N = Node<UMaterialExpressionConstant>(M); N->R = Value; return N; }
void Input(UMaterialExpressionCustom* N, const TCHAR* Name, UMaterialExpression* Value)
{ FCustomInput I; I.InputName = Name; I.Input.Connect(0, Value); N->Inputs.Add(I); }
void Output(UMaterialExpressionCustom* N, const TCHAR* Name, ECustomMaterialOutputType Type)
{ FCustomOutput O; O.OutputName = Name; O.OutputType = Type; N->AdditionalOutputs.Add(O); }
void Prepare(UMaterial* M, bool bSkeletal)
{
    M->GetExpressionCollection().Empty(); M->SetShadingModel(MSM_DefaultLit);
    M->TwoSided = true; M->bTangentSpaceNormal = false;
    M->SetUsageByFlag(MATUSAGE_SkeletalMesh, bSkeletal);
    M->SetUsageByFlag(MATUSAGE_InstancedStaticMeshes, !bSkeletal);
    M->GetEditorOnlyData()->Normal.Expression = nullptr;
}
bool Save(UObject* Object)
{
    const FString Package = Object->GetOutermost()->GetName();
    if (!FPackageName::DoesPackageExist(Package)) FAssetRegistryModule::AssetCreated(Object);
    Object->MarkPackageDirty(); FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone;
    const FString File = FPackageName::LongPackageNameToFilename(Package, FPackageName::GetAssetPackageExtension());
    const bool OK = UPackage::SavePackage(Object->GetOutermost(), Object, *File, Args);
    UE_LOG(LogTemp, Display, TEXT("VISUAL_POLISH_ASSET %s %s"), *Object->GetPathName(), OK ? TEXT("SAVED") : TEXT("FAILED"));
    return OK;
}
UMaterial* StoneSurface()
{
    auto* M = Asset<UMaterial>(Base + TEXT("M_StoneSurface")); Prepare(M, false);
    auto* Pattern = Node<UMaterialExpressionCustom>(M); Pattern->Inputs.Reset(); Pattern->OutputType = CMOT_Float3;
    Output(Pattern, TEXT("WorldNormal"), CMOT_Float3); Output(Pattern, TEXT("SurfaceRoughness"), CMOT_Float1);
    Pattern->Description = TEXT("Quiet world-space stone, earth and wall courses (centimetres)");
    Pattern->Code = TEXT(R"(
struct SurfacePattern {
    float hash(float2 p) { return frac(sin(dot(p,float2(127.1,311.7)))*43758.5453); }
    // x: albedo multiplier, y: height in cm. Bounded, broad detail; no texture dependencies.
    float2 evaluate(float2 q, float mode, float pixelWidth) {
        float broad = sin(q.x*.043 + sin(q.y*.019))*sin(q.y*.037);
        float grainAA = 1-smoothstep(1.0,4.0,pixelWidth);
        float grain = sin(q.x*.29 + sin(q.y*.12))*sin(q.y*.24)*grainAA;
        if (mode < .5 || (mode >= 1.5 && mode < 2.5)) {
            bool wall = mode > 1.5;
            float rowSize = wall ? 40.0 : 44.0;
            float row = floor(q.y/rowSize);
            float width = wall ? 88.0 : lerp(54.0,78.0,hash(float2(row,7)));
            float2 g = float2(q.x/width + hash(float2(row,3))*.7, q.y/rowSize);
            float2 cell = floor(g), f = frac(g);
            float edge = min(min(f.x,1-f.x)*width,min(f.y,1-f.y)*rowSize);
            float variation = hash(cell);
            float seamWidth = wall ? 1.2 : 1.7;
            float stone = smoothstep(.25, seamWidth + max(.4,pixelWidth*.6), edge);
            float tone = .91 + variation*.15 + broad*.035 + grain*.012;
            return float2(lerp(.68,tone,stone), stone*(wall ? .42 : .65)+broad*.12+grain*.025);
        }
        // Earth and broken concrete remain irregular, without a repeated brick pattern.
        float earth = mode < 1.5 ? 1.0 : 0.0;
        return float2(.96+broad*(earth>.5 ? .08 : .045)+grain*.018,
                      broad*(earth>.5 ? .22 : .13)+grain*.035);
    }
};
SurfacePattern surface;
float3 n = normalize(N), a = abs(n);
float3 U = a.z>.6 ? float3(1,0,0) : (a.x>a.y ? float3(0,1,0) : float3(1,0,0));
float3 V = a.z>.6 ? float3(0,1,0) : float3(0,0,1);
float2 q = float2(dot(P,U),dot(P,V));
float pixelWidth = max(length(ddx(q)),length(ddy(q)));
float2 s = surface.evaluate(q,Mode,pixelWidth);
float stepSize = max(.6,pixelWidth*.5);
float dx = (surface.evaluate(q+float2(stepSize,0),Mode,pixelWidth).y-s.y)/stepSize;
float dy = (surface.evaluate(q+float2(0,stepSize),Mode,pixelWidth).y-s.y)/stepSize;
WorldNormal = normalize(n-U*dx-V*dy);
SurfaceRoughness = clamp(Roughness+(1-s.x)*.12,.68,.98);
return Tint.rgb*s.x;
)");
    Input(Pattern, TEXT("P"), Node<UMaterialExpressionWorldPosition>(M));
    Input(Pattern, TEXT("N"), Node<UMaterialExpressionVertexNormalWS>(M));
    auto* Tint = Node<UMaterialExpressionVectorParameter>(M); Tint->ParameterName = TEXT("Tint"); Tint->DefaultValue = FLinearColor(.16f,.18f,.20f);
    Input(Pattern, TEXT("Tint"), Tint); Input(Pattern, TEXT("Mode"), Scalar(M,TEXT("Mode"),0));
    Input(Pattern, TEXT("Roughness"), Scalar(M,TEXT("Roughness"),.88f));
    Pattern->RebuildOutputs();
    M->GetEditorOnlyData()->BaseColor.Connect(0,Pattern); M->GetEditorOnlyData()->Normal.Connect(1,Pattern);
    M->GetEditorOnlyData()->Roughness.Connect(2,Pattern); M->GetEditorOnlyData()->Metallic.Connect(0,Constant(M,0));
    M->GetEditorOnlyData()->Specular.Connect(0,Constant(M,.24f)); M->GetEditorOnlyData()->EmissiveColor.Connect(0,Constant(M,0));
    M->PostEditChange(); return M;
}
void PolishPainted(UMaterial* M, UTexture2D* Texture, int32 Kind)
{
    Prepare(M,Kind==0);
    auto* Sample = Node<UMaterialExpressionTextureSample>(M); Sample->Texture = Texture;
    auto* Pattern = Node<UMaterialExpressionCustom>(M); Pattern->Inputs.Reset(); Pattern->OutputType = CMOT_Float3;
    Output(Pattern,TEXT("SurfaceRoughness"),CMOT_Float1); Output(Pattern,TEXT("Metalness"),CMOT_Float1);
    Output(Pattern,TEXT("WorldNormal"),CMOT_Float3);
    Pattern->Description = TEXT("S334 painted atlas surface identities; original face colour remains intact");
    Pattern->Code = TEXT(R"(
// The FBX importer restores texture top-left UV origin. S334 padded islands do not straddle tile edges.
float2 uv = saturate(UV);
float luma = dot(Color.rgb,float3(.2126,.7152,.0722));
float3 n = normalize(N), a = abs(n);
float3 U = a.z>.6 ? float3(1,0,0) : (a.x>a.y ? float3(0,1,0) : float3(1,0,0));
float3 V = a.z>.6 ? float3(0,1,0) : float3(0,0,1);
float2 q = float2(dot(P,U),dot(P,V));
float pixelWidth = max(length(ddx(q)),length(ddy(q)));
float grainAA = 1-smoothstep(.8,3.0,pixelWidth);
float x = q.x*.31+sin(q.y*.09), y = q.y*.27;
float grain = sin(x)*sin(y)*grainAA;
float broad = sin(q.x*.026+sin(q.y*.015))*sin(q.y*.022);
Metalness = 0; WorldNormal = n;
float3 color = Color.rgb;
if (Kind < .5) {
    // Four columns, two rows above the painted head (lower half). Skin/head get no metal or relief.
    int tile = min(3,(int)floor(uv.x*4))+4*min(1,(int)floor(uv.y*4));
    bool head = uv.y>=.5, skin = tile==3;
    bool metal = !head && (tile==5 || tile==6);
    bool leather = !head && tile==1;
    SurfaceRoughness = head || skin ? .84 : (metal ? .69 : (leather ? .76 : .88));
    SurfaceRoughness = clamp(SurfaceRoughness+(luma-.18)*.045,.68,.90);
    Metalness = metal ? .42 : 0;
} else {
    int tile = min(1,(int)floor(uv.x*2))+2*min(1,(int)floor(uv.y*2));
    bool tank = Kind<1.5, fire = Kind<2.5;
    bool metal = tank && tile==1;
    SurfaceRoughness = tank ? (metal ? .60 : .83) : (fire && tile==1 ? .94 : .90);
    SurfaceRoughness = clamp(SurfaceRoughness+broad*.035+grain*.025,.55,.97);
    Metalness = metal ? .48 : (tank ? .055 : 0);
    if (tank && tile==0) color *= .97+broad*.085; // Muted rust variation, no artificial bright glints.
    if (Kind>=2.5) color *= .65; // Bring pale concrete into the chapter maps' weathered stone palette.
    float strength = tank ? .013 : .023;
    float dx = cos(x)*.31*sin(y)*grainAA*strength;
    float dy = (sin(x)*cos(y)*.27+cos(x)*cos(q.y*.09)*.09*sin(y))*grainAA*strength;
    WorldNormal = normalize(n-U*dx-V*dy);
}
return color;
)");
    Input(Pattern,TEXT("Color"),Sample); Input(Pattern,TEXT("UV"),Node<UMaterialExpressionTextureCoordinate>(M));
    Input(Pattern,TEXT("P"),Node<UMaterialExpressionWorldPosition>(M));
    Input(Pattern,TEXT("N"),Node<UMaterialExpressionVertexNormalWS>(M)); Input(Pattern,TEXT("Kind"),Constant(M,static_cast<float>(Kind)));
    Pattern->RebuildOutputs();
    M->GetEditorOnlyData()->BaseColor.Connect(0,Pattern); M->GetEditorOnlyData()->Roughness.Connect(1,Pattern);
    M->GetEditorOnlyData()->Metallic.Connect(2,Pattern);
    if (Kind!=0) M->GetEditorOnlyData()->Normal.Connect(3,Pattern);
    M->GetEditorOnlyData()->Specular.Connect(0,Constant(M,Kind==0 ? .28f : .32f));
    // S335 intentionally retained this neutral fill to keep unlit sides legible. Preserve it exactly.
    auto* Fill = Node<UMaterialExpressionMultiply>(M); Fill->A.Connect(0,Sample); Fill->ConstB = .10f;
    M->GetEditorOnlyData()->EmissiveColor.Connect(0,Fill); M->PostEditChange();
}
UStaticMesh* BeveledBlock(UMaterial* Surface)
{
    FMeshDescription Description; FStaticMeshAttributes Attributes(Description); Attributes.Register();
    auto Positions = Attributes.GetVertexPositions(); auto Normals = Attributes.GetVertexInstanceNormals();
    auto Tangents = Attributes.GetVertexInstanceTangents(); auto Signs = Attributes.GetVertexInstanceBinormalSigns();
    auto UVs = Attributes.GetVertexInstanceUVs(); UVs.SetNumChannels(1);
    const auto Group = Description.CreatePolygonGroup(); Attributes.GetPolygonGroupMaterialSlotNames()[Group] = TEXT("Surface");
    // 6 broad planes, 12 bevel strips, 8 corner triangles. Exactly 100 cm across, 3 cm edge chamfer.
    auto Face = [&](TArray<FVector3f> Points) {
        // Match FStaticMeshOperations: Unreal's left-handed coordinates use the reverse cross.
        // The build recomputes normals from this winding; opposite winding gives inward lit normals.
        FVector3f Normal = FVector3f::CrossProduct(Points[2]-Points[0],Points[1]-Points[0]).GetSafeNormal();
        FVector3f Center = FVector3f::ZeroVector; for (const auto& P : Points) Center+=P; Center/=Points.Num();
        if (FVector3f::DotProduct(Normal,Center)<0) { Swap(Points[1],Points.Last()); Normal=-Normal; }
        const FVector3f Tangent = (Points[1]-Points[0]).GetSafeNormal();
        TArray<FVertexInstanceID> Instances;
        for (int32 I=0; I<Points.Num(); ++I) {
            const auto Vertex = Description.CreateVertex(); Positions[Vertex]=Points[I];
            const auto VI = Description.CreateVertexInstance(Vertex); Instances.Add(VI);
            Normals[VI]=Normal; Tangents[VI]=Tangent; Signs[VI]=1;
            UVs.Set(VI,0,FVector2f(FVector3f::DotProduct(Points[I],Tangent)/100.f,
                FVector3f::DotProduct(Points[I],FVector3f::CrossProduct(Normal,Tangent))/100.f));
        }
        Description.CreatePolygon(Group,Instances);
    };
    for (int32 Axis=0; Axis<3; ++Axis) for (float Sign : {-1.f,1.f}) {
        const int32 A=(Axis+1)%3,B=(Axis+2)%3;
        TArray<FVector3f> P;
        for (const FVector2f Corner : {FVector2f(-1,-1),FVector2f(1,-1),FVector2f(1,1),FVector2f(-1,1)}) {
            FVector3f V(0); V[Axis]=50*Sign; V[A]=47*Corner.X; V[B]=47*Corner.Y; P.Add(V);
        }
        Face(P);
    }
    for (int32 Axis=0; Axis<3; ++Axis) for (float ASign : {-1.f,1.f}) for (float BSign : {-1.f,1.f}) {
        const int32 A=(Axis+1)%3,B=(Axis+2)%3;
        TArray<FVector3f> P;
        for (int32 I=0; I<4; ++I) {
            FVector3f V(0); V[Axis]=(I<2 ? -47.f : 47.f);
            V[A]=(I==0 || I==3 ? 50.f : 47.f)*ASign; V[B]=(I==0 || I==3 ? 47.f : 50.f)*BSign; P.Add(V);
        }
        Face(P);
    }
    for (float X : {-1.f,1.f}) for (float Y : {-1.f,1.f}) for (float Z : {-1.f,1.f})
        Face({FVector3f(50*X,47*Y,47*Z),FVector3f(47*X,50*Y,47*Z),FVector3f(47*X,47*Y,50*Z)});
    auto* Mesh = Asset<UStaticMesh>(Base+TEXT("SM_BeveledBlock")); Mesh->GetStaticMaterials().Reset();
    Mesh->GetStaticMaterials().Add(FStaticMaterial(Surface,TEXT("Surface")));
    UStaticMesh::FBuildMeshDescriptionsParams Params; Params.bBuildSimpleCollision=false; Params.bFastBuild=false; Params.bAllowCpuAccess=true;
    if (!Mesh->BuildFromMeshDescriptions({&Description},Params)) return nullptr;
    FStaticMeshCompilingManager::Get().FinishCompilation({Mesh});
    const FVector Size = Mesh->GetBounds().BoxExtent*2;
    UE_LOG(LogTemp,Display,TEXT("VISUAL_POLISH_GEOMETRY %s vertices=%d triangles=%d bounds=%.1fx%.1fx%.1f bevel=3.0cm collision=none"),
        *Mesh->GetPathName(),Description.Vertices().Num(),Description.Triangles().Num(),Size.X,Size.Y,Size.Z);
    if (!Size.Equals(FVector(100),.01f) || Description.Triangles().Num()!=44) return nullptr;
    // Verify the actual compiled LOD, not just authored normals (Build recomputes them by default).
    const FStaticMeshRenderData* RenderData=Mesh->GetRenderData();
    if (!RenderData || RenderData->LODResources.Num()!=1) return nullptr;
    const FStaticMeshLODResources& LOD=RenderData->LODResources[0];
    const auto& PositionBuffer=LOD.VertexBuffers.PositionVertexBuffer;
    const auto& NormalBuffer=LOD.VertexBuffers.StaticMeshVertexBuffer;
    int32 InwardVertices=0,InwardTriangles=0;
    for (uint32 I=0; I<PositionBuffer.GetNumVertices(); ++I) {
        const FVector4f N=NormalBuffer.VertexTangentZ(I);
        const float Facing=FVector3f::DotProduct(PositionBuffer.VertexPosition(I),FVector3f(N.X,N.Y,N.Z));
        if (!FMath::IsFinite(Facing) || Facing<=0.f) ++InwardVertices;
    }
    for (uint32 I=0; I<static_cast<uint32>(LOD.GetNumTriangles()*3); I+=3) {
        const FVector3f A=PositionBuffer.VertexPosition(LOD.IndexBuffer.GetIndex(I));
        const FVector3f B=PositionBuffer.VertexPosition(LOD.IndexBuffer.GetIndex(I+1));
        const FVector3f C=PositionBuffer.VertexPosition(LOD.IndexBuffer.GetIndex(I+2));
        const float Facing=FVector3f::DotProduct((A+B+C)/3.f,FVector3f::CrossProduct(C-A,B-A).GetSafeNormal());
        if (!FMath::IsFinite(Facing) || Facing<=0.f) ++InwardTriangles;
    }
    UE_LOG(LogTemp,Display,TEXT("VISUAL_POLISH_GEOMETRY_NORMALS %s rendered_vertices=%u triangles=%d inward_vertices=%d inward_triangles=%d"),
        *Mesh->GetPathName(),PositionBuffer.GetNumVertices(),LOD.GetNumTriangles(),InwardVertices,InwardTriangles);
    if (PositionBuffer.GetNumVertices()==0 || LOD.GetNumTriangles()!=44 || InwardVertices || InwardTriangles) {
        UE_LOG(LogTemp,Error,TEXT("VISUAL_POLISH_GEOMETRY_NORMALS_FAILED %s"),*Mesh->GetPathName()); return nullptr;
    }
    return Mesh;
}
}
UMemoriaVisualPolishAssetsCommandlet::UMemoriaVisualPolishAssetsCommandlet()
{ IsClient=false; IsServer=false; IsEditor=true; LogToConsole=true; }
int32 UMemoriaVisualPolishAssetsCommandlet::Main(const FString& Params)
{
    if (!FApp::CanEverRender()) { UE_LOG(LogTemp,Error,TEXT("VISUAL_POLISH_RENDERING_REQUIRED use -AllowCommandletRendering")); return 1; }
    const bool bRefresh=FParse::Param(*Params,TEXT("Refresh"));
    TArray<FString> Owned={Base+TEXT("M_StoneSurface"),Base+TEXT("SM_BeveledBlock")};
    TArray<UMaterial*> Materials; TArray<UTexture2D*> Textures;
    // Preflight before touching any asset: six specific existing materials and their original colour textures.
    for (const auto& A : Painted) {
        const FString Path=PaintedPath(A,TEXT("M_")); Owned.Add(Path);
        auto* M=Load<UMaterial>(Path); auto* T=Load<UTexture2D>(PaintedPath(A,TEXT("T_")));
        if (!M || !T) { UE_LOG(LogTemp,Error,TEXT("VISUAL_POLISH_MISSING %s or its colour texture"),*Path); return 1; }
        Materials.Add(M); Textures.Add(T);
    }
    for (const FString& Path : Owned) if (!bRefresh && FPackageName::DoesPackageExist(Path)) {
        UE_LOG(LogTemp,Error,TEXT("VISUAL_POLISH_REFUSED %s already exists; use -Refresh for these eight owned packages"),*Path); return 1;
    }
    auto* Stone=StoneSurface();
    for (int32 I=0; I<Materials.Num(); ++I) PolishPainted(Materials[I],Textures[I],Painted[I].Kind);
    Materials.Add(Stone);
    auto* Block=BeveledBlock(Stone); if (!Block) return 1;
    // UE 5.8 PostEditChange prepares shader resources with PrecompileMode::None. A commandlet has
    // no drawn primitives to request their permutations, so explicitly compile the seven full maps.
    for (UMaterial* M : Materials) M->CacheShaders(EMaterialShaderPrecompileMode::Default);
    if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
    // A failed material must not silently save a default-grey fallback. Diagnose every owned material.
    const auto Quality = GetCachedScalabilityCVars().MaterialQualityLevel;
    bool bAllCompiled = true;
    for (UMaterial* M : Materials) {
        const FMaterialResource* R=M->GetMaterialResource(GMaxRHIShaderPlatform,Quality);
        const bool bMap = R && R->GetGameThreadShaderMap();
        const bool bComplete = R && R->IsGameThreadShaderMapComplete();
        const bool bOwner = R && R->GetMaterialInterface()==M;
        UE_LOG(LogTemp,Display,TEXT("VISUAL_POLISH_SHADER_STATE %s resource=%d map=%d complete=%d owner=%d platform=%d quality=%d resource_platform=%d resource_quality=%d errors=%d"),
            *M->GetPathName(),R!=nullptr,bMap,bComplete,bOwner,static_cast<int32>(GMaxRHIShaderPlatform),static_cast<int32>(Quality),
            R ? static_cast<int32>(R->GetShaderPlatform()) : -1,R ? static_cast<int32>(R->GetQualityLevel()) : -1,R ? R->GetCompileErrors().Num() : -1);
        if (R) for (const FString& Error : R->GetCompileErrors()) UE_LOG(LogTemp,Error,TEXT("VISUAL_POLISH_SHADER %s %s"),*M->GetPathName(),*Error);
        if (!R || !R->GetCompileErrors().IsEmpty() || !bMap || !bComplete || !bOwner) {
            UE_LOG(LogTemp,Error,TEXT("VISUAL_POLISH_COMPILE_FAILED %s"),*M->GetPathName()); bAllCompiled=false;
        } else UE_LOG(LogTemp,Display,TEXT("VISUAL_POLISH_COMPILED %s"),*M->GetPathName());
    }
    if (!bAllCompiled) return 1;
    for (UMaterial* M : Materials) if (!Save(M)) return 1;
    if (!Save(Block)) return 1;
    UE_LOG(LogTemp,Display,TEXT("VISUAL_POLISH_COMPLETE materials=7 meshes=1 original_textures_preserved=6"));
    return 0;
}
