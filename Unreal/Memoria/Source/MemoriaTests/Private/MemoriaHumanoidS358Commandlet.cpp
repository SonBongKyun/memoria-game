#include "MemoriaHumanoidS358Commandlet.h"
#include "MemoriaRetargetKit.h"
#include "Factories/FbxFactory.h"
#include "Factories/FbxImportUI.h"
#include "Factories/FbxSkeletalMeshImportData.h"
#include "AssetImportTask.h"
#include "Engine/SkeletalMesh.h"
#include "SkinnedAssetCompiler.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Misc/App.h"
#include "Misc/Paths.h"
using namespace MemoriaRetargetKit;
namespace {
const FString Root=TEXT("/Game/Memoria/Presentation/Field3D/HumanoidBodiesS358/R3/");
struct FPerson {const TCHAR* Name;const TCHAR* Previous;bool Ambient;};
const FPerson People[]={
{TEXT("Traveler"),TEXT("/Game/Memoria/Presentation/Field3D/Traveler/SK_Traveler_Detail"),true},
{TEXT("Bureauagent"),TEXT("/Game/Memoria/Presentation/Field3D/Bureauagent/SK_Bureauagent_Detail"),true},
{TEXT("Guard"),TEXT("/Game/Memoria/Presentation/Field3D/Guard/SK_Guard_Detail"),true},
{TEXT("Villagerf"),TEXT("/Game/Memoria/Presentation/Field3D/Villagerf/SK_Villagerf_Detail"),true},
{TEXT("Villagerm"),TEXT("/Game/Memoria/Presentation/Field3D/Villagerm/SK_Villagerm_Detail"),true},
{TEXT("Fisherman"),TEXT("/Game/Memoria/Presentation/Field3D/Fisherman/SK_Fisherman_Detail"),true},
{TEXT("Elder"),TEXT("/Game/Memoria/Presentation/Field3D/Elder/SK_Elder_Detail"),true},
{TEXT("Child"),TEXT("/Game/Memoria/Presentation/Field3D/Child/SK_Child_Detail"),true},
{TEXT("Scholar"),TEXT("/Game/Memoria/Presentation/Field3D/Scholar/SK_Scholar_Detail"),true},
{TEXT("Elia"),TEXT("/Game/Memoria/Presentation/Field3D/CharacterUpgradeS356/Accepted/Elia/SK_Elia"),false},
{TEXT("Malet"),TEXT("/Game/Memoria/Presentation/Field3D/CharacterUpgradeS356/Accepted/Malet/SK_Malet"),false},
{TEXT("AshWalker"),TEXT("/Game/Memoria/Presentation/Field3D/HumanoidFacesS357/R4/AshWalker/SK_AshWalker"),false},
{TEXT("BeltScavenger"),TEXT("/Game/Memoria/Presentation/Field3D/HumanoidFacesS357/R4/BeltScavenger/SK_BeltScavenger"),false},
};
FString Asset(const FPerson& P){return Root+P.Name+TEXT("/SK_")+P.Name+(P.Ambient?TEXT("_Detail"):TEXT(""));}
USkeletalMesh* Import(const FString& File,const FString& Path,USkeleton* Skeleton)
{
    if(FPackageName::DoesPackageExist(Path))return nullptr;
    auto* Factory=NewObject<UFbxFactory>();Factory->SetDetectImportTypeOnImport(false);auto* UI=Factory->ImportUI.Get();
    UI->MeshTypeToImport=UI->OriginalImportType=FBXIT_SkeletalMesh;UI->bAutomatedImportShouldDetectType=false;
    UI->bImportAsSkeletal=true;UI->bImportMesh=true;UI->bImportAnimations=false;UI->Skeleton=Skeleton;
    UI->bCreatePhysicsAsset=false;UI->bImportMaterials=false;UI->bImportTextures=false;UI->bOverrideFullName=true;
    auto* Data=UI->SkeletalMeshImportData.Get();Data->bConvertScene=true;Data->bForceFrontXAxis=true;Data->bConvertSceneUnit=true;Data->ImportRotation=FRotator(0,-90,0);
    Data->bUseT0AsRefPose=false;Data->NormalImportMethod=FBXNIM_ImportNormalsAndTangents;Data->VertexColorImportOption=EVertexColorImportOption::Replace;
    auto* Task=NewObject<UAssetImportTask>();Task->bAutomated=true;Task->Options=UI;Factory->SetAssetImportTask(Task);
    bool Cancelled=false;auto* Mesh=Cast<USkeletalMesh>(Factory->ImportObject(USkeletalMesh::StaticClass(),CreatePackage(*Path),*FPackageName::GetShortName(Path),RF_Public|RF_Standalone,File,nullptr,Cancelled));
    return Cancelled?nullptr:Mesh;
}
}
UMemoriaHumanoidS358Commandlet::UMemoriaHumanoidS358Commandlet(){IsClient=false;IsServer=false;IsEditor=true;LogToConsole=true;}
int32 UMemoriaHumanoidS358Commandlet::Main(const FString& Params)
{
 if(!FApp::CanEverRender())return 1;
 const FString Art=FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()/TEXT("../ArtSource/HumanoidBodiesS358/R3"));
 for(const auto& P:People)
 {
  auto* Old=Load<USkeletalMesh>(P.Previous);
  if(!Old || !Old->GetSkeleton() || Old->GetRefSkeleton().GetNum()!=77 || FPackageName::DoesPackageExist(Asset(P)) || !IFileManager::Get().FileExists(*(Art/P.Name/(FString(P.Name)+TEXT(".fbx")))))return 1;
 }
 for(const auto& P:People)
 {
  auto* Old=Load<USkeletalMesh>(P.Previous);
  auto* Mesh=Import(Art/P.Name/(FString(P.Name)+TEXT(".fbx")),Asset(P),Old->GetSkeleton());
  if(!Mesh || Mesh->GetSkeleton()!=Old->GetSkeleton())return 1;
  FSkinnedAssetCompilingManager::Get().FinishCompilation({Mesh});
  const auto& A=Mesh->GetRefSkeleton();const auto& B=Old->GetRefSkeleton();
  if(A.GetNum()!=77 || Mesh->GetMaterials().Num()!=Old->GetMaterials().Num())return 1;
  for(int32 I=0;I<77;++I)
   if(A.GetBoneName(I)!=B.GetBoneName(I) || A.GetParentIndex(I)!=B.GetParentIndex(I) || !A.GetRefBonePose()[I].Equals(B.GetRefBonePose()[I],.015f))return 1;
  for(int32 I=0;I<Mesh->GetMaterials().Num();++I)Mesh->GetMaterials()[I].MaterialInterface=Old->GetMaterials()[I].MaterialInterface;
  Mesh->PostEditChange();FSkinnedAssetCompilingManager::Get().FinishCompilation({Mesh});
  const auto* Render=Mesh->GetResourceForRendering();if(!Render || Render->LODRenderData.IsEmpty())return 1;
  int32 Triangles=Render->LODRenderData[0].MultiSizeIndexContainer.GetIndexBuffer()->Num()/3;
  if(Triangles<2000 || Triangles>40000 || !FMath::IsNearlyEqual(Mesh->GetBounds().BoxExtent.Z*2,Old->GetBounds().BoxExtent.Z*2,.15f))return 1;
  for(int32 I=0;I<Mesh->GetMaterials().Num();++I)
  {int32 Count=0;for(const auto& S:Render->LODRenderData[0].RenderSections)if(S.MaterialIndex==I)Count+=S.NumTriangles;if(Count==0)return 1;}
  if(!Save(Mesh))return 1;
  UE_LOG(LogTemp,Display,TEXT("S358_IMPORTED name=%s bones=77 slots=%d triangles=%d height=%.5f original_materials=1"),P.Name,Mesh->GetMaterials().Num(),Triangles,Mesh->GetBounds().BoxExtent.Z*2);
 }
 UE_LOG(LogTemp,Display,TEXT("S358_IMPORT_COMPLETE meshes=13 new_materials=0 new_textures=0"));return 0;
}
