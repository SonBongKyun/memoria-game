#include "Settings/MemoriaSettingsSubsystem.h"
#include "GameFramework/GameUserSettings.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/App.h"
#include "Engine/Engine.h"
namespace
{
const TCHAR* Section = TEXT("/Script/Memoria.MemoriaSettings");
}
void UMemoriaSettingsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    // Tests and commandlets never read or write the player's settings file.
    bPersist = !GIsAutomationTesting && !IsRunningCommandlet() && !FApp::IsUnattended();
    if (!bPersist || !GConfig) return;
    GConfig->GetInt(Section, TEXT("MasterVolume"), Master, GGameUserSettingsIni);
    GConfig->GetInt(Section, TEXT("MusicVolume"), Music, GGameUserSettingsIni);
    GConfig->GetInt(Section, TEXT("SfxVolume"), Sfx, GGameUserSettingsIni);
    GConfig->GetBool(Section, TEXT("Fullscreen"), bFullscreen, GGameUserSettingsIni);
    GConfig->GetString(Section, TEXT("Locale"), Locale, GGameUserSettingsIni);
    Master = FMath::Clamp(Master, 0, 100); Music = FMath::Clamp(Music, 0, 100); Sfx = FMath::Clamp(Sfx, 0, 100);
    if (Locale != TEXT("en") && Locale != TEXT("ko")) Locale = TEXT("ko");
    ApplyWindowMode();
}
void UMemoriaSettingsSubsystem::SetMasterVolume(int32 Value) { Master = FMath::Clamp(Value, 0, 100); Changed(); }
void UMemoriaSettingsSubsystem::SetMusicVolume(int32 Value) { Music = FMath::Clamp(Value, 0, 100); Changed(); }
void UMemoriaSettingsSubsystem::SetSfxVolume(int32 Value) { Sfx = FMath::Clamp(Value, 0, 100); Changed(); }
void UMemoriaSettingsSubsystem::SetFullscreen(bool bValue) { bFullscreen = bValue; ApplyWindowMode(); Changed(); }
void UMemoriaSettingsSubsystem::SetLocale(const FString& Value) { Locale = Value == TEXT("en") ? TEXT("en") : TEXT("ko"); Changed(); }
void UMemoriaSettingsSubsystem::Changed()
{
    if (bPersist && GConfig)
    {
        GConfig->SetInt(Section, TEXT("MasterVolume"), Master, GGameUserSettingsIni);
        GConfig->SetInt(Section, TEXT("MusicVolume"), Music, GGameUserSettingsIni);
        GConfig->SetInt(Section, TEXT("SfxVolume"), Sfx, GGameUserSettingsIni);
        GConfig->SetBool(Section, TEXT("Fullscreen"), bFullscreen, GGameUserSettingsIni);
        GConfig->SetString(Section, TEXT("Locale"), *Locale, GGameUserSettingsIni);
        GConfig->Flush(false, GGameUserSettingsIni);
    }
    OnChanged.Broadcast();
}
void UMemoriaSettingsSubsystem::ApplyWindowMode() const
{
    // Only a standalone game owns its window; the editor and tests keep theirs.
    if (!bPersist || GIsEditor || !GEngine) return;
    if (UGameUserSettings* User = GEngine->GetGameUserSettings())
    {
        User->SetFullscreenMode(bFullscreen ? EWindowMode::WindowedFullscreen : EWindowMode::Windowed);
        User->ApplySettings(false);
    }
}
