// S362: inspect real bound detail, painted texture overrides and the graph that
// consumes the current camera-to-character reveal. Native captures remain the
// separate visual acceptance route.
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "Framework/MemoriaFieldPawn.h"
#include "Narrative/MemoriaNarrativeSubsystem.h"
#include "Presentation/MemoriaVerdanPresentation.h"
#include "Presentation/MemoriaFieldCharacterComponent.h"
#include "Chapter/MemoriaChapterPresentation.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialExpression.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionTextureSampleParameter.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
bool ParameterReaches(UMaterialExpression* Root,FName Name)
{
    TSet<UMaterialExpression*> Seen;
    TFunction<bool(UMaterialExpression*)> Visit=[&](UMaterialExpression* Expression)
    {
        if (!Expression || Seen.Contains(Expression)) return false;
        Seen.Add(Expression);
        if (auto* Vector=Cast<UMaterialExpressionVectorParameter>(Expression);Vector && Vector->ParameterName==Name) return true;
        // Architecture samples a 2D parameter; M_EnvProp samples a TextureObject
        // parameter inside its Custom expression. Both share this parameter base.
        if (auto* Texture=Cast<UMaterialExpressionTextureSampleParameter>(Expression);Texture && Texture->ParameterName==Name) return true;
        for (FExpressionInputIterator Input{Expression};Input;++Input) if (Visit(Input->Expression)) return true;
        return false;
    };
    return Visit(Root);
}

// Sharing the valid source parent preserves the graph and static settings.
// Every unchanged runtime uniform must also survive the sibling MID copy.
void CheckCopiedUniforms(FAutomationTestBase* Test,UMaterialInstanceDynamic* Detail,
    UMaterialInstanceDynamic* Source,FName ChangedTint)
{
    const FString Label=Detail->GetName()+TEXT(" / ");
    TArray<FMaterialParameterInfo> Infos;
    TArray<FGuid> Ids;
    Source->GetAllScalarParameterInfo(Infos,Ids);
    for (const auto& Info : Infos)
    {
        if (Info.Name==TEXT("Roughness")) continue;
        float Expected=0,Actual=0;
        const bool bExpected=Source->GetScalarParameterValue(Info,Expected);
        Test->TestTrue(*(Label+Info.Name.ToString()+TEXT(" scalar preserved")),
            bExpected && Detail->GetScalarParameterValue(Info,Actual) && FMath::IsNearlyEqual(Actual,Expected,.00001f));
    }
    Infos.Reset(); Ids.Reset(); Source->GetAllVectorParameterInfo(Infos,Ids);
    for (const auto& Info : Infos)
    {
        if (Info.Name==ChangedTint) continue;
        FLinearColor Expected,Actual;
        const bool bExpected=Source->GetVectorParameterValue(Info,Expected);
        Test->TestTrue(*(Label+Info.Name.ToString()+TEXT(" vector preserved")),
            bExpected && Detail->GetVectorParameterValue(Info,Actual) && Actual.Equals(Expected,.00001f));
    }
    Infos.Reset(); Ids.Reset(); Source->GetAllTextureParameterInfo(Infos,Ids);
    for (const auto& Info : Infos)
    {
        UTexture* Expected=nullptr;
        UTexture* Actual=nullptr;
        Test->TestTrue(*(Label+Info.Name.ToString()+TEXT(" actual texture override preserved")),
            Source->GetTextureParameterValue(Info,Expected) && Expected &&
            Detail->GetTextureParameterValue(Info,Actual) && Actual==Expected);
    }
}

class FRegionalDetailProbe final : public IAutomationLatentCommand
{
public:
    FRegionalDetailProbe(FAutomationTestBase* InTest,const FString& InMap)
        : Test(InTest),Map(InMap),Started(FPlatformTime::Seconds()) {}
    bool Update() override
    {
        if (LastFrame==GFrameCounter) return false;
        LastFrame=GFrameCounter;
        UWorld* World=GEditor ? GEditor->PlayWorld : nullptr;
        if (FPlatformTime::Seconds()-Started>45.)
        {
            Test->AddError(FString::Printf(TEXT("S362 regional live-binding probe timed out: region=%s world=%s"),
                *Map,World ? *World->GetMapName() : TEXT("none")));
            return true;
        }
        auto* PC=World ? World->GetFirstPlayerController() : nullptr;
        auto* Pawn=PC ? Cast<AMemoriaFieldPawn>(PC->GetPawn()) : nullptr;
        if (!Pawn || World->GetTimeSeconds()<.3) return false;
        if (Map==TEXT("verdan_market"))
        {
            auto* Host=World->GetGameInstance()->GetSubsystem<UMemoriaNarrativeSubsystem>();
            if (!Host) return false;
            if (!bProgressedVerdanFixture)
            {
                // The slice starts in VN. Follow the same existing paid route as
                // VerdanExploration; campaign tests own physical dialogue input.
                for (int32 I=0;I<20 && Host->GetState()==EMemoriaSliceState::VN;++I) Host->Confirm(1);
                bProgressedVerdanFixture=true;
                return false;
            }
            if (!World->GetMapName().EndsWith(TEXT("L_VerdanHost")) || Host->GetState()!=EMemoriaSliceState::Exploration) return false;
        }
        if (World->GetTimeSeconds()<.8) return false;
        AActor* Owner=nullptr;
        UMemoriaFieldCharacterComponent* Character=nullptr;
        if (Map==TEXT("verdan_market"))
        {
            for (TActorIterator<AMemoriaVerdanPresentation> It(World);It;++It)
            {
                Owner=*It; Character=It->CharacterFigure();
                Test->TestEqual(TEXT("The original ten perimeter stalls remain"),It->GetMarketStallCount(),10);
                Test->TestEqual(TEXT("The original seventeen market lights remain"),It->GetMarketLightCount(),17);
            }
        }
        else
        {
            for (TActorIterator<AMemoriaChapterPresentation> It(World);It;++It)
            {
                if (It->GetMap()!=Map) continue;
                Owner=*It; Character=It->GetArrelFigure();
                const auto* Spec=It->GetSpec();
                if (!Test->TestNotNull(TEXT("Source-derived regional spec"),Spec)) return true;
                int32 Solid=0; for (int32 Type : Spec->Tiles) Solid+=Spec->IsSolid(Type) ? 1 : 0;
                Test->TestEqual(TEXT("One original blocker per source solid tile"),It->GetBlockerCount(),Solid);
                Test->TestEqual(TEXT("Original regional lamp count"),It->GetLampCount(),Map==TEXT("belt_waystation") ? 8 : 7);
                Test->TestEqual(TEXT("Original environment kit identities"),It->GetKitKindCount(),Map==TEXT("belt_waystation") ? 9 : 7);
            }
        }
        if (!Owner || !Character || !Pawn->GetFieldCamera()) return false;
        TArray<UInstancedStaticMeshComponent*> Batches; Owner->GetComponents(Batches);
        TSet<UMaterialInstanceDynamic*> Materials;
        TMap<FName,UMaterialInstanceDynamic*> Originals;
        int32 Instances=0;
        for (auto* Batch : Batches)
        {
            auto* Material=Cast<UMaterialInstanceDynamic>(Batch->GetMaterial(0));
            if (!Material) continue;
            if (!Material->GetName().StartsWith(TEXT("S362")))
            { Originals.Add(Material->GetFName(),Material); continue; }
            Materials.Add(Material); Instances+=Batch->GetInstanceCount();
            Test->TestNotNull(TEXT("New detail binds a real existing static mesh"),Batch->GetStaticMesh().Get());
            Test->TestTrue(TEXT("New detail has actual instances"),Batch->GetInstanceCount()>0);
            Test->TestEqual(TEXT("New detail has no collision"),Batch->GetCollisionEnabled(),ECollisionEnabled::NoCollision);
            Test->TestFalse(TEXT("New detail adds no overlaps"),Batch->GetGenerateOverlapEvents());
            Test->TestFalse(TEXT("New detail changes no navigation"),Batch->CanEverAffectNavigation());
        }
        Test->TestTrue(TEXT("The live region actually uses its bounded new detail"),Instances>=10 && Instances<500);
        Test->TestEqual(TEXT("Expected sibling MID families are bound"),Materials.Num(),Map==TEXT("belt_waystation") ? 1 : 3);
        const FVector Eye=Pawn->GetFieldCamera()->GetComponentLocation(),Focus=Character->FocusPosition(),Up=Pawn->GetFieldCamera()->GetUpVector();
        int32 AtlasBindings=0;
        for (auto* Material : Materials)
        {
            const FString Name=Material->GetName();
            const bool bWood=Name.Contains(TEXT("CounterWood")) || Name.Contains(TEXT("CounterRecess"));
            const bool bCut=Name.Contains(TEXT("CutStone")),bJoint=Name.Contains(TEXT("WetJoint"));
            const bool bAtlas=bWood || bCut || bJoint;
            const bool bHem=Name.Contains(TEXT("DriftCanvasHem"));
            const FName SourceName=bWood ? TEXT("Timber") : bCut ? TEXT("ChapterRuinStone") :
                bJoint ? TEXT("ChapterSlab") : bHem ? TEXT("KitDriftFFFFFFFF") : TEXT("Iron");
            auto* Source=Originals.FindRef(SourceName);
            Test->TestNotNull(*(Name+TEXT(" original live source material")),Source);
            Test->TestNotNull(TEXT("Detail retains a valid original material parent"),Material->Parent.Get());
            Test->TestFalse(TEXT("A dynamic material is never used as a MID parent"),Material->Parent && Material->Parent->IsA<UMaterialInstanceDynamic>());
            Test->TestEqual(TEXT("New detail retains masked camera reveal"),Material->GetBlendMode(),BLEND_Masked);
            if (Source)
            {
                Test->TestTrue(*(Name+TEXT(" shares the exact original valid parent")),Material->Parent==Source->Parent && Material->Parent!=nullptr);
                CheckCopiedUniforms(Test,Material,Source,bAtlas ? TEXT("ArchitectureTint") : TEXT("Tint"));
            }
            auto* Base=Material->GetMaterial();
            const FString BasePath=bAtlas ? TEXT("/Game/Memoria/Presentation/EnvironmentS356/M_ArchitectureSurface.M_ArchitectureSurface") :
                bHem ? TEXT("/Game/Memoria/Presentation/Environment/M_EnvProp.M_EnvProp") :
                TEXT("/Game/Memoria/Presentation/Depth2/M_FocusSurface.M_FocusSurface");
            if (Test->TestNotNull(*(Name+TEXT(" bound shader resolves")),Base))
            {
                Test->TestEqual(*(Name+TEXT(" uses the original shader graph")),Base->GetPathName(),BasePath);
                Test->TestTrue(TEXT("New detail uses lit instanced rendering"),Base->GetShadingModels().HasShadingModel(MSM_DefaultLit) && Base->GetUsageByFlag(MATUSAGE_InstancedStaticMeshes));
                auto* Data=Base->GetEditorOnlyData();
                Test->TestNotNull(TEXT("Reveal reaches the actual OpacityMask"),Data->OpacityMask.Expression);
                for (const TCHAR* Parameter : {TEXT("OcclusionEye"),TEXT("OcclusionFocus"),TEXT("OcclusionUp")})
                    Test->TestTrue(*(Name+TEXT(" mask consumes ")+Parameter),ParameterReaches(Data->OpacityMask.Expression,FName(Parameter)));
                if (bAtlas || bHem)
                    Test->TestTrue(TEXT("Original painted texture parameter reaches actual BaseColor"),ParameterReaches(Data->BaseColor.Expression,bAtlas ? TEXT("ArchitectureAtlas") : TEXT("Color")));
            }
            for (const auto& Expected : TArray<TPair<FName,FVector>>{{TEXT("OcclusionEye"),Eye},{TEXT("OcclusionFocus"),Focus},{TEXT("OcclusionUp"),Up}})
            {
                FLinearColor Actual;
                Test->TestTrue(*(Name+TEXT(" tracks actual ")+Expected.Key.ToString()),
                    Material->GetVectorParameterValue(FMaterialParameterInfo(Expected.Key),Actual) && FVector(Actual.R,Actual.G,Actual.B).Equals(Expected.Value,.1));
            }
            if (bAtlas)
            {
                UTexture* Atlas=nullptr;
                const bool bOriginalAtlas=Material->GetTextureParameterValue(FMaterialParameterInfo(TEXT("ArchitectureAtlas")),Atlas) && Atlas &&
                    Atlas->GetPathName()==TEXT("/Game/Memoria/Presentation/EnvironmentS356/T_ArchitectureAtlas.T_ArchitectureAtlas");
                Test->TestTrue(TEXT("Detail retains the original S356 painted atlas"),bOriginalAtlas);
                AtlasBindings+=bOriginalAtlas ? 1 : 0;
            }
            if (bHem)
            {
                UTexture* Canvas=nullptr;
                Test->TestTrue(TEXT("Hem actually binds Drift canvas rather than the Belt shader default"),
                    Material->GetTextureParameterValue(FMaterialParameterInfo(TEXT("Color")),Canvas) && Canvas &&
                    Canvas->GetPathName()==TEXT("/Game/Memoria/Presentation/Environment/T_EnvDrift.T_EnvDrift"));
            }
        }
        Test->TestEqual(TEXT("All expected original painted atlas families remain live"),AtlasBindings,Map==TEXT("belt_waystation") ? 1 : 2);
        Test->AddInfo(FString::Printf(TEXT("S362_REGIONAL_LIVE map=%s materials=%d instances=%d verified_original_atlas=%d"),
            *Map,Materials.Num(),Instances,AtlasBindings));
        return true;
    }
private:
    FAutomationTestBase* Test;
    FString Map;
    double Started;
    uint64 LastFrame=MAX_uint64;
    bool bProgressedVerdanFixture=false;
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVerdanRegionalS362,"MemoriaVisual.RegionalS362.Verdan",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FVerdanRegionalS362::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice"))) return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FRegionalDetailProbe(this,TEXT("verdan_market"))));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand()); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBeltRegionalS362,"MemoriaVisual.RegionalS362.Belt",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FBeltRegionalS362::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Memoria/Maps/L_BeltWaystation"))) return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FRegionalDetailProbe(this,TEXT("belt_waystation"))));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand()); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDriftRegionalS362,"MemoriaVisual.RegionalS362.Drift",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDriftRegionalS362::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Memoria/Maps/L_DriftShelter"))) return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FRegionalDetailProbe(this,TEXT("drift_shelter"))));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand()); return true;
}
#endif
