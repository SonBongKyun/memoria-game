#include "MemoriaShopEvidence.h"
#include "Save/MemoriaCheckpointSubsystem.h"
#include "Presentation/MemoriaShopWidget.h"
#include "Presentation/MemoriaArchiveWidget.h"
#include "Presentation/MemoriaBattleEntryWidget.h"
#include "Presentation/MemoriaBattleEntryArt.h"
#include "Battle/MemoriaBattleEntrySubsystem.h"
#include "Framework/MemoriaCoordinates.h"
#include "MemoriaPotionEvidence.h"
#include "MemoriaPlayerObservation.h"
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
#include "Save/MemoriaRunSaveGame.h"
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
#include "Input/Events.h"
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
Obj RewardCase(const FString& Id, const FString& Label)
{
    auto Cases = Json(Base()/TEXT("fixtures/malet_reward/contract_expected.v1.json"));
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
    FString LastArtworkHeader;
    int32 ArtworkStableFrames=0, ArtworkCaptureIndex=0;
    FMaletReplay(FAutomationTestBase* InTest, bool Paid, FString InMode = FString()) : Test(InTest), bPaid(Paid), Started(FPlatformTime::Seconds()), RefusalMode(InMode) {}
    ~FMaletReplay() override { if(ObservedRun.IsValid())ObservedRun->OnInventoryChanged.Remove(InventoryObserverHandle); if(ObservedHost.IsValid()) ObservedHost->OnRewardBoundaryObserved.Remove(BoundaryObserverHandle); if (bStarted) { FApp::SetUseFixedTimeStep(bWasFixed); FApp::SetFixedDeltaTime(OldDelta); } }
    bool Update() override
    {
        if (LastFrame == GFrameCounter) return false; LastFrame = GFrameCounter;
        if (FPlatformTime::Seconds() - Started > 180) { Test->AddError(TEXT("Malet actual input replay timed out")); return true; }
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
            Test->TestFalse(TEXT("Reward callback ownership cancelled on teardown"),Host->IsRewardCallbackPending());
            if(RefusalMode.StartsWith(TEXT("FirstEffectCancel")) || RefusalMode.StartsWith(TEXT("CancelRewardField")) || RefusalMode.StartsWith(TEXT("CancelRewardBoundary")))
                Test->TestTrue(TEXT("Teardown disposes active or completed reward boundary"),Host->GetState()==EMemoriaSliceState::Idle);
            Test->TestEqual(TEXT("No stale continuation after owner teardown"), FString::Join(Host->GetTrace(),TEXT("\n")), CancellationTrace);
            Test->TestEqual(TEXT("Teardown preserves run"), StateJson(Run->GetRunSnapshot()), CancellationRun);
            Test->TestEqual(TEXT("Teardown preserves memory"), StateJson(Run->GetPlayerMemory()->GetSnapshot()), CancellationMemory);
            Test->TestEqual(TEXT("Native travel retains persistent cognition exact"),Run->GetWorldCognition()->ExportJson(),CancellationWorld);
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
            {
                // Only redraw recorded/live development views after the synchronous reward has completed.
                // This changes capture presentation, never grant order or gameplay timing.
                if(Label==TEXT("Firebomb_Before") || Label==TEXT("Firebomb_Granted") || Label==TEXT("Reward_Toasts") || Label==TEXT("Shop_Deferred"))
                {
                    if(auto* Widget=PC->GetNarrativeWidget()){Widget->ForceVolatile(true);Widget->InvalidateLayoutAndVolatility();Widget->ForceLayoutPrepass();}
                    if(auto Window=World->GetGameViewport()->GetWindow())FSlateApplication::Get().ForceRedrawWindow(Window.ToSharedRef());
                }
                FScreenshotRequest::RequestScreenshot(Output()/(Mode()+TEXT("_")+Label+TEXT(".png")), true, false);
            }
        };
        if(RefusalMode==TEXT("ShopBattle") && Stage==18)
        {
            if(Frame==5){BattleReturnTrace=Host->GetTrace();Key(EKeys::Enter,IE_Repeat);Key(EKeys::Escape,IE_Repeat);}
            if(Frame==9)
            {
                Test->TestFalse(TEXT("Unmatched repeat after actual travel cannot open a modal"),PC->IsModalOpen());
                Test->TestTrue(TEXT("Held confirm after actual travel cannot advance narrative"),Host->GetTrace()==BattleReturnTrace);
                Key(EKeys::Enter,IE_Released);Key(EKeys::Escape,IE_Released);
            }
        }
        if(RefusalMode==TEXT("ShopCanonical") && PC->GetNarrativeWidget() && PC->GetNarrativeWidget()->DisplayedBackdrop())
        {
            const FString Header=Host->GetView().Header;
            if(Header!=LastArtworkHeader){LastArtworkHeader=Header;ArtworkStableFrames=0;}
            else if(++ArtworkStableFrames==3)Capture(FString::Printf(TEXT("Artwork_%02d"),ArtworkCaptureIndex++));
        }
        if (!bStarted)
        {
            if(RefusalMode==TEXT("ShopCheckpoint") || RefusalMode==TEXT("ShopArchive") || RefusalMode==TEXT("ShopBattle"))Test->TestTrue(TEXT("Canonical saves are isolated"),Run->GetGameInstance()->GetSubsystem<UMemoriaCheckpointSubsystem>()->ConfigureTestStorage(TEXT("canonical-")+FGuid::NewGuid().ToString(EGuidFormats::Digits)));
            bStarted = true; bWasFixed = FApp::UseFixedTimeStep(); OldDelta = FApp::GetFixedDeltaTime();
            FApp::SetUseFixedTimeStep(true); FApp::SetFixedDeltaTime(1.0/60.0);
            // Resize the actual PIE window, independent of remembered editor
            // client dimensions that shrink across unattended map sessions.
            if (auto Window = World->GetGameViewport()->GetWindow()) Window->Resize(FVector2D(1280, 720));
            RunId = Run->GetRunSnapshot().RunId; Domain = Run->GetPlayerMemory(); OriginalWorld = World; WorldDomain = Run->GetWorldCognition();
            if (!Test->TestNotNull(TEXT("Imported Malet asset ready"), Asset())) return true;
        }
        if(RefusalMode==TEXT("ShopArchive"))
        {
            int32 NextProbe=INDEX_NONE;
            if(Stage==1 && Frame==0)NextProbe=0;
            if(Stage==14 && Frame==0)NextProbe=1;
            if(Stage==14 && Frame==50)NextProbe=2;
            if(Stage==14 && Frame==65)NextProbe=3;
            if(Stage==14 && Frame==81)NextProbe=4;
            if(ArchiveProbe==INDEX_NONE && NextProbe!=INDEX_NONE && !(ArchiveCompleted&(1<<NextProbe)))
            {
                ArchiveProbe=NextProbe;ArchiveFrame=0;
                ArchiveBefore=Canon(MemoriaPotionEvidence::Full(*Run));ArchiveTrace=Host->GetTrace();
                ArchivePosition=Pawn->GetActorLocation();ArchiveHostState=Host->GetState();
                ArchiveShopBefore=Canon(MemoriaShopEvidence::View(Run->GetGameInstance()->GetSubsystem<UMemoriaShopSubsystem>()->GetView()));
            }
            if(ArchiveProbe!=INDEX_NONE)
            {
                static const TCHAR* Labels[]={TEXT("archive_exploration"),TEXT("archive_shop_before"),TEXT("archive_after_trade"),TEXT("archive_checkpoint"),TEXT("archive_restored")};
                const FKey OpenKey=ArchiveProbe%2?EKeys::M:EKeys::Tab;
                const FKey CloseKey=ArchiveProbe==1?EKeys::Escape:OpenKey;
                auto SlateDown=[](FKey K,bool Repeat=false)
                { FSlateApplication::Get().ProcessKeyDownEvent(FKeyEvent(K,FModifierKeysState(),0,Repeat,0,0)); };
                auto SlateUp=[](FKey K)
                { FSlateApplication::Get().ProcessKeyUpEvent(FKeyEvent(K,FModifierKeysState(),0,false,0,0)); };
                if(ArchiveFrame==0)Key(OpenKey,IE_Pressed);
                if(ArchiveFrame==3)Key(OpenKey,IE_Released);
                auto* Archive=PC->GetArchiveWidget();
                if(ArchiveFrame==4)
                {
                    if(!Test->TestNotNull(TEXT("Tab/M opens the real archive widget"),Archive))return true;
                    Test->TestTrue(TEXT("Archive owns modal focus and blocks movement"),PC->IsModalOpen() && PC->IsMoveInputIgnored() && Archive->HasUserFocus(PC));
                    Test->TestTrue(TEXT("Archive replaces only presentation of the underlying owner"),PC->GetNarrativeWidget()==nullptr && Host->GetState()==ArchiveHostState);
                    Test->TestEqual(TEXT("Archive starts at all grades"),Archive->GetFilter(),-1);
                    const auto Snapshot=Run->GetPlayerMemory()->GetSnapshot();const auto& View=Archive->GetView();
                    Test->TestEqual(TEXT("Archive includes every actual owned entry"),View.Rows.Num(),Snapshot.Owned.Num());
                    Test->TestEqual(TEXT("Archive burn count matches authoritative history"),View.BurnedCount,Snapshot.BurnedHistory.Num());
                    for(const auto& Owned:Snapshot.Owned)
                    {
                        const auto* Row=View.Rows.FindByPredicate([&](const auto& R){return R.Id==Owned.Id;});
                        if(Test->TestNotNull(TEXT("Owned ID has a visible archive row"),Row))
                            Test->TestTrue(TEXT("Burn, residue and fade feedback reflect actual memory state"),Row->bBurned==Owned.bBurned && Row->bResidue==Owned.bResidue && Row->bFaded==Owned.bFaded);
                    }
                    if(ArchiveProbe>=2)
                    {
                        const auto* Bought=View.Rows.FindByPredicate([](const auto& R){return R.Id==TEXT("sense_copper_taste");});
                        Test->TestTrue(TEXT("Purchased source stock appears intact in archive after trade/load"),Bought && !Bought->bBurned && !Bought->bFaded);
                        Test->TestEqual(TEXT("Archive observes two sales plus prior burns"),View.BurnedCount,4);
                        Test->TestEqual(TEXT("Archive observes purchase without spending again"),Run->GetRunSnapshot().Player.Grains,int64(2));
                    }
                }
                if(ArchiveFrame==5)
                {
                    SlateDown(OpenKey,true);
                    Test->TestTrue(TEXT("Slate auto-repeat of opening Tab/M keeps archive open"),PC->GetArchiveWidget()==Archive && Archive!=nullptr);
                }
                if(ArchiveFrame==6)Key(EKeys::Right,IE_Pressed);
                if(ArchiveFrame==9)Key(EKeys::Right,IE_Released);
                if(ArchiveFrame==10 && Archive)
                {
                    Test->TestEqual(TEXT("Physical Right selects sensory grade"),Archive->GetFilter(),0);
                    for(const auto& Row:Archive->GetView().Rows)Test->TestEqual(TEXT("Grade filter excludes other grades"),Row.Grade,0);
                    ArchiveSelection=Archive->GetSelectedId();
                }
                if(ArchiveFrame==12)Key(EKeys::Down,IE_Pressed);
                if(ArchiveFrame==15)Key(EKeys::Down,IE_Released);
                if(ArchiveFrame==16 && Archive && Archive->GetView().Rows.Num()>1)
                    Test->TestTrue(TEXT("Physical Down browses cards"),Archive->GetSelectedId()!=ArchiveSelection);
                if(ArchiveFrame==18)Key(EKeys::Left,IE_Pressed);
                if(ArchiveFrame==21)Key(EKeys::Left,IE_Released);
                if(ArchiveFrame==24 && Archive)
                {
                    Test->TestEqual(TEXT("Physical Left restores all grades"),Archive->GetFilter(),-1);
                    const FString DetailId=ArchiveProbe>=2?TEXT("sense_copper_taste"):FString(MemoriaMaletReaction::Food);
                    const int32 DetailIndex=Archive->GetView().Rows.IndexOfByPredicate([&](const auto& R){return R.Id==DetailId;});
                    if(Test->TestTrue(TEXT("Trade or source burn detail is selectable"),DetailIndex!=INDEX_NONE))
                    {
                        Archive->Select(DetailIndex);const auto& Row=Archive->GetView().Rows[DetailIndex];
                        Test->TestTrue(TEXT("Selected detail contains source title and state"),Archive->VisibleText().Contains(Row.Title) && Archive->VisibleText().Contains(Row.StateLabel));
                        Test->TestEqual(TEXT("Stable memory ID owns selected detail"),Archive->GetSelectedId(),DetailId);
                        Test->TestNotNull(TEXT("Archive displays retained provisional original artwork"),Archive->DisplayedArtwork());
                    }
                }
                if(ArchiveFrame==26)Capture(Labels[ArchiveProbe]);
                if(ArchiveFrame==28)for(const FKey K:{EKeys::Enter,EKeys::E,EKeys::SpaceBar,EKeys::Gamepad_FaceButton_Bottom})SlateDown(K);
                if(ArchiveFrame==32)for(const FKey K:{EKeys::E,EKeys::SpaceBar,EKeys::Gamepad_FaceButton_Bottom})SlateUp(K);
                if(ArchiveFrame==34)Key(EKeys::D,IE_Pressed);
                if(ArchiveFrame==40)Key(EKeys::D,IE_Released);
                if(ArchiveFrame==42)
                {
                    Test->TestEqual(TEXT("Browsing and confirmation cannot mutate full run/world/memory"),Canon(MemoriaPotionEvidence::Full(*Run)),ArchiveBefore);
                    Test->TestTrue(TEXT("Archive cannot advance hidden narrative"),Host->GetTrace()==ArchiveTrace);
                    Test->TestEqual(TEXT("Archive cannot select or transact in hidden shop"),Canon(MemoriaShopEvidence::View(Run->GetGameInstance()->GetSubsystem<UMemoriaShopSubsystem>()->GetView())),ArchiveShopBefore);
                    Test->TestTrue(TEXT("Movement input is physically blocked while reading"),Pawn->GetActorLocation().Equals(ArchivePosition,.001));
                    Write(Host,Run,Pawn,Labels[ArchiveProbe]);
                }
                // Closing while Enter is still physically held must not click the restored owner.
                if(ArchiveFrame==46)
                {
                    if(ArchiveProbe==1)SlateDown(CloseKey);
                    else Key(CloseKey,IE_Pressed);
                }
                if(ArchiveFrame==49 && ArchiveProbe!=1)Key(CloseKey,IE_Released);
                if(ArchiveFrame==50)
                {
                    // These now arrive through the restored viewport after Slate ate the presses.
                    if(ArchiveProbe==1)SlateDown(CloseKey,true);
                    SlateDown(EKeys::Enter,true);
                }
                if(ArchiveFrame==54)
                {
                    if(ArchiveProbe==1)SlateUp(CloseKey);
                    SlateUp(EKeys::Enter);
                }
                if(ArchiveFrame==60)
                {
                    Test->TestTrue(TEXT("Tab/M or Slate Escape closes archive and restores the same owner"),PC->GetArchiveWidget()==nullptr && Host->GetState()==ArchiveHostState);
                    if(ArchiveProbe==0)AssertExploration(PC,Host,World);
                    else Test->TestTrue(TEXT("Shop/checkpoint screen restored after archive"),PC->GetNarrativeWidget()!=nullptr && PC->IsModalOpen());
                    Test->TestEqual(TEXT("Slate-consumed confirm/close repeats cannot leak after closing archive"),Canon(MemoriaPotionEvidence::Full(*Run)),ArchiveBefore);
                    Test->TestTrue(TEXT("Closing archive cannot replay dialogue or checkpoint load"),Host->GetTrace()==ArchiveTrace);
                    Test->TestEqual(TEXT("Closing archive preserves underlying shop selection"),Canon(MemoriaShopEvidence::View(Run->GetGameInstance()->GetSubsystem<UMemoriaShopSubsystem>()->GetView())),ArchiveShopBefore);
                    ArchiveCompleted|=1<<ArchiveProbe;ArchiveProbe=INDEX_NONE;
                }
                ++ArchiveFrame;return false;
            }
            // Authored dialogue remains the input owner; opening a read-only archive cannot skip it.
            if(Stage==3 && Frame==1)ArchiveBefore=Canon(MemoriaPotionEvidence::Full(*Run));
            if(Stage==3 && Frame==2)Key(EKeys::Tab,IE_Pressed);
            if(Stage==3 && Frame==4)
            {
                Key(EKeys::Tab,IE_Released);
                Test->TestTrue(TEXT("Archive shortcut cannot interrupt an authored Field row"),PC->GetArchiveWidget()==nullptr && Host->GetState()==EMemoriaSliceState::Field);
                Test->TestEqual(TEXT("Rejected archive shortcut preserves full dialogue state"),Canon(MemoriaPotionEvidence::Full(*Run)),ArchiveBefore);
            }
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
                Test->TestTrue(TEXT("Reward request now executes existing Field with modal"), Host->GetState()==EMemoriaSliceState::Field && PC->IsModalOpen());
                Test->TestEqual(TEXT("Reaction, normal, deal, reward exactly once"),Host->GetFieldInvocationCount(),4);
                Test->TestEqual(TEXT("Reward invocation exactly once"),Host->GetRewardFieldInvocationCount(),1);
                Test->TestTrue(TEXT("Source-like reward listener pending"),Host->IsRewardCallbackPending());
                if(RefusalMode==TEXT("FirstEffectPreexistingTrue") || RefusalMode==TEXT("FirstEffectPreexistingFalse"))
                    Test->TestTrue(TEXT("Supplementary preexisting flag via authoritative API"),Run->SetStoryFlag(TEXT("ch2_malet_done"),RefusalMode==TEXT("FirstEffectPreexistingTrue")));
                // Read-only synchronous snapshots; cancellation probes use real run/world APIs.
                {
                    const FString Point=RefusalMode.Contains(TEXT("BeforeCallback"))?TEXT("before_callback"):(RefusalMode.Contains(TEXT("BeforeFlag"))?TEXT("before_flag"):TEXT("after_flag"));
                    ObservedHost=Host; BoundaryObserverHandle=Host->OnRewardBoundaryObserved.AddLambda([this,World,Run,Host,Point](const FString& Observed)
                    {
                        Test->TestEqual(TEXT("All player definitions/connections/power/carry remain exact at world boundary"),Canon(MemoriaPlayerObservation(*Run)),RewardBeforeObservables);
                        Write(Host,Run,nullptr,TEXT("synchronous_")+Observed);
                        if(Observed==TEXT("after_flag"))Write(Host,Run,nullptr,TEXT("before_world_seed"));
                        if(Observed==TEXT("after_knowledge") || Observed==TEXT("after_memory") || Observed==TEXT("world_seed_complete") || Observed.Contains(TEXT("potion")) || Observed.StartsWith(TEXT("after_inventory")) || Observed==TEXT("after_recent_items"))Write(Host,Run,nullptr,Observed);
                        if(Observed==TEXT("potion_contract_complete"))
                        {
                            AssertPotionComplete(Host,Run);
                            CompareReward(Host,Run,TEXT("completion_before_callback"),true,true);
                            if(RefusalMode==TEXT("PotionCanonical"))MemoriaPotionEvidence::SaveRoundTrip(*Test,*Run,TEXT("potion_canonical_save.json"));
                        }
                        if(Observed==TEXT("antidote_contract_complete"))
                        {
                            AssertAntidoteComplete(Host,Run);
                            CompareReward(Host,Run,TEXT("completion_before_callback"),true,false,true);
                            if(RefusalMode==TEXT("AntidoteCanonical"))MemoriaPotionEvidence::SaveRoundTrip(*Test,*Run,TEXT("antidote_canonical_save.json"));
                        }
                        const FString PotionPoint=RefusalMode.Contains(TEXT("BeforePotion"))?TEXT("before_potion"):TEXT("after_inventory_changed");
                        if(RefusalMode.StartsWith(TEXT("PotionCancel")) && Observed==PotionPoint)
                        {Write(Host,Run,nullptr,TEXT("lifetime_before_cancel"));Cancel(World,Run,Host);return;}
                        const bool Preexisting=RefusalMode==TEXT("FirstEffectPreexistingTrue") || RefusalMode==TEXT("FirstEffectPreexistingFalse");
                        Test->TestEqual(TEXT("Presence at exact synchronous boundary"),Run->GetRunSnapshot().HasFlag(TEXT("ch2_malet_done")),(Observed!=TEXT("before_callback") && Observed!=TEXT("before_flag")) || Preexisting);
                        if(Observed==TEXT("after_flag")) Test->TestTrue(TEXT("Committed flag before seed boundary"),Run->GetRunSnapshot().GetFlag(TEXT("ch2_malet_done")));
                        if(!RefusalMode.StartsWith(TEXT("FirstEffectCancel")) || Observed!=Point) return;
                        Test->TestEqual(TEXT("Exact synchronous flag timing at lifetime boundary"),Run->GetRunSnapshot().HasFlag(TEXT("ch2_malet_done")),Point==TEXT("after_flag"));
                        Write(Host,Run,nullptr,TEXT("lifetime_before_cancel"));
                        Cancel(World,Run,Host);
                    });
                }
                ObservedRun=Run;
                InventoryObserverHandle=Run->OnInventoryChanged.AddLambda([this,World,Run,Host](const FString& Id)
                {
                    Write(Host,Run,nullptr,TEXT("actual_signal_")+Id);
                    if((RefusalMode==TEXT("FirebombSignalFirebombOnRunReplace") && Id==TEXT("firebomb")) ||
                       (RefusalMode==TEXT("AntidoteSignalPotionOnRunReplace") && Id==TEXT("potion")) ||
                       (RefusalMode==TEXT("AntidoteSignalAntidoteOnRunReplace") && Id==TEXT("antidote")))
                    {Write(Host,Run,nullptr,TEXT("lifetime_before_cancel"));Cancel(World,Run,Host);}
                });
                RewardBeforeSnapshot=Run->GetRunSnapshot(); RewardBeforeRun=StateJson(RewardBeforeSnapshot); RewardBeforeMemory=StateJson(Run->GetPlayerMemory()->GetSnapshot()); RewardBeforeObservables=Canon(MemoriaPlayerObservation(*Run));
                RewardBeforePosition=Pawn->GetActorLocation();
                auto Preserved=Run->GetRunSnapshot();
                Preserved.StoryFlags.RemoveAll([](const auto& F){return F.Id==TEXT("malet_deal_accepted") || F.Id==TEXT("talked_Malet_malet_encounter");});
                const bool Preexisting=RefusalMode==TEXT("FirstEffectPreexistingTrue") || RefusalMode==TEXT("FirstEffectPreexistingFalse");
                if(Preexisting) Preserved.StoryFlags.RemoveAll([](const auto& F){return F.Id.Equals(TEXT("ch2_malet_done"),ESearchCase::CaseSensitive);});
                Test->TestEqual(TEXT("Entire run outside source flags and explicit supplementary setup unchanged"),StateJson(Preserved),NormalBeforeRun);
                Test->TestEqual(TEXT("Canonical reward flag absent; supplementary entry is explicit"),Run->GetRunSnapshot().HasFlag(TEXT("ch2_malet_done")),Preexisting);
                CompareReward(Host,Run,TEXT("reward_first"));
                Capture(TEXT("Reward_FirstLine")); Write(Host,Run,Pawn,TEXT("before_reward"));
                Stage=13; Frame=-1;
            }
        }
        else if (Stage == 13)
        {
            // Every press passes real Enhanced Input and the existing widget.
            if (Frame==2) { Key(EKeys::D,IE_Pressed); Key(EKeys::Escape,IE_Pressed); }
            if (Frame==6) { Key(EKeys::D,IE_Released); Key(EKeys::Escape,IE_Released); }
            if (Frame==8)
            {
                Test->TestTrue(TEXT("Reward owns modal input; Back cannot cancel"),PC->IsModalOpen() && PC->IsMoveInputIgnored() && Host->GetState()==EMemoriaSliceState::Field);
                Test->TestTrue(TEXT("Physical movement blocked during reward"),Pawn->GetActorLocation().Equals(RewardBeforePosition,.01f));
            }
            if (Frame<160 && Frame%20==0)
            {
                const int32 Row=Frame/20;
                Test->TestEqual(TEXT("Reward original row exact"),Host->GetView().Body,RewardAsset()->Definition.Rows[Row].Text.Text);
                Test->TestTrue(TEXT("Reward has no invented choices"),Host->GetView().Choices.IsEmpty());
                Test->TestTrue(TEXT("Reward source text displayed in existing widget"),PC->GetNarrativeWidget() && PC->GetNarrativeWidget()->VisibleText().Contains(RewardAsset()->Definition.Rows[Row].Text.Text));
                Test->TestEqual(TEXT("Every reward row preserves full run"),StateJson(Run->GetRunSnapshot()),RewardBeforeRun);
                Test->TestEqual(TEXT("Every reward row preserves full memory"),StateJson(Run->GetPlayerMemory()->GetSnapshot()),RewardBeforeMemory);
                CompareReward(Host,Run,FString::Printf(TEXT("reward_row_%d"),Row));
                if(Row==3) { Capture(TEXT("Reward_MiddleLine")); Write(Host,Run,Pawn,TEXT("reward_middle")); }
                if(Row==7) { Capture(TEXT("Reward_LastLine")); Write(Host,Run,Pawn,TEXT("reward_last")); }
                if(Row==3 && RefusalMode.StartsWith(TEXT("CancelRewardField"))) { Cancel(World,Run,Host); return false; }
            }
            if (Frame<160 && Frame%20==10) { Key(EKeys::E,IE_Pressed); if(Stage!=13) return false; }
            if (Frame<160 && Frame%20==14) Key(EKeys::E,IE_Released);
            if(Frame==158)
            {
                AssertRewardStop(PC,Host,Run); CompareReward(Host,Run,TEXT("completion_before_callback"),true);
                Test->TestTrue(TEXT("First shop screen has no selected memory"),PC->GetNarrativeWidget()->GetShopWidget()->SelectedRow()==INDEX_NONE);
                Capture(TEXT("Reward_Completion")); Write(Host,Run,Pawn,TEXT("reward_completion"));
                if(RefusalMode.StartsWith(TEXT("CancelRewardBoundary")) || RefusalMode.StartsWith(TEXT("PotionAfterStop")) || RefusalMode.StartsWith(TEXT("AntidoteAfterStop")) || RefusalMode.StartsWith(TEXT("FirebombAfterStop"))) { Cancel(World,Run,Host); return false; }
            }
            if(Frame==161) Host->PresentSeedObservation(0);
            if(Frame==163) Capture(TEXT("MaletDone_Set"));
            if(Frame==165) Host->PresentSeedObservation(1);
            if(Frame==167) Capture(TEXT("WorldKnowledge_Seeded"));
            if(Frame==169) Host->PresentSeedObservation(2);
            if(Frame==171) { Capture(TEXT("WorldMemory_Seeded")); Capture(TEXT("WorldSeed_Complete")); }
            if(Frame==173) Host->PresentPotionObservation(0);
            if(Frame==175) Capture(TEXT("Potion_Before"));
            if(Frame==177) Host->PresentPotionObservation(1);
            if(Frame==179) Capture(TEXT("Potion_Granted"));
            if(Frame==181) Host->PresentPotionObservation(4);
            if(Frame==183) Capture(TEXT("Potion_Toast"));
            if(Frame==185) Host->PresentAntidoteObservation(0);
            if(Frame==187) Capture(TEXT("Antidote_Before"));
            if(Frame==189) Host->PresentAntidoteObservation(1);
            if(Frame==191) Capture(TEXT("Antidote_Granted"));
            if(Frame==193) Host->PresentAntidoteObservation(3);
            if(Frame==195) Capture(TEXT("Antidote_Signal"));
            if(Frame==197) Host->PresentAntidoteObservation(4);
            if(Frame==199) Capture(TEXT("Antidote_Toast"));
            if(Frame==201) Host->PresentFirebombObservation(0);
            if(Frame==209) Capture(TEXT("Firebomb_Before"));
            if(Frame==213) Host->PresentFirebombObservation(1);
            if(Frame==221) Capture(TEXT("Firebomb_Granted"));
            if(Frame==225) Host->PresentFirebombObservation(4);
            if(Frame==233) Capture(TEXT("Reward_Toasts"));
            if(Frame==237) Host->PresentFirebombObservation(INDEX_NONE);
            if(Frame==239) {Key(EKeys::E,IE_Pressed);Key(EKeys::Enter,IE_Pressed);Key(EKeys::SpaceBar,IE_Pressed);}
            if(Frame==250 && RefusalMode==TEXT("ShopCanonical"))Key(EKeys::Down,IE_Pressed);
            if(Frame==254 && RefusalMode==TEXT("ShopCanonical"))Key(EKeys::Down,IE_Released);
            if(Frame==258) {Key(EKeys::E,IE_Released);Key(EKeys::Enter,IE_Released);Key(EKeys::SpaceBar,IE_Released);}
            if(Frame==164) { Key(EKeys::E,IE_Pressed); Key(EKeys::D,IE_Pressed); }
            if(Frame==168) { Key(EKeys::E,IE_Released); Key(EKeys::D,IE_Released); }
            if(Frame==264)
            {
                AssertRewardStop(PC,Host,Run);
                Test->TestTrue(TEXT("Development stop cannot move pawn"),Pawn->GetActorLocation().Equals(RewardBeforePosition,.01f));
                CompareReward(Host,Run,TEXT("completion_before_callback"),true);
                Capture(TEXT("Shop_Deferred")); Write(Host,Run,Pawn,TEXT("shop_deferred"));
                auto* Shop=Run->GetGameInstance()->GetSubsystem<UMemoriaShopSubsystem>();
                TestTrueShop(PC,Shop,Run);
                if(RefusalMode==TEXT("ShopCanonical")) {Capture(TEXT("Shop_FirstScreen"));Write(Host,Run,Pawn,TEXT("shop_first_screen"));MemoriaPotionEvidence::SaveRoundTrip(*Test,*Run,TEXT("shop_canonical_save.json"));}
                if(RefusalMode==TEXT("FirebombCanonical"))MemoriaPotionEvidence::SaveRoundTrip(*Test,*Run,TEXT("save_roundtrip.json")); Write(Host,Run,Pawn,TEXT("reward_effects_deferred"));
                Stage=(RefusalMode==TEXT("ShopTransactions") || RefusalMode==TEXT("ShopCheckpoint") || RefusalMode==TEXT("ShopArchive") || RefusalMode==TEXT("ShopBattle"))?14:9; Frame=-1;
            }
        }
        else if (Stage == 8 && Frame == 40)
        {
            Test->TestTrue(TEXT("Cancelled callback cannot start stale dialogue"),Host->GetState()==EMemoriaSliceState::Idle && Host->GetTrace().IsEmpty());
            Test->TestEqual(TEXT("Cancelled callback cannot mutate replacement run"),StateJson(Run->GetRunSnapshot()),CancellationRun);
            Test->TestEqual(TEXT("Cancelled callback cannot mutate replacement memory"),StateJson(Run->GetPlayerMemory()->GetSnapshot()),CancellationMemory);
            Test->TestEqual(TEXT("Replacement world revision reset"),Run->GetWorldCognition()->GetSnapshot().Revision,int64(0));
            Test->TestFalse(TEXT("Reward listener cancelled on run replacement"),Host->IsRewardCallbackPending());
            Test->TestTrue(TEXT("Both owned timer handles cancelled"),!Host->IsMaletDelayPending() && !Host->IsMaletRewardDelayPending());
            Write(Host,Run,Pawn,TEXT("cancelled_callback")); return true;
        }
        else if (Stage == 14)
        {
            auto* Shop=Run->GetGameInstance()->GetSubsystem<UMemoriaShopSubsystem>();
            if(Frame==0 || Frame==18 || Frame==36)Key(EKeys::Down,IE_Pressed);
            if(Frame==3 || Frame==21 || Frame==39)Key(EKeys::Down,IE_Released);
            if(Frame==6 || Frame==24 || Frame==42)Key(EKeys::Enter,IE_Pressed);
            if(Frame==9 || Frame==27 || Frame==45)Key(EKeys::Enter,IE_Released);
            if(Frame==12) {Capture(TEXT("Shop_Sold"));Test->TestEqual(TEXT("First physical sale credits five grains"),Run->GetRunSnapshot().Player.Grains,int64(5));}
            if(Frame==30)Key(EKeys::Right,IE_Pressed);
            if(Frame==33)Key(EKeys::Right,IE_Released);
            if(Frame==48)
            {
                Capture(TEXT("Shop_Bought"));
                Test->TestTrue(TEXT("Physical tab and confirm acquire source stock"),Run->GetPlayerMemory()->IsIntact(TEXT("sense_copper_taste")));
                Test->TestEqual(TEXT("Two sales minus copper cost"),Run->GetRunSnapshot().Player.Grains,int64(2));
                Test->TestEqual(TEXT("Successful trade clears selection"),PC->GetNarrativeWidget()->GetShopWidget()->SelectedRow(),INDEX_NONE);
            }
            if(Frame==54)Key(EKeys::Escape,IE_Pressed);
            if(Frame==57)Key(EKeys::Escape,IE_Released);
            if(Frame==60)Capture(TEXT("Shop_Closed"));
            if(Frame==64)
            {
                Test->TestFalse(TEXT("Escape closes live shop"),Shop->IsOpen());
                Test->TestTrue(TEXT("Close commits chapter flag"),Run->GetRunSnapshot().GetFlag(TEXT("ch2_complete")));
                Test->TestEqual(TEXT("Close assigns source chapter"),Run->GetRunSnapshot().CurrentChapter,int64(3));
                Test->TestEqual(TEXT("No unimplemented travel"),Host->GetDeferredInteraction(),FString(TEXT("before:chapter_transition_delay")));
                Test->TestTrue(TEXT("Chapter 3 remains explicitly deferred"),Host->GetView().Body.Contains(TEXT("Chapter 3")));
                Test->TestTrue(TEXT("Actual save status visible"),PC->GetNarrativeWidget()->VisibleText().Contains(Run->GetGameInstance()->GetSubsystem<UMemoriaCheckpointSubsystem>()->GetStatusText()));
                Write(Host,Run,Pawn,TEXT("shop_transactions_closed"));
                MemoriaPotionEvidence::SaveRoundTrip(*Test,*Run,TEXT("shop_transactions_canonical_save.json"));
                if(RefusalMode!=TEXT("ShopCheckpoint") && RefusalMode!=TEXT("ShopArchive") && RefusalMode!=TEXT("ShopBattle"))return true;
                Test->TestTrue(TEXT("Close wrote real disk"),Run->GetGameInstance()->GetSubsystem<UMemoriaCheckpointSubsystem>()->GetStatusText().Contains(TEXT("saved to disk")));
                CheckpointBefore=Canon(MemoriaPotionEvidence::Full(*Run));CheckpointPosition=Pawn->GetActorLocation();
                Run->SetStoryFlag(TEXT("after_save_mutation"),true);
            }
            if(RefusalMode==TEXT("ShopCheckpoint") || RefusalMode==TEXT("ShopArchive") || RefusalMode==TEXT("ShopBattle"))
            {
                if(Frame==68)Key(EKeys::Enter,IE_Pressed);
                if(Frame==71)Key(EKeys::Enter,IE_Released);
                if(Frame==80)
                {
                    Test->TestEqual(TEXT("Physical load restored complete disk state"),Canon(MemoriaPotionEvidence::Full(*Run)),CheckpointBefore);
                    Test->TestTrue(TEXT("Physical load restored pawn"),Pawn->GetActorLocation().Equals(CheckpointPosition,.01f));
                    Test->TestTrue(TEXT("No reward/arrival replay"),Host->GetTrace()==TArray<FString>{TEXT("checkpoint:restored:before:chapter_transition_delay")});
                    Test->TestTrue(TEXT("Loaded status and actions rendered"),PC->GetNarrativeWidget()->VisibleText().Contains(TEXT("Checkpoint loaded")) && PC->GetNarrativeWidget()->VisibleText().Contains(TEXT("Save checkpoint again")));
                    Capture(TEXT("Checkpoint_Loaded"));Write(Host,Run,Pawn,TEXT("checkpoint_loaded"));
                    if(RefusalMode==TEXT("ShopBattle")){Stage=15;Frame=-1;}
                    else if(RefusalMode!=TEXT("ShopArchive"))return true;
                }
            }
        }
        else if(Stage==15)
        {
            if(Frame==1)Key(EKeys::Up,IE_Pressed);
            if(Frame==4)Key(EKeys::Up,IE_Released);
            if(Frame==8)
            {
                Test->TestFalse(TEXT("Loading alone does not activate encounters"),Host->IsVerdanRevisit());
                Test->TestEqual(TEXT("Physical Up selects new reentry choice"),PC->GetNarrativeWidget()->SelectedOriginalIndex(),3);
                BattleOwner=World;Key(EKeys::Enter,IE_Pressed);
            }
            if(Frame>=9 && World!=BattleOwner.Get() && Host->IsVerdanRevisit())
            {
                Key(EKeys::Enter,IE_Released);
                Test->TestEqual(TEXT("Actual checkpoint reentry preserves full state"),Canon(MemoriaPotionEvidence::Full(*Run)),CheckpointBefore);
                Test->TestTrue(TEXT("Reentry restores saved field position"),Pawn->GetActorLocation().Equals(CheckpointPosition,.01f));
                Test->TestTrue(TEXT("Actual reentry reaches exploration"),Host->GetState()==EMemoriaSliceState::Exploration);
                BattleOwner=World;Stage=16;Frame=-1;
            }
        }
        else if(Stage==16)
        {
            auto* Battle=Run->GetGameInstance()->GetSubsystem<UMemoriaBattleEntrySubsystem>();
            if(Battle->IsActive() && PC->GetBattleWidget())
            {
                Key(EKeys::D,IE_Released);Key(EKeys::A,IE_Released);
                FirstBattle=Battle->GetView();
                Test->TestTrue(TEXT("Actual walking emitted warning before encounter"),bBattleWarning);
                Test->TestTrue(TEXT("Source first-turn HP scaling"),FirstBattle.PlayerHp==115 && FirstBattle.PlayerMaxHp==130);
                Test->TestEqual(TEXT("Source battle_started increments persistent statistic"),Run->GetRunSnapshot().TotalBattles,int64(1));
                Test->TestEqual(TEXT("Entry preserves Grains"),Run->GetRunSnapshot().Player.Grains,int64(2));
                Test->TestTrue(TEXT("Battle modal blocks movement"),PC->IsModalOpen() && PC->IsMoveInputIgnored());
                Test->TestTrue(TEXT("Battle source artwork loaded"),PC->GetBattleWidget()->DisplayedBackdrop() && PC->GetBattleWidget()->DisplayedPlayerArtwork() && PC->GetBattleWidget()->DisplayedEnemyArtwork());
                if(FirstBattle.EnemyIndex==0)Test->TestEqual(TEXT("Alley Rat study displays instead of legacy hound"),PC->GetBattleWidget()->DisplayedEnemyArtwork(),MemoriaBattleEntryArt::Load(MemoriaBattleEntryArt::AlleyRatStudySource()));
                PC->ToggleArchive();Test->TestNull(TEXT("Archive cannot interrupt battle"),PC->GetArchiveWidget());
                Test->TestFalse(TEXT("Direct Malet interaction cannot interrupt battle"),Host->InteractWithMalet());
                BattleBeforeFlee=Canon(MemoriaPotionEvidence::Full(*Run));Stage=17;Frame=-1;
            }
            else
            {
                if(Frame%160==0){Key(EKeys::A,IE_Released);Key(EKeys::D,IE_Pressed);}
                if(Frame%160==80){Key(EKeys::D,IE_Released);Key(EKeys::A,IE_Pressed);}
                if(PC->GetEncounterModel().bWarningEmitted && !bBattleWarning)
                {bBattleWarning=true;Capture(TEXT("EncounterWarning"));}
            }
        }
        else if(Stage==17)
        {
            auto* Battle=Run->GetGameInstance()->GetSubsystem<UMemoriaBattleEntrySubsystem>();
            if(Frame==24){Capture(TEXT("BattleEntry"));Write(Host,Run,Pawn,TEXT("battle_entry"));}
            if(Frame==30)
            {
                const uint64 Revision=Battle->GetRevision();
                Test->TestFalse(TEXT("Stale flee cannot consume turn"),Battle->Flee(Revision-1));
                FSlateApplication::Get().ProcessKeyDownEvent(FKeyEvent(EKeys::Enter,FModifierKeysState(),0,false,0,0));
                Test->TestTrue(TEXT("Physical confirm begins source return delay"),Battle->IsReturning());
                Test->TestFalse(TEXT("Duplicate flee rejected"),Battle->Flee(Revision));
                FSlateApplication::Get().ProcessKeyDownEvent(FKeyEvent(EKeys::Enter,FModifierKeysState(),0,true,0,0));
                Test->TestEqual(TEXT("Ambient flee grants no rewards or memory changes"),Canon(MemoriaPotionEvidence::Full(*Run)),BattleBeforeFlee);
            }
            // Hold Enter across the actual world replacement; release in the new owner.
            if(Frame==36)
            {
                Test->TestTrue(TEXT("Original 300ms cleanup has not fired early"),World==BattleOwner.Get() && Battle->IsReturning());
                Capture(TEXT("BattleWithdrawal"));
            }
            if(Frame>36 && World!=BattleOwner.Get() && Host->IsVerdanRevisit())
            { Stage=18;Frame=-1; }
        }
        else if(Stage==18 && Frame==30)
        {
            auto* Battle=Run->GetGameInstance()->GetSubsystem<UMemoriaBattleEntrySubsystem>();
            Test->TestFalse(TEXT("Real field return disposes battle"),Battle->IsActive());
            Test->TestNull(TEXT("Real field return disposes modal"),PC->GetBattleWidget());
            Test->TestTrue(TEXT("Real field return restores controls"),!PC->IsModalOpen() && !PC->IsMoveInputIgnored());
            Test->TestTrue(TEXT("Source flee respawns at 128,288"),Memoria::Coordinates::ToSource(Pawn->GetActorLocation()).Equals(FVector2D(128,288),.01));
            Test->TestEqual(TEXT("Map return preserves completed first-turn state"),Canon(MemoriaPotionEvidence::Full(*Run)),BattleBeforeFlee);
            Test->TestTrue(TEXT("New visit resets encounter distance"),PC->GetEncounterModel().StepCount<.01);
            Capture(TEXT("FieldReturned"));Write(Host,Run,Pawn,TEXT("field_returned"));
            // Second image is an explicit alternative-enemy presentation fixture.
            AlternativeEnemy=1-FirstBattle.EnemyIndex;auto Rng=FMemoriaEncounterRng::Random();
            Test->TestTrue(TEXT("Alternative source enemy starts in live owner"),Battle->BeginEncounter(AlternativeEnemy,Rng,World));
            Stage=19;Frame=-1;
        }
        else if(Stage==19)
        {
            if(Frame==30)
            {
                Test->TestNotNull(TEXT("Alternative source artwork rendered"),PC->GetBattleWidget());
                if(AlternativeEnemy==0 && PC->GetBattleWidget())Test->TestEqual(TEXT("Alternative Alley Rat study displays"),PC->GetBattleWidget()->DisplayedEnemyArtwork(),MemoriaBattleEntryArt::Load(MemoriaBattleEntryArt::AlleyRatStudySource()));
                Capture(TEXT("BattleAlternativeEnemy"));Write(Host,Run,Pawn,TEXT("alternative_enemy_fixture"));
                PC->GetBattleWidget()->ConfirmIntent();BattleOwner=World;
            }
            if(Frame>32 && World!=BattleOwner.Get() && Host->IsVerdanRevisit())
            {Stage=20;Frame=-1;}
        }
        else if(Stage==20 && Frame==24)
        {
            Test->TestEqual(TEXT("Two starts persist exactly two battles"),Run->GetRunSnapshot().TotalBattles,int64(2));
            Test->TestFalse(TEXT("Second source ambient flee clears modal"),PC->IsModalOpen());
            Capture(TEXT("SecondReturn"));return true;
        }
        else if (Stage == 9 && Frame == 10) return true;
        if(RefusalMode==TEXT("ShopArchive") && Stage==14 && Frame==82)
        {
            Test->TestEqual(TEXT("Exploration, shop, trade, checkpoint and restored archive probes completed"),ArchiveCompleted,31);
            Test->TestEqual(TEXT("All archive interactions preserve restored disk state"),Canon(MemoriaPotionEvidence::Full(*Run)),CheckpointBefore);
            return true;
        }

        ++Frame; return false;
    }
private:
    FAutomationTestBase* Test; bool bPaid, bStarted=false, bWasFixed=false;
    double Started, OldDelta=0; uint64 LastFrame=MAX_uint64; int32 Frame=0, Stage=0, ArrivalTraceCount=0;
    FGuid RunId; TWeakObjectPtr<UMemoriaWorldCognition> WorldDomain; TWeakObjectPtr<UMemoriaPlayerMemoryDomain> Domain; TWeakObjectPtr<UWorld> OriginalWorld;
    TWeakObjectPtr<AMemoriaMaletActor> Malet; FKey Held; FString BeforeMemory;
    FMemoriaRunSnapshot BeforeRun; FVector BeforePosition, CameraOrigin;
    FMemoriaRunSnapshot RewardBeforeSnapshot;
    FDelegateHandle InventoryObserverHandle; TWeakObjectPtr<UMemoriaRunSubsystem> ObservedRun;
    FDelegateHandle BoundaryObserverHandle; TWeakObjectPtr<UMemoriaNarrativeSubsystem> ObservedHost;
    FString RewardBeforeRun, RewardBeforeMemory, RewardBeforeObservables; FVector RewardBeforePosition;
    FString CheckpointBefore; FVector CheckpointPosition;
    TWeakObjectPtr<UWorld> BattleOwner;
    bool bBattleWarning=false;FMemoriaBattleEntryView FirstBattle;
    FString BattleBeforeFlee;int32 AlternativeEnemy=INDEX_NONE;TArray<FString> BattleReturnTrace;
    int32 ArchiveProbe=INDEX_NONE,ArchiveFrame=0,ArchiveCompleted=0;
    FString ArchiveBefore,ArchiveShopBefore,ArchiveSelection;TArray<FString> ArchiveTrace;
    FVector ArchivePosition;EMemoriaSliceState ArchiveHostState=EMemoriaSliceState::Idle;
    FString RefusalMode, NormalBeforeRun, BeforeAcceptRun, BeforeAcceptMemory, CancellationRun, CancellationMemory, CancellationTrace, CancellationWorld;
    TWeakObjectPtr<UWorld> CancelledWorld;
    bool IsDealMode() const
    {
        return RefusalMode.StartsWith(TEXT("Shop")) || RefusalMode.StartsWith(TEXT("Firebomb")) || RefusalMode.StartsWith(TEXT("Antidote")) || RefusalMode.StartsWith(TEXT("Potion")) || RefusalMode==TEXT("WorldSeedCanonical") || RefusalMode.StartsWith(TEXT("FirstEffect")) || RefusalMode==TEXT("CanonicalReward") || RefusalMode==TEXT("AcceptPreEffectDeferred") || RefusalMode==TEXT("CanonicalPayment") ||
            RefusalMode==TEXT("AlreadyBurnedPayment") || RefusalMode.StartsWith(TEXT("CancelNormal")) || RefusalMode.StartsWith(TEXT("CancelReward"));
    }
    UMemoriaFieldAsset* DealAsset() const { return LoadObject<UMemoriaFieldAsset>(nullptr,*MemoriaNarrativeImport::ObjectPath(false,TEXT("malet_deal"))); }
    UMemoriaFieldAsset* RewardAsset() const { return LoadObject<UMemoriaFieldAsset>(nullptr,*MemoriaNarrativeImport::ObjectPath(false,TEXT("malet_reward"))); }
    void TestTrueShop(AMemoriaSliceController* PC,UMemoriaShopSubsystem* Shop,UMemoriaRunSubsystem* Run)
    {
        const auto Before=MemoriaPotionEvidence::Full(*Run);
        Test->TestTrue(TEXT("Shop is actual run-owned production subsystem"),Shop->IsOpen());
        Test->TestTrue(TEXT("Real widget brushes hold both imported source textures"),PC->GetNarrativeWidget()->GetShopWidget()->HasArtwork());
        if(RefusalMode==TEXT("ShopCanonical"))Test->TestEqual(TEXT("Physical Down previews first source row"),PC->GetNarrativeWidget()->GetShopWidget()->SelectedRow(),0);
        Test->TestEqual(TEXT("One canonical shop entry"),Shop->GetOpenCount(),1);
        Test->TestFalse(TEXT("Repeated open ignored"),Shop->OpenMalet());
        Test->TestEqual(TEXT("Duplicate entry preserves full state"),Canon(Before),Canon(MemoriaPotionEvidence::Full(*Run)));
        Test->TestEqual(TEXT("Default sell list excludes payment burns and core"),Shop->GetView().Rows.Num(),4);
        Test->TestTrue(TEXT("Actual widget displays shop title and inventory"),PC->GetNarrativeWidget()->VisibleText().Contains(Shop->GetView().Title)&&PC->GetNarrativeWidget()->VisibleText().Contains(Shop->GetView().Rows[0].Title));
    }
    void AssertRewardStop(AMemoriaSliceController* PC,UMemoriaNarrativeSubsystem* Host,UMemoriaRunSubsystem* Run)
    {
        Test->TestTrue(TEXT("Completed Field retained in development modal stop"),Host->GetState()==EMemoriaSliceState::Deferred && PC->IsModalOpen() && PC->IsMoveInputIgnored());
        Test->TestEqual(TEXT("One reward Field completion"),Host->GetRewardCompletionCount(),1);
        Test->TestEqual(TEXT("One synchronous callback intent"),Host->GetRewardCallbackIntentCount(),1);
        Test->TestFalse(TEXT("Completed one-shot no longer pending"),Host->IsRewardCallbackPending());
        Test->TestEqual(TEXT("Exact pre-effect deferred target"),Host->GetDeferredInteraction(),FString(TEXT("before:shop_actions")));
        auto ExpectedRun=RewardBeforeSnapshot;
        auto* Done=ExpectedRun.StoryFlags.FindByPredicate([](const auto& V){return V.Id.Equals(TEXT("ch2_malet_done"),ESearchCase::CaseSensitive);});
        if(Done) Done->bValue=true;
        else { FMemoriaStoryFlag F; F.Id=TEXT("ch2_malet_done");F.bValue=true;ExpectedRun.StoryFlags.Add(F); }
        FMemoriaItemCount Potion;Potion.Id=TEXT("potion");Potion.Count=2;ExpectedRun.Player.Items.Add(Potion);ExpectedRun.Player.RecentItems={TEXT("potion")};
        FMemoriaItemCount Antidote;Antidote.Id=TEXT("antidote");Antidote.Count=1;ExpectedRun.Player.Items.Add(Antidote);ExpectedRun.Player.RecentItems={TEXT("antidote"),TEXT("potion")};
        FMemoriaItemCount Firebomb;Firebomb.Id=TEXT("firebomb");Firebomb.Count=1;ExpectedRun.Player.Items.Add(Firebomb);ExpectedRun.Player.RecentItems={TEXT("firebomb"),TEXT("antidote"),TEXT("potion")};
        Test->TestEqual(TEXT("Only authorized run deltas are done flag and source potion plus antidote plus firebomb contracts"),StateJson(Run->GetRunSnapshot()),StateJson(ExpectedRun));
        Test->TestEqual(TEXT("All derived player observations unchanged at STOP"),Canon(MemoriaPlayerObservation(*Run)),RewardBeforeObservables);
        Test->TestEqual(TEXT("Full memory unchanged from reward entry"),StateJson(Run->GetPlayerMemory()->GetSnapshot()),RewardBeforeMemory);
        Test->TestTrue(TEXT("First source effect persisted as an actual true entry"),Run->GetRunSnapshot().HasFlag(TEXT("ch2_malet_done")) && Run->GetRunSnapshot().GetFlag(TEXT("ch2_malet_done")));
        Test->TestTrue(TEXT("Same run ID and domain through reward stop"),RunId==Run->GetRunSnapshot().RunId && Domain.Get()==Run->GetPlayerMemory());
        Test->TestTrue(TEXT("No pending source or invented reward completion timer"),!Host->IsMaletDelayPending() && !Host->IsMaletRewardDelayPending());
        Test->TestEqual(TEXT("No downstream field or duplicate invocation"),Host->GetFieldInvocationCount(),4);
        Test->TestTrue(TEXT("Same persistent world owner through native travel and seed"),WorldDomain.Get()==Run->GetWorldCognition());
        auto GoldWorld=Json(Base()/TEXT("fixtures/malet_world_seed/contract_expected.v1.json"));
        if(Test->TestTrue(TEXT("Fresh source seed oracle exists"),GoldWorld.IsValid()))
        { Obj Current; FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Run->GetWorldCognition()->ExportJson()),Current);
          Test->TestEqual(TEXT("Canonical actual world equals entire source fresh result"),Canon(Current),Canon(GoldWorld->AsArray()[0]->AsObject()->GetObjectField(TEXT("after")))); }

    }
    void AssertAntidoteComplete(UMemoriaNarrativeSubsystem* Host,UMemoriaRunSubsystem* Run)
    {
        Test->TestEqual(TEXT("One reward Field completion"),Host->GetRewardCompletionCount(),1);
        Test->TestEqual(TEXT("One synchronous callback intent"),Host->GetRewardCallbackIntentCount(),1);
        Test->TestFalse(TEXT("Completed one-shot no longer pending"),Host->IsRewardCallbackPending());
        auto ExpectedRun=RewardBeforeSnapshot;
        auto* Done=ExpectedRun.StoryFlags.FindByPredicate([](const auto& V){return V.Id.Equals(TEXT("ch2_malet_done"),ESearchCase::CaseSensitive);});
        if(Done) Done->bValue=true;
        else { FMemoriaStoryFlag F; F.Id=TEXT("ch2_malet_done");F.bValue=true;ExpectedRun.StoryFlags.Add(F); }
        FMemoriaItemCount Potion;Potion.Id=TEXT("potion");Potion.Count=2;ExpectedRun.Player.Items.Add(Potion);ExpectedRun.Player.RecentItems={TEXT("potion")};
        FMemoriaItemCount Antidote;Antidote.Id=TEXT("antidote");Antidote.Count=1;ExpectedRun.Player.Items.Add(Antidote);ExpectedRun.Player.RecentItems={TEXT("antidote"),TEXT("potion")};
        Test->TestEqual(TEXT("Only authorized run deltas are done flag and source potion plus antidote contracts"),StateJson(Run->GetRunSnapshot()),StateJson(ExpectedRun));
        Test->TestEqual(TEXT("All derived player observations unchanged at STOP"),Canon(MemoriaPlayerObservation(*Run)),RewardBeforeObservables);
        Test->TestEqual(TEXT("Full memory unchanged from reward entry"),StateJson(Run->GetPlayerMemory()->GetSnapshot()),RewardBeforeMemory);
        Test->TestTrue(TEXT("First source effect persisted as an actual true entry"),Run->GetRunSnapshot().HasFlag(TEXT("ch2_malet_done")) && Run->GetRunSnapshot().GetFlag(TEXT("ch2_malet_done")));
        Test->TestTrue(TEXT("Same run ID and domain through reward stop"),RunId==Run->GetRunSnapshot().RunId && Domain.Get()==Run->GetPlayerMemory());
        Test->TestTrue(TEXT("No pending source or invented reward completion timer"),!Host->IsMaletDelayPending() && !Host->IsMaletRewardDelayPending());
        Test->TestEqual(TEXT("No downstream field or duplicate invocation"),Host->GetFieldInvocationCount(),4);
        Test->TestTrue(TEXT("Same persistent world owner through native travel and seed"),WorldDomain.Get()==Run->GetWorldCognition());
        auto GoldWorld=Json(Base()/TEXT("fixtures/malet_world_seed/contract_expected.v1.json"));
        if(Test->TestTrue(TEXT("Fresh source seed oracle exists"),GoldWorld.IsValid()))
        { Obj Current; FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Run->GetWorldCognition()->ExportJson()),Current);
          Test->TestEqual(TEXT("Canonical actual world equals entire source fresh result"),Canon(Current),Canon(GoldWorld->AsArray()[0]->AsObject()->GetObjectField(TEXT("after")))); }

    }
    void AssertPotionComplete(UMemoriaNarrativeSubsystem* Host,UMemoriaRunSubsystem* Run)
    {
        auto ExpectedRun=RewardBeforeSnapshot;
        auto* Done=ExpectedRun.StoryFlags.FindByPredicate([](const auto& V){return V.Id.Equals(TEXT("ch2_malet_done"),ESearchCase::CaseSensitive);});
        if(Done) Done->bValue=true;
        else { FMemoriaStoryFlag F; F.Id=TEXT("ch2_malet_done");F.bValue=true;ExpectedRun.StoryFlags.Add(F); }
        FMemoriaItemCount Potion;Potion.Id=TEXT("potion");Potion.Count=2;ExpectedRun.Player.Items.Add(Potion);ExpectedRun.Player.RecentItems={TEXT("potion")};
        Test->TestEqual(TEXT("Only authorized run deltas are done flag and source potion contract"),StateJson(Run->GetRunSnapshot()),StateJson(ExpectedRun));
        Test->TestEqual(TEXT("All derived player observations unchanged at STOP"),Canon(MemoriaPlayerObservation(*Run)),RewardBeforeObservables);
        Test->TestEqual(TEXT("Full memory unchanged from reward entry"),StateJson(Run->GetPlayerMemory()->GetSnapshot()),RewardBeforeMemory);
        Test->TestTrue(TEXT("First source effect persisted as an actual true entry"),Run->GetRunSnapshot().HasFlag(TEXT("ch2_malet_done")) && Run->GetRunSnapshot().GetFlag(TEXT("ch2_malet_done")));
        Test->TestTrue(TEXT("Same run ID and domain through reward stop"),RunId==Run->GetRunSnapshot().RunId && Domain.Get()==Run->GetPlayerMemory());
        Test->TestTrue(TEXT("No pending source or invented reward completion timer"),!Host->IsMaletDelayPending() && !Host->IsMaletRewardDelayPending());
        Test->TestEqual(TEXT("No downstream field or duplicate invocation"),Host->GetFieldInvocationCount(),4);
        Test->TestTrue(TEXT("Same persistent world owner through native travel and seed"),WorldDomain.Get()==Run->GetWorldCognition());
        auto GoldWorld=Json(Base()/TEXT("fixtures/malet_world_seed/contract_expected.v1.json"));
        if(Test->TestTrue(TEXT("Fresh source seed oracle exists"),GoldWorld.IsValid()))
        { Obj Current; FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Run->GetWorldCognition()->ExportJson()),Current);
          Test->TestEqual(TEXT("Canonical actual world equals entire source fresh result"),Canon(Current),Canon(GoldWorld->AsArray()[0]->AsObject()->GetObjectField(TEXT("after")))); }

    }
    void CompareReward(UMemoriaNarrativeSubsystem* Host,UMemoriaRunSubsystem* Run,const FString& Label,bool Completed=false,bool PotionOnly=false,bool AntidoteOnly=false)
    {
        auto Expected=RewardCase(TEXT("reward_en"),Label);
        if(!Test->TestTrue(TEXT("Executable reward source state exists"),Expected.IsValid()))return;
        auto Gold=Strings(Expected->GetArrayField(TEXT("events")));
        if(Completed) { Gold.Add(TEXT("callback:reward:enter")); Gold.Add(TEXT("flag:ch2_malet_done")); Gold.Add(TEXT("worldseed:enter")); Gold.Add(TEXT("world:knowledge:npc.malet:fact.bl07.route_request_received")); Gold.Add(TEXT("world:revision:1")); Gold.Add(TEXT("world:memory:npc.malet:memory.malet.bl07_request_source")); Gold.Add(TEXT("world:revision:2")); Gold.Add(TEXT("worldseed:end"));
            for(const TCHAR* E:{TEXT("item:add:begin:potion:2"),TEXT("inventory:potion:0->2"),TEXT("recent_items:potion"),TEXT("inventory_changed:potion"),TEXT("toast:+2 Potion:1"),TEXT("item:add:end:potion:2")})Gold.Add(E);
            if(!PotionOnly)for(const TCHAR* E:{TEXT("item:add:begin:antidote:1"),TEXT("inventory:antidote:0->1"),TEXT("recent_items:antidote,potion"),TEXT("inventory_changed:antidote"),TEXT("toast:+1 Antidote:1"),TEXT("item:add:end:antidote:1")})Gold.Add(E);
            if(!PotionOnly && !AntidoteOnly)for(const TCHAR* E:{TEXT("item:add:begin:firebomb:1"),TEXT("inventory:firebomb:0->1"),TEXT("recent_items:firebomb,antidote,potion"),TEXT("inventory_changed:firebomb"),TEXT("toast:+1 Firebomb:1"),TEXT("item:add:end:firebomb:1"),TEXT("development:deferred:before:shop_open"),TEXT("shop:open:Malet:sell"),TEXT("request:audio:ui_open"),TEXT("request:achievement:check_grains"),TEXT("request:tutorial:first_shop"),TEXT("development:deferred:before:shop_actions")})Gold.Add(E); }
        // Keep the historical already-burned payment prefix intact. Compare all
        // reward events against the source and retain the Phase1H memory oracle.
        TArray<FString> Actual;for(int32 I=NormalTraceStart;I<Host->GetTrace().Num();++I)Actual.Add(Host->GetTrace()[I]);
        Actual=Normalize(Actual);
        if(RefusalMode==TEXT("AlreadyBurnedPayment"))
        {
            const int32 Start=Actual.IndexOfByKey(TEXT("field:start:malet_reward"));
            const int32 GStart=Gold.IndexOfByKey(TEXT("field:start:malet_reward"));
            Actual.RemoveAt(0,Start);Gold.RemoveAt(0,GStart);
            CompareDealMemory(*Test,Run->GetPlayerMemory()->GetSnapshot(),DealCase(TEXT("already_burned"),TEXT("reward_boundary")));
        }
        else CompareDealMemory(*Test,Run->GetPlayerMemory()->GetSnapshot(),Expected);
        Test->TestEqual(TEXT("Exact source reward entry/rows/completion order up to pre-effect cut"),FString::Join(Actual,TEXT("\n")),FString::Join(Gold,TEXT("\n")));
    }
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
            CancellationTrace=FString::Join(Host->GetTrace(),TEXT("\n")); CancellationWorld=Run->GetWorldCognition()->ExportJson(); CancelledWorld=World;
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
    FString Output() const { return FPaths::ProjectSavedDir()/TEXT("Validation/Phase1O"); }
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
        if(Label.StartsWith(TEXT("actual_signal_"))){auto Payload=MakeShared<FJsonObject>();Payload->SetStringField(TEXT("item_id"),Label.Mid(14));O->SetObjectField(TEXT("actual_signal_payload"),Payload);O->SetStringField(TEXT("observation_kind"),TEXT("actual inventory_changed listener; no post-broadcast substitution"));}
        O->SetArrayField(TEXT("trace"),Events); O->SetObjectField(TEXT("run"),FJsonObjectConverter::UStructToJsonObject(Run->GetRunSnapshot()));
        O->SetObjectField(TEXT("memory"),FJsonObjectConverter::UStructToJsonObject(Run->GetPlayerMemory()->GetSnapshot()));
        O->SetObjectField(TEXT("player_observables"),MemoriaPlayerObservation(*Run));
        O->SetBoolField(TEXT("same_run_id"),RunId==Run->GetRunSnapshot().RunId);
        O->SetBoolField(TEXT("same_memory_domain"),Domain.Get()==Run->GetPlayerMemory());
        O->SetStringField(TEXT("state"),Label); O->SetStringField(TEXT("pawn_position"),Pawn?Pawn->GetActorLocation().ToString():TEXT("owner_world_destroyed"));
        O->SetNumberField(TEXT("actual_delay_microseconds"),FMath::RoundToDouble(Host->GetMaletDelaySeconds()*1000000.0)); O->SetBoolField(TEXT("talk_cache"),Host->IsMaletTalkCached()); O->SetBoolField(TEXT("normal_callback_connected"),Host->IsMaletCallbackConnected());
        O->SetNumberField(TEXT("field_invocations"),Host->GetFieldInvocationCount()); O->SetNumberField(TEXT("reaction_invocations"),Host->GetMaletReactionCount());
        O->SetNumberField(TEXT("actual_reward_delay_microseconds"),FMath::RoundToDouble(Host->GetMaletRewardDelaySeconds()*1000000.0));
        O->SetBoolField(TEXT("normal_delay_pending"),Host->IsMaletDelayPending()); O->SetBoolField(TEXT("reward_delay_pending"),Host->IsMaletRewardDelayPending());
        O->SetNumberField(TEXT("reward_field_invocations"),Host->GetRewardFieldInvocationCount());
        O->SetNumberField(TEXT("reward_completions"),Host->GetRewardCompletionCount());
        O->SetNumberField(TEXT("reward_callback_intents"),Host->GetRewardCallbackIntentCount());
        O->SetBoolField(TEXT("reward_callback_pending"),Host->IsRewardCallbackPending());
        O->SetBoolField(TEXT("ch2_malet_done_value"),Run->GetRunSnapshot().GetFlag(TEXT("ch2_malet_done")));
        O->SetBoolField(TEXT("ch2_malet_done_present"),Run->GetRunSnapshot().HasFlag(TEXT("ch2_malet_done")));
        // Observe the production shop owner without rewriting historical snapshots.
        Obj WorldState;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Run->GetWorldCognition()->ExportJson()),WorldState);
        O->SetObjectField(TEXT("world_memory_snapshot"),WorldState);
        O->SetBoolField(TEXT("same_world_domain"),WorldDomain.Get()==Run->GetWorldCognition());
        const auto ShopView=Run->GetGameInstance()->GetSubsystem<UMemoriaShopSubsystem>()->GetView();
        if(ShopView.bOpen)O->SetObjectField(TEXT("shop_snapshot"),MemoriaShopEvidence::View(ShopView));
        else O->SetField(TEXT("shop_snapshot"),MakeShared<FJsonValueNull>());
        O->SetStringField(TEXT("downstream_owners"),TEXT("World and shop owners are authoritative; transactions, close callback, autosave and request handlers remain deferred."));
        Obj Counts=MakeShared<FJsonObject>();
        for(const TCHAR* Prefix:{TEXT("flag:ch2_malet_done"),TEXT("world:"),TEXT("item:"),TEXT("shop:"),TEXT("chapter:"),TEXT("autosave:"),TEXT("achievement:")})
        {
            int32 Count=0;bool InReward=false;
            for(const auto& E:Normalize(Host->GetTrace())) { if(E==TEXT("field:start:malet_reward"))InReward=true; if(InReward && E.StartsWith(Prefix))++Count; }
            Counts->SetNumberField(Prefix,Count);
        }
        O->SetObjectField(TEXT("observed_downstream_effect_events"),Counts);
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
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRewardImport,"Memoria.MaletReward.ImportContract",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRewardImport::RunTest(const FString&)
{
    const FString Path=Base()/TEXT("ir/narrative/malet_reward.field.v1.json");
    TStrongObjectPtr<UMemoriaFieldAsset> Actual(LoadObject<UMemoriaFieldAsset>(nullptr,*MemoriaNarrativeImport::ObjectPath(false,TEXT("malet_reward")))), Expected(NewObject<UMemoriaFieldAsset>());
    FString Error;
    if(!TestNotNull(TEXT("Saved typed Malet reward package"),Actual.Get()) || !TestTrue(TEXT("Strict source attestation"),MemoriaNarrativeImport::ReadIr(Path,*Expected,Error))) { AddError(Error); return false; }
    TestEqual(TEXT("Exactly eight authored rows"),Actual->Definition.Rows.Num(),8);
    for(TFieldIterator<FProperty> P(UMemoriaFieldAsset::StaticClass(),EFieldIterationFlags::None);P;++P)
        TestTrue(*P->GetName(),P->Identical_InContainer(Actual.Get(),Expected.Get()));
    for(int32 I=0;I<Actual->Definition.Rows.Num();++I)
    {
        const auto& Row=Actual->Definition.Rows[I];
        TestEqual(TEXT("Original row index"),Row.OriginalIndex,I);
        TestEqual(TEXT("Original group position"),Row.Provenance.GroupPosition,3);
        TestTrue(TEXT("Both localized text presence bits"),Row.Text.bHasText && Row.Text.bHasTextKo);
        TestTrue(TEXT("No invented reward choices"),Row.Choices.IsEmpty());
    }
    Obj Report; TestTrue(TEXT("Repeated import is semantic no-op"),MemoriaNarrativeImport::Import(Path,false,true,Report,Error));
    TestTrue(TEXT("No unnecessary package save"),Report && Report->GetStringField(TEXT("result"))==TEXT("UNCHANGED") && !Report->GetBoolField(TEXT("saved")));
    for(const TCHAR* Variant:{TEXT("modified"),TEXT("reject_position"),TEXT("reject_count"),TEXT("reject_index"),TEXT("reject_downstream")})
    {
        TStrongObjectPtr<UMemoriaFieldAsset> Probe(NewObject<UMemoriaFieldAsset>());
        const auto File=Base()/TEXT("fixtures/malet_reward")/(FString(Variant)+TEXT(".field.v1.json"));
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
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FMaletRewardRuntime,"Memoria.MaletReward",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
void FMaletRewardRuntime::GetTests(TArray<FString>& Names,TArray<FString>& Commands) const
{
    for(const TCHAR* N:{TEXT("CanonicalReward"),TEXT("CancelRewardFieldOnRunReplace"),TEXT("CancelRewardFieldOnWorldTeardown"),TEXT("CancelRewardBoundaryOnRunReplace"),TEXT("CancelRewardBoundaryOnWorldTeardown")}) {Names.Add(N);Commands.Add(N);}
}
bool FMaletRewardRuntime::RunTest(const FString& Parameters)
{
    if(!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice")))return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FMaletReplay(this,true,Parameters)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRewardLanguages,"Memoria.MaletReward.EnglishKoreanExecution",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRewardLanguages::RunTest(const FString&)
{
    TStrongObjectPtr<UGameInstance> Game(NewObject<UGameInstance>());Game->Init();
    auto* Run=Game->GetSubsystem<UMemoriaRunSubsystem>();
    auto* Reward=LoadObject<UMemoriaFieldAsset>(nullptr,*MemoriaNarrativeImport::ObjectPath(false,TEXT("malet_reward")));
    if(!TestNotNull(TEXT("Typed reward asset"),Reward)) {Game->Shutdown();return false;}
    for(const TCHAR* Locale:{TEXT("en"),TEXT("ko")})
    {
        Run->BeginStartingMemoryRun();Run->BurnMemory(MemoriaMaletReaction::Food);Run->BurnMemory(TEXT("identity_first_sword"));
        auto State=Run->GetRunSnapshot();State.CurrentLocale=Locale;
        const auto Before=StateJson(State),Memory=StateJson(Run->GetPlayerMemory()->GetSnapshot());
        FMemoriaNarrativeContext Context(State,*Run->GetPlayerMemory());FMemoriaFieldInterpreter Field(Reward->Definition,Context);
        Field.Start();
        for(int32 I=0;I<8;++I)
        {
            const auto& Row=Reward->Definition.Rows[I];
            TestEqual(TEXT("Eight exact originals"),Field.OriginalIndex(),I);
            TestTrue(TEXT("No choices at any row"),Field.VisibleOriginalIndices().IsEmpty() && !Row.bChoicesPresent);
            TestEqual(TEXT("Exact source localized text"),Context.Localized(Row.Text),FString(Locale)==TEXT("ko")?Row.Text.TextKo:Row.Text.Text);
            Field.Advance();
        }
        TestFalse(TEXT("Reward completes at eight"),Field.IsActive());
        TestEqual(TEXT("No reward row mutates run"),StateJson(State),Before);
        TestEqual(TEXT("No reward row mutates memory"),StateJson(Run->GetPlayerMemory()->GetSnapshot()),Memory);
        auto Expected=RewardCase(FString(Locale)==TEXT("ko")?TEXT("reward_ko"):TEXT("reward_en"),TEXT("completion_before_callback"));
        if(TestTrue(TEXT("Executed localized oracle exists"),Expected.IsValid()))
        {
            auto Events=Strings(Expected->GetArrayField(TEXT("events")));const int32 Start=Events.IndexOfByKey(TEXT("field:start:malet_reward"))+1;
            TArray<FString> Golden;for(int32 I=Start;I<Events.Num();++I) {Golden.Add(Events[I]);if(Events[I]==TEXT("end"))break;}
            TestEqual(TEXT("Exact source English/Korean interpreter events"),FString::Join(Context.Events,TEXT("\n")),FString::Join(Golden,TEXT("\n")));
        }
    }
    Game->Shutdown();return !HasAnyErrors();
}
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FMaletFirstEffectRuntime,"Memoria.MaletFirstEffect",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
void FMaletFirstEffectRuntime::GetTests(TArray<FString>& Names,TArray<FString>& Commands) const
{
    for(const TCHAR* N:{TEXT("Canonical"),TEXT("PreexistingTrue"),TEXT("PreexistingFalse"),TEXT("CancelBeforeCallbackOnRunReplace"),TEXT("CancelBeforeFlagOnRunReplace"),TEXT("CancelAfterFlagOnRunReplace"),TEXT("CancelBeforeCallbackOnWorldTeardown"),TEXT("CancelBeforeFlagOnWorldTeardown"),TEXT("CancelAfterFlagOnWorldTeardown")}) {Names.Add(N);Commands.Add(FString(TEXT("FirstEffect"))+N);}
}
bool FMaletFirstEffectRuntime::RunTest(const FString& Parameters)
{
    if(!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice")))return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FMaletReplay(this,true,Parameters)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorldSeedCanonical,"Memoria.WorldSeed.Canonical",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FWorldSeedCanonical::RunTest(const FString&)
{
    if(!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice")))return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FMaletReplay(this,true,TEXT("WorldSeedCanonical"))));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFirstFlagContract,"Memoria.MaletFirstEffect.AuthoritativeFlagContract",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFirstFlagContract::RunTest(const FString&)
{
    auto Cases=Json(Base()/TEXT("fixtures/malet_first_effect/contract_expected.v1.json"));
    if(!TestTrue(TEXT("Fresh executable first-effect source contract"),Cases.IsValid()))return false;
    for(auto C:Cases->AsArray())
    {
        auto O=C->AsObject();const FString Id=O->GetStringField(TEXT("id"));
        if(Id==TEXT("seed_guard_false"))continue;
        auto Boundary=O->GetArrayField(TEXT("boundary_snapshots"))[0]->AsObject();
        const auto Events=Strings(Boundary->GetArrayField(TEXT("events")));
        TestEqual(TEXT("Source callback commits flag before seed entry"),FString::Join(Events,TEXT("\n")),FString(TEXT("callback:reward:enter\nflag:ch2_malet_done")));
        TestTrue(TEXT("Source boundary flag is true"),Boundary->GetObjectField(TEXT("flags"))->GetBoolField(TEXT("ch2_malet_done")));
    }
    TStrongObjectPtr<UGameInstance> Game(NewObject<UGameInstance>());Game->Init();auto* Run=Game->GetSubsystem<UMemoriaRunSubsystem>();
    TestTrue(TEXT("Real starting run"),Run->BeginStartingMemoryRun()==EMemoriaMemoryResult::Success);
    const auto Before=Run->GetRunSnapshot();const FString Memory=StateJson(Run->GetPlayerMemory()->GetSnapshot());
    TestFalse(TEXT("Absent is distinct from false entry"),Before.HasFlag(TEXT("ch2_malet_done")));
    TestTrue(TEXT("Case-distinct false entry"),Run->SetStoryFlag(TEXT("CH2_MALET_DONE"),false));
    TestTrue(TEXT("Authoritative explicit false"),Run->SetStoryFlag(TEXT("ch2_malet_done"),false));
    TestTrue(TEXT("Explicit false exists"),Run->GetRunSnapshot().HasFlag(TEXT("ch2_malet_done")));
    TestTrue(TEXT("Authoritative false to true"),Run->SetStoryFlag(TEXT("ch2_malet_done"),true));
    const FString Committed=StateJson(Run->GetRunSnapshot());
    TestTrue(TEXT("Repeated true set succeeds"),Run->SetStoryFlag(TEXT("ch2_malet_done"),true));
    TestEqual(TEXT("Repeated set preserves exact storage without duplicates"),StateJson(Run->GetRunSnapshot()),Committed);
    TestTrue(TEXT("Case-sensitive positive lookup"),Run->GetRunSnapshot().GetFlag(TEXT("ch2_malet_done")));
    TestFalse(TEXT("Case-sensitive other key remains false"),Run->GetRunSnapshot().GetFlag(TEXT("CH2_MALET_DONE")));
    TestFalse(TEXT("Mixed case remains absent"),Run->GetRunSnapshot().HasFlag(TEXT("Ch2_malet_done")));
    TestEqual(TEXT("Flag API does not replace run"),Run->GetRunSnapshot().RunId,Before.RunId);
    TestEqual(TEXT("Flag API preserves memory"),StateJson(Run->GetPlayerMemory()->GetSnapshot()),Memory);
    auto* Save=NewObject<UMemoriaRunSaveGame>();Save->Run=Run->GetRunSnapshot();Save->ContentRevision=Save->Run.ContentRevision;
    Save->MemoryDefinitions=Run->GetPlayerMemory()->GetDefinitions();Save->PlayerMemory=Run->GetPlayerMemory()->GetSnapshot();
    TArray<uint8> Bytes;TestTrue(TEXT("Real SaveGame serialization"),UGameplayStatics::SaveGameToMemory(Save,Bytes));
    auto* Loaded=Cast<UMemoriaRunSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
    if(TestNotNull(TEXT("Real save DTO reload"),Loaded))TestEqual(TEXT("True/false and case identities survive save"),StateJson(Loaded->Run),Committed);
    Game->Shutdown();return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPotionCanonicalPotionGrant,"Memoria.Potion.CanonicalPotionGrant",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FPotionCanonicalPotionGrant::RunTest(const FString&)
{ if(!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice")))return false; FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FMaletReplay(this,true,TEXT("PotionCanonical")))); ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());return true; }
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPotionReplacementBeforePotion,"Memoria.Potion.ReplacementBeforePotion",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FPotionReplacementBeforePotion::RunTest(const FString&)
{ if(!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice")))return false; FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FMaletReplay(this,true,TEXT("PotionCancelBeforePotionOnRunReplace")))); ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());return true; }
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPotionReplacementAfterCommit,"Memoria.Potion.ReplacementAfterCommit",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FPotionReplacementAfterCommit::RunTest(const FString&)
{ if(!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice")))return false; FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FMaletReplay(this,true,TEXT("PotionCancelAfterCommitOnRunReplace")))); ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());return true; }
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPotionReplacementAtStop,"Memoria.Potion.ReplacementAtStop",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FPotionReplacementAtStop::RunTest(const FString&)
{ if(!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice")))return false; FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FMaletReplay(this,true,TEXT("PotionAfterStopOnRunReplace")))); ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());return true; }
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPotionWorldTeardownAfterCommit,"Memoria.Potion.WorldTeardownAfterCommit",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FPotionWorldTeardownAfterCommit::RunTest(const FString&)
{ if(!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice")))return false; FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FMaletReplay(this,true,TEXT("PotionAfterStopOnWorldTeardown")))); ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());return true; }
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBattleRenderedRevisit,"Memoria.BattleEntry.RenderedRevisitFlow",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FBattleRenderedRevisit::RunTest(const FString&)
{ if(!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice")))return false; FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FMaletReplay(this,true,TEXT("ShopBattle")))); ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());return true; }
#endif

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAntidoteCanonical,"Memoria.Antidote.Canonical",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FAntidoteCanonical::RunTest(const FString&)
{ if(!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice")))return false; FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FMaletReplay(this,true,TEXT("AntidoteCanonical")))); ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());return true; }
#endif

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAntidotePotionSignalReplacement,"Memoria.Antidote.PotionSignalReplacement",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FAntidotePotionSignalReplacement::RunTest(const FString&)
{ if(!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice")))return false; FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FMaletReplay(this,true,TEXT("AntidoteSignalPotionOnRunReplace")))); ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());return true; }
#endif

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAntidoteAntidoteSignalReplacement,"Memoria.Antidote.AntidoteSignalReplacement",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FAntidoteAntidoteSignalReplacement::RunTest(const FString&)
{ if(!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice")))return false; FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FMaletReplay(this,true,TEXT("AntidoteSignalAntidoteOnRunReplace")))); ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());return true; }
#endif

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAntidoteReplacementAtStop,"Memoria.Antidote.ReplacementAtStop",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FAntidoteReplacementAtStop::RunTest(const FString&)
{ if(!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice")))return false; FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FMaletReplay(this,true,TEXT("AntidoteAfterStopOnRunReplace")))); ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());return true; }
#endif

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAntidoteWorldTeardownAtStop,"Memoria.Antidote.WorldTeardownAtStop",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FAntidoteWorldTeardownAtStop::RunTest(const FString&)
{ if(!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice")))return false; FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FMaletReplay(this,true,TEXT("AntidoteAfterStopOnWorldTeardown")))); ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());return true; }
#endif

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFirebombCanonical,"Memoria.Firebomb.Canonical",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFirebombCanonical::RunTest(const FString&)
{ if(!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice")))return false; FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FMaletReplay(this,true,TEXT("FirebombCanonical")))); ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());return true; }
#endif

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFirebombFirebombSignalReplacement,"Memoria.Firebomb.FirebombSignalReplacement",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFirebombFirebombSignalReplacement::RunTest(const FString&)
{ if(!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice")))return false; FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FMaletReplay(this,true,TEXT("FirebombSignalFirebombOnRunReplace")))); ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());return true; }
#endif

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFirebombReplacementAtStop,"Memoria.Firebomb.ReplacementAtStop",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFirebombReplacementAtStop::RunTest(const FString&)
{ if(!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice")))return false; FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FMaletReplay(this,true,TEXT("FirebombAfterStopOnRunReplace")))); ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());return true; }
#endif

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFirebombWorldTeardownAtStop,"Memoria.Firebomb.WorldTeardownAtStop",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFirebombWorldTeardownAtStop::RunTest(const FString&)
{ if(!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice")))return false; FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FMaletReplay(this,true,TEXT("FirebombAfterStopOnWorldTeardown")))); ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());return true; }
#endif

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShopTransactionsCanonical,"Memoria.ShopTransactions.Canonical",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FShopTransactionsCanonical::RunTest(const FString&)
{ if(!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice")))return false; FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FMaletReplay(this,true,TEXT("ShopTransactions")))); ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());return true; }
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShopCanonical,"Memoria.Shop.Canonical",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FShopCanonical::RunTest(const FString&)
{ if(!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice")))return false; FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FMaletReplay(this,true,TEXT("ShopCanonical")))); ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());return true; }

#endif

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCheckpointCanonical,"Memoria.Checkpoint.Canonical",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCheckpointCanonical::RunTest(const FString&)
{ if(!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice")))return false; FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FMaletReplay(this,true,TEXT("ShopCheckpoint")))); ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());return true; }
#endif

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArchiveRenderedInputFlow,"Memoria.Archive.RenderedInputFlow",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FArchiveRenderedInputFlow::RunTest(const FString&)
{ if(!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice")))return false; FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FMaletReplay(this,true,TEXT("ShopArchive")))); ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());return true; }
#endif
