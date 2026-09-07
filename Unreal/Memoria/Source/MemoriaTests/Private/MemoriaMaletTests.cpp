#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "Tests/AutomationCommon.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "EngineUtils.h"
#include "Framework/MemoriaSliceHost.h"
#include "Framework/MemoriaFieldPawn.h"
#include "Interaction/MemoriaMaletActor.h"
#include "Interaction/MemoriaInteractionComponent.h"
#include "Narrative/MemoriaMaletReaction.h"
#include "Narrative/MemoriaNarrativeSubsystem.h"
#include "Presentation/MemoriaDevelopmentNarrativeWidget.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Import/MemoriaNarrativeImport.h"
#include "Import/MemoriaStartingCatalogImport.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"
#include "Serialization/JsonSerializer.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/App.h"
#include "Misc/Parse.h"
#include "Misc/CommandLine.h"
#include "InputKeyEventArgs.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Framework/Application/SlateApplication.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PawnMovementComponent.h"
#include "UnrealClient.h"
#include "Widgets/SWindow.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
using Obj = TSharedPtr<FJsonObject>;
using Val = TSharedPtr<FJsonValue>;
FString Base() { return FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()/TEXT("../../docs/unreal-migration/")); }
FString Ir() { return Base()/TEXT("ir/narrative/malet_taste_burned.field.v1.json"); }
Val Json(const FString& Path)
{
    FString Text; Val Value;
    if (FFileHelper::LoadFileToString(Text, *Path)) FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Value);
    return Value;
}
Obj Case(const FString& Id, bool Inputs = false)
{
    auto Value = Json(Base()/TEXT("fixtures/malet")/(Inputs?TEXT("contract_inputs.v1.json"):TEXT("contract_expected.v1.json")));
    if (Value) for (const auto& Item : Value->AsArray())
        if (Item->AsObject()->GetStringField(TEXT("id")) == Id)
            return Inputs ? Item->AsObject() : Item->AsObject()->GetArrayField(TEXT("states"))[0]->AsObject();
    return nullptr;
}
TArray<FString> Strings(const TArray<Val>& Values)
{ TArray<FString> Result; for (auto Value : Values) Result.Add(Value->AsString()); return Result; }
FString Canon(const Obj& Value) { return MemoriaCatalogImport::Canonical(MakeShared<FJsonValueObject>(Value)); }
template<class T> FString StateJson(const T& Value) { return Canon(FJsonObjectConverter::UStructToJsonObject(Value)); }
UMemoriaFieldAsset* Asset()
{ return LoadObject<UMemoriaFieldAsset>(nullptr, *MemoriaNarrativeImport::ObjectPath(false, MemoriaMaletReaction::Group)); }
TArray<FString> Normalize(const TArray<FString>& Trace)
{
    TArray<FString> Result;
    for (auto Event : Trace)
    {
        if (Event.StartsWith(TEXT("vn:")) && !Event.StartsWith(TEXT("vn:start:"))) Event.RightChopInline(3);
        if (Event.StartsWith(TEXT("field:")) && !Event.StartsWith(TEXT("field:start:")) && !Event.StartsWith(TEXT("field:skip:"))) Event.RightChopInline(6);
        Result.Add(Event);
    }
    return Result;
}
void Compare(FAutomationTestBase& Test, const TArray<FString>& Events, const FMemoriaRunSnapshot& Run,
    const FMemoriaMemorySnapshot& Memory, const Obj& Expected)
{
    Test.TestEqual(TEXT("Exact source-authentic ordered trace"), FString::Join(Events, TEXT("\n")), FString::Join(Strings(Expected->GetArrayField(TEXT("events"))), TEXT("\n")));
    auto Flags = Expected->GetObjectField(TEXT("flags"));
    Test.TestEqual(TEXT("Exact source flag count"), Run.StoryFlags.Num(), Flags->Values.Num());
    for (const auto& Flag : Flags->Values)
        Test.TestEqual(*FString(Flag.Key.ToView()), Run.GetFlag(FString(Flag.Key.ToView())), Flag.Value->AsBool());
    Test.TestTrue(TEXT("Exact source burn history"), Memory.BurnedHistory == Strings(Expected->GetArrayField(TEXT("burned"))));
    Test.TestEqual(TEXT("Source HP preserved"), Run.Player.Hp, int64(Expected->GetNumberField(TEXT("hp"))));
    Test.TestEqual(TEXT("Source Grains preserved"), Run.Player.Grains, int64(Expected->GetNumberField(TEXT("grains"))));
}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMaletImport, "Memoria.Malet.ImportContract", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMaletImport::RunTest(const FString&)
{
    TStrongObjectPtr<UMemoriaFieldAsset> Actual(Asset()), Expected(NewObject<UMemoriaFieldAsset>()); FString Error;
    if (!TestNotNull(TEXT("Actual saved reaction asset"), Actual.Get()) ||
        !TestTrue(TEXT("Strict typed IR and authored source attestation"), MemoriaNarrativeImport::ReadIr(Ir(), *Expected, Error))) { AddError(Error); return false; }
    TestEqual(TEXT("Exactly three authored rows"), Actual->Definition.Rows.Num(), 3);
    for (TFieldIterator<FProperty> P(UMemoriaFieldAsset::StaticClass(), EFieldIterationFlags::None); P; ++P)
        TestTrue(*P->GetName(), P->Identical_InContainer(Actual.Get(), Expected.Get()));
    for (int32 I = 0; I < Actual->Definition.Rows.Num(); ++I)
    {
        const auto& Row = Actual->Definition.Rows[I];
        TestEqual(TEXT("Original source index"), Row.OriginalIndex, I);
        TestEqual(TEXT("Original group position"), Row.Provenance.GroupPosition, 16);
        TestTrue(TEXT("English and Korean presence retained"), Row.Text.bHasText && Row.Text.bHasTextKo);
    }
    Obj Report;
    TestTrue(TEXT("Unchanged import in loaded process"), MemoriaNarrativeImport::Import(Ir(), false, true, Report, Error));
    if (Report)
    {
        TestEqual(TEXT("Semantic no-op"), Report->GetStringField(TEXT("result")), FString(TEXT("UNCHANGED")));
        TestFalse(TEXT("No unnecessary save"), Report->GetBoolField(TEXT("saved")));
    }
    const auto Directory = FPaths::ProjectSavedDir()/TEXT("Validation/Phase1FTemporary");
    IFileManager::Get().MakeDirectory(*Directory, true);
    const auto Temporary = Directory/TEXT("modified_ir.json");
    TestEqual(TEXT("Temporary semantic probe copied outside production"), IFileManager::Get().Copy(*Temporary, *(Base()/TEXT("fixtures/malet/modified_semantic.field.v1.json"))), COPY_OK);
    TStrongObjectPtr<UMemoriaFieldAsset> Changed(NewObject<UMemoriaFieldAsset>());
    TestTrue(TEXT("Temporary typed semantic change loads"), MemoriaNarrativeImport::ReadIr(Temporary, *Changed, Error, false));
    TestTrue(TEXT("Typed fingerprint detects changed text"), MemoriaNarrativeImport::Fingerprint(*Changed) != MemoriaNarrativeImport::Fingerprint(*Actual));
    TestFalse(TEXT("Changed authored content cannot be promoted"), MemoriaNarrativeImport::ReadIr(Temporary, *Changed, Error, true));
    TestTrue(TEXT("Temporary object never becomes a package"), Changed->GetOutermost() == GetTransientPackage());
    for (const TCHAR* Name : {TEXT("group_position"), TEXT("count"), TEXT("unknown_group")})
        TestFalse(Name, MemoriaNarrativeImport::ReadIr(Base()/TEXT("fixtures/malet")/(FString(TEXT("reject_"))+Name+TEXT(".field.v1.json")), *Changed, Error, false));
    return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMaletDispatch, "Memoria.Malet.SourceDispatch", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMaletDispatch::RunTest(const FString&)
{
    TStrongObjectPtr<UGameInstance> Game(NewObject<UGameInstance>()); Game->Init();
    auto* Run = Game->GetSubsystem<UMemoriaRunSubsystem>();
    auto* Reaction = Asset();
    if (!TestNotNull(TEXT("Typed reaction"), Reaction)) { Game->Shutdown(); return false; }
    for (const TCHAR* Id : {TEXT("burned_unheard"), TEXT("burned_heard"), TEXT("intact_unheard"), TEXT("burned_unheard_talked"), TEXT("burned_heard_talked"), TEXT("missing_group")})
    {
        auto Input = Case(Id, true), Expected = Case(Id);
        if (!TestTrue(TEXT("Real source oracle case"), Input.IsValid() && Expected.IsValid())) { Game->Shutdown(); return false; }
        TestTrue(TEXT("Real imported catalog run"), Run->BeginStartingMemoryRun() == EMemoriaMemoryResult::Success);
        if (Input->GetBoolField(TEXT("burned"))) TestTrue(TEXT("Burn uses existing domain"), Run->BurnMemory(MemoriaMaletReaction::Food) == EMemoriaMemoryResult::Success);
        if (Input->GetBoolField(TEXT("heard"))) Run->SetStoryFlag(MemoriaMaletReaction::Heard, true);
        if (Input->GetBoolField(TEXT("talked"))) Run->SetStoryFlag(TEXT("talked_Malet_malet_encounter"), true);
        const FString Before = StateJson(Run->GetPlayerMemory()->GetSnapshot());
        const auto Dispatch = MemoriaMaletReaction::Resolve(*Run, false, Input->GetBoolField(TEXT("missing")) ? nullptr : Reaction);
        TArray<FString> Events{TEXT("interact:Malet")}; Events.Append(Dispatch.Events);
        Events.Add(TEXT("request:") + Dispatch.File + TEXT("::") + Dispatch.Group);
        if (Dispatch.bReaction)
        {
            TestTrue(TEXT("Heard flag observable before Field Start"), Run->GetRunSnapshot().GetFlag(MemoriaMaletReaction::Heard));
            TestTrue(TEXT("Reaction precedes normal or authored repeat"), Dispatch.Group == MemoriaMaletReaction::Group);
            TestTrue(TEXT("Active dialogue cannot consume another reaction"), MemoriaMaletReaction::Resolve(*Run, true, Reaction).Group.IsEmpty());
            Events.Add(FString(TEXT("field:start:")) + Dispatch.Group + TEXT(":heard=true"));
            auto State = Run->GetRunSnapshot(); FMemoriaNarrativeContext Context(State, *Run->GetPlayerMemory());
            FMemoriaFieldInterpreter Field(Reaction->Definition, Context); Field.Start();
            for (int32 I = 0; I < 3; ++I) { TestEqual(TEXT("Rows execute in original order"), Field.OriginalIndex(), I); Field.Advance(); }
            TestFalse(TEXT("Field completes after third row"), Field.IsActive());
            Events.Append(Context.Events); Events.Add(TEXT("exploration:ready"));
        }
        else Events.Add(TEXT("development:deferred:") + Dispatch.Group);
        Compare(*this, Events, Run->GetRunSnapshot(), Run->GetPlayerMemory()->GetSnapshot(), Expected);
        TestEqual(TEXT("No re-burn or hidden memory mutation"), StateJson(Run->GetPlayerMemory()->GetSnapshot()), Before);
    }
    // Faded/absent is not an alias for source is_memory_burned.
    Run->BeginStartingMemoryRun(); auto Memory = Run->GetPlayerMemory()->GetSnapshot();
    for (auto& M : Memory.Owned) if (M.Id == MemoriaMaletReaction::Food) M.bFaded = true;
    TestTrue(TEXT("Faded setup through validated domain restore"), Run->RestoreRun(Run->GetRunSnapshot(), Run->GetPlayerMemory()->GetDefinitions(), Memory) == EMemoriaMemoryResult::Success);
    TestFalse(TEXT("Faded intact predicate is false"), Run->GetPlayerMemory()->IsIntact(MemoriaMaletReaction::Food));
    TestFalse(TEXT("Faded without history does not trigger burn reaction"), MemoriaMaletReaction::Resolve(*Run, false, Reaction).bReaction);
    Game->Shutdown(); return !HasAnyErrors();
}
namespace
{
class FMaletReplay final : public IAutomationLatentCommand
{
public:
    FMaletReplay(FAutomationTestBase* InTest, bool Paid) : Test(InTest), bPaid(Paid), Started(FPlatformTime::Seconds()) {}
    ~FMaletReplay() override { if (bStarted) { FApp::SetUseFixedTimeStep(bWasFixed); FApp::SetFixedDeltaTime(OldDelta); } }
    bool Update() override
    {
        if (LastFrame == GFrameCounter) return false; LastFrame = GFrameCounter;
        if (FPlatformTime::Seconds() - Started > 120) { Test->AddError(TEXT("Malet actual input replay timed out")); return true; }
        UWorld* World = GEditor->PlayWorld;
        auto* PC = World ? Cast<AMemoriaSliceController>(World->GetFirstPlayerController()) : nullptr;
        auto* Pawn = PC ? Cast<AMemoriaFieldPawn>(PC->GetPawn()) : nullptr;
        if (!Pawn || !PC->PlayerInput || World->GetTimeSeconds() < .3) return false;
        auto* Run = World->GetGameInstance()->GetSubsystem<UMemoriaRunSubsystem>();
        auto* Host = World->GetGameInstance()->GetSubsystem<UMemoriaNarrativeSubsystem>();
        auto Key = [&](FKey K, EInputEvent Event) { PC->InputKey(FInputKeyEventArgs::CreateSimulated(K, Event, Event == IE_Released ? 0.f : 1.f)); };
        auto Capture = [&](const FString& Label)
        {
            if (FParse::Param(FCommandLine::Get(), TEXT("MemoriaCapture")))
                FScreenshotRequest::RequestScreenshot(Output()/(Mode()+TEXT("_")+Label+TEXT(".png")), true, false);
        };
        if (!bStarted)
        {
            bStarted = true; bWasFixed = FApp::UseFixedTimeStep(); OldDelta = FApp::GetFixedDeltaTime();
            FApp::SetUseFixedTimeStep(true); FApp::SetFixedDeltaTime(1.0/60.0);
            // Resize the actual PIE window, independent of remembered editor
            // client dimensions that shrink across unattended map sessions.
            if (auto Window = World->GetGameViewport()->GetWindow()) Window->Resize(FVector2D(1280, 720));
            RunId = Run->GetRunSnapshot().RunId; Domain = Run->GetPlayerMemory(); OriginalWorld = World;
            if (!Test->TestNotNull(TEXT("Imported Malet asset ready"), Asset())) return true;
        }
        if (Stage == 0)
        {
            if (Frame >= 10 && Frame <= 100 && Frame % 10 == 0) Key(EKeys::E, IE_Pressed);
            if (Frame >= 14 && Frame <= 104 && Frame % 10 == 4) Key(EKeys::E, IE_Released);
            if (Frame == 115)
            {
                if (!Test->TestEqual(TEXT("Original VN choice step"), Host->GetContinuation().Current.OriginalIndex, 10)) return true;
                if (bPaid) Key(EKeys::Down, IE_Pressed);
            }
            if (Frame == 119 && bPaid) Key(EKeys::Down, IE_Released);
            if (Frame == 125) Key(EKeys::Gamepad_FaceButton_Bottom, IE_Pressed);
            if (Frame == 129) Key(EKeys::Gamepad_FaceButton_Bottom, IE_Released);
            if (Frame == 140)
            {
                Test->TestEqual(TEXT("VN advances after selected original choice"), Host->GetContinuation().Current.OriginalIndex, 11);
                Test->TestFalse(TEXT("VN-seen absent before terminal"), Run->GetRunSnapshot().GetFlag(TEXT("ch2_arrival_vn_seen")));
            }
            if (Frame == 150) Key(EKeys::Enter, IE_Pressed);
            if (Frame == 154) Key(EKeys::Enter, IE_Released);
            if (Frame == 180)
            {
                if (!Test->TestTrue(TEXT("Real travel reaches Verdan exploration"), World != OriginalWorld && World->GetMapName().EndsWith(TEXT("L_VerdanHost")) && Host->GetState() == EMemoriaSliceState::Exploration)) return true;
                Test->TestEqual(TEXT("Canonical arrival still skips Field"), Host->GetFieldInvocationCount(), 0);
                Test->TestTrue(TEXT("Run/domain survive real travel"), RunId == Run->GetRunSnapshot().RunId && Domain.Get() == Run->GetPlayerMemory());
                Test->TestEqual(TEXT("Paid VN is the sole source of burned food"), Run->GetPlayerMemory()->GetSnapshot().BurnedHistory.Contains(MemoriaMaletReaction::Food), bPaid);
                int32 Count = 0; for (TActorIterator<AMemoriaMaletActor> It(World); It; ++It) { ++Count; Malet = *It; }
                if (!Test->TestEqual(TEXT("Exactly one actual placeholder Malet"), Count, 1)) return true;
                Test->TestTrue(TEXT("Authored deterministic Malet location"), Malet->GetActorLocation().Equals(AMemoriaMaletActor::DevelopmentLocation()));
                BeforeMemory = StateJson(Run->GetPlayerMemory()->GetSnapshot());
                BeforeRun = Run->GetRunSnapshot(); ArrivalTraceCount = Host->GetTrace().Num();
                Test->TestTrue(TEXT("Rendered viewport has readable evidence dimensions"), World->GetGameViewport()->Viewport->GetSizeXY().Y >= 600);
                Capture(TEXT("BeforeInteraction"));
            }
            if (Frame == 190) Key(EKeys::E, IE_Pressed);
            if (Frame == 194) Key(EKeys::E, IE_Released);
            if (Frame == 199)
            {
                Test->TestEqual(TEXT("Out of range Interact does not reach host"), Host->GetTrace().Num(), ArrivalTraceCount);
                Stage = 1; Frame = -1;
            }
        }
        else if (Stage == 1)
        {
            const FVector Goal(240, -95, 0), Delta = Goal - Pawn->GetActorLocation();
            // Physical-key pulses allow deterministic approach with the accepted
            // movement acceleration/deceleration, without teleporting to the NPC.
            if (Frame % 5 == 0)
            {
                if (FMath::Abs(Delta.X) > 8) Held = Delta.X > 0 ? EKeys::D : EKeys::A;
                else if (FMath::Abs(Delta.Y) > 8) Held = Delta.Y > 0 ? EKeys::W : EKeys::S;
                else Held = FKey();
                if (Held.IsValid()) Key(Held, IE_Pressed);
            }
            if (Frame % 5 == 1 && Held.IsValid()) Key(Held, IE_Released);
            if (FMath::Abs(Delta.X) <= 8 && FMath::Abs(Delta.Y) <= 8 && Pawn->GetVelocity().Size() < .01)
            { if (Held.IsValid()) Key(Held, IE_Released); Stage = 2; Frame = -1; }
        }
        else if (Stage == 2)
        {
            if (Frame == 5)
            {
                Test->TestTrue(TEXT("Player moved into actual NPC range"), Pawn->GetActorLocation().Size() > 150 && Malet->CanInteract(*Pawn));
                Test->TestTrue(TEXT("Overlap/interface resolver selects Malet"), PC->GetInteraction()->GetTarget() == Malet.Get());
                Test->TestTrue(TEXT("Runtime interaction prompt visible"), PC->GetInteractionPrompt().Contains(TEXT("Malet")));
                Capture(TEXT("Prompt")); BeforePosition = Pawn->GetActorLocation();
            }
            if (Frame == 10) Key(EKeys::E, IE_Pressed);
            if (Frame == 14) Key(EKeys::E, IE_Repeat);
            if (Frame == 20) Key(EKeys::E, IE_Released);
            if (Frame == 25)
            {
                if (!bPaid)
                {
                    Test->TestEqual(TEXT("Intact route resolves exact normal target without executing"), Host->GetDeferredInteraction(), FString(TEXT("malet_encounter")));
                    Test->TestEqual(TEXT("Intact fallback never starts Field"), Host->GetFieldInvocationCount(), 0);
                    Test->TestFalse(TEXT("Intact fallback does not consume heard flag"), Run->GetRunSnapshot().GetFlag(MemoriaMaletReaction::Heard));
                    Test->TestTrue(TEXT("Fallback stays exploration, no modal"), Host->GetState() == EMemoriaSliceState::Exploration && !PC->IsModalOpen());
                    TArray<FString> Suffix; for (int32 I = ArrivalTraceCount; I < Host->GetTrace().Num(); ++I) Suffix.Add(Host->GetTrace()[I]);
                    Test->TestEqual(TEXT("Intact source dispatch trace"), FString::Join(Suffix,TEXT("\n")), FString::Join(Strings(Case(TEXT("intact_unheard"))->GetArrayField(TEXT("events"))),TEXT("\n")));
                    Test->TestEqual(TEXT("Intact fallback leaves memory unchanged"), StateJson(Run->GetPlayerMemory()->GetSnapshot()), BeforeMemory);
                    Capture(TEXT("Fallback")); Write(Host, Run, Pawn, TEXT("intact_fallback")); Stage = 4; Frame = -1;
                }
                else
                {
                    Test->TestTrue(TEXT("Actual Interact starts source reaction Field modal"), Host->GetState() == EMemoriaSliceState::Field && PC->IsModalOpen() && PC->GetNarrativeWidget());
                    Test->TestTrue(TEXT("Heard flag before actual first row"), Run->GetRunSnapshot().GetFlag(MemoriaMaletReaction::Heard));
                    Test->TestEqual(TEXT("Holding Interact does not click through first row"), Host->GetView().Body, Asset()->Definition.Rows[0].Text.Text);
                    Test->TestTrue(TEXT("Actual first authored row visible"), PC->GetNarrativeWidget() && PC->GetNarrativeWidget()->VisibleText().Contains(Asset()->Definition.Rows[0].Text.Text));
                    Test->TestTrue(TEXT("Modal owns user focus"), PC->GetNarrativeWidget() && PC->GetNarrativeWidget()->HasUserFocus(PC));
                    const int32 Count = Host->GetTrace().Num();
                    Test->TestFalse(TEXT("Second interaction while Field active is rejected"), Host->InteractWithMalet());
                    Test->TestEqual(TEXT("Active rejection does not mutate trace or state"), Host->GetTrace().Num(), Count);
                    Capture(TEXT("FirstLine")); Stage = 3; Frame = -1;
                }
            }
        }
        else if (Stage == 3)
        {
            if (Frame == 5) Key(EKeys::D, IE_Pressed);
            if (Frame == 12)
            {
                Key(EKeys::D, IE_Released);
                Test->TestTrue(TEXT("Narrative modal blocks movement"), Pawn->GetActorLocation().Equals(BeforePosition,.001));
                Key(EKeys::Escape, IE_Pressed);
            }
            if (Frame == 16) Key(EKeys::Escape, IE_Released);
            if (Frame == 20) Key(EKeys::E, IE_Pressed);
            if (Frame == 24) Key(EKeys::E, IE_Released);
            if (Frame == 30) Test->TestEqual(TEXT("Second original row"), Host->GetView().Body, Asset()->Definition.Rows[1].Text.Text);
            if (Frame == 40) Key(EKeys::Gamepad_FaceButton_Bottom, IE_Pressed);
            if (Frame == 44) Key(EKeys::Gamepad_FaceButton_Bottom, IE_Released);
            if (Frame == 50)
            {
                Test->TestEqual(TEXT("Third original row"), Host->GetView().Body, Asset()->Definition.Rows[2].Text.Text);
                Capture(TEXT("LaterLine"));
            }
            if (Frame == 60) Key(EKeys::E, IE_Pressed);
            if (Frame == 64) Key(EKeys::E, IE_Released);
            if (Frame == 75)
            {
                AssertExploration(PC, Host, World);
                Test->TestEqual(TEXT("Exactly one reaction invocation"), Host->GetMaletReactionCount(), 1);
                Test->TestEqual(TEXT("No arrival or normal Field group started"), Host->GetFieldInvocationCount(), 1);
                Test->TestTrue(TEXT("Heard flag remains after completion"), Run->GetRunSnapshot().GetFlag(MemoriaMaletReaction::Heard));
                Test->TestEqual(TEXT("Full memory snapshot unchanged by interaction"), StateJson(Run->GetPlayerMemory()->GetSnapshot()), BeforeMemory);
                Test->TestTrue(TEXT("Same run and domain after reaction"), RunId == Run->GetRunSnapshot().RunId && Domain.Get() == Run->GetPlayerMemory());
                auto Expected = Case(TEXT("canonical_paid"));
                if (!Test->TestNotNull(TEXT("Canonical executable source trace"), Expected.Get())) return true;
                Compare(*Test, Normalize(Host->GetTrace()), Run->GetRunSnapshot(), Run->GetPlayerMemory()->GetSnapshot(), Expected);
                Capture(TEXT("AfterReaction")); Write(Host, Run, Pawn, TEXT("canonical_complete"));
            }
            // Supplemental boundary probe after canonical acceptance/capture:
            // a second ordinary interaction records the source request only.
            if (Frame == 90) Key(EKeys::E, IE_Pressed);
            if (Frame == 94) Key(EKeys::E, IE_Released);
            if (Frame == 102)
            {
                Test->TestEqual(TEXT("Already-heard next request recorded exactly"), Host->GetDeferredInteraction(), FString(TEXT("malet_encounter")));
                Test->TestEqual(TEXT("Second press cannot replay reaction or normal chain"), Host->GetFieldInvocationCount(), 1);
                AssertExploration(PC, Host, World);
                Write(Host, Run, Pawn, TEXT("second_press_boundary")); Stage = 4; Frame = -1;
            }
        }
        else if (Stage == 4)
        {
            if (Frame == 5)
            {
                BeforePosition = Pawn->GetActorLocation(); CameraOrigin = Pawn->GetFieldCamera()->GetComponentLocation();
                Key(EKeys::W, IE_Pressed);
            }
            if (Frame == 18)
            {
                Key(EKeys::W, IE_Released);
                Test->TestTrue(TEXT("Actual movement restored after reaction/fallback"), Pawn->GetActorLocation().Y > BeforePosition.Y + 5);
                Test->TestTrue(TEXT("Z stable after restored movement"), FMath::IsNearlyZero(Pawn->GetActorLocation().Z,.001));
                Test->TestTrue(TEXT("Accepted camera follows pawn displacement"), (Pawn->GetFieldCamera()->GetComponentLocation()-CameraOrigin).Equals(Pawn->GetActorLocation()-BeforePosition,.001));
            }
            if (Frame == 25) { Capture(TEXT("Moved")); }
            if (Frame == 32) return true;
        }
        ++Frame; return false;
    }
private:
    FAutomationTestBase* Test; bool bPaid, bStarted=false, bWasFixed=false;
    double Started, OldDelta=0; uint64 LastFrame=MAX_uint64; int32 Frame=0, Stage=0, ArrivalTraceCount=0;
    FGuid RunId; TWeakObjectPtr<UMemoriaPlayerMemoryDomain> Domain; TWeakObjectPtr<UWorld> OriginalWorld;
    TWeakObjectPtr<AMemoriaMaletActor> Malet; FKey Held; FString BeforeMemory;
    FMemoriaRunSnapshot BeforeRun; FVector BeforePosition, CameraOrigin;
    FString Mode() const { return bPaid?TEXT("CanonicalInteraction"):TEXT("IntactInteraction"); }
    FString Output() const { return FPaths::ProjectSavedDir()/TEXT("Validation/Phase1F"); }
    void AssertExploration(AMemoriaSliceController* PC, UMemoriaNarrativeSubsystem* Host, UWorld* World)
    {
        Test->TestTrue(TEXT("Exploration restored, modal and choices removed"), Host->GetState()==EMemoriaSliceState::Exploration && !PC->IsModalOpen() && !PC->GetNarrativeWidget() && Host->GetView().Choices.IsEmpty());
        Test->TestFalse(TEXT("Movement no longer ignored"), PC->IsMoveInputIgnored());
        auto* Input=PC->GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
        auto* Modal=LoadObject<UInputMappingContext>(nullptr,TEXT("/Game/Tests/Foundation/IMC_Modal.IMC_Modal"));
        auto* Explore=LoadObject<UInputMappingContext>(nullptr,TEXT("/Game/Tests/Foundation/IMC_Foundation.IMC_Foundation"));
        Test->TestTrue(TEXT("Modal context removed, exploration context remains"), !Input->HasMappingContext(Modal) && Input->HasMappingContext(Explore));
        Test->TestTrue(TEXT("Viewport focus restored"), FSlateApplication::Get().GetUserFocusedWidget(0)==World->GetGameViewport()->GetGameViewportWidget());
    }
    void Write(UMemoriaNarrativeSubsystem* Host, UMemoriaRunSubsystem* Run, AMemoriaFieldPawn* Pawn, const FString& Label)
    {
        Obj O=MakeShared<FJsonObject>(); TArray<Val> Events;
        for (const auto& E:Host->GetTrace()) Events.Add(MakeShared<FJsonValueString>(E));
        O->SetArrayField(TEXT("trace"),Events); O->SetObjectField(TEXT("run"),FJsonObjectConverter::UStructToJsonObject(Run->GetRunSnapshot()));
        O->SetObjectField(TEXT("memory"),FJsonObjectConverter::UStructToJsonObject(Run->GetPlayerMemory()->GetSnapshot()));
        O->SetStringField(TEXT("state"),Label); O->SetStringField(TEXT("pawn_position"),Pawn->GetActorLocation().ToString());
        O->SetNumberField(TEXT("field_invocations"),Host->GetFieldInvocationCount()); O->SetNumberField(TEXT("reaction_invocations"),Host->GetMaletReactionCount());
        O->SetStringField(TEXT("deferred_target"),Host->GetDeferredInteraction()); IFileManager::Get().MakeDirectory(*Output(),true);
        FFileHelper::SaveStringToFile(Canon(O)+TEXT("\n"),*(Output()/(Mode()+TEXT("_")+Label+TEXT(".json"))),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    }
};
}
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FMaletRuntime, "Memoria.Malet", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
void FMaletRuntime::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{ for (const TCHAR* Name:{TEXT("CanonicalInteraction"),TEXT("IntactInteraction")}) { Names.Add(Name); Commands.Add(Name); } }
bool FMaletRuntime::RunTest(const FString& Parameters)
{
    if (!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice"))) return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FMaletReplay(this,Parameters==TEXT("CanonicalInteraction"))));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand()); return true;
}
#endif
