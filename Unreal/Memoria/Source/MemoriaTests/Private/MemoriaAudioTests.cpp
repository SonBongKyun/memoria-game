#include "Misc/AutomationTest.h"
#include "Audio/MemoriaAudioCatalog.h"
#include "Sound/SoundWave.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include <limits>
#include "Audio/MemoriaAudioSubsystem.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Save/MemoriaRunSaveGame.h"
#include "Engine/GameInstance.h"

#if WITH_DEV_AUTOMATION_TESTS
#define AUDIO_TEST(Class,Name) IMPLEMENT_SIMPLE_AUTOMATION_TEST(Class,Name,EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)

AUDIO_TEST(FAudioCatalogAssets,"Memoria.Audio.CatalogAssets")
bool FAudioCatalogAssets::RunTest(const FString&)
{
    FString Text; TSharedPtr<FJsonObject> Manifest;
    const FString Path = FPaths::ProjectDir() / TEXT("../ArtSource/Audio/manifest.json");
    if (!TestTrue(TEXT("Rendered manifest loads"), FFileHelper::LoadFileToString(Text, *Path) &&
        FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Manifest) && Manifest.IsValid())) return false;
    const auto Cues = Manifest->GetObjectField(TEXT("cues"));
    auto Load = [](const FString& Package) { return LoadObject<USoundWave>(nullptr, *MemoriaAudio::ObjectPath(Package)); };
    for (const auto& Cue : MemoriaAudio::Cues())
    {
        const auto* Wave = Load(MemoriaAudio::CuePackage(Cue.Id));
        if (!TestNotNull(*FString::Printf(TEXT("Cue asset %s"), Cue.Id), Wave)) continue;
        TestFalse(*FString::Printf(TEXT("One-shot %s does not loop"), Cue.Id), Wave->bLooping != 0);
        const double Expected = Cues->GetObjectField(Cue.Id)->GetNumberField(TEXT("duration"));
        TestTrue(*FString::Printf(TEXT("%s keeps the source duration %.3fs"), Cue.Id, Expected), FMath::IsNearlyEqual(double(Wave->GetDuration()), Expected, .002));
    }
    for (const auto& Track : MemoriaAudio::Tracks())
    {
        const auto* Wave = Load(MemoriaAudio::TrackPackage(Track.Id));
        if (!TestNotNull(*FString::Printf(TEXT("Track asset %s"), Track.Id), Wave)) continue;
        TestTrue(*FString::Printf(TEXT("%s loops"), Track.Id), Wave->bLooping != 0);
        // GetDuration() reports an indefinite length for looping waves; Duration keeps the asset length.
        if (Track.bMusic) TestTrue(*FString::Printf(TEXT("%s is a full BGM track"), Track.Id), Wave->Duration > 30.f);
        else TestTrue(*FString::Printf(TEXT("%s keeps the source loop length"), Track.Id),
            FMath::IsNearlyEqual(double(Wave->Duration), Cues->GetObjectField(Track.Id)->GetNumberField(TEXT("duration")), .002));
    }
    return !HasAnyErrors();
}

AUDIO_TEST(FAudioRouting,"Memoria.Audio.Routing")
bool FAudioRouting::RunTest(const FString&)
{
    // Gait feet plant at 0 (wrap) and 0.5; one footfall per crossing, none while between.
    TestTrue(TEXT("Wrap plants the left foot"), MemoriaAudio::CrossedFootContact(.93f, .04f));
    TestTrue(TEXT("Passing 0.5 plants the right foot"), MemoriaAudio::CrossedFootContact(.46f, .52f));
    TestFalse(TEXT("Mid-swing is silent"), MemoriaAudio::CrossedFootContact(.10f, .20f));
    TestFalse(TEXT("A stationary phase is silent"), MemoriaAudio::CrossedFootContact(.5f, .5f));
    TestFalse(TEXT("Non-finite phase is silent"), MemoriaAudio::CrossedFootContact(std::numeric_limits<float>::quiet_NaN(), .2f));
    // Shop requests keep source order; only audio requests become cues.
    TestEqual(TEXT("Shop open request plays ui_open"), MemoriaAudio::CueForRequest(TEXT("request:audio:ui_open")), FName(TEXT("ui_open")));
    TestEqual(TEXT("Purchase request plays memory_add"), MemoriaAudio::CueForRequest(TEXT("request:audio:memory_add")), FName(TEXT("memory_add")));
    TestTrue(TEXT("Achievement request is not audio"), MemoriaAudio::CueForRequest(TEXT("request:achievement:check_grains")).IsNone());
    TestTrue(TEXT("Unknown audio cue is ignored"), MemoriaAudio::CueForRequest(TEXT("request:audio:missing")).IsNone());
    TestTrue(TEXT("Prefix is case-sensitive"), MemoriaAudio::CueForRequest(TEXT("REQUEST:audio:ui_open")).IsNone());
    // Source player volumes (audio_manager.gd) and the dialogue duck level.
    TestEqual(TEXT("Steps play 9 dB under other cues"), MemoriaAudio::FindCue(TEXT("step_stone"))->VolumeDb, -12.f);
    TestEqual(TEXT("UI confirm uses the sfx player"), MemoriaAudio::FindCue(TEXT("confirm"))->VolumeDb, -3.f);
    TestEqual(TEXT("BGM base volume"), MemoriaAudio::FindTrack(TEXT("ch2_verdan"))->VolumeDb, -5.f);
    TestTrue(TEXT("Dialogue duck is -13 dB"), FMath::IsNearlyEqual(MemoriaAudio::DbToLinear(MemoriaAudio::DuckDb), .2239f, .001f));
    TestTrue(TEXT("Every cue has a package"), MemoriaAudio::CuePackage(TEXT("confirm")) == TEXT("/Game/Memoria/Audio/Sfx/S_Sfx_confirm"));
    TestTrue(TEXT("Music and ambience live apart"), MemoriaAudio::TrackPackage(TEXT("battle")).StartsWith(TEXT("/Game/Memoria/Audio/Music/")) &&
        MemoriaAudio::TrackPackage(TEXT("wind_light")).StartsWith(TEXT("/Game/Memoria/Audio/Ambient/")));
    return !HasAnyErrors();
}
AUDIO_TEST(FAudioRunReplacement,"Memoria.Audio.RunReplacement")
bool FAudioRunReplacement::RunTest(const FString&)
{
    TStrongObjectPtr<UGameInstance> Game(NewObject<UGameInstance>()); Game->Init();
    auto* Run = Game->GetSubsystem<UMemoriaRunSubsystem>();
    auto* Audio = Game->GetSubsystem<UMemoriaAudioSubsystem>();
    if (!TestNotNull(TEXT("Audio subsystem exists"), Audio)) { Game->Shutdown(); return false; }
    for (bool bRestore : {false, true})
    {
        TestTrue(TEXT("Start run"), Run->BeginStartingMemoryRun() == EMemoriaMemoryResult::Success);
        auto* Save = Run->CaptureSave();
        TestTrue(TEXT("Burn identity memory"), Run->BurnMemory(TEXT("identity_first_sword")) == EMemoriaMemoryResult::Success);
        TestTrue(TEXT("Identity burn starts drama"), Audio->IsBurnDramaActive());
        TestTrue(TEXT("Replace or restore run"), bRestore ? Run->RestoreSave(*Save) : Run->BeginStartingMemoryRun() == EMemoriaMemoryResult::Success);
        TestFalse(TEXT("Old burn drama cannot continue into replacement run"), Audio->IsBurnDramaActive());
        TestTrue(TEXT("Replacement keeps identity memory intact"), Run->GetPlayerMemory()->IsIntact(TEXT("identity_first_sword")));
    }
    Game->Shutdown(); return !HasAnyErrors();
}
#undef AUDIO_TEST
#endif
