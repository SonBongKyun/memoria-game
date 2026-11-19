#pragma once
#include "Materials/MaterialExpression.h"
#include "Misc/AutomationTest.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/Texture2D.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "MaterialShared.h"
#include "RHI.h"
#include "TextureCompiler.h"
#include "ShaderCompiler.h"
// Verify delivered data maps are connected to the materials actually used by the field meshes.
// A PNG on disk or an unused texture node is insufficient evidence of live graphics integration.
inline bool MemoriaSurfaceReachable(UMaterialExpression* Root,FName Parameter,UTexture* Texture)
{
    TSet<UMaterialExpression*> Seen;
    TFunction<bool(UMaterialExpression*)> Visit=[&](UMaterialExpression* Expression)
    {
        if(!Expression||Seen.Contains(Expression))return false;
        Seen.Add(Expression);
        if(auto* Sample=Cast<UMaterialExpressionTextureSampleParameter2D>(Expression);
            Sample&&Sample->ParameterName==Parameter&&Sample->Texture==Texture)return true;
        for (FExpressionInputIterator Input{Expression}; Input; ++Input)if(Input&&Visit(Input->Expression))return true;
        return false;
    };
    return Visit(Root);
}
inline void CheckMemoriaSurfaceDetail(FAutomationTestBase* Test)
{
    const TCHAR* Names[]={TEXT("Arrel"),TEXT("Elia"),TEXT("Malet"),TEXT("Traveler"),TEXT("Bureauagent"),TEXT("Guard"),
        TEXT("Villagerf"),TEXT("Villagerm"),TEXT("Fisherman"),TEXT("Elder"),TEXT("Child"),TEXT("Scholar"),TEXT("Husk"),TEXT("Thief")};
    for(const TCHAR* Name:Names)
    {
        const FString Prefix=FString(Name)+TEXT(" surface ");
        const FString Base=FString(TEXT("/Game/Memoria/Presentation/Field3D/"))+Name+TEXT("/");
        auto* Material=LoadObject<UMaterial>(nullptr,*(Base+TEXT("M_")+Name));
        auto* Mesh=LoadObject<USkeletalMesh>(nullptr,*(Base+TEXT("SK_")+Name));
        if(!Test->TestNotNull(*(Prefix+TEXT("material loads")),Material)||!Test->TestNotNull(*(Prefix+TEXT("mesh loads")),Mesh))continue;
        if(Test->TestEqual(*(Prefix+TEXT("one body slot")),Mesh->GetMaterials().Num(),1))
            Test->TestEqual(*(Prefix+TEXT("actual body binding")),Mesh->GetMaterials()[0].MaterialInterface.Get(),static_cast<UMaterialInterface*>(Material));
        auto* Inputs=Material->GetEditorOnlyData();
        Test->TestNotNull(*(Prefix+TEXT("normal connected")),Inputs->Normal.Expression);
        Test->TestNotNull(*(Prefix+TEXT("roughness connected")),Inputs->Roughness.Expression);
        Test->TestNotNull(*(Prefix+TEXT("metal connected")),Inputs->Metallic.Expression);
        Test->TestNotNull(*(Prefix+TEXT("AO connected")),Inputs->AmbientOcclusion.Expression);
        const bool Hero=FString(Name)==TEXT("Arrel")||FString(Name)==TEXT("Elia")||FString(Name)==TEXT("Malet");
        const FString Profile=Hero?TEXT("HeroDetail"):(FString(Name)==TEXT("Husk")?TEXT("HuskDetail"):(FString(Name)==TEXT("Thief")?TEXT("ThiefDetail"):TEXT("AmbientDetail")));
        for(const bool Normal:{true,false})
        {
            UTexture* Parameter=nullptr;
            const FName ParameterName=Normal?TEXT("DetailNormal"):TEXT("SurfaceData");
            Test->TestTrue(*(Prefix+ParameterName.ToString()+TEXT(" parameter bound")),Material->GetTextureParameterValue(FMaterialParameterInfo(ParameterName),Parameter));
            auto* Texture=Cast<UTexture2D>(Parameter);
            if(!Test->TestNotNull(*(Prefix+TEXT("data texture resolves")),Texture))continue;
            // Newly loaded editor textures expose a temporary 32px resource until async compilation ends.
            FTextureCompilingManager::Get().FinishCompilation({Texture});
            Test->TestEqual(*(Prefix+TEXT("correct atlas profile")),Texture->GetName(),TEXT("T_")+Profile+(Normal?TEXT("_Normal"):TEXT("_Surface")));
            Test->TestFalse(*(Prefix+TEXT("data stays linear")),Texture->SRGB!=0);
            Test->TestEqual(*(Prefix+TEXT("correct sampler compression")),int32(Texture->CompressionSettings),int32(Normal?TC_Normalmap:TC_Masks));
            Test->TestTrue(*(Prefix+TEXT("filtered at gameplay distance")),Texture->Filter==TF_Trilinear);
            Test->TestEqual(*(Prefix+TEXT("data width")),Texture->GetSizeX(),Hero?512:2048);
            if(FApp::CanEverRender())Test->TestTrue(*(Prefix+TEXT("mip chain built")),Texture->GetNumMips()>4);
            bool Connected=false;
            for(UMaterialExpression* Expression:Material->GetExpressionCollection().Expressions)
                if(auto* Sample=Cast<UMaterialExpressionTextureSampleParameter2D>(Expression);Sample&&Sample->ParameterName==ParameterName)
                    Connected=Sample->Texture==Texture&&Sample->SamplerType==(Normal?SAMPLERTYPE_Normal:SAMPLERTYPE_Masks)&&Sample->Coordinates.Expression!=nullptr;
            Test->TestTrue(*(Prefix+TEXT("sample uses matching data sampler and UV")),Connected);
            if(Normal)Test->TestTrue(*(Prefix+TEXT("normal texture reaches the live Normal input")),MemoriaSurfaceReachable(Inputs->Normal.Expression,ParameterName,Texture));
            else
            {
                Test->TestTrue(*(Prefix+TEXT("surface texture reaches Roughness")),MemoriaSurfaceReachable(Inputs->Roughness.Expression,ParameterName,Texture));
                if(FString(Name)!=TEXT("Arrel"))Test->TestTrue(*(Prefix+TEXT("surface texture reaches Metallic")),MemoriaSurfaceReachable(Inputs->Metallic.Expression,ParameterName,Texture));
                Test->TestTrue(*(Prefix+TEXT("surface texture reaches AO")),MemoriaSurfaceReachable(Inputs->AmbientOcclusion.Expression,ParameterName,Texture));
                auto* Metal=Cast<UMaterialExpressionComponentMask>(Inputs->Metallic.Expression);
                auto* AO=Cast<UMaterialExpressionComponentMask>(Inputs->AmbientOcclusion.Expression);
                if(FString(Name)!=TEXT("Arrel"))Test->TestTrue(*(Prefix+TEXT("packed metallic uses B only")),Metal&&Metal->B&&!Metal->R&&!Metal->G&&!Metal->A);
                Test->TestTrue(*(Prefix+TEXT("packed AO uses R only")),AO&&AO->R&&!AO->G&&!AO->B&&!AO->A);
            }
        }
        if(FApp::CanEverRender())
        {
            GShaderCompilingManager->FinishAllCompilation();
            const auto* Resource=Material->GetMaterialResource(GMaxRHIShaderPlatform);
            if(Test->TestNotNull(*(Prefix+TEXT("compiled native resource")),Resource))
                Test->TestEqual(*(Prefix+TEXT("no shader compile errors")),Resource->GetCompileErrors().Num(),0);
        }
        Test->AddInfo(FString::Printf(TEXT("SURFACE_DETAIL_LIVE %s profile=%s normal_roughness_AO=bound metallic=profile"),Name,*Profile));
    }
}
