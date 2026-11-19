#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MemoriaSettingsSubsystem.generated.h"

DECLARE_MULTICAST_DELEGATE(FMemoriaSettingsChanged);

// The options_menu.gd settings the title's Options panel exposes: master, bgm and sfx volume (0..100),
// fullscreen and language, plus bounded S362 presentation accessibility. Persisted
// outside automation. Difficulty/gameplay timing are independent of these options.
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
    bool IsReduceMotion() const { return bReduceMotion; }
    bool IsReduceFlash() const { return bReduceFlash; }
    int32 GetShakePercent() const { return ShakePercent; }
    int32 GetTextPercent() const { return TextPercent; }
    void SetReduceMotion(bool bValue);
    void SetReduceFlash(bool bValue);
    void SetShakePercent(int32 Value);
    void SetTextPercent(int32 Value);
    // Default values reproduce the existing presentation exactly. Consumers
    // apply these to decoration/shake/flash/text, never animation or combat time.
    float MotionScale() const { return bReduceMotion ? 0.f : 1.f; }
    float ShakeScale() const { return bReduceMotion ? 0.f : ShakePercent / 100.f; }
    float FlashScale() const { return bReduceFlash ? .15f : 1.f; }
    float TextScale() const { return TextPercent / 100.f; }
    int32 ScaledTextSize(int32 Base) const { return FMath::RoundToInt(Base * TextScale()); }
    // Linear gains for the audio players: master x bus.
    float MusicGain() const { return Master * Music / 10000.f; }
    float SfxGain() const { return Master * Sfx / 10000.f; }
    FMemoriaSettingsChanged OnChanged;
private:
    int32 Master = DefaultMaster, Music = DefaultMusic, Sfx = DefaultSfx;
    bool bFullscreen = false, bPersist = false;
    bool bReduceMotion = false, bReduceFlash = false;
    int32 ShakePercent = 100, TextPercent = 100;
    FString Locale = TEXT("ko");
    void Changed();
    void ApplyWindowMode() const;
};
