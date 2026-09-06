#include "Domain/MemoriaMemoryModel.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <utility>

namespace Memoria::Memory
{
namespace
{
template <typename T> bool Contains(const std::vector<T>& Values, const T& Value)
{
    return std::find(Values.begin(), Values.end(), Value) != Values.end();
}
void Emit(const Observer& Notify, const Event& Value) { if (Notify) { Notify(Value); } }
struct MutationScope
{
    bool& Flag;
    explicit MutationScope(bool& Value) : Flag(Value) { Flag = true; }
    ~MutationScope() { Flag = false; }
};
void Link(State& A, State& B)
{
    if (A.Id == B.Id) { return; }
    if (!Contains(A.Connections, B.Id)) { A.Connections.push_back(B.Id); }
    if (!Contains(B.Connections, A.Id)) { B.Connections.push_back(A.Id); }
}
}

Result Model::Restore(const std::vector<Definition>& Definitions, const Snapshot& Value)
{
    if (Mutating) { return Result::Busy; }
    if (Definitions.size() != Value.Owned.size()) { return Result::InvalidSnapshot; }
    std::vector<std::string> Ids;
    for (std::size_t I = 0; I < Definitions.size(); ++I)
    {
        const auto& Def = Definitions[I];
        if (Def.Id.empty() || Contains(Ids, Def.Id) || Def.Id != Value.Owned[I].Id ||
            static_cast<unsigned>(Def.RawGrade) > 4)
        {
            return Result::InvalidSnapshot;
        }
        Ids.push_back(Def.Id);
    }
    std::vector<std::string> History;
    for (const auto& Id : Value.BurnedHistory)
    {
        const auto It = std::find_if(Value.Owned.begin(), Value.Owned.end(), [&](const State& S) { return S.Id == Id; });
        if (It == Value.Owned.end() || !It->Burned || Contains(History, Id)) { return Result::InvalidSnapshot; }
        History.push_back(Id);
    }
    for (const auto& S : Value.Owned)
    {
        if (S.Burned && !Contains(History, S.Id)) { return Result::InvalidSnapshot; }
    }
    Catalog = Definitions;
    Data = Value;
    RefreshConnections();
    return Result::Success;
}

const State* Model::Find(const std::string& Id) const
{
    const auto It = std::find_if(Data.Owned.begin(), Data.Owned.end(), [&](const State& S) { return S.Id == Id; });
    return It == Data.Owned.end() ? nullptr : &*It;
}
State* Model::FindMutable(const std::string& Id)
{
    const auto It = std::find_if(Data.Owned.begin(), Data.Owned.end(), [&](const State& S) { return S.Id == Id; });
    return It == Data.Owned.end() ? nullptr : &*It;
}
const Definition* Model::FindDefinition(const std::string& Id) const
{
    const auto It = std::find_if(Catalog.begin(), Catalog.end(), [&](const Definition& D) { return D.Id == Id; });
    return It == Catalog.end() ? nullptr : &*It;
}
const State* Model::FindResidue(const std::string& Id) const
{
    const State* S = Find(Id);
    return S && S->Burned && S->Residue ? S : nullptr;
}
bool Model::IsIntact(const std::string& Id) const
{
    const State* S = Find(Id);
    return S && !S->Burned && !S->Faded;
}
bool Model::IsCollateral(const std::string& Id) const
{
    return Data.ActiveLoan.Active && Data.ActiveLoan.MemoryId == Id;
}
Result Model::CanBurn(const std::string& Id, bool AllowFaded) const
{
    const State* S = Find(Id);
    if (!S) { return Result::Missing; }
    if (S->Burned) { return Result::AlreadyBurned; }
    if (S->Faded && !AllowFaded) { return Result::Faded; }
    if (IsCollateral(Id)) { return Result::Collateral; }
    return Result::Success;
}

Result Model::Add(const Definition& Value, const Context& Runtime, const Observer& Notify)
{
    if (Mutating) { return Result::Busy; }
    if (Value.Id.empty() || Find(Value.Id) || static_cast<unsigned>(Value.RawGrade) > 4) { return Result::InvalidSnapshot; }
    MutationScope Scope(Mutating);
    Catalog.push_back(Value);
    State S;
    S.Id = Value.Id;
    Data.Owned.push_back(S);
    Event Added;
    Added.Kind = EventKind::Added;
    Added.MemoryId = Value.Id;
    Emit(Notify, Added);
    NotifyCarry(Runtime, Notify);
    // The source refreshes connections AFTER both acquisition notifications.
    RefreshConnections();
    return Result::Success;
}

void Model::RefreshConnections()
{
    for (auto& S : Data.Owned) { S.Connections.clear(); }
    // Enumerate groups in first-occurrence order, matching Godot Dictionary order.
    std::vector<std::string> Npcs;
    for (const auto& D : Catalog)
    {
        if (!D.RelatedNpc.empty() && !Contains(Npcs, D.RelatedNpc)) { Npcs.push_back(D.RelatedNpc); }
    }
    for (const auto& Npc : Npcs)
    {
        for (std::size_t I = 0; I < Catalog.size(); ++I)
        {
            if (Catalog[I].RelatedNpc != Npc) { continue; }
            for (std::size_t J = I + 1; J < Catalog.size(); ++J)
            {
                if (Catalog[J].RelatedNpc == Npc) { Link(Data.Owned[I], Data.Owned[J]); }
            }
        }
    }
    std::vector<std::string> Prefixes;
    for (const auto& D : Catalog)
    {
        const auto At = D.Id.find('_');
        if (At != std::string::npos && !Contains(Prefixes, D.Id.substr(0, At))) { Prefixes.push_back(D.Id.substr(0, At)); }
    }
    for (const auto& Prefix : Prefixes)
    {
        State* Previous = nullptr;
        for (auto& S : Data.Owned)
        {
            const auto At = S.Id.find('_');
            if (At == std::string::npos || S.Id.substr(0, At) != Prefix) { continue; }
            if (Previous) { Link(*Previous, S); }
            Previous = &S;
        }
    }
}

Result Model::Burn(const std::string& Id, BurnMode Mode, bool AllowFaded, const Context& Runtime, const Observer& Notify)
{
    if (Mutating) { return Result::Busy; }
    const Result Eligibility = CanBurn(Id, AllowFaded);
    if (Eligibility != Result::Success) { return Eligibility; }
    MutationScope Scope(Mutating);
    State& S = *FindMutable(Id);
    const Definition& D = *FindDefinition(Id);
    S.Burned = true;
    if (Mode == BurnMode::Normal && Runtime.EliaWithParty && D.RawGrade >= Grade::Grade3)
    {
        S.Residue = true;
        Event Residue;
        Residue.Kind = EventKind::ResidueCreated;
        Residue.MemoryId = Id;
        Emit(Notify, Residue); // Burned flag is set; history still excludes this burn.
    }
    Data.BurnedHistory.push_back(Id);
    Cascade(Id, Notify);
    Event Burned;
    Burned.Kind = EventKind::Burned;
    Burned.MemoryId = Id;
    Emit(Notify, Burned); // History + cascade committed, new passives not yet evaluated.
    NotifyCarry(Runtime, Notify);
    UnlockPassives(Notify);
    return Result::Success;
}

void Model::Cascade(const std::string& SourceId, const Observer& Notify)
{
    constexpr std::array<std::int64_t, 5> Amounts{2, 4, 7, 11, 0};
    const auto Amount = Amounts[static_cast<unsigned>(FindDefinition(SourceId)->RawGrade)];
    if (Amount <= 0) { return; }
    const auto Neighbors = Find(SourceId)->Connections;
    Event Cascaded;
    Cascaded.Kind = EventKind::Cascaded;
    Cascaded.MemoryId = SourceId;
    Cascaded.Amount = Amount;
    for (const auto& Id : Neighbors)
    {
        State* S = FindMutable(Id);
        const Definition* D = FindDefinition(Id);
        if (!S || S->Burned || S->Faded || D->RawGrade >= Grade::Grade1) { continue; }
        const auto Guard = std::find(Data.ErosionGuarded.begin(), Data.ErosionGuarded.end(), Id);
        if (Guard != Data.ErosionGuarded.end()) { Data.ErosionGuarded.erase(Guard); continue; }
        // Source cascade does NOT exempt collateral or Still Hands memories.
        S->Erosion += Amount;
        Cascaded.AffectedIds.push_back(Id);
        if (S->Erosion >= static_cast<std::int64_t>(static_cast<double>(D->BurnPower) * 0.7) && !S->Faded)
        {
            S->Faded = true;
            Event Faded;
            Faded.Kind = EventKind::Faded;
            Faded.MemoryId = Id;
            Emit(Notify, Faded);
        }
    }
    if (!Cascaded.AffectedIds.empty()) { Emit(Notify, Cascaded); }
}

Result Model::ApplyErosion(std::int64_t Chapter, const Context& Runtime, const Observer& Notify)
{
    if (Mutating) { return Result::Busy; }
    MutationScope Scope(Mutating);
    const double Overload = std::clamp(CarryOverload(Runtime), 0.0, 1.0);
    const auto Base = std::max<std::int64_t>(1, static_cast<std::int64_t>(std::round(static_cast<double>(Chapter) * (1.0 + Overload))));
    std::int64_t Count = 0;
    for (std::size_t I = 0; I < Data.Owned.size(); ++I)
    {
        State& S = Data.Owned[I];
        const Definition& D = Catalog[I];
        if (S.Burned || S.Faded || D.RawGrade >= Grade::Grade1) { continue; }
        const auto Guard = std::find(Data.ErosionGuarded.begin(), Data.ErosionGuarded.end(), S.Id);
        if (Guard != Data.ErosionGuarded.end()) { Data.ErosionGuarded.erase(Guard); continue; }
        if (D.RelatedNpc == "Elia" && Runtime.StillHandsActive) { continue; }
        std::int64_t Amount = Base;
        if (D.RelatedNpc == "Elia" && Runtime.EliaWithParty) { Amount = static_cast<std::int64_t>(static_cast<double>(Amount) * 0.5); }
        if (D.RawGrade == Grade::Grade2) { Amount = static_cast<std::int64_t>(static_cast<double>(Amount) * 0.75); }
        S.Erosion += Amount;
        ++Count; // Zero actual increment still counts as processed in the source.
        if (S.Erosion >= static_cast<std::int64_t>(static_cast<double>(D.BurnPower) * 0.7) && !S.Faded)
        {
            S.Faded = true;
            Event Faded;
            Faded.Kind = EventKind::Faded;
            Faded.MemoryId = S.Id;
            Emit(Notify, Faded);
        }
    }
    if (Count > 0)
    {
        Event Eroded;
        Eroded.Kind = EventKind::MemoriesEroded;
        Eroded.Amount = Count;
        Emit(Notify, Eroded);
    }
    return Result::Success;
}

bool Model::HasPassive(const std::string& Id) const
{
    return std::any_of(Data.BurnPassives.begin(), Data.BurnPassives.end(), [&](const NamedFlag& P) { return P.Id == Id; });
}
std::int64_t Model::VoluntaryBurnCount() const
{
    return std::max<std::int64_t>(0, static_cast<std::int64_t>(Data.BurnedHistory.size()) - static_cast<std::int64_t>(Data.Extracted.size()));
}
void Model::UnlockPassives(const Observer& Notify)
{
    struct Threshold { std::int64_t Count; const char* Id; const char* Name; };
    constexpr std::array<Threshold, 5> Thresholds{{
        {5, "ember_affinity", "Ember Affinity"}, {10, "residual_warmth", "Residual Warmth"},
        {20, "ash_sight", "Ash Sight"}, {30, "void_touch", "Void Touch"}, {50, "memory_cascade", "Memory Cascade"}
    }};
    for (const auto& T : Thresholds)
    {
        if (VoluntaryBurnCount() < T.Count || HasPassive(T.Id)) { continue; }
        Data.BurnPassives.push_back({T.Id, true});
        Event Unlocked;
        Unlocked.Kind = EventKind::PassiveUnlocked;
        Unlocked.PassiveId = T.Id;
        Unlocked.PassiveName = T.Name;
        Emit(Notify, Unlocked);
    }
}
Result Model::EvaluatePassives(const Observer& Notify)
{
    if (Mutating) { return Result::Busy; }
    MutationScope Scope(Mutating);
    UnlockPassives(Notify);
    return Result::Success;
}
std::int64_t Model::EffectiveBurnPower(const std::string& Id) const
{
    const State* S = Find(Id);
    const Definition* D = FindDefinition(Id);
    return S && D ? std::max<std::int64_t>(1, D->BurnPower - S->Erosion) : 0;
}
std::int64_t Model::WeightOf(Grade Value)
{
    constexpr std::array<std::int64_t, 5> Weights{1, 2, 3, 4, 6};
    return Weights[std::min<unsigned>(static_cast<unsigned>(Value), 4)];
}
std::int64_t Model::CarryWeight() const
{
    std::int64_t Weight = 0;
    for (std::size_t I = 0; I < Data.Owned.size(); ++I)
    {
        if (!Data.Owned[I].Burned && !IsCollateral(Data.Owned[I].Id)) { Weight += WeightOf(Catalog[I].RawGrade); }
    }
    return Weight;
}
std::int64_t Model::CarryCapacity(std::int64_t Chapter)
{
    // Equivalent for all chapter integers, without overflow on extreme inputs.
    return 14 + std::clamp<std::int64_t>(Chapter > 1 ? Chapter - 1 : 0, 0, 10) * 2;
}
double Model::CarryOverload(const Context& Runtime) const
{
    const auto Capacity = CarryCapacity(Runtime.CurrentChapter);
    return std::max(0.0, static_cast<double>(CarryWeight() - Capacity) / static_cast<double>(Capacity));
}
void Model::NotifyCarry(const Context& Runtime, const Observer& Notify) const
{
    Event Carry;
    Carry.Kind = EventKind::CarryChanged;
    Carry.CarryWeight = CarryWeight();
    Carry.CarryCapacity = CarryCapacity(Runtime.CurrentChapter);
    Emit(Notify, Carry);
}
std::vector<std::string> Model::Available(Grade Minimum, bool AllowFaded) const
{
    std::vector<std::string> Ids;
    for (std::size_t I = 0; I < Data.Owned.size(); ++I)
    {
        if (CanBurn(Data.Owned[I].Id, AllowFaded) == Result::Success && Catalog[I].RawGrade >= Minimum) { Ids.push_back(Data.Owned[I].Id); }
    }
    return Ids;
}
}
