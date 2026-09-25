#pragma once
#include "CoreMinimal.h"

// Source AudioManager (scripts/systems/audio_manager.gd) cue and track table.
// Cue ids keep the source play_sfx / play_combat_sfx names so battle code can
// call the same identifiers. Sounds are rendered by Unreal/Tools/generate_audio_sources.py.
namespace MemoriaAudio
{
struct FCue
{
    const TCHAR* Id;
    float VolumeDb;        // source player volume (sfx -3, step -12, layered 0 = baked per layer)
    float PitchVariation;  // source SFX_PITCH_VARIATION / combat layer +-0.06
};
struct FTrack
{
    const TCHAR* Id;
    const TCHAR* Source;   // source file or generator, for provenance
    float VolumeDb;        // BGM -5, ambient -10, heartbeat -14
    bool bMusic;
};
MEMORIA_API const TArray<FCue>& Cues();
MEMORIA_API const TArray<FTrack>& Tracks();
MEMORIA_API const FCue* FindCue(FName Id);
MEMORIA_API const FTrack* FindTrack(FName Id);
MEMORIA_API FString CuePackage(FName Id);    // /Game/Memoria/Audio/Sfx/S_Sfx_<id>
MEMORIA_API FString TrackPackage(FName Id);  // /Game/Memoria/Audio/Music|Ambient/S_<Bgm|Amb>_<id>
MEMORIA_API FString ObjectPath(const FString& Package);
MEMORIA_API float DbToLinear(float Db);
// Source BGM ducking during dialogue: -13 dB against the -5 dB base.
inline constexpr float DuckDb = -13.f, CrossfadeSeconds = .8f, DuckFadeSeconds = .3f;
// Gait feet plant at phase 0 (left) and 0.5 (right); a crossing is one footfall.
MEMORIA_API bool CrossedFootContact(float PreviousPhase, float Phase);
// "request:audio:<cue>" recorded by the shop in source order -> cue, else NAME_None.
MEMORIA_API FName CueForRequest(const FString& Request);
}
