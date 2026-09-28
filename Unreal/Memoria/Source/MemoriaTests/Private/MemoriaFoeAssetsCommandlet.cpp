#include "MemoriaFoeAssetsCommandlet.h"
#include "MemoriaRetargetKit.h"
#include "Presentation/MemoriaCombatClips.h"
#include "Animation/AnimSequence.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionFresnel.h"
#include "Materials/MaterialExpressionPreSkinnedPosition.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionTime.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionVertexInterpolator.h"
#include "RetargetEditor/IKRetargetBatchOperation.h"
using namespace MemoriaRetargetKit;
namespace
{
template<class T> T* Node(UMaterial* M)
{ auto* N = NewObject<T>(M); M->GetExpressionCollection().AddExpression(N); return N; }
UMaterialExpressionScalarParameter* Scalar(UMaterial* M, const TCHAR* Name, float Value)
{ auto* N = Node<UMaterialExpressionScalarParameter>(M); N->ParameterName = Name; N->DefaultValue = Value; return N; }
UMaterialExpressionVectorParameter* Vector(UMaterial* M, const TCHAR* Name, const FLinearColor& Value)
{ auto* N = Node<UMaterialExpressionVectorParameter>(M); N->ParameterName = Name; N->DefaultValue = Value; return N; }
void Input(UMaterialExpressionCustom* N, const TCHAR* Name, UMaterialExpression* Value, int32 Output = 0)
{ FCustomInput I; I.InputName = Name; I.Input.Connect(Output, Value); N->Inputs.Add(I); }
bool MakeMaterial()
{
    const FString Path = FString(CombatRoot) + TEXT("M_FieldFoe");
    if (Load<UMaterial>(Path)) { UE_LOG(LogTemp, Display, TEXT("FOE_KEPT %s"), *Path); return true; }
    auto* M = NewObject<UMaterial>(CreatePackage(*Path), TEXT("M_FieldFoe"), RF_Public | RF_Standalone);
    M->bUsedWithSkeletalMesh = true;
    auto* Color = Vector(M, TEXT("Color"), FLinearColor(.035f, .025f, .05f));
    M->GetEditorOnlyData()->BaseColor.Connect(0, Color);
    M->GetEditorOnlyData()->Roughness.Connect(0, Scalar(M, TEXT("Roughness"), .82f));
    auto* Fresnel = Node<UMaterialExpressionFresnel>(M); Fresnel->Exponent = 3.f;
    // Cracks in the reference-pose position, so they ride on the skin instead of sliding through it, and a
    // slow pulse; the rim keeps the dark body readable against the night market.
    auto* Glow = Node<UMaterialExpressionCustom>(M); Glow->Inputs.Reset(); Glow->OutputType = CMOT_Float3;
    Glow->Code = TEXT(R"(
float3 p = P * 0.045;
float a = sin(p.x * 2.1 + sin(p.z * 1.3) * 2.0);
float b = sin(p.z * 2.6 + sin(p.y * 1.7) * 2.0);
float c = sin(p.y * 2.3 + sin(p.x * 1.1) * 2.0);
float crack = 1.0 - smoothstep(0.02, 0.10, abs(a * b + 0.35 * c));
float pulse = 0.72 + 0.28 * sin(T * 2.2 + p.z * 1.5);
return G.rgb * (crack * pulse * K + F * R) + HC.rgb * H;
)");
    // The pre-skinned position exists only in the vertex shader; interpolate it to the pixels.
    auto* Rest = Node<UMaterialExpressionVertexInterpolator>(M); Rest->Input.Connect(0, Node<UMaterialExpressionPreSkinnedPosition>(M));
    Input(Glow, TEXT("P"), Rest);
    Input(Glow, TEXT("T"), Node<UMaterialExpressionTime>(M));
    Input(Glow, TEXT("F"), Fresnel);
    Input(Glow, TEXT("G"), Vector(M, TEXT("Glow"), FLinearColor(.55f, .18f, 1.f)));
    Input(Glow, TEXT("K"), Scalar(M, TEXT("CrackStrength"), 6.f));
    Input(Glow, TEXT("R"), Scalar(M, TEXT("RimStrength"), .8f));
    Input(Glow, TEXT("H"), Scalar(M, TEXT("Hit"), 0.f));
    Input(Glow, TEXT("HC"), Vector(M, TEXT("HitColor"), FLinearColor(1.f, .75f, .5f)));
    M->GetEditorOnlyData()->EmissiveColor.Connect(0, Glow);
    M->PostEditChange();
    return Save(M);
}
}
UMemoriaFoeAssetsCommandlet::UMemoriaFoeAssetsCommandlet()
{ IsClient = false; IsServer = false; IsEditor = true; LogToConsole = true; }
int32 UMemoriaFoeAssetsCommandlet::Main(const FString& Params)
{
    if (!MakeMaterial()) return 1;
    const auto& Names = MemoriaCombatClips::FoeClips();
    if (Load<UAnimSequence>(MemoriaCombatClips::ClipPath(TEXT("Mannequin"), Names[0]))) { UE_LOG(LogTemp, Display, TEXT("FOE_CLIPS_KEPT")); return 0; }
    IAssetRegistry::GetChecked().SearchAllAssets(true);
    TArray<FAssetData> Assets; IAssetRegistry::GetChecked().GetAssetsByPath(FName(TEXT("/Game/Memoria/Imported/UAL2")), Assets, true);
    USkeletalMesh* Source = nullptr; TArray<FAssetData> Clips; FString Prefix;
    for (const FAssetData& Asset : Assets) if (Asset.IsInstanceOf(USkeletalMesh::StaticClass())) Source = Cast<USkeletalMesh>(Asset.GetAsset());
    for (const TCHAR* Name : Names)
    {
        const FAssetData* Found = Assets.FindByPredicate([&](const FAssetData& A) { return A.IsInstanceOf(UAnimSequence::StaticClass()) && A.AssetName.ToString().EndsWith(Name); });
        if (!Found) { UE_LOG(LogTemp, Error, TEXT("FOE_CLIP_MISSING %s (run -run=MemoriaSwordRetarget first)"), Name); return 1; }
        Clips.Add(*Found); Prefix = Found->AssetName.ToString().LeftChop(FCString::Strlen(Name));
    }
    auto* Target = Load<USkeletalMesh>(MemoriaCombatClips::MannequinMesh());
    if (!Source || !Target) { UE_LOG(LogTemp, Error, TEXT("FOE_MESH_MISSING source=%d mannequin=%d"), Source != nullptr, Target != nullptr); return 1; }
    FAutoCharacterizeResults SourceResults, TargetResults;
    UIKRigDefinition* SourceRig = MakeRig(TEXT("IK_UAL2_Foes"), Source, SourceResults);
    UIKRigDefinition* TargetRig = MakeRig(TEXT("IK_Mannequin_Foes"), Target, TargetResults);
    if (!SourceRig || !TargetRig) return 1;
    UIKRetargeter* Retargeter = MakeRetargeter(TEXT("RTG_UAL2_Mannequin"), SourceRig, SourceResults, TargetRig, TargetResults);
    if (!Save(Retargeter)) return 1;
    FIKRetargetBatchOperationInputs Inputs;
    Inputs.AssetsToRetarget = Clips; Inputs.SourceMesh = Source; Inputs.TargetMesh = Target; Inputs.IKRetargetAsset = Retargeter;
    Inputs.TargetPath = TEXT("/Game/Memoria/Presentation/Combat/Foes");
    Inputs.Search = Prefix; Inputs.Replace = TEXT(""); Inputs.Prefix = TEXT("A_Mannequin_");
    Inputs.bIncludeReferencedAssets = false; Inputs.bOverwriteExistingFiles = true;
    int32 Made = 0;
    for (const FAssetData& Asset : UIKRetargetBatchOperation::RunBatchRetarget(Inputs))
        if (UObject* Object = Asset.GetAsset()) { if (!Save(Object)) return 1; ++Made; }
    UE_LOG(LogTemp, Display, TEXT("FOE_RETARGET_DONE %d"), Made);
    return Made == Clips.Num() ? 0 : 1;
}
