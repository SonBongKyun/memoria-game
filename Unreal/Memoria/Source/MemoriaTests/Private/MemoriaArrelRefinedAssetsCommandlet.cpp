#include "MemoriaArrelRefinedAssetsCommandlet.h"
#include "Animation/Skeleton.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/Material.h"
#include "MeshDescription.h"
#include "SkeletalMeshAttributes.h"
#include "StaticToSkeletalMeshConverter.h"
#include "SkinnedAssetCompiler.h"
#include "ReferenceSkeleton.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/PackageName.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/SavePackage.h"
namespace
{
const FString Base=TEXT("/Game/Memoria/Presentation/Character2/");
template<class T> T* Asset(const TCHAR* N)
{ return NewObject<T>(CreatePackage(*(Base+N)),N,RF_Public|RF_Standalone); }
bool Save(UObject* O)
{
    FAssetRegistryModule::AssetCreated(O);O->MarkPackageDirty();
    FSavePackageArgs A;A.TopLevelFlags=RF_Public|RF_Standalone;
    const bool OK=UPackage::SavePackage(O->GetOutermost(),O,*FPackageName::LongPackageNameToFilename(O->GetOutermost()->GetName(),FPackageName::GetAssetPackageExtension()),A);
    UE_LOG(LogTemp,Display,TEXT("ARREL_ASSET %s %s"),*O->GetPathName(),OK?TEXT("SAVED"):TEXT("FAILED"));return OK;
}
FVector Position(const TArray<TSharedPtr<FJsonValue>>& A)
{ return FVector(A[0]->AsNumber(),A[1]->AsNumber(),A[2]->AsNumber()); }
}
UMemoriaArrelRefinedAssetsCommandlet::UMemoriaArrelRefinedAssetsCommandlet()
{ IsClient=false;IsServer=false;IsEditor=true;LogToConsole=true; }
int32 UMemoriaArrelRefinedAssetsCommandlet::Main(const FString& ParamsText)
{
    for (const TCHAR* N:{TEXT("SK_ArrelRefined"),TEXT("SKEL_ArrelRefined")})
        if(FPackageName::DoesPackageExist(Base+N)){UE_LOG(LogTemp,Error,TEXT("Refusing existing Arrel package %s"),N);return 1;}
    FString Text;TSharedPtr<FJsonObject> Source;
    if(!FFileHelper::LoadFileToString(Text,*(FPaths::ProjectDir()/TEXT("../../Unreal/ArtSource/Arrel3D/arrel_refined.v2.json"))) || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Source) || !Source.IsValid())return 1;
    if(Source->GetIntegerField(TEXT("schema"))!=1 || Source->GetIntegerField(TEXT("revision"))!=2)return 1;
    const auto& Bones=Source->GetArrayField(TEXT("bones"));const auto& Vertices=Source->GetArrayField(TEXT("vertices"));const auto& Triangles=Source->GetArrayField(TEXT("triangles"));
    if(Bones.Num()!=23 || Vertices.IsEmpty() || Triangles.IsEmpty())return 1;
    FReferenceSkeleton Ref;TArray<FVector> Origins;
    {
    FReferenceSkeletonModifier Modifier(Ref,nullptr);
    for(int32 I=0;I<Bones.Num();++I)
    {
        auto B=Bones[I]->AsObject();const int32 Parent=B->GetIntegerField(TEXT("parent"));
        if(Parent>=I || Parent<INDEX_NONE || (I>0 && Parent==INDEX_NONE))return 1;
        const FVector P=Position(B->GetArrayField(TEXT("position")));Origins.Add(P);
        const FString Name=B->GetStringField(TEXT("name"));
        Modifier.Add(FMeshBoneInfo(FName(*Name),Name,Parent),FTransform(P-(Parent>=0?Origins[Parent]:FVector::ZeroVector)));
    }
    } // Finalize the reference skeleton before the mesh converter reads it.
    if(Ref.GetNum()!=Bones.Num())return 1;
    FMeshDescription D;FSkeletalMeshAttributes A(D);A.Register();A.GetVertexInstanceUVs().SetNumChannels(1);
    const FPolygonGroupID Fabric=D.CreatePolygonGroup(),Steel=D.CreatePolygonGroup();
    A.GetPolygonGroupMaterialSlotNames()[Fabric]=TEXT("Fabric");A.GetPolygonGroupMaterialSlotNames()[Steel]=TEXT("Steel");
    TArray<FVertexID> IDs;TArray<FVector4f> Colors;
    for(const auto& Row:Vertices)
    {
        const auto& R=Row->AsArray();if(R.Num()!=5)return 1;
        const auto ID=D.CreateVertex();IDs.Add(ID);A.GetVertexPositions()[ID]=FVector3f(Position(R));
        const auto& C=R[3]->AsArray();if(C.Num()!=3)return 1;Colors.Add(FVector4f(C[0]->AsNumber(),C[1]->AsNumber(),C[2]->AsNumber(),1));
        TArray<UE::AnimationCore::FBoneWeight> Weights;
        double Total=0;
        for(const auto& W:R[4]->AsArray())
        {
            const auto& Pair=W->AsArray();if(Pair.Num()!=2)return 1;
            const int32 B=Pair[0]->AsNumber();const float Weight=Pair[1]->AsNumber();
            if(B<0 || B>=Bones.Num() || Weight<0 || !FMath::IsFinite(Weight))return 1;
            if(Weight>0)Weights.Emplace(B,Weight);Total+=Weight;
        }
        if(!FMath::IsNearlyEqual(Total,1.0,.001))return 1;
        A.GetVertexSkinWeights().Set(ID,MakeArrayView(Weights));
    }
    for(const auto& Row:Triangles)
    {
        const auto& R=Row->AsArray();if(R.Num()!=4)return 1;
        const int32 Slot=R[3]->AsNumber();if(Slot<0 || Slot>1)return 1;
        TArray<FVertexInstanceID> Corners;
        FVector3f P[3];
        for(int32 I=0;I<3;++I){const int32 Index=R[I]->AsNumber();if(!IDs.IsValidIndex(Index))return 1;P[I]=A.GetVertexPositions()[IDs[Index]];}
        const FVector3f N=FVector3f::CrossProduct(P[1]-P[0],P[2]-P[0]).GetSafeNormal();
        if(N.IsNearlyZero())return 1;
        FVector3f Tangent=FVector3f::CrossProduct(FMath::Abs(N.Z)>.9f?FVector3f(0,1,0):FVector3f(0,0,1),N).GetSafeNormal();
        for(int32 I=0;I<3;++I)
        {
            const int32 Index=R[I]->AsNumber();auto V=D.CreateVertexInstance(IDs[Index]);Corners.Add(V);
            A.GetVertexInstanceNormals()[V]=N;A.GetVertexInstanceTangents()[V]=Tangent;A.GetVertexInstanceBinormalSigns()[V]=1;
            A.GetVertexInstanceColors()[V]=Colors[Index];A.GetVertexInstanceUVs().Set(V,0,FVector2f(I==1?1:0,I==2?1:0));
        }
        D.CreatePolygon(Slot==0?Fabric:Steel,Corners);
    }
    auto* FabricMat=LoadObject<UMaterial>(nullptr,TEXT("/Game/Memoria/Presentation/Character1/M_ArrelFabric.M_ArrelFabric"));
    auto* SteelMat=LoadObject<UMaterial>(nullptr,TEXT("/Game/Memoria/Presentation/Character1/M_ArrelSteel.M_ArrelSteel"));
    if(!FabricMat || !SteelMat)return 1;
    auto* Mesh=Asset<USkeletalMesh>(TEXT("SK_ArrelRefined"));auto* Skeleton=Asset<USkeleton>(TEXT("SKEL_ArrelRefined"));
    TArray<FSkeletalMaterial> Mats;Mats.Add(FSkeletalMaterial(FabricMat,true,false,TEXT("Fabric"),TEXT("Fabric")));Mats.Add(FSkeletalMaterial(SteelMat,true,false,TEXT("Steel"),TEXT("Steel")));
    FStaticToSkeletalMeshConverter::FInitializationParams Params;Params.Materials=Mats;Params.bRecomputeNormals=false;Params.bRecomputeTangents=false;
    TArray<const FMeshDescription*> Descriptions{&D};
    if(!FStaticToSkeletalMeshConverter::InitializeSkeletalMeshFromMeshDescriptions(Mesh,Descriptions,Ref,Params))return 1;
    Mesh->SetSkeleton(Skeleton);Skeleton->MergeAllBonesToBoneTree(Mesh);Skeleton->SetPreviewMesh(Mesh);
    Mesh->SetHasVertexColors(true);Mesh->SetVertexColorGuid(FGuid::NewGuid());Mesh->PostEditChange();
    FSkinnedAssetCompilingManager::Get().FinishCompilation({Mesh});
    UE_LOG(LogTemp,Display,TEXT("ARREL_GEOMETRY bones=%d vertices=%d triangles=%d"),Bones.Num(),Vertices.Num(),Triangles.Num());
    return Save(Skeleton)&&Save(Mesh)?0:1;
}
