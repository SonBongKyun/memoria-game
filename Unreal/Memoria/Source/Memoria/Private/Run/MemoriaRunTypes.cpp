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
