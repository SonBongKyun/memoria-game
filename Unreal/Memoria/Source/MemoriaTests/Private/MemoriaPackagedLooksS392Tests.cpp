#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "Tests/AutomationCommon.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "Framework/MemoriaSliceHost.h"
#include "Chapter/MemoriaChapterMap.h"
#include "Chapter/MemoriaChapterPresentation.h"
#include "Interaction/MemoriaEliaCompanion.h"
#include "Interaction/MemoriaTobiasCompanion.h"
#include "Presentation/MemoriaFieldCharacterComponent.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Misc/PackageName.h"
#include "Misc/App.h"
#if WITH_DEV_AUTOMATION_TESTS
namespace
{
bool CodeHas(const UMaterialInterface* Material, const TCHAR* A, const TCHAR* B)
{
    const UMaterial* Source = Material ? Material->GetMaterial() : nullptr;
    if (!Source) return false;
#if WITH_EDITORONLY_DATA
    for (const TObjectPtr<UMaterialExpression>& Expr : Source->GetExpressionCollection().Expressions)
    {
        const auto* Custom = Cast<UMaterialExpressionCustom>(Expr.Get());
        if (Custom && Custom->Code.Contains(A) && Custom->Code.Contains(B)) return true;
    }
#endif
    return false;
}
// A packaged look is a saved asset under FieldLooks, not a transient UMaterial built in code.
bool SavedLook(FAutomationTestBase* Test, const UMaterialInterface* Material, const TCHAR* Asset, const TCHAR* TintA, const TCHAR* TintB)
{
    if (!Test->TestNotNull(Asset, Material)) return false;
    const FString Path = Material->GetPathName();
    Test->TestTrue(FString::Printf(TEXT("%s is a saved FieldLooks asset (%s)"), Asset, *Path),
        Material->GetOutermost() != GetTransientPackage() && Path.StartsWith(TEXT("/Game/Memoria/Presentation/FieldLooks/")) && Path.Contains(Asset));
    const UMaterial* Source = Material->GetMaterial();
    Test->TestTrue(FString::Printf(TEXT("%s parent is saved"), Asset),
        Source && Source->GetOutermost() != GetTransientPackage() && Source->GetPathName().StartsWith(TEXT("/Game/Memoria/Presentation/FieldLooks/")));
    Test->TestTrue(FString::Printf(TEXT("%s keeps its tint"), Asset), CodeHas(Material, TintA, TintB));
    const FString Package = FString(TEXT("/Game/Memoria/Presentation/FieldLooks/")) + Asset;
    Test->TestTrue(FString::Printf(TEXT("%s is on disk"), Asset), FPackageName::DoesPackageExist(Package));
    return true;
}
UMaterialInterface* SlotLook(UMemoriaFieldCharacterComponent* Figure, const TCHAR* Token)
{
    USkeletalMeshComponent* Body = Figure ? Figure->GetSkeletalMesh() : nullptr;
    if (!Body) return nullptr;
    const int32 Count = Body->GetNumMaterials();
    for (int32 Index = 0; Index < Count; ++Index)
    {
        UMaterialInterface* Material = Body->GetMaterial(Index);
        if (Material && Material->GetName().Contains(Token)) return Material;
    }
    return nullptr;
}
void QuietBelt(UMemoriaRunSubsystem* Run, const FMemoriaChapterMapSpec* Spec)
{
    if (!Run || !Spec) return;
    const FString Closed = Spec->Exit.Completes;
    auto Set = [&](const FString& Flag)
    {
        if (!Flag.IsEmpty() && !Flag.Equals(Closed)) Run->SetStoryFlag(Flag, true);
    };
    for (const auto& Step : Spec->Sequence)
    {
        Set(Step.Flag);
        for (const FString& Flag : Step.Flags) Set(Flag);
    }
    for (const auto& Chest : Spec->Chests) Set(Chest.Flag);
    for (const auto& Clue : Spec->Clues) Set(Clue.Flag);
    for (const auto& Trigger : Spec->Triggers) Set(Trigger.Flag);
    for (int32 Index = 0; Index < Spec->Battles.Num(); ++Index)
        Set(FString::Printf(TEXT("battle_%s_%d"), *Spec->Map, Index + 1));
}
AMemoriaEliaCompanion* FindElia(UWorld* World)
{
    if (!World) return nullptr;
    for (TActorIterator<AMemoriaEliaCompanion> It(World); It; ++It) return *It;
    return nullptr;
}
AMemoriaTobiasCompanion* FindTobias(UWorld* World)
{
    if (!World) return nullptr;
    for (TActorIterator<AMemoriaTobiasCompanion> It(World); It; ++It) return *It;
    return nullptr;
}
class FPackagedLooksReplay final : public IAutomationLatentCommand
{
public:
    explicit FPackagedLooksReplay(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
    ~FPackagedLooksReplay() override
    {
        if (UWorld* World = GEditor ? GEditor->PlayWorld : nullptr)
            if (UGameInstance* Game = World->GetGameInstance())
                if (auto* Run = Game->GetSubsystem<UMemoriaRunSubsystem>()) Run->SetStoryFlag(TEXT("party_tobias"), false);
        if (bFixed) { FApp::SetUseFixedTimeStep(bOldFixed); FApp::SetFixedDeltaTime(OldDelta); }
    }
    bool Update() override
    {
        if (LastFrame == GFrameCounter) return false;
        LastFrame = GFrameCounter;
        if (FPlatformTime::Seconds() - Started > 120.0)
        {
            Test->AddError(FString::Printf(TEXT("Packaged looks timeout at step %d"), Step));
            return true;
        }
        UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
        if (!World || World->GetTimeSeconds() < .2) return false;
        if (!bFixed)
        {
            bFixed = true; bOldFixed = FApp::UseFixedTimeStep(); OldDelta = FApp::GetFixedDeltaTime();
            FApp::SetUseFixedTimeStep(true); FApp::SetFixedDeltaTime(1.0 / 60.0);
        }
        ++Frame;
        auto* Run = World->GetGameInstance() ? World->GetGameInstance()->GetSubsystem<UMemoriaRunSubsystem>() : nullptr;
        AMemoriaChapterPresentation* Map = nullptr;
        for (TActorIterator<AMemoriaChapterPresentation> It(World); It; ++It) Map = *It;
        const FMemoriaChapterMapSpec* Spec = Map ? Map->GetSpec() : nullptr;
        if (!Run || !Spec) return false;
        switch (Step)
        {
        case 0:
        {
            if (!World->GetMapName().EndsWith(TEXT("L_BeltWaystation"))) break;
            AMemoriaEliaCompanion* Elia = FindElia(World);
            if (!Elia) break;
            if (!bQuieted) { QuietBelt(Run, Spec); bQuieted = true; Mark = Frame; break; }
            if (Frame < Mark + 8) break;
            SavedLook(Test, SlotLook(Elia->GetFigure(), TEXT("EliaHair")), TEXT("M_EliaHair"),
                TEXT("float3 silver=float3(0.80,0.82,0.84)"), TEXT("step(.10,A)*(1-step(.35,A))"));
            ++Step;
            break;
        }
        case 1:
            Run->SetStoryFlag(TEXT("party_tobias"), true);
            From = World;
            UGameplayStatics::OpenLevel(World, TEXT("/Game/Memoria/Maps/L_BeltWaystation"));
            ++Step;
            break;
        case 2:
        {
            if (World == From.Get() || World->GetTimeSeconds() < .2) break;
            if (!World->GetMapName().EndsWith(TEXT("L_BeltWaystation"))) break;
            AMemoriaEliaCompanion* Elia = FindElia(World);
            AMemoriaTobiasCompanion* Tobias = FindTobias(World);
            if (!Elia || !Tobias) break;
            QuietBelt(Run, Spec);
            SavedLook(Test, SlotLook(Elia->GetFigure(), TEXT("EliaHair")), TEXT("M_EliaHair"),
                TEXT("float3 silver=float3(0.80,0.82,0.84)"), TEXT("step(.10,A)*(1-step(.35,A))"));
            SavedLook(Test, SlotLook(Tobias->GetFigure(), TEXT("TobiasCoat")), TEXT("M_TobiasCoat"),
                TEXT("float3 ink=float3(0.10,0.075,0.055)"), TEXT("Category-.75"));
            return true;
        }
        default: break;
        }
        return false;
    }
private:
    FAutomationTestBase* Test;
    double Started = 0, OldDelta = 0;
    uint64 LastFrame = MAX_uint64;
    int32 Step = 0, Frame = 0, Mark = 0;
    bool bFixed = false, bOldFixed = false, bQuieted = false;
    TWeakObjectPtr<UWorld> From;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPackagedLooksS392, "Memoria.S392.PackagedLooks", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPackagedLooksS392::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Memoria/Maps/L_BeltWaystation"))) return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FPackagedLooksReplay(this)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
