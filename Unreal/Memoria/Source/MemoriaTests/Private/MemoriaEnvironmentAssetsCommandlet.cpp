#include "MemoriaEnvironmentAssetsCommandlet.h"
#include "AssetImportTask.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Factories/FbxFactory.h"
#include "Factories/FbxImportUI.h"
#include "Factories/FbxStaticMeshImportData.h"
#include "Factories/TextureFactory.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionAppendVector.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionMaterialFunctionCall.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionTextureObjectParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionWorldPosition.h"
#include "Materials/MaterialFunctionInterface.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "StaticMeshCompiler.h"
#include "TextureCompiler.h"
#include "UObject/SavePackage.h"
namespace
{
const FString Base = TEXT("/Game/Memoria/Presentation/Environment/");
// The delivered size of each model in cm (models/S343_ENVIRONMENT_MANIFEST.md): the import is refused if it differs.
struct FEnvModel { const TCHAR* Source; const TCHAR* Name; FVector Size; };
const FEnvModel Models[] = {
    {TEXT("env_signal_post"), TEXT("SignalPost"), FVector(110, 60, 330)}, {TEXT("env_platform_shelter"), TEXT("PlatformShelter"), FVector(380, 260, 260)},
    {TEXT("env_crate_stack"), TEXT("CrateStack"), FVector(200, 160, 150)}, {TEXT("env_fence_chain"), TEXT("FenceChain"), FVector(192, 30, 90)},
    {TEXT("env_lantern_post"), TEXT("LanternPost"), FVector(40, 40, 210)}, {TEXT("env_rail_track"), TEXT("RailTrack"), FVector(192, 150, 20)},
    {TEXT("env_tarp_canopy"), TEXT("TarpCanopy"), FVector(380, 280, 220)}, {TEXT("env_ruin_wall"), TEXT("RuinWall"), FVector(192, 96, 150)},
    {TEXT("env_gramophone"), TEXT("Gramophone"), FVector(90, 70, 110)}, {TEXT("env_dead_tree"), TEXT("DeadTree"), FVector(250, 250, 420)},
    {TEXT("env_banner_pole"), TEXT("BannerPole"), FVector(80, 40, 320)}, {TEXT("env_dry_grass"), TEXT("DryGrass"), FVector(45, 45, 40)}};
// The atlases. Several models were delivered with byte-identical colour files (one palette for the Belt's six,
// one for Drift's three, one for the lantern and the gramophone); each is imported once, from the file named here.
struct FEnvTexture { const TCHAR* File; const TCHAR* Name; bool bMask; };
const FEnvTexture Textures[] = {
    {TEXT("env_signal_post_basecolor"), TEXT("T_EnvBelt"), false}, {TEXT("env_tarp_canopy_basecolor"), TEXT("T_EnvDrift"), false},
    {TEXT("env_lantern_post_basecolor"), TEXT("T_EnvSmall"), false}, {TEXT("env_dry_grass_basecolor"), TEXT("T_EnvGrass"), false},
    {TEXT("env_signal_post_emissive"), TEXT("T_EnvBeltGlow"), true}, {TEXT("env_lantern_post_emissive"), TEXT("T_EnvSmallGlow"), true}};
template<class T> T* Find(const FString& Name) { return LoadObject<T>(nullptr, *(Base + Name + TEXT(".") + Name), nullptr, LOAD_NoWarn | LOAD_Quiet); }
template<class T> T* Node(UMaterial* M) { auto* N = NewObject<T>(M); M->GetExpressionCollection().AddExpression(N); return N; }
bool Save(UObject* Object)
{
    FAssetRegistryModule::AssetCreated(Object); Object->MarkPackageDirty();
    FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone;
    const FString File = FPackageName::LongPackageNameToFilename(Object->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension());
    const bool bSaved = UPackage::SavePackage(Object->GetOutermost(), Object, *File, Args);
    UE_LOG(LogTemp, Display, TEXT("ENV_ASSET %s %s"), *Object->GetPathName(), bSaved ? TEXT("SAVED") : TEXT("FAILED"));
    return bSaved;
}
void Input(UMaterialExpressionCustom* N, const TCHAR* Name, UMaterialExpression* Value, int32 Output = 0)
{ FCustomInput I; I.InputName = Name; I.Input.Connect(Output, Value); N->Inputs.Add(I); }
UMaterialExpression* Vector(UMaterial* M, const TCHAR* Name, const FLinearColor& Value)
{
    auto* N = Node<UMaterialExpressionVectorParameter>(M); N->ParameterName = Name; N->DefaultValue = Value;
    auto* Four = Node<UMaterialExpressionAppendVector>(M); Four->A.Connect(0, N); Four->B.Connect(4, N);
    return Four;
}
UMaterialExpressionScalarParameter* Scalar(UMaterial* M, const TCHAR* Name, float Value)
{ auto* N = Node<UMaterialExpressionScalarParameter>(M); N->ParameterName = Name; N->DefaultValue = Value; return N; }
UMaterialExpressionTextureObjectParameter* Texture(UMaterial* M, const TCHAR* Name, UTexture* Value, EMaterialSamplerType Type)
{ auto* N = Node<UMaterialExpressionTextureObjectParameter>(M); N->ParameterName = Name; N->Texture = Value; N->SamplerType = Type; return N; }
// The S335 convention for Codex's models (-Y forward, Z up, cm): convert the scene and its unit, force the front
// to X and turn it back a quarter. No collision is generated: the map's blocks stay one per solid tile.
UStaticMesh* ImportMesh(const FString& File, const FString& Path)
{
    auto* Factory = NewObject<UFbxFactory>(); Factory->SetDetectImportTypeOnImport(false);
    auto* UI = Factory->ImportUI.Get();
    UI->MeshTypeToImport = FBXIT_StaticMesh; UI->OriginalImportType = FBXIT_StaticMesh; UI->bAutomatedImportShouldDetectType = false;
    UI->bImportAsSkeletal = false; UI->bImportMesh = true; UI->bImportAnimations = false; UI->bCreatePhysicsAsset = false;
    UI->bImportMaterials = false; UI->bImportTextures = false; UI->bOverrideFullName = true;
    auto* Data = UI->StaticMeshImportData.Get();
    Data->bConvertScene = true; Data->bForceFrontXAxis = true; Data->bConvertSceneUnit = true; Data->ImportRotation = FRotator(0, -90, 0);
    Data->NormalImportMethod = FBXNIM_ImportNormalsAndTangents; Data->bAutoGenerateCollision = false; Data->bCombineMeshes = true;
    auto* Task = NewObject<UAssetImportTask>(); Task->bAutomated = true; Task->Options = UI; Factory->SetAssetImportTask(Task);
    bool bCancelled = false;
    return Cast<UStaticMesh>(Factory->ImportObject(UStaticMesh::StaticClass(), CreatePackage(*Path), *FPackageName::GetShortName(Path), RF_Public | RF_Standalone, File, nullptr, bCancelled));
}
// The prop's surface: the atlas tinted, a glow through the mask, a little of its own colour given back as fill.
const TCHAR* PropCode = TEXT(R"(
float4 c = Texture2DSample(Color, ColorSampler, UV);
float g = Texture2DSample(Glow, GlowSampler, UV).r;
float3 base = c.rgb * Tint.rgb;
OutEmissive = GlowColor.rgb * GlowColor.a * g + base * Fill;
OutAlpha = c.a;
return base * (1 - saturate(g));
)");
// M_FocusSurface's opening: what stands between the camera and Arrel is cut away round the line of sight.
const TCHAR* FocusCode = TEXT(R"(
float3 ray = Focus.xyz - Eye.xyz;
float t = dot(P - Eye.xyz, ray) / max(dot(ray, ray), 1.0);
float3 delta = P - (Eye.xyz + t * ray);
float2 offset = float2(delta.x, dot(delta, Up.xyz)) / (float2(48, 80) * max(t, 0.2));
float hole = smoothstep(0.68, 1.0, dot(offset, offset));
float alpha = (t > 0.0 && t < 1.0 && P.z > -5.0) ? hole : 1.0;
return lerp(1.0, alpha, saturate(Strength));
)");
}
UMemoriaEnvironmentAssetsCommandlet::UMemoriaEnvironmentAssetsCommandlet()
{ IsClient = false; IsServer = false; IsEditor = true; LogToConsole = true; }
int32 UMemoriaEnvironmentAssetsCommandlet::Main(const FString& Params)
{
    const FString Art = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("../ArtSource/Environment"));
    TMap<FString, UTexture2D*> Loaded;
    for (const FEnvTexture& Source : Textures)
    {
        UTexture2D* T = Find<UTexture2D>(Source.Name);
        if (!T)
        {
            const FString File = Art / (FString(Source.File) + TEXT(".png"));
            auto* Factory = NewObject<UTextureFactory>(); bool bCancelled = false;
            T = Cast<UTexture2D>(Factory->FactoryCreateFile(UTexture2D::StaticClass(), CreatePackage(*(Base + Source.Name)), Source.Name, RF_Public | RF_Standalone, *File, nullptr, GWarn, bCancelled));
            if (!T || bCancelled) { UE_LOG(LogTemp, Error, TEXT("ENV_TEXTURE_FAILED %s"), *File); return 1; }
            // The colour atlases are sRGB; the glow masks are linear.
            T->LODGroup = TEXTUREGROUP_World; T->NeverStream = true; T->Filter = TF_Trilinear;
            T->SRGB = !Source.bMask; if (Source.bMask) T->CompressionSettings = TC_Masks;
            T->PostEditChange(); FTextureCompilingManager::Get().FinishCompilation({T});
            if (!Save(T)) return 1;
        }
        else UE_LOG(LogTemp, Display, TEXT("ENV_KEPT %s"), Source.Name);
        Loaded.Add(Source.Name, T);
    }
    // The glow parameter's default: nothing glows.
    UTexture2D* NoGlow = Find<UTexture2D>(TEXT("T_EnvNoGlow"));
    if (!NoGlow)
    {
        NoGlow = NewObject<UTexture2D>(CreatePackage(*(Base + TEXT("T_EnvNoGlow"))), TEXT("T_EnvNoGlow"), RF_Public | RF_Standalone);
        TArray<uint8> Pixels; Pixels.Init(0, 4 * 4 * 4);
        NoGlow->Source.Init(4, 4, 1, 1, TSF_BGRA8, Pixels.GetData());
        NoGlow->SRGB = false; NoGlow->CompressionSettings = TC_Masks; NoGlow->MipGenSettings = TMGS_NoMipmaps; NoGlow->NeverStream = true;
        NoGlow->PostEditChange(); FTextureCompilingManager::Get().FinishCompilation({NoGlow});
        if (!Save(NoGlow)) return 1;
    }
    UMaterial* M = Find<UMaterial>(TEXT("M_EnvProp"));
    if (!M || FParse::Param(*Params, TEXT("Rebuild")))
    {
        auto* Dither = LoadObject<UMaterialFunctionInterface>(nullptr, TEXT("/Engine/Functions/Engine_MaterialFunctions02/Utility/DitherTemporalAA.DitherTemporalAA"));
        if (!Dither) { UE_LOG(LogTemp, Error, TEXT("ENV_DITHER_MISSING")); return 1; }
        M = NewObject<UMaterial>(CreatePackage(*(Base + TEXT("M_EnvProp"))), TEXT("M_EnvProp"), RF_Public | RF_Standalone);
        M->SetShadingModel(MSM_DefaultLit); M->BlendMode = BLEND_Masked; M->OpacityMaskClipValue = .333f; M->TwoSided = true; M->bUsedWithInstancedStaticMeshes = true;
        auto* Prop = Node<UMaterialExpressionCustom>(M);
        Prop->Inputs.Reset(); Prop->OutputType = CMOT_Float3; Prop->Code = PropCode;
        Input(Prop, TEXT("UV"), Node<UMaterialExpressionTextureCoordinate>(M));
        Input(Prop, TEXT("Color"), Texture(M, TEXT("Color"), Loaded[TEXT("T_EnvBelt")], SAMPLERTYPE_Color));
        Input(Prop, TEXT("Glow"), Texture(M, TEXT("Glow"), NoGlow, SAMPLERTYPE_Masks));
        Input(Prop, TEXT("Tint"), Vector(M, TEXT("Tint"), FLinearColor::White));
        // GlowColor: the lamp's colour (rgb) and its strength (a).
        Input(Prop, TEXT("GlowColor"), Vector(M, TEXT("GlowColor"), FLinearColor(1.f, .5f, .16f, 14.f)));
        Input(Prop, TEXT("Fill"), Scalar(M, TEXT("Fill"), .04f));
        FCustomOutput Emissive; Emissive.OutputName = TEXT("OutEmissive"); Emissive.OutputType = CMOT_Float3;
        FCustomOutput Alpha; Alpha.OutputName = TEXT("OutAlpha"); Alpha.OutputType = CMOT_Float1;
        Prop->AdditionalOutputs = {Emissive, Alpha}; Prop->RebuildOutputs();
        auto* Mask = Node<UMaterialExpressionCustom>(M);
        Mask->Inputs.Reset(); Mask->OutputType = CMOT_Float1; Mask->Code = FocusCode;
        Input(Mask, TEXT("P"), Node<UMaterialExpressionWorldPosition>(M));
        Input(Mask, TEXT("Eye"), Vector(M, TEXT("OcclusionEye"), FLinearColor::Black));
        Input(Mask, TEXT("Focus"), Vector(M, TEXT("OcclusionFocus"), FLinearColor::Black));
        Input(Mask, TEXT("Up"), Vector(M, TEXT("OcclusionUp"), FLinearColor::Black));
        Input(Mask, TEXT("Strength"), Scalar(M, TEXT("FocusStrength"), 1.f));
        auto* Fade = Node<UMaterialExpressionMaterialFunctionCall>(M);
        if (!Fade->SetMaterialFunction(Dither)) return 1;
        bool bLinked = false;
        for (int32 I = 0; I < Fade->FunctionInputs.Num(); ++I)
            if (Fade->GetInputName(I).ToString().Contains(TEXT("Alpha"))) { Fade->FunctionInputs[I].Input.Connect(0, Mask); bLinked = true; }
        if (!bLinked) { UE_LOG(LogTemp, Error, TEXT("ENV_DITHER_INPUT_MISSING")); return 1; }
        auto* Opacity = Node<UMaterialExpressionMultiply>(M); Opacity->A.Connect(0, Fade); Opacity->B.Connect(2, Prop);
        auto* Specular = Node<UMaterialExpressionConstant>(M); Specular->R = .25f;
        M->GetEditorOnlyData()->BaseColor.Connect(0, Prop);
        M->GetEditorOnlyData()->EmissiveColor.Connect(1, Prop);
        M->GetEditorOnlyData()->OpacityMask.Connect(0, Opacity);
        M->GetEditorOnlyData()->Roughness.Connect(0, Scalar(M, TEXT("Roughness"), .88f));
        M->GetEditorOnlyData()->Specular.Connect(0, Specular);
        M->PostEditChange();
        if (!Save(M)) return 1;
    }
    else UE_LOG(LogTemp, Display, TEXT("ENV_KEPT M_EnvProp"));
    for (const FEnvModel& Model : Models)
    {
        const FString Name = FString(TEXT("SM_Env")) + Model.Name;
        UStaticMesh* Mesh = Find<UStaticMesh>(Name);
        if (!Mesh)
        {
            Mesh = ImportMesh(Art / (FString(Model.Source) + TEXT(".fbx")), Base + Name);
            if (!Mesh) { UE_LOG(LogTemp, Error, TEXT("ENV_MODEL_IMPORT_FAILED %s"), Model.Name); return 1; }
            FStaticMeshCompilingManager::Get().FinishCompilation({Mesh});
            if (Mesh->GetStaticMaterials().Num() != 1) { UE_LOG(LogTemp, Error, TEXT("ENV_MODEL_MATERIALS %s %d"), Model.Name, Mesh->GetStaticMaterials().Num()); return 1; }
            Mesh->SetMaterial(0, M);
            if (!Save(Mesh)) return 1;
        }
        else UE_LOG(LogTemp, Display, TEXT("ENV_KEPT %s"), *Name);
        const FBox Box = Mesh->GetBoundingBox();
        const FVector Size = Box.GetSize();
        UE_LOG(LogTemp, Display, TEXT("ENV_MODEL %s size=%.1fx%.1fx%.1f min=(%.1f,%.1f,%.1f) max=(%.1f,%.1f,%.1f) triangles=%d"), Model.Name, Size.X, Size.Y, Size.Z,
            Box.Min.X, Box.Min.Y, Box.Min.Z, Box.Max.X, Box.Max.Y, Box.Max.Z, Mesh->GetNumTriangles(0));
        if (!Size.Equals(Model.Size, 1.0) || FMath::Abs(Box.Min.Z) > 1.0)
        { UE_LOG(LogTemp, Error, TEXT("ENV_MODEL_SIZE %s is not %.0fx%.0fx%.0f standing on the ground"), Model.Name, Model.Size.X, Model.Size.Y, Model.Size.Z); return 1; }
    }
    return 0;
}
