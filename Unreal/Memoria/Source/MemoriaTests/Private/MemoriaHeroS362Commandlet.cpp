#include "MemoriaHeroS362Commandlet.h"
#include "MemoriaRetargetKit.h"
#include "Presentation/MemoriaHeroFinish.h"
#include "Factories/FbxFactory.h"
#include "Factories/FbxImportUI.h"
#include "Factories/FbxSkeletalMeshImportData.h"
#include "AssetImportTask.h"
#include "Animation/MorphTarget.h"
#include "Animation/Skeleton.h"
#include "Engine/SkeletalMesh.h"
#include "SkinnedAssetCompiler.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Rendering/SkinWeightVertexBuffer.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionVertexColor.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "ShaderCompiler.h"
#include "MaterialShared.h"
#include "RHI.h"
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "Misc/Parse.h"

using namespace MemoriaRetargetKit;
namespace
{
FString Root;

template<class T> T* Node(UMaterial* Material)
{
    auto* Result = NewObject<T>(Material);
    Material->GetExpressionCollection().AddExpression(Result);
    return Result;
}

UMaterialExpressionCustom* Custom(UMaterial* Material, const FString& Code,
    ECustomMaterialOutputType Type, std::initializer_list<TPair<FName, UMaterialExpression*>> Inputs)
{
    auto* Result = Node<UMaterialExpressionCustom>(Material);
    Result->Code = Code; Result->OutputType = Type;
    for (const auto& Pair : Inputs)
    {
        FCustomInput Input; Input.InputName = Pair.Key; Input.Input.Connect(0, Pair.Value);
        Result->Inputs.Add(Input);
    }
    return Result;
}

UMaterial* Surface(bool bFace)
{
    const FString Name = bFace ? TEXT("M_EliaSkin") : TEXT("M_HeroFinish");
    const FString Path = Root + Name;
    if (FPackageName::DoesPackageExist(Path)) return nullptr;
    auto* Material = NewObject<UMaterial>(CreatePackage(*Path), *Name, RF_Public | RF_Standalone);
    Material->SetShadingModel(MSM_DefaultLit);
    Material->SetUsageByFlag(MATUSAGE_SkeletalMesh, true); Material->TwoSided = true;
    // Compile the morph vertex-factory variant during immutable authoring,
    // rather than discover/compile it during the first short live blink.
    Material->SetUsageByFlag(MATUSAGE_MorphTargets, true);
    auto* Color = Node<UMaterialExpressionVertexColor>(Material);
    auto* UV = Node<UMaterialExpressionTextureCoordinate>(Material);
    auto* Alpha = Node<UMaterialExpressionComponentMask>(Material);
    Alpha->Input.Connect(4, Color); Alpha->R = true;
    auto* Base = Custom(Material,
        bFace ? TEXT("float wet=step(.03,A)*(1-step(.09,A));float pore=sin(UV.x*701)*sin(UV.y*673)*.0015;return saturate(C.rgb*(1+pore*(1-wet)));")
              : TEXT("float hair=step(.1,A)*(1-step(.35,A));float flow=sin(UV.y*173+UV.x*23)*.006;return saturate(C.rgb*(1+flow*hair));"),
        CMOT_Float3, {{TEXT("C"), Color}, {TEXT("UV"), UV}, {TEXT("A"), Alpha}});
    auto* Rough = Custom(Material,
        bFace ? TEXT("return A>.03 && A<.09?.24:.59;")
              : TEXT("return A<.1?.59:(A<.35?.43:(A<.65?.82:(A<.9?.32:.18)));"),
        CMOT_Float1, {{TEXT("A"), Alpha}});
    auto* Specular = Custom(Material,
        bFace ? TEXT("return A>.03 && A<.09?.44:.25;")
              : TEXT("return A<.1?.25:(A<.35?.30:(A<.65?.22:.48));"),
        CMOT_Float1, {{TEXT("A"), Alpha}});
    Material->GetEditorOnlyData()->BaseColor.Connect(0, Base);
    Material->GetEditorOnlyData()->Roughness.Connect(0, Rough);
    Material->GetEditorOnlyData()->Specular.Connect(0, Specular);
    Material->PostEditChange();
    if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
    if (!Material->GetUsageByFlag(MATUSAGE_SkeletalMesh) || !Material->GetUsageByFlag(MATUSAGE_MorphTargets)) return nullptr;
    const auto* Resource = Material->GetMaterialResource(GMaxRHIShaderPlatform);
    if (!Resource || !Resource->GetCompileErrors().IsEmpty() || !Save(Material)) return nullptr;
    UE_LOG(LogTemp, Display, TEXT("S362_MATERIAL %s shader_errors=0"), *Name);
    return Material;
}

USkeletalMesh* Import(const FString& File, const FString& Path, USkeleton* Skeleton)
{
    if (FPackageName::DoesPackageExist(Path)) return nullptr;
    auto* Factory = NewObject<UFbxFactory>(); Factory->SetDetectImportTypeOnImport(false);
    auto* UI = Factory->ImportUI.Get();
    UI->MeshTypeToImport = UI->OriginalImportType = FBXIT_SkeletalMesh;
    UI->bAutomatedImportShouldDetectType = false;
    UI->bImportAsSkeletal = true; UI->bImportMesh = true; UI->bImportAnimations = false;
    UI->Skeleton = Skeleton; UI->bCreatePhysicsAsset = false;
    UI->bImportMaterials = false; UI->bImportTextures = false; UI->bOverrideFullName = true;
    auto* Data = UI->SkeletalMeshImportData.Get();
    Data->bConvertScene = true; Data->bForceFrontXAxis = true; Data->bConvertSceneUnit = true;
    Data->ImportRotation = FRotator(0, -90, 0);
    Data->bUpdateSkeletonReferencePose = false; Data->bUseT0AsRefPose = false;
    Data->NormalImportMethod = FBXNIM_ImportNormalsAndTangents;
    Data->VertexColorImportOption = EVertexColorImportOption::Replace;
    // Deliberately legacy FBX import, now with real lid morphs enabled.
    Data->bImportMorphTargets = true;
    auto* Task = NewObject<UAssetImportTask>(); Task->bAutomated = true; Task->Options = UI;
    Factory->SetAssetImportTask(Task);
    bool bCancelled = false;
    auto* Result = Cast<USkeletalMesh>(Factory->ImportObject(USkeletalMesh::StaticClass(),
        CreatePackage(*Path), *FPackageName::GetShortName(Path), RF_Public | RF_Standalone,
        File, nullptr, bCancelled));
    return bCancelled ? nullptr : Result;
}

FString VertexKey(const FSkeletalMeshLODRenderData& LOD, const FSkelMeshRenderSection& Section,
    uint32 Index, bool bWeights)
{
    const auto P = LOD.StaticVertexBuffers.PositionVertexBuffer.VertexPosition(Index);
    const auto UV = LOD.StaticVertexBuffers.StaticMeshVertexBuffer.GetVertexUV(Index, 0);
    FString Key = FString::Printf(TEXT("%d,%d,%d:%d,%d"), FMath::RoundToInt(P.X*1000),
        FMath::RoundToInt(P.Y*1000), FMath::RoundToInt(P.Z*1000),
        FMath::RoundToInt(UV.X*100000), FMath::RoundToInt(UV.Y*100000));
    if (!bWeights) return Key;
    const auto* Skin = LOD.GetSkinWeightVertexBuffer();
    TArray<TPair<int32, uint32>> Rows;
    uint32 Total = 0;
    for (uint32 I = 0; I < Skin->GetMaxBoneInfluences(); ++I)
    {
        const uint32 Weight = Skin->GetBoneWeight(Index, I);
        if (!Weight) continue;
        const uint32 Local = Skin->GetBoneIndex(Index, I);
        if (!Section.BoneMap.IsValidIndex(Local)) return TEXT("INVALID_SKIN_ROW");
        Rows.Emplace(Section.BoneMap[Local], Weight); Total += Weight;
    }
    Rows.Sort([](const auto& A, const auto& B) { return A.Key < B.Key; });
    for (const auto& Row : Rows)
        Key += FString::Printf(TEXT(":%d=%d"), Row.Key,
            FMath::RoundToInt(double(Row.Value) / FMath::Max(Total, 1u) * 10000.));
    return Key;
}

bool CheckPreserved(USkeletalMesh* Mesh, USkeletalMesh* Old, const FString& Name)
{
    if (!Mesh || Mesh->GetSkeleton() != Old->GetSkeleton()) return false;
    const auto& NewRef = Mesh->GetRefSkeleton(); const auto& OldRef = Old->GetRefSkeleton();
    if (NewRef.GetNum() != 77 || OldRef.GetNum() != 77) return false;
    for (int32 I = 0; I < 77; ++I)
        if (NewRef.GetBoneName(I) != OldRef.GetBoneName(I)
            || NewRef.GetParentIndex(I) != OldRef.GetParentIndex(I)
            || !NewRef.GetRefBonePose()[I].Equals(OldRef.GetRefBonePose()[I], .015f)) return false;
    const auto* NewRender = Mesh->GetResourceForRendering(); const auto* OldRender = Old->GetResourceForRendering();
    if (!NewRender || !OldRender || NewRender->LODRenderData.IsEmpty() || OldRender->LODRenderData.IsEmpty()) return false;
    const auto& N = NewRender->LODRenderData[0]; const auto& O = OldRender->LODRenderData[0];
    if (FMath::Abs(Mesh->GetBounds().BoxExtent.Z*2 - Old->GetBounds().BoxExtent.Z*2) > .5f) return false;
    const FBox NewBox = Mesh->GetBounds().GetBox(), OldBox = Old->GetBounds().GetBox();
    for (int32 Axis = 0; Axis < 2; ++Axis)
        if (NewBox.Min[Axis] < OldBox.Min[Axis]-1 || NewBox.Max[Axis] > OldBox.Max[Axis]+1) return false;
    // Elia material2 is still the same face surface; only material3's recorded
    // hair was removed. Arrel keeps every original surface, including S361 trims.
    const int32 Slots = Name == TEXT("Elia") ? 3 : 2;
    TSet<FString> Retained;
    for (const auto& Section : N.RenderSections)
        if (Section.MaterialIndex < Slots)
            for (uint32 I = Section.BaseVertexIndex; I < Section.BaseVertexIndex+Section.NumVertices; ++I)
                Retained.Add(VertexKey(N, Section, I, true));
    int32 Count = 0, Matches = 0;
    for (const auto& Section : O.RenderSections)
        if (Section.MaterialIndex < Slots)
            for (uint32 I = Section.BaseVertexIndex; I < Section.BaseVertexIndex+Section.NumVertices; ++I)
            { ++Count; Matches += Retained.Contains(VertexKey(O, Section, I, true)); }
    if (Count < 1000 || Matches < Count*.999f) return false;
    UE_LOG(LogTemp, Display, TEXT("S362_PRESERVED %s surface_position_UV_skin=%d/%d bones=77 source_slots=%d"),
        *Name, Matches, Count, Slots);
    return true;
}

bool CheckMorphs(USkeletalMesh* Mesh, const FString& Name)
{
    if (Name == TEXT("Arrel")) return Mesh->GetMorphTargets().IsEmpty();
    if (Mesh->GetMorphTargets().Num() != 2 || !FMemoriaHeroFinish::HasDedicatedLids(Mesh)) return false;
    const auto& LOD = Mesh->GetResourceForRendering()->LODRenderData[0];
    const auto& Colors = LOD.StaticVertexBuffers.ColorVertexBuffer;
    if (Colors.GetNumVertices() != LOD.GetNumVertices() || !Colors.GetAllowCPUAccess()) return false;
    TSet<uint32> Dedicated;
    for (const auto& Section : LOD.RenderSections)
        if (Section.MaterialIndex == 4)
            for (uint32 I = Section.BaseVertexIndex; I < Section.BaseVertexIndex+Section.NumVertices; ++I)
                if (Colors.VertexColor(I).A < 5) Dedicated.Add(I);
    for (const FName MorphName : {FMemoriaHeroFinish::BlinkLeft, FMemoriaHeroFinish::BlinkRight})
    {
        const auto* Morph = Mesh->FindMorphTarget(MorphName);
        const auto Deltas = Morph->GetMorphTargetDeltas(0);
        if (Deltas.Num() < 100 || Deltas.Num() > 1000) return false;
        FBox Open(ForceInit), Closed(ForceInit); int32 Down = 0;
        for (const auto& Delta : Deltas)
        {
            if (!Dedicated.Contains(Delta.SourceIdx) || Delta.PositionDelta.ContainsNaN()
                || Delta.PositionDelta.Size() > 3.f) return false;
            const FVector P(LOD.StaticVertexBuffers.PositionVertexBuffer.VertexPosition(Delta.SourceIdx));
            Open += P; Closed += P + FVector(Delta.PositionDelta);
            Down += Delta.PositionDelta.Z < -.005f;
        }
        if (Open.GetSize().GetMax() > 4.f || Down < Deltas.Num()*.60f
            || Closed.GetSize().Z < Open.GetSize().Z*1.6f) return false;
        UE_LOG(LogTemp, Display, TEXT("S362_LID_MORPH %s vertices=%d downward=%d open=%s closed=%s dedicated_only=1 eye_vertices_moved=0"),
            *MorphName.ToString(), Deltas.Num(), Down, *Open.GetSize().ToString(), *Closed.GetSize().ToString());
    }
    return true;
}
}

UMemoriaHeroS362Commandlet::UMemoriaHeroS362Commandlet()
{ IsClient = false; IsServer = false; IsEditor = true; LogToConsole = true; }

int32 UMemoriaHeroS362Commandlet::Main(const FString& Params)
{
    if (!FApp::CanEverRender()) return 1;
    FString Revision = TEXT("R1"); FParse::Value(*Params, TEXT("Revision="), Revision);
    if (Revision.Len() < 2 || Revision[0] != TCHAR('R') || !Revision.Mid(1).IsNumeric()) return 1;
    Root = TEXT("/Game/Memoria/Presentation/Field3D/HeroFinishS362/") + Revision + TEXT("/");
    const FString Art = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()/TEXT("../ArtSource/HeroFinishS362")/Revision);
    for (const TCHAR* Name : {TEXT("Arrel"), TEXT("Elia")})
        if (FPackageName::DoesPackageExist(Root+Name+TEXT("/SK_")+Name)
            || !IFileManager::Get().FileExists(*(Art/Name/(FString(Name)+TEXT(".fbx"))))) return 1;
    if (FPackageName::DoesPackageExist(Root+TEXT("M_HeroFinish"))
        || FPackageName::DoesPackageExist(Root+TEXT("M_EliaSkin"))) return 1;
    auto* Finish = Surface(false); auto* Face = Surface(true);
    if (!Finish || !Face) return 1;
    int32 Count = 0;
    for (const TCHAR* N : {TEXT("Arrel"), TEXT("Elia")})
    {
        const FString Name = N;
        auto* Old = Load<USkeletalMesh>(TEXT("/Game/Memoria/Presentation/Field3D/HeroPolishS361/R4/")+Name+TEXT("/SK_")+Name);
        if (!Old || !Old->GetSkeleton()) return 1;
        const FReferenceSkeleton OriginalRig = Old->GetSkeleton()->GetReferenceSkeleton();
        auto* Mesh = Import(Art/Name/(Name+TEXT(".fbx")), Root+Name+TEXT("/SK_")+Name, Old->GetSkeleton());
        if (!Mesh) return 1;
        const auto& After = Old->GetSkeleton()->GetReferenceSkeleton();
        if (After.GetNum() != OriginalRig.GetNum()) return 1;
        for (int32 I = 0; I < OriginalRig.GetNum(); ++I)
            if (After.GetBoneName(I) != OriginalRig.GetBoneName(I)
                || After.GetParentIndex(I) != OriginalRig.GetParentIndex(I)
                || !After.GetRefBonePose()[I].Equals(OriginalRig.GetRefBonePose()[I], .00001f)) return 1;
        FSkinnedAssetCompilingManager::Get().FinishCompilation({Mesh});
        const int32 AddedSlots = Name == TEXT("Elia") ? 1 : 0;
        if (Mesh->GetMaterials().Num() != Old->GetMaterials().Num()+AddedSlots) return 1;
        for (int32 I = 0; I < Old->GetMaterials().Num(); ++I)
            Mesh->GetMaterials()[I].MaterialInterface = Old->GetMaterials()[I].MaterialInterface.Get();
        if (Name == TEXT("Elia"))
        {
            Mesh->GetMaterials()[2].MaterialInterface = Face;
            Mesh->GetMaterials().Last().MaterialInterface = Finish;
        }
        Mesh->PostEditChange(); FSkinnedAssetCompilingManager::Get().FinishCompilation({Mesh});
        if (!CheckPreserved(Mesh, Old, Name) || !CheckMorphs(Mesh, Name) || !Save(Mesh)) return 1;
        ++Count;
    }
    UE_LOG(LogTemp, Display, TEXT("S362_IMPORT_COMPLETE meshes=%d materials=2 skeletons=0 clips=0 textures=0 morphs=2"), Count);
    return Count == 2 ? 0 : 1;
}
