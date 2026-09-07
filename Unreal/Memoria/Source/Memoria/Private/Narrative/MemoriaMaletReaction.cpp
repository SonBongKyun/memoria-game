#include "Narrative/MemoriaMaletReaction.h"
#include "Narrative/MemoriaNarrativeData.h"
#include "Run/MemoriaRunSubsystem.h"

FMemoriaMaletDispatch MemoriaMaletReaction::Resolve(UMemoriaRunSubsystem& Run, bool bDialogueActive, const UMemoriaFieldAsset* Reaction, bool bTalkCached)
{
    FMemoriaMaletDispatch Result;
    if (!Run.HasActiveRun() || bDialogueActive || Run.GetPlayerMemory()->IsDispatchingEvent()) return Result;
    const auto Snapshot = Run.GetRunSnapshot();
    const bool Burned = Run.GetPlayerMemory()->GetSnapshot().BurnedHistory.ContainsByPredicate(
        [](const FString& Id) { return Id.Equals(Food, ESearchCase::CaseSensitive); });
    Result.Events.Add(FString::Printf(TEXT("resolver:begin:burned=%s:heard=%s"), Burned?TEXT("true"):TEXT("false"), Snapshot.GetFlag(Heard)?TEXT("true"):TEXT("false")));
    Result.File = TEXT("res://data/chapter2_dialogue.json");
    if (Burned && !Snapshot.GetFlag(Heard) && Reaction &&
        Reaction->Definition.Id.Equals(Group, ESearchCase::CaseSensitive) && Reaction->Definition.Rows.Num() == 3)
    {
        // Source consumes only an existing reaction, before load_and_start.
        Run.SetStoryFlag(Heard, true);
        Result.Events.Add(FString(TEXT("flag:")) + Heard);
        Result.bReaction = true; Result.Group = Group;
        Result.Events.Add(FString(TEXT("resolver:reaction:")) + Group);
    }
    else
    {
        Result.Events.Add(TEXT("resolver:none"));
        // Source checks both the NPC cache and the persisted talked flag.
        Result.Group = bTalkCached || Snapshot.GetFlag(TEXT("talked_Malet_malet_encounter"))
            ? TEXT("malet_memory_world_followup") : TEXT("malet_encounter");
    }
    return Result;
}
