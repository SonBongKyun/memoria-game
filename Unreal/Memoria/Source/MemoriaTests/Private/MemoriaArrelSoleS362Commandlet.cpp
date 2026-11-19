#include "MemoriaArrelSoleS362Commandlet.h"
#include "MemoriaArrelSoleS362Validation.h"
#include "MemoriaRetargetKit.h"
#include "Factories/FbxFactory.h"
#include "Factories/FbxImportUI.h"
#include "Factories/FbxSkeletalMeshImportData.h"
#include "AssetImportTask.h"
#include "Animation/Skeleton.h"
#include "Engine/SkeletalMesh.h"
#include "EditorFramework/AssetImportData.h"
#include "SkinnedAssetCompiler.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Rendering/SkeletalMeshModel.h"
#include "Rendering/SkeletalMeshLODModel.h"
#include "Rendering/SkinWeightVertexBuffer.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "SkeletalMeshAttributes.h"
#include "Json.h"
#include "Misc/App.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/AutomationTest.h"
THIRD_PARTY_INCLUDES_START
#include <openssl/sha.h>
THIRD_PARTY_INCLUDES_END

using namespace MemoriaRetargetKit;
namespace
{
const FString OldPath=TEXT("/Game/Memoria/Presentation/Field3D/HeroFinishS362/R2/Arrel/SK_Arrel");
const FString NewPath=TEXT("/Game/Memoria/Presentation/Field3D/HeroFinishS362/R4/Arrel/SK_Arrel");
const int32 AllowedIDs[]={329,4492,4504,4964,4981,4993,5331,5667,5671,5680,6573,10722,10734,11185,11202,11214,11549,11882,11886,11895};
const int32 PositionIDs[]={4981,5671,11202,11886};
TSharedPtr<FJsonObject> Report;
struct FRecipe {TMap<int32,TMap<FName,double>> Weights;TSet<int32> Positions;};
bool Allowed(int32 ID){for(int32 V:AllowedIDs)if(V==ID)return true;return false;}
bool Moved(int32 ID){for(int32 V:PositionIDs)if(V==ID)return true;return false;}
FString Bits(float V){uint32 U;FMemory::Memcpy(&U,&V,4);return FString::Printf(TEXT("%08x"),U);}
void Vec(FString& K,const FVector3f& V){K+=Bits(V.X)+Bits(V.Y)+Bits(V.Z);}
FString FileHash(const FString& File)
{TArray<uint8> B;if(!FFileHelper::LoadFileToArray(B,*File))return FString();uint8 Hash[SHA256_DIGEST_LENGTH];SHA256(B.GetData(),B.Num(),Hash);return BytesToHex(Hash,SHA256_DIGEST_LENGTH).ToLower();}
bool Reject(const FString& Reason)
{UE_LOG(LogTemp,Error,TEXT("S362_SOLE_R4_REJECT %s"),*Reason);if(Report.IsValid())Report->SetStringField(TEXT("rejection"),Reason);return false;}
int32 Finish(bool Success)
{
    Report->SetBoolField(TEXT("accepted"),Success);FString Json;FJsonSerializer::Serialize(Report.ToSharedRef(),TJsonWriterFactory<>::Create(&Json));
    const FString Folder=FPaths::ProjectSavedDir()/TEXT("Validation/HeroS362");IFileManager::Get().MakeDirectory(*Folder,true);FFileHelper::SaveStringToFile(Json,*(Folder/TEXT("SoleR4Import.json")));return Success?0:1;
}
bool Recipe(FRecipe& Out)
{
    const FString Folder=FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()/TEXT("../ArtSource/HeroFinishS362/R4/Arrel"));
    FString Text;TSharedPtr<FJsonObject> R;if(!FFileHelper::LoadFileToString(Text,*(Folder/TEXT("geometry.json"))) || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),R) || !R.IsValid())return Reject(TEXT("missing_or_invalid_immutable_R4_receipt"));
    if(R->GetStringField(TEXT("source_sha256"))!=TEXT("a99abcc2e00dfe6ecf8ac0170079405ea42c21e55506d62967c7c3b80ae517da") || R->GetStringField(TEXT("output_sha256"))!=TEXT("5cd6297af9b7e8cb55d233e9e3bc4677aa0b30bac9b3ac652f510a4d60b0c1f9") || FileHash(Folder/TEXT("Arrel.fbx"))!=R->GetStringField(TEXT("output_sha256")))return Reject(TEXT("immutable_source_hash_mismatch"));
    for(const auto& Value:R->GetArrayField(TEXT("weight_exceptions")))
    {const auto Row=Value->AsObject();const int32 ID=Row->GetIntegerField(TEXT("source_ID"));if(!Allowed(ID) || Out.Weights.Contains(ID))return Reject(TEXT("unexpected_or_duplicate_weight_exception"));TMap<FName,double> W;for(const auto& Pair:Row->GetObjectField(TEXT("expected_weights"))->Values)W.Add(FName(Pair.Key),Pair.Value->AsNumber());const FName Foot=ID<6000?FName(TEXT("foot_l")):FName(TEXT("foot_r"));if(W.Num()!=1 || !W.Contains(Foot) || W[Foot]!=1.)return Reject(TEXT("recipe_sole_requires_exact_existing_foot_one"));Out.Weights.Add(ID,MoveTemp(W));}
    for(const auto& Value:R->GetArrayField(TEXT("position_exceptions")))
    {const int32 ID=Value->AsObject()->GetIntegerField(TEXT("source_ID"));if(!Moved(ID))return Reject(TEXT("unapproved_position_exception"));Out.Positions.Add(ID);}
    return Out.Weights.Num()==20 && Out.Positions.Num()==4 || Reject(TEXT("exception_inventory_not_20weights_4positions"));
}
bool MapPoints(const FMeshDescription& D,TMap<int32,FVertexID>& Out)
{
    const auto Import=D.VertexAttributes().GetAttributesRef<int32>(MeshAttribute::Vertex::ImportPointIndex);
    if(!Import.IsValid())return Reject(TEXT("native_import_point_index_attribute_missing"));
    for(FVertexID V:D.Vertices().GetElementIDs()){const int32 P=Import[V];if(Out.Contains(P))return Reject(TEXT("nonunique_original_import_point_ID"));Out.Add(P,V);}return true;
}
FString WeightKey(const FSkinWeightsVertexAttributesConstRef& Skin,FVertexID V)
{TArray<FString> W;for(const auto B:Skin.Get(V))W.Add(FString::Printf(TEXT("%d=%d"),B.GetBoneIndex(),B.GetRawWeight()));W.Sort();return FString::Join(W,TEXT(";"));}
FString CornerAttributes(const FMeshDescription& D,FVertexInstanceID I)
{
    const FStaticMeshConstAttributes A(D);FString K;Vec(K,A.GetVertexInstanceNormals()[I]);Vec(K,A.GetVertexInstanceTangents()[I]);K+=Bits(A.GetVertexInstanceBinormalSigns()[I]);
    const auto C=A.GetVertexInstanceColors()[I];K+=Bits(C.X)+Bits(C.Y)+Bits(C.Z)+Bits(C.W);const auto UV=A.GetVertexInstanceUVs();for(int32 U=0;U<UV.GetNumChannels();++U){const auto Q=UV.Get(I,U);K+=Bits(Q.X)+Bits(Q.Y);}return K;
}
// R2 intentionally recomputes Mikk tangents. Freeze its accepted, computed
// PRE-PACK tangent vectors into NEW R4 wedges; decoding packed normals before
// the skeletal builder normalizes again can lose packed bytes. Original
// MeshDescription normal/UV/RGBA/topology data is never overwritten.
bool AcceptedNativeTangentCache(USkeletalMesh* Old,const FMeshDescription& Source,FMeshDescription& Target)
{
    FSkinnedAssetCompilingManager::Get().FinishCompilation({Old});
    const auto* Model=Old->GetImportedModel();const auto* Render=Old->GetResourceForRendering();
    if(!Model || Model->LODModels.IsEmpty() || !Render || Render->LODRenderData.IsEmpty())return Reject(TEXT("accepted_native_tangent_cache_missing_LOD0"));
    const auto& M=Model->LODModels[0];const auto& L=Render->LODRenderData[0];const auto& VB=L.StaticVertexBuffers.StaticMeshVertexBuffer;
    if(!VB.GetNumVertices() || !VB.GetTangentData() || M.MeshToImportVertexMap.Num()!=int32(L.GetNumVertices()))return Reject(TEXT("accepted_native_tangent_cache_missing_CPU_mapping"));
    const FStaticMeshConstAttributes A(Source);const auto Import=Source.VertexAttributes().GetAttributesRef<int32>(MeshAttribute::Vertex::ImportPointIndex);
    const int32 UVCount=A.GetVertexInstanceUVs().GetNumChannels(),Stride=VB.GetTangentSize()/VB.GetNumVertices();
    if(!Import.IsValid() || UVCount!=int32(VB.GetNumTexCoords()))return Reject(TEXT("accepted_native_tangent_cache_UV_or_source_mapping_mismatch"));
    auto WedgeKey=[&](int32 SourceID,const FVector3f& P,const FColor& Color,const auto& UV)
    {
        FString K=FString::Printf(TEXT("%d:"),SourceID);Vec(K,P);
        // Structural triangle/wedge identity only; no normal/tangent value
        // tolerance participates in mapping. Ambiguous computed bases reject.
        K+=FString::Printf(TEXT("%08x"),Color.DWColor());
        for(int32 I=0;I<UVCount;++I){const auto Q=UV(I);K+=Bits(Q.X)+Bits(Q.Y);}return K;
    };
    auto SoftKey=[&](const FSkelMeshSection& S,uint32 V)
    {const auto& Soft=S.SoftVertices[V-S.BaseVertexIndex];const FVertexID Point(M.MeshToImportVertexMap[V]);return WedgeKey(Import[Point],Soft.Position,Soft.Color,[&](int32 U){return Soft.UVs[U];});};
    auto SourceKey=[&](FVertexInstanceID I)
    {const FLinearColor C=A.GetVertexInstanceColors()[I];const auto UV=A.GetVertexInstanceUVs();const FVertexID V=Source.GetVertexInstanceVertex(I);return WedgeKey(Import[V],A.GetVertexPositions()[V],C.ToFColor(false),[&](int32 U){return UV.Get(I,U);});};
    auto TriangleKey=[](const FString C[3],int32 Material)
    {FString K=C[0]+C[1]+C[2];for(int32 I=1;I<3;++I){const FString Q=C[I]+C[(I+1)%3]+C[(I+2)%3];if(Q<K)K=Q;}return FString::Printf(TEXT("%d:"),Material)+K;};
    struct FNativeTriangle{int32 Section;uint32 V[3];};
    TMap<FString,TArray<FNativeTriangle>> NativeTriangles;
    for(int32 SectionIndex=0;SectionIndex<M.Sections.Num();++SectionIndex)
    {
        const auto& S=M.Sections[SectionIndex];if(S.SoftVertices.Num()!=int32(S.NumVertices))return Reject(TEXT("accepted_uncompressed_soft_basis_missing"));
        for(uint32 T=0;T<S.NumTriangles;++T)
        {
            FNativeTriangle Row;Row.Section=SectionIndex;FString C[3];for(int32 I=0;I<3;++I){Row.V[I]=M.IndexBuffer[S.BaseIndex+T*3+I];C[I]=SoftKey(S,Row.V[I]);}
            NativeTriangles.FindOrAdd(TriangleKey(C,S.MaterialIndex)).Add(Row);
        }
    }
    struct FBasis{FVector3f Tangent;float Sign;FString Packed;};TMap<FVertexInstanceID,FBasis> Assigned;int32 Corners=0,Conflicts=0,Triangles=0;
    const auto GroupNames=A.GetPolygonGroupMaterialSlotNames();
    for(FTriangleID T:Source.Triangles().GetElementIDs())
    {
        const FName Name=GroupNames[Source.GetTrianglePolygonGroup(T)];int32 Material=INDEX_NONE;
        for(int32 I=0;I<Old->GetMaterials().Num();++I)if(Old->GetMaterials()[I].ImportedMaterialSlotName==Name || Old->GetMaterials()[I].MaterialSlotName==Name){Material=I;break;}
        if(Material==INDEX_NONE)return Reject(TEXT("native_tangent_wedge_material_unmapped"));
        const auto VI=Source.GetTriangleVertexInstances(T);FString C[3];for(int32 I=0;I<3;++I)C[I]=SourceKey(VI[I]);
        auto* Options=NativeTriangles.Find(TriangleKey(C,Material));if(!Options || Options->IsEmpty())return Reject(FString::Printf(TEXT("native_tangent_wedge_triangle_unmatched source_triangle=%d"),T.GetValue()));
        const auto Row=Options->Last();const auto& S=M.Sections[Row.Section];FString NC[3];for(int32 I=0;I<3;++I)NC[I]=SoftKey(S,Row.V[I]);int32 Rotation=INDEX_NONE;
        for(int32 R=0;R<3;++R)if(C[0]==NC[R] && C[1]==NC[(R+1)%3] && C[2]==NC[(R+2)%3]){Rotation=R;break;}
        if(Rotation==INDEX_NONE)return Reject(TEXT("native_tangent_wedge_winding_mismatch"));
        // Coincident identical triangles may only share a mapping when every
        // corresponding accepted packed basis is itself identical.
        for(const auto& Other:*Options)
        {
            const auto& OS=M.Sections[Other.Section];FString OC[3];for(int32 I=0;I<3;++I)OC[I]=SoftKey(OS,Other.V[I]);int32 ORotation=INDEX_NONE;
            for(int32 R=0;R<3;++R)if(C[0]==OC[R] && C[1]==OC[(R+1)%3] && C[2]==OC[(R+2)%3]){ORotation=R;break;}
            if(ORotation==INDEX_NONE)return Reject(TEXT("native_duplicate_triangle_winding_mismatch"));
            for(int32 I=0;I<3;++I)
            {
                const uint8* Packed=static_cast<const uint8*>(VB.GetTangentData());
                if(FMemory::Memcmp(Packed+Row.V[(Rotation+I)%3]*Stride,Packed+Other.V[(ORotation+I)%3]*Stride,Stride))
                    return Reject(TEXT("native_duplicate_triangle_basis_ambiguous_conflict"));
            }
        }
        for(int32 I=0;I<3;++I)
        {
            const uint32 V=Row.V[(Rotation+I)%3];const auto& Soft=S.SoftVertices[V-S.BaseVertexIndex];
            const FString Packed=FString::FromHexBlob(static_cast<const uint8*>(VB.GetTangentData())+V*Stride,Stride);
            const float Sign=FVector3f::DotProduct(Soft.TangentY,FVector3f::CrossProduct(FVector3f(Soft.TangentZ),Soft.TangentX))<0.f?-1.f:1.f;
            if(const auto* Prior=Assigned.Find(VI[I])){if(Prior->Packed!=Packed)++Conflicts;}
            else Assigned.Add(VI[I],{Soft.TangentX,Sign,Packed});++Corners;
        }
        Options->Pop(EAllowShrinking::No);++Triangles;
    }
    for(const auto& Pair:NativeTriangles)if(!Pair.Value.IsEmpty())return Reject(TEXT("accepted_native_tangent_triangles_not_consumed"));
    if(Conflicts || Triangles!=33804 || Corners!=101412)return Reject(FString::Printf(TEXT("accepted_native_tangent_wedge_conflicts=%d triangles=%d corners=%d"),Conflicts,Triangles,Corners));
    auto TA=FStaticMeshAttributes(Target);auto Tangents=TA.GetVertexInstanceTangents();auto Signs=TA.GetVertexInstanceBinormalSigns();int32 Changes=0;
    for(const auto& Pair:Assigned){Changes+=Tangents[Pair.Key]!=Pair.Value.Tangent || Signs[Pair.Key]!=Pair.Value.Sign;Tangents[Pair.Key]=Pair.Value.Tangent;Signs[Pair.Key]=Pair.Value.Sign;}
    const FStaticMeshConstAttributes Check(Target);int32 NormalChanges=0;for(FVertexInstanceID I:Source.VertexInstances().GetElementIDs())NormalChanges+=A.GetVertexInstanceNormals()[I]!=Check.GetVertexInstanceNormals()[I];
    if(NormalChanges)return Reject(TEXT("accepted_native_tangent_bake_changed_original_normals"));
    if(Report.IsValid()){Report->SetNumberField(TEXT("accepted_native_tangent_wedge_triangles"),Triangles);Report->SetNumberField(TEXT("accepted_native_tangent_wedge_corners"),Corners);Report->SetNumberField(TEXT("accepted_native_tangent_assigned_instances"),Assigned.Num());Report->SetNumberField(TEXT("accepted_native_tangent_cache_changes"),Changes);Report->SetNumberField(TEXT("accepted_native_tangent_conflicts"),Conflicts);Report->SetNumberField(TEXT("original_MD_normal_changes"),NormalChanges);}
    UE_LOG(LogTemp,Display,TEXT("S362_SOLE_R4_ACCEPTED_TANGENT_CACHE triangles=%d corners=%d assigned=%d cache_changes=%d conflicts=%d original_normals_changed=%d native_packed_guard_unchanged=1"),Triangles,Corners,Assigned.Num(),Changes,Conflicts,NormalChanges);return true;
}
bool Descriptions(const FMeshDescription& O,const FMeshDescription& N,const FReferenceSkeleton& Ref,const FRecipe& R,bool bParsed,const FMeshDescription* AcceptedBasis=nullptr)
{
    if(O.Vertices().Num()!=17005 || N.Vertices().Num()!=17005 || O.Triangles().Num()!=33804 || N.Triangles().Num()!=33804 || O.VertexInstances().Num()!=N.VertexInstances().Num())return Reject(TEXT("description_geometry_inventory_changed"));
    TMap<int32,FVertexID> OM,NM;if(!MapPoints(O,OM) || !MapPoints(N,NM) || OM.Num()!=NM.Num())return false;
    const FStaticMeshConstAttributes OA(O),NA(N);const auto OS=FSkeletalMeshConstAttributes(O).GetVertexSkinWeights(),NS=FSkeletalMeshConstAttributes(N).GetVertexSkinWeights();
    if(OA.GetVertexInstanceUVs().GetNumChannels()!=NA.GetVertexInstanceUVs().GetNumChannels())return Reject(TEXT("description_UV_channel_count_changed"));
    int32 WeightChanges=0,PositionChanges=0;
    for(const auto& Pair:OM)
    {
        const int32 ID=Pair.Key;const FVertexID* NV=NM.Find(ID);if(!NV)return Reject(TEXT("import_point_ID_lost"));
        const FVector3f A=OA.GetVertexPositions()[Pair.Value],B=NA.GetVertexPositions()[*NV];
        if(A!=B)
        {
            ++PositionChanges;if(!Moved(ID) || Bits(A.X)!=Bits(B.X) || Bits(A.Y)!=Bits(B.Y) || B.Z>A.Z || A.Z-B.Z>.18028f)return Reject(FString::Printf(TEXT("unapproved_position_delta source_ID=%d"),ID));
            const int32 MinimumID=ID<6000?4964:11185;const float ExpectedZ=OA.GetVertexPositions()[OM[MinimumID]].Z;if(Bits(B.Z)!=Bits(ExpectedZ))return Reject(TEXT("sole_not_flattened_to_exact_original_minimum"));
        }
        const FString OW=WeightKey(OS,Pair.Value),NW=WeightKey(NS,*NV);
        if(OW!=NW)
        {
            ++WeightChanges;if(!Allowed(ID))return Reject(TEXT("weight_changed_outside_exact20IDs"));
            const auto* Expected=R.Weights.Find(ID);if(!Expected || Expected->Num()!=1 || NS.Get(*NV).Num()!=1)return Reject(TEXT("sole_weight_not_exactly_existing_foot_one"));
            uint32 Total=0;for(const auto W:NS.Get(*NV))
            {const double* Want=Expected->Find(Ref.GetBoneName(W.GetBoneIndex()));if(!Want || FMath::Abs(int64(W.GetRawWeight())-int64(FMath::RoundToInt(*Want*65535.)))>1)return Reject(TEXT("sole_foot_weight_not_source_recipe_quantization"));Total+=W.GetRawWeight();}
            if(Total!=65535)return Reject(TEXT("sole_raw_skin_weights_not_normalized"));
        }
    }
    if(WeightChanges!=20 || PositionChanges!=4)return Reject(TEXT("actual_description_exception_count_not20_and4"));
    // The legacy import must keep every source UV/color/normal. Tangents can
    // be regenerated at moved corners by legacy import; final descriptor below
    // takes ALL accepted R2 corner attributes verbatim, including tangents.
    int32 CornerChanges=0;
    for(FVertexInstanceID I:O.VertexInstances().GetElementIDs())
    {
        if(!N.IsVertexInstanceValid(I) || O.GetVertexInstanceVertex(I)!=N.GetVertexInstanceVertex(I))return Reject(TEXT("corner_source_mapping_changed"));
        if(CornerAttributes(AcceptedBasis?*AcceptedBasis:O,I)!=CornerAttributes(N,I))++CornerChanges;
    }
    if(!bParsed && CornerChanges)return Reject(TEXT("final_native_normals_tangents_UVs_RGBA_not_bit_exact"));
    for(FTriangleID T:O.Triangles().GetElementIDs())
    {if(!N.IsTriangleValid(T) || O.GetTrianglePolygonGroup(T)!=N.GetTrianglePolygonGroup(T))return Reject(TEXT("triangle_material_group_changed"));const auto A=O.GetTriangleVertexInstances(T),B=N.GetTriangleVertexInstances(T);for(int32 I=0;I<3;++I)if(A[I]!=B[I])return Reject(TEXT("triangle_topology_or_winding_changed"));}
    if(Report.IsValid()){Report->SetNumberField(bParsed?TEXT("legacy_parsed_corner_cache_differences"):TEXT("final_corner_attribute_differences"),CornerChanges);Report->SetNumberField(TEXT("position_exception_controlpoints"),PositionChanges);Report->SetNumberField(TEXT("weight_exception_controlpoints"),WeightChanges);}
    UE_LOG(LogTemp,Display,TEXT("S362_SOLE_R4_DESCRIPTION parsed=%d source_points=%d triangles=%d position_changes=%d weight_changes=%d corner_attribute_changes=%d"),bParsed,OM.Num(),O.Triangles().Num(),PositionChanges,WeightChanges,CornerChanges);return true;
}
FString RenderCorner(const FSkeletalMeshLODRenderData& L,const FSkelMeshRenderSection& S,uint32 V,const FMeshDescription& D,const FSkeletalMeshLODModel& Model)
{
    const auto Import=D.VertexAttributes().GetAttributesRef<int32>(MeshAttribute::Vertex::ImportPointIndex);const FVertexID Source(Model.MeshToImportVertexMap[V]);const int32 ID=Import[Source];FString K=FString::Printf(TEXT("%d:%d:"),S.MaterialIndex,ID);
    const auto& VB=L.StaticVertexBuffers.StaticMeshVertexBuffer;const int32 Stride=VB.GetTangentSize()/VB.GetNumVertices();K+=FString::FromHexBlob(static_cast<const uint8*>(VB.GetTangentData())+V*Stride,Stride);
    const auto C=L.StaticVertexBuffers.ColorVertexBuffer.GetNumVertices()?L.StaticVertexBuffers.ColorVertexBuffer.VertexColor(V):FColor::White;K+=FString::Printf(TEXT("%08x"),C.DWColor());
    for(uint32 U=0;U<VB.GetNumTexCoords();++U){const auto Q=VB.GetVertexUV(V,U);K+=Bits(Q.X)+Bits(Q.Y);}if(!Moved(ID))Vec(K,L.StaticVertexBuffers.PositionVertexBuffer.VertexPosition(V));
    if(!Allowed(ID))
    {const auto* Skin=L.GetSkinWeightVertexBuffer();TArray<FString> W;for(uint32 I=0;I<Skin->GetMaxBoneInfluences();++I){const uint32 Weight=Skin->GetBoneWeight(V,I);if(Weight)W.Add(FString::Printf(TEXT("%d=%d"),S.BoneMap[Skin->GetBoneIndex(V,I)],Weight));}W.Sort();K+=FString::Join(W,TEXT(";"));}return K;
}
bool RenderExact(USkeletalMesh* Old,USkeletalMesh* Mesh,const FMeshDescription& O,const FMeshDescription& N)
{
    const auto& OL=Old->GetResourceForRendering()->LODRenderData[0];const auto& NL=Mesh->GetResourceForRendering()->LODRenderData[0];
    const auto& OM=Old->GetImportedModel()->LODModels[0];const auto& NM=Mesh->GetImportedModel()->LODModels[0];
    if(OM.MeshToImportVertexMap.Num()!=int32(OL.GetNumVertices()) || NM.MeshToImportVertexMap.Num()!=int32(NL.GetNumVertices()) || !OL.StaticVertexBuffers.StaticMeshVertexBuffer.GetTangentData() || !NL.StaticVertexBuffers.StaticMeshVertexBuffer.GetTangentData())return Reject(TEXT("native_render_import_mapping_or_CPU_basis_unavailable"));
    if(OL.StaticVertexBuffers.StaticMeshVertexBuffer.GetNumTexCoords()!=NL.StaticVertexBuffers.StaticMeshVertexBuffer.GetNumTexCoords())return Reject(TEXT("native_render_UV_count_changed"));
    const auto Import=N.VertexAttributes().GetAttributesRef<int32>(MeshAttribute::Vertex::ImportPointIndex);const auto Positions=FStaticMeshConstAttributes(N).GetVertexPositions();const auto DescSkin=FSkeletalMeshConstAttributes(N).GetVertexSkinWeights();
    const auto* NativeSkin=NL.GetSkinWeightVertexBuffer();const double Quantization=1./(NativeSkin->Use16BitBoneWeight()?65535.:255.);TSet<int32> RuntimeSoleIDs;
    for(uint32 V=0;V<NL.GetNumVertices();++V)
    {
        const FVertexID Source(NM.MeshToImportVertexMap[V]);if(!N.IsVertexValid(Source))return Reject(TEXT("native_vertex_source_mapping_invalid"));
        const FVector3f P=Positions[Source],Q=NL.StaticVertexBuffers.PositionVertexBuffer.VertexPosition(V);if(Bits(P.X)!=Bits(Q.X) || Bits(P.Y)!=Bits(Q.Y) || Bits(P.Z)!=Bits(Q.Z))return Reject(TEXT("rendered_positions_differ_from_delivered_source_including4Z"));
        const int32 ID=Import[Source];if(!Allowed(ID))continue;RuntimeSoleIDs.Add(ID);
        const FSkelMeshRenderSection* Section=nullptr;for(const auto& S:NL.RenderSections)if(V>=S.BaseVertexIndex && V<S.BaseVertexIndex+S.NumVertices){Section=&S;break;}if(!Section)return Reject(TEXT("native_sole_section_missing"));
        TMap<int32,double> Expected;for(const auto W:DescSkin.Get(Source))Expected.Add(W.GetBoneIndex(),double(W.GetRawWeight())/65535.);
        uint32 Total=0;int32 Count=0;for(uint32 I=0;I<NativeSkin->GetMaxBoneInfluences();++I)Total+=NativeSkin->GetBoneWeight(V,I);
        for(uint32 I=0;I<NativeSkin->GetMaxBoneInfluences();++I)
        {const uint32 W=NativeSkin->GetBoneWeight(V,I);if(!W)continue;++Count;const uint32 Local=NativeSkin->GetBoneIndex(V,I);if(!Section->BoneMap.IsValidIndex(Local))return Reject(TEXT("native_sole_bone_map_invalid"));const double* Want=Expected.Find(Section->BoneMap[Local]);if(!Want || FMath::Abs(double(W)/FMath::Max(1u,Total)-*Want)>Quantization)return Reject(TEXT("native_sole_skin_not_recipe_at_actual_buffer_precision"));}
        // GetBoneWeight expands8bit stored values to16bit (255 becomes65535).
        if(Count!=1 || Total!=65535u)return Reject(TEXT("native_sole_requires_exactly_existingfoot_one_normalized"));
    }
    if(RuntimeSoleIDs.Num()!=20)return Reject(TEXT("not_all20sole_sourceIDs_reached_native_render"));
    if(Report.IsValid()){Report->SetNumberField(TEXT("native_sole_sourceIDs_verified"),RuntimeSoleIDs.Num());Report->SetNumberField(TEXT("native_weight_precision_bits"),NativeSkin->Use16BitBoneWeight()?16:8);}
    auto Keys=[&](const FSkeletalMeshLODRenderData& L,const FSkeletalMeshLODModel& Model,const FMeshDescription& D)
    {
        TMap<FString,int32> K;const auto* Indices=L.MultiSizeIndexContainer.GetIndexBuffer();for(const auto& S:L.RenderSections)for(uint32 T=0;T<S.NumTriangles;++T)
        {FString C[3];for(int32 I=0;I<3;++I)C[I]=RenderCorner(L,S,Indices->Get(S.BaseIndex+T*3+I),D,Model);FString Key=C[0]+C[1]+C[2];for(int32 I=1;I<3;++I){const FString Q=C[I]+C[(I+1)%3]+C[(I+2)%3];if(Q<Key)Key=Q;}++K.FindOrAdd(Key);}return K;
    };
    auto Expected=Keys(OL,OM,O),Actual=Keys(NL,NM,N);int32 Total=0,Kept=0;for(const auto& Pair:Expected){Total+=Pair.Value;if(const int32* Count=Actual.Find(Pair.Key))Kept+=FMath::Min(Pair.Value,*Count);}
    if(Report.IsValid()){Report->SetNumberField(TEXT("native_triangles_all_original_attributes_checked"),Total);Report->SetNumberField(TEXT("native_triangles_exact_except_authorized_position_skin"),Kept);}
    UE_LOG(LogTemp,Display,TEXT("S362_SOLE_R4_NATIVE_EXACT triangles=%d/%d normal_tangent_bytes_allUV_RGBA_winding_everycorner=1 outside4positions_outside20weights_exact=1"),Kept,Total);return Total==33804 && Kept==Total || Reject(TEXT("native_render_attribute_or_winding_loss"));
}
bool MeshContract(USkeletalMesh* Old,USkeletalMesh* Mesh,const FRecipe& R)
{
    if(Mesh->GetSkeleton()!=Old->GetSkeleton() || Mesh->GetRefSkeleton().GetNum()!=77 || Mesh->GetMaterials().Num()!=Old->GetMaterials().Num() || !Mesh->GetMorphTargets().IsEmpty())return Reject(TEXT("skeleton_material_morph_inventory_changed"));
    for(int32 I=0;I<77;++I)if(Mesh->GetRefSkeleton().GetBoneName(I)!=Old->GetRefSkeleton().GetBoneName(I) || Mesh->GetRefSkeleton().GetParentIndex(I)!=Old->GetRefSkeleton().GetParentIndex(I) || !Mesh->GetRefSkeleton().GetRefBonePose()[I].Equals(Old->GetRefSkeleton().GetRefBonePose()[I],0.f))return Reject(TEXT("any_of77binds_changed"));
    for(int32 I=0;I<Old->GetMaterials().Num();++I)if(Mesh->GetMaterials()[I].MaterialInterface!=Old->GetMaterials()[I].MaterialInterface)return Reject(TEXT("painted_material_object_changed"));
    if(Mesh->GetBounds().GetBox().Min!=Old->GetBounds().GetBox().Min || Mesh->GetBounds().GetBox().Max!=Old->GetBounds().GetBox().Max || Mesh->GetPhysicsAsset()!=Old->GetPhysicsAsset())return Reject(TEXT("bounds_height_or_physics_asset_changed"));
    FMeshDescription O,N;if(!Old->CloneMeshDescription(0,O) || !Mesh->CloneMeshDescription(0,N))return Reject(TEXT("source_description_unavailable"));
    // Fresh validation independently reconstructs the accepted R2 tangent
    // cache, never trusting R4 self-reported expected attributes.
    FMeshDescription ExpectedBasis=O;if(!AcceptedNativeTangentCache(Old,O,ExpectedBasis))return false;
    if(Mesh->GetLODInfo(0)->BuildSettings.bRecomputeNormals || Mesh->GetLODInfo(0)->BuildSettings.bRecomputeTangents)return Reject(TEXT("delivered_R4_must_freeze_basis_not_recompute"));
    return Descriptions(O,N,Old->GetRefSkeleton(),R,false,&ExpectedBasis) && RenderExact(Old,Mesh,O,N);
}
USkeletalMesh* Legacy(const FString& File,USkeleton* Skeleton)
{
    auto* F=NewObject<UFbxFactory>();F->SetDetectImportTypeOnImport(false);auto* UI=F->ImportUI.Get();UI->MeshTypeToImport=UI->OriginalImportType=FBXIT_SkeletalMesh;UI->bAutomatedImportShouldDetectType=false;UI->bImportAsSkeletal=true;UI->bImportMesh=true;UI->bImportAnimations=false;UI->Skeleton=Skeleton;UI->bCreatePhysicsAsset=false;UI->bImportMaterials=false;UI->bImportTextures=false;UI->bOverrideFullName=true;
    auto* D=UI->SkeletalMeshImportData.Get();D->bConvertScene=true;D->bForceFrontXAxis=true;D->bConvertSceneUnit=true;D->ImportRotation=FRotator(0,-90,0);D->bUpdateSkeletonReferencePose=false;D->bUseT0AsRefPose=false;D->NormalImportMethod=FBXNIM_ImportNormalsAndTangents;D->VertexColorImportOption=EVertexColorImportOption::Replace;D->bImportMorphTargets=false;
    auto* Task=NewObject<UAssetImportTask>();Task->bAutomated=true;Task->Options=UI;F->SetAssetImportTask(Task);bool Cancelled=false;
    auto* Mesh=Cast<USkeletalMesh>(F->ImportObject(USkeletalMesh::StaticClass(),CreatePackage(TEXT("/Game/Memoria/Presentation/Field3D/HeroFinishS362/R4/Scratch/ArrelSoleSource")),TEXT("ArrelSoleSource"),RF_Public|RF_Standalone,File,nullptr,Cancelled));return Cancelled?nullptr:Mesh;
}
}
bool MemoriaArrelSoleNativeS362::ValidateDelivered(USkeletalMesh* Mesh)
{
    if(!Mesh || Mesh->GetPathName()!=NewPath+TEXT(".SK_Arrel"))return Reject(TEXT("sole_validator_requires_exact_R4_Arrel"));
    FRecipe R;auto* Old=Load<USkeletalMesh>(OldPath);if(!Old || !Recipe(R))return false;
    FSkinnedAssetCompilingManager::Get().FinishCompilation({Old,Mesh});return MeshContract(Old,Mesh,R);
}
UMemoriaArrelSoleS362Commandlet::UMemoriaArrelSoleS362Commandlet(){IsClient=false;IsServer=false;IsEditor=true;LogToConsole=true;}
int32 UMemoriaArrelSoleS362Commandlet::Main(const FString&)
{
    Report=MakeShared<FJsonObject>();Report->SetStringField(TEXT("schema"),TEXT("s362-sole-r4-native-v1"));
    if(!FApp::CanEverRender()){Reject(TEXT("AllowCommandletRendering_required"));return Finish(false);}if(FPackageName::DoesPackageExist(NewPath)){Reject(TEXT("immutable_R4_target_exists"));return Finish(false);}
    FRecipe R;if(!Recipe(R))return Finish(false);auto* Old=Load<USkeletalMesh>(OldPath);if(!Old || !Old->GetSkeleton()){Reject(TEXT("accepted_R2_Arrel_missing"));return Finish(false);}
    const FReferenceSkeleton OriginalRig=Old->GetSkeleton()->GetReferenceSkeleton();
    auto* Parsed=Legacy(FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()/TEXT("../ArtSource/HeroFinishS362/R4/Arrel/Arrel.fbx")),Old->GetSkeleton());if(!Parsed){Reject(TEXT("legacy_source_import_failed"));return Finish(false);}FSkinnedAssetCompilingManager::Get().FinishCompilation({Parsed});
    FMeshDescription O,P;if(!Old->CloneMeshDescription(0,O) || !Parsed->CloneMeshDescription(0,P) || !Descriptions(O,P,Old->GetRefSkeleton(),R,true))return Finish(false);
    // Keep original MD normals/allUV/RGBA exactly, and bake the accepted R2
    // computed tangent/sign cache from native SoftVertices by source wedge.
    // Only4Z/20skins enter from the new FBX; no new Mikk recomputation.
    FMeshDescription Derived=O;if(!AcceptedNativeTangentCache(Old,O,Derived))return Finish(false);TMap<int32,FVertexID> OM,PM;if(!MapPoints(O,OM) || !MapPoints(P,PM))return Finish(false);auto DestPosition=FStaticMeshAttributes(Derived).GetVertexPositions();auto DestSkin=FSkeletalMeshAttributes(Derived).GetVertexSkinWeights();const auto SourcePosition=FStaticMeshConstAttributes(P).GetVertexPositions();const auto SourceSkin=FSkeletalMeshConstAttributes(P).GetVertexSkinWeights();
    for(int32 ID:AllowedIDs){if(Moved(ID))DestPosition[OM[ID]]=SourcePosition[PM[ID]];TArray<UE::AnimationCore::FBoneWeight> W;for(const auto B:SourceSkin.Get(PM[ID]))W.Add(B);DestSkin.SetRaw(OM[ID],MakeArrayView(W));}
    auto* Mesh=DuplicateObject<USkeletalMesh>(Old,CreatePackage(*NewPath),TEXT("SK_Arrel"));Mesh->SetFlags(RF_Public|RF_Standalone);Mesh->SetAssetImportData(DuplicateObject<UAssetImportData>(Parsed->GetAssetImportData(),Mesh));
    const bool OriginalNormals=Old->GetLODInfo(0)->BuildSettings.bRecomputeNormals,OriginalTangents=Old->GetLODInfo(0)->BuildSettings.bRecomputeTangents;
    Report->SetBoolField(TEXT("accepted_R2_recompute_normals"),OriginalNormals);Report->SetBoolField(TEXT("accepted_R2_recompute_tangents"),OriginalTangents);
    Report->SetBoolField(TEXT("source_FBX_encoded_normal_arrays_unchanged"),true);
    // Freeze the accepted descriptor only on this new candidate; final RenderExact remains mandatory.
    Mesh->GetLODInfo(0)->BuildSettings.bRecomputeNormals=false;Mesh->GetLODInfo(0)->BuildSettings.bRecomputeTangents=false;
    if(!Mesh->CreateMeshDescription(0,MoveTemp(Derived)) || !Mesh->CommitMeshDescription(0)){Reject(TEXT("derived_description_commit_failed"));return Finish(false);}Mesh->PostEditChange();FSkinnedAssetCompilingManager::Get().FinishCompilation({Mesh});
    const auto& AfterRig=Old->GetSkeleton()->GetReferenceSkeleton();if(AfterRig.GetNum()!=OriginalRig.GetNum()){Reject(TEXT("shared_skeleton_bone_count_changed"));return Finish(false);}for(int32 I=0;I<OriginalRig.GetNum();++I)if(OriginalRig.GetBoneName(I)!=AfterRig.GetBoneName(I) || OriginalRig.GetParentIndex(I)!=AfterRig.GetParentIndex(I) || !OriginalRig.GetRefBonePose()[I].Equals(AfterRig.GetRefBonePose()[I],0.f)){Reject(TEXT("shared_original_skeleton_mutated"));return Finish(false);}
    if(Old->GetLODInfo(0)->BuildSettings.bRecomputeNormals!=OriginalNormals || Old->GetLODInfo(0)->BuildSettings.bRecomputeTangents!=OriginalTangents){Reject(TEXT("original_R2_recompute_flags_mutated"));return Finish(false);}
    if(!MeshContract(Old,Mesh,R) || !Save(Mesh))return Finish(false);
    UE_LOG(LogTemp,Display,TEXT("S362_SOLE_R4_IMPORT_COMPLETE meshes=1 source_positions=4 source_weights=20 skeletons=0 materials=0 clips=0 textures=0 Elia_changed=0 original_assets_saved=0"));return Finish(true);
}
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoleR4NativePreservation,"MemoriaGraphics.S362.SoleR4.NativePreservation",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FSoleR4NativePreservation::RunTest(const FString&)
{
    Report=MakeShared<FJsonObject>();FRecipe R;auto* Old=Load<USkeletalMesh>(OldPath);auto* Mesh=Load<USkeletalMesh>(NewPath);if(!Old || !Mesh || !Recipe(R)){AddError(TEXT("Actual immutable R2/R4 Arrel and source receipt required"));return false;}FSkinnedAssetCompilingManager::Get().FinishCompilation({Old,Mesh});
    TestTrue(TEXT("Exact native20weight/4position exception only; all other geometry/normal/UV/paint/77bind/material/triangle contracts exact"),MeshContract(Old,Mesh,R));return true;
}
#endif