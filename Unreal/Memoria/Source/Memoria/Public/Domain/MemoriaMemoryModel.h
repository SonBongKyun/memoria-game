#pragma once

// Engine-independent authoritative rules. The native test build defines
// MEMORIA_API empty; UBT supplies the module export macro in Unreal builds.
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Memoria::Memory
{
enum class Grade : std::uint8_t { Grade5 = 0, Grade4 = 1, Grade3 = 2, Grade2 = 3, Grade1 = 4 };
enum class BurnMode : std::uint8_t { Normal, Silent };
enum class Result : std::uint8_t { Success, Missing, AlreadyBurned, Faded, Collateral, Busy, InvalidSnapshot };
enum class EventKind : std::uint8_t { Added, ResidueCreated, Faded, Cascaded, Burned, CarryChanged, PassiveUnlocked, MemoriesEroded };

struct Definition
{
    std::string Id;
    std::string Title;
    std::string Description;
    Grade RawGrade = Grade::Grade5;
    std::int64_t BurnPower = 0;
    std::string StoryEffect;
    std::string RelatedNpc;
};
struct State
{
    std::string Id;
    bool Burned = false;
    bool Residue = false;
    bool Faded = false;
    std::int64_t Erosion = 0;
    std::vector<std::string> Connections;
};
struct NamedFlag { std::string Id; bool Value = true; };
struct Loan
{
    bool Active = false;
    std::string MemoryId;
    std::int64_t Principal = 0;
    std::int64_t Repay = 0;
    std::int64_t DueChapter = 0;
};
struct Snapshot
{
    std::vector<State> Owned;
    std::vector<std::string> BurnedHistory;
    std::vector<NamedFlag> BurnPassives;
    std::int64_t AnchorVigil = 0;
    std::vector<NamedFlag> AnchorPassives;
    std::vector<std::int64_t> VigilChapters;
    std::vector<std::string> ErosionGuarded;
    std::int64_t GuardSlotsUsed = 0;
    Loan ActiveLoan;
    std::vector<std::string> Extracted;
};
struct Context
{
    std::int64_t CurrentChapter = 1;
    bool EliaWithParty = true;
    bool StillHandsActive = false;
};
struct Event
{
    EventKind Kind = EventKind::Added;
    std::string MemoryId;
    std::vector<std::string> AffectedIds;
    std::int64_t Amount = 0;
    std::int64_t CarryWeight = 0;
    std::int64_t CarryCapacity = 0;
    std::string PassiveId;
    std::string PassiveName;
};
using Observer = std::function<void(const Event&)>;

class MEMORIA_API Model
{
public:
    // Atomic bootstrap/UE snapshot restore, not the legacy Godot JSON importer.
    Result Restore(const std::vector<Definition>& Definitions, const Snapshot& Value);
    Result Add(const Definition& Value, const Context& Runtime, const Observer& Notify = {});
    Result CanBurn(const std::string& Id, bool AllowFaded = false) const;
    Result Burn(const std::string& Id, BurnMode Mode, bool AllowFaded, const Context& Runtime, const Observer& Notify = {});
    Result ApplyErosion(std::int64_t Chapter, const Context& Runtime, const Observer& Notify = {});
    Result EvaluatePassives(const Observer& Notify = {});

    const Snapshot& GetSnapshot() const { return Data; }
    const std::vector<Definition>& GetDefinitions() const { return Catalog; }
    const State* Find(const std::string& Id) const;
    const Definition* FindDefinition(const std::string& Id) const;
    const State* FindResidue(const std::string& Id) const;
    bool IsIntact(const std::string& Id) const;
    bool IsCollateral(const std::string& Id) const;
    bool HasPassive(const std::string& Id) const;
    std::vector<std::string> Available(Grade Minimum = Grade::Grade5, bool AllowFaded = false) const;
    std::int64_t EffectiveBurnPower(const std::string& Id) const;
    std::int64_t CarryWeight() const;
    static std::int64_t WeightOf(Grade Value);
    static std::int64_t CarryCapacity(std::int64_t Chapter);
    double CarryOverload(const Context& Runtime) const;
    std::int64_t VoluntaryBurnCount() const;
    static std::int32_t DisplayRank(Grade Value) { return static_cast<std::int32_t>(Value) + 1; }

private:
    std::vector<Definition> Catalog;
    Snapshot Data;
    bool Mutating = false;
    State* FindMutable(const std::string& Id);
    void RefreshConnections();
    void Cascade(const std::string& SourceId, const Observer& Notify);
    void UnlockPassives(const Observer& Notify);
    void NotifyCarry(const Context& Runtime, const Observer& Notify) const;
};
}
