#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Actor.h"
#include "Framework/MemoriaSliceHost.h"
#include "Framework/MemoriaFieldPawn.h"
#include "Framework/MemoriaCoordinates.h"
#include "Framework/MemoriaVerdanTuning.h"
#include "Framework/Application/SlateApplication.h"
#include "Engine/GameViewportClient.h"
#include "Components/BoxComponent.h"
#include "Combat/MemoriaFieldCombatSubsystem.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Narrative/MemoriaNarrativeSubsystem.h"
#include "Presentation/MemoriaFieldCharacterComponent.h"
#include "Combat/MemoriaFieldCombatTypes.h"
#include "MemoriaPerformanceInventoryS362.h"
#include "InputKeyEventArgs.h"
#include "Misc/App.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "ProfilingDebugging/CsvProfiler.h"
#include "Async/Future.h"

#if WITH_DEV_AUTOMATION_TESTS
CSV_DEFINE_CATEGORY(MemoriaS362,true);
namespace
{
class FPresentationEightS362 final : public IAutomationLatentCommand
{
public:
    explicit FPresentationEightS362(FAutomationTestBase* InTest):Test(InTest),Started(FPlatformTime::Seconds()){}
    ~FPresentationEightS362() override
    {
        if (bConfigured) FApp::SetUseFixedTimeStep(bOldFixed);
        if (Controller.IsValid()) ReleaseAll();
        for (auto& Actor : Fixtures) if (Actor.IsValid()) Actor->Destroy();
#if CSV_PROFILER
        if (bCapturing) FCsvProfiler::Get()->EndCapture();
#endif
    }
    bool Update() override
    {
        if (FrameId==GFrameCounter) return false; FrameId=GFrameCounter;
        const double Now=FPlatformTime::Seconds();
        if (Now-Started>120.) { Test->AddError(TEXT("S362 presentation perf scenario timeout")); return true; }
        UWorld* World=GEditor->PlayWorld.Get(); auto* C=World ? Cast<AMemoriaSliceController>(World->GetFirstPlayerController()) : nullptr;
        auto* Pawn=C ? Cast<AMemoriaFieldPawn>(C->GetPawn()) : nullptr;
        if (!Pawn || World->GetTimeSeconds()<.3f) return false;
        auto* Host=World->GetGameInstance()->GetSubsystem<UMemoriaNarrativeSubsystem>();
        if (!bConfigured)
        {
            bConfigured=true; bOldFixed=FApp::UseFixedTimeStep(); FApp::SetUseFixedTimeStep(false);
            for (int32 I=0;I<20 && Host->GetState()==EMemoriaSliceState::VN;++I) Host->Confirm(1);
            return false;
        }
        if (!World->GetMapName().EndsWith(TEXT("L_VerdanHost")) || Host->GetState()!=EMemoriaSliceState::Exploration)
        {
            if (bBuilt) { Test->AddError(TEXT("A gameplay modal interrupted the presentation profiling fixture")); return true; }
            return false;
        }
        if (!bBuilt)
        {
            bBuilt=true; Controller=C; Warmup=Now;
            // The retained source map's only internal blockers are the two stalls at
            // (+/-400, 200), each 100x100. Keep the original W/D/S/A route in the
            // clear central 240cm square, verifying its real collision before profiling.
            const FVector Origin=Memoria::Coordinates::FromSource(FVector2D(-120,120))+FVector(0,0,Pawn->GetActorLocation().Z);
            Pawn->SetActorLocation(Origin); PreviousPlayer=Pawn->GetActorLocation();
            const auto* Body=Cast<UBoxComponent>(Pawn->GetRootComponent());
            if (!Test->TestNotNull(TEXT("Profile route uses the actual pawn collision box"),Body)) return true;
            const double Leg=MemoriaVerdanTuning::WalkSpeed*2.;
            const FVector Corners[]={Origin,Origin+FVector(0,Leg,0),Origin+FVector(Leg,Leg,0),Origin+FVector(Leg,0,0),Origin};
            FCollisionQueryParams RouteQuery(SCENE_QUERY_STAT(MemoriaS362ProfileRoute),false,Pawn);
            for (int32 I=0;I<4;++I)
                if (!Test->TestFalse(TEXT("The source-derived keyboard route is clear for the real pawn body"),
                    World->SweepTestByChannel(Corners[I],Corners[I+1],FQuat::Identity,ECC_Pawn,FCollisionShape::MakeBox(Body->GetScaledBoxExtent()),RouteQuery))) return true;
            // Automation map travel can leave Slate on the editor. Acquire the real game
            // viewport before held keyboard events; do not inject movement action values.
            C->SetInputMode(FInputModeGameOnly());
            FSlateApplication::Get().SetAllUserFocusToGameViewport();
            ReleaseAll();
            Test->AddInfo(FString::Printf(TEXT("S362_PERF_ROUTE source=(-120,120) world=%s leg=%.1f fixed=%d"),*Origin.ToString(),Leg,int32(FApp::UseFixedTimeStep())));
            const EMemoriaFoeKind Kinds[]={EMemoriaFoeKind::VoidHusk,EMemoriaFoeKind::MarketThief,EMemoriaFoeKind::BeltScavenger,EMemoriaFoeKind::VoidWisp,
                EMemoriaFoeKind::DustCrawler,EMemoriaFoeKind::MemoryLeech,EMemoriaFoeKind::RubbleRat,EMemoriaFoeKind::AshWalker};
            for (int32 I=0;I<8;++I)
            {
                const auto& S=FoeSpec(Kinds[I]); FMemoriaFoeLook Look;
                Look.Model=S.Model; Look.Height=S.Height; Look.bQuinn=S.bQuinn; Look.Idle=S.Idle; Look.Walk=S.Walk;
                Look.Color=S.Color; Look.Glow=S.Glow; Look.Crack=S.Crack; Look.Rim=S.Rim; Look.BladeScale=S.BladeScale;
                Look.bRetint=S.bRetint; Look.ModelGlow=S.ModelGlow; Look.ModelCrack=S.ModelCrack; Look.ModelRim=S.ModelRim;
                Look.ModelRimStrength=S.ModelRimStrength; Look.SurfaceTint=S.SurfaceTint; Look.Weathering=S.Weathering;
                auto* Actor=World->SpawnActor<AActor>(); Fixtures.Add(Actor);
                auto* Figure=NewObject<UMemoriaFieldCharacterComponent>(Actor); Actor->AddInstanceComponent(Figure); Actor->SetRootComponent(Figure); Figure->RegisterComponent();
                if (!Test->TestTrue(TEXT("Real current archetype presentation initializes"),Figure->InitializeFoe(Look))) return true;
                Test->TestTrue(TEXT("Perf fixture loads the actual creature/humanoid binary rather than a mannequin fallback"),Figure->IsFoeModel());
                const FVector Home(-450.f+(I%4)*300.f,I<4 ? 80.f : 330.f,0);
                Homes.Add(Home); Figure->SetWorldLocation(Home); Figures.Add(Figure); PreviousFigures.Add(Home);
            }
            return false;
        }
        const double Last=PreviousTime>0. ? PreviousTime : Now; const double Dt=FMath::Clamp(Now-Last,0.,.1); PreviousTime=Now;
        for (int32 I=0;I<Figures.Num();++I) if (auto* Figure=Figures[I].Get())
        {
            const FVector At=Homes[I]+FVector(FMath::Sin((Now-Warmup)*.65+I)*35,0,0),Step=At-PreviousFigures[I];
            Figure->SetWorldLocation(At); Figure->AdvanceLocomotion(Step,Dt); Figure->SetAim(Step.X<0 ? 180.f : 0.f); PreviousFigures[I]=At;
        }
        if (Now-Warmup<3.) return false;
        if (!bMeasured)
        {
            bMeasured=true; MeasureStart=Now; LastMeasured=Now;
            Folder=FPaths::ProjectSavedDir()/TEXT("Validation/S362Performance"); IFileManager::Get().MakeDirectory(*Folder,true);
            Test->TestTrue(TEXT("Inventory of actual loaded binary LODs/textures/components writes"),MemoriaPerformanceInventoryS362::WriteWorld(World,Folder/TEXT("loaded_start.json")));
#if CSV_PROFILER
            if (FCsvProfiler::Get()->IsCapturing())
            { Test->AddError(TEXT("An existing CSV capture is active; this scenario will not replace it")); return true; }
            FCsvProfiler::Get()->BeginCapture(-1,Folder,TEXT("EnginePresentationEight.csv")); bCapturing=true;
#else
            Test->AddWarning(TEXT("CSV profiler is unavailable; only explicit wall-frame CSV is written, no GPU/render-thread metrics claimed"));
#endif
        }
        if (!bEnded)
        {
            const double Elapsed=Now-MeasureStart;
            const int32 Segment=FMath::FloorToInt(Elapsed/2.)%4;
            if (Segment!=CurrentSegment)
            {
                static const FKey Keys[]={EKeys::W,EKeys::D,EKeys::S,EKeys::A};
                // Release only the old direction. An unmatched release of the NEXT key
                // followed by its press in this same input frame makes UE's PressDelta
                // zero, preserving bDownPrevious=false (PlayerInput::ProcessNonAxesKeys).
                if (CurrentSegment!=INDEX_NONE)
                    FSlateApplication::Get().ProcessKeyUpEvent(FKeyEvent(Keys[CurrentSegment],FModifierKeysState(),0,false,0,0));
                CurrentSegment=Segment;
                // The focused physical-key route updates both viewport and controller state.
                FSlateApplication::Get().ProcessKeyDownEvent(FKeyEvent(Keys[Segment],FModifierKeysState(),0,false,0,0));
                SegmentStarted=Now; bSegmentProbed=false;
            }
            static const FKey Keys[]={EKeys::W,EKeys::D,EKeys::S,EKeys::A};
            const auto* Combat=World->GetSubsystem<UMemoriaFieldCombatSubsystem>();
            const bool bKeyDown=C->IsInputKeyDown(Keys[Segment]);
            const bool bViewportFocus=World->GetGameViewport() && World->GetGameViewport()->Viewport && World->GetGameViewport()->Viewport->HasFocus();
            const bool bViewportKeyDown=World->GetGameViewport() && World->GetGameViewport()->Viewport && World->GetGameViewport()->Viewport->KeyState(Keys[Segment]);
            if (!bSegmentProbed && Now-SegmentStarted>.25)
            {
                bSegmentProbed=true;
                Test->AddInfo(FString::Printf(TEXT("S362_PERF_INPUT route=focused_slate key=%s down=%d viewportDown=%d focus=%d ignored=%d canMove=%d velocity=%s at=%s"),*Keys[Segment].ToString(),int32(bKeyDown),int32(bViewportKeyDown),int32(bViewportFocus),int32(C->IsMoveInputIgnored()),int32(!Combat || Combat->CanMove()),*Pawn->GetVelocity().ToString(),*Pawn->GetActorLocation().ToString()));
                Test->TestTrue(TEXT("Each actual held keyboard segment remains pressed after input ticks"),bKeyDown);
                Test->TestTrue(TEXT("Each physical keyboard segment is held in the actual viewport"),bViewportKeyDown);
                Test->TestFalse(TEXT("Actual keyboard profiling never ignores movement"),C->IsMoveInputIgnored());
                Test->TestTrue(TEXT("Actual keyboard profiling keeps combat movement available"),!Combat || Combat->CanMove());
            }
            const double Travel=FVector::Dist2D(Pawn->GetActorLocation(),PreviousPlayer); PreviousPlayer=Pawn->GetActorLocation(); TotalTravel+=Travel;
            if (Samples>0) Rows+=FString::Printf(TEXT("%llu,%.6f,%.6f,%.6f,8,%d,%d,%d,%d,%d,%.3f,%.3f\n"),GFrameCounter,(Now-LastMeasured)*1000.,Travel,Elapsed,int32(bKeyDown),int32(bViewportKeyDown),int32(bViewportFocus),int32(C->IsMoveInputIgnored()),int32(!Combat || Combat->CanMove()),PreviousPlayer.X,PreviousPlayer.Y);
            LastMeasured=Now; ++Samples;
            CSV_CUSTOM_STAT(MemoriaS362,FixtureArchetypes,8,ECsvCustomStatOp::Set);
            CSV_CUSTOM_STAT(MemoriaS362,PlayerTravelCm,float(Travel),ECsvCustomStatOp::Set);
            if (Elapsed<16.) return false;
            bEnded=true; EndedAt=Now; ReleaseAll();
            Test->TestTrue(TEXT("The actual player moved over the original keyboard route"),TotalTravel>200.);
            Test->TestTrue(TEXT("At least sixty rendered frames were measured"),Samples>=60);
#if CSV_PROFILER
            Completion=FCsvProfiler::Get()->EndCapture(); bCapturing=false;
#endif
            return false; // end the capture before serializing inventory/measurement artifacts
        }
#if CSV_PROFILER
        if (Completion.IsValid() && !Completion.IsReady())
        { if (Now-EndedAt>15.) { Test->AddError(TEXT("CSV profiler async write did not finish")); return true; } return false; }
        if (Completion.IsValid()) Test->AddInfo(TEXT("S362_ENGINE_CSV ")+Completion.Get());
#endif
        Test->TestTrue(TEXT("Actual end-of-scenario inventory writes"),MemoriaPerformanceInventoryS362::WriteWorld(World,Folder/TEXT("loaded_end.json")));
        Test->TestTrue(TEXT("Unfixed wall-frame sample CSV writes"),FFileHelper::SaveStringToFile(TEXT("engine_frame,wall_ms,player_step_cm,elapsed_seconds,presentation_archetypes,key_down,viewport_key_down,viewport_focus,move_ignored,combat_can_move,player_x,player_y\n")+Rows,*(Folder/TEXT("wall_frames.csv"))));
        Test->AddInfo(FString::Printf(TEXT("S362_PRESENTATION_PERF frames=%d travelled=%.3f fixture=8_rendering_archetypes no_AI no_optimizations folder=%s"),Samples,TotalTravel,*Folder));
        return true;
    }
private:
    void ReleaseAll()
    {
        if (FSlateApplication::IsInitialized())
            for (FKey Key : {EKeys::W,EKeys::A,EKeys::S,EKeys::D})
                FSlateApplication::Get().ProcessKeyUpEvent(FKeyEvent(Key,FModifierKeysState(),0,false,0,0));
    }
    FAutomationTestBase* Test; TWeakObjectPtr<AMemoriaSliceController> Controller;
    TArray<TWeakObjectPtr<AActor>> Fixtures; TArray<TWeakObjectPtr<UMemoriaFieldCharacterComponent>> Figures;
    TArray<FVector> Homes,PreviousFigures; FVector PreviousPlayer=FVector::ZeroVector;
    FString Folder,Rows; double Started,Warmup=0.,MeasureStart=0.,PreviousTime=0.,LastMeasured=0.,EndedAt=0.,TotalTravel=0.,SegmentStarted=0.;
    uint64 FrameId=MAX_uint64; int32 CurrentSegment=INDEX_NONE,Samples=0;
    bool bConfigured=false,bOldFixed=false,bBuilt=false,bMeasured=false,bEnded=false,bCapturing=false,bSegmentProbed=false;
    TSharedFuture<FString> Completion;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPresentationPerfS362,"MemoriaPerformance.S362.PresentationEight",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FPresentationPerfS362::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice"))) return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FPresentationEightS362(this)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand()); return true;
}
#endif
