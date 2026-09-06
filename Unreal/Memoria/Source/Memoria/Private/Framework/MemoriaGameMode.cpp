#include "Framework/MemoriaGameMode.h"
#include "Framework/MemoriaFieldPawn.h"
#include "Framework/MemoriaPlayerController.h"

AMemoriaGameMode::AMemoriaGameMode()
{
    DefaultPawnClass = AMemoriaFieldPawn::StaticClass();
    PlayerControllerClass = AMemoriaPlayerController::StaticClass();
}
