#include "Audio/MemoriaAudioSubsystem.h"
#include "Audio/MemoriaAudioCatalog.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Shop/MemoriaShopSubsystem.h"
#include "Narrative/MemoriaNarrativeSubsystem.h"
#include "Combat/MemoriaFieldCombatSubsystem.h"
#include "Settings/MemoriaSettingsSubsystem.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

namespace
{
USoundBase* LoadSound(const FString& Package)
{ return LoadObject<USoundBase>(nullptr, *MemoriaAudio::ObjectPath(Package)); }
}
void UMemoriaAudioSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    Collection.InitializeDependency<UMemoriaRunSubsystem>();
    Collection.InitializeDependency<UMemoriaShopSubsystem>();
    Collection.InitializeDependency<UMemoriaNarrativeSubsystem>();
    Collection.InitializeDependency<UMemoriaSettingsSubsystem>();
    auto* GI = GetGameInstance();
    // options_menu.gd master/bgm/sfx sliders drive the loop and one-shot players live.
    GI->GetSubsystem<UMemoriaSettingsSubsystem>()->OnChanged.AddUObject(this, &UMemoriaAudioSubsystem::ApplyGains);
    // The player memory domain is restored in place, so one binding covers every run.
    GI->GetSubsystem<UMemoriaRunSubsystem>()->GetPlayerMemory()->OnObserved.AddUObject(this, &UMemoriaAudioSubsystem::OnMemoryEvent);
    GI->GetSubsystem<UMemoriaShopSubsystem>()->OnRequestRecorded.AddUObject(this, &UMemoriaAudioSubsystem::OnShopRequest);
    GI->GetSubsystem<UMemoriaRunSubsystem>()->OnRunReplaced.AddUObject(this, &UMemoriaAudioSubsystem::OnRunReplaced);
    bReady = true;
}
void UMemoriaAudioSubsystem::Deinitialize()
{
    SetLowHealth(false);
    bReady = false;
    if (auto* GI = GetGameInstance())
    {
        if (auto* Run = GI->GetSubsystem<UMemoriaRunSubsystem>())
        {
            Run->OnRunReplaced.RemoveAll(this);
            if (Run->GetPlayerMemory()) Run->GetPlayerMemory()->OnObserved.RemoveAll(this);
        }
        if (auto* Shop = GI->GetSubsystem<UMemoriaShopSubsystem>()) Shop->OnRequestRecorded.RemoveAll(this);
        if (auto* Settings = GI->GetSubsystem<UMemoriaSettingsSubsystem>()) Settings->OnChanged.RemoveAll(this);
    }
    for (auto* Component : {MusicComponent.Get(), FadingMusic.Get(), AmbientComponent.Get(), FadingAmbient.Get()})
        if (IsValid(Component)) { Component->Stop(); Component->DestroyComponent(); }
    MusicComponent = FadingMusic = AmbientComponent = FadingAmbient = nullptr;
    Music = Ambient = NAME_None; DramaStage = 0;
    Super::Deinitialize();
}
bool UMemoriaAudioSubsystem::IsMusicPlaying() const
{ return IsValid(MusicComponent) && MusicComponent->IsPlaying(); }
bool UMemoriaAudioSubsystem::IsAmbientPlaying() const
{ return IsValid(AmbientComponent) && AmbientComponent->IsPlaying(); }
void UMemoriaAudioSubsystem::OnRunReplaced()
{
    // A save restore can retain the same RunId; observe the replacement event itself.
    SetLowHealth(false);
    DramaStage = 0; DramaStarted = 0; LastPlayed.Reset();
    bBattleActive = bBattleReturning = false;
    if (IsValid(MusicComponent)) MusicComponent->AdjustVolume(.08f, LoopLevel(true));
    if (IsValid(AmbientComponent)) AmbientComponent->AdjustVolume(.08f, 1.f);
}
bool UMemoriaAudioSubsystem::PlaySfx(FName Cue)
{
    const auto* Def = MemoriaAudio::FindCue(Cue);
    if (!Def) return false;
    const double Now = FPlatformTime::Seconds();
    // The source plays cues on one shared player: an immediate repeat restarts it, never stacks.
    if (const double* Last = LastPlayed.Find(Cue); Last && Now - *Last < .05) return true;
    LastPlayed.Add(Cue, Now);
    RecentCues.Add(Cue); if (RecentCues.Num() > 64) RecentCues.RemoveAt(0);
    ++CueCounts.FindOrAdd(Cue);
    UWorld* World = GetTickableGameObjectWorld();
    USoundBase* Sound = World ? LoadSound(MemoriaAudio::CuePackage(Cue)) : nullptr;
    if (!Sound) return true;
    const float Pitch = 1.f + FMath::FRandRange(-Def->PitchVariation, Def->PitchVariation);
    UGameplayStatics::PlaySound2D(World, Sound, MemoriaAudio::DbToLinear(Def->VolumeDb) * Gain(false), Pitch);
    return true;
}
float UMemoriaAudioSubsystem::Gain(bool bLoop) const
{
    const auto* GI = GetGameInstance();
    const auto* Settings = GI ? GI->GetSubsystem<UMemoriaSettingsSubsystem>() : nullptr;
    return Settings ? (bLoop ? Settings->MusicGain() : Settings->SfxGain()) : 1.f;
}
float UMemoriaAudioSubsystem::GetMusicVolumeMultiplier() const
{ return IsValid(MusicComponent) ? MusicComponent->VolumeMultiplier : 0.f; }
void UMemoriaAudioSubsystem::ApplyGains()
{
    // Music and ambience loops follow master x bgm; the heartbeat is an sfx player.
    auto Apply = [this](UAudioComponent* Component, FName Track, bool bLoop)
    {
        const auto* Def = MemoriaAudio::FindTrack(Track);
        if (IsValid(Component) && Def) Component->SetVolumeMultiplier(MemoriaAudio::DbToLinear(Def->VolumeDb) * Gain(bLoop));
    };
    Apply(MusicComponent, Music, true); Apply(AmbientComponent, Ambient, true);
    Apply(HeartbeatComponent, TEXT("heartbeat"), false);
}
float UMemoriaAudioSubsystem::LoopLevel(bool bForMusic) const
{
    const auto* Def = MemoriaAudio::FindTrack(Music);
    return bForMusic && bDucked && Def ? MemoriaAudio::DbToLinear(MemoriaAudio::DuckDb - Def->VolumeDb) : 1.f;
}
void UMemoriaAudioSubsystem::SetLoop(FName Track, TObjectPtr<UAudioComponent>& Current, TObjectPtr<UAudioComponent>& Fading, FName& CurrentId, float FadeIn, float FadeOut)
{
    if (Track == CurrentId && (Track.IsNone() || (IsValid(Current) && Current->IsPlaying()))) return;
    if (IsValid(Fading)) { Fading->Stop(); Fading->DestroyComponent(); }
    Fading = Current; Current = nullptr; CurrentId = Track;
    if (Fading) Fading->FadeOut(FadeOut, 0.f);
    const auto* Def = MemoriaAudio::FindTrack(Track);
    UWorld* World = GetTickableGameObjectWorld();
    USoundBase* Sound = Def && World ? LoadSound(MemoriaAudio::TrackPackage(Track)) : nullptr;
    if (!Sound) return;
    // Persist across OpenLevel: the source keeps one BGM playing through scene changes.
    Current = UGameplayStatics::CreateSound2D(World, Sound, MemoriaAudio::DbToLinear(Def->VolumeDb) * Gain(true), 1.f, 0.f, nullptr, true, false);
    if (!Current) return;
    Current->FadeIn(FadeIn, 1.f);
    if (Def->bMusic && bDucked) Current->AdjustVolume(0.f, LoopLevel(true));
}
void UMemoriaAudioSubsystem::SetDuck(bool bDuck)
{
    if (bDuck == bDucked) return;
    bDucked = bDuck;
    if (MusicComponent && DramaStage == 0) MusicComponent->AdjustVolume(MemoriaAudio::DuckFadeSeconds, LoopLevel(true));
}
void UMemoriaAudioSubsystem::SyncContext()
{
    auto* GI = GetGameInstance();
    UWorld* World = GI ? GI->GetWorld() : nullptr;
    if (!World || World->bIsTearingDown) return;
    // Source SCENE_BGM/SCENE_AMBIENT: verdan_market.tscn -> ch2_verdan + wind_light, and the
    // arrival VN declares the same BGM; battle_scene.tscn -> battle_theme with no ambience, now while field
    // foes live (S329: the turn-based battle scene is retired).
    const FString Map = World->GetMapName();
    const bool bVerdan = Map.EndsWith(TEXT("L_VerdanHost")) || Map.EndsWith(TEXT("L_Ch2VerdanSlice")) || Map.EndsWith(TEXT("L_VerdanUnseenFixture"));
    const auto* Fight = World->GetSubsystem<UMemoriaFieldCombatSubsystem>();
    const bool bActive = Fight && Fight->LiveMonsterCount() > 0;
    // battle_intro is defined but never played by the source; entering a fight uses it as an addition.
    if (bActive && !bBattleActive) PlaySfx(TEXT("battle_intro"));
    if (!bActive) SetLowHealth(false);
    bBattleActive = bActive; bBattleReturning = false;
    const auto* Narrative = GI->GetSubsystem<UMemoriaNarrativeSubsystem>();
    // main.gd _play_title_bgm: the title plays title.mp3 alone, with no ambience or duck.
    if (Narrative && Narrative->IsOnTitle())
    {
        SetLoop(FName(TEXT("title")), MusicComponent, FadingMusic, Music, MemoriaAudio::CrossfadeSeconds, MemoriaAudio::CrossfadeSeconds);
        SetLoop(FName(), AmbientComponent, FadingAmbient, Ambient, 1.5f, 1.f);
        SetDuck(false); return;
    }
    // SceneFlow.play: on the VN host a scene's declared bgm replaces the map track (Chapter 1).
    const FName Scene = Narrative && Map.EndsWith(TEXT("L_Ch2VerdanSlice")) ? Narrative->GetSceneMusic() : NAME_None;
    const bool bSceneTrack = !Scene.IsNone() && MemoriaAudio::FindTrack(Scene);
    const FName VerdanTrack(TEXT("ch2_verdan"));
    // S345: the chapter maps. SCENE_AMBIENT gives belt_waystation wind_light and drift_shelter rain. SCENE_BGM names
    // neither, so in the source the previous scene's track plays on (the title's, after a Continue; the battle
    // theme, after a fight). The port plays the source's exploration.mp3 there instead, which no ported scene uses.
    const bool bBelt = Map.EndsWith(TEXT("L_BeltWaystation")), bDrift = Map.EndsWith(TEXT("L_DriftShelter"));
    const FName MapTrack = bVerdan ? VerdanTrack : bBelt || bDrift ? FName(TEXT("exploration")) : FName();
    const FName MapAir = bVerdan || bBelt ? FName(TEXT("wind_light")) : bDrift ? FName(TEXT("rain")) : FName();
    SetLoop(bActive ? FName(TEXT("battle")) : bSceneTrack ? Scene : MapTrack, MusicComponent, FadingMusic, Music, MemoriaAudio::CrossfadeSeconds, MemoriaAudio::CrossfadeSeconds);
    SetLoop(!bActive && (!bSceneTrack || Scene == VerdanTrack) ? MapAir : FName(), AmbientComponent, FadingAmbient, Ambient, 1.5f, 1.f);
    const auto State = Narrative ? Narrative->GetState() : EMemoriaSliceState::Idle;
    // Source ducks BGM between dialogue_started and dialogue_ended.
    SetDuck(!bActive && (State == EMemoriaSliceState::VN || State == EMemoriaSliceState::Field));
}
void UMemoriaAudioSubsystem::OnMemoryEvent(const FMemoriaMemoryEvent& Event)
{
    // Source memory_manager.gd add_memory plays memory_add.
    if (Event.Kind == EMemoriaMemoryEventKind::Added) { PlaySfx(TEXT("memory_add")); return; }
    if (Event.Kind != EMemoriaMemoryEventKind::Burned || DramaStage != 0) return;
    // Source _on_memory_burned_drama: grade >= 3 (Grade 2 identity and Grade 1 core).
    const auto* Run = GetGameInstance()->GetSubsystem<UMemoriaRunSubsystem>();
    const auto* D = Run->GetPlayerMemory()->GetDefinitions().FindByPredicate([&](const auto& V){ return V.Id.Equals(Event.MemoryId, ESearchCase::CaseSensitive); });
    if (!D || static_cast<uint8>(D->RawGrade) < 3) return;
    DramaStage = 1; DramaStarted = FPlatformTime::Seconds();
    for (auto* Component : {MusicComponent.Get(), AmbientComponent.Get()}) if (Component) Component->AdjustVolume(.08f, .003f);
}
void UMemoriaAudioSubsystem::AdvanceDrama()
{
    // Source _play_burn_drama: 0.08 s duck, 0.3 s silence, rising tone while restoring 0.4 s, then burn_ignite.
    const double T = FPlatformTime::Seconds() - DramaStarted;
    if (DramaStage == 1 && T >= .38)
    {
        PlaySfx(TEXT("rising_tone"));
        if (MusicComponent) MusicComponent->AdjustVolume(.4f, LoopLevel(true));
        if (AmbientComponent) AmbientComponent->AdjustVolume(.4f, 1.f);
        DramaStage = 2;
    }
    if (DramaStage == 2 && T >= .78)
    {
        PlaySfx(TEXT("burn_ignite")); DramaStage = 0;
        // Dialogue can start/end during the restoration stage; use its latest duck state.
        if (MusicComponent) MusicComponent->AdjustVolume(MemoriaAudio::DuckFadeSeconds, LoopLevel(true));
    }
}
void UMemoriaAudioSubsystem::OnShopRequest(const FString& Request)
{
    const FName Cue = MemoriaAudio::CueForRequest(Request);
    if (!Cue.IsNone()) PlaySfx(Cue);
}
void UMemoriaAudioSubsystem::Tick(float)
{
    SyncContext();
    AdvanceDrama();
    if (FadingMusic && !FadingMusic->IsPlaying()) { FadingMusic->DestroyComponent(); FadingMusic = nullptr; }
    if (FadingAmbient && !FadingAmbient->IsPlaying()) { FadingAmbient->DestroyComponent(); FadingAmbient = nullptr; }
}
TStatId UMemoriaAudioSubsystem::GetStatId() const { RETURN_QUICK_DECLARE_CYCLE_STAT(UMemoriaAudioSubsystem, STATGROUP_Tickables); }
ETickableTickType UMemoriaAudioSubsystem::GetTickableTickType() const
{ return HasAnyFlags(RF_ClassDefaultObject) ? ETickableTickType::Never : ETickableTickType::Conditional; }
UWorld* UMemoriaAudioSubsystem::GetTickableGameObjectWorld() const
{
    const auto* GI = GetGameInstance();
    return GI ? GI->GetWorld() : nullptr;
}

void UMemoriaAudioSubsystem::SetLowHealth(bool bEnabled)
{
    if(!bEnabled){if(IsValid(HeartbeatComponent)){HeartbeatComponent->Stop();HeartbeatComponent->DestroyComponent();}HeartbeatComponent=nullptr;return;}
    if(IsHeartbeatPlaying())return;
    if(IsValid(HeartbeatComponent)){HeartbeatComponent->DestroyComponent();HeartbeatComponent=nullptr;}
    const auto* Def=MemoriaAudio::FindTrack(TEXT("heartbeat"));UWorld* World=GetTickableGameObjectWorld();
    USoundBase* Sound=Def&&World?LoadSound(MemoriaAudio::TrackPackage(TEXT("heartbeat"))):nullptr;
    if(!Sound)return;
    HeartbeatComponent=UGameplayStatics::CreateSound2D(World,Sound,MemoriaAudio::DbToLinear(Def->VolumeDb)*Gain(false),1.f,0.f,nullptr,false,false);
    if(HeartbeatComponent)HeartbeatComponent->FadeIn(.15f);
}
bool UMemoriaAudioSubsystem::IsHeartbeatPlaying() const{return IsValid(HeartbeatComponent)&&HeartbeatComponent->IsPlaying();}
