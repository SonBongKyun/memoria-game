#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "Tests/AutomationCommon.h"
#include "Editor.h"
#include "Kismet/GameplayStatics.h"
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
Obj DealCase(const FString& Id, const FString& Label)
{
    auto Cases = Json(Base()/TEXT("fixtures/malet_deal/contract_expected.v1.json"));
    if (Cases) for (auto C : Cases->AsArray()) if (C->AsObject()->GetStringField(TEXT("id")) == Id)
        for (auto S : C->AsObject()->GetArrayField(TEXT("states")))
            if (S->AsObject()->GetStringField(TEXT("label")) == Label) return S->AsObject();
    return nullptr;
}
void CompareDealMemory(FAutomationTestBase& Test, const FMemoriaMemorySnapshot& Memory, const Obj& Expected)
{
    Test.TestTrue(TEXT("Source ordered history, including failed payment"), Memory.BurnedHistory == Strings(Expected->GetArrayField(TEXT("burned"))));
    const auto& Owned = Expected->GetArrayField(TEXT("memory_state"));
    if (!Test.TestEqual(TEXT("All source-owned memories retained"), Memory.Owned.Num(), Owned.Num())) return;
    for (int32 I=0; I<Owned.Num(); ++I)
    {
        const auto E=Owned[I]->AsObject(); const auto& A=Memory.Owned[I];
        Test.TestEqual(TEXT("Owned source order"), A.Id, E->GetStringField(TEXT("id")));
        Test.TestEqual(*A.Id, A.bBurned, E->GetBoolField(TEXT("burned")));
        Test.TestEqual(TEXT("Source residue preserved"), A.bResidue, E->GetBoolField(TEXT("residue")));
        Test.TestEqual(TEXT("Source faded state"), A.bFaded, E->GetBoolField(TEXT("faded")));
        Test.TestEqual(TEXT("Source erosion state"), A.Erosion, int64(E->GetNumberField(TEXT("erosion"))));
    }
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
    FMaletReplay(FAutomationTestBase* InTest, bool Paid, FString InMode = FString()) : Test(InTest), bPaid(Paid), Started(FPlatformTime::Seconds()), RefusalMode(InMode) {}
    ~FMaletReplay() override { if (bStarted) { FApp::SetUseFixedTimeStep(bWasFixed); FApp::SetFixedDeltaTime(OldDelta); } }
    bool Update() override
    {
        if (LastFrame == GFrameCounter) return false; LastFrame = GFrameCounter;
        if (FPlatformTime::Seconds() - Started > 120) { Test->AddError(TEXT("Malet actual input replay timed out")); return true; }
        UWorld* World = GEditor->PlayWorld;
        if (Stage == 12)
        {
            // Real level travel destroys the owner world. No simulated cleanup broadcast.
            if (!World || World == CancelledWorld.Get() || !World->GetGameInstance()) return false;
            auto* Run = World->GetGameInstance()->GetSubsystem<UMemoriaRunSubsystem>();
            auto* Host = World->GetGameInstance()->GetSubsystem<UMemoriaNarrativeSubsystem>();
            if (++Frame < 45) return false;
            Test->TestTrue(TEXT("Actual replacement world loaded"), World->GetMapName().EndsWith(TEXT("L_FoundationTest")));
            Test->TestFalse(TEXT("World teardown cancels 300 ms timer"), Host->IsMaletDelayPending());
            Test->TestFalse(TEXT("World teardown cancels 500 ms timer"), Host->IsMaletRewardDelayPending());
            Test->TestEqual(TEXT("No stale continuation after owner teardown"), FString::Join(Host->GetTrace(),TEXT("\n")), CancellationTrace);
            Test->TestEqual(TEXT("Teardown preserves run"), StateJson(Run->GetRunSnapshot()), CancellationRun);
            Test->TestEqual(TEXT("Teardown preserves memory"), StateJson(Run->GetPlayerMemory()->GetSnapshot()), CancellationMemory);
            Write(Host,Run,nullptr,TEXT("world_callback_cancelled"));
            return true;
        }
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
                    Test->TestEqual(TEXT("Newly authorized normal group starts once"), Host->GetFieldInvocationCount(), 1);
                    Test->TestFalse(TEXT("Intact fallback does not consume heard flag"), Run->GetRunSnapshot().GetFlag(MemoriaMaletReaction::Heard));
                    Test->TestTrue(TEXT("Normal route has actual Field modal"), Host->GetState() == EMemoriaSliceState::Field && PC->IsModalOpen());
                    Test->TestTrue(TEXT("Normal request retains source resolver priority"), Host->GetTrace().Contains(TEXT("resolver:begin:burned=false:heard=false")) && Host->GetTrace().Contains(TEXT("request:res://data/chapter2_dialogue.json::malet_encounter")));
                    Test->TestEqual(TEXT("Normal first row remains original zero"), Host->GetView().Body, NormalAsset()->Definition.Rows[0].Text.Text);
                    Test->TestEqual(TEXT("Intact fallback leaves memory unchanged"), StateJson(Run->GetPlayerMemory()->GetSnapshot()), BeforeMemory);
                    Capture(TEXT("Fallback")); Write(Host, Run, Pawn, TEXT("intact_normal_boundary")); Stage = 9; Frame = -1;
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
            if (Frame == 90) { Stage = 4; Frame = -1; }
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
            if (Frame == 32) { Key(EKeys::S, IE_Pressed); }
            if (Frame == 45) { Key(EKeys::S, IE_Released); }
            if (Frame == 65)
            {
                Test->TestTrue(TEXT("Ordinary next E uses actual range resolver"), PC->GetInteraction()->GetTarget() == Malet.Get());
                if (RefusalMode == TEXT("AlreadyBurnedPayment"))
                    Test->TestTrue(TEXT("Explicit edge fixture burns sword before ordinary interaction"), Run->BurnMemory(TEXT("identity_first_sword")) == EMemoriaMemoryResult::Success);
                NormalTraceStart = Host->GetTrace().Num(); NormalBeforeRun = StateJson(Run->GetRunSnapshot());
                Key(EKeys::E, IE_Pressed);
            }
            if (Frame == 69) Key(EKeys::E, IE_Released);
            if (Frame == 76)
            {
                Test->TestEqual(TEXT("Already heard starts normal once"), Host->GetFieldInvocationCount(), 2);
                Test->TestEqual(TEXT("No repeated reaction"), Host->GetMaletReactionCount(), 1);
                Test->TestEqual(TEXT("Original normal first row"), Host->GetView().Body, NormalAsset()->Definition.Rows[0].Text.Text);
                Test->TestTrue(TEXT("Normal modal visible"), PC->IsModalOpen() && PC->GetNarrativeWidget()->VisibleText().Contains(NormalAsset()->Definition.Rows[0].Text.Text));
                Capture(TEXT("NormalEncounter_FirstLine")); Write(Host, Run, Pawn, TEXT("normal_first"));
                Stage = RefusalMode.IsEmpty() ? 9 : 5; Frame = -1;
            }
        }
        else if (Stage == 5)
        {
            if (Frame >= 10 && Frame <= 90 && Frame % 10 == 0) Key(EKeys::E, IE_Pressed);
            if (Frame >= 14 && Frame <= 94 && Frame % 10 == 4) Key(EKeys::E, IE_Released);
            if (Frame == 100)
            {
                const auto View = Host->GetView();
                Test->TestTrue(TEXT("Both original choices preserved"), View.Choices.Num()==2 && View.Choices[0].OriginalIndex==0 && View.Choices[1].OriginalIndex==1);
                Test->TestEqual(TEXT("Accept label unchanged"), View.Choices[0].Text, FString(TEXT("Accept the deal.")));
                Test->TestEqual(TEXT("Refuse label unchanged"), View.Choices[1].Text, FString(TEXT("Refuse.")));
                Capture(TEXT("NormalEncounter_Choices"));
                if (IsDealMode())
                {
                    Test->TestEqual(TEXT("Canonical sword intact before actual choice"), Run->GetPlayerMemory()->IsIntact(TEXT("identity_first_sword")), RefusalMode!=TEXT("AlreadyBurnedPayment"));
                    Write(Host,Run,Pawn,TEXT("before_accept"));
                }
                BeforeAcceptRun=StateJson(Run->GetRunSnapshot()); BeforeAcceptMemory=StateJson(Run->GetPlayerMemory()->GetSnapshot()); BeforeAcceptTrace=Host->GetTrace().Num();
            }
            if (Frame == 110)
            {
                if (!IsDealMode()) Key(EKeys::Down, IE_Pressed);
            }
            if (Frame == 114)
            {
                Key(EKeys::Down, IE_Released); Key(EKeys::Gamepad_FaceButton_Bottom, IE_Released);
            }
            if (Frame == 120)
            {
                if (IsDealMode())
                {
                    Test->TestEqual(TEXT("Highlight before confirm cannot pay"),StateJson(Run->GetRunSnapshot()),BeforeAcceptRun);
                    Test->TestEqual(TEXT("Highlight preserves memory"),StateJson(Run->GetPlayerMemory()->GetSnapshot()),BeforeAcceptMemory);
                    Test->TestEqual(TEXT("Highlight emits no choice effects"),Host->GetTrace().Num(),BeforeAcceptTrace);
                    Capture(TEXT("Accept_Selected"));
                }
                else Capture(TEXT("NormalEncounter_RefusalSelected"));
            }
            if (Frame == 130) Key(EKeys::E, IE_Pressed);
            if (Frame == 134) Key(EKeys::E, IE_Released);
            if (Frame == 138)
            {
                Test->TestTrue(TEXT("Actual callback waiting during source exploration gap"), Host->IsMaletDelayPending() && Host->GetState()==EMemoriaSliceState::Exploration && !PC->IsModalOpen());
                Test->TestTrue(TEXT("Selected and first-talk flags set before timer"), Run->GetRunSnapshot().GetFlag(IsDealMode()?TEXT("malet_deal_accepted"):TEXT("malet_deal_refused")) && Run->GetRunSnapshot().GetFlag(TEXT("talked_Malet_malet_encounter")));
                if (IsDealMode())
                {
                    CompareDeal(Host,Run,TEXT("after_accept"));
                    const auto Events=Normalize(Host->GetTrace());
                    const int32 Flag=Events.IndexOfByKey(TEXT("flag:malet_deal_accepted"));
                    const int32 Burn=Events.IndexOfByKey(RefusalMode==TEXT("AlreadyBurnedPayment")?TEXT("burn:identity_first_sword:fail"):TEXT("burn:identity_first_sword:ok"));
                    Test->TestTrue(TEXT("Accepted flag precedes actual burn attempt"), Flag>=0 && Burn==Flag+1);
                    Test->TestTrue(TEXT("Payment uses same run and domain"), RunId==Run->GetRunSnapshot().RunId && Domain.Get()==Run->GetPlayerMemory());
                    Write(Host,Run,Pawn,TEXT("after_accept"));
                    if (RefusalMode==TEXT("CancelNormalOnRunReplace") || RefusalMode==TEXT("CancelNormalOnWorldTeardown"))
                        Cancel(World,Run,Host);
                }
                Test->TestFalse(TEXT("Normal callback disconnected while pending"), Host->IsMaletCallbackConnected());
                Write(Host,Run,Pawn,TEXT("delay_pending"));
                if (RefusalMode==TEXT("CallbackCancellation"))
                {
                    Run->BeginStartingMemoryRun(); CancellationRun=StateJson(Run->GetRunSnapshot()); CancellationMemory=StateJson(Run->GetPlayerMemory()->GetSnapshot());
                    Test->TestFalse(TEXT("New run cancels pending callback"), Host->IsMaletDelayPending());
                    Stage=8; Frame=-1;
                }
            }
            if (Frame == 160)
            {
                Test->TestTrue(TEXT("Real 0.3 second world timer"), Host->GetMaletDelaySeconds()>=.299 && Host->GetMaletDelaySeconds()<.35);
                if (IsDealMode())
                {
                    Test->TestEqual(TEXT("Original deal first line"), Host->GetView().Body, DealAsset()->Definition.Rows[0].Text.Text);
                    Test->TestTrue(TEXT("Actual deal modal"), PC->IsModalOpen() && PC->GetNarrativeWidget()->VisibleText().Contains(DealAsset()->Definition.Rows[0].Text.Text));
                    CompareDeal(Host,Run,TEXT("deal_first"));
                    Capture(TEXT("Deal_FirstLine")); Write(Host,Run,Pawn,TEXT("deal_first"));
                    Stage=10; Frame=-1;
                }
                else
                {
                    Test->TestEqual(TEXT("Original refused first line"), Host->GetView().Body, RefusalAsset()->Definition.Rows[0].Text.Text);
                    Test->TestTrue(TEXT("Actual refused modal"), PC->IsModalOpen() && PC->GetNarrativeWidget()->VisibleText().Contains(RefusalAsset()->Definition.Rows[0].Text.Text));
                    Capture(TEXT("Refused_FirstLine")); Write(Host,Run,Pawn,TEXT("refused_first"));
                    Stage=6; Frame=-1;
                }
            }
        }
        else if (Stage == 6)
        {
            if (Frame == 10 || Frame == 30 || Frame == 60) Key(EKeys::E,IE_Pressed);
            if (Frame == 14 || Frame == 34 || Frame == 64) Key(EKeys::E,IE_Released);
            if (Frame == 20) Test->TestEqual(TEXT("Refused original row one"),Host->GetView().Body,RefusalAsset()->Definition.Rows[1].Text.Text);
            if (Frame == 40)
            {
                Test->TestEqual(TEXT("Refused original row two"),Host->GetView().Body,RefusalAsset()->Definition.Rows[2].Text.Text);
                Capture(TEXT("Refused_LastLine"));
            }
            if (Frame == 75)
            {
                AssertExploration(PC,Host,World);
                Test->TestFalse(TEXT("Refusal flag erased, not false entry"),Run->GetRunSnapshot().HasFlag(TEXT("malet_deal_refused")));
                Test->TestFalse(TEXT("Talked flag erased, not false entry"),Run->GetRunSnapshot().HasFlag(TEXT("talked_Malet_malet_encounter")));
                Test->TestTrue(TEXT("NPC cache cleared and normal callback reconnected"),!Host->IsMaletTalkCached() && Host->IsMaletCallbackConnected());
                Test->TestEqual(TEXT("Run restored exactly to pre-normal state"),StateJson(Run->GetRunSnapshot()),NormalBeforeRun);
                Test->TestEqual(TEXT("No hidden memory/reward mutation"),StateJson(Run->GetPlayerMemory()->GetSnapshot()),BeforeMemory);
                CompareRefusal(Host,TEXT("cleanup"));
                Capture(TEXT("Exploration_AfterRefusal")); Write(Host,Run,Pawn,TEXT("refusal_cleanup"));
                BeforePosition=Pawn->GetActorLocation(); CameraOrigin=Pawn->GetFieldCamera()->GetComponentLocation();
            }
            if (Frame == 85) Key(EKeys::W,IE_Pressed);
            if (Frame == 98)
            {
                Key(EKeys::W,IE_Released);
                Test->TestTrue(TEXT("Actual movement after refusal"),Pawn->GetActorLocation().Y>BeforePosition.Y+5);
                Test->TestTrue(TEXT("Camera restored after refusal"),(Pawn->GetFieldCamera()->GetComponentLocation()-CameraOrigin).Equals(Pawn->GetActorLocation()-BeforePosition,.001));
            }
            if (Frame == 110) Key(EKeys::S,IE_Pressed);
            if (Frame == 123) Key(EKeys::S,IE_Released);
            if (Frame == 145) Key(EKeys::E,IE_Pressed);
            if (Frame == 149) Key(EKeys::E,IE_Released);
            if (Frame == 158)
            {
                Test->TestEqual(TEXT("Retry is ordinary normal request at row zero"),Host->GetView().Body,NormalAsset()->Definition.Rows[0].Text.Text);
                Test->TestEqual(TEXT("Reaction, normal, refused, retry only"),Host->GetFieldInvocationCount(),4);
                Test->TestEqual(TEXT("Retry cannot replay memory reaction"),Host->GetMaletReactionCount(),1);
                CompareRefusal(Host,TEXT("retry"));
                Capture(TEXT("RetryBoundary")); Write(Host,Run,Pawn,TEXT("retry_boundary"));
                Stage=9; Frame=-1;
            }
        }
        else if (Stage == 10)
        {
            if (Frame==10 || Frame==30 || Frame==50 || Frame==70 || Frame==90) Key(EKeys::E,IE_Pressed);
            if (Frame==14 || Frame==34 || Frame==54 || Frame==74 || Frame==94) Key(EKeys::E,IE_Released);
            if (Frame==20 || Frame==40 || Frame==60 || Frame==80)
            {
                const int32 Row=Frame/20;
                Test->TestEqual(TEXT("Each original deal row executes in order"), Host->GetView().Body, DealAsset()->Definition.Rows[Row].Text.Text);
                CompareDeal(Host,Run,FString::Printf(TEXT("deal_row_%d"),Row));
                if (Frame==40) Capture(TEXT("Deal_MiddleLine"));
                if (Frame==80) Capture(TEXT("Deal_LastLine"));
            }
            if (Frame==98)
            {
                AssertExploration(PC,Host,World);
                Test->TestTrue(TEXT("Separate 500 ms continuation is pending"), Host->IsMaletRewardDelayPending() && !Host->IsMaletDelayPending());
                Test->TestTrue(TEXT("Reward absent during second delay"), Host->GetDeferredInteraction().IsEmpty());
                CompareDeal(Host,Run,TEXT("deal_end")); Write(Host,Run,Pawn,TEXT("deal_end"));
                if (RefusalMode==TEXT("CancelRewardOnRunReplace") || RefusalMode==TEXT("CancelRewardOnWorldTeardown"))
                    Cancel(World,Run,Host);
            }
            if (Frame==135)
            {
                Test->TestTrue(TEXT("Real separate 0.5 second world timer"), Host->GetMaletRewardDelaySeconds()>=.499 && Host->GetMaletRewardDelaySeconds()<.55);
                Test->TestFalse(TEXT("Reward timer has completed once"), Host->IsMaletRewardDelayPending());
                Test->TestEqual(TEXT("Hard pre-effect reward boundary"), Host->GetDeferredInteraction(), FString(TEXT("malet_reward")));
                Test->TestEqual(TEXT("Reaction, normal and deal only; reward not started"), Host->GetFieldInvocationCount(),3);
                Test->TestEqual(TEXT("Reaction remains single"), Host->GetMaletReactionCount(),1);
                AssertExploration(PC,Host,World); CompareDeal(Host,Run,TEXT("reward_boundary"));
                auto Preserved=Run->GetRunSnapshot();
                Preserved.StoryFlags.RemoveAll([](const auto& F){return F.Id==TEXT("malet_deal_accepted") || F.Id==TEXT("talked_Malet_malet_encounter");});
                Test->TestEqual(TEXT("Entire run outside two source flags unchanged"),StateJson(Preserved),NormalBeforeRun);
                Test->TestTrue(TEXT("No run or domain replacement"),RunId==Run->GetRunSnapshot().RunId && Domain.Get()==Run->GetPlayerMemory());
                Test->TestTrue(TEXT("No downstream Field start"), !Host->GetTrace().Contains(TEXT("field:start:malet_reward")) && !Run->GetRunSnapshot().HasFlag(TEXT("ch2_malet_done")));
                Capture(TEXT("RewardRequest_Deferred")); Write(Host,Run,Pawn,TEXT("reward_boundary"));
                Stage=9; Frame=-1;
            }
        }
        else if (Stage == 8 && Frame == 40)
        {
            Test->TestTrue(TEXT("Cancelled callback cannot start stale dialogue"),Host->GetState()==EMemoriaSliceState::Idle && Host->GetTrace().IsEmpty());
            Test->TestEqual(TEXT("Cancelled callback cannot mutate replacement run"),StateJson(Run->GetRunSnapshot()),CancellationRun);
            Test->TestEqual(TEXT("Cancelled callback cannot mutate replacement memory"),StateJson(Run->GetPlayerMemory()->GetSnapshot()),CancellationMemory);
            Test->TestTrue(TEXT("Both owned timer handles cancelled"),!Host->IsMaletDelayPending() && !Host->IsMaletRewardDelayPending());
            Write(Host,Run,Pawn,TEXT("cancelled_callback")); return true;
        }
        else if (Stage == 9 && Frame == 10) return true;

        ++Frame; return false;
    }
private:
    FAutomationTestBase* Test; bool bPaid, bStarted=false, bWasFixed=false;
    double Started, OldDelta=0; uint64 LastFrame=MAX_uint64; int32 Frame=0, Stage=0, ArrivalTraceCount=0;
    FGuid RunId; TWeakObjectPtr<UMemoriaPlayerMemoryDomain> Domain; TWeakObjectPtr<UWorld> OriginalWorld;
    TWeakObjectPtr<AMemoriaMaletActor> Malet; FKey Held; FString BeforeMemory;
    FMemoriaRunSnapshot BeforeRun; FVector BeforePosition, CameraOrigin;
    FString RefusalMode, NormalBeforeRun, BeforeAcceptRun, BeforeAcceptMemory, CancellationRun, CancellationMemory, CancellationTrace;
    TWeakObjectPtr<UWorld> CancelledWorld;
    bool IsDealMode() const
    {
        return RefusalMode==TEXT("AcceptPreEffectDeferred") || RefusalMode==TEXT("CanonicalPayment") ||
            RefusalMode==TEXT("AlreadyBurnedPayment") || RefusalMode.StartsWith(TEXT("CancelNormal")) || RefusalMode.StartsWith(TEXT("CancelReward"));
    }
    UMemoriaFieldAsset* DealAsset() const { return LoadObject<UMemoriaFieldAsset>(nullptr,*MemoriaNarrativeImport::ObjectPath(false,TEXT("malet_deal"))); }
    void CompareDeal(UMemoriaNarrativeSubsystem* Host, UMemoriaRunSubsystem* Run, const FString& Label)
    {
        auto Expected=DealCase(RefusalMode==TEXT("AlreadyBurnedPayment")?TEXT("already_burned"):TEXT("accept_intact"),Label);
        if(!Test->TestTrue(TEXT("Executable Phase 1H oracle state"),Expected.IsValid())) return;
        TArray<FString> Actual; for(int32 I=NormalTraceStart;I<Host->GetTrace().Num();++I) Actual.Add(Host->GetTrace()[I]);
        Test->TestEqual(TEXT("Exact source Accept/deal/callback trace"),FString::Join(Normalize(Actual),TEXT("\n")),FString::Join(Strings(Expected->GetArrayField(TEXT("events"))),TEXT("\n")));
        CompareDealMemory(*Test,Run->GetPlayerMemory()->GetSnapshot(),Expected);
    }
    void Cancel(UWorld* World, UMemoriaRunSubsystem* Run, UMemoriaNarrativeSubsystem* Host)
    {
        if (RefusalMode.EndsWith(TEXT("OnRunReplace")))
        {
            Test->TestTrue(TEXT("Real run replacement"),Run->BeginStartingMemoryRun()==EMemoriaMemoryResult::Success);
            CancellationRun=StateJson(Run->GetRunSnapshot()); CancellationMemory=StateJson(Run->GetPlayerMemory()->GetSnapshot());
            Stage=8; Frame=-1;
        }
        else
        {
            CancellationRun=StateJson(Run->GetRunSnapshot()); CancellationMemory=StateJson(Run->GetPlayerMemory()->GetSnapshot());
            CancellationTrace=FString::Join(Host->GetTrace(),TEXT("\n")); CancelledWorld=World;
            UGameplayStatics::OpenLevel(World,TEXT("/Game/Tests/Foundation/L_FoundationTest")); Stage=12; Frame=-1;
        }
    }
    int32 NormalTraceStart=0, BeforeAcceptTrace=0;
    UMemoriaFieldAsset* NormalAsset() const { return LoadObject<UMemoriaFieldAsset>(nullptr,*MemoriaNarrativeImport::ObjectPath(false,TEXT("malet_encounter"))); }
    UMemoriaFieldAsset* RefusalAsset() const { return LoadObject<UMemoriaFieldAsset>(nullptr,*MemoriaNarrativeImport::ObjectPath(false,TEXT("malet_refused"))); }
    void CompareRefusal(UMemoriaNarrativeSubsystem* Host,const FString& Label)
    {
        auto Cases=Json(Base()/TEXT("fixtures/malet_refusal/contract_expected.v1.json"));
        for (auto C:Cases->AsArray()) if(C->AsObject()->GetStringField(TEXT("id"))==TEXT("refusal_retry"))
            for(auto V:C->AsObject()->GetArrayField(TEXT("states"))) if(V->AsObject()->GetStringField(TEXT("label"))==Label)
            {
                TArray<FString> Actual;
                for(int32 I=NormalTraceStart;I<Host->GetTrace().Num();++I) Actual.Add(Host->GetTrace()[I]);
                Test->TestEqual(TEXT("Exact executable source normal/refusal/retry trace"),FString::Join(Normalize(Actual),TEXT("\n")),FString::Join(Strings(V->AsObject()->GetArrayField(TEXT("events"))),TEXT("\n")));
                return;
            }
        Test->AddError(TEXT("Missing source refusal trace"));
    }
    FString Mode() const { if (!RefusalMode.IsEmpty()) return RefusalMode; return bPaid?TEXT("CanonicalInteraction"):TEXT("IntactInteraction"); }
    FString Output() const { return FPaths::ProjectSavedDir()/TEXT("Validation/Phase1H"); }
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
        O->SetBoolField(TEXT("same_run_id"),RunId==Run->GetRunSnapshot().RunId);
        O->SetBoolField(TEXT("same_memory_domain"),Domain.Get()==Run->GetPlayerMemory());
        O->SetStringField(TEXT("state"),Label); O->SetStringField(TEXT("pawn_position"),Pawn?Pawn->GetActorLocation().ToString():TEXT("owner_world_destroyed"));
        O->SetNumberField(TEXT("actual_delay_microseconds"),FMath::RoundToDouble(Host->GetMaletDelaySeconds()*1000000.0)); O->SetBoolField(TEXT("talk_cache"),Host->IsMaletTalkCached()); O->SetBoolField(TEXT("normal_callback_connected"),Host->IsMaletCallbackConnected());
        O->SetNumberField(TEXT("field_invocations"),Host->GetFieldInvocationCount()); O->SetNumberField(TEXT("reaction_invocations"),Host->GetMaletReactionCount());
        O->SetNumberField(TEXT("actual_reward_delay_microseconds"),FMath::RoundToDouble(Host->GetMaletRewardDelaySeconds()*1000000.0));
        O->SetBoolField(TEXT("normal_delay_pending"),Host->IsMaletDelayPending()); O->SetBoolField(TEXT("reward_delay_pending"),Host->IsMaletRewardDelayPending());
        O->SetStringField(TEXT("deferred_target"),Host->GetDeferredInteraction()); IFileManager::Get().MakeDirectory(*Output(),true);
        const FString Serialized = Canon(O)+TEXT("\n"); Obj Parsed;
        Test->TestTrue(TEXT("Machine-readable evidence JSON parses"),FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Serialized),Parsed));
        Test->TestTrue(TEXT("Machine-readable evidence saved"),FFileHelper::SaveStringToFile(Serialized,*(Output()/(Mode()+TEXT("_")+Label+TEXT(".json"))),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM));
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRefusalImport,"Memoria.MaletRefusal.ImportContract",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRefusalImport::RunTest(const FString&)
{
    for (const TCHAR* Group:{TEXT("malet_encounter"),TEXT("malet_refused")})
    {
        TStrongObjectPtr<UMemoriaFieldAsset> Actual(LoadObject<UMemoriaFieldAsset>(nullptr,*MemoriaNarrativeImport::ObjectPath(false,Group))), Expected(NewObject<UMemoriaFieldAsset>());
        FString Error; const FString Path=Base()/TEXT("ir/narrative")/(FString(Group)+TEXT(".field.v1.json"));
        if(!TestNotNull(TEXT("Actual saved Phase 1G asset"),Actual.Get()) || !TestTrue(TEXT("Strict source attestation"),MemoriaNarrativeImport::ReadIr(Path,*Expected,Error))) { AddError(Error); return false; }
        for(TFieldIterator<FProperty> P(UMemoriaFieldAsset::StaticClass(),EFieldIterationFlags::None);P;++P) TestTrue(*P->GetName(),P->Identical_InContainer(Actual.Get(),Expected.Get()));
        Obj Report; TestTrue(TEXT("Check-only semantic no-op"),MemoriaNarrativeImport::Import(Path,false,true,Report,Error));
        TestTrue(TEXT("No package save"),Report && !Report->GetBoolField(TEXT("saved")));
        for(const TCHAR* Variant:{TEXT("modified"),TEXT("reject_position"),TEXT("reject_choice"),TEXT("reject_downstream")})
        {
            const auto Probe=Base()/TEXT("fixtures/malet_refusal")/(FString(Group)+TEXT(".")+Variant+TEXT(".json"));
            TStrongObjectPtr<UMemoriaFieldAsset> Temporary(NewObject<UMemoriaFieldAsset>());
            const bool Modified=FString(Variant)==TEXT("modified");
            TestEqual(TEXT("Transient strict structural validation"),MemoriaNarrativeImport::ReadIr(Probe,*Temporary,Error,false),Modified);
            if(Modified)
            {
                TestTrue(TEXT("Typed semantic change is detected"),MemoriaNarrativeImport::Fingerprint(*Temporary)!=MemoriaNarrativeImport::Fingerprint(*Actual));
                TestFalse(TEXT("Modified payload cannot enter production"),MemoriaNarrativeImport::ReadIr(Probe,*Temporary,Error,true));
                TestTrue(TEXT("Probe remains transient"),Temporary->GetOutermost()==GetTransientPackage());
            }
        }
    }
    return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRefusalChoiceEffects,"Memoria.MaletRefusal.ChoiceEffects",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRefusalChoiceEffects::RunTest(const FString&)
{
    TStrongObjectPtr<UGameInstance> Game(NewObject<UGameInstance>()); Game->Init();
    auto* Run=Game->GetSubsystem<UMemoriaRunSubsystem>();
    auto* Normal=LoadObject<UMemoriaFieldAsset>(nullptr,*MemoriaNarrativeImport::ObjectPath(false,TEXT("malet_encounter")));
    auto Cases=Json(Base()/TEXT("fixtures/malet_refusal/contract_expected.v1.json"));
    if(!TestNotNull(TEXT("Normal contract"),Normal) || !TestTrue(TEXT("Source cases"),Cases.IsValid())) {Game->Shutdown();return false;}
    for(const TCHAR* Id:{TEXT("refusal_retry"),TEXT("refusal_ko"),TEXT("accept_source")})
    {
        Run->BeginStartingMemoryRun(); Run->BurnMemory(MemoriaMaletReaction::Food);
        Run->SetStoryFlag(MemoriaMaletReaction::Heard,true); Run->SetStoryFlag(TEXT("ch2_arrived"),true);
        auto Snapshot=Run->GetRunSnapshot(); FMemoriaNarrativeContext Context(Snapshot,*Run->GetPlayerMemory());
        Snapshot.CurrentLocale=FString(Id)==TEXT("refusal_ko")?TEXT("ko"):TEXT("en");
        FMemoriaFieldInterpreter Field(Normal->Definition,Context); Field.Start();
        for(int32 I=0;I<9;++I) Field.Advance();
        TestTrue(TEXT("Original choices zero and one"),Field.VisibleOriginalIndices()==TArray<int32>{0,1});
        Field.SelectFilteredChoice(FString(Id)==TEXT("accept_source")?0:1);
        TestFalse(TEXT("Source choice ends normal group"),Field.IsActive());
        for(auto C:Cases->AsArray()) if(C->AsObject()->GetStringField(TEXT("id"))==Id)
        {
            auto State=C->AsObject()->GetArrayField(TEXT("states"))[2]->AsObject();
            auto Events=Strings(State->GetArrayField(TEXT("events")));
            int32 First=Events.IndexOfByKey(TEXT("visit:0")), Last=Events.IndexOfByKey(TEXT("end"));
            TArray<FString> Expected; for(int32 I=First;I<=Last;++I) if(!Events[I].StartsWith(TEXT("select:field:"))) Expected.Add(Events[I]);
            TestEqual(TEXT("Exact interpreter trace through source choice effects"),FString::Join(Context.Events,TEXT("\n")),FString::Join(Expected,TEXT("\n")));
            TestTrue(TEXT("Source original choice burn history"),Run->GetPlayerMemory()->GetSnapshot().BurnedHistory==Strings(State->GetArrayField(TEXT("burned"))));
        }
    }
    Game->Shutdown();return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRefusalRepeatCache,"Memoria.MaletRefusal.RepeatCache",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRefusalRepeatCache::RunTest(const FString&)
{
    TStrongObjectPtr<UGameInstance> Game(NewObject<UGameInstance>());Game->Init();
    auto* Run=Game->GetSubsystem<UMemoriaRunSubsystem>();
    Run->BeginStartingMemoryRun(); Run->BurnMemory(MemoriaMaletReaction::Food);
    TestTrue(TEXT("Unheard reaction wins over transient normal cache"),MemoriaMaletReaction::Resolve(*Run,false,Asset(),true).bReaction);
    TestEqual(TEXT("Heard plus transient cache requests exact authored repeat"),MemoriaMaletReaction::Resolve(*Run,false,Asset(),true).Group,FString(TEXT("malet_memory_world_followup")));
    TestEqual(TEXT("Heard without either talked state requests ordinary group"),MemoriaMaletReaction::Resolve(*Run,false,Asset(),false).Group,FString(TEXT("malet_encounter")));
    Run->SetStoryFlag(TEXT("talked_Malet_malet_encounter"),true);
    TestEqual(TEXT("Persisted talked state requests same repeat"),MemoriaMaletReaction::Resolve(*Run,false,Asset(),false).Group,FString(TEXT("malet_memory_world_followup")));
    Run->RemoveStoryFlag(TEXT("talked_Malet_malet_encounter"));
    TestFalse(TEXT("Erase removes record rather than assigning false"),Run->GetRunSnapshot().HasFlag(TEXT("talked_Malet_malet_encounter")));
    Game->Shutdown();return !HasAnyErrors();
}
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FMaletRefusalRuntime, "Memoria.MaletRefusal", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
void FMaletRefusalRuntime::GetTests(TArray<FString>& Names,TArray<FString>& Commands) const
{ for(const TCHAR* N:{TEXT("CanonicalRefusalRetry"),TEXT("AcceptPreEffectDeferred"),TEXT("CallbackCancellation")}) { Names.Add(N); Commands.Add(N); } }
bool FMaletRefusalRuntime::RunTest(const FString& Parameters)
{
    if(!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice"))) return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FMaletReplay(this,true,Parameters)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand()); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDealImport,"Memoria.MaletDeal.ImportContract",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDealImport::RunTest(const FString&)
{
    const FString Path=Base()/TEXT("ir/narrative/malet_deal.field.v1.json");
    TStrongObjectPtr<UMemoriaFieldAsset> Actual(LoadObject<UMemoriaFieldAsset>(nullptr,*MemoriaNarrativeImport::ObjectPath(false,TEXT("malet_deal")))), Expected(NewObject<UMemoriaFieldAsset>());
    FString Error;
    if(!TestNotNull(TEXT("Saved typed Malet deal package"),Actual.Get()) || !TestTrue(TEXT("Strict source attestation"),MemoriaNarrativeImport::ReadIr(Path,*Expected,Error))) { AddError(Error); return false; }
    TestEqual(TEXT("Exactly five authored rows"),Actual->Definition.Rows.Num(),5);
    for(TFieldIterator<FProperty> P(UMemoriaFieldAsset::StaticClass(),EFieldIterationFlags::None);P;++P)
        TestTrue(*P->GetName(),P->Identical_InContainer(Actual.Get(),Expected.Get()));
    for(int32 I=0;I<Actual->Definition.Rows.Num();++I)
    {
        const auto& Row=Actual->Definition.Rows[I];
        TestEqual(TEXT("Original row index"),Row.OriginalIndex,I);
        TestEqual(TEXT("Original group position"),Row.Provenance.GroupPosition,2);
        TestTrue(TEXT("Both localized text presence bits"),Row.Text.bHasText && Row.Text.bHasTextKo);
        TestTrue(TEXT("No invented deal choices"),Row.Choices.IsEmpty());
    }
    Obj Report; TestTrue(TEXT("Repeated import is semantic no-op"),MemoriaNarrativeImport::Import(Path,false,true,Report,Error));
    TestTrue(TEXT("No unnecessary package save"),Report && Report->GetStringField(TEXT("result"))==TEXT("UNCHANGED") && !Report->GetBoolField(TEXT("saved")));
    for(const TCHAR* Variant:{TEXT("modified"),TEXT("reject_position"),TEXT("reject_count"),TEXT("reject_index"),TEXT("reject_downstream")})
    {
        TStrongObjectPtr<UMemoriaFieldAsset> Probe(NewObject<UMemoriaFieldAsset>());
        const auto File=Base()/TEXT("fixtures/malet_deal")/(FString(Variant)+TEXT(".field.v1.json"));
        const bool Modified=FString(Variant)==TEXT("modified");
        TestEqual(TEXT("Transient strict structural validation"),MemoriaNarrativeImport::ReadIr(File,*Probe,Error,false),Modified);
        if(Modified)
        {
            TestTrue(TEXT("Typed semantic change detected"),MemoriaNarrativeImport::Fingerprint(*Probe)!=MemoriaNarrativeImport::Fingerprint(*Actual));
            TestFalse(TEXT("Authored-source mismatch cannot be promoted"),MemoriaNarrativeImport::ReadIr(File,*Probe,Error,true));
            TestTrue(TEXT("Modified probe remains transient"),Probe->GetOutermost()==GetTransientPackage());
        }
    }
    return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDealEdges,"Memoria.MaletDeal.PaymentEdgeCases",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDealEdges::RunTest(const FString&)
{
    TStrongObjectPtr<UGameInstance> Game(NewObject<UGameInstance>()); Game->Init();
    auto* Run=Game->GetSubsystem<UMemoriaRunSubsystem>();
    auto* Normal=LoadObject<UMemoriaFieldAsset>(nullptr,*MemoriaNarrativeImport::ObjectPath(false,TEXT("malet_encounter")));
    auto* Deal=LoadObject<UMemoriaFieldAsset>(nullptr,*MemoriaNarrativeImport::ObjectPath(false,TEXT("malet_deal")));
    if(!TestNotNull(TEXT("Normal typed contract"),Normal) || !TestNotNull(TEXT("Deal typed contract"),Deal)) { Game->Shutdown();return false; }
    for(const TCHAR* CaseId:{TEXT("accept_intact"),TEXT("accept_ko"),TEXT("already_burned"),TEXT("unrelated_state"),TEXT("faded_sword")})
    {
        const FString Id(CaseId);
        auto Before=DealCase(Id,TEXT("before_accept")),After=DealCase(Id,TEXT("after_accept"));
        if(!TestTrue(TEXT("Executable source before/after"),Before.IsValid() && After.IsValid())) continue;
        Run->BeginStartingMemoryRun();Run->BurnMemory(MemoriaMaletReaction::Food);
        if(Id==TEXT("already_burned")) Run->BurnMemory(TEXT("identity_first_sword"));
        auto State=Run->GetRunSnapshot();auto Memory=Run->GetPlayerMemory()->GetSnapshot();
        State.CurrentLocale=Id==TEXT("accept_ko")?TEXT("ko"):TEXT("en");
        State.Player.Hp=int64(Before->GetNumberField(TEXT("hp")));State.Player.Grains=int64(Before->GetNumberField(TEXT("grains")));
        for(auto& M:Memory.Owned)
        {
            if(Id==TEXT("faded_sword") && M.Id==TEXT("identity_first_sword")) M.bFaded=true;
            if(Id==TEXT("unrelated_state") && M.Id==TEXT("sense_forest_smell")) M.Erosion=2;
        }
        TestTrue(TEXT("Validated isolated edge setup"),Run->RestoreRun(State,Run->GetPlayerMemory()->GetDefinitions(),Memory)==EMemoriaMemoryResult::Success);
        for(const auto& Flag:Before->GetObjectField(TEXT("flags"))->Values) Run->SetStoryFlag(FString(Flag.Key.ToView()),Flag.Value->AsBool());
        State=Run->GetRunSnapshot(); const auto UnrelatedBefore=State;
        auto* Domain=Run->GetPlayerMemory(); CompareDealMemory(*this,Domain->GetSnapshot(),Before);
        FMemoriaNarrativeContext Context(State,*Domain);FMemoriaFieldInterpreter Field(Normal->Definition,Context);
        Field.Start();for(int32 I=0;I<9;++I) Field.Advance();
        TestTrue(TEXT("Original choice identities retained"),Field.VisibleOriginalIndices()==TArray<int32>{0,1});
        Field.SelectFilteredChoice(0);
        TestFalse(TEXT("Failed payment also completes source normal group"),Field.IsActive());
        auto Events=Strings(After->GetArrayField(TEXT("events")));TArray<FString> Expected;
        const int32 First=Events.IndexOfByKey(TEXT("visit:0")),Last=Events.IndexOfByKey(TEXT("end"));
        for(int32 I=First;I<=Last;++I) if(!Events[I].StartsWith(TEXT("select:field:"))) Expected.Add(Events[I]);
        TestEqual(TEXT("Exact source choice, accepted flag and burn result order"),FString::Join(Context.Events,TEXT("\n")),FString::Join(Expected,TEXT("\n")));
        TestTrue(TEXT("Actual accepted flag applied"),State.GetFlag(TEXT("malet_deal_accepted")));
        CompareDealMemory(*this,Domain->GetSnapshot(),After);
        auto Preserved=State;Preserved.StoryFlags.RemoveAll([](const auto& F){return F.Id==TEXT("malet_deal_accepted");});
        TestEqual(TEXT("Unrelated run fields preserved"),StateJson(Preserved),StateJson(UnrelatedBefore));
        TestTrue(TEXT("Domain unchanged during actual interpreter payment"),Domain==Run->GetPlayerMemory());
        Context.Events.Reset(); FMemoriaFieldInterpreter DealField(Deal->Definition,Context);DealField.Start();
        for(int32 I=0;I<5;++I) {TestEqual(TEXT("Deal original index"),DealField.OriginalIndex(),I);DealField.Advance();}
        TestFalse(TEXT("Deal completes on fifth row"),DealField.IsActive());
        CompareDealMemory(*this,Domain->GetSnapshot(),DealCase(Id,TEXT("reward_boundary")));
        const auto Golden=Strings(DealCase(Id,TEXT("reward_boundary"))->GetArrayField(TEXT("events")));
        const int32 Begin=Golden.IndexOfByKey(TEXT("field:start:malet_deal"))+1;
        TArray<FString> DealEvents;
        for(int32 I=Begin;I<Golden.Num();++I) {DealEvents.Add(Golden[I]);if(Golden[I]==TEXT("end"))break;}
        TestEqual(TEXT("English or Korean exact authored deal execution"),FString::Join(Context.Events,TEXT("\n")),FString::Join(DealEvents,TEXT("\n")));
    }
    Game->Shutdown();return !HasAnyErrors();
}
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FMaletDealRuntime,"Memoria.MaletDeal",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
void FMaletDealRuntime::GetTests(TArray<FString>& Names,TArray<FString>& Commands) const
{
    for(const TCHAR* N:{TEXT("CanonicalPayment"),TEXT("AlreadyBurnedPayment"),TEXT("CancelNormalOnRunReplace"),TEXT("CancelRewardOnRunReplace"),TEXT("CancelNormalOnWorldTeardown"),TEXT("CancelRewardOnWorldTeardown")}) {Names.Add(N);Commands.Add(N);}
}
bool FMaletDealRuntime::RunTest(const FString& Parameters)
{
    if(!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice")))return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FMaletReplay(this,true,Parameters)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());return true;
}
#endif
