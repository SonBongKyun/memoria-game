#pragma once
#include "CoreMinimal.h"
#include "Domain/MemoriaMemoryTypes.h"
struct FMemoriaRunSnapshot;

// Source scripts/utils/side_quest.gd "sump_ledger" and verdan_market.gd _setup_side_quests.
// Quest state lives only in story flags, as in the source; this namespace is pure.
namespace MemoriaSumpLedger
{
    struct FStep { const TCHAR* Flag; const TCHAR* Desc; const TCHAR* DescKo; };
    enum class ETraderAction : uint8 { None, Start, Return, Remind };
    MEMORIA_API const TArray<FStep>& Steps();
    MEMORIA_API FString Title(bool bKo);
    MEMORIA_API bool IsAvailable(const FMemoriaRunSnapshot& S);
    MEMORIA_API bool IsActive(const FMemoriaRunSnapshot& S);
    MEMORIA_API bool IsComplete(const FMemoriaRunSnapshot& S);
    MEMORIA_API FString CurrentStepText(const FMemoriaRunSnapshot& S, bool bKo);
    MEMORIA_API ETraderAction TraderAction(const FMemoriaRunSnapshot& S);
    MEMORIA_API FMemoriaMemoryDefinition RewardMemory();
    constexpr int64 ChapterRequired = 3, RewardGrains = 40, RewardItemCount = 1;
    inline const TCHAR* RewardItem = TEXT("hi_potion");
    inline const TCHAR* TraderPoint = TEXT("sump_ledger_trader");
    inline const TCHAR* LedgerPoint = TEXT("sump_ledger_ledger");
    // Source Area2D rects (pixels) for parity; development-courtyard placements for play.
    inline const FIntRect TraderSourceRect(56, 504, 104, 552), LedgerSourceRect(384, 384, 416, 416);
    inline const FVector TraderLocation(-640, -420, 0), LedgerLocation(760, -500, 0);
}
