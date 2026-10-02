#include "MemoriaChapterGroundAssetsCommandlet.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Texture2D.h"
#include "Factories/TextureFactory.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionAppendVector.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionTextureObjectParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionWorldPosition.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "TextureCompiler.h"
#include "UObject/SavePackage.h"
namespace
{
const FString Base = TEXT("/Game/Memoria/Presentation/Chapter/");
struct FGroundTexture { const TCHAR* File; const TCHAR* Name; };
const FGroundTexture Textures[] = {{TEXT("belt_soil"), TEXT("T_BeltSoil")}, {TEXT("belt_paved"), TEXT("T_BeltPaved")},
    {TEXT("drift_soil"), TEXT("T_DriftSoil")}, {TEXT("drift_paved"), TEXT("T_DriftPaved")}, {TEXT("belt_dial"), TEXT("T_BeltDial")}};
template<class T> T* Find(const FString& Name) { return LoadObject<T>(nullptr, *(Base + Name + TEXT(".") + Name), nullptr, LOAD_NoWarn | LOAD_Quiet); }
template<class T> T* Node(UMaterial* M) { auto* N = NewObject<T>(M); M->GetExpressionCollection().AddExpression(N); return N; }
bool Save(UObject* Object)
{
    FAssetRegistryModule::AssetCreated(Object); Object->MarkPackageDirty();
    FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone;
    const FString File = FPackageName::LongPackageNameToFilename(Object->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension());
    const bool bSaved = UPackage::SavePackage(Object->GetOutermost(), Object, *File, Args);
    UE_LOG(LogTemp, Display, TEXT("CHAPTER_GROUND_ASSET %s %s"), *Object->GetPathName(), bSaved ? TEXT("SAVED") : TEXT("FAILED"));
    return bSaved;
}
void Input(UMaterialExpressionCustom* N, const TCHAR* Name, UMaterialExpression* Value)
{ FCustomInput I; I.InputName = Name; I.Input.Connect(0, Value); N->Inputs.Add(I); }
// A vector parameter's first output is RGB only; its alpha is appended so the node reads all four.
UMaterialExpression* Vector(UMaterial* M, const TCHAR* Name, const FLinearColor& Value)
{
    auto* N = Node<UMaterialExpressionVectorParameter>(M); N->ParameterName = Name; N->DefaultValue = Value;
    auto* Four = Node<UMaterialExpressionAppendVector>(M); Four->A.Connect(0, N); Four->B.Connect(4, N);
    return Four;
}
UMaterialExpressionTextureObjectParameter* Texture(UMaterial* M, const TCHAR* Name, UTexture* Value, EMaterialSamplerType Type)
{ auto* N = Node<UMaterialExpressionTextureObjectParameter>(M); N->ParameterName = Name; N->Texture = Value; N->SamplerType = Type; return N; }
// The ground, in world space. Rough and Normal leave through the node's additional outputs.
const TCHAR* GroundCode = TEXT(R"(
struct FGround
{
    float Hash(float2 p) { return frac(sin(dot(p, float2(127.1, 311.7))) * 43758.5453); }
    float Noise(float2 p)
    {
        float2 i = floor(p), f = frac(p); f = f * f * (3 - 2 * f);
        return lerp(lerp(Hash(i), Hash(i + float2(1, 0)), f.x), lerp(Hash(i + float2(0, 1)), Hash(i + float2(1, 1)), f.x), f.y);
    }
    float Fbm(float2 p) { return Noise(p) * .5 + Noise(p * 2.03 + 17) * .3 + Noise(p * 4.01 + 41) * .2; }
} G;
float2 w = P.xy;
// The tile mask: R paved, G interior floor, B closeness to a building wall. Its edges are pushed about by noise
// so that the roads end in a worn line and not along the tile grid.
float2 muv = float2(w.x / MapSize.x, -w.y / MapSize.y);
float2 warp = (float2(G.Fbm(w / 85.0), G.Fbm(w / 85.0 + 31.7)) - .5) * float2(.9 / MapSize.z, .9 / MapSize.w);
float4 m = Texture2DSampleLevel(Mask, MaskSampler, muv + warp, 0);
float4 mFlat = Texture2DSampleLevel(Mask, MaskSampler, muv, 0);
float paved = smoothstep(.36, .64, m.r + (G.Noise(w / 21.0) - .5) * .28);
float inside = smoothstep(.4, .6, mFlat.g);
// Soil: two takes of the painted patch, one turned, traded by a slow noise so the repeat does not show.
float2 ua = float2(w.x, -w.y) / Scales.x, ub = float2(w.x, -w.y) / Scales.y;
float3 soilA = Texture2DSample(Soil, SoilSampler, ua).rgb;
float3 soilB = Texture2DSample(Soil, SoilSampler, float2(-ua.y, ua.x) * .83 + .37).rgb;
float3 soil = lerp(soilA, soilB, smoothstep(.38, .62, G.Fbm(w / 240.0)));
float3 stone = Texture2DSample(Paved, PavedSampler, ub).rgb;
// Old paving shows through the soil in patches, which also keeps the soil's repeat from reading.
float worn = smoothstep(.6, .72, G.Fbm(w / 150.0 + 9)) * .75;
float3 c = lerp(soil, stone, max(paved, worn));
// Height from the painting's light and dark, for the low light to catch (soil A and the paving only).
float e = 1.5;
float3 lum = float3(.3, .59, .11);
float hx = lerp(dot(Texture2DSample(Soil, SoilSampler, ua + float2(e, 0) / Scales.x).rgb, lum), dot(Texture2DSample(Paved, PavedSampler, ub + float2(e, 0) / Scales.y).rgb, lum), paved);
float hy = lerp(dot(Texture2DSample(Soil, SoilSampler, ua - float2(0, e) / Scales.x).rgb, lum), dot(Texture2DSample(Paved, PavedSampler, ub - float2(0, e) / Scales.y).rgb, lum), paved);
float h0 = lerp(dot(soilA, lum), dot(stone, lum), paved);
float3 n = normalize(float3((h0 - hx) * Relief, (h0 - hy) * Relief, 1));
// The interior floor: timber planks (FloorMode 0) or the paving, cooled and darkened (FloorMode 1).
float2 q = w / float2(30.0, 170.0); q.y += fmod(abs(floor(q.x)), 2) * .5;
float2 f = frac(q);
float gap = smoothstep(0, .07, min(f.x, 1 - f.x)) * smoothstep(0, .012, min(f.y, 1 - f.y));
float grain = .9 + .1 * sin(w.y * .8 + G.Noise(w / 14.0) * 6);
float3 planks = FloorTint.rgb * (.72 + .4 * G.Hash(floor(q))) * grain * lerp(.3, 1, gap);
float3 floorC = lerp(planks, stone * FloorTint.rgb * 2.2, saturate(Scales.w));
c = lerp(c, floorC, inside);
n = normalize(lerp(n, float3(0, 0, 1), inside * (1 - saturate(Scales.w))));
// Slow tone drift, grime where the walls stand, and the dark beyond the map.
c *= .82 + .36 * G.Fbm(w / 380.0);
c *= 1 - .5 * m.b;
float2 beyond = max(max(-muv, muv - 1), 0) * MapSize.xy;
c *= lerp(1, .55, saturate(length(beyond) / 1400.0));
// Puddles (Scales.z wet): darker, bluer and smooth, less of them on the paving.
float puddle = smoothstep(.6, .68, G.Fbm(w / 75.0 + 5)) * (1 - paved * .65) * (1 - inside) * saturate(Scales.z);
c = lerp(c, c * float3(.82, .86, .98), puddle);
n = normalize(lerp(n, float3(0, 0, 1), puddle));
// The inlay: a painted round set into the ground (InlayRect: centre xy, half size z, on w).
float2 iuv = float2((w.x - InlayRect.x) / (2 * InlayRect.z) + .5, .5 - (w.y - InlayRect.y) / (2 * InlayRect.z));
float4 inlay = Texture2DSample(Inlay, InlaySampler, saturate(iuv));
float on = inlay.a * saturate(InlayRect.w) * step(0, iuv.x) * step(iuv.x, 1) * step(0, iuv.y) * step(iuv.y, 1);
c = lerp(c, inlay.rgb, on);
OutRough = lerp(lerp(.94, .62, saturate(Scales.z)), .3, puddle);
OutNormal = n;
return c * Tint.rgb;
)");
}
UMemoriaChapterGroundAssetsCommandlet::UMemoriaChapterGroundAssetsCommandlet()
{ IsClient = false; IsServer = false; IsEditor = true; LogToConsole = true; }
int32 UMemoriaChapterGroundAssetsCommandlet::Main(const FString& Params)
{
    const FString Art = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("../ArtSource/ChapterGround"));
    TMap<FString, UTexture2D*> Loaded;
    for (const FGroundTexture& Source : Textures)
    {
        UTexture2D* T = Find<UTexture2D>(Source.Name);
        if (!T)
        {
            const FString File = Art / (FString(Source.File) + TEXT(".png"));
            auto* Factory = NewObject<UTextureFactory>(); bool bCancelled = false;
            T = Cast<UTexture2D>(Factory->FactoryCreateFile(UTexture2D::StaticClass(), CreatePackage(*(Base + Source.Name)), Source.Name, RF_Public | RF_Standalone, *File, nullptr, GWarn, bCancelled));
            if (!T || bCancelled) { UE_LOG(LogTemp, Error, TEXT("CHAPTER_GROUND_TEXTURE_FAILED %s"), *File); return 1; }
            T->LODGroup = TEXTUREGROUP_World; T->SRGB = true; T->NeverStream = true; T->Filter = TF_Trilinear;
            T->PostEditChange(); FTextureCompilingManager::Get().FinishCompilation({T});
            if (!Save(T)) return 1;
        }
        else UE_LOG(LogTemp, Display, TEXT("CHAPTER_GROUND_KEPT %s"), Source.Name);
        Loaded.Add(Source.Name, T);
    }
    // The mask parameter's default: a small black linear texture (no paving, no floor, no walls).
    UTexture2D* Blank = Find<UTexture2D>(TEXT("T_ChapterMaskBlank"));
    if (!Blank)
    {
        Blank = NewObject<UTexture2D>(CreatePackage(*(Base + TEXT("T_ChapterMaskBlank"))), TEXT("T_ChapterMaskBlank"), RF_Public | RF_Standalone);
        TArray<uint8> Pixels; Pixels.Init(0, 4 * 4 * 4);
        Blank->Source.Init(4, 4, 1, 1, TSF_BGRA8, Pixels.GetData());
        Blank->SRGB = false; Blank->CompressionSettings = TC_VectorDisplacementmap; Blank->MipGenSettings = TMGS_NoMipmaps; Blank->NeverStream = true;
        Blank->PostEditChange(); FTextureCompilingManager::Get().FinishCompilation({Blank});
        if (!Save(Blank)) return 1;
    }
    if (Find<UMaterial>(TEXT("M_ChapterGround")) && !FParse::Param(*Params, TEXT("Rebuild")))
    { UE_LOG(LogTemp, Display, TEXT("CHAPTER_GROUND_KEPT M_ChapterGround")); return 0; }
    auto* M = NewObject<UMaterial>(CreatePackage(*(Base + TEXT("M_ChapterGround"))), TEXT("M_ChapterGround"), RF_Public | RF_Standalone);
    M->SetShadingModel(MSM_DefaultLit); M->bTangentSpaceNormal = false;
    auto* Ground = Node<UMaterialExpressionCustom>(M);
    Ground->Inputs.Reset(); Ground->OutputType = CMOT_Float3; Ground->Code = GroundCode;
    Input(Ground, TEXT("P"), Node<UMaterialExpressionWorldPosition>(M));
    Input(Ground, TEXT("Soil"), Texture(M, TEXT("Soil"), Loaded[TEXT("T_BeltSoil")], SAMPLERTYPE_Color));
    Input(Ground, TEXT("Paved"), Texture(M, TEXT("Paved"), Loaded[TEXT("T_BeltPaved")], SAMPLERTYPE_Color));
    Input(Ground, TEXT("Mask"), Texture(M, TEXT("Mask"), Blank, SAMPLERTYPE_LinearColor));
    Input(Ground, TEXT("Inlay"), Texture(M, TEXT("Inlay"), Loaded[TEXT("T_BeltDial")], SAMPLERTYPE_Color));
    // MapSize: the map in world units (xy) and in tiles (zw). Scales: soil repeat, paving repeat, wet, floor mode.
    Input(Ground, TEXT("MapSize"), Vector(M, TEXT("MapSize"), FLinearColor(2400, 1728, 25, 18)));
    Input(Ground, TEXT("Scales"), Vector(M, TEXT("Scales"), FLinearColor(300, 300, 0, 0)));
    Input(Ground, TEXT("Tint"), Vector(M, TEXT("Tint"), FLinearColor(1.5f, 1.5f, 1.5f)));
    Input(Ground, TEXT("FloorTint"), Vector(M, TEXT("FloorTint"), FLinearColor(.2f, .13f, .08f)));
    Input(Ground, TEXT("InlayRect"), Vector(M, TEXT("InlayRect"), FLinearColor(0, 0, 100, 0)));
    auto* Relief = Node<UMaterialExpressionScalarParameter>(M); Relief->ParameterName = TEXT("Relief"); Relief->DefaultValue = 3.f;
    Input(Ground, TEXT("Relief"), Relief);
    FCustomOutput Rough; Rough.OutputName = TEXT("OutRough"); Rough.OutputType = CMOT_Float1;
    FCustomOutput Normal; Normal.OutputName = TEXT("OutNormal"); Normal.OutputType = CMOT_Float3;
    Ground->AdditionalOutputs = {Rough, Normal}; Ground->RebuildOutputs();
    auto* Specular = Node<UMaterialExpressionConstant>(M); Specular->R = .28f;
    M->GetEditorOnlyData()->BaseColor.Connect(0, Ground);
    M->GetEditorOnlyData()->Roughness.Connect(1, Ground);
    M->GetEditorOnlyData()->Normal.Connect(2, Ground);
    M->GetEditorOnlyData()->Specular.Connect(0, Specular);
    M->PostEditChange();
    return Save(M) ? 0 : 1;
}
