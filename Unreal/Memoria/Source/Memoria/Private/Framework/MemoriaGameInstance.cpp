#include "Framework/MemoriaGameInstance.h"
#include "Validation/MemoriaUpgradeSmokeSubsystem.h"

void UMemoriaGameInstance::Init()
{
    if (!MemoriaUpgradeSmoke::BeforeGameInstanceInit()) return;
    Super::Init();
}