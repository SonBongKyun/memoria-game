#pragma once
#include "Run/MemoriaRunSubsystem.h"
#include "JsonObjectConverter.h"

// Explicit derived observations: default UStruct JSON omits Transient connections.
// Read through the real domain; never writes or reconstructs player authority.
inline TSharedPtr<FJsonObject> MemoriaPlayerObservation(const UMemoriaRunSubsystem& Run)
{
    auto O=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Definitions,Derived;
    const auto* Player=Run.GetPlayerMemory();
    for(const auto& D:Player->GetDefinitions())Definitions.Add(MakeShared<FJsonValueObject>(FJsonObjectConverter::UStructToJsonObject(D)));
    for(const auto& M:Player->GetSnapshot().Owned)
    {
        auto V=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Connections;
        for(const auto& Id:M.Connections)Connections.Add(MakeShared<FJsonValueString>(Id));
        V->SetStringField(TEXT("id"),M.Id);V->SetArrayField(TEXT("connections"),Connections);V->SetNumberField(TEXT("effective_burn_power"),Player->GetEffectiveBurnPower(M.Id));Derived.Add(MakeShared<FJsonValueObject>(V));
    }
    O->SetArrayField(TEXT("definitions"),Definitions);O->SetArrayField(TEXT("derived_owned"),Derived);
    O->SetNumberField(TEXT("carry_weight"),Player->GetCarryWeight());O->SetNumberField(TEXT("carry_capacity"),Player->GetCarryCapacity(Run.GetRunSnapshot().CurrentChapter));O->SetStringField(TEXT("carry_overload_float64"),FString::Printf(TEXT("%.17g"),Player->GetCarryOverload(Run.GetMemoryContext())));return O;
}
