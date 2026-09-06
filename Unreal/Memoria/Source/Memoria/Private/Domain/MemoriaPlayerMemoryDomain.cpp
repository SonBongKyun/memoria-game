#include "Domain/MemoriaPlayerMemoryDomain.h"
#include "Domain/MemoriaMemoryModel.h"
#include "Templates/UnrealTemplate.h"

namespace
{
namespace M = Memoria::Memory;
std::string Utf8(const FString& Value) { return std::string(TCHAR_TO_UTF8(*Value)); }
FString Text(const std::string& Value) { return FString(UTF8_TO_TCHAR(Value.c_str())); }
std::vector<std::string> ToStrings(const TArray<FString>& Values)
{
    std::vector<std::string> Result;
    for (const auto& Value : Values) { Result.push_back(Utf8(Value)); }
    return Result;
}
TArray<FString> FromStrings(const std::vector<std::string>& Values)
{
    TArray<FString> Result;
    for (const auto& Value : Values) { Result.Add(Text(Value)); }
    return Result;
}
M::Definition ToDefinition(const FMemoriaMemoryDefinition& D)
{
    return {Utf8(D.Id), Utf8(D.Title), Utf8(D.Description), static_cast<M::Grade>(D.RawGrade), D.BurnPower, Utf8(D.StoryEffect), Utf8(D.RelatedNpc)};
}
M::Context ToContext(const FMemoriaMemoryContext& C) { return {C.CurrentChapter, C.bEliaWithParty, C.bStillHandsActive}; }
M::Snapshot ToSnapshot(const FMemoriaMemorySnapshot& S)
{
    M::Snapshot R;
    for (const auto& V : S.Owned) { R.Owned.push_back({Utf8(V.Id), V.bBurned, V.bResidue, V.bFaded, V.Erosion, ToStrings(V.Connections)}); }
    R.BurnedHistory = ToStrings(S.BurnedHistory);
    for (const auto& V : S.BurnPassives) { R.BurnPassives.push_back({Utf8(V.Id), V.bValue}); }
    R.AnchorVigil = S.AnchorVigil;
    for (const auto& V : S.AnchorPassives) { R.AnchorPassives.push_back({Utf8(V.Id), V.bValue}); }
    for (const auto V : S.VigilChapters) { R.VigilChapters.push_back(V); }
    R.ErosionGuarded = ToStrings(S.ErosionGuarded);
    R.GuardSlotsUsed = S.GuardSlotsUsed;
    R.ActiveLoan = {S.ActiveLoan.bActive, Utf8(S.ActiveLoan.MemoryId), S.ActiveLoan.Principal, S.ActiveLoan.Repay, S.ActiveLoan.DueChapter};
    R.Extracted = ToStrings(S.Extracted);
    return R;
}
FMemoriaMemorySnapshot FromSnapshot(const M::Snapshot& S)
{
    FMemoriaMemorySnapshot R;
    for (const auto& V : S.Owned)
    {
        FMemoriaMemoryState E;
        E.Id = Text(V.Id); E.bBurned = V.Burned; E.bResidue = V.Residue; E.bFaded = V.Faded;
        E.Erosion = V.Erosion; E.Connections = FromStrings(V.Connections); R.Owned.Add(E);
    }
    R.BurnedHistory = FromStrings(S.BurnedHistory);
    for (const auto& V : S.BurnPassives) { FMemoriaMemoryFlag E; E.Id = Text(V.Id); E.bValue = V.Value; R.BurnPassives.Add(E); }
    R.AnchorVigil = S.AnchorVigil;
    for (const auto& V : S.AnchorPassives) { FMemoriaMemoryFlag E; E.Id = Text(V.Id); E.bValue = V.Value; R.AnchorPassives.Add(E); }
    for (const auto V : S.VigilChapters) { R.VigilChapters.Add(V); }
    R.ErosionGuarded = FromStrings(S.ErosionGuarded); R.GuardSlotsUsed = S.GuardSlotsUsed;
    R.ActiveLoan.bActive = S.ActiveLoan.Active; R.ActiveLoan.MemoryId = Text(S.ActiveLoan.MemoryId);
    R.ActiveLoan.Principal = S.ActiveLoan.Principal; R.ActiveLoan.Repay = S.ActiveLoan.Repay;
    R.ActiveLoan.DueChapter = S.ActiveLoan.DueChapter; R.Extracted = FromStrings(S.Extracted);
    return R;
}
FMemoriaMemoryEvent FromEvent(const M::Event& E)
{
    FMemoriaMemoryEvent R;
    R.Kind = static_cast<EMemoriaMemoryEventKind>(E.Kind); R.MemoryId = Text(E.MemoryId);
    R.AffectedIds = FromStrings(E.AffectedIds); R.Amount = E.Amount;
    R.CarryWeight = E.CarryWeight; R.CarryCapacity = E.CarryCapacity;
    R.PassiveId = Text(E.PassiveId); R.PassiveName = Text(E.PassiveName);
    return R;
}
}

static_assert(static_cast<uint8>(EMemoriaMemoryGrade::Grade5) == 0);
static_assert(static_cast<uint8>(EMemoriaMemoryGrade::Grade4) == 1);
static_assert(static_cast<uint8>(EMemoriaMemoryGrade::Grade3) == 2);
static_assert(static_cast<uint8>(EMemoriaMemoryGrade::Grade2) == 3);
static_assert(static_cast<uint8>(EMemoriaMemoryGrade::Grade1) == 4);

UMemoriaPlayerMemoryDomain::UMemoriaPlayerMemoryDomain() : Model(MakeUnique<Memoria::Memory::Model>()) {}
UMemoriaPlayerMemoryDomain::~UMemoriaPlayerMemoryDomain() = default;
EMemoriaMemoryResult UMemoriaPlayerMemoryDomain::Restore(const TArray<FMemoriaMemoryDefinition>& Definitions, const FMemoriaMemorySnapshot& Snapshot)
{
    std::vector<Memoria::Memory::Definition> Values;
    for (const auto& D : Definitions) { Values.push_back(ToDefinition(D)); }
    const auto Result = Model->Restore(Values, ToSnapshot(Snapshot));
    if (Result == Memoria::Memory::Result::Success) { DefinitionCatalog = Definitions; }
    return static_cast<EMemoriaMemoryResult>(Result);
}
void UMemoriaPlayerMemoryDomain::Publish(const FMemoriaMemoryEvent& Event)
{
    TGuardValue<bool> Guard(bDispatchingEvent, true);
    OnObserved.Broadcast(Event);
    OnPresentationEvent.Broadcast(Event);
}
EMemoriaMemoryResult UMemoriaPlayerMemoryDomain::Add(const FMemoriaMemoryDefinition& Definition, const FMemoriaMemoryContext& Context)
{
    if (bDispatchingEvent) { return EMemoriaMemoryResult::Busy; }
    // Definition data must be visible while the Added event exposes the new state.
    DefinitionCatalog.Add(Definition);
    const auto Result = Model->Add(ToDefinition(Definition), ToContext(Context), [this](const auto& E) { Publish(FromEvent(E)); });
    if (Result != Memoria::Memory::Result::Success) { DefinitionCatalog.Pop(); }
    return static_cast<EMemoriaMemoryResult>(Result);
}
EMemoriaMemoryResult UMemoriaPlayerMemoryDomain::Burn(const FString& Id, EMemoriaBurnMode Mode, bool bAllowFaded, const FMemoriaMemoryContext& Context)
{
    return static_cast<EMemoriaMemoryResult>(Model->Burn(Utf8(Id), static_cast<Memoria::Memory::BurnMode>(Mode), bAllowFaded, ToContext(Context), [this](const auto& E) { Publish(FromEvent(E)); }));
}
EMemoriaMemoryResult UMemoriaPlayerMemoryDomain::ApplyErosion(int64 Chapter, const FMemoriaMemoryContext& Context)
{
    return static_cast<EMemoriaMemoryResult>(Model->ApplyErosion(Chapter, ToContext(Context), [this](const auto& E) { Publish(FromEvent(E)); }));
}
EMemoriaMemoryResult UMemoriaPlayerMemoryDomain::EvaluatePassives()
{
    return static_cast<EMemoriaMemoryResult>(Model->EvaluatePassives([this](const auto& E) { Publish(FromEvent(E)); }));
}
FMemoriaMemorySnapshot UMemoriaPlayerMemoryDomain::GetSnapshot() const { return FromSnapshot(Model->GetSnapshot()); }
EMemoriaMemoryResult UMemoriaPlayerMemoryDomain::CanBurn(const FString& Id, bool bAllowFaded) const { return static_cast<EMemoriaMemoryResult>(Model->CanBurn(Utf8(Id), bAllowFaded)); }
int64 UMemoriaPlayerMemoryDomain::GetEffectiveBurnPower(const FString& Id) const { return Model->EffectiveBurnPower(Utf8(Id)); }
int64 UMemoriaPlayerMemoryDomain::GetCarryWeight() const { return Model->CarryWeight(); }
int64 UMemoriaPlayerMemoryDomain::GetCarryCapacity(int64 CurrentChapter) const { return Memoria::Memory::Model::CarryCapacity(CurrentChapter); }
double UMemoriaPlayerMemoryDomain::GetCarryOverload(const FMemoriaMemoryContext& Context) const { return Model->CarryOverload(ToContext(Context)); }
TArray<FString> UMemoriaPlayerMemoryDomain::GetAvailable(EMemoriaMemoryGrade MinimumGrade, bool bAllowFaded) const
{
    return FromStrings(Model->Available(static_cast<Memoria::Memory::Grade>(MinimumGrade), bAllowFaded));
}
bool UMemoriaPlayerMemoryDomain::HasPassive(const FString& Id) const { return Model->HasPassive(Utf8(Id)); }
bool UMemoriaPlayerMemoryDomain::HasResidue(const FString& Id) const { return Model->FindResidue(Utf8(Id)) != nullptr; }
bool UMemoriaPlayerMemoryDomain::IsIntact(const FString& Id) const { return Model->IsIntact(Utf8(Id)); }
