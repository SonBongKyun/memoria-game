#include "Narrative/MemoriaNarrativeRuntime.h"
namespace
{
FString Num(int32 N) { return FString::FromInt(N); }
FString Indices(const TArray<int32>& V) { TArray<FString> S; for (int32 I:V) S.Add(Num(I)); return FString::Join(S,TEXT(",")); }
}
bool FMemoriaNarrativeContext::Gate(const FMemoriaNarrativeGate& G) const
{
    return !(G.bHasRequiresFlag && !Run.GetFlag(G.RequiresFlag)) && !(G.bHasRequiresNotFlag && Run.GetFlag(G.RequiresNotFlag)) &&
        !(G.bHasRequiresMemoryIntact && !Memory.IsIntact(G.RequiresMemoryIntact)) && !(G.bHasRequiresMemoryGone && Memory.IsIntact(G.RequiresMemoryGone));
}
bool FMemoriaNarrativeContext::ExposeCost(const FMemoriaNarrativeEffects& E) const
{
    FString Id=E.bHasCostMemory?E.CostMemory:(E.bHasBurnMemory?E.BurnMemory:TEXT("")); if (Id.IsEmpty()) return true;
    for (const auto& M:Memory.GetSnapshot().Owned) if (M.Id.Equals(Id,ESearchCase::CaseSensitive)) return !M.bBurned && !M.bFaded;
    return false;
}
void FMemoriaNarrativeContext::Flag(const FString& Id)
{
    bool Found=false; for (auto& F:Run.StoryFlags) if (F.Id.Equals(Id,ESearchCase::CaseSensitive)) { F.bValue=true; Found=true; break; }
    if (!Found) { FMemoriaStoryFlag F; F.Id=Id; F.bValue=true; Run.StoryFlags.Add(F); }
    Events.Add(TEXT("flag:")+Id);
}
bool FMemoriaNarrativeContext::Burn(const FString& Id,bool Allow)
{
    FMemoriaMemoryContext C; C.bEliaWithParty=Run.Player.bEliaWithParty;
    const bool Result=Memory.Burn(Id,EMemoriaBurnMode::Normal,Allow,C)==EMemoriaMemoryResult::Success;
    Events.Add(TEXT("burn:")+Id+(Result?TEXT(":ok"):TEXT(":fail"))); return Result;
}
void FMemoriaNarrativeContext::Rewards(const FMemoriaNarrativeEffects& E,bool VN)
{
    if (E.bHasAddGrains) { Run.Player.Grains+=E.AddGrains; if (VN) Events.Add(TEXT("stat:total_grains_earned:")+Num(E.AddGrains)); }
    if (E.bHasAddItem)
    {
        int32 Count=E.bHasAddItemCount?E.AddItemCount:1; bool Found=false;
        for (auto& I:Run.Player.Items) if (I.Id.Equals(E.AddItem,ESearchCase::CaseSensitive)) { I.Count+=Count; Found=true; break; }
        if (!Found) { FMemoriaItemCount I; I.Id=E.AddItem; I.Count=Count; Run.Player.Items.Add(I); }
        Events.Add(TEXT("item:")+E.AddItem+TEXT(":")+Num(Count));
    }
    if (E.bHasHealPlayer) Run.Player.Hp+=FMath::Max<int64>(0,FMath::Min<int64>(E.HealPlayer,Run.Player.MaxHp-Run.Player.Hp));
}
FString FMemoriaNarrativeContext::Localized(const FMemoriaNarrativeText& T,bool Narrate) const
{
    bool KO=Run.CurrentLocale==TEXT("ko");
    return Narrate?(KO && T.bHasNarrateKo?T.NarrateKo:T.Narrate):(KO && T.bHasTextKo?T.TextKo:T.Text);
}
void FMemoriaFieldInterpreter::Start() { Index=0; bActive=true; Visible.Reset(); Show(); }
void FMemoriaFieldInterpreter::Advance() { if (bActive) { ++Index; Show(); } }
void FMemoriaFieldInterpreter::Show()
{
    // Iteration preserves source recursive skip behavior without stack growth.
    while (bActive)
    {
        Context.Events.Add(TEXT("visit:")+Num(Index));
        if (!Definition.Rows.IsValidIndex(Index)) { bActive=false; Index=0; Visible.Reset(); Context.Events.Add(TEXT("end")); return; }
        const auto& R=Definition.Rows[Index]; bool Pass=Context.Gate(R.Gate);
        Context.Events.Add(TEXT("gate:")+Num(Index)+(Pass?TEXT(":pass"):TEXT(":skip")));
        if (!Pass) { ++Index; continue; }
        Context.Events.Add(TEXT("effects:")+Num(Index));
        if (R.Effects.bHasSetFlag) Context.Flag(R.Effects.SetFlag);
        if (R.Effects.bHasRecordEnding) { Context.Endings.Add(R.Effects.RecordEnding); Context.Events.Add(TEXT("ending:")+R.Effects.RecordEnding); }
        if (R.bChoicesPresent)
        {
            Visible.Reset(); for (const auto& C:R.Choices) if (Context.Gate(C.Gate)) Visible.Add(C.OriginalIndex);
            if (Visible.IsEmpty()) { ++Index; continue; }
            Context.Events.Add(TEXT("choices:")+Indices(Visible)); return;
        }
        Context.Events.Add(TEXT("line:")+Num(Index)+TEXT(":")+Context.Localized(R.Text)); return;
    }
}
void FMemoriaFieldInterpreter::SelectFilteredChoice(int32 I)
{
    if (!bActive || !Definition.Rows.IsValidIndex(Index)) return;
    if (Visible.IsValidIndex(I))
    {
        const auto& C=Definition.Rows[Index].Choices[Visible[I]]; const auto& E=C.Effects;
        Context.Events.Add(TEXT("choice:")+Context.Localized(C.Text));
        if (E.bHasSetFlag) Context.Flag(E.SetFlag);
        if (E.bHasBurnMemory) Context.Burn(E.BurnMemory);
        if (E.bHasCostMemory) Context.Burn(E.CostMemory); // source deliberately continues on failure
        if (E.bHasRecordEnding) { Context.Endings.Add(E.RecordEnding); Context.Events.Add(TEXT("ending:")+E.RecordEnding); }
        Context.Rewards(E,false);
        if (C.bHasJump) Index=C.Jump-1;
    }
    Visible.Reset(); Advance();
}
void FMemoriaVNInterpreter::Play(int32 Start)
{
    Continuation.Current.SequenceId=Definition.Id; Continuation.Current.OriginalIndex=FMath::Clamp(Start,0,Definition.Steps.Num()); Continuation.bActive=true; Execute();
}
void FMemoriaVNInterpreter::Advance() { if (Continuation.bActive) { ++Continuation.Current.OriginalIndex; Execute(); } }
void FMemoriaVNInterpreter::End()
{ Continuation.bActive=false; Continuation.Current={}; Context.Events.Add(TEXT("end")); }
TArray<int32> FMemoriaVNInterpreter::VisibleOriginalIndices() const
{
    TArray<int32> R; if (!Continuation.bActive || !Definition.Steps.IsValidIndex(Continuation.Current.OriginalIndex)) return R;
    for (const auto& C:Definition.Steps[Continuation.Current.OriginalIndex].Choices) if (Context.Gate(C.Gate) && Context.ExposeCost(C.Effects)) R.Add(C.OriginalIndex);
    return R;
}
void FMemoriaVNInterpreter::Execute()
{
    int32 Budget=4096; // Report a bounded contract failure rather than hang on malformed synthetic loops.
    while (Continuation.bActive && Budget-->0)
    {
        auto& Index=Continuation.Current.OriginalIndex; Index=FMath::Max(0,Index); Context.Events.Add(TEXT("visit:")+Num(Index));
        if (!Definition.Steps.IsValidIndex(Index)) { End(); return; }
        const auto& S=Definition.Steps[Index]; const auto& E=S.Effects;
        if (E.bHasSetFlag) Context.Flag(E.SetFlag);
        if (E.bHasBurnMemory) Context.Burn(E.BurnMemory,E.bHasAllowFadedBurn && E.AllowFadedBurn);
        if (E.bHasRecordEnding) { Context.Endings.Add(E.RecordEnding); Context.Events.Add(TEXT("ending:")+E.RecordEnding); }
        if (!Context.Gate(S.Gate)) { ++Index; continue; }
        Context.Rewards(E,true);
        if (S.Action.bHasAction)
        {
            const auto& A=S.Action;
            if (A.Action==TEXT("goto_map"))
            {
                if (A.bHasResumeScene) { FMemoriaVNCursor C; C.SequenceId=A.ResumeScene; C.OriginalIndex=A.bHasResumeIndex?A.ResumeIndex:0; Continuation.ResumeQueue.Add(C); }
                Continuation.bActive=false; Context.RequestedMap=A.Path; Context.Events.Add(TEXT("map:")+A.Path); return;
            }
            if (A.Action==TEXT("goto_scene")) { Index=A.bHasStartIndex?A.StartIndex:0; continue; }
            if (A.Action==TEXT("end")) { End(); return; }
        }
        Context.Events.Add(TEXT("step:")+Num(Index));
        if (S.bChoicesPresent) Context.Events.Add(TEXT("choices:")+Indices(VisibleOriginalIndices()));
        return;
    }
    if (Budget<=0) { Continuation.bActive=false; Context.Events.Add(TEXT("contract_error:step_budget")); }
}
void FMemoriaVNInterpreter::SelectOriginalChoice(int32 I)
{
    if (!Continuation.bActive || !Definition.Steps.IsValidIndex(Continuation.Current.OriginalIndex)) return;
    const auto& S=Definition.Steps[Continuation.Current.OriginalIndex]; if (!S.bChoicesPresent || !S.Choices.IsValidIndex(I)) return;
    const auto& C=S.Choices[I]; if (!Context.Gate(C.Gate)) return;
    Context.Events.Add(TEXT("choice:")+Context.Localized(C.Text)); const auto& E=C.Effects;
    if (E.bHasCostMemory && !Context.Burn(E.CostMemory,E.bHasAllowFadedBurn && E.AllowFadedBurn)) return;
    if (E.bHasSetFlag) Context.Flag(E.SetFlag);
    if (E.bHasBurnMemory) Context.Burn(E.BurnMemory,E.bHasAllowFadedBurn && E.AllowFadedBurn);
    // SceneFlow ignores choice record_ending even though field supports it.
    Context.Rewards(E,true); if (C.bHasJump) Continuation.Current.OriginalIndex=C.Jump-1; Advance();
}
bool FMemoriaVNInterpreter::PrepareResume(const FMemoriaVNContinuation& Saved)
{
    auto Valid=[&](const FMemoriaVNCursor& C){return (C.SequenceId.IsEmpty() || C.SequenceId.Equals(Definition.Id,ESearchCase::CaseSensitive)) && C.OriginalIndex>=0;};
    if (Saved.SchemaVersion!=1 || Saved.IndexMappingVersion!=Definition.IndexMappingVersion || !Valid(Saved.Current) || !Valid(Saved.Pending)) return false;
    for (const auto& C:Saved.ResumeQueue) if (C.SequenceId.IsEmpty() || !Valid(C)) return false;
    auto Pending=Saved.bActive && !Saved.Current.SequenceId.IsEmpty()?Saved.Current:Saved.Pending;
    if (Pending.SequenceId.IsEmpty()) return false;
    Continuation=Saved; Continuation.Pending=Pending; Continuation.bActive=false;
    Continuation.LedgerBurnSnapshot=FMath::Clamp<int64>(Saved.LedgerBurnSnapshot,0,Context.Memory.GetSnapshot().BurnedHistory.Num()); return true;
}
bool FMemoriaVNInterpreter::ConsumePendingOrQueue()
{
    FMemoriaVNCursor Next;
    if (!Continuation.Pending.SequenceId.IsEmpty()) { Next=Continuation.Pending; Continuation.Pending={}; }
    else if (!Continuation.ResumeQueue.IsEmpty()) { Next=Continuation.ResumeQueue[0]; Continuation.ResumeQueue.RemoveAt(0); }
    else return false;
    Play(Next.OriginalIndex); return true;
}
