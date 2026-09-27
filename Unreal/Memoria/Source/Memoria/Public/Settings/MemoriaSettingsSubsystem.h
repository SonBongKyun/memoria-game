#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MemoriaSettingsSubsystem.generated.h"

DECLARE_MULTICAST_DELEGATE(FMemoriaSettingsChanged);

// The options_menu.gd settings the title's Options panel exposes: master, bgm and sfx volume (0..100),
// fullscreen and language. Persisted in GameUserSettings.ini outside automation; other source
// options (text speed, difficulty, accessibility) are not ported yet.
UCLASS()
class MEMORIA_API UMemoriaSettingsSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    // options_menu.gd defaults: master 80, bgm 70, sfx 80, windowed, Korean first.
    static constexpr int32 DefaultMaster = 80;
    static constexpr int32 DefaultMusic = 70;
    static constexpr int32 DefaultSfx = 80;
    int32 GetMasterVolume() const { return Master; }
    int32 GetMusicVolume() const { return Music; }
    int32 GetSfxVolume() const { return Sfx; }
    bool IsFullscreen() const { return bFullscreen; }
    const FString& GetLocale() const { return Locale; }
    void SetMasterVolume(int32 Value);
    void SetMusicVolume(int32 Value);
    void SetSfxVolume(int32 Value);
    void SetFullscreen(bool bValue);
    void SetLocale(const FString& Value);
    // Linear gains for the audio players: master x bus.
    float MusicGain() const { return Master * Music / 10000.f; }
    float SfxGain() const { return Master * Sfx / 10000.f; }
    FMemoriaSettingsChanged OnChanged;
private:
    int32 Master = DefaultMaster, Music = DefaultMusic, Sfx = DefaultSfx;
    bool bFullscreen = false, bPersist = false;
    FString Locale = TEXT("ko");
    void Changed();
    void ApplyWindowMode() const;
};
