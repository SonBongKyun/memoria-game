#include "Chapter/MemoriaChapterMap.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "MemoriaChapterMapSources.inl"
namespace
{
using Obj = TSharedPtr<FJsonObject>;
FVector2D Vec(const TArray<TSharedPtr<FJsonValue>>& A) { return A.Num() >= 2 ? FVector2D(A[0]->AsNumber(), A[1]->AsNumber()) : FVector2D::ZeroVector; }
FVector2D VecField(const Obj& O, const TCHAR* Key) { const TArray<TSharedPtr<FJsonValue>>* A; return O->TryGetArrayField(Key, A) ? Vec(*A) : FVector2D::ZeroVector; }
FLinearColor Col(const Obj& O, const TCHAR* Key, const FLinearColor& Default)
{
    const TArray<TSharedPtr<FJsonValue>>* A;
    if (!O->TryGetArrayField(Key, A) || A->Num() < 3) return Default;
    return FLinearColor((*A)[0]->AsNumber(), (*A)[1]->AsNumber(), (*A)[2]->AsNumber(), A->Num() > 3 ? (*A)[3]->AsNumber() : 1.0);
}
FString Str(const Obj& O, const TCHAR* Key) { FString S; if (O) O->TryGetStringField(Key, S); return S; }
TArray<FString> Strings(const Obj& O, const TCHAR* Key)
{
    TArray<FString> Out; const TArray<TSharedPtr<FJsonValue>>* A;
    if (O->TryGetArrayField(Key, A)) for (const auto& V : *A) Out.Add(V->AsString());
    return Out;
}
FMemoriaChapterRect Rect(const Obj& O)
{
    FMemoriaChapterRect R;
    if (O->HasField(TEXT("origin"))) { R.Origin = VecField(O, TEXT("origin")); R.Size = VecField(O, TEXT("size")); }
    else { R.Size = VecField(O, TEXT("size")); R.Origin = VecField(O, TEXT("center")) - R.Size * .5; }
    return R;
}
FMemoriaChapterMapSpec Parse(const TCHAR* Json)
{
    FMemoriaChapterMapSpec S; Obj O;
    if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), O) || !O) return S;
    S.Map = Str(O, TEXT("map")); S.AssetPrefix = Str(O, TEXT("asset_prefix")); S.DialogueFile = Str(O, TEXT("dialogue_file")); S.Source = Str(O, TEXT("source"));
    S.Chapter = O->GetIntegerField(TEXT("chapter")); S.TileSize = O->GetIntegerField(TEXT("tile_size"));
    S.Width = O->GetIntegerField(TEXT("width")); S.Height = O->GetIntegerField(TEXT("height"));
    if (const Obj* Title; O->TryGetObjectField(TEXT("title"), Title)) { S.TitleName = Str(*Title, TEXT("name")); S.Subtitle = Str(*Title, TEXT("subtitle")); }
    for (const auto& Row : O->GetArrayField(TEXT("tiles"))) for (const auto& V : Row->AsArray()) S.Tiles.Add(int32(V->AsNumber()));
    for (const auto& D : O->GetArrayField(TEXT("tile_defs")))
    { const Obj T = D->AsObject(); S.TileColors.Add(Col(T, TEXT("color"), FLinearColor::Gray)); S.TileDetails.Add(Str(T, TEXT("detail"))); }
    S.TileNames = Strings(O, TEXT("tile_names"));
    for (const auto& V : O->GetArrayField(TEXT("solid"))) S.Solid.Add(int32(V->AsNumber()));
    if (const Obj* A; O->TryGetObjectField(TEXT("atmosphere"), A))
    {
        S.Hue = Col(*A, TEXT("hue"), FLinearColor::White); S.Light = Col(*A, TEXT("light"), FLinearColor::White);
        (*A)->TryGetNumberField(TEXT("mood"), S.Mood); (*A)->TryGetNumberField(TEXT("brightness"), S.Brightness);
        (*A)->TryGetNumberField(TEXT("saturation"), S.Saturation); S.Splash = Str(*A, TEXT("splash"));
    }
    S.Spawn = VecField(O, TEXT("spawn")); S.EliaRepeat = Str(O, TEXT("elia_repeat"));
    for (const auto& V : O->GetArrayField(TEXT("sequence")))
    {
        const Obj Q = V->AsObject(); FMemoriaChapterStep Step;
        Step.Flag = Str(Q, TEXT("flag")); Step.Group = Str(Q, TEXT("group")); Step.Flags = Strings(Q, TEXT("flags")); Step.Toasts = Strings(Q, TEXT("toasts"));
        S.Sequence.Add(Step);
    }
    if (const Obj* E; O->TryGetObjectField(TEXT("exit"), E))
    {
        S.Exit.Rect = Rect(*E); S.Exit.Requires = Str(*E, TEXT("requires")); S.Exit.Completes = Str(*E, TEXT("completes"));
        S.Exit.Group = Str(*E, TEXT("group")); S.Exit.NextMap = Str(*E, TEXT("next_map")); S.Exit.NextChapter = (*E)->GetIntegerField(TEXT("next_chapter"));
    }
    for (const auto& V : O->GetArrayField(TEXT("triggers")))
    {
        const Obj T = V->AsObject(); FMemoriaChapterTrigger Trigger;
        Trigger.Rect = Rect(T->GetObjectField(TEXT("rect"))); Trigger.Group = Str(T, TEXT("group")); Trigger.Flag = Str(T, TEXT("flag")); Trigger.Gate = Str(T, TEXT("gate"));
        S.Triggers.Add(Trigger);
    }
    S.ObjectsGate = Str(O, TEXT("objects_gate")); S.BattlesGate = Str(O, TEXT("battles_gate")); S.EncountersGate = Str(O, TEXT("encounters_gate"));
    for (const auto& V : O->GetArrayField(TEXT("chests")))
    {
        const Obj C = V->AsObject(); FMemoriaChapterChest Chest;
        Chest.Origin = VecField(C, TEXT("origin")); Chest.Flag = Str(C, TEXT("flag"));
        const Obj R = C->GetObjectField(TEXT("rewards")); double Grains = 0; R->TryGetNumberField(TEXT("grains"), Grains); Chest.Grains = int64(Grains);
        if (const Obj* Items; R->TryGetObjectField(TEXT("items"), Items)) for (const auto& Pair : (*Items)->Values) Chest.Items.Emplace(Pair.Key, int64(Pair.Value->AsNumber()));
        S.Chests.Add(Chest);
    }
    for (const auto& V : O->GetArrayField(TEXT("clues")))
    { const Obj C = V->AsObject(); S.Clues.Add({VecField(C, TEXT("origin")), Str(C, TEXT("flag")), Str(C, TEXT("text"))}); }
    for (const auto& V : O->GetArrayField(TEXT("battles")))
    {
        const Obj B = V->AsObject(); FMemoriaChapterBattle Battle;
        Battle.Rect = Rect(B->GetObjectField(TEXT("rect"))); Battle.Name = Str(B, TEXT("name"));
        Battle.Hp = B->GetIntegerField(TEXT("hp")); Battle.Atk = B->GetIntegerField(TEXT("atk")); Battle.bVoid = B->GetBoolField(TEXT("is_void"));
        S.Battles.Add(Battle);
    }
    return S;
}
FString Pascal(const FString& Snake)
{
    TArray<FString> Parts; Snake.ParseIntoArray(Parts, TEXT("_"));
    FString Out; for (const FString& P : Parts) Out += P.Left(1).ToUpper() + P.Mid(1);
    return Out;
}
}
namespace MemoriaChapterMaps
{
const FMemoriaChapterMapSpec* Find(const FString& Map)
{
    static TMap<FString, FMemoriaChapterMapSpec> Specs = []
    {
        TMap<FString, FMemoriaChapterMapSpec> Out;
        for (const auto& Source : MemoriaChapterMapSources) Out.Add(Source.Key, Parse(Source.Value));
        return Out;
    }();
    const FMemoriaChapterMapSpec* Spec = Specs.Find(Map);
    return Spec && !Spec->Map.IsEmpty() && Spec->Tiles.Num() == Spec->Width * Spec->Height ? Spec : nullptr;
}
FString GroupAsset(const FMemoriaChapterMapSpec& Spec, const FString& Group) { return TEXT("DA_Field_") + Spec.AssetPrefix + Pascal(Group); }
FString LevelPath(const FString& Map) { return TEXT("/Game/Memoria/Maps/L_") + Pascal(Map); }
FString MapFromLevel(const FString& LevelName)
{
    for (const auto& Source : MemoriaChapterMapSources)
        if (LevelName.EndsWith(TEXT("L_") + Pascal(Source.Key))) return Source.Key;
    return FString();
}
}
