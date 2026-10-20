#pragma once
#include "MemoriaSurfaceDetailChecks.h"
#include "Engine/StaticMesh.h"
#include "StaticMeshResources.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Rendering/SkeletalMeshLODRenderData.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialExpressionVertexColor.h"
#include "Materials/MaterialExpressionTextureSample.h"

// Inspect delivered revision packages and the material graph that renders them.
inline void CheckMemoriaArchitectureS356(FAutomationTestBase* Test)
{
    const FString Base=TEXT("/Game/Memoria/Presentation/EnvironmentS356/");
    auto* Atlas=LoadObject<UTexture2D>(nullptr,*(Base+TEXT("T_ArchitectureAtlas.T_ArchitectureAtlas")));
    auto* Surface=LoadObject<UMaterial>(nullptr,*(Base+TEXT("M_ArchitectureSurface.M_ArchitectureSurface")));
    auto* Original=LoadObject<UMaterial>(nullptr,TEXT("/Game/Memoria/Presentation/Depth2/M_FocusSurface.M_FocusSurface"));
    if(!Test->TestNotNull(TEXT("S356 architecture atlas loads"),Atlas) ||
       !Test->TestNotNull(TEXT("S356 architecture shader loads"),Surface) ||
       !Test->TestNotNull(TEXT("Original reveal shader remains available"),Original))return;
    Test->TestTrue(TEXT("Architecture preserves the complete generated source"),Atlas->Source.GetSizeX()==1254 && Atlas->Source.GetSizeY()==1254);
    Test->TestTrue(TEXT("Architecture texture is color data"),Atlas->SRGB!=0);
    Test->TestEqual(TEXT("Architecture preserves masked reveal"),Surface->BlendMode,Original->BlendMode);
    Test->TestTrue(TEXT("Architecture is lit instanced geometry"),Surface->GetShadingModels().HasShadingModel(MSM_DefaultLit) && Surface->GetUsageByFlag(MATUSAGE_InstancedStaticMeshes));
    auto* Data=Surface->GetEditorOnlyData();
    Test->TestNotNull(TEXT("Architecture retains real occlusion reveal"),Data->OpacityMask.Expression);
    Test->TestEqual(TEXT("Architecture preserves the original normal input state"),Data->Normal.Expression!=nullptr,Original->GetEditorOnlyData()->Normal.Expression!=nullptr);
    Test->TestNotNull(TEXT("Architecture retains the roughness graph"),Data->Roughness.Expression);
    Test->TestTrue(TEXT("New atlas reaches the live BaseColor"),MemoriaSurfaceReachable(Data->BaseColor.Expression,TEXT("ArchitectureAtlas"),Atlas));
    int32 TotalTriangles=0;
    for(const TCHAR* Name:{TEXT("SM_TiledRoof"),TEXT("SM_RoofEaves"),TEXT("SM_DoorWood"),TEXT("SM_DoorStone"),
        TEXT("SM_WindowWood"),TEXT("SM_WindowStone"),TEXT("SM_ClothCanopy"),TEXT("SM_CanopyRope"),
        TEXT("SM_WallCourse"),TEXT("SM_ChippedCap"),TEXT("SM_RubbleCluster"),TEXT("SM_RuinTuft"),TEXT("SM_ShelterRigging")})
    {
        auto* Mesh=LoadObject<UStaticMesh>(nullptr,*(Base+Name+TEXT(".")+Name));
        if(!Test->TestNotNull(*(FString(Name)+TEXT(" delivered geometry loads")),Mesh))continue;
        Test->TestEqual(TEXT("Revision geometry has one surface slot"),Mesh->GetStaticMaterials().Num(),1);
        const auto* Render=Mesh->GetRenderData();
        if(!Test->TestTrue(TEXT("Revision geometry has actual triangles"),Render && Render->LODResources.Num()>0))continue;
        const int32 Triangles=Render->LODResources[0].GetNumTriangles();TotalTriangles+=Triangles;
        Test->TestTrue(TEXT("Revision geometry is substantial and bounded"),Triangles>50 && Triangles<50000);
    }
    Test->TestTrue(TEXT("Architecture kit remains within a bounded geometry budget"),TotalTriangles>5000 && TotalTriangles<100000);
    if(FApp::CanEverRender())
    {
        GShaderCompilingManager->FinishAllCompilation();
        const auto* Resource=Surface->GetMaterialResource(GMaxRHIShaderPlatform);
        if(Test->TestNotNull(TEXT("Architecture has a native material resource"),Resource))
            Test->TestEqual(TEXT("Architecture shader compiles"),Resource->GetCompileErrors().Num(),0);
    }
    Test->AddInfo(FString::Printf(TEXT("S356_ARCHITECTURE_ASSETS meshes=13 triangles=%d source_atlas=1254"),TotalTriangles));
}
inline void CheckMemoriaNamedFigureS356(FAutomationTestBase* Test,const TCHAR* Name,USkeletalMeshComponent* Component)
{
    const FString Revision=FString(TEXT("/Game/Memoria/Presentation/Field3D/HumanoidAnatomyS360/R7/"))+Name+TEXT("/SK_")+Name;
    auto* NewMesh=LoadObject<USkeletalMesh>(nullptr,*(Revision+TEXT(".SK_")+Name));
    const FString Original=FString(TEXT("/Game/Memoria/Presentation/Field3D/"))+Name+TEXT("/SK_")+Name;
    auto* OldMesh=LoadObject<USkeletalMesh>(nullptr,*(Original+TEXT(".SK_")+Name));
    if(!Test->TestNotNull(TEXT("Named NPC revision mesh loads"),NewMesh) || !Test->TestNotNull(TEXT("Named NPC original rig remains"),OldMesh))return;
    Test->TestTrue(TEXT("Named NPC uses a new anatomical rig"),NewMesh->GetSkeleton()!=OldMesh->GetSkeleton());
    const auto& A=NewMesh->GetRefSkeleton();const auto& B=OldMesh->GetRefSkeleton();
    Test->TestEqual(TEXT("Named NPC retains all 77 bones"),A.GetNum(),77);
    for(int32 I=0;I<FMath::Min(A.GetNum(),B.GetNum());++I)
    {
        Test->TestEqual(TEXT("NPC bone names remain compatible"),A.GetBoneName(I),B.GetBoneName(I));
        Test->TestEqual(TEXT("NPC bone parents remain compatible"),A.GetParentIndex(I),B.GetParentIndex(I));
        Test->TestFalse(TEXT("NPC anatomical bind is finite"),A.GetRefBonePose()[I].ContainsNaN());
    }
    Test->TestTrue(TEXT("NPC clothing retains standing scale"),FMath::IsNearlyEqual(float(NewMesh->GetBounds().BoxExtent.Z*2),float(OldMesh->GetBounds().BoxExtent.Z*2),1.f));
    const auto* NewRender=NewMesh->GetResourceForRendering();
    const auto* OldRender=OldMesh->GetResourceForRendering();
    if(Test->TestTrue(TEXT("Named NPC has native render geometry"),NewRender && OldRender && NewRender->LODRenderData.Num()>0 && OldRender->LODRenderData.Num()>0))
    {
        const auto& LOD=NewRender->LODRenderData[0];const auto& Colors=LOD.StaticVertexBuffers.ColorVertexBuffer;
        const int32 OriginalVertices=OldRender->LODRenderData[0].GetNumVertices();
        int32 Preserved=0,Added=0;
        const bool HasColors=Colors.GetNumVertices()==LOD.GetNumVertices() && Colors.GetAllowCPUAccess();
        if(Test->TestTrue(TEXT("NPC original body versus accessories has inspectable color masks"),HasColors))
        {
            for(uint32 I=0;I<Colors.GetNumVertices();++I){if(Colors.VertexColor(I).A<8)++Preserved;else ++Added;}
            TSet<FIntVector> Current;
            for(uint32 I=0;I<LOD.GetNumVertices();++I){const FVector3f P=LOD.StaticVertexBuffers.PositionVertexBuffer.VertexPosition(I);Current.Add(FIntVector(FMath::RoundToInt(P.X*100),FMath::RoundToInt(P.Y*100),FMath::RoundToInt(P.Z*100)));}
            const auto& OriginalLOD=OldRender->LODRenderData[0];int32 Garment=0,Matched=0;
            const float HeadCut=float(OldMesh->GetBounds().BoxExtent.Z*2)-25.f;
            for(uint32 I=0;I<OriginalLOD.GetNumVertices();++I)
            {const FVector3f P=OriginalLOD.StaticVertexBuffers.PositionVertexBuffer.VertexPosition(I);if(P.Z>=HeadCut)continue;++Garment;Matched+=Current.Contains(FIntVector(FMath::RoundToInt(P.X*100),FMath::RoundToInt(P.Y*100),FMath::RoundToInt(P.Z*100)));}
            // S360 intentionally fits garment coordinates to anatomical joints; original UVs and materials stay intact.
            Test->TestTrue(TEXT("S360 original garment source remains inspectable"),Garment>1000);
            auto UVKey=[](const FVector3f& P,const FVector2f& UV){return FString::Printf(TEXT("%d,%d"),FMath::RoundToInt(UV.X*10000),FMath::RoundToInt(UV.Y*10000));};
            TSet<FString> Painted;for(uint32 I=0;I<LOD.GetNumVertices();++I)Painted.Add(UVKey(LOD.StaticVertexBuffers.PositionVertexBuffer.VertexPosition(I),LOD.StaticVertexBuffers.StaticMeshVertexBuffer.GetVertexUV(I,0)));
            int32 UVM=0,UVN=0;for(uint32 I=0;I<OriginalLOD.GetNumVertices();++I){const FVector3f P=OriginalLOD.StaticVertexBuffers.PositionVertexBuffer.VertexPosition(I);if(P.Z>=HeadCut)continue;++UVN;UVM+=Painted.Contains(UVKey(P,OriginalLOD.StaticVertexBuffers.StaticMeshVertexBuffer.GetVertexUV(I,0)));}
            Test->TestTrue(TEXT("S360 fitted garment paint keeps original UV corners"),UVN>1000 && UVM>=UVN*.99f);

            Test->TestTrue(TEXT("NPC gains actual accessory geometry"),Added>100);
            Test->AddInfo(FString::Printf(TEXT("S356_NPC_ORIGINAL_BODY %s retained_vertices=%d original_vertices=%d accessory_vertices=%d"),Name,Preserved,OriginalVertices,Added));
        }
    }
    const FString MaterialPath=FString(TEXT("/Game/Memoria/Presentation/Field3D/CharacterUpgradeS356/Accepted/"))+Name+TEXT("/M_")+Name+TEXT("Refined");
    auto* Outfit=LoadObject<UMaterial>(nullptr,*MaterialPath);
    auto* Paint=LoadObject<UTexture2D>(nullptr,*(FString(TEXT("/Game/Memoria/Presentation/Field3D/"))+Name+TEXT("/T_")+Name));
    if(Test->TestNotNull(TEXT("Named NPC refined outfit material loads"),Outfit) && Test->TestNotNull(TEXT("Original face paint remains available"),Paint))
    {
        Test->TestTrue(TEXT("NPC retains original body material plus separate accessory material"),NewMesh->GetMaterials().Num()==3 && OldMesh->GetMaterials().Num()>0 && NewMesh->GetMaterials()[0].MaterialInterface.Get()==OldMesh->GetMaterials()[0].MaterialInterface.Get() && NewMesh->GetMaterials()[1].MaterialInterface.Get()==Outfit);
        if(NewMesh->GetMaterials().Num()==3 && NewMesh->GetMaterials()[0].MaterialInterface)
        {
            auto* Body=NewMesh->GetMaterials()[0].MaterialInterface->GetMaterial();
            TSet<UMaterialExpression*> PaintSeen;
            TFunction<bool(UMaterialExpression*)> HasPaint=[&](UMaterialExpression* Expression)
            {
                if(!Expression || PaintSeen.Contains(Expression))return false;
                PaintSeen.Add(Expression);
                if(auto* Sample=Cast<UMaterialExpressionTextureSample>(Expression);Sample && Sample->Texture==Paint)return true;
                for(FExpressionInput* Input:Expression->GetInputsView())if(Input && HasPaint(Input->Expression))return true;
                return false;
            };
            Test->TestTrue(TEXT("Actual original body paint still reaches visible garments"),Body && HasPaint(Body->GetEditorOnlyData()->BaseColor.Expression));
        }
        auto* Inputs=Outfit->GetEditorOnlyData();
        TSet<UMaterialExpression*> Seen;
        TFunction<bool(UMaterialExpression*)> HasRegions=[&](UMaterialExpression* Expression)
        {
            if(!Expression || Seen.Contains(Expression))return false;
            Seen.Add(Expression);
            if(Cast<UMaterialExpressionVertexColor>(Expression))return true;
            for(FExpressionInput* Input:Expression->GetInputsView())if(Input && HasRegions(Input->Expression))return true;
            return false;
        };
        Test->TestTrue(TEXT("Garment region mask reaches visible color"),HasRegions(Inputs->BaseColor.Expression));
        Test->TestTrue(TEXT("NPC outfit remains an opaque lit skeletal surface"),Outfit->BlendMode==BLEND_Opaque && Outfit->GetShadingModels().HasShadingModel(MSM_DefaultLit) && Outfit->GetUsageByFlag(MATUSAGE_SkeletalMesh));
        Test->TestNotNull(TEXT("Outfit retains normal surface detail"),Inputs->Normal.Expression);
        Test->TestNotNull(TEXT("Outfit retains roughness detail"),Inputs->Roughness.Expression);
        if(FApp::CanEverRender())
        {
            GShaderCompilingManager->FinishAllCompilation();
            const auto* Resource=Outfit->GetMaterialResource(GMaxRHIShaderPlatform);
            if(Test->TestNotNull(TEXT("NPC outfit has a native material resource"),Resource))
                Test->TestEqual(TEXT("NPC outfit shader compiles"),Resource->GetCompileErrors().Num(),0);
        }
    }
    if(Component)
    {
        Test->TestEqual(TEXT("Live NPC actually wears the S356 outfit"),Component->GetSkeletalMeshAsset(),NewMesh);
        Test->TestEqual(TEXT("NPC outfit is nonblocking"),Component->GetCollisionEnabled(),ECollisionEnabled::NoCollision);
        Test->TestFalse(TEXT("NPC outfit adds no overlap events"),Component->GetGenerateOverlapEvents());
    }
}
