#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "Tests/AutomationCommon.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Framework/MemoriaSliceHost.h"
#include "Narrative/MemoriaNarrativeSubsystem.h"
#include "Presentation/MemoriaTitleWidget.h"
#include "Presentation/MemoriaDevelopmentNarrativeWidget.h"
#include "Settings/MemoriaSettingsSubsystem.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Audio/MemoriaAudioSubsystem.h"
#include "Audio/MemoriaAudioCatalog.h"
#include "Framework/Application/SlateApplication.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "UObject/StrongObjectPtr.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTitleSettings, "Memoria.Title.Settings", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTitleSettings::RunTest(const FString&)
{
    // options_menu.gd defaults and ranges; tests never touch the player's settings file.
    TStrongObjectPtr<UGameInstance> Game(NewObject<UGameInstance>()); Game->Init();
    auto* Settings = Game->GetSubsystem<UMemoriaSettingsSubsystem>();
    TestEqual(TEXT("Master default"), Settings->GetMasterVolume(), 80);
    TestEqual(TEXT("BGM default"), Settings->GetMusicVolume(), 70);
    TestEqual(TEXT("SFX default"), Settings->GetSfxVolume(), 80);
    TestFalse(TEXT("Windowed default"), Settings->IsFullscreen());
    TestEqual(TEXT("Korean first"), Settings->GetLocale(), FString(TEXT("ko")));
    TestTrue(TEXT("Music gain is master x bgm"), FMath::IsNearlyEqual(Settings->MusicGain(), .56f));
    int32 Changes = 0; Settings->OnChanged.AddLambda([&] { ++Changes; });
    Settings->SetMasterVolume(130); TestEqual(TEXT("Clamped high"), Settings->GetMasterVolume(), 100);
    Settings->SetSfxVolume(-20); TestEqual(TEXT("Clamped low"), Settings->GetSfxVolume(), 0);
    Settings->SetLocale(TEXT("fr")); TestEqual(TEXT("Unknown language falls back to Korean"), Settings->GetLocale(), FString(TEXT("ko")));
    Settings->SetLocale(TEXT("en")); TestEqual(TEXT("English"), Settings->GetLocale(), FString(TEXT("en")));
    TestEqual(TEXT("Every change is observed"), Changes, 4);
    FString Stored;
    TestFalse(TEXT("Automation does not persist settings"), GConfig->GetString(TEXT("/Script/Memoria.MemoriaSettings"), TEXT("Locale"), Stored, GGameUserSettingsIni) && Stored == TEXT("en"));
    Game->Shutdown();
    return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTitleMenu, "Memoria.Title.Menu", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTitleMenu::RunTest(const FString&)
{
    // main.gd menu order and focus; the options panel edits the settings it shows.
    TStrongObjectPtr<UGameInstance> Game(NewObject<UGameInstance>()); Game->Init();
    auto* Settings = Game->GetSubsystem<UMemoriaSettingsSubsystem>();
    auto* Title = CreateWidget<UMemoriaTitleWidget>(Game.Get()); Title->TakeWidget();
    TArray<EMemoriaTitleAction> Actions; Title->OnAction.BindLambda([&](EMemoriaTitleAction A) { Actions.Add(A); });
    Title->Configure(false, Settings);
    TestEqual(TEXT("Focus starts on New Game"), Title->GetSelected(), 0);
    TestFalse(TEXT("Continue disabled without a save"), Title->IsItemEnabled(1));
    const FString Korean = Title->VisibleText();
    for (const TCHAR* Line : {TEXT("01    새 게임"), TEXT("02    이어하기"), TEXT("03    옵션"), TEXT("04    게임 종료"), TEXT("MEMORIA"), TEXT("망각의 대가"), TEXT("기억의 문을 연다")})
        TestTrue(*(FString(TEXT("Title shows ")) + Line), Korean.Contains(Line));
    Title->Navigate(1); TestEqual(TEXT("Down skips the disabled Continue"), Title->GetSelected(), 2);
    Title->Navigate(-1); TestEqual(TEXT("Up skips it too"), Title->GetSelected(), 0);
    Title->Navigate(-1); TestEqual(TEXT("Focus wraps"), Title->GetSelected(), 3);
    Title->ActivateItem(1); TestEqual(TEXT("Disabled Continue does nothing"), Actions.Num(), 0);
    Title->ActivateItem(2); TestTrue(TEXT("Options opens"), Title->IsOptionsOpen());
    Title->Adjust(1); TestEqual(TEXT("Master +10"), Settings->GetMasterVolume(), 90);
    Title->Navigate(1); Title->Adjust(-1); TestEqual(TEXT("BGM -10"), Settings->GetMusicVolume(), 60);
    Title->ActivateOption(4, 0); TestEqual(TEXT("Language toggles"), Settings->GetLocale(), FString(TEXT("en")));
    TestTrue(TEXT("Menu relabels in English"), Title->VisibleText().Contains(TEXT("01    New Game")));
    Title->ConfirmIntent(); TestTrue(TEXT("Confirm on a slider leaves it open"), Title->IsOptionsOpen());
    TestTrue(TEXT("Back closes Options"), Title->Back()); TestFalse(TEXT("Options closed"), Title->IsOptionsOpen());
    TestEqual(TEXT("Focus returns to Options"), Title->GetSelected(), 2);
    TestFalse(TEXT("Back on the menu does nothing"), Title->Back());
    Title->Configure(true, Settings); TestTrue(TEXT("Continue enabled with a save"), Title->IsItemEnabled(1));
    Title->Navigate(1); TestEqual(TEXT("Down reaches Continue"), Title->GetSelected(), 1);
    Title->ConfirmIntent(); Title->ActivateItem(0); Title->ActivateItem(3);
    TestTrue(TEXT("Actions in order"), Actions.Num() == 4 && Actions[0] == EMemoriaTitleAction::Options && Actions[1] == EMemoriaTitleAction::Continue
        && Actions[2] == EMemoriaTitleAction::NewGame && Actions[3] == EMemoriaTitleAction::Quit);
    TestNotNull(TEXT("Title key art"), Title->DisplayedArtwork());
    Title->AdvancePresentation(UMemoriaTitleWidget::IntroSeconds); TestTrue(TEXT("Intro completes"), Title->IsIntroComplete());
    Game->Shutdown();
    return !HasAnyErrors();
}
namespace
{
// The game's main scene in PIE: the title with real keys, its Options, then New Game into Chapter 1.
class FTitleJourney final : public IAutomationLatentCommand
{
public:
    explicit FTitleJourney(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
    ~FTitleJourney() override { if (bFixed) { FApp::SetUseFixedTimeStep(bOldFixed); FApp::SetFixedDeltaTime(OldDelta); } }
    bool Update() override
    {
        if (LastFrame == GFrameCounter) return false;
        LastFrame = GFrameCounter;
        if (FPlatformTime::Seconds() - Started > 240) { Test->AddError(FString::Printf(TEXT("Title journey timeout in phase %d"), Phase)); return true; }
        UWorld* World = GEditor->PlayWorld;
        auto* PC = World ? Cast<AMemoriaSliceController>(World->GetFirstPlayerController()) : nullptr;
        if (!PC || World->GetTimeSeconds() < .3) return false;
        auto* GI = World->GetGameInstance();
        auto* Host = GI->GetSubsystem<UMemoriaNarrativeSubsystem>(); auto* Audio = GI->GetSubsystem<UMemoriaAudioSubsystem>();
        auto* Settings = GI->GetSubsystem<UMemoriaSettingsSubsystem>();
        if (Phase == 0)
        {
            bFixed = true; bOldFixed = FApp::UseFixedTimeStep(); OldDelta = FApp::GetFixedDeltaTime();
            FApp::SetUseFixedTimeStep(true); FApp::SetFixedDeltaTime(1.0 / 60.0);
            FirstWorld = World; Phase = 1;
            // GameMapsSettings LocalMapOptions opens the default map with ?Title.
            UGameplayStatics::OpenLevel(World, TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice"), true, TEXT("Title"));
            return false;
        }
        if (World == FirstWorld.Get() || World->GetTimeSeconds() == LastWorldTime) return false;
        LastWorldTime = World->GetTimeSeconds(); ++Frame;
        auto Capture = [&](const TCHAR* Name) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Validation/Title") / (FString(Name) + TEXT(".png")), true, false); };
        auto Key = [&](FKey K)
        {
            FSlateApplication::Get().ProcessKeyDownEvent(FKeyEvent(K, FModifierKeysState(), 0, false, 0, 0));
            FSlateApplication::Get().ProcessKeyUpEvent(FKeyEvent(K, FModifierKeysState(), 0, false, 0, 0));
        };
        auto* Title = PC->GetTitleWidget();
        if (Phase == 1)
        {
            if (!Host->IsOnTitle() || !Title || !Title->IsIntroComplete() || Frame < 100) return false;
            Test->TestEqual(TEXT("Title plays title.mp3"), Audio->GetMusic(), FName(TEXT("title")));
            Test->TestFalse(TEXT("No ambience on the title"), Audio->IsAmbientPlaying());
            Test->TestFalse(TEXT("No save, Continue disabled"), Title->IsItemEnabled(1));
            Test->TestNotNull(TEXT("Key art rendered"), Title->DisplayedArtwork());
            Test->TestEqual(TEXT("Nothing runs behind the title"), Host->GetState(), EMemoriaSliceState::Idle);
            Capture(TEXT("Title")); Phase = 2; Mark = Frame; return false;
        }
        if (Phase == 2 && Frame == Mark + 12)
        {
            Key(EKeys::Down); Test->TestEqual(TEXT("Down skips Continue to Options"), Title->GetSelected(), 2);
            Key(EKeys::Enter); Test->TestTrue(TEXT("Enter opens Options"), Title->IsOptionsOpen());
            const float Before = Audio->GetMusicVolumeMultiplier();
            Key(EKeys::Down); Key(EKeys::Left);
            Test->TestEqual(TEXT("Left lowers the BGM volume"), Settings->GetMusicVolume(), 60);
            const float Expected = MemoriaAudio::DbToLinear(-5.f) * .8f * .6f;
            Test->TestTrue(FString::Printf(TEXT("The playing music follows the setting (%.4f -> %.4f, want %.4f)"), Before, Audio->GetMusicVolumeMultiplier(), Expected),
                FMath::IsNearlyEqual(Audio->GetMusicVolumeMultiplier(), Expected, .001f));
            Phase = 3; Mark = Frame; return false;
        }
        if (Phase == 3 && Frame == Mark + 8) { Capture(TEXT("TitleOptions")); Phase = 4; Mark = Frame; return false; }
        if (Phase == 4 && Frame == Mark + 8)
        {
            Key(EKeys::Escape); Test->TestFalse(TEXT("Escape closes Options"), Title->IsOptionsOpen());
            Key(EKeys::Up); Test->TestEqual(TEXT("Up skips Continue to New Game"), Title->GetSelected(), 0);
            Capture(TEXT("TitleFocus")); Phase = 5; Mark = Frame; return false;
        }
        if (Phase == 5 && Frame == Mark + 8) { Key(EKeys::Enter); Phase = 6; Mark = Frame; return false; }
        if (Phase == 6)
        {
            if (Host->IsOnTitle() || Host->GetState() != EMemoriaSliceState::VN || !PC->GetNarrativeWidget()) return false;
            Test->TestNull(TEXT("The title is gone"), PC->GetTitleWidget());
            Test->TestTrue(TEXT("New Game route"), Host->IsNewGameRoute());
            Test->TestTrue(TEXT("Starts at the cold open"), Host->GetView().CueKey == TEXT("ch1_cold_open:0"));
            Test->TestEqual(TEXT("New Game uses the language setting"), GI->GetSubsystem<UMemoriaRunSubsystem>()->GetRunSnapshot().CurrentLocale, FString(TEXT("ko")));
            Phase = 7; Mark = Frame; return false;
        }
        if (Phase == 7 && Frame == Mark + 45)
        {
            Test->TestTrue(TEXT("The Enter that started the game did not advance the first line"), Host->GetView().CueKey == TEXT("ch1_cold_open:0"));
            Test->TestNotEqual(TEXT("Title music handed over"), Audio->GetMusic(), FName(TEXT("title")));
            Capture(TEXT("TitleNewGame")); Phase = 8; Mark = Frame; return false;
        }
        return Phase == 8 && Frame > Mark + 6;
    }
private:
    FAutomationTestBase* Test;
    double Started, OldDelta = 0, LastWorldTime = -1;
    uint64 LastFrame = MAX_uint64;
    int32 Phase = 0, Frame = 0, Mark = 0;
    bool bFixed = false, bOldFixed = false;
    TWeakObjectPtr<UWorld> FirstWorld;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTitleScreenTest, "MemoriaVisual.TitleScreen", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTitleScreenTest::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice"))) return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FTitleJourney(this)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
