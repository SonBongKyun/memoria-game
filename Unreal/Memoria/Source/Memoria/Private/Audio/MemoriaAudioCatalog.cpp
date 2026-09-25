#include "Audio/MemoriaAudioCatalog.h"
#include "Misc/Paths.h"

namespace MemoriaAudio
{
const TArray<FCue>& Cues()
{
    // Volumes: sfx_player -3 dB, step_player -12 dB (audio_manager.gd:91,97); layered
    // combat cues bake their per-layer dB (:686); rising tone plays at -4 dB (:1078).
    static const TArray<FCue> Values = {
        {TEXT("confirm"), -3.f, .08f}, {TEXT("cancel"), -3.f, 0.f}, {TEXT("burn"), -3.f, 0.f},
        {TEXT("hit"), -3.f, .1f}, {TEXT("heal"), -3.f, 0.f}, {TEXT("step_stone"), -12.f, .1f},
        {TEXT("shield"), -3.f, 0.f}, {TEXT("drain"), -3.f, 0.f}, {TEXT("phase_change"), -3.f, 0.f},
        {TEXT("defeat"), -3.f, 0.f}, {TEXT("enemy_die"), -3.f, .09f}, {TEXT("flee"), -3.f, 0.f},
        {TEXT("memory_add"), -3.f, 0.f}, {TEXT("ui_hover"), -3.f, .06f}, {TEXT("ui_select"), -3.f, .08f},
        {TEXT("ui_open"), -3.f, 0.f}, {TEXT("ui_close"), -3.f, 0.f}, {TEXT("battle_intro"), -3.f, 0.f},
        {TEXT("sword_slash"), 0.f, .06f}, {TEXT("burn_ignite"), 0.f, .06f}, {TEXT("shield_break"), 0.f, .06f},
        {TEXT("heal_layered"), 0.f, .06f}, {TEXT("rising_tone"), -4.f, 0.f},
    };
    return Values;
}
const TArray<FTrack>& Tracks()
{
    static const TArray<FTrack> Values = {
        {TEXT("ch2_verdan"), TEXT("assets/audio/bgm/ch2_verdan.mp3"), -5.f, true},
        {TEXT("battle"), TEXT("assets/audio/bgm/battle_theme.mp3"), -5.f, true},
        {TEXT("wind_light"), TEXT("Unreal/ArtSource/Audio/wind_light.wav"), -10.f, false},
        {TEXT("heartbeat"), TEXT("Unreal/ArtSource/Audio/heartbeat.wav"), -14.f, false},
    };
    return Values;
}
const FCue* FindCue(FName Id)
{ return Cues().FindByPredicate([&](const FCue& C){ return Id == FName(C.Id); }); }
const FTrack* FindTrack(FName Id)
{ return Tracks().FindByPredicate([&](const FTrack& T){ return Id == FName(T.Id); }); }
FString CuePackage(FName Id) { return TEXT("/Game/Memoria/Audio/Sfx/S_Sfx_") + Id.ToString(); }
FString TrackPackage(FName Id)
{
    const FTrack* T = FindTrack(Id);
    return T && T->bMusic ? TEXT("/Game/Memoria/Audio/Music/S_Bgm_") + Id.ToString() : TEXT("/Game/Memoria/Audio/Ambient/S_Amb_") + Id.ToString();
}
FString ObjectPath(const FString& Package) { return Package + TEXT(".") + FPaths::GetBaseFilename(Package); }
float DbToLinear(float Db) { return FMath::Pow(10.f, Db / 20.f); }
bool CrossedFootContact(float Previous, float Phase)
{
    if (!FMath::IsFinite(Previous) || !FMath::IsFinite(Phase)) return false;
    // Phase is fmod 1: a wrap is the left plant, passing 0.5 is the right plant.
    return Phase < Previous || (Previous < .5f && Phase >= .5f);
}
FName CueForRequest(const FString& Request)
{
    static const FString Prefix = TEXT("request:audio:");
    if (!Request.StartsWith(Prefix, ESearchCase::CaseSensitive)) return NAME_None;
    const FName Cue(*Request.RightChop(Prefix.Len()));
    return FindCue(Cue) ? Cue : NAME_None;
}
}
