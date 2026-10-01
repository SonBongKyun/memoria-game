#include "Modules/ModuleManager.h"
#include "Editor.h"
#include "Settings/LevelEditorPlaySettings.h"
#include "Misc/CommandLine.h"
#include "Misc/CoreDelegates.h"
#include "Misc/Parse.h"

// S335: under -MemoriaCapture every play session opens at 1280x720. The editor saves the play window's size
// back on each close, a title bar smaller every time, so captures shrank through a suite and from run to run
// (the saved size had crept down to 1208x240).
class FMemoriaTestsModule final : public FDefaultModuleImpl
{
public:
    virtual void StartupModule() override
    {
        if (!GIsEditor || !FParse::Param(FCommandLine::Get(), TEXT("MemoriaCapture"))) return;
        // A play request copies the settings when it is queued, before PreBeginPIE, so the size is put back on
        // every frame between sessions.
        Handle = FCoreDelegates::OnBeginFrame.AddLambda([]
        {
            if (!GEditor || GEditor->PlayWorld) return;
            auto* Settings = GetMutableDefault<ULevelEditorPlaySettings>();
            Settings->NewWindowWidth = 1280; Settings->NewWindowHeight = 720;
        });
    }
    virtual void ShutdownModule() override { if (Handle.IsValid()) FCoreDelegates::OnBeginFrame.Remove(Handle); }
private:
    FDelegateHandle Handle;
};
IMPLEMENT_MODULE(FMemoriaTestsModule, MemoriaTests);
