#pragma once
#include "Narrative/MemoriaNarrativeData.h"
#include "Narrative/MemoriaNarrativeContracts.h"
#include "Domain/MemoriaPlayerMemoryDomain.h"
#include "Run/MemoriaRunTypes.h"

// Borrowed run/domain references must outlive an interpreter. No campaign boot,
// travel or UI occurs here. Effects use the accepted memory domain, never copies.
// What a VN step shows after scene_flow.gd distortion (distort_if_burned + distorted_*).
struct FMemoriaVNDisplay { bool bDistorted=false; FString Speaker, Text, Narrate, Portrait, Cg; };
struct MEMORIA_API FMemoriaNarrativeContext
{
    FMemoriaRunSnapshot& Run;
    UMemoriaPlayerMemoryDomain& Memory;
    TArray<FString> Events;
    TArray<FString> Endings;
    FString RequestedMap;
    FMemoriaNarrativeContext(FMemoriaRunSnapshot& R, UMemoriaPlayerMemoryDomain& M) : Run(R),Memory(M) {}
    bool Gate(const FMemoriaNarrativeGate& G) const;
    bool ExposeCost(const FMemoriaNarrativeEffects& E) const;
    void Flag(const FString& Id);
    // bPlayerChoice: a choice the player picked (source JourneyOath.on_player_burn).
    // Authored step burns are not Arrel's own hand and never break Still Hands.
    bool Burn(const FString& Id,bool AllowFaded=false,bool bPlayerChoice=false);
    void Rewards(const FMemoriaNarrativeEffects& E,bool VN);
    FString Localized(const FMemoriaNarrativeText& T,bool Narrate=false) const;
    // Legacy dialogue_manager.gd rule: requires_memory + burned_text swap the line (and
    // burned_portrait) once that memory is in the burned list; the row always shows.
    bool UsesBurnedText(const FMemoriaNarrativeText& T) const;
    // scene_flow.gd set_chapter: a new chapter runs MemoryManager.add_chapter_memories once.
    void SetChapter(int32 Chapter);
    FMemoriaVNDisplay VNDisplay(const FMemoriaVNStep& S) const;
};

class MEMORIA_API FMemoriaFieldInterpreter
{
public:
    FMemoriaFieldInterpreter(const FMemoriaFieldDefinition& D,FMemoriaNarrativeContext& C) : Definition(D),Context(C) {}
    void Start();
    void Advance();
    void SelectFilteredChoice(int32 VisibleIndex);
    bool IsActive() const { return bActive; }
    int32 OriginalIndex() const { return Index; }
    const TArray<int32>& VisibleOriginalIndices() const { return Visible; }
private:
    const FMemoriaFieldDefinition& Definition;
    FMemoriaNarrativeContext& Context;
    int32 Index=0;
    bool bActive=false;
    TArray<int32> Visible;
    void Show();
};

class MEMORIA_API FMemoriaVNInterpreter
{
public:
    // Resolve maps a goto_scene id to another imported definition (the Chapter 1 route); without
    // one only same-scene goto_scene is followed. bQualified adds scene ids and displayed text to
    // the event trace, as the Chapter 1 oracle records them.
    using FResolver=TFunction<const FMemoriaVNDefinition*(const FString&)>;
    FMemoriaVNInterpreter(const FMemoriaVNDefinition& D,FMemoriaNarrativeContext& C,FResolver InResolve=nullptr,bool bInQualified=false)
        : Definition(&D),Context(C),Resolve(MoveTemp(InResolve)),bQualified(bInQualified) {}
    const FMemoriaVNDefinition& GetDefinition() const { return *Definition; }
    void Play(int32 StartIndex=0);
    void Advance();
    void SelectOriginalChoice(int32 OriginalChoiceIndex);
    TArray<int32> VisibleOriginalIndices() const;
    FMemoriaVNContinuation ExportContinuation() const { return Continuation; }
    // Like prepare_resume_from_save: active cursor takes precedence over pending;
    // re-execution may repeat effects. Unknown external sequence stays rejected
    // until a later content resolver exists; no invented ch1 fallback asset.
    bool PrepareResume(const FMemoriaVNContinuation& Saved);
    bool ConsumePendingOrQueue();
private:
    const FMemoriaVNDefinition* Definition;
    FMemoriaNarrativeContext& Context;
    FResolver Resolve;
    bool bQualified=false;
    FString Where(int32 Index) const;
    FMemoriaVNContinuation Continuation;
    void Execute();
    void End();
};
