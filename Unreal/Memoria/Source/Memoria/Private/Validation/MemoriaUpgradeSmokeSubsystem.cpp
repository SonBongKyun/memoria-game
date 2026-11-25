#include "Validation/MemoriaUpgradeSmokeSubsystem.h"
#include "Templates/Atomic.h"
#include "Combat/MemoriaCombatHudWidget.h"
#include "Combat/MemoriaFieldHudWidget.h"
#include "Achievements/MemoriaAchievementSubsystem.h"
#include "Achievements/MemoriaAchievementWidgets.h"
#include "Presentation/MemoriaFieldAnimInstance.h"
#include "Presentation/MemoriaCombatClips.h"
#include "Animation/AnimSequence.h"
#include "UObject/UObjectIterator.h"
#include "Blueprint/GameViewportSubsystem.h"
#include "Slate/SGameLayerManager.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SConstraintCanvas.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Engine/SkeletalMesh.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Presentation/MemoriaNativePropFingerprint.h"
#include "Presentation/MemoriaAcceptedPropsS362.h"
#include "Engine/Texture.h"
#include "EngineUtils.h"
#include "Framework/MemoriaSliceHost.h"
#include "Framework/MemoriaFieldPawn.h"
#include "Interaction/MemoriaInteractionComponent.h"
#include "Interaction/MemoriaMaletActor.h"
#include "Framework/MemoriaCoordinates.h"
#include "Narrative/MemoriaNarrativeSubsystem.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Combat/MemoriaFieldCombatSubsystem.h"
#include "Combat/MemoriaFieldMonster.h"
#include "Domain/MemoriaPlayerMemoryDomain.h"
#include "Save/MemoriaCheckpointSubsystem.h"
#include "Save/MemoriaSaveSlotsWidget.h"
#include "Settings/MemoriaSettingsSubsystem.h"
#include "Presentation/MemoriaFieldCharacterComponent.h"
#include "Presentation/MemoriaHeroFinish.h"
#include "Presentation/MemoriaVerdanPresentation.h"
#include "Presentation/MemoriaPauseWidget.h"
#include "Presentation/MemoriaTitleWidget.h"
#include "Presentation/MemoriaDevelopmentNarrativeWidget.h"
#include "Framework/Application/SlateApplication.h"
#include "Chapter/MemoriaChapterMap.h"
#include "Chapter/MemoriaChapterPresentation.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "InputKeyEventArgs.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformProperties.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/EngineVersion.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/PackageName.h"
#include "Misc/SecureHash.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "UnrealClient.h"
#if UE_BUILD_DEVELOPMENT && WITH_DEV_AUTOMATION_TESTS
#include "DynamicRHI.h"
#include "RHIStats.h"
#include "RenderTimer.h"
#include "RHICommandList.h"
#include "RenderingThread.h"
#include "Templates/RefCounting.h"
#include "Presentation/MemoriaPresentationMetrics.h"
#include "Misc/ScopeExit.h"
#include "HAL/IConsoleManager.h"
#include "ProfilingDebugging/CsvProfiler.h"
#include <atomic>
#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
THIRD_PARTY_INCLUDES_START
#include <d3d11.h>
#include <dxgi1_4.h>
THIRD_PARTY_INCLUDES_END
#include "Windows/HideWindowsPlatformTypes.h"
#endif
#if CSV_PROFILER
CSV_DEFINE_CATEGORY(MemoriaS362Smoke, false);
#endif
#endif

namespace
{
#if UE_BUILD_DEVELOPMENT && WITH_DEV_AUTOMATION_TESTS
TAtomic<bool> bIsolatedSmokeActive{false};
#endif
bool Requested() { return FParse::Param(FCommandLine::Get(), TEXT("MemoriaUpgradeSmoke")); }
bool GraphicsS363Requested() { return FParse::Param(FCommandLine::Get(), TEXT("MemoriaGraphicsS363Smoke")); }
bool GameplayS364Requested() { return FParse::Param(FCommandLine::Get(), TEXT("MemoriaS364Smoke")); }
bool SafeLeaf(const FString& Leaf)
{
    if (Leaf.IsEmpty() || Leaf.Len() > 80) return false;
    for (TCHAR C : Leaf) if (!FChar::IsAlnum(C) && C != TCHAR('-')) return false;
    return true;
}
bool IsolatedUserDir(FString& Out)
{
    Out.Reset();
    // Parse the same public command-line value that UE uses internally; its FPaths accessor is private.
    if (!FParse::Value(FCommandLine::Get(), TEXT("UserDir="), Out) || Out.IsEmpty() || FPaths::IsRelative(Out)) return false;
    Out = FPaths::ConvertRelativePathToFull(Out); FPaths::NormalizeDirectoryName(Out);
    if (!FPaths::CollapseRelativeDirectories(Out)) return false;
    const FString Marker = TEXT("/Saved/Validation/UpgradeSmokeRuntime/");
    const int32 At = Out.Find(Marker, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
    return Out.Len() > 3 && Out[1] == TCHAR(':') && Out[2] == TCHAR('/') && At != INDEX_NONE && SafeLeaf(Out.Mid(At + Marker.Len()));
}
bool EffectiveSavedDirIsolated(const FString& UserDir)
{
    FString SavedDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir());
    FPaths::NormalizeDirectoryName(SavedDir);
    return FPaths::CollapseRelativeDirectories(SavedDir) && SavedDir.StartsWith(UserDir + TEXT("/"), ESearchCase::IgnoreCase);
}
bool WriteJson(const FString& Path, const TSharedRef<FJsonObject>& Object)
{
    FString Text;
    return IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path), true) &&
        FJsonSerializer::Serialize(Object, TJsonWriterFactory<>::Create(&Text)) &&
        FFileHelper::SaveStringToFile(Text, *Path, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}
void BootstrapRefused(const TCHAR* Why)
{
    // Only a verified isolated -UserDir may receive even a refused-start evidence file.
    FString UserDir;
    if (IsolatedUserDir(UserDir))
    {
        auto Object = MakeShared<FJsonObject>();
        Object->SetStringField(TEXT("status"), TEXT("UNAVAILABLE")); Object->SetStringField(TEXT("reason"), Why);
        Object->SetNumberField(TEXT("exit_code"), 77);
        // Do not trust ProjectSavedDir here: a refusal may be precisely because it ignored UserDir.
        WriteJson(UserDir / TEXT("Saved/Validation/UpgradeSmoke/bootstrap_refused.json"), Object);
    }
    UE_LOG(LogTemp, Error, TEXT("MEMORIA_UPGRADE_SMOKE_UNAVAILABLE %s"), Why);
    FPlatformMisc::RequestExitWithStatus(true, 77);
}
FString Read(const FString& Path) { FString Text; FFileHelper::LoadFileToString(Text, *Path); return Text; }
FString Hash(const TArray<uint8>& Bytes)
{
    FSHAHash Digest; FSHA1::HashBuffer(Bytes.GetData(), Bytes.Num(), Digest.Hash); return Digest.ToString();
}
void Tap(AMemoriaSliceController* PC, const FKey& Key)
{
    PC->InputKey(FInputKeyEventArgs::CreateSimulated(Key, IE_Pressed, 1.f));
    PC->InputKey(FInputKeyEventArgs::CreateSimulated(Key, IE_Released, 0.f));
}
void Down(AMemoriaSliceController* PC, int32 Count) { for (int32 I = 0; I < Count; ++I) Tap(PC, EKeys::Gamepad_DPad_Down); }
void SlateTap(const FKey& Key)
{
    FSlateApplication::Get().ProcessKeyDownEvent(FKeyEvent(Key, FModifierKeysState(), 0, false, 0, 0));
    FSlateApplication::Get().ProcessKeyUpEvent(FKeyEvent(Key, FModifierKeysState(), 0, false, 0, 0));
}
#if UE_BUILD_DEVELOPMENT && WITH_DEV_AUTOMATION_TESTS
template<class T> T* GraphicsWidget(AMemoriaSliceController* PC)
{
    for(TObjectIterator<T> It;It;++It) if(It->GetOwningPlayer()==PC && It->IsInViewport()) return *It;
    return nullptr;
}
struct FS363OverlayHost { const SWidget* Overlay=nullptr; const SWidget* Host=nullptr; int32 Index=INDEX_NONE,Z=0; };
bool GraphicsOuterHost(UGameViewportClient* Client,const UWidget* Widget,FS363OverlayHost& Out)
{
    Out={};if(!Client || !Widget)return false;
    auto Manager=Client->GetGameLayerManager();TSharedPtr<SWidget> Node=Widget->GetCachedWidget();
    if(!Manager || !Node)return false;TSharedPtr<SWidget> Root=Manager->AsWidget();
    for(int32 Depth=0;Node && Depth<64;++Depth)
    {
        if(Node==Root)return Out.Overlay!=nullptr;
        auto Parent=Node->GetParentWidget();if(!Parent)break;
        if(!Out.Overlay && Parent->GetWidgetClass().GetWidgetType()==SOverlay::StaticWidgetClass().GetWidgetType()
            && Node->GetWidgetClass().GetWidgetType()==SConstraintCanvas::StaticWidgetClass().GetWidgetType())
        {
            FChildren* Children=Parent->GetChildren();
            for(int32 I=0;I<Children->Num();++I)if(Children->GetChildAt(I)==Node)
            {Out.Overlay=Parent.Get();Out.Host=Node.Get();Out.Index=I;Out.Z=static_cast<const SOverlay::FOverlaySlot&>(Children->GetSlotAt(I)).GetZOrder();break;}
        }
        Node=Parent;
    }
    Out={};return false;
}
#endif
bool ArrivalDone(const UMemoriaNarrativeSubsystem& Host, const UMemoriaRunSubsystem& Run, const FString& Map)
{
    if (Host.GetChapterMap() != Map || Host.GetState() != EMemoriaSliceState::Exploration) return false;
    const auto* Spec = MemoriaChapterMaps::Find(Map); if (!Spec) return false;
    for (const auto& Link : Spec->Sequence) if (!Link.Flag.IsEmpty() && !Run.GetRunSnapshot().GetFlag(Link.Flag)) return false;
    return true;
}

#if UE_BUILD_DEVELOPMENT && WITH_DEV_AUTOMATION_TESTS
// Published once after an asynchronous render-thread query. No UObject/world reference crosses threads.
struct FSmokeProcessVram
{
    std::atomic<bool> bComplete{false};
    bool bAvailable = false;
    uint64 CurrentUsage = 0, Budget = 0;
    double AtSeconds = 0.;
    FString Reason;
    TSharedRef<FJsonObject> Json() const
    {
        auto Item = MakeShared<FJsonObject>();
        if (!bComplete.load(std::memory_order_acquire))
        { Item->SetStringField(TEXT("status"), TEXT("pending")); return Item; }
        Item->SetStringField(TEXT("status"), bAvailable ? TEXT("measured") : TEXT("unavailable"));
        Item->SetNumberField(TEXT("sample_elapsed_seconds"), AtSeconds);
        Item->SetStringField(TEXT("scope"), TEXT("DXGI process LOCAL current usage/budget on the native D3D11 device adapter, node0; not physical-GPU total usage or texture-only allocation."));
        Item->SetNumberField(TEXT("node_index"), 0);
        if (bAvailable)
        {
            Item->SetNumberField(TEXT("current_usage_mib"), double(CurrentUsage) / (1024. * 1024.));
            Item->SetNumberField(TEXT("budget_mib"), double(Budget) / (1024. * 1024.));
        }
        else Item->SetStringField(TEXT("reason"), Reason);
        return Item;
    }
};
void CollectProcessVram(FSmokeProcessVram& Sample, double Started)
{
    // RHIGetNativeDevice internally flushes the RHI thread and must run on the render thread.
    // Its one-shot synchronization cost remains part of the observed smoke, not a new phase wait.
    ON_SCOPE_EXIT
    {
        Sample.AtSeconds = FPlatformTime::Seconds() - Started;
        Sample.bComplete.store(true, std::memory_order_release);
    };
#if PLATFORM_WINDOWS
    if (!GDynamicRHI || RHIGetInterfaceType() != ERHIInterfaceType::D3D11)
    { Sample.Reason = TEXT("Process LOCAL budget query supports only the Windows native D3D11 backend in this harness."); return; }
    auto* Device = static_cast<ID3D11Device*>(RHIGetNativeDevice()); // borrowed; do not Release
    if (!Device) { Sample.Reason = TEXT("Native D3D11 device is unavailable."); return; }
    TRefCountPtr<IDXGIDevice> DxgiDevice;
    TRefCountPtr<IDXGIAdapter> Adapter;
    TRefCountPtr<IDXGIAdapter3> Adapter3;
    HRESULT Result = Device->QueryInterface(IID_PPV_ARGS(DxgiDevice.GetInitReference()));
    if (FAILED(Result) || !DxgiDevice) { Sample.Reason = FString::Printf(TEXT("IDXGIDevice query unavailable (0x%08x)."), uint32(Result)); return; }
    Result = DxgiDevice->GetAdapter(Adapter.GetInitReference());
    if (FAILED(Result) || !Adapter) { Sample.Reason = FString::Printf(TEXT("Native device adapter unavailable (0x%08x)."), uint32(Result)); return; }
    Result = Adapter->QueryInterface(IID_PPV_ARGS(Adapter3.GetInitReference()));
    if (FAILED(Result) || !Adapter3) { Sample.Reason = FString::Printf(TEXT("IDXGIAdapter3 query unavailable (0x%08x)."), uint32(Result)); return; }
    DXGI_QUERY_VIDEO_MEMORY_INFO Memory{};
    Result = Adapter3->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &Memory);
    if (FAILED(Result)) { Sample.Reason = FString::Printf(TEXT("Process LOCAL video-memory query failed (0x%08x)."), uint32(Result)); return; }
    if (Memory.CurrentUsage == 0 || Memory.Budget == 0)
    { Sample.Reason = TEXT("DXGI did not report positive process LOCAL usage and budget; zero is not accepted as a measured rendering footprint."); return; }
    Sample.CurrentUsage = Memory.CurrentUsage; Sample.Budget = Memory.Budget; Sample.bAvailable = true;
#else
    Sample.Reason = TEXT("Windows DXGI process LOCAL memory query is unavailable on this platform.");
#endif
}
// Online aggregates have constant storage. No per-frame JSON, trace stream or GPU timing query.
struct FSmokePerfMetric
{
    uint64 Count = 0;
    double Sum = 0., Minimum = 0., Maximum = 0.;
    void Add(double Value)
    {
        if (!FMath::IsFinite(Value) || Value < 0.) return;
        if (Count == 0) Minimum = Maximum = Value;
        else { Minimum = FMath::Min(Minimum, Value); Maximum = FMath::Max(Maximum, Value); }
        ++Count; Sum += Value;
    }
    TSharedRef<FJsonObject> Json(const TCHAR* Unit, bool bRequirePositive = false) const
    {
        auto Item = MakeShared<FJsonObject>();
        const bool bAvailable = Count > 0 && (!bRequirePositive || Maximum > 0.);
        Item->SetStringField(TEXT("status"), bAvailable ? TEXT("measured") : TEXT("unavailable"));
        Item->SetStringField(TEXT("unit"), Unit); Item->SetNumberField(TEXT("samples"), double(Count));
        if (bAvailable)
        {
            Item->SetNumberField(TEXT("min"), Minimum); Item->SetNumberField(TEXT("mean"), Sum / double(Count));
            Item->SetNumberField(TEXT("max"), Maximum);
        }
        else Item->SetStringField(TEXT("reason"), Count == 0 ? TEXT("No valid observations in this window.") : TEXT("The published counter stayed zero; support or activity cannot be established."));
        return Item;
    }
};
struct FSmokePerfFrame
{
    double WallMs = 0., GameMs = 0., RenderMs = 0., RhiMs = 0.;
    int32 Draws = 0, Primitives = 0;
    uint64 StreamingKB = 0, NonStreamingKB = 0;
    int64 PoolBytes = 0;
    bool bRhi = false, bCapturePending = false, bPaused = false;
    MemoriaPresentationMetrics::FSnapshot Cpu;
};
struct FSmokePerfWindow
{
    FString Map, Activity;
    int32 Phase = INDEX_NONE, WorldInstance = 0;
    uint64 Frames = 0, CaptureFrames = 0, PausedFrames = 0, Over16 = 0, Over33 = 0, Over50 = 0, Over100 = 0;
    double FirstAt = 0., LastAt = 0.;
    FSmokePerfMetric Wall, Game, Render, Rhi, Draws, Primitives, Streaming, NonStreaming, TextureTotal, Pool;
    FSmokePerfMetric AnimationMs, ComponentMs;
    uint64 AnimationCalls = 0, ComponentCalls = 0;
    TSharedPtr<FSmokeProcessVram, ESPMode::ThreadSafe> ProcessVram;
    void Add(const FSmokePerfFrame& Frame, double At)
    {
        if (Frames == 0) FirstAt = At;
        LastAt = At; ++Frames;
        CaptureFrames += Frame.bCapturePending ? 1 : 0; PausedFrames += Frame.bPaused ? 1 : 0;
        Over16 += Frame.WallMs > (1000. / 60.) ? 1 : 0; Over33 += Frame.WallMs > (1000. / 30.) ? 1 : 0;
        Over50 += Frame.WallMs > 50. ? 1 : 0; Over100 += Frame.WallMs > 100. ? 1 : 0;
        Wall.Add(Frame.WallMs); Game.Add(Frame.GameMs); Render.Add(Frame.RenderMs); Rhi.Add(Frame.RhiMs);
        AnimationMs.Add(FPlatformTime::ToMilliseconds64(Frame.Cpu.AnimationEvaluate.Cycles)); AnimationCalls += Frame.Cpu.AnimationEvaluate.Calls;
        ComponentMs.Add(FPlatformTime::ToMilliseconds64(Frame.Cpu.CharacterUpdate.Cycles)); ComponentCalls += Frame.Cpu.CharacterUpdate.Calls;
        if (Frame.bRhi)
        {
            Draws.Add(Frame.Draws); Primitives.Add(Frame.Primitives);
            // Zero streaming allocations are valid when non-streaming accounting is populated.
            // If both counters stay zero, do not present unsupported accounting as zero VRAM usage.
            if (Frame.StreamingKB > 0 || Frame.NonStreamingKB > 0)
            {
                Streaming.Add(double(Frame.StreamingKB) / 1024.); NonStreaming.Add(double(Frame.NonStreamingKB) / 1024.);
                TextureTotal.Add((double(Frame.StreamingKB) + double(Frame.NonStreamingKB)) / 1024.);
                Pool.Add(double(Frame.PoolBytes) / (1024. * 1024.));
            }
        }
    }
    TSharedRef<FJsonObject> Json() const
    {
        auto Item = MakeShared<FJsonObject>();
        Item->SetStringField(TEXT("map"), Map); Item->SetStringField(TEXT("activity"), Activity);
        Item->SetNumberField(TEXT("phase"), Phase); Item->SetNumberField(TEXT("world_instance"), WorldInstance);
        Item->SetStringField(TEXT("status"), Frames > 0 ? TEXT("measured") : TEXT("unavailable"));
        Item->SetNumberField(TEXT("frames"), double(Frames)); Item->SetNumberField(TEXT("observed_interval_seconds"), Wall.Sum / 1000.);
        Item->SetNumberField(TEXT("first_sample_elapsed_seconds"), FirstAt); Item->SetNumberField(TEXT("last_sample_elapsed_seconds"), LastAt);
        Item->SetNumberField(TEXT("capture_pending_frames"), double(CaptureFrames)); Item->SetNumberField(TEXT("paused_frames"), double(PausedFrames));
        Item->SetNumberField(TEXT("frame_intervals_over_16_67_ms"), double(Over16)); Item->SetNumberField(TEXT("frame_intervals_over_33_33_ms"), double(Over33));
        Item->SetNumberField(TEXT("frame_intervals_over_50_ms"), double(Over50)); Item->SetNumberField(TEXT("frame_intervals_over_100_ms"), double(Over100));
        if (Wall.Sum > 0.) Item->SetNumberField(TEXT("observed_interval_fps"), double(Wall.Count) * 1000. / Wall.Sum);
        Item->SetObjectField(TEXT("wall_frame_interval_ms"), Wall.Json(TEXT("ms")));
        Item->SetObjectField(TEXT("game_thread_cpu_ms"), Game.Json(TEXT("ms"), true));
        Item->SetObjectField(TEXT("render_thread_cpu_ms"), Render.Json(TEXT("ms"), true));
        Item->SetObjectField(TEXT("rhi_thread_cpu_ms"), Rhi.Json(TEXT("ms"), true));
        Item->SetObjectField(TEXT("rhi_draw_calls_gpu0"), Draws.Json(TEXT("draw_calls"), true));
        Item->SetObjectField(TEXT("rhi_primitives_gpu0"), Primitives.Json(TEXT("primitives"), true));
        Item->SetObjectField(TEXT("rhi_streaming_texture_mib"), Streaming.Json(TEXT("MiB")));
        Item->SetObjectField(TEXT("rhi_non_streaming_texture_mib"), NonStreaming.Json(TEXT("MiB")));
        Item->SetObjectField(TEXT("rhi_texture_total_mib"), TextureTotal.Json(TEXT("MiB")));
        Item->SetObjectField(TEXT("rhi_texture_pool_limit_mib"), Pool.Json(TEXT("MiB")));
        Item->SetObjectField(TEXT("animation_evaluate_scoped_ms_per_observed_frame"), AnimationMs.Json(TEXT("ms"), true));
        Item->SetObjectField(TEXT("character_update_scoped_ms_per_observed_frame"), ComponentMs.Json(TEXT("ms"), true));
        Item->SetNumberField(TEXT("animation_evaluate_completed_calls"), double(AnimationCalls));
        Item->SetNumberField(TEXT("character_update_completed_calls"), double(ComponentCalls));
        Item->SetNumberField(TEXT("animation_evaluate_total_scoped_ms"), AnimationMs.Sum);
        Item->SetNumberField(TEXT("character_update_total_scoped_ms"), ComponentMs.Sum);
        if (AnimationCalls > 0) Item->SetNumberField(TEXT("animation_evaluate_ms_per_completed_call"), AnimationMs.Sum / double(AnimationCalls));
        if (ComponentCalls > 0) Item->SetNumberField(TEXT("character_update_ms_per_completed_call"), ComponentMs.Sum / double(ComponentCalls));
        if (Phase == INDEX_NONE)
        {
            if (ProcessVram) Item->SetObjectField(TEXT("dxgi_process_local_memory"), ProcessVram->Json());
            else
            {
                auto Missing = MakeShared<FJsonObject>(); Missing->SetStringField(TEXT("status"), TEXT("unavailable"));
                Missing->SetStringField(TEXT("reason"), TEXT("This region has not reached its existing live inspection; no memory query was requested."));
                Item->SetObjectField(TEXT("dxgi_process_local_memory"), Missing);
            }
        }
        return Item;
    }
};
struct FSmokePerformance
{
    static constexpr int32 MaxWindows = 64;
    TArray<FSmokePerfWindow> Regions, Windows;
    TWeakObjectPtr<UWorld> CachedWorld, PreviousWorld;
    FString CachedMap, PreviousMap, PreviousActivity;
    int32 PreviousPhase = INDEX_NONE, WorldInstance = 0;
    double PreviousAt = 0.;
    uint64 BoundaryIntervalsOmitted = 0, UnmappedFrames = 0, WindowLimitFrames = 0;
    MemoriaPresentationMetrics::FSnapshot PreviousCpu;
    FSmokePerformance()
    {
        for (const TCHAR* Map : {TEXT("verdan_market"), TEXT("belt_waystation"), TEXT("drift_shelter")})
        {
            auto& Region = Regions.AddDefaulted_GetRef(); Region.Map = Map; Region.Activity = TEXT("all_observed_runtime_activities");
        }
        Windows.Reserve(MaxWindows);
    }
    void RequestProcessVram(const FString& Map, double Started)
    {
        auto* Region = Regions.FindByPredicate([&](const auto& Item) { return Item.Map == Map; });
        if (!Region || Region->ProcessVram) return;
        const auto Request = MakeShared<FSmokeProcessVram, ESPMode::ThreadSafe>(); Region->ProcessVram = Request;
        ENQUEUE_RENDER_COMMAND(MemoriaUpgradeProcessVram)([Request, Started](FRHICommandListImmediate&)
        { CollectProcessVram(*Request, Started); });
    }
    void Observe(UWorld* World, int32 Phase, bool bCapturePending, double Now, double Started)
    {
        const auto CurrentCpu = MemoriaPresentationMetrics::Capture();
        const auto CpuDelta = MemoriaPresentationMetrics::Delta(CurrentCpu, PreviousCpu); PreviousCpu = CurrentCpu;
        if (CachedWorld.Get() != World)
        {
            CachedWorld = World; CachedMap.Reset(); ++WorldInstance;
            const FString Name = World ? World->GetMapName() : FString();
            if (Name.EndsWith(TEXT("L_VerdanHost"))) CachedMap = TEXT("verdan_market");
            else if (Name.EndsWith(TEXT("L_BeltWaystation"))) CachedMap = TEXT("belt_waystation");
            else if (Name.EndsWith(TEXT("L_DriftShelter"))) CachedMap = TEXT("drift_shelter");
        }
#if CSV_PROFILER_STATS
        if (FCsvProfiler::IsCapturing())
        {
            const int32 Region = CachedMap == TEXT("verdan_market") ? 1 : CachedMap == TEXT("belt_waystation") ? 2 : CachedMap == TEXT("drift_shelter") ? 3 : 0;
            const auto* CsvCombat = World && !World->bIsTearingDown ? World->GetSubsystem<UMemoriaFieldCombatSubsystem>() : nullptr;
            const int32 ActualCombat = CsvCombat && (CsvCombat->IsAttacking() || CsvCombat->IsCasting() || CsvCombat->GetBurnWave().bLive || CsvCombat->LiveMonsterCount() > 0) ? 1 : 0;
            CSV_CUSTOM_STAT(MemoriaS362Smoke, Region, Region, ECsvCustomStatOp::Set);
            CSV_CUSTOM_STAT(MemoriaS362Smoke, Phase, Phase, ECsvCustomStatOp::Set);
            CSV_CUSTOM_STAT(MemoriaS362Smoke, CapturePending, bCapturePending ? 1 : 0, ECsvCustomStatOp::Set);
            CSV_CUSTOM_STAT(MemoriaS362Smoke, ActualCombat, ActualCombat, ECsvCustomStatOp::Set);
        }
#endif
        if (!World || World->bIsTearingDown || World->GetTimeSeconds() < .15 || CachedMap.IsEmpty())
        { PreviousAt = 0.; ++UnmappedFrames; return; }
        auto* PC = Cast<AMemoriaSliceController>(World->GetFirstPlayerController());
        const auto* Host = World->GetGameInstance()->GetSubsystem<UMemoriaNarrativeSubsystem>();
        const auto* Combat = World->GetSubsystem<UMemoriaFieldCombatSubsystem>();
        FString Activity = TEXT("narrative_or_transition_runtime");
        if (PC && PC->GetSaveSlotsWidget()) Activity = TEXT("save_load_ui_runtime");
        else if (PC && PC->GetPauseWidget()) Activity = TEXT("pause_ui_runtime");
        else if (Combat && (Combat->IsAttacking() || Combat->IsCasting() || Combat->GetBurnWave().bLive || Combat->LiveMonsterCount() > 0)) Activity = TEXT("combat_runtime");
        else if (Host && Host->GetState() == EMemoriaSliceState::Exploration) Activity = TEXT("field_rendering_runtime");
        else if (Host && Host->GetState() == EMemoriaSliceState::Deferred) Activity = TEXT("deferred_field_runtime");
        const double Interval = Now - PreviousAt;
        const bool bSameContext = PreviousAt > 0. && PreviousWorld.Get() == World && PreviousMap == CachedMap && PreviousPhase == Phase && PreviousActivity == Activity;
        PreviousAt = Now; PreviousWorld = World; PreviousMap = CachedMap; PreviousPhase = Phase; PreviousActivity = Activity;
        // Avoid attributing travel/phase/activity boundary intervals to the destination window.
        if (!bSameContext || !FMath::IsFinite(Interval) || Interval <= 0.) { ++BoundaryIntervalsOmitted; return; }
        FSmokePerfFrame Frame;
        Frame.WallMs = Interval * 1000.; Frame.Cpu = CpuDelta;
        // As in the engine frame chart, copy each published counter once; RT/RHI can lag GT.
        const uint32 GameCycles = GGameThreadTime, RenderCycles = GRenderThreadTime, RhiCycles = GRHIThreadTime;
        Frame.GameMs = FPlatformTime::ToMilliseconds(GameCycles); Frame.RenderMs = FPlatformTime::ToMilliseconds(RenderCycles); Frame.RhiMs = FPlatformTime::ToMilliseconds(RhiCycles);
        Frame.bRhi = GDynamicRHI && !GUsingNullRHI; Frame.bCapturePending = bCapturePending; Frame.bPaused = World->IsPaused();
        if (Frame.bRhi)
        {
            Frame.Draws = GNumDrawCallsRHI[0]; Frame.Primitives = GNumPrimitivesDrawnRHI[0];
            // Same baseline accounting as RHIGetTextureMemoryStats, without driver-budget polling.
            Frame.StreamingKB = GRHIGlobals.StreamingTextureMemorySizeInKB; Frame.NonStreamingKB = GRHIGlobals.NonStreamingTextureMemorySizeInKB;
            Frame.PoolBytes = GRHIGlobals.TexturePoolSize;
        }
        if (auto* Region = Regions.FindByPredicate([&](const auto& Item) { return Item.Map == CachedMap; })) Region->Add(Frame, Now - Started);
        auto* Window = Windows.FindByPredicate([&](const auto& Item) { return Item.Map == CachedMap && Item.Phase == Phase && Item.Activity == Activity && Item.WorldInstance == WorldInstance; });
        if (!Window && Windows.Num() < MaxWindows)
        {
            Window = &Windows.AddDefaulted_GetRef(); Window->Map = CachedMap; Window->Phase = Phase;
            Window->Activity = Activity; Window->WorldInstance = WorldInstance;
        }
        if (Window) Window->Add(Frame, Now - Started); else ++WindowLimitFrames;
    }
    TSharedRef<FJsonObject> Json() const
    {
        auto Item = MakeShared<FJsonObject>();
        Item->SetNumberField(TEXT("schema_version"), 1); Item->SetStringField(TEXT("scope"), TEXT("Cooked Development process runtime, not editor; bounded smoke observations, not a steady-state benchmark."));
        Item->SetStringField(TEXT("sampling"), TEXT("One observation per game-frame tick before the existing 100ms walkthrough cadence; online min/mean/max only, no retained frame stream or added waits."));
        Item->SetStringField(TEXT("wall_source"), TEXT("FPlatformTime::Seconds between matching world/phase/activity ticks; includes frame caps, waits, streaming, native screenshot/file work and validation overhead."));
        Item->SetStringField(TEXT("cpu_source"), TEXT("RenderTimer GGameThreadTime/GRenderThreadTime/GRHIThreadTime converted from cycles; published prior-frame counters excluding idle, not process utilization or per-function cost."));
        Item->SetStringField(TEXT("render_counter_source"), TEXT("Published GNumDrawCallsRHI[0]/GNumPrimitivesDrawnRHI[0], GPU0 whole-frame counts; may lag the game frame and include UI/shadow/other passes. Primitives are not unique visible triangles."));
        Item->SetStringField(TEXT("texture_source"), TEXT("GRHIGlobals streaming/non-streaming allocation KB, the RHI baseline counters; total is their sum, not resident VRAM, all GPU allocations or asset-only memory. Pool zero means no pool limit."));
        Item->SetStringField(TEXT("activity_scope"), TEXT("All windows measure the full process frame. field_rendering_runtime has no observed live foe/action or modal; combat_runtime includes actual encounter work; UI/narrative/deferred activity is separate. None isolates rendering-only CPU cost."));
        Item->SetStringField(TEXT("comparability_limit"), TEXT("Gameplay/UI/save phases are unchanged; phase17 requests engine default blocking CSV finalization, then polls its future/artifact, and is excluded from gameplay comparisons. The native poll does not bound the engine end-frame wait; the external process deadline remains required. No warmup, fixed-duration benchmark window, uncapped FPS guarantee, editor comparison, before/after baseline, human playthrough or Shipping performance claim."));
        Item->SetStringField(TEXT("frame_alignment_limit"), TEXT("Drop the first interval after each world/phase/activity change. Published CPU/RHI data can still refer to earlier render pipeline work; these are approximate attribution windows, not synchronized trace events."));
        Item->SetBoolField(TEXT("performance_thresholds_affect_smoke_pass"), false);
        Item->SetNumberField(TEXT("phase_window_capacity"), MaxWindows); Item->SetNumberField(TEXT("phase_window_limit_frames"), double(WindowLimitFrames));
        Item->SetNumberField(TEXT("boundary_intervals_omitted"), double(BoundaryIntervalsOmitted)); Item->SetNumberField(TEXT("unmapped_or_early_world_frames"), double(UnmappedFrames));
        auto Gpu = MakeShared<FJsonObject>(); Gpu->SetStringField(TEXT("status"), TEXT("unavailable"));
        Gpu->SetStringField(TEXT("reason"), TEXT("This inline summary does not collect hardware GPU timestamps. The separately finalized engine_csv may provide valid positive GPUTime; inspect its raw samples. Wall/render/RHI thread times are not GPU execution time."));
        Item->SetObjectField(TEXT("gpu_frame_ms"), Gpu);
        uint64 EvaluationCalls = 0, UpdateCalls = 0; double EvaluationMs = 0., UpdateMs = 0.;
        for (const auto& Region : Regions)
        { EvaluationCalls += Region.AnimationCalls; UpdateCalls += Region.ComponentCalls; EvaluationMs += Region.AnimationMs.Sum; UpdateMs += Region.ComponentMs.Sum; }
        auto Animation = MakeShared<FJsonObject>();
        Animation->SetStringField(TEXT("status"), EvaluationCalls > 0 && EvaluationMs > 0. ? TEXT("measured_scoped_cpu_elapsed") : TEXT("unavailable"));
        Animation->SetStringField(TEXT("source"), TEXT("Opt-in Cycles64 RAII scopes around all FFieldAnimProxy::Evaluate and UMemoriaFieldCharacterComponent::TickComponent calls; atomic cumulative cycles/call deltas, no UObject/world reads in the collector or animation worker scope."));
        Animation->SetStringField(TEXT("limits"), TEXT("Aggregates all field figures, not only Arrel. Elapsed CPU work spans may include preemption and overlap parallel workers; their sum is not exclusive game-thread or process CPU time. Evaluate includes pose evaluation/personality/root lock; component scope includes its update/ApplyFrame. Excludes separate animation PreUpdate/CacheBones/UpdateAnimationNode, engine bone refresh, physics/cloth and GPU skinning. Scope completions and independently atomic cycle/call snapshots may straddle frame/phase boundaries; per-call means are approximate."));
        Animation->SetBoolField(TEXT("collector_enabled"), MemoriaPresentationMetrics::IsEnabled());
        Animation->SetNumberField(TEXT("observed_evaluation_calls"), double(EvaluationCalls)); Animation->SetNumberField(TEXT("observed_component_update_calls"), double(UpdateCalls));
        if (EvaluationCalls > 0 && EvaluationMs > 0.) Animation->SetNumberField(TEXT("observed_evaluation_scoped_ms"), EvaluationMs);
        if (UpdateCalls > 0 && UpdateMs > 0.) Animation->SetNumberField(TEXT("observed_component_update_scoped_ms"), UpdateMs);
        Item->SetObjectField(TEXT("cpu_animation_ms"), Animation);
        Item->SetStringField(TEXT("dxgi_process_memory_sampling"), TEXT("One asynchronous render-thread query per canonical region after its existing live inspection, including only Windows D3D11 device-adapter node0 LOCAL usage/budget. RHIGetNativeDevice internally synchronizes the RHI thread once; no game-thread wait, repeated budget polling, extra phase or timing gate. Snapshot is not peak VRAM and not physical-GPU total usage; unsupported/failure/zero reports are unavailable."));
        TArray<TSharedPtr<FJsonValue>> RegionJson, WindowJson;
        int32 MeasuredRegions = 0;
        for (const auto& Region : Regions) { RegionJson.Add(MakeShared<FJsonValueObject>(Region.Json())); MeasuredRegions += Region.Frames > 0 ? 1 : 0; }
        for (const auto& Window : Windows) WindowJson.Add(MakeShared<FJsonValueObject>(Window.Json()));
        Item->SetStringField(TEXT("status"), MeasuredRegions > 0 ? TEXT("measured_bounded_runtime_samples") : TEXT("unavailable"));
        Item->SetNumberField(TEXT("measured_region_count"), MeasuredRegions); Item->SetArrayField(TEXT("regions"), RegionJson); Item->SetArrayField(TEXT("phase_windows"), WindowJson);
        return Item;
    }
};
#endif
}

bool MemoriaUpgradeSmoke::IsIsolatedSmokeActive()
{
#if UE_BUILD_DEVELOPMENT && WITH_DEV_AUTOMATION_TESTS
    return bIsolatedSmokeActive.Load(EMemoryOrder::Relaxed);
#else
    return false;
#endif
}

bool MemoriaUpgradeSmoke::BeforeGameInstanceInit()
{
#if !UE_BUILD_SHIPPING
    if (Requested())
    {
        FString UserDir;
        if (!IsolatedUserDir(UserDir))
        { BootstrapRefused(TEXT("A fresh absolute -UserDir under Saved/Validation/UpgradeSmokeRuntime/<leaf> is required; no normal user-storage fallback.")); return false; }
        if (!EffectiveSavedDirIsolated(UserDir))
        { BootstrapRefused(TEXT("The effective project Saved directory did not honor isolated -UserDir.")); return false; }
#if UE_BUILD_DEVELOPMENT && WITH_DEV_AUTOMATION_TESTS
        if (GIsEditor || !FPlatformProperties::RequiresCookedData())
        { BootstrapRefused(TEXT("Smoke accepts only a cooked Development game executable, not editor/uncooked execution.")); return false; }
        // Profile subsystem persistence gates are evaluated during Super::Init. This private mode stays
        // armed through teardown without telling AutomationWorker that a framework test is running.
        bIsolatedSmokeActive.Store(true, EMemoryOrder::Relaxed);
        return true;
#else
        BootstrapRefused(TEXT("Smoke requires Development WITH_DEV_AUTOMATION_TESTS; game subsystems were not initialized."));
        return false;
#endif
    }
#endif
    return true;
}

struct FMemoriaUpgradeSmokeState
{
    int32 Phase = 0;
    int32 S364FocusPhase=0,S364CombatPhase=0;
    TWeakObjectPtr<UWorld> S364CaptureWorld;
    bool bS364CapturePauseOwned=false,bS364CaptureWasPaused=false;
    bool bS364FixedStepOwned=false,bS364PreviousFixed=false;
    double S364PreviousFixedDelta=0.;uint64 S364ReplayArmedFrame=0;
    bool bS364FocusDone=false,bS364CombatDone=false,bS364TargetTick=false;
    double S364At=0.,S364BlockAt=0.;uint64 S364Frame=0;
    FVector S364OriginalPosition=FVector::ZeroVector;
    TWeakObjectPtr<AActor> S364FocusTarget;
    FMemoriaRunSnapshot S364FocusBefore;
    int64 S364BlockHp=0,S364BlockLoss=0,S364ParryHp=0;
    bool bS363GuardStarted=false,bS363GuardCaptured=false,bS363GuardHeld=false,bS363GuardDone=false;
    double S363GuardAt=0.;uint64 S363GuardFrame=0;
    double Started = FPlatformTime::Seconds(), PhaseAt = Started, LastStep = 0., PendingAt = 0.;
    uint64 LastFrame = MAX_uint64;
    FString Leaf, Output, PendingPng, BurnId, SavedMap, SavedBytes;
    int32 HitsBefore = 0, BurnedBefore = 0, SavedBurnCount = 0;
    int64 SavedHp = 0, SavedGrains = 0;
    FString VerdanSavedBytes;
    int64 VerdanSavedHp = 0, VerdanSavedGrains = 0;
    int32 VerdanSavedBurnCount = 0;
    FVector2D VerdanSavedPosition = FVector2D::ZeroVector;
    FGuid RunId;
    FVector2D SavedPosition = FVector2D::ZeroVector;
    TWeakObjectPtr<UWorld> TravelFrom;
    TWeakObjectPtr<AMemoriaFieldMonster> Target;
    TSharedRef<FJsonObject> Report = MakeShared<FJsonObject>();
    TArray<TSharedPtr<FJsonValue>> Checks, Regions, Captures;
    bool bDone = false, bTitleVerdanProbeDone = false;
#if UE_BUILD_DEVELOPMENT && WITH_DEV_AUTOMATION_TESTS
    FSmokePerformance Performance;
    TSharedRef<FJsonObject> CsvReport = MakeShared<FJsonObject>();
    FString CsvPath;
    bool bCsvRequested = false, bCsvOwned = false, bCsvEndRequested = false, bCsvFinished = false, bCsvCategoryChanged = false, bCsvCategoryWasEnabled = false;
    double CsvRequestedAt = 0., CsvEndAt = 0.;
    TSharedFuture<FString> CsvCompletion;
    bool SameCsvPath(const FString& Path) const
    {
        if (Path.IsEmpty()) return false;
        FString Actual = FPaths::ConvertRelativePathToFull(Path); FPaths::NormalizeFilename(Actual);
        return FPaths::CollapseRelativeDirectories(Actual) && Actual.Equals(CsvPath, ESearchCase::IgnoreCase);
    }
    bool CsvBlockingEnd() const
    {
        const auto* Mode = IConsoleManager::Get().FindConsoleVariable(TEXT("csv.BlockOnCaptureEnd"));
        return Mode && Mode->GetInt() == 1;
    }
    void CleanupCsv()
    {
#if CSV_PROFILER_STATS
        auto* Profiler = FCsvProfiler::Get();
        // Exact fresh path is the ownership token. Never stop a pre-existing/replacement capture.
        if (bCsvRequested && !bCsvEndRequested && Profiler->IsCapturing() && SameCsvPath(Profiler->GetOutputFilename()))
        {
            if (CsvBlockingEnd())
            {
                CsvCompletion = Profiler->EndCapture(); bCsvEndRequested = true;
                CsvReport->SetBoolField(TEXT("failure_cleanup_stop_requested"), true);
            }
            else CsvReport->SetStringField(TEXT("cleanup_limit"), TEXT("Owned capture left to process teardown because blocking engine completion was not enabled safely; no game-thread wait was added."));
        }
        bCsvOwned = false;
        if (bCsvCategoryChanged && !Profiler->IsCapturing() && !Profiler->IsWritingFile())
        {
            Profiler->EnableCategoryByIndex(CSV_CATEGORY_INDEX(MemoriaS362Smoke), bCsvCategoryWasEnabled); bCsvCategoryChanged = false;
        }
#endif
    }
    bool CsvFailure(const FString& Why)
    {
        CsvReport->SetStringField(TEXT("status"), TEXT("unavailable")); CsvReport->SetStringField(TEXT("reason"), Why);
        Finish(false, Why); return false;
    }
    bool BeginCsv()
    {
        CsvPath = Output / TEXT("EngineCookedSmoke.csv");
        CsvReport->SetStringField(TEXT("expected_path"), CsvPath);
        CsvReport->SetStringField(TEXT("completion_limit"), TEXT("Use engine default blocking finalization with the normal processing thread to avoid the installed UE5.8 nonblocking Stop retry race. The fifteen-second future/artifact poll does not bound the engine end-frame file-write wait; an external600second process deadline is required. Phase17 is excluded from gameplay measurements."));
        if (FParse::Param(FCommandLine::Get(), TEXT("csvNoProcessingThread"))
            || FParse::Param(FCommandLine::Get(), TEXT("onethread")) || !FPlatformProcess::SupportsMultithreading())
            return CsvFailure(TEXT("Smoke CSV requires the normal multithreaded processing backend."));
        CsvReport->SetStringField(TEXT("source"), TEXT("Owned cooked-process engine CSV; raw GPUTime/GPUMem/animation counters are separate from inline manual CPU/RHI/DXGI summaries."));
        CsvReport->SetStringField(TEXT("tag_mapping"), TEXT("MemoriaS362Smoke/Region:0=unmapped,1=Verdan,2=Belt,3=Drift; Phase=harness phase; CapturePending/ActualCombat are 0 or 1. Render/GPU pipeline counters may lag these game-thread tags."));
        CsvReport->SetStringField(TEXT("gpu_timing_status"), TEXT("Not inferred here; inspect finalized raw GPUTime for valid positive samples. Missing/nonpositive GPUTime is unavailable, never substituted with CPU/wall time."));
#if CSV_PROFILER_STATS
        auto* Profiler = FCsvProfiler::Get();
        if (Profiler->IsCapturing() || Profiler->IsWritingFile() || Profiler->IsEndCapturePending())
            return CsvFailure(TEXT("Engine CSV is already owned/busy; smoke will not stop or replace another capture."));
        if (IFileManager::Get().FileExists(*CsvPath)) return CsvFailure(TEXT("Fresh smoke CSV path already exists; refusing replacement."));
        CsvRequestedAt = FPlatformTime::Seconds(); bCsvRequested = true;
        Profiler->BeginCapture(-1, Output, TEXT("EngineCookedSmoke.csv"));
        CsvReport->SetStringField(TEXT("status"), TEXT("requested"));
        return true;
#else
        return CsvFailure(TEXT("Full engine CSV instrumentation is unavailable in this build; no GPU timing was measured."));
#endif
    }
    bool PollCsvStart(double Now)
    {
#if CSV_PROFILER_STATS
        if (!bCsvRequested || bCsvOwned || bCsvEndRequested || bCsvFinished) return true;
        auto* Profiler = FCsvProfiler::Get();
        if (Profiler->IsCapturing())
        {
            if (!SameCsvPath(Profiler->GetOutputFilename())) return CsvFailure(TEXT("A different pending engine CSV capture started; smoke did not take its ownership."));
            bCsvOwned = true;
            bCsvCategoryWasEnabled = Profiler->IsCategoryEnabled(CSV_CATEGORY_INDEX(MemoriaS362Smoke));
            Profiler->EnableCategoryByIndex(CSV_CATEGORY_INDEX(MemoriaS362Smoke), true); bCsvCategoryChanged = true;
            CsvReport->SetStringField(TEXT("status"), TEXT("capturing"));
            CsvReport->SetStringField(TEXT("actual_path"), Profiler->GetOutputFilename());
            CsvReport->SetNumberField(TEXT("capture_started_elapsed_seconds"), Now - Started);
        }
        else if (Now - CsvRequestedAt >= 5.) return CsvFailure(TEXT("Owned engine CSV did not start within five seconds of native ticks."));
        return true;
#else
        return false;
#endif
    }
    bool FinalizeCsv(double Now)
    {
        if (bCsvFinished) return true;
#if CSV_PROFILER_STATS
        if (!bCsvEndRequested)
        {
            auto* Profiler = FCsvProfiler::Get();
            if (!bCsvOwned || !Profiler->IsCapturing() || !SameCsvPath(Profiler->GetOutputFilename()))
                return CsvFailure(TEXT("Owned engine CSV is no longer the active capture; no other capture was stopped."));
            // ExecCmds is processed after startup on some hosts; validate this at the actual stop boundary.
            if (!CsvBlockingEnd()) return CsvFailure(TEXT("CSV requires -ExecCmds=\"csv.BlockOnCaptureEnd 1\" to avoid the installed engine nonblocking Stop retry race."));
            CsvCompletion = Profiler->EndCapture(); bCsvEndRequested = true; CsvEndAt = Now;
            if (!CsvCompletion.IsValid()) return CsvFailure(TEXT("Owned engine CSV did not return a completion future."));
            CsvReport->SetStringField(TEXT("status"), TEXT("finalizing")); CsvReport->SetNumberField(TEXT("stop_requested_elapsed_seconds"), Now - Started);
            Write(); return false;
        }
        if (!CsvCompletion.IsReady())
        {
            if (Now - CsvEndAt >= 15.) return CsvFailure(TEXT("Owned engine CSV did not finish within fifteen seconds of native ticks."));
            return false;
        }
        const FString ActualPath = CsvCompletion.Get(); // IsReady above; never blocks the game thread.
        bCsvOwned = false;
        if (!SameCsvPath(ActualPath)) return CsvFailure(TEXT("Engine CSV completion returned a different path."));
        const int64 FileBytes = IFileManager::Get().FileSize(*CsvPath);
        TArray<uint8> Bytes;
        if (FileBytes <= 128 || FileBytes > 64 * 1024 * 1024 || !FFileHelper::LoadFileToArray(Bytes, *CsvPath))
            return CsvFailure(TEXT("Finalized engine CSV is missing, empty or exceeds the bounded evidence size."));
        FString Text; FFileHelper::BufferToString(Text, Bytes.GetData(), Bytes.Num());
        if (!Text.Contains(TEXT("HasHeaderRowAtEnd")) || !Text.Contains(TEXT("MemoriaS362Smoke/Region")) || !Text.Contains(TEXT("MemoriaS362Smoke/Phase")) ||
            !Text.Contains(TEXT("MemoriaS362Smoke/CapturePending")) || !Text.Contains(TEXT("MemoriaS362Smoke/ActualCombat")))
            return CsvFailure(TEXT("Engine CSV lacks its final header metadata or actual smoke tag columns."));
        bCsvFinished = true;
        CsvReport->SetStringField(TEXT("status"), TEXT("complete")); CsvReport->SetStringField(TEXT("actual_path"), ActualPath);
        CsvReport->SetNumberField(TEXT("bytes"), Bytes.Num()); CsvReport->SetStringField(TEXT("sha1"), Hash(Bytes));
        CsvReport->SetStringField(TEXT("completed_utc"), FDateTime::UtcNow().ToIso8601());
        CsvReport->SetNumberField(TEXT("completion_elapsed_seconds"), Now - Started);
        CsvReport->SetBoolField(TEXT("gpu_time_column_present"), Text.Contains(TEXT("GPUTime")));
        CsvReport->SetBoolField(TEXT("completion_future_ready"), true); CsvReport->SetBoolField(TEXT("final_header_metadata_present"), true);
        CleanupCsv(); Write(); return true;
#else
        return CsvFailure(TEXT("Engine CSV finalization is unavailable in this build."));
#endif
    }
#endif

    bool Write()
    {
        Report->SetNumberField(TEXT("phase"), Phase); Report->SetNumberField(TEXT("elapsed_seconds"), FPlatformTime::Seconds() - Started);
        Report->SetBoolField(TEXT("s363_requested"),GraphicsS363Requested());
        Report->SetBoolField(TEXT("s364_requested"),GameplayS364Requested());
        Report->SetBoolField(TEXT("s365_requested"),FParse::Param(FCommandLine::Get(),TEXT("MemoriaS365Smoke")));
        Report->SetArrayField(TEXT("checks"), Checks); Report->SetArrayField(TEXT("regions"), Regions); Report->SetArrayField(TEXT("captures"), Captures);
#if UE_BUILD_DEVELOPMENT && WITH_DEV_AUTOMATION_TESTS
        Report->SetObjectField(TEXT("performance"), Performance.Json());
        Report->SetObjectField(TEXT("engine_csv"), CsvReport);
#endif
        return WriteJson(Output / TEXT("status.json"), Report);
    }
    void BeginReplayStepS364()
    {
        if(bS364FixedStepOwned)return;
        bS364PreviousFixed=FApp::UseFixedTimeStep();S364PreviousFixedDelta=FApp::GetFixedDeltaTime();
        FApp::SetFixedDeltaTime(1./60.);FApp::SetUseFixedTimeStep(true);bS364FixedStepOwned=true;
        Report->SetNumberField(TEXT("s364_controlled_replay_fixed_delta"),1./60.);
        Report->SetStringField(TEXT("s364_controlled_replay_scope"),TEXT("Only the opt-in phase2/3 combo/defense fixture uses engine fixed1/60 gameplay steps. No attack clock, queue threshold, HP, pose or result is directly assigned. Prior timestep is restored before burn/travel and on failure/teardown. Phase2/3 timing is controlled validation, not natural-frame/FPS evidence."));
    }
    void RestoreReplayStepS364()
    {
        if(!bS364FixedStepOwned)return;
        FApp::SetFixedDeltaTime(S364PreviousFixedDelta);FApp::SetUseFixedTimeStep(bS364PreviousFixed);bS364FixedStepOwned=false;
    }
    void RestoreCapturePauseS364()
    {
        if(!bS364CapturePauseOwned)return;
        if(auto* CaptureWorld=S364CaptureWorld.Get();CaptureWorld && !CaptureWorld->bIsTearingDown)
            UGameplayStatics::SetGamePaused(CaptureWorld,bS364CaptureWasPaused);
        bS364CapturePauseOwned=false;S364CaptureWorld.Reset();
    }
    void Finish(bool bPass, const FString& Reason)
    {
        if (bDone) return;
        RestoreCapturePauseS364();RestoreReplayStepS364();
#if UE_BUILD_DEVELOPMENT && WITH_DEV_AUTOMATION_TESTS
        if (!bPass) CleanupCsv();
#endif
        bDone = true; Report->SetStringField(TEXT("status"), bPass ? TEXT("PASS") : TEXT("FAIL"));
        Report->SetStringField(TEXT("reason"), Reason); Report->SetNumberField(TEXT("exit_code"), bPass ? 0 : 2);
        if (!Write()) { bPass = false; UE_LOG(LogTemp, Error, TEXT("MEMORIA_UPGRADE_SMOKE evidence write failed")); }
        UE_LOG(LogTemp, Display, TEXT("MEMORIA_UPGRADE_SMOKE_%s output=%s reason=%s"), bPass ? TEXT("PASS") : TEXT("FAIL"), *Output, *Reason);
        FPlatformMisc::RequestExitWithStatus(false, bPass ? 0 : 2);
    }
    bool Check(bool bValue, const FString& Label)
    {
        auto Item = MakeShared<FJsonObject>(); Item->SetStringField(TEXT("label"), Label); Item->SetBoolField(TEXT("passed"), bValue); Item->SetNumberField(TEXT("phase"), Phase);
        Checks.Add(MakeShared<FJsonValueObject>(Item));
        if (!bValue) Finish(false, Label);
        return bValue;
    }
    void Next() { ++Phase; PhaseAt = FPlatformTime::Seconds(); Write(); }
    void Capture(const TCHAR* Name)
    {
        PendingPng = Output / (FString(Name) + TEXT(".png")); PendingAt = FPlatformTime::Seconds();
        FScreenshotRequest::RequestScreenshot(PendingPng, true, false);
    }
    bool PendingCaptureDone()
    {
        if (PendingPng.IsEmpty()) return true;
        TArray<uint8> Bytes;
        if (IFileManager::Get().FileSize(*PendingPng) > 1024 && FFileHelper::LoadFileToArray(Bytes, *PendingPng) && Bytes.Num() > 1024)
        {
            static const uint8 Png[8] = {137,80,78,71,13,10,26,10};
            if (Bytes.Num() < 24 || FMemory::Memcmp(Bytes.GetData(), Png, 8) != 0) { Finish(false, TEXT("Native screenshot is not a PNG")); return false; }
            // The image writer can be asynchronous: wait for the terminal IEND chunk before hashing.
            static const uint8 End[12] = {0,0,0,0,73,69,78,68,174,66,96,130};
            if (FMemory::Memcmp(Bytes.GetData() + Bytes.Num() - 12, End, 12) != 0)
            {
                if (FPlatformTime::Seconds() - PendingAt > 12.) Finish(false, TEXT("Native PNG did not finish writing"));
                return false;
            }
            auto U32 = [&](int32 At) { return (uint32(Bytes[At]) << 24) | (uint32(Bytes[At+1]) << 16) | (uint32(Bytes[At+2]) << 8) | uint32(Bytes[At+3]); };
            const uint32 W = U32(16), H = U32(20);
            if (W < 640 || H < 360) { Finish(false, TEXT("Native screenshot resolution is below the smoke minimum")); return false; }
            auto Image = MakeShared<FJsonObject>(); Image->SetStringField(TEXT("path"), PendingPng); Image->SetStringField(TEXT("sha1"), Hash(Bytes));
            Image->SetNumberField(TEXT("bytes"), Bytes.Num()); Image->SetNumberField(TEXT("width"), W); Image->SetNumberField(TEXT("height"), H);
            Captures.Add(MakeShared<FJsonValueObject>(Image)); PendingPng.Reset(); RestoreCapturePauseS364(); Write(); return true;
        }
        if (FPlatformTime::Seconds() - PendingAt > 12.) Finish(false, TEXT("Native screenshot was not written within12seconds"));
        return false;
    }
#if UE_BUILD_DEVELOPMENT && WITH_DEV_AUTOMATION_TESTS
    bool FocusS364(UWorld& World,AMemoriaSliceController& PC,AMemoriaFieldPawn& Pawn,UMemoriaRunSubsystem& Run,double Now)
    {
        if(!GameplayS364Requested() || bS364FocusDone)return true;
        if(S364FocusPhase==0)
        {
            AMemoriaMaletActor* Malet=nullptr;for(TActorIterator<AMemoriaMaletActor> It(&World);It;++It){Malet=*It;break;}
            if(!Check(Malet!=nullptr,TEXT("S364 focus uses the real loaded Malet interactable")))return false;
            S364FocusBefore=Run.GetRunSnapshot();S364OriginalPosition=Pawn.GetActorLocation();S364FocusTarget=Malet;
            // Explicit isolated positioning fixture, matching native interaction tests; never a forced resolver target.
            Pawn.SetActorLocation(Malet->GetActorLocation()+FVector(-40,0,0),false,nullptr,ETeleportType::TeleportPhysics);
            S364At=Now;S364Frame=GFrameCounter;S364FocusPhase=1;return false;
        }
        if(Now-S364At>8.){Finish(false,TEXT("S364 actual interaction focus did not settle"));return false;}
        if(S364FocusPhase==1)
        {
            auto* Field=GraphicsWidget<UMemoriaFieldHudWidget>(&PC);auto* Interaction=PC.GetInteraction();
            if(!Field || !Interaction)return false;
            const auto& Probe=Field->GetInteractionFocusPaintProbe();
            if(!Probe.bDrawn || Probe.Frame<=S364Frame)return false;
            const FMemoriaRunSnapshot Current=Run.GetRunSnapshot();
            const bool Unchanged=FMemoriaRunSnapshot::StaticStruct()->CompareScriptStruct(&S364FocusBefore,&Current,0);
            const bool Exact=Probe.Target.Get()==S364FocusTarget.Get() && Interaction->GetTarget()==S364FocusTarget.Get() && !PC.GetInteractionPrompt().IsEmpty();
            if(!Check(Exact,TEXT("S364 painted focus equals actual executable resolver target")) ||
                !Check(Unchanged,TEXT("S364 observing interaction focus does not change the complete run snapshot")))return false;
            Report->SetStringField(TEXT("s364_focus_target"),S364FocusTarget->GetClass()->GetPathName());
            Report->SetStringField(TEXT("s364_focus_bounds"),Probe.Bounds.ToString());
            Report->SetBoolField(TEXT("s364_interaction_focus_verified"),true);
            Capture(TEXT("17_InteractionFocus"));S364FocusPhase=2;return false;
        }
        Pawn.SetActorLocation(S364OriginalPosition,false,nullptr,ETeleportType::TeleportPhysics);
        bS364FocusDone=true;Write();return true;
    }
    bool CombatS364(UWorld& World,AMemoriaSliceController& PC,UMemoriaFieldCombatSubsystem& Combat,double Now)
    {
        if(!GameplayS364Requested() || bS364CombatDone)return true;
        if(S364At==0.)S364At=Now;
        if(Now-S364At>12.){Finish(false,TEXT("S364 combo and defense fixture timed out"));return false;}
        auto* Figure=Combat.GetPlayerFigure();auto* Hud=GraphicsWidget<UMemoriaCombatHudWidget>(&PC);
        if(!Figure || !Hud || !Target.IsValid())return false;
        const auto Combo=Combat.GetComboInputProbe();
        if(S364CombatPhase==0)
        {
            const float Fraction=Combo.StepLength>0.f?Combo.StepTime/Combo.StepLength:0.f;
            if(Combo.Step!=0 || Fraction<.28f)return false;
            Report->SetNumberField(TEXT("s364_observed_press_fraction"),Fraction);
            Report->SetNumberField(TEXT("s364_observed_game_delta"),World.GetDeltaSeconds());
            Report->SetNumberField(TEXT("s364_observed_app_delta"),FApp::GetDeltaTime());
            if(!Check(Fraction<MemoriaCombatTuning::QueueFrom,TEXT("S364 physical second press occurs before the original queue window")))return false;
            Tap(&PC,EKeys::J);
            const auto Buffered=Combat.GetComboInputProbe();
            if(!Check(Buffered.Step==0 && Buffered.EarlySeconds>0.f && !Buffered.bQueued,TEXT("S364 actual physical J occupies the early one-slot buffer")))return false;
            Report->SetNumberField(TEXT("s364_early_press_fraction"),Fraction);Report->SetNumberField(TEXT("s364_early_buffer_seconds"),Buffered.EarlySeconds);
            S364CombatPhase=1;return false;
        }
        if(S364CombatPhase==1)
        {
            if(Combo.Step!=1)return false;
            if(!Check(Figure->GetActionClip()==MemoriaCombatClips::Load(TEXT("Arrel"),MemoriaCombatClips::ForAction(TEXT("Arrel"),MemoriaCombatClips::Attack(1))),
                TEXT("S364 one early press starts the actual second combo clip")))return false;
            Report->SetBoolField(TEXT("s364_early_combo_verified"),true);Capture(TEXT("14_ComboChain"));S364CombatPhase=2;return false;
        }
        if(S364CombatPhase==2)
        {
            if(Combat.IsAttacking())return false;
            PC.InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::RightMouseButton,IE_Pressed,1.f));
            if(!Check(Combat.IsBlocking(),TEXT("S364 physical RMB enters guard before the controlled strike")))return false;
            S364BlockAt=World.GetTimeSeconds();S364BlockHp=Combat.GetPlayerHp();S364CombatPhase=3;return false;
        }
        if(S364CombatPhase==3)
        {
            if(World.GetTimeSeconds()-S364BlockAt<=MemoriaCombatTuning::ParryWindow+.06f)return false;
            Combat.StrikePlayer(Target.Get(),20.f);
            S364BlockLoss=S364BlockHp-Combat.GetPlayerHp();S364Frame=GFrameCounter;
            if(!Check(S364BlockLoss>0 && !Combat.GetPopups().IsEmpty() && Combat.GetPopups().Last().Outcome==EMemoriaDefenseOutcome::Block &&
                Combat.GetPopups().Last().Amount==float(S364BlockLoss),TEXT("S364 held guard records only actual authoritative HP loss")))return false;
            S364CombatPhase=4;return false;
        }
        if(S364CombatPhase==4)
        {
            const auto& Probe=Hud->GetDefensePaintProbe();if(Probe.Frame<=S364Frame)return false;
            const auto* Painted=Probe.Popups.FindByPredicate([](const auto& P){return P.Outcome==EMemoriaDefenseOutcome::Block;});
            const bool Shape=Probe.Shapes.ContainsByPredicate([](const auto& P){return P.Outcome==EMemoriaDefenseOutcome::Block;});
            if(!Check(Painted && Shape && Painted->Amount==float(S364BlockLoss) && Painted->Text.Contains(FString::Printf(TEXT("%lld HP"),S364BlockLoss)),
                TEXT("S364 actual painted block shows real HP cost and distinct shield")))return false;
            Report->SetNumberField(TEXT("s364_actual_block_hp_loss"),S364BlockLoss);Report->SetStringField(TEXT("s364_actual_block_text"),Painted->Text);
            Report->SetBoolField(TEXT("s364_block_damage_feedback_verified"),true);Capture(TEXT("15_BlockOutcome"));S364CombatPhase=5;return false;
        }
        if(S364CombatPhase==5)
        {
            PC.InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::RightMouseButton,IE_Released,0.f));
            PC.InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::RightMouseButton,IE_Pressed,1.f));
            S364ParryHp=Combat.GetPlayerHp();Combat.StrikePlayer(Target.Get(),20.f);S364Frame=GFrameCounter;
            if(!Check(Combat.GetPlayerHp()==S364ParryHp && !Combat.GetPopups().IsEmpty() && Combat.GetPopups().Last().Outcome==EMemoriaDefenseOutcome::Parry && Combat.GetPopups().Last().Amount==0.f,
                TEXT("S364 fresh actual guard parries without HP loss")))return false;
            S364CombatPhase=6;return false;
        }
        if(S364CombatPhase==6)
        {
            const auto& Probe=Hud->GetDefensePaintProbe();if(Probe.Frame<=S364Frame)return false;
            const bool Text=Probe.Popups.ContainsByPredicate([](const auto& P){return P.Outcome==EMemoriaDefenseOutcome::Parry && P.Amount==0.f && !P.Text.IsEmpty();});
            const bool Shape=Probe.Shapes.ContainsByPredicate([](const auto& P){return P.Outcome==EMemoriaDefenseOutcome::Parry;});
            if(!Check(Text && Shape,TEXT("S364 actual painted parry has distinct gold crest and zero-loss label")))return false;
            if(FParse::Param(FCommandLine::Get(),TEXT("MemoriaS365Smoke")))
            {
                const auto& Feedback=Hud->GetCombatFeedbackPaintProbe();
                bool Inside=true,Separate=true,Complete=true;TArray<TSharedPtr<FJsonValue>> Entries;
                for(int32 I=0;I<Feedback.Popups.Num();++I)
                {
                    const auto& P=Feedback.Popups[I];
                    Inside &= P.Bounds.bIsValid && P.Bounds.Min.X>=0 && P.Bounds.Min.Y>=0 && P.Bounds.Max.X<=Feedback.Viewport.X && P.Bounds.Max.Y<=Feedback.Viewport.Y;
                    Complete &= !P.bOverloaded && P.Alpha>0 && !P.Text.IsEmpty();
                    for(int32 J=0;J<I;++J)Separate &= !P.Bounds.Intersect(Feedback.Popups[J].Bounds);
                    auto Entry=MakeShared<FJsonObject>();Entry->SetStringField(TEXT("text"),P.Text);Entry->SetNumberField(TEXT("amount"),P.Amount);Entry->SetNumberField(TEXT("source_index"),P.SourceIndex);
                    Entry->SetNumberField(TEXT("left"),P.Bounds.Min.X);Entry->SetNumberField(TEXT("top"),P.Bounds.Min.Y);Entry->SetNumberField(TEXT("right"),P.Bounds.Max.X);Entry->SetNumberField(TEXT("bottom"),P.Bounds.Max.Y);
                    Entries.Add(MakeShared<FJsonValueObject>(Entry));
                }
                const bool BlockPresent=Feedback.Popups.ContainsByPredicate([](const auto& P){return P.Text.Contains(TEXT("Blocked"));});
                const bool ParryPresent=Feedback.Popups.ContainsByPredicate([](const auto& P){return P.Text.Contains(TEXT("Parry"));});
                if(!Check(Feedback.bDrawn && Feedback.bEnabled && Feedback.Frame>S364Frame && Feedback.Popups.Num()>=2 && BlockPresent && ParryPresent && Inside && Separate && Complete,
                    TEXT("S365 actual concurrent block/parry popup glyphs remain truthful, separate and within the cooked viewport")))return false;
                Report->SetArrayField(TEXT("s365_concurrent_popups"),Entries);Report->SetBoolField(TEXT("s365_popup_layout_verified"),true);
            }
            Report->SetBoolField(TEXT("s364_parry_feedback_verified"),true);Capture(TEXT("16_ParryOutcome"));S364CombatPhase=7;return false;
        }
        PC.InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::RightMouseButton,IE_Released,0.f));
        Target->SetActorTickEnabled(bS364TargetTick);RestoreReplayStepS364();bS364CombatDone=true;Write();return true;
    }
    bool GraphicsGuard(UWorld& World,AMemoriaSliceController& PC,UMemoriaFieldCombatSubsystem& Combat,double Now)
    {
        if(!GraphicsS363Requested() || bS363GuardDone)return true;
        if(!bS363GuardStarted)
        {
            // Finish the existing victory panel before recording the ordinary held-guard layout.
            if(Combat.GetLastReward().Age<3.6f)return false;
            PC.InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::RightMouseButton,IE_Pressed,1.f));
            if(!Check(Combat.IsBlocking(),TEXT("S363 physical RMB enters actual guard")))return false;
            bS363GuardStarted=bS363GuardHeld=true;S363GuardAt=Now;S363GuardFrame=GFrameCounter;return false;
        }
        if(Now-S363GuardAt>10.){Finish(false,TEXT("S363 real held guard/layout did not settle"));return false;}
        if(!bS363GuardCaptured)
        {
            auto* Figure=Combat.GetPlayerFigure();auto* Skin=Figure?Figure->GetSkeletalMesh():nullptr;
            auto* Anim=Skin?Cast<UMemoriaFieldAnimInstance>(Skin->GetAnimInstance()):nullptr;
            auto* Hud=GraphicsWidget<UMemoriaCombatHudWidget>(&PC);auto* Field=GraphicsWidget<UMemoriaFieldHudWidget>(&PC);
            const float ExpectedHold=Figure?Figure->GetActionLength()*.38f:0.f;
            if(!Anim || !Hud || !Field || Anim->ActionWeight<.98f || FMath::Abs(Anim->ActionTime-ExpectedHold)>.012f)return false;
            const auto& Ribbon=Hud->GetRibbonPaintProbe();const auto& Tray=Field->GetQuickTrayPaintProbe();
            if(!Ribbon.bDrawn || !Tray.bDrawn || Ribbon.Frame<=S363GuardFrame || Tray.Frame<=S363GuardFrame)return false;
            const bool Clip=Combat.IsBlocking() && Skin->GetSkeletalMeshAsset()->GetPathName()==TEXT("/Game/Memoria/Presentation/Field3D/HeroFinishS362/R4/Arrel/SK_Arrel.SK_Arrel")
                && Figure->GetActionClip()==MemoriaCombatClips::Load(TEXT("Arrel"),TEXT("Sword_Block")) && Anim->Action==Figure->GetActionClip();
            const bool Layout=Ribbon.Viewport==Tray.Viewport && Tray.bLifted && !Tray.Bounds.Intersect(Ribbon.OccupiedBounds)
                && !Tray.Bounds.Intersect(Ribbon.MemoryOrbBounds) && Tray.Bounds.Max.Y<=Ribbon.OccupiedBounds.Min.Y-MemoriaHudLayoutS363::TrayClearance+.1f
                && Ribbon.ActionCells.Num()==7 && Tray.Keys.Num()==5;
            if(!Check(Clip,TEXT("S363 actual R4 player holds the loaded Sword_Block at fraction.38 with full action weight"))
                || !Check(Layout,TEXT("S363 actual painted quicktray clears the ribbon/MEM with12Slate-unitgap")))return false;
            TArray<TSharedPtr<FJsonValue>> Bones;const auto& CS=Skin->GetComponentSpaceTransforms();const auto& Ref=Skin->GetSkeletalMeshAsset()->GetRefSkeleton();
            for(const TCHAR* Name:{TEXT("cloth_0_1"),TEXT("cloth_0_2"),TEXT("cloth_1_1"),TEXT("cloth_1_2"),TEXT("cloth_7_1"),TEXT("cloth_7_2")})
            {
                const int32 Index=Ref.FindBoneIndex(Name),Parent=Index>=0?Ref.GetParentIndex(Index):INDEX_NONE;
                if(!Check(CS.IsValidIndex(Index) && CS.IsValidIndex(Parent),TEXT("S363 actual cloth bones exist in completed runtime pose")))return false;
                const FTransform Local=CS[Index].GetRelativeTransform(CS[Parent]);
                if(!Check(!Local.ContainsNaN(),TEXT("S363 completed cloth local pose is finite")))return false;
                auto B=MakeShared<FJsonObject>();B->SetStringField(TEXT("name"),Name);B->SetStringField(TEXT("local_translation"),Local.GetTranslation().ToString());
                B->SetStringField(TEXT("local_rotation"),Local.GetRotation().ToString());B->SetStringField(TEXT("local_scale"),Local.GetScale3D().ToString());Bones.Add(MakeShared<FJsonValueObject>(B));
            }
            Report->SetArrayField(TEXT("s363_actual_guard_cloth_locals"),Bones);
            Report->SetStringField(TEXT("s363_guard_clip"),Figure->GetActionClip()->GetPathName());Report->SetNumberField(TEXT("s363_guard_clip_time"),Anim->ActionTime);
            Report->SetNumberField(TEXT("s363_guard_action_weight"),Anim->ActionWeight);
            Report->SetNumberField(TEXT("s363_guard_hold_fraction"),.38);Report->SetNumberField(TEXT("s363_guard_expected_hold_seconds"),ExpectedHold);
            Report->SetStringField(TEXT("s363_guard_proof_scope"),TEXT("Actual original R4 player clip/time/weight, six completed cloth locals and gameplay-camera PNG with the new HUD. S363 cloth-correction candidates failed their separate contact comparison and were archived; no cloth correction is active."));
            Report->SetBoolField(TEXT("s363_guard_clip_verified"),true);Report->SetBoolField(TEXT("s363_hud_non_overlap_verified"),true);
            auto R=MakeShared<FJsonObject>();R->SetStringField(TEXT("ribbon_occupied"),Ribbon.OccupiedBounds.ToString());R->SetStringField(TEXT("tray"),Tray.Bounds.ToString());
            R->SetNumberField(TEXT("actual_vertical_gap_slate_units"),Ribbon.OccupiedBounds.Min.Y-Tray.Bounds.Max.Y);Report->SetObjectField(TEXT("s363_actual_hud_layout"),R);
            Capture(TEXT("13_GuardHud"));bS363GuardCaptured=true;return false;
        }
        if(bS363GuardHeld)
        {PC.InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::RightMouseButton,IE_Released,0.f));bS363GuardHeld=false;return false;}
        if(Combat.IsBlocking() || (Combat.GetPlayerFigure() && Combat.GetPlayerFigure()->IsActing()))return false;
        bS363GuardDone=true;Write();return true;
    }
#endif
    bool Region(UWorld& World, AMemoriaFieldPawn& Pawn, const FString& Map)
    {
        AActor* Owner = nullptr; UMemoriaFieldCharacterComponent* Arrel = nullptr;
        if (Map == TEXT("verdan_market"))
        {
            for (TActorIterator<AMemoriaVerdanPresentation> It(&World); It; ++It) { Owner = *It; Arrel = It->CharacterFigure(); }
        }
        else
        {
            for (TActorIterator<AMemoriaChapterPresentation> It(&World); It; ++It)
                if (It->GetMap() == Map) { Owner = *It; Arrel = It->GetArrelFigure(); }
        }
        if (!Check(Owner && Arrel && Arrel->GetSkeletalMesh() && Pawn.GetFieldCamera(), Map + TEXT(": live presentation, gameplay camera and rigged Arrel"))) return false;
        const FString ArrelPath = TEXT("/Game/Memoria/Presentation/Field3D/HeroFinishS362/R4/Arrel/SK_Arrel.SK_Arrel");
        const FString EliaPath = TEXT("/Game/Memoria/Presentation/Field3D/HeroFinishS362/R2/Elia/SK_Elia.SK_Elia");
        auto* ArrelMesh = LoadObject<USkeletalMesh>(nullptr, *ArrelPath);
        auto* EliaMesh = LoadObject<USkeletalMesh>(nullptr, *EliaPath);
        if (!Check(ArrelMesh && EliaMesh && Arrel->GetSkeletalMesh()->GetSkeletalMeshAsset() == ArrelMesh,
            Map + TEXT(": cooked Arrel R4 and Elia R2 exist; runtime Arrel uses R4 without fallback"))) return false;
        if (!Check(ArrelMesh->GetRefSkeleton().GetNum() == 77 && EliaMesh->GetRefSkeleton().GetNum() == 77 &&
            ArrelMesh->GetMorphTargets().IsEmpty() && FMemoriaHeroFinish::HasDedicatedLids(EliaMesh), Map + TEXT(": original77binds, painted Arrel with rigid sole and dedicated Elia lid morphs"))) return false;
        const auto* ArrelRender=ArrelMesh->GetResourceForRendering();
        const auto* EliaRender=EliaMesh->GetResourceForRendering();
        if(!Check(ArrelRender && EliaRender && !ArrelRender->LODRenderData.IsEmpty() && !EliaRender->LODRenderData.IsEmpty(),Map+TEXT(": cooked skeletal LOD buffers exist")))return false;
        uint32 ArrelTriangles=0,EliaTriangles=0;
        for(const auto& Section:ArrelRender->LODRenderData[0].RenderSections)ArrelTriangles+=Section.NumTriangles;
        for(const auto& Section:EliaRender->LODRenderData[0].RenderSections)EliaTriangles+=Section.NumTriangles;
        if(!Check(ArrelTriangles==33804 && EliaTriangles==64832,Map+TEXT(": actual cooked Arrel33804/Elia64832 LOD0 triangles retained")))return false;
        // Compare actual cooked CPU buffers with the accepted editor import, including
        // original painted hilt UV0/1 and all eight initialized delivered sword channels.
        auto* Sword = LoadObject<UStaticMesh>(nullptr, MemoriaAcceptedPropsS362::Sword);
        auto* Staff = LoadObject<UStaticMesh>(nullptr, MemoriaAcceptedPropsS362::Staff);
        if (!Check(Sword && Staff && Sword->bAllowCPUAccess && Staff->bAllowCPUAccess,
            Map + TEXT(": cooked R5 props retain required CPU buffers"))) return false;
        const FString SwordHash = FMemoriaNativePropFingerprint::MeshLOD0(Sword);
        const FString StaffHash = FMemoriaNativePropFingerprint::MeshLOD0(Staff);
        const FString HiltHash = FMemoriaNativePropFingerprint::MeshLOD0(Sword, 0, 2);
        if (!Check(!SwordHash.IsEmpty() && !StaffHash.IsEmpty() && !HiltHash.IsEmpty()
            && SwordHash == MemoriaAcceptedPropsS362::SwordHash && StaffHash == MemoriaAcceptedPropsS362::StaffHash
            && HiltHash == MemoriaAcceptedPropsS362::OriginalHiltHash,
            Map + TEXT(": cooked R5 geometry/UV/paint/packed tangents match accepted native fingerprints"))) return false;
        if (!Check(Arrel->GetBlade() && Arrel->GetBlade()->GetStaticMesh() == Sword
            && GetPathNameSafe(Arrel->GetBlade()->GetMaterial(0)) == MemoriaAcceptedPropsS362::OriginalHiltMaterial,
            Map + TEXT(": actual drawn sword binds R5 and its original painted hilt material"))) return false;
        int32 EliaFigures = 0, BoundEliaStaffs = 0;
        for (TActorIterator<AActor> It(&World); It; ++It)
        {
            TInlineComponentArray<UMemoriaFieldCharacterComponent*> Figures; It->GetComponents(Figures);
            for (auto* Figure : Figures)
                if (Figure->GetCharacterId().Equals(TEXT("elia"), ESearchCase::IgnoreCase) && Figure->GetSkeletalMesh())
                {
                    ++EliaFigures;
                    TInlineComponentArray<UStaticMeshComponent*> Props; It->GetComponents(Props);
                    for (auto* Prop : Props)
                        if (Prop->GetAttachParent() == Figure->GetSkeletalMesh() && Prop->GetStaticMesh() == Staff && Prop->IsVisible()) ++BoundEliaStaffs;
                    if (!Check(Figure->GetSkeletalMesh()->GetSkeletalMeshAsset() == EliaMesh, Map + TEXT(": visible Elia uses cooked R2"))) return false;
                }
        }
        const auto* Run = World.GetGameInstance()->GetSubsystem<UMemoriaRunSubsystem>();
        if (!Check(!Run->GetRunSnapshot().Player.bEliaWithParty || EliaFigures > 0, Map + TEXT(": party Elia has a visible rigged runtime figure"))) return false;
        // S388 (T-15): the canon gives Elia no staff; the cooked R5 staff stays in the package but no figure binds it.
        if (!Check(BoundEliaStaffs == 0, Map + TEXT(": actual party Elia carries no staff"))) return false;
        TInlineComponentArray<UInstancedStaticMeshComponent*> Batches; Owner->GetComponents(Batches);
        TSet<UMaterialInstanceDynamic*> Materials; int32 Instances = 0, AtlasBindings = 0;
        for (auto* Batch : Batches)
        {
            auto* Material = Cast<UMaterialInstanceDynamic>(Batch->GetMaterial(0));
            if (!Material || !Material->GetName().StartsWith(TEXT("S362"))) continue;
            Materials.Add(Material); Instances += Batch->GetInstanceCount();
            if (!Check(Batch->GetStaticMesh() && Batch->GetInstanceCount() > 0 && Batch->GetCollisionEnabled() == ECollisionEnabled::NoCollision &&
                !Batch->GetGenerateOverlapEvents() && !Batch->CanEverAffectNavigation(), Map + TEXT(": S362 ISM detail has cooked mesh and preserves collision/navigation"))) return false;
            UTexture* Atlas = nullptr;
            if (Material->GetTextureParameterValue(FMaterialParameterInfo(TEXT("ArchitectureAtlas")), Atlas) && Atlas &&
                Atlas->GetPathName() == TEXT("/Game/Memoria/Presentation/EnvironmentS356/T_ArchitectureAtlas.T_ArchitectureAtlas")) ++AtlasBindings;
        }
        if (!Check(Instances >= 10 && Instances < 500 && Materials.Num() == (Map == TEXT("belt_waystation") ? 1 : 3) && AtlasBindings > 0,
            Map + TEXT(": new regional material/ISM families and original painted atlas are live"))) return false;
        auto Entry = MakeShared<FJsonObject>(); Entry->SetStringField(TEXT("map"), Map); Entry->SetStringField(TEXT("world"), World.GetMapName());
        if (const auto* Client=World.GetGameViewport(); Client && Client->Viewport)
        {
            const auto Pixels=Client->Viewport->GetSizeXY(), TargetPixels=Client->Viewport->GetRenderTargetTextureSizeXY();
            Entry->SetNumberField(TEXT("viewport_width"),Pixels.X); Entry->SetNumberField(TEXT("viewport_height"),Pixels.Y);
            Entry->SetNumberField(TEXT("render_target_width"),TargetPixels.X); Entry->SetNumberField(TEXT("render_target_height"),TargetPixels.Y);
        }
        Entry->SetStringField(TEXT("arrel_mesh"), ArrelMesh->GetPathName()); Entry->SetStringField(TEXT("elia_mesh"), EliaMesh->GetPathName());
        Entry->SetNumberField(TEXT("arrel_render_lods"),ArrelRender->LODRenderData.Num()); Entry->SetNumberField(TEXT("elia_render_lods"),EliaRender->LODRenderData.Num());
        Entry->SetNumberField(TEXT("arrel_LOD0_triangles"),ArrelTriangles); Entry->SetNumberField(TEXT("elia_LOD0_triangles"),EliaTriangles);
        Entry->SetNumberField(TEXT("arrel_LOD0_vertices"),ArrelRender->LODRenderData[0].GetNumVertices()); Entry->SetNumberField(TEXT("elia_LOD0_vertices"),EliaRender->LODRenderData[0].GetNumVertices());
        Entry->SetStringField(TEXT("sword_native_fingerprint"), SwordHash); Entry->SetStringField(TEXT("staff_native_fingerprint"), StaffHash);
        Entry->SetStringField(TEXT("original_hilt_fingerprint"), HiltHash); Entry->SetNumberField(TEXT("bound_elia_staffs"), BoundEliaStaffs);
        Entry->SetNumberField(TEXT("visible_elia_figures"), EliaFigures); Entry->SetNumberField(TEXT("s362_materials"), Materials.Num());
        Entry->SetNumberField(TEXT("s362_instances"), Instances); Entry->SetNumberField(TEXT("atlas_bindings"), AtlasBindings);
        if(GraphicsS363Requested())
        {
            int32 Solid=0,SoftCount=0,BadCollision=0;TInlineComponentArray<UStaticMeshComponent*> Parts;Owner->GetComponents(Parts);
            for(auto* Part:Parts)if(Part->ComponentHasTag(TEXT("MemoriaRegionalS363")))
            {
                const auto* Batch=Cast<UInstancedStaticMeshComponent>(Part);const int32 Count=Batch?Batch->GetInstanceCount():1;
                if(Part->ComponentHasTag(TEXT("S363Solid")))Solid+=Count;
                if(Part->ComponentHasTag(TEXT("S363Soft")))SoftCount+=Count;
                if(Part->GetCollisionEnabled()!=ECollisionEnabled::NoCollision || Part->CanEverAffectNavigation())++BadCollision;
            }
            Entry->SetNumberField(TEXT("s363_solid_instances"),Solid);Entry->SetNumberField(TEXT("s363_soft_planes"),SoftCount);Entry->SetNumberField(TEXT("s363_invalid_collision_components"),BadCollision);
            if(!Check(Solid>0 && Solid<=280 && SoftCount>0 && SoftCount<=8 && BadCollision==0,Map+TEXT(": actual bounded noncolliding S363 regional detail exists")))return false;
        }
        Regions.Add(MakeShared<FJsonValueObject>(Entry));
#if UE_BUILD_DEVELOPMENT && WITH_DEV_AUTOMATION_TESTS
        Performance.RequestProcessVram(Map, Started);
#endif
        Write(); return true;
    }
};

void FMemoriaUpgradeSmokeStateDeleter::operator()(FMemoriaUpgradeSmokeState* Pointer) const
{
    delete Pointer; // State is complete here; generated VTable constructor cleanup calls this function.
}
UMemoriaUpgradeSmokeSubsystem::UMemoriaUpgradeSmokeSubsystem() = default;
UMemoriaUpgradeSmokeSubsystem::~UMemoriaUpgradeSmokeSubsystem() = default;
bool UMemoriaUpgradeSmokeSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_DEVELOPMENT && WITH_DEV_AUTOMATION_TESTS
    return MemoriaUpgradeSmoke::IsIsolatedSmokeActive() && Requested() && !GIsEditor && Super::ShouldCreateSubsystem(Outer);
#else
    return false;
#endif
}
void UMemoriaUpgradeSmokeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
#if UE_BUILD_DEVELOPMENT && WITH_DEV_AUTOMATION_TESTS
    FString UserDir;
    if (!MemoriaUpgradeSmoke::IsIsolatedSmokeActive() || GIsEditor || !FPlatformProperties::RequiresCookedData() ||
        !IsolatedUserDir(UserDir) || !EffectiveSavedDirIsolated(UserDir))
    { BootstrapRefused(TEXT("Smoke requires the pre-Super isolated smoke bootstrap, isolated effective Saved root and a cooked Development game executable.")); return; }
    Collection.InitializeDependency<UMemoriaCheckpointSubsystem>();
    Collection.InitializeDependency<UMemoriaNarrativeSubsystem>();
    Collection.InitializeDependency<UMemoriaSettingsSubsystem>();
    State.Reset(new FMemoriaUpgradeSmokeState());
    // Only reached after the existing cooked/opt-in/pre-Super/isolation bootstrap checks above.
    MemoriaPresentationMetrics::SetEnabled(true);
    State->Leaf = TEXT("upgrade-smoke-") + FGuid::NewGuid().ToString(EGuidFormats::Digits);
    State->Output = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Validation/UpgradeSmoke") / State->Leaf);
    State->Report->SetStringField(TEXT("status"), TEXT("RUNNING")); State->Report->SetStringField(TEXT("mode"), TEXT("cooked_development_render_smoke"));
    State->Report->SetStringField(TEXT("engine_version"), FEngineVersion::Current().ToString());
    State->Report->SetStringField(TEXT("executable"), FPlatformProcess::ExecutablePath());
    State->Report->SetStringField(TEXT("started_utc"), FDateTime::UtcNow().ToIso8601());
    State->Report->SetBoolField(TEXT("cooked_data_required"), FPlatformProperties::RequiresCookedData()); State->Report->SetBoolField(TEXT("is_editor"), GIsEditor);
    State->Report->SetBoolField(TEXT("isolated_smoke_until_process_exit"), MemoriaUpgradeSmoke::IsIsolatedSmokeActive());
    State->Report->SetBoolField(TEXT("engine_automation_testing"), GIsAutomationTesting);
    State->Report->SetStringField(TEXT("user_dir"), UserDir);
    auto PerformanceEnvironment = MakeShared<FJsonObject>();
    PerformanceEnvironment->SetStringField(TEXT("cpu"), FPlatformMisc::GetCPUBrand());
    PerformanceEnvironment->SetNumberField(TEXT("logical_cpu_cores"), FPlatformMisc::NumberOfCoresIncludingHyperthreads());
    PerformanceEnvironment->SetStringField(TEXT("gpu"), FPlatformMisc::GetPrimaryGPUBrand());
    PerformanceEnvironment->SetStringField(TEXT("rhi"), GDynamicRHI ? GDynamicRHI->GetName() : TEXT("unavailable"));
    PerformanceEnvironment->SetBoolField(TEXT("render_offscreen_requested"), FParse::Param(FCommandLine::Get(), TEXT("RenderOffscreen")));
    PerformanceEnvironment->SetBoolField(TEXT("is_editor"), GIsEditor);
    State->Report->SetObjectField(TEXT("performance_environment"), PerformanceEnvironment);
    State->Report->SetStringField(TEXT("limits"), TEXT("Bounded seeded render/asset/input/save smoke. Not a full human playthrough, Shipping certificate, physical-gamepad test or performance benchmark."));
    auto* C = GetGameInstance()->GetSubsystem<UMemoriaCheckpointSubsystem>();
    if (!State->Check(!C->IsStorageEnabled(), TEXT("No normal checkpoint root was enabled before test storage")) ||
        !State->Check(C->ConfigureTestStorage(State->Leaf), TEXT("Checkpoint configured an isolated Saved/Validation leaf"))) return;
    State->Report->SetStringField(TEXT("checkpoint_root"), FPaths::GetPath(C->GetSlotPath()));
    GetGameInstance()->GetSubsystem<UMemoriaSettingsSubsystem>()->SetLocale(TEXT("en"));
    if (!State->Write()) { State->Finish(false, TEXT("Cannot create isolated smoke evidence directory")); return; }
    if (!State->BeginCsv()) return;
    State->Write();
    UE_LOG(LogTemp, Display, TEXT("MEMORIA_UPGRADE_SMOKE_STARTED output=%s checkpoint=%s"), *State->Output, *FPaths::GetPath(C->GetSlotPath()));
#endif
}
void UMemoriaUpgradeSmokeSubsystem::Deinitialize()
{
    if (State) { State->RestoreCapturePauseS364();State->RestoreReplayStepS364(); }
    if (State && !State->bDone) State->Finish(false, TEXT("Game instance terminated before smoke completion"));
#if UE_BUILD_DEVELOPMENT && WITH_DEV_AUTOMATION_TESTS
    if (State) { State->CleanupCsv(); MemoriaPresentationMetrics::SetEnabled(false); }
#endif
    State.Reset();
    // Never disarm isolated smoke mode: profile persistence remains disabled through process teardown.
    Super::Deinitialize();
}
bool UMemoriaUpgradeSmokeSubsystem::IsTickable() const { return !HasAnyFlags(RF_ClassDefaultObject) && State && !State->bDone; }
UWorld* UMemoriaUpgradeSmokeSubsystem::GetTickableGameObjectWorld() const { return GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr; }
void UMemoriaUpgradeSmokeSubsystem::Tick(float DeltaTime)
{
#if UE_BUILD_DEVELOPMENT && WITH_DEV_AUTOMATION_TESTS
    if (!State || State->bDone) return;
    auto& S = *State;
    if (S.LastFrame == GFrameCounter) return; S.LastFrame = GFrameCounter;
    const double Now = FPlatformTime::Seconds();
    if (!S.PollCsvStart(Now)) return;
    // Observe all game frames, including existing PNG/UI waits; do not change walkthrough cadence or gates.
    S.Performance.Observe(GetTickableGameObjectWorld(), S.Phase, !S.PendingPng.IsEmpty(), Now, S.Started);
    if (Now - S.Started > 360.) { S.Finish(false, TEXT("Packaged smoke timed out")); return; }
    const bool bS364Fast=GameplayS364Requested() && ((S.Phase==2 && S.bS364FixedStepOwned) || (S.Phase==3 && !S.bS364CombatDone));
    if (!S.PendingCaptureDone() || S.bDone || (!bS364Fast && Now - S.LastStep < .10)) return;
    S.LastStep = Now;
    UWorld* World = GetTickableGameObjectWorld();
    auto* PC = World ? Cast<AMemoriaSliceController>(World->GetFirstPlayerController()) : nullptr;
    auto* Pawn = PC ? Cast<AMemoriaFieldPawn>(PC->GetPawn()) : nullptr;
    if (!World || World->bIsTearingDown || !PC || !Pawn || World->GetTimeSeconds() < .15) return;
    auto* Game = GetGameInstance(); auto* Host = Game->GetSubsystem<UMemoriaNarrativeSubsystem>();
    auto* Run = Game->GetSubsystem<UMemoriaRunSubsystem>(); auto* C = Game->GetSubsystem<UMemoriaCheckpointSubsystem>();
    auto* Combat = World->GetSubsystem<UMemoriaFieldCombatSubsystem>();
    if (!MemoriaUpgradeSmoke::IsIsolatedSmokeActive()) { S.Finish(false, TEXT("Persistence isolation was unexpectedly disarmed")); return; }
    if ((S.Phase == 1 || S.Phase == 10 || S.Phase == 12) &&
        (Host->GetState() == EMemoriaSliceState::VN || Host->GetState() == EMemoriaSliceState::Field))
    {
        // Existing public development/source narrative fixture; not a Chapter1–5 human playthrough.
        Host->Confirm(S.Phase == 1 ? 1 : INDEX_NONE); return;
    }
    switch (S.Phase)
    {
    case 0:
        for (const TCHAR* Map : {TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice"), TEXT("/Game/Tests/Campaign/L_VerdanHost"), TEXT("/Game/Memoria/Maps/L_BeltWaystation"), TEXT("/Game/Memoria/Maps/L_DriftShelter")})
            if (!S.Check(FPackageName::DoesPackageExist(Map), FString(TEXT("Required cooked map exists: ")) + Map)) return;
        for (const TCHAR* Input : {TEXT("IMC_Foundation"), TEXT("IMC_Modal"), TEXT("IA_Move"), TEXT("IA_Confirm"), TEXT("IA_Back"), TEXT("IA_Menu")})
            if (!S.Check(FPackageName::DoesPackageExist(FString(TEXT("/Game/Tests/Foundation/")) + Input), FString(TEXT("Required cooked input asset exists: ")) + Input)) return;
        S.TravelFrom = World;
        UGameplayStatics::OpenLevel(this, TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice"), true, TEXT("UpgradeSmoke")); S.Next(); break;
    case 1:
        if (!World->GetMapName().EndsWith(TEXT("L_VerdanHost")) || Host->GetState() != EMemoriaSliceState::Exploration || !Combat || !Combat->GetPlayerFigure() || World->GetTimeSeconds() < .8) break;
        if(!S.FocusS364(*World,*PC,*Pawn,*Run,Now))break;
        S.RunId = Run->GetRunSnapshot().RunId;
        if (!S.Region(*World, *Pawn, TEXT("verdan_market"))) return;
        S.Capture(TEXT("01_Verdan")); S.Next(); break;
    case 2:
    {
        if (!S.Check(Combat && Combat->LiveMonsterCount() == 0, TEXT("Controlled field combat starts without an existing encounter"))) return;
        if(GameplayS364Requested())
        {
            if(!S.bS364FixedStepOwned){S.BeginReplayStepS364();S.S364ReplayArmedFrame=GFrameCounter;break;}
            // TickObjects has already cached this frame's pause/delta before
            // calling tickables. Arm on prior engine frames, never start a cut
            // in the old variable-delta frame which enabled fixed stepping.
            if(GFrameCounter<S.S364ReplayArmedFrame+2)break;
            if(!S.Check(FApp::UseFixedTimeStep() && FMath::Abs(FApp::GetDeltaTime()-1./60.)<1.e-5 && FMath::Abs(World->GetDeltaSeconds()-1./60.)<1.e-5,
                TEXT("S364 initial attack starts only after measured fixed engine/world steps")))return;
        }
        const auto Spawned = Combat->SpawnWave(1, Pawn->GetActorLocation() + FVector(45, 0, 0), 0.f, EMemoriaFoeKind::VoidHusk);
        if (!S.Check(Spawned.Num() == 1, TEXT("Cooked field foe spawned through actual combat subsystem"))) return;
        S.Target = Spawned[0]; S.HitsBefore = Combat->GetHitsLanded();
        if(GameplayS364Requested()){S.bS364TargetTick=Spawned[0]->IsActorTickEnabled();Spawned[0]->SetActorTickEnabled(false);S.S364At=Now;}
        auto* FoeFigure = Spawned[0]->GetFigure();
        if (!S.Check(FoeFigure && FoeFigure->IsFoeModel() && FoeFigure->GetSkeletalMesh() && FoeFigure->GetSkeletalMesh()->GetSkeletalMeshAsset(),
            TEXT("Controlled foe binds a cooked authored skeletal model"))) return;
        S.Report->SetStringField(TEXT("field_foe_mesh"), FoeFigure->GetSkeletalMesh()->GetSkeletalMeshAsset()->GetPathName());
        // Bounded close target placement makes the actual swing independent of unattended OS cursor aim.
        // It is an explicit smoke fixture, not movement/collision or tactical-AI acceptance.
        Spawned[0]->SetActorLocation(Pawn->GetActorLocation() + FVector(45, 0, 0), false, nullptr, ETeleportType::TeleportPhysics);
        Tap(PC, EKeys::J);
        if (!S.Check(Combat->IsAttacking() && Combat->GetPlayerFigure()->GetActionClip(), TEXT("Physical J starts a real loaded attack clip"))) return;
        if(GameplayS364Requested())
        {
            // PNG readback/file completion may outlast the first cut's .40 queue
            // window. Freeze ONLY this isolated capture, then restore the exact
            // previous pause state before the real-time input replay continues.
            S.S364CaptureWorld=World;S.bS364CaptureWasPaused=World->IsPaused();
            if(!S.bS364CaptureWasPaused)
            {
                S.bS364CapturePauseOwned=UGameplayStatics::SetGamePaused(World,true);
                if(!S.Check(S.bS364CapturePauseOwned,TEXT("S364 owns only the initial attack PNG simulation pause")))return;
            }
            S.Report->SetStringField(TEXT("s364_capture_timing_scope"),TEXT("The first attack simulation pauses only until its asynchronous PNG completes, then restores prior pause state. Subsequent early J replay runs at actual gameplay time; no queue threshold or attack clock is forced."));
        }
        S.Capture(TEXT("02_Attack")); S.Next(); break;
    }
    case 3:
        if(!S.CombatS364(*World,*PC,*Combat,Now))break;
        if (Combat->GetHitsLanded() <= S.HitsBefore)
        { if (Now - S.PhaseAt > 8.) S.Finish(false, TEXT("Actual field attack did not hit its nearby foe")); break; }
        if (!S.Check(S.Target.IsValid() && S.Target->GetHealth() < S.Target->GetMaxHealth(), TEXT("Actual attack damaged the field foe"))) return;
        if (Combat->IsAttacking()) break;
        Tap(PC, EKeys::R);
        if (!S.Check(Combat->IsPickingBurn() && !Combat->GetBurnChoices().IsEmpty(), TEXT("Physical R opens cooked burn picker"))) return;
        {
            int32 Choice = INDEX_NONE;
            for (int32 I = 0; I < Combat->GetBurnChoices().Num(); ++I) if (Combat->GetBurnChoices()[I].Grade >= 2) { Choice = I; break; }
            if (!S.Check(Choice != INDEX_NONE, TEXT("Source fixture owns a memory strong enough for the controlled burn"))) return;
            Combat->SelectBurn(Choice); S.BurnId = Combat->GetBurnChoices()[Choice].Id;
            S.BurnedBefore = Run->GetPlayerMemory()->GetSnapshot().BurnedHistory.Num();
        }
        if(GraphicsS363Requested())
        {
            auto* Hud=GraphicsWidget<UMemoriaCombatHudWidget>(PC);FS363OverlayHost Raised;
            const bool bRaisedHostAttached=Hud && GraphicsOuterHost(World->GetGameViewport(),Hud,Raised);
            bool Above=bRaisedHostAttached && Raised.Z==110 && UGameViewportSubsystem::Get()->GetWidgetSlot(Hud).ZOrder==100;
            if(const auto* Popup=PC->GetAchievementPopup())
            {FS363OverlayHost PopupHost;Above=Above && GraphicsOuterHost(World->GetGameViewport(),Popup,PopupHost) && Raised.Overlay==PopupHost.Overlay && Raised.Z>PopupHost.Z && Raised.Index>PopupHost.Index;}
            if(!S.Check(Above,TEXT("S363 physical R returns with actual picker outer host110 above popup BEFORE its first paint")))return;
            S.Report->SetBoolField(TEXT("s363_picker_immediate_order_verified"),true);S.Report->SetNumberField(TEXT("s363_picker_immediate_outer_z"),Raised.Z);
        }
        S.Capture(TEXT("03_BurnPicker")); S.Next(); break;
    case 4:
        Tap(PC, EKeys::Enter);
        if (Combat->IsPickingBurn() && Combat->IsBurnArmed()) Tap(PC, EKeys::Enter);
        if (!S.Check(!Combat->IsPickingBurn() && Combat->IsCasting() && Run->GetPlayerMemory()->GetSnapshot().BurnedHistory.Num() == S.BurnedBefore + 1,
            TEXT("Physical confirmation commits exactly one real memory burn"))) return;
        S.Next(); break;
    case 5:
        if (!Combat->GetBurnWave().bLive) { if (Now - S.PhaseAt > 8.) S.Finish(false, TEXT("Cooked burn never released its live ring")); break; }
        S.Capture(TEXT("04_BurnWave")); S.Next(); break;
    case 6:
        if (Combat->IsCasting() || Combat->GetBurnWave().bLive || Combat->LiveMonsterCount() > 0)
        { if (Now - S.PhaseAt > 10.) S.Finish(false, TEXT("Controlled burn did not resolve the real encounter")); break; }
        if (!S.Check(Combat->GetKills() >= 1 && Run->GetPlayerMemory()->GetSnapshot().BurnedHistory.Contains(S.BurnId), TEXT("Encounter kill and authored memory loss remain authoritative"))) return;
        if(!S.GraphicsGuard(*World,*PC,*Combat,Now))break;
        // Explicit isolated closed-boundary fixture enables the existing Verdan save contract.
        Run->SetCurrentChapter(3); Run->SetStoryFlag(TEXT("ch2_complete"), true); Run->SetStoryFlag(TEXT("ch2_arrived"), true);
        Combat->Revive(.61f);
        S.SavedMap = TEXT("verdan_market"); S.SavedPosition = Memoria::Coordinates::ToSource(Pawn->GetActorLocation());
        S.SavedHp = Run->GetRunSnapshot().Player.Hp; S.SavedGrains = Run->GetRunSnapshot().Player.Grains;
        S.SavedBurnCount = Run->GetPlayerMemory()->GetSnapshot().BurnedHistory.Num();
        Tap(PC, EKeys::Escape); Down(PC, 2); Tap(PC, EKeys::Enter);
        if (!S.Check(PC->GetSaveSlotsWidget() && PC->GetSaveSlotsWidget()->GetSelectedSlot() == 1, TEXT("Actual Pause Save opens manual1"))) return;
        Tap(PC, EKeys::Enter);
        if (!S.Check(C->GetManualSlotSummary(1).CanLoad(), (FString(TEXT("Actual UI writes isolated Verdan manual1: "))+C->GetStatusText()))) return;
        S.SavedBytes = Read(C->GetManualSlotPath(1));
        S.VerdanSavedBytes = S.SavedBytes; S.VerdanSavedHp = S.SavedHp; S.VerdanSavedGrains = S.SavedGrains;
        S.VerdanSavedBurnCount = S.SavedBurnCount; S.VerdanSavedPosition = S.SavedPosition;
        S.Capture(TEXT("05_VerdanManualSaved")); S.Next(); break;
    case 7:
        Tap(PC, EKeys::Escape); Tap(PC, EKeys::Escape);
        Run->AddGrains(7); Combat->Revive(.87f); Pawn->SetActorLocation(Pawn->GetActorLocation() + FVector(80, 0, 0));
        Tap(PC, EKeys::Escape); Down(PC, 3); Tap(PC, EKeys::Enter); Down(PC, 1);
        if (!S.Check(PC->GetSaveSlotsWidget() && PC->GetSaveSlotsWidget()->GetSelectedSlot() == 1, TEXT("Actual Pause Load explicitly selects manual1"))) return;
        Tap(PC, EKeys::Enter); S.Next(); break;
    case 8:
        if (!S.Check(Host->GetState() == EMemoriaSliceState::Deferred && Run->GetRunSnapshot().RunId == S.RunId && Run->GetRunSnapshot().Player.Hp == S.SavedHp &&
            Run->GetRunSnapshot().Player.Grains == S.SavedGrains && Memoria::Coordinates::ToSource(Pawn->GetActorLocation()).Equals(S.SavedPosition, .5) &&
            Run->GetPlayerMemory()->GetSnapshot().BurnedHistory.Num() == S.SavedBurnCount && Read(C->GetManualSlotPath(1)) == S.SavedBytes,
            TEXT("Cooked same-world manual load restores exact HP/position/run/memory without rewriting its file"))) return;
        S.Capture(TEXT("06_VerdanManualLoaded")); S.Next(); break;
    case 9:
        S.TravelFrom = World;
        if (!S.Check(Host->TravelToChapterMap(TEXT("belt_waystation")), TEXT("Existing public narrative travel requests Belt fixture"))) return;
        S.Next(); break;
    case 10:
        if (World == S.TravelFrom.Get() || !ArrivalDone(*Host, *Run, TEXT("belt_waystation")) || World->GetTimeSeconds() < .8) break;
        if (!S.Check(Run->GetRunSnapshot().RunId == S.RunId && Run->GetPlayerMemory()->GetSnapshot().BurnedHistory.Contains(S.BurnId), TEXT("Run and real memory loss survive travel to Belt")) ||
            !S.Region(*World, *Pawn, TEXT("belt_waystation"))) return;
        S.Capture(TEXT("07_Belt")); S.TravelFrom = World; S.Next(); break;
    case 11:
        // PendingCaptureDone has already verified the Belt PNG before this travel is requested.
        if (!S.Check(Host->TravelToChapterMap(TEXT("drift_shelter")), TEXT("Existing public narrative travel requests Drift fixture"))) return;
        S.Next(); break;
    case 12:
        if (World == S.TravelFrom.Get() || !ArrivalDone(*Host, *Run, TEXT("drift_shelter")) || World->GetTimeSeconds() < .8) break;
        if (!S.Region(*World, *Pawn, TEXT("drift_shelter"))) return;
        S.Capture(TEXT("08_Drift")); S.Next(); break;
    case 13:
        Combat->Revive(.53f); S.SavedMap = TEXT("drift_shelter"); S.SavedPosition = MemoriaChapterMaps::ToSource(Pawn->GetActorLocation());
        S.SavedHp = Run->GetRunSnapshot().Player.Hp; S.SavedGrains = Run->GetRunSnapshot().Player.Grains;
        S.SavedBurnCount = Run->GetPlayerMemory()->GetSnapshot().BurnedHistory.Num();
        Tap(PC, EKeys::Escape); Down(PC, 2); Tap(PC, EKeys::Enter); Down(PC, 1);
        if (!S.Check(PC->GetSaveSlotsWidget() && PC->GetSaveSlotsWidget()->GetSelectedSlot() == 2, TEXT("Actual Drift Save selects manual2"))) return;
        Tap(PC, EKeys::Enter);
        if (!S.Check(C->GetManualSlotSummary(2).CanLoad(), (FString(TEXT("Actual UI writes isolated Drift manual2: "))+C->GetStatusText()))) return;
        S.SavedBytes = Read(C->GetManualSlotPath(2)); S.Capture(TEXT("09_DriftManualSaved")); S.Next(); break;
    case 14:
        Tap(PC, EKeys::Escape); Tap(PC, EKeys::Escape);
        Run->AddGrains(7); Combat->Revive(.87f); Pawn->SetActorLocation(Pawn->GetActorLocation() + FVector(80, 0, 0));
        Tap(PC, EKeys::Escape); Down(PC, 3); Tap(PC, EKeys::Enter); Down(PC, 2);
        if (!S.Check(PC->GetSaveSlotsWidget() && PC->GetSaveSlotsWidget()->GetSelectedSlot() == 2, TEXT("Actual Drift Load explicitly selects manual2"))) return;
        S.TravelFrom = World; Tap(PC, EKeys::Enter); S.Next(); break;
    case 15:
        if (World == S.TravelFrom.Get() || Host->GetChapterMap() != S.SavedMap || Host->GetState() != EMemoriaSliceState::Exploration || World->GetTimeSeconds() < .8) break;
        if (!S.Check(Run->GetRunSnapshot().RunId == S.RunId && Run->GetRunSnapshot().Player.Hp == S.SavedHp && Run->GetRunSnapshot().Player.Grains == S.SavedGrains &&
            MemoriaChapterMaps::ToSource(Pawn->GetActorLocation()).Equals(S.SavedPosition, .5) && Run->GetPlayerMemory()->GetSnapshot().BurnedHistory.Num() == S.SavedBurnCount &&
            Read(C->GetManualSlotPath(2)) == S.SavedBytes && !PC->GetPauseWidget() && !PC->GetSaveSlotsWidget(),
            TEXT("Cooked traveled manual load restores exact state/position and releases old UI owners")) || !S.Region(*World, *Pawn, TEXT("drift_shelter"))) return;
        S.Capture(TEXT("10_DriftManualLoaded")); S.Next(); break;
    case 16:
        if (!S.bTitleVerdanProbeDone)
        {
            Tap(PC, EKeys::Escape);
            if (!S.Check(PC->GetPauseWidget() != nullptr, TEXT("Cooked Title probe opens actual Pause from restored Drift"))) return;
            Down(PC, 7);
            if (!S.Check(PC->GetPauseWidget()->GetSelected() == 7, TEXT("Physical Pause navigation selects Return to Title"))) return;
            S.TravelFrom = World; Tap(PC, EKeys::Gamepad_FaceButton_Bottom);
            S.Phase = 18; S.PhaseAt = Now; S.Write(); break;
        }
        Tap(PC, EKeys::Escape); Down(PC, 8); Tap(PC, EKeys::Enter);
        if (!S.Check(PC->GetPauseWidget() && PC->GetPauseWidget()->IsAskingQuit(), TEXT("Actual Pause Quit asks before exit"))) return;
        S.Capture(TEXT("11_ExitQuestion")); S.Next(); break;
    case 17:
        if (!S.bCsvEndRequested && !S.Check(S.Captures.Num() == (12+(GraphicsS363Requested()?1:0)+(GameplayS364Requested()?4:0)) && S.Regions.Num() == 4 && S.bTitleVerdanProbeDone,
            TEXT("All requested native PNGs, three regions, loaded Drift and actual Title-to-Verdan restoration completed"))) return;
        if (!S.FinalizeCsv(Now)) return;
        Tap(PC, EKeys::Left); Tap(PC, EKeys::Enter);
        S.Report->SetBoolField(TEXT("quit_intent_dispatched"), true);
        S.Report->SetStringField(TEXT("exit_mechanism"), TEXT("UI quit intent is intentionally suppressed by isolated smoke mode; harness requests graceful process exit with status."));
        S.Finish(true, TEXT("Cooked Development bounded render/combat/manual-save/Title-continue smoke complete")); break;
    case 18:
    {
        auto* Title = PC->GetTitleWidget();
        if (World == S.TravelFrom.Get() || !Host->IsOnTitle() || !Title || !Title->IsIntroComplete())
        { if (Now - S.PhaseAt > 15.) S.Finish(false, TEXT("Cooked Pause-to-Title did not present its real focused Title")); break; }
        if (!S.Check(FSlateApplication::IsInitialized() && Title->IsItemEnabled(1) && C->GetManualSlotSummary(1).CanLoad() &&
            !PC->GetPauseWidget() && !PC->GetSaveSlotsWidget() && !World->IsPaused(),
            TEXT("Cooked Title enables Continue and releases prior Pause/browser owners"))) return;
        SlateTap(EKeys::Gamepad_DPad_Down);
        if (!S.Check(Title->GetSelected() == 1, TEXT("Focused Slate Dpad selects actual Title Continue"))) return;
        SlateTap(EKeys::Gamepad_FaceButton_Bottom);
        if (!S.Check(PC->GetSaveSlotsWidget() && PC->GetTitleWidget() && !PC->GetTitleWidget()->GetIsEnabled(),
            TEXT("Actual cooked Title Continue opens Load browser above disabled Title"))) return;
        S.Next(); break;
    }
    case 19:
        if (!S.Check(PC->GetSaveSlotsWidget() && PC->GetSaveSlotsWidget()->GetSelectedSlot() == 0,
            TEXT("Title Load browser starts at the separate autosave row"))) return;
        Down(PC, 1);
        if (!S.Check(PC->GetSaveSlotsWidget()->GetSelectedSlot() == 1 && PC->GetSaveSlotsWidget()->IsRowEnabled(1),
            TEXT("Physical Title browser navigation selects readable Verdan manual1"))) return;
        S.TravelFrom = World; Tap(PC, EKeys::Gamepad_FaceButton_Bottom); S.Next(); break;
    case 20:
        if (World == S.TravelFrom.Get() || !World->GetMapName().EndsWith(TEXT("L_VerdanHost")) || World->GetTimeSeconds() < .8)
        { if (Now - S.PhaseAt > 15.) S.Finish(false, TEXT("Cooked Title manual1 did not travel to the real Verdan world")); break; }
        if (!S.Check(Host->GetState() == EMemoriaSliceState::Deferred && !Host->IsOnTitle() &&
            !PC->GetTitleWidget() && !PC->GetPauseWidget() && !PC->GetSaveSlotsWidget() && !World->IsPaused() &&
            Run->GetRunSnapshot().RunId == S.RunId && Run->GetRunSnapshot().Player.Hp == S.VerdanSavedHp &&
            Run->GetRunSnapshot().Player.Grains == S.VerdanSavedGrains &&
            Memoria::Coordinates::ToSource(Pawn->GetActorLocation()).Equals(S.VerdanSavedPosition, .5) &&
            Run->GetPlayerMemory()->GetSnapshot().BurnedHistory.Num() == S.VerdanSavedBurnCount &&
            Run->GetPlayerMemory()->GetSnapshot().BurnedHistory.Contains(S.BurnId) && Read(C->GetManualSlotPath(1)) == S.VerdanSavedBytes,
            TEXT("Cooked Title-to-Verdan restores exact saved state and remains free of recreated Title across world ticks"))) return;
        S.Report->SetBoolField(TEXT("title_verdan_manual_restore_verified"), true);
        S.Capture(TEXT("12_TitleVerdanLoaded")); S.Next(); break;
    case 21:
        if (!S.Check(PC->GetNarrativeWidget() && PC->GetNarrativeWidget()->SelectedOriginalIndex() == 3,
            TEXT("Restored checkpoint presents the actual Return to Verdan choice"))) return;
        S.TravelFrom = World; Tap(PC, EKeys::Gamepad_FaceButton_Bottom); S.Next(); break;
    case 22:
        if (World == S.TravelFrom.Get() || Host->GetState() != EMemoriaSliceState::Exploration || World->GetTimeSeconds() < .8)
        { if (Now - S.PhaseAt > 15.) S.Finish(false, TEXT("Physical Return to Verdan did not resume exploration for final Quit check")); break; }
        if (!S.Check(!Host->IsOnTitle() && !PC->GetTitleWidget() && !PC->GetNarrativeWidget() &&
            Run->GetRunSnapshot().RunId == S.RunId && Read(C->GetManualSlotPath(1)) == S.VerdanSavedBytes,
            TEXT("Checkpoint Return to Verdan restores field ownership without rewriting manual1"))) return;
        S.bTitleVerdanProbeDone = true;
        S.Phase = 16; S.PhaseAt = Now; S.Write(); break;
    default:
        S.Finish(false, TEXT("Unknown smoke phase")); break;
    }
#endif
}
