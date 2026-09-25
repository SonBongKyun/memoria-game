#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "Tests/AutomationCommon.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Framework/MemoriaSliceHost.h"
#include "Framework/MemoriaFieldPawn.h"
#include "Presentation/MemoriaDevelopmentNarrativeWidget.h"
#include "Narrative/MemoriaNarrativeSubsystem.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Audio/MemoriaAudioSubsystem.h"
#include "Save/MemoriaRunSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/CameraComponent.h"
#include "InputKeyEventArgs.h"
#include "InputMappingContext.h"
#include "EnhancedInputSubsystems.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/App.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Serialization/JsonSerializer.h"
#include "JsonObjectConverter.h"
#include "UnrealClient.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
class FSliceReplay final : public IAutomationLatentCommand
{
public:
    FSliceReplay(FAutomationTestBase* InTest, FString InMode) : Test(InTest), Mode(InMode), Started(FPlatformTime::Seconds()) {}
    ~FSliceReplay() override { if (bStarted) { FApp::SetUseFixedTimeStep(bWasFixed); FApp::SetFixedDeltaTime(OldDelta); } }
    bool Update() override
    {
        if (LastFrame == GFrameCounter) return false;
        LastFrame = GFrameCounter;
        if (FPlatformTime::Seconds() - Started > 100) { Test->AddError(TEXT("Slice PIE replay timed out")); return true; }
        UWorld* World = GEditor->PlayWorld;
        auto* PC = World ? Cast<AMemoriaSliceController>(World->GetFirstPlayerController()) : nullptr;
        auto* Pawn = PC ? Cast<AMemoriaFieldPawn>(PC->GetPawn()) : nullptr;
        if (!Pawn || !PC->PlayerInput || World->GetTimeSeconds() < 0.3) return false;
        auto* Host = World->GetGameInstance()->GetSubsystem<UMemoriaNarrativeSubsystem>();
        auto* Run = World->GetGameInstance()->GetSubsystem<UMemoriaRunSubsystem>();
        const bool IsField = Mode == TEXT("UnseenFieldRoute");
        const bool IsResume = Mode == TEXT("HostContinuation");
        const bool IsFiltered = Mode == TEXT("FilteredOriginalChoice");
        auto Key = [&](FKey K, EInputEvent Event) { PC->InputKey(FInputKeyEventArgs::CreateSimulated(K, Event, Event == IE_Released ? 0.0f : 1.0f)); };
        auto Capture = [&](const FString& Suffix)
        {
            if (FParse::Param(FCommandLine::Get(), TEXT("MemoriaCapture")))
                FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Validation/Phase1E")/(Mode+Suffix+TEXT(".png")), true, false);
        };
        if (!bStarted)
        {
            bStarted = true; bWasFixed = FApp::UseFixedTimeStep(); OldDelta = FApp::GetFixedDeltaTime();
            FApp::SetFixedDeltaTime(1.0/60.0); FApp::SetUseFixedTimeStep(true);
            OriginalRun = Run->GetRunSnapshot(); Memory = Run->GetPlayerMemory(); OriginalWorld = World;
            Test->TestTrue(TEXT("Fresh run initialized through production catalog"), OriginalRun.RunId.IsValid() && Memory->GetSnapshot().Owned.Num() == 7);
            // Source ch1_after_forest sets chapter 2 before Verdan; the slice must not stay at New Game's 1.
            Test->TestEqual(TEXT("Slice run begins in its entry sequence's source chapter"), OriginalRun.CurrentChapter, int64(2));
            Test->TestEqual(TEXT("Memory rules read that chapter"), Run->GetMemoryContext().CurrentChapter, int64(2));
            Test->TestTrue(TEXT("Narrative owns real imported production asset"), Host->GetView().Header.Contains(IsField ? TEXT("FIELD") : TEXT("VN")));
            if (IsFiltered) Test->TestTrue(TEXT("Filter setup uses actual domain"), Run->BurnMemory(TEXT("daily_market_food")) == EMemoriaMemoryResult::Success);
            Origin = Pawn->GetActorLocation();
        }
        if (Frame == 5)
        {
            Test->TestTrue(TEXT("Real temporary UMG is present"), PC->GetNarrativeWidget() && PC->IsModalOpen());
            if (PC->GetNarrativeWidget())
            {
                Test->TestTrue(TEXT("Modal owns user focus"), PC->GetNarrativeWidget()->HasUserFocus(PC));
                Test->TestNotNull(TEXT("Authored arrival illustration is rendered"), PC->GetNarrativeWidget()->DisplayedBackdrop());
                Test->TestTrue(TEXT("Actual imported text visible"), PC->GetNarrativeWidget()->VisibleText().Contains(IsField ? TEXT("largest settlement") : TEXT("Verdan")));
            }
            Key(EKeys::D, IE_Pressed);
        }
        if (Frame == 12) { Capture(TEXT("_Text")); }
        if (Frame == 76 && !IsField) Capture(TEXT("_EliaPortrait"));
        if (Frame == 18)
        {
            Test->TestTrue(TEXT("Narrative blocks movement"), Pawn->GetActorLocation().Equals(Origin, 0.001)); Key(EKeys::D, IE_Released);
            Key(EKeys::Escape, IE_Pressed);
        }
        if (Frame == 22) { Key(EKeys::Escape, IE_Repeat); }
        if (Frame == 26)
        {
            Key(EKeys::Escape, IE_Released);
            Test->TestEqual(TEXT("Source VN pause permitted, Field cancel unavailable"), Host->IsPaused(), !IsField);
            Test->TestTrue(TEXT("Back never dismisses story modal"), PC->IsModalOpen());
            Test->TestEqual(TEXT("Back does not advance original step"), Host->GetContinuation().Current.OriginalIndex, 0);
        }
        if (Frame == 32 && !IsField) Key(EKeys::Gamepad_FaceButton_Right, IE_Pressed);
        if (Frame == 36 && !IsField) Key(EKeys::Gamepad_FaceButton_Right, IE_Released);
        // Input events enter the real EnhancedInput player; every press is released.
        if (Frame >= 50 && Frame <= (IsField ? 90 : 140) && Frame % 10 == 0) Key(EKeys::E, IE_Pressed);
        if (Frame >= 54 && Frame <= (IsField ? 94 : 144) && Frame % 10 == 4) Key(EKeys::E, IE_Released);
        if (Frame == 160 && !IsField)
        {
            if (!Test->TestEqual(TEXT("VN reaches original choice step 10"), Host->GetContinuation().Current.OriginalIndex, 10)) return true;
            Test->TestFalse(TEXT("Arrival flag absent before terminal"), Run->GetRunSnapshot().GetFlag(TEXT("ch2_arrival_vn_seen")));
            if (auto* Audio = World->GetGameInstance()->GetSubsystem<UMemoriaAudioSubsystem>(); Test->TestNotNull(TEXT("Audio subsystem exists"), Audio))
            {
                // Source VN metadata bgm is ch2_verdan; dialogue ducks it.
                Test->TestEqual(TEXT("Arrival VN plays its declared BGM"), Audio->GetMusic(), FName(TEXT("ch2_verdan")));
                Test->TestTrue(TEXT("Dialogue ducks the BGM"), Audio->IsDucked());
                Test->TestTrue(TEXT("Advancing lines plays confirm"), Audio->GetCueCount(TEXT("confirm")) > 0);
            }
            Test->TestEqual(TEXT("Presented choice count respects actual filter"), Host->GetView().Choices.Num(), IsFiltered ? 2 : 3);
            Capture(TEXT("_Choices"));
            if (IsResume)
            {
                auto* Save = Host->CaptureSave(); if (!Test->TestNotNull(TEXT("Actual host active save"), Save)) return true;
                Save->SceneFlow.Pending.SequenceId = TEXT("ch2_market_arrival"); Save->SceneFlow.Pending.OriginalIndex = 1;
                FMemoriaVNCursor A; A.SequenceId = TEXT("ch2_market_arrival"); A.OriginalIndex = 3;
                auto B = A; B.OriginalIndex = 4; Save->SceneFlow.ResumeQueue = {A, B};
                TArray<uint8> Bytes; Test->TestTrue(TEXT("SaveGame serialization"), UGameplayStatics::SaveGameToMemory(Save, Bytes));
                auto* Loaded = Cast<UMemoriaRunSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
                if (!Test->TestNotNull(TEXT("SaveGame deserialization"), Loaded)) return true;
                Test->TestEqual(TEXT("Save schema stays 1"), Loaded->SchemaVersion, 1);
                Test->TestTrue(TEXT("Saved cursor active"), Loaded->SceneFlow.bActive);
                Test->TestEqual(TEXT("Saved sequence exact case"), Loaded->SceneFlow.Current.SequenceId, FString(TEXT("ch2_market_arrival")));
                Test->TestEqual(TEXT("Saved pending preserved"), Loaded->SceneFlow.Pending.OriginalIndex, 1);
                Test->TestTrue(TEXT("Host validates/restores"), Host->PrepareRestore(*Loaded));
                auto C = Host->GetContinuation();
                Test->TestFalse(TEXT("Prepare becomes inactive"), C.bActive);
                Test->TestEqual(TEXT("Active cursor overrides pending"), C.Pending.OriginalIndex, 10);
                Test->TestTrue(TEXT("Current preserved in prepare"), C.Current.OriginalIndex == 10 && C.Current.SequenceId == TEXT("ch2_market_arrival"));
                Test->TestTrue(TEXT("FIFO preserved"), C.ResumeQueue.Num() == 2 && C.ResumeQueue[0].OriginalIndex == 3 && C.ResumeQueue[1].OriginalIndex == 4);
                Test->TestTrue(TEXT("Host consumes prepared cursor"), Host->ResumePrepared());
                C = Host->GetContinuation();
                Test->TestTrue(TEXT("Resume active at original 10, pending consumed, FIFO intact"), C.bActive && C.Current.OriginalIndex == 10 && C.Pending.SequenceId.IsEmpty() && C.ResumeQueue.Num() == 2);
                Loaded->SceneFlow.Current.SequenceId = TEXT("CH2_MARKET_ARRIVAL");
                Test->TestFalse(TEXT("Invalid restore rejected atomically"), Host->PrepareRestore(*Loaded));
                Test->TestTrue(TEXT("Invalid restore leaves active cursor/run"), Host->GetContinuation().bActive && Run->GetRunSnapshot().RunId == OriginalRun.RunId);
                WriteEvidence(Host, Run, Pawn, TEXT("continuation"));
            }
        }
        if (IsResume && Frame == 168)
        {
            Test->TestTrue(TEXT("Restored presentation rebuilt at choice"), PC->GetNarrativeWidget() && PC->GetNarrativeWidget()->VisibleText().Contains(TEXT("hunters")));
            // New run cancels old borrowed interpreters without leaving a stale modal.
            Run->BeginStartingMemoryRun();
        }
        if (IsResume && Frame == 175)
        {
            Test->TestTrue(TEXT("Run replacement cancels host and clears old view"), Host->GetState() == EMemoriaSliceState::Idle && !PC->IsModalOpen());
            return true;
        }
        if (Frame == 170 && !IsField) Key(IsFiltered ? EKeys::Gamepad_DPad_Down : EKeys::Down, IE_Pressed);
        if (Frame == 174 && !IsField) Key(IsFiltered ? EKeys::Gamepad_DPad_Down : EKeys::Down, IE_Released);
        if (Frame == 180 && !IsField)
        {
            if (!PC->GetNarrativeWidget()) { Test->AddError(TEXT("Missing choices widget")); return true; }
            Test->TestEqual(TEXT("Visible second choice retains original index"), PC->GetNarrativeWidget()->SelectedOriginalIndex(), IsFiltered ? 2 : 1);
        }
        if (Frame == 188 && !IsField) Capture(TEXT("_SelectedChoice"));
        if (Frame == 200 && !IsField) Key(EKeys::Gamepad_FaceButton_Bottom, IE_Pressed);
        if (Frame == 204 && !IsField) Key(EKeys::Gamepad_FaceButton_Bottom, IE_Released);
        if (Frame == 220 && !IsField)
        {
            Test->TestEqual(TEXT("Choice advances to 11"), Host->GetContinuation().Current.OriginalIndex, 11);
            Test->TestFalse(TEXT("Arrival flag still absent at 11"), Run->GetRunSnapshot().GetFlag(TEXT("ch2_arrival_vn_seen")));
        }
        if (Frame == 230 && !IsField) Key(EKeys::Enter, IE_Pressed);
        if (Frame == 234 && !IsField) Key(EKeys::Enter, IE_Released);
        if (Frame == 260)
        {
            Test->TestTrue(TEXT("Exploration activates"), Host->GetState() == EMemoriaSliceState::Exploration);
            Test->TestEqual(TEXT("Actual Field invocation count"), Host->GetFieldInvocationCount(), IsField ? 1 : 0);
            Test->TestTrue(TEXT("Travel preserves run identity"), Run->GetRunSnapshot().RunId == OriginalRun.RunId);
            Test->TestTrue(TEXT("Travel preserves domain instance"), Run->GetPlayerMemory() == Memory.Get());
            Test->TestTrue(TEXT("Valid run and seven imported memories"), Run->GetRunSnapshot().IsValid() && Run->GetPlayerMemory()->GetSnapshot().Owned.Num() == 7);
            Test->TestTrue(TEXT("Actual native world replacement on VN route"), IsField || (World != OriginalWorld && World->GetMapName().EndsWith(TEXT("L_VerdanHost"))));
            Test->TestTrue(TEXT("No active narrative modal or choices"), !PC->IsModalOpen() && !PC->GetNarrativeWidget() && Host->GetView().Choices.IsEmpty());
            Test->TestFalse(TEXT("Movement input unblocked"), PC->IsMoveInputIgnored());
            auto* Input = PC->GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
            auto* ModalContext = LoadObject<UInputMappingContext>(nullptr, TEXT("/Game/Tests/Foundation/IMC_Modal.IMC_Modal"));
            auto* ExploreContext = LoadObject<UInputMappingContext>(nullptr, TEXT("/Game/Tests/Foundation/IMC_Foundation.IMC_Foundation"));
            Test->TestTrue(TEXT("Modal context removed; exploration restored"), !Input->HasMappingContext(ModalContext) && Input->HasMappingContext(ExploreContext));
            Test->TestTrue(TEXT("Viewport focus restored"), FSlateApplication::Get().GetUserFocusedWidget(0) == World->GetGameViewport()->GetGameViewportWidget());
            CompareOracle(Host, Run, IsField ? TEXT("source_field_en") : IsFiltered ? TEXT("source_vn_filtered_original") : TEXT("source_vn_choice_1"));
            if (auto* Audio = World->GetGameInstance()->GetSubsystem<UMemoriaAudioSubsystem>(); Test->TestNotNull(TEXT("Audio subsystem exists"), Audio))
            {
                Test->TestEqual(TEXT("Verdan exploration plays the source market BGM"), Audio->GetMusic(), FName(TEXT("ch2_verdan")));
                Test->TestEqual(TEXT("Verdan exploration adds the source light wind"), Audio->GetAmbient(), FName(TEXT("wind_light")));
                Test->TestFalse(TEXT("Exploration restores full BGM volume"), Audio->IsDucked());
                Test->TestTrue(TEXT("Dialogue advance played confirm"), Audio->GetCueCount(TEXT("confirm")) > 0);
                if (!IsField) Test->TestTrue(TEXT("Choosing a VN option played ui_select"), Audio->GetCueCount(TEXT("ui_select")) > 0);
            }
            Origin = Pawn->GetActorLocation(); CameraOrigin = Pawn->GetFieldCamera()->GetComponentLocation();
            Capture(TEXT("_Exploration")); Key(EKeys::D, IE_Pressed);
        }
        if (Frame == 290)
        {
            Key(EKeys::D, IE_Released);
            Test->TestTrue(TEXT("Movement works after handoff and modal"), Pawn->GetActorLocation().X > Origin.X + 10);
            Test->TestTrue(TEXT("Plane Z fixed"), FMath::IsNearlyZero(Pawn->GetActorLocation().Z, 0.001));
            Test->TestTrue(TEXT("Foundation camera follows actual pawn displacement"), (Pawn->GetFieldCamera()->GetComponentLocation() - CameraOrigin).Equals(Pawn->GetActorLocation() - Origin, 0.001));
            WriteEvidence(Host, Run, Pawn, TEXT("exploration"));
            // Reentry must not invoke arrival again even for the Field fixture.
            const int32 Before = Host->GetFieldInvocationCount(); Host->EnterVerdan();
            Test->TestEqual(TEXT("Already-arrived source guard prevents repeat"), Host->GetFieldInvocationCount(), Before);
        }
        if (Frame == 300) Capture(TEXT("_Moved"));
        if (Frame == 310) { Pawn->SetActorLocation(FVector(850, 0, 0)); Key(EKeys::D, IE_Pressed); }
        if (Frame == 340)
        {
            Key(EKeys::D, IE_Released);
            Test->TestTrue(TEXT("Swept boundary collision contains pawn"), Pawn->GetActorLocation().X > 850 && Pawn->GetActorLocation().X <= 882.1);
            Test->TestTrue(TEXT("Collision holds plane"), FMath::IsNearlyZero(Pawn->GetActorLocation().Z, 0.001));
            return true;
        }
        ++Frame; return false;
    }
private:
    FAutomationTestBase* Test; FString Mode;
    double Started, OldDelta = 0; bool bStarted = false, bWasFixed = false;
    int32 Frame = 0; uint64 LastFrame = MAX_uint64;
    FMemoriaRunSnapshot OriginalRun; TWeakObjectPtr<UMemoriaPlayerMemoryDomain> Memory;
    TWeakObjectPtr<UWorld> OriginalWorld;
    FVector Origin, CameraOrigin;
    TArray<FString> Normalized(UMemoriaNarrativeSubsystem* Host)
    {
        TArray<FString> Events;
        for (auto Event : Host->GetTrace())
        {
            if (Event.StartsWith(TEXT("vn:")) && !Event.StartsWith(TEXT("vn:start:"))) Event.RightChopInline(3);
            if (Event.StartsWith(TEXT("field:")) && !Event.StartsWith(TEXT("field:start:")) && !Event.StartsWith(TEXT("field:skip:"))) Event.RightChopInline(6);
            Events.Add(Event);
        }
        return Events;
    }
    void CompareOracle(UMemoriaNarrativeSubsystem* Host, UMemoriaRunSubsystem* Run, const FString& Id)
    {
        FString Text; FFileHelper::LoadFileToString(Text, *(FPaths::ProjectDir()/TEXT("../../docs/unreal-migration/fixtures/campaign/contract_expected.v1.json")));
        TArray<TSharedPtr<FJsonValue>> Cases;
        if (!Test->TestTrue(TEXT("Source-authentic route oracle loads"), FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Cases))) return;
        for (auto Case : Cases) if (Case->AsObject()->GetStringField(TEXT("id")) == Id)
        {
            const auto Expected = Case->AsObject()->GetArrayField(TEXT("states"))[0]->AsObject();
            TArray<FString> ExpectedEvents; for (auto E : Expected->GetArrayField(TEXT("events"))) ExpectedEvents.Add(E->AsString());
            Test->TestEqual(TEXT("Exact ordered source route trace"), FString::Join(Normalized(Host), TEXT("\n")), FString::Join(ExpectedEvents, TEXT("\n")));
            auto State = Run->GetRunSnapshot(); auto Flags = Expected->GetObjectField(TEXT("flags"));
            Test->TestEqual(TEXT("Exact final flag count"), State.StoryFlags.Num(), Flags->Values.Num());
            for (const auto& Flag : Flags->Values) Test->TestEqual(*Flag.Key, State.GetFlag(FString(Flag.Key)), Flag.Value->AsBool());
            Test->TestEqual(TEXT("Source HP"), State.Player.Hp, static_cast<int64>(Expected->GetNumberField(TEXT("hp"))));
            Test->TestEqual(TEXT("Source grains"), State.Player.Grains, static_cast<int64>(Expected->GetNumberField(TEXT("grains"))));
            TArray<FString> Burned; for (auto E : Expected->GetArrayField(TEXT("burned"))) Burned.Add(E->AsString());
            Test->TestTrue(TEXT("Source burned-memory history"), Run->GetPlayerMemory()->GetSnapshot().BurnedHistory == Burned);
            return;
        }
        Test->AddError(TEXT("Missing oracle case"));
    }
    void WriteEvidence(UMemoriaNarrativeSubsystem* Host, UMemoriaRunSubsystem* Run, AMemoriaFieldPawn* Pawn, const FString& State)
    {
        auto Object = MakeShared<FJsonObject>(); Object->SetStringField(TEXT("mode"), Mode); Object->SetStringField(TEXT("state"), State);
        TArray<TSharedPtr<FJsonValue>> Events; for (auto E : Host->GetTrace()) Events.Add(MakeShared<FJsonValueString>(E)); Object->SetArrayField(TEXT("trace"), Events);
        Object->SetObjectField(TEXT("run"), FJsonObjectConverter::UStructToJsonObject(Run->GetRunSnapshot()));
        Object->SetObjectField(TEXT("memory"), FJsonObjectConverter::UStructToJsonObject(Run->GetPlayerMemory()->GetSnapshot()));
        Object->SetObjectField(TEXT("continuation"), FJsonObjectConverter::UStructToJsonObject(Host->GetContinuation()));
        Object->SetNumberField(TEXT("field_invocations"), Host->GetFieldInvocationCount());
        Object->SetStringField(TEXT("pawn_position"), Pawn->GetActorLocation().ToString());
        FString Text; FJsonSerializer::Serialize(Object, TJsonWriterFactory<>::Create(&Text));
        const FString Dir = FPaths::ProjectSavedDir()/TEXT("Validation/Phase1E"); IFileManager::Get().MakeDirectory(*Dir, true);
        FFileHelper::SaveStringToFile(Text, *(Dir/(Mode+TEXT(".json"))), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    }
};
}
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FMemoriaSliceTest, "Memoria.Campaign", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
void FMemoriaSliceTest::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{
    for (const TCHAR* Name : {TEXT("CanonicalPaidRoute"), TEXT("FilteredOriginalChoice"), TEXT("UnseenFieldRoute"), TEXT("HostContinuation")})
    { Names.Add(Name); Commands.Add(Name); }
}
bool FMemoriaSliceTest::RunTest(const FString& Parameters)
{
    if (!AutomationOpenMap(Parameters == TEXT("UnseenFieldRoute") ? TEXT("/Game/Tests/Campaign/L_VerdanUnseenFixture") : TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice"))) return false;
    // UE's editor AutomationOpenMap delegate already starts PIE and queues the
    // map-ready wait. A second FStartPIECommand replaces the session mid-probe.
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FSliceReplay(this, Parameters)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand()); return true;
}
#endif
