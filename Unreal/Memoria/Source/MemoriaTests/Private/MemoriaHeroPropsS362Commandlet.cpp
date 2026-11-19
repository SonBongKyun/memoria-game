#include "MemoriaHeroPropsS362Commandlet.h"
#include "MemoriaRetargetKit.h"
#include "Factories/FbxFactory.h"
#include "Factories/FbxImportUI.h"
#include "Factories/FbxStaticMeshImportData.h"
#include "AssetImportTask.h"
#include "Engine/StaticMesh.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "StaticMeshResources.h"
#include "StaticMeshCompiler.h"
#include "ShaderCompiler.h"
#include "MaterialShared.h"
#include "RHI.h"
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "Json.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "StaticMeshOperations.h"
#include "EditorFramework/AssetImportData.h"
#include "Misc/AutomationTest.h"
#include "Presentation/MemoriaNativePropFingerprint.h"
#include "UObject/UnrealType.h"

using namespace MemoriaRetargetKit;
namespace
{
FString Root;
FString OutputRevision=TEXT("R5");
TArray<TSharedPtr<FJsonValue>> NativeEvidence;
FString FloatBits(float Value)
{uint32 Bits;FMemory::Memcpy(&Bits,&Value,sizeof(Bits));return FString::Printf(TEXT("%08x"),Bits);}
void AddVector(FString& Key,const FVector3f& P)
{Key+=FloatBits(P.X)+FloatBits(P.Y)+FloatBits(P.Z);}
void AddUV(FString& Key,const FVector2f& P)
{Key+=FloatBits(P.X)+FloatBits(P.Y);}
FString DescriptionCorner(const FMeshDescription& D,FVertexInstanceID VI,int32 UVChannels=INDEX_NONE)
{
    const FStaticMeshConstAttributes A(D);FString K;
    AddVector(K,A.GetVertexPositions()[D.GetVertexInstanceVertex(VI)]);AddVector(K,A.GetVertexInstanceNormals()[VI]);
    AddVector(K,A.GetVertexInstanceTangents()[VI]);K+=FloatBits(A.GetVertexInstanceBinormalSigns()[VI]);
    const auto C=A.GetVertexInstanceColors()[VI];K+=FloatBits(C.X)+FloatBits(C.Y)+FloatBits(C.Z)+FloatBits(C.W);
    const auto UV=A.GetVertexInstanceUVs();const int32 Count=UVChannels==INDEX_NONE?UV.GetNumChannels():UVChannels;
    for(int32 I=0;I<Count;++I)AddUV(K,UV.Get(VI,I));return K;
}
FName SlotName(const UStaticMesh* Mesh,int32 Slot)
{
    const auto& M=Mesh->GetStaticMaterials()[Slot];return M.ImportedMaterialSlotName.IsNone()?M.MaterialSlotName:M.ImportedMaterialSlotName;
}
int32 DescriptionSlot(const UStaticMesh* Mesh,const FMeshDescription& D,FPolygonGroupID Group)
{
    const FName Name=FStaticMeshConstAttributes(D).GetPolygonGroupMaterialSlotNames()[Group];
    for(int32 I=0;I<Mesh->GetStaticMaterials().Num();++I)if(SlotName(Mesh,I)==Name)return I;return INDEX_NONE;
}
bool PreserveDescription(const FMeshDescription& Before,const FMeshDescription& After,const TSet<FPolygonID>& Retained,const FString& Label)
{
    // Exact float-bit comparison of every original retained vertex-instance
    // attribute and triangle, including ALL UV channels. No quantized tolerance.
    if(FStaticMeshConstAttributes(Before).GetVertexInstanceUVs().GetNumChannels()!=FStaticMeshConstAttributes(After).GetVertexInstanceUVs().GetNumChannels())return false;
    int32 Corners=0;
    for(FPolygonID P:Retained)
    {
        if(!After.IsPolygonValid(P))return false;
        const auto BT=Before.GetPolygonTriangles(P),AT=After.GetPolygonTriangles(P);if(BT.Num()!=AT.Num())return false;
        for(int32 T=0;T<BT.Num();++T)
        {
            const auto B=Before.GetTriangleVertexInstances(BT[T]),A=After.GetTriangleVertexInstances(AT[T]);
            for(int32 I=0;I<3;++I){if(DescriptionCorner(Before,B[I])!=DescriptionCorner(After,A[I]))return false;++Corners;}
        }
    }
    auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("check"),TEXT("native_description_exact"));Row->SetStringField(TEXT("label"),Label);
    Row->SetNumberField(TEXT("retained_polygons"),Retained.Num());Row->SetNumberField(TEXT("exact_corners_all_attributes"),Corners);
    Row->SetNumberField(TEXT("UV_channels"),FStaticMeshConstAttributes(Before).GetVertexInstanceUVs().GetNumChannels());NativeEvidence.Add(MakeShared<FJsonValueObject>(Row));
    UE_LOG(LogTemp,Display,TEXT("S362_NATIVE_DESCRIPTION_EXACT %s polygons=%d corners=%d all_UVs_normals_tangents_colors_float_bits=1"),*Label,Retained.Num(),Corners);return Corners>100;
}
FString RenderCorner(const FStaticMeshLODResources& LOD,uint32 V,int32 UVChannels,bool bBasis=true)
{
    FString K;AddVector(K,LOD.VertexBuffers.PositionVertexBuffer.VertexPosition(V));
    const auto& VB=LOD.VertexBuffers.StaticMeshVertexBuffer;
    if(bBasis)
    {
        const auto X=VB.VertexTangentX(V),Z=VB.VertexTangentZ(V);AddVector(K,FVector3f(X));AddVector(K,FVector3f(Z));K+=FloatBits(X.W)+FloatBits(Z.W);
        const int32 Stride=VB.GetTangentSize()/VB.GetNumVertices();
        K+=FString::FromHexBlob(static_cast<const uint8*>(VB.GetTangentData())+V*Stride,Stride);
    }
    const FColor C=LOD.VertexBuffers.ColorVertexBuffer.GetNumVertices()?LOD.VertexBuffers.ColorVertexBuffer.VertexColor(V):FColor::White;
    K+=FString::Printf(TEXT("%08x"),C.DWColor());
    for(int32 I=0;I<UVChannels;++I)AddUV(K,LOD.VertexBuffers.StaticMeshVertexBuffer.GetVertexUV(V,I));return K;
}
void AuditMesh(UStaticMesh* Mesh,const FString& Label)
{
    const auto& LOD=Mesh->GetRenderData()->LODResources[0];auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("check"),TEXT("native_inventory"));
    Row->SetStringField(TEXT("label"),Label);Row->SetStringField(TEXT("mesh"),Mesh->GetPathName());Row->SetNumberField(TEXT("render_triangles"),LOD.GetNumTriangles());
    Row->SetNumberField(TEXT("UV_channels"),LOD.VertexBuffers.StaticMeshVertexBuffer.GetNumTexCoords());Row->SetBoolField(TEXT("nanite_enabled"),Mesh->GetNaniteSettings().bEnabled);
    Row->SetBoolField(TEXT("render_high_precision_tangents"),LOD.VertexBuffers.StaticMeshVertexBuffer.GetUseHighPrecisionTangentBasis());Row->SetBoolField(TEXT("render_full_precision_UVs"),LOD.VertexBuffers.StaticMeshVertexBuffer.GetUseFullPrecisionUVs());
    const auto& Nanite=Mesh->GetNaniteSettings();Row->SetNumberField(TEXT("nanite_fallback_target"),int32(Nanite.FallbackTarget));
    Row->SetNumberField(TEXT("nanite_fallback_percent_triangles"),Nanite.FallbackPercentTriangles);Row->SetNumberField(TEXT("nanite_fallback_relative_error"),Nanite.FallbackRelativeError);
    Row->SetNumberField(TEXT("nanite_position_precision"),Nanite.PositionPrecision);
    const auto& B=Mesh->GetSourceModel(0).BuildSettings;const auto& R=Mesh->GetSourceModel(0).ReductionSettings;
    const auto* Fast=FindFProperty<FBoolProperty>(UStaticMesh::StaticClass(),TEXT("bDoFastBuild"));Row->SetBoolField(TEXT("serialized_fast_build"),Fast && Fast->GetPropertyValue_InContainer(Mesh));
    Row->SetStringField(TEXT("native_LOD0_fingerprint_sha1_v1"),FMemoriaNativePropFingerprint::MeshLOD0(Mesh));
    Row->SetStringField(TEXT("native_slot0_fingerprint_sha1_v1"),FMemoriaNativePropFingerprint::MeshLOD0(Mesh,0));
    if(Label==TEXT("delivered_native_sword"))Row->SetStringField(TEXT("native_slot0_original2UV_fingerprint_sha1_v1"),FMemoriaNativePropFingerprint::MeshLOD0(Mesh,0,2));
    Row->SetBoolField(TEXT("CPU_tangent_data_available"),LOD.VertexBuffers.StaticMeshVertexBuffer.GetTangentData()!=nullptr);
    Row->SetBoolField(TEXT("CPU_position_data_available"),LOD.VertexBuffers.PositionVertexBuffer.GetVertexData()!=nullptr);
    Row->SetBoolField(TEXT("allow_CPU_access"),Mesh->bAllowCPUAccess);Row->SetBoolField(TEXT("generate_lightmap_UVs"),B.bGenerateLightmapUVs);Row->SetStringField(TEXT("build_scale"),B.BuildScale3D.ToString());
    Row->SetBoolField(TEXT("full_precision_UVs"),B.bUseFullPrecisionUVs);Row->SetBoolField(TEXT("F16_truncation"),B.bUseBackwardsCompatibleF16TruncUVs);
    Row->SetBoolField(TEXT("remove_degenerates"),B.bRemoveDegenerates);Row->SetBoolField(TEXT("recompute_normals"),B.bRecomputeNormals);Row->SetBoolField(TEXT("recompute_tangents"),B.bRecomputeTangents);
    Row->SetNumberField(TEXT("reduction_percent_triangles"),R.PercentTriangles);Row->SetNumberField(TEXT("reduction_percent_vertices"),R.PercentVertices);
    TArray<TSharedPtr<FJsonValue>> Sections;
    for(const auto& S:LOD.Sections)
    {auto Q=MakeShared<FJsonObject>();Q->SetNumberField(TEXT("slot"),S.MaterialIndex);Q->SetStringField(TEXT("slot_name"),SlotName(Mesh,S.MaterialIndex).ToString());Q->SetStringField(TEXT("material"),GetPathNameSafe(Mesh->GetStaticMaterials()[S.MaterialIndex].MaterialInterface));Q->SetNumberField(TEXT("triangles"),S.NumTriangles);Sections.Add(MakeShared<FJsonValueObject>(Q));}
    Row->SetArrayField(TEXT("sections"),Sections);FMeshDescription D;if(Mesh->CloneMeshDescription(0,D))Row->SetNumberField(TEXT("description_triangles"),D.Triangles().Num());
    NativeEvidence.Add(MakeShared<FJsonValueObject>(Row));
}
// Installed UE5.8 TArray vertex Init ignores InNumTexCoords and exposes
// all8 storage slots, also on reload/cook. Explicitly zero NEW source copies.
bool PadFastBuildUVs(FMeshDescription& D,int32 OriginalChannels,const FString& Label)
{
    auto UV=FStaticMeshAttributes(D).GetVertexInstanceUVs();
    if(OriginalChannels!=2 || UV.GetNumChannels()!=OriginalChannels)
    {UE_LOG(LogTemp,Error,TEXT("S362_FASTBUILD_UV_SOURCE_REJECT %s expected_original2 actual=%d"),*Label,UV.GetNumChannels());return false;}
    TMap<FVertexInstanceID,FString> OriginalRecords;
    for(FVertexInstanceID V:D.VertexInstances().GetElementIDs())OriginalRecords.Add(V,DescriptionCorner(D,V,OriginalChannels));
    UV.SetNumChannels(MAX_STATIC_TEXCOORDS);
    int32 NonzeroPadding=0;
    for(FVertexInstanceID V:D.VertexInstances().GetElementIDs())
    {
        if(OriginalRecords[V]!=DescriptionCorner(D,V,OriginalChannels))
        {UE_LOG(LogTemp,Error,TEXT("S362_FASTBUILD_UV_SOURCE_REJECT %s original_attributes_changed vertex_instance=%d"),*Label,V.GetValue());return false;}
        for(int32 C=OriginalChannels;C<MAX_STATIC_TEXCOORDS;++C)
        {UV.Set(V,C,FVector2f::ZeroVector);const auto Q=UV.Get(V,C);NonzeroPadding+=FloatBits(Q.X)!=FloatBits(0.f);NonzeroPadding+=FloatBits(Q.Y)!=FloatBits(0.f);}
    }
    if(NonzeroPadding){UE_LOG(LogTemp,Error,TEXT("S362_FASTBUILD_UV_SOURCE_REJECT %s nonzero_padding=%d"),*Label,NonzeroPadding);return false;}
    auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("check"),TEXT("explicit_fastbuild_UV_padding_source"));Row->SetStringField(TEXT("label"),Label);
    Row->SetNumberField(TEXT("original_UV_channels"),OriginalChannels);Row->SetNumberField(TEXT("initialized_UV_channels"),UV.GetNumChannels());
    Row->SetNumberField(TEXT("zero_padding_scalars"),D.VertexInstances().Num()*(MAX_STATIC_TEXCOORDS-OriginalChannels)*2);
    Row->SetNumberField(TEXT("original_attribute_records_bit_exact"),OriginalRecords.Num());Row->SetNumberField(TEXT("nonzero_padding_scalars"),NonzeroPadding);
    Row->SetStringField(TEXT("reason"),TEXT("UE5.8 TArray vertex Init ignores requested channel count and exposes8 slots. Explicit zero source padding persists through FastBuild reload/cook; every original UV remains bit-exact."));NativeEvidence.Add(MakeShared<FJsonValueObject>(Row));
    UE_LOG(LogTemp,Display,TEXT("S362_FASTBUILD_UV_SOURCE_PADDING %s original=%d initialized=%d"),*Label,OriginalChannels,UV.GetNumChannels());return true;
}
bool PreserveRendered(UStaticMesh* Mesh,UStaticMesh* Old,bool bSword)
{
    auto Reject=[&](const TCHAR* Reason)
    {
        auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("check"),TEXT("native_render_reject"));Row->SetStringField(TEXT("reason"),Reason);
        Row->SetStringField(TEXT("mesh"),GetPathNameSafe(Mesh));Row->SetStringField(TEXT("original"),GetPathNameSafe(Old));NativeEvidence.Add(MakeShared<FJsonValueObject>(Row));
        UE_LOG(LogTemp,Error,TEXT("S362_NATIVE_RENDER_REJECT reason=%s mesh=%s original=%s"),Reason,*GetPathNameSafe(Mesh),*GetPathNameSafe(Old));return false;
    };
    if(!Mesh || !Old)return Reject(TEXT("null_mesh"));
    if(!Mesh->GetRenderData() || !Old->GetRenderData())return Reject(TEXT("missing_render_data"));
    if(Mesh->GetRenderData()->LODResources.IsEmpty() || Old->GetRenderData()->LODResources.IsEmpty())return Reject(TEXT("missing_LOD0"));
    const auto& N=Mesh->GetRenderData()->LODResources[0];const auto& O=Old->GetRenderData()->LODResources[0];
    const auto& NV=N.VertexBuffers.StaticMeshVertexBuffer;const auto& OV=O.VertexBuffers.StaticMeshVertexBuffer;
    const int32 UVChannels=OV.GetNumTexCoords(),NewUVChannels=NV.GetNumTexCoords();
    auto State=MakeShared<FJsonObject>();State->SetStringField(TEXT("check"),TEXT("native_render_preflight"));State->SetStringField(TEXT("mesh"),Mesh->GetPathName());
    State->SetNumberField(TEXT("original_UV_channels"),UVChannels);State->SetNumberField(TEXT("new_UV_channels"),NewUVChannels);
    State->SetBoolField(TEXT("new_CPU_tangents"),NV.GetTangentData()!=nullptr);State->SetBoolField(TEXT("original_CPU_tangents"),OV.GetTangentData()!=nullptr);
    State->SetBoolField(TEXT("new_CPU_UVs"),NV.GetTexCoordData()!=nullptr);State->SetBoolField(TEXT("original_CPU_UVs"),OV.GetTexCoordData()!=nullptr);
    State->SetBoolField(TEXT("new_CPU_positions"),N.VertexBuffers.PositionVertexBuffer.GetVertexData()!=nullptr);State->SetBoolField(TEXT("original_CPU_positions"),O.VertexBuffers.PositionVertexBuffer.GetVertexData()!=nullptr);
    State->SetNumberField(TEXT("new_vertices"),NV.GetNumVertices());State->SetNumberField(TEXT("original_vertices"),OV.GetNumVertices());
    State->SetNumberField(TEXT("new_index_count"),N.IndexBuffer.GetNumIndices());State->SetNumberField(TEXT("original_index_count"),O.IndexBuffer.GetNumIndices());NativeEvidence.Add(MakeShared<FJsonValueObject>(State));
    UE_LOG(LogTemp,Display,TEXT("S362_NATIVE_RENDER_PREFLIGHT %s UVs=%d/%d CPU_tangents=%d/%d vertices=%u/%u indices=%d/%d"),*Mesh->GetName(),NewUVChannels,UVChannels,NV.GetTangentData()!=nullptr,OV.GetTangentData()!=nullptr,NV.GetNumVertices(),OV.GetNumVertices(),N.IndexBuffer.GetNumIndices(),O.IndexBuffer.GetNumIndices());
    if(!NV.GetNumVertices() || !OV.GetNumVertices())return Reject(TEXT("empty_vertex_buffer"));
    if(!NV.GetTangentData())return Reject(TEXT("new_CPU_tangents_unavailable"));
    if(!OV.GetTangentData())return Reject(TEXT("original_CPU_tangents_unavailable"));
    if(!NV.GetTexCoordData())return Reject(TEXT("new_CPU_UVs_unavailable"));
    if(!OV.GetTexCoordData())return Reject(TEXT("original_CPU_UVs_unavailable"));
    if(N.VertexBuffers.ColorVertexBuffer.GetNumVertices() && !N.VertexBuffers.ColorVertexBuffer.GetVertexData())return Reject(TEXT("new_CPU_colors_unavailable"));
    if(O.VertexBuffers.ColorVertexBuffer.GetNumVertices() && !O.VertexBuffers.ColorVertexBuffer.GetVertexData())return Reject(TEXT("original_CPU_colors_unavailable"));
    if(!N.VertexBuffers.PositionVertexBuffer.GetVertexData())return Reject(TEXT("new_CPU_positions_unavailable"));
    if(!O.VertexBuffers.PositionVertexBuffer.GetVertexData())return Reject(TEXT("original_CPU_positions_unavailable"));
    if(N.IndexBuffer.GetNumIndices()!=N.GetNumTriangles()*3 || O.IndexBuffer.GetNumIndices()!=O.GetNumTriangles()*3)return Reject(TEXT("index_triangle_inventory_mismatch"));
    if(bSword)
    {
        if(UVChannels!=2 || NewUVChannels!=MAX_STATIC_TEXCOORDS)return Reject(TEXT("unexpected_sword_UV_layout_expected_original2_new8"));
        int32 Nonzero=0,Scalars=0;for(uint32 V=0;V<NV.GetNumVertices();++V)for(int32 C=UVChannels;C<NewUVChannels;++C)
        {const auto Q=NV.GetVertexUV(V,C);Nonzero+=FloatBits(Q.X)!=FloatBits(0.f);Nonzero+=FloatBits(Q.Y)!=FloatBits(0.f);Scalars+=2;}
        auto Padding=MakeShared<FJsonObject>();Padding->SetStringField(TEXT("check"),TEXT("native_fastbuild_zero_UV_padding"));Padding->SetNumberField(TEXT("original_UV_channels_exactly_compared"),UVChannels);
        Padding->SetNumberField(TEXT("render_UV_channels"),NewUVChannels);Padding->SetNumberField(TEXT("padding_scalars_checked"),Scalars);Padding->SetNumberField(TEXT("nonzero_padding_scalars"),Nonzero);NativeEvidence.Add(MakeShared<FJsonValueObject>(Padding));
        UE_LOG(LogTemp,Display,TEXT("S362_NATIVE_UV_PADDING exact_original_channels=%d extra_channels=%d zero_scalars=%d nonzero=%d"),UVChannels,NewUVChannels-UVChannels,Scalars,Nonzero);
        if(Nonzero)return Reject(TEXT("nonzero_sword_padding_UV"));
    }
    else if(NewUVChannels!=UVChannels)return Reject(TEXT("staff_UV_channel_count_mismatch"));
    TMap<FString,int32> Keys;const auto NI=N.IndexBuffer.GetArrayView(),OI=O.IndexBuffer.GetArrayView();
    TMap<FString,TArray<uint32>> Geometry;for(const auto& S:N.Sections)if(!bSword || S.MaterialIndex==0)
        for(uint32 I=S.FirstIndex;I<S.FirstIndex+S.NumTriangles*3;++I)Geometry.FindOrAdd(RenderCorner(N,NI[I],UVChannels,false)).AddUnique(NI[I]);
    auto TriangleKey=[UVChannels](const FStaticMeshLODResources& LOD,const FIndexArrayView& Indices,uint32 I)
    {
        FString C[3];for(int32 J=0;J<3;++J)C[J]=RenderCorner(LOD,Indices[I+J],UVChannels);
        FString Key=C[0]+C[1]+C[2];for(int32 J=1;J<3;++J){const FString Rotated=C[J]+C[(J+1)%3]+C[(J+2)%3];if(Rotated<Key)Key=Rotated;}return Key;
    };
    for(const auto& S:N.Sections)if(!bSword || S.MaterialIndex==0)for(uint32 I=S.FirstIndex;I<S.FirstIndex+S.NumTriangles*3;I+=3)++Keys.FindOrAdd(TriangleKey(N,NI,I));
    int32 Total=0,Kept=0;for(const auto& S:O.Sections)if(!bSword || S.MaterialIndex==0)for(uint32 I=S.FirstIndex;I<S.FirstIndex+S.NumTriangles*3;I+=3)
    {Total+=3;if(int32* Count=Keys.Find(TriangleKey(O,OI,I));Count && *Count>0){--*Count;Kept+=3;}}
    int32 GeometryKept=0;double MaxNormalAngle=0.,MaxTangentAngle=0.;
    for(const auto& S:O.Sections)if(!bSword || S.MaterialIndex==0)for(uint32 I=S.FirstIndex;I<S.FirstIndex+S.NumTriangles*3;++I)
    {
        const auto* Matches=Geometry.Find(RenderCorner(O,OI[I],UVChannels,false));if(!Matches)continue;++GeometryKept;
        const FVector3f OX(O.VertexBuffers.StaticMeshVertexBuffer.VertexTangentX(OI[I])),OZ(O.VertexBuffers.StaticMeshVertexBuffer.VertexTangentZ(OI[I]));
        double Best=DBL_MAX,Normal=0.,Tangent=0.;for(uint32 V:*Matches)
        {
            const FVector3f NX(N.VertexBuffers.StaticMeshVertexBuffer.VertexTangentX(V)),NZ(N.VertexBuffers.StaticMeshVertexBuffer.VertexTangentZ(V));
            const double Score=FVector3f::DistSquared(OX,NX)+FVector3f::DistSquared(OZ,NZ);
            if(Score<Best){Best=Score;Normal=OZ==NZ?0.:FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(FVector(OZ).GetSafeNormal(),FVector(NZ).GetSafeNormal()),-1.,1.)));Tangent=OX==NX?0.:FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(FVector(OX).GetSafeNormal(),FVector(NX).GetSafeNormal()),-1.,1.)));}
        }
        MaxNormalAngle=FMath::Max(MaxNormalAngle,Normal);MaxTangentAngle=FMath::Max(MaxTangentAngle,Tangent);
    }
    auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("check"),TEXT("native_render_exact"));Row->SetStringField(TEXT("mesh"),Mesh->GetPathName());Row->SetNumberField(TEXT("retained_corners"),Kept);Row->SetNumberField(TEXT("original_corners"),Total);
    Row->SetNumberField(TEXT("position_allUV_RGBA_exact_corners"),GeometryKept);Row->SetNumberField(TEXT("maximum_normal_angle_diagnostic_degrees"),MaxNormalAngle);Row->SetNumberField(TEXT("maximum_tangent_angle_diagnostic_degrees"),MaxTangentAngle);
    Row->SetBoolField(TEXT("packed_tangent_bytes_in_exact_key"),true);NativeEvidence.Add(MakeShared<FJsonValueObject>(Row));
    UE_LOG(LogTemp,Display,TEXT("S362_NATIVE_RENDER_EXACT %s retained=%d/%d position_all_UVs_normals_tangents_colors=1"),*Mesh->GetName(),Kept,Total);if(Total<=100)return Reject(TEXT("original_retained_inventory_too_small"));if(Kept!=Total)return Reject(TEXT("original_native_attributes_or_winding_not_exact"));return true;
}
UMaterialExpressionConstant* Scalar(UMaterial* Material,float Value)
{
    auto* Node=NewObject<UMaterialExpressionConstant>(Material);Node->R=Value;
    Material->GetExpressionCollection().AddExpression(Node);return Node;
}
UMaterial* AtlasFinish(UMaterialInterface* Original,const FString& Name,bool bCrystal)
{
    // The pinned staff's original material is a direct texture-sample graph.
    // Duplicate the whole graph so every atlas/UV/color and the neutral fill stay exact.
    auto* Source=Cast<UMaterial>(Original);if(!Source)return nullptr;
    const FString Path=Root+Name;if(FPackageName::DoesPackageExist(Path))return nullptr;
    auto* Result=DuplicateObject<UMaterial>(Source,CreatePackage(*Path),*Name);
    Result->SetFlags(RF_Public|RF_Standalone);
    auto* SourceSample=Cast<UMaterialExpressionTextureSample>(Source->GetEditorOnlyData()->BaseColor.Expression);
    auto* NewSample=Cast<UMaterialExpressionTextureSample>(Result->GetEditorOnlyData()->BaseColor.Expression);
    if(!SourceSample || !NewSample || NewSample->Texture!=SourceSample->Texture
        || NewSample->GetOutermost()!=Result->GetOutermost())return nullptr;
    Result->GetEditorOnlyData()->Roughness.Connect(0,Scalar(Result,bCrystal?.16f:.31f));
    Result->GetEditorOnlyData()->Metallic.Connect(0,Scalar(Result,bCrystal?0.f:.72f));
    Result->GetEditorOnlyData()->Specular.Connect(0,Scalar(Result,bCrystal?.56f:.48f));
    Result->PostEditChange();if(GShaderCompilingManager)GShaderCompilingManager->FinishAllCompilation();
    const auto* Resource=Result->GetMaterialResource(GMaxRHIShaderPlatform);
    if(!Resource || !Resource->GetCompileErrors().IsEmpty() || !Save(Result))return nullptr;
    UE_LOG(LogTemp,Display,TEXT("S362_PROP_ATLAS_MATERIAL %s original_atlas=%s shader_errors=0"),*Name,*GetPathNameSafe(NewSample->Texture));
    return Result;
}
UMaterialInterface* SteelFinish(UMaterialInterface* Original)
{
    // R1 exposed no supported roughness scalar (changed=0). Preserve the MIC's
    // authored overrides and clone its entire base graph, changing only that
    // NEW parent's actual Roughness input; never guess an inactive parameter.
    auto* Source=Cast<UMaterialInstanceConstant>(Original);if(!Source)return nullptr;
    const FString Name=TEXT("M_SwordSteel"),Path=Root+Name;
    if(FPackageName::DoesPackageExist(Path))return nullptr;
    auto* Result=DuplicateObject<UMaterialInstanceConstant>(Source,CreatePackage(*Path),*Name);
    Result->SetFlags(RF_Public|RF_Standalone);
    auto* OldParent=Source->GetMaterial();if(!OldParent)return nullptr;
    const FString ParentName=TEXT("M_SwordSteelParent"),ParentPath=Root+ParentName;
    if(FPackageName::DoesPackageExist(ParentPath))return nullptr;
    auto* Parent=DuplicateObject<UMaterial>(OldParent,CreatePackage(*ParentPath),*ParentName);
    Parent->SetFlags(RF_Public|RF_Standalone);Parent->GetEditorOnlyData()->Roughness.Connect(0,Scalar(Parent,.26f));
    Parent->PostEditChange();if(GShaderCompilingManager)GShaderCompilingManager->FinishAllCompilation();
    const auto* Resource=Parent->GetMaterialResource(GMaxRHIShaderPlatform);
    if(!Resource || !Resource->GetCompileErrors().IsEmpty() || !Save(Parent))return nullptr;
    Result->SetParentEditorOnly(Parent);
    Result->PostEditChange();if(GShaderCompilingManager)GShaderCompilingManager->FinishAllCompilation();
    if(!Save(Result))return nullptr;
    UE_LOG(LogTemp,Display,TEXT("S362_PROP_STEEL_MATERIAL source=%s cloned_color_texture_parameters=1 cloned_parent=%s actual_roughness_input=.26"),*Original->GetPathName(),*Parent->GetPathName());
    return Result;
}
UStaticMesh* Import(const FString& File,const FString& Path)
{
    if(FPackageName::DoesPackageExist(Path))return nullptr;
    auto* Factory=NewObject<UFbxFactory>();Factory->SetDetectImportTypeOnImport(false);
    auto* UI=Factory->ImportUI.Get();UI->MeshTypeToImport=UI->OriginalImportType=FBXIT_StaticMesh;
    UI->bAutomatedImportShouldDetectType=false;UI->bImportAsSkeletal=false;UI->bImportMesh=true;UI->bImportAnimations=false;
    UI->bImportMaterials=false;UI->bImportTextures=false;UI->bOverrideFullName=true;UI->bCreatePhysicsAsset=false;
    auto* Data=UI->StaticMeshImportData.Get();Data->bConvertScene=true;Data->bForceFrontXAxis=true;
    Data->bConvertSceneUnit=true;Data->ImportRotation=FRotator(0,-90,0);
    Data->NormalImportMethod=FBXNIM_ImportNormalsAndTangents;Data->VertexColorImportOption=EVertexColorImportOption::Replace;
    Data->bAutoGenerateCollision=false;Data->bCombineMeshes=true;Data->bGenerateLightmapUVs=false;
    auto* Task=NewObject<UAssetImportTask>();Task->bAutomated=true;Task->Options=UI;Factory->SetAssetImportTask(Task);
    bool Cancelled=false;auto* Result=Cast<UStaticMesh>(Factory->ImportObject(UStaticMesh::StaticClass(),CreatePackage(*Path),
        *FPackageName::GetShortName(Path),RF_Public|RF_Standalone,File,nullptr,Cancelled));
    if(Result)Result->bAllowCPUAccess=true;
    return Cancelled?nullptr:Result;
}
void DiagnoseR1(UStaticMesh* Old,UStaticMesh* Raw)
{
    const auto& O=Old->GetRenderData()->LODResources[0];const auto& N=Raw->GetRenderData()->LODResources[0];
    const auto OI=O.IndexBuffer.GetArrayView(),NI=N.IndexBuffer.GetArrayView();TSet<uint32> SameSlot;
    for(const auto& S:N.Sections)if(S.MaterialIndex==0)for(uint32 I=S.FirstIndex;I<S.FirstIndex+S.NumTriangles*3;++I)SameSlot.Add(NI[I]);
    int32 Total=0,PositionSameSlot=0,PositionAnySlot=0;const int32 Channels=N.VertexBuffers.StaticMeshVertexBuffer.GetNumTexCoords();
    TArray<int32> UVHits,VFlipHits;TArray<double> MaximumUV;UVHits.Init(0,Channels);VFlipHits.Init(0,Channels);MaximumUV.Init(0.,Channels);
    for(const auto& S:O.Sections)if(S.MaterialIndex==0)for(uint32 I=S.FirstIndex;I<S.FirstIndex+S.NumTriangles*3;++I)
    {
        ++Total;const FVector3f P=O.VertexBuffers.PositionVertexBuffer.VertexPosition(OI[I]);const auto UV=O.VertexBuffers.StaticMeshVertexBuffer.GetVertexUV(OI[I],0);
        bool bSame=false,bAny=false;TArray<double> Best,Flip;Best.Init(DBL_MAX,Channels);Flip.Init(DBL_MAX,Channels);
        for(uint32 V=0;V<N.VertexBuffers.PositionVertexBuffer.GetNumVertices();++V)
        {
            if(FVector3f::DistSquared(P,N.VertexBuffers.PositionVertexBuffer.VertexPosition(V))>1e-6f)continue;
            bAny=true;bSame|=SameSlot.Contains(V);
            for(int32 C=0;C<Channels;++C)
            {const auto Q=N.VertexBuffers.StaticMeshVertexBuffer.GetVertexUV(V,C);Best[C]=FMath::Min(Best[C],double((UV-Q).Size()));Flip[C]=FMath::Min(Flip[C],double((UV-FVector2f(Q.X,1.f-Q.Y)).Size()));}
        }
        PositionSameSlot+=bSame;PositionAnySlot+=bAny;
        for(int32 C=0;C<Channels;++C)if(Best[C]!=DBL_MAX){UVHits[C]+=Best[C]<=1e-5;VFlipHits[C]+=Flip[C]<=1e-5;MaximumUV[C]=FMath::Max(MaximumUV[C],Best[C]);}
    }
    auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("check"),TEXT("rejected_R1_diagnosis_only"));Row->SetNumberField(TEXT("old_painted_corners"),Total);
    Row->SetNumberField(TEXT("position_tolerance_cm"),.001);Row->SetNumberField(TEXT("UV_tolerance"),1e-5);
    Row->SetNumberField(TEXT("position_matching_corners_same_slot"),PositionSameSlot);Row->SetNumberField(TEXT("position_matching_corners_any_slot"),PositionAnySlot);
    TArray<TSharedPtr<FJsonValue>> UVRows;for(int32 C=0;C<Channels;++C)
    {auto Q=MakeShared<FJsonObject>();Q->SetNumberField(TEXT("candidate_UV_channel"),C);Q->SetNumberField(TEXT("old_UV0_matching_corners"),UVHits[C]);Q->SetNumberField(TEXT("old_UV0_Vflipped_matching_corners"),VFlipHits[C]);Q->SetNumberField(TEXT("maximum_nearest_UV_error_at_matched_position"),MaximumUV[C]);UVRows.Add(MakeShared<FJsonValueObject>(Q));}
    Row->SetArrayField(TEXT("UV_channel_diagnosis"),UVRows);NativeEvidence.Add(MakeShared<FJsonValueObject>(Row));
    UE_LOG(LogTemp,Display,TEXT("S362_R1_DIAGNOSE old_corners=%d position_same_slot=%d position_any_slot=%d tolerance_cm=.001 acceptance_not_relaxed=1"),Total,PositionSameSlot,PositionAnySlot);
}
FPolygonGroupID Group(FMeshDescription& D,FName Name)
{
    FStaticMeshAttributes A(D);for(FPolygonGroupID G:D.PolygonGroups().GetElementIDs())if(A.GetPolygonGroupMaterialSlotNames()[G]==Name)return G;
    const auto G=D.CreatePolygonGroup();A.GetPolygonGroupMaterialSlotNames()[G]=Name;return G;
}
void AppendOnly(const FMeshDescription& Addition,FMeshDescription& Target,FPolygonGroupID TargetGroup)
{
    FStaticMeshOperations::FAppendSettings Settings;
    const int32 ExistingChannels=FStaticMeshConstAttributes(Target).GetVertexInstanceUVs().GetNumChannels();
    for(int32 I=0;I<FStaticMeshOperations::FAppendSettings::MAX_NUM_UV_CHANNELS;++I)Settings.bMergeUVChannels[I]=I<ExistingChannels;
    Settings.PolygonGroupsDelegate.BindLambda([TargetGroup](const FMeshDescription& Source,FMeshDescription&,PolygonGroupMap& Map)
    {for(FPolygonGroupID G:Source.PolygonGroups().GetElementIDs())Map.Add(G,TargetGroup);});
    FStaticMeshOperations::AppendMeshDescription(Addition,Target,Settings);
}
double TriangleError(const FVector3f A[3],const FVector3f B[3])
{
    double Best=DBL_MAX;static const int32 Permutations[6][3]={{0,1,2},{0,2,1},{1,0,2},{1,2,0},{2,0,1},{2,1,0}};
    for(const auto& P:Permutations)
    {double Error=0.;for(int32 I=0;I<3;++I)Error=FMath::Max(Error,double(FVector3f::DistSquared(A[I],B[P[I]])));Best=FMath::Min(Best,Error);}
    return FMath::Sqrt(Best);
}
bool FinishStaffRegions(FMeshDescription& D,UStaticMesh* Reference)
{
    FMeshDescription R;if(!Reference->CloneMeshDescription(0,R))return false;
    const FStaticMeshConstAttributes RA(R);const FStaticMeshAttributes A(D);
    struct FRegion {FVector3f P[3];int32 Material;};TArray<FRegion> Regions;
    for(FTriangleID T:R.Triangles().GetElementIDs())
    {FRegion Q;Q.Material=DescriptionSlot(Reference,R,R.GetTrianglePolygonGroup(T));if(Q.Material<0 || Q.Material>2)return false;const auto V=R.GetTriangleVertices(T);for(int32 I=0;I<3;++I)Q.P[I]=RA.GetVertexPositions()[V[I]];Regions.Add(Q);}
    const auto Gold=Group(D,TEXT("S362_NativeStaffMetal")),Crystal=Group(D,TEXT("S362_NativeStaffCrystal"));
    int32 Counts[3]={0,0,0};double MaximumError=0.;TMap<FPolygonID,int32> PolygonMaterials;
    for(FTriangleID T:D.Triangles().GetElementIDs())
    {
        FVector3f P[3];const auto V=D.GetTriangleVertices(T);for(int32 I=0;I<3;++I)P[I]=A.GetVertexPositions()[V[I]];
        int32 Material=INDEX_NONE;double Best=DBL_MAX;
        for(const auto& Q:Regions)
        {const double Error=TriangleError(P,Q.P);if(Error<=.001){if(Material!=INDEX_NONE && Material!=Q.Material)return false;Material=Q.Material;Best=FMath::Min(Best,Error);}}
        if(Material==INDEX_NONE){UE_LOG(LogTemp,Error,TEXT("S362_STAFF_NATIVE_REGION_UNMATCHED triangle=%d no_source_position_match_within_.001cm"),T.GetValue());return false;}
        MaximumError=FMath::Max(MaximumError,Best);++Counts[Material];const auto Polygon=D.GetTrianglePolygon(T);
        if(const int32* Prior=PolygonMaterials.Find(Polygon);Prior && *Prior!=Material)return false;PolygonMaterials.Add(Polygon,Material);
    }
    if(Counts[0]<20 || Counts[1]<100 || Counts[2]!=20)return false;
    for(const auto& P:PolygonMaterials)if(P.Value)D.SetPolygonPolygonGroup(P.Key,P.Value==1?Gold:Crystal);
    auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("check"),TEXT("native_staff_material_regions"));Row->SetNumberField(TEXT("wood_leather_triangles"),Counts[0]);Row->SetNumberField(TEXT("original_gold_triangles"),Counts[1]);Row->SetNumberField(TEXT("original_crystal_triangles"),Counts[2]);Row->SetNumberField(TEXT("maximum_source_position_match_error_cm"),MaximumError);NativeEvidence.Add(MakeShared<FJsonValueObject>(Row));
    UE_LOG(LogTemp,Display,TEXT("S362_NATIVE_STAFF_REGIONS wood=%d gold=%d crystal=%d max_position_error_cm=%.9f original_geometry_unchanged=1"),Counts[0],Counts[1],Counts[2],MaximumError);return true;
}
bool PreserveAddition(const FMeshDescription& Addition,const FMeshDescription& Composite,FName Slot)
{
    const int32 Channels=FStaticMeshConstAttributes(Composite).GetVertexInstanceUVs().GetNumChannels();
    if(Channels>FStaticMeshConstAttributes(Addition).GetVertexInstanceUVs().GetNumChannels())return false;
    auto Key=[Channels](const FMeshDescription& D,FTriangleID T)
    {
        const auto VI=D.GetTriangleVertexInstances(T);FString C[3];for(int32 I=0;I<3;++I)C[I]=DescriptionCorner(D,VI[I],Channels);
        FString K=C[0]+C[1]+C[2];for(int32 I=1;I<3;++I){const FString Q=C[I]+C[(I+1)%3]+C[(I+2)%3];if(Q<K)K=Q;}return K;
    };
    TMap<FString,int32> Keys;const FStaticMeshConstAttributes A(Composite);
    for(FTriangleID T:Composite.Triangles().GetElementIDs())if(A.GetPolygonGroupMaterialSlotNames()[Composite.GetTrianglePolygonGroup(T)]==Slot)++Keys.FindOrAdd(Key(Composite,T));
    int32 Matched=0;for(FTriangleID T:Addition.Triangles().GetElementIDs())if(int32* N=Keys.Find(Key(Addition,T));N && *N>0){--*N;++Matched;}
    auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("check"),TEXT("authored_addition_description_exact"));Row->SetStringField(TEXT("slot"),Slot.ToString());
    Row->SetNumberField(TEXT("authored_triangles"),Addition.Triangles().Num());Row->SetNumberField(TEXT("exact_triangles"),Matched);Row->SetNumberField(TEXT("native_UV_channels"),Channels);NativeEvidence.Add(MakeShared<FJsonValueObject>(Row));
    UE_LOG(LogTemp,Display,TEXT("S362_AUTHORED_ADDITION_EXACT slot=%s triangles=%d/%d attributes_float_bits=1"),*Slot.ToString(),Matched,Addition.Triangles().Num());return Matched==Addition.Triangles().Num();
}
struct FSteelTriangle{FVector P[3];};
double SampledSurfaceError(const TArray<FSteelTriangle>& From,const TArray<FSteelTriangle>& To,int32& Samples)
{
    double Maximum=0.;Samples=0;
    // Every authored profile vertex, edge and triangle interior is sampled.
    // This measures the derived fallback surface, never changes source art.
    constexpr int32 Subdivisions=8;
    for(const auto& T:From)for(int32 I=0;I<=Subdivisions;++I)for(int32 J=0;J<=Subdivisions-I;++J)
    {
        const FVector P=T.P[0]+(T.P[1]-T.P[0])*(double(I)/Subdivisions)+(T.P[2]-T.P[0])*(double(J)/Subdivisions);
        double Minimum=DBL_MAX;for(const auto& U:To)Minimum=FMath::Min(Minimum,FVector::DistSquared(P,FMath::ClosestPointOnTriangleToPoint(P,U.P[0],U.P[1],U.P[2])));
        Maximum=FMath::Max(Maximum,FMath::Sqrt(Minimum));++Samples;
    }
    return Maximum;
}
bool CheckSteelSurface(UStaticMesh* Mesh,const FMeshDescription& Authored)
{
    TArray<FSteelTriangle> Source,Fallback;FBox SourceBounds(ForceInit),FallbackBounds(ForceInit);
    const FStaticMeshConstAttributes A(Authored);
    for(FTriangleID T:Authored.Triangles().GetElementIDs())
    {FSteelTriangle Q;const auto V=Authored.GetTriangleVertices(T);for(int32 I=0;I<3;++I){Q.P[I]=FVector(A.GetVertexPositions()[V[I]]);SourceBounds+=Q.P[I];}Source.Add(Q);}
    const auto& LOD=Mesh->GetRenderData()->LODResources[0];const auto Indices=LOD.IndexBuffer.GetArrayView();
    for(const auto& S:LOD.Sections)if(S.MaterialIndex==1)for(uint32 T=0;T<S.NumTriangles;++T)
    {FSteelTriangle Q;for(int32 I=0;I<3;++I){Q.P[I]=FVector(LOD.VertexBuffers.PositionVertexBuffer.VertexPosition(Indices[S.FirstIndex+T*3+I]));FallbackBounds+=Q.P[I];}Fallback.Add(Q);}
    if(Source.Num()!=158 || Fallback.IsEmpty())return false;
    int32 ThinAxis=0;const FVector Span=SourceBounds.GetSize();for(int32 I=1;I<3;++I)if(Span[I]<Span[ThinAxis])ThinAxis=I;
    // Immutable authoring recipe: crest/trough difference=.22 of half-depth;
    // section half-depth stays at least.8 of the root. Bound fallback error to
    // one quarter of this smallest authored fuller feature (~.0198cm). This
    // validates visible geometry rather than allowing an arbitrary lost count.
    const double MinimumGrooveDepth=Span[ThinAxis]*.5*.8*.22,ErrorBudget=MinimumGrooveDepth*.25;
    int32 SourceSamples=0,FallbackSamples=0;
    const double Forward=SampledSurfaceError(Source,Fallback,SourceSamples),Reverse=SampledSurfaceError(Fallback,Source,FallbackSamples);
    double EnvelopeError=0.;for(int32 I=0;I<3;++I)EnvelopeError=FMath::Max(EnvelopeError,FMath::Max(FMath::Abs(SourceBounds.Min[I]-FallbackBounds.Min[I]),FMath::Abs(SourceBounds.Max[I]-FallbackBounds.Max[I])));
    auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("check"),TEXT("native_steel_surface_equivalence"));Row->SetNumberField(TEXT("authored_triangles"),Source.Num());Row->SetNumberField(TEXT("rendered_triangles"),Fallback.Num());
    Row->SetNumberField(TEXT("source_to_fallback_max_sample_error_cm"),Forward);Row->SetNumberField(TEXT("fallback_to_source_max_sample_error_cm"),Reverse);Row->SetNumberField(TEXT("fallback_envelope_error_cm"),EnvelopeError);
    Row->SetNumberField(TEXT("authored_minimum_groove_depth_cm"),MinimumGrooveDepth);Row->SetNumberField(TEXT("maximum_allowed_surface_error_cm"),ErrorBudget);Row->SetNumberField(TEXT("source_samples"),SourceSamples);Row->SetNumberField(TEXT("fallback_samples"),FallbackSamples);
    Row->SetStringField(TEXT("scope"),TEXT("Bidirectional native triangle-surface sampling at barycentric grid8, including every profile vertex. Error budget is quarter smallest authored fuller depth; exact source attributes and original hilt fallback checked separately."));NativeEvidence.Add(MakeShared<FJsonValueObject>(Row));
    UE_LOG(LogTemp,Display,TEXT("S362_NATIVE_STEEL_SURFACE authored=%d rendered=%d error_cm=%.9f,%.9f envelope_cm=%.9f feature_budget_cm=%.9f"),Source.Num(),Fallback.Num(),Forward,Reverse,EnvelopeError,ErrorBudget);
    return Forward<=ErrorBudget && Reverse<=ErrorBudget && EnvelopeError<=ErrorBudget;
}
UStaticMesh* NativeComposite(UStaticMesh* Old,UStaticMesh* Addition,UStaticMesh* StaffRegions,const FString& Path,UMaterialInterface* Finish,UMaterialInterface* Crystal)
{
    const bool bSword=StaffRegions==nullptr;FMeshDescription Original,AuthoredOriginal,Added;
    if(!Old->CloneMeshDescription(0,AuthoredOriginal) || !Addition->CloneMeshDescription(0,Added))return nullptr;
    if(Added.Triangles().Num()!=(bSword?158:760))return nullptr;
    TSet<FPolygonID> AuthoredRetained;
    for(FPolygonID P:AuthoredOriginal.Polygons().GetElementIDs())if(!bSword || DescriptionSlot(Old,AuthoredOriginal,AuthoredOriginal.GetPolygonPolygonGroup(P))==0)AuthoredRetained.Add(P);
    Original=AuthoredOriginal;
    if(bSword)
    {
        // The accepted DX11-visible hilt is its native LOD0, not a fresh
        // simplification of848source faces. Export exactly that518triangle
        // fallback, including every native UV/packed-normal/color decode.
        Old->ExportStaticMeshLOD(Old->GetRenderData()->LODResources[0],Original);
        TArray<FPolygonID> Steel;int32 SteelTriangles=0;
        for(FPolygonID P:Original.Polygons().GetElementIDs())
        {
            const int32 Slot=DescriptionSlot(Old,Original,Original.GetPolygonPolygonGroup(P));if(Slot<0 || Slot>1)return nullptr;
            if(Slot==1){Steel.Add(P);SteelTriangles+=Original.GetPolygonTriangles(P).Num();}
        }
        if(SteelTriangles!=14)return nullptr;Original.DeletePolygons(Steel);FElementIDRemappings Remappings;Original.Compact(Remappings);
        if(Original.Triangles().Num()!=518)return nullptr;
        auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("check"),TEXT("accepted_native_LOD0_hilt_basis"));Row->SetNumberField(TEXT("accepted_hilt_triangles"),518);
        Row->SetNumberField(TEXT("diagnostic_authored_hilt_triangles"),AuthoredRetained.Num());Row->SetNumberField(TEXT("old_steel_triangles_removed"),14);
        Row->SetStringField(TEXT("method"),TEXT("UE ExportStaticMeshLOD of unchanged accepted asset; native positions/allUVs/tangent basis/sRGB colors/winding copied. New derived asset only."));NativeEvidence.Add(MakeShared<FJsonValueObject>(Row));
    }
    if(bSword && (!PadFastBuildUVs(Original,2,TEXT("native_hilt_copy")) || !PadFastBuildUVs(Added,2,TEXT("authored_blade_copy"))))return nullptr;
    FMeshDescription Combined=Original;TSet<FPolygonID> Retained;for(FPolygonID P:Original.Polygons().GetElementIDs())Retained.Add(P);
    if(!bSword && !FinishStaffRegions(Combined,StaffRegions))return nullptr;
    const FName AddSlot=bSword?SlotName(Old,1):FName(TEXT("S362_NativeStaffMetal"));AppendOnly(Added,Combined,Group(Combined,AddSlot));
    if(!PreserveDescription(Original,Combined,Retained,bSword?TEXT("Arrel_accepted_native_LOD0_hilt"):TEXT("Elia_entire_original_staff")))return nullptr;
    auto* Mesh=DuplicateObject<UStaticMesh>(Old,CreatePackage(*Path),*FPackageName::GetShortName(Path));Mesh->SetFlags(RF_Public|RF_Standalone);Mesh->bAllowCPUAccess=true;
    // A composite has two immutable provenance inputs, not a safe single FBX
    // reimport target. Do not retain the old asset's misleading reimport link.
    Mesh->SetAssetImportData(NewObject<UAssetImportData>(Mesh));
    auto Materials=Old->GetStaticMaterials();if(bSword){Materials[1].MaterialInterface=Finish;for(int32 I=0;I<Materials.Num();++I)Materials[I].MaterialSlotName=SlotName(Old,I);}
    else{Materials.Add(FStaticMaterial(Finish,TEXT("S362_NativeStaffMetal"),TEXT("S362_NativeStaffMetal")));Materials.Add(FStaticMaterial(Crystal,TEXT("S362_NativeStaffCrystal"),TEXT("S362_NativeStaffCrystal")));}
    Mesh->SetStaticMaterials(Materials);
    if(bSword)
    {
        // Fast native builder copies supplied basis vectors directly; unlike
        // the Nanite/global build it does not re-normalize/re-tessellate the
        // accepted hilt. Its serialized bDoFastBuild repeats this same path
        // on load. Match the accepted8bit/half storage, avoiding another
        // precision conversion on reload; exact render attributes still gate.
        const auto& Buffer=Old->GetRenderData()->LODResources[0].VertexBuffers.StaticMeshVertexBuffer;
        if(Buffer.GetUseHighPrecisionTangentBasis() || Buffer.GetUseFullPrecisionUVs())
        {UE_LOG(LogTemp,Error,TEXT("S362_NATIVE_STORAGE_REJECT expected accepted8bit_tangents_halfUVs; do_not_change_precision_blindly"));return nullptr;}
        const auto& OldLOD=Old->GetRenderData()->LODResources[0];const auto OldIndices=OldLOD.IndexBuffer.GetArrayView();
        const uint8* Packed=static_cast<const uint8*>(Buffer.GetTangentData());if(!Packed)return nullptr;
        int32 UnsupportedBasis=0;for(const auto& S:OldLOD.Sections)if(S.MaterialIndex==0)for(uint32 I=S.FirstIndex;I<S.FirstIndex+S.NumTriangles*3;++I)
        {const uint8* P=Packed+OldIndices[I]*8;UnsupportedBasis+=P[3]!=127 || (P[7]!=127 && P[7]!=129);}
        if(UnsupportedBasis){UE_LOG(LogTemp,Error,TEXT("S362_NATIVE_PACKED_BASIS_REJECT unsupported_hilt_corners=%d no_normal_tolerance_relaxed"),UnsupportedBasis);return nullptr;}
        auto Nanite=Mesh->GetNaniteSettings();Nanite.bEnabled=false;Mesh->SetNaniteSettings(Nanite);Mesh->SetNumSourceModels(1);
        auto& Model=Mesh->GetSourceModel(0);auto& B=Model.BuildSettings;
        B.bRecomputeNormals=false;B.bRecomputeTangents=false;B.bRemoveDegenerates=false;B.bGenerateLightmapUVs=false;
        B.bUseHighPrecisionTangentBasis=false;B.bUseFullPrecisionUVs=false;B.BuildScale3D=FVector(1);
        Model.ReductionSettings.PercentTriangles=1.f;Model.ReductionSettings.PercentVertices=1.f;Model.ReductionSettings.MaxDeviation=0.f;
        UStaticMesh::FBuildMeshDescriptionsParams Params;Params.bFastBuild=true;Params.bAllowCpuAccess=true;Params.bBuildSimpleCollision=false;
        Params.PerLODOverrides.SetNum(1);Params.PerLODOverrides[0].bUseHighPrecisionTangentBasis=false;Params.PerLODOverrides[0].bUseFullPrecisionUVs=false;
        if(!Mesh->BuildFromMeshDescriptions({&Combined},Params))return nullptr;
    }
    else
    {Mesh->CreateMeshDescription(0,Combined);Mesh->CommitMeshDescription(0);Mesh->PostEditChange();FStaticMeshCompilingManager::Get().FinishCompilation({Mesh});}
    FMeshDescription After;if(!Mesh->CloneMeshDescription(0,After) || !PreserveDescription(Original,After,Retained,TEXT("after_native_build")))return nullptr;
    if(bSword)
    {const auto* Fast=FindFProperty<FBoolProperty>(UStaticMesh::StaticClass(),TEXT("bDoFastBuild"));if(!Fast || !Fast->GetPropertyValue_InContainer(Mesh) || Mesh->GetNaniteSettings().bEnabled || Mesh->GetSourceModel(0).BuildSettings.bRecomputeNormals || Mesh->GetSourceModel(0).BuildSettings.bRecomputeTangents)return nullptr;}
    else if(!(Mesh->GetSourceModel(0).BuildSettings==Old->GetSourceModel(0).BuildSettings) || !(Mesh->GetNaniteSettings()==Old->GetNaniteSettings()))return nullptr;
    FMeshDescription OriginalStill;if(!Old->CloneMeshDescription(0,OriginalStill) || !PreserveDescription(AuthoredOriginal,OriginalStill,AuthoredRetained,TEXT("accepted_original_authored_source_unchanged")))return nullptr;
    if(!PreserveAddition(Added,After,AddSlot))return nullptr;
    const FBox A=Mesh->GetBounds().GetBox(),B=Old->GetBounds().GetBox();
    for(int32 I=0;I<3;++I)if(FMath::Abs(A.Min[I]-B.Min[I])>.02 || FMath::Abs(A.Max[I]-B.Max[I])>.02)return nullptr;
    AuditMesh(Mesh,bSword?TEXT("delivered_native_sword"):TEXT("delivered_native_staff"));
    int32 OldRenderedSteel=0;if(bSword)for(const auto& S:Old->GetRenderData()->LODResources[0].Sections)if(S.MaterialIndex==1)OldRenderedSteel+=S.NumTriangles;
    const int32 ExpectedRendered=Old->GetRenderData()->LODResources[0].GetNumTriangles()-OldRenderedSteel+Addition->GetRenderData()->LODResources[0].GetNumTriangles();
    const int32 ActualRendered=Mesh->GetRenderData()->LODResources[0].GetNumTriangles();
    UE_LOG(LogTemp,Display,TEXT("S362_NATIVE_ADDITIONS_RENDERED %s triangles=%d expected=%d appended_description_triangles=%d"),*Mesh->GetName(),ActualRendered,ExpectedRendered,Added.Triangles().Num());
    if(!PreserveRendered(Mesh,Old,bSword)){UE_LOG(LogTemp,Error,TEXT("S362_NATIVE_RENDER_PRESERVATION_REJECT no_tolerance_relaxation"));return nullptr;}
    if(bSword)
    {if(!CheckSteelSurface(Mesh,Added)){UE_LOG(LogTemp,Error,TEXT("S362_NATIVE_STEEL_SURFACE_REJECT authored_fuller_not_faithful"));return nullptr;}}
    if(ActualRendered!=ExpectedRendered){UE_LOG(LogTemp,Error,TEXT("S362_NATIVE_ADDITIONS_RENDER_COUNT_REJECT old_build_settings_not_changed"));return nullptr;}
    return Mesh;
}
bool AdditionFrame(UStaticMesh* Blade,UStaticMesh* OldSword,UStaticMesh* Collars,UStaticMesh* OldStaff)
{
    FBox Steel(ForceInit);const auto& O=OldSword->GetRenderData()->LODResources[0];const auto Indices=O.IndexBuffer.GetArrayView();
    for(const auto& S:O.Sections)if(S.MaterialIndex==1)for(uint32 I=S.FirstIndex;I<S.FirstIndex+S.NumTriangles*3;++I)Steel+=FVector(O.VertexBuffers.PositionVertexBuffer.VertexPosition(Indices[I]));
    if(!Steel.IsValid)return false;const auto New=Blade->GetBounds().GetBox(),Staff=OldStaff->GetBounds().GetBox(),Bands=Collars->GetBounds().GetBox();
    for(int32 I=0;I<3;++I)if(FMath::Abs(Steel.Min[I]-New.Min[I])>.02 || FMath::Abs(Steel.Max[I]-New.Max[I])>.02 || Bands.Min[I]<Staff.Min[I]-.02 || Bands.Max[I]>Staff.Max[I]+.02)return false;
    UE_LOG(LogTemp,Display,TEXT("S362_NATIVE_ADDITION_FRAME blade_exact_old_steel_envelope_lt_.02cm collars_inside_old_staff_bounds=1"));return true;
}
int32 FinishEvidence(bool bSuccess)
{
    auto Report=MakeShared<FJsonObject>();Report->SetStringField(TEXT("schema"),TEXT("s362-native-composite-r5-v1"));Report->SetBoolField(TEXT("accepted"),bSuccess);
    Report->SetStringField(TEXT("output_revision"),OutputRevision);Report->SetStringField(TEXT("immutable_source_revision"),TEXT("R2"));
    Report->SetStringField(TEXT("method"),TEXT("New sword derives accepted native LOD0 hilt518tri plus158authored steel; Nanite/reduction/recompute disabled only new sword, persistent FastBuild copies supplied attributes; original2UVs remain exact and6unused storage channels are explicitly zero. Staff original MeshDescription/build-settings unchanged with collars appended. Exact original rendered attributes/winding required, no tolerance relaxation."));
    Report->SetArrayField(TEXT("native_checks"),NativeEvidence);FString Json;FJsonSerializer::Serialize(Report,TJsonWriterFactory<>::Create(&Json));
    const auto Folder=FPaths::ProjectSavedDir()/TEXT("Validation/HeroContactS362");IFileManager::Get().MakeDirectory(*Folder,true);FFileHelper::SaveStringToFile(Json,*(Folder/(TEXT("PropsNative_")+OutputRevision+TEXT(".json"))));
    return bSuccess?0:1;
}
}

UMemoriaHeroPropsS362Commandlet::UMemoriaHeroPropsS362Commandlet()
{IsClient=false;IsServer=false;IsEditor=true;LogToConsole=true;}
int32 UMemoriaHeroPropsS362Commandlet::Main(const FString& Params)
{
    if(!FApp::CanEverRender())return 1;FString Revision=TEXT("R5");FParse::Value(*Params,TEXT("Revision="),Revision);
    if(Revision!=TEXT("R5")){UE_LOG(LogTemp,Error,TEXT("S362 native composite importer accepts fresh immutableR5 only; rejectedR1/R2/R3/R4 preserved"));return 1;}
    OutputRevision=Revision;
    Root=TEXT("/Game/Memoria/Presentation/Field3D/HeroContactS362/")+Revision+TEXT("/");
    const FString Art=FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()/TEXT("../ArtSource/HeroContactS362/R2"));
    const FString SwordTarget=Root+TEXT("Arrel/SM_Arrel_sword_drawn"),StaffTarget=Root+TEXT("Elia/SM_Elia_staff");
    for(const auto& Path:{SwordTarget,StaffTarget,Root+TEXT("M_SwordSteel"),Root+TEXT("M_SwordSteelParent"),Root+TEXT("M_StaffMetal"),Root+TEXT("M_StaffCrystal")})
        if(FPackageName::DoesPackageExist(Path))return 1;
    if(!FPaths::FileExists(Art/TEXT("ArrelBlade/ArrelBlade.fbx")) || !FPaths::FileExists(Art/TEXT("EliaCollars/EliaCollars.fbx")) || !FPaths::FileExists(Art/TEXT("EliaRegions/EliaRegions.fbx")))return 1;
    auto* OldSword=Load<UStaticMesh>(TEXT("/Game/Memoria/Presentation/Field3D/Arrel/SM_Arrel_sword_drawn"));
    auto* OldStaff=Load<UStaticMesh>(TEXT("/Game/Memoria/Presentation/Field3D/Elia/SM_Elia_staff"));
    if(!OldSword || !OldStaff || OldSword->GetStaticMaterials().Num()!=2 || OldStaff->GetStaticMaterials().Num()!=1)return 1;
    AuditMesh(OldSword,TEXT("accepted_original_sword"));AuditMesh(OldStaff,TEXT("accepted_original_staff"));
    const auto RejectedFile=FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()/TEXT("../ArtSource/HeroContactS362/R1/ArrelSword/ArrelSword.fbx"));
    auto* Rejected=Import(RejectedFile,Root+TEXT("Scratch/RejectedR1Sword"));
    auto* Blade=Import(Art/TEXT("ArrelBlade/ArrelBlade.fbx"),Root+TEXT("Scratch/BladeAddition"));
    auto* Collars=Import(Art/TEXT("EliaCollars/EliaCollars.fbx"),Root+TEXT("Scratch/CollarAddition"));
    auto* Regions=Import(Art/TEXT("EliaRegions/EliaRegions.fbx"),Root+TEXT("Scratch/StaffRegionReference"));
    if(!Rejected || !Blade || !Collars || !Regions)return FinishEvidence(false);
    FStaticMeshCompilingManager::Get().FinishCompilation({Rejected,Blade,Collars,Regions});
    AuditMesh(Rejected,TEXT("rejected_R1_legacy_import_diagnostic_only"));DiagnoseR1(OldSword,Rejected);
    if(!AdditionFrame(Blade,OldSword,Collars,OldStaff))return FinishEvidence(false);
    auto* Steel=SteelFinish(OldSword->GetStaticMaterials()[1].MaterialInterface);
    auto* Metal=AtlasFinish(OldStaff->GetStaticMaterials()[0].MaterialInterface,TEXT("M_StaffMetal"),false);
    auto* Crystal=AtlasFinish(OldStaff->GetStaticMaterials()[0].MaterialInterface,TEXT("M_StaffCrystal"),true);
    if(!Steel || !Metal || !Crystal)return FinishEvidence(false);
    auto* Sword=NativeComposite(OldSword,Blade,nullptr,SwordTarget,Steel,nullptr);
    auto* Staff=NativeComposite(OldStaff,Collars,Regions,StaffTarget,Metal,Crystal);
    if(!Sword || !Staff || !Save(Sword) || !Save(Staff))return FinishEvidence(false);
    UE_LOG(LogTemp,Display,TEXT("S362_PROP_IMPORT_COMPLETE revision=R5 immutable_source=R2 native_retained_exact=1 meshes=2 materials=4 textures=0 skeletons=0 clips=0 original_assets_saved=0"));return FinishEvidence(true);
}

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHeroPropsS362R5Reload,"MemoriaGraphics.S362.PropsR5Reload",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FHeroPropsS362R5Reload::RunTest(const FString&)
{
    auto* OldSword=Load<UStaticMesh>(TEXT("/Game/Memoria/Presentation/Field3D/Arrel/SM_Arrel_sword_drawn"));
    auto* OldStaff=Load<UStaticMesh>(TEXT("/Game/Memoria/Presentation/Field3D/Elia/SM_Elia_staff"));
    auto* Sword=Load<UStaticMesh>(TEXT("/Game/Memoria/Presentation/Field3D/HeroContactS362/R5/Arrel/SM_Arrel_sword_drawn"));
    auto* Staff=Load<UStaticMesh>(TEXT("/Game/Memoria/Presentation/Field3D/HeroContactS362/R5/Elia/SM_Elia_staff"));
    if(!OldSword || !OldStaff || !Sword || !Staff){AddError(TEXT("R5 reload proof requires actual saved sword/staff packages"));return false;}
    FStaticMeshCompilingManager::Get().FinishCompilation({OldSword,OldStaff,Sword,Staff});
    TestFalse(TEXT("Only new derived sword disables Nanite"),Sword->GetNaniteSettings().bEnabled);
    const auto* Fast=FindFProperty<FBoolProperty>(UStaticMesh::StaticClass(),TEXT("bDoFastBuild"));TestTrue(TEXT("Saved native FastBuild flag persists on fresh reload"),Fast && Fast->GetPropertyValue_InContainer(Sword));
    TestTrue(TEXT("Original sword retains its original Nanite configuration"),OldSword->GetNaniteSettings().bEnabled);
    TestEqual(TEXT("Reloaded derived sword retains518native hilt plus158steel triangles"),int32(Sword->GetRenderData()->LODResources[0].GetNumTriangles()),676);
    TestEqual(TEXT("Reloaded staff retains836original plus760collar triangles"),int32(Staff->GetRenderData()->LODResources[0].GetNumTriangles()),1596);
    TestTrue(TEXT("Reloaded sword preserves every original native painted hilt triangle/attribute/packed tangent byte"),PreserveRendered(Sword,OldSword,true));
    TestTrue(TEXT("Reloaded staff preserves every original native painted triangle/attribute/packed tangent byte"),PreserveRendered(Staff,OldStaff,false));
    TestEqual(TEXT("Reloaded native hilt fingerprint matches accepted original"),FMemoriaNativePropFingerprint::MeshLOD0(Sword,0,2),FMemoriaNativePropFingerprint::MeshLOD0(OldSword,0));
    AddInfo(TEXT("S362_R5_RELOAD_FINGERPRINT sword=")+FMemoriaNativePropFingerprint::MeshLOD0(Sword)+TEXT(" staff=")+FMemoriaNativePropFingerprint::MeshLOD0(Staff));
    TestEqual(TEXT("Reloaded sword keeps exact original painted material"),Sword->GetStaticMaterials()[0].MaterialInterface.Get(),OldSword->GetStaticMaterials()[0].MaterialInterface.Get());
    FMeshDescription D;
    if(!Sword->CloneMeshDescription(0,D)){AddError(TEXT("Reloaded derived sword source unavailable"));return false;}
    TestEqual(TEXT("Reloaded derived sword source is676native triangles"),D.Triangles().Num(),676);
    const auto SourceUV=FStaticMeshConstAttributes(D).GetVertexInstanceUVs();
    TestEqual(TEXT("Reloaded source persists8initialized channels"),SourceUV.GetNumChannels(),int32(MAX_STATIC_TEXCOORDS));
    int32 SourceNonzero=0;
    if(SourceUV.GetNumChannels()==MAX_STATIC_TEXCOORDS)for(FVertexInstanceID V:D.VertexInstances().GetElementIDs())for(int32 C=2;C<MAX_STATIC_TEXCOORDS;++C)
    {const auto Q=SourceUV.Get(V,C);SourceNonzero+=FloatBits(Q.X)!=FloatBits(0.f);SourceNonzero+=FloatBits(Q.Y)!=FloatBits(0.f);}
    TestEqual(TEXT("Reloaded source padding remains exact positive zero"),SourceNonzero,0);
    FString Json;TSharedPtr<FJsonObject> NativeReport;
    if(!FFileHelper::LoadFileToString(Json,*(FPaths::ProjectDir()/TEXT("../ArtSource/HeroContactS362/R5/native_acceptance.json"))) || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),NativeReport) || !NativeReport.IsValid())
    {AddError(TEXT("Fresh-process proof requires the pinned accepted R5 native contract"));return false;}
    TestTrue(TEXT("Native import report accepted"),NativeReport->GetBoolField(TEXT("accepted")));
    TMap<FString,FString> Expected;
    for(const auto& Value:NativeReport->GetArrayField(TEXT("native_checks")))
    {const auto Row=Value->AsObject();if(Row->GetStringField(TEXT("check"))==TEXT("native_inventory"))Expected.Add(Row->GetStringField(TEXT("label")),Row->GetStringField(TEXT("native_LOD0_fingerprint_sha1_v1")));}
    const FString SwordHash=FMemoriaNativePropFingerprint::MeshLOD0(Sword),StaffHash=FMemoriaNativePropFingerprint::MeshLOD0(Staff);
    TestFalse(TEXT("Fresh sword all8UV fingerprint is available"),SwordHash.IsEmpty());TestFalse(TEXT("Fresh staff fingerprint is available"),StaffHash.IsEmpty());
    TestEqual(TEXT("Fresh sword full native fingerprint equals accepted import including all8UVs"),SwordHash,Expected.FindRef(TEXT("delivered_native_sword")));
    TestEqual(TEXT("Fresh staff full native fingerprint equals accepted import"),StaffHash,Expected.FindRef(TEXT("delivered_native_staff")));
    AddInfo(TEXT("S362_R5_FRESH_PROCESS_RELOAD_PROOF derived_sword676 staff1596 original_native_render_exact=1"));return true;
}
#endif
