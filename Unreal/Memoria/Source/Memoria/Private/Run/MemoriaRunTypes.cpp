#include "Run/MemoriaRunTypes.h"
#include "Misc/Crc.h"

namespace
{
// Default FString set keys compare without case; source IDs distinguish case.
struct FCaseSensitiveStringKeys : BaseKeyFuncs<FString, FString, false>
{
    static const FString& GetSetKey(const FString& Value) { return Value; }
    static bool Matches(const FString& A, const FString& B) { return A.Equals(B, ESearchCase::CaseSensitive); }
    static uint32 GetKeyHash(const FString& Value) { return FCrc::StrCrc32(*Value); }
};
}

bool FMemoriaRunSnapshot::HasFlag(const FString& Id) const
{
    return StoryFlags.ContainsByPredicate([&](const auto& F) { return F.Id.Equals(Id, ESearchCase::CaseSensitive); });
}
bool FMemoriaRunSnapshot::GetFlag(const FString& Id) const
{
    const auto* Flag = StoryFlags.FindByPredicate([&](const auto& F) { return F.Id.Equals(Id, ESearchCase::CaseSensitive); });
    return Flag && Flag->bValue;
}
FMemoriaMemoryContext FMemoriaRunSnapshot::MemoryContext() const
{
    FMemoriaMemoryContext C;
    C.CurrentChapter = CurrentChapter; C.bEliaWithParty = Player.bEliaWithParty;
    C.bStillHandsActive = GetFlag(TEXT("oath_still_sworn")) && !GetFlag(TEXT("oath_still_broken"));
    return C;
}
bool FMemoriaRunSnapshot::IsSourceItem(const FString& Id)
{
    for (const TCHAR* Known : {TEXT("potion"),TEXT("hi_potion"),TEXT("antidote"),TEXT("firebomb"),TEXT("smoke_bomb"),TEXT("witness_ink"),TEXT("root_balm"),TEXT("signal_jammer"),TEXT("lantern_salve"),TEXT("name_thread"),TEXT("compass_shard"),TEXT("seed_capsule"),TEXT("anchor_lantern"),TEXT("ledger_chalk"),TEXT("cinder_vial"),TEXT("witness_knot")})
        if (Id.Equals(Known, ESearchCase::CaseSensitive)) return true;
    return false;
}
TArray<FString> FMemoriaRunSnapshot::NormalizedRecentItems() const
{
    TArray<FString> Recent;
    for (const auto& Value : Player.RecentItems)
    {
        if (IsSourceItem(Value) && !Recent.ContainsByPredicate([&](const auto& R) { return R.Equals(Value, ESearchCase::CaseSensitive); })) Recent.Add(Value);
        if (Recent.Num() >= 5) break;
    }
    return Recent;
}
void FMemoriaRunSnapshot::RecordRecentItem(const FString& Id)
{
    auto Recent = NormalizedRecentItems();
    Recent.RemoveAll([&](const auto& R) { return R.Equals(Id, ESearchCase::CaseSensitive); });
    Recent.Insert(Id, 0); if (Recent.Num() > 5) Recent.SetNum(5);
    Player.RecentItems = MoveTemp(Recent);
}
bool FMemoriaRunSnapshot::IsValid() const
{
    if (!RunId.IsValid() || ContentRevision.IsEmpty()) { return false; }
    TSet<FString, FCaseSensitiveStringKeys> Ids;
    for (const auto& Flag : StoryFlags)
    {
        if (Flag.Id.IsEmpty() || Ids.Contains(Flag.Id)) { return false; }
        Ids.Add(Flag.Id);
    }
    Ids.Reset();
    for (const auto& Item : Player.Items)
    {
        if (Item.Id.IsEmpty() || Ids.Contains(Item.Id)) { return false; }
        Ids.Add(Item.Id);
    }
    return true;
}
