#include "MemoriaPerformanceInventoryS362.h"
#include "Engine/World.h"
#include "Engine/GameViewportClient.h"
#include "UnrealClient.h"
#include "Misc/App.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/Texture2D.h"
#include "GameFramework/Actor.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Rendering/SkeletalMeshLODRenderData.h"
#include "StaticMeshResources.h"
#include "UObject/UObjectIterator.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

namespace
{
TSharedPtr<FJsonValue> Number(double V) { return MakeShared<FJsonValueNumber>(V); }
TSharedPtr<FJsonValue> Object(const TSharedPtr<FJsonObject>& V) { return MakeShared<FJsonValueObject>(V); }
}
bool MemoriaPerformanceInventoryS362::WriteWorld(UWorld* World,const FString& File)
{
    if (!World) return false;
    TMap<FString,UStaticMesh*> StaticAssets; TMap<FString,USkeletalMesh*> SkinnedAssets;
    TMap<FString,UTexture2D*> Textures;
    TArray<TSharedPtr<FJsonValue>> Components,StaticRows,SkinnedRows,TextureRows,LoadedTextureRows;
    int32 VisiblePrimitives=0,RegisteredPrimitives=0,VisibleSkin=0,VisibleStatic=0;
    int64 VisibleInstances=0;
    for (TObjectIterator<UPrimitiveComponent> It;It;++It)
    {
        UPrimitiveComponent* C=*It;
        if (C->GetWorld()!=World || C->IsTemplate() || !C->IsRegistered()) continue;
        ++RegisteredPrimitives;
        const bool Visible=C->IsVisible() && !C->bHiddenInGame && (!C->GetOwner() || !C->GetOwner()->IsHidden());
        if (Visible) ++VisiblePrimitives;
        auto Row=MakeShared<FJsonObject>(); Row->SetStringField(TEXT("component"),C->GetPathName());
        Row->SetBoolField(TEXT("visible_flag"),Visible); Row->SetStringField(TEXT("class"),C->GetClass()->GetName());
        Row->SetNumberField(TEXT("materials"),C->GetNumMaterials());
        if (auto* Static=Cast<UStaticMeshComponent>(C); Static && Static->GetStaticMesh())
        {
            UStaticMesh* Mesh=Static->GetStaticMesh(); StaticAssets.Add(Mesh->GetPathName(),Mesh);
            Row->SetStringField(TEXT("asset"),Mesh->GetPathName());
            const auto* Ism=Cast<UInstancedStaticMeshComponent>(Static);
            const int32 Count=Ism ? Ism->GetInstanceCount() : 1;
            Row->SetNumberField(TEXT("instances"),Count);
            if (Visible) { ++VisibleStatic; VisibleInstances+=Count; }
        }
        if (auto* Skin=Cast<USkeletalMeshComponent>(C); Skin && Skin->GetSkeletalMeshAsset())
        {
            USkeletalMesh* Mesh=Skin->GetSkeletalMeshAsset(); SkinnedAssets.Add(Mesh->GetPathName(),Mesh);
            Row->SetStringField(TEXT("asset"),Mesh->GetPathName());
            Row->SetNumberField(TEXT("predicted_lod"),Skin->GetPredictedLODLevel());
            if (Visible) ++VisibleSkin;
        }
        for (int32 I=0;I<C->GetNumMaterials();++I)
            if (auto* M=C->GetMaterial(I))
            {
                TArray<UTexture*> Used; M->GetUsedTextures(Used);
                for (auto* T : Used) if (auto* T2=Cast<UTexture2D>(T)) Textures.Add(T2->GetPathName(),T2);
            }
        Components.Add(Object(Row));
    }
    TArray<FString> Keys; StaticAssets.GenerateKeyArray(Keys); Keys.Sort();
    for (const auto& Key : Keys)
    {
        auto* Mesh=StaticAssets[Key]; auto Row=MakeShared<FJsonObject>(); Row->SetStringField(TEXT("asset"),Key);
        Row->SetBoolField(TEXT("transient"),Mesh->HasAnyFlags(RF_Transient));
        const auto* Data=Mesh->GetRenderData(); Row->SetBoolField(TEXT("render_data_available"),Data!=nullptr);
        TArray<TSharedPtr<FJsonValue>> Triangles,Vertices;
        if (Data) for (const auto& LOD : Data->LODResources)
        { Triangles.Add(Number(LOD.GetNumTriangles())); Vertices.Add(Number(LOD.GetNumVertices())); }
        Row->SetNumberField(TEXT("lod_count"),Triangles.Num()); Row->SetArrayField(TEXT("triangles_by_lod"),Triangles); Row->SetArrayField(TEXT("vertices_by_lod"),Vertices);
        StaticRows.Add(Object(Row));
    }
    Keys.Reset(); SkinnedAssets.GenerateKeyArray(Keys); Keys.Sort();
    for (const auto& Key : Keys)
    {
        auto* Mesh=SkinnedAssets[Key]; auto Row=MakeShared<FJsonObject>(); Row->SetStringField(TEXT("asset"),Key);
        Row->SetBoolField(TEXT("transient"),Mesh->HasAnyFlags(RF_Transient));
        const auto* Data=Mesh->GetResourceForRendering(); Row->SetBoolField(TEXT("render_data_available"),Data!=nullptr);
        TArray<TSharedPtr<FJsonValue>> Triangles,Vertices;
        if (Data) for (const auto& LOD : Data->LODRenderData)
        { Triangles.Add(Number(LOD.GetTotalFaces())); Vertices.Add(Number(LOD.GetNumVertices())); }
        Row->SetNumberField(TEXT("lod_count"),Triangles.Num()); Row->SetArrayField(TEXT("triangles_by_lod"),Triangles); Row->SetArrayField(TEXT("vertices_by_lod"),Vertices);
        Row->SetNumberField(TEXT("bones"),Mesh->GetRefSkeleton().GetNum()); SkinnedRows.Add(Object(Row));
    }
    Keys.Reset(); Textures.GenerateKeyArray(Keys); Keys.Sort();
    for (const auto& Key : Keys)
    {
        auto* T=Textures[Key]; auto Row=MakeShared<FJsonObject>(); Row->SetStringField(TEXT("asset"),Key);
        Row->SetBoolField(TEXT("transient"),T->HasAnyFlags(RF_Transient) || T->GetOutermost()==GetTransientPackage());
        Row->SetNumberField(TEXT("width"),T->GetSizeX()); Row->SetNumberField(TEXT("height"),T->GetSizeY());
        Row->SetNumberField(TEXT("mips"),T->GetNumMips()); Row->SetNumberField(TEXT("resident_mips"),T->GetNumResidentMips());
        Row->SetBoolField(TEXT("never_stream"),T->NeverStream); TextureRows.Add(Object(Row));
    }
    // UI brushes and editor assets need an explicitly separate process-wide
    // view; they are not mistaken for this world's material texture workload.
    int32 LoadedNeverStream=0;
    for (TObjectIterator<UTexture2D> It;It;++It)
    {
        auto* T=*It; if (T->IsTemplate()) continue;
        auto Row=MakeShared<FJsonObject>(); Row->SetStringField(TEXT("asset"),T->GetPathName());
        Row->SetBoolField(TEXT("transient"),T->HasAnyFlags(RF_Transient) || T->GetOutermost()==GetTransientPackage());
        Row->SetNumberField(TEXT("width"),T->GetSizeX()); Row->SetNumberField(TEXT("height"),T->GetSizeY());
        Row->SetNumberField(TEXT("mips"),T->GetNumMips()); Row->SetNumberField(TEXT("resident_mips"),T->GetNumResidentMips());
        Row->SetBoolField(TEXT("never_stream"),T->NeverStream); Row->SetBoolField(TEXT("used_by_world_material"),Textures.Contains(T->GetPathName()));
        if (T->NeverStream) ++LoadedNeverStream;
        LoadedTextureRows.Add(Object(Row));
    }
    auto Root=MakeShared<FJsonObject>(); Root->SetStringField(TEXT("map"),World->GetMapName());
    Root->SetStringField(TEXT("scope"),TEXT("actual registered PIE-world components and their loaded binary assets; visibility flag is not GPU visibility"));
    Root->SetStringField(TEXT("missing_render_data"),TEXT("explicitly marked unavailable; zero LOD count must not be read as zero geometry"));
    if (const auto* Client=World->GetGameViewport(); Client && Client->Viewport)
    {
        const auto Size=Client->Viewport->GetSizeXY(), Target=Client->Viewport->GetRenderTargetTextureSizeXY();
        auto Pixels=MakeShared<FJsonObject>();
        Pixels->SetNumberField(TEXT("viewport_width"),Size.X); Pixels->SetNumberField(TEXT("viewport_height"),Size.Y);
        Pixels->SetNumberField(TEXT("render_target_width"),Target.X); Pixels->SetNumberField(TEXT("render_target_height"),Target.Y);
        Pixels->SetBoolField(TEXT("fixed_timestep"),FApp::UseFixedTimeStep()); Root->SetObjectField(TEXT("pixels"),Pixels);
    }
    Root->SetNumberField(TEXT("registered_primitives"),RegisteredPrimitives); Root->SetNumberField(TEXT("visible_primitives"),VisiblePrimitives);
    Root->SetNumberField(TEXT("visible_skinned_components"),VisibleSkin); Root->SetNumberField(TEXT("visible_static_components"),VisibleStatic);
    Root->SetNumberField(TEXT("visible_static_instances"),double(VisibleInstances));
    Root->SetArrayField(TEXT("components"),Components); Root->SetArrayField(TEXT("static_meshes"),StaticRows);
    Root->SetArrayField(TEXT("skeletal_meshes"),SkinnedRows); Root->SetArrayField(TEXT("material_textures"),TextureRows);
    Root->SetArrayField(TEXT("process_loaded_textures_including_ui_and_editor"),LoadedTextureRows);
    Root->SetNumberField(TEXT("process_loaded_never_stream_textures"),LoadedNeverStream);
    FString Json; auto Writer=TJsonWriterFactory<>::Create(&Json); if (!FJsonSerializer::Serialize(Root,Writer)) return false;
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(File),true);
    return FFileHelper::SaveStringToFile(Json,*File);
}
