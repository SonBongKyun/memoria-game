#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "Tests/AutomationCommon.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Framework/MemoriaSliceHost.h"
#include "Framework/MemoriaFieldPawn.h"
#include "Presentation/MemoriaFieldCharacterComponent.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Combat/MemoriaFieldCombatSubsystem.h"
#include "Chapter/MemoriaChapterMap.h"
#include "Chapter/MemoriaChapterPresentation.h"
#include "EngineUtils.h"
#include "Misc/App.h"
#if WITH_DEV_AUTOMATION_TESTS
namespace
{
// S381: the cached Belt spec is patched before the map starts, then put back. The first revisit figure
// holds its tile and looks at a point north of it, glances at Arrel inside 150 cm, and turns back.
class FAmbientHoldReplay final : public IAutomationLatentCommand
{
public:
    FAmbientHoldReplay(FAutomationTestBase* InTest, bool bInStill, bool bInFace, const FVector2D& InFace)
        : Test(InTest), SavedStill(bInStill), SavedHasFace(bInFace), SavedFace(InFace), Started(FPlatformTime::Seconds()) {}
    ~FAmbientHoldReplay() override
    {
        Restore();
        if (bFixed) { FApp::SetUseFixedTimeStep(bOldFixed); FApp::SetFixedDeltaTime(OldDelta); }
    }
    bool Update() override
    {
        if (LastFrame == GFrameCounter) return false;
        LastFrame = GFrameCounter;
        if (FPlatformTime::Seconds() - Started > 180) { Test->AddError(FString::Printf(TEXT("Ambient hold timeout at step %d"), Step)); return true; }
        UWorld* World = GEditor->PlayWorld;
        auto* PC = World ? Cast<AMemoriaSliceController>(World->GetFirstPlayerController()) : nullptr;
        auto* Pawn = PC ? Cast<AMemoriaFieldPawn>(PC->GetPawn()) : nullptr;
        if (!Pawn || World->GetTimeSeconds() < .2) return false;
        if (!bFixed) { bFixed = true; bOldFixed = FApp::UseFixedTimeStep(); OldDelta = FApp::GetFixedDeltaTime(); FApp::SetUseFixedTimeStep(true); FApp::SetFixedDeltaTime(1.0 / 60.0); }
        ++Frame;
        auto* Run = World->GetGameInstance()->GetSubsystem<UMemoriaRunSubsystem>();
        auto* Combat = World->GetSubsystem<UMemoriaFieldCombatSubsystem>();
        AMemoriaChapterPresentation* Map = nullptr; for (TActorIterator<AMemoriaChapterPresentation> It(World); It; ++It) Map = *It;
        const auto* Spec = Map ? Map->GetSpec() : nullptr;
        auto* Figure = Map ? Map->GetAmbientNpc(0) : nullptr;
        if (!Spec || !Run || !Combat || !Figure || Spec->AmbientNpcs.Num() < 1) return false;
        auto Place = [&](const FVector2D& Source) { Pawn->SetActorLocation(MemoriaChapterMaps::ToWorld(Source) + FVector(0, 0, Pawn->GetActorLocation().Z)); };
        auto YawOf = [](const FVector& From, const FVector& Toward) { return ((Toward - From) * FVector(1, 1, 0)).Rotation().Yaw; };
        auto Faced = [&](float Want) { return FMath::Abs(FMath::FindDeltaAngleDegrees(Map->GetAmbientYaw(0), Want)) < 8.f && FMath::Abs(FMath::FindDeltaAngleDegrees(Figure->GetYaw(), Want)) < 8.f; };
        const FVector2D Spot = Spec->AmbientNpcs[0].Position;
        const FVector Home = MemoriaChapterMaps::ToWorld(Spot);
        const FVector Point = MemoriaChapterMaps::ToWorld(Spec->AmbientNpcs[0].Face);
        switch (Step)
        {
        case 0:
            Test->TestTrue(TEXT("The patched figure holds and has a face point"), Spec->AmbientNpcs[0].bStill && Spec->AmbientNpcs[0].bHasFace);
            Test->TestFalse(TEXT("The other authored figures are unchanged"), Spec->AmbientNpcs[1].bStill || Spec->AmbientNpcs[1].bHasFace);
            Test->TestTrue(TEXT("It spawned on its tile"), Map->GetAmbientHome(0).Equals(Home, 1.f));
            Test->TestTrue(TEXT("The held figure is rigged, so it can turn freely"), Figure->IsRigged());
            for (const auto& S : Spec->Sequence) { Run->SetStoryFlag(S.Flag, true); for (const FString& F : S.Flags) Run->SetStoryFlag(F, true); }
            Run->SetStoryFlag(Spec->Exit.Requires, true); Run->SetStoryFlag(Spec->Exit.Completes, true); Run->SetCurrentChapter(4);
            for (const auto& C : Spec->Chests) Run->SetStoryFlag(C.Flag, true);
            for (const auto& C : Spec->Clues) Run->SetStoryFlag(C.Flag, true);
            for (int32 I = 0; I < Spec->Battles.Num(); ++I) Run->SetStoryFlag(FString::Printf(TEXT("battle_%s_%d"), *Spec->Map, I + 1), true);
            for (const auto& T : Spec->Triggers) Run->SetStoryFlag(T.Flag, true);
            Place(Spot + FVector2D(0.f, 180.f));
            ++Step; Mark = Frame; break;
        case 1:
            if (Frame < Mark + 8 || Map->GetVisibleNpcCount() < 1) break;
            Held = Figure->GetComponentLocation();
            Test->TestTrue(TEXT("Arrel is outside the glance"), FVector::Dist2D(Held, Pawn->GetActorLocation()) > AMemoriaChapterPresentation::NpcNotice);
            Test->TestEqual(TEXT("No fight is pulling its gaze"), Combat->LiveMonsterCount(), 0);
            ++Step; Mark = Frame; break;
        case 2:
            if (Frame < Mark + 200) break;
            Test->TestTrue(TEXT("A few seconds later it has not moved"), FVector::Dist2D(Figure->GetComponentLocation(), Held) < 2.f);
            Test->TestTrue(TEXT("And it still stands on its tile"), FVector::Dist2D(Figure->GetComponentLocation(), Home) < 2.f);
            Test->TestTrue(TEXT("It faces its point"), Faced(YawOf(Figure->GetComponentLocation(), Point)));
            Place(Spot + FVector2D(16.f, 0.f));
            ++Step; Mark = Frame; break;
        case 3:
            if (Frame < Mark + 100) break;
            Test->TestTrue(TEXT("Arrel is inside the glance"), FVector::Dist2D(Figure->GetComponentLocation(), Pawn->GetActorLocation()) < 150.f);
            Test->TestTrue(TEXT("The glance does not take it off its tile"), FVector::Dist2D(Figure->GetComponentLocation(), Held) < 2.f);
            Test->TestEqual(TEXT("Still no fight"), Combat->LiveMonsterCount(), 0);
            Test->TestTrue(TEXT("It turns to Arrel"), Faced(YawOf(Figure->GetComponentLocation(), Pawn->GetActorLocation())));
            Place(Spot + FVector2D(0.f, 180.f));
            ++Step; Mark = Frame; break;
        case 4:
            if (Frame < Mark + 100) break;
            Test->TestTrue(TEXT("Arrel has stepped back"), FVector::Dist2D(Figure->GetComponentLocation(), Pawn->GetActorLocation()) > 150.f);
            Test->TestTrue(TEXT("It is still on its tile"), FVector::Dist2D(Figure->GetComponentLocation(), Held) < 2.f);
            Test->TestTrue(TEXT("It turns back to its point"), Faced(YawOf(Figure->GetComponentLocation(), Point)));
            return true;
        default: break;
        }
        return false;
    }
private:
    void Restore()
    {
        if (FMemoriaChapterMapSpec* Spec = const_cast<FMemoriaChapterMapSpec*>(MemoriaChapterMaps::Find(TEXT("belt_waystation"))))
            if (Spec->AmbientNpcs.IsValidIndex(0))
            {
                auto& Npc = Spec->AmbientNpcs[0];
                Npc.bStill = SavedStill; Npc.bHasFace = SavedHasFace; Npc.Face = SavedFace;
            }
    }
    FAutomationTestBase* Test;
    bool SavedStill, SavedHasFace;
    FVector2D SavedFace;
    double Started, OldDelta = 0;
    uint64 LastFrame = MAX_uint64;
    int32 Step = 0, Frame = 0, Mark = 0;
    FVector Held = FVector::ZeroVector;
    bool bFixed = false, bOldFixed = false;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAmbientHoldS381, "Memoria.S381.AmbientHold", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAmbientHoldS381::RunTest(const FString&)
{
    FMemoriaChapterMapSpec* Spec = const_cast<FMemoriaChapterMapSpec*>(MemoriaChapterMaps::Find(TEXT("belt_waystation")));
    if (!TestNotNull(TEXT("Belt spec"), Spec) || !TestTrue(TEXT("Belt has revisit figures"), Spec && Spec->AmbientNpcs.Num() >= 2)) return false;
    auto& Npc = Spec->AmbientNpcs[0];
    const bool SavedStill = Npc.bStill; const bool SavedHasFace = Npc.bHasFace; const FVector2D SavedFace = Npc.Face;
    Npc.bStill = true; Npc.bHasFace = true; Npc.Face = FVector2D(Npc.Position.X, Npc.Position.Y - 68.f);
    if (!AutomationOpenMap(TEXT("/Game/Memoria/Maps/L_BeltWaystation")))
    {
        Npc.bStill = SavedStill; Npc.bHasFace = SavedHasFace; Npc.Face = SavedFace;
        return false;
    }
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FAmbientHoldReplay(this, SavedStill, SavedHasFace, SavedFace)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
