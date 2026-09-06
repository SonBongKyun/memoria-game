#pragma once
#include "Domain/MemoriaMemoryModel.h"
#include <algorithm>
#include <cmath>
#include <sstream>

namespace Memoria::Tests
{
namespace M = Memoria::Memory;
struct Observation { std::vector<std::string> Tokens; double Overload = 0.0; };
struct ObservedEvent { std::vector<std::string> Tokens; Observation State; };
struct Step { bool Success = true; std::vector<ObservedEvent> Events; Observation State; };
struct Command
{
    std::string Op;
    std::string Id;
    bool AllowFaded = false;
    std::int64_t Chapter = 1;
    M::Definition Added;
};
struct Fixture
{
    std::string Id;
    M::Context Context;
    std::vector<M::Definition> Definitions;
    M::Snapshot Initial;
    std::vector<Command> Commands;
    Observation ExpectedInitial;
    std::vector<Step> Expected;
};
inline void List(std::vector<std::string>& T, const std::string& Name, const std::vector<std::string>& Values)
{
    T.push_back(Name); T.push_back(std::to_string(Values.size()));
    T.insert(T.end(), Values.begin(), Values.end());
}
inline void Flags(std::vector<std::string>& T, const std::string& Name, std::vector<M::NamedFlag> Values)
{
    std::sort(Values.begin(), Values.end(), [](const auto& A, const auto& B) { return A.Id < B.Id; });
    T.push_back(Name); T.push_back(std::to_string(Values.size()));
    for (const auto& V : Values) { T.push_back(V.Id); T.push_back(V.Value ? "1" : "0"); }
}
inline Observation Observe(const M::Model& Model, const M::Context& C)
{
    Observation R;
    auto& T = R.Tokens;
    const auto& S = Model.GetSnapshot();
    T = {"owned", std::to_string(S.Owned.size())};
    for (const auto& V : S.Owned)
    {
        T.insert(T.end(), {V.Id, V.Burned ? "1" : "0", V.Residue ? "1" : "0", V.Faded ? "1" : "0", std::to_string(V.Erosion)});
        List(T, "connections", V.Connections);
    }
    List(T, "history", S.BurnedHistory);
    Flags(T, "passives", S.BurnPassives);
    List(T, "guards", S.ErosionGuarded);
    T.insert(T.end(), {"loan", S.ActiveLoan.Active ? "1" : "0"});
    if (S.ActiveLoan.Active)
    {
        T.insert(T.end(), {S.ActiveLoan.MemoryId, std::to_string(S.ActiveLoan.Principal), std::to_string(S.ActiveLoan.Repay), std::to_string(S.ActiveLoan.DueChapter)});
    }
    List(T, "extracted", S.Extracted);
    T.insert(T.end(), {"anchor_vigil", std::to_string(S.AnchorVigil)});
    Flags(T, "anchor_passives", S.AnchorPassives);
    std::vector<std::string> Chapters;
    for (auto V : S.VigilChapters) { Chapters.push_back(std::to_string(V)); }
    List(T, "vigil_chapters", Chapters);
    T.insert(T.end(), {"guard_slots_used", std::to_string(S.GuardSlotsUsed),
        "carry_weight", std::to_string(Model.CarryWeight()), "carry_capacity", std::to_string(M::Model::CarryCapacity(C.CurrentChapter)),
        "voluntary_burn_count", std::to_string(Model.VoluntaryBurnCount())});
    List(T, "available", Model.Available());
    List(T, "available_allow_faded", Model.Available(M::Grade::Grade5, true));
    std::vector<std::string> Residues, Powers, Grades, Weights, Intact;
    for (const auto& V : S.Owned)
    {
        if (Model.FindResidue(V.Id)) { Residues.push_back(V.Id); }
        Powers.push_back(std::to_string(Model.EffectiveBurnPower(V.Id)));
        const auto Grade = Model.FindDefinition(V.Id)->RawGrade;
        Grades.push_back(std::to_string(static_cast<unsigned>(Grade)));
        Weights.push_back(std::to_string(M::Model::WeightOf(Grade)));
        Intact.push_back(Model.IsIntact(V.Id) ? "1" : "0");
    }
    List(T, "residue_ids", Residues); List(T, "effective_powers", Powers);
    List(T, "grade_ordinals", Grades); List(T, "weights", Weights); List(T, "intact", Intact);
    R.Overload = Model.CarryOverload(C);
    return R;
}
inline std::vector<std::string> EventTokens(const M::Event& E)
{
    static const char* Names[] = {"Added", "ResidueCreated", "Faded", "Cascaded", "Burned", "CarryChanged", "PassiveUnlocked", "MemoriesEroded"};
    std::vector<std::string> T{Names[static_cast<unsigned>(E.Kind)], E.MemoryId, std::to_string(E.Amount), std::to_string(E.CarryWeight), std::to_string(E.CarryCapacity), E.PassiveName};
    List(T, "affected_ids", E.AffectedIds);
    return T;
}
inline bool CompareTokens(const std::vector<std::string>& A, const std::vector<std::string>& B, const std::string& At, std::vector<std::string>& Errors)
{
    if (A == B) { return true; }
    const auto N = std::min(A.size(), B.size());
    std::size_t I = 0;
    while (I < N && A[I] == B[I]) { ++I; }
    Errors.push_back(At + " token " + std::to_string(I) + " actual=" + (I < A.size() ? A[I] : "<end>") + " expected=" + (I < B.size() ? B[I] : "<end>"));
    return false;
}
inline void CompareObservation(const Observation& A, const Observation& B, const std::string& At, std::vector<std::string>& Errors)
{
    CompareTokens(A.Tokens, B.Tokens, At, Errors);
    // Godot JSON prints a bounded decimal precision; integer/rule values stay exact.
    if (std::abs(A.Overload - B.Overload) > 1e-9) { Errors.push_back(At + " overload differs"); }
}
inline std::vector<std::string> Run(const Fixture& F)
{
    std::vector<std::string> Errors;
    M::Model Model;
    if (Model.Restore(F.Definitions, F.Initial) != M::Result::Success) { return {F.Id + " initial snapshot rejected"}; }
    CompareObservation(Observe(Model, F.Context), F.ExpectedInitial, F.Id + " initial", Errors);
    if (F.Commands.size() != F.Expected.size()) { return {F.Id + " fixture command/expected mismatch"}; }
    for (std::size_t I = 0; I < F.Commands.size(); ++I)
    {
        const auto& C = F.Commands[I];
        Step Actual;
        M::Observer Observer = [&](const M::Event& E) { Actual.Events.push_back({EventTokens(E), Observe(Model, F.Context)}); };
        M::Result R = M::Result::Success;
        if (C.Op == "burn" || C.Op == "silent") { R = Model.Burn(C.Id, C.Op == "burn" ? M::BurnMode::Normal : M::BurnMode::Silent, C.AllowFaded, F.Context, Observer); }
        else if (C.Op == "erode") { R = Model.ApplyErosion(C.Chapter, F.Context, Observer); }
        else if (C.Op == "passives") { R = Model.EvaluatePassives(Observer); }
        else if (C.Op == "add") { R = Model.Add(C.Added, F.Context, Observer); }
        else if (C.Op == "query")
        {
            for (const auto& S : Model.GetSnapshot().Owned) { Model.FindResidue(S.Id); Model.FindResidue(S.Id); }
        }
        else { Errors.push_back("Unsupported command " + C.Op); }
        Actual.Success = R == M::Result::Success;
        Actual.State = Observe(Model, F.Context);
        const auto& Expected = F.Expected[I];
        const auto At = F.Id + " step " + std::to_string(I);
        if (Actual.Success != Expected.Success) { Errors.push_back(At + " result differs"); }
        CompareObservation(Actual.State, Expected.State, At + " state", Errors);
        if (Actual.Events.size() != Expected.Events.size()) { Errors.push_back(At + " event count differs"); }
        for (std::size_t J = 0; J < std::min(Actual.Events.size(), Expected.Events.size()); ++J)
        {
            const auto EventAt = At + " event " + std::to_string(J);
            CompareTokens(Actual.Events[J].Tokens, Expected.Events[J].Tokens, EventAt, Errors);
            CompareObservation(Actual.Events[J].State, Expected.Events[J].State, EventAt + " observed state", Errors);
        }
    }
    return Errors;
}
}
