#include "MemoriaBattleStageAssetsCommandlet.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Texture2D.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "Materials/MaterialExpressionVertexColor.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "UObject/SavePackage.h"
namespace
{
template<class T> T* Node(UMaterial* M)
{ auto* N = NewObject<T>(M); M->GetExpressionCollection().AddExpression(N); return N; }
UMaterialExpressionScalarParameter* Scalar(UMaterial* M, const TCHAR* Name, float Value)
{ auto* N = Node<UMaterialExpressionScalarParameter>(M); N->ParameterName = Name; N->DefaultValue = Value; return N; }
void Input(UMaterialExpressionCustom* N, const TCHAR* Name, UMaterialExpression* Value, int32 Output = 0)
{ FCustomInput I; I.InputName = Name; I.Input.Connect(Output, Value); N->Inputs.Add(I); }
UMaterialExpressionMultiply* Mul(UMaterial* M, UMaterialExpression* A, int32 AOut, UMaterialExpression* B, int32 BOut)
{ auto* N = Node<UMaterialExpressionMultiply>(M); N->A.Connect(AOut, A); N->B.Connect(BOut, B); return N; }
}
UMemoriaBattleStageAssetsCommandlet::UMemoriaBattleStageAssetsCommandlet()
{ IsClient = false; IsServer = false; IsEditor = true; LogToConsole = true; }
int32 UMemoriaBattleStageAssetsCommandlet::Main(const FString& Params)
{
    const FString Path = TEXT("/Game/Memoria/Presentation/BattleEntry/M_BattlePlate");
    // Additive unless -Force, which replaces the file before anything loads it.
    if (FPackageName::DoesPackageExist(Path))
    {
        if (!FParse::Param(*Params, TEXT("Force"))) { UE_LOG(LogTemp, Display, TEXT("BATTLE_STAGE_KEPT %s"), *Path); return 0; }
        IFileManager::Get().Delete(*FPackageName::LongPackageNameToFilename(Path, FPackageName::GetAssetPackageExtension()), false, true, true);
    }
    auto* M = NewObject<UMaterial>(CreatePackage(*Path), TEXT("M_BattlePlate"), RF_Public | RF_Standalone);
    M->MaterialDomain = MD_UI; M->BlendMode = BLEND_Translucent;
    auto* Plate = Node<UMaterialExpressionTextureSampleParameter2D>(M); Plate->ParameterName = TEXT("Plate");
    Plate->Texture = LoadObject<UTexture2D>(nullptr, TEXT("/Engine/EngineResources/WhiteSquareTexture.WhiteSquareTexture"));
    auto* UV = Node<UMaterialExpressionTextureCoordinate>(M);
    // Region (u0, v0, u1, v1) crops the art like a brush UV region; the blend works in the cropped space.
    auto* Region = Node<UMaterialExpressionVectorParameter>(M); Region->ParameterName = TEXT("Region"); Region->DefaultValue = FLinearColor(0, 0, 1, 1);
    auto* Remap = Node<UMaterialExpressionCustom>(M); Remap->Inputs.Reset(); Remap->OutputType = CMOT_Float2;
    Remap->Code = TEXT("return lerp(R.xy, R.zw, UV);");
    Input(Remap, TEXT("UV"), UV); Input(Remap, TEXT("R"), Region, 5);
    Plate->Coordinates.Connect(0, Remap);
    // battle_stage_blend.gdshader: edge feather, an optional oval, and the 0.45 floor floor-blend.
    auto* Blend = Node<UMaterialExpressionCustom>(M); Blend->Inputs.Reset(); Blend->OutputType = CMOT_Float1;
    Blend->Code = TEXT(R"(
float left = smoothstep(0.0, E, UV.x);
float right = smoothstep(0.0, E, 1.0 - UV.x);
float top = smoothstep(0.0, E * 0.7, UV.y);
float bottom = smoothstep(0.0, E, 1.0 - UV.y);
float edgeMask = left * right * top * bottom;
float floorBlend = 1.0 - smoothstep(L, 1.0, UV.y);
float2 centered = (UV - 0.5) * float2(1.0, 0.92);
float oval = 1.0 - smoothstep(0.78, 1.16, length(centered) * 2.0);
return edgeMask * lerp(1.0, oval, O) * lerp(0.45, 1.0, floorBlend);
)");
    Input(Blend, TEXT("UV"), UV); Input(Blend, TEXT("E"), Scalar(M, TEXT("EdgeSoftness"), .20f));
    Input(Blend, TEXT("O"), Scalar(M, TEXT("OvalMask"), 0.f)); Input(Blend, TEXT("L"), Scalar(M, TEXT("LowerFade"), .92f));
    // plate_modulate may exceed 1 (the enemy is lifted 1.5x), so it is a parameter, not the 8-bit vertex tint.
    auto* Modulate = Node<UMaterialExpressionVectorParameter>(M); Modulate->ParameterName = TEXT("Modulate"); Modulate->DefaultValue = FLinearColor::White;
    auto* Vertex = Node<UMaterialExpressionVertexColor>(M);
    // Outputs: texture sample, vector parameter and vertex color expose RGB at 0 and alpha at 4.
    auto* Rgb = Mul(M, Mul(M, Plate, 0, Modulate, 0), 0, Vertex, 0);
    auto* Alpha = Mul(M, Mul(M, Mul(M, Plate, 4, Blend, 0), 0, Modulate, 4), 0, Vertex, 4);
    M->GetEditorOnlyData()->EmissiveColor.Connect(0, Rgb);
    M->GetEditorOnlyData()->Opacity.Connect(0, Alpha);
    M->PostEditChange();
    FAssetRegistryModule::AssetCreated(M); M->MarkPackageDirty();
    FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone;
    const bool bSaved = UPackage::SavePackage(M->GetOutermost(), M, *FPackageName::LongPackageNameToFilename(Path, FPackageName::GetAssetPackageExtension()), Args);
    UE_LOG(LogTemp, Display, TEXT("BATTLE_STAGE_ASSET %s %s"), *M->GetPathName(), bSaved ? TEXT("SAVED") : TEXT("FAILED"));
    return bSaved ? 0 : 1;
}
