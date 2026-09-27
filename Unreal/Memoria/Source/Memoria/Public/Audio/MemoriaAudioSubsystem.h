#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "MemoriaAudioSubsystem.generated.h"

class UAudioComponent;
struct FMemoriaMemoryEvent;

// Source AudioManager behavior for the bounded slice: context music with crossfade,
// dialogue ducking, Verdan ambience, cue playback and the high-grade burn drama.
// Presentation only: it observes run/narrative/shop/battle state and never mutates it.
UCLASS()
class MEMORIA_API UMemoriaAudioSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    // Source cue ids (play_sfx / play_combat_sfx / play_step). Returns false for unknown cues.
    bool PlaySfx(FName Cue);
    void SetLowHealth(bool bEnabled);
    bool IsHeartbeatPlaying() const;
    FName GetMusic() const { return Music; }
    // The playing music loop's volume multiplier: its source dB times the master x bgm setting.
    float GetMusicVolumeMultiplier() const;
    FName GetAmbient() const { return Ambient; }
    bool IsMusicPlaying() const;
    bool IsAmbientPlaying() const;
    bool IsDucked() const { return bDucked; }
    bool IsBurnDramaActive() const { return DramaStage != 0; }
    // Cues requested this session, newest last (bounded), and per-cue totals, for tests and diagnostics.
    const TArray<FName>& GetRecentCues() const { return RecentCues; }
    int32 GetCueCount(FName Cue) const { const int32* Count = CueCounts.Find(Cue); return Count ? *Count : 0; }

    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickable() const override { return bReady; }
    virtual ETickableTickType GetTickableTickType() const override;
    virtual UWorld* GetTickableGameObjectWorld() const override;
private:
    UPROPERTY(Transient) TObjectPtr<UAudioComponent> HeartbeatComponent;
    UPROPERTY(Transient) TObjectPtr<UAudioComponent> MusicComponent;
    UPROPERTY(Transient) TObjectPtr<UAudioComponent> FadingMusic;
    UPROPERTY(Transient) TObjectPtr<UAudioComponent> AmbientComponent;
    UPROPERTY(Transient) TObjectPtr<UAudioComponent> FadingAmbient;
    FName Music, Ambient;
    bool bReady = false, bDucked = false, bBattleActive = false, bBattleReturning = false;
    int32 DramaStage = 0;
    double DramaStarted = 0;
    TArray<FName> RecentCues;
    TMap<FName, int32> CueCounts;
    TMap<FName, double> LastPlayed;
    void OnRunReplaced();
    void SyncContext();
    void SetLoop(FName Track, TObjectPtr<UAudioComponent>& Current, TObjectPtr<UAudioComponent>& Fading, FName& CurrentId, float FadeIn, float FadeOut);
    void SetDuck(bool bDuck);
    float LoopLevel(bool bForMusic) const;
    float Gain(bool bLoop) const;
    void ApplyGains();
    void OnMemoryEvent(const FMemoriaMemoryEvent& Event);
    void OnShopRequest(const FString& Request);
    void AdvanceDrama();
};
