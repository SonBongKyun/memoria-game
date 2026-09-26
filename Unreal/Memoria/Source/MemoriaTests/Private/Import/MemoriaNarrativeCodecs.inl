// Generated typed primitive codecs; strict key/type checks, no runtime JSON dependency.
bool ReadText(const Obj& O, FMemoriaNarrativeText& D)
{
    if (!O) return false;
    D.bHasSpeaker=Has(O,TEXT("speaker"));
    if (D.bHasSpeaker && !ReadFString(O,TEXT("speaker"),D.Speaker)) return false;
    D.bHasText=Has(O,TEXT("text"));
    if (D.bHasText && !ReadFString(O,TEXT("text"),D.Text)) return false;
    D.bHasTextKo=Has(O,TEXT("text_ko"));
    if (D.bHasTextKo && !ReadFString(O,TEXT("text_ko"),D.TextKo)) return false;
    D.bHasNarrate=Has(O,TEXT("narrate"));
    if (D.bHasNarrate && !ReadFString(O,TEXT("narrate"),D.Narrate)) return false;
    D.bHasNarrateKo=Has(O,TEXT("narrate_ko"));
    if (D.bHasNarrateKo && !ReadFString(O,TEXT("narrate_ko"),D.NarrateKo)) return false;
    D.bHasRequiresMemory=Has(O,TEXT("requires_memory"));
    if (D.bHasRequiresMemory && !ReadFString(O,TEXT("requires_memory"),D.RequiresMemory)) return false;
    D.bHasBurnedText=Has(O,TEXT("burned_text"));
    if (D.bHasBurnedText && !ReadFString(O,TEXT("burned_text"),D.BurnedText)) return false;
    D.bHasBurnedTextKo=Has(O,TEXT("burned_text_ko"));
    if (D.bHasBurnedTextKo && !ReadFString(O,TEXT("burned_text_ko"),D.BurnedTextKo)) return false;
    return KeysSubset(O, {TEXT("speaker"),TEXT("text"),TEXT("text_ko"),TEXT("narrate"),TEXT("narrate_ko"),TEXT("requires_memory"),TEXT("burned_text"),TEXT("burned_text_ko")});
}
Obj WriteText(const FMemoriaNarrativeText& D)
{
    auto O=MakeShared<FJsonObject>();
    if (D.bHasSpeaker) O->SetStringField(TEXT("speaker"),D.Speaker);
    if (D.bHasText) O->SetStringField(TEXT("text"),D.Text);
    if (D.bHasTextKo) O->SetStringField(TEXT("text_ko"),D.TextKo);
    if (D.bHasNarrate) O->SetStringField(TEXT("narrate"),D.Narrate);
    if (D.bHasNarrateKo) O->SetStringField(TEXT("narrate_ko"),D.NarrateKo);
    if (D.bHasRequiresMemory) O->SetStringField(TEXT("requires_memory"),D.RequiresMemory);
    if (D.bHasBurnedText) O->SetStringField(TEXT("burned_text"),D.BurnedText);
    if (D.bHasBurnedTextKo) O->SetStringField(TEXT("burned_text_ko"),D.BurnedTextKo);
    return O;
}
bool ReadPresentation(const Obj& O, FMemoriaNarrativePresentation& D)
{
    if (!O) return false;
    D.bHasCg=Has(O,TEXT("cg"));
    if (D.bHasCg && !ReadFString(O,TEXT("cg"),D.Cg)) return false;
    D.bHasPortrait=Has(O,TEXT("portrait"));
    if (D.bHasPortrait && !ReadFString(O,TEXT("portrait"),D.Portrait)) return false;
    D.bHasSide=Has(O,TEXT("side"));
    if (D.bHasSide && !ReadFString(O,TEXT("side"),D.Side)) return false;
    D.bHasFadeMs=Has(O,TEXT("fade_ms"));
    if (D.bHasFadeMs && !Readint32(O,TEXT("fade_ms"),D.FadeMs)) return false;
    D.bHasBurnedPortrait=Has(O,TEXT("burned_portrait"));
    if (D.bHasBurnedPortrait && !ReadFString(O,TEXT("burned_portrait"),D.BurnedPortrait)) return false;
    return KeysSubset(O, {TEXT("cg"),TEXT("portrait"),TEXT("side"),TEXT("fade_ms"),TEXT("burned_portrait")});
}
Obj WritePresentation(const FMemoriaNarrativePresentation& D)
{
    auto O=MakeShared<FJsonObject>();
    if (D.bHasCg) O->SetStringField(TEXT("cg"),D.Cg);
    if (D.bHasPortrait) O->SetStringField(TEXT("portrait"),D.Portrait);
    if (D.bHasSide) O->SetStringField(TEXT("side"),D.Side);
    if (D.bHasFadeMs) O->SetNumberField(TEXT("fade_ms"),D.FadeMs);
    if (D.bHasBurnedPortrait) O->SetStringField(TEXT("burned_portrait"),D.BurnedPortrait);
    return O;
}
bool ReadGate(const Obj& O, FMemoriaNarrativeGate& D)
{
    if (!O) return false;
    D.bHasRequiresFlag=Has(O,TEXT("requires_flag"));
    if (D.bHasRequiresFlag && !ReadFString(O,TEXT("requires_flag"),D.RequiresFlag)) return false;
    D.bHasRequiresNotFlag=Has(O,TEXT("requires_not_flag"));
    if (D.bHasRequiresNotFlag && !ReadFString(O,TEXT("requires_not_flag"),D.RequiresNotFlag)) return false;
    D.bHasRequiresMemoryIntact=Has(O,TEXT("requires_memory_intact"));
    if (D.bHasRequiresMemoryIntact && !ReadFString(O,TEXT("requires_memory_intact"),D.RequiresMemoryIntact)) return false;
    D.bHasRequiresMemoryGone=Has(O,TEXT("requires_memory_gone"));
    if (D.bHasRequiresMemoryGone && !ReadFString(O,TEXT("requires_memory_gone"),D.RequiresMemoryGone)) return false;
    return KeysSubset(O, {TEXT("requires_flag"),TEXT("requires_not_flag"),TEXT("requires_memory_intact"),TEXT("requires_memory_gone")});
}
Obj WriteGate(const FMemoriaNarrativeGate& D)
{
    auto O=MakeShared<FJsonObject>();
    if (D.bHasRequiresFlag) O->SetStringField(TEXT("requires_flag"),D.RequiresFlag);
    if (D.bHasRequiresNotFlag) O->SetStringField(TEXT("requires_not_flag"),D.RequiresNotFlag);
    if (D.bHasRequiresMemoryIntact) O->SetStringField(TEXT("requires_memory_intact"),D.RequiresMemoryIntact);
    if (D.bHasRequiresMemoryGone) O->SetStringField(TEXT("requires_memory_gone"),D.RequiresMemoryGone);
    return O;
}
bool ReadEffects(const Obj& O, FMemoriaNarrativeEffects& D)
{
    if (!O) return false;
    D.bHasSetFlag=Has(O,TEXT("set_flag"));
    if (D.bHasSetFlag && !ReadFString(O,TEXT("set_flag"),D.SetFlag)) return false;
    D.bHasRecordEnding=Has(O,TEXT("record_ending"));
    if (D.bHasRecordEnding && !ReadFString(O,TEXT("record_ending"),D.RecordEnding)) return false;
    D.bHasBurnMemory=Has(O,TEXT("burn_memory"));
    if (D.bHasBurnMemory && !ReadFString(O,TEXT("burn_memory"),D.BurnMemory)) return false;
    D.bHasCostMemory=Has(O,TEXT("cost_memory"));
    if (D.bHasCostMemory && !ReadFString(O,TEXT("cost_memory"),D.CostMemory)) return false;
    D.bHasAllowFadedBurn=Has(O,TEXT("allow_faded_burn"));
    if (D.bHasAllowFadedBurn && !Readbool(O,TEXT("allow_faded_burn"),D.AllowFadedBurn)) return false;
    D.bHasAddGrains=Has(O,TEXT("add_grains"));
    if (D.bHasAddGrains && !Readint32(O,TEXT("add_grains"),D.AddGrains)) return false;
    D.bHasAddItem=Has(O,TEXT("add_item"));
    if (D.bHasAddItem && !ReadFString(O,TEXT("add_item"),D.AddItem)) return false;
    D.bHasAddItemCount=Has(O,TEXT("add_item_count"));
    if (D.bHasAddItemCount && !Readint32(O,TEXT("add_item_count"),D.AddItemCount)) return false;
    D.bHasHealPlayer=Has(O,TEXT("heal_player"));
    if (D.bHasHealPlayer && !Readint32(O,TEXT("heal_player"),D.HealPlayer)) return false;
    return KeysSubset(O, {TEXT("set_flag"),TEXT("record_ending"),TEXT("burn_memory"),TEXT("cost_memory"),TEXT("allow_faded_burn"),TEXT("add_grains"),TEXT("add_item"),TEXT("add_item_count"),TEXT("heal_player")});
}
Obj WriteEffects(const FMemoriaNarrativeEffects& D)
{
    auto O=MakeShared<FJsonObject>();
    if (D.bHasSetFlag) O->SetStringField(TEXT("set_flag"),D.SetFlag);
    if (D.bHasRecordEnding) O->SetStringField(TEXT("record_ending"),D.RecordEnding);
    if (D.bHasBurnMemory) O->SetStringField(TEXT("burn_memory"),D.BurnMemory);
    if (D.bHasCostMemory) O->SetStringField(TEXT("cost_memory"),D.CostMemory);
    if (D.bHasAllowFadedBurn) O->SetBoolField(TEXT("allow_faded_burn"),D.AllowFadedBurn);
    if (D.bHasAddGrains) O->SetNumberField(TEXT("add_grains"),D.AddGrains);
    if (D.bHasAddItem) O->SetStringField(TEXT("add_item"),D.AddItem);
    if (D.bHasAddItemCount) O->SetNumberField(TEXT("add_item_count"),D.AddItemCount);
    if (D.bHasHealPlayer) O->SetNumberField(TEXT("heal_player"),D.HealPlayer);
    return O;
}
bool ReadAction(const Obj& O, FMemoriaNarrativeAction& D)
{
    if (!O) return false;
    D.bHasAction=Has(O,TEXT("action"));
    if (D.bHasAction && !ReadFString(O,TEXT("action"),D.Action)) return false;
    D.bHasPath=Has(O,TEXT("path"));
    if (D.bHasPath && !ReadFString(O,TEXT("path"),D.Path)) return false;
    D.bHasId=Has(O,TEXT("id"));
    if (D.bHasId && !ReadFString(O,TEXT("id"),D.Id)) return false;
    D.bHasStartIndex=Has(O,TEXT("start_index"));
    if (D.bHasStartIndex && !Readint32(O,TEXT("start_index"),D.StartIndex)) return false;
    D.bHasResumeScene=Has(O,TEXT("resume_scene"));
    if (D.bHasResumeScene && !ReadFString(O,TEXT("resume_scene"),D.ResumeScene)) return false;
    D.bHasResumeIndex=Has(O,TEXT("resume_index"));
    if (D.bHasResumeIndex && !Readint32(O,TEXT("resume_index"),D.ResumeIndex)) return false;
    return KeysSubset(O, {TEXT("action"),TEXT("path"),TEXT("id"),TEXT("start_index"),TEXT("resume_scene"),TEXT("resume_index")});
}
Obj WriteAction(const FMemoriaNarrativeAction& D)
{
    auto O=MakeShared<FJsonObject>();
    if (D.bHasAction) O->SetStringField(TEXT("action"),D.Action);
    if (D.bHasPath) O->SetStringField(TEXT("path"),D.Path);
    if (D.bHasId) O->SetStringField(TEXT("id"),D.Id);
    if (D.bHasStartIndex) O->SetNumberField(TEXT("start_index"),D.StartIndex);
    if (D.bHasResumeScene) O->SetStringField(TEXT("resume_scene"),D.ResumeScene);
    if (D.bHasResumeIndex) O->SetNumberField(TEXT("resume_index"),D.ResumeIndex);
    return O;
}
bool ReadMetadata(const Obj& O, FMemoriaNarrativeMetadata& D)
{
    if (!O) return false;
    D.bHasTitle=Has(O,TEXT("title"));
    if (D.bHasTitle && !ReadFString(O,TEXT("title"),D.Title)) return false;
    D.bHasTitleKo=Has(O,TEXT("title_ko"));
    if (D.bHasTitleKo && !ReadFString(O,TEXT("title_ko"),D.TitleKo)) return false;
    D.bHasChapter=Has(O,TEXT("chapter"));
    if (D.bHasChapter && !Readint32(O,TEXT("chapter"),D.Chapter)) return false;
    D.bHasBgm=Has(O,TEXT("bgm"));
    if (D.bHasBgm && !ReadFString(O,TEXT("bgm"),D.Bgm)) return false;
    return KeysSubset(O, {TEXT("title"),TEXT("title_ko"),TEXT("chapter"),TEXT("bgm")});
}
Obj WriteMetadata(const FMemoriaNarrativeMetadata& D)
{
    auto O=MakeShared<FJsonObject>();
    if (D.bHasTitle) O->SetStringField(TEXT("title"),D.Title);
    if (D.bHasTitleKo) O->SetStringField(TEXT("title_ko"),D.TitleKo);
    if (D.bHasChapter) O->SetNumberField(TEXT("chapter"),D.Chapter);
    if (D.bHasBgm) O->SetStringField(TEXT("bgm"),D.Bgm);
    return O;
}
