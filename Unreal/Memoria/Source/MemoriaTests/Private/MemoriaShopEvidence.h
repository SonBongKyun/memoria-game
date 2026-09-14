#pragma once
#include "Shop/MemoriaShopSubsystem.h"
#include "MemoriaPotionEvidence.h"
namespace MemoriaShopEvidence
{
inline MemoriaPotionEvidence::Obj View(const FMemoriaShopView& V)
{
    auto O=MakeShared<FJsonObject>();
    O->SetBoolField(TEXT("open"),V.bOpen);O->SetStringField(TEXT("merchant"),V.Merchant);O->SetStringField(TEXT("mode"),V.Mode);
    O->SetStringField(TEXT("title"),V.Title);O->SetStringField(TEXT("caption"),V.Caption);O->SetStringField(TEXT("grains_text"),V.GrainsText);
    O->SetStringField(TEXT("detail_title"),V.EmptyDetail);O->SetStringField(TEXT("portrait"),V.PortraitSource);
    TArray<TSharedPtr<FJsonValue>> Rows,Stock,Events;
    for(const auto& R:V.Rows)
    {
        auto X=MakeShared<FJsonObject>();X->SetStringField(TEXT("id"),R.Id);X->SetStringField(TEXT("title"),R.Title);X->SetStringField(TEXT("description"),R.Description);
        X->SetStringField(TEXT("story_effect"),R.StoryEffect);X->SetNumberField(TEXT("grade"),R.Grade);X->SetNumberField(TEXT("price"),R.Price);Rows.Add(MakeShared<FJsonValueObject>(X));
    }
    for(const auto& R:V.Stock)
    {
        auto X=MakeShared<FJsonObject>();X->SetStringField(TEXT("id"),R.Id);X->SetStringField(TEXT("title"),R.Title);X->SetStringField(TEXT("description"),R.Description);
        if(!R.StoryEffect.IsEmpty())X->SetStringField(TEXT("story_effect"),R.StoryEffect);if(!R.RelatedNpc.IsEmpty())X->SetStringField(TEXT("related_npc"),R.RelatedNpc);
        X->SetNumberField(TEXT("grade"),R.Grade);X->SetNumberField(TEXT("burn_power"),R.BurnPower);X->SetNumberField(TEXT("price"),R.Price);Stock.Add(MakeShared<FJsonValueObject>(X));
    }
    for(const auto& E:V.Requests)Events.Add(MakeShared<FJsonValueString>(E));
    O->SetArrayField(TEXT("rows"),Rows);O->SetArrayField(TEXT("stock"),Stock);O->SetArrayField(TEXT("events"),Events);return O;
}
}
