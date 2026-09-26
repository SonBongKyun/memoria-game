#include "Narrative/MemoriaSumpLedger.h"
#include "Run/MemoriaRunTypes.h"

namespace MemoriaSumpLedger
{
const TArray<FStep>& Steps()
{
    static const TArray<FStep> Values = {
        {TEXT("sq_sump_ledger_started"), TEXT("Talk to the Nervous Trader near the alley."), TEXT("골목 근처의 불안한 상인과 대화하기")},
        {TEXT("sq_sump_ledger_found"), TEXT("Find the hidden ledger in the Sump."), TEXT("웅덩이에 숨겨진 장부 찾기")},
        {TEXT("sq_sump_ledger_done"), TEXT("Decide: return the ledger or burn it."), TEXT("선택하기: 장부를 돌려주거나 태우거나")}
    };
    return Values;
}
FString Title(bool bKo) { return bKo ? TEXT("웅덩이의 장부") : TEXT("The Sump Ledger"); }
// is_available: chapter gate, no prerequisite flag, not yet started.
bool IsAvailable(const FMemoriaRunSnapshot& S) { return S.CurrentChapter >= ChapterRequired && !S.GetFlag(Steps()[0].Flag); }
bool IsComplete(const FMemoriaRunSnapshot& S) { return S.GetFlag(Steps().Last().Flag); }
bool IsActive(const FMemoriaRunSnapshot& S) { return S.GetFlag(Steps()[0].Flag) && !IsComplete(S); }
FString CurrentStepText(const FMemoriaRunSnapshot& S, bool bKo)
{
    for (const auto& Step : Steps()) if (!S.GetFlag(Step.Flag)) return bKo ? Step.DescKo : Step.Desc;
    return FString();
}
// The trader's body_entered branches, in source order.
ETraderAction TraderAction(const FMemoriaRunSnapshot& S)
{
    if (IsAvailable(S)) return ETraderAction::Start;
    if (IsActive(S) && S.GetFlag(Steps()[1].Flag)) return ETraderAction::Return;
    if (IsActive(S)) return ETraderAction::Remind;
    return ETraderAction::None;
}
// _grant_rewards: MemoryManager.Memory.new(id, title, desc, grade, burn_power, effect, npc).
FMemoriaMemoryDefinition RewardMemory()
{
    FMemoriaMemoryDefinition D; D.Id = TEXT("sq_debt_ash"); D.Title = TEXT("Debt Written in Ash"); D.Description = TEXT("Names and numbers, all owed, all forgotten. Someone paid dearly for this silence.");
    D.RawGrade = static_cast<EMemoriaMemoryGrade>(2); D.BurnPower = 55; D.StoryEffect = TEXT("Lose awareness of financial transactions around you."); D.RelatedNpc = TEXT("");
    D.SourcePath = TEXT("scripts/utils/side_quest.gd::sump_ledger"); D.TextId = D.Id;
    return D;
}
}
